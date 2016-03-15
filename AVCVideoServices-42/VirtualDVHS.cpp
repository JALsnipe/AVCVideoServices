/*
	File:		VirtualDVHS.cpp
 
 Synopsis: This is a console mode application that shows an example of using
 the code in AVS for emulating a DVHS VCR.
 
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

// Enable the following #define to use an monitor or tuner AVCDevice
// for initializing the Virtual tape subunt.
// 
// Disabling the following #define will result in the Virtual tape subunit
// being installed on the first FireWire Local-Node it can find in the registry.
//
//#define kFindAVCDevice 1 

#ifdef kFindAVCDevice	
IOReturn MyAVCDeviceControllerNotification(AVCDeviceController *pAVCDeviceController, void *pRefCon, AVCDevice* pDevice);
IOReturn MyAVCDeviceMessageNotification (class AVCDevice *pAVCDevice, natural_t messageType, void * messageArgument, void *pRefCon);
#endif

//////////////////////////////////////////////////////
// Main
//////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
	// Local Vars
    IOReturn result ;
	AVCDevice *pAVCDevice = nil;

#ifdef kFindAVCDevice	
	AVCDeviceController *pAVCDeviceController = nil;
	UInt32 i;
	bool deviceFound = false;
#endif
	
	// Parse the command line
	if (argc != 2)
	{
		printf("Usage: %s inFileName\n",argv[0]);
		return -1;
	}

#ifdef kFindAVCDevice	
	result = CreateAVCDeviceController(&pAVCDeviceController,MyAVCDeviceControllerNotification);
	if (!pAVCDeviceController)
	{
		printf("VirtualDVHS: Error creating VirtualMPEGTapePlayerRecorder\n");
		return -1;
	}

	// Search through all the connected devices to find one we're interested in
	for (i=0;i<(UInt32)CFArrayGetCount(pAVCDeviceController->avcDeviceArray);i++)
	{
		pAVCDevice = (AVCDevice*) CFArrayGetValueAtIndex(pAVCDeviceController->avcDeviceArray,i);

		if ((pAVCDevice->hasMonitorOrTunerSubunit) && (pAVCDevice->isAttached))
		{
			// Found our device, try to open it
			result = pAVCDevice->openDevice(MyAVCDeviceMessageNotification, nil);
			if (result != kIOReturnSuccess)
				continue;	// Couldn't open this one, so keep looking
			else
			{
				deviceFound = true;
				break;	// We found our device, and successfully opened it, so we can move on now
			}
		}
	}
	
	if (!deviceFound)
	{
		printf("VirtualDVHS: No external AV/C Monitor or Tuner found!\n");
		DestroyAVCDeviceController(pAVCDeviceController);
		return -1;
	}
	
#endif	
	
	// Create the VirtualMPEGTapePlayerRecorder
	VirtualMPEGTapePlayerRecorder *pPlayer = new VirtualMPEGTapePlayerRecorder;
	if (!pPlayer)
	{
		printf("VirtualDVHS: Error creating VirtualMPEGTapePlayerRecorder\n");
		return -1;
	}
	
	// Enable looping
	pPlayer->setLoopModeState(true);
	
	// Initialize the VirtualMPEGTapePlayerRecorder
	printf("VirtualDVHS: Initializing VirtualMPEGTapePlayerRecorder\n");
	result = pPlayer->initWithFileName(argv[1],pAVCDevice);
	if (result != kIOReturnSuccess)
	{
		printf("VirtualDVHS: Error initializing VirtualMPEGTapePlayerRecorder: 0x%08X\n",result);
		return -1;
	}
	
#ifdef kFindAVCDevice	
	// At this point, unless we want notification that our tuner/monitor device has been
	// disconnected (which we currently don't), we can go ahead and close the AVCDevice
	pAVCDevice->closeDevice();
#endif	

	// TODO: Just hang here for now
	for (;;);	
	
	// Delete the VirtualMPEGTapePlayerRecorder
	delete pPlayer;

#ifdef kFindAVCDevice	
	DestroyAVCDeviceController(pAVCDeviceController);
#endif	
	
	// We're done!
	printf("VirtualDVHS complete!\n");
	return result;
}

#ifdef kFindAVCDevice	
//////////////////////////////////////////////////////////////////////
// MyAVCDeviceControllerNotification
//////////////////////////////////////////////////////////////////////
IOReturn MyAVCDeviceControllerNotification(AVCDeviceController *pAVCDeviceController, void *pRefCon, AVCDevice* pDevice)
{
	// Note: This app doesn't currently use this callback.
	return kIOReturnSuccess;
}	

//////////////////////////////////////////////////////////////////////
// MyAVCDeviceMessageNotification
//////////////////////////////////////////////////////////////////////
IOReturn MyAVCDeviceMessageNotification (class AVCDevice *pAVCDevice,
										 natural_t messageType,
										 void * messageArgument,
										 void *pRefCon)
{	
	// Note: This app doesn't currently use this callback.
	return kIOReturnSuccess;
}
#endif
