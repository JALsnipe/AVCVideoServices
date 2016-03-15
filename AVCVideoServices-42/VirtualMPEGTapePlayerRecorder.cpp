/*
	File:		VirtualMPEGTapePlayerRecorder.cpp
 
 Synopsis: This is the source file for the VirtualMPEGTapePlayerRecorder class.
 
	Copyright: 	© Copyright 2001-2003 Apple Computer, Inc. All rights reserved.
 
	Written by: ayanowitz
 
 Disclaimer:	IMPORTANT:  This Apple software is supplied to you by Apple Computer, Inc.
 ("Apple") in consideration of your agreement to the following terms, and your
 use, installation, modification or redistribution of this Apple software
 constitutes acceptance of these terms.  If you do not agree with these terms,
 please do not use, install, modify or redistribute this Apple software.
 
 In consideration of your agreement to abide by the following terms, and subject
 to these terms, Apple grants you a personal, non-exclusive license, under Apple’s
 copyrights in this original Apple software (the "Apple Software"), to use,
 reproduce, modify and redistribute the Apple Software, with or without
 modifications, in source and/or binary forms; provided that if you redistribute
 the Apple Software in its entirety and without modifications, you must retain
 this notice and the following text and disclaimers in all such redistributions of
 the Apple Software.  Neither the name, trademarks, service marks or logos of
 Apple Computer, Inc. may be used to endorse or promote products derived from the
 Apple Software without specific prior written permission from Apple.  Except as
 expressly stated in this notice, no other rights or licenses, express or implied,
 are granted by Apple herein, including but not limited to any patent rights that
 may be infringed by your derivative works or by other works in which the Apple
 Software may be incorporated.
 
 The Apple Software is provided by Apple on an "AS IS" basis.  APPLE MAKES NO
 WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED
 WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND OPERATION ALONE OR IN
 COMBINATION WITH YOUR PRODUCTS.
 
 IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR
 CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION
 OF THE APPLE SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT, TORT
 (INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN
 ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 
 */

#include "AVCVideoServices.h"

