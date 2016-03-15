/*
	File:   FWAVC.h
 
 Synopsis: This is the header for the C front-end to AVCVideoServices 
 
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

#ifndef __AVCVIDEOSERVICES_FWAVC__
#define __AVCVIDEOSERVICES_FWAVC__

// Include IOKit / FireWire Family/ FireWire AVC lib headers
#include <IOKit/IOKitLib.h>
#include <IOKit/IOMessage.h>
#include <IOKit/firewire/IOFireWireLib.h>
#include <IOKit/firewire/IOFireWireLibIsoch.h>
#include <IOKit/firewire/IOFireWireFamilyCommon.h>
#include <IOKit/avc/IOFireWireAVCLib.h>
#include <IOKit/avc/IOFireWireAVCConsts.h>

#ifdef __cplusplus
extern "C" {
#endif	

// Include the AVS/FWAVC shared header
#include "AVSShared.h"

#pragma mark -
#pragma mark ======================================
#pragma mark Opaque Struct References
#pragma mark ======================================

/*!
	@typedef FWAVCDeviceControllerRef
	 
	 @abstract This is the opaque reference to an AVC device controller object.
*/
typedef struct FWAVCDeviceController *FWAVCDeviceControllerRef;

/*!
	@typedef FWAVCDeviceRef
	 
	 @abstract This is the opaque reference to an AVC device object.
*/
typedef struct FWAVCDevice *FWAVCDeviceRef;

/*!
	@typedef FWAVCDeviceStreamRef
 
	@abstract This is the opaque reference to an AVC device stream object.
*/
typedef struct FWAVCDeviceStream *FWAVCDeviceStreamRef;

/*!
	@typedef FWAVCDVTransmitterFrameRef
 
	@abstract This is the opaque reference to a DV frame used by the DV transmitter.
*/
typedef struct FWAVCDVTransmitterFrame *FWAVCDVTransmitterFrameRef;

/*!
	@typedef FWAVCDVReceiverFrameRef
 
	@abstract This is the opaque reference to a DV frame used by the DV receiver.
*/
typedef struct FWAVCDVReceiveFrame *FWAVCDVReceiverFrameRef;

/*!
	@typedef FWAVCDVFramerRef
 
	@abstract This is the opaque reference to a DV framer object.
*/
typedef struct FWAVCDVFramer *FWAVCDVFramerRef;

/*!
	@typedef FWAVCDVFramerFrameRef
 
	@abstract This is the opaque reference to a DV frame used by the DV framer.
*/
typedef struct FWAVCDVFramerFrame *FWAVCDVFramerFrameRef;

/*!
	@typedef FWAVCTSDemuxerRef
 
	@abstract This is the opaque reference to a MPEG-2 TS demuxer object.
*/
typedef struct FWAVCTSDemuxer *FWAVCTSDemuxerRef;

/*!
	@typedef FWAVCTSDemuxerPESPacketRef
 
	@abstract This is the opaque reference to a PES packet used by the TS demuxer.
*/
typedef struct FWAVCTSDemuxerPESPacket *FWAVCTSDemuxerPESPacketRef;

#pragma mark -
#pragma mark ======================================
#pragma mark Typedefs
#pragma mark ======================================

/*!
	@typedef FWAVCDVFramerCallbackMessage
 
	@abstract This is the message delivered with the DV framer callback.
 */
typedef unsigned int FWAVCDVFramerCallbackMessage;

/*!
	@typedef FWAVCTSDemuxerCallbackMessage
 
	@abstract This is the message delivered with the TS demuxer PES callback.
 */
typedef unsigned int FWAVCTSDemuxerCallbackMessage;

/*!
	@typedef FWAVCTSDemuxerPESPacketStreamType
 
	@abstract This is the stream type for PES packets. 
 */
typedef unsigned int FWAVCTSDemuxerPESPacketStreamType;

/*!
	@typedef FWAVCDVReceiverCallbackMessage
 
	@abstract This is the message delivered with the DV receiver callback.
 */
typedef unsigned int FWAVCDVReceiverCallbackMessage;

