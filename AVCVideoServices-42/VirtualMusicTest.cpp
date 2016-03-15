/*
	File:		VirtualMusicTest.cpp
 
 Synopsis: This is a simple console mode application that shows an example of using
 the code in AVS for emulating a AV/C Music device.
 
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

#include "FWAUserLib/AppleFWAudioUserLib.h"
#include "FireWireAudio.h"

#define kVirtualMusicTestNumAudioInputs 4
#define kVirtualMusicTestNumAudioOutputs 4

// Callback for p2p connection handling
void MyVirtualMusicCMPConnectionHandler(void *pRefCon, bool isInputPlug, UInt8 isochChannel, UInt8 isochSpeed, UInt8 p2pCount);

// Callback for sample rate change requests.
IOReturn MyVirtualMusicSampleRateChangeHandler(void *pRefCon, MusicSubunitSampleRate newSampleRate);

// Thread funct for changing sample rates on the fly!
static void *ChangeFireWireAudioSampleRateThread(void* pParams);

// Globals
bool fireWireAudioTransmitterStarted = false;
bool fireWireAudioReceiverStarted = false;

UInt8 transmitterIsochChannel;
UInt8 receiverIsochChannel;
IOFWSpeed transmitterIsochSpeed;
IOFWSpeed receiverIsochSpeed;

pthread_t sampleRateChangeThread;
pthread_attr_t threadAttr;

VirtualMusicSubunit *pMusicSubunit = nil;
FireWireAudio *pFireWireAudio = nil;

//////////////////////////////////////////////////////
// MyExit - Exit Cleanup Routine
//////////////////////////////////////////////////////
static void MyExit(int signal)
{
	printf("VirtualMusicTest: MyExit Called!\n");
	
	if (pMusicSubunit)
		DestroyVirtualMusicSubunit(pMusicSubunit);
	
	if (pFireWireAudio)
		delete pFireWireAudio;
	
	exit(-1);
}


//////////////////////////////////////////////////////
// Main
//////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
	// Local Vars
    IOReturn result ;

	/* Handle error */;
	signal(SIGINT,MyExit);
	
	pFireWireAudio = new FireWireAudio(kVirtualMusicTestNumAudioOutputs,kVirtualMusicTestNumAudioInputs);
	if (!pFireWireAudio)
	{
		printf("VirtualMusicTest: Error creating FireWireAudio\n");
		return -1;
	}
	
	result = pFireWireAudio->setupFireWireAudio();
	if (result != kIOReturnSuccess)
	{
		printf("VirtualMusicTest: Error during setup of FireWireAudio: 0x%08X\n",result);
		return -1;
	}
	
	// Create and initialize the VirtualMusicSubunit on a dedicated thread
	result = CreateVirtualMusicSubunit(&pMusicSubunit,
									   kMusicSubunitSampleRate_48000, 
									   MyVirtualMusicCMPConnectionHandler,
									   MyVirtualMusicSampleRateChangeHandler,
									   NULL,
									   NULL,
									   kVirtualMusicTestNumAudioInputs,
									   kVirtualMusicTestNumAudioOutputs);	
	if (result != kIOReturnSuccess)
	{
		printf("VirtualMusicTest: Error creating VirtualMusicSubunit: 0x%08X\n",result);
		return -1;
	}
	
	// This app currently just has an endlelss loop.
	while (1)
	{	
		usleep(1000000); // Sleep for one second
	}
	
	// Delete the VirtuaMusicSubunit
	DestroyVirtualMusicSubunit(pMusicSubunit);
	pMusicSubunit = nil;
	
	// Dispose of the FireWireAudio engine
	// The destructor will clean up everything
	delete pFireWireAudio;
	pFireWireAudio = nil;
	
	// We're done!
	printf("VirtualMusicTest complete!\n");
	return result;
}

//////////////////////////////////////////////////////
// MyVirtualMusicCMPConnectionHandler
//////////////////////////////////////////////////////
IOReturn MyVirtualMusicSampleRateChangeHandler(void *pRefCon, MusicSubunitSampleRate newSampleRate)
{

#if 0	

	// NOTE: Re-enable this code to play with the dynamic sample-rate stuff. Currently Disabled!!!!!
	
	printf("MyVirtualMusicSampleRateChangeHandler, newSampleRate=%d\n",newSampleRate);
	
	
	if ((fireWireAudioTransmitterStarted == true) && (fireWireAudioReceiverStarted == true))
	{
		pthread_attr_init(&threadAttr);
		pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_DETACHED);
		pthread_create(&sampleRateChangeThread, &threadAttr, (void *(*)(void *))ChangeFireWireAudioSampleRateThread, (void*) newSampleRate);
		pthread_attr_destroy(&threadAttr);
	}
	
	return kIOReturnSuccess;

