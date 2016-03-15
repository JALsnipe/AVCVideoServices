/*
	File:		VirtualMusicSubunit.cpp
 
 Synopsis: This is the implementation file for the VirtualMusicSubunit class.
 
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

namespace AVS
{

// Define the following to have stream-format-info command responses show we include MIDI in the streams
#define kVirtualMusicSubunitHasMidi 1	
	
// Some defines for our virtual music-subunit
#define kSubunitPlugDefault_FDF_FMT 0x9002
#define kNumMusicSubunitSourcePlugs 3
#define kNumMusicSubunitDestPlugs 4
	
// Read descriptor results values
#define kDescriptorCompleteRead 0x10
#define kDescriptorMoreToRead 0x11
#define kDescriptorDataLenTooLarge 0x12

/////////////////////////////////////////////////////////////////////////
//
// Music Plug Clusters
//
// This device has the following clusters:
//
// Audio Out (n channels)
// Midi Out (1 channel)
//
// Audio In (n channels)
// Midi In (1 channel)
//
// Sync I/O
//
// = 5 Total Clusters
//
/////////////////////////////////////////////////////////////////////////
MusicPlugCluster musicPlugClusters[] =
{
	{   // 0
		"Audio Out",
		kStreamFormatMBLA,
		kMusicPortTypeLine,
		0,	// Will be filled in later
		NULL 	// Will be filled in later
	},
	{   // 1
		"Midi Out",
		kStreamFormatMidiConf,
		kMusicPortTypeMidi,
		0,	// Will be filled in later
		NULL 	// Will be filled in later
	},
	{   // 2
		"Audio In",
		kStreamFormatMBLA,
		kMusicPortTypeLine,
		0,	// Will be filled in later
		NULL 	// Will be filled in later
	},
	{   // 3
		"Midi In",
		kStreamFormatMidiConf,
		kMusicPortTypeMidi,
		0,	// Will be filled in later
		NULL 	// Will be filled in later
	},
	{   // 4
		"Sync",
		kStreamFormatMBLA,
		kMusicPortTypeNoType,
		0,	// Will be filled in later
		NULL 	// Will be filled in later
	}
};

/////////////////////////////////////////////////
// Array of Clusters for Subunit Source Plug 0
/////////////////////////////////////////////////
MusicPlugCluster *SubunitSourcePlug0_ClusterArray[] =
{
	&musicPlugClusters[2],
	&musicPlugClusters[3]
};

/////////////////////////////////////////////////
// Array of Clusters for Subunit Source Plug 1
/////////////////////////////////////////////////
MusicPlugCluster *SubunitSourcePlug1_ClusterArray[] =
{
	&musicPlugClusters[0]
};

/////////////////////////////////////////////////
// Array of Clusters for Subunit Source Plug 2
/////////////////////////////////////////////////
MusicPlugCluster *SubunitSourcePlug2_ClusterArray[] =
{
	&musicPlugClusters[1]
};

/////////////////////////////////////////////////
// Array of Clusters for Subunit Dest Plug 0
/////////////////////////////////////////////////
MusicPlugCluster *SubunitDestPlug0_ClusterArray[] =
{
	&musicPlugClusters[0],
	&musicPlugClusters[1]
};

/////////////////////////////////////////////////
// Array of Clusters for Subunit Dest Plug 1
/////////////////////////////////////////////////
MusicPlugCluster *SubunitDestPlug1_ClusterArray[] =
{
	&musicPlugClusters[2]
};

/////////////////////////////////////////////////
// Array of Clusters for Subunit Dest Plug 2
/////////////////////////////////////////////////
MusicPlugCluster *SubunitDestPlug2_ClusterArray[] =
{
	&musicPlugClusters[3]
};

/////////////////////////////////////////////////
// Array of Clusters for Subunit Dest Plug 3
/////////////////////////////////////////////////
MusicPlugCluster *SubunitDestPlug3_ClusterArray[] =
{
	&musicPlugClusters[4]
};

/////////////////////////////////////////////////
// Subunit Source Plugs
/////////////////////////////////////////////////
MusicSubunitPlug SubunitSourcePlugs[] =
{
	{   // 0
		NULL,  //"Isoch Out",
		0,
		kSubunitPlugDefault_FDF_FMT,
		kIsochStreamSubunitPlug,
		sizeof(SubunitSourcePlug0_ClusterArray)/sizeof(MusicPlugCluster*),
		SubunitSourcePlug0_ClusterArray
	},	
	{   // 1
		NULL,  //"Audio Out",
		1,
		kSubunitPlugDefault_FDF_FMT,
		kDigitalSubunitPlug,
		sizeof(SubunitSourcePlug1_ClusterArray)/sizeof(MusicPlugCluster*),
		SubunitSourcePlug1_ClusterArray
	},	
	{   // 2
		NULL,  //"Midi Out",
		2,
		kSubunitPlugDefault_FDF_FMT,
		kMidiSubunitPlug,
		sizeof(SubunitSourcePlug2_ClusterArray)/sizeof(MusicPlugCluster*),
		SubunitSourcePlug2_ClusterArray
	}
};

/////////////////////////////////////////////////
// Subunit Dest Plugs
/////////////////////////////////////////////////
MusicSubunitPlug SubunitDestPlugs[] =
{
	{   // 0
		NULL,  //"Isoch In",
		0,
		kSubunitPlugDefault_FDF_FMT,
		kIsochStreamSubunitPlug,
		sizeof(SubunitDestPlug0_ClusterArray)/sizeof(MusicPlugCluster*),
		SubunitDestPlug0_ClusterArray
	},	
	{   // 1
		NULL,  //"Audio In",
		1,
		kSubunitPlugDefault_FDF_FMT,
		kDigitalSubunitPlug,
		sizeof(SubunitDestPlug1_ClusterArray)/sizeof(MusicPlugCluster*),
		SubunitDestPlug1_ClusterArray
	},	
	{   // 2
		NULL,  //"Midi In",
		2,
		kSubunitPlugDefault_FDF_FMT,
		kMidiSubunitPlug,
		sizeof(SubunitDestPlug2_ClusterArray)/sizeof(MusicPlugCluster*),
		SubunitDestPlug2_ClusterArray
	},	
	{   // 3
		NULL,  //"Sync In",
		3,
		kSubunitPlugDefault_FDF_FMT,
		kSyncSubunitPlug,
		sizeof(SubunitDestPlug3_ClusterArray)/sizeof(MusicPlugCluster*),
		SubunitDestPlug3_ClusterArray
	}
};

// Thread parameter structures
struct VirtualMusicSubunitThreadParams
{
	volatile bool threadReady;
	VirtualMusicSubunit *pMusicSubunit;
	MusicSubunitSampleRate initialSampleRate;
	VirtualMusicCMPConnectionHandler cmpConnectionHandler;
	VirtualMusicSampleRateChangeHandler sampleRateChangeHandler;
	void *pCallbackRefCon;
	AVCDevice *pAVCDevice;
	UInt32 numAudioInputs;
	UInt32 numAudioOutputs;
};

//////////////////////////////////////////////////////////////////////
// VirtualMusicSubunitThreadStart
//////////////////////////////////////////////////////////////////////
static void *VirtualMusicSubunitThreadStart(VirtualMusicSubunitThreadParams* pParams)
{
	IOReturn result = kIOReturnSuccess ;
	VirtualMusicSubunit *pMusicSubunit;
	
	// Instantiate a new receiver object
	pMusicSubunit = new VirtualMusicSubunit(pParams->initialSampleRate,
											pParams->cmpConnectionHandler,
											pParams->sampleRateChangeHandler,
											pParams->pCallbackRefCon,
											pParams->numAudioInputs,
											pParams->numAudioOutputs);
	
	// Setup the receiver object
	if (pMusicSubunit)
	{
		if (pParams->pAVCDevice)
			result = pMusicSubunit->setupVirtualMusicSubunitWithAVCDevice(pParams->pAVCDevice);
		else
			result = pMusicSubunit->setupVirtualMusicSubunit();
	}
	
	// Update the return parameter with a pointer to the new receiver object
	if (result == kIOReturnSuccess)
		pParams->pMusicSubunit = pMusicSubunit;
	else
	{
		delete pMusicSubunit;
		pMusicSubunit = nil;
		pParams->pMusicSubunit = nil;
	}
	
	// Signal that this thread is ready
	pParams->threadReady = true;
	
	// Start the run loop
	if ((pMusicSubunit) && (result == kIOReturnSuccess))
		CFRunLoopRun();
	
	return nil;
}

//////////////////////////////////////////////////////
// CreateVirtualMusicSubunit
//////////////////////////////////////////////////////
IOReturn CreateVirtualMusicSubunit(VirtualMusicSubunit **ppMusicSubunit,
								   MusicSubunitSampleRate initialSampleRate,
								   VirtualMusicCMPConnectionHandler cmpConnectionHandler,
								   VirtualMusicSampleRateChangeHandler sampleRateChangeHandler,
								   void *pCallbackRefCon,
								   AVCDevice *pAVCDevice,
								   UInt32 numAudioInputSignals,
								   UInt32 numAudioOutputSignals)
{
	VirtualMusicSubunitThreadParams threadParams;
	pthread_t rtThread;
	pthread_attr_t threadAttr;
	
	threadParams.threadReady = false;
	threadParams.pMusicSubunit = nil;
	threadParams.pAVCDevice = pAVCDevice;
	
	threadParams.initialSampleRate = initialSampleRate;
	threadParams.cmpConnectionHandler = cmpConnectionHandler;
	threadParams.pCallbackRefCon = pCallbackRefCon;
	threadParams.sampleRateChangeHandler = sampleRateChangeHandler;
	
	threadParams.numAudioInputs = numAudioInputSignals;
	threadParams.numAudioOutputs = numAudioOutputSignals;
	
	// Create the thread which will instantiate and setup new VirtualMusicSubunit object
	pthread_attr_init(&threadAttr);
	pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_DETACHED);
	pthread_create(&rtThread, &threadAttr, (void *(*)(void *))VirtualMusicSubunitThreadStart, &threadParams);
	pthread_attr_destroy(&threadAttr);
	
	// Wait forever for the new thread to be ready
	while (threadParams.threadReady == false) usleep(1000);
	
	*ppMusicSubunit = threadParams.pMusicSubunit;
	
	if (threadParams.pMusicSubunit)
		return kIOReturnSuccess;
	else
		return kIOReturnError;
}

//////////////////////////////////////////////////////
// DestroyVirtualMusicSubunit
//////////////////////////////////////////////////////
IOReturn DestroyVirtualMusicSubunit(VirtualMusicSubunit *pMusicSubunit)
{
	IOReturn result = kIOReturnSuccess ;
	CFRunLoopRef runLoopRef;
	
	// Save the ref to the run loop the receiver is using
	runLoopRef = pMusicSubunit->runLoopRef;
	
	// Delete receiver object
	delete pMusicSubunit;
	
	// Stop the run-loop in the RT thread. The RT thread will then terminate
	CFRunLoopStop(runLoopRef);
	
	return result;
}

//////////////////////////////////////////////////////
// AVCSubUnitPlugHandlerCallbackHelper
//////////////////////////////////////////////////////
static IOReturn AVCSubUnitPlugHandlerCallbackHelper(void *refCon,
													UInt32 subunitTypeAndID,
													IOFWAVCPlugTypes plugType,
													UInt32 plugID,
													IOFWAVCSubunitPlugMessages plugMessage,
													UInt32 messageParams)
{
	VirtualMusicSubunit *pVirtualMusicSubunit = (VirtualMusicSubunit*) refCon;
	
	return pVirtualMusicSubunit->AVCSubUnitPlugHandlerCallback(subunitTypeAndID,
															  plugType,
															  plugID,
															  plugMessage,
															  messageParams);
}

//////////////////////////////////////////////////////
// InputPlugReconnectTimeoutHelper
//////////////////////////////////////////////////////
static void InputPlugReconnectTimeoutHelper(CFRunLoopTimerRef timer, void *data)
{
	VirtualMusicSubunit *pVirtualMusicSubunit = (VirtualMusicSubunit*) data;
	pVirtualMusicSubunit->inputPlugReconnectTimeout();
}

//////////////////////////////////////////////////////
// OutputPlugReconnectTimeoutHelper
//////////////////////////////////////////////////////
static void OutputPlugReconnectTimeoutHelper(CFRunLoopTimerRef timer, void *data)
{
	VirtualMusicSubunit *pVirtualMusicSubunit = (VirtualMusicSubunit*) data;
	pVirtualMusicSubunit->outputPlugReconnectTimeout();
}

//////////////////////////////////////////////////////
// VirtualMusicSubunit Constructor
//////////////////////////////////////////////////////
VirtualMusicSubunit::VirtualMusicSubunit(MusicSubunitSampleRate initialSampleRate,
										 VirtualMusicCMPConnectionHandler cmpConnectionHandler,
										 VirtualMusicSampleRateChangeHandler sampleRateChangeHandler,
										 void *pCallbackRefCon,
										 UInt32 numAudioInputSignals,
										 UInt32 numAudioOutputSignals)
{
	runLoopRef = nil;
	
	clientCMPConnectionHandler = cmpConnectionHandler;
	clientSampleRateChangeHandler = sampleRateChangeHandler;
	pClientCallbackRefCon = pCallbackRefCon;

	numAudioInputs = numAudioInputSignals;
	numAudioOutputs = numAudioOutputSignals;
	
	numMusicPlugs = 0;
	pMusicSubunitMusicPlugs = nil;
	
	ppAudioOutClusterPlugArray = nil;
	ppMidiOutClusterPlugArray = nil;
	ppAudioInClusterPlugArray = nil;
	ppMidiInClusterPlugArray = nil;
	ppSyncClusterPlugArray = nil;
	
	sampleRate = initialSampleRate;
	
	pStatusDescriptor = nil;
	
    outputPlugReconnectTimer = NULL;
    inputPlugReconnectTimer = NULL;
	
	nodeAVCProtocolInterface = nil;
}

//////////////////////////////////////////////////////
// VirtualMusicSubunit Destructor
//////////////////////////////////////////////////////
VirtualMusicSubunit::~VirtualMusicSubunit()
{
	// Remove callback dispatcher from run loop
	if (nodeAVCProtocolInterface != nil)
	{
		(*nodeAVCProtocolInterface)->removeCallbackDispatcherFromRunLoop(nodeAVCProtocolInterface);
		(*nodeAVCProtocolInterface)->Release(nodeAVCProtocolInterface) ;
	}
	
	if (nodeNubInterface != nil)
		(*nodeNubInterface)->Release(nodeNubInterface);
	
	if (pMusicSubunitMusicPlugs)
		delete [] pMusicSubunitMusicPlugs;
	
	if (ppAudioOutClusterPlugArray)
		delete [] ppAudioOutClusterPlugArray;
			
	if (ppMidiOutClusterPlugArray)
		delete [] ppMidiOutClusterPlugArray;

	if (ppAudioInClusterPlugArray)
		delete [] ppAudioInClusterPlugArray;

	if (ppMidiInClusterPlugArray)
		delete [] ppMidiInClusterPlugArray;
	
	if (ppSyncClusterPlugArray)
		delete [] ppSyncClusterPlugArray;
	
	if (pStatusDescriptor)
		delete [] pStatusDescriptor;
}

//////////////////////////////////////////////////////
// VirtualMusicSubunit::setupVirtualMusicSubunitWithAVCDevice
//////////////////////////////////////////////////////
IOReturn VirtualMusicSubunit::setupVirtualMusicSubunitWithAVCDevice(AVCDevice *pAVCDevice)
{
	IOReturn result;
	pAVCDeviceForBusIdentification = pAVCDevice;
	
	result = GetAVCProtocolInterfaceWithAVCDevice(pAVCDeviceForBusIdentification, 
												  &nodeAVCProtocolInterface,
												  &nodeNubInterface);
	if (result != kIOReturnSuccess)
		return result ;
	else
		return completeVirtualMusicSubunitSetup();
}

//////////////////////////////////////////////////////
// VirtualMusicSubunit::setupVirtualMusicSubunit
//////////////////////////////////////////////////////
IOReturn VirtualMusicSubunit::setupVirtualMusicSubunit(void)
{
	IOReturn result;
	
	pAVCDeviceForBusIdentification = nil;
	
	result = GetDefaultAVCProtocolInterface(&nodeAVCProtocolInterface, &nodeNubInterface);
	
	if (result != kIOReturnSuccess)
		return result ;
	else
		return completeVirtualMusicSubunitSetup();
}


//////////////////////////////////////////////////////
// VirtualMusicSubunit::completeVirtualMusicSubunitSetup
//////////////////////////////////////////////////////
IOReturn VirtualMusicSubunit::completeVirtualMusicSubunitSetup(void)
{
	IOReturn result = kIOReturnSuccess ;
	UInt32 sourcePlugNum = 0;
	UInt32 destPlugNum = 0;
	UInt32 retry = 5;
	UInt32 payloadInQuadlets;
	UInt32 AudioSubUnitTypeAndID;
	
	// Make sure the specified number of audio
	// ins and outs are in range.
	if (numAudioOutputs < 2)
		numAudioOutputs = 2;
	if (numAudioOutputs > 64)
		numAudioOutputs = 64;
	if (numAudioInputs < 2)
		numAudioInputs = 2;
	if (numAudioInputs > 64)
		numAudioInputs = 64;
	
	// Save a reference to the current run loop
	runLoopRef = CFRunLoopGetCurrent();
	
	////////////////////////////////////////////////////////////////////////////
	//
	// Music Subunit Status Descriptor
	// -------------------------------------------
	// descriptor_len:									2
 	//		
	// general_music_subunit_status_area_info_block:	
	//		compound_len (0x000A):						2
	//		info_block_type (0x8100):					2
	//		primary_fields_len (0x0006):				2
	//		current_transmit_capability (0x01):			1
	//		current_receive_capability (0x01):			1
	//		current_latency_capability (0xFFFFFFFF):	4
	//		
	// music_output_plug_status_area_info_block:		0 (now optional!)
 	//		
	// routing_status_info_block:						As configured
	//		
	// --------------------------------------------
	// status descriptor length = size_of_routing_status_info_block + 14
	//	
	////////////////////////////////////////////////////////////////////////////

	// Prepare the dynamic data structures used to build the descriptor
	generateDiscriptorStructs();
	
	statusDescriptorSize = SizeOfMusicSubinitRoutingStatusInfoBlock() + 14;
	pStatusDescriptor = new unsigned char[statusDescriptorSize];
	if (pStatusDescriptor == NULL)
		return kIOReturnNoMemory;
	
	// Build Status Descriptor
	pStatusDescriptor[0] = (((statusDescriptorSize-2) & 0xFF00) >> 8);
	pStatusDescriptor[1] = ((statusDescriptorSize-2) & 0xFF);
	pStatusDescriptor[2] = 0x00;
	pStatusDescriptor[3] = 0x0A;
	pStatusDescriptor[4] = 0x81;
	pStatusDescriptor[5] = 0x00;
	pStatusDescriptor[6] = 0x00;
	pStatusDescriptor[7] = 0x06;
	pStatusDescriptor[8] = 0x01;
	pStatusDescriptor[9] = 0x01;
	pStatusDescriptor[10] = 0xFF;
	pStatusDescriptor[11] = 0xFF;
	pStatusDescriptor[12] = 0xFF;
	pStatusDescriptor[13] = 0xFF;
	SetByteBuf(pStatusDescriptor+14);
	SetRoutingStatusInfoBlock();
	
	// Add a Music subunit
	result = (*nodeAVCProtocolInterface)->addSubunit(nodeAVCProtocolInterface,
													 0x0C,	// Music Subunit
													 kNumMusicSubunitSourcePlugs,
													 kNumMusicSubunitDestPlugs,
													 this,
													 AVCSubUnitPlugHandlerCallbackHelper,
													 &subUnitTypeAndID);
    if (result != kIOReturnSuccess)
        return result ;

#ifdef kEnableAudioSubunitForDriverMatching
	
	// Add an Audio subunit (just for driver matching!)
	result = (*nodeAVCProtocolInterface)->addSubunit(nodeAVCProtocolInterface,
													 1,	// Audio Subunit
													 1,
													 1,
													 this,
													 nil,
													 &AudioSubUnitTypeAndID);
    if (result != kIOReturnSuccess)
        return result ;
	
	// Install command handler for the newly installed audio subunit
	result = (*nodeAVCProtocolInterface)->installAVCCommandHandler(nodeAVCProtocolInterface,
																   AudioSubUnitTypeAndID,
																   kAVCAllOpcodes,
																   this,
																   VirtualMusicSubunit::AudioSubunitCommandHandlerCallback);
	if (result != kIOReturnSuccess)
		return -1;
	
#endif
	
	// Connect the subunit source plug 0 to unit isoch output plug 0
	sourcePlugNum = 0;
	destPlugNum = 0;
	result = (*nodeAVCProtocolInterface)->connectTargetPlugs(nodeAVCProtocolInterface,
															 subUnitTypeAndID,
															 IOFWAVCPlugSubunitSourceType,
															 &sourcePlugNum,
															 kAVCUnitAddress,
															 IOFWAVCPlugIsochOutputType,
															 &destPlugNum,
															 true,
															 true);
	if (result != kIOReturnSuccess)
		return result;
	
	// Save the isoch out plug num
	isochOutPlugNum = destPlugNum;
	
	// Connect the subunit dest plug 0 to unit isoch input plug 0
	sourcePlugNum = 0;
	destPlugNum = 0;
	result = (*nodeAVCProtocolInterface)->connectTargetPlugs(nodeAVCProtocolInterface,
															 kAVCUnitAddress,
															 IOFWAVCPlugIsochInputType,
															 &sourcePlugNum,
															 subUnitTypeAndID,
															 IOFWAVCPlugSubunitDestType,
															 &destPlugNum,
															 true,
															 true);
	if (result != kIOReturnSuccess)
		return result;
	
	// Save the isoch in plug num
	isochInPlugNum = sourcePlugNum;

	// Set the subunitplugs signal format for the subunit plugs connected to isoch plugs, based on the sample rate
	setPlugSignalFormatWithSampleRate();
	
	// Connect subunit dest plug 1 to unit external input plug 0 (audio in)
	sourcePlugNum = 0x80;
	destPlugNum = 1;
	result = (*nodeAVCProtocolInterface)->connectTargetPlugs(nodeAVCProtocolInterface,
															 kAVCUnitAddress,
															 IOFWAVCPlugExternalInputType,
															 &sourcePlugNum,
															 subUnitTypeAndID,
															 IOFWAVCPlugSubunitDestType,
															 &destPlugNum,
															 true,
															 true);
	if (result != kIOReturnSuccess)
		return result;
	
	// Connect subunit dest plug 2 to unit external input plug 1 (midi in)
	sourcePlugNum = 0x81;
	destPlugNum = 2;
	result = (*nodeAVCProtocolInterface)->connectTargetPlugs(nodeAVCProtocolInterface,
															 kAVCUnitAddress,
															 IOFWAVCPlugExternalInputType,
															 &sourcePlugNum,
															 subUnitTypeAndID,
															 IOFWAVCPlugSubunitDestType,
															 &destPlugNum,
															 true,
															 true);
	if (result != kIOReturnSuccess)
		return result;
	
	// Connect subunit dest plug 3 to unit isoch input plug 0 (isoch audio is only sync source)
	sourcePlugNum = 0;
	destPlugNum = 3;
	result = (*nodeAVCProtocolInterface)->connectTargetPlugs(nodeAVCProtocolInterface,
															 kAVCUnitAddress,
															 IOFWAVCPlugIsochInputType,
															 &sourcePlugNum,
															 subUnitTypeAndID,
															 IOFWAVCPlugSubunitDestType,
															 &destPlugNum,
															 true,
															 true);
	if (result != kIOReturnSuccess)
		return result;

	// Connect the subunit source plug 1 to unit external output plug 0 (audio out)
	sourcePlugNum = 1;
	destPlugNum = 0x80;
	result = (*nodeAVCProtocolInterface)->connectTargetPlugs(nodeAVCProtocolInterface,
															 subUnitTypeAndID,
															 IOFWAVCPlugSubunitSourceType,
															 &sourcePlugNum,
															 kAVCUnitAddress,
															 IOFWAVCPlugExternalOutputType,
															 &destPlugNum,
															 true,
															 true);
	if (result != kIOReturnSuccess)
		return result;
	
	// Connect the subunit source plug 2 to unit external output plug 1 (midi out)
	sourcePlugNum = 2;
	destPlugNum = 0x81;
	result = (*nodeAVCProtocolInterface)->connectTargetPlugs(nodeAVCProtocolInterface,
															 subUnitTypeAndID,
															 IOFWAVCPlugSubunitSourceType,
															 &sourcePlugNum,
															 kAVCUnitAddress,
															 IOFWAVCPlugExternalOutputType,
															 &destPlugNum,
															 true,
															 true);
	if (result != kIOReturnSuccess)
		return result;
	
	// Install command handler for the newly installed music subunit
	result = (*nodeAVCProtocolInterface)->installAVCCommandHandler(nodeAVCProtocolInterface,
																   subUnitTypeAndID,
																   kAVCAllOpcodes,
																   this,
																   VirtualMusicSubunit::MusicSubunitCommandHandlerCallback);
	if (result != kIOReturnSuccess)
		return -1;
	
	// Install a command handler to override the unit-level plug-info command 
	result = (*nodeAVCProtocolInterface)->installAVCCommandHandler(nodeAVCProtocolInterface,
																   kAVCUnitAddress,
																   kAVCPlugInfoOpcode,
																   this,
																   VirtualMusicSubunit::AVCUnit_PlugInfoHandlerCallback);
	if (result != kIOReturnSuccess)
		return -1;
	
	// Install a command handler for the unit-level stream-format-info command
	result = (*nodeAVCProtocolInterface)->installAVCCommandHandler(nodeAVCProtocolInterface,
																   kAVCUnitAddress,
																   0x2F, // Stream-Format-Info Opcode
																   this,
																   VirtualMusicSubunit::AVCUnit_StreamInfoHandlerCallback);
	if (result != kIOReturnSuccess)
		return -1;
	
	// Install a command handler to override the unit-level Input Plug Signal Format command
	result = (*nodeAVCProtocolInterface)->installAVCCommandHandler(nodeAVCProtocolInterface,
																   kAVCUnitAddress,
																   kAVCInputPlugSignalFormatOpcode,
																   this,
																   VirtualMusicSubunit::AVCUnit_InputPlugSignalFormatHandlerCallback);
	if (result != kIOReturnSuccess)
		return -1;
	
	// Install a command handler to override the unit-level Output Plug Signal Format command
	result = (*nodeAVCProtocolInterface)->installAVCCommandHandler(nodeAVCProtocolInterface,
																   kAVCUnitAddress,
																   kAVCOutputPlugSignalFormatOpcode,
																   this,
																   VirtualMusicSubunit::AVCUnit_OutputPlugSignalFormatHandlerCallback);
	if (result != kIOReturnSuccess)
		return -1;
	
	// Install a command handler for the the unit-level signal-source command
	result = (*nodeAVCProtocolInterface)->installAVCCommandHandler(nodeAVCProtocolInterface,
																   kAVCUnitAddress,
																   0x1A, // Signal-Source Opcode
																   this,
																   VirtualMusicSubunit::AVCUnit_SignalSourceHandlerCallback);
	if (result != kIOReturnSuccess)
		return -1;
	
	// Set the master plug registers 
	retry = 0;
	do
	{
		result =(*nodeAVCProtocolInterface)->updateOutputMasterPlug(nodeAVCProtocolInterface,
																	(*nodeAVCProtocolInterface)->readOutputMasterPlug(nodeAVCProtocolInterface),
																	0xBFFFFF01); // broadcast chan = 63, max speed = 400, num oPCR = 1
	}
	while ((result != kIOReturnSuccess) && (retry++ < 5));
	
	retry = 0;
	do
	{
		result = (*nodeAVCProtocolInterface)->updateInputMasterPlug(nodeAVCProtocolInterface,
																	(*nodeAVCProtocolInterface)->readInputMasterPlug(nodeAVCProtocolInterface),
																	0x80FFFF01); // max speed = 400, num oPCR = 1
	}
	while ((result != kIOReturnSuccess) && (retry++ < 5));
	
	// Determine the payload size based on the signalMode
	payloadInQuadlets = subunitSampleRateToPayloadInQuadlets();
	
	// Set the oPCR/iPCR correctly
	setNewOutputPlugValue((0x003F0000+payloadInQuadlets));
	setNewInputPlugValue(0x003F0000);
	
	// Publish the config-ROM (if needed) and do bus reset
	return (*nodeAVCProtocolInterface)->publishAVCUnitDirectory(nodeAVCProtocolInterface);
}

//////////////////////////////////////////////////////
// VirtualMusicSubunit::GetCurrentSubunitSampleRate
//////////////////////////////////////////////////////
MusicSubunitSampleRate VirtualMusicSubunit::GetCurrentSubunitSampleRate(void)
{
	return sampleRate;
}

//////////////////////////////////////////////////////
// VirtualMusicSubunit::SetSubunitSampleRate
//////////////////////////////////////////////////////
void VirtualMusicSubunit::SetSubunitSampleRate(MusicSubunitSampleRate newSampleRate)
{
	if (sampleRate == newSampleRate)
		return;
	
	sampleRate = newSampleRate;
	setPlugSignalFormatWithSampleRate();
	updateMusicSubunitDescriptorWithNewSampleRate();
	// TODO: Need to update payload size in plug register
}

//////////////////////////////////////////////////////
// VirtualMusicSubunit::setNewInputPlugValue
//////////////////////////////////////////////////////
IOReturn VirtualMusicSubunit::setNewInputPlugValue(UInt32 newVal)
{
	IOReturn result = kIOReturnSuccess ;
	UInt32 retry;
	
	retry = 0;
	isochInPlugVal = newVal;
	do
	{
		(*nodeAVCProtocolInterface)->updateInputPlug(nodeAVCProtocolInterface,
													 isochInPlugNum,
													 (*nodeAVCProtocolInterface)->readInputPlug(nodeAVCProtocolInterface,isochInPlugNum),
													 isochInPlugVal);
	}
	while ((result != kIOReturnSuccess) && (retry++ < 5));
	
	return result;
}

//////////////////////////////////////////////////////
// VirtualMusicSubunit::setNewOutputPlugValue
//////////////////////////////////////////////////////
IOReturn VirtualMusicSubunit::setNewOutputPlugValue(UInt32 newVal)
{
	IOReturn result = kIOReturnSuccess ;
	UInt32 retry;
	
	retry = 0;
	isochOutPlugVal = newVal;
	do
	{
		result = (*nodeAVCProtocolInterface)->updateOutputPlug(nodeAVCProtocolInterface,
															   isochOutPlugNum,
															   (*nodeAVCProtocolInterface)->readOutputPlug(nodeAVCProtocolInterface,isochOutPlugNum),
															   isochOutPlugVal); 
	}
	while ((result != kIOReturnSuccess) && (retry++ < 5));
	
	return result;
}

////////////////////////////////////////////////////////
// VirtualMusicSubunit::setPlugSignalFormatWithSampleRate
////////////////////////////////////////////////////////
void VirtualMusicSubunit::setPlugSignalFormatWithSampleRate(void)
{
	UInt32 signalFormat = (0x90000000 + ((sampleRate & 0xFF) << 16));
	
	(*nodeAVCProtocolInterface)->setSubunitPlugSignalFormat(nodeAVCProtocolInterface,
															subUnitTypeAndID,
															IOFWAVCPlugSubunitSourceType,
															0,
															signalFormat);
	
	(*nodeAVCProtocolInterface)->setSubunitPlugSignalFormat(nodeAVCProtocolInterface,
															subUnitTypeAndID,
															IOFWAVCPlugSubunitDestType,
															0,
															signalFormat);
}


//////////////////////////////////////////////////////
// VirtualMusicSubunit::getSubunitTypeAndID
//////////////////////////////////////////////////////
UInt8 VirtualMusicSubunit::getSubunitTypeAndID(void)
{
	return subUnitTypeAndID;
}

//////////////////////////////////////////////////////
// VirtualMusicSubunit::getPlugParameters
//////////////////////////////////////////////////////
void VirtualMusicSubunit::getPlugParameters(bool isInputPlug, UInt8 *pIsochChannel, UInt8 *pIsochSpeed, UInt8 *pP2PCount)
{
	if (isInputPlug == true)
	{
		// iPCR settings
		*pIsochChannel = ((isochInPlugVal & 0x003F0000) >> 16);
		*pIsochSpeed = 0xFF;	// Unused for input plug
		*pP2PCount = ((isochInPlugVal & 0x3F000000) >> 24);
	}
	else
	{
		// oPCR settings
		*pIsochChannel = ((isochOutPlugVal & 0x003F0000) >> 16);
		*pIsochSpeed = ((isochOutPlugVal & 0x0000C000) >> 14);
		*pP2PCount = ((isochOutPlugVal & 0x3F000000) >> 24);
	}
}

//////////////////////////////////////////////////////
// VirtualMusicSubunit::setPlugParameters
//////////////////////////////////////////////////////
IOReturn VirtualMusicSubunit::setPlugParameters(bool isInputPlug, UInt8 isochChannel, UInt8 isochSpeed)
{
	UInt32 p2pCount;
	UInt32 newVal;
	IOReturn result;
	
	if (isInputPlug == true)
	{
		p2pCount = ((isochInPlugVal & 0x3F000000) >> 24);
		if (p2pCount > 0)
			return kIOReturnNotWritable;
		else
		{
			// Calculate new iPCR value, and write it to iPCR
			newVal = isochInPlugVal & 0xFFC03FFF;	// Clear current channel and speed
			newVal |= ((UInt32)(isochChannel & 0x3F) << 16); // Set new channel
			newVal |=  ((UInt32)(isochSpeed & 0x03) << 14); // Set new speed
			
			result = setNewInputPlugValue(newVal); 
		}
	}
	else
	{
		p2pCount = ((isochOutPlugVal & 0x3F000000) >> 24);
		if (p2pCount > 0)
			return kIOReturnNotWritable;
		else
		{
			// Calculate new oPCR value, and write it to oPCR
			newVal = isochOutPlugVal & 0xFFC03FFF;	// Clear current channel and speed
			newVal |= ((UInt32)(isochChannel & 0x3F) << 16); // Set new channel
			newVal |=  ((UInt32)(isochSpeed & 0x03) << 14); // Set new speed	
			result = setNewOutputPlugValue(newVal); 
		}
	}
	
	return result;
}

#ifdef kEnableAudioSubunitForDriverMatching
//////////////////////////////////////////////////////
// VirtualMusicSubunit::AudioSubunitCommandHandlerCallback
//////////////////////////////////////////////////////
IOReturn VirtualMusicSubunit::AudioSubunitCommandHandlerCallback( void *refCon,
																  UInt32 generation,
																  UInt16 srcNodeID,
																  IOFWSpeed speed,
																  const UInt8 * command,
																  UInt32 cmdLen)
{
	/* Local Vars */
	VirtualMusicSubunit *pVirtualMusicSubunit = (VirtualMusicSubunit*) refCon;

	UInt8 cType = command[kAVCCommandResponse] & 0x0F;
	UInt8 *pRspFrame;
	
	pRspFrame = (UInt8*) malloc(cmdLen);
	if (!pRspFrame)
		return kIOReturnNoMemory;
	
	/* copy cmd frame to rsp frame */
	bcopy(command,pRspFrame,cmdLen);
	
	// Currently we only support the plug-info command for this audio subunit!
	if ((command[kAVCOpcode] == kAVCPlugInfoOpcode) && (cType == kAVCStatusInquiryCommand) && (command[kAVCOperand0] == 0x00) && (cmdLen == 8))
	{
		pRspFrame[kAVCOperand1] = 1;	
		pRspFrame[kAVCOperand2] = 1;	
		pRspFrame[kAVCOperand3] = 0xFF;	
		pRspFrame[kAVCOperand4] = 0xFF;	
		pRspFrame[kAVCCommandResponse] = kAVCImplementedStatus;
	}
	else 
		pRspFrame[kAVCCommandResponse] = kAVCNotImplementedStatus;
	
	/* Send the response */
	(*(pVirtualMusicSubunit->nodeAVCProtocolInterface))->sendAVCResponse(pVirtualMusicSubunit->nodeAVCProtocolInterface,
																		 generation,
																		 srcNodeID,
																		 (const char*) pRspFrame,
																		 cmdLen);
	// Free allocated response frame
	free(pRspFrame);
	
	return kIOReturnSuccess;
}	
#endif

