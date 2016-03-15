/*
	File:		VirtualTapeSubunit.cpp
 
 Synopsis: This is the implementation file for the VirtualTapeSubunit class.
 
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
	
// Local Prototypes
static unsigned int bcd2bin(unsigned int input);
static UInt32 intToBCD(unsigned int value);

// Thread parameter structures
struct VirtualTapeSubunitThreadParams
{
	volatile bool threadReady;
	VirtualTapeSubunit *pTapeSubunit;
	UInt8 initialSignalMode;
	TapeMediumInfo initialMediumInfo;
	VirtualTapeCMPConnectionHandler cmpConnectionHandler;
	VirtualTapeTransportStateChangeHandler transportStateChangeHandler; 
	VirtualTapeSignalModeChangeHandler signalModeChangeHandler;
	VirtualTapeTimeCodeRepositionHandler timeCodeRepositionHandler;
	void *pCallbackRefCon;
	AVCDevice *pAVCDevice;
};

//////////////////////////////////////////////////////////////////////
// VirtualTapeSubunitThreadStart
//////////////////////////////////////////////////////////////////////
static void *VirtualTapeSubunitThreadStart(VirtualTapeSubunitThreadParams* pParams)
{
	IOReturn result = kIOReturnSuccess ;
	VirtualTapeSubunit *pTapeSubunit;
	
	// Instantiate a new receiver object
	pTapeSubunit = new VirtualTapeSubunit(pParams->initialSignalMode,
										  pParams->initialMediumInfo,
										  pParams->cmpConnectionHandler,
										  pParams->transportStateChangeHandler, 
										  pParams->signalModeChangeHandler,
										  pParams->timeCodeRepositionHandler,
										  pParams->pCallbackRefCon);
	
	// Setup the receiver object
	if (pTapeSubunit)
	{
		if (pParams->pAVCDevice)
			result = pTapeSubunit->setupVirtualTapeSubunitWithAVCDevice(pParams->pAVCDevice);
		else
			result = pTapeSubunit->setupVirtualTapeSubunit();
	}
		
	// Update the return parameter with a pointer to the new receiver object
	if (result == kIOReturnSuccess)
		pParams->pTapeSubunit = pTapeSubunit;
	else
	{
		delete pTapeSubunit;
		pTapeSubunit = nil;
		pParams->pTapeSubunit = nil;
	}
	
	// Signal that this thread is ready
	pParams->threadReady = true;
	
	// Start the run loop
	if ((pTapeSubunit) && (result == kIOReturnSuccess))
		CFRunLoopRun();
	
	return nil;
}

//////////////////////////////////////////////////////
// CreateVirtualTapeSubunit
//////////////////////////////////////////////////////
IOReturn CreateVirtualTapeSubunit(VirtualTapeSubunit **ppTapeSubunit,
								  UInt8 initialSignalMode,
								  TapeMediumInfo initialMediumInfo,
								  VirtualTapeCMPConnectionHandler cmpConnectionHandler,
								  VirtualTapeTransportStateChangeHandler transportStateChangeHandler, 
								  VirtualTapeSignalModeChangeHandler signalModeChangeHandler,
								  VirtualTapeTimeCodeRepositionHandler timeCodeRepositionHandler,
								  void *pCallbackRefCon,
								  AVCDevice *pAVCDevice)
{
	VirtualTapeSubunitThreadParams threadParams;
	pthread_t rtThread;
	pthread_attr_t threadAttr;
	
	threadParams.threadReady = false;
	threadParams.pTapeSubunit = nil;
	threadParams.initialSignalMode = initialSignalMode;
	threadParams.initialMediumInfo = initialMediumInfo;
	threadParams.pAVCDevice = pAVCDevice;
	
	threadParams.cmpConnectionHandler = cmpConnectionHandler;
	threadParams.transportStateChangeHandler = transportStateChangeHandler; 
	threadParams.signalModeChangeHandler = signalModeChangeHandler;
	threadParams.timeCodeRepositionHandler = timeCodeRepositionHandler;
	threadParams.pCallbackRefCon = pCallbackRefCon;
	
	// Create the thread which will instantiate and setup new VirtualTapeSubunit object
	pthread_attr_init(&threadAttr);
	pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_DETACHED);
	pthread_create(&rtThread, &threadAttr, (void *(*)(void *))VirtualTapeSubunitThreadStart, &threadParams);
	pthread_attr_destroy(&threadAttr);
	
	// Wait forever for the new thread to be ready
	while (threadParams.threadReady == false) usleep(1000);
	
	*ppTapeSubunit = threadParams.pTapeSubunit;
	
	if (threadParams.pTapeSubunit)
		return kIOReturnSuccess;
	else
		return kIOReturnError;
}

//////////////////////////////////////////////////////
// DestroyVirtualTapeSubunit
//////////////////////////////////////////////////////
IOReturn DestroyVirtualTapeSubunit(VirtualTapeSubunit *pTapeSubunit)
{
	IOReturn result = kIOReturnSuccess ;
	CFRunLoopRef runLoopRef;
	
	// Save the ref to the run loop the receiver is using
	runLoopRef = pTapeSubunit->runLoopRef;
	
	// Delete receiver object
	delete pTapeSubunit;
	
	// Stop the run-loop in the RT thread. The RT thread will then terminate
	CFRunLoopStop(runLoopRef);
	
	return result;
}

#ifdef kAVS_Enable_VirtualTape_AVCResponse_Delays
//////////////////////////////////////////////////////
// AVCCommandSlowDownResponse
//////////////////////////////////////////////////////
#define kAVCCmdSlowDownDelayTimeInMicroSeconds 25000 
static IOReturn AVCCommandSlowDownResponse( void *refCon,
											UInt32 generation,
											UInt16 srcNodeID,
											IOFWSpeed speed,
											const UInt8 * command,
											UInt32 cmdLen)
{
	// Delay 
	usleep(kAVCCmdSlowDownDelayTimeInMicroSeconds); 
	
	return kIOReturnError;
}
#endif

//////////////////////////////////////////////////////
// AVCCommandUnitVendorUniqueHandlerHelper
//////////////////////////////////////////////////////
static IOReturn AVCCommandUnitVendorUniqueHandlerHelper( void *refCon,
														 UInt32 generation,
														 UInt16 srcNodeID,
														 IOFWSpeed speed,
														 const UInt8 * command,
														 UInt32 cmdLen)
{
	VirtualTapeSubunit *pVirtualTapeSubunit = (VirtualTapeSubunit*) refCon;
	
	return pVirtualTapeSubunit->AVCCommandUnitVendorUniqueHandler(generation,
																  srcNodeID,
																  speed,
																  command,
																  cmdLen);
}

//////////////////////////////////////////////////////
// AVCCommandHandlerCallbackHelper
//////////////////////////////////////////////////////
static IOReturn AVCCommandHandlerCallbackHelper( void *refCon,
									UInt32 generation,
									UInt16 srcNodeID,
									IOFWSpeed speed,
									const UInt8 * command,
									UInt32 cmdLen)
{
	VirtualTapeSubunit *pVirtualTapeSubunit = (VirtualTapeSubunit*) refCon;
	
	return pVirtualTapeSubunit->AVCCommandHandlerCallback(generation,
														srcNodeID,
														speed,
														command,
														cmdLen);
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
	VirtualTapeSubunit *pVirtualTapeSubunit = (VirtualTapeSubunit*) refCon;
	
	return pVirtualTapeSubunit->AVCSubUnitPlugHandlerCallback(subunitTypeAndID,
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
	VirtualTapeSubunit *pVirtualTapeSubunit = (VirtualTapeSubunit*) data;
	pVirtualTapeSubunit->inputPlugReconnectTimeout();
}

//////////////////////////////////////////////////////
// OutputPlugReconnectTimeoutHelper
//////////////////////////////////////////////////////
static void OutputPlugReconnectTimeoutHelper(CFRunLoopTimerRef timer, void *data)
{
	VirtualTapeSubunit *pVirtualTapeSubunit = (VirtualTapeSubunit*) data;
	pVirtualTapeSubunit->outputPlugReconnectTimeout();
}

//////////////////////////////////////////////////////
// VirtualTapeSubunit Constructor
//////////////////////////////////////////////////////
VirtualTapeSubunit::VirtualTapeSubunit(UInt8 initialSignalMode,
									   TapeMediumInfo initialMediumInfo,
									   VirtualTapeCMPConnectionHandler cmpConnectionHandler,
									   VirtualTapeTransportStateChangeHandler transportStateChangeHandler, 
									   VirtualTapeSignalModeChangeHandler signalModeChangeHandler,
									   VirtualTapeTimeCodeRepositionHandler timeCodeRepositionHandler,
									   void *pCallbackRefCon)
{
	runLoopRef = nil;

	signalMode = initialSignalMode;
	mediumInfo = initialMediumInfo;
	clientCMPConnectionHandler = cmpConnectionHandler;
	clientTransportStateChangeHandler = transportStateChangeHandler; 
	clientSignalModeChangeHandler = signalModeChangeHandler;
	clientTimeCodeRepositionHandler = timeCodeRepositionHandler;
	pClientCallbackRefCon = pCallbackRefCon;

    outputPlugReconnectTimer = NULL;
    inputPlugReconnectTimer = NULL;
	
	nodeAVCProtocolInterface = nil;
	
	// Initialize the mutexes
    pthread_mutex_init(&transportControlMutex, NULL);
    pthread_mutex_init(&signalModeControlMutex, NULL);
}

//////////////////////////////////////////////////////
// VirtualTapeSubunit Destructor
//////////////////////////////////////////////////////
VirtualTapeSubunit::~VirtualTapeSubunit()
{
	// Remove callback dispatcher from run loop
	if (nodeAVCProtocolInterface != nil)
	{
		(*nodeAVCProtocolInterface)->removeCallbackDispatcherFromRunLoop(nodeAVCProtocolInterface);
		(*nodeAVCProtocolInterface)->Release(nodeAVCProtocolInterface) ;
	}
	
	if (nodeNubInterface != nil)
		(*nodeNubInterface)->Release(nodeNubInterface);
	
	pthread_mutex_destroy(&signalModeControlMutex);
	pthread_mutex_destroy(&transportControlMutex);
}

//////////////////////////////////////////////////////
// VirtualTapeSubunit::setupVirtualTapeSubunitWithAVCDevice
//////////////////////////////////////////////////////
IOReturn VirtualTapeSubunit::setupVirtualTapeSubunitWithAVCDevice(AVCDevice *pAVCDevice)
{
	IOReturn result;
	pAVCDeviceForBusIdentification = pAVCDevice;
	
	result = GetAVCProtocolInterfaceWithAVCDevice(pAVCDeviceForBusIdentification, 
												  &nodeAVCProtocolInterface,
												  &nodeNubInterface);
	if (result != kIOReturnSuccess)
		return result ;
	else
		return completeVirtualTapeSubunitSetup();
}

//////////////////////////////////////////////////////
// VirtualTapeSubunit::setupVirtualTapeSubunit
//////////////////////////////////////////////////////
IOReturn VirtualTapeSubunit::setupVirtualTapeSubunit(void)
{
	IOReturn result;

	pAVCDeviceForBusIdentification = nil;
	
	result = GetDefaultAVCProtocolInterface(&nodeAVCProtocolInterface, &nodeNubInterface);

	if (result != kIOReturnSuccess)
		return result ;
	else
		return completeVirtualTapeSubunitSetup();
}


//////////////////////////////////////////////////////
// VirtualTapeSubunit::completeVirtualTapeSubunitSetup
//////////////////////////////////////////////////////
IOReturn VirtualTapeSubunit::completeVirtualTapeSubunitSetup(void)
{
	IOReturn result = kIOReturnSuccess ;
	UInt32 sourcePlugNum = 0;
	UInt32 destPlugNum = 0;
	UInt32 retry = 5;
	UInt32 payloadInQuadlets;

	// Initialize private data vars
	timeCodeFrameCount = 0;
	transportMode = kAVCTapeWindOpcode;
	transportState = kAVCTapeWindStop;
	transportIsStable = true;
	
	// Save a reference to the current run loop
	runLoopRef = CFRunLoopGetCurrent();
	
	// Add a tape subunit
	result = (*nodeAVCProtocolInterface)->addSubunit(nodeAVCProtocolInterface,
													 kAVCTapeRecorder,
													 1,
													 1,
													 this,
													 AVCSubUnitPlugHandlerCallbackHelper,
													 &subUnitTypeAndID);
    if (result != kIOReturnSuccess)
        return result ;
	
	// Set the plugs signal format, based on the signalMode
	setPlugSignalFormatWithMode();
	
	// Connect the subunit source to any available unit output plug
	sourcePlugNum = 0;
	destPlugNum = kAVCAnyAvailableIsochPlug;
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
	
	// Connect the subunit dest to any available unit input plug
	sourcePlugNum = kAVCAnyAvailableIsochPlug;
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
	
	// Install command handler for the newly installed tape subunit
	result = (*nodeAVCProtocolInterface)->installAVCCommandHandler(nodeAVCProtocolInterface,
																   subUnitTypeAndID,
																   kAVCAllOpcodes,
																   this,
																   AVCCommandHandlerCallbackHelper);
	if (result != kIOReturnSuccess)
		return -1;

	// Install command handler for the D-VHS "Vendor Unique" unit-level command handler
	result = (*nodeAVCProtocolInterface)->installAVCCommandHandler(nodeAVCProtocolInterface,
																   0xFF,
																   0x00,
																   this,
																   AVCCommandUnitVendorUniqueHandlerHelper);
	if (result != kIOReturnSuccess)
		return -1;
	
#ifdef kAVS_Enable_VirtualTape_AVCResponse_Delays
	// Sharp HDTV Workaround: Install a additional command handler for all AVC commands. This command
	// handler will be called before the regular AVC command handlers for this class (and in the kernel).
	// This special command handler will delay, then return an error, which will allow IOFireWireAVC to
	// pass the command on to the real command handler.
	// Install command handler for the newly installed tape subunit
	result = (*nodeAVCProtocolInterface)->installAVCCommandHandler(nodeAVCProtocolInterface,
																   kAVCAllSubunitsAndUnit,
																   kAVCAllOpcodes,
																   this,
																   AVCCommandSlowDownResponse);
	if (result != kIOReturnSuccess)
		return -1;
#endif	
	
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
	payloadInQuadlets = getSignalModePayloadSize();
	
	// Set the oPCR/iPCR correctly
	setNewOutputPlugValue((0x003F0000+payloadInQuadlets));
	setNewInputPlugValue(0x003F0000);

	// Publish the config-ROM (if needed) and do bus reset
	return (*nodeAVCProtocolInterface)->publishAVCUnitDirectory(nodeAVCProtocolInterface);
}

//////////////////////////////////////////////////////
// VirtualTapeSubunit::setNewInputPlugValue
//////////////////////////////////////////////////////
IOReturn VirtualTapeSubunit::setNewInputPlugValue(UInt32 newVal)
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
// VirtualTapeSubunit::setNewOutputPlugValue
//////////////////////////////////////////////////////
IOReturn VirtualTapeSubunit::setNewOutputPlugValue(UInt32 newVal)
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

//////////////////////////////////////////////////////
// VirtualTapeSubunit::setPlugSignalFormatWithMode
//////////////////////////////////////////////////////
void VirtualTapeSubunit::setPlugSignalFormatWithMode(void)
{
	switch (signalMode)
	{
		case kAVCTapeSigModeHDV1_60:
		case kAVCTapeSigModeMPEG12Mbps_60:
		case kAVCTapeSigModeMPEG6Mbps_60:
		case kAVCTapeSigModeDVHS:
		case kAVCTapeSigModeMicroMV12Mbps_60:
		case kAVCTapeSigModeMicroMV6Mbps_60:
		case kAVCTapeSigModeHDV2_60:
			timeCodeFrameRate = kTapeTimeCodeFrameRate_29_97fps;
			signalFormat = kAVCPlugSignalFormatMPEGTS;
			break;

		case kAVCTapeSigModeHDV1_50:
		case kAVCTapeSigModeMPEG12Mbps_50:
		case kAVCTapeSigModeMPEG6Mbps_50:
		case kAVCTapeSigModeMicroMV12Mbps_50:
		case kAVCTapeSigModeMicroMV6Mbps_50:
		case kAVCTapeSigModeHDV2_50:
			timeCodeFrameRate = kTapeTimeCodeFrameRate_25fps;
			signalFormat = kAVCPlugSignalFormatMPEGTS;
			break;
			
		case kAVCTapeSigModeSD525_60:
		case kAVCTapeSigModeSDL525_60:
		case kAVCTapeSigModeHD1125_60:
		case kAVCTapeSigModeDVCPro25_525_60:
		case kAVCTapeSigModeDVCPro50_525_60:
		case kAVCTapeSigModeDVCPro100_60:
			timeCodeFrameRate = kTapeTimeCodeFrameRate_29_97fps;
			signalFormat = kAVCPlugSignalFormatNTSCDV + (signalMode << 16);
			break;

		case kAVCTapeSigModeSD625_50:
		case kAVCTapeSigModeSDL625_50:
		case kAVCTapeSigModeHD1250_50:
		case kAVCTapeSigModeDVCPro25_625_50:
		case kAVCTapeSigModeDVCPro50_625_50:
		case kAVCTapeSigModeDVCPro100_50:
			timeCodeFrameRate = kTapeTimeCodeFrameRate_25fps;
			signalFormat = kAVCPlugSignalFormatPalDV + (signalMode<< 16);
			break;
			
		default:
			timeCodeFrameRate = kTapeTimeCodeFrameRate_29_97fps;
			signalFormat = 0xFFFFFFFF;
			break;
	};	
	
	// Setup signal formats for plugs
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
// VirtualTapeSubunit::getSignalModePayloadSize
//////////////////////////////////////////////////////
UInt32 VirtualTapeSubunit::getSignalModePayloadSize(void)
{
	UInt32 payloadSize;
	
	switch (signalMode)
	{
		case kAVCTapeSigModeHDV1_60:
		case kAVCTapeSigModeHDV1_50:
		case kAVCTapeSigModeMPEG6Mbps_60:
		case kAVCTapeSigModeMPEG6Mbps_50:
		case kAVCTapeSigModeMicroMV6Mbps_60:
		case kAVCTapeSigModeMicroMV6Mbps_50:
		case kAVCTapeSigModeMPEG12Mbps_60:
		case kAVCTapeSigModeMPEG12Mbps_50:
		case kAVCTapeSigModeMicroMV12Mbps_60:
		case kAVCTapeSigModeMicroMV12Mbps_50:
			payloadSize = 98; // max 2 TS packets per isoch, 192+192+8 = 392 bytes = 98 quadlets
			break;

		case kAVCTapeSigModeDVHS:
		case kAVCTapeSigModeHDV2_60:
		case kAVCTapeSigModeHDV2_50:
			payloadSize = 144; // max 3 TS packets per isoch, 192+192+192+8 = 576 bytes = 144 quadlets
			break;
			
		case kAVCTapeSigModeSD525_60:
		case kAVCTapeSigModeSDL525_60:
		case kAVCTapeSigModeDVCPro25_525_60:
		case kAVCTapeSigModeSD625_50:
		case kAVCTapeSigModeSDL625_50:
		case kAVCTapeSigModeDVCPro25_625_50:
			payloadSize = 122; //488 bytes = 122 quadlets
			break;

		case kAVCTapeSigModeHD1125_60:
		case kAVCTapeSigModeHD1250_50:
		case kAVCTapeSigModeDVCPro50_525_60:
		case kAVCTapeSigModeDVCPro50_625_50:
			payloadSize = 244; //968 byte payload = 244 quadlets
			break;

		case kAVCTapeSigModeDVCPro100_60:
		case kAVCTapeSigModeDVCPro100_50:
			payloadSize = 488; //1920 byte payload = 488 quadlets
			break;
			
		default:
			payloadSize = 0; // The mode is not one we support, so payload size unkown.
			break;
	};	
	
	return payloadSize;
}

//////////////////////////////////////////////////////
// VirtualTapeSubunit::getSubunitTypeAndID
//////////////////////////////////////////////////////
UInt8 VirtualTapeSubunit::getSubunitTypeAndID(void)
{
	return subUnitTypeAndID;
}

//////////////////////////////////////////////////////
// VirtualTapeSubunit::getMediumInfo
//////////////////////////////////////////////////////
TapeMediumInfo VirtualTapeSubunit::getMediumInfo(void)
{
	return mediumInfo;
}

//////////////////////////////////////////////////////
// VirtualTapeSubunit::getTransportState
//////////////////////////////////////////////////////
void VirtualTapeSubunit::getTransportState(UInt8 *pCurrentTransportMode, UInt8 *pCurrentTransportState, bool *pIsStable)
{
	*pCurrentTransportMode = transportMode;
	*pCurrentTransportState = transportState;
	*pIsStable = transportIsStable;
}

//////////////////////////////////////////////////////
// VirtualTapeSubunit::getSignalMode
//////////////////////////////////////////////////////
UInt8 VirtualTapeSubunit::getSignalMode(void)
{
	return signalMode;
}

//////////////////////////////////////////////////////
// VirtualTapeSubunit::getTimeCodeFrameCount
//////////////////////////////////////////////////////
UInt32 VirtualTapeSubunit::getTimeCodeFrameCount(void)
{
	return timeCodeFrameCount;
}

//////////////////////////////////////////////////////
// VirtualTapeSubunit::getTimeCodeFrameCountInHMSF
//////////////////////////////////////////////////////
void VirtualTapeSubunit::getTimeCodeFrameCountInHMSF(UInt32 *pHours, UInt32 *pMinutes, UInt32 *pSeconds, UInt32 *pFrames)
{	
	UInt32 framesPerHour;
	UInt32 framesPerMinute;
	UInt32 framesPerSecond;
	
	switch (timeCodeFrameRate)
	{
		// Note: This code currently treats 29.97 drop frame
		// time-code as 30fps non-drop time-code
		
		case kTapeTimeCodeFrameRate_25fps:
			framesPerHour = 90000;
			framesPerMinute = 1500;
			framesPerSecond = 25;
			break;
			
		case kTapeTimeCodeFrameRate_60fps:
			framesPerHour = 216000;
			framesPerMinute = 3600;
			framesPerSecond = 60;
			break;
			
		case kTapeTimeCodeFrameRate_29_97fps:
		case kTapeTimeCodeFrameRate_30fps:
		default:
			framesPerHour = 108000;
			framesPerMinute = 1800;
			framesPerSecond = 30;
			break;
	};
				
	// Take a snapshot of the current timeCodeFrameCount
	*pFrames = timeCodeFrameCount;
				
	// Calculate hh:mm:ss.ff
	*pHours = *pFrames / framesPerHour;
	*pFrames -= (*pHours*framesPerHour);
	*pMinutes = *pFrames / framesPerMinute;
	*pFrames -= (*pMinutes*framesPerMinute);
	*pSeconds = *pFrames / framesPerSecond;
	*pFrames -= (*pSeconds*framesPerSecond);
	
	return;
}

//////////////////////////////////////////////////////
// VirtualTapeSubunit::getTimeCodeFrameRate
//////////////////////////////////////////////////////
TapeTimeCodeFrameRate VirtualTapeSubunit::getTimeCodeFrameRate(void)	
{
	return timeCodeFrameRate;
}

//////////////////////////////////////////////////////
// VirtualTapeSubunit::getPlugParameters
//////////////////////////////////////////////////////
void VirtualTapeSubunit::getPlugParameters(bool isInputPlug, UInt8 *pIsochChannel, UInt8 *pIsochSpeed, UInt8 *pP2PCount)
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
// VirtualTapeSubunit::setMediumInfo
//////////////////////////////////////////////////////
IOReturn VirtualTapeSubunit::setMediumInfo(TapeMediumInfo newMediumInfo)
{
	mediumInfo = newMediumInfo;
	return kIOReturnSuccess;
}

//////////////////////////////////////////////////////
// VirtualTapeSubunit::setTransportState
//////////////////////////////////////////////////////
IOReturn VirtualTapeSubunit::setTransportState(UInt8 newTransportMode, UInt8 newTransportState, bool isStable)
{
	pthread_mutex_lock(&transportControlMutex);
	
	// If transport is currently in play or record mode, we need to clear on-line bit in corresponding
	// PCR, if the newTransportMode specifies a different mode!
	
	if ((transportMode == kAVCTapeTportModeRecord) && (newTransportMode != kAVCTapeTportModeRecord))
	{
		// If we have no p2p connection on our iPCR, we need to clear both the broadcast bit and the on-line bit
		// If we do have a p2p connection on our iPCR, we need to clear just the broadcast bit
		if (((isochInPlugVal & 0x3F000000) >> 24) == 0)
			setNewInputPlugValue((isochInPlugVal & 0x3FFFFFFF)); // Clear broadcast bit and online bit
		else
			setNewInputPlugValue((isochInPlugVal & 0xBFFFFFFF)); // Clear broadcast bit
	}
	else if ((transportMode == kAVCTapeTportModePlay) && (newTransportMode != kAVCTapeTportModePlay))
		setNewOutputPlugValue((isochOutPlugVal & 0x3FFFFFFF)); // Clear online and broadcast bits

	// Set the new transport state
	transportMode = newTransportMode;
	transportState = newTransportState;
	transportIsStable = isStable;

	// If new transport state is play or record, set the online and broadcast bits in the corresponding PCR
	if (transportMode == kAVCTapeTportModeRecord)
	{
		// If we have no p2p connection on our iPCR, we need to set both the broadcast bit and the on-line bit
		// If we do have a p2p connection on our iPCR, we need to set just the broadcast bit
		// because the on-line bit is already set when the p2p connection was made
		if (((isochInPlugVal & 0x3F000000) >> 24) == 0)
			setNewInputPlugValue((isochInPlugVal | 0xC0000000)); // Set broadcast bit and on-line bit
		else
			setNewInputPlugValue((isochInPlugVal | 0x40000000)); // Set broadcast bit
	}
	else if (transportMode == kAVCTapeTportModePlay)
		setNewOutputPlugValue((isochOutPlugVal | 0xC0000000)); // Set online and broadcast bits
	
	pthread_mutex_unlock(&transportControlMutex);
	
	return kIOReturnSuccess;
}

//////////////////////////////////////////////////////
// VirtualTapeSubunit::setSignalMode
//////////////////////////////////////////////////////
IOReturn VirtualTapeSubunit::setSignalMode(UInt8 newSignalMode)
{
	UInt32 payloadInQuadlets;

	pthread_mutex_lock(&signalModeControlMutex);
	
	signalMode = newSignalMode;
	setPlugSignalFormatWithMode();

	// Need to update payload field in oPCR
	payloadInQuadlets = getSignalModePayloadSize();
	setNewOutputPlugValue((isochOutPlugVal & 0xFFFFFC00)+payloadInQuadlets); // Set the new payload size

	pthread_mutex_unlock(&signalModeControlMutex);

	return kIOReturnSuccess;
}

//////////////////////////////////////////////////////
// VirtualTapeSubunit::setTimeCodeFrameCount
//////////////////////////////////////////////////////
IOReturn VirtualTapeSubunit::setTimeCodeFrameCount(UInt32 newTimeCodeFrameCount)
{
	timeCodeFrameCount = newTimeCodeFrameCount;
	return kIOReturnSuccess;
}

//////////////////////////////////////////////////////
// VirtualTapeSubunit::setTimeCodeFrameRate
//////////////////////////////////////////////////////
IOReturn VirtualTapeSubunit::setTimeCodeFrameRate(TapeTimeCodeFrameRate newTimeCodeFrameRate)
{
	timeCodeFrameRate = newTimeCodeFrameRate;
	return kIOReturnSuccess;
}

//////////////////////////////////////////////////////
// VirtualTapeSubunit::setPlugParameters
//////////////////////////////////////////////////////
IOReturn VirtualTapeSubunit::setPlugParameters(bool isInputPlug, UInt8 isochChannel, UInt8 isochSpeed)
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

//////////////////////////////////////////////////////
// VirtualTapeSubunit::AVCTransportStateChangeRequested
//////////////////////////////////////////////////////
IOReturn VirtualTapeSubunit::AVCTransportStateChangeRequested(UInt8 newTransportMode, UInt8 newTransportState)
{
	IOReturn result = kIOReturnSuccess;
	
	// If client provided a VirtualTapeTransportStateChangeHandler then call it now
	if (clientTransportStateChangeHandler != nil)
		result = clientTransportStateChangeHandler(pClientCallbackRefCon,newTransportMode,newTransportState);
	
	if (result == kIOReturnSuccess)
	{
		// Client said to go ahead a do the transport state change, as "stable"
		setTransportState(newTransportMode,newTransportState,true);
	}
	else if (result == kIOReturnNotReady)
	{
		// Client said to go ahead a do the transport state change, as "in-transisition"
		setTransportState(newTransportMode,newTransportState,false);
		result = kIOReturnSuccess;
	}
	else if (result == kIOReturnAcceptWithNoAutoStateChange)
	{
		// Client probably already changed the state manually
		result = kIOReturnSuccess;	
	}
	
	return result;
}

//////////////////////////////////////////////////////
// VirtualTapeSubunit::AVCCommandHandlerCallback
//////////////////////////////////////////////////////
IOReturn VirtualTapeSubunit::AVCCommandHandlerCallback(UInt32 generation,
								   UInt16 srcNodeID,
								   IOFWSpeed speed,
								   const UInt8 * command,
								   UInt32 cmdLen)
{
	/* Local Vars */
	IOReturn result;
	UInt8 cType = command[kAVCCommandResponse] & 0x0F;
	UInt8 *pRspFrame;
	UInt32 timeInFrames;
	UInt32 hours;
	UInt32 minutes;
	UInt32 seconds;
	UInt32 framesPerHour;
	UInt32 framesPerMinute;
	UInt32 framesPerSecond;
	
	pRspFrame = (UInt8*) malloc(cmdLen);
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
		///////////////////////////////////////////////////////
		//
		// Transport Control Commands (Play, Record, Wind)
		//
		///////////////////////////////////////////////////////
		case kAVCTapePlayOpcode:
		case kAVCTapeWindOpcode:
		case kAVCTapeRecordOpcode:
			if ((transportMode== command[kAVCOpcode])  && (transportState == command[kAVCOperand0]))
				break;	// Transport state command results in no state change
			else if (AVCTransportStateChangeRequested(command[kAVCOpcode],command[kAVCOperand0]) != kIOReturnSuccess)
				pRspFrame[kAVCCommandResponse] = kAVCRejectedStatus;
			break;
			
		///////////////////////////////////////////////////////
		//
		// AVC Tape Relative Time Counter Command
		//
		///////////////////////////////////////////////////////
		case kAVCTapeRelativeTimeCounterOpcode:
			if (cType == kAVCStatusInquiryCommand)
			{
				getTimeCodeFrameCountInHMSF(&hours,&minutes,&seconds,&timeInFrames);

				pRspFrame[kAVCOperand1] = intToBCD(timeInFrames);
				pRspFrame[kAVCOperand2] = intToBCD(seconds);
				pRspFrame[kAVCOperand3] = intToBCD(minutes);
				pRspFrame[kAVCOperand4] = intToBCD(hours);
				
				pRspFrame[kAVCCommandResponse] = kAVCImplementedStatus;
			}
			else // must be a control command, so reject it
				pRspFrame[kAVCCommandResponse] = kAVCRejectedStatus;
			break;
			
		////////////////////////////
		//
		// AVC Tape (Absolute) Time Counter Command
		//
		////////////////////////////
		case kAVCTapeTimeCodeOpcode:
			if (cType == kAVCControlCommand)
			{
				if (command[kAVCOperand0] != 0x20)
				{
					pRspFrame[kAVCCommandResponse] = kAVCRejectedStatus;
				}
				else
				{
					switch (timeCodeFrameRate)
					{
						// Note: This code currently treats 29.97 drop frame
						// time-code as 30fps non-drop time-code
						
						case kTapeTimeCodeFrameRate_25fps:
							framesPerHour = 90000;
							framesPerMinute = 1500;
							framesPerSecond = 25;
							break;
							
						case kTapeTimeCodeFrameRate_60fps:
							framesPerHour = 216000;
							framesPerMinute = 3600;
							framesPerSecond = 60;
							break;
							
						case kTapeTimeCodeFrameRate_29_97fps:
						case kTapeTimeCodeFrameRate_30fps:
						default:
							framesPerHour = 108000;
							framesPerMinute = 1800;
							framesPerSecond = 30;
							break;
					}
					
					timeInFrames = bcd2bin(command[kAVCOperand1]);
					timeInFrames += (bcd2bin(command[kAVCOperand2])*framesPerSecond);
					timeInFrames += (bcd2bin(command[kAVCOperand3])*framesPerMinute);
					timeInFrames += (bcd2bin(command[kAVCOperand4])*framesPerHour);
					
					// Alert the client to the time-code reposition control command
					if (clientTimeCodeRepositionHandler)
					{
						result = clientTimeCodeRepositionHandler(pClientCallbackRefCon,timeInFrames);
						if (result != kIOReturnSuccess)
							pRspFrame[kAVCCommandResponse] = kAVCRejectedStatus;
					}
				}
			}
			else if (cType == kAVCStatusInquiryCommand)
			{
				if (command[kAVCOperand0] != 0x71)
				{
					pRspFrame[kAVCCommandResponse] = kAVCRejectedStatus;
				}
				else
				{
					getTimeCodeFrameCountInHMSF(&hours,&minutes,&seconds,&timeInFrames);
					
					pRspFrame[kAVCOperand1] = intToBCD(timeInFrames);
					pRspFrame[kAVCOperand2] = intToBCD(seconds);
					pRspFrame[kAVCOperand3] = intToBCD(minutes);
					pRspFrame[kAVCOperand4] = intToBCD(hours);
				}
			}
			break;
			
		/////////////////////////////////////////////////
		//
		// AVC Tape Input/Output Signal Mode Commands 
		//
		/////////////////////////////////////////////////
		case kAVCTapeOutputSignalModeOpcode:
		case kAVCTapeInputSignalModeOpcode:
			if (cType == kAVCStatusInquiryCommand)
			{
				// Return the current signal mode
				pRspFrame[kAVCOperand0] = signalMode;
			}
			else if (cType == kAVCControlCommand)
			{
				// If this control command results in no signal-mode change (same as before),
				// (new signal mode equals current signal mode) accept the command,
				// without notifying client.
				if (command[kAVCOperand0] == signalMode)
				{
					break;
				}
				else if (clientSignalModeChangeHandler)
				{
					result = clientSignalModeChangeHandler(pClientCallbackRefCon,command[kAVCOperand0]);
					if (result != kIOReturnSuccess)
						pRspFrame[kAVCCommandResponse] = kAVCRejectedStatus;
					else
						setSignalMode(command[kAVCOperand0]);
				}
				else
				{	
					// No client signal mode change handler, so no changes allowed
					pRspFrame[kAVCCommandResponse] = kAVCRejectedStatus;
				}
			}
			break;
			
		/////////////////////////////////////////////////
		//
		// AVC Tape Medium Info Command
		//
		/////////////////////////////////////////////////
		case kAVCTapeMediumInfoOpcode:
			if (cType == kAVCStatusInquiryCommand)
			{
				pRspFrame[kAVCOperand0] = (UInt8)((mediumInfo & 0xFF00) >> 8);
				pRspFrame[kAVCOperand1] = (UInt8)(mediumInfo & 0x00FF);
			}
			else // must be a control command, so reject it
				pRspFrame[kAVCCommandResponse] = kAVCRejectedStatus;
			break;
			
		/////////////////////////////////////////////////
		//
		// AVC Tape Transport State Command
		//
		/////////////////////////////////////////////////
		case kAVCTapeTransportStateOpcode:
			if (cType == kAVCStatusInquiryCommand)
			{
				pRspFrame[kAVCOpcode] = transportMode;
				pRspFrame[kAVCOperand0] = transportState;
			}
			else // must be a control command, so reject it
				pRspFrame[kAVCCommandResponse] = kAVCRejectedStatus;
			break;
			
		/////////////////////////////////////////////////
		//
		// AVC Tape Search Mode Command
		//
		/////////////////////////////////////////////////
		case kAVCTapeSearchModeOpcode:
			if (cType == kAVCStatusInquiryCommand)
			{
				pRspFrame[kAVCOpcode] = 0x50;
				pRspFrame[kAVCOperand0] = 0x60;
			}
			else // must be a control command, so reject it
				pRspFrame[kAVCCommandResponse] = kAVCRejectedStatus;
			break;
			
			
		/////////////////////////////////////////////////
		//
		// AVC Tape Record Speed Command
		//
		/////////////////////////////////////////////////
		case kAVCTapeRecordingSpeedOpcode:
			if (cType == kAVCStatusInquiryCommand)
			{
				pRspFrame[kAVCOperand0] = kAVCTapeRecSpeedSP;
			}
			else // must be a control command, so reject it
				pRspFrame[kAVCCommandResponse] = kAVCRejectedStatus;
			break;
			
		/////////////////////////////////////////////////
		//
		// AVC Tape Recording/Playback Format Command
		//
		/////////////////////////////////////////////////
		case kAVCTapeTapePlaybackFormatOpcode:
		case kAVCTapeTapeRecordingFormatOpcode:
			if (cType == kAVCStatusInquiryCommand)
			{
				// These bytes signify that we are in HS recording/playback mode
				pRspFrame[kAVCOperand0] = 0x08;
				pRspFrame[kAVCOperand1] = 0x10;
				pRspFrame[kAVCOperand2] = 0x00;
				pRspFrame[kAVCOperand3] = 0x00;
				pRspFrame[kAVCOperand4] = 0x01;
				pRspFrame[kAVCOperand5] = 0x00;
				pRspFrame[kAVCOperand6] = 0x00;
				pRspFrame[kAVCOperand7] = 0x00;
				pRspFrame[kAVCOperand8] = 0x00;
				break;
			}
			else if (cType == kAVCControlCommand)
			{
				// We accept these control commands, but don't do anything with them
				break;
			}
			else
				pRspFrame[kAVCCommandResponse] = kAVCRejectedStatus;
			break;
			
		/////////////////////////////////////////////////
		//
		// AVC Tape Absolute Track Number Command
		//
		/////////////////////////////////////////////////
		case kAVCTapeAbsoluteTrackNumberOpcode:
			if (cType == kAVCStatusInquiryCommand)
			{
				// This response signifies no useful ATN information
				pRspFrame[kAVCOperand0] = 0x71;
				pRspFrame[kAVCOperand1] = 0xFF;
				pRspFrame[kAVCOperand2] = 0xFF;
				pRspFrame[kAVCOperand3] = 0xFF;
				pRspFrame[kAVCOperand4] = 0xFF;
			}
			else // must be a control command, so reject it
				pRspFrame[kAVCCommandResponse] = kAVCRejectedStatus;
			break;

		/////////////////////////////////////////////////
		//
		// AVC Plug Info Command
		//
		/////////////////////////////////////////////////
		case kAVCPlugInfoOpcode:
			if ((cType == kAVCStatusInquiryCommand) && (command[kAVCOperand0] == 0x00))
			{
				// Report that this subunit has one input plug and one output plug
				pRspFrame[kAVCOperand1] = 0x01;
				pRspFrame[kAVCOperand2] = 0x01;
			}
			else // must be a control command, so reject it
				pRspFrame[kAVCCommandResponse] = kAVCRejectedStatus;
			break;
			
			
		/////////////////////////////////////////////////
		//
		// All Other Tape Subunit Commands
		//
		/////////////////////////////////////////////////
		default:
			pRspFrame[kAVCCommandResponse] = kAVCNotImplementedStatus;
			break;
	}
	
	/* Send the response */
	(*nodeAVCProtocolInterface)->sendAVCResponse(nodeAVCProtocolInterface,
												 generation,
												 srcNodeID,
												 (const char*) pRspFrame,
												 cmdLen);
	// Free allocated response frame
	free(pRspFrame);
	
	return kIOReturnSuccess;
}

