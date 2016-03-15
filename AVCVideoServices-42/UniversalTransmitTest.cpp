/*
	File:		UniversalTransmitTest.cpp

   Synopsis: This is a simple console mode application that shows an example of using
   the UniversalTransmitter object.

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

// Defines
#define kMicroSecondsPerSecond 1000000

// Prototypes
void PrintLogMessage(char *pString);
IOReturn MyUniversalTransmitCallback(UniversalTransmitterCycleInfo *pCycleInfo, UniversalTransmitter *pTransmitter, void *pRefCon);
void MessageReceivedProc(UInt32 msg, UInt32 param1, UInt32 param2, void *pRefCon);

// Globals
unsigned int packetCount = 0;
bool transmitDone = false;
UInt32 packetSize;
UInt32 fireWireSpeedCode;

//////////////////////////////////////////////////////
//
// Main
//
//////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
	// Local Vars
    IOReturn result = kIOReturnSuccess ;
	UniversalTransmitter *transmitter = nil;
	unsigned int isochChannel;
	
	// Parse the command line
	if (argc != 4)
	{
		printf("Usage: %s isochChan packetSize fireWireSpeedCode\n",argv[0]);
		printf("(Speed codes 0=S100, 1=S200, 2=S400, 3=S800\n");

		return -1;
	}
	isochChannel = atoi(argv[1]);
	packetSize = atoi(argv[2]);
	fireWireSpeedCode = atoi(argv[3]);

	if (fireWireSpeedCode > 3)
		fireWireSpeedCode = 0;
	
	// Make sure isoch packet size is at least 4 (since we're storing a 32-bit number in the first quad of each packet buffer)
	if (packetSize < 4)
		packetSize = 4;
	
	switch (fireWireSpeedCode)
	{
		case 0:
			printf("Isoch speed: S100\n");
			if (packetSize > 1024)
			{
				printf("Limiting isoch packet size to 1024 at S100\n");
				packetSize = 1024;
			}
			break;
		
		case 1:
			printf("Isoch speed: S200\n");
			if (packetSize > 2048)
			{
				printf("Limiting isoch packet size to 2048 at S200\n");
				packetSize = 2048;
			}
			break;
			
		case 2:
			printf("Isoch speed: S400\n");
			if (packetSize > 4096)
			{
				printf("Limiting isoch packet size to 4096 at S400\n");
				packetSize = 4096;
			}
			break;
			
		case 3:
		default:
			printf("Isoch speed: S800\n");
			if (packetSize > 8192)
			{
				printf("Limiting isoch packet size to 8192 at S800\n");
				packetSize = 8192;
			}
			break;
			
	};
	
	printf("Isoch packet size: %d\n",packetSize);
	
	// Alloacate a string logger object and pass it our callback func
	StringLogger logger(PrintLogMessage);

	// Note: Here we are relying on a number of default parameters.
	result = CreateUniversalTransmitter(&transmitter,
										MyUniversalTransmitCallback,
										nil,
										MessageReceivedProc,
										nil,
										&logger,
										nil,
										kUniversalTransmitterCyclesPerSegment,
										kUniversalTransmitterNumSegments,
										(kUniversalTransmitterCyclesPerSegment*kUniversalTransmitterNumSegments*packetSize),
										true,
										packetSize,
										13);	// 13 bits means start on any 1-second boundary (i.e. ignoring seconds bits, but when cycle number = 0)
	if (!transmitter)
	{
		printf("Error creating UniversalTransmitter object: %d\n",result);
		return -1;
	}

	// Set the channel to transmit on
	transmitter->setTransmitIsochChannel(isochChannel);
	printf("Isoch transmit on channel: %d\n",isochChannel);

	// Set the isoch packet speed
	transmitter->setTransmitIsochSpeed((IOFWSpeed)fireWireSpeedCode);

	// Initialize the data buffers with a 32-bit buffer num value at the beginning of each packet buf.
	UInt8 *pClientBuffer = transmitter->getClientBufferPointer(); 
	for (UInt32 i=0; i< (kUniversalTransmitterCyclesPerSegment*kUniversalTransmitterNumSegments); i++)
	{
		UInt32 bufferByteOffset = i*packetSize;
		UInt32 *pBufWord = (UInt32*) &pClientBuffer[bufferByteOffset];
		*pBufWord = EndianU32_NtoB(i);
	}
	
	// Start the transmitter
	result = transmitter->startTransmit();
	if (result != kIOReturnSuccess)
	{
		printf("Error starting UniversalTransmitter object: %d\n",result);
		return -1;
	}
	
	// Monitor Progress every second until we're done
	while (transmitDone == false)
	{
		usleep(kMicroSecondsPerSecond);
	}

	// Stop the transmitter
	transmitter->stopTransmit();
	
	// Delete the transmitter object
	DestroyUniversalTransmitter(transmitter);

	// We're done!
	printf("UniversalTransmitTest complete!\n");
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
void MessageReceivedProc(UInt32 msg, UInt32 param1, UInt32 param2, void *pRefCon)
{
	printf("Message from Universal Transmitter: %d\n",(int) msg);

	if (msg == kUniversalTransmitterAllocateIsochPort)
		printf("Channel: %d, Speed %d\n",(int)param2,(int)param1);

	return;
}
//////////////////////////////////////////////////////
// MyUniversalTransmitCallback
//////////////////////////////////////////////////////
IOReturn MyUniversalTransmitCallback(UniversalTransmitterCycleInfo *pCycleInfo, UniversalTransmitter *pTransmitter, void *pRefCon)
{
#if 0	
	AbsoluteTime currentUpTime;
	Nanoseconds currentUpTimeInNanoSeconds;
	UInt64 currentUpTimeInNanoSecondsU64;
	currentUpTime = UpTime();
	currentUpTimeInNanoSeconds = AbsoluteToNanoseconds(currentUpTime);
	currentUpTimeInNanoSecondsU64 = ((UInt64) currentUpTimeInNanoSeconds.hi << 32) | currentUpTimeInNanoSeconds.lo;
	printf("MyUniversalTransmitCallback: Time-Stamp: 0x%08X, seconds=%02d cycles=%04d (dclProgramRunning=%d) transmitTimeInNanoSeconds=0x%016llX secUntilTransmit=%10.8f\n",
		   pCycleInfo->expectedTransmitCycleTime,
		   ((pCycleInfo->expectedTransmitCycleTime & 0xFE000000) >> 25), 
		   ((pCycleInfo->expectedTransmitCycleTime & 0x1fff000) >> 12), 
		   pCycleInfo->dclProgramRunning,
		   pCycleInfo->transmitTimeInNanoSeconds,
		   ((pCycleInfo->transmitTimeInNanoSeconds-currentUpTimeInNanoSecondsU64)/1000000000.0));
#endif
	
	// Set the buffer ranges for this cycle
	pCycleInfo->numRanges = 1;
	pCycleInfo->ranges[0].address = (IOVirtualAddress) pTransmitter->getClientBufferPointer() + (pCycleInfo->index*packetSize);
	pCycleInfo->ranges[0].length = (IOByteCount) packetSize ;
	
	// Put the expectedTransmitCycleTime value into the first quadlet of the buffer
	UInt32 *pBufWord = (UInt32*) pCycleInfo->ranges[0].address;
	*pBufWord = EndianU32_NtoB(pCycleInfo->expectedTransmitCycleTime);
	
	// Set sy and tag fields for this cycle
	pCycleInfo->sy = 0;
	pCycleInfo->tag = 0;
	
	return kIOReturnSuccess;
}