//////////////////////////////////////////////////////
// VirtualMusicSubunit::MusicSubunitCommandHandlerCallback
//////////////////////////////////////////////////////
IOReturn VirtualMusicSubunit::MusicSubunitCommandHandlerCallback( void *refCon,
														 UInt32 generation,
														 UInt16 srcNodeID,
														 IOFWSpeed speed,
														 const UInt8 * command,
														 UInt32 cmdLen)
{
	/* Local Vars */
	VirtualMusicSubunit *pVirtualMusicSubunit = (VirtualMusicSubunit*) refCon;

	UInt8 cType = command[kAVCCommandResponse] & 0x0F;
	UInt8 *pRspFrame;
	unsigned int responseLen = cmdLen;
	unsigned short descriptorRequestedReadAddress;
	unsigned short descriptorRequestedReadLength;
	unsigned short descriptorActualReadLength;
	unsigned char descriptorReadResultStatus;
	
	pRspFrame = (UInt8*) malloc(512);
	if (!pRspFrame)
		return kIOReturnNoMemory;
	
	/* copy cmd frame to rsp frame */
	bcopy(command,pRspFrame,cmdLen);
	
	/* We currently don't support notify type commands */
	/* For others, set the default response as accepted or stable */
	if (cType == kAVCNotifyCommand)
		pRspFrame[kAVCCommandResponse] = kAVCNotImplementedStatus;
	else if (cType == kAVCControlCommand)
		pRspFrame[kAVCCommandResponse] = kAVCAcceptedStatus;
	else
		pRspFrame[kAVCCommandResponse] = kAVCImplementedStatus;
	
	/* parse the command */
	switch (command[kAVCOpcode])
	{
		/////////////////////////////////////////////////
		//
		// AVC Plug Info Command
		//
		/////////////////////////////////////////////////
		case kAVCPlugInfoOpcode:
			if ((cType == kAVCStatusInquiryCommand) && (command[kAVCOperand0] == 0x00))
			{
				// Report that this subunit has one input plug and one output plug
				pRspFrame[kAVCOperand1] = kNumMusicSubunitDestPlugs;
				pRspFrame[kAVCOperand2] = kNumMusicSubunitSourcePlugs;
			}
			else // must be a control command, so reject it
				pRspFrame[kAVCCommandResponse] = kAVCRejectedStatus;
			break;
			
		/////////////////////////////////////////////////
		//
		// AVC Stream-Format-Info Opcode
		//
		/////////////////////////////////////////////////
		case 0x2F:
			// Determine if the command is formatted as we expect
			if ((cType == kAVCStatusInquiryCommand) &&
				(command[kAVCOperand0] == 0xC0) &&	// Single Request Subfunction
				((command[kAVCOperand1] == 0) || (command[kAVCOperand1] == 1)) &&	// Input or Output direction
				(command[kAVCOperand2] == 1) &&  // SubUnit Plug
				(command[kAVCOperand4] == 0xFF) &&  // Always 0xFF 
				(command[kAVCOperand5] == 0xFF)) // Always 0xFF
			{
				if ((command[kAVCOperand1] == 0) && (command[kAVCOperand3] == 0))
				{
					// Subunit Dest Plug 0
					// Fill in the details
					pRspFrame[10] = 0x90; //AM824 
					pRspFrame[11] = 0x40; // Compound Stream
					pRspFrame[12] = pVirtualMusicSubunit->subunitSampleRateToStreamFormatInfoSampleRate();
					pRspFrame[13] = 0x02; // rate-control not supported
					pRspFrame[14] = 0x02; // 2 stream info
					pRspFrame[15] = pVirtualMusicSubunit->numAudioOutputs; // num channels
					pRspFrame[16] = 0x06; // MBLA
					pRspFrame[17] = 0x01; // num channels
					pRspFrame[18] = 0x0D; // MIDI
					pRspFrame[kAVCCommandResponse] = kAVCImplementedStatus;
					responseLen = 19;
				}
				else if ((command[kAVCOperand1] == 0) && (command[kAVCOperand3] == 1))
				{
					// Subunit Dest Plug 1
					// Fill in the details
					pRspFrame[10] = 0x90; //AM824 
					pRspFrame[11] = 0x40; // Compound Stream
					pRspFrame[12] = pVirtualMusicSubunit->subunitSampleRateToStreamFormatInfoSampleRate();
					pRspFrame[13] = 0x02; // rate-control not supported
					pRspFrame[14] = 0x01; // 1 stream info
					pRspFrame[15] = pVirtualMusicSubunit->numAudioInputs; // num channels
					pRspFrame[16] = 0x06; // MBLA
					pRspFrame[kAVCCommandResponse] = kAVCImplementedStatus;
					responseLen = 17;
				}
				else if ((command[kAVCOperand1] == 0) && (command[kAVCOperand3] == 2))
				{
					// Subunit Dest Plug 2
					// Fill in the details
					pRspFrame[10] = 0x90; //AM824 
					pRspFrame[11] = 0x40; // Compound Stream
					pRspFrame[12] = pVirtualMusicSubunit->subunitSampleRateToStreamFormatInfoSampleRate();
					pRspFrame[13] = 0x02; // rate-control not supported
					pRspFrame[14] = 0x01; // 1 stream info
					pRspFrame[15] = 0x01; // num channels
					pRspFrame[16] = 0x0D; // MIDI
					pRspFrame[kAVCCommandResponse] = kAVCImplementedStatus;
					responseLen = 17;
				}
				else if ((command[kAVCOperand1] == 0) && (command[kAVCOperand3] == 3))
				{
					// Subunit Dest Plug 3
					// Fill in the details
					pRspFrame[10] = 0x90; //AM824 
					pRspFrame[11] = 0x40; // Compound Stream
					pRspFrame[12] = pVirtualMusicSubunit->subunitSampleRateToStreamFormatInfoSampleRate();
					pRspFrame[13] = 0x02; // rate-control not supported
					pRspFrame[14] = 0x01; // 1 stream info
					pRspFrame[15] = 1;
					pRspFrame[16] = 0x40; // Sync
					pRspFrame[kAVCCommandResponse] = kAVCImplementedStatus;
					responseLen = 17;
				}
				else if ((command[kAVCOperand1] == 1) && (command[kAVCOperand3] == 0))
				{
					// Subunit Source Plug 0
					pRspFrame[10] = 0x90; //AM824 
					pRspFrame[11] = 0x40; // Compound Stream
					pRspFrame[12] = pVirtualMusicSubunit->subunitSampleRateToStreamFormatInfoSampleRate();
					pRspFrame[13] = 0x02; // rate-control not supported
					pRspFrame[14] = 0x02; // 2 stream info
					pRspFrame[15] = pVirtualMusicSubunit->numAudioInputs; // num channels
					pRspFrame[16] = 0x06; // MBLA
					pRspFrame[17] = 0x01; // num channels
					pRspFrame[18] = 0x0D; // MIDI
					pRspFrame[kAVCCommandResponse] = kAVCImplementedStatus;
					responseLen = 19;
				}
				else if ((command[kAVCOperand1] == 1) && (command[kAVCOperand3] == 1))
				{
					// Subunit Source Plug 1
					// Fill in the details
					pRspFrame[10] = 0x90; //AM824 
					pRspFrame[11] = 0x40; // Compound Stream
					pRspFrame[12] = pVirtualMusicSubunit->subunitSampleRateToStreamFormatInfoSampleRate();
					pRspFrame[13] = 0x02; // rate-control not supported
					pRspFrame[14] = 0x01; // 1 stream info
					pRspFrame[15] = pVirtualMusicSubunit->numAudioOutputs; // num channels
					pRspFrame[16] = 0x06; // MBLA
					pRspFrame[kAVCCommandResponse] = kAVCImplementedStatus;
					responseLen = 17;
				}
				else if ((command[kAVCOperand1] == 1) && (command[kAVCOperand3] == 2))
				{
					// Subunit Source Plug 2
					// Fill in the details
					pRspFrame[10] = 0x90; //AM824 
					pRspFrame[11] = 0x40; // Compound Stream
					pRspFrame[12] = pVirtualMusicSubunit->subunitSampleRateToStreamFormatInfoSampleRate();
					pRspFrame[13] = 0x02; // rate-control not supported
					pRspFrame[14] = 0x01; // 1 stream info
					pRspFrame[15] = 0x01; // num channels
					pRspFrame[16] = 0x0D; // MIDI
					pRspFrame[kAVCCommandResponse] = kAVCImplementedStatus;
					responseLen = 17;
				}
				else
				{
					// None of the above
					pRspFrame[kAVCCommandResponse] = kAVCNotImplementedStatus;
				}
			}
			else
			{
				// We don't support this particular command
				pRspFrame[kAVCCommandResponse] = kAVCNotImplementedStatus;
			}
			break;
			
		/////////////////////////////////////////////////
		//
		// AVC Open Descriptor Command
		//
		/////////////////////////////////////////////////
		case 0x08:  // Open Descriptor
			if ((cType == kAVCControlCommand) && (command[kAVCOperand0] == 0x80))
				pRspFrame[kAVCCommandResponse] = kAVCAcceptedStatus;
			else
				pRspFrame[kAVCCommandResponse] = kAVCNotImplementedStatus;
			break;
			
		/////////////////////////////////////////////////
		//
		// AVC Read Descriptor Command
		//
		/////////////////////////////////////////////////
		case 0x09:  // Read Descriptor
			if ((cType == kAVCControlCommand) && (command[kAVCOperand0] == 0x80) && (cmdLen == 10))
			{
				do
				{
					// Get the address and length from the command
					descriptorRequestedReadAddress = ((unsigned short) command[kAVCOperand5] << 8) + command[kAVCOperand6];  
					descriptorRequestedReadLength = ((unsigned short) command[kAVCOperand3] << 8) + command[kAVCOperand4];
					
					// If length is zero, address must be zero
					if (descriptorRequestedReadLength == 0)
						descriptorRequestedReadAddress = 0;
					
					// Make sure the read address is valid
					if (descriptorRequestedReadAddress > (pVirtualMusicSubunit->statusDescriptorSize-1))
					{
						pRspFrame[kAVCCommandResponse] = kAVCRejectedStatus;
						break;
					}
					
					// Determine how many descriptor bytes to respond with
					if (descriptorRequestedReadLength == 0)
					{
						if (pVirtualMusicSubunit->statusDescriptorSize > 502)
						{
							descriptorActualReadLength = 502;
							descriptorReadResultStatus = kDescriptorMoreToRead;
						}
						else
						{
							descriptorActualReadLength = pVirtualMusicSubunit->statusDescriptorSize;
							descriptorReadResultStatus = kDescriptorCompleteRead;
						}
					}
					else
					{
						if ((descriptorRequestedReadAddress+descriptorRequestedReadLength) <= pVirtualMusicSubunit->statusDescriptorSize)
						{
							if (descriptorRequestedReadLength > 502)
							{
								descriptorActualReadLength = 502;
								descriptorReadResultStatus = kDescriptorMoreToRead;
							}
							else
							{
								descriptorActualReadLength = descriptorRequestedReadLength;
								descriptorReadResultStatus = kDescriptorCompleteRead;
							}
						}
						else
						{
							descriptorActualReadLength = (pVirtualMusicSubunit->statusDescriptorSize - descriptorRequestedReadAddress);
							if (descriptorActualReadLength > 502)
								descriptorActualReadLength = 502;
							descriptorReadResultStatus = kDescriptorDataLenTooLarge;
						}
					}
					
					// Build the response frame
					pRspFrame[kAVCCommandResponse] = kAVCAcceptedStatus;
					pRspFrame[kAVCOperand1] = descriptorReadResultStatus;
					pRspFrame[kAVCOperand3] = ((descriptorActualReadLength & 0xFF00) >> 8);
					pRspFrame[kAVCOperand4] = (descriptorActualReadLength & 0xFF);
					bcopy(&pVirtualMusicSubunit->pStatusDescriptor[descriptorRequestedReadAddress],&pRspFrame[kAVCOperand7],descriptorActualReadLength);
					responseLen = descriptorActualReadLength+10;
					
				}while(false);
			}
			else
			{
				pRspFrame[kAVCCommandResponse] = kAVCNotImplementedStatus;
			}
			break;
			
			/////////////////////////////////////////////////
			//
			// All Other Music Subunit Commands
			//
			/////////////////////////////////////////////////
		default:
			pRspFrame[kAVCCommandResponse] = kAVCNotImplementedStatus;
			break;
	}
	
	/* Send the response */
	(*(pVirtualMusicSubunit->nodeAVCProtocolInterface))->sendAVCResponse(pVirtualMusicSubunit->nodeAVCProtocolInterface,
												 generation,
												 srcNodeID,
												 (const char*) pRspFrame,
												 responseLen);
	// Free allocated response frame
	free(pRspFrame);
	
	return kIOReturnSuccess;
}

