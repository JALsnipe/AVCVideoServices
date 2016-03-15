/*
	File:		DVTransmitTest.cpp
 
 Synopsis: This is a simple command-line app for testing the DVTransmitter.
 
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

void PrintLogMessage(char *pString);
IOReturn FramePullProc (UInt32 *pFrameIndex, void *pRefCon);
IOReturn FrameReleaseProc (UInt32 frameIndex, void *pRefCon);
void MessageReceivedProc(UInt32 msg, UInt32 param1, UInt32 param2, void *pRefCon);
void initialzeFrameQueue(void);

UInt32 *pFrameQueue;
UInt32 numFrames;
FILE *inFile;
DVTransmitFrame* pFrameQueueHead;
DVTransmitter *xmitter = nil;
bool transmitDone = false;
UInt32 frameTransmittedCount = 0;
bool loopFileMode = false;

//////////////////////////////////////////////////////
//
// Main
//
//////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
	// Local Vars
    IOReturn result = kIOReturnSuccess ;
	unsigned int isochChannel;
	unsigned int isochSpeedCode;
	UInt8 dvMode;

	// Parse the command line
	if (argc < 5)
	{
		printf("Usage: %s isochChan isochSpeedCode dvMode inFileName\n",argv[0]);
		return -1;
	}
	if (argc > 5)
		loopFileMode = true;
	
	isochChannel = atoi(argv[1]);
	isochSpeedCode = atoi(argv[2]);
	if (isochSpeedCode > 2)
	{
		printf("Isoch speed code should be 0 (100mbps),  1 (200mpbs), or 3(400mbps)\n");
		return -1;
	}
	dvMode = atoi(argv[3]);

	// Open the input file
	inFile = fopen(argv[4],"rb");
	if (inFile == nil)
	{
		printf("Unable to open input file: %s\n",argv[4]);
		return -1;
	}
	
	// Alloacate a string logger object and pass it our callback func
	StringLogger logger(PrintLogMessage);

	result = CreateDVTransmitter(&xmitter,
							  FramePullProc,
							  nil,
							  FrameReleaseProc,
							  nil,
							  MessageReceivedProc,
							  nil,
							  &logger,
							  nil,
							  kCyclesPerDVTransmitSegment,
							  kNumDVTransmitSegments,
							  dvMode,
							  8,
							  false);
	if (!xmitter)
	{
		printf("Error creating DVTransmitter object: %d\n",result);
		return -1;
	}

	numFrames = xmitter->getNumFrames();

	// Set the channel to transmit on
	xmitter->setTransmitIsochChannel(isochChannel);
	printf("DV transmit on channel: %d\n",isochChannel);
	
	// Set the isoch packet speed
	xmitter->setTransmitIsochSpeed((IOFWSpeed)isochSpeedCode);

	// Start the transmitter
	xmitter->startTransmit();

	// Monitor Progress every second until we're done
	while (transmitDone == false)
	{
		usleep(kMicroSecondsPerSecond);
		printf("DV frames transmitted: %d\n",(int)frameTransmittedCount);
	}

	// Stop the transmitter
	xmitter->stopTransmit();

	// Delete the transmitter object
	DestroyDVTransmitter(xmitter);

	// We're done!
	printf("DVTransmitTest complete!\n");
	return result;
}

//////////////////////////////////////////////////////
// initialzeFrameQueue
//////////////////////////////////////////////////////
void initialzeFrameQueue(void)
{
	UInt32 i;
	DVTransmitFrame* pFrame;

	pFrameQueueHead = nil;
	for (i=0;i<numFrames;i++)
	{
		pFrame = xmitter->getFrame(i);
		pFrame->pNext = pFrameQueueHead;
		pFrameQueueHead = pFrame;
	}
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
	printf("Message from DV Transmitter: %d\n",(int) msg);

	switch (msg)
	{
		case kDVTransmitterAllocateIsochPort:
			printf("Channel: %d, Speed %d\n",(int)param2,(int)param1);
			break;

		case kDVTransmitterPreparePacketFetcher:
			initialzeFrameQueue();
			break;

		default:
			break;
			
	};

	return;
}

//////////////////////////////////////////////////////
// FramePullProc
//////////////////////////////////////////////////////
IOReturn FramePullProc (UInt32 *pFrameIndex, void *pRefCon)
{
	static bool flushMode = false;
	static unsigned int flushCnt = 0;
	unsigned int cnt;
	IOReturn result = 0;
	DVTransmitFrame* pFrame;

	if (flushMode == false)
	{
		if (pFrameQueueHead != nil)
		{
			pFrame = pFrameQueueHead;
			pFrameQueueHead = pFrame->pNext;

			// Read the next TS packet from the input file
			cnt = fread(pFrame->pFrameData,1,pFrame->frameLen,inFile);
			if (cnt != pFrame->frameLen)
			{
				if (loopFileMode == false)
				{
					flushMode = true;
					FrameReleaseProc(pFrame->frameIndex,pRefCon);
					result = -1;	// Causes The previous frame to be used
				}
				else
				{
					// Rewind file
					rewind(inFile);

					// Read again
					cnt = fread(pFrame->pFrameData,1,pFrame->frameLen,inFile);
					if (cnt != pFrame->frameLen)
					{
							flushMode = true;
							FrameReleaseProc(pFrame->frameIndex,pRefCon);
							result = -1;	// Causes The previous frame to be used
					}
					else
					{
						*pFrameIndex = pFrame->frameIndex;
						frameTransmittedCount += 1;
					}
				}
			}
			else
			{
				*pFrameIndex = pFrame->frameIndex;
				frameTransmittedCount += 1;
			}
		}
		else
		{
			printf("DVTransmitTest: Repeating previous frame because no frames on framequeue!\n");
			result = -1;	// Causes The previous frame to be used
		}
	}
	else
	{
		// This code runs the transmitter for enough additional cycles to
		// flush all the DV data from the DCL buffers. A pretty sloppy way
		// of doing it, though.
		if (flushCnt > (numFrames-2))
			transmitDone = true;
		else
			flushCnt += 1;
		result = -1;	// Causes a the previous frame to be used
	}

	return result;
}

//////////////////////////////////////////////////////
// FrameReleaseProc
//////////////////////////////////////////////////////
IOReturn FrameReleaseProc (UInt32 frameIndex, void *pRefCon)
{
	DVTransmitFrame* pFrame;
	pFrame = xmitter->getFrame(frameIndex);
	pFrame->pNext = pFrameQueueHead;
	pFrameQueueHead = pFrame;
	return kIOReturnSuccess;
}

