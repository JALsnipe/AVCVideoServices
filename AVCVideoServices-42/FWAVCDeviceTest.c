/*
	File:		FWAVCDeviceTest.cpp
 
 Synopsis: This is a simple command-line app for testing the FWAVC C-APIs.
 
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

#include "FWAVC.h"

// Prototypes
IOReturn MyAVCDeviceControllerNotification (FWAVCDeviceControllerRef fwavcDeviceControllerRef, 
											void *pRefCon, 
											FWAVCDeviceRef fwavcDeviceRef);

IOReturn MyAVCDeviceMessageNotification(FWAVCDeviceRef fwavcDeviceRef, 
										natural_t messageType, 
										void * messageArgument, 
										void *pRefCon);

void GetDeviceTimeCode(void);
static unsigned int bcd2bin(unsigned int input);

void PrintLogMessage(char *pString);

IOReturn PESCallback(FWAVCTSDemuxerCallbackMessage msg, 
					 FWAVCTSDemuxerPESPacketRef fwavcTSDemuxerPESPacketRef, 
					 void *pRefCon,
					 FWAVCTSDemuxerRef fwavcTSDemuxerRef);

IOReturn MyDVFramerCallback(FWAVCDVFramerCallbackMessage msg, 
							FWAVCDVFramerFrameRef fwavcDVFramerFrameRef, 
							void *pRefCon, 
							FWAVCDVFramerRef fwavcDVFramerRef);

IOReturn UniversalReceiveCallback(FWAVCDeviceStreamRef fwavcDeviceStreamRef,
								  FWAVCDeviceRef fwavcDeviceRef,
								  UInt32 CycleDataCount, 
								  FWAVCUniversalReceiveCycleData *pCycleData, 
								  void *pRefCon);

UInt32 framesReceived = 0;

FWAVCTSDemuxerRef MyFWAVCTSDemuxerRef = nil;
FWAVCDVFramerRef MyFWAVCDVFramerRef = nil;
FWAVCDeviceControllerRef MyFWAVCDeviceControllerRef = nil;
FWAVCDeviceStreamRef MyFWAVCDeviceStreamRef = nil;
FWAVCDeviceRef StreamReceiveDeviceRef = nil;

//////////////////////////////////////////////////////
//
// Main
//
//////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
	IOReturn result;
	
	// Create a TSDemuxer
	result = FWAVCTSDemuxerCreate(PESCallback,
								  nil,
								  1,
								  kFWAVCDeviceStreamDefaultParameter,
								  kFWAVCDeviceStreamDefaultParameter,
								  kFWAVCDeviceStreamDefaultParameter,
								  kFWAVCDeviceStreamDefaultParameter,
								  &MyFWAVCTSDemuxerRef,
								  PrintLogMessage);
	if (result != kIOReturnSuccess)
	{
		printf("Error Allocating MPEG-2 Transport Stream Demuxer: %d\n",result);
		return -1;
	}
	
	// Create a DVFramer
	// Note that this also does the setup of the DVFramer object (unlike the C++ API version).
	result = FWAVCDVFramerCreate(MyDVFramerCallback,
								 nil,
								 kFWAVCDeviceStreamDefaultParameter,
								 kFWAVCDeviceStreamDefaultParameter,
								 &MyFWAVCDVFramerRef,
								 PrintLogMessage);
	if (result != kIOReturnSuccess)
	{
		printf("Error Allocating DV Framer: %d\n",result);
		return -1;
	}

	
	// Create the AVCDeviceController and dedicated thread
	result = FWAVCDeviceControllerCreate(&MyFWAVCDeviceControllerRef,
										 MyAVCDeviceControllerNotification,
										 nil,
										 nil,
										 PrintLogMessage);
	if (result != kIOReturnSuccess)
	{
		printf("Error creating AVCDeviceController: %d\n",result);
		return -1;
	}
	
	// For now, just endless loop here!
	for(;;)
	{
		usleep(1000000); // 1 second

#if 0	
		CFArrayRef myArray =  FWAVCDeviceControllerCopyDeviceArray(MyFWAVCDeviceControllerRef);
		if (myArray)
		{
			printf("Number of devices in AVCDeviceController's array: %d\n",CFArrayGetCount(myArray));
			CFRelease(myArray);
		}
#endif
		
		if (MyFWAVCDeviceStreamRef != nil)
		{
#if 0		
			// Note: This code is unsafe for hot-unplugging events, since the device may be
			// closed on a different thread while it's being used here.
			// Proper code would protect the "closing while in use" case.
			if ((pStreamReceiveDevice->supportsFCP) && (pStreamReceiveDevice->isOpened()))
				GetDeviceTimeCode();		
#endif
		}
	}
	
	// NOTE: Since this app never ends (endless-loop), there's no clean-up code here!
	
	return result;
}

//////////////////////////////////////////////////////
// MyAVCDeviceControllerNotification
//////////////////////////////////////////////////////
IOReturn MyAVCDeviceControllerNotification (FWAVCDeviceControllerRef fwavcDeviceControllerRef, 
											void *pRefCon, 
											FWAVCDeviceRef fwavcDeviceRef)
{
	IOReturn result = kIOReturnSuccess ;
	char name[80] = "";
	CFStringRef deviceNameString = FWAVCDeviceCopyDeviceName(fwavcDeviceRef);
	CFStringGetCString(deviceNameString, name, 80, kCFStringEncodingMacRoman);
	CFRelease(deviceNameString);

	// Test the API for retrieving the array of devices!
	CFArrayRef deviceArray = FWAVCDeviceControllerCopyDeviceArray(fwavcDeviceControllerRef);
	CFRelease(deviceArray);
	
	printf("\nMyAVCDeviceControllerNotification\n");
	printf("=================================\n");
	
	printf("\n  Device: %s\n",name);
	
	printf("    isAttached: %s   ",(FWAVCDeviceIsAttached(fwavcDeviceRef) == true) ? "Yes" : "No");
	printf("capabilitiesDiscovered: %s\n",(FWAVCDeviceCapabilitiesDiscovered(fwavcDeviceRef) == true) ? "Yes" : "No");
	printf("    supportsFCP: %s   ",(FWAVCDeviceSupportsFCP(fwavcDeviceRef) == true) ? "Yes" : "No");
	printf("subUnits: 0x%08X   ",(int)FWAVCDeviceGetSubUnits(fwavcDeviceRef));
	printf("hasTapeSubunit: %s\n",(FWAVCDeviceHasTapeSubunit(fwavcDeviceRef) == true) ? "Yes" : "No");
	printf("    numInputPlugs: 0x%08X   ",(int)FWAVCDeviceNumInputPlugs(fwavcDeviceRef));
	printf("numOutputPlugs: 0x%08X\n",(int)FWAVCDeviceNumOutputPlugs(fwavcDeviceRef));
	printf("    isDVDevice: %s   ",(FWAVCDeviceIsDVDevice(fwavcDeviceRef) == true) ? "Yes" : "No");
	if (FWAVCDeviceIsDVDevice(fwavcDeviceRef) == true)
		printf("dvMode: 0x%02X   ",FWAVCDeviceGetDVMode(fwavcDeviceRef));
	printf("isMPEGDevice: %s   ",(FWAVCDeviceIsMpegDevice(fwavcDeviceRef) == true) ? "Yes" : "No");
	printf("isDVCProDevice: %s\n",(FWAVCDeviceIsDVCProDevice(fwavcDeviceRef) == true) ? "Yes" : "No");
	
	// If the device is not open, open it now
	if ((FWAVCDeviceIsOpened(fwavcDeviceRef) == false) && (FWAVCDeviceIsAttached(fwavcDeviceRef) == true))
	{
		UInt32 retryOpenCnt = 0;
		do
		{
			printf ("  Opening Device: %s\n",name);
			result = FWAVCDeviceOpen(fwavcDeviceRef, MyAVCDeviceMessageNotification, nil);
			if (result != kIOReturnSuccess)
			{
				printf ("  Error Opening Device: 0x%08X\n",result);
				usleep(1000000); // 1 second
			}
		}while ((result != kIOReturnSuccess) && (++retryOpenCnt < 5));
	}
	
	// Create a universal receive object for the device if we haven't done so already
	if ((FWAVCDeviceIsOpened(fwavcDeviceRef) == true) && (MyFWAVCDeviceStreamRef == nil))
	{
		framesReceived = 0;
		
		MyFWAVCDeviceStreamRef = FWAVCDeviceCreateUniversalReceiver(fwavcDeviceRef,
																	0,
																	nil,
																	nil,
																	nil,
																	nil,
																	kFWAVCDeviceStreamDefaultParameter,
																	kFWAVCDeviceStreamDefaultParameter,
																	kFWAVCDeviceStreamDefaultParameter,
																	PrintLogMessage);
		if (MyFWAVCDeviceStreamRef == nil)
		{
			printf("Error creating UniversalReceiver object for device\n");
			return -1;
		}
		else
		{
			// Register a data-receive callback (using structure method and specifying only one callback per DCL program segment
			FWAVCUniversalReceiverSetStructuredDataReceivedCallback(MyFWAVCDeviceStreamRef,
																	UniversalReceiveCallback, 
																	kFWAVCDeviceStreamDefaultParameter, 
																	nil);
			
		}
		
		printf("  Starting Isoch Receive for device\n");
		result = FWAVCDeviceStreamStart(MyFWAVCDeviceStreamRef);
		
		StreamReceiveDeviceRef = fwavcDeviceRef;
	}
	
	printf ("\n");
	return kIOReturnSuccess ;
}

//////////////////////////////////////////////////////
// MyAVCDeviceMessageNotification
//////////////////////////////////////////////////////
IOReturn MyAVCDeviceMessageNotification(FWAVCDeviceRef fwavcDeviceRef, 
										natural_t messageType, 
										void * messageArgument, 
										void *pRefCon)
{
	IOReturn result = kIOReturnSuccess ;
	char name[80] = "";
	CFStringRef deviceNameString = FWAVCDeviceCopyDeviceName(fwavcDeviceRef);
	CFStringGetCString(deviceNameString, name, 80, kCFStringEncodingMacRoman);
	CFRelease(deviceNameString);
	
	printf("MyAVCDeviceMessageNotification:  Device: %s  Message: ",name);
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
			if (StreamReceiveDeviceRef == fwavcDeviceRef)
			{
				printf("  Stopping Receive\n");
				FWAVCDeviceStreamStop(MyFWAVCDeviceStreamRef);
				FWAVCDeviceStreamRelease(MyFWAVCDeviceStreamRef);				
				StreamReceiveDeviceRef = nil;
				MyFWAVCDeviceStreamRef = nil;
			}
			printf ("\n  Closing Device: %s\n",name);
			result = FWAVCDeviceClose(fwavcDeviceRef);
			if (result != kIOReturnSuccess)
				printf ("  Error Closing Device: 0x%08X\n",result);
				break;
			
		default:
			break;
	};
	
	return kIOReturnSuccess ;
}

//////////////////////////////////////////////////////
// UniversalReceiveCallback
//////////////////////////////////////////////////////
IOReturn UniversalReceiveCallback(FWAVCDeviceStreamRef fwavcDeviceStreamRef,
								  FWAVCDeviceRef fwavcDeviceRef,
								  UInt32 CycleDataCount, 
								  FWAVCUniversalReceiveCycleData *pCycleData, 
								  void *pRefCon)
{
	IOReturn result = kIOReturnSuccess ;
	FWAVCCIPPacketParserInfo parsedPacket;
	UInt32 cycle;
	UInt32 sourcePacket;
	
	// Perserve some state on the last time this was called,
	// so that we can reset the demuxer or dv-framer when a mode
	// change from MPEG->DV, or visa-versa, occurs.
	static UInt8 lastFmt = kFWAVC61883Fmt_Invalid;
	
	for (cycle=0;cycle<CycleDataCount;cycle++)
	{
		// Only process CIP-style packet. 
		if (FWAVCIsCIPPacket(pCycleData[cycle].isochHeader))
		{
			result = FWAVCParseCIPPacket(pCycleData[cycle].pPayload, pCycleData[cycle].payloadLength, &parsedPacket);
			if (result != kIOReturnSuccess)
			{
				// The only way this can fail, is if there is more source
				// packets in the CIP-packet than what we ever expect.
				// (basically, just a sanity check here!)
				printf("ParseCIPPacket failed. Result: 0x%08X\n",result);
			}
			
			if (parsedPacket.fmt == kFWAVC61883Fmt_MPEG2TS)
			{
				// Packet is MPEG2-TS. Pass the source packets to the TSDemuxer
				
				// Reset the demuxer, if needed!
				if (lastFmt != kFWAVC61883Fmt_MPEG2TS)
				{
					printf("ReceiveCallback: Reset TS Demuxer\n");
					FWAVCTSDemuxerReset(MyFWAVCTSDemuxerRef);
					lastFmt = kFWAVC61883Fmt_MPEG2TS;
				}
				
				// Notice that we offset each source packet by 4 bytes to skip over the 
				// source-packet-header present in 61883-4 streams. (i.e. - in 61883-4
				// source packets are 192 bytes, but the TSDemuxer only wants the 188 byte
				// transport stream packets that follows the source-packet header.
				for (sourcePacket=0;sourcePacket<parsedPacket.numSourcePackets;sourcePacket++)
					result = FWAVCTSDemuxerNextTSPacket(MyFWAVCTSDemuxerRef, 
														&parsedPacket.pSourcePacket[sourcePacket][4],
														pCycleData[cycle].fireWireTimeStamp,
														pCycleData[cycle].nanoSecondsTimeStamp);				
				if (result != kIOReturnSuccess)
					printf("DataReceiveCallback: nextTSPacket failed. Result: 0x%08X\n",result);
			}
			else if (parsedPacket.fmt == kFWAVC61883Fmt_DV)
			{
				// Packet is DV. Pass the source packets to the DVFramer
				
				// Reset the dv-framer, if needed!
				if (lastFmt != kFWAVC61883Fmt_DV)
				{	
					printf("ReceiveCallback: Reset DV Framer\n");
					FWAVCDVFramerReset(MyFWAVCDVFramerRef);
					lastFmt = kFWAVC61883Fmt_DV;
				}
				
				for (sourcePacket=0;sourcePacket<parsedPacket.numSourcePackets;sourcePacket++)
					result = FWAVCDVFramerNextDVSourcePacket(MyFWAVCDVFramerRef,
															 parsedPacket.pSourcePacket[sourcePacket], 
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
				lastFmt = kFWAVC61883Fmt_Invalid;
			}
		}
	}
	
	return kIOReturnSuccess;
}

//////////////////////////////////////////////////////
// PrintLogMessage
//////////////////////////////////////////////////////
void PrintLogMessage(char *pString)
{
	printf("%s",pString);
}

//////////////////////////////////////////////////////
// PESCallback
//////////////////////////////////////////////////////
IOReturn PESCallback(FWAVCTSDemuxerCallbackMessage msg, 
					 FWAVCTSDemuxerPESPacketRef fwavcTSDemuxerPESPacketRef, 
					 void *pRefCon,
					 FWAVCTSDemuxerRef fwavcTSDemuxerRef)
{
	switch (msg)
	{
		case kFWAVCTSDemuxerPESReceived:
			printf("MPEG PES Packet Received. Stream Type: %s PES Len: %6d, PID: 0x%04X, FireWire Time-Stamp: 0x%08X, System Time-Stamp 0x%016llX\n",
				   (FWAVCTSDemuxerPESPacketGetStreamType(fwavcTSDemuxerPESPacketRef) == kFWAVCTSDemuxerStreamTypeVideo) ? "Video" : "Audio",
				   FWAVCTSDemuxerPESPacketGetLen(fwavcTSDemuxerPESPacketRef),
				   FWAVCTSDemuxerPESPacketGetPid(fwavcTSDemuxerPESPacketRef),
				   FWAVCTSDemuxerPESPacketGetStartTimeStamp(fwavcTSDemuxerPESPacketRef),
				   FWAVCTSDemuxerPESPacketGetStartU64TimeStamp(fwavcTSDemuxerPESPacketRef));
			break;
			
		case kFWAVCTSDemuxerPacketError:
			printf("PESCallback: kTSDemuxerPacketError\n");
			break;
			
		case kFWAVCTSDemuxerDiscontinuity:
			printf("PESCallback: kTSDemuxerDiscontinuity\n");
			break;
			
		case kFWAVCTSDemuxerIllegalAdaptationFieldCode:
			printf("PESCallback: kTSDemuxerIllegalAdaptationFieldCode\n");
			break;
			
		case kFWAVCTSDemuxerBadAdaptationFieldLength:
			printf("PESCallback: kTSDemuxerBadAdaptationFieldLength\n");
			break;
			
		case kFWAVCTSDemuxerPESLargerThanAllocatedBuffer:
			printf("PESCallback: kTSDemuxerPESLargerThanAllocatedBuffer\n");
			break;
			
		case kFWAVCTSDemuxerFlushedPESBuffer:
			printf("PESCallback: kTSDemuxerFlushedPESBuffer\n");
			break;
			
		case kFWAVCTSDemuxerRescanningForPSI:
			printf("PESCallback: kTSDemuxerRescanningForPSI\n");
			break;
			
		default:
			printf("PESCallback: Invalid Message: 0x%08X\n",msg);
			break;
	}
	
	
	// Don't forget to release this PES buffer
	// Note that no matter what the msg is, this callback includes a PESPacketBuf
	FWAVCTSDemuxerReturnPESPacket(fwavcTSDemuxerPESPacketRef);
	
	return kIOReturnSuccess;
}

//////////////////////////////////////////////////////
// MyDVFramerCallback
//////////////////////////////////////////////////////
IOReturn MyDVFramerCallback(FWAVCDVFramerCallbackMessage msg, 
							FWAVCDVFramerFrameRef fwavcDVFramerFrameRef, 
							void *pRefCon, 
							FWAVCDVFramerRef fwavcDVFramerRef)
{
	switch (msg)
	{
		case kFWAVCDVFramerFrameReceivedSuccessfully:
			printf("DV Frame Received. Mode:0x%02X, Frame-Length: %6d, SYT: 0x%04X, FireWire Time-Stamp: 0x%08X, System Time-Stamp 0x%016llX\n",
				   FWAVCDVFramerFrameGetDVMode(fwavcDVFramerFrameRef),
				   FWAVCDVFramerFrameGetLen(fwavcDVFramerFrameRef),
				   FWAVCDVFramerFrameGetSYTTime(fwavcDVFramerFrameRef),
				   FWAVCDVFramerFrameGetStartTimeStamp(fwavcDVFramerFrameRef),
				   FWAVCDVFramerFrameGetStartU64TimeStamp(fwavcDVFramerFrameRef));
			break;
			
		case kFWAVCDVFramerPartialFrameReceived:
			printf("MyDVFramerCallback: kDVFramerPartialFrameReceived\n");
			break;
			
		case kFWAVCDVFramerCorruptFrameReceived:
			printf("MyDVFramerCallback: kDVFramerCorruptFrameReceived\n");
			break;
			
		case kFWAVCDVFramerNoFrameBufferAvailable:
			printf("MyDVFramerCallback: kDVFramerNoFrameBufferAvailable\n");
			break;
			
		case kFWAVCDVFramerNoMemoryForFrameBuffer:
			printf("MyDVFramerCallback: kDVFramerNoMemoryForFrameBuffer\n");
			break;
			
		default:
			printf("MyDVFramerCallback: Invalid Message: 0x%08X\n",msg);
			break;
	}
	
	// Don't forget to release this DV Frame, if needed
	// Note that only certain msg values include a passed-in DVFrame object
	if (fwavcDVFramerFrameRef)
		FWAVCDVFramerReturnDVFrame(fwavcDVFramerFrameRef);
	
	return kIOReturnSuccess;
}

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
	
	res = FWAVCDeviceAVCCommand(StreamReceiveDeviceRef,cmd, 8, response, &size);
	
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
			
			res = FWAVCDeviceAVCCommand(StreamReceiveDeviceRef,cmd, 8, response, &size);
			
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