//////////////////////////////////////////////////
// VirtualMusicSubunit::AVCUnit_PlugInfoHandlerCallback
//////////////////////////////////////////////////
IOReturn VirtualMusicSubunit::AVCUnit_PlugInfoHandlerCallback( void *refCon,
										  UInt32 generation,
										  UInt16 srcNodeID,
										  IOFWSpeed speed,
										  const UInt8 * command,
										  UInt32 cmdLen)
{
	/* Local Vars */
	VirtualMusicSubunit *pVirtualMusicSubunit = (VirtualMusicSubunit*) refCon;
	
	UInt8 cType = command[kAVCCommandResponse] & 0x0F;
	UInt8 *pRspFrame;
	
	pRspFrame = (UInt8*) malloc(cmdLen);
	if (!pRspFrame)
		return kIOReturnNoMemory;
	
	/* copy cmd frame to rsp frame */
	bcopy(command,pRspFrame,cmdLen);
	
	if ((cType == kAVCStatusInquiryCommand) && (command[kAVCOperand0] == 0x00) && (cmdLen == 8))
	{
		pRspFrame[kAVCOperand1] = 1;	// isoch in plug count
		pRspFrame[kAVCOperand2] = 1;	// isoch out plug count
		pRspFrame[kAVCOperand3] = 2;	// external in plug count
		pRspFrame[kAVCOperand4] = 2;	// external out plug count
		pRspFrame[kAVCCommandResponse] = kAVCImplementedStatus;
	}
	else // must be a control command, so reject it
		pRspFrame[kAVCCommandResponse] = kAVCRejectedStatus;
	
	/* Send the response */
	(*(pVirtualMusicSubunit->nodeAVCProtocolInterface))->sendAVCResponse(pVirtualMusicSubunit->nodeAVCProtocolInterface,
																		 generation,
																		 srcNodeID,
																		 (const char*) pRspFrame,
																		 cmdLen);
	// Free allocated response frame
	free(pRspFrame);
	
	return kIOReturnSuccess;
}

