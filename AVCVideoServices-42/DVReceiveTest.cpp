/*
	File:		DVReceiveTest.cpp
 
 Synopsis: This is a simple command-line app for testing the DVReceiver.
 
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

// Globals
DVReceiver *receiver = nil;
FILE *outFile;
UInt32 framesReceived = 0;

// Prototypes
void PrintLogMessage(char *pString);
IOReturn MyFrameReceivedProc (DVFrameReceiveMessage msg, DVReceiveFrame* pFrame, void *pRefCon);

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
	UInt8 dvMode;

	// Parse the command line
	if (argc != 5)
	{
		printf("Usage: %s isochChan dvMode outFileName captureTimeInSeconds\n",argv[0]);
		return -1;
	}
	isochChannel = atoi(argv[1]);
	dvMode = atoi(argv[2]);

	captureTimeInSeconds = atoi(argv[4]);

	// Create the output file
	outFile = fopen(argv[3],"wb");
	if (outFile == nil)
	{
		printf("Unable to open output file: %s\n",argv[3]);
		return -1;
	}
	
	// Alloacate a string logger object and pass it our callback func
	StringLogger logger(PrintLogMessage);

	result = CreateDVReceiver(&receiver,
						   MyFrameReceivedProc,
						   nil,
						   nil,
						   nil,
						   &logger,
						   nil,
						   kCyclesPerDVReceiveSegment*2,
						   kNumDVReceiveSegments,
						   dvMode,
						   kDVReceiveNumFrames,
						   false);
	if (!receiver)
	{
		printf("Error creating DVReceiver object: %d\n",result);
		return -1;
	}

	// Set the channel to receive on
	receiver->setReceiveIsochChannel(isochChannel);
	printf("DV receive on channel: %d\n",isochChannel);

	// Set the expected isoch packet receive speed (only needed if allocating IRM bandwidth)
	receiver->setReceiveIsochSpeed(kFWSpeed100MBit);

	// Start the receiver
	receiver->startReceive();

	// Monitor Progress every second until we're done
	for (unsigned int i=0;i<captureTimeInSeconds;i++)
	{
		usleep(kMicroSecondsPerSecond);
		printf("DV frames received: %d\n",(int) framesReceived);
	}

	// Stop the receiver
	if (receiver->transportState == kDVReceiverTransportRecording)
		receiver->stopReceive();

	// Delete the receiver object
	DestroyDVReceiver(receiver);

	// We're done!
	printf("DVReceiveTest complete!\n");
	return result;
}

//////////////////////////////////////////////////////
// FrameReceivedProc
//////////////////////////////////////////////////////
IOReturn MyFrameReceivedProc (DVFrameReceiveMessage msg, DVReceiveFrame* pFrame, void *pRefCon)
{
	UInt32 cnt;

	if (msg == kDVFrameReceivedSuccessfully)
	{
		framesReceived += 1;

		cnt = fwrite(pFrame->pFrameData,1,pFrame->frameLen,outFile);
		if (cnt != pFrame->frameLen)
		{
			printf("Error writing to dv output file.\n");
			exit(-1);
		}
	}
	
	return kIOReturnError;
}


//////////////////////////////////////////////////////
// PrintLogMessage
//////////////////////////////////////////////////////
void PrintLogMessage(char *pString)
{
	printf("%s",pString);
}
