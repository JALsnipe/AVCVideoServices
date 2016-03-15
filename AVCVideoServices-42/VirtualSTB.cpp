/*
	File:		VirtualSTB.cpp
 
 Synopsis: This is a simple console mode application that shows an example of using
 the code in AVS for emulating a CableTV STB.
 
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

#include "VirtualSTB.h"

// Thread parameter structures
struct VirtualSTBThreadParams
{
	volatile bool threadReady;
	VirtualSTB *pSTB;
};

//////////////////////////////////////////////////////////////////////
// VirtualSTBThreadStart
//////////////////////////////////////////////////////////////////////
static void *VirtualSTBThreadStart(VirtualSTBThreadParams* pParams)
{
	IOReturn result = kIOReturnSuccess ;
	VirtualSTB *pSTB;
	
	// Instantiate a new receiver object
	pSTB = new VirtualSTB();
	
	// Setup the receiver object
	if (pSTB)
	{
			result = pSTB->setupVirtualSTB();
	}
	
	// Update the return parameter with a pointer to the new receiver object
	if (result == kIOReturnSuccess)
		pParams->pSTB = pSTB;
	else
	{
		delete pSTB;
		pSTB = nil;
		pParams->pSTB = nil;
	}
	
	// Signal that this thread is ready
	pParams->threadReady = true;
	
	// Start the run loop
	if ((pSTB) && (result == kIOReturnSuccess))
		CFRunLoopRun();
	
	return nil;
}

//////////////////////////////////////////////////////
// CreateVirtualSTB
//////////////////////////////////////////////////////
IOReturn CreateVirtualSTB(VirtualSTB **ppSTB)
{
	VirtualSTBThreadParams threadParams;
	pthread_t rtThread;
	pthread_attr_t threadAttr;
	
	threadParams.threadReady = false;
	threadParams.pSTB = nil;
	
	// Create the thread which will instantiate and setup new VirtualSTB object
	pthread_attr_init(&threadAttr);
	pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_DETACHED);
	pthread_create(&rtThread, &threadAttr, (void *(*)(void *))VirtualSTBThreadStart, &threadParams);
	pthread_attr_destroy(&threadAttr);
	
	// Wait forever for the new thread to be ready
	while (threadParams.threadReady == false) usleep(1000);
	
	*ppSTB = threadParams.pSTB;
	
	if (threadParams.pSTB)
		return kIOReturnSuccess;
	else
		return kIOReturnError;
}

//////////////////////////////////////////////////////
// DestroyVirtualSTB
//////////////////////////////////////////////////////
IOReturn DestroyVirtualSTB(VirtualSTB *pSTB)
{
	IOReturn result = kIOReturnSuccess ;
	CFRunLoopRef runLoopRef;
	
	// Save the ref to the run loop the receiver is using
	runLoopRef = pSTB->runLoopRef;
	
	// Delete receiver object
	delete pSTB;
	
	// Stop the run-loop in the RT thread. The RT thread will then terminate
	CFRunLoopStop(runLoopRef);
	
	return result;
}

//////////////////////////////////////////////////////
// AVCTunerCommandHandlerCallbackHelper
//////////////////////////////////////////////////////
static IOReturn AVCTunerCommandHandlerCallbackHelper( void *refCon,
												 UInt32 generation,
												 UInt16 srcNodeID,
												 IOFWSpeed speed,
												 const UInt8 * command,
												 UInt32 cmdLen)
{
	VirtualSTB *pVirtualSTB = (VirtualSTB*) refCon;
	
	return pVirtualSTB->AVCTunerCommandHandlerCallback(generation,
														  srcNodeID,
														  speed,
														  command,
														  cmdLen);
}

//////////////////////////////////////////////////////
// AVCPanelCommandHandlerCallbackHelper
//////////////////////////////////////////////////////
static IOReturn AVCPanelCommandHandlerCallbackHelper( void *refCon,
													  UInt32 generation,
													  UInt16 srcNodeID,
													  IOFWSpeed speed,
													  const UInt8 * command,
													  UInt32 cmdLen)
{
	VirtualSTB *pVirtualSTB = (VirtualSTB*) refCon;
	
	return pVirtualSTB->AVCPanelCommandHandlerCallback(generation,
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
	VirtualSTB *pVirtualSTB = (VirtualSTB*) refCon;
	
	return pVirtualSTB->AVCSubUnitPlugHandlerCallback(subunitTypeAndID,
															  plugType,
															  plugID,
															  plugMessage,
															  messageParams);
}

//////////////////////////////////////////////////////
// OutputPlugReconnectTimeoutHelper
//////////////////////////////////////////////////////
static void OutputPlugReconnectTimeoutHelper(CFRunLoopTimerRef timer, void *data)
{
	VirtualSTB *pVirtualSTB = (VirtualSTB*) data;
	pVirtualSTB->outputPlugReconnectTimeout();
}

//////////////////////////////////////////////////////
// VirtualSTB Constructor
//////////////////////////////////////////////////////
VirtualSTB::VirtualSTB()
{
	runLoopRef = nil;
    outputPlugReconnectTimer = NULL;
	pTransmitter = nil;
	nodeAVCProtocolInterface = nil;
	pReader = nil;
	currentTunerChannelNumber = 1;
}

//////////////////////////////////////////////////////
// VirtualSTB Destructor
//////////////////////////////////////////////////////
VirtualSTB::~VirtualSTB()
{
	// Remove callback dispatcher from run loop
	if (nodeAVCProtocolInterface != nil)
	{
		(*nodeAVCProtocolInterface)->removeCallbackDispatcherFromRunLoop(nodeAVCProtocolInterface);
		(*nodeAVCProtocolInterface)->Release(nodeAVCProtocolInterface) ;
	}
	
	if (nodeNubInterface != nil)
		(*nodeNubInterface)->Release(nodeNubInterface);
}

//////////////////////////////////////////////////////
// VirtualSTB::setupVirtualSTB
//////////////////////////////////////////////////////
IOReturn VirtualSTB::setupVirtualSTB(void)
{
	IOReturn result;
		
	result = GetDefaultAVCProtocolInterface(&nodeAVCProtocolInterface, &nodeNubInterface);
	
	if (result != kIOReturnSuccess)
		return result ;
	else
		return completeVirtualSTBSetup();
}


//////////////////////////////////////////////////////
// VirtualSTB::completeVirtualSTBSetup
//////////////////////////////////////////////////////
IOReturn VirtualSTB::completeVirtualSTBSetup(void)
{
	IOReturn result = kIOReturnSuccess ;
	UInt32 sourcePlugNum = 0;
	UInt32 destPlugNum = 0;
	UInt32 retry = 5;
		
	// Save a reference to the current run loop
	runLoopRef = CFRunLoopGetCurrent();
	
	// Add a tuner subunit
	result = (*nodeAVCProtocolInterface)->addSubunit(nodeAVCProtocolInterface,
													 kAVCTuner,
													 1,
													 0,
													 this,
													 AVCSubUnitPlugHandlerCallbackHelper,
													 &tunerSubUnitTypeAndID);
    if (result != kIOReturnSuccess)
        return result ;

	// Add a tuner subunit
	result = (*nodeAVCProtocolInterface)->addSubunit(nodeAVCProtocolInterface,
													 9, // Panel Subunit
													 0,
													 0,
													 this,
													 nil,
													 &panelSubUnitTypeAndID);
    if (result != kIOReturnSuccess)
        return result ;
	
	
	// Connect the subunit source to any available unit output plug
	sourcePlugNum = 0;
	destPlugNum = kAVCAnyAvailableIsochPlug;
	result = (*nodeAVCProtocolInterface)->connectTargetPlugs(nodeAVCProtocolInterface,
															 tunerSubUnitTypeAndID,
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
	
	printf("VirtualSTB: Tuner subunit source plug 0 connected to oPCR%u\n", (unsigned int) isochOutPlugNum);
		
	// Setup signal formats for plugs
	result = (*nodeAVCProtocolInterface)->setSubunitPlugSignalFormat(nodeAVCProtocolInterface,
															tunerSubUnitTypeAndID,
															IOFWAVCPlugSubunitSourceType,
															0,
															kAVCPlugSignalFormatMPEGTS);
	if (result != kIOReturnSuccess)
		return -1;
	
	// Install command handler for the newly installed tuner subunit
	result = (*nodeAVCProtocolInterface)->installAVCCommandHandler(nodeAVCProtocolInterface,
																   tunerSubUnitTypeAndID,
																   kAVCAllOpcodes,
																   this,
																   AVCTunerCommandHandlerCallbackHelper);
	if (result != kIOReturnSuccess)
		return -1;

	// Install command handler for the newly installed panel subunit
	result = (*nodeAVCProtocolInterface)->installAVCCommandHandler(nodeAVCProtocolInterface,
																   panelSubUnitTypeAndID,
																   kAVCAllOpcodes,
																   this,
																   AVCPanelCommandHandlerCallbackHelper);
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
																	0x80FFFF00); // max speed = 400, num iPCR = 0
	}
	while ((result != kIOReturnSuccess) && (retry++ < 5));
	
	// Set the oPCR/iPCR correctly
	setNewOutputPlugValue((0x003F0000+146));	// 146 quadlets equals 3 TS packets + CIP
	
	printf("VirtualSTB: Tuner Currently Set to Channel %u\n",(unsigned int) currentTunerChannelNumber);

	// Create and initialize the file reader
	pReader = new MPEGNaviFileReader;
	if (pReader)
	{
		sprintf(inFileName,"%u.m2t",(unsigned int) currentTunerChannelNumber);
		result = pReader->InitWithTSFile(inFileName, false);
		if (result != kIOReturnSuccess)
		{
			printf("VirtualSTB: Unable to initialize the MPEG file reader with file: %s\n",inFileName);
			delete pReader;
			pReader = nil;
		}
		else
		{
			printf("VirtualSTB: Successfully initialized the MPEG file reader with file: %s\n",inFileName);
		}
	}
	
	// Publish the config-ROM (if needed) and do bus reset
	return (*nodeAVCProtocolInterface)->publishAVCUnitDirectory(nodeAVCProtocolInterface);
}

//////////////////////////////////////////////////////
// VirtualSTB::setNewOutputPlugValue
//////////////////////////////////////////////////////
IOReturn VirtualSTB::setNewOutputPlugValue(UInt32 newVal)
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
// VirtualSTB::getPlugParameters
//////////////////////////////////////////////////////
void VirtualSTB::getPlugParameters(UInt8 *pIsochChannel, UInt8 *pIsochSpeed, UInt8 *pP2PCount)
{
	// oPCR settings
	*pIsochChannel = ((isochOutPlugVal & 0x003F0000) >> 16);
	*pIsochSpeed = ((isochOutPlugVal & 0x0000C000) >> 14);
	*pP2PCount = ((isochOutPlugVal & 0x3F000000) >> 24);
}

//////////////////////////////////////////////////////
// VirtualSTB::AVCTunerCommandHandlerCallback
//////////////////////////////////////////////////////
IOReturn VirtualSTB::AVCTunerCommandHandlerCallback(UInt32 generation,
													   UInt16 srcNodeID,
													   IOFWSpeed speed,
													   const UInt8 * command,
													   UInt32 cmdLen)
{
	/* Local Vars */
	UInt8 cType = command[kAVCCommandResponse] & 0x0F;
	UInt8 *pRspFrame;
	
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
			
			/////////////////////////////////////////////////
			//
			// AVC Plug Info Command
			//
			/////////////////////////////////////////////////
		case kAVCPlugInfoOpcode:
			if ((cType == kAVCStatusInquiryCommand) && (command[kAVCOperand0] == 0x00))
			{
				// Report that this subunit has zero input plugs and one output plug
				pRspFrame[kAVCOperand1] = 0x00;
				pRspFrame[kAVCOperand2] = 0x01;
			}
			else // must be a control command, so reject it
				pRspFrame[kAVCCommandResponse] = kAVCRejectedStatus;
			break;
			
			
			/////////////////////////////////////////////////
			//
			// All Other Tuner Subunit Commands
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