//////////////////////////////////////////////////
// VirtualMusicSubunit::AVCUnit_StreamInfoHandlerCallback
//////////////////////////////////////////////////
IOReturn VirtualMusicSubunit::AVCUnit_StreamInfoHandlerCallback( void *refCon,
											UInt32 generation,
											UInt16 srcNodeID,
											IOFWSpeed speed,
											const UInt8 * command,
											UInt32 cmdLen)
{
	/* Local Vars */
	VirtualMusicSubunit *pVirtualMusicSubunit = (VirtualMusicSubunit*) refCon;
	
	UInt8 cType = command[kAVCCommandResponse] & 0x0F;
	UInt8 *pRspFrame;
	unsigned int responseLen = cmdLen;
	
	pRspFrame = (UInt8*) malloc(512);
	if (!pRspFrame)
		return kIOReturnNoMemory;
	
	/* copy cmd frame to rsp frame */
	bcopy(command,pRspFrame,cmdLen);
	
	// TODO: We should handle this command for the unit external in/out plugs as well!
	
	// Determine if the command is formatted as we expect
	if ((cType == kAVCStatusInquiryCommand) &&
		(command[kAVCOperand0] == 0xC0) &&	// Single Request Subfunction
		((command[kAVCOperand1] == 0) || (command[kAVCOperand1] == 1)) &&	// Input or Output direction
		(command[kAVCOperand2] == 0) &&  // Unit Plug
		(command[kAVCOperand3] == 0) &&  // PCR
		(command[kAVCOperand4] == 0) &&  // Plug number 
		(command[kAVCOperand5] == 0xFF)) // Always 0xFF
	{
		// Fill in the details
		pRspFrame[10] = 0x90; //AM824 
		pRspFrame[11] = 0x40; // Compound Stream
		pRspFrame[12] = pVirtualMusicSubunit->subunitSampleRateToStreamFormatInfoSampleRate();
		pRspFrame[13] = 0x02; // rate-control not supported

#ifdef kVirtualMusicSubunitHasMidi
		pRspFrame[14] = 0x02; // 2 stream info
#else
		pRspFrame[14] = 0x01; // 1 stream info
#endif		
		
		if (command[kAVCOperand1] == 0)
			pRspFrame[15] = pVirtualMusicSubunit->numAudioInputs; // num channels
		else
			pRspFrame[15] = pVirtualMusicSubunit->numAudioOutputs; // num channels
		
		pRspFrame[16] = 0x06; // MBLA
		
#ifdef kVirtualMusicSubunitHasMidi
		pRspFrame[17] = 0x01; // num channels
		pRspFrame[18] = 0x0D; // MIDI
#endif		
		
		pRspFrame[kAVCCommandResponse] = kAVCImplementedStatus;
#ifdef kVirtualMusicSubunitHasMidi
		responseLen = 19;
#else
		responseLen = 17;
#endif		
	}
	else
	{
		// We don't support this particular command
		pRspFrame[kAVCCommandResponse] = kAVCNotImplementedStatus;
	}
	
	/* Send the response */
	(*(pVirtualMusicSubunit->nodeAVCProtocolInterface))->sendAVCResponse(pVirtualMusicSubunit->nodeAVCProtocolInterface,
																		 generation,
																		 srcNodeID,
																		 (const char*) pRspFrame,
																		 responseLen);
	// Free allocated response frame
	free(pRspFrame);
	
	return kIOReturnSuccess;
}

