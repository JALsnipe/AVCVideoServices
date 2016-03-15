/*
	File:		AVCInfo.cpp
 
 Synopsis: This is a simple command-line app for testing the AVCDeviceController and AVCDevice classes.
 
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

#define kEnableMusicSubunitControllerTestMode 1

IOReturn MyAVCDeviceControllerNotification(AVCDeviceController *pAVCDeviceController, void *pRefCon, AVCDevice* pDevice);
IOReturn MyAVCDeviceMessageNotification(AVCDevice *pAVCDevice,
										natural_t messageType,
										void * messageArgument,
										void *pRefCon);

#if kEnableMusicSubunitControllerTestMode
//////////////////////////////////////////////////////
// PrintLogMessage
//////////////////////////////////////////////////////
void PrintLogMessage(char *pString)
{
	printf("%s",pString);
}
#endif

//////////////////////////////////////////////////////
//
// Main
//
//////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
	IOReturn result = kIOReturnSuccess ;
	AVCDeviceController *pAVCDeviceController;
	AVCDevice *pAVCDevice = nil;
	unsigned int i;
	UInt16 nodeID;
	UInt8 powerState;

	// Create the AVCDeviceController and dedicated thread
	result = CreateAVCDeviceController(&pAVCDeviceController,MyAVCDeviceControllerNotification, nil); //,MyAVCDeviceMessageNotification);
	if (!pAVCDeviceController)
	{
		printf("Error creating AVCDeviceController object: %d\n",result);
		return -1;
	}

	// Search through all the connected devices to find one we're interested in
	for (i=0;i<(UInt32)CFArrayGetCount(pAVCDeviceController->avcDeviceArray);i++)
	{
		pAVCDevice = (AVCDevice*) CFArrayGetValueAtIndex(pAVCDeviceController->avcDeviceArray,i);
		
		printf("\n==========================================================\n");
		
		printf("Device %d: %s\n",(int)i,(pAVCDevice->deviceName != nil) ? pAVCDevice->deviceName : "Unknown Device Name");
		
		printf("    Guid: 0x%016llX\n",pAVCDevice->guid);
		printf("    isAttached: %s\n",(pAVCDevice->isAttached == true) ? "Yes" : "No");
		
		printf("    Vendor ID: 0x%08X\n",(unsigned int)pAVCDevice->vendorID);

		if (pAVCDevice->vendorName)
			printf("    Vendor Name: %s\n",pAVCDevice->vendorName);

		printf("    Model ID: 0x%08X\n",(unsigned int)pAVCDevice->modelID);
		
		if (pAVCDevice->isAttached == true)
		{
			nodeID = 0xFFFF;
			pAVCDevice->GetCurrentNodeID(&nodeID);
			printf("    Current NodeID: 0x%04X\n",nodeID);
		}
		
		printf("    supportsFCP: %s\n",(pAVCDevice->supportsFCP == true) ? "Yes" : "No");
		printf("    capabilitiesDiscovered: %s\n",(pAVCDevice->capabilitiesDiscovered == true) ? "Yes" : "No");
		
		if (pAVCDevice->capabilitiesDiscovered == true)
		{
			printf("    subUnits: 0x%08X\n",(int)pAVCDevice->subUnits);
			printf("    hasTapeSubunit: %s\n",(pAVCDevice->hasTapeSubunit == true) ? "Yes" : "No");
			printf("    hasMonitorOrTunerSubunit: %s\n",(pAVCDevice->hasMonitorOrTunerSubunit == true) ? "Yes" : "No");
			printf("    hasMusicSubunit: %s\n",(pAVCDevice->hasMusicSubunit == true) ? "Yes" : "No");
			printf("    hasAudioSubunit: %s\n",(pAVCDevice->hasAudioSubunit == true) ? "Yes" : "No");
			printf("    numInputPlugs:  %u\n",(int)pAVCDevice->numInputPlugs);
			printf("    numOutputPlugs: %u\n",(int)pAVCDevice->numOutputPlugs);
			printf("    isDVDevice: %s\n",(pAVCDevice->isDVDevice == true) ? "Yes" : "No");
			if (pAVCDevice->isDVDevice == true)
			{
				printf("    dvMode: 0x%02X\n",pAVCDevice->dvMode);
				printf("    isDVCProDevice: %s\n",(pAVCDevice->isDVCProDevice == true) ? "Yes" : "No");
			}
			printf("    isMPEGDevice: %s\n",(pAVCDevice->isMPEGDevice == true) ? "Yes" : "No");
			if (pAVCDevice->isMPEGDevice == true)
			{
				printf("    mpegMode: 0x%02X\n",pAVCDevice->mpegMode);
			}
		}
		
		// If we can open the device (not already in use by another app), open it, and try to get the current power state
		if (pAVCDevice->isAttached == true)
		{
			result = pAVCDevice->openDevice(MyAVCDeviceMessageNotification, nil);
			if (result == kIOReturnSuccess)
			{
				result = pAVCDevice->GetPowerState(&powerState);
				if (result == kIOReturnSuccess)
					printf("    Current Power State: %s\n",(powerState == 0x70) ? "On" : "Off");

#if kEnableMusicSubunitControllerTestMode
				if ((pAVCDevice->hasMusicSubunit == true) || (pAVCDevice->hasAudioSubunit == true))
				{
					StringLogger logger(PrintLogMessage);
					pAVCDevice->getIsochPlugCount();	// Do this just in case!
					MusicSubunitController *pMusicSubunitController = new MusicSubunitController(pAVCDevice,0,&logger);
					if (pMusicSubunitController)
					{
						// Discover the configuration of this device
						pMusicSubunitController->DiscoverConfiguration();
						
						// We're done
						delete pMusicSubunitController;
					}
				}
#endif		
				
				pAVCDevice->closeDevice();
			}
		}
		
		printf("\n");
	}
	
	DestroyAVCDeviceController(pAVCDeviceController);

	return result;
}

//////////////////////////////////////////////////////
// MyAVCDeviceControllerNotification
//////////////////////////////////////////////////////
IOReturn MyAVCDeviceControllerNotification(AVCDeviceController *pAVCDeviceController, void *pRefCon, AVCDevice* pDevice)
{
	// Not used by this app
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
#if 0
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
			
		case kIOFWMessageTopologyChanged:
			printf("kIOFWMessageTopologyChanged\n");
			break;
			
		default:
			printf("Unknown Message: 0x%08X\n",messageType);
			break;
	};
	
#endif
	
	return kIOReturnSuccess ;
}

