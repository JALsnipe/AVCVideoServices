/*
	File:		AVCDeviceTest.cpp
 
 Synopsis: This is a simple command-line app for testing the AVCDeviceController and AVCDevice classes.
 
	Copyright: 	© Copyright 2001-2003 Apple Computer, Inc. All rights reserved.
 
	Written by: ayanowitz
 
 Disclaimer:	IMPORTANT:  This Apple software is supplied to you by Apple Computer, Inc.
 ("Apple") in consideration of your agreement to the following terms, and your
 use, installation, modification or redistribution of this Apple software
 constitutes acceptance of these terms.  If you do not agree with these terms,
 please do not use, install, modify or redistribute this Apple software.
 
 In consideration of your agreement to abide by the following terms, and subject
 to these terms, Apple grants you a personal, non-exclusive license, under AppleÕs
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

//#define kTestUsingUniversalReceiver 1


// Prototypes
IOReturn MyAVCDeviceControllerNotification(AVCDeviceController *pAVCDeviceController, void *pRefCon, AVCDevice* pDevice);
IOReturn MyAVCDeviceMessageNotification(AVCDevice *pAVCDevice,
										natural_t messageType,
										void * messageArgument,
										void *pRefCon);

void GetDeviceTimeCode(void);
static unsigned int bcd2bin(unsigned int input);

void PrintLogMessage(char *pString);
StringLogger logger(PrintLogMessage);

#ifdef kTestUsingUniversalReceiver	
IOReturn PESCallback(TSDemuxerMessage msg, PESPacketBuf* pPESPacket,void *pRefCon);
IOReturn MyDVFramerCallback(DVFramerCallbackMessage msg, DVFrame* pDVFrame, void *pRefCon, DVFramer *pDVFramer);
IOReturn UniversalReceiveCallback(UInt32 CycleDataCount, UniversalReceiveCycleData *pCycleData, void *pRefCon);
#else
IOReturn MyFrameReceivedProc (DVFrameReceiveMessage msg, DVReceiveFrame* pFrame, void *pRefCon);
#endif

AVCDeviceController *pAVCDeviceController;
AVCDevice *pStreamReceiveDevice = nil;
AVCDeviceStream* pAVCDeviceStream = nil;
TSDemuxer *mpegDeMuxer;
DVFramer *dvFramer;

UInt32 framesReceived = 0;

//////////////////////////////////////////////////////
//
// Main
//
//////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
	IOReturn result = kIOReturnSuccess ;


#ifdef kTestUsingUniversalReceiver	
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
#endif
	
	// Create the AVCDeviceController and dedicated thread
	result = CreateAVCDeviceController(&pAVCDeviceController,MyAVCDeviceControllerNotification, nil);
	if (!pAVCDeviceController)
	{
		printf("Error creating AVCDeviceController object: %d\n",result);
		return -1;
	}

	// For now, just endless loop here!
	for(;;)
	{
		usleep(1000000); // 1 second
		if (pAVCDeviceStream != nil)
		{
#if 0		
			// Note: This code is unsafe for hot-unplugging events, since the device may be
			// closed on a different thread while it's being used here.
			// Proper code would protect the "closing while in use" case.
			if ((pStreamReceiveDevice->supportsFCP) && (pStreamReceiveDevice->isOpened()))
				GetDeviceTimeCode();		
#endif

#ifndef kTestUsingUniversalReceiver	
			printf("DV frames received: %d\n",(int) framesReceived);
#endif
		}
	}
	
	// NOTE: Since this app never ends (endless-loop), there's no clean-up code here!

	return result;
}

//////////////////////////////////////////////////////
// MyAVCDeviceControllerNotification
//////////////////////////////////////////////////////
IOReturn MyAVCDeviceControllerNotification(AVCDeviceController *pAVCDeviceController, void *pRefCon, AVCDevice* pDevice)
{
	IOReturn result = kIOReturnSuccess ;
	AVCDevice *pAVCDevice;
	UInt32 i;
	
	printf("\nMyAVCDeviceControllerNotification\n");
	printf("=================================\n");
	for (i=0;i<(UInt32)CFArrayGetCount(pAVCDeviceController->avcDeviceArray);i++)
	{
		pAVCDevice = (AVCDevice*) CFArrayGetValueAtIndex(pAVCDeviceController->avcDeviceArray,i);

		printf("\n  Device %d: %s\n",(int)i,(pAVCDevice->deviceName != nil) ? pAVCDevice->deviceName : "Unknown Device Name");
		printf("    isAttached: %s   ",(pAVCDevice->isAttached == true) ? "Yes" : "No");
		printf("capabilitiesDiscovered: %s\n",(pAVCDevice->capabilitiesDiscovered == true) ? "Yes" : "No");
		printf("    supportsFCP: %s   ",(pAVCDevice->supportsFCP == true) ? "Yes" : "No");
		printf("subUnits: 0x%08X   ",(int)pAVCDevice->subUnits);
		printf("hasTapeSubunit: %s\n",(pAVCDevice->hasTapeSubunit == true) ? "Yes" : "No");
		printf("    numInputPlugs: 0x%08X   ",(int)pAVCDevice->numInputPlugs);
		printf("numOutputPlugs: 0x%08X\n",(int)pAVCDevice->numOutputPlugs);
		printf("    isDVDevice: %s   ",(pAVCDevice->isDVDevice == true) ? "Yes" : "No");
		if (pAVCDevice->isDVDevice == true)
			printf("dvMode: 0x%02X   ",pAVCDevice->dvMode);
		printf("isMPEGDevice: %s   ",(pAVCDevice->isMPEGDevice == true) ? "Yes" : "No");
		printf("isDVCProDevice: %s\n",(pAVCDevice->isDVCProDevice == true) ? "Yes" : "No");

		// If the device is not open, open it now
		if ((pAVCDevice->isOpened() == false) && (pAVCDevice->isAttached == true))
		{
			UInt32 retryOpenCnt = 0;
			do
			{
				printf ("  Opening Device: %s\n",(pAVCDevice->deviceName != nil) ? pAVCDevice->deviceName : "Unknown Device Name");
				result = pAVCDevice->openDevice(MyAVCDeviceMessageNotification, nil);
				if (result != kIOReturnSuccess)
				{
					printf ("  Error Opening Device: 0x%08X\n",result);
					usleep(1000000); // 1 second
				}
			}while ((result != kIOReturnSuccess) && (++retryOpenCnt < 5));
		}

#ifdef kTestUsingUniversalReceiver	
		// Create a universal receive object for the device if we haven't done so already
		if ((pAVCDevice->isOpened() == true) && (pAVCDeviceStream == nil))
#else
			// If the device is a dv device, create a dv receive object if we haven't done so already
		if ((pAVCDevice->isOpened() == true) && (pAVCDevice->isDVDevice == true) && (pAVCDeviceStream == nil))
#endif
		{
			framesReceived = 0;

#ifdef kTestUsingUniversalReceiver	
			pAVCDeviceStream = pAVCDevice->CreateUniversalReceiverForDevicePlug(0,
																				nil,
																				nil,
																				nil,
																				nil,
																				&logger,
																				kCyclesPerUniversalReceiveSegment,
																				kNumUniversalReceiveSegments,
																				kUniversalReceiverDefaultBufferSize);
			if (pAVCDeviceStream == nil)
			{
				printf("Error creating UniversalReceiver object for device\n");
				return -1;
			}
			else
			{
				// Register a data-receive callback (using structure method and specifying only one callback per DCL program segment
				pAVCDeviceStream->pUniversalReceiver->registerStructuredDataPushCallback(UniversalReceiveCallback, kCyclesPerUniversalReceiveSegment, nil);
			}
#else
			pAVCDeviceStream = pAVCDevice->CreateDVReceiverForDevicePlug(0,
																MyFrameReceivedProc,
																nil,
																nil,
																nil,
																&logger,
																kCyclesPerDVReceiveSegment*2,
																kNumDVReceiveSegments,
																pAVCDevice->dvMode,
																kDVReceiveNumFrames);
			if (pAVCDeviceStream == nil)
			{
				printf("Error creating DVReceiver object for device\n");
				return -1;
			}
#endif


			printf("  Starting Isoch Receive for device\n");
			result = pAVCDevice->StartAVCDeviceStream(pAVCDeviceStream);
			
			pStreamReceiveDevice = pAVCDevice;
		}
	}

	printf ("\n");
	return kIOReturnSuccess ;
}

//////////////////////////////////////////////////////
// MyAVCDeviceMessageNotification
//////////////////////////////////////////////////////
IOReturn MyAVCDeviceMessageNotification(AVCDevice *pAVCDevice,
										natural_t messageType,
										void * messageArgument,
										void *pRefCon)
{
	IOReturn result = kIOReturnSuccess ;

	printf("MyAVCDeviceMessageNotification:  Device: %s  Message: ",
		(pAVCDevice->deviceName != nil) ? pAVCDevice->deviceName : "Unknown Device Name");
	switch(messageType)
	{

		case kIOMessageServiceIsTerminated:
			printf("kIOMessageServiceIsTerminated\n");
			break;

		case kIOMessageServiceIsSuspended:
			printf("kIOMessageServiceIsSuspended\n");
			break;

		case kIOMessageServiceIsResumed:
			printf("kIOMessageServiceIsResumed\n");
			break;

		case kIOMessageServiceIsRequestingClose:
			printf("kIOMessageServiceIsRequestingClose\n");
			break;

		case kIOMessageServiceIsAttemptingOpen:
			printf("kIOMessageServiceIsAttemptingOpen\n");
			break;

		case kIOMessageServiceWasClosed:
			printf("kIOMessageServiceWasClosed\n");
			break;

		case kIOMessageServiceBusyStateChange:
			printf("kIOMessageServiceBusyStateChange\n");
			break;

		case kIOMessageCanDevicePowerOff:
			printf("kIOMessageCanDevicePowerOff\n");
			break;

		case kIOMessageDeviceWillPowerOff:
			printf("kIOMessageDeviceWillPowerOff\n");
			break;

		case kIOMessageDeviceWillNotPowerOff:
			printf("kIOMessageDeviceWillNotPowerOff\n");
			break;

		case kIOMessageDeviceHasPoweredOn:
			printf("kIOMessageDeviceHasPoweredOn\n");
			break;

		case kIOMessageCanSystemPowerOff:
			printf("kIOMessageCanSystemPowerOff\n");
			break;

		case kIOMessageSystemWillPowerOff:
			printf("kIOMessageSystemWillPowerOff\n");
			break;

		case kIOMessageSystemWillNotPowerOff:
			printf("kIOMessageSystemWillNotPowerOff\n");
			break;

		case kIOMessageCanSystemSleep:
			printf("kIOMessageCanSystemSleep\n");
			break;

		case kIOMessageSystemWillSleep:
			printf("kIOMessageSystemWillSleep\n");
			break;

		case kIOMessageSystemWillNotSleep:
			printf("kIOMessageSystemWillNotSleep\n");
			break;

		case kIOMessageSystemHasPoweredOn:
			printf("kIOMessageSystemHasPoweredOn\n");
			break;

		case kIOMessageSystemWillRestart:
			printf("kIOMessageSystemWillRestart\n");
			break;

		case kIOFWMessageServiceIsRequestingClose:
			printf("kIOFWMessageServiceIsRequestingClose\n");
			break;

		case kIOFWMessagePowerStateChanged:
			printf("kIOFWMessagePowerStateChanged\n");
			break;

		case kIOFWMessageTopologyChanged:
			printf("kIOFWMessageTopologyChanged\n");
			break;
			
		default:
			printf("Unknown Message: 0x%08X\n",messageType);
			break;
	};
	
	switch (messageType)
	{
		case kIOMessageServiceIsRequestingClose:
			if (pStreamReceiveDevice == pAVCDevice)
			{
				printf("  Stopping Receive\n");
				pAVCDevice->StopAVCDeviceStream(pAVCDeviceStream);
				pAVCDevice->DestroyAVCDeviceStream(pAVCDeviceStream);				
				pStreamReceiveDevice = nil;
				pAVCDeviceStream = nil;
			}
			printf ("\n  Closing Device: %s\n",(pAVCDevice->deviceName != nil) ? pAVCDevice->deviceName : "Unknown Device Name");
			pAVCDevice->closeDevice();
			if (result != kIOReturnSuccess)
				printf ("  Error Closing Device: 0x%08X\n",result);
			break;

		default:
			break;
	};
	
	return kIOReturnSuccess ;
}

#ifdef kTestUsingUniversalReceiver	
//////////////////////////////////////////////////////
// UniversalReceiveCallback
//////////////////////////////////////////////////////
IOReturn UniversalReceiveCallback(UInt32 CycleDataCount, UniversalReceiveCycleData *pCycleData, void *pRefCon)
{
	IOReturn result = kIOReturnSuccess ;
	CIPPacketParserStruct parsedPacket;
	UInt32 cycle;
	UInt32 sourcePacket;
	
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
					printf("ReceiveCallback: Reset TS Demuxer\n");
					mpegDeMuxer->resetTSDemuxer();
					lastFmt = k61883Fmt_MPEG2TS;
				}
				
				// Notice that we offset each source packet by 4 bytes to skip over the 
				// source-packet-header present in 61883-4 streams. (i.e. - in 61883-4
				// source packets are 192 bytes, but the TSDemuxer only wants the 188 byte
				// transport stream packets that follows the source-packet header.
				for (sourcePacket=0;sourcePacket<parsedPacket.numSourcePackets;sourcePacket++)
					result = mpegDeMuxer->nextTSPacket(&parsedPacket.pSourcePacket[sourcePacket][4],
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
					printf("ReceiveCallback: Reset DV Framer\n");
					dvFramer->resetDVFramer();
					lastFmt = k61883Fmt_DV;
				}
				
				for (sourcePacket=0;sourcePacket<parsedPacket.numSourcePackets;sourcePacket++)
					result = dvFramer->nextDVSourcePacket(parsedPacket.pSourcePacket[sourcePacket], 
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
#else
//////////////////////////////////////////////////////
// FrameReceivedProc
//////////////////////////////////////////////////////
IOReturn MyFrameReceivedProc (DVFrameReceiveMessage msg, DVReceiveFrame* pFrame, void *pRefCon)
{
	if (msg == kDVFrameReceivedSuccessfully)
	{
		framesReceived += 1;
	}
		
	return kIOReturnError;
}
#endif

//////////////////////////////////////////////////////
// PrintLogMessage
//////////////////////////////////////////////////////
void PrintLogMessage(char *pString)
{
	printf("%s",pString);
}

#ifdef kTestUsingUniversalReceiver	
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
	switch (msg)
	{
		case kDVFramerFrameReceivedSuccessfully:
			printf("DV Frame Received. Mode:0x%02X, Frame-Length: %6d, SYT: 0x%04X, FireWire Time-Stamp: 0x%08X, System Time-Stamp 0x%016llX\n",
				   pDVFrame->frameMode,
				   pDVFrame->frameLen,
				   pDVFrame->frameSYTTime,
				   pDVFrame->packetStartTimeStamp,
				   pDVFrame->packetStartU64TimeStamp);
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
#endif

///////////////////////////////////////////////////////////////////////////////////////
// GetDeviceTimeCode
///////////////////////////////////////////////////////////////////////////////////////
void GetDeviceTimeCode(void)
{
    UInt32 size;
    UInt8 cmd[8],response[8];
    IOReturn res;

	cmd[0] = kAVCStatusInquiryCommand;
	cmd[1] = IOAVCAddress(kAVCTapeRecorder, 0);
	cmd[2] = 0x57;
	cmd[3] = 0x71;
	cmd[4] = 0xFF;
	cmd[5] = 0xFF;
	cmd[6] = 0xFF;
	cmd[7] = 0xFF;

	size = 8;

	res = pStreamReceiveDevice->AVCCommand(cmd, 8, response, &size);

	if (res == kIOReturnSuccess)
	{
		if (response[0] == kAVCImplementedStatus)
		{
			// Formulate the relative time code into a string
			printf("Device Relative Time-Code: %c%02d:%02d:%02d.%02d\n",((response[4] & 0x80) == 0x80) ? '-':'+',
				bcd2bin(response[7]),
				bcd2bin(response[6]),
		  bcd2bin(response[5]),
		  bcd2bin((response[4]&0x7F)));
		}
		else
		{
			// Try the timecode comand
			cmd[0] = kAVCStatusInquiryCommand;
			cmd[1] = IOAVCAddress(kAVCTapeRecorder, 0);
			cmd[2] = 0x51;
			cmd[3] = 0x71;
			cmd[4] = 0xFF;
			cmd[5] = 0xFF;
			cmd[6] = 0xFF;
			cmd[7] = 0xFF;

			size = 8;

			res = pStreamReceiveDevice->AVCCommand( cmd, 8, response, &size);

			if ((res == kIOReturnSuccess) && (response[0] == kAVCImplementedStatus))
			{
				printf("Device Time-Code: %02d:%02d:%02d.%02d\n",
		   bcd2bin(response[7]),
		   bcd2bin(response[6]),
		   bcd2bin(response[5]),
		   bcd2bin(response[4]));
			}
		}
	}
}

//////////////////////////////////////////////
// bcd2bin - BCD to Bin conversion
//////////////////////////////////////////////
static unsigned int bcd2bin(unsigned int input)
{
	unsigned int shift,output;

	shift = 1;
	output = 0;
	while(input)
	{
		output += (input%16) * shift;
		input /= 16;
		shift *= 10;
	};

	return output;
}