//////////////////////////////////////////////////
// VirtualMusicSubunit::AVCUnit_InputPlugSignalFormatHandlerCallback
//////////////////////////////////////////////////
IOReturn VirtualMusicSubunit::AVCUnit_InputPlugSignalFormatHandlerCallback( void *refCon,
													   UInt32 generation,
													   UInt16 srcNodeID,
													   IOFWSpeed speed,
													   const UInt8 * command,
													   UInt32 cmdLen)
{
	/* Local Vars */
	VirtualMusicSubunit *pVirtualMusicSubunit = (VirtualMusicSubunit*) refCon;
	
	UInt8 cType = command[kAVCCommandResponse] & 0x0F;
	UInt8 *pRspFrame;
	
	// This handler overrides the default OS supplied handler
	// only for specific inquiry type commands
	if (cType != kAVCSpecificInquiryCommand)
		return kIOReturnError;	// Returning an error here will allow the system to invoke the default handler for this command
	
	pRspFrame = (UInt8*) malloc(cmdLen);
	if (!pRspFrame)
		return kIOReturnNoMemory;
	
	/* copy cmd frame to rsp frame */
	bcopy(command,pRspFrame,cmdLen);
	
	// Fill in the response!
	if ((command[kAVCOperand0] == 0) && (command[kAVCOperand1] == 0x90))
		pRspFrame[kAVCCommandResponse] = kAVCImplementedStatus;
	else
		pRspFrame[kAVCCommandResponse] = kAVCNotImplementedStatus;
	
	/* Send the response */
	(*(pVirtualMusicSubunit->nodeAVCProtocolInterface))->sendAVCResponse(pVirtualMusicSubunit->nodeAVCProtocolInterface,
																		 generation,
																		 srcNodeID,
																		 (const char*) pRspFrame,
																		 cmdLen);
	// Free allocated response frame
	free(pRspFrame);
	
	return kIOReturnSuccess;
}

//////////////////////////////////////////////////
// VirtualMusicSubunit::AVCUnit_OutputPlugSignalFormatHandlerCallback
//////////////////////////////////////////////////
IOReturn VirtualMusicSubunit::AVCUnit_OutputPlugSignalFormatHandlerCallback( void *refCon,
														UInt32 generation,
														UInt16 srcNodeID,
														IOFWSpeed speed,
														const UInt8 * command,
														UInt32 cmdLen)
{
	/* Local Vars */
	VirtualMusicSubunit *pVirtualMusicSubunit = (VirtualMusicSubunit*) refCon;
	
	UInt8 cType = command[kAVCCommandResponse] & 0x0F;
	UInt8 *pRspFrame;
	
	// This handler overrides the default OS supplied handler
	// only for specific inquiry type commands
	if (cType != kAVCSpecificInquiryCommand)
		return kIOReturnError;	// Returning an error here will allow the system to invoke the default handler for this command
	
	pRspFrame = (UInt8*) malloc(cmdLen);
	if (!pRspFrame)
		return kIOReturnNoMemory;
	
	/* copy cmd frame to rsp frame */
	bcopy(command,pRspFrame,cmdLen);
	
	// Fill in the response!
	if ((command[kAVCOperand0] == 0) && (command[kAVCOperand1] == 0x90))
		pRspFrame[kAVCCommandResponse] = kAVCImplementedStatus;
	else
		pRspFrame[kAVCCommandResponse] = kAVCNotImplementedStatus;
	
	/* Send the response */
	(*(pVirtualMusicSubunit->nodeAVCProtocolInterface))->sendAVCResponse(pVirtualMusicSubunit->nodeAVCProtocolInterface,
																		 generation,
																		 srcNodeID,
																		 (const char*) pRspFrame,
																		 cmdLen);
	// Free allocated response frame
	free(pRspFrame);
	
	return kIOReturnSuccess;
}

//////////////////////////////////////////////////
// VirtualMusicSubunit::AVCUnit_SignalSourceHandlerCallback
//////////////////////////////////////////////////
IOReturn VirtualMusicSubunit::AVCUnit_SignalSourceHandlerCallback( void *refCon,
											  UInt32 generation,
											  UInt16 srcNodeID,
											  IOFWSpeed speed,
											  const UInt8 * command,
											  UInt32 cmdLen)
{
	/* Local Vars */
	VirtualMusicSubunit *pVirtualMusicSubunit = (VirtualMusicSubunit*) refCon;
	
	UInt8 cType = command[kAVCCommandResponse] & 0x0F;
	UInt8 *pRspFrame;
	
	pRspFrame = (UInt8*) malloc(cmdLen);
	if (!pRspFrame)
		return kIOReturnNoMemory;
	
	/* copy cmd frame to rsp frame */
	bcopy(command,pRspFrame,cmdLen);

	// Initialize the response to not-implemented!
	pRspFrame[kAVCCommandResponse] = kAVCNotImplementedStatus;
	
	if ((cType == kAVCStatusInquiryCommand) &&
		(command[kAVCOperand0] == 0xFF) && 
		(command[kAVCOperand1] == 0xFF) &&
		(command[kAVCOperand2] == 0xFE))
	{
		// Which Destination is specified?
		if ((command[kAVCOperand3] == 0x60) && (command[kAVCOperand4] == 0x00))
		{
			// Destination is music subunit source plug 0
			pRspFrame[kAVCCommandResponse] = kAVCImplementedStatus;
			pRspFrame[kAVCOperand0] = 0x00;	// Output Status, Conv, and Signal Status
			
			// This subunit plug is connected to unit isoch in plug 0 
			pRspFrame[kAVCOperand1] = 0xFF;
			pRspFrame[kAVCOperand2] = 0x00;
		}
		else if ((command[kAVCOperand3] == 0x60) && (command[kAVCOperand4] == 0x01))
		{
			// Destination is music subunit source plug 1
			pRspFrame[kAVCCommandResponse] = kAVCImplementedStatus;
			pRspFrame[kAVCOperand0] = 0x00;	// Output Status, Conv, and Signal Status
			
			// This subunit plug is connected to unit external in plug 0 
			pRspFrame[kAVCOperand1] = 0xFF;
			pRspFrame[kAVCOperand2] = 0x80;
		}
		else if ((command[kAVCOperand3] == 0x60) && (command[kAVCOperand4] == 0x02))
		{
			// Destination is music subunit source plug 2
			pRspFrame[kAVCCommandResponse] = kAVCImplementedStatus;
			pRspFrame[kAVCOperand0] = 0x00;	// Output Status, Conv, and Signal Status
			
			// This subunit plug is connected to unit external in plug 1 
			pRspFrame[kAVCOperand1] = 0xFF;
			pRspFrame[kAVCOperand2] = 0x81;
			
		}
		else if ((command[kAVCOperand3] == 0x60) && (command[kAVCOperand4] == 0x03))
		{
			// Destination is music subunit source plug 3
			pRspFrame[kAVCCommandResponse] = kAVCImplementedStatus;
			pRspFrame[kAVCOperand0] = 0x00;	// Output Status, Conv, and Signal Status
			
			// This subunit plug is connected to unit isoch in plug 0 
			pRspFrame[kAVCOperand1] = 0xFF;
			pRspFrame[kAVCOperand2] = 0x00;
		}
		else if ((command[kAVCOperand3] == 0xFF) && (command[kAVCOperand4] == 0x00))
		{
			// Destination is unit isoch out plug 0
			pRspFrame[kAVCCommandResponse] = kAVCImplementedStatus;
			pRspFrame[kAVCOperand0] = 0x00;	// Output Status, Conv, and Signal Status
			
			// This unit plug is connected to music subunit source plug 0 
			pRspFrame[kAVCOperand1] = 0x60;
			pRspFrame[kAVCOperand2] = 0x00;
		}
		else if ((command[kAVCOperand3] == 0xFF) && (command[kAVCOperand4] == 0x80))
		{
			// Destination is unit external out plug 0
			pRspFrame[kAVCCommandResponse] = kAVCImplementedStatus;
			pRspFrame[kAVCOperand0] = 0x00;	// Output Status, Conv, and Signal Status
			
			// This unit plug is connected to music subunit source plug 01
			pRspFrame[kAVCOperand1] = 0x60;
			pRspFrame[kAVCOperand2] = 0x01;
		}
		else if ((command[kAVCOperand3] == 0xFF) && (command[kAVCOperand4] == 0x81))
		{
			// Destination is unit external out plug 1
			pRspFrame[kAVCCommandResponse] = kAVCImplementedStatus;
			pRspFrame[kAVCOperand0] = 0x00;	// Output Status, Conv, and Signal Status
			
			// This unit plug is connected to music subunit source plug 2 
			pRspFrame[kAVCOperand1] = 0x60;
			pRspFrame[kAVCOperand2] = 0x02;
		}
	}
	
	/* Send the response */
	(*(pVirtualMusicSubunit->nodeAVCProtocolInterface))->sendAVCResponse(pVirtualMusicSubunit->nodeAVCProtocolInterface,
																		 generation,
																		 srcNodeID,
																		 (const char*) pRspFrame,
																		 cmdLen);
	// Free allocated response frame
	free(pRspFrame);
	
	return kIOReturnSuccess;
}

