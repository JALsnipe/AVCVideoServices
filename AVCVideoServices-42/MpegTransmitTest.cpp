/*
	File:		MpegTransmitTest.cpp

   Synopsis: This is a simple console mode application that shows an example of using
   the code in the FireWireMPEG.framework for transmitting MPEG2 transport streams
   over firewire.

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

//#define kUsesTimeStampInfoDataPullProc 1


// Prototypes
void PrintLogMessage(char *pString);
IOReturn MpegTransmitCallback(UInt32 **ppBuf, bool *pDiscontinuityFlag, void *pRefCon);
void MessageReceivedProc(UInt32 msg, UInt32 param1, UInt32 param2, void *pRefCon);

#ifdef kUsesTimeStampInfoDataPullProc
void MyMPEG2TransmitterTimeStampProc(UInt64 pcr, UInt64 transmitTimeInNanoSeconds, void *pRefCon);
#endif

// Globals
FILE *inFile;
unsigned int packetCount = 0;
bool transmitDone = false;
char tsPacketBuf[kMPEG2TSPacketSize];

//////////////////////////////////////////////////////
//
// Main
//
//////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
	// Local Vars
    IOReturn result = kIOReturnSuccess ;
	MPEG2Transmitter *transmitter = nil;
	unsigned int isochChannel;
	
	// Parse the command line
	if (argc != 3)
	{
		printf("Usage: %s isochChan inFileName\n",argv[0]);
		return -1;
	}
	isochChannel = atoi(argv[1]);

	// Open the input file
	inFile = fopen(argv[2],"rb");
	if (inFile == nil)
	{
		printf("Unable to open input file: %s\n",argv[2]);
		return -1;
	}

	// Alloacate a string logger object and pass it our callback func
	StringLogger logger(PrintLogMessage);

	// Use the FireWireMPEG framework's helper function to create the
	// MPEG2Transmitter object and dedicated real-time thread.
	// Note: Here we are relying on a number of default parameters.
	result = CreateMPEG2Transmitter(&transmitter,
								 MpegTransmitCallback,
								 nil,
								 MessageReceivedProc,
								 nil,
								 &logger);
	if (!transmitter)
	{
		printf("Error creating MPEG2Transmitter object: %d\n",result);
		return -1;
	}

#ifdef kUsesTimeStampInfoDataPullProc
	// Register a handler to get time-stamp callbacks.
	transmitter->registerTimeStampCallback(MyMPEG2TransmitterTimeStampProc,nil);
#endif
	
	// Set the channel to transmit on
	transmitter->setTransmitIsochChannel(isochChannel);
	printf("Mpeg transmit on channel: %d\n",isochChannel);

	// Set the isoch packet speed
	transmitter->setTransmitIsochSpeed(kFWSpeed100MBit);

	// Start the transmitter
	transmitter->startTransmit();
	
	// Monitor Progress every second until we're done
	while (transmitDone == false)
	{
		usleep(kMicroSecondsPerSecond);
#ifndef kUsesTimeStampInfoDataPullProc
		printf("MPEG packets transmitted: %d\n",packetCount);
#endif
	}

	// Stop the transmitter
	transmitter->stopTransmit();
	
	// Delete the transmitter object
	DestroyMPEG2Transmitter(transmitter);

	// We're done!
	printf("MpegTransmitTest complete!\n");
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
	printf("Message from MPEG Transmitter: %d\n",(int) msg);

	if (msg == kMpeg2TransmitterAllocateIsochPort)
		printf("Channel: %d, Speed %d\n",(int)param2,(int)param1);

	return;
}
//////////////////////////////////////////////////////
// MpegTransmitCallback
//////////////////////////////////////////////////////
IOReturn MpegTransmitCallback(UInt32 **ppBuf, bool *pDiscontinuityFlag, void *pRefCon)
{
	static bool flushMode = false;
	static unsigned int flushCnt = 0;
	unsigned int cnt;
	IOReturn result = 0;

	// Signal no discontinuity
	*pDiscontinuityFlag = false;

	if (flushMode == false)
	{
		// Read the next TS packet from the input file
		cnt = fread(tsPacketBuf,1,kMPEG2TSPacketSize,inFile);
		if (cnt != kMPEG2TSPacketSize)
		{
			flushMode = true;
			result = -1;	// Causes a CIP only cycle to be filled
		}
		else
		{
			packetCount += 1;
			*ppBuf = (UInt32*) tsPacketBuf;
		}
	}
	else
	{
		// This code runs the transmitter for enough additional cycles to 
		// flush all the MPEG data from the DCL buffers 
		if (flushCnt > (kCyclesPerTransmitSegment * kNumTransmitSegments))
			transmitDone = true;
		else
			flushCnt += 1;
		result = -1;	// Causes a CIP only cycle to be filled
	}

	return result;
}

#ifdef kUsesTimeStampInfoDataPullProc
//////////////////////////////////////////////////////
// MyMPEG2TransmitterTimeStampProc
//////////////////////////////////////////////////////
void MyMPEG2TransmitterTimeStampProc(UInt64 pcr, UInt64 transmitTimeInNanoSeconds, void *pRefCon)
{
	static UInt64 lastPCR, lastTransmitTime;
	
	AbsoluteTime currentUpTime;
	Nanoseconds currentUpTimeInNanoSeconds;
	UInt64 currentUpTimeInNanoSecondsU64;

	currentUpTime = UpTime();
	currentUpTimeInNanoSeconds = AbsoluteToNanoseconds(currentUpTime);
	currentUpTimeInNanoSecondsU64 = ((UInt64) currentUpTimeInNanoSeconds.hi << 32) | currentUpTimeInNanoSeconds.lo;
	
	printf("CurrentUpTime: %f  PCRTime: %f  TransmitTime: %f  PCRDelta: %f  TransmitTimeDelta: %f  DifferenceDelta: %f\n",
		   currentUpTimeInNanoSecondsU64/1000000000.0,
		   ((1.0/27000000.0) * pcr),
		   (transmitTimeInNanoSeconds / 1000000000.0),
		   ((1.0/27000000.0) * (pcr - lastPCR)),
		   ((transmitTimeInNanoSeconds - lastTransmitTime) / 1000000000.0),
		   ((1.0/27000000.0) * (pcr - lastPCR)) - ((transmitTimeInNanoSeconds - lastTransmitTime) / 1000000000.0));
	
	lastPCR = pcr;
	lastTransmitTime = transmitTimeInNanoSeconds;
}
#endif