#else
	return kIOReturnError;
#endif
	

}

//////////////////////////////////////////////////////
// MyVirtualMusicCMPConnectionHandler
//////////////////////////////////////////////////////
void MyVirtualMusicCMPConnectionHandler(void *pRefCon, bool isInputPlug, UInt8 isochChannel, UInt8 isochSpeed, UInt8 p2pCount)
{
	printf("+MyVirtualMusicCMPConnectionHandler\n");
	printf("%s: Chan=%d Speed=%d p2pCount=%d\n",(isInputPlug == true) ? "iPCR Modified" : "oPCR  Modified",isochChannel, isochSpeed, p2pCount);
	printf("fireWireAudioTransmitterStarted=%d fireWireAudioReceiverStarted=%d\n",fireWireAudioTransmitterStarted,fireWireAudioReceiverStarted);
	
	if ( (isInputPlug == false) && (fireWireAudioTransmitterStarted == false) && (p2pCount > 0))
	{
		transmitterIsochChannel = isochChannel;
		transmitterIsochSpeed = (IOFWSpeed) isochSpeed;
		pFireWireAudio->SetAudioSampleRate(pMusicSubunit->GetCurrentSubunitSampleRate());
		pFireWireAudio->StartFireWireAudioTransmitter(pMusicSubunit->GetCurrentSubunitSampleRate(), isochChannel, (IOFWSpeed) isochSpeed);
		fireWireAudioTransmitterStarted = true;
	}
	else if ( (isInputPlug == false) && (fireWireAudioTransmitterStarted == true) && (p2pCount == 0))
	{
		pFireWireAudio->StopFireWireAudioTransmitter();
		fireWireAudioTransmitterStarted = false;
	}
	else if ( (isInputPlug == true) && (fireWireAudioReceiverStarted == false) && (p2pCount > 0))
	{
		receiverIsochChannel = isochChannel;
		receiverIsochSpeed = (IOFWSpeed) isochSpeed;
		pFireWireAudio->SetAudioSampleRate(pMusicSubunit->GetCurrentSubunitSampleRate());
		pFireWireAudio->StartFireWireAudioReceiver(pMusicSubunit->GetCurrentSubunitSampleRate(), isochChannel, (IOFWSpeed) isochSpeed);
		fireWireAudioReceiverStarted = true;
	}
	else if ( (isInputPlug == true) && (fireWireAudioReceiverStarted == true) && (p2pCount == 0))
	{
		pFireWireAudio->StopFireWireAudioReceiver();
		fireWireAudioReceiverStarted = false;
	}

	printf("-MyVirtualMusicCMPConnectionHandler\n");		
}

//////////////////////////////////////////////////////////////////////
// ChangeFireWireAudioSampleRateThread
//////////////////////////////////////////////////////////////////////
void *ChangeFireWireAudioSampleRateThread(void* pParams)
{
	IOReturn result;
	UInt32 newSampleRate = (UInt32) pParams;

	printf("ChangeFireWireAudioSampleRateThread, newSampleRate=%d\n",(int)newSampleRate);
	
	// Dispose of the FireWireAudio engine. The destructor will clean up everything
	delete pFireWireAudio;
	pFireWireAudio = nil;
	
	// Create a new FireWireAudio 
	pFireWireAudio = new FireWireAudio(kVirtualMusicTestNumAudioOutputs,kVirtualMusicTestNumAudioInputs);
	if (!pFireWireAudio)
	{
		printf("VirtualMusicTest: Error creating FireWireAudio\n");
		return 0;
	}
	
	result = pFireWireAudio->setupFireWireAudio();
	if (result != kIOReturnSuccess)
	{
		printf("VirtualMusicTest: Error during setup of FireWireAudio: 0x%08X\n",result);
		return 0;
	}
	
	// Set the new sample rate
	pFireWireAudio->SetAudioSampleRate(newSampleRate); 
	
	if (fireWireAudioTransmitterStarted == true)
		pFireWireAudio->StartFireWireAudioTransmitter(newSampleRate, transmitterIsochChannel, transmitterIsochSpeed);
	
	if (fireWireAudioReceiverStarted == true)
		pFireWireAudio->StartFireWireAudioReceiver(newSampleRate, receiverIsochChannel, receiverIsochSpeed);

	return 0;
}	
	