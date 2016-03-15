/*
	File:		DVTransmitToDevice.cpp
 
 Synopsis: This is a simple test app for testing AVCDevice streaming helper-functions.
 
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

// Prototypes
IOReturn MyAVCDeviceControllerNotification(AVCDeviceController *pAVCDeviceController, void *pRefCon, AVCDevice* pDevice);
IOReturn MyAVCDeviceMessageNotification(AVCDevice *pAVCDevice,
										natural_t messageType,
										void * messageArgument,
										void *pRefCon);
void PrintLogMessage(char *pString);
IOReturn FramePullProc (UInt32 *pFrameIndex, void *pRefCon);
IOReturn FrameReleaseProc (UInt32 frameIndex, void *pRefCon);
void MessageReceivedProc(UInt32 msg, UInt32 param1, UInt32 param2, void *pRefCon);
void initialzeFrameQueue(void);

// Globals
StringLogger logger(PrintLogMessage);
AVCDeviceController *pAVCDeviceController;
UInt32 *pFrameQueue;
UInt32 numFrames;
FILE *inFile;
DVTransmitFrame* pFrameQueueHead;
bool transmitDone = false;
UInt32 frameTransmittedCount = 0;
bool loopFileMode = false;
AVCDeviceStream* pAVCDeviceStream = nil;
AVCDevice *pTargetAVCDevice = nil;

//////////////////////////////////////////////////////
//
// Main
//
//////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
	IOReturn result = kIOReturnSuccess ;

	// Parse the command line
	if (argc != 2)
	{
		printf("Usage: %s inFileName\n",argv[0]);
		return -1;
	}
		
	// Open the input file
	inFile = fopen(argv[1],"rb");
	if (inFile == nil)
	{
		printf("Unable to open input file: %s\n",argv[1]);
		return -1;
	}
	
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
			printf("DV frames transmitted: %d\n",(int)frameTransmittedCount);
			if (transmitDone == true)
				break;
		}
	}
	
	printf ("\n  Closing Device: %s\n",(pTargetAVCDevice->deviceName != nil) ? pTargetAVCDevice->deviceName : "Unknown Device Name");
	if (pTargetAVCDevice != nil)
	{
		pTargetAVCDevice->StopAVCDeviceStream(pAVCDeviceStream);
		pTargetAVCDevice->DestroyAVCDeviceStream(pAVCDeviceStream);
		pTargetAVCDevice = nil;
	}
	if(pTargetAVCDevice != nil)
	{
		pTargetAVCDevice->closeDevice();
		if (result != kIOReturnSuccess)
			printf ("  Error Closing Device: 0x%08X\n",result);
		pAVCDeviceStream = nil;
	}
	
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
		if ((pAVCDevice->isOpened() == false) && (pAVCDevice->isAttached == true) && (pAVCDevice->isDVDevice == true) && (pAVCDeviceStream == nil))
		{
			printf ("  Opening Device: %s\n",(pAVCDevice->deviceName != nil) ? pAVCDevice->deviceName : "Unknown Device Name");
			result = pAVCDevice->openDevice(MyAVCDeviceMessageNotification, nil);
			if (result != kIOReturnSuccess)
				printf ("  Error Opening Device: 0x%08X\n",result);
			else
			{
				pTargetAVCDevice = pAVCDevice;

				// Create a DV Transmit stream for this device
				pAVCDeviceStream = pAVCDevice->CreateDVTransmitterForDevicePlug(0,
																	FramePullProc,
																	nil,
																	FrameReleaseProc,
																	nil,
																	MessageReceivedProc,
																	nil,
																	&logger,
																	kCyclesPerDVTransmitSegment,
																	kNumDVTransmitSegments,
																	0x00,
																	8);
				if (pAVCDeviceStream == nil)
				{
					result = pAVCDevice->closeDevice();
					if (result != kIOReturnSuccess)
						printf ("  Error Closing Device: 0x%08X\n",result);
					pTargetAVCDevice = nil;
					pAVCDeviceStream = nil;
				}
				else
				{
					printf("  Starting DV Transmit to device\n");
					numFrames = pAVCDeviceStream->pDVTransmitter->getNumFrames();
					result = pAVCDevice->StartAVCDeviceStream(pAVCDeviceStream);
				}
			}
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
			
		default:
			printf("Unknown Message\n");
			break;
	};
	
	switch (messageType)
	{
		case kIOMessageServiceIsRequestingClose:
			printf ("\n  Closing Disconnected Device: %s\n",(pAVCDevice->deviceName != nil) ? pAVCDevice->deviceName : "Unknown Device Name");
			pAVCDevice->closeDevice();
			if (result != kIOReturnSuccess)
				printf ("  Error Closing Device: 0x%08X\n",result);
			pTargetAVCDevice = nil;
			pAVCDeviceStream = nil;
			break;
			
		default:
			break;
	};
	
	return kIOReturnSuccess ;
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
		pFrame = pAVCDeviceStream->pDVTransmitter->getFrame(i);
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
			printf("Channel: %d, Speed: %d\n",(int)param2,(int)param1);
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
	pFrame = pAVCDeviceStream->pDVTransmitter->getFrame(frameIndex);
	pFrame->pNext = pFrameQueueHead;
	pFrameQueueHead = pFrame;
	return kIOReturnSuccess;
}

