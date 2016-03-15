/*
	File:		CreateTSNavi.cpp
 
 Synopsis: This simple console mode application shows how to use a NaviFileCreator
           to create a .tsnavi file for an existing TS file, while monitoring progress
    
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

//////////////////////////////////////////////////////
// MyNaviFileCreatorProgressCallback
//////////////////////////////////////////////////////
IOReturn MyNaviFileCreatorProgressCallback(UInt32 percentageComplete, void *pRefCon)
{
	printf("Percentage Complete: %u\n",(unsigned int)percentageComplete);
	return kIOReturnSuccess;
}

//////////////////////////////////////////////////////
// Main
//////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
	// Locals
	IOReturn result = kIOReturnSuccess;
	NaviFileCreator *pCreator;
	MPEGNaviFileReader *pReader;
	UInt32 horizontalResolution;
	UInt32 verticalResolution;
	MPEGFrameRate frameRate; 
	UInt32 bitRate;
	UInt32 numFrames;
	UInt32 numTSPackets;

	 // Parse the command line
	if (argc != 2)
	{
		printf("Usage: %s inFileName\n",argv[0]);
		return -1;
	}
	
	// Create the NaviFileCreator object
	pCreator = new NaviFileCreator;
	if (!pCreator)
	{
		printf("Could not create NaviFileCreator object\n");
		return -1;
	}
	
	// Register for progress notifications
	pCreator->RegisterProgressNotificationCallback(MyNaviFileCreatorProgressCallback,nil);
	
	// Create the .tsnavi file
	printf("Creating .tsnavi file for TS File: %s\n",argv[1]);
	result = pCreator->CreateMPEGNavigationFileForTSFile(argv[1]);
		
	// If .tsnavi was created successfully, use a MPEGNaviFileReader to get the stream info
	if (result != kIOReturnSuccess)
		printf("Error creating .tsnavi file: 0x%08X\n",result);
	else
	{
		// Open the files with a navi file reader and get the stream info
		pReader = new MPEGNaviFileReader;
		if (pReader)
		{
			result = pReader->InitWithTSFile(argv[1]);
			if (result == kIOReturnSuccess)
			{
				result = pReader->GetStreamInfo(&horizontalResolution, &verticalResolution, &frameRate, &bitRate, &numFrames, &numTSPackets);
				if (result == kIOReturnSuccess)
				{
					printf("Stream Information:\n");
					printf("  Horizontal Resolution: %u\n",(unsigned int) horizontalResolution);
					printf("  Vertical Resolution:   %u\n",(unsigned int) verticalResolution);
					printf("  MPEGFrameRate:         ");
					switch (frameRate)
					{
						case MPEGFrameRate_23_976:
							printf("23.976\n");
							break;

						case MPEGFrameRate_24:
							printf("24.0\n");
							break;
							
						case MPEGFrameRate_25:
							printf("25.0\n");
							break;
							
						case MPEGFrameRate_29_97:
							printf("29.97\n");
							break;
							
						case MPEGFrameRate_30:
							printf("30.0\n");
							break;
							
						case MPEGFrameRate_50:
							printf("50.0\n");
							break;
							
						case MPEGFrameRate_59_94:
							printf("59.94\n");
							break;
							
						case MPEGFrameRate_60:
							printf("60.0\n");
							break;
							
						case MPEGFrameRate_Unknown:
						default:
							printf("Unknown\n");
							break;
					};
					printf("  Bit-Rate:              %u\n",(unsigned int) bitRate);
					printf("  Number of Frames:      %u\n",(unsigned int) numFrames);
				}
			}
			delete pReader;
		}
		printf("Success!\n");
	}
	
	delete pCreator;
	
	return result;
}	