#pragma mark -
#pragma mark ======================================
#pragma mark Callbacks
#pragma mark ======================================

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@typedef FWAVCDeviceControllerNotification

	@abstract This is the prototype for the AVC device controller "device arrived" callback.
  
	@discussion The AVC device controller uses this callback to notify the clients that an AVC device has been discovered
 (or rediscovered after reconnection) by the AVC device controller.

	@param fwavcDeviceControllerRef The reference to the AVC device controller that is making this call.

	@param pRefCon A pointer to client supplied private data returned with this callback.

	@param fwavcDeviceRef  The reference to the AVC device that has arrived. 
 
	@result The value return by the client is currently ignored. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef IOReturn (*FWAVCDeviceControllerNotification) (FWAVCDeviceControllerRef fwavcDeviceControllerRef, 
													   void *pRefCon, 
													   FWAVCDeviceRef fwavcDeviceRef);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@typedef FWAVCDeviceMessageNotification
 
	@abstract This is the prototype for delivering IOKit messages for an AVC device to the client.
 
	@discussion This is the prototype for the AVC device controller's globalAVCDeviceMessageProc, called to deliver
 IOKit messages for all AVC devices in the AVC device controllers array of known devices. It is also the prototype 
 for the deviceMessageProc, registered when a client opens a particular AVC device for exclusive access, used to to 
 deliver IOKit messages for that particular AVC device.
 
	@param fwavcDeviceRef The reference to the AVC device that the message is associated with.
 
	@param messageType The type of IOKit message.
 
	@param messageArgument The argument associated with this IOKit message.

	@param pRefCon A pointer to client supplied private data returned with this callback.
 
	@result The value return by the client is currently ignored. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef IOReturn (*FWAVCDeviceMessageNotification) (FWAVCDeviceRef fwavcDeviceRef, 
													natural_t messageType, 
													void * messageArgument, 
													void *pRefCon);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@typedef FWAVCUniversalReceiverDataReceivedProc
 
	@abstract This is the prototype for the default callback for data delivery by a universal-receiver AVC device stream.
 
	@discussion This will provide data for one isochronous cycle.

 	@param fwavcDeviceStreamRef The reference to the AVC device stream that is making this call.
 
	@param fwavcDeviceRef The reference to the AVC device associated with this stream.
 
	@param pRefCon A pointer to client supplied private data returned with this callback.

	@param payloadLength The length of the packet.
 
	@param pPayload A pointer to the packet buffer.
 
	@param isochHeader The 32-bit (host-endiness) isochronous header.
 
	@param fireWireTimeStamp The 32-bit FireWire time-stamp for this packet.
  
	@result The value return by the client is currently ignored. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef IOReturn (*FWAVCUniversalReceiverDataReceivedProc) (FWAVCDeviceStreamRef fwavcDeviceStreamRef,
														FWAVCDeviceRef fwavcDeviceRef,
														void *pRefCon, 
														UInt32 payloadLength, 
														UInt8 *pPayload,  
														UInt32 isochHeader, 
														UInt32 fireWireTimeStamp);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@typedef FWAVCUniversalReceiverStructuredDataReceivedProc
 
	@abstract This is the prototype for the alternate callback for data delivery by a universal-receiver AVC device stream.
 
	@discussion This can provide data for multiple isochronous cycles (i.e. - reduces the total number of callbacks).
 
	@param fwavcDeviceStreamRef The reference to the AVC device stream that is making this call.
 
	@param fwavcDeviceRef The reference to the AVC device associated with this stream.
  
	@param CycleDataCount The number of FWAVCUniversalReceiveCycleData structs in the pCycleData array.
 
	@param pCycleData An array of FWAVCUniversalReceiveCycleData, one for each isochronous cycle.
 
	@param pRefCon A pointer to client supplied private data returned with this callback.
 
	@result The value return by the client is currently ignored. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef IOReturn (*FWAVCUniversalReceiverStructuredDataReceivedProc) (FWAVCDeviceStreamRef fwavcDeviceStreamRef,
																  FWAVCDeviceRef fwavcDeviceRef,
																  UInt32 CycleDataCount, 
																  FWAVCUniversalReceiveCycleData *pCycleData, 
																  void *pRefCon);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@typedef FWAVCMPEG2ReceiverDataReceivedProc
 
	@abstract This is the prototype for the default callback for data delivery by a mpeg2-receiver AVC device stream.
 
	@discussion This will provide data for one isochronous cycle.
 
	@param fwavcDeviceStreamRef The reference to the AVC device stream that is making this call.
 
	@param fwavcDeviceRef The reference to the AVC device associated with this stream.
 
	@param tsPacketCount The number of TS packets received

	@param ppBuf An array of TS packet buffers
	
	@param pRefCon A pointer to client supplied private data returned with this callback.

	@param isochHeader The 32-bit (host-endiness) isochronous header.
	
	@param cipHeader0 The first CIP quadlet (Big-Endian)

	@param cipHeader1 The second CIP quadlet (Big-Endian)

	@param fireWireTimeStamp The 32-bit FireWire time-stamp for this cycle.
  
	@result The value return by the client is currently ignored. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef IOReturn (*FWAVCMPEG2ReceiverDataReceivedProc) (FWAVCDeviceStreamRef fwavcDeviceStreamRef,
													FWAVCDeviceRef fwavcDeviceRef,
													UInt32 tsPacketCount,
													UInt32 **ppBuf, 
													void *pRefCon, 
													UInt32 isochHeader,
													UInt32 cipHeader0,
													UInt32 cipHeader1,
													UInt32 fireWireTimeStamp);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@typedef FWAVCMPEG2ReceiverStructuredDataReceivedProc
 
	@abstract This is the prototype for the alternate callback for data delivery by a mpeg2-receiver AVC device stream.
 
	@discussion This can provide data for multiple isochronous cycles (i.e. - reduces the total number of callbacks).
 
	@param fwavcDeviceStreamRef The reference to the AVC device stream that is making this call.
 
	@param fwavcDeviceRef The reference to the AVC device associated with this stream.
 
	@param CycleDataCount The number of FWAVCMPEGReceiveCycleData structs in the pCycleData array.
 
	@param pCycleData An array of FWAVCMPEGReceiveCycleData, one for each isochronous cycle.
 
	@param pRefCon A pointer to client supplied private data returned with this callback.
 
	@result The value return by the client is currently ignored. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef IOReturn (*FWAVCMPEG2ReceiverStructuredDataReceivedProc) (FWAVCDeviceStreamRef fwavcDeviceStreamRef,
															  FWAVCDeviceRef fwavcDeviceRef,
															  UInt32 CycleDataCount, 
															  FWAVCMPEGReceiveCycleData *pCycleData, 
															  void *pRefCon);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@typedef FWAVCMPEG2TransmitterDataRequestProc
 
	@abstract This is the prototype for the callback to pull data from the client for a mpeg2-transmitter AVC device stream.
 
	@discussion This callback requests one transport stream packet from the client.
 
	@param fwavcDeviceStreamRef The reference to the AVC device stream that is making this call.
 
	@param fwavcDeviceRef The reference to the AVC device associated with this stream.
 
	@param ppBuf A pointer to the client's TS packet buffer.
 
	@param pDiscontinuityFlag The client sets this to true to indicate a stream discontinuity before this packet.
 
	@param pRefCon A pointer to client supplied private data returned with this callback.
 
	@result A return value of anything other than kIOReturnSuccess implies the client has no TS packet to transmit. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef IOReturn (*FWAVCMPEG2TransmitterDataRequestProc) (FWAVCDeviceStreamRef fwavcDeviceStreamRef,
														  FWAVCDeviceRef fwavcDeviceRef,
														  UInt32 **ppBuf, 
														  bool *pDiscontinuityFlag, 
														  void *pRefCon);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@typedef FWAVCMPEG2TransmitterTimeStampProc
 
	@abstract This is the prototype for the callback that reports time-stamap information for a mpeg2-transmitter AVC device stream.
 
	@discussion This callback provides a transmit-time in nano-seconds (based on the CPU-time) for a transport stream packet with a PCR.
 
	@param fwavcDeviceStreamRef The reference to the AVC device stream that is making this call.
 
	@param fwavcDeviceRef The reference to the AVC device associated with this stream.
 
	@param pcr The PCR of the transport stream packet.
 
	@param transmitTimeInNanoSeconds The future transmit-time in nano-seconds (based on the CPU-time) for the packet containing the PCR value.
 
	@param pRefCon A pointer to client supplied private data returned with this callback.
 
	@result A return value of anything other than kIOReturnSuccess implies the client has no TS packet to transmit. 
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef IOReturn (*FWAVCMPEG2TransmitterTimeStampProc) (FWAVCDeviceStreamRef fwavcDeviceStreamRef,
														FWAVCDeviceRef fwavcDeviceRef,
														UInt64 pcr, 
														UInt64 transmitTimeInNanoSeconds, 
														void *pRefCon);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@typedef FWAVCDVTransmitterFrameRequestProc
 
	@abstract This is the prototype for the callback to pull data from the client for a dv-transmitter AVC device stream.
 
	@discussion This callback requests one DV frame from the client.
 
	@param fwavcDeviceStreamRef The reference to the AVC device stream that is making this call.
 
	@param fwavcDeviceRef The reference to the AVC device associated with this stream.
 
	@param pTransmitterFrameRef A pointer for the client to pass in a reference to the DV transmitter frame.
 
	@param pRefCon A pointer to client supplied private data returned with this callback.
 
	@result A return value of anything other than kIOReturnSuccess implies the client has no frame to transmit. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef IOReturn (*FWAVCDVTransmitterFrameRequestProc)  (FWAVCDeviceStreamRef fwavcDeviceStreamRef,
														 FWAVCDeviceRef fwavcDeviceRef,
														 FWAVCDVTransmitterFrameRef *pTransmitterFrameRef,
														 void *pRefCon);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@typedef FWAVCDVTransmitterFrameReturnProc
 
	@abstract This is the prototype for the callback from a dv-transmitter AVC device stream, to return a DV 
 frame back to the client, for potential reuse.
 
	@discussion Note that the SYT and transmit time-stamps for the returned frame are now valid.
 
	@param fwavcDeviceStreamRef The reference to the AVC device stream that is making this call.
 
	@param fwavcDeviceRef The reference to the AVC device associated with this stream.

	@param transmitterFrameRef The reference to the frame being returned to the client.
 
	@param pRefCon A pointer to client supplied private data returned with this callback.
 
	@result The value return by the client is currently ignored. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef IOReturn (*FWAVCDVTransmitterFrameReturnProc)  (FWAVCDeviceStreamRef fwavcDeviceStreamRef,
														 FWAVCDeviceRef fwavcDeviceRef,
														 FWAVCDVTransmitterFrameRef transmitterFrameRef,
														 void *pRefCon);


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@typedef FWAVCDVReceiverFrameReceivedProc
 
	@abstract This is the prototype for the callback from a dv-receiver AVC device stream, to deliver a DV frame to the client.
 
	@param fwavcDeviceStreamRef The reference to the AVC device stream that is making this call.
 
	@param fwavcDeviceRef The reference to the AVC device associated with this stream.
 
	@param msg The message associated with this callback.
 
	@param fwavcDVReceiverFrameRef If non-nil, a reference to a DV receiver frame.
 
	@param pRefCon A pointer to client supplied private data returned with this callback.
 
	@result A return value of anything other than kIOReturnSuccess implies the client will not call 
 FWAVCDVReceiverReleaseDVFrame on the DV receiver frame.
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef IOReturn (*FWAVCDVReceiverFrameReceivedProc) (FWAVCDeviceStreamRef fwavcDeviceStreamRef,
													  FWAVCDeviceRef fwavcDeviceRef,
													  FWAVCDVReceiverCallbackMessage msg,
													  FWAVCDVReceiverFrameRef fwavcDVReceiverFrameRef, 
													  void *pRefCon);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@typedef FWAVCDeviceStreamMessageProc
 
	@abstract This is the prototype for the callback from an AVC device stream, to deliver stream-related messages to the client.

	@param fwavcDeviceStreamRef The reference to the AVC device stream that is making this call.
 
	@param fwavcDeviceRef The reference to the AVC device associated with this stream.
 
	@param msg The message being sent to the client.
 
	@param param1 The first parameter associated with the message.
 
	@param param2 The second parameter associated with the message.
 
	@param pRefCon A pointer to client supplied private data returned with this callback.
 
	@result The value return by the client is currently ignored. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef void (*FWAVCDeviceStreamMessageProc) (FWAVCDeviceStreamRef fwavcDeviceStreamRef,
											  FWAVCDeviceRef fwavcDeviceRef,
											  UInt32 msg, 
											  UInt32 param1, 
											  UInt32 param2, 
											  void *pRefCon);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@typedef FWAVCIsochReceiverNoDataProc
 
	@abstract This is the prototype for the callback from a receive-type AVC device stream, to report no data
 has been received for the specified amout of time.
  
	@param fwavcDeviceStreamRef The reference to the AVC device stream that is making this call.
 
	@param fwavcDeviceRef The reference to the AVC device associated with this stream.
 
	@param pRefCon A pointer to client supplied private data returned with this callback.
 
	@result The value return by the client is currently ignored. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef IOReturn (*FWAVCIsochReceiverNoDataProc) (FWAVCDeviceStreamRef fwavcDeviceStreamRef,
												  FWAVCDeviceRef fwavcDeviceRef,
												  void *pRefCon);


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@typedef FWAVCDVFramerCallback
 
	@abstract This is the prototype for the callback from the DV framer to deliver fully "re-framed" DV frames, or to report errors.
 
	@param msg The message associated with this callback.
 
	@param fwavcDVFramerFrameRef If non-nil, a reference to the DV framer frame.
 
	@param pRefCon A pointer to client supplied private data returned with this callback.

	@param fwavcDVFramerRef A reference to the DV framer making this call.
  
	@result The value return by the client is currently ignored. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef IOReturn (*FWAVCDVFramerCallback) (FWAVCDVFramerCallbackMessage msg, 
										   FWAVCDVFramerFrameRef fwavcDVFramerFrameRef, 
										   void *pRefCon, 
										   FWAVCDVFramerRef fwavcDVFramerRef);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FWAVCTSDemuxerCallback - 