//////////////////////////////////////////////////
// VirtualMusicSubunit::updateMusicSubunitDescriptorWithNewSampleRate
//////////////////////////////////////////////////
void VirtualMusicSubunit::updateMusicSubunitDescriptorWithNewSampleRate(void)
{
	// TODO
}

//////////////////////////////////////////////////
// VirtualMusicSubunit::subunitSampleRateToPayloadInQuadlets
//////////////////////////////////////////////////
UInt32 VirtualMusicSubunit::subunitSampleRateToPayloadInQuadlets(void)
{
	UInt32 retVal;

	// This formula assumes 6 streams of audio and 1 stream of MIDI = 7 total quadlets per source packet
	// It also assumes blocking mode (worst-case payload size)
	
	switch (sampleRate)
	{
		case kMusicSubunitSampleRate_32000:
			retVal = (7*8);
			break;
			
		case kMusicSubunitSampleRate_44100:
			retVal = (7*8);
			break;
			
		case kMusicSubunitSampleRate_88200:
			retVal = (7*16);
			break;
			
		case kMusicSubunitSampleRate_96000:
			retVal = (7*16);
			break;
			
		case kMusicSubunitSampleRate_176400:
			retVal = (7*32);
			break;
			
		case kMusicSubunitSampleRate_192000:
			retVal = (7*32);
			break;
			
		case kMusicSubunitSampleRate_48000:
		default:
			retVal = (7*8);
			break;
	};
	
	return retVal;
}	

//////////////////////////////////////////////////
// VirtualMusicSubunit::subunitSampleRateToStreamFormatInfoSampleRate
//////////////////////////////////////////////////
UInt8 VirtualMusicSubunit::subunitSampleRateToStreamFormatInfoSampleRate(void)
{
	UInt8 retVal;

	// Unfortunately, the sample rate constants used by the AM824 signal-format commands
	// are completely different from those used by the stream format info command.
	// This function determines the correct value to use with the stream-format info command,
	// based on the current subunit sample rate. 
	
	switch (sampleRate)
	{
		case kMusicSubunitSampleRate_32000:
			retVal = 2;
			break;
			
		case kMusicSubunitSampleRate_44100:
			retVal = 3;
			break;
			
		case kMusicSubunitSampleRate_88200:
			retVal = 0xA;
			break;
			
		case kMusicSubunitSampleRate_96000:
			retVal = 5;
			break;
			
		case kMusicSubunitSampleRate_176400:
			retVal = 6;
			break;
			
		case kMusicSubunitSampleRate_192000:
			retVal = 7;
			break;

		case kMusicSubunitSampleRate_48000:
		default:
			retVal = 4;
			break;
	};
	
	return retVal;
}

//////////////////////////////////////////////////
// VirtualMusicSubunit::startInputPlugReconnectTimer
//////////////////////////////////////////////////
void VirtualMusicSubunit::startInputPlugReconnectTimer( void )
{
	CFRunLoopTimerContext		context;
	CFAbsoluteTime				time;
	
    // stop if necessary
    stopInputPlugReconnectTimer();
	
    context.version             = 0;
    context.info                = this;
    context.retain              = NULL;
    context.release             = NULL;
    context.copyDescription     = NULL;
	
    time = CFAbsoluteTimeGetCurrent() + kMusicSubunitCMPBusResetReconnectTime;
	
	inputPlugReconnectTimer = CFRunLoopTimerCreate(NULL, time,
												   0,
												   0,
												   0,
												   (CFRunLoopTimerCallBack)&InputPlugReconnectTimeoutHelper,
												   &context);
	
	if ( inputPlugReconnectTimer )
		CFRunLoopAddTimer( runLoopRef, inputPlugReconnectTimer, kCFRunLoopDefaultMode );
}

//////////////////////////////////////////////////
// VirtualMusicSubunit::startOutputPlugReconnectTimer
//////////////////////////////////////////////////
void VirtualMusicSubunit::startOutputPlugReconnectTimer( void )
{
	CFRunLoopTimerContext		context;
	CFAbsoluteTime				time;
	
    // stop if necessary
    stopOutputPlugReconnectTimer();
	
    context.version             = 0;
    context.info                = this;
    context.retain              = NULL;
    context.release             = NULL;
    context.copyDescription     = NULL;
	
    time = CFAbsoluteTimeGetCurrent() + kMusicSubunitCMPBusResetReconnectTime;
	
	outputPlugReconnectTimer = CFRunLoopTimerCreate(NULL, time,
													0,
													0,
													0,
													(CFRunLoopTimerCallBack)&OutputPlugReconnectTimeoutHelper,
													&context);
	
	if ( outputPlugReconnectTimer )
		CFRunLoopAddTimer( runLoopRef, outputPlugReconnectTimer, kCFRunLoopDefaultMode );
}

//////////////////////////////////////////////////
// VirtualMusicSubunit::stopInputPlugReconnectTimer
//////////////////////////////////////////////////
void VirtualMusicSubunit::stopInputPlugReconnectTimer( void )
{
	if ( inputPlugReconnectTimer )
	{
		CFRunLoopTimerInvalidate( inputPlugReconnectTimer );
		CFRelease( inputPlugReconnectTimer );
		inputPlugReconnectTimer = NULL;
	}
}

//////////////////////////////////////////////////
// VirtualMusicSubunit::stopOutputPlugReconnectTimer
//////////////////////////////////////////////////
void VirtualMusicSubunit::stopOutputPlugReconnectTimer( void )
{
	if ( outputPlugReconnectTimer )
	{
		CFRunLoopTimerInvalidate( outputPlugReconnectTimer );
		CFRelease( outputPlugReconnectTimer );
		outputPlugReconnectTimer = NULL;
	}
}

//////////////////////////////////////////////////
// VirtualMusicSubunit::inputPlugReconnectTimeout
//////////////////////////////////////////////////
void VirtualMusicSubunit::inputPlugReconnectTimeout(void)
{
    UInt32 currentP2PCount = ((isochInPlugVal & 0x3F000000) >> 24);
	UInt32 currentChannel = ((isochInPlugVal & 0x003F0000) >> 16);;
	
	// Stop/delete the timer
	stopInputPlugReconnectTimer();
	
	if (currentP2PCount == 0)
	{
		// Clear the on-line bit in the iPCR
		setNewInputPlugValue((isochInPlugVal & 0x7FFFFFFF));
		
		if (clientCMPConnectionHandler != nil)
			clientCMPConnectionHandler(pClientCallbackRefCon, true, currentChannel, 0xFF, currentP2PCount);
	}
}

//////////////////////////////////////////////////
// VirtualMusicSubunit::outputPlugReconnectTimeout
//////////////////////////////////////////////////
void VirtualMusicSubunit::outputPlugReconnectTimeout(void)
{
    UInt32 currentP2PCount = ((isochOutPlugVal & 0x3F000000) >> 24);
	UInt32 currentChannel = ((isochOutPlugVal & 0x003F0000) >> 16);;
	UInt32 currentSpeed = ((isochOutPlugVal & 0x0000C000) >> 14);
	
	// Stop/delete the timer
	stopOutputPlugReconnectTimer();
	
	if ((currentP2PCount == 0) && (clientCMPConnectionHandler != nil))
		clientCMPConnectionHandler(pClientCallbackRefCon, false, currentChannel, currentSpeed, currentP2PCount);
}

//////////////////////////////////////////////////////
// VirtualMusicSubunit::AVCSubUnitPlugHandlerCallback
//////////////////////////////////////////////////////
IOReturn VirtualMusicSubunit::AVCSubUnitPlugHandlerCallback(UInt32 subunitTypeAndID,
														   IOFWAVCPlugTypes plugType,
														   UInt32 plugID,
														   IOFWAVCSubunitPlugMessages plugMessage,
														   UInt32 messageParams)
{
	IOReturn result = kIOReturnSuccess;
	UInt32 currentP2PCount;
    UInt32 newP2PCount = ((messageParams & 0x3F000000) >> 24);
	UInt32 newChannel = ((messageParams & 0x003F0000) >> 16);;
	UInt32 newSpeed = ((messageParams & 0x0000C000) >> 14);
	bool isReconnectAfterBusReset = false;
	MusicSubunitSampleRate newSampleRate = 0;
	
	// Handle the various plug messages
	switch (plugMessage)
	{
		////////////////////////////////////	
		// CMP plug register modification
		////////////////////////////////////	
		case kIOFWAVCSubunitPlugMsgConnectedPlugModified:
			switch (plugType)
			{
				// A CMP operation on our connected oPCR
				case IOFWAVCPlugSubunitSourceType:
					if ((newP2PCount > 0) && (outputPlugReconnectTimer != nil))
					{
						isReconnectAfterBusReset = true;
						stopOutputPlugReconnectTimer();
					}
					currentP2PCount = ((isochOutPlugVal & 0x3F000000) >> 24);
					if (newP2PCount == currentP2PCount)
						break;	// If the p2p hasn't changed, we can ignore this callback
						isochOutPlugVal = messageParams; // Save the new plug value
					if (newP2PCount == 0)
					{
						// Start the timer to wait before alerting client
						startOutputPlugReconnectTimer();
					}
						else if ((clientCMPConnectionHandler != nil) && (isReconnectAfterBusReset == false))
						{
							// Notify the client of the new connection
							clientCMPConnectionHandler(pClientCallbackRefCon, false, newChannel, newSpeed, newP2PCount);
						}
						break;
					
					// A CMP operation on our connected iPCR	
				case IOFWAVCPlugSubunitDestType:
					if ((newP2PCount > 0) && (inputPlugReconnectTimer != nil))
					{
						isReconnectAfterBusReset = true;
						stopInputPlugReconnectTimer();
					}
					currentP2PCount = ((isochInPlugVal & 0x3F000000) >> 24);
					if (newP2PCount == currentP2PCount)
						break;	// If the p2p hasn't changed, we can ignore this callback
						isochInPlugVal = messageParams; // Save the new plug value
					if (newP2PCount == 0)
					{
						// Start the timer to wait before alerting client
						startInputPlugReconnectTimer();
					}
						else if ((clientCMPConnectionHandler != nil) && (isReconnectAfterBusReset == false))
						{
							// Set the on-line bit in our iPCR
							setNewInputPlugValue((isochInPlugVal | 0x80000000));
							
							// Notify the client of the new connection
							clientCMPConnectionHandler(pClientCallbackRefCon, true, newChannel, 0xFF, newP2PCount);
						}
						break;
					
				default:
					break;
			};
			break;
			
		case kIOFWAVCSubunitPlugMsgSignalFormatModified:
			//printf("Sample Rate Change. New Signal Format: 0x%08X\n",(int)messageParams);
			if ((clientSampleRateChangeHandler != nil) && ((messageParams & 0x90000000) == 0x90000000)) 
			{
				newSampleRate = ((messageParams & 0x00FF0000) >> 16);
				//printf("AY_DEBUG: newSampleRate = %d\n",newSampleRate);
				if (newSampleRate == sampleRate)
					result = kIOReturnSuccess;
				else
				{
					result = clientSampleRateChangeHandler(pClientCallbackRefCon, newSampleRate);
					if (result == kIOReturnSuccess)
					{
						updateMusicSubunitDescriptorWithNewSampleRate();
						// TODO: Need to update payload size in plug register
					}
				}
			}
			else
			{
				// reject the sample rate change
				result = kIOReturnError;	
			}
			break;
			
			//////////////////////////////////////////	
			// Not a callback we need to deal with
			//////////////////////////////////////////	
		default:
			break;
	};
	
	return result;
}

