/*
	File:		VirtualSTB.h
 
 Synopsis: This is header file for the VirtualSTB sample command-line app.
 
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

#ifndef __AVCVIDEOSERVICES_VIRTUALSTB__
#define __AVCVIDEOSERVICES_VIRTUALSTB__

//namespace AVS
//{

// The spec says all isoch p2p connections must be reconnected
// within one second of each bus-reset. We'll wait 1.5 to be consertive
#define kTunerSubunitCMPBusResetReconnectTime 1.5 	// 1.5 seconds

class VirtualSTB;

//////////////////////////////////////////////////////////////////////////////
// Helper functions for object creation/distruction with dedicated thread
//////////////////////////////////////////////////////////////////////////////

// Create and setup a VirtualSTB object and it's dedicated thread
IOReturn CreateVirtualSTB(VirtualSTB **ppSTB);

// Destroy a VirtualSTB object created with CreateVirtualSTB(), and it's dedicated thread
IOReturn DestroyVirtualSTB(VirtualSTB *pSTB);

/////////////////////////////////////////
//
// VirtualSTB Class definition
//
/////////////////////////////////////////

class VirtualSTB
{
	
public:
	// Constructor
	VirtualSTB();
	
	// Destructor
	~VirtualSTB();
	
	// Setup funcion that finds the first available local node
    IOReturn setupVirtualSTB(void);
		
	// Function for client to get current PCR info
	void getPlugParameters(UInt8 *pIsochChannel, UInt8 *pIsochSpeed, UInt8 *pP2PCount);
	
	//////////////////////////////////////////////////////////////////
	//
	// These four functions shouldn't be called by clients!
	//
	//////////////////////////////////////////////////////////////////
	void outputPlugReconnectTimeout(void);

	IOReturn AVCTunerCommandHandlerCallback(UInt32 generation,
									   UInt16 srcNodeID,
									   IOFWSpeed speed,
									   const UInt8 * command,
									   UInt32 cmdLen);

	IOReturn AVCPanelCommandHandlerCallback(UInt32 generation,
											UInt16 srcNodeID,
											IOFWSpeed speed,
											const UInt8 * command,
											UInt32 cmdLen);
	
	IOReturn AVCSubUnitPlugHandlerCallback(UInt32 subunitTypeAndID,
										   IOFWAVCPlugTypes plugType,
										   UInt32 plugID,
										   IOFWAVCSubunitPlugMessages plugMessage,
										   UInt32 messageParams);
	
	// A reference to the current run loop for callbacks
	CFRunLoopRef runLoopRef;	
	
private:
	IOReturn completeVirtualSTBSetup(void);
	
	IOReturn setNewOutputPlugValue(UInt32 newVal);
		
	void startOutputPlugReconnectTimer( void );
	
	void stopOutputPlugReconnectTimer( void );
	
	void CMPOutputConnectionHandler(UInt8 isochChannel, UInt8 isochSpeed, UInt8 p2pCount);
	
	static void MessageReceivedProc(UInt32 msg, UInt32 param1, UInt32 param2, void *pRefCon);
	static IOReturn MpegTransmitCallback(UInt32 **ppBuf, bool *pDiscontinuityFlag, void *pRefCon);
	
	IOFireWireAVCLibProtocolInterface **nodeAVCProtocolInterface;
	IOFireWireLibNubRef nodeNubInterface;
	
	CFRunLoopTimerRef outputPlugReconnectTimer;
	
	UInt32 isochOutPlugNum;
	UInt32 isochOutPlugVal;
		
	UInt32 tunerSubUnitTypeAndID;
	UInt32 panelSubUnitTypeAndID;

	MPEGNaviFileReader *pReader;
	char inFileName[80];
	char tsPacketBuf[kMPEG2TSPacketSize];
	
	UInt32 currentTunerChannelNumber;
	
	MPEG2Transmitter *pTransmitter;
};
	
//} // namespace AVS

#endif // __AVCVIDEOSERVICES_VIRTUALSTB__