///////////////////////////////////////////////////////////
// VirtualTapeSubunit::AVCCommandUnitVendorUniqueHandler
///////////////////////////////////////////////////////////
IOReturn VirtualTapeSubunit::AVCCommandUnitVendorUniqueHandler(UInt32 generation,
															   UInt16 srcNodeID,
															   IOFWSpeed speed,
															   const UInt8 * command,
															   UInt32 cmdLen)
{
	/* Local Vars */
	IOReturn result;
	UInt8 cType = command[kAVCCommandResponse] & 0x0F;
	UInt8 *pRspFrame;
	
	pRspFrame = (UInt8*) malloc(cmdLen);
	if (!pRspFrame)
		return kIOReturnNoMemory;
	
	/* copy cmd frame to rsp frame */
	bcopy(command,pRspFrame,cmdLen);
	
	// Pre-initialize as if not handling the command!
	pRspFrame[kAVCCommandResponse] = kAVCNotImplementedStatus;
	result = kIOReturnError;
	
	// See if this is a command we should handle!
	if ((cType == kAVCStatusInquiryCommand) && 
		(command[kAVCOperand0] == 0x00) &&
		(command[kAVCOperand1] == 0x80) &&
		(command[kAVCOperand2] == 0x88) &&
		(command[kAVCOperand3] == 0x00) &&
		(command[kAVCOperand4] == 0x20) &&
		(command[kAVCOperand5] == 0x00) &&
		(command[kAVCOperand7] == 0xFF) &&
		(command[kAVCOperand8] == 0xFF) &&
		(cmdLen == 11))
	{
		if (command[kAVCOperand6] == 0xA0)
		{
			// VERSION Command
			pRspFrame[kAVCOperand7] = 0x01;
			pRspFrame[kAVCOperand8] = 0x00;
			pRspFrame[kAVCCommandResponse] = kAVCImplementedStatus;
			result = kIOReturnSuccess;
		}
		else if (command[kAVCOperand6] == 0xA1)
		{
			// TIMER Command
			pRspFrame[kAVCOperand7] = 0x80;
			pRspFrame[kAVCOperand8] = 0x80;
			pRspFrame[kAVCCommandResponse] = kAVCImplementedStatus;
			result = kIOReturnSuccess;
		}
	}
	
	/* Send the response if no error*/
	if (result == kIOReturnSuccess)
		(*nodeAVCProtocolInterface)->sendAVCResponse(nodeAVCProtocolInterface,
													 generation,
													 srcNodeID,
													 (const char*) pRspFrame,
													 cmdLen);
	
	// Free allocated response frame
	free(pRspFrame);
	
	return result;
}

