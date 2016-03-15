/*
	File:   PanelSubunitController.cpp
 
 Synopsis: This is the sourcecode for the PanelSubunitController Class
 
	Copyright: 	¬© Copyright 2001-2003 Apple Computer, Inc. All rights reserved.
 
	Written by: ayanowitz
 
 Disclaimer:	IMPORTANT:  This Apple software is supplied to you by Apple Computer, Inc.
 ("Apple") in consideration of your agreement to the following terms, and your
 use, installation, modification or redistribution of this Apple software
 constitutes acceptance of these terms.  If you do not agree with these terms,
 please do not use, install, modify or redistribute this Apple software.
 
 In consideration of your agreement to abide by the following terms, and subject
 to these terms, Apple grants you a personal, non-exclusive license, under Apple‚Äôs
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
	
///////////////////////////////
// Constructor
///////////////////////////////
PanelSubunitController::PanelSubunitController(AVCDevice *pDevice, UInt8 subUnitID)
{
	pAVCDevice = pDevice;
	pAVCDeviceCommandInterface = nil;
	subunitTypeAndID = 0x48 + (subUnitID & 0x07);
}

///////////////////////////////
// Alternate Constructor
///////////////////////////////
PanelSubunitController::PanelSubunitController(AVCDeviceCommandInterface *pDeviceCommandInterface, UInt8 subUnitID)
{
	pAVCDevice = nil;
	pAVCDeviceCommandInterface = pDeviceCommandInterface;
	subunitTypeAndID = 0x48 + (subUnitID & 0x07);
}

///////////////////////////////
// Destructor
///////////////////////////////
PanelSubunitController::~PanelSubunitController()
{
	
}

//////////////////////////////////
// PanelSubunitController::Tune
//////////////////////////////////
IOReturn PanelSubunitController::Tune(UInt16 channelNum)
{
	UInt8 opData[4];
	
	opData[0] = ((channelNum & 0x0F00) >> 8);
	opData[1] = (channelNum & 0x00FF);
	opData[2] = 0x00;
	opData[3] = 0x00;
	
	return PassThrough(kAVCPanelKeyTuneFunction, kPassThroughStatePressed, 4, opData);
}

///////////////////////////////////////////////
// PanelSubunitController::TuneTwoPartChannel
///////////////////////////////////////////////
IOReturn PanelSubunitController::TuneTwoPartChannel(UInt16 majorChannelNum,UInt16 minorChannelNum)
{
	UInt8 opData[4];
	
	opData[0] = 0x80 + ((majorChannelNum & 0x0F00) >> 8);
	opData[1] = (majorChannelNum & 0x00FF);
	opData[2] = ((minorChannelNum & 0x0F00) >> 8);
	opData[3] = (minorChannelNum & 0x00FF);
	
	return PassThrough(kAVCPanelKeyTuneFunction, kPassThroughStatePressed, 4, opData);
}

///////////////////////////////////////////////
// PanelSubunitController::PassThrough
///////////////////////////////////////////////
IOReturn PanelSubunitController::PassThrough(UInt8 operationID, bool stateFlag, UInt8 operationDataLen, UInt8 *pOperationData)
{
    UInt32 size = 5+operationDataLen;
    UInt8 cmd[size],response[size];
    IOReturn res = kIOReturnSuccess;
	UInt8 i;
	
	cmd[0] = kAVCControlCommand;
	cmd[1] = subunitTypeAndID;
	cmd[2] = 0x7C;	// PASS_THROUGH opcode
	
	cmd[3] = operationID;	// operation_id
	if (stateFlag == kPassThroughStateReleased)
		cmd[3] |= 0x80;
		
	cmd[4] = operationDataLen;	// Lenght of operation data
	
	for (i=0;i<operationDataLen;i++)
		cmd[5+i] = pOperationData[i];

	res = DoAVCCommand(cmd, size, response, &size);
	if (!((res == kIOReturnSuccess) && (response[0] == kAVCAcceptedStatus)))
	{
		res = kIOReturnError;
	}
	
	return res;
}

///////////////////////////////
// PanelSubunitController::DoAVCCommand
///////////////////////////////
IOReturn PanelSubunitController::DoAVCCommand(const UInt8 *command, UInt32 cmdLen, UInt8 *response, UInt32 *responseLen)
{
	if (pAVCDevice)
		return pAVCDevice->AVCCommand(command,cmdLen,response,responseLen);
	if (pAVCDeviceCommandInterface)
		return pAVCDeviceCommandInterface->AVCCommand(command,cmdLen,response,responseLen);
	else
		return kIOReturnNoDevice;
}


} // namespace AVS	