//     The prototype for the callback from the TS demuxer to deliver fully "demuxed" PES packets, 
//     or to report errors.
/*!
	@typedef FWAVCTSDemuxerCallback
 
	@abstract This is the prototype for the callback from the TS demuxer, to deliver fully "demuxed" PES packets, or to report errors.
 
	@param msg The message associated with this callback.

	@param fwavcTSDemuxerPESPacketRef A reference to the PES packet.

	@param pRefCon A pointer to client supplied private data returned with this callback.
	
	@param fwavcDVFramerRef A reference to the TS demuxer making this call.
 
	@result The value return by the client is currently ignored. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef IOReturn (*FWAVCTSDemuxerCallback) (FWAVCTSDemuxerCallbackMessage msg, 
											FWAVCTSDemuxerPESPacketRef fwavcTSDemuxerPESPacketRef, 
											void *pRefCon,
											FWAVCTSDemuxerRef fwavcTSDemuxerRef);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@typedef FWAVCTSDemuxerHDV2VAUXCallback
 
	@abstract This is the prototype for the callback from the TS demuxer, called when a HDV2 VAux packet is received and parsed.
 
	@param pFWAVCTSDemuxerHDV2VideoFramePack A pointer to the parsed VAux packet structure.
 
	@param pRefCon A pointer to client supplied private data returned with this callback.
 
	@param fwavcDVFramerRef A reference to the TS demuxer making this call.

	@result The value return by the client is currently ignored. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef void (*FWAVCTSDemuxerHDV2VAUXCallback) (FWAVCTSDemuxerHDV2VideoFramePack *pFWAVCTSDemuxerHDV2VideoFramePack, 
												void *pRefCon,
												FWAVCTSDemuxerRef fwavcTSDemuxerRef);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@typedef FWAVCTSDemuxerHDV1PackDataCallback
 
	@abstract This is the prototype for the callback from the TS demuxer, called when a HDV1 PackData packet is received and parsed.
 
	@param pFWAVCTSDemuxerHDV1PackData A pointer to the parsed PackData packet structure.
 
	@param pRefCon A pointer to client supplied private data returned with this callback.
 
	@param fwavcDVFramerRef A reference to the TS demuxer making this call.
 
	@result The value return by the client is currently ignored. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef void (*FWAVCTSDemuxerHDV1PackDataCallback) (FWAVCTSDemuxerHDV1PackData *pFWAVCTSDemuxerHDV1PackData, 
													void *pRefCon,
													FWAVCTSDemuxerRef fwavcTSDemuxerRef);


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@typedef FWAVCDebugLoggingStringHandler
 
	@abstract This is the prototype for the callback that delivers debugging log-strings to the client.  
 
	@param pString A pointer to the log string.
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef void (*FWAVCDebugLoggingStringHandler) (char *pString);


#pragma mark -
#pragma mark ======================================
#pragma mark Front-end APIs for the AVCDeviceController class object
#pragma mark ======================================

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDeviceControllerCreate

	@abstract Create an AVC device controller (and its dedicated notification thread).

	@discussion Note that the returned FWAVCDeviceControllerRef already has a retain count of 1.

	@param pFWAVCDeviceControllerRef A pointer to return the reference to the created AVC device controller.

	@param clientNotificationProc The callback function for alerting the client when an AVC device is discovered.

	@param pRefCon A pointer to client supplied private data returned with the callbacks.
	
	@param globalAVCDeviceMessageProc The (optional) callback for delivering IOKit messages for all AVC devices to the client.
 
	@param debugLogStringProc The (optional) callback for delivery of debugging log-strings to the client.
	
	@result kIOReturnSuccess if successful, specific error otherwise. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
IOReturn FWAVCDeviceControllerCreate(FWAVCDeviceControllerRef *pFWAVCDeviceControllerRef,
									 FWAVCDeviceControllerNotification clientNotificationProc,
									 void *pRefCon,
									 FWAVCDeviceMessageNotification globalAVCDeviceMessageProc,
									 FWAVCDebugLoggingStringHandler debugLogStringProc)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDeviceControllerRetain
 
	@abstract Increment the retain count on an AVC device controller.
 
	@param fwavcDeviceControllerRef The reference to the AVC device controller.
 
	@result The reference to the AVC device controller. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
FWAVCDeviceControllerRef FWAVCDeviceControllerRetain(FWAVCDeviceControllerRef fwavcDeviceControllerRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDeviceControllerRelease
 
	@abstract Decrement the retain count on an AVC device controller, and delete it if the retain count drops down to 0.
 
	@param fwavcDeviceControllerRef The reference to the AVC device controller.
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
void  FWAVCDeviceControllerRelease(FWAVCDeviceControllerRef fwavcDeviceControllerRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDeviceControllerCopyDeviceArray
 
	@abstract Get a copy of the AVC device controller's array of detected devices.
 
	@discussion The returned array contains a FWAVCDeviceRef for each discovered AVC device. Note that these 
 FWAVCDeviceRef instances exist in the array, even when an AVC device has been disconnected, 
 (though the device is marked as not-attached, until reconnected). The client shall release this array when done
 with it.
 
	@param fwavcDeviceControllerRef The reference to the AVC device controller.
 
	@result A CFArrayRef with the AVC device controller's list of known AVC devices.  
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
CFArrayRef FWAVCDeviceControllerCopyDeviceArray(FWAVCDeviceControllerRef fwavcDeviceControllerRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;


#pragma mark -
#pragma mark ======================================
#pragma mark Front-end APIs for the AVCDevice class object
#pragma mark ======================================

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDeviceOpen
 
	@abstract Open an AVC device for exclusive access.
 
	@discussion This function must be called before commands can be sent to the AVC device, or before AVC device
 streams can be created for the AVC device.
 
	@param fwavcDeviceRef The reference to the AVC device.
 
	@param deviceMessageProc The callback for delivering IOKit messages for this AVC device to the client.
 
	@param pMessageProcRefCon A pointer to client supplied private data returned with the callbacks.
 
	@result kIOReturnSuccess if successful, specific error otherwise. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
IOReturn FWAVCDeviceOpen(FWAVCDeviceRef fwavcDeviceRef, FWAVCDeviceMessageNotification deviceMessageProc, void *pMessageProcRefCon)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDeviceClose
 
	@abstract Close an AVC device, and release exclusive access.
 
	@param fwavcDeviceRef The reference to the AVC device.
 
	@result kIOReturnSuccess if successful, specific error otherwise. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
IOReturn FWAVCDeviceClose(FWAVCDeviceRef fwavcDeviceRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDeviceAVCCommand
 
	@abstract Send an AVC command to an (opened) AVC device.
 
	@param fwavcDeviceRef The reference to the AVC device.

 	@param command The buffer containing the AVC command bytes.
 
	@param cmdLen The number of AVC command bytes.
 
	@param response The buffer for returning the AVC response bytes.

	@param responseLen On input, the size of the response buffer. On output, the actual number of response bytes returned.
	
	@result kIOReturnSuccess if successful, specific error otherwise. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
IOReturn FWAVCDeviceAVCCommand(FWAVCDeviceRef fwavcDeviceRef, const UInt8 *command, UInt32 cmdLen, UInt8 *response, UInt32 *responseLen)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDeviceIsOpened
 
	@abstract Check to see if the specified AVC device has been opened by this client for exclusive access.
 
	@param fwavcDeviceRef The reference to the AVC device.
 
	@result Returns true, if the AVC device is currently opened by this client, false otherwise. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
Boolean FWAVCDeviceIsOpened(FWAVCDeviceRef fwavcDeviceRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDeviceCapabilitiesDiscovered
 
	@abstract Check to see if the AVC device controller has been able to discover the capabilities 
 of the AVC device.
 
	@discussion The AVC device controller must be able to open the device for exclusive access for a short time before
 it can determine many of the AVC device's properties. If the AVC device is already in use (by another process or component,
 it will monitor the AVC device's IOKit messages, and when it sees that the current owner of the device releases 
 its exclusive access (closes it), it will try again to get access to the device to discover its capabilites. 
 Note that many properties of the AVC devices are indeterminate until this is true.
 
	@param fwavcDeviceRef The reference to the AVC device.
 
	@result Returns true if the AVC device controller has been able to discover the capabilities of the AVC
 device, false otherwise. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
Boolean FWAVCDeviceCapabilitiesDiscovered(FWAVCDeviceRef fwavcDeviceRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDeviceReDiscoverAVCDeviceCapabilities
 
	@abstract Allows the client to manually force the rediscovery of an AVC device's capabilites.
 
	@discussion Useful if the client suspects the AVC device's capabilites may have changed since they 
 were last discovered. Note that this function will fail if the device is currently opened by this client,
 or any other client.
 
	@param fwavcDeviceRef The reference to the AVC device.
 
	@result kIOReturnSuccess if successful, specific error otherwise. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
IOReturn FWAVCDeviceReDiscoverAVCDeviceCapabilities(FWAVCDeviceRef fwavcDeviceRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDeviceGetGUID
 
	@abstract Check the GUID of the AVC device.
 
	@param fwavcDeviceRef The reference to the AVC device.
 
	@result Returns the 64-bit GUID of the AVC device. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
UInt64 FWAVCDeviceGetGUID(FWAVCDeviceRef fwavcDeviceRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDeviceCopyDeviceName
 
	@abstract Access the AVC device's model name.
 
	@param fwavcDeviceRef The reference to the AVC device.
 
	@result Returns a CFStringRef containing the AVC device's model name. The client shall release the CFStringRef when done. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
CFStringRef FWAVCDeviceCopyDeviceName(FWAVCDeviceRef fwavcDeviceRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDeviceCopyVendorName
 
	@abstract Access the AVC device's vendor name.
 
	@param fwavcDeviceRef The reference to the AVC device.
 
	@result Returns a CFStringRef containing the AVC device's vendor name. The client shall release the CFStringRef when done. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
CFStringRef FWAVCDeviceCopyVendorName(FWAVCDeviceRef fwavcDeviceRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDeviceGetVendorID
 
	@abstract Check the AVC device's vendor ID.
 
	@param fwavcDeviceRef The reference to the AVC device.
 
	@result Returns the 32-bit vendor-ID for the AVC device. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
UInt32 FWAVCDeviceGetVendorID(FWAVCDeviceRef fwavcDeviceRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDeviceGetModelID
 
	@abstract Check the AVC device's model ID.
 
	@param fwavcDeviceRef The reference to the AVC device.
 
	@result Returns the 32-bit model-ID for the AVC device. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
UInt32 FWAVCDeviceGetModelID(FWAVCDeviceRef fwavcDeviceRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDeviceIsAttached
 
	@abstract Check the see if the AVC device is currently connected to the Mac.
 
	@param fwavcDeviceRef The reference to the AVC device.
 
	@result Returns true if the AVC device is currently attached to the Mac, false otherwise. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
Boolean FWAVCDeviceIsAttached(FWAVCDeviceRef fwavcDeviceRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDeviceSupportsFCP
 
	@abstract Check to see if the AVC device is known to responds to AVC commands.
 
	@param fwavcDeviceRef The reference to the AVC device.
 
	@result Returns true if the AVC device is known to responds to AVC commands, false otherwise. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
Boolean FWAVCDeviceSupportsFCP(FWAVCDeviceRef fwavcDeviceRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDeviceHasTapeSubunit
 
	@abstract Check to see if the AVC device has an AVC tape subunit.
 
	@discussion Note that the response is only valid once the AVC device's capabilities have been discovered
 by the AVC device controller.
 
	@param fwavcDeviceRef The reference to the AVC device.
 
	@result Returns true if the AVC device reports to include a tape-subunit, false otherwise. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
Boolean FWAVCDeviceHasTapeSubunit(FWAVCDeviceRef fwavcDeviceRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDeviceHasMonitorOrTunerSubunit
 
	@abstract Check to see if the AVC device has an AVC monitor or tuner subunit.
 
	@discussion Note that the response is only valid once the AVC device's capabilities have been discovered
 by the AVC device controller.
 
	@param fwavcDeviceRef The reference to the AVC device.
 
	@result Returns true if the AVC device reports to include a monitor or tuner subunit, false otherwise. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
Boolean FWAVCDeviceHasMonitorOrTunerSubunit(FWAVCDeviceRef fwavcDeviceRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FWAVCDeviceGetSubUnits - 
/*!
	@function FWAVCDeviceGetSubUnits
 
	@abstract Get the subunit info for an AVC device.
 
	@discussion Returns a 32-bit value containing information on what kind and count of subunits the AVC device
 contains. The 32-bit value can be interpreted as 4 8-bit fields, and follows the response format from the
 1394 trade-association's "AVC General Specification" on the SUBUNIT_INFO command. Specifically, eacch 8-bit
 field includes a 5-bit subunit-type, and a 3-bit subunit count. A value of 0xFF for any of the bytes indicates
 no subunit information. Note that the response is only valid once the AVC device's capabilities have been 
 discovered by the AVC device controller.
  
	@param fwavcDeviceRef The reference to the AVC device.
 
	@result Returns the 32-bit subunit info value. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
UInt32 FWAVCDeviceGetSubUnits(FWAVCDeviceRef fwavcDeviceRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDeviceNumInputPlugs
 
	@abstract Get the number of input plugs for an AVC device.
 
	@discussion Note that the response is only valid once the AVC device's capabilities have been discovered
 by the AVC device controller.
 
	@param fwavcDeviceRef The reference to the AVC device.
 
	@result Returns the number of input plugs the AVC device reports to have. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
UInt32 FWAVCDeviceNumInputPlugs(FWAVCDeviceRef fwavcDeviceRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDeviceNumOutputPlugs
 
	@abstract Get the number of output plugs for an AVC device.
 
	@discussion Note that the response is only valid once the AVC device's capabilities have been discovered
 by the AVC device controller.
 
	@param fwavcDeviceRef The reference to the AVC device.
 
	@result Returns the number of output plugs the AVC device reports to have. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
UInt32 FWAVCDeviceNumOutputPlugs(FWAVCDeviceRef fwavcDeviceRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDeviceIsDVDevice
 
	@abstract Check to see if the AVC device is a DV streaming device.
 
	@discussion Note that the response is only valid once the AVC device's capabilities have been discovered
 by the AVC device controller.
 
	@param fwavcDeviceRef The reference to the AVC device.
 
	@result Returns true if the AVC device is a DV device, false otherwise. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
Boolean FWAVCDeviceIsDVDevice(FWAVCDeviceRef fwavcDeviceRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDeviceIsDVCProDevice
 
	@abstract Check to see if the AVC device is a DVCPro type DV streaming device.
 
	@discussion A DVCProDevice type device includes DVCPro, DVCPro50, and DVCProHD devices. Note that the
 response is only valid once the AVC device's capabilities have been discovered by the AVC device controller.
 
	@param fwavcDeviceRef The reference to the AVC device.
 
	@result Returns true if the AVC device is a DVCPro device, false otherwise. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
Boolean FWAVCDeviceIsDVCProDevice(FWAVCDeviceRef fwavcDeviceRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDeviceGetDVMode
 
	@abstract For DV type AVC devices, check to see what DV streaming mode the device supports.
 
	@discussion Note that the response is only valid once the AVC device's capabilities have been discovered
 by the AVC device controller.
 
	@param fwavcDeviceRef The reference to the AVC device.
 
	@result For DV and/or DVCPro type AVC devices, returns the specific DV mode the AVC device reports to support. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
UInt8 FWAVCDeviceGetDVMode(FWAVCDeviceRef fwavcDeviceRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDeviceIsMpegDevice
 
	@abstract Check to see if the AVC device is a MPEG streaming device.
 
	@discussion Note that the response is only valid once the AVC device's capabilities have been discovered
 by the AVC device controller.
 
	@param fwavcDeviceRef The reference to the AVC device.
 
	@result Returns true if the AVC device is a MPEG device, false otherwise. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
Boolean FWAVCDeviceIsMpegDevice(FWAVCDeviceRef fwavcDeviceRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDeviceGetMpegMode
 
	@abstract For MPEG type AVC devices, check to see what MPEG streaming mode the device supports.
 
	@discussion Note that the response is only valid once the AVC device's capabilities have been discovered
 by the AVC device controller.
 
	@param fwavcDeviceRef The reference to the AVC device.
 
	@result For MPEG type AVC devices, returns the specific MPEG mode the AVC device reports to support. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
UInt8 FWAVCDeviceGetMpegMode(FWAVCDeviceRef fwavcDeviceRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FWAVCDeviceGetAVCInterface - 
//     Returns a pointer to the IOFireWireAVCLibUnit interface for the AVC device, if it's open.
/*!
	@function FWAVCDeviceGetAVCInterface
 
	@abstract Get access to the AVC device's IOFireWireAVCLibUnit interface for the AVC device, if it's open.
 
	@param fwavcDeviceRef The reference to the AVC device.
 
	@result Returns the IOFireWireAVCLibUnit interface for the AVC device, if it's open. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
IOFireWireAVCLibUnitInterface** FWAVCDeviceGetAVCInterface(FWAVCDeviceRef fwavcDeviceRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDeviceGetDeviceInterface
 
	@abstract Get access to the AVC device's IOFireWireAVCLibUnit interface for the AVC device, if it's open.
 
	@param fwavcDeviceRef The reference to the AVC device.
 
	@result Returns the IOFireWireAVCLibUnit interface for the AVC device, if it's open. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
IOFireWireLibDeviceRef FWAVCDeviceGetDeviceInterface(FWAVCDeviceRef fwavcDeviceRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDeviceSetClientPrivateData
 
	@abstract Allows the client to attach some private data to an AVC device.
 
	@param fwavcDeviceRef The reference to the AVC device.
 
	@param pClientPrivateData A pointer to the client's private data.
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
void FWAVCDeviceSetClientPrivateData(FWAVCDeviceRef fwavcDeviceRef, void *pClientPrivateData)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDeviceGetClientPrivateData
 
	@abstract Allows the client to fetch the client's private data previously attached to an AVC device.
 
	@param fwavcDeviceRef The reference to the AVC device.

	@result Returns the pointer to the client's private data. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
void *FWAVCDeviceGetClientPrivateData(FWAVCDeviceRef fwavcDeviceRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDeviceCreateUniversalReceiver
 
	@abstract Create a universal isoch receiver for an AVC device.
 
	@discussion Note that the client can specify kFWAVCDeviceStreamDefaultParameter for cyclesPerSegment, numSegments,
 and cycleBufferSize as an alternative to specifying specific values. This will cause the default values to be used
 instead.
 
	@param fwavcDeviceRef The reference to the AVC device.
 
	@param deviceOutputPlugNum The AVC device's plug number for the stream.
 
	@param dataReceivedProcHandler The callback function for delivering isoch data to the client.
 
	@param pDataReceivedProcRefCon A pointer to client supplied private data returned with the data delivery callback.
 
	@param messageProcHandler The callback function for delivering messages regarding the device stream.
 
	@param pMessageProcRefCon A pointer to client supplied private data returned with the message delivery callback.
 
	@param cyclesPerSegment The number of isoch cycles per DCL program segment.
 
	@param numSegments The number of segments in the DCL program.
 
	@param cycleBufferSize The size of the isoch receive buffer for each isoch cycle.

	@param debugLogStringProc The (optional) callback for delivery of debugging log-strings to the client.

	@result If successful, returns an AVC device stream ref. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
FWAVCDeviceStreamRef FWAVCDeviceCreateUniversalReceiver(FWAVCDeviceRef fwavcDeviceRef,
														UInt8 deviceOutputPlugNum,
														FWAVCUniversalReceiverDataReceivedProc dataReceivedProcHandler,
														void *pDataReceivedProcRefCon,
														FWAVCDeviceStreamMessageProc messageProcHandler,
														void *pMessageProcRefCon,
														unsigned int cyclesPerSegment,
														unsigned int numSegments,
														unsigned int cycleBufferSize,
														FWAVCDebugLoggingStringHandler debugLogStringProc)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDeviceCreateMPEGReceiver
 
	@abstract Create a mpeg-2 receiver for an AVC device.
 
	@discussion Note that the client can specify kFWAVCDeviceStreamDefaultParameter for cyclesPerSegment, and numSegments,
 as an alternative to specifying specific values. This will cause the default values to be used instead.
 
	@param fwavcDeviceRef The reference to the AVC device.
 
	@param deviceOutputPlugNum The AVC device's plug number for the stream.
 
	@param dataReceivedProcHandler The callback function for delivering isoch data to the client.
 
	@param pDataReceivedProcRefCon A pointer to client supplied private data returned with the data delivery callback.
 
	@param messageProcHandler The callback function for delivering messages regarding the device stream.
 
	@param pMessageProcRefCon A pointer to client supplied private data returned with the message delivery callback.
 
	@param cyclesPerSegment The number of isoch cycles per DCL program segment.
 
	@param numSegments The number of segments in the DCL program.
 
	@param debugLogStringProc The (optional) callback for delivery of debugging log-strings to the client.
 
	@result If successful, returns an AVC device stream ref. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
FWAVCDeviceStreamRef FWAVCDeviceCreateMPEGReceiver(FWAVCDeviceRef fwavcDeviceRef,
												   UInt8 deviceOutputPlugNum,
												   FWAVCMPEG2ReceiverDataReceivedProc dataReceivedProcHandler,
												   void *pDataReceivedProcRefCon,
												   FWAVCDeviceStreamMessageProc messageProcHandler,
												   void *pMessageProcRefCon,
												   unsigned int cyclesPerSegment,
												   unsigned int numSegments,
												   FWAVCDebugLoggingStringHandler debugLogStringProc)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDeviceCreateMPEGTransmitter
 
	@abstract Create a mpeg-2 transmitter for an AVC device.
 
	@discussion Note that the client can specify kFWAVCDeviceStreamDefaultParameter for cyclesPerSegment, numSegments,
 packetsPerCycle, and tsPacketQueueSizeInPackets as an alternative to specifying specific values. This will cause the
 default values to be used instead.
 
	@param fwavcDeviceRef The reference to the AVC device.
 
	@param deviceOutputPlugNum The AVC device's plug number for the stream.
 
	@param dataRequestProcHandler The callback function for requesting isoch data from the client.
 
	@param pDataRequestProcRefCon A pointer to client supplied private data returned with the data request callback.
 
	@param messageProcHandler The callback function for delivering messages regarding the device stream.
 
	@param pMessageProcRefCon A pointer to client supplied private data returned with the message delivery callback.
 
	@param cyclesPerSegment The number of isoch cycles per DCL program segment.
 
	@param numSegments The number of segments in the DCL program.
 
	@param packetsPerCycle The number of TS packets included in each isoch packet that contains mpeg-2 data.

	@param tsPacketQueueSizeInPackets The size (in packets) if the data-rate analysis queue.
 
	@param debugLogStringProc The (optional) callback for delivery of debugging log-strings to the client.

	@result If successful, returns an AVC device stream ref. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
FWAVCDeviceStreamRef FWAVCDeviceCreateMPEGTransmitter(FWAVCDeviceRef fwavcDeviceRef,
													  UInt8 deviceInputPlugNum,
													  FWAVCMPEG2TransmitterDataRequestProc dataRequestProcHandler,
													  void *pDataRequestProcRefCon,
													  FWAVCDeviceStreamMessageProc messageProcHandler,
													  void *pMessageProcRefCon,
													  unsigned int cyclesPerSegment,
													  unsigned int numSegments,
													  unsigned int packetsPerCycle,
													  unsigned int tsPacketQueueSizeInPackets,
													  FWAVCDebugLoggingStringHandler debugLogStringProc)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDeviceCreateDVTransmitter
 
	@abstract Create a DV transmitter for an AVC device.
 
	@discussion Note that the client can specify kFWAVCDeviceStreamDefaultParameter for cyclesPerSegment, numSegments,
 transmitterDVMode, and numFrameBuffers as an alternative to specifying specific values. This will cause the
 default values to be used instead.
 
	@param fwavcDeviceRef The reference to the AVC device.
 
	@param deviceOutputPlugNum The AVC device's plug number for the stream.
 
	@param frameRequestProcHandler The callback function for requesting DV frames from the client.
 
	@param pFrameRequestProcRefCon A pointer to client supplied private data returned with the DV frame request callback.
 
	@param frameReturnProcHandler The callback function for returning DV frames to the client.
 
	@param pFrameReleaseProcRefCon A pointer to client supplied private data returned with the DV frame return callback.
 
	@param messageProcHandler The callback function for delivering messages regarding the device stream.
 
	@param pMessageProcRefCon A pointer to client supplied private data returned with the message delivery callback.
 
	@param cyclesPerSegment The number of isoch cycles per DCL program segment.
 
	@param numSegments The number of segments in the DCL program.
 
	@param transmitterDVMode The DV mode for this DV transmitter.
 
	@param numFrameBuffers The number of frame buffers to create for this DV transmitter.
 
	@param debugLogStringProc The (optional) callback for delivery of debugging log-strings to the client.
 
	@result If successful, returns an AVC device stream ref. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
FWAVCDeviceStreamRef FWAVCDeviceCreateDVTransmitter(FWAVCDeviceRef fwavcDeviceRef,
													UInt8 deviceInputPlugNum,
													FWAVCDVTransmitterFrameRequestProc frameRequestProcHandler,
													void *pFrameRequestProcRefCon,
													FWAVCDVTransmitterFrameReturnProc frameReturnProcHandler,
													void *pFrameReleaseProcRefCon,
													FWAVCDeviceStreamMessageProc messageProcHandler,
													void *pMessageProcRefCon,
													unsigned int cyclesPerSegment,
													unsigned int numSegments,
													UInt8 transmitterDVMode,
													UInt32 numFrameBuffers,
													FWAVCDebugLoggingStringHandler debugLogStringProc)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDeviceCreateDVReceiver
 
	@abstract Create a DV receiver for an AVC device.
 
	@discussion Note that the client can specify kFWAVCDeviceStreamDefaultParameter for cyclesPerSegment, numSegments,
 receiverDVMode, and numFrameBuffers as an alternative to specifying specific values. This will cause the
 default values to be used instead.

	@param fwavcDeviceRef The reference to the AVC device.
 
	@param deviceOutputPlugNum The AVC device's plug number for the stream.
 
	@param frameReceivedProcHandler The callback function for delivering DV frames from the client.
 
	@param pFrameReceivedProcRefCon A pointer to client supplied private data returned with the DV frame received callback.
 
	@param messageProcHandler The callback function for delivering messages regarding the device stream.
 
	@param pMessageProcRefCon A pointer to client supplied private data returned with the message delivery callback.
 
	@param cyclesPerSegment The number of isoch cycles per DCL program segment.
 
	@param numSegments The number of segments in the DCL program.
 
	@param receiverDVMode The DV mode for this DV receiver.
 
	@param numFrameBuffers The number of frame buffers to create for this DV receiver.
 
	@param debugLogStringProc The (optional) callback for delivery of debugging log-strings to the client.

	@result If successful, returns an AVC device stream ref. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
FWAVCDeviceStreamRef FWAVCDeviceCreateDVReceiver(FWAVCDeviceRef fwavcDeviceRef,
												 UInt8 deviceOutputPlugNum,
												 FWAVCDVReceiverFrameReceivedProc frameReceivedProcHandler,
												 void *pFrameReceivedProcRefCon,
												 FWAVCDeviceStreamMessageProc messageProcHandler,
												 void *pMessageProcRefCon,
												 unsigned int cyclesPerSegment,
												 unsigned int numSegments,
												 UInt8 receiverDVMode,
												 UInt32 numFrameBuffers,
												 FWAVCDebugLoggingStringHandler debugLogStringProc)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDeviceStreamRetain
 
	@abstract Increment the retain count on an AVC device stream.
 
	@param fwavcDeviceStreamRef The reference to the AVC device stream.
 
	@result The reference to the AVC device stream. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
FWAVCDeviceStreamRef FWAVCDeviceStreamRetain(FWAVCDeviceStreamRef fwavcDeviceStreamRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDeviceStreamRelease
 
	@abstract Decrement the retain count on an AVC device stream, and delete it if the retain count drops down to 0.
 
	@param fwavcDeviceStreamRef The reference to the AVC device stream.
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
void FWAVCDeviceStreamRelease(FWAVCDeviceStreamRef fwavcDeviceStreamRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDeviceStreamStart
 
	@abstract Start a managed AVC device stream.
 
	@param fwavcDeviceStreamRef The reference to the AVC device stream.
 
	@result kIOReturnSuccess if successful, specific error otherwise. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
IOReturn FWAVCDeviceStreamStart(FWAVCDeviceStreamRef fwavcDeviceStreamRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDeviceStreamStop
 
	@abstract Stop a managed AVC device stream.
 
	@param fwavcDeviceStreamRef The reference to the AVC device stream.
 
	@result kIOReturnSuccess if successful, specific error otherwise. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
IOReturn FWAVCDeviceStreamStop(FWAVCDeviceStreamRef fwavcDeviceStreamRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDeviceStreamSetNoDataCallback
 
	@abstract For receive-type AVC device streams, register a no-data callback.
 
	@param fwavcDeviceStreamRef The reference to the AVC device stream.

 	@param handler The callback function for notifying the client of no data received for the specified duration.
	
	@param pRefCon A pointer to client supplied private data returned with the no-data received callback.
	
	@param noDataTimeInMSec How long to wait (in milliseconds) before reporting no-data to the client
	
	@result kIOReturnSuccess if successful, specific error otherwise. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
IOReturn FWAVCDeviceStreamSetNoDataCallback(FWAVCDeviceStreamRef fwavcDeviceStreamRef,
											FWAVCIsochReceiverNoDataProc handler, 
											void *pRefCon, 
											UInt32 noDataTimeInMSec)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCUniversalReceiverSetStructuredDataReceivedCallback
 
	@abstract For universal receiver type AVC device streams, register the alternate "structured" callback.
 
	@discussion This alternative "structured" callback, allows the client to spzecifiy that it wishes to reduce 
 the overall number of data-delivery callbacks made from the universal receiver. With this callback, data from
 more than one isochronous cycle can be combined into a single callback. The minimum number of cycles worth of data
 to be delivered by this callback is 1. The maximum is the number of cycles in the universal receiver's DCL 
 program segment (which was specified when the universal receiver was created).
 
	@param fwavcDeviceStreamRef The reference to the AVC device stream.
 
	@param handler The callback function for delivering data to the client.
	
	@param maxCycleStructsPerCallback The maximum number of cycle worth of data that should be delivered in one callback.
 
	@param pRefCon A pointer to client supplied private data returned with the data received callback.
 
	@result kIOReturnSuccess if successful, specific error otherwise. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
IOReturn FWAVCUniversalReceiverSetStructuredDataReceivedCallback(FWAVCDeviceStreamRef fwavcDeviceStreamRef,
																 FWAVCUniversalReceiverStructuredDataReceivedProc handler, 
																 UInt32 maxCycleStructsPerCallback, 
																 void *pRefCon)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCMPEG2TransmitterSetTimeStampCallback
 
	@abstract For mpeg-2 transmitter type AVC device streams, register a callback for delivery of transmit time-stamp information.
 
	@discussion This function allows the client to register for callbacks containing timing information regarding when transport-stream
 packets with PCRs are scheduled for transmission over the FireWire bus. The reported time-stamp is in nano-seconds and based on the CPU
 UpTime() clock.
 
	@param fwavcDeviceStreamRef The reference to the AVC device stream.
 
	@param handler The callback function for delivering time-stamp information to the client.
	 
	@param pRefCon A pointer to client supplied private data returned with the data received callback.
 
	@result kIOReturnSuccess if successful, specific error otherwise. 
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
IOReturn FWAVCMPEG2TransmitterSetTimeStampCallback(FWAVCDeviceStreamRef fwavcDeviceStreamRef,
												   FWAVCMPEG2TransmitterTimeStampProc handler, 
												   void *pRefCon)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;
	
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCMPEG2ReceiverSetStructuredDataReceivedCallback
 
	@abstract For mpeg-2 receiver type AVC device streams, register the alternate "structured" callback.
 
	@discussion This alternative "structured" callback, allows the client to spzecifiy that it wishes to reduce 
 the overall number of data-delivery callbacks made from the mpeg-2 receiver. With this callback, data from
 more than one isochronous cycle can be combined into a single callback. The minimum number of cycles worth of data
 to be delivered by this callback is 1. The maximum is the number of cycles in the mpeg-2 receiver's DCL 
 program segment (which was specified when the mpeg-2 receiver was created).
 
	@param fwavcDeviceStreamRef The reference to the AVC device stream.

	@param handler The callback function for delivering data to the client.
	
	@param maxCycleStructsPerCallback The maximum number of cycle worth of data that should be delivered in one callback.
 
	@param pRefCon A pointer to client supplied private data returned with the data received callback.

	@result kIOReturnSuccess if successful, specific error otherwise. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
IOReturn FWAVCMPEG2ReceiverSetStructuredDataReceivedCallback(FWAVCDeviceStreamRef fwavcDeviceStreamRef,
															 FWAVCMPEG2ReceiverStructuredDataReceivedProc handler, 
															 UInt32 maxCycleStructsPerCallback, 
															 void *pRefCon)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCMPEG2ReceiverIncludeSourcePacketHeaders
 
	@abstract For mpeg2-receiver type avc device streams, specify whether or not the client wants access to the 
 source-packet-headers for received TS packets.
 
 	@discussion If the client calls this with a value of true, then when TS packets are delivered to the client, they will
 be 192-byte buffers including a 4-byte prepended source-packet-header. A value of False causes the mpeg2-receiver to deliver
 188-byte buffers without source-packet-headers. The initial-condition of the mpeg2-receiver is to deliver only 188-byte buffers
 without source-packet-headers.
 
	@param fwavcDeviceStreamRef The reference to the AVC device stream.

	@param wantSPH A value of True indicates the client wants 192-byte packets which include SPH. A value of false indicates
 the client wants 188-byte packets without SPH.
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
void FWAVCMPEG2ReceiverIncludeSourcePacketHeaders(FWAVCDeviceStreamRef fwavcDeviceStreamRef, Boolean wantSPH)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDVTransmitterCopyFrameRefArray
 
	@abstract For dv-transmitter type avc device streams, copy the array of dv transmit frame refs. Note that the
 client is responsible for releasing this array when done with it.
 
	@param fwavcDeviceStreamRef The reference to the AVC device stream.
 
	@result A CFArrayRef with the list of the DV transmitter's frames. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
CFArrayRef FWAVCDVTransmitterCopyFrameRefArray(FWAVCDeviceStreamRef fwavcDeviceStreamRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDVTransmitterFrameGetDataBuf
 
	@abstract For dv-transmitter type avc device streams frames, get pointers to a frame's buffer, and length.
 
	@param fwavcDVTransmitterFrameRef The reference to the DV transmitter frame.
 
	@param  ppFrameData A pointer to return the pointer to the frame-buffer.

	@param pFrameLen A pointer to return the length of the frame buffer.
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
void FWAVCDVTransmitterFrameGetDataBuf(FWAVCDVTransmitterFrameRef fwavcDVTransmitterFrameRef, UInt8 **ppFrameData, UInt32 *pFrameLen)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDVTransmitterFrameGetSYTTime
 
	@abstract For dv-transmitter type avc device streams frames, get the SYT time.
 
	@param fwavcDVTransmitterFrameRef The reference to the DV transmitter frame.
  
	@param pFrameSYTTime A pointer to return the SYT time-stamp .
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
void FWAVCDVTransmitterFrameGetSYTTime(FWAVCDVTransmitterFrameRef fwavcDVTransmitterFrameRef, UInt32 *pFrameSYTTime)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDVTransmitterFrameGetFireWireTimeStamp
 
	@abstract For dv-transmitter type avc device streams frames, get the FireWire cycle-timer time.
 
 	@discussion The "seconds" field in the time-stamp is not valid for frames returned before the first DCL callback in the program. The pTimeStampSecondsFieldValid
 frame parameter specifies whether or not the client can rely on the seconds field in the returned time-stamp. 
 
	@param fwavcDVTransmitterFrameRef The reference to the DV transmitter frame.
 
	@param pFrameTransmitStartCycleTime A pointer to return the FireWire time-stamp .

	@param pTimeStampSecondsFieldValid A pointer to return the boolean stating whether or not the seconds-field in the time-stamp is valid.
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
void FWAVCDVTransmitterFrameGetFireWireTimeStamp(FWAVCDVTransmitterFrameRef fwavcDVTransmitterFrameRef, UInt32 *pFrameTransmitStartCycleTime, Boolean *pTimeStampSecondsFieldValid)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCIsCIPPacket
 
	@abstract Evaluate the "tag" field from an isoch-header value, to determine if this packet is CIP-based.
 
	@param isochHeaderValue The 32-bit isoch header quadlet (in host-endianess)
 
	@result True if the isoch header specifies a CIP packet, false otherwise.
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
Boolean FWAVCIsCIPPacket(UInt32 isochHeaderValue)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCParseCIPPacket
 
	@abstract Parse a CIP-based packet.
 
	@param pPacketPayload A pointer to the isochronous packet payload.
 
	@param payloadLength The length of the isochronous packet payload.
 
	@param pParsedPacket A pointer to a FWAVCCIPPacketParserInfo structure used to return the results of the parsing.
 
	@result kIOReturnSuccess if successful, specific error otherwise. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
IOReturn FWAVCParseCIPPacket(UInt8 *pPacketPayload, UInt32 payloadLength, FWAVCCIPPacketParserInfo *pParsedPacket)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDVReceiverReleaseDVFrame
 
	@abstract Release a DV frame back to the DV receiver for reuse.
 
	@param fwavcDVReceiverFrameRef The reference to the DV receiver frame.
	
	@result kIOReturnSuccess if successful, specific error otherwise. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
IOReturn FWAVCDVReceiverReleaseDVFrame(FWAVCDVReceiverFrameRef fwavcDVReceiverFrameRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDVReceiverFrameGetDataBuf
 
	@abstract Returns a pointer to a frame's data-buffer.
 
	@param fwavcDVReceiverFrameRef The reference to the DV receiver frame.
	
	@result The pointer to the frame's buffer.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
UInt8 *FWAVCDVReceiverFrameGetDataBuf(FWAVCDVReceiverFrameRef fwavcDVReceiverFrameRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDVReceiverFrameGetLen
 
	@abstract Returns the length of a frame's data-buffer.
 
	@param fwavcDVReceiverFrameRef The reference to the DV receiver frame.
	
	@result The length of the frame's data-buffer.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
UInt32 FWAVCDVReceiverFrameGetLen(FWAVCDVReceiverFrameRef fwavcDVReceiverFrameRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDVReceiverFrameGetDVMode
 
	@abstract Returns the DV-mode of a frame.
 
	@param fwavcDVReceiverFrameRef The reference to the DV receiver frame.
	
	@result The DV mode of the frame.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
UInt8 FWAVCDVReceiverFrameGetDVMode(FWAVCDVReceiverFrameRef fwavcDVReceiverFrameRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDVReceiverFrameGetSYTTime
 
	@abstract Returns the SYT time for the start of a DV frame.
 
	@param fwavcDVReceiverFrameRef The reference to the DV receiver frame.
	
	@result The SYT time of the frame.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
UInt32 FWAVCDVReceiverFrameGetSYTTime(FWAVCDVReceiverFrameRef fwavcDVReceiverFrameRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDVReceiverFrameGetFireWireTimeStamp
 
	@abstract Returns the 32-bit FireWire time-stamp for the start of a DV frame.
 
	@param fwavcDVReceiverFrameRef The reference to the DV receiver frame.
	
	@result The 32-bit time-stamp for the start of the frame.
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
UInt32 FWAVCDVReceiverFrameGetFireWireTimeStamp(FWAVCDVReceiverFrameRef fwavcDVReceiverFrameRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

#pragma mark -
#pragma mark ======================================
#pragma mark Front-end APIs for the DVFramer class object
#pragma mark ======================================

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDVFramerCreate
 
	@abstract Create a DV framer.
 
	@discussion Note that the returned FWAVCDVFramerRef already has a retain count of 1. Also, note 
 that the client can specify kFWAVCDeviceStreamDefaultParameter for initialDVMode, and initialDVFrameCount, 
 as an alternative to specifying specific values. This will cause the default values to be used instead.
 
	@param framerCallback The callback function for delivering complete frames to the client, or to report errors.
	
	@param pCallbackRefCon A pointer to client supplied private data returned with the callback.
	
	@param initialDVMode The initial DV mode the DV framer is setup to receive. Will auto-adjust as needed.
	
	@param initialDVFrameCount The number of frame buffers that the DV framer should create.
	
	@param pFWAVCDVFramerRef A pointer to return the reference to the created DV framer.
 
	@param debugLogStringProc The (optional) callback for delivery of debugging log-strings to the client.
	
	@result kIOReturnSuccess if successful, specific error otherwise. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
IOReturn FWAVCDVFramerCreate(FWAVCDVFramerCallback framerCallback,
							 void *pCallbackRefCon,
							 UInt8 initialDVMode,
							 UInt32 initialDVFrameCount,
							 FWAVCDVFramerRef *pFWAVCDVFramerRef,
							 FWAVCDebugLoggingStringHandler debugLogStringProc)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDVFramerRetain
 
	@abstract Increment the retain count on a DV framer.
 
	@param fwavcDVFramerRef The reference to the DV framer.
 
	@result The reference to the DV framer. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
FWAVCDVFramerRef FWAVCDVFramerRetain(FWAVCDVFramerRef fwavcDVFramerRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDVFramerRelease
 
	@abstract Decrement the retain count on a DV framer, and delete it if the retain count drops down to 0.
 
	@param fwavcDVFramerRef The reference to the DV framer.
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
void FWAVCDVFramerRelease(FWAVCDVFramerRef fwavcDVFramerRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDVFramerReset
 
	@abstract Reset a DV framer to its initial state.
 
	@param fwavcDVFramerRef The reference to the DV framer.
	
	@result kIOReturnSuccess if successful, specific error otherwise. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
IOReturn FWAVCDVFramerReset(FWAVCDVFramerRef fwavcDVFramerRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDVFramerNextDVSourcePacket
 
	@abstract Pass in the next DV "source packet" into the DV framer.
 
	@param fwavcDVFramerRef The reference to the DV framer.
	
	@param pSourcePacket A pointer to the source-packet buffer.

	@param packetLen The length of the source-packet buffer.
 
	@param dvMode The DV mode of the source-packet.
 
	@param syt The SYT time associated with the source packet.
 
	@param packetTimeStamp The FireWire time-stamp, or any other 32-bit value associated with the source packet.
 
	@param packetU64TimeStamp The system time-stamp, or any other 64-bit value associated with the source packet. 
 
	@result kIOReturnSuccess if successful, specific error otherwise. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
IOReturn FWAVCDVFramerNextDVSourcePacket(FWAVCDVFramerRef fwavcDVFramerRef,
										 UInt8 *pSourcePacket, 
										 UInt32 packetLen, 
										 UInt8 dvMode, 
										 UInt16 syt, 
										 UInt32 packetTimeStamp,
										 UInt64 packetU64TimeStamp)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDVFramerReturnDVFrame
 
	@abstract Return a DV frame back to the DV framer for reuse.
 
	@param fwavcDVFramerFrameRef The reference to the DV framer frame.
	
	@result kIOReturnSuccess if successful, specific error otherwise. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
IOReturn FWAVCDVFramerReturnDVFrame(FWAVCDVFramerFrameRef fwavcDVFramerFrameRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDVFramerFrameGetFramer
 
	@abstract Returns the DV framer who created the specified DV frame.
 
	@param fwavcDVFramerFrameRef The reference to the DV framer frame.
	
	@result The reference to the associated DV framer.
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
FWAVCDVFramerRef FWAVCDVFramerFrameGetFramer(FWAVCDVFramerFrameRef fwavcDVFramerFrameRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDVFramerFrameGetFrameDataBuf
 
	@abstract Returns a pointer to a frame's data-buffer.
 
	@param fwavcDVFramerFrameRef The reference to the DV framer frame.
	
	@result The pointer to the frame's buffer.
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
UInt8 *FWAVCDVFramerFrameGetFrameDataBuf(FWAVCDVFramerFrameRef fwavcDVFramerFrameRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDVFramerFrameGetLen
 
	@abstract Returns the length of a frame's data-buffer.
 
	@param fwavcDVFramerFrameRef The reference to the DV framer frame.
	
	@result The length of the frame's data-buffer.
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
UInt32 FWAVCDVFramerFrameGetLen(FWAVCDVFramerFrameRef fwavcDVFramerFrameRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDVFramerFrameGetDVMode
 
	@abstract Returns the DV-mode of a frame.
 
	@param fwavcDVFramerFrameRef The reference to the DV framer frame.
	
	@result The DV mode of the frame.
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
UInt8 FWAVCDVFramerFrameGetDVMode(FWAVCDVFramerFrameRef fwavcDVFramerFrameRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDVFramerFrameGetSYTTime
 
	@abstract Returns the SYT time for the start of a DV frame.
 
	@param fwavcDVFramerFrameRef The reference to the DV framer frame.
	
	@result The SYT time of the frame.
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
UInt32 FWAVCDVFramerFrameGetSYTTime(FWAVCDVFramerFrameRef fwavcDVFramerFrameRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDVFramerFrameGetStartTimeStamp
 
	@abstract Returns the 32-bit time-stamp for the start of a DV frame.
 
	@param fwavcDVFramerFrameRef The reference to the DV framer frame.
	
	@result The 32-bit time-stamp for the start of the frame.
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
UInt32 FWAVCDVFramerFrameGetStartTimeStamp(FWAVCDVFramerFrameRef fwavcDVFramerFrameRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDVFramerFrameGetStartU64TimeStamp
 
	@abstract Returns the 64-bit time-stamp for the start of a DV frame.
 
	@param fwavcDVFramerFrameRef The reference to the DV framer frame.
	
	@result The 64-bit time-stamp for the start of the frame.
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
UInt64 FWAVCDVFramerFrameGetStartU64TimeStamp(FWAVCDVFramerFrameRef fwavcDVFramerFrameRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDVFramerFrameSetClientPrivateData
 
	@abstract Allows the client to attach some private data to a DV frame.
 
	@param fwavcDVFramerFrameRef The reference to the DV framer frame.

	@param pClientPrivateData A pointer to the client's private data.
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
void FWAVCDVFramerFrameSetClientPrivateData(FWAVCDVFramerFrameRef fwavcDVFramerFrameRef, void *pClientPrivateData)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCDVFramerFrameGetClientPrivateData
 
	@abstract Allows the client to fetch the client's private data previously attached to a DV frame.
 
	@param fwavcDVFramerFrameRef The reference to the DV framer frame.
 
	@result Returns the pointer to the client's private data. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
void *FWAVCDVFramerFrameGetClientPrivateData(FWAVCDVFramerFrameRef fwavcDVFramerFrameRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCGetDVModeFromFrameData
 
	@abstract Parses the first 480 bytes of a DV frame buffer, and determines its dvMode.
 
	@param pDVFrameData A pointer to at least the first 480 bytes of the frame buffer
	
	@param pDVMode A pointer to return the dvMode.
	
	@param pFrameSize A pointer to return the frame-size
	
	@param pSourcePacketSize A pointer to return the source-packet-size (for feeding into the DVFramer).
	
	@result kIOReturnSuccess if successful, specific error otherwise. 
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
IOReturn FWAVCGetDVModeFromFrameData(UInt8 *pDVFrameData, UInt8 *pDVMode, UInt32 *pFrameSize, UInt32 *pSourcePacketSize)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

#pragma mark -
#pragma mark ======================================
#pragma mark Front-end APIs for the TSDemuxer class object
#pragma mark ======================================

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCTSDemuxerCreate
 
	@abstract Create a TS demuxer using the auto-PSI method of PID detection.
 
	@discussion Note that the returned FWAVCTSDemuxerRef already has a retain count of 1. Also, note 
 that the client can specify kFWAVCDeviceStreamDefaultParameter for maxVideoPESSize, maxAudioPESSize, 
 initialVideoPESBufferCount and initialAudioPESBufferCount as an alternative to specifying specific values. 
 This will cause the default values to be used instead.
 
	@param pesCallback The callback function for delivering complete PES packets to the client, or to report errors.
	
	@param pCallbackRefCon A pointer to client supplied private data returned with the callback.
 
	@param selectedProgram The program number the demuxer should demux. NOTE: Here a 1 means the first program in the PAT, 2 the second program, etc
 
	@param maxVideoPESSize The maximum size of a video PES buffer.
 
	@param maxAudioPESSize The maximum size of an audio PES buffer.
 
	@param initialVideoPESBufferCount The number of video buffers to create.
 
	@param initialAudioPESBufferCount The number of audio buffers to create.
 
	@param pFWAVCTSDemuxerRef A pointer to return the reference to the created TS demuxer.
 
	@param debugLogStringProc The (optional) callback for delivery of debugging log-strings to the client.
 
	@result kIOReturnSuccess if successful, specific error otherwise. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
IOReturn FWAVCTSDemuxerCreate(FWAVCTSDemuxerCallback pesCallback,
							  void *pCallbackRefCon,
							  UInt32 selectedProgram,
							  UInt32 maxVideoPESSize,
							  UInt32 maxAudioPESSize,
							  UInt32 initialVideoPESBufferCount,
							  UInt32 initialAudioPESBufferCount,
							  FWAVCTSDemuxerRef *pFWAVCTSDemuxerRef,
							  FWAVCDebugLoggingStringHandler debugLogStringProc)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCTSDemuxerCreateWithPIDs
 
	@abstract Create a TS demuxer with hard-coded PIDs for video and audio.
 
	@discussion Note that the returned FWAVCTSDemuxerRef already has a retain count of 1. Also, note 
 that the client can specify kFWAVCDeviceStreamDefaultParameter for maxVideoPESSize, maxAudioPESSize, 
 initialVideoPESBufferCount and initialAudioPESBufferCount as an alternative to specifying specific values. 
 This will cause the default values to be used instead.
 
	@param pesCallback The callback function for delivering complete PES packets to the client, or to report errors.
	
	@param pCallbackRefCon A pointer to client supplied private data returned with the callback.
 
	@param videoPid The The PID of the video packets to demux

	@param audioPid The The PID of the audio packets to demux

	@param maxVideoPESSize The maximum size of a video PES buffer.
 
	@param maxAudioPESSize The maximum size of an audio PES buffer.
 
	@param initialVideoPESBufferCount The number of video buffers to create.
 
	@param initialAudioPESBufferCount The number of audio buffers to create.
 
	@param pFWAVCTSDemuxerRef A pointer to return the reference to the created TS demuxer.
 
	@param debugLogStringProc The (optional) callback for delivery of debugging log-strings to the client.
 
	@result kIOReturnSuccess if successful, specific error otherwise. 
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
	IOReturn FWAVCTSDemuxerCreateWithPIDs(FWAVCTSDemuxerCallback pesCallback,
										  void *pCallbackRefCon,
										  UInt32 videoPid,
										  UInt32 audioPid,
										  UInt32 maxVideoPESSize,
										  UInt32 maxAudioPESSize,
										  UInt32 initialVideoPESBufferCount,
										  UInt32 initialAudioPESBufferCount,
										  FWAVCTSDemuxerRef *pFWAVCTSDemuxerRef,
										  FWAVCDebugLoggingStringHandler debugLogStringProc)
	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCTSDemuxerRetain
 
	@abstract Increment the retain count on a TS demuxer.
 
	@param fwavcTSDemuxerRef The reference to the TS demuxer.
 
	@result The reference to the TS demuxer. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
FWAVCTSDemuxerRef FWAVCTSDemuxerRetain(FWAVCTSDemuxerRef fwavcTSDemuxerRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCTSDemuxerRelease
	 
	 @abstract Decrement the retain count on a TS demuxer, and delete it if the retain count drops down to 0.
	 
	 @param fwavcTSDemuxerRef The reference to the TS demuxer.
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
void FWAVCTSDemuxerRelease(FWAVCTSDemuxerRef fwavcTSDemuxerRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCTSDemuxerNextTSPacket
 
	@abstract Pass in the next MPEG-2 transport-stream packet into the TS demuxer.
 
	@param fwavcTSDemuxerRef The reference to the TS demuxer.
	
	@param pPacket A pointer to the transport-stream packet.

	@param packetTimeStamp The FireWire time-stamp, or any other 32-bit value associated with the TS packet.
 
	@param packetU64TimeStamp The system time-stamp, or any other 64-bit value associated with the TS packet. 
 
	@result kIOReturnSuccess if successful, specific error otherwise. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
IOReturn FWAVCTSDemuxerNextTSPacket(FWAVCTSDemuxerRef fwavcTSDemuxerRef, 
									UInt8 *pPacket, 
									UInt32 packetTimeStamp, 
									UInt64 packetU64TimeStamp)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCTSDemuxerReset
 
	@abstract Reset a TS demuxer to its initial state.
 
	@param fwavcTSDemuxerRef The reference to the TS demuxer.
	
	@result kIOReturnSuccess if successful, specific error otherwise. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
IOReturn FWAVCTSDemuxerReset(FWAVCTSDemuxerRef fwavcTSDemuxerRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCTSDemuxerFlush
 
	@abstract Flush out any in-process PES packets..
 
	@param fwavcTSDemuxerRef The reference to the TS demuxer.
	
	@result kIOReturnSuccess if successful, specific error otherwise. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
void FWAVCTSDemuxerFlush(FWAVCTSDemuxerRef fwavcTSDemuxerRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCTSDemuxerReturnPESPacket
 
	@abstract Return a PES packet back to the TS demuxer for reuse.
 
	@param fwavcTSDemuxerPesPacketRef The reference to the TS demuxer PES packet.
	
	@result kIOReturnSuccess if successful, specific error otherwise. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
IOReturn FWAVCTSDemuxerReturnPESPacket(FWAVCTSDemuxerPESPacketRef fwavcTSDemuxerPesPacketRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCTSDemuxerSetHDV2VAuxCallback
 
	@abstract Register a callback for HDV2 V-Aux packet notification.
 
	@param fwavcTSDemuxerRef The reference to the TS demuxer.
	
	@param fVAuxCallback The callback function for notifying the client that an HDV2 VAux packet has been received and parsed.
 
 	@param pRefCon A pointer to client supplied private data returned with the callback.
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
void FWAVCTSDemuxerSetHDV2VAuxCallback(FWAVCTSDemuxerRef fwavcTSDemuxerRef,
									   FWAVCTSDemuxerHDV2VAUXCallback fVAuxCallback, 
									   void *pRefCon)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCTSDemuxerGetPMTInfo
 
	@abstract Copy the primary program's PMT descriptors (if any) into the designated buffers. This function should be called either during a FWAVCTSDemuxerCallback
 callback, or when NO other thread is currently processing a FWAVCTSDemuxerNextTSPacket(...) call for this demuxer instance.
 
	@param fwavcTSDemuxerRef The reference to the TS demuxer.

	@param pPrimaryProgramPMTPid If not NULL, will return the PID of the primary program's PMT table.
 
 	@param pPrimaryProgramPMTVersion If not NULL, will return the version number of the primary program's PMT table
 
	@param pProgramVideoPid If not NULL, will returns the PID of the program's main video elementary stream.
 	
	@param pProgramVideoStreamType If not NULL, will returns the stream-type of the program's main video elementary stream.
 
 	@param pProgramAudioPid  If not NULL, will returns the PID of the program's main audio elementary stream.
 
	@param pProgramAudioStreamType  If not NULL, will returns the stream-type of the program's main audio elementary stream.
 
	@param pProgramDescriptors A buffer to store the PMT program descriptors, or NULL to skip retrieval of the PMT program descriptors.
 
 	@param pProgramDescriptorsLen On input, the size of the pProgramDescriptors buffer. On output the total number of PMT program descriptor bytes returned.
 
	@param pVideoESDescriptors A buffer to store the PMT Video ES descriptors, or NULL to skip retrieval of the PMT Video ES descriptors.
 	
	@param pVideoESDescriptorsLen On input, the size of the pVideoESDescriptors buffer. On output the total number of PMT Video ES descriptor bytes returned.
 	
	@param pAudioESDescriptors A buffer to store the PMT Audio ES descriptors, or NULL to skip retrieval of the PMT Audio ES descriptors.
 	
	@param pAudioESDescriptorsLen On input, the size of the pAudioESDescriptors buffer. On output the total number of PMT Audio ES descriptor bytes returned.
 
	@result kIOReturnSuccess if successful, specific error otherwise. Will return kIOReturnNoMemory if any of the specified (non-NULL) buffers is too small to hold all the 
 existing descriptor bytes of that type. Will return kIOReturnNoResources if no PMT for the primary program has been found. 
 */
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
IOReturn FWAVCTSDemuxerGetPMTInfo(FWAVCTSDemuxerRef fwavcTSDemuxerRef,
								  UInt16 *pPrimaryProgramPMTPid,
								  UInt8 *pPrimaryProgramPMTVersion,
								  UInt16 *pProgramVideoPid,
								  UInt8 *pProgramVideoStreamType,
								  UInt16 *pProgramAudioPid,
								  UInt8 *pProgramAudioStreamType,
								  UInt8 *pProgramDescriptors,
								  UInt32 *pProgramDescriptorsLen,
								  UInt8 *pVideoESDescriptors,
								  UInt32 *pVideoESDescriptorsLen,
								  UInt8 *pAudioESDescriptors,
								  UInt32 *pAudioESDescriptorsLen)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCTSDemuxerSetHDV1PackDataCallback
 
	@abstract Register a callback for HDV1 PackData packet notification.
 
	@param fwavcTSDemuxerRef The reference to the TS demuxer.
	
	@param fVAuxCallback The callback function for notifying the client that an HDV1 PackData packet has been received and parsed.
 
	@param pRefCon A pointer to client supplied private data returned with the callback.
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
void FWAVCTSDemuxerSetHDV1PackDataCallback(FWAVCTSDemuxerRef fwavcTSDemuxerRef,
										   FWAVCTSDemuxerHDV1PackDataCallback fPackDataCallback, 
										   void *pRefCon)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCTSDemuxerPESPacketGetDemuxer
 
	@abstract Returns the TS demuxer who created the specified PES packet.
 
	@param fwavcTSDemuxerPesPacketRef The reference to the TS demuxer PES packet.
	
	@result The reference to the associated TS demuxer.
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
FWAVCTSDemuxerRef FWAVCTSDemuxerPESPacketGetDemuxer(FWAVCTSDemuxerPESPacketRef fwavcTSDemuxerPESPacketRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCTSDemuxerPESPacketGetStreamType
 
	@abstract Returns the stream-type (audio or video) of the specified PES packet.
 
	@param fwavcTSDemuxerPesPacketRef The reference to the TS demuxer PES packet.
	
	@result The stream-type of the PES packet.
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
FWAVCTSDemuxerPESPacketStreamType FWAVCTSDemuxerPESPacketGetStreamType(FWAVCTSDemuxerPESPacketRef fwavcTSDemuxerPESPacketRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCTSDemuxerPESPacketGetPESBuf
 
	@abstract Returns a pointer to a PES packet's data-buffer.
 
	@param fwavcTSDemuxerPesPacketRef The reference to the TS demuxer PES packet.
	
	@result A pointer to the PES data-buffer.
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
UInt8 *FWAVCTSDemuxerPESPacketGetPESBuf(FWAVCTSDemuxerPESPacketRef fwavcTSDemuxerPESPacketRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCTSDemuxerPESPacketGetLen
 
	@abstract Returns the length of a PES packet's data-buffer.
 
	@param fwavcTSDemuxerPesPacketRef The reference to the TS demuxer PES packet.
	
	@result The length of the PES packet.
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
UInt32 FWAVCTSDemuxerPESPacketGetLen(FWAVCTSDemuxerPESPacketRef fwavcTSDemuxerPESPacketRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCTSDemuxerPESPacketGetPid
 
	@abstract Returns the PID of a PES packet.
 
	@param fwavcTSDemuxerPesPacketRef The reference to the TS demuxer PES packet.
	
	@result The PID of the PES packet.
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
UInt32 FWAVCTSDemuxerPESPacketGetPid(FWAVCTSDemuxerPESPacketRef fwavcTSDemuxerPESPacketRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCTSDemuxerPESPacketGetStartTimeStamp
 
	@abstract Returns the 32-bit time-stamp for the start of a PES packet.
 
	@param fwavcTSDemuxerPesPacketRef The reference to the TS demuxer PES packet.
	
	@result The 32-bit time-stamp associated with the start of the PES packet.
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
UInt32 FWAVCTSDemuxerPESPacketGetStartTimeStamp(FWAVCTSDemuxerPESPacketRef fwavcTSDemuxerPESPacketRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCTSDemuxerPESPacketGetStartU64TimeStamp
 
	@abstract Returns the 64-bit time-stamp for the start of a PES packet.
 
	@param fwavcTSDemuxerPesPacketRef The reference to the TS demuxer PES packet.
	
	@result The 64-bit time-stamp associated with the start of the PES packet.
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
UInt64 FWAVCTSDemuxerPESPacketGetStartU64TimeStamp(FWAVCTSDemuxerPESPacketRef fwavcTSDemuxerPESPacketRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCTSDemuxerPESPacketSetClientPrivateData
 
	@abstract Allows the client to attach some private data to a PES packet.
 
	@param fwavcTSDemuxerPesPacketRef The reference to the TS demuxer PES packet.
 
	@param pClientPrivateData A pointer to the client's private data.
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
void FWAVCTSDemuxerPESPacketSetClientPrivateData(FWAVCTSDemuxerPESPacketRef fwavcTSDemuxerPESPacketRef, void *pClientPrivateData)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*!
	@function FWAVCTSDemuxerPESPacketGetClientPrivateData
 
	@abstract Allows the client to fetch the client's private data previously attached to a PES packet.
 
	@param fwavcTSDemuxerPesPacketRef The reference to the TS demuxer PES packet.
 
	@result Returns the pointer to the client's private data. 
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern
void *FWAVCTSDemuxerPESPacketGetClientPrivateData(FWAVCTSDemuxerPESPacketRef fwavcTSDemuxerPESPacketRef)
																	AVAILABLE_MAC_OS_X_VERSION_10_4_AND_LATER;


#ifdef __cplusplus
} // extern "C"
#endif	

#endif // __AVCVIDEOSERVICES_FWAVC__