//////////////////////////////////////////////////////
// VirtualMusicSubunit::GetNub
//////////////////////////////////////////////////////
IOFireWireLibNubRef VirtualMusicSubunit::GetNub(void)
{
	return nodeNubInterface;
}

//////////////////////////////////////////////
// VirtualMusicSubunit::generateDiscriptorStructs
//////////////////////////////////////////////
void VirtualMusicSubunit::generateDiscriptorStructs(void)
{
	////////////////////////////////////////////////////////////////////////////////////////
	//
	//  NOTE: This function makes assumptions as to the layout of the subunit plugs and 
	//        clusters. Its only function is to suport a dynamic number of audio in and 
	//        audio out plugs.
	//
	//  It assumes that:
	//          subunit dest plug 0 is isoch in
	//          subunit dest plug 1 is audio in
	//          subunit dest plug 2 is midi in
	//          subunit dest plug 3 is sync in
	//
	//          subunit source plug 0 is isoch out
	//          subunit source plug 1 is audio out
	//          subunit source plug 2 is midi out
	//
	//          cluster 0 is audio out
	//          cluster 1 is midi out
	//          cluster 2 is audio in
	//          cluster 3 is midi in
	//          cluster 4 is sync out
	//
	//          The isoch streams are n-number of audio signals followed by one midi signal 
	//
	////////////////////////////////////////////////////////////////////////////////////////
	
	UInt32 i;
	
	// Calculate the number of music plugs we need
	numMusicPlugs = numAudioInputs + numAudioOutputs + 2 + 1;	// n-audio, 2-midi, 1-sync

	// Create the music plugs
	pMusicSubunitMusicPlugs = new MusicPlug[numMusicPlugs];
	
	for (i=0;i<numAudioOutputs;i++)
	{
		pMusicSubunitMusicPlugs[i].pName = NULL;
		pMusicSubunitMusicPlugs[i].musicPlugID = i;
		pMusicSubunitMusicPlugs[i].type = kMusicPlugTypeAudio;
		pMusicSubunitMusicPlugs[i].routingSupport = kMusicPlugRoutingFixed;
		pMusicSubunitMusicPlugs[i].sourcePlugID = 0;
		pMusicSubunitMusicPlugs[i].sourceStreamPosition = i;
		pMusicSubunitMusicPlugs[i].sourceStreamLocation = kMusicPlugLocationUnknown;
		pMusicSubunitMusicPlugs[i].destPlugID = 1;
		pMusicSubunitMusicPlugs[i].destStreamPosition = i;
		pMusicSubunitMusicPlugs[i].destStreamLocation = kMusicPlugLocationUnknown;
	}

	for (i=0;i<numAudioInputs;i++)
	{
		pMusicSubunitMusicPlugs[i+numAudioOutputs].pName = NULL;
		pMusicSubunitMusicPlugs[i+numAudioOutputs].musicPlugID = i+numAudioOutputs;
		pMusicSubunitMusicPlugs[i+numAudioOutputs].type = kMusicPlugTypeAudio;
		pMusicSubunitMusicPlugs[i+numAudioOutputs].routingSupport = kMusicPlugRoutingFixed;
		pMusicSubunitMusicPlugs[i+numAudioOutputs].sourcePlugID = 1;
		pMusicSubunitMusicPlugs[i+numAudioOutputs].sourceStreamPosition = i;
		pMusicSubunitMusicPlugs[i+numAudioOutputs].sourceStreamLocation = kMusicPlugLocationUnknown;
		pMusicSubunitMusicPlugs[i+numAudioOutputs].destPlugID = 0;
		pMusicSubunitMusicPlugs[i+numAudioOutputs].destStreamPosition = i;
		pMusicSubunitMusicPlugs[i+numAudioOutputs].destStreamLocation = kMusicPlugLocationUnknown;
	}

	
	// Midi Out Plug 
	pMusicSubunitMusicPlugs[numAudioOutputs+numAudioInputs].pName = NULL;
	pMusicSubunitMusicPlugs[numAudioOutputs+numAudioInputs].musicPlugID = numAudioOutputs+numAudioInputs;
	pMusicSubunitMusicPlugs[numAudioOutputs+numAudioInputs].type = kMusicPlugTypeMidi;
	pMusicSubunitMusicPlugs[numAudioOutputs+numAudioInputs].routingSupport = kMusicPlugRoutingFixed;
	pMusicSubunitMusicPlugs[numAudioOutputs+numAudioInputs].sourcePlugID = 0;
	pMusicSubunitMusicPlugs[numAudioOutputs+numAudioInputs].sourceStreamPosition = numAudioOutputs;
	pMusicSubunitMusicPlugs[numAudioOutputs+numAudioInputs].sourceStreamLocation = 0;
	pMusicSubunitMusicPlugs[numAudioOutputs+numAudioInputs].destPlugID = 2;
	pMusicSubunitMusicPlugs[numAudioOutputs+numAudioInputs].destStreamPosition = 0;
	pMusicSubunitMusicPlugs[numAudioOutputs+numAudioInputs].destStreamLocation = 0;
	
	// Midi In Plug
	pMusicSubunitMusicPlugs[numAudioOutputs+numAudioInputs+1].pName = NULL;
	pMusicSubunitMusicPlugs[numAudioOutputs+numAudioInputs+1].musicPlugID = numAudioOutputs+numAudioInputs+1;
	pMusicSubunitMusicPlugs[numAudioOutputs+numAudioInputs+1].type = kMusicPlugTypeMidi;
	pMusicSubunitMusicPlugs[numAudioOutputs+numAudioInputs+1].routingSupport = kMusicPlugRoutingFixed;
	pMusicSubunitMusicPlugs[numAudioOutputs+numAudioInputs+1].sourcePlugID = 2;
	pMusicSubunitMusicPlugs[numAudioOutputs+numAudioInputs+1].sourceStreamPosition = 0;
	pMusicSubunitMusicPlugs[numAudioOutputs+numAudioInputs+1].sourceStreamLocation = 0;
	pMusicSubunitMusicPlugs[numAudioOutputs+numAudioInputs+1].destPlugID = 0;
	pMusicSubunitMusicPlugs[numAudioOutputs+numAudioInputs+1].destStreamPosition = numAudioInputs;
	pMusicSubunitMusicPlugs[numAudioOutputs+numAudioInputs+1].destStreamLocation = 0;
	
	// Sync In Plug
	pMusicSubunitMusicPlugs[numAudioOutputs+numAudioInputs+2].pName = NULL;
	pMusicSubunitMusicPlugs[numAudioOutputs+numAudioInputs+2].musicPlugID = numAudioOutputs+numAudioInputs+2;
	pMusicSubunitMusicPlugs[numAudioOutputs+numAudioInputs+2].type = kMusicPlugTypeMidi;
	pMusicSubunitMusicPlugs[numAudioOutputs+numAudioInputs+2].routingSupport = kMusicPlugRoutingFixed;
	pMusicSubunitMusicPlugs[numAudioOutputs+numAudioInputs+2].sourcePlugID = 3;
	pMusicSubunitMusicPlugs[numAudioOutputs+numAudioInputs+2].sourceStreamPosition = 0;
	pMusicSubunitMusicPlugs[numAudioOutputs+numAudioInputs+2].sourceStreamLocation = kMusicPlugLocationUnknown;
	pMusicSubunitMusicPlugs[numAudioOutputs+numAudioInputs+2].destPlugID = 0xFF;
	pMusicSubunitMusicPlugs[numAudioOutputs+numAudioInputs+2].destStreamPosition = 0xFF;
	pMusicSubunitMusicPlugs[numAudioOutputs+numAudioInputs+2].destStreamLocation = kMusicPlugLocationUnknown;

	// Allocate the cluster arrays
	ppAudioOutClusterPlugArray = new MusicPlug*[numAudioOutputs];
	ppMidiOutClusterPlugArray = new MusicPlug*[1];
	ppAudioInClusterPlugArray = new MusicPlug*[numAudioInputs];
	ppMidiInClusterPlugArray = new MusicPlug*[1];
	ppSyncClusterPlugArray = new MusicPlug*[1];

	// Fill in the cluster arrays
	for (i=0;i<numAudioOutputs;i++)
		ppAudioOutClusterPlugArray[i] = &pMusicSubunitMusicPlugs[i];
	for (i=0;i<numAudioInputs;i++)
		ppAudioInClusterPlugArray[i] = &pMusicSubunitMusicPlugs[i+numAudioOutputs];
	ppMidiOutClusterPlugArray[0] = &pMusicSubunitMusicPlugs[numAudioOutputs+numAudioInputs];
	ppMidiInClusterPlugArray[0] = &pMusicSubunitMusicPlugs[numAudioOutputs+numAudioInputs+1];
	ppSyncClusterPlugArray[0] = &pMusicSubunitMusicPlugs[numAudioOutputs+numAudioInputs+2];

	// Change the number of audio in and out signals in the clusters
	musicPlugClusters[0].numPlugs = numAudioOutputs;	// Audio output cluster
	musicPlugClusters[0].ppMusicPlugs = ppAudioOutClusterPlugArray;

	musicPlugClusters[1].numPlugs = 1;	// Midi out cluster
	musicPlugClusters[1].ppMusicPlugs = ppMidiOutClusterPlugArray;

	musicPlugClusters[2].numPlugs = numAudioInputs;		// Audio input cluster
	musicPlugClusters[2].ppMusicPlugs = ppAudioInClusterPlugArray;

	musicPlugClusters[3].numPlugs = 1;		// Midi input cluster
	musicPlugClusters[3].ppMusicPlugs = ppMidiInClusterPlugArray;
		
	musicPlugClusters[4].numPlugs = 1;		// Sync input cluster
	musicPlugClusters[4].ppMusicPlugs = ppSyncClusterPlugArray;
}

//////////////////////////////////////////////
// VirtualMusicSubunit::SizeOfMusicPlugInfoBlock
//////////////////////////////////////////////
unsigned short VirtualMusicSubunit::SizeOfMusicPlugInfoBlock(MusicPlug *pMusicPlug)
{
	/////////////////////////////////////////////////////////////////////
	// The size of a Music Plug Info Block includes the following:
	//
	// compound length:			2
	// info block type:			2
	// primary length:			2
	// music plug type:			1
	// music plug id:			2
	// routing support:			1
	// connected source plug:   5
	// connected dest plug:     5
	// name (raw text):			6 + strlen(name)
	//                          -----------------
	//              total:		26 + strlen(name)
	/////////////////////////////////////////////////////////////////////
	
	if (pMusicPlug->pName != NULL)
		return (26+strlen(pMusicPlug->pName));
	else
		return (20);
}

