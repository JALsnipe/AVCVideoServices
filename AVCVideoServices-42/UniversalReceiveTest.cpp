/*
	File:		UniversalReceiveTest.cpp
 
 Synopsis: This is a simple command-line app for testing the UniversalReceiver, DVFramer, and TSDemuxer.
 
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

// Defines
#define kMicroSecondsPerSecond 1000000


// Prototypes
void PrintLogMessage(char *pString);
void PrintLogMessage(char *pString);
void MessageReceivedProc(UInt32 msg, UInt32 param1, UInt32 param2, void *pRefCont);
IOReturn DataReceiveCallback(UInt32 CycleDataCount, UniversalReceiveCycleData *pCycleData, void *pRefCon);
IOReturn MyNoDataProc(void *pRefCon);
IOReturn PESCallback(TSDemuxerMessage msg, PESPacketBuf* pPESPacket,void *pRefCon);
IOReturn MyDVFramerCallback(DVFramerCallbackMessage msg, DVFrame* pDVFrame, void *pRefCon, DVFramer *pDVFramer);

// A structure to use as the refCon for the 
// universal-receiver callbacks, containing
// a pointer to the app's DVFramer, and TSDemuxer.
struct DV_MPEG_Parsers
{
	TSDemuxer *mpegDeMuxer;
	DVFramer *dvFramer;
};

//////////////////////////////////////////////////////
//
// Main
//
//////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
	// Local Vars
    IOReturn result = kIOReturnSuccess ;
	unsigned int captureTimeInSeconds;
	unsigned int isochChannel;
	UniversalReceiver *receiver = nil;
	TSDemuxer *mpegDeMuxer;
	DVFramer *dvFramer;
	DV_MPEG_Parsers parsers;
	
	// Parse the command line
	if (argc != 3)
	{
		printf("Usage: %s isochChan captureTimeInSeconds\n",argv[0]);
		return -1;
	}
	isochChannel = atoi(argv[1]);
	captureTimeInSeconds = atoi(argv[2]);
		
	printf("Starting UniversalReceiveTest!\n");
	
	// Alloacate a string logger object and pass it our callback func
	StringLogger logger(PrintLogMessage);
	
	// Create a TSDemuxer
	mpegDeMuxer = new TSDemuxer(PESCallback,
								nil,
								nil,
								nil,
								1,
								kMaxVideoPESSizeDefault,
								kMaxAudioPESSizeDefault,
								kDefaultVideoPESBufferCount,
								kDefaultAudioPESBufferCount,
								&logger);
	if (!mpegDeMuxer)
	{
		printf("Error Allocating MPEG-2 Transport Stream Demuxer Object\n");
		return -1;
	}
	
	// Create a DVFramer
	dvFramer = new DVFramer(MyDVFramerCallback,
							nil,
							0x00,
							kDVFramerDefaultFrameCount,
							&logger);
	if (!dvFramer)
	{
		printf("Error Allocating DV Framer Object\n");
		return -1;
	}
	
	// Don't forget to call setup on the DVFramer object!
	result = dvFramer->setupDVFramer();
	if (result != kIOReturnSuccess)
	{
		printf("Error Setting up DV Framer Object: 0x%08X\n",result);
		return -1;
	}
		
	// Initialize the refcon struct;
	parsers.dvFramer = dvFramer;
	parsers.mpegDeMuxer = mpegDeMuxer;
	
	// Use the UniversalReceiver helper function to create the
	// UniversalReceiver object and dedicated real-time thread.
	result = CreateUniversalReceiver(&receiver,
									 nil,	// Don't pass in a default data-push callback, we'll specify one later!
									 nil,
									 MessageReceivedProc,
									 nil,
									 &logger,
									 nil,
									 kCyclesPerUniversalReceiveSegment,
									 kNumUniversalReceiveSegments,
									 kUniversalReceiverDefaultBufferSize,
									 false,
									 kUniversalReceiverDefaultBufferSize);
	if (!receiver)
	{
		printf("Error creating UniveralReceiver object: %d\n",result);
		return -1;
	}
	
	// Register a data-receive callback (using structure method and specifying only one callback per DCL program segment
	receiver->registerStructuredDataPushCallback(DataReceiveCallback, kCyclesPerUniversalReceiveSegment, &parsers);
	
	// Register a no-data notification callback, for noitification of 5 seconds of no received isoch!
	receiver->registerNoDataNotificationCallback(MyNoDataProc, nil, 5000);

	// Set the channel to receive on
	receiver->setReceiveIsochChannel(isochChannel);
	printf("Receive on channel: %d\n",isochChannel);
	
	// Start the receiver
	receiver->startReceive();
	
	// Monitor Progress every second until we're done
	for (unsigned int i=0;i<captureTimeInSeconds;i++)
	{
		// Not currently doing anything here in this app!
		usleep(kMicroSecondsPerSecond);
	}
	
	// Stop the receiver
	if (receiver->transportState == kUniversalReceiverTransportRecording)
		receiver->stopReceive();
	
	// Delete the AVS objects
	DestroyUniversalReceiver(receiver);
	delete mpegDeMuxer;
	delete dvFramer;
	
	return result;
}	


//////////////////////////////////////////////////////
// PrintLogMessage
//////////////////////////////////////////////////////
void PrintLogMessage(char *pString)
{
	printf("%s",pString);
}

//////////////////////////////////////////////////////
// MessageReceivedProc
//////////////////////////////////////////////////////
void MessageReceivedProc(UInt32 msg, UInt32 param1, UInt32 param2, void *pRefCont)
{
	printf("Message from Universal Isoch Receiver: %d\n",(int) msg);
	return;
}


//////////////////////////////////////////////////////
// DataReceiveCallback
//////////////////////////////////////////////////////
IOReturn DataReceiveCallback(UInt32 CycleDataCount, UniversalReceiveCycleData *pCycleData, void *pRefCon)
{
	IOReturn result = kIOReturnSuccess ;
	CIPPacketParserStruct parsedPacket;
	UInt32 cycle;
	UInt32 sourcePacket;
	DV_MPEG_Parsers *pParsers = (DV_MPEG_Parsers*) pRefCon;
	
	// Perserve some state on the last time this was called,
	// so that we can reset the demuxer or dv-framer when a mode
	// change from MPEG->DV, or visa-versa, occurs.
	static UInt8 lastFmt = k61883Fmt_Invalid;

	for (int cycle=0;cycle<CycleDataCount;cycle++)
	{
		// Only process CIP-style packet. 
		if (IsCIPPacket(pCycleData[cycle].isochHeader))
		{
			result = ParseCIPPacket(pCycleData[cycle].pPayload, pCycleData[cycle].payloadLength, &parsedPacket);
			if (result != kIOReturnSuccess)
			{
				// The only way this can fail, is if there is more source
				// packets in the CIP-packet than what we ever expect.
				// (basically, just a sanity check here!)
				printf("ParseCIPPacket failed. Result: 0x%08X\n",result);
			}
			
			if (parsedPacket.fmt == k61883Fmt_MPEG2TS)
			{
				// Packet is MPEG2-TS. Pass the source packets to the TSDemuxer

				// Reset the demuxer, if needed!
				if (lastFmt != k61883Fmt_MPEG2TS)
				{
					printf("PESCallback: Reset TS Demuxer\n");
					pParsers->mpegDeMuxer->resetTSDemuxer();
					lastFmt = k61883Fmt_MPEG2TS;
				}
				
				// Notice that we offset each source packet by 4 bytes to skip over the 
				// source-packet-header present in 61883-4 streams. (i.e. - in 61883-4
				// source packets are 192 bytes, but the TSDemuxer only wants the 188 byte
				// transport stream packets that follows the source-packet header.
				for (sourcePacket=0;sourcePacket<parsedPacket.numSourcePackets;sourcePacket++)
					result = pParsers->mpegDeMuxer->nextTSPacket(&parsedPacket.pSourcePacket[sourcePacket][4],
																 pCycleData[cycle].fireWireTimeStamp,
																 pCycleData[cycle].nanoSecondsTimeStamp);
					if (result != kIOReturnSuccess)
						printf("DataReceiveCallback: nextTSPacket failed. Result: 0x%08X\n",result);
			}
			else if (parsedPacket.fmt == k61883Fmt_DV)
			{
				// Packet is DV. Pass the source packets to the DVFramer

				// Reset the dv-framer, if needed!
				if (lastFmt != k61883Fmt_DV)
				{	
					printf("PESCallback: Reset DV Framer\n");
					pParsers->dvFramer->resetDVFramer();
					lastFmt = k61883Fmt_DV;
				}
				
				for (sourcePacket=0;sourcePacket<parsedPacket.numSourcePackets;sourcePacket++)
					result = pParsers->dvFramer->nextDVSourcePacket(parsedPacket.pSourcePacket[sourcePacket], 
																	parsedPacket.sourcePacketSize, 
																	parsedPacket.dvMode, 
																	parsedPacket.syt, 
																	pCycleData[cycle].fireWireTimeStamp,
																	pCycleData[cycle].nanoSecondsTimeStamp);
					if (result != kIOReturnSuccess)
						printf("DataReceiveCallback: nextTSPacket failed. Result: 0x%08X\n",result);
			}
			else
			{
				// This CIP-based isoch packet is something other than MPEG2-TS or DV
				// (perhaps AM824). It's probably a good idea to set lastFmt to
				// an invalid value here, so the demxuer/dv-framer will be reset
				// on the next MPEG or DV packet.
				lastFmt = k61883Fmt_Invalid;
			}
		}
	}
	
	return kIOReturnSuccess;
}

//////////////////////////////////////////////////////
// MyNoDataProc
//////////////////////////////////////////////////////
IOReturn MyNoDataProc(void *pRefCon)
{
	// Called when the univeral receiver doesn't get any 
	// data for the specified amount of time.
	
	printf("MyNoDataProc called!\n");
	return kIOReturnSuccess;
}

//////////////////////////////////////////////////////
// PESCallback
//////////////////////////////////////////////////////
IOReturn PESCallback(TSDemuxerMessage msg, PESPacketBuf* pPESPacket,void *pRefCon)
{
	switch (msg)
	{
		case kTSDemuxerPESReceived:
			printf("MPEG PES Packet Received. Stream Type: %s PES Len: %6d, PID: 0x%04X, FireWire Time-Stamp: 0x%08X, System Time-Stamp 0x%016llX\n",
				   (pPESPacket->streamType == kTSDemuxerStreamTypeVideo) ? "Video" : "Audio",
				   pPESPacket->pesBufLen,
				   pPESPacket->pid,
				   pPESPacket->startTSPacketTimeStamp,
				   pPESPacket->startTSPacketU64TimeStamp);
			break;
			
		case kTSDemuxerPacketError:
			printf("PESCallback: kTSDemuxerPacketError\n");
			break;
			
		case kTSDemuxerDiscontinuity:
			printf("PESCallback: kTSDemuxerDiscontinuity\n");
			break;
			
		case kTSDemuxerIllegalAdaptationFieldCode:
			printf("PESCallback: kTSDemuxerIllegalAdaptationFieldCode\n");
			break;
			
		case kTSDemuxerBadAdaptationFieldLength:
			printf("PESCallback: kTSDemuxerBadAdaptationFieldLength\n");
			break;
			
		case kTSDemuxerPESLargerThanAllocatedBuffer:
			printf("PESCallback: kTSDemuxerPESLargerThanAllocatedBuffer\n");
			break;
			
		case kTSDemuxerFlushedPESBuffer:
			printf("PESCallback: kTSDemuxerFlushedPESBuffer\n");
			break;
			
		case kTSDemuxerRescanningForPSI:
			printf("PESCallback: kTSDemuxerRescanningForPSI\n");
			break;
			
		default:
			printf("PESCallback: Invalid Message: 0x%08X\n",msg);
			break;
	}
	
	
	// Don't forget to release this PES buffer
	// Note that no matter what the msg is, this callback includes a PESPacketBuf
	pPESPacket->pTSDemuxer->ReleasePESPacketBuf(pPESPacket);
	
	return kIOReturnSuccess;
}

//////////////////////////////////////////////////////
// MyDVFramerCallback
//////////////////////////////////////////////////////
IOReturn MyDVFramerCallback(DVFramerCallbackMessage msg, DVFrame* pDVFrame, void *pRefCon, DVFramer *pDVFramer)
{
	UInt8 dvMode;
	UInt32 dvFrameSize;
	UInt32 dvSourcePacketSize;
	IOReturn result;
	
	switch (msg)
	{
		case kDVFramerFrameReceivedSuccessfully:
			printf("DV Frame Received. Mode:0x%02X, Frame-Length: %6d, SYT: 0x%04X, FireWire Time-Stamp: 0x%08X, System Time-Stamp 0x%016llX\n",
				   pDVFrame->frameMode,
				   pDVFrame->frameLen,
				   pDVFrame->frameSYTTime,
				   pDVFrame->packetStartTimeStamp,
				   pDVFrame->packetStartU64TimeStamp);

#if 0			
			// For testing purposes, try to extract the DV mode from the raw frame data
			result = GetDVModeFromFrameData(pDVFrame->pFrameData,&dvMode,&dvFrameSize,&dvSourcePacketSize);
			if (result == kIOReturnSuccess)
				printf("GetDVModeFromFrameData() determined the frame was dvMode: 0x%02X, frame-size = %d, source-packet-size = %d\n",dvMode,dvFrameSize,dvSourcePacketSize);
			else
				printf("GetDVModeFromFrameData() was unable to determined the framedvMode!\n");
#endif	
			
			break;
			
		case kDVFramerPartialFrameReceived:
			printf("MyDVFramerCallback: kDVFramerPartialFrameReceived\n");
			break;
			
		case kDVFramerCorruptFrameReceived:
			printf("MyDVFramerCallback: kDVFramerCorruptFrameReceived\n");
			break;
			
		case kDVFramerNoFrameBufferAvailable:
			printf("MyDVFramerCallback: kDVFramerNoFrameBufferAvailable\n");
			break;
			
		case kDVFramerNoMemoryForFrameBuffer:
			printf("MyDVFramerCallback: kDVFramerNoMemoryForFrameBuffer\n");
			break;
			
		default:
			printf("MyDVFramerCallback: Invalid Message: 0x%08X\n",msg);
			break;
	}
	
	// Don't forget to release this DV Frame, if needed
	// Note that only certain msg values include a passed-in DVFrame object
	if (pDVFrame)
		pDVFramer->ReleaseDVFrame(pDVFrame);
	
	return kIOReturnSuccess;
}