//////////////////////////////////////////////////
// VirtualTapeSubunit::startInputPlugReconnectTimer
//////////////////////////////////////////////////
void VirtualTapeSubunit::startInputPlugReconnectTimer( void )
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
	
    time = CFAbsoluteTimeGetCurrent() + kTapeSubunitCMPBusResetReconnectTime;
	
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
// VirtualTapeSubunit::startOutputPlugReconnectTimer
//////////////////////////////////////////////////
void VirtualTapeSubunit::startOutputPlugReconnectTimer( void )
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
	
    time = CFAbsoluteTimeGetCurrent() + kTapeSubunitCMPBusResetReconnectTime;
	
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
// VirtualTapeSubunit::stopInputPlugReconnectTimer
//////////////////////////////////////////////////
void VirtualTapeSubunit::stopInputPlugReconnectTimer( void )
{
	if ( inputPlugReconnectTimer )
	{
		CFRunLoopTimerInvalidate( inputPlugReconnectTimer );
		CFRelease( inputPlugReconnectTimer );
		inputPlugReconnectTimer = NULL;
	}
}

//////////////////////////////////////////////////
// VirtualTapeSubunit::stopOutputPlugReconnectTimer
//////////////////////////////////////////////////
void VirtualTapeSubunit::stopOutputPlugReconnectTimer( void )
{
	if ( outputPlugReconnectTimer )
	{
		CFRunLoopTimerInvalidate( outputPlugReconnectTimer );
		CFRelease( outputPlugReconnectTimer );
		outputPlugReconnectTimer = NULL;
	}
}

