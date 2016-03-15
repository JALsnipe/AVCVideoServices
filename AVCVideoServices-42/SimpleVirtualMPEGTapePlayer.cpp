/*
	File:		SimpleVirtualMPEGTapePlayer.h
 
 Synopsis: This is the source file for the SimpleVirtualMPEGTapePlayer class.
 
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

using namespace AVS;

#include "SimpleVirtualMPEGTapePlayer.h"

//////////////////////////////////////////////////////
// SimpleVirtualMPEGTapePlayer constructor
//////////////////////////////////////////////////////
SimpleVirtualMPEGTapePlayer::SimpleVirtualMPEGTapePlayer()
{
	packetsConsumed = 0;
	transmitDone = false;
	pTransmitter = nil;
	pTapeSubunit = nil;
	pTSDemuxer = nil;
	inFile = nil;
	p2pConnectionExists = false;
	loopMode = false;
	flushMode = false;
	flushCnt = 0;
}

//////////////////////////////////////////////////////
// SimpleVirtualMPEGTapePlayer destructor
//////////////////////////////////////////////////////
SimpleVirtualMPEGTapePlayer::~SimpleVirtualMPEGTapePlayer()
{
	// If the transmitter still exists, delete it here
	if (pTransmitter)
		DestroyMPEG2Transmitter(pTransmitter);
	
	// Delete the virtual tape subunit
	if (pTapeSubunit)
		DestroyVirtualTapeSubunit(pTapeSubunit);
	
	// Close the input file
	if (inFile)
		fclose(inFile);
}

//////////////////////////////////////////////////////
// SimpleVirtualMPEGTapePlayer::initWithFileName
//////////////////////////////////////////////////////
IOReturn SimpleVirtualMPEGTapePlayer::initWithFileName(char* pMpegTSFileName, AVCDevice *pAVCDevice)
{
	// Local Vars
    IOReturn result ;
	
	// Prevent init after already inited
	if ((pTransmitter) || (pTapeSubunit) ||(inFile))
		return kIOReturnNotPermitted;
	
	// Open the input file
	inFile = fopen(pMpegTSFileName,"rb");
	if (inFile == nil)
		return kIOReturnError;
	
	result = CreateVirtualTapeSubunit(&pTapeSubunit,
									  kAVCTapeSigModeDVHS,
									  kDVHSTape_WriteProtect,
									  SimpleVirtualMPEGTapePlayer::MyVirtualTapeCMPConnectionHandler,
									  SimpleVirtualMPEGTapePlayer::MyVirtualTapeTransportStateChangeHandler, 
									  SimpleVirtualMPEGTapePlayer::MyVirtualTapeSignalModeChangeHandler,
									  SimpleVirtualMPEGTapePlayer::MyVirtualTapeTimeCodeRepositionHandler,
									  this,
									  pAVCDevice);
	
	return result;
}

///////////////////////////////////////////////////////////////////////////
// SimpleVirtualMPEGTapePlayer::setLoopModeState
///////////////////////////////////////////////////////////////////////////
void SimpleVirtualMPEGTapePlayer::setLoopModeState(bool enableLoopMode)
{
	loopMode = enableLoopMode;
}

///////////////////////////////////////////////////////////////////////////
// SimpleVirtualMPEGTapePlayer::restartTransmit
///////////////////////////////////////////////////////////////////////////
void  SimpleVirtualMPEGTapePlayer::restartTransmit(void)
{
	// Rewind to the beginning of the file.
	if (inFile)
		rewind(inFile);
	transmitDone = false;
	flushMode = false;
	flushCnt = 0;
	pTapeSubunit->setTimeCodeFrameCount(0);
}

///////////////////////////////////////////////////////////////////////////
// SimpleVirtualMPEGTapePlayer::getTapeSubunitTimeCodeFrameCount
///////////////////////////////////////////////////////////////////////////
UInt32 SimpleVirtualMPEGTapePlayer::getTapeSubunitTimeCodeFrameCount()
{
	return pTapeSubunit->getTimeCodeFrameCount();
}

///////////////////////////////////////////////////////////////////////////
// SimpleVirtualMPEGTapePlayer::getTapeSubunitTimeCodeFrameCountInHMSF
///////////////////////////////////////////////////////////////////////////
void SimpleVirtualMPEGTapePlayer::getTapeSubunitTimeCodeFrameCountInHMSF(UInt32 *pHours, UInt32 *pMinutes, UInt32 *pSeconds, UInt32 *pFrames)
{
	UInt32 hours,minutes,seconds,frames;
	
	pTapeSubunit->getTimeCodeFrameCountInHMSF(&hours,&minutes,&seconds,&frames);
	
	*pHours = hours;
	*pMinutes = minutes;
	*pSeconds = seconds;
	*pFrames = frames;
}

///////////////////////////////////////////////////////////////////////////
// SimpleVirtualMPEGTapePlayer::MyVirtualTapeTransportStateChangeHandler
///////////////////////////////////////////////////////////////////////////
IOReturn SimpleVirtualMPEGTapePlayer::MyVirtualTapeTransportStateChangeHandler(void *pRefCon, UInt8 newTransportMode, UInt8 newTransportState)
{
	//printf("Transport State Change Requested: Mode=0x%02X, State=0x%02X\n",newTransportMode,newTransportState);
	
	// This app, doesn't AVC transport mode changes changes.
	// Returning and error here will cause the transport control command to be rejected 
	return kIOReturnUnsupported;
}

///////////////////////////////////////////////////////////////////////////
// SimpleVirtualMPEGTapePlayer::MyVirtualTapeSignalModeChangeHandler
///////////////////////////////////////////////////////////////////////////
IOReturn SimpleVirtualMPEGTapePlayer::MyVirtualTapeSignalModeChangeHandler(void *pRefCon, UInt8 newSignalMode)
{
	//printf("Signal Mode Change Requested: Mode=0x%02X\n",newSignalMode);
	
	// This app, doesn't allow signal mode changes
	// Returning and error here will cause the signal modecontrol command to be rejected 
	return kIOReturnUnsupported;	
}

///////////////////////////////////////////////////////////////////////////
// SimpleVirtualMPEGTapePlayer::MyVirtualTapeTimeCodeRepositionHandler
///////////////////////////////////////////////////////////////////////////
IOReturn SimpleVirtualMPEGTapePlayer::MyVirtualTapeTimeCodeRepositionHandler(void *pRefCon, UInt32 newTimeCode)
{
	//printf("Signal Time Code Reposition Requested: Frame=0x%08X\n",(unsigned int) newTimeCode);
	
	// Time-code reposition command not supported, for now!
	// Returning and error here will cause the time-code reposition control command to be rejected 
	return kIOReturnUnsupported;
}

///////////////////////////////////////////////////////////////////////////
// SimpleVirtualMPEGTapePlayer::MyVirtualTapeCMPConnectionHandler
///////////////////////////////////////////////////////////////////////////
void SimpleVirtualMPEGTapePlayer::MyVirtualTapeCMPConnectionHandler(void *pRefCon, bool isInputPlug, UInt8 isochChannel, UInt8 isochSpeed, UInt8 p2pCount)
{
	// Local Vars
    IOReturn result = kIOReturnSuccess ;
	SimpleVirtualMPEGTapePlayer *pPlayer = (SimpleVirtualMPEGTapePlayer*) pRefCon;
	
	//printf("CMP Notification for %s Plug p2p=0x%02X Channel=0x%02X Speed=0x%02X\n",((isInputPlug == true) ? "Input" : "Output"), p2pCount, isochChannel, isochSpeed);
	
	// This app is a playback only app.
	if (isInputPlug == true)
		return;
	
	if ((p2pCount > 0) && (pPlayer->pTransmitter == nil))
	{
		// Use the AVCVideoServices framework's helper function to create the
		// MPEG2Transmitter object and dedicated real-time thread.
		// Note: Here we are relying on a number of default parameters.
		result = CreateMPEG2Transmitter(&pPlayer->pTransmitter,
										SimpleVirtualMPEGTapePlayer::MpegTransmitCallback,
										pPlayer,
										SimpleVirtualMPEGTapePlayer::MessageReceivedProc,
										pPlayer,
										nil,
										pPlayer->pTapeSubunit->GetNub(),
										kCyclesPerTransmitSegment,
										kNumTransmitSegments,
										false,
										3,	// Supports up to 36mbps streams.
										kTSPacketQueueSizeInPackets);
		if (!pPlayer->pTransmitter)
		{
			//printf("Error creating MPEG2Transmitter object: %d\n",result);
			return;
		}
		else
		{
			// Create the TSDemuxer for frame detection
			pPlayer->pTSDemuxer = new TSDemuxer(SimpleVirtualMPEGTapePlayer::PESCallback,
									   pPlayer,
									   nil,
									   nil,
									   1,	// programNum
									   564, // Only need to have the first few TS packets in the frame to determine frame type
									   0);	// We don't need audio PES packets
			
			// Set the channel to transmit on
			pPlayer->pTransmitter->setTransmitIsochChannel(isochChannel);
			
			// Set the isoch packet speed
			pPlayer->pTransmitter->setTransmitIsochSpeed((IOFWSpeed)isochSpeed);
			
			// Start the transmitter
			pPlayer->pTransmitter->startTransmit();
			
			// Set the transport in play
			pPlayer->pTapeSubunit->setTransportState(kAVCTapeTportModePlay, kAVCTapePlayFwd, true);
			
			// Flag that we have a p2p connection
			pPlayer->p2pConnectionExists = true;
			
		}
	}
	else if ((p2pCount == 0) && (pPlayer->pTransmitter != nil))
	{
		if (pPlayer->pTransmitter)
		{
			// Stop the transmitter
			pPlayer->pTransmitter->stopTransmit();
			
			// Delete the transmitter object
			DestroyMPEG2Transmitter(pPlayer->pTransmitter);
			pPlayer->pTransmitter = nil;
			
			if (pPlayer->pTSDemuxer)
				delete pPlayer->pTSDemuxer;
			pPlayer->pTSDemuxer = nil;
		}
		
		// Set the transport in stop
		pPlayer->pTapeSubunit->setTransportState(kAVCTapeTportModeWind, kAVCTapeWindStop, true);		
		
		// Flag that we have no p2p connections
		pPlayer->p2pConnectionExists = false;
	}
	
	return;
}

///////////////////////////////////////////////////////////////////////////
// SimpleVirtualMPEGTapePlayer::MessageReceivedProc
///////////////////////////////////////////////////////////////////////////
void SimpleVirtualMPEGTapePlayer::MessageReceivedProc(UInt32 msg, UInt32 param1, UInt32 param2, void *pRefCon)
{
	// This class currently doesn't do anything with this callback!
	return;
}

///////////////////////////////////////////////////////////////////////////
// SimpleVirtualMPEGTapePlayer::MpegTransmitCallback
///////////////////////////////////////////////////////////////////////////
IOReturn SimpleVirtualMPEGTapePlayer::MpegTransmitCallback(UInt32 **ppBuf, bool *pDiscontinuityFlag, void *pRefCon)
{
	unsigned int cnt;
	IOReturn result = 0;
	
	SimpleVirtualMPEGTapePlayer *pPlayer = (SimpleVirtualMPEGTapePlayer*) pRefCon;
	
	// Signal no discontinuity
	*pDiscontinuityFlag = false;
	
	// Just a sanity check.
	if (!pPlayer->inFile)
		return -1;
	
	if (pPlayer->flushMode == false)
	{
		// Read the next TS packet from the input file
		cnt = fread(pPlayer->tsPacketBuf,1,kMPEG2TSPacketSize,pPlayer->inFile);
		if (cnt != kMPEG2TSPacketSize)
		{
			if (pPlayer->loopMode == true)
			{
				rewind(pPlayer->inFile);
				cnt = fread(pPlayer->tsPacketBuf,1,kMPEG2TSPacketSize,pPlayer->inFile);
				if (cnt != kMPEG2TSPacketSize)
				{
					// We tried going back to the beginning of the file, and
					// failed to read again, so we'll disable loopMode now, 
					// which and flush, and set transmitDone to true.
					pPlayer->loopMode = false;
					pPlayer->flushMode = true;
					result = -1;	// Causes a CIP only cycle to be filled
				}
				else
				{
					pPlayer->pTapeSubunit->setTimeCodeFrameCount(0);
					pPlayer->packetsConsumed += 1;
					*ppBuf = (UInt32*) pPlayer->tsPacketBuf;
				}
			}
			else
			{
				// Transmit done, flush now!
				pPlayer->flushMode = true;
				result = -1;	// Causes a CIP only cycle to be filled
			}
		}
		else
		{
			pPlayer->packetsConsumed += 1;
			*ppBuf = (UInt32*) pPlayer->tsPacketBuf;
		}
		
		if (pPlayer->pTSDemuxer)
			pPlayer->pTSDemuxer->nextTSPacket((UInt8*) &pPlayer->tsPacketBuf);
	}
	else
	{
		// This code runs the transmitter for enough additional cycles to 
		// flush all the MPEG data from the DCL buffers 
		if (pPlayer->flushCnt > (kCyclesPerTransmitSegment * kNumTransmitSegments))
			pPlayer->transmitDone = true;
		else
			pPlayer->flushCnt += 1;
		result = -1;	// Causes a CIP only cycle to be filled
	}
	
	return result;
}

//////////////////////////////////////////////////////
// SimpleVirtualMPEGTapePlayer::PESCallback
//////////////////////////////////////////////////////
IOReturn SimpleVirtualMPEGTapePlayer::PESCallback(TSDemuxerMessage msg, PESPacketBuf* pPESPacket,void *pRefCon)
{
	SimpleVirtualMPEGTapePlayer *pPlayer = (SimpleVirtualMPEGTapePlayer*) pRefCon;
	
	if (((msg == kTSDemuxerPESReceived) || (msg == kTSDemuxerPESLargerThanAllocatedBuffer)) &&
		(pPESPacket->streamType == kTSDemuxerStreamTypeVideo))
	{
		pPlayer->pTapeSubunit->setTimeCodeFrameCount(pPlayer->pTapeSubunit->getTimeCodeFrameCount()+1);
	}
	
	// Don't forget to release this PES buffer
	pPlayer->pTSDemuxer->ReleasePESPacketBuf(pPESPacket);
	
	return kIOReturnSuccess;
}


