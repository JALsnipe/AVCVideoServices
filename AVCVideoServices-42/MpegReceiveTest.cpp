/*
	File:		MpegReceiveTest.cpp

   Synopsis: This is a simple console mode application that shows an example of using
   the code in the FireWireMPEG.framework for receiving MPEG2 transport streams
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

// Enable one of the following defines to get verbose info on every data callback, using the alternate (extended) callback prototype
// or to use the structured callback method (the 3rd mechanism now supported by the MPEG2Receiver for data delivery to clients)
// If both defines are commented-out, we use the default callback.
//#define kUsesExtendedDataCallback 1
#define kUsesStructuredDataCallback 1

#define kMaxCycleStructsInStructuredDataCallback kCyclesPerReceiveSegment

// Prototypes
void PrintLogMessage(char *pString);
IOReturn MpegReceiveCallback(UInt32 tsPacketCount, UInt32 **ppBuf,void *pRefCont);
void MessageReceivedProc(UInt32 msg, UInt32 param1, UInt32 param2,void *pRefCont);
IOReturn MyMPEG2NoDataProc(void *pRefCon);
IOReturn MyExtendedMpegReceiveCallback(UInt32 tsPacketCount, 
									   UInt32 **ppBuf, 
									   void *pRefCon, 
									   UInt32 isochHeader,
									   UInt32 cipHeader0,
									   UInt32 cipHeader1,
									   UInt32 fireWireTimeStamp);
IOReturn MyStructuredDataPushProc(UInt32 CycleDataCount, 
								  MPEGReceiveCycleData *pCycleData, 
								  void *pRefCon);
// Globals
FILE *outFile;
unsigned int packetCount = 0;

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
	MPEG2Receiver *receiver = nil;

	// Parse the command line
	if (argc != 4)
	{
		printf("Usage: %s isochChan outFileName captureTimeInSeconds\n",argv[0]);
		return -1;
	}
	isochChannel = atoi(argv[1]);
	captureTimeInSeconds = atoi(argv[3]);

	// Create the output file
	outFile = fopen(argv[2],"wb");
	if (outFile == nil)
	{
		printf("Unable to open output file: %s\n",argv[2]);
		return -1;
	}

	// Alloacate a string logger object and pass it our callback func
	StringLogger logger(PrintLogMessage);
	
	// Use the FireWireMPEG framework's helper function to create the
	// MPEG2Receiver object and dedicated real-time thread.
	// Note: Here we are relying on a number of default parameters.
	result = CreateMPEG2Receiver(&receiver,
							  MpegReceiveCallback,
							  nil,
							  MessageReceivedProc,
							  nil,
							  &logger);
	if (!receiver)
	{
		printf("Error creating MPEG2Receiver object: %d\n",result);
		return -1;
	}

	// Set the channel to receive on
	receiver->setReceiveIsochChannel(isochChannel);
	printf("Mpeg receive on channel: %d\n",isochChannel);

	// Set the expected isoch packet receive speed (only needed if allocating IRM bandwidth)
	receiver->setReceiveIsochSpeed(kFWSpeed100MBit);

	// Register a no-data notification callback, for noitification of 5 seconds of no received MPEG!
	receiver->registerNoDataNotificationCallback(MyMPEG2NoDataProc, nil, 5000);

#ifdef kUsesStructuredDataCallback
	receiver->registerStructuredDataPushCallback(MyStructuredDataPushProc, kMaxCycleStructsInStructuredDataCallback, nil);
#endif

#ifdef kUsesExtendedDataCallback
	// Register a no-data notification callback, for noitification of 5 seconds of no received MPEG!
	receiver->registerExtendedDataPushCallback(MyExtendedMpegReceiveCallback,nil);
#endif	
	
	// Start the receiver
	receiver->startReceive();
	
	// Monitor Progress every second until we're done
	for (unsigned int i=0;i<captureTimeInSeconds;i++)
	{
		usleep(kMicroSecondsPerSecond);
		printf("MPEG packets received: %d\n",packetCount);
	}

	// Stop the receiver
	if (receiver->transportState == kMpeg2ReceiverTransportRecording)
		receiver->stopReceive();
	
	// Delete the receiver object
	DestroyMPEG2Receiver(receiver);

	// We're done!
	printf("MpegReceiveTest complete!\n");
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
	printf("Message from MPEG Receiver: %d\n",(int) msg);

	if (msg == kMpeg2ReceiverAllocateIsochPort)
		printf("Channel: %d, Speed %d\n",(int)param2,(int)param1);

	return;
}

//////////////////////////////////////////////////////
// MpegReceiveCallback
//////////////////////////////////////////////////////
IOReturn MpegReceiveCallback(UInt32 tsPacketCount, UInt32 **ppBuf, void *pRefCont)
{
	unsigned int i;
	unsigned int cnt;

	// Increment packet count for progress display
	packetCount += tsPacketCount;

	// Write packets to file
	for (i=0;i<tsPacketCount;i++)
	{
		cnt = fwrite(ppBuf[i],1,kMPEG2TSPacketSize,outFile);
		if (cnt != kMPEG2TSPacketSize)
		{
			printf("Error writing to output file.\n");
			exit(-1);
		}
	}
	
	return 0;
}

#ifdef kUsesExtendedDataCallback
//////////////////////////////////////////////////////
// MpegReceiveCallback
//////////////////////////////////////////////////////
IOReturn MyExtendedMpegReceiveCallback(UInt32 tsPacketCount, 
									  UInt32 **ppBuf, 
									  void *pRefCon, 
									  UInt32 isochHeader,
									  UInt32 cipHeader0,
									  UInt32 cipHeader1,
									  UInt32 fireWireTimeStamp)
{

#if 0	
	printf("tsPacketCount: %u, isochHeader=0x%08X, cip0=0x%08X, cip1=0x%08X, fireWireTimeStamp=0x%08X\n",
		   (unsigned int) tsPacketCount,
		   (unsigned int) isochHeader,
		   (unsigned int) cipHeader0,
		   (unsigned int) cipHeader1,
		   (unsigned int) fireWireTimeStamp);
#endif
	
	return MpegReceiveCallback(tsPacketCount,ppBuf,pRefCon);
}
#endif

#ifdef kUsesStructuredDataCallback
//////////////////////////////////////////////////////
// MyStructuredDataPushProc
//////////////////////////////////////////////////////
IOReturn MyStructuredDataPushProc(UInt32 CycleDataCount, MPEGReceiveCycleData *pCycleData, void *pRefCon)
{

#if 0	
	static UInt64 savedNanoSecondsTimeStamp = 0LL;
	
	printf("MyStructuredDataPushProc: CycleDataCount=%5d, First cycle's time: %10.10f, delta: %10.10f\n",
		   (int)CycleDataCount,
		   pCycleData[0].nanoSecondsTimeStamp/1000000000.0,
		   (pCycleData[0].nanoSecondsTimeStamp - savedNanoSecondsTimeStamp)/1000000000.0);
	
	savedNanoSecondsTimeStamp = pCycleData[0].nanoSecondsTimeStamp;
#endif
	
	for (UInt32 i=0;i<CycleDataCount;i++)
	{
		MpegReceiveCallback(pCycleData[i].tsPacketCount,(UInt32**)&pCycleData[i].pBuf,pRefCon);
	}
		 
	return 0;
}
#endif
//////////////////////////////////////////////////////
// MyMPEG2NoDataProc
//////////////////////////////////////////////////////
IOReturn MyMPEG2NoDataProc(void *pRefCon)
{
	printf("MyMPEG2NoDataProc Called\n");
	
	return 0;
}