//////////////////////////////////////////////////
// VirtualTapeSubunit::inputPlugReconnectTimeout
//////////////////////////////////////////////////
void VirtualTapeSubunit::inputPlugReconnectTimeout(void)
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
// VirtualTapeSubunit::outputPlugReconnectTimeout
//////////////////////////////////////////////////
void VirtualTapeSubunit::outputPlugReconnectTimeout(void)
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
// VirtualTapeSubunit::AVCSubUnitPlugHandlerCallback
//////////////////////////////////////////////////////
IOReturn VirtualTapeSubunit::AVCSubUnitPlugHandlerCallback(UInt32 subunitTypeAndID,
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
	UInt32 currentSignalFormat;
	
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
		
		//////////////////////////////////////////////////////////////////	
		// We received an AV/C input/outputsignal format control 
		// command for an isoch plug connected to our subunit plug.
		//
		// Note: Currently we do not support a signal format change
		// via the input/output signal format control commands, since most
		// tape subunit based devices only support a format change
		// via the tape subunit's input/output mode commands.
		//
		// Maybe we'll change this in the future, but its somewhat
		// non-explicit for MPEG modes, because the unit-level signal
		// format commands don't distinguish between the different
		// types of MPEG modes (HDV, DVHS, MicroMV, etc.).
		//
		// Note: If the requested signal format is the current signal
		// format for the specified plug, then accept the command anyway,
		// since we're already in that format.
		//
		//////////////////////////////////////////////////////////////////	
		case kIOFWAVCSubunitPlugMsgSignalFormatModified:
			// Get the current signal format for this plug
			(*nodeAVCProtocolInterface)->getSubunitPlugSignalFormat(nodeAVCProtocolInterface,
																	subUnitTypeAndID,
																	plugType,
																	plugID,
																	&currentSignalFormat);
			// See if this is a request to change into the mode we're already in
			if (messageParams == currentSignalFormat)
				result = kIOReturnSuccess;
			else
				result = kIOReturnUnsupported;
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
// VirtualTapeSubunit::GetNub
//////////////////////////////////////////////////////
IOFireWireLibNubRef VirtualTapeSubunit::GetNub(void)
{
	return nodeNubInterface;
}

//////////////////////////////////////////////
// bcd2bin - BCD to Bin conversion
//////////////////////////////////////////////
static unsigned int bcd2bin(unsigned int input)
{
	unsigned int shift,output;
	
	shift = 1;
	output = 0;
	while(input)
	{
		output += (input%16) * shift;
		input /= 16;
		shift *= 10;
	};
	
	return output;
}

//////////////////////////////////////////////
// intToBCD
//////////////////////////////////////////////
static UInt32 intToBCD(unsigned int value)
{
	int result = 0;
	int i = 0;
	
	while (value > 0) {
		result |= ((value % 10) << (4 * i++));
		
		value /= 10;
	}
	
	return (result);
}

} // namespace AVS	