//////////////////////////////////////////////
// VirtualMusicSubunit::SizeOfMusicPlugClusterInfoBlock
//////////////////////////////////////////////
unsigned short VirtualMusicSubunit::SizeOfMusicPlugClusterInfoBlock(MusicPlugCluster *pCluster)
{
	/////////////////////////////////////////////////////////////////////
	// The size of a Music Plug Cluster Info Block includes the following:
	//
	// compound length:			2
	// info block type:			2
	// primary length:			2
	// format:					1
	// port type:				1
	// number of signals:       1
	// name (raw text):			6 + strlen(name)
	// signals:					4*number_of_signals
	//							------------------------------
	//              total:      15 + strlen(name) + 4*number_of_signals
	/////////////////////////////////////////////////////////////////////
	
	if (pCluster->pName != NULL)
		return (15+strlen(pCluster->pName)+(4*pCluster->numPlugs));
	else
		return (9+(4*pCluster->numPlugs));
}

//////////////////////////////////////////////
// VirtualMusicSubunit::SizeOfMusicSubunitPlugInfoBlock
//////////////////////////////////////////////
unsigned short VirtualMusicSubunit::SizeOfMusicSubunitPlugInfoBlock(MusicSubunitPlug *pSubunitPlug)
{
	/////////////////////////////////////////////////////////////////////
	// The size of a Music Subunit Plug Info Block includes the following:
	//
	// compound length:			2
	// info block type:			2
	// primary length:			2
	// subunit plug id:			1
	// signal format (fdf/fmt): 2
	// plug usage:				1
	// number of clusters:		2
	// number of channels:		2
	// name (raw text):			6 + strlen(name)
	// clusters:				size of each nested cluster info block
	//							-------------------------------------
	//              total:      20 + strlen(name) + size of all nested cluster info blocks
	/////////////////////////////////////////////////////////////////////
	
	unsigned char cluster;
	unsigned short clusterInfoBlocksLen = 0;
	
	for (cluster=0;cluster< pSubunitPlug->numClusters;cluster++)
		clusterInfoBlocksLen += SizeOfMusicPlugClusterInfoBlock(pSubunitPlug->ppMusicPlugClusters[cluster]);
	
	if (pSubunitPlug->pName != NULL)
		return (20+strlen(pSubunitPlug->pName)+clusterInfoBlocksLen);
	else
		return (14+clusterInfoBlocksLen);
}

///////////////////////////////////////////////
// VirtualMusicSubunit::SizeOfMusicSubinitRoutingStatusInfoBlock
///////////////////////////////////////////////
unsigned short VirtualMusicSubunit::SizeOfMusicSubinitRoutingStatusInfoBlock(void)
{
	/////////////////////////////////////////////////////////////////////
	// The size of the music subunit routing status Info Block includes the following:
	//
	// compound length:			2
	// info block type:			2
	// primary length:			2
	// num  dest plugs:			1
	// num  source plugs:		1
	// num music plugs:			2
	// src plugs:				size of each nested src plug info block
	// dst plugs:				size of each nested dest plug info block
	// music plugs:				size of each nested music plug info block
	//							-------------------------------------
	//              total:      10 + size of all nested info blocks
	/////////////////////////////////////////////////////////////////////
	
	unsigned short srcPlugInfoBlockArraySize = 0;
	unsigned short dstPlugInfoBlockArraySize = 0;
	unsigned short musicPlugInfoBlockArraySize = 0;
	unsigned short i;
	
	for (i=0;i<kNumMusicSubunitSourcePlugs;i++)
		srcPlugInfoBlockArraySize += SizeOfMusicSubunitPlugInfoBlock(&SubunitSourcePlugs[i]);
	
	for (i=0;i<kNumMusicSubunitDestPlugs;i++)
		dstPlugInfoBlockArraySize += SizeOfMusicSubunitPlugInfoBlock(&SubunitDestPlugs[i]);
	
	for (i=0;i<numMusicPlugs;i++)
		musicPlugInfoBlockArraySize	+= SizeOfMusicPlugInfoBlock(&pMusicSubunitMusicPlugs[i]);
	
	return (10+srcPlugInfoBlockArraySize+dstPlugInfoBlockArraySize+musicPlugInfoBlockArraySize);
}

///////////////////////////
// VirtualMusicSubunit::SetByteBuf
///////////////////////////
void VirtualMusicSubunit::SetByteBuf(unsigned char *pByteBuf)
{
	pDescriptorByteBuf = pByteBuf;
}

///////////////////////////
// VirtualMusicSubunit::SetNextByte
///////////////////////////
void VirtualMusicSubunit::SetNextByte(unsigned char byteVal)
{
	*pDescriptorByteBuf = byteVal;
	pDescriptorByteBuf++;
}

///////////////////////////
// VirtualMusicSubunit::SetNameInfoBlockBytes
///////////////////////////
void VirtualMusicSubunit::SetNameInfoBlockBytes(char* str, unsigned short len)
{
	// Build the name info block bytes
	SetNextByte((((len+4) & 0xFF00) >> 8));
	SetNextByte(((len+4) & 0xFF));
	SetNextByte(0x00);
	SetNextByte(0x0A);
	SetNextByte(((len & 0xFF00) >> 8));
	SetNextByte((len & 0xFF));
	for (unsigned int i = 0; i< len; i++)
		SetNextByte(str[i]);
}

///////////////////////////
// VirtualMusicSubunit::SetMusicPlugInfoBlockBytes
///////////////////////////
void VirtualMusicSubunit::SetMusicPlugInfoBlockBytes(MusicPlug *pMusicPlug)
{
	unsigned short infoBlockLen = SizeOfMusicPlugInfoBlock(pMusicPlug);
	
	unsigned short compoundLen = infoBlockLen-2;
	unsigned short primaryLen = 14;
	
	// Build the  info block
	SetNextByte(((compoundLen & 0xFF00) >> 8));
	SetNextByte((compoundLen & 0xFF));
	SetNextByte(((kMusicPlugInfoBlockType & 0xFF00) >> 8));
	SetNextByte((kMusicPlugInfoBlockType & 0xFF));
	SetNextByte(((primaryLen & 0xFF00) >> 8));
	SetNextByte((primaryLen & 0xFF));
	SetNextByte(pMusicPlug->type);
	SetNextByte(((pMusicPlug->musicPlugID & 0xFF00) >> 8));
	SetNextByte((pMusicPlug->musicPlugID & 0xFF));
	SetNextByte(pMusicPlug->routingSupport);
	SetNextByte(0xF0);
	SetNextByte(pMusicPlug->sourcePlugID);
	SetNextByte(0xFF);
	SetNextByte(pMusicPlug->sourceStreamPosition);
	SetNextByte(pMusicPlug->sourceStreamLocation);
	SetNextByte(0xF1);
	SetNextByte(pMusicPlug->destPlugID);
	SetNextByte(0xFF);
	SetNextByte(pMusicPlug->destStreamPosition);
	SetNextByte(pMusicPlug->destStreamLocation);
	if (pMusicPlug->pName != NULL)
		SetNameInfoBlockBytes(pMusicPlug->pName,strlen(pMusicPlug->pName));
}

///////////////////////////
// VirtualMusicSubunit::SetClusterInfoBlockBytes
///////////////////////////
void VirtualMusicSubunit::SetClusterInfoBlockBytes(MusicPlugCluster *pCluster, bool isInputCluster)
{
	unsigned short infoBlockLen = SizeOfMusicPlugClusterInfoBlock(pCluster);
	unsigned short compoundLen = infoBlockLen-2;
	unsigned short primaryLen = 3+(4*pCluster->numPlugs);
	
	// Build the info block
	SetNextByte(((compoundLen & 0xFF00) >> 8));
	SetNextByte((compoundLen & 0xFF));
	SetNextByte(((kClusterInfoBlockType & 0xFF00) >> 8));
	SetNextByte((kClusterInfoBlockType & 0xFF));
	SetNextByte(((primaryLen & 0xFF00) >> 8));
	SetNextByte((primaryLen & 0xFF));
	SetNextByte(pCluster->format);
	SetNextByte(pCluster->portType);
	SetNextByte(pCluster->numPlugs);
	for (unsigned int i=0; i< pCluster->numPlugs; i++)
	{
		SetNextByte(((pCluster->ppMusicPlugs[i]->musicPlugID & 0xFF00) >> 8));
		SetNextByte((pCluster->ppMusicPlugs[i]->musicPlugID & 0xFF));
		
		if (isInputCluster == true)
		{
			SetNextByte(pCluster->ppMusicPlugs[i]->sourceStreamPosition);
			SetNextByte(pCluster->ppMusicPlugs[i]->sourceStreamLocation);
		}
		else
		{
			SetNextByte(pCluster->ppMusicPlugs[i]->destStreamPosition);
			SetNextByte(pCluster->ppMusicPlugs[i]->destStreamLocation);
		}
	}
	if (pCluster->pName != NULL)
		SetNameInfoBlockBytes(pCluster->pName,strlen(pCluster->pName));
}

///////////////////////////
// VirtualMusicSubunit::SetSubunitPlugInfoBlock
///////////////////////////
void VirtualMusicSubunit::SetSubunitPlugInfoBlock(MusicSubunitPlug *pSubunitPlug, bool isSubunitSourcePlug)
{
	unsigned short infoBlockLen = SizeOfMusicSubunitPlugInfoBlock(pSubunitPlug);
	unsigned short compoundLen = infoBlockLen-2;
	unsigned short primaryLen = 8;
	unsigned short numChannels = 0;
	unsigned int i;
	
	// Calculate number of channels for this subunit plug
	for (i=0;i<pSubunitPlug->numClusters;i++)
		numChannels += pSubunitPlug->ppMusicPlugClusters[i]->numPlugs;
	
	SetNextByte(((compoundLen & 0xFF00) >> 8));
	SetNextByte((compoundLen & 0xFF));
	SetNextByte(((kSubunitPlugInfoBlockType & 0xFF00) >> 8));
	SetNextByte((kSubunitPlugInfoBlockType & 0xFF));
	SetNextByte(((primaryLen & 0xFF00) >> 8));
	SetNextByte((primaryLen & 0xFF));
	SetNextByte(pSubunitPlug->subunitPlugID);
	SetNextByte(((pSubunitPlug->fdf_fmt & 0xFF00) >> 8));
	SetNextByte((pSubunitPlug->fdf_fmt & 0xFF));
	SetNextByte(pSubunitPlug->plugUsage);
	SetNextByte(((pSubunitPlug->numClusters & 0xFF00) >> 8));
	SetNextByte((pSubunitPlug->numClusters & 0xFF));
	SetNextByte(((numChannels & 0xFF00) >> 8));
	SetNextByte((numChannels & 0xFF));
	for (i=0;i<pSubunitPlug->numClusters;i++)
		SetClusterInfoBlockBytes(pSubunitPlug->ppMusicPlugClusters[i],(isSubunitSourcePlug == true) ? false : true);
	if (pSubunitPlug->pName != NULL)
		SetNameInfoBlockBytes(pSubunitPlug->pName,strlen(pSubunitPlug->pName));
}

///////////////////////////////
// VirtualMusicSubunit::SetRoutingStatusInfoBlock
///////////////////////////////
void VirtualMusicSubunit::SetRoutingStatusInfoBlock(void)
{
	unsigned short infoBlockLen = SizeOfMusicSubinitRoutingStatusInfoBlock();
	unsigned short compoundLen = infoBlockLen-2;
	unsigned short primaryLen = 4;
	unsigned int i;
	
	SetNextByte(((compoundLen & 0xFF00) >> 8));
	SetNextByte((compoundLen & 0xFF));
	SetNextByte(((kRoutingStatusInfoBlockType & 0xFF00) >> 8));
	SetNextByte((kRoutingStatusInfoBlockType & 0xFF));
	SetNextByte(((primaryLen & 0xFF00) >> 8));
	SetNextByte((primaryLen & 0xFF));
	SetNextByte(kNumMusicSubunitDestPlugs); 
	SetNextByte(kNumMusicSubunitSourcePlugs);
	SetNextByte(((numMusicPlugs & 0xFF00) >> 8));
	SetNextByte((numMusicPlugs & 0xFF));
	for (i=0;i<kNumMusicSubunitDestPlugs;i++)
		SetSubunitPlugInfoBlock(&SubunitDestPlugs[i], false);
	for (i=0;i<kNumMusicSubunitSourcePlugs;i++)
		SetSubunitPlugInfoBlock(&SubunitSourcePlugs[i], true);
	for (i=0;i<numMusicPlugs;i++)
		SetMusicPlugInfoBlockBytes(&pMusicSubunitMusicPlugs[i]);
}

} // namespace AVS	