namespace AVS
{
			
//////////////////////////////////////////////////////
// VirtualMPEGTapePlayerRecorder constructor
//////////////////////////////////////////////////////
VirtualMPEGTapePlayerRecorder::VirtualMPEGTapePlayerRecorder()
{
	flushCnt = 0;
	flushMode = false;
	loopMode = false;
	broadcastStreamingMode = false;
	pFileName = nil;
	pRecordingPath = nil;
	writeProtectEnabled = false;
	pTapeSubunit = nil;
	pTransmitter = nil;
	pReceiver = nil;
	pWriter = nil;
	pReader = nil;
	overrunCount = 0;
	currentFrameOffset = 0;
	currentFrameRate = MPEGFrameRate_Unknown;
	currentTSFileLengthInFrames = 0;

	repositionRequested = false;
	repositionFrame = 0;
	
	transmitterBroadcastIsochChannel = 63;
	receiverBroadcastIsochChannel = 63;
	
	// Initialize the transport control mutex
	pthread_mutex_init(&transportControlMutex,NULL);
}

//////////////////////////////////////////////////////
// VirtualMPEGTapePlayerRecorder destructor
//////////////////////////////////////////////////////
VirtualMPEGTapePlayerRecorder::~VirtualMPEGTapePlayerRecorder()
{
	// If the transmitter still exists, delete it here
	if (pTransmitter)
		DestroyMPEG2Transmitter(pTransmitter);

	// If the receiver still exists, delete it here
	if (pReceiver)
		DestroyMPEG2Receiver(pReceiver);

	if (pWriter)
		delete pWriter;
	
	if (pReader)
		delete pReader;
	
	// Delete the virtual tape subunit
	if (pTapeSubunit)
		DestroyVirtualTapeSubunit(pTapeSubunit);
	
	if (pRecordingPath)
		delete [] pRecordingPath;
	
	if (pFileName)
		delete [] pFileName;
	
	// Release the transport control mutex
	pthread_mutex_destroy(&transportControlMutex);
}

//////////////////////////////////////////////////////
// VirtualMPEGTapePlayerRecorder::initWithFileName
//////////////////////////////////////////////////////
IOReturn VirtualMPEGTapePlayerRecorder::initWithFileName(char* pMpegTSFileName, AVCDevice *pAVCDevice)
{
	// Local Vars
    IOReturn result ;
	UInt32 horizontalResolution;
	UInt32 verticalResolution;
	UInt32 bitRate;
	UInt32 numTSPackets;
	
	// Prevent init after already inited
	if (pTapeSubunit)
		return kIOReturnNotPermitted;
		
	// If we are being initialized with a non-nil filename, open the 
	// file and get the stream info
	if (pMpegTSFileName != nil)
	{
		// Save the file name
		pFileName = new char[strlen(pMpegTSFileName)+1];
		if (pFileName)
			strcpy(pFileName,pMpegTSFileName);
			
		// Create a navi based player for the file, and determine it's frame-rate
		pReader = new MPEGNaviFileReader;
		if (!pReader)
			return kIOReturnNoMemory;
		
		// Open a ts file, and a navi file	
		result = pReader->InitWithTSFile(pFileName,false);
		if (result != kIOReturnSuccess)
		{
			delete pReader;
			pReader = nil;
			return kIOReturnError;
		}
		else
		{
			readerHasNaviFile = pReader->hasNaviFile;
		}

		// Get the stream info
		pReader->GetStreamInfo(&horizontalResolution, &verticalResolution, &currentFrameRate, &bitRate, &currentTSFileLengthInFrames, &numTSPackets);
		
		// Delete the file reader (we don't need it for now)
		delete pReader;
		pReader = nil;
	}
	else
	{
		currentFrameRate = MPEGFrameRate_Unknown;
		currentTSFileLengthInFrames = 0;
	}

	// Set the current time code to 0
	currentFrameOffset = 0;
	
	// Create the tape subunit
	result = CreateVirtualTapeSubunit(&pTapeSubunit,
									  kAVCTapeSigModeDVHS,
									  kDVHSTape,
									  VirtualMPEGTapePlayerRecorder::MyVirtualTapeCMPConnectionHandler,
									  VirtualMPEGTapePlayerRecorder::MyVirtualTapeTransportStateChangeHandler, 
									  VirtualMPEGTapePlayerRecorder::MyVirtualTapeSignalModeChangeHandler,
									  VirtualMPEGTapePlayerRecorder::MyVirtualTapeTimeCodeRepositionHandler,
									  this,
									  pAVCDevice);
	
	if (pTapeSubunit)
	{
		pTapeSubunit->setTimeCodeFrameRate(convertMPEGFrameRateToTapeSubunitFrameRate(currentFrameRate));
	}
	
	return result;
}

///////////////////////////////////////////////////////////////////////////
// VirtualMPEGTapePlayerRecorder::setLoopModeState
///////////////////////////////////////////////////////////////////////////
void VirtualMPEGTapePlayerRecorder::setLoopModeState(bool enableLoopMode)
{
	loopMode = enableLoopMode;
}

///////////////////////////////////////////////////////////////////////////
// VirtualMPEGTapePlayerRecorder::restartTransmit
///////////////////////////////////////////////////////////////////////////
void  VirtualMPEGTapePlayerRecorder::restartTransmit(void)
{
	// TODO: Could do this a bit better.
	repositionPlayer(0);
}

///////////////////////////////////////////////////////////////////////////
// VirtualMPEGTapePlayerRecorder::repositionPlayer
///////////////////////////////////////////////////////////////////////////
IOReturn VirtualMPEGTapePlayerRecorder::repositionPlayer(UInt32 targetFrame)
{	
	if (pReader)
	{
		repositionRequested = true;
		repositionFrame = targetFrame;
		return kIOReturnSuccess;
	}
	else
	{
		if (targetFrame < currentTSFileLengthInFrames)
		{
			currentFrameOffset = targetFrame;
			pTapeSubunit->setTimeCodeFrameCount(currentFrameOffset);
		}
	}
	
	return kIOReturnError;
}

///////////////////////////////////////////////////////////////////////////
// VirtualMPEGTapePlayerRecorder::isRepositionPending
///////////////////////////////////////////////////////////////////////////
bool VirtualMPEGTapePlayerRecorder::isRepositionPending(void)
{
	return repositionRequested;
}

///////////////////////////////////////////////////////////////////////////
// VirtualMPEGTapePlayerRecorder::getTapeSubunitTimeCodeFrameCount
///////////////////////////////////////////////////////////////////////////
UInt32 VirtualMPEGTapePlayerRecorder::getTapeSubunitTimeCodeFrameCount()
{
	return pTapeSubunit->getTimeCodeFrameCount();
}

///////////////////////////////////////////////////////////////////////////
// VirtualMPEGTapePlayerRecorder::getTapeSubunitTimeCodeFrameCountInHMSF
///////////////////////////////////////////////////////////////////////////
void VirtualMPEGTapePlayerRecorder::getTapeSubunitTimeCodeFrameCountInHMSF(UInt32 *pHours, UInt32 *pMinutes, UInt32 *pSeconds, UInt32 *pFrames)
{
	UInt32 hours,minutes,seconds,frames;
	
	pTapeSubunit->getTimeCodeFrameCountInHMSF(&hours,&minutes,&seconds,&frames);
	
	*pHours = hours;
	*pMinutes = minutes;
	*pSeconds = seconds;
	*pFrames = frames;
}

///////////////////////////////////////////////////////////////////////////
// VirtualMPEGTapePlayerRecorder::getTransportState
///////////////////////////////////////////////////////////////////////////
void VirtualMPEGTapePlayerRecorder::getTransportState(UInt8 *pCurrentTransportMode, UInt8 *pCurrentTransportState, bool *pIsStable)
{
	pTapeSubunit->getTransportState(pCurrentTransportMode, pCurrentTransportState, pIsStable);
}

///////////////////////////////////////////////////////////////////////////
// VirtualMPEGTapePlayerRecorder::setRecordInhibit
///////////////////////////////////////////////////////////////////////////
void VirtualMPEGTapePlayerRecorder::setRecordInhibit(bool isWriteProtected)
{
	if (isWriteProtected == true)
	{
		writeProtectEnabled = true;
		pTapeSubunit->setMediumInfo(kDVHSTape_WriteProtect);
	}
	else
	{
		writeProtectEnabled = false;
		pTapeSubunit->setMediumInfo(kDVHSTape);
	}
}

///////////////////////////////////////////////////////////////////////////
// VirtualMPEGTapePlayerRecorder::isRecordInhibited
///////////////////////////////////////////////////////////////////////////
bool VirtualMPEGTapePlayerRecorder::isRecordInhibited(void)
{
	return writeProtectEnabled;
}

///////////////////////////////////////////////////////////////////////////
// VirtualMPEGTapePlayerRecorder::isNaviFileEnabled
///////////////////////////////////////////////////////////////////////////
bool VirtualMPEGTapePlayerRecorder::isNaviFileEnabled(void)
{
	return readerHasNaviFile;
}

///////////////////////////////////////////////////////////////////////////
// VirtualMPEGTapePlayerRecorder::getOverrunCount
///////////////////////////////////////////////////////////////////////////
UInt32 VirtualMPEGTapePlayerRecorder::getOverrunCount(void)
{
	return overrunCount;
}

#define kRecordFilePartialFileNameLength 60

///////////////////////////////////////////////////////////////////////////
// VirtualMPEGTapePlayerRecorder::MyVirtualTapeTransportStateChangeHandler
///////////////////////////////////////////////////////////////////////////
IOReturn VirtualMPEGTapePlayerRecorder::MyVirtualTapeTransportStateChangeHandler(void *pRefCon, UInt8 newTransportMode, UInt8 newTransportState)
{
	VirtualMPEGTapePlayerRecorder *pPlayerRecorder = (VirtualMPEGTapePlayerRecorder*) pRefCon;
    IOReturn result = kIOReturnUnsupported ;
	UInt8 currentTransportMode;
	UInt8 currentTransportState;
	bool isStable;
	UInt8 isochChannel;
	UInt8 isochSpeed; 
	UInt8 p2pCount;
	UInt32 horizontalResolution;
	UInt32 verticalResolution;
	UInt32 bitRate;
	UInt32 numTSPackets;
	time_t timer;
	struct tm *date;
	char pPartialFileName[kRecordFilePartialFileNameLength];	
	char timeDateString[kRecordFilePartialFileNameLength-15] ="";
	
	//printf("Transport State Change Requested: Mode=0x%02X, State=0x%02X\n",newTransportMode,newTransportState);

	// Lock the transport control mutex
	pthread_mutex_lock(&pPlayerRecorder->transportControlMutex);
	
	// Get the current transport mode/state
	pPlayerRecorder->pTapeSubunit->getTransportState(&currentTransportMode, &currentTransportState, &isStable);

	// If the new transport mode/state is the same mode/state we're already in, we're done (accept)
	if ((newTransportMode == currentTransportMode) && (newTransportState == currentTransportState))
	{
		if (isStable)
			result = kIOReturnSuccess; // Accept as stable
		else
			result = kIOReturnNotReady; // Accept, and stay in in-transition
		
		// Unlock the transport control mutex
		pthread_mutex_unlock(&pPlayerRecorder->transportControlMutex);

		return result;
	}
	
	// Handle the state change request 
	switch (currentTransportMode)
	{
		//////////////////////////////////////////////////////////////
		//
		// Current mode is Record
		//
		//////////////////////////////////////////////////////////////
		case kAVCTapeTportModeRecord:
			if (newTransportMode == kAVCTapeTportModeRecord)
			{
				if ((currentTransportState == kAVCTapeRecRecord) && (newTransportState == kAVCTapeRecordRecordPause))
				{
					// rec/normal to rec/pause transition
					result = kIOReturnSuccess; // Accept as stable
				}
				else if ((currentTransportState == kAVCTapeRecordRecordPause) && (newTransportState == kAVCTapeRecRecord))
				{
					// rec/pause to rec/normal transition
					result = kIOReturnSuccess; // Accept as stable
				}
			}
			else if (newTransportMode == kAVCTapeTportModeWind)
			{		 
				if (newTransportState == kAVCTapeWindStop)
				{
					// rec/normal (or rec/pause) to wind/stop transition
					if (pPlayerRecorder->pReceiver)
					{
						// Stop the receiver
						pPlayerRecorder->pReceiver->stopReceive();
						
						// Delete the receiver object
						DestroyMPEG2Receiver(pPlayerRecorder->pReceiver);
						pPlayerRecorder->pReceiver = nil;
					}
					if (pPlayerRecorder->pWriter)
					{
						delete pPlayerRecorder->pWriter;
						pPlayerRecorder->pWriter = nil;
					}
					
					// Set the current frame offset to 0
					pPlayerRecorder->currentFrameOffset = 0;

					result = kIOReturnSuccess; // Accept as stable
				}
			}
			break;

		//////////////////////////////////////////////////////////////
		//
		// Current mode is Play
		//
		//////////////////////////////////////////////////////////////
		case kAVCTapeTportModePlay:
			if (newTransportMode == kAVCTapeTportModePlay)
			{
				if ((currentTransportState == kAVCTapePlayFwd) && ((newTransportState == kAVCTapePlayFwdPause) || (newTransportState == kAVCTapePlayRevPause)))
				{
					// play/normal to play/pause transition
					pPlayerRecorder->pTapeSubunit->setTransportState(kAVCTapeTportModePlay, kAVCTapePlayFwdPause, true);
					result = kIOReturnAcceptWithNoAutoStateChange;
				}
				else if ((currentTransportState == kAVCTapePlayFwdPause) && ((newTransportState == kAVCTapePlayFwd) || (newTransportState == kAVCTapePlayX1)))
				{
					// play/pause to play/normal transition
					pPlayerRecorder->pTapeSubunit->setTransportState(kAVCTapeTportModePlay, kAVCTapePlayFwd, true);
					result = kIOReturnAcceptWithNoAutoStateChange;
				}
				else if ((currentTransportState == kAVCTapePlayFwd) && ((newTransportState >= kAVCTapePlayFastFwd1) && (newTransportState <= kAVCTapePlayFastestFwd)))
				{
					// one shot skip forward
					result = kIOReturnNotReady; // Accept as in-transition
				}
				else if ((currentTransportState == kAVCTapePlayFwd) && ((newTransportState >= kAVCTapePlayFastRev1) && (newTransportState <= kAVCTapePlayFastestRev)))
				{
					// one shot skip back
					result = kIOReturnNotReady; // Accept as in-transition
				}
			}
			else if (newTransportMode == kAVCTapeTportModeWind)
			{
				if (newTransportState == kAVCTapeWindStop)
				{
					// play/normal (or play/pause) to wind/stop transition
					if (pPlayerRecorder->pTransmitter)
					{
						// Stop the transmitter
						pPlayerRecorder->pTransmitter->stopTransmit();
						
						// Delete the transmitter object
						DestroyMPEG2Transmitter(pPlayerRecorder->pTransmitter);
						pPlayerRecorder->pTransmitter = nil;
					}
					if (pPlayerRecorder->pReader)
					{
						delete pPlayerRecorder->pReader;
						pPlayerRecorder->pReader = nil;
					}
					result = kIOReturnSuccess; // Accept as stable
				}
			}
			break;

		//////////////////////////////////////////////////////////////
		//
		// Current state is Wind (stop)
		//
		//////////////////////////////////////////////////////////////
		case kAVCTapeTportModeWind:
		default:
			if (newTransportMode == kAVCTapeTportModePlay)
			{
				// Only proceed if we're in wind/stop
				if (currentTransportState != kAVCTapeWindStop)
					break;
				
				// see if the new state is play/pause or play/normal (or something else)
				switch (newTransportState)
				{
					case kAVCTapePlayX1:
					case kAVCTapePlayFwd:
					case kAVCTapePlayRevPause:
					case kAVCTapePlayFwdPause:
						// Make sure we have a play file
						if (pPlayerRecorder->pFileName == nil)
						{
							result = kIOReturnError;
							break;
						}
						
						// Create the ts/navi reader
						pPlayerRecorder->pReader = new MPEGNaviFileReader;
						if (!pPlayerRecorder->pReader)
						{
							result = kIOReturnNoMemory;
							break;
						}
							
						// Open a ts file, and a navi file	
						result = pPlayerRecorder->pReader->InitWithTSFile(pPlayerRecorder->pFileName,false);
						if (result != kIOReturnSuccess)
						{
							delete pPlayerRecorder->pReader;
							pPlayerRecorder->pReader = nil;
							result = kIOReturnError;
							break;
						}
						else
						{
							pPlayerRecorder->readerHasNaviFile = pPlayerRecorder->pReader->hasNaviFile;
						}
						
						// Get the current oPCR params
						pPlayerRecorder->pTapeSubunit->
							getPlugParameters(false, &isochChannel, &isochSpeed, &p2pCount);
							
						// Create the transmitter
						result = CreateMPEG2Transmitter(&pPlayerRecorder->pTransmitter,
														VirtualMPEGTapePlayerRecorder::MpegTransmitCallback,
														pPlayerRecorder,
														VirtualMPEGTapePlayerRecorder::MPEG2TransmitterMessageProc,
														pPlayerRecorder,
														nil,
														pPlayerRecorder->pTapeSubunit->GetNub(),
														kCyclesPerTransmitSegment,
														kNumTransmitSegments,
														(p2pCount > 0) ? false : true,
														3,	// Supports up to 36mbps streams.
														kTSPacketQueueSizeInPackets);
						if (!pPlayerRecorder->pTransmitter)
						{	
							delete pPlayerRecorder->pReader;
							pPlayerRecorder->pReader = nil;
							result = kIOReturnError;
							break;
						}
						else
						{
							// Setup and Start the transmitter
							
							if (p2pCount == 0)
							{
								pPlayerRecorder->broadcastStreamingMode = true;
								
								// Set the channel to transmit on
								pPlayerRecorder->pTransmitter->setTransmitIsochChannel((pPlayerRecorder->transmitterBroadcastIsochChannel & 0x3F));
								
								// Set the isoch packet speed
								pPlayerRecorder->pTransmitter->setTransmitIsochSpeed(kFWSpeed100MBit);
								
								// Set the oPCR to reflect the broadcast channel
								pPlayerRecorder->pTapeSubunit->
									setPlugParameters(false, (pPlayerRecorder->transmitterBroadcastIsochChannel & 0x3F), kFWSpeed100MBit);
							}
							else
							{
								pPlayerRecorder->broadcastStreamingMode = false;

								// Set the channel to transmit on
								pPlayerRecorder->pTransmitter->setTransmitIsochChannel(isochChannel);
								
								// Set the isoch packet speed
								pPlayerRecorder->pTransmitter->setTransmitIsochSpeed((IOFWSpeed)isochSpeed);
							}
							
							// Reposition file pointer to match the current timecode.
							if (pPlayerRecorder->currentFrameOffset != 0)
							{
								result = pPlayerRecorder->pReader->SeekToSpecificFrame(pPlayerRecorder->currentFrameOffset);
								pPlayerRecorder->currentFrameOffset = pPlayerRecorder->pReader->GetCurrentTimeCodePositionInFrames();
								pPlayerRecorder->pTapeSubunit->setTimeCodeFrameCount(pPlayerRecorder->currentFrameOffset);
							}
							
							// Clear any pending reposition
							pPlayerRecorder->repositionRequested = false;
							pPlayerRecorder->repositionFrame = 0;
							
							// Get the stream info (for the frame rate)
							pPlayerRecorder->pReader->GetStreamInfo(&horizontalResolution,
																	&verticalResolution,
																	&pPlayerRecorder->currentFrameRate,
																	&bitRate,
																	&pPlayerRecorder->currentTSFileLengthInFrames,
																	&numTSPackets);
							
							// Set the tape subunit's frame rate based on the current TS file
							pPlayerRecorder->pTapeSubunit->setTimeCodeFrameRate(
								pPlayerRecorder->convertMPEGFrameRateToTapeSubunitFrameRate(pPlayerRecorder->currentFrameRate));
							
							// Start the transmitter
							pPlayerRecorder->overrunCount = -1;	// Starting transmit will bump this by one
							result = pPlayerRecorder->pTransmitter->startTransmit();
							if (result != kIOReturnSuccess)
							{
								delete pPlayerRecorder->pReader;
								pPlayerRecorder->pReader = nil;
								DestroyMPEG2Transmitter(pPlayerRecorder->pTransmitter);
								pPlayerRecorder->pTransmitter = nil;
								result = kIOReturnError;
								break;
							}
							
							// Accept the transition change as stable
							result = kIOReturnSuccess;
						}
						break;
							
					default:
						break;
				};
			}
			else if (newTransportMode == kAVCTapeTportModeRecord)
			{
				// Only proceed if we're in wind/stop
				if (currentTransportState != kAVCTapeWindStop)
					break;
				
				// see if the new state is rec/pause or rec/normal (or something else)
				switch (newTransportState)
				{
					case kAVCTapeRecRecord:
					case kAVCTapeRecordRecordPause:
						if (pPlayerRecorder->writeProtectEnabled == true)
						{
							result = kIOReturnNotWritable;
							break;
						}
						
						// If we already have a file-name string, delete it
						if (pPlayerRecorder->pFileName)
							delete pPlayerRecorder->pFileName;
						
						// Allocate a new file-name string
						if (pPlayerRecorder->pRecordingPath)
						{
							// Make it big enough to hold the path and the generated name
							pPlayerRecorder->pFileName = new char[strlen(pPlayerRecorder->pRecordingPath) + kRecordFilePartialFileNameLength]; 
						}
						else
						{
							// Make it big enough to hold just the generated name
							pPlayerRecorder->pFileName = new char[kRecordFilePartialFileNameLength]; // Slightly overallocate
						}
						if (!pPlayerRecorder->pFileName)
						{
							result = kIOReturnNoMemory;
							break;
						}
						
						// Generate the new partial file-name with the C-string containing the formatted date/time
						strcpy(pPartialFileName,"Capture");

						timer = time(NULL);
						date = localtime(&timer);
						strftime(timeDateString, sizeof(timeDateString),"_%b_%d_%Y__%I_%M_%S_%p", date);
						strncat(pPartialFileName, timeDateString, sizeof(timeDateString));
							
						strcat(pPartialFileName,".m2t");
						
						// If we have a specified recording path, use it in the new filename
						if (pPlayerRecorder->pRecordingPath)
						{
							strcpy(pPlayerRecorder->pFileName, pPlayerRecorder->pRecordingPath);
							strcat(pPlayerRecorder->pFileName,"/");
							strcat(pPlayerRecorder->pFileName, pPartialFileName);
						}
						else
						{
							strcpy(pPlayerRecorder->pFileName, pPartialFileName);
						}

						// Create the ts/navi writer
						pPlayerRecorder->pWriter = new MPEGNaviFileWriter;
						if (!pPlayerRecorder->pWriter)
						{
							result = kIOReturnNoMemory;
							break;
						}
							
						// Initialize the file writer
						result = pPlayerRecorder->pWriter->InitWithTSFile(pPlayerRecorder->pFileName,true);
						if (result != kIOReturnSuccess)
						{
							delete pPlayerRecorder->pWriter;
							pPlayerRecorder->pWriter = nil;
							result = kIOReturnError;
							break;
						}
						else
						{
							pPlayerRecorder->readerHasNaviFile = true;
						}
							
						// Get the current iPCR params
						pPlayerRecorder->pTapeSubunit->
							getPlugParameters(true, &isochChannel, &isochSpeed, &p2pCount);

						// Create the receiver
						result = CreateMPEG2Receiver(&pPlayerRecorder->pReceiver,
													 nil,	// We'll register a extended callback, below.
													 pPlayerRecorder,
													 VirtualMPEGTapePlayerRecorder::MPEG2ReceiverMessageProc,
													 pPlayerRecorder,
													 nil,
													 pPlayerRecorder->pTapeSubunit->GetNub(),
													 kCyclesPerReceiveSegment,
													 kNumReceiveSegments,
													 false);
						if (!pPlayerRecorder->pReceiver)
						{	
							delete pPlayerRecorder->pWriter;
							pPlayerRecorder->pWriter = nil;
							result = kIOReturnError;
							break;
						}
						else
						{
							// Register the extneded MPEG2Recceiver callback
							pPlayerRecorder->pReceiver->registerExtendedDataPushCallback(VirtualMPEGTapePlayerRecorder::MpegReceiveCallback, pPlayerRecorder);
							
							// Setup and Start the transmitter
							
							if (p2pCount == 0)
							{
								pPlayerRecorder->broadcastStreamingMode = true;
								
								// Set the channel to transmit on
								pPlayerRecorder->pReceiver->setReceiveIsochChannel((pPlayerRecorder->receiverBroadcastIsochChannel & 0x3F));
								
								// Set the isoch packet speed
								pPlayerRecorder->pReceiver->setReceiveIsochSpeed(kFWSpeed100MBit);
								
								// Set the iPCR to to reflect the braodcast channel
								pPlayerRecorder->pTapeSubunit->
									setPlugParameters(true, (pPlayerRecorder->receiverBroadcastIsochChannel & 0x3F), kFWSpeed100MBit);
							}
							else
							{
								pPlayerRecorder->broadcastStreamingMode = false;
								
								// Set the channel to transmit on
								pPlayerRecorder->pReceiver->setReceiveIsochChannel(isochChannel);
								
								// Set the isoch packet speed
								pPlayerRecorder->pReceiver->setReceiveIsochSpeed((IOFWSpeed)isochSpeed);
							}

							// Initialize the time-code, frame-rate, etc.
							pPlayerRecorder->currentFrameOffset = 0;
							pPlayerRecorder->currentTSFileLengthInFrames = 0;
							pPlayerRecorder->currentFrameRate = MPEGFrameRate_Unknown;
							pPlayerRecorder->pTapeSubunit->setTimeCodeFrameCount(pPlayerRecorder->currentFrameOffset);
							pPlayerRecorder->pTapeSubunit->setTimeCodeFrameRate(
								pPlayerRecorder->convertMPEGFrameRateToTapeSubunitFrameRate(pPlayerRecorder->currentFrameRate));

							// Start the receiver
							pPlayerRecorder->overrunCount = 0;
							result = pPlayerRecorder->pReceiver->startReceive();
							if (result != kIOReturnSuccess)
							{
								DestroyMPEG2Receiver(pPlayerRecorder->pReceiver);
								pPlayerRecorder->pReceiver = nil;
								delete pPlayerRecorder->pWriter;
								pPlayerRecorder->pWriter = nil;
								result = kIOReturnError;
								break;
							}
							
							// Accept the transition change as stable
							result = kIOReturnSuccess;
						}		
						break;
						
					default:
						break;
				};
			}
			else if (newTransportMode == kAVCTapeTportModeWind)
			{
				if ((newTransportState == kAVCTapeWindRew) || (newTransportState == kAVCTapeWindHighSpdRew))
				{
					// TODO: stop to rewind (as one-shot. Note that no file is currently open, so this only adjusts our concept of current frame. Auto transition back to wind/stop)
				}
				else if (newTransportState == kAVCTapeWindFastFwd)
				{
					// TODO: stop to fast-fwd (as one-shot. Note that no file is currently open, so this only adjusts our concept of current frame. Auto transition back to wind/stop)
				}
			}
			break;
	};
	
	// Unlock the transport control mutex
	pthread_mutex_unlock(&pPlayerRecorder->transportControlMutex);

	return result;
}

///////////////////////////////////////////////////////////////////////////
// VirtualMPEGTapePlayerRecorder::MyVirtualTapeSignalModeChangeHandler
///////////////////////////////////////////////////////////////////////////
IOReturn VirtualMPEGTapePlayerRecorder::MyVirtualTapeSignalModeChangeHandler(void *pRefCon, UInt8 newSignalMode)
{
	//printf("Signal Mode Change Requested: Mode=0x%02X\n",newSignalMode);
	
	// This app, doesn't allow signal mode changes
	// Returning and error here will cause the signal modecontrol command to be rejected 
	return kIOReturnUnsupported;	
}

///////////////////////////////////////////////////////////////////////////
// VirtualMPEGTapePlayerRecorder::MyVirtualTapeTimeCodeRepositionHandler
///////////////////////////////////////////////////////////////////////////
IOReturn VirtualMPEGTapePlayerRecorder::MyVirtualTapeTimeCodeRepositionHandler(void *pRefCon, UInt32 newTimeCode)
{
	//printf("Signal Time Code Reposition Requested: Frame=0x%08X\n",(unsigned int) newTimeCode);
	
	// Time-code reposition command not supported, for now!
	// Returning and error here will cause the time-code reposition control command to be rejected 
	return kIOReturnUnsupported;
}

///////////////////////////////////////////////////////////////////////////
// VirtualMPEGTapePlayerRecorder::MyVirtualTapeCMPConnectionHandler
///////////////////////////////////////////////////////////////////////////
void VirtualMPEGTapePlayerRecorder::MyVirtualTapeCMPConnectionHandler(void *pRefCon, bool isInputPlug, UInt8 isochChannel, UInt8 isochSpeed, UInt8 p2pCount)
{
	// Local Vars
    IOReturn result = kIOReturnSuccess ;
	VirtualMPEGTapePlayerRecorder *pPlayerRecorder = (VirtualMPEGTapePlayerRecorder*) pRefCon;
	
	//printf("CMP Notification for %s Plug p2p=0x%02X Channel=0x%02X Speed=0x%02X\n",((isInputPlug == true) ? "Input" : "Output"), p2pCount, isochChannel, isochSpeed);
	
	
	if (isInputPlug == true)
	{
		// If we are currently receiving in broadcast mode (meaning we didn't have a p2p connection
		// when we started the receiver, and we started the receiver with doIRM=true), and now p2pCount
		// is greater than 0, then stop the receiver, delete the receiver, create a new receiver with
		// doIRM=false, set the proper isoch channel and speed, set broadcastStreamingMode to false
		// and start the new receiver.
		if ((pPlayerRecorder->pReceiver != nil) && (pPlayerRecorder->broadcastStreamingMode == true) && (p2pCount > 0))
		{
			pPlayerRecorder->pReceiver->stopReceive();
			DestroyMPEG2Receiver(pPlayerRecorder->pReceiver);
			pPlayerRecorder->pReceiver = nil;
			result = CreateMPEG2Receiver(&pPlayerRecorder->pReceiver,
										 nil,	// We'll register a extended callback, below.
										 pPlayerRecorder,
										 VirtualMPEGTapePlayerRecorder::MPEG2ReceiverMessageProc,
										 pPlayerRecorder,
										 nil,
										 pPlayerRecorder->pTapeSubunit->GetNub(),
										 kCyclesPerReceiveSegment,
										 kNumReceiveSegments,
										 false);
			if (!pPlayerRecorder->pReceiver)
			{
				// Couldn't create the new receiver, so delete the file writer, and stop the transport
				if (pPlayerRecorder->pWriter)
				{
					delete pPlayerRecorder->pWriter;
					pPlayerRecorder->pWriter = nil;
				}
				pPlayerRecorder->currentFrameOffset = 0;
				pPlayerRecorder->pTapeSubunit->setTransportState(kAVCTapeTportModeWind, kAVCTapeWindStop, true);
				return;
			}
			else
			{
				// Register the extneded MPEG2Recceiver callback
				pPlayerRecorder->pReceiver->registerExtendedDataPushCallback(VirtualMPEGTapePlayerRecorder::MpegReceiveCallback, pPlayerRecorder);
				
				pPlayerRecorder->pReceiver->setReceiveIsochChannel(isochChannel);
				pPlayerRecorder->pReceiver->setReceiveIsochSpeed((IOFWSpeed)isochSpeed);
				pPlayerRecorder->broadcastStreamingMode = false;
				pPlayerRecorder->overrunCount = 0;
				pPlayerRecorder->pReceiver->startReceive();
				return;
			}
		}
		
		// If we are currently receiving in p2p mode, and the p2p count is now zero, we need to stop 
		// receiving, and set the tape transport to stop
		if ((pPlayerRecorder->pReceiver != nil) && (pPlayerRecorder->broadcastStreamingMode == false) && (p2pCount == 0))
		{
			pPlayerRecorder->tapeTransportStateChange(kAVCTapeTportModeWind, kAVCTapeWindStop);
			return;
		}
	}
	else
	{
		// If we are currently transmitting in broadcast mode (meaning we didn't have a p2p connection
		// when we started the transmitter, and we started the transmitter with doIRM=true), and now p2pCount
		// is greater than 0, then stop the tranmistter, delete the transmitter, create a new transmitter with
		// doIRM=false, set the proper isoch channel and speed, set broadcastStreamingMode to false
		// and start the new transmitter.
		if ((pPlayerRecorder->pTransmitter != nil) && (pPlayerRecorder->broadcastStreamingMode == true) && (p2pCount > 0))
		{
			pPlayerRecorder->pTransmitter->stopTransmit();
			DestroyMPEG2Transmitter(pPlayerRecorder->pTransmitter);
			pPlayerRecorder->pTransmitter = nil;
			result = CreateMPEG2Transmitter(&pPlayerRecorder->pTransmitter,
											VirtualMPEGTapePlayerRecorder::MpegTransmitCallback,
											pPlayerRecorder,
											VirtualMPEGTapePlayerRecorder::MPEG2TransmitterMessageProc,
											pPlayerRecorder,
											nil,
											pPlayerRecorder->pTapeSubunit->GetNub(),
											kCyclesPerTransmitSegment,
											kNumTransmitSegments,
											false,
											3,	// Supports up to 36mbps streams.
											kTSPacketQueueSizeInPackets);
			if (!pPlayerRecorder->pTransmitter)
			{	
				// Couldn't create the new trasnmitter, so delete the file reader, and stop the transport
				if (pPlayerRecorder->pReader)
				{
					delete pPlayerRecorder->pReader;
					pPlayerRecorder->pReader = nil;
				}
				pPlayerRecorder->pTapeSubunit->setTransportState(kAVCTapeTportModeWind, kAVCTapeWindStop, true);
				return;
			}
			else
			{
				pPlayerRecorder->pTransmitter->setTransmitIsochChannel(isochChannel);
				pPlayerRecorder->pTransmitter->setTransmitIsochSpeed((IOFWSpeed)isochSpeed);
				pPlayerRecorder->broadcastStreamingMode = false;
				pPlayerRecorder->overrunCount = -1;	// Starting transmit will bump this by one
				pPlayerRecorder->pTransmitter->startTransmit();
				return;
			}
		}
		
		// If we are currently transmitting in p2p mode, and the p2p count is now zero, we need to stop 
		// transmitting, and set the tape transport to stop
		if ((pPlayerRecorder->pTransmitter != nil) && (pPlayerRecorder->broadcastStreamingMode == false) && (p2pCount == 0))
		{
			pPlayerRecorder->tapeTransportStateChange(kAVCTapeTportModeWind, kAVCTapeWindStop);
			return;
		}
	}
	
	return;
}

///////////////////////////////////////////////////////////////////////////
// VirtualMPEGTapePlayerRecorder::getStreamInformation
///////////////////////////////////////////////////////////////////////////
void VirtualMPEGTapePlayerRecorder::getStreamInformation(UInt32 *pFrameHorizontalSize,
														 UInt32 *pFrameVerticalSize,
														 UInt32 *pBitRate,
														 MPEGFrameRate *pFrameRate,
														 UInt32 *pNumFrames,
														 UInt32 *pNumTSPackets,
														 double *pCurrentMPEGDataRate)
{
	UInt32 horizontalResolution; 
	UInt32 verticalResolution; 
	MPEGFrameRate frameRate; 
	UInt32 bitRate;
	UInt32 numFrames;
	UInt32 numTSPackets;
	
	if (pWriter)
	{
		pWriter->GetStreamInfo(&horizontalResolution, &verticalResolution, &frameRate, &bitRate, &numFrames, &numTSPackets);
		*pFrameHorizontalSize = horizontalResolution;
		*pFrameVerticalSize = verticalResolution;
		*pBitRate = bitRate;
		*pNumTSPackets = numTSPackets;
		if (pReceiver)
			*pCurrentMPEGDataRate = pReceiver->mpegDataRate;
		else
			*pCurrentMPEGDataRate = 0;
	}
	else if (pReader)
	{
		pReader->GetStreamInfo(&horizontalResolution, &verticalResolution, &frameRate, &bitRate, &numFrames, &numTSPackets);
		*pFrameHorizontalSize = horizontalResolution;
		*pFrameVerticalSize = verticalResolution;
		*pBitRate = bitRate;
		*pNumTSPackets = numTSPackets;
		if (pTransmitter)
			*pCurrentMPEGDataRate = pTransmitter->mpegDataRate;
		else
			*pCurrentMPEGDataRate = 0;
	}
	else
	{
		*pFrameHorizontalSize = 0;
		*pFrameVerticalSize = 0;
		*pBitRate = 0;
		*pNumTSPackets = 0; // TODO
		*pCurrentMPEGDataRate = 0;
	}

	*pFrameRate = currentFrameRate;
	*pNumFrames = currentTSFileLengthInFrames;
}

///////////////////////////////////////////////////////////////////////////
// VirtualMPEGTapePlayerRecorder::getPlugInformation
///////////////////////////////////////////////////////////////////////////
void VirtualMPEGTapePlayerRecorder::getPlugInformation(UInt32 *pInputPlugConnectionCount,
						UInt32 *pInputPlugChannel,
						UInt32 *pOutputPlugConnectionCount,
						UInt32 *pOutputPlugChannel,
						UInt32 *pOutputPlugSpeed)
{
	UInt8 isochChannel;
	UInt8 isochSpeed;
	UInt8 p2pCount;
	
	// Input Plug
	pTapeSubunit->getPlugParameters(true, &isochChannel, &isochSpeed, &p2pCount);
	*pInputPlugConnectionCount = p2pCount;
	*pInputPlugChannel = isochChannel;
	
	// Output Plug
	pTapeSubunit->getPlugParameters(false, &isochChannel, &isochSpeed, &p2pCount);
	*pOutputPlugConnectionCount = p2pCount; 
	*pOutputPlugChannel = isochChannel;
	*pOutputPlugSpeed = isochSpeed;
}

///////////////////////////////////////////////////////////////////////////
// VirtualMPEGTapePlayerRecorder::tapeTransportStateChange
///////////////////////////////////////////////////////////////////////////
IOReturn VirtualMPEGTapePlayerRecorder::tapeTransportStateChange(UInt8 newTransportMode, UInt8 newTransportState)
{
	IOReturn result;
	
	result = MyVirtualTapeTransportStateChangeHandler(this, newTransportMode, newTransportState);
	if (result == kIOReturnSuccess)
	{
		pTapeSubunit->setTransportState(newTransportMode, newTransportState, true);
	}
	else if (result == kIOReturnNotReady)
	{
		pTapeSubunit->setTransportState(newTransportMode, newTransportState, false);
		result = kIOReturnSuccess;
	}
	else if (result == kIOReturnAcceptWithNoAutoStateChange)
	{
		result = kIOReturnSuccess;	
	}
	return result;
}

///////////////////////////////////////////////////////////////////////////
// VirtualMPEGTapePlayerRecorder::setPlaybackFileName
///////////////////////////////////////////////////////////////////////////
IOReturn VirtualMPEGTapePlayerRecorder::setPlaybackFileName(char* pMpegTSFileName)
{
	// Local Vars
	MPEGNaviFileReader *pTempFileReader;
    IOReturn result ;
	UInt32 horizontalResolution;
	UInt32 verticalResolution;
	UInt32 bitRate;
	UInt32 numTSPackets;
	
	// Make sure we're in wind/stop
	tapeTransportStateChange(kAVCTapeTportModeWind, kAVCTapeWindStop);

	// Set the playback file name
	if (pMpegTSFileName)
	{
		if (pFileName)
			delete [] pFileName;
		
		pFileName = new char[strlen(pMpegTSFileName)+1];
		if (pFileName)
		{
			strcpy(pFileName,pMpegTSFileName);

			// Create a navi based player for the file, and determine it's frame-rate
			pTempFileReader = new MPEGNaviFileReader;
			if (pTempFileReader)
			{
				// Open a ts file, and a navi file	
				result = pTempFileReader->InitWithTSFile(pFileName,false);
				if (result == kIOReturnSuccess)
				{
					pTempFileReader->GetStreamInfo(&horizontalResolution, &verticalResolution, &currentFrameRate, &bitRate, &currentTSFileLengthInFrames, &numTSPackets);
					readerHasNaviFile = pTempFileReader->hasNaviFile;
				}
				else
				{
					currentTSFileLengthInFrames	= 0;
					currentFrameRate = MPEGFrameRate_Unknown;
					readerHasNaviFile = false;
				}
				
				// Delete the file reader (we don't need it anymore)
				delete pTempFileReader;
			}
		}
	}
	
	// Set the current time code to 0
	currentFrameOffset = 0;
	pTapeSubunit->setTimeCodeFrameCount(currentFrameOffset);
	
	// Clear any pending reposition
	repositionRequested = false;
	repositionFrame = 0;

	return kIOReturnSuccess;
}

///////////////////////////////////////////////////////////////////////////
// VirtualMPEGTapePlayerRecorder::getPlaybackFileName
///////////////////////////////////////////////////////////////////////////
char* VirtualMPEGTapePlayerRecorder::getPlaybackFileName(void)
{
	return pFileName;
}

///////////////////////////////////////////////////////////////////////////
// VirtualMPEGTapePlayerRecorder::setRecordFileDirectoryPath
///////////////////////////////////////////////////////////////////////////
void VirtualMPEGTapePlayerRecorder::setRecordFileDirectoryPath(char *pDataPath)
{
	// If we already have a recording path string, delete it
	if (pRecordingPath)
		delete [] pRecordingPath;
	
	// Create a new recording path string, and copy the passed in string.
	pRecordingPath = new char[strlen(pDataPath)+1];
	if (pRecordingPath)
		strcpy(pRecordingPath,pDataPath);
}

///////////////////////////////////////////////////////////////////////////
// VirtualMPEGTapePlayerRecorder::setTransmitterBroadcastIsochChannel
///////////////////////////////////////////////////////////////////////////
void  VirtualMPEGTapePlayerRecorder::setTransmitterBroadcastIsochChannel(UInt32 isochChannel)
{
	transmitterBroadcastIsochChannel = isochChannel;
}

///////////////////////////////////////////////////////////////////////////
// VirtualMPEGTapePlayerRecorder::setReceiverBroadcastIsochChannel
///////////////////////////////////////////////////////////////////////////
void  VirtualMPEGTapePlayerRecorder::setReceiverBroadcastIsochChannel(UInt32 isochChannel)
{
	receiverBroadcastIsochChannel = isochChannel;
}

///////////////////////////////////////////////////////////////////////////
// VirtualMPEGTapePlayerRecorder::convertMPEGFrameRateToTapeSubunitFrameRate
///////////////////////////////////////////////////////////////////////////
TapeTimeCodeFrameRate VirtualMPEGTapePlayerRecorder::convertMPEGFrameRateToTapeSubunitFrameRate(UInt32 mpegFrameRate)
{
	TapeTimeCodeFrameRate retVal;
	
	switch(mpegFrameRate)
	{
		case MPEGFrameRate_23_976:
		case MPEGFrameRate_24:
		case MPEGFrameRate_25:
			retVal = kTapeTimeCodeFrameRate_25fps;
			break;
			
		case MPEGFrameRate_29_97:
			retVal = kTapeTimeCodeFrameRate_29_97fps;
			break;
			
		case MPEGFrameRate_30:
			retVal = kTapeTimeCodeFrameRate_30fps;
			break;

		case MPEGFrameRate_50:
		case MPEGFrameRate_59_94:
		case MPEGFrameRate_60:
			retVal = kTapeTimeCodeFrameRate_60fps;
			break;

		case MPEGFrameRate_Unknown:
		default:
			retVal = kTapeTimeCodeFrameRate_30fps;
			break;
	};
	
	return retVal;
}

///////////////////////////////////////////////////////////////////////////
// VirtualMPEGTapePlayerRecorder::MpegReceiveCallback
///////////////////////////////////////////////////////////////////////////
IOReturn VirtualMPEGTapePlayerRecorder::MpegReceiveCallback(UInt32 tsPacketCount, 
															UInt32 **ppBuf, 
															void *pRefCon,
															UInt32 isochHeader,
															UInt32 cipHeader0,
															UInt32 cipHeader1,
															UInt32 fireWireTimeStamp)
{
	VirtualMPEGTapePlayerRecorder *pPlayerRecorder;
	unsigned int cnt;
	UInt8 currentTransportMode;
	UInt8 currentTransportState;
	bool isStable;
	unsigned int i;
	UInt32 horizontalResolution;
	UInt32 verticalResolution;
	MPEGFrameRate frameRate;
	UInt32 bitRate;
	UInt32 numFrames;
	UInt32 numTSPackets;

#if 0
	UInt32 emi = ((isochHeader & 0x0000000C) >> 2);
	UInt32 oddEven = ((isochHeader & 0x00000002) >> 1);
	UInt32 sourceDeviceNodeNumber = ((cipHeader0 & 0x3F000000) >> 24);
#endif	
	
	pPlayerRecorder = (VirtualMPEGTapePlayerRecorder*) pRefCon;
	
	// If no file writer, return now!
	if (pPlayerRecorder->pWriter == nil)
		return kIOReturnSuccess;

	// Get the current transport state
	pPlayerRecorder->pTapeSubunit->getTransportState(&currentTransportMode, &currentTransportState, &isStable);

	// If we are in rec/pause mode, don't write any TS packets
	if (currentTransportState == kAVCTapeRecordRecordPause)
		return kIOReturnSuccess;
	
	// Write packets to file
	for (i=0;i<tsPacketCount;i++)
	{
		cnt = pPlayerRecorder->pWriter->WriteNextTSPackets(ppBuf[i],1);
		if (cnt != 1)
		{
			// The file writer had a write error, close it now
			// TODO: We cannot change the transport state here becuase stopping
			// the receiver would cause a deadlock. Need to address this problem!
			if (pPlayerRecorder->pWriter)
			{
				delete pPlayerRecorder->pWriter;
				pPlayerRecorder->pWriter = nil;
			}
			return kIOReturnSuccess;
		}
	}

	// Update the current time-code
	pPlayerRecorder->currentTSFileLengthInFrames = pPlayerRecorder->pWriter->GetCurrentTimeCodePositionInFrames();
	pPlayerRecorder->currentFrameOffset = pPlayerRecorder->pWriter->GetCurrentTimeCodePositionInFrames();
	pPlayerRecorder->pTapeSubunit->setTimeCodeFrameCount(pPlayerRecorder->currentFrameOffset);
	if ((pPlayerRecorder->currentFrameRate == MPEGFrameRate_Unknown) && (pPlayerRecorder->currentFrameOffset != 0))
	{
		pPlayerRecorder->pWriter->GetStreamInfo(&horizontalResolution, &verticalResolution, &frameRate, &bitRate, &numFrames, &numTSPackets);
		pPlayerRecorder->currentFrameRate = frameRate;
		pPlayerRecorder->pTapeSubunit->setTimeCodeFrameRate(
			pPlayerRecorder->convertMPEGFrameRateToTapeSubunitFrameRate(pPlayerRecorder->currentFrameRate));
	}
	
	return kIOReturnSuccess;
}

///////////////////////////////////////////////////////////////////////////
// VirtualMPEGTapePlayerRecorder::MPEG2ReceiverMessageProc
///////////////////////////////////////////////////////////////////////////
void VirtualMPEGTapePlayerRecorder::MPEG2ReceiverMessageProc(UInt32 msg, UInt32 param1, UInt32 param2, void *pRefCon)
{
	VirtualMPEGTapePlayerRecorder *pPlayerRecorder = (VirtualMPEGTapePlayerRecorder*) pRefCon;

	if (msg == kMpeg2ReceiverDCLOverrun)
		pPlayerRecorder->overrunCount += 1;
	
	//printf("AY_DEBUG: MPEG2ReceiverMessageProc=%d ",(int)msg);
#if 0
	switch (msg)
	{
		case kMpeg2ReceiverReceivedBadPacket:
			printf("(kMpeg2ReceiverReceivedBadPacket)\n");
			break;
			
		case kMpeg2ReceiverDCLOverrun:
			printf("(kMpeg2ReceiverDCLOverrun)\n");
			break;
			
		case kMpeg2ReceiverAllocateIsochPort:
			printf("(kMpeg2ReceiverAllocateIsochPort)\n");
			break;
			
		case kMpeg2ReceiverReleaseIsochPort:
			printf("(kMpeg2ReceiverReleaseIsochPort)\n");
			break;
			
		default:
			printf("(Unknown)\n");
			break;
	};
#endif	
	return;
}

///////////////////////////////////////////////////////////////////////////
// VirtualMPEGTapePlayerRecorder::MpegTransmitCallback
///////////////////////////////////////////////////////////////////////////
IOReturn VirtualMPEGTapePlayerRecorder::MpegTransmitCallback(UInt32 **ppBuf, bool *pDiscontinuityFlag, void *pRefCon)
{
	VirtualMPEGTapePlayerRecorder *pPlayerRecorder = (VirtualMPEGTapePlayerRecorder*) pRefCon;
	unsigned int cnt;
	IOReturn result = 0;
	UInt8 currentTransportMode;
	UInt8 currentTransportState;
	bool isStable;
	
	// If firstTSPacket is true, we should set the discontinuity flag, to 
	// require two PCRs before calculate the next data-rate.
	if (pPlayerRecorder->firstTSPacket == true)
	{
		// Signal a discontinuity
		*pDiscontinuityFlag = true;
		pPlayerRecorder->firstTSPacket = false;
	}
	else
	{
		// Signal no discontinuity
		*pDiscontinuityFlag = false;
	}

	// If no file reader, causes a CIP only cycle to be filled
	if (pPlayerRecorder->pReader == nil)
		return -1;
	
	// If pReader file has no transport stream packets, causes a CIP only cycle to be filled
	if (pPlayerRecorder->pReader->FileLenInTSPackets() == 0)
		return -1;
	
	if (pPlayerRecorder->flushMode == false)
	{
		// Get the current transport state
		pPlayerRecorder->pTapeSubunit->getTransportState(&currentTransportMode, &currentTransportState, &isStable);

		// See if we have a reposition request to take care of
		if (pPlayerRecorder->repositionRequested == true)
		{
			result = pPlayerRecorder->pReader->SeekToSpecificFrame(pPlayerRecorder->repositionFrame);
			pPlayerRecorder->currentFrameOffset = pPlayerRecorder->pReader->GetCurrentTimeCodePositionInFrames();
			pPlayerRecorder->pTapeSubunit->setTimeCodeFrameCount(pPlayerRecorder->currentFrameOffset);
			pPlayerRecorder->repositionRequested = false;
			pPlayerRecorder->repositionFrame = 0;
			*pDiscontinuityFlag = true;
		}
		
		// If we are in play/pause mode, causes a CIP only cycle to be filled
		if (currentTransportState == kAVCTapePlayFwdPause)
		{
			*pDiscontinuityFlag = true;
			return -1;
		}
		
		// If we're waiting to skip forward, see if it's time
		else if ((currentTransportState >= kAVCTapePlayFastFwd1) && 
				 (currentTransportState <= kAVCTapePlayFastestFwd) && 
				 ((pPlayerRecorder->pReader->isIFrameBoundary()) || (pPlayerRecorder->pReader->hasNaviFile == false)))
		{
			//printf("AY_DEBUG: skip forward\n");
			pPlayerRecorder->pReader->SeekForwards(30);
			pPlayerRecorder->currentFrameOffset = pPlayerRecorder->pReader->GetCurrentTimeCodePositionInFrames();
			pPlayerRecorder->pTapeSubunit->setTransportState(kAVCTapeTportModePlay, kAVCTapePlayFwd, true);
			*pDiscontinuityFlag = true;
		}

		// If we're waiting to skip backward, see if it's time
		else if ((currentTransportState >= kAVCTapePlayFastRev1) && 
				 (currentTransportState <= kAVCTapePlayFastestRev) &&
				 ((pPlayerRecorder->pReader->isIFrameBoundary()) || (pPlayerRecorder->pReader->hasNaviFile == false)))
		{
			//printf("AY_DEBUG: skip backward\n");
			pPlayerRecorder->pReader->SeekBackwards(15);
			pPlayerRecorder->currentFrameOffset = pPlayerRecorder->pReader->GetCurrentTimeCodePositionInFrames();
			pPlayerRecorder->pTapeSubunit->setTransportState(kAVCTapeTportModePlay, kAVCTapePlayFwd, true);
			*pDiscontinuityFlag = true;
  		}
		
		// Read the next transport stream paccket
		cnt = pPlayerRecorder->pReader->ReadNextTSPackets(pPlayerRecorder->tsPacketBuf,1);
		if (cnt != 1)
		{
			//printf("AY_DEBUG: reader error\n");

			if (pPlayerRecorder->loopMode == true)
			{
				pPlayerRecorder->pReader->SeekToBeginning();	
				cnt = pPlayerRecorder->pReader->ReadNextTSPackets(pPlayerRecorder->tsPacketBuf,1);
				if (cnt != 1)
				{
					// We tried going back to the beginning of the file, and
					// failed to read again, 
					pPlayerRecorder->flushMode = true;
					pPlayerRecorder->flushCnt = 0;
					result = -1;	// Causes a CIP only cycle to be filled
				}
				else
				{
					*ppBuf = (UInt32*) pPlayerRecorder->tsPacketBuf;
					pPlayerRecorder->currentFrameOffset = pPlayerRecorder->pReader->GetCurrentTimeCodePositionInFrames();
					pPlayerRecorder->pTapeSubunit->setTimeCodeFrameCount(pPlayerRecorder->currentFrameOffset);
					*pDiscontinuityFlag = true;
				}
			}
			else
			{
				// We've reached the end of the file, and we're not in loop mode, so flush now!
				pPlayerRecorder->flushMode = true;
				pPlayerRecorder->flushCnt = 0;
				result = -1;	// Causes a CIP only cycle to be filled
			}
		}
		else
		{
			*ppBuf = (UInt32*) pPlayerRecorder->tsPacketBuf;
			pPlayerRecorder->currentFrameOffset = pPlayerRecorder->pReader->GetCurrentTimeCodePositionInFrames();
			pPlayerRecorder->pTapeSubunit->setTimeCodeFrameCount(pPlayerRecorder->currentFrameOffset);
		}
	}
	else
	{
		// This code runs the transmitter for enough additional cycles to 
		// flush all the MPEG data from the DCL buffers 
		if (pPlayerRecorder->flushCnt > (kCyclesPerTransmitSegment * kNumTransmitSegments))
		{
			// TODO: We cannot change the transport state here becuase stopping
			// the transmitter would cause a deadlock. Need to address this problem!
			if (pPlayerRecorder->pReader)
			{
				delete pPlayerRecorder->pReader;
				pPlayerRecorder->pReader = nil;
			}
		}
		else
			pPlayerRecorder->flushCnt += 1;

		// Cause a CIP only cycle to be filled
		result = -1;
	}
	
	return result;
}

///////////////////////////////////////////////////////////////////////////
// VirtualMPEGTapePlayerRecorder::MPEG2TransmitterMessageProc
///////////////////////////////////////////////////////////////////////////
void VirtualMPEGTapePlayerRecorder::MPEG2TransmitterMessageProc(UInt32 msg, UInt32 param1, UInt32 param2, void *pRefCon)
{
	VirtualMPEGTapePlayerRecorder *pPlayerRecorder = (VirtualMPEGTapePlayerRecorder*) pRefCon;

	if (msg == kMpeg2TransmitterPreparePacketFetcher)
	{
		pPlayerRecorder->overrunCount += 1;
		pPlayerRecorder->firstTSPacket = true;
	}
		
	//printf("AY_DEBUG: MPEG2TransmitterMessageProc=%d ",(int)msg);
#if 0	
	switch (msg)
	{
		case kMpeg2TransmitterPreparePacketFetcher:
			printf("(kMpeg2TransmitterPreparePacketFetcher)\n");
			break;
			
		case kMpeg2TransmitterAllocateIsochPort:
			printf("(kMpeg2TransmitterAllocateIsochPort)\n");
			break;
			
		case kMpeg2TransmitterReleaseIsochPort:
			printf("(kMpeg2TransmitterReleaseIsochPort)\n");
			break;
			
		case kMpeg2TransmitterDCLOverrun:
			printf("(kMpeg2TransmitterDCLOverrun)\n");
			break;
			
		case kMpeg2TransmitterTimeStampAdjust:	
			printf("(kMpeg2TransmitterTimeStampAdjust)\n");
			break;
			
		default:
			printf("(Unknown)\n");
			break;
	};
#endif	
	return;
}

} // namespace AVS

