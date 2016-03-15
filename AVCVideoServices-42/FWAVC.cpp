/*
	File:   FWAVC.c
 
 Synopsis: This is the source for the C front-end to AVCVideoServices 
 
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

extern "C"
{
#include "FWAVC.h"
}

#include "AVCVideoServices.h"
using namespace AVS;

#pragma mark -
#pragma mark ======================================
#pragma mark Private structs
#pragma mark ======================================

struct FWAVCDeviceController
{
	pthread_mutex_t retainReleaseMutex;
	UInt32 retainCount;
	AVCDeviceController *pAVCDeviceController;
	FWAVCDeviceControllerNotification clientNotificationProc;
	void *pRefCon;
	FWAVCDeviceMessageNotification globalAVCDeviceMessageProc;
	CFMutableArrayRef fwavcDeviceArray;
};

struct FWAVCDevice
{
	AVCDevice *pAVCDevice;
	FWAVCDeviceMessageNotification deviceMessageProc;
	void *pMessageProcRefCon;
};

struct FWAVCDeviceStream
{
	pthread_mutex_t retainReleaseMutex;
	UInt32 retainCount;
	AVCDeviceStream *pAVCDeviceStream;
	FWAVCDevice *pFWAVCDevice;
	FWAVCUniversalReceiverDataReceivedProc universalReceiverDataReceivedProcHandler;
	FWAVCMPEG2ReceiverDataReceivedProc mpeg2ReceiverDataReceivedProcHandler;
	FWAVCMPEG2TransmitterDataRequestProc mpeg2TransmitterDataRequestProcHandler;
	FWAVCDVReceiverFrameReceivedProc dvRececiverFrameReceivedProc;
	void *pDataProcRefCon;
	CFMutableArrayRef dvTransmitterFrameRefArray;
	FWAVCDVTransmitterFrameRequestProc dvTransmitterframeRequestProcHandler;
	void *pDVTransmitterFrameRequestProcRefCon;
	FWAVCDVTransmitterFrameReturnProc dvTransmitterframeReturnProcHandler;
	void *pDVTransmitterFrameReleaseProcRefCon;
	FWAVCDeviceStreamMessageProc messageProcHandler;
	void *pMessageProcRefCon;
	FWAVCIsochReceiverNoDataProc noDataHandler;
	void *pNoDataHandlerRefCon;
	FWAVCUniversalReceiverStructuredDataReceivedProc universalReceiverStructuredDataHandler;
	FWAVCMPEG2ReceiverStructuredDataReceivedProc mpeg2ReceiverStructuredDataHandler;
	void *pStructuredDataHandlerRefCon;
	FWAVCMPEG2TransmitterTimeStampProc mpeg2TransmitterTimeStampProcHandler; 
	void *pMPEG2TransmitterTimeStampProcHandlerRefCon;
	StringLogger *pStringLogger;
};

struct FWAVCDVFramer
{
	pthread_mutex_t retainReleaseMutex;
	UInt32 retainCount;
	DVFramer *pDVFramer;
	FWAVCDVFramerCallback framerCallback;
	void *pCallbackRefCon;
	StringLogger *pStringLogger;
};

struct FWAVCTSDemuxer
{
	pthread_mutex_t retainReleaseMutex;
	UInt32 retainCount;
	TSDemuxer *pTSDemuxer;
	FWAVCTSDemuxerCallback pesCallback;
	void *pCallbackRefCon;
	FWAVCTSDemuxerHDV2VAUXCallback fVAuxCallback;
	void *pVAuxRefCon;
	FWAVCTSDemuxerHDV1PackDataCallback fPackDataCallback; 
	void *pPackDataRefCon;
	StringLogger *pStringLogger;
	Boolean autoPSI;
	UInt32 videoPid;
	UInt32 audioPid;
};

#pragma mark -
#pragma mark ======================================
#pragma mark Static functions used internally
#pragma mark ======================================

//////////////////////////////////////////////////////////
// MyAVCDeviceControllerNotification
//////////////////////////////////////////////////////////
static FWAVCDevice *FindFWAVCDeviceForAVCDevice(FWAVCDeviceController *pFWAVCDeviceController, AVCDevice* pAVCDevice)
{
	// Itertate through the array to see if this AVCDevice has a corresponding FWAVCDevice
	UInt32 i;
	FWAVCDevice *pFWAVCDevice;
	
	for (i=0;i<(UInt32)CFArrayGetCount(pFWAVCDeviceController->fwavcDeviceArray);i++)
	{
		pFWAVCDevice = (FWAVCDevice*) CFArrayGetValueAtIndex(pFWAVCDeviceController->fwavcDeviceArray,i);
		
		if (pFWAVCDevice->pAVCDevice == pAVCDevice)
			return pFWAVCDevice;
	}
	
	return nil;
}

//////////////////////////////////////////////////////////
// MyAVCDeviceControllerNotification
//////////////////////////////////////////////////////////
static IOReturn MyAVCDeviceControllerNotification(AVCDeviceController *pAVCDeviceController, void *pRefCon, AVCDevice* pAVCDevice)
{
	FWAVCDeviceController *pFWAVCDeviceController = (FWAVCDeviceController*) pRefCon;
	FWAVCDevice *pFWAVCDevice;
		
	// See if this AVCDevice already has an associated FWAVCDevice. If not, create one
	pFWAVCDevice = FindFWAVCDeviceForAVCDevice(pFWAVCDeviceController,pAVCDevice);
	if (!pFWAVCDevice)
	{
		// Create a FWAVCDevice and add it to the array
		pFWAVCDevice = new FWAVCDevice;
		if (pFWAVCDevice)
		{
			pFWAVCDevice->pAVCDevice = pAVCDevice;
			CFArrayAppendValue(pFWAVCDeviceController->fwavcDeviceArray,pFWAVCDevice);
		}
	}

	// If we found (or successfully created) the device, and we have a non-nil callback, do the callback
	if ((pFWAVCDevice) && (pFWAVCDeviceController->clientNotificationProc))
		return pFWAVCDeviceController->clientNotificationProc (pFWAVCDeviceController, pFWAVCDeviceController->pRefCon, pFWAVCDevice);
	else
		return kIOReturnSuccess;
}

//////////////////////////////////////////////////////////
// MyGlobalAVCDeviceMessageNotification
//////////////////////////////////////////////////////////
static IOReturn MyGlobalAVCDeviceMessageNotification(AVCDevice *pAVCDevice, natural_t messageType, void * messageArgument, void *pRefCon)
{
	FWAVCDeviceController *pFWAVCDeviceController = (FWAVCDeviceController*) pRefCon;
	FWAVCDevice *pFWAVCDevice;
	
	// See if this AVCDevice already has an associated FWAVCDevice.
	pFWAVCDevice = FindFWAVCDeviceForAVCDevice(pFWAVCDeviceController,pAVCDevice);

	// If we found the device, and we have a non-nil callback, do the callback
	if ((pFWAVCDevice) && (pFWAVCDeviceController->globalAVCDeviceMessageProc))
		return pFWAVCDeviceController->globalAVCDeviceMessageProc(pFWAVCDevice,  messageType, messageArgument, pFWAVCDeviceController->pRefCon);
	else
		return kIOReturnSuccess;
}

//////////////////////////////////////////////////////////
// MyAVCDeviceMessageNotification
//////////////////////////////////////////////////////////
static IOReturn MyAVCDeviceMessageNotification(AVCDevice *pAVCDevice, natural_t messageType, void * messageArgument, void *pRefCon)
{
	FWAVCDevice *pFWAVCDevice = (FWAVCDevice*) pRefCon;
	
	if (pFWAVCDevice->deviceMessageProc)
		return pFWAVCDevice->deviceMessageProc(pFWAVCDevice,  messageType, messageArgument, pFWAVCDevice->pMessageProcRefCon);
	else
		return kIOReturnSuccess;
}


////////////////////////////////////////////////////
// DeviceControllerArrayReleaseObjectCallback
////////////////////////////////////////////////////
static void DeviceControllerArrayReleaseObjectCallback(CFAllocatorRef allocator,const void *ptr)
{
	delete (FWAVCDevice*) ptr;
	return;
}

////////////////////////////////////////////////////
// MyDVFramerCallback
////////////////////////////////////////////////////
static IOReturn MyDVFramerCallback(DVFramerCallbackMessage msg, DVFrame* pDVFrame, void *pRefCon, DVFramer *pDVFramer)
{
	IOReturn result = kIOReturnSuccess;
	FWAVCDVFramer *pFWAVCDVFramer = (FWAVCDVFramer*) pRefCon;
	
	if (pFWAVCDVFramer->framerCallback)
	{
		// If we have a non-nil frame here, we retain the FWAVCDVFramer until the frame is released back to the framer.
		if (pDVFrame)
		{
			pDVFrame->pFWAVCPrivateData = pFWAVCDVFramer;
			FWAVCDVFramerRetain(pFWAVCDVFramer);
		}

		result = pFWAVCDVFramer->framerCallback(msg, (FWAVCDVFramerFrameRef) pDVFrame, pFWAVCDVFramer->pCallbackRefCon, pFWAVCDVFramer);
	}
	else if (pDVFrame)
		pDVFrame->pDVFramer->ReleaseDVFrame(pDVFrame);
	
	return result;
}

////////////////////////////////////////////////////
// MyTSDemuxerCallback
////////////////////////////////////////////////////
static IOReturn MyTSDemuxerCallback(TSDemuxerMessage msg, PESPacketBuf* pPESBuf, void *pRefCon)
{
	IOReturn result = kIOReturnSuccess;
	FWAVCTSDemuxer *pFWAVCTSDemuxer = (FWAVCTSDemuxer*) pRefCon;

	if (pFWAVCTSDemuxer->pesCallback)
	{
		// We retain the FWAVCTSDemuxer until the PES packet is released back to the demuxer
		pPESBuf->pFWAVCPrivateData = pFWAVCTSDemuxer;
		FWAVCTSDemuxerRetain(pFWAVCTSDemuxer);
		
		result = pFWAVCTSDemuxer->pesCallback(msg, 
											  (FWAVCTSDemuxerPESPacketRef) pPESBuf, 
											  pFWAVCTSDemuxer->pCallbackRefCon,
											  pFWAVCTSDemuxer);
	}
	else
		pFWAVCTSDemuxer->pTSDemuxer->ReleasePESPacketBuf(pPESBuf);
	
	return result;
}

////////////////////////////////////////////////////
// MyHDV2VAUXCallback
////////////////////////////////////////////////////
static void MyHDV2VAUXCallback(HDV2VideoFramePack *pVAux, void *pRefCon)
{
	FWAVCTSDemuxer *pFWAVCTSDemuxer = (FWAVCTSDemuxer*) pRefCon;
	if (pFWAVCTSDemuxer->fVAuxCallback)
		pFWAVCTSDemuxer->fVAuxCallback(pVAux, 
									   pFWAVCTSDemuxer->pVAuxRefCon,
									   pFWAVCTSDemuxer);
}

////////////////////////////////////////////////////
// MyHDV1PackDataCallback
////////////////////////////////////////////////////
static void MyHDV1PackDataCallback(HDV1PackData *pPack, void *pRefCon)
{
	FWAVCTSDemuxer *pFWAVCTSDemuxer = (FWAVCTSDemuxer*) pRefCon;
	if (pFWAVCTSDemuxer->fPackDataCallback)
		pFWAVCTSDemuxer->fPackDataCallback(pPack, 
										   pFWAVCTSDemuxer->pPackDataRefCon,
										   pFWAVCTSDemuxer);
}

////////////////////////////////////////////////////
// MyUniversalReceiverDataPushProc
////////////////////////////////////////////////////
static IOReturn MyUniversalReceiverDataPushProc(void *pRefCon, UInt32 payloadLength, UInt8 *pPayload, UInt32 isochHeader, UInt32 fireWireTimeStamp)
{
	FWAVCDeviceStream *pFWAVCDeviceStream = (FWAVCDeviceStream*) pRefCon;
	if (pFWAVCDeviceStream->universalReceiverDataReceivedProcHandler)
		return pFWAVCDeviceStream->universalReceiverDataReceivedProcHandler(pFWAVCDeviceStream, 
																			pFWAVCDeviceStream->pFWAVCDevice,
																			pFWAVCDeviceStream->pDataProcRefCon, 
																			payloadLength, 
																			pPayload, 
																			isochHeader, 
																			fireWireTimeStamp);
	else
		return kIOReturnSuccess;
}

////////////////////////////////////////////////////
// MyDeviceStreamMessageProc
////////////////////////////////////////////////////
static void MyDeviceStreamMessageProc(UInt32 msg, UInt32 param1, UInt32 param2, void *pRefCon)
{
	FWAVCDeviceStream *pFWAVCDeviceStream = (FWAVCDeviceStream*) pRefCon;
	if (pFWAVCDeviceStream->messageProcHandler)
		pFWAVCDeviceStream->messageProcHandler(pFWAVCDeviceStream, 
											   pFWAVCDeviceStream->pFWAVCDevice,
											   msg, 
											   param1, 
											   param2, 
											   pFWAVCDeviceStream->pMessageProcRefCon);
}

////////////////////////////////////////////////////
// MyFWAVCIsochReceiverNoDataProc
////////////////////////////////////////////////////
static IOReturn MyFWAVCIsochReceiverNoDataProc(void *pRefCon)
{
	FWAVCDeviceStream *pFWAVCDeviceStream = (FWAVCDeviceStream*) pRefCon;
	if (pFWAVCDeviceStream->noDataHandler)
		return pFWAVCDeviceStream->noDataHandler(pFWAVCDeviceStream, 
												 pFWAVCDeviceStream->pFWAVCDevice, 
												 pFWAVCDeviceStream->pNoDataHandlerRefCon);
	else
		return kIOReturnSuccess;
}

////////////////////////////////////////////////////
// MyStructuredUniversalReceiverDataPushProc
////////////////////////////////////////////////////
static IOReturn MyStructuredUniversalReceiverDataPushProc(UInt32 CycleDataCount, UniversalReceiveCycleData *pCycleData, void *pRefCon)
{
	FWAVCDeviceStream *pFWAVCDeviceStream = (FWAVCDeviceStream*) pRefCon;
	if (pFWAVCDeviceStream->universalReceiverStructuredDataHandler)
		return pFWAVCDeviceStream->universalReceiverStructuredDataHandler(pFWAVCDeviceStream, 
																		  pFWAVCDeviceStream->pFWAVCDevice,
																		  CycleDataCount, 
																		  pCycleData, 
																		  pFWAVCDeviceStream->pStructuredDataHandlerRefCon);
	else
		return kIOReturnSuccess;
}

//////////////////////////////////////////////////////////
// MyMPEG2TransmitterTimeStampProc
//////////////////////////////////////////////////////////
static void MyMPEG2TransmitterTimeStampProc(UInt64 pcr, UInt64 transmitTimeInNanoSeconds, void *pRefCon)
{
	FWAVCDeviceStream *pFWAVCDeviceStream = (FWAVCDeviceStream*) pRefCon;
	if (pFWAVCDeviceStream->mpeg2TransmitterTimeStampProcHandler)
		pFWAVCDeviceStream->mpeg2TransmitterTimeStampProcHandler(pFWAVCDeviceStream, 
																 pFWAVCDeviceStream->pFWAVCDevice,
																 pcr, 
																 transmitTimeInNanoSeconds, 
																 pFWAVCDeviceStream->pMPEG2TransmitterTimeStampProcHandlerRefCon);
}

//////////////////////////////////////////////////////////
// MyMPEG2ReceiverExtendedDataPushProc
//////////////////////////////////////////////////////////
static IOReturn MyMPEG2ReceiverExtendedDataPushProc(UInt32 tsPacketCount, 
													UInt32 **ppBuf, 
													void *pRefCon, 
													UInt32 isochHeader,
													UInt32 cipHeader0,
													UInt32 cipHeader1,
													UInt32 fireWireTimeStamp)
{
	FWAVCDeviceStream *pFWAVCDeviceStream = (FWAVCDeviceStream*) pRefCon;
	if (pFWAVCDeviceStream->mpeg2ReceiverDataReceivedProcHandler)
		return pFWAVCDeviceStream->mpeg2ReceiverDataReceivedProcHandler(pFWAVCDeviceStream, 
																		pFWAVCDeviceStream->pFWAVCDevice,
																		tsPacketCount, 
																		ppBuf, 
																		pFWAVCDeviceStream->pDataProcRefCon, 
																		isochHeader,
																		cipHeader0,
																		cipHeader1,
																		fireWireTimeStamp);
	else
		return kIOReturnSuccess;
}

//////////////////////////////////////////////////////////
// MyStructuredMPEG2ReceiverDataPushProc
//////////////////////////////////////////////////////////
static IOReturn MyStructuredMPEG2ReceiverDataPushProc(UInt32 CycleDataCount, MPEGReceiveCycleData *pCycleData, void *pRefCon)
{
	FWAVCDeviceStream *pFWAVCDeviceStream = (FWAVCDeviceStream*) pRefCon;
	if (pFWAVCDeviceStream->mpeg2ReceiverStructuredDataHandler)
		return pFWAVCDeviceStream->mpeg2ReceiverStructuredDataHandler(pFWAVCDeviceStream, 
																	  pFWAVCDeviceStream->pFWAVCDevice,
																	  CycleDataCount, 
																	  pCycleData, 
																	  pFWAVCDeviceStream->pStructuredDataHandlerRefCon);
	else
		return kIOReturnSuccess;
}

//////////////////////////////////////////////////////////
// MyMPEG2TransmitterDataPullProc
//////////////////////////////////////////////////////////
static IOReturn MyMPEG2TransmitterDataPullProc(UInt32 **ppBuf, bool *pDiscontinuityFlag, void *pRefCon)
{
	FWAVCDeviceStream *pFWAVCDeviceStream = (FWAVCDeviceStream*) pRefCon;
	if (pFWAVCDeviceStream->mpeg2TransmitterDataRequestProcHandler)
		return pFWAVCDeviceStream->mpeg2TransmitterDataRequestProcHandler(pFWAVCDeviceStream, 
																	  pFWAVCDeviceStream->pFWAVCDevice,
																	  ppBuf, 
																	  pDiscontinuityFlag, 
																	  pFWAVCDeviceStream->pDataProcRefCon);
	else
		return kIOReturnSuccess;
}

//////////////////////////////////////////////////////////
// MyDVFramePullProc
//////////////////////////////////////////////////////////
static IOReturn MyDVFramePullProc(UInt32 *pFrameIndex, void *pRefCon)
{
	IOReturn result = kIOReturnError;
	FWAVCDeviceStream *pFWAVCDeviceStream = (FWAVCDeviceStream*) pRefCon;
	FWAVCDVTransmitterFrameRef requestedFrame;
	DVTransmitFrame *pDVTransmitFrame;

	if (pFWAVCDeviceStream->dvTransmitterframeRequestProcHandler)
	{
		result = pFWAVCDeviceStream->dvTransmitterframeRequestProcHandler(pFWAVCDeviceStream,
																		  pFWAVCDeviceStream->pFWAVCDevice,
																		  &requestedFrame,
																		  pFWAVCDeviceStream->pDVTransmitterFrameRequestProcRefCon);
		if (result == kIOReturnSuccess)
		{
			pDVTransmitFrame = (DVTransmitFrame*) requestedFrame;
			*pFrameIndex = pDVTransmitFrame->frameIndex;
		}
	}
	
	return result;
}

//////////////////////////////////////////////////////////
// MyDVFrameReleaseProc
//////////////////////////////////////////////////////////
static IOReturn MyDVFrameReleaseProc(UInt32 frameIndex, void *pRefCon)
{
	IOReturn result = kIOReturnSuccess;
	FWAVCDeviceStream *pFWAVCDeviceStream = (FWAVCDeviceStream*) pRefCon;
	DVTransmitFrame *pDVTransmitFrame = pFWAVCDeviceStream->pAVCDeviceStream->pDVTransmitter->getFrame(frameIndex);

	if (pFWAVCDeviceStream->dvTransmitterframeReturnProcHandler)
		result = pFWAVCDeviceStream->dvTransmitterframeReturnProcHandler(pFWAVCDeviceStream,
																		  pFWAVCDeviceStream->pFWAVCDevice,
																		  (FWAVCDVTransmitterFrameRef) pDVTransmitFrame,
																		  pFWAVCDeviceStream->pDVTransmitterFrameReleaseProcRefCon);
	return result;
}

//////////////////////////////////////////////////////////
// MyDVFrameReceivedProc
//////////////////////////////////////////////////////////
static IOReturn MyDVFrameReceivedProc(DVFrameReceiveMessage msg, DVReceiveFrame* pFrame, void *pRefCon)
{
	IOReturn result = kIOReturnError;
	FWAVCDeviceStream *pFWAVCDeviceStream = (FWAVCDeviceStream*) pRefCon;

	// If we have a valid frame here, add an extra retain the device stream object
	if (pFrame)
	{
		FWAVCDeviceStreamRetain(pFWAVCDeviceStream);
		pFrame->pFWAVCPrivateData = pFWAVCDeviceStream;
	}
	if (pFWAVCDeviceStream->dvRececiverFrameReceivedProc)
		result = pFWAVCDeviceStream->dvRececiverFrameReceivedProc(pFWAVCDeviceStream,
																  pFWAVCDeviceStream->pFWAVCDevice,
																  msg,
																  (FWAVCDVReceiverFrameRef) pFrame, 
																  pFWAVCDeviceStream->pDataProcRefCon);

	// If we have have an error here, and a valid frame, remove the extra retain from the device stream object
	if ((result != kIOReturnSuccess) && (pFrame))
		FWAVCDeviceStreamRelease(pFWAVCDeviceStream);
	
	return result;
}


#pragma mark -
#pragma mark ======================================
#pragma mark Front-end APIs for the AVCDeviceController class object
#pragma mark ======================================

//////////////////////////////////////////////////////////
// FWAVCDeviceControllerCreate
//////////////////////////////////////////////////////////
IOReturn FWAVCDeviceControllerCreate(FWAVCDeviceControllerRef *pFWAVCDeviceControllerRef,
									 FWAVCDeviceControllerNotification clientNotificationProc,
									 void *pRefCon,
									 FWAVCDeviceMessageNotification globalAVCDeviceMessageProc,
									 FWAVCDebugLoggingStringHandler debugLogStringProc)
{
	IOReturn result;
	CFArrayCallBacks arrayCallbacks;

	// Allocate a FWAVCDeviceController struct
	FWAVCDeviceController *pFWAVCDeviceController = new FWAVCDeviceController;
	if (!pFWAVCDeviceController)
		return kIOReturnNoMemory;

	// Create an array to hold FWAVCDevice instances
	arrayCallbacks.version = 0;
	arrayCallbacks.retain = NULL;
	arrayCallbacks.copyDescription = NULL;
	arrayCallbacks.equal = NULL;
	arrayCallbacks.release = DeviceControllerArrayReleaseObjectCallback;
	pFWAVCDeviceController->fwavcDeviceArray = CFArrayCreateMutable(NULL,0,&arrayCallbacks);
	if (!pFWAVCDeviceController->fwavcDeviceArray)
	{
		delete pFWAVCDeviceController;
		return kIOReturnNoMemory;
	}

	// Initialize 
	pthread_mutex_init(&pFWAVCDeviceController->retainReleaseMutex,NULL);
	pFWAVCDeviceController->retainCount = 1;
	pFWAVCDeviceController->clientNotificationProc = clientNotificationProc;
	pFWAVCDeviceController->pRefCon = pRefCon;
	pFWAVCDeviceController->globalAVCDeviceMessageProc = globalAVCDeviceMessageProc;
	
	// Create an AVS AVCDeviceController
	result = CreateAVCDeviceController(&pFWAVCDeviceController->pAVCDeviceController, 
									   MyAVCDeviceControllerNotification, 
									   pFWAVCDeviceController, 
									   MyGlobalAVCDeviceMessageNotification);

	*pFWAVCDeviceControllerRef = pFWAVCDeviceController;

	// If we got an error, we need to delete the allocated struct
	if (result != kIOReturnSuccess)
	{
		pthread_mutex_destroy(&pFWAVCDeviceController->retainReleaseMutex);
		delete pFWAVCDeviceController;
	}	

	return result;
}

//////////////////////////////////////////////////////////
// FWAVCDeviceControllerRetain
//////////////////////////////////////////////////////////
FWAVCDeviceControllerRef FWAVCDeviceControllerRetain(FWAVCDeviceControllerRef fwavcDeviceControllerRef)
{
	if (fwavcDeviceControllerRef)
	{
		pthread_mutex_lock(&fwavcDeviceControllerRef->retainReleaseMutex);
		fwavcDeviceControllerRef->retainCount += 1;
		pthread_mutex_unlock(&fwavcDeviceControllerRef->retainReleaseMutex);
	}
	return fwavcDeviceControllerRef;
}

//////////////////////////////////////////////////////////
// FWAVCDeviceControllerRelease
//////////////////////////////////////////////////////////
void  FWAVCDeviceControllerRelease(FWAVCDeviceControllerRef fwavcDeviceControllerRef)
{
	if (fwavcDeviceControllerRef)
	{
		pthread_mutex_lock(&fwavcDeviceControllerRef->retainReleaseMutex);
		fwavcDeviceControllerRef->retainCount -= 1;
		
		if (fwavcDeviceControllerRef->retainCount == 0)
		{
			// Destroy the AVS AVCDeviceController
			DestroyAVCDeviceController(fwavcDeviceControllerRef->pAVCDeviceController);
			
			// Delete all objects in fwavcDeviceArray, and release array object
			if (fwavcDeviceControllerRef->fwavcDeviceArray)
				CFRelease(fwavcDeviceControllerRef->fwavcDeviceArray);

			// Destroy the retain/release mutex
			pthread_mutex_destroy(&fwavcDeviceControllerRef->retainReleaseMutex);

			// Delete the FWAVCDeviceController struct
			delete fwavcDeviceControllerRef;
		}
		else
			pthread_mutex_unlock(&fwavcDeviceControllerRef->retainReleaseMutex);
	}
}

//////////////////////////////////////////////////////////
// FWAVCDeviceControllerCopyDeviceArray
//////////////////////////////////////////////////////////
CFArrayRef FWAVCDeviceControllerCopyDeviceArray(FWAVCDeviceControllerRef fwavcDeviceControllerRef)
{	
	CFMutableArrayRef newArray = CFArrayCreateMutable(NULL,0,NULL);
	if ((newArray) && (CFArrayGetCount(fwavcDeviceControllerRef->fwavcDeviceArray) != 0))
		CFArrayAppendArray(newArray, fwavcDeviceControllerRef->fwavcDeviceArray, CFRangeMake(0, CFArrayGetCount(fwavcDeviceControllerRef->fwavcDeviceArray)));
	return newArray;
}


#pragma mark -
#pragma mark ======================================
#pragma mark Front-end APIs for the AVCDevice class object
#pragma mark ======================================

//////////////////////////////////////////////////////////
// FWAVCDeviceOpen
//////////////////////////////////////////////////////////
IOReturn FWAVCDeviceOpen(FWAVCDeviceRef fwavcDeviceRef, FWAVCDeviceMessageNotification deviceMessageProc, void *pMessageProcRefCon)
{
	// If we're already open, return an error
	if (fwavcDeviceRef->pAVCDevice->isOpened())
		return kIOReturnExclusiveAccess;
	
	fwavcDeviceRef->deviceMessageProc = deviceMessageProc;
	fwavcDeviceRef->pMessageProcRefCon = pMessageProcRefCon;
	return fwavcDeviceRef->pAVCDevice->openDevice(MyAVCDeviceMessageNotification,fwavcDeviceRef);
}

//////////////////////////////////////////////////////////
// FWAVCDeviceClose
//////////////////////////////////////////////////////////
IOReturn FWAVCDeviceClose(FWAVCDeviceRef fwavcDeviceRef)
{
	return fwavcDeviceRef->pAVCDevice->closeDevice();
}

//////////////////////////////////////////////////////////
// FWAVCDeviceAVCCommand
//////////////////////////////////////////////////////////
IOReturn FWAVCDeviceAVCCommand(FWAVCDeviceRef fwavcDeviceRef, const UInt8 *command, UInt32 cmdLen, UInt8 *response, UInt32 *responseLen)
{
	return fwavcDeviceRef->pAVCDevice->AVCCommand(command,cmdLen,response,responseLen);
}

//////////////////////////////////////////////////////////
// FWAVCDeviceIsOpened
//////////////////////////////////////////////////////////
Boolean FWAVCDeviceIsOpened(FWAVCDeviceRef fwavcDeviceRef)
{
	return (Boolean) fwavcDeviceRef->pAVCDevice->isOpened();
}

//////////////////////////////////////////////////////////
// FWAVCDeviceGetGUID
//////////////////////////////////////////////////////////
UInt64 FWAVCDeviceGetGUID(FWAVCDeviceRef fwavcDeviceRef)
{
	return fwavcDeviceRef->pAVCDevice->guid;
}

//////////////////////////////////////////////////////////
// FWAVCDeviceCopyDeviceName
//////////////////////////////////////////////////////////
CFStringRef FWAVCDeviceCopyDeviceName(FWAVCDeviceRef fwavcDeviceRef)
{
	CFStringRef nameString = CFStringCreateWithCString(NULL, fwavcDeviceRef->pAVCDevice->deviceName, kCFStringEncodingMacRoman);
	return nameString;
}

//////////////////////////////////////////////////////////
// FWAVCDeviceCopyVendorName
//////////////////////////////////////////////////////////
CFStringRef FWAVCDeviceCopyVendorName(FWAVCDeviceRef fwavcDeviceRef)
{
	CFStringRef nameString = CFStringCreateWithCString(NULL, fwavcDeviceRef->pAVCDevice->vendorName, kCFStringEncodingMacRoman);
	return nameString;
}

//////////////////////////////////////////////////////////
// FWAVCDeviceGetVendorID
//////////////////////////////////////////////////////////
UInt32 FWAVCDeviceGetVendorID(FWAVCDeviceRef fwavcDeviceRef)
{
	return fwavcDeviceRef->pAVCDevice->vendorID;
}

//////////////////////////////////////////////////////////
// FWAVCDeviceGetModelID
//////////////////////////////////////////////////////////
UInt32 FWAVCDeviceGetModelID(FWAVCDeviceRef fwavcDeviceRef)
{
	return fwavcDeviceRef->pAVCDevice->modelID;
}

//////////////////////////////////////////////////////////
// FWAVCDeviceIsAttached
//////////////////////////////////////////////////////////
Boolean FWAVCDeviceIsAttached(FWAVCDeviceRef fwavcDeviceRef)
{
	return (Boolean) fwavcDeviceRef->pAVCDevice->isAttached;
}

//////////////////////////////////////////////////////////
// FWAVCDeviceSupportsFCP
//////////////////////////////////////////////////////////
Boolean FWAVCDeviceSupportsFCP(FWAVCDeviceRef fwavcDeviceRef)
{
	return (Boolean) fwavcDeviceRef->pAVCDevice->supportsFCP;
}

//////////////////////////////////////////////////////////
// FWAVCDeviceHasTapeSubunit
//////////////////////////////////////////////////////////
Boolean FWAVCDeviceHasTapeSubunit(FWAVCDeviceRef fwavcDeviceRef)
{
	return (Boolean) fwavcDeviceRef->pAVCDevice->hasTapeSubunit;
}

//////////////////////////////////////////////////////////
// FWAVCDeviceHasMonitorOrTunerSubunit
//////////////////////////////////////////////////////////
Boolean FWAVCDeviceHasMonitorOrTunerSubunit(FWAVCDeviceRef fwavcDeviceRef)
{
	return (Boolean) fwavcDeviceRef->pAVCDevice->hasMonitorOrTunerSubunit;
}

//////////////////////////////////////////////////////////
// FWAVCDeviceGetSubUnits
//////////////////////////////////////////////////////////
UInt32 FWAVCDeviceGetSubUnits(FWAVCDeviceRef fwavcDeviceRef)
{
	return fwavcDeviceRef->pAVCDevice->subUnits;
}

//////////////////////////////////////////////////////////
// FWAVCDeviceNumInputPlugs
//////////////////////////////////////////////////////////
UInt32 FWAVCDeviceNumInputPlugs(FWAVCDeviceRef fwavcDeviceRef)
{
	return fwavcDeviceRef->pAVCDevice->numInputPlugs;
}

//////////////////////////////////////////////////////////
// FWAVCDeviceNumOutputPlugs
//////////////////////////////////////////////////////////
UInt32 FWAVCDeviceNumOutputPlugs(FWAVCDeviceRef fwavcDeviceRef)
{
	return fwavcDeviceRef->pAVCDevice->numOutputPlugs;
}

//////////////////////////////////////////////////////////
// FWAVCDeviceCapabilitiesDiscovered
//////////////////////////////////////////////////////////
Boolean FWAVCDeviceCapabilitiesDiscovered(FWAVCDeviceRef fwavcDeviceRef)
{
	return (Boolean) fwavcDeviceRef->pAVCDevice->capabilitiesDiscovered;
}

//////////////////////////////////////////////////////////
// FWAVCDeviceIsDVDevice
//////////////////////////////////////////////////////////
Boolean FWAVCDeviceIsDVDevice(FWAVCDeviceRef fwavcDeviceRef)
{
	return (Boolean) fwavcDeviceRef->pAVCDevice->isDVDevice;
}

//////////////////////////////////////////////////////////
// FWAVCDeviceIsDVCProDevice
//////////////////////////////////////////////////////////
Boolean FWAVCDeviceIsDVCProDevice(FWAVCDeviceRef fwavcDeviceRef)
{
	return (Boolean) fwavcDeviceRef->pAVCDevice->isDVCProDevice;
}

//////////////////////////////////////////////////////////
// FWAVCDeviceGetDVMode
//////////////////////////////////////////////////////////
UInt8 FWAVCDeviceGetDVMode(FWAVCDeviceRef fwavcDeviceRef)
{
	return fwavcDeviceRef->pAVCDevice->dvMode;
}

//////////////////////////////////////////////////////////
// FWAVCDeviceIsMpegDevice
//////////////////////////////////////////////////////////
Boolean FWAVCDeviceIsMpegDevice(FWAVCDeviceRef fwavcDeviceRef)
{
	return (Boolean) fwavcDeviceRef->pAVCDevice->isMPEGDevice;
}

//////////////////////////////////////////////////////////
// FWAVCDeviceGetMpegMode
//////////////////////////////////////////////////////////
UInt8 FWAVCDeviceGetMpegMode(FWAVCDeviceRef fwavcDeviceRef)
{
	return fwavcDeviceRef->pAVCDevice->mpegMode;
}

//////////////////////////////////////////////////////////
// FWAVCDeviceGetAVCInterface
//////////////////////////////////////////////////////////
IOFireWireAVCLibUnitInterface** FWAVCDeviceGetAVCInterface(FWAVCDeviceRef fwavcDeviceRef)
{
	return fwavcDeviceRef->pAVCDevice->avcInterface;
}

//////////////////////////////////////////////////////////
// FWAVCDeviceGetDeviceInterface
//////////////////////////////////////////////////////////
IOFireWireLibDeviceRef FWAVCDeviceGetDeviceInterface(FWAVCDeviceRef fwavcDeviceRef)
{
	return fwavcDeviceRef->pAVCDevice->deviceInterface;
}

//////////////////////////////////////////////////////////
// FWAVCDeviceGetClientPrivateData
//////////////////////////////////////////////////////////
void *FWAVCDeviceGetClientPrivateData(FWAVCDeviceRef fwavcDeviceRef)
{
	return fwavcDeviceRef->pAVCDevice->GetClientPrivateData();
}

//////////////////////////////////////////////////////////
// FWAVCDeviceSetClientPrivateData
//////////////////////////////////////////////////////////
void FWAVCDeviceSetClientPrivateData(FWAVCDeviceRef fwavcDeviceRef, void *pClientPrivateData)
{
	fwavcDeviceRef->pAVCDevice->SetClientPrivateData(pClientPrivateData);
}

//////////////////////////////////////////////////////////
// FWAVCDeviceReDiscoverAVCDeviceCapabilities
//////////////////////////////////////////////////////////
IOReturn FWAVCDeviceReDiscoverAVCDeviceCapabilities(FWAVCDeviceRef fwavcDeviceRef)
{
	return fwavcDeviceRef->pAVCDevice->discoverAVCDeviceCapabilities();
}

//////////////////////////////////////////////////////////
// FWAVCDeviceCreateUniversalReceiver
//////////////////////////////////////////////////////////
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
{
	// Allocate a FWAVCDeviceStream struct
	FWAVCDeviceStream *pFWAVCDeviceStream = new FWAVCDeviceStream;
	if (!pFWAVCDeviceStream)
		return nil;

	// Allocate a StringLogger
	pFWAVCDeviceStream->pStringLogger = new StringLogger(debugLogStringProc);
	if (!pFWAVCDeviceStream->pStringLogger)
	{
		delete pFWAVCDeviceStream;
		return nil;
	}
	
	// Initialize
	pthread_mutex_init(&pFWAVCDeviceStream->retainReleaseMutex,NULL);
	pFWAVCDeviceStream->retainCount = 1;
	pFWAVCDeviceStream->pFWAVCDevice = fwavcDeviceRef;
	pFWAVCDeviceStream->universalReceiverDataReceivedProcHandler = dataReceivedProcHandler;
	pFWAVCDeviceStream->pDataProcRefCon = pDataReceivedProcRefCon;
	pFWAVCDeviceStream->messageProcHandler = messageProcHandler;
	pFWAVCDeviceStream->pMessageProcRefCon = pMessageProcRefCon;
	
	// Create the AVS universal receiver using the AVC device class
	pFWAVCDeviceStream->pAVCDeviceStream = 
		fwavcDeviceRef->pAVCDevice->CreateUniversalReceiverForDevicePlug(deviceOutputPlugNum,
																	 MyUniversalReceiverDataPushProc,
																	 pFWAVCDeviceStream,
																	 MyDeviceStreamMessageProc,
																	 pFWAVCDeviceStream,
																	 pFWAVCDeviceStream->pStringLogger,
																	 (cyclesPerSegment == kFWAVCDeviceStreamDefaultParameter) ? 
																		kFWAVCCyclesPerUniversalReceiveSegment : cyclesPerSegment,
																	 (numSegments == kFWAVCDeviceStreamDefaultParameter) ?
																		kFWAVCNumUniversalReceiveSegments : numSegments,
																	 (cycleBufferSize == kFWAVCDeviceStreamDefaultParameter) ?
																		kFWAVCUniversalReceiverDefaultBufferSize : cycleBufferSize);
	if (!pFWAVCDeviceStream->pAVCDeviceStream)
	{
		pthread_mutex_destroy(&pFWAVCDeviceStream->retainReleaseMutex);
		delete pFWAVCDeviceStream->pStringLogger;
		delete pFWAVCDeviceStream;
		pFWAVCDeviceStream = nil;
	}

	return pFWAVCDeviceStream;
}

//////////////////////////////////////////////////////////
// FWAVCDeviceCreateMPEGReceiver
//////////////////////////////////////////////////////////
FWAVCDeviceStreamRef FWAVCDeviceCreateMPEGReceiver(FWAVCDeviceRef fwavcDeviceRef,
												   UInt8 deviceOutputPlugNum,
												   FWAVCMPEG2ReceiverDataReceivedProc dataReceivedProcHandler,
												   void *pDataReceivedProcRefCon,
												   FWAVCDeviceStreamMessageProc messageProcHandler,
												   void *pMessageProcRefCon,
												   unsigned int cyclesPerSegment,
												   unsigned int numSegments,
												   FWAVCDebugLoggingStringHandler debugLogStringProc)
{
	// Allocate a FWAVCDeviceStream struct
	FWAVCDeviceStream *pFWAVCDeviceStream = new FWAVCDeviceStream;
	if (!pFWAVCDeviceStream)
		return nil;
	
	// Allocate a StringLogger
	pFWAVCDeviceStream->pStringLogger = new StringLogger(debugLogStringProc);
	if (!pFWAVCDeviceStream->pStringLogger)
	{
		delete pFWAVCDeviceStream;
		return nil;
	}
	
	// Initialize
	pthread_mutex_init(&pFWAVCDeviceStream->retainReleaseMutex,NULL);
	pFWAVCDeviceStream->retainCount = 1;
	pFWAVCDeviceStream->pFWAVCDevice = fwavcDeviceRef;
	pFWAVCDeviceStream->mpeg2ReceiverDataReceivedProcHandler = dataReceivedProcHandler;
	pFWAVCDeviceStream->pDataProcRefCon = pDataReceivedProcRefCon;
	pFWAVCDeviceStream->messageProcHandler = messageProcHandler;
	pFWAVCDeviceStream->pMessageProcRefCon = pMessageProcRefCon;
	
	// Create the AVS universal receiver using the AVC device class
	pFWAVCDeviceStream->pAVCDeviceStream = 
		fwavcDeviceRef->pAVCDevice->CreateMPEGReceiverForDevicePlug(deviceOutputPlugNum,
																nil,	// Note: We'll register an extended data push proc, below!
																nil,
																MyDeviceStreamMessageProc,
																pFWAVCDeviceStream,
																pFWAVCDeviceStream->pStringLogger,
																(cyclesPerSegment == kFWAVCDeviceStreamDefaultParameter) ?
																	kFWAVCCyclesPerMPEG2ReceiveSegment : cyclesPerSegment,
																(numSegments == kFWAVCDeviceStreamDefaultParameter) ?
																	kFWAVCNumMPEG2ReceiveSegments : numSegments);
	if (!pFWAVCDeviceStream->pAVCDeviceStream)
	{
		pthread_mutex_destroy(&pFWAVCDeviceStream->retainReleaseMutex);
		delete pFWAVCDeviceStream->pStringLogger;
		delete pFWAVCDeviceStream;
		pFWAVCDeviceStream = nil;
	}
	else
	{
		pFWAVCDeviceStream->pAVCDeviceStream->pMPEGReceiver->registerExtendedDataPushCallback( MyMPEG2ReceiverExtendedDataPushProc, pFWAVCDeviceStream);
	}
	
	return pFWAVCDeviceStream;
}

//////////////////////////////////////////////////////////
// FWAVCDeviceCreateMPEGTransmitter
//////////////////////////////////////////////////////////
FWAVCDeviceStreamRef FWAVCDeviceCreateMPEGTransmitter(FWAVCDeviceRef fwavcDeviceRef,
													  UInt8 deviceInputPlugNum,
													  FWAVCMPEG2TransmitterDataRequestProc dataRequestProcHandler,
													  void *pDataReuestProcRefCon,
													  FWAVCDeviceStreamMessageProc messageProcHandler,
													  void *pMessageProcRefCon,
													  unsigned int cyclesPerSegment,
													  unsigned int numSegments,
													  unsigned int packetsPerCycle,
													  unsigned int tsPacketQueueSizeInPackets,
													  FWAVCDebugLoggingStringHandler debugLogStringProc)
{
	// Allocate a FWAVCDeviceStream struct
	FWAVCDeviceStream *pFWAVCDeviceStream = new FWAVCDeviceStream;
	if (!pFWAVCDeviceStream)
		return nil;
	
	// Allocate a StringLogger
	pFWAVCDeviceStream->pStringLogger = new StringLogger(debugLogStringProc);
	if (!pFWAVCDeviceStream->pStringLogger)
	{
		delete pFWAVCDeviceStream;
		return nil;
	}
	
	// Initialize
	pthread_mutex_init(&pFWAVCDeviceStream->retainReleaseMutex,NULL);
	pFWAVCDeviceStream->retainCount = 1;
	pFWAVCDeviceStream->pFWAVCDevice = fwavcDeviceRef;
	pFWAVCDeviceStream->mpeg2TransmitterDataRequestProcHandler = dataRequestProcHandler;
	pFWAVCDeviceStream->pDataProcRefCon = pDataReuestProcRefCon;
	pFWAVCDeviceStream->messageProcHandler = messageProcHandler;
	pFWAVCDeviceStream->pMessageProcRefCon = pMessageProcRefCon;
	
	// Create the AVS mpeg transmitter using the AVC device class
	pFWAVCDeviceStream->pAVCDeviceStream = 
		fwavcDeviceRef->pAVCDevice->CreateMPEGTransmitterForDevicePlug(deviceInputPlugNum,
																   MyMPEG2TransmitterDataPullProc,
																   pFWAVCDeviceStream,
																   MyDeviceStreamMessageProc,
																   pFWAVCDeviceStream,
																   pFWAVCDeviceStream->pStringLogger,
																   (cyclesPerSegment == kFWAVCDeviceStreamDefaultParameter) ?
																		kFWAVCCyclesPerMPEG2TransmitSegment : cyclesPerSegment,
																   (numSegments == kFWAVCDeviceStreamDefaultParameter) ?
																		kFWAVCNumMPEG2TransmitSegments : numSegments,
																   (packetsPerCycle == kFWAVCDeviceStreamDefaultParameter) ?
																		kFWAVCNumTSPacketsPerMPEG2TransmitCycle : packetsPerCycle,
																   (tsPacketQueueSizeInPackets == kFWAVCDeviceStreamDefaultParameter) ?
																		kFWAVCMPEG2TransmitTSPacketQueueSizeInPackets : tsPacketQueueSizeInPackets);
	if (!pFWAVCDeviceStream->pAVCDeviceStream)
	{
		pthread_mutex_destroy(&pFWAVCDeviceStream->retainReleaseMutex);
		delete pFWAVCDeviceStream->pStringLogger;
		delete pFWAVCDeviceStream;
		pFWAVCDeviceStream = nil;
	}
	
	return pFWAVCDeviceStream;
}

//////////////////////////////////////////////////////////
// FWAVCDeviceCreateDVTransmitter
//////////////////////////////////////////////////////////
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
{
	DVTransmitFrame *pDVTransmitFrame;
	
	// Allocate a FWAVCDeviceStream struct
	FWAVCDeviceStream *pFWAVCDeviceStream = new FWAVCDeviceStream;
	if (!pFWAVCDeviceStream)
		return nil;
	
	// Allocate a StringLogger
	pFWAVCDeviceStream->pStringLogger = new StringLogger(debugLogStringProc);
	if (!pFWAVCDeviceStream->pStringLogger)
	{
		delete pFWAVCDeviceStream;
		return nil;
	}
	
	// Initialize
	pthread_mutex_init(&pFWAVCDeviceStream->retainReleaseMutex,NULL);
	pFWAVCDeviceStream->retainCount = 1;
	pFWAVCDeviceStream->pFWAVCDevice = fwavcDeviceRef;

	pFWAVCDeviceStream->dvTransmitterframeRequestProcHandler = frameRequestProcHandler;
	pFWAVCDeviceStream->pDVTransmitterFrameRequestProcRefCon = pFrameRequestProcRefCon;
	pFWAVCDeviceStream->dvTransmitterframeReturnProcHandler = frameReturnProcHandler;
	pFWAVCDeviceStream->pDVTransmitterFrameReleaseProcRefCon = pFrameReleaseProcRefCon;
	
	pFWAVCDeviceStream->messageProcHandler = messageProcHandler;
	pFWAVCDeviceStream->pMessageProcRefCon = pMessageProcRefCon;
	
	// Create the AVS mpeg transmitter using the AVC device class
	pFWAVCDeviceStream->pAVCDeviceStream = 
		fwavcDeviceRef->pAVCDevice->CreateDVTransmitterForDevicePlug(deviceInputPlugNum,
																 MyDVFramePullProc,
																 pFWAVCDeviceStream,
																 MyDVFrameReleaseProc,
																 pFWAVCDeviceStream,
																 MyDeviceStreamMessageProc,
																 pFWAVCDeviceStream,
																 pFWAVCDeviceStream->pStringLogger,
																 (cyclesPerSegment == kFWAVCDeviceStreamDefaultParameter) ?
																		kFWAVCCyclesPerDVTransmitSegment : cyclesPerSegment,
																 (numSegments == kFWAVCDeviceStreamDefaultParameter) ?
																		kFWAVCNumDVTransmitSegments : numSegments,
																 (transmitterDVMode == kFWAVCDeviceStreamDefaultParameter) ?
																		kFWAVCDVMode_SD_525_60 : transmitterDVMode,
																 (numFrameBuffers == kFWAVCDeviceStreamDefaultParameter) ?
																		kFWAVCDVTransmitNumFrameBuffers : numFrameBuffers);
	if (!pFWAVCDeviceStream->pAVCDeviceStream)
	{
		pthread_mutex_destroy(&pFWAVCDeviceStream->retainReleaseMutex);
		delete pFWAVCDeviceStream->pStringLogger;
		delete pFWAVCDeviceStream;
		pFWAVCDeviceStream = nil;
	}
	else
	{
		// Create an array to hold all the frame refs
		pFWAVCDeviceStream->dvTransmitterFrameRefArray = CFArrayCreateMutable(NULL,pFWAVCDeviceStream->pAVCDeviceStream->pDVTransmitter->getNumFrames(),NULL);
		if (!pFWAVCDeviceStream->dvTransmitterFrameRefArray)
		{
			pthread_mutex_destroy(&pFWAVCDeviceStream->retainReleaseMutex);
			delete pFWAVCDeviceStream->pStringLogger;
			delete pFWAVCDeviceStream;
			pFWAVCDeviceStream = nil;
		}
		else
		{
			// Add the frames to the array
			for (UInt32 i=0;i<pFWAVCDeviceStream->pAVCDeviceStream->pDVTransmitter->getNumFrames();i++)
			{
				pDVTransmitFrame = pFWAVCDeviceStream->pAVCDeviceStream->pDVTransmitter->getFrame(i);
				CFArrayAppendValue(pFWAVCDeviceStream->dvTransmitterFrameRefArray,pDVTransmitFrame);
			}
		}
	}
	
	return pFWAVCDeviceStream;
}

//////////////////////////////////////////////////////////
// FWAVCDeviceCreateDVReceiver
//////////////////////////////////////////////////////////
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
{
	// Allocate a FWAVCDeviceStream struct
	FWAVCDeviceStream *pFWAVCDeviceStream = new FWAVCDeviceStream;
	if (!pFWAVCDeviceStream)
		return nil;
	
	// Allocate a StringLogger
	pFWAVCDeviceStream->pStringLogger = new StringLogger(debugLogStringProc);
	if (!pFWAVCDeviceStream->pStringLogger)
	{
		delete pFWAVCDeviceStream;
		return nil;
	}
	
	// Initialize
	pthread_mutex_init(&pFWAVCDeviceStream->retainReleaseMutex,NULL);
	pFWAVCDeviceStream->retainCount = 1;
	pFWAVCDeviceStream->pFWAVCDevice = fwavcDeviceRef;
	pFWAVCDeviceStream->dvRececiverFrameReceivedProc = frameReceivedProcHandler;
	pFWAVCDeviceStream->pDataProcRefCon = pFrameReceivedProcRefCon;
	pFWAVCDeviceStream->messageProcHandler = messageProcHandler;
	pFWAVCDeviceStream->pMessageProcRefCon = pMessageProcRefCon;
	
	// Create the AVS DV receiver using the AVC device class
	pFWAVCDeviceStream->pAVCDeviceStream = 
		fwavcDeviceRef->pAVCDevice->CreateDVReceiverForDevicePlug(deviceOutputPlugNum,
															  MyDVFrameReceivedProc,
															  pFWAVCDeviceStream,
															  MyDeviceStreamMessageProc,
															  pFWAVCDeviceStream,
															  pFWAVCDeviceStream->pStringLogger,
															  (cyclesPerSegment == kFWAVCDeviceStreamDefaultParameter) ?
																	kFWAVCCyclesPerDVReceiveSegment : cyclesPerSegment,
															  (numSegments == kFWAVCDeviceStreamDefaultParameter) ?
																	kFWAVCNumDVReceiveSegments : numSegments,
															  (receiverDVMode == kFWAVCDeviceStreamDefaultParameter) ?
																	kFWAVCDVMode_SD_525_60 : receiverDVMode,
															  (numFrameBuffers == kFWAVCDeviceStreamDefaultParameter) ?
																	kFWAVCDVReceiveNumFrameBuffers : numFrameBuffers);
	if (!pFWAVCDeviceStream->pAVCDeviceStream)
	{
		pthread_mutex_destroy(&pFWAVCDeviceStream->retainReleaseMutex);
		delete pFWAVCDeviceStream->pStringLogger;
		delete pFWAVCDeviceStream;
		pFWAVCDeviceStream = nil;
	}
	
	return pFWAVCDeviceStream;
}

//////////////////////////////////////////////////////////
// FWAVCDeviceStreamStart
//////////////////////////////////////////////////////////
IOReturn FWAVCDeviceStreamStart(FWAVCDeviceStreamRef fwavcDeviceStreamRef)
{
	return fwavcDeviceStreamRef->pAVCDeviceStream->pAVCDevice->StartAVCDeviceStream(fwavcDeviceStreamRef->pAVCDeviceStream);
}

//////////////////////////////////////////////////////////
// FWAVCDeviceStreamStop
//////////////////////////////////////////////////////////
IOReturn FWAVCDeviceStreamStop(FWAVCDeviceStreamRef fwavcDeviceStreamRef)
{
	return fwavcDeviceStreamRef->pAVCDeviceStream->pAVCDevice->StopAVCDeviceStream(fwavcDeviceStreamRef->pAVCDeviceStream);
}

//////////////////////////////////////////////////////////
// FWAVCDeviceStreamRetain
//////////////////////////////////////////////////////////
FWAVCDeviceStreamRef FWAVCDeviceStreamRetain(FWAVCDeviceStreamRef fwavcDeviceStreamRef)
{
	if (fwavcDeviceStreamRef)
	{
		pthread_mutex_lock(&fwavcDeviceStreamRef->retainReleaseMutex);
		fwavcDeviceStreamRef->retainCount += 1;
		pthread_mutex_unlock(&fwavcDeviceStreamRef->retainReleaseMutex);
	}
	return fwavcDeviceStreamRef;
}

//////////////////////////////////////////////////////////
// FWAVCDeviceStreamRelease
//////////////////////////////////////////////////////////
void FWAVCDeviceStreamRelease(FWAVCDeviceStreamRef fwavcDeviceStreamRef)
{
	if (fwavcDeviceStreamRef)
	{
		pthread_mutex_lock(&fwavcDeviceStreamRef->retainReleaseMutex);
		fwavcDeviceStreamRef->retainCount -= 1;
		
		if (fwavcDeviceStreamRef->retainCount == 0)
		{
			// If this stream is for a DVTransmitter, we need to release the frame-ref array
			if (fwavcDeviceStreamRef->pAVCDeviceStream->streamType == kStreamTypeDVTransmitter)
				CFRelease(fwavcDeviceStreamRef->dvTransmitterFrameRefArray);
			
			// Destroy the device stream
			fwavcDeviceStreamRef->pAVCDeviceStream->pAVCDevice->DestroyAVCDeviceStream(fwavcDeviceStreamRef->pAVCDeviceStream);			
			
			// Delete the StringLogger
			delete fwavcDeviceStreamRef->pStringLogger;
			
			// Destroy the retain/release mutex
			pthread_mutex_destroy(&fwavcDeviceStreamRef->retainReleaseMutex);
			
			// Delete the FWAVCTSDemuxer struct object
			delete fwavcDeviceStreamRef;
		}
		else
			pthread_mutex_unlock(&fwavcDeviceStreamRef->retainReleaseMutex);
	}
}

//////////////////////////////////////////////////////////
// FWAVCDeviceStreamSetNoDataCallback
//////////////////////////////////////////////////////////
IOReturn FWAVCDeviceStreamSetNoDataCallback(FWAVCDeviceStreamRef fwavcDeviceStreamRef,
											FWAVCIsochReceiverNoDataProc handler, 
											void *pRefCon, 
											UInt32 noDataTimeInMSec)
{
	IOReturn result;

	switch (fwavcDeviceStreamRef->pAVCDeviceStream->streamType)
	{
		case kStreamTypeDVReceiver:
			result = fwavcDeviceStreamRef->pAVCDeviceStream->pDVReceiver->registerNoDataNotificationCallback(MyFWAVCIsochReceiverNoDataProc, fwavcDeviceStreamRef, noDataTimeInMSec);
			break;
			
		case kStreamTypeMPEGReceiver:
			result = fwavcDeviceStreamRef->pAVCDeviceStream->pMPEGReceiver->registerNoDataNotificationCallback(MyFWAVCIsochReceiverNoDataProc, fwavcDeviceStreamRef, noDataTimeInMSec);
			break;
			
		case kStreamTypeUniversalReceiver:
			result = fwavcDeviceStreamRef->pAVCDeviceStream->pUniversalReceiver->registerNoDataNotificationCallback(MyFWAVCIsochReceiverNoDataProc, fwavcDeviceStreamRef, noDataTimeInMSec);
			break;
			
		default:
			result = kIOReturnError;
			break;
	}
	
	if (result == kIOReturnSuccess)
	{
		fwavcDeviceStreamRef->noDataHandler = handler;
		fwavcDeviceStreamRef->pNoDataHandlerRefCon = pRefCon;
	}

	return result;
}

//////////////////////////////////////////////////////////
// FWAVCUniversalReceiverSetStructuredDataReceivedCallback
//////////////////////////////////////////////////////////
IOReturn FWAVCUniversalReceiverSetStructuredDataReceivedCallback(FWAVCDeviceStreamRef fwavcDeviceStreamRef,
																 FWAVCUniversalReceiverStructuredDataReceivedProc handler, 
																 UInt32 maxCycleStructsPerCallback, 
																 void *pRefCon)
{
	IOReturn result;

	if (fwavcDeviceStreamRef->pAVCDeviceStream->streamType != kStreamTypeUniversalReceiver)
		return kIOReturnError;

	result = fwavcDeviceStreamRef->pAVCDeviceStream->pUniversalReceiver->registerStructuredDataPushCallback(MyStructuredUniversalReceiverDataPushProc, 
																										  (maxCycleStructsPerCallback == kFWAVCDeviceStreamDefaultParameter) ?
																												kFWAVCCyclesPerUniversalReceiveSegment : maxCycleStructsPerCallback, 
																										  fwavcDeviceStreamRef);
	if (result == kIOReturnSuccess)
	{
		fwavcDeviceStreamRef->universalReceiverStructuredDataHandler = handler;
		fwavcDeviceStreamRef->pStructuredDataHandlerRefCon = pRefCon;
	}

	return result;
}


//////////////////////////////////////////////////////////
// FWAVCMPEG2TransmitterSetTimeStampCallback
//////////////////////////////////////////////////////////
IOReturn FWAVCMPEG2TransmitterSetTimeStampCallback(FWAVCDeviceStreamRef fwavcDeviceStreamRef,
												   FWAVCMPEG2TransmitterTimeStampProc handler, 
												   void *pRefCon)
{
	IOReturn result;
	
	if (fwavcDeviceStreamRef->pAVCDeviceStream->streamType != kStreamTypeMPEGTransmitter)
		return kIOReturnError;
	
	result = fwavcDeviceStreamRef->pAVCDeviceStream->pMPEGTransmitter->registerTimeStampCallback(MyMPEG2TransmitterTimeStampProc, fwavcDeviceStreamRef);
	if (result == kIOReturnSuccess)
	{
		fwavcDeviceStreamRef->mpeg2TransmitterTimeStampProcHandler = handler;
		fwavcDeviceStreamRef->pMPEG2TransmitterTimeStampProcHandlerRefCon = pRefCon;
	}
	
	return result;
}

//////////////////////////////////////////////////////////
// FWAVCMPEG2ReceiverSetStructuredDataReceivedCallback
//////////////////////////////////////////////////////////
IOReturn FWAVCMPEG2ReceiverSetStructuredDataReceivedCallback(FWAVCDeviceStreamRef fwavcDeviceStreamRef,
															 FWAVCMPEG2ReceiverStructuredDataReceivedProc handler, 
															 UInt32 maxCycleStructsPerCallback, 
															 void *pRefCon)
{
	IOReturn result;
	
	if (fwavcDeviceStreamRef->pAVCDeviceStream->streamType != kStreamTypeMPEGReceiver)
		return kIOReturnError;
	
	result = fwavcDeviceStreamRef->pAVCDeviceStream->pMPEGReceiver->registerStructuredDataPushCallback(MyStructuredMPEG2ReceiverDataPushProc, 
																									 (maxCycleStructsPerCallback == kFWAVCDeviceStreamDefaultParameter) ?
																									 kFWAVCCyclesPerMPEG2ReceiveSegment : maxCycleStructsPerCallback, 
																									 fwavcDeviceStreamRef);
	if (result == kIOReturnSuccess)
	{
		fwavcDeviceStreamRef->mpeg2ReceiverStructuredDataHandler = handler;
		fwavcDeviceStreamRef->pStructuredDataHandlerRefCon = pRefCon;
	}
	
	return result;
}

//////////////////////////////////////////////////////////
// FWAVCMPEG2ReceiverRegisterStructuredDataReceivedCallback
//////////////////////////////////////////////////////////
void FWAVCMPEG2ReceiverIncludeSourcePacketHeaders(FWAVCDeviceStreamRef fwavcDeviceStreamRef, Boolean wantSPH)
{
	if (fwavcDeviceStreamRef->pAVCDeviceStream->streamType == kStreamTypeMPEGReceiver)
		fwavcDeviceStreamRef->pAVCDeviceStream->pMPEGReceiver->ReceiveSourcePacketHeaders(wantSPH);
}

//////////////////////////////////////////////////////////
// FWAVCDVTransmitterCopyFrameRefArray
//////////////////////////////////////////////////////////
CFArrayRef FWAVCDVTransmitterCopyFrameRefArray(FWAVCDeviceStreamRef fwavcDeviceStreamRef)
{
	if (fwavcDeviceStreamRef->pAVCDeviceStream->streamType == kStreamTypeDVTransmitter)
	{
		return CFArrayCreateCopy(NULL,fwavcDeviceStreamRef->dvTransmitterFrameRefArray);
	}
	else
		return nil;
}

//////////////////////////////////////////////////////////
// FWAVCDVTransmitterFrameGetDataBuf
//////////////////////////////////////////////////////////
void FWAVCDVTransmitterFrameGetDataBuf(FWAVCDVTransmitterFrameRef fwavcDVTransmitterFrameRef, UInt8 **ppFrameData, UInt32 *pFrameLen)
{
	DVTransmitFrame *pDVTransmitFrame = (DVTransmitFrame*) fwavcDVTransmitterFrameRef;
	*ppFrameData = pDVTransmitFrame->pFrameData; 
	*pFrameLen = pDVTransmitFrame->frameLen;
}

//////////////////////////////////////////////////////////
// FWAVCDVTransmitterFrameGetSYTTime
//////////////////////////////////////////////////////////
void FWAVCDVTransmitterFrameGetSYTTime(FWAVCDVTransmitterFrameRef fwavcDVTransmitterFrameRef, UInt32 *pFrameSYTTime)
{
	DVTransmitFrame *pDVTransmitFrame = (DVTransmitFrame*) fwavcDVTransmitterFrameRef;
	*pFrameSYTTime = pDVTransmitFrame->frameSYTTime; 
}

//////////////////////////////////////////////////////////
// FWAVCDVTransmitterFrameGetFireWireTimeStamp
//////////////////////////////////////////////////////////
void FWAVCDVTransmitterFrameGetFireWireTimeStamp(FWAVCDVTransmitterFrameRef fwavcDVTransmitterFrameRef, UInt32 *pFrameTransmitStartCycleTime, Boolean *pTimeStampSecondsFieldValid)
{
	DVTransmitFrame *pDVTransmitFrame = (DVTransmitFrame*) fwavcDVTransmitterFrameRef;
	*pFrameTransmitStartCycleTime = pDVTransmitFrame->frameTransmitStartCycleTime;
	*pTimeStampSecondsFieldValid = (Boolean) pDVTransmitFrame->timeStampSecondsFieldValid;
}

//////////////////////////////////////////////////////////
// FWAVCIsCIPPacket
//////////////////////////////////////////////////////////
Boolean FWAVCIsCIPPacket(UInt32 isochHeaderValue)
{
	return (bool) IsCIPPacket(isochHeaderValue);
}

//////////////////////////////////////////////////////////
// FWAVCParseCIPPacket
//////////////////////////////////////////////////////////
IOReturn FWAVCParseCIPPacket(UInt8 *pPacketPayload, UInt32 payloadLength, FWAVCCIPPacketParserInfo *pParsedPacket)
{
	return ParseCIPPacket(pPacketPayload, payloadLength, pParsedPacket); 
}

//////////////////////////////////////////////////////////
// FWAVCDVReceiverReleaseDVFrame
//////////////////////////////////////////////////////////
IOReturn FWAVCDVReceiverReleaseDVFrame(FWAVCDVReceiverFrameRef fwavcDVReceiverFrameRef)
{
	IOReturn result = kIOReturnError;
	DVReceiveFrame *pFrame = (DVReceiveFrame*) fwavcDVReceiverFrameRef;
	FWAVCDeviceStream *pFWAVCDeviceStream;
	
	if (pFrame)
	{
		pFWAVCDeviceStream = (FWAVCDeviceStream*) pFrame->pFWAVCPrivateData;
	
		result = pFrame->pDVReceiver->releaseFrame(pFrame);

		// Remove the extra retain on the device stream
		FWAVCDeviceStreamRelease(pFWAVCDeviceStream);
	}

	return result;
}

//////////////////////////////////////////////////////////
// FWAVCDVReceiverFrameGetDataBuf
//////////////////////////////////////////////////////////
UInt8 *FWAVCDVReceiverFrameGetDataBuf(FWAVCDVReceiverFrameRef fwavcDVReceiverFrameRef)
{
	DVReceiveFrame *pFrame = (DVReceiveFrame*) fwavcDVReceiverFrameRef;
	return pFrame->pFrameData;
}

//////////////////////////////////////////////////////////
// FWAVCDVReceiverFrameGetLen
//////////////////////////////////////////////////////////
UInt32 FWAVCDVReceiverFrameGetLen(FWAVCDVReceiverFrameRef fwavcDVReceiverFrameRef)
{
	DVReceiveFrame *pFrame = (DVReceiveFrame*) fwavcDVReceiverFrameRef;
	return pFrame->frameLen;
}

//////////////////////////////////////////////////////////
// FWAVCDVReceiverFrameGetDVMode
//////////////////////////////////////////////////////////
UInt8 FWAVCDVReceiverFrameGetDVMode(FWAVCDVReceiverFrameRef fwavcDVReceiverFrameRef)
{
	DVReceiveFrame *pFrame = (DVReceiveFrame*) fwavcDVReceiverFrameRef;
	return pFrame->frameMode;
}

//////////////////////////////////////////////////////////
// FWAVCDVReceiverFrameGetSYTTime
//////////////////////////////////////////////////////////
UInt32 FWAVCDVReceiverFrameGetSYTTime(FWAVCDVReceiverFrameRef fwavcDVReceiverFrameRef)
{
	DVReceiveFrame *pFrame = (DVReceiveFrame*) fwavcDVReceiverFrameRef;
	return pFrame->frameSYTTime;
}

//////////////////////////////////////////////////////////
// FWAVCDVReceiverFrameGetFireWireTimeStamp
//////////////////////////////////////////////////////////
UInt32 FWAVCDVReceiverFrameGetFireWireTimeStamp(FWAVCDVReceiverFrameRef fwavcDVReceiverFrameRef)
{
	DVReceiveFrame *pFrame = (DVReceiveFrame*) fwavcDVReceiverFrameRef;
	return pFrame->frameReceivedTimeStamp;
}

#pragma mark -
#pragma mark ======================================
#pragma mark Front-end APIs for the DVFramer class object
#pragma mark ======================================

//////////////////////////////////////////////////////////
// FWAVCDVFramerCreate
//////////////////////////////////////////////////////////
IOReturn FWAVCDVFramerCreate(FWAVCDVFramerCallback framerCallback,
							 void *pCallbackRefCon,
							 UInt8 initialDVMode,
							 UInt32 initialDVFrameCount,
							 FWAVCDVFramerRef *pFWAVCDVFramerRef,
							 FWAVCDebugLoggingStringHandler debugLogStringProc)
{
	IOReturn result;

	// Allocate a FWAVCDVFramer struct
	FWAVCDVFramer *pFWAVCDVFramer = new FWAVCDVFramer;
	if (!pFWAVCDVFramer)
		return kIOReturnNoMemory;
	
	// Allocate a StringLogger
	pFWAVCDVFramer->pStringLogger = new StringLogger(debugLogStringProc);
	if (!pFWAVCDVFramer->pStringLogger)
	{
		delete pFWAVCDVFramer;
		return kIOReturnNoMemory;
	}
	
	// Initialize
	pthread_mutex_init(&pFWAVCDVFramer->retainReleaseMutex,NULL);
	pFWAVCDVFramer->retainCount = 1;
	pFWAVCDVFramer->framerCallback = framerCallback;
	pFWAVCDVFramer->pCallbackRefCon = pCallbackRefCon;
	
	// Create the DVFramer object
	pFWAVCDVFramer->pDVFramer = new DVFramer(MyDVFramerCallback,
											 pFWAVCDVFramer,
											 (initialDVMode == kFWAVCDeviceStreamDefaultParameter) ?
												kFWAVCDVMode_SD_525_60 : initialDVMode,
											 (initialDVFrameCount == kFWAVCDeviceStreamDefaultParameter) ?
												kFWAVCDVFramerDefaultFrameCount : initialDVFrameCount,
											 pFWAVCDVFramer->pStringLogger);

	if (pFWAVCDVFramer->pDVFramer)
	{
		// Setup the DVFramer
		result = pFWAVCDVFramer->pDVFramer->setupDVFramer();
		if (result != kIOReturnSuccess)
		{
			delete pFWAVCDVFramer->pDVFramer;
			pFWAVCDVFramer->pDVFramer = nil;
		}
	}
	else
		result = kIOReturnNoMemory;
		
	// If we were not successful creating/setting-up the DVFramer, delete the FWAVCDVFramer struct object
	if (!pFWAVCDVFramer->pDVFramer)
	{
		pthread_mutex_destroy(&pFWAVCDVFramer->retainReleaseMutex);
		delete pFWAVCDVFramer->pStringLogger;
		delete pFWAVCDVFramer;
		pFWAVCDVFramer = nil;
	}
	
	*pFWAVCDVFramerRef = pFWAVCDVFramer;

	return result;
}

//////////////////////////////////////////////////////////
// FWAVCDVFramerRetain
//////////////////////////////////////////////////////////
FWAVCDVFramerRef FWAVCDVFramerRetain(FWAVCDVFramerRef fwavcDVFramerRef)
{
	if (fwavcDVFramerRef)
	{
		pthread_mutex_lock(&fwavcDVFramerRef->retainReleaseMutex);
		fwavcDVFramerRef->retainCount += 1;
		pthread_mutex_unlock(&fwavcDVFramerRef->retainReleaseMutex);
	}
	return fwavcDVFramerRef;
}

//////////////////////////////////////////////////////////
// FWAVCDVFramerRelease
//////////////////////////////////////////////////////////
void FWAVCDVFramerRelease(FWAVCDVFramerRef fwavcDVFramerRef)
{
	if (fwavcDVFramerRef)
	{
		pthread_mutex_lock(&fwavcDVFramerRef->retainReleaseMutex);
		fwavcDVFramerRef->retainCount -= 1;
		
		if (fwavcDVFramerRef->retainCount == 0)
		{
			// Delete the DVFramer
			delete fwavcDVFramerRef->pDVFramer;

			// Delete the StringLogger
			delete fwavcDVFramerRef->pStringLogger;
			
			// Destroy the retain/release mutex
			pthread_mutex_destroy(&fwavcDVFramerRef->retainReleaseMutex);
			
			// Delete the FWAVCDeviceController struct
			delete fwavcDVFramerRef;
		}
		else
			pthread_mutex_unlock(&fwavcDVFramerRef->retainReleaseMutex);
	}
}

//////////////////////////////////////////////////////////
// FWAVCDVFramerReset
//////////////////////////////////////////////////////////
IOReturn FWAVCDVFramerReset(FWAVCDVFramerRef fwavcDVFramerRef)
{
	return fwavcDVFramerRef->pDVFramer->resetDVFramer();
}

//////////////////////////////////////////////////////////
// FWAVCDVFramerNextDVSourcePacket
//////////////////////////////////////////////////////////
IOReturn FWAVCDVFramerNextDVSourcePacket(FWAVCDVFramerRef fwavcDVFramerRef,
										 UInt8 *pSourcePacket, 
										 UInt32 packetLen, 
										 UInt8 dvMode, 
										 UInt16 syt, 
										 UInt32 packetTimeStamp,
										 UInt64 packetU64TimeStamp)
{
	return fwavcDVFramerRef->pDVFramer->nextDVSourcePacket(pSourcePacket,packetLen,dvMode,syt,packetTimeStamp,packetU64TimeStamp);
}

//////////////////////////////////////////////////////////
// FWAVCDVFramerReturnDVFrame
//////////////////////////////////////////////////////////
IOReturn FWAVCDVFramerReturnDVFrame(FWAVCDVFramerFrameRef fwavcDVFramerFrameRef)
{
	IOReturn result = kIOReturnSuccess;
	DVFrame *pDVFrame = (DVFrame*) fwavcDVFramerFrameRef;
	FWAVCDVFramer *pFWAVCDVFramer;
	
	if (pDVFrame)
	{
		pFWAVCDVFramer = (FWAVCDVFramer*) pDVFrame->pFWAVCPrivateData;
		result = pDVFrame->pDVFramer->ReleaseDVFrame(pDVFrame);

		// Release the extra retain on the FWAVCDVFramer
		FWAVCDVFramerRelease(pFWAVCDVFramer);
	}
	
	return result;
}

//////////////////////////////////////////////////////////
// FWAVCDVFramerFrameGetFramer
//////////////////////////////////////////////////////////
FWAVCDVFramerRef FWAVCDVFramerFrameGetFramer(FWAVCDVFramerFrameRef fwavcDVFramerFrameRef)
{
	DVFrame *pDVFrame = (DVFrame*) fwavcDVFramerFrameRef;
	return (FWAVCDVFramerRef) pDVFrame->pFWAVCPrivateData;
}

//////////////////////////////////////////////////////////
// FWAVCDVFramerFrameGetFrameDataBuf
//////////////////////////////////////////////////////////
UInt8 *FWAVCDVFramerFrameGetFrameDataBuf(FWAVCDVFramerFrameRef fwavcDVFramerFrameRef)
{
	DVFrame *pDVFrame = (DVFrame*) fwavcDVFramerFrameRef;
	return pDVFrame->pFrameData;
}

//////////////////////////////////////////////////////////
// FWAVCDVFramerFrameGetLen
//////////////////////////////////////////////////////////
UInt32 FWAVCDVFramerFrameGetLen(FWAVCDVFramerFrameRef fwavcDVFramerFrameRef)
{
	DVFrame *pDVFrame = (DVFrame*) fwavcDVFramerFrameRef;
	return pDVFrame->frameLen;
}

//////////////////////////////////////////////////////////
// FWAVCDVFramerFrameGetDVMode
//////////////////////////////////////////////////////////
UInt8 FWAVCDVFramerFrameGetDVMode(FWAVCDVFramerFrameRef fwavcDVFramerFrameRef)
{
	DVFrame *pDVFrame = (DVFrame*) fwavcDVFramerFrameRef;
	return pDVFrame->frameMode;
}

//////////////////////////////////////////////////////////
// FWAVCDVFramerFrameGetSYTTime
//////////////////////////////////////////////////////////
UInt32 FWAVCDVFramerFrameGetSYTTime(FWAVCDVFramerFrameRef fwavcDVFramerFrameRef)
{
	DVFrame *pDVFrame = (DVFrame*) fwavcDVFramerFrameRef;
	return pDVFrame->frameSYTTime;
}

//////////////////////////////////////////////////////////
// FWAVCDVFramerFrameGetStartTimeStamp
//////////////////////////////////////////////////////////
UInt32 FWAVCDVFramerFrameGetStartTimeStamp(FWAVCDVFramerFrameRef fwavcDVFramerFrameRef)
{
	DVFrame *pDVFrame = (DVFrame*) fwavcDVFramerFrameRef;
	return pDVFrame->packetStartTimeStamp;
}

//////////////////////////////////////////////////////////
// FWAVCDVFramerFrameGetStartU64TimeStamp
//////////////////////////////////////////////////////////
UInt64 FWAVCDVFramerFrameGetStartU64TimeStamp(FWAVCDVFramerFrameRef fwavcDVFramerFrameRef)
{
	DVFrame *pDVFrame = (DVFrame*) fwavcDVFramerFrameRef;
	return pDVFrame->packetStartU64TimeStamp;
}

//////////////////////////////////////////////////////////
// FWAVCDVFramerFrameGetClientPrivateData
//////////////////////////////////////////////////////////
void *FWAVCDVFramerFrameGetClientPrivateData(FWAVCDVFramerFrameRef fwavcDVFramerFrameRef)
{
	DVFrame *pDVFrame = (DVFrame*) fwavcDVFramerFrameRef;
	return pDVFrame->pClientPrivateData;
}

//////////////////////////////////////////////////////////
// FWAVCDVFramerFrameSetClientPrivateData
//////////////////////////////////////////////////////////
void FWAVCDVFramerFrameSetClientPrivateData(FWAVCDVFramerFrameRef fwavcDVFramerFrameRef, void *pClientPrivateData)
{
	DVFrame *pDVFrame = (DVFrame*) fwavcDVFramerFrameRef;
	pDVFrame->pClientPrivateData = pClientPrivateData;
}

//////////////////////////////////////////////////////////
// FWAVCGetDVModeFromFrameData
//////////////////////////////////////////////////////////
IOReturn FWAVCGetDVModeFromFrameData(UInt8 *pDVFrameData, UInt8 *pDVMode, UInt32 *pFrameSize, UInt32 *pSourcePacketSize)
{
	return GetDVModeFromFrameData(pDVFrameData, pDVMode, pFrameSize, pSourcePacketSize);
}

#pragma mark -
#pragma mark ======================================
#pragma mark Front-end APIs for the TSDemuxer class object
#pragma mark ======================================

//////////////////////////////////////////////////////////
// FWAVCTSDemuxerCreate
//////////////////////////////////////////////////////////
IOReturn FWAVCTSDemuxerCreate(FWAVCTSDemuxerCallback pesCallback,
							  void *pCallbackRefCon,
							  UInt32 selectedProgram,
							  UInt32 maxVideoPESSize,
							  UInt32 maxAudioPESSize,
							  UInt32 initialVideoPESBufferCount,
							  UInt32 initialAudioPESBufferCount,
							  FWAVCTSDemuxerRef *pFWAVCTSDemuxerRef,
							  FWAVCDebugLoggingStringHandler debugLogStringProc)
{
	IOReturn result = kIOReturnSuccess;
	
	// Allocate a FWAVCTSDemuxer struct
	FWAVCTSDemuxer *pFWAVCTSDemuxer = new FWAVCTSDemuxer;
	if (!pFWAVCTSDemuxer)
		return kIOReturnNoMemory;
	
	// Allocate a StringLogger
	pFWAVCTSDemuxer->pStringLogger = new StringLogger(debugLogStringProc);
	if (!pFWAVCTSDemuxer->pStringLogger)
	{
		delete pFWAVCTSDemuxer;
		return kIOReturnNoMemory;
	}
	
	// Initialize
	pthread_mutex_init(&pFWAVCTSDemuxer->retainReleaseMutex,NULL);
	pFWAVCTSDemuxer->retainCount = 1;
	pFWAVCTSDemuxer->pesCallback = pesCallback;
	pFWAVCTSDemuxer->pCallbackRefCon = pCallbackRefCon;
	pFWAVCTSDemuxer->autoPSI = true;
	
	// Create the TSDemuxer object
	pFWAVCTSDemuxer->pTSDemuxer = new TSDemuxer(MyTSDemuxerCallback,
												pFWAVCTSDemuxer,
												nil,
												nil,
												selectedProgram,
												(maxVideoPESSize == kFWAVCDeviceStreamDefaultParameter) ?
													kFWAVCTSDemuxerMaxVideoPESSizeDefault : maxVideoPESSize,
												(maxAudioPESSize == kFWAVCDeviceStreamDefaultParameter) ?
													kFWAVCTSDemuxerMaxAudioPESSizeDefault : maxAudioPESSize,
												(initialVideoPESBufferCount == kFWAVCDeviceStreamDefaultParameter) ?
													kFWAVCTSDemuxerDefaultVideoPESBufferCount : initialVideoPESBufferCount,
												(initialAudioPESBufferCount == kFWAVCDeviceStreamDefaultParameter) ?
													kFWAVCTSDemuxerDefaultAudioPESBufferCount : initialAudioPESBufferCount,
												pFWAVCTSDemuxer->pStringLogger);
		
	// If we were not successful creating the TSDemuxer, delete the FWAVCTSDemuxer struct object
	if (!pFWAVCTSDemuxer->pTSDemuxer)
	{
		result = kIOReturnNoMemory;
		pthread_mutex_destroy(&pFWAVCTSDemuxer->retainReleaseMutex);
		delete pFWAVCTSDemuxer->pStringLogger;
		delete pFWAVCTSDemuxer;
		pFWAVCTSDemuxer = nil;
	}
	
	*pFWAVCTSDemuxerRef = pFWAVCTSDemuxer;

	return result;
}

//////////////////////////////////////////////////////////
// FWAVCTSDemuxerCreateWithPIDs
//////////////////////////////////////////////////////////
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
{
	IOReturn result = kIOReturnSuccess;
	
	// Allocate a FWAVCTSDemuxer struct
	FWAVCTSDemuxer *pFWAVCTSDemuxer = new FWAVCTSDemuxer;
	if (!pFWAVCTSDemuxer)
		return kIOReturnNoMemory;
	
	// Allocate a StringLogger
	pFWAVCTSDemuxer->pStringLogger = new StringLogger(debugLogStringProc);
	if (!pFWAVCTSDemuxer->pStringLogger)
	{
		delete pFWAVCTSDemuxer;
		return kIOReturnNoMemory;
	}
	
	// Initialize
	pthread_mutex_init(&pFWAVCTSDemuxer->retainReleaseMutex,NULL);
	pFWAVCTSDemuxer->retainCount = 1;
	pFWAVCTSDemuxer->pesCallback = pesCallback;
	pFWAVCTSDemuxer->pCallbackRefCon = pCallbackRefCon;
	pFWAVCTSDemuxer->autoPSI = false;
	pFWAVCTSDemuxer->videoPid = videoPid;
	pFWAVCTSDemuxer->audioPid = audioPid;
	
	// Create the TSDemuxer object
	pFWAVCTSDemuxer->pTSDemuxer = new TSDemuxer(videoPid,
												audioPid,
												kIgnoreStream,
												MyTSDemuxerCallback,
												pFWAVCTSDemuxer,
												nil,
												nil,
												(maxVideoPESSize == kFWAVCDeviceStreamDefaultParameter) ?
												kFWAVCTSDemuxerMaxVideoPESSizeDefault : maxVideoPESSize,
												(maxAudioPESSize == kFWAVCDeviceStreamDefaultParameter) ?
												kFWAVCTSDemuxerMaxAudioPESSizeDefault : maxAudioPESSize,
												(initialVideoPESBufferCount == kFWAVCDeviceStreamDefaultParameter) ?
												kFWAVCTSDemuxerDefaultVideoPESBufferCount : initialVideoPESBufferCount,
												(initialAudioPESBufferCount == kFWAVCDeviceStreamDefaultParameter) ?
												kFWAVCTSDemuxerDefaultAudioPESBufferCount : initialAudioPESBufferCount,
												pFWAVCTSDemuxer->pStringLogger);
	
	// If we were not successful creating the TSDemuxer, delete the FWAVCTSDemuxer struct object
	if (!pFWAVCTSDemuxer->pTSDemuxer)
	{
		result = kIOReturnNoMemory;
		pthread_mutex_destroy(&pFWAVCTSDemuxer->retainReleaseMutex);
		delete pFWAVCTSDemuxer->pStringLogger;
		delete pFWAVCTSDemuxer;
		pFWAVCTSDemuxer = nil;
	}
	
	*pFWAVCTSDemuxerRef = pFWAVCTSDemuxer;
	
	return result;
}

//////////////////////////////////////////////////////////
// FWAVCTSDemuxerRetain
//////////////////////////////////////////////////////////
FWAVCTSDemuxerRef FWAVCTSDemuxerRetain(FWAVCTSDemuxerRef fwavcTSDemuxerRef)
{
	if (fwavcTSDemuxerRef)
	{
		pthread_mutex_lock(&fwavcTSDemuxerRef->retainReleaseMutex);
		fwavcTSDemuxerRef->retainCount += 1;
		pthread_mutex_unlock(&fwavcTSDemuxerRef->retainReleaseMutex);
	}
	return fwavcTSDemuxerRef;
}

//////////////////////////////////////////////////////////
// FWAVCTSDemuxerRelease
//////////////////////////////////////////////////////////
void FWAVCTSDemuxerRelease(FWAVCTSDemuxerRef fwavcTSDemuxerRef)
{
	if (fwavcTSDemuxerRef)
	{
		pthread_mutex_lock(&fwavcTSDemuxerRef->retainReleaseMutex);
		fwavcTSDemuxerRef->retainCount -= 1;
		
		if (fwavcTSDemuxerRef->retainCount == 0)
		{
			// Delete the TSDemuxer
			delete fwavcTSDemuxerRef->pTSDemuxer;
			
			// Delete the StringLogger
			delete fwavcTSDemuxerRef->pStringLogger;
			
			// Destroy the retain/release mutex
			pthread_mutex_destroy(&fwavcTSDemuxerRef->retainReleaseMutex);
			
			// Delete the FWAVCTSDemuxer struct object
			delete fwavcTSDemuxerRef;
		}
		else
			pthread_mutex_unlock(&fwavcTSDemuxerRef->retainReleaseMutex);
	}
}

//////////////////////////////////////////////////////////
// FWAVCTSDemuxerNextTSPacket
//////////////////////////////////////////////////////////
IOReturn FWAVCTSDemuxerNextTSPacket(FWAVCTSDemuxerRef fwavcTSDemuxerRef, 
									UInt8 *pPacket, 
									UInt32 packetTimeStamp, 
									UInt64 packetU64TimeStamp)
{
	return fwavcTSDemuxerRef->pTSDemuxer->nextTSPacket(pPacket, packetTimeStamp, packetU64TimeStamp);
}

//////////////////////////////////////////////////////////
// FWAVCTSDemuxerReset
//////////////////////////////////////////////////////////
IOReturn FWAVCTSDemuxerReset(FWAVCTSDemuxerRef fwavcTSDemuxerRef)
{
	if 	(fwavcTSDemuxerRef->autoPSI == true)		
		return fwavcTSDemuxerRef->pTSDemuxer->resetTSDemuxer();
	else
		return fwavcTSDemuxerRef->pTSDemuxer->resetTSDemuxer(fwavcTSDemuxerRef->videoPid,fwavcTSDemuxerRef->audioPid,kIgnoreStream);
}

//////////////////////////////////////////////////////////
// FWAVCTSDemuxerFlush
//////////////////////////////////////////////////////////
void FWAVCTSDemuxerFlush(FWAVCTSDemuxerRef fwavcTSDemuxerRef)
{
	fwavcTSDemuxerRef->pTSDemuxer->Flush();
}

//////////////////////////////////////////////////////////
// FWAVCTSDemuxerReturnPESPacket
//////////////////////////////////////////////////////////
IOReturn FWAVCTSDemuxerReturnPESPacket(FWAVCTSDemuxerPESPacketRef fwavcTSDemuxerPesPacketRef)
{
	IOReturn result;
	
	PESPacketBuf *pPESPacketBuf = (PESPacketBuf*) fwavcTSDemuxerPesPacketRef;
	FWAVCTSDemuxer *pFWAVCTSDemuxer = (FWAVCTSDemuxer*) pPESPacketBuf->pFWAVCPrivateData;

	result = pPESPacketBuf->pTSDemuxer->ReleasePESPacketBuf(pPESPacketBuf);

	// Release the extra retain on the FWAVCTSDemuxer
	FWAVCTSDemuxerRelease(pFWAVCTSDemuxer);
	
	return result;
}

//////////////////////////////////////////////////////////
// FWAVCTSDemuxerSetHDV2VAuxCallback
//////////////////////////////////////////////////////////
void FWAVCTSDemuxerSetHDV2VAuxCallback(FWAVCTSDemuxerRef fwavcTSDemuxerRef,
									   FWAVCTSDemuxerHDV2VAUXCallback fVAuxCallback, 
									   void *pRefCon)
{
	fwavcTSDemuxerRef->fVAuxCallback = fVAuxCallback;
	fwavcTSDemuxerRef->pVAuxRefCon = pRefCon;
	
	fwavcTSDemuxerRef->pTSDemuxer->InstallHDV2VAuxCallback(MyHDV2VAUXCallback, fwavcTSDemuxerRef);
}

//////////////////////////////////////////////////////////
// FWAVCTSDemuxerGetPMTInfo
//////////////////////////////////////////////////////////
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
{
	if (!fwavcTSDemuxerRef->pTSDemuxer->psiTables)
		return kIOReturnError;

	if (pPrimaryProgramPMTPid)
		*pPrimaryProgramPMTPid = (fwavcTSDemuxerRef->pTSDemuxer->psiTables->primaryProgramPmtPid & 0x0000FFFF);
				
	if (pPrimaryProgramPMTVersion)
		*pPrimaryProgramPMTVersion = (fwavcTSDemuxerRef->pTSDemuxer->psiTables->primaryPMTVersion & 0x000000FF);
	
	if (pProgramVideoPid)
		*pProgramVideoPid = (fwavcTSDemuxerRef->pTSDemuxer->psiTables->primaryProgramVideoPid & 0x0000FFFF);

	if (pProgramVideoStreamType)
		*pProgramVideoStreamType = fwavcTSDemuxerRef->pTSDemuxer->psiTables->primaryProgramVideoStreamType;
	
	if (pProgramAudioPid)
		*pProgramAudioPid = (fwavcTSDemuxerRef->pTSDemuxer->psiTables->primaryProgramAudioPid & 0x0000FFFF);
		
	if (pProgramAudioStreamType)
		*pProgramAudioStreamType = fwavcTSDemuxerRef->pTSDemuxer->psiTables->primaryProgramAudioStreamType;
	
	if (pProgramDescriptors)
	{
		if (fwavcTSDemuxerRef->pTSDemuxer->psiTables->primaryProgramDescriptorsLen > *pProgramDescriptorsLen)
			return kIOReturnNoMemory;
		else
		{
			if (fwavcTSDemuxerRef->pTSDemuxer->psiTables->primaryProgramDescriptorsLen > 0)
				bcopy(fwavcTSDemuxerRef->pTSDemuxer->psiTables->pPrimaryProgramDescriptors,
					  pProgramDescriptors,
					  fwavcTSDemuxerRef->pTSDemuxer->psiTables->primaryProgramDescriptorsLen);
				
			*pProgramDescriptorsLen = fwavcTSDemuxerRef->pTSDemuxer->psiTables->primaryProgramDescriptorsLen;
		}	
	}
	
	if (pVideoESDescriptors)
	{
		if (fwavcTSDemuxerRef->pTSDemuxer->psiTables->primaryVideoESDescriptorsLen > *pVideoESDescriptorsLen)
			return kIOReturnNoMemory;
		else
		{
			if (fwavcTSDemuxerRef->pTSDemuxer->psiTables->primaryVideoESDescriptorsLen > 0)
				bcopy(fwavcTSDemuxerRef->pTSDemuxer->psiTables->pPrimaryVideoESDescriptors,
					  pVideoESDescriptors,
					  fwavcTSDemuxerRef->pTSDemuxer->psiTables->primaryVideoESDescriptorsLen);
			
			*pVideoESDescriptorsLen = fwavcTSDemuxerRef->pTSDemuxer->psiTables->primaryVideoESDescriptorsLen;
		}	
	}

	if (pAudioESDescriptors)
	{
		if (fwavcTSDemuxerRef->pTSDemuxer->psiTables->primaryAudioESDescriptorsLen > *pAudioESDescriptorsLen)
			return kIOReturnNoMemory;
		else
		{
			if (fwavcTSDemuxerRef->pTSDemuxer->psiTables->primaryAudioESDescriptorsLen > 0)
				bcopy(fwavcTSDemuxerRef->pTSDemuxer->psiTables->pPrimaryAudioESDescriptors,
					  pAudioESDescriptors,
					  fwavcTSDemuxerRef->pTSDemuxer->psiTables->primaryAudioESDescriptorsLen);
			
			*pAudioESDescriptorsLen = fwavcTSDemuxerRef->pTSDemuxer->psiTables->primaryAudioESDescriptorsLen;
		}	
	}
	
	return kIOReturnSuccess;
}


//////////////////////////////////////////////////////////
// FWAVCTSDemuxerSetHDV1PackDataCallback
//////////////////////////////////////////////////////////
void FWAVCTSDemuxerSetHDV1PackDataCallback(FWAVCTSDemuxerRef fwavcTSDemuxerRef,
										   FWAVCTSDemuxerHDV1PackDataCallback fPackDataCallback, 
										   void *pRefCon)
{
	fwavcTSDemuxerRef->fPackDataCallback = fPackDataCallback; 
	fwavcTSDemuxerRef->pPackDataRefCon = pRefCon;

	fwavcTSDemuxerRef->pTSDemuxer->InstallHDV1PackDataCallback(MyHDV1PackDataCallback, fwavcTSDemuxerRef);
}

//////////////////////////////////////////////////////////
// FWAVCTSDemuxerPESPacketGetDemuxer
//////////////////////////////////////////////////////////
FWAVCTSDemuxerRef FWAVCTSDemuxerPESPacketGetDemuxer(FWAVCTSDemuxerPESPacketRef fwavcTSDemuxerPESPacketRef)
{
	PESPacketBuf* pPESBuf = (PESPacketBuf*) fwavcTSDemuxerPESPacketRef;
	return (FWAVCTSDemuxerRef) pPESBuf->pFWAVCPrivateData;
}

//////////////////////////////////////////////////////////
// FWAVCTSDemuxerPESPacketGetStreamType
//////////////////////////////////////////////////////////
FWAVCTSDemuxerPESPacketStreamType FWAVCTSDemuxerPESPacketGetStreamType(FWAVCTSDemuxerPESPacketRef fwavcTSDemuxerPESPacketRef)
{
	PESPacketBuf* pPESBuf = (PESPacketBuf*) fwavcTSDemuxerPESPacketRef;
	return (FWAVCTSDemuxerPESPacketStreamType) pPESBuf->streamType;
}

//////////////////////////////////////////////////////////
// FWAVCTSDemuxerPESPacketGetPESBuf
//////////////////////////////////////////////////////////
UInt8 *FWAVCTSDemuxerPESPacketGetPESBuf(FWAVCTSDemuxerPESPacketRef fwavcTSDemuxerPESPacketRef)
{
	PESPacketBuf* pPESBuf = (PESPacketBuf*) fwavcTSDemuxerPESPacketRef;
	return pPESBuf->pPESBuf;
}

//////////////////////////////////////////////////////////
// FWAVCTSDemuxerPESPacketGetLen
//////////////////////////////////////////////////////////
UInt32 FWAVCTSDemuxerPESPacketGetLen(FWAVCTSDemuxerPESPacketRef fwavcTSDemuxerPESPacketRef)
{
	PESPacketBuf* pPESBuf = (PESPacketBuf*) fwavcTSDemuxerPESPacketRef;
	return pPESBuf->pesBufLen; 
}

//////////////////////////////////////////////////////////
// FWAVCTSDemuxerPESPacketGetPid
//////////////////////////////////////////////////////////
UInt32 FWAVCTSDemuxerPESPacketGetPid(FWAVCTSDemuxerPESPacketRef fwavcTSDemuxerPESPacketRef)
{
	PESPacketBuf* pPESBuf = (PESPacketBuf*) fwavcTSDemuxerPESPacketRef;
	return pPESBuf->pid;
}

//////////////////////////////////////////////////////////
// FWAVCTSDemuxerPESPacketGetStartTimeStamp
//////////////////////////////////////////////////////////
UInt32 FWAVCTSDemuxerPESPacketGetStartTimeStamp(FWAVCTSDemuxerPESPacketRef fwavcTSDemuxerPESPacketRef)
{
	PESPacketBuf* pPESBuf = (PESPacketBuf*) fwavcTSDemuxerPESPacketRef;
	return pPESBuf->startTSPacketTimeStamp;
}

//////////////////////////////////////////////////////////
// FWAVCTSDemuxerPESPacketGetStartU64TimeStamp
//////////////////////////////////////////////////////////
UInt64 FWAVCTSDemuxerPESPacketGetStartU64TimeStamp(FWAVCTSDemuxerPESPacketRef fwavcTSDemuxerPESPacketRef)
{
	PESPacketBuf* pPESBuf = (PESPacketBuf*) fwavcTSDemuxerPESPacketRef;
	return pPESBuf->startTSPacketU64TimeStamp;
}

//////////////////////////////////////////////////////////
// FWAVCTSDemuxerPESPacketGetClientPrivateData
//////////////////////////////////////////////////////////
void *FWAVCTSDemuxerPESPacketGetClientPrivateData(FWAVCTSDemuxerPESPacketRef fwavcTSDemuxerPESPacketRef)
{
	PESPacketBuf* pPESBuf = (PESPacketBuf*) fwavcTSDemuxerPESPacketRef;
	return pPESBuf->pClientPrivateData;
}

//////////////////////////////////////////////////////////
// FWAVCTSDemuxerPESPacketSetClientPrivateData
//////////////////////////////////////////////////////////
void FWAVCTSDemuxerPESPacketSetClientPrivateData(FWAVCTSDemuxerPESPacketRef fwavcTSDemuxerPESPacketRef, void *pClientPrivateData)
{
	PESPacketBuf* pPESBuf = (PESPacketBuf*) fwavcTSDemuxerPESPacketRef;
	pPESBuf->pClientPrivateData = pClientPrivateData; 
}
