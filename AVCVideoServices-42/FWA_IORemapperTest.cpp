/*
	File:		FWA_IORemapperTest.cpp
 
 Synopsis: This command-line test app shows a use of the FWA_IORemapper object.
 
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

//////////////////////////////////////////////////////
//
// Main
//
//////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
	// Local Vars
    IOReturn result = kIOReturnSuccess ;
	UInt8 isochInChannel,isochOutChannel;
	
	// Parse the command line
	if (argc != 3)
	{
		printf("Usage: %s isochInChan isochOutChan\n",argv[0]);
		return -1;
	}
	isochInChannel = atoi(argv[1]);
	isochOutChannel = atoi(argv[2]);
	
	UInt8 sourceSubStreamIndexes[18] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17};
	//UInt8 sourceSubStreamIndexes[7] = {0,1,2,3,4,5,6};
	//UInt8 sourceSubStreamIndexes[2] = {4,5};
	
	FWA_IORemapper *pRemapper = new FWA_IORemapper(isochInChannel,isochOutChannel,18,sourceSubStreamIndexes,kFWSpeed400MBit);
	if (!pRemapper)
	{
		printf("Error: Unable to allocate FWA_IORemapper\n");
		return -1;
	}
	
	result = pRemapper->start();
	if (result != kIOReturnSuccess)
	{
		printf("Error: Could not start the FWA_IORemapper: 0x%08X\n",result);
		return -1;
	}
	
	// Monitor Progress every second until we're done
	for (;;)
	{
		// Not currently doing anything here in this app!
		usleep(kMicroSecondsPerSecond);
	}
	
	result = pRemapper->stop();
	if (result != kIOReturnSuccess)
	{
		printf("Error: Could not stop the FWA_IORemapper: 0x%08X\n",result);
		return -1;
	}
	
	delete pRemapper;
	
	return result;
}	