//////////////////////////////////////////////////////
// VirtualSTB::AVCPanelCommandHandlerCallback
//////////////////////////////////////////////////////
IOReturn VirtualSTB::AVCPanelCommandHandlerCallback(UInt32 generation,
													UInt16 srcNodeID,
													IOFWSpeed speed,
													const UInt8 * command,
													UInt32 cmdLen)
{
	/* Local Vars */
	IOReturn result = kIOReturnSuccess ;
	UInt8 cType = command[kAVCCommandResponse] & 0x0F;
	UInt8 *pRspFrame;
	UInt32 newChannel;
	
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
		
		/////////////////////////////////////////////////
		//
		// AVC Plug Info Command
		//
		/////////////////////////////////////////////////
		case kAVCPlugInfoOpcode:
			if ((cType == kAVCStatusInquiryCommand) && (command[kAVCOperand0] == 0x00))
			{
				// Report that this subunit has zero input plugs and zero output plugs
				pRspFrame[kAVCOperand1] = 0x00;
				pRspFrame[kAVCOperand2] = 0x00;
			}
			else // must be a control command, so reject it
				pRspFrame[kAVCCommandResponse] = kAVCRejectedStatus;
			break;
			
			
		/////////////////////////////////////////////////
		//
		// AVC Plug Info Command
		//
		/////////////////////////////////////////////////
		case 0x7C: /* pass through command */
			// TODO;
			if (cType == kAVCControlCommand)
			{
				// Currently, we only react to the "deterministic tune" operation_id
				if ((command[kAVCOperand0] & 0x7F) == 0x67)
				{
					// Make sure the command has the correct number of operation_data bytes
					if (command[kAVCOperand1] == 4)
					{
						// Extract the "major" channel number (we currently don't support the "minor" channel number)
						newChannel = ((UInt32)(command[kAVCOperand2] & 0x0F) << 8) + command[kAVCOperand3];
						printf("VirtualSTB: Tuner channel changed. Old channel: %u  New channel: %u\n", (unsigned int)currentTunerChannelNumber, (unsigned int) newChannel);
						if (newChannel != currentTunerChannelNumber)
						{
							// Switch channels
							currentTunerChannelNumber = newChannel;
							
							// If the transmitter exists, we need to stop it before we swtich files
							if (pTransmitter)
							{
								pTransmitter->stopTransmit();
								printf("VirtualSTB: MPEG2Transmitter stopped\n");
							}

							// Switch files
							if (!pReader)
								pReader = new MPEGNaviFileReader;

							if (pReader)
							{
								sprintf(inFileName,"%u.m2t",(unsigned int) currentTunerChannelNumber);
								result = pReader->InitWithTSFile(inFileName, false);
								if (result != kIOReturnSuccess)
								{
									printf("VirtualSTB: Unable to initialize the MPEG file reader with file: %s\n",inFileName);
									delete pReader;
									pReader = nil;
								}
								else
								{
									printf("VirtualSTB: Successfully initialized the MPEG file reader with file: %s\n",inFileName);
								}
							}
							
							// If the transmitter exists, we need to restart it now
							if (pTransmitter)
							{	
								pTransmitter->startTransmit();
								printf("VirtualSTB: MPEG2Transmitter started\n");
							}
						}
					}
					else
						pRspFrame[kAVCCommandResponse] = kAVCRejectedStatus; // Reject the mal-formed command
				}
				else
					pRspFrame[kAVCCommandResponse] = kAVCAcceptedStatus; // Accept the command, but do nothing
			}
			else
				pRspFrame[kAVCCommandResponse] = kAVCNotImplementedStatus;
			break;
				
			
		/////////////////////////////////////////////////
		//
		// All Other Panel Subunit Commands
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

//////////////////////////////////////////////////
// VirtualSTB::startOutputPlugReconnectTimer
//////////////////////////////////////////////////
void VirtualSTB::startOutputPlugReconnectTimer( void )
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
	
    time = CFAbsoluteTimeGetCurrent() + kTunerSubunitCMPBusResetReconnectTime;
	
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
// VirtualSTB::stopOutputPlugReconnectTimer
//////////////////////////////////////////////////
void VirtualSTB::stopOutputPlugReconnectTimer( void )
{
	if ( outputPlugReconnectTimer )
	{
		CFRunLoopTimerInvalidate( outputPlugReconnectTimer );
		CFRelease( outputPlugReconnectTimer );
		outputPlugReconnectTimer = NULL;
	}
}

//////////////////////////////////////////////////
// VirtualSTB::outputPlugReconnectTimeout
//////////////////////////////////////////////////
void VirtualSTB::outputPlugReconnectTimeout(void)
{
    UInt32 currentP2PCount = ((isochOutPlugVal & 0x3F000000) >> 24);
	UInt32 currentChannel = ((isochOutPlugVal & 0x003F0000) >> 16);;
	UInt32 currentSpeed = ((isochOutPlugVal & 0x0000C000) >> 14);
	
	// Stop/delete the timer
	stopOutputPlugReconnectTimer();
	
	if (currentP2PCount == 0)
		CMPOutputConnectionHandler(currentChannel, currentSpeed, currentP2PCount);
}

//////////////////////////////////////////////////////
// VirtualSTB::AVCSubUnitPlugHandlerCallback
//////////////////////////////////////////////////////
IOReturn VirtualSTB::AVCSubUnitPlugHandlerCallback(UInt32 subunitTypeAndID,
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
					else if (isReconnectAfterBusReset == false)
					{
						// Notify the client of the new connection
						CMPOutputConnectionHandler(newChannel, newSpeed, newP2PCount);
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
			// via the input/output signal format control commands.
			//
			// Note: If the requested signal format is the current signal
			// format for the specified plug, then accept the command anyway,
			// since we're already in that format.
			//
			//////////////////////////////////////////////////////////////////	
		case kIOFWAVCSubunitPlugMsgSignalFormatModified:
			// Get the current signal format for this plug
			(*nodeAVCProtocolInterface)->getSubunitPlugSignalFormat(nodeAVCProtocolInterface,
																	tunerSubUnitTypeAndID,
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
// VirtualSTB::CMPOutputConnectionHandler
//////////////////////////////////////////////////////
void VirtualSTB::CMPOutputConnectionHandler(UInt8 isochChannel, UInt8 isochSpeed, UInt8 p2pCount)
{
	IOReturn result;
	
	if ((p2pCount > 0) && (pTransmitter == nil))
	{
		printf("VirtualSTB: p2p connection to Tuner established\n");
		
		// Use the AVCVideoServices framework's helper function to create the
		// MPEG2Transmitter object and dedicated real-time thread.
		// Note: Here we are relying on a number of default parameters.
		result = CreateMPEG2Transmitter(&pTransmitter,
										VirtualSTB::MpegTransmitCallback,
										this,
										VirtualSTB::MessageReceivedProc,
										this,
										nil,
										nodeNubInterface,
										kCyclesPerTransmitSegment,
										kNumTransmitSegments,
										false,
										3,	// Supports up to 36mbps streams.
										kTSPacketQueueSizeInPackets);
		if (!pTransmitter)
		{
			//printf("Error creating MPEG2Transmitter object: %d\n",result);
			return;
		}
		else
		{
			// Set the channel to transmit on
			pTransmitter->setTransmitIsochChannel(isochChannel);
			
			// Set the isoch packet speed
			pTransmitter->setTransmitIsochSpeed((IOFWSpeed)isochSpeed);
			
			// Start the transmitter
			pTransmitter->startTransmit();
			
			printf("VirtualSTB: MPEG2Transmitter started\n");
		}
	}
	else if ((p2pCount == 0) && (pTransmitter != nil))
	{
		printf("VirtualSTB: p2p connection to Tuner broken\n");

		// Stop the transmitter
		pTransmitter->stopTransmit();
		printf("VirtualSTB: MPEG2Transmitter stopped\n");
		
		// Delete the transmitter object
		DestroyMPEG2Transmitter(pTransmitter);
		pTransmitter = nil;
	}
	
	return;
}

///////////////////////////////////////////////////////////////////////////
// VirtualSTB::MessageReceivedProc
///////////////////////////////////////////////////////////////////////////
void VirtualSTB::MessageReceivedProc(UInt32 msg, UInt32 param1, UInt32 param2, void *pRefCon)
{
	// This class currently doesn't do anything with this callback!
	return;
}

///////////////////////////////////////////////////////////////////////////
// VirtualSTB::MpegTransmitCallback
///////////////////////////////////////////////////////////////////////////
IOReturn VirtualSTB::MpegTransmitCallback(UInt32 **ppBuf, bool *pDiscontinuityFlag, void *pRefCon)
{
	unsigned int cnt;
	IOReturn result = 0;
	
	VirtualSTB *pSTB = (VirtualSTB*) pRefCon;
	
	// Signal no discontinuity
	*pDiscontinuityFlag = false;
	
	// Just a sanity check.
	if (!pSTB->pReader)
		return -1;
	
	// Read the next TS packet from the input file
	cnt = pSTB->pReader->ReadNextTSPackets(pSTB->tsPacketBuf,1);
	if (cnt != 1)
	{
		pSTB->pReader->SeekToBeginning();	
		cnt = pSTB->pReader->ReadNextTSPackets(pSTB->tsPacketBuf,1);
		if (cnt != 1)
		{
			// We tried going back to the beginning of the file, and failed to read again
			result = -1;	// Causes a CIP only cycle to be filled
		}
		else
		{
			*ppBuf = (UInt32*) pSTB->tsPacketBuf;
		}
	}
	else
	{
		*ppBuf = (UInt32*) pSTB->tsPacketBuf;
	}
	
	return result;
}

//////////////////////////////////////////////////////
// Main
//////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
	IOReturn result = kIOReturnSuccess;

	VirtualSTB *pSTB;
	
	result = CreateVirtualSTB(&pSTB);
	
	if (result != kIOReturnSuccess)
	{
		printf("Error Creating VirtualSTB: 0x%08X",result);
		exit(1);
	}
	
	for (;;)
		usleep(1000000);
	
	return result;
}
