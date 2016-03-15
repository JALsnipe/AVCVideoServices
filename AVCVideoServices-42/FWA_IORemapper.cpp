/*
	File:		FWA_IORemapper.cpp
 
 Synopsis: This is implementation file for the FWA_IORemapper class object.
 
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
	
////////////////////////////////////////////
// FWA_IORemapper Constructor
////////////////////////////////////////////
FWA_IORemapper::FWA_IORemapper(UInt8 isochInChan,
							   UInt8 isochOutChan,
							   UInt8 numSubStreamsInOutput,
							   UInt8 *pInputSubStreamIndexes,
							   IOFWSpeed transmitterFWSpeed,
							   Boolean doIRMForTransmitter)
{
	isochInChannel = isochInChan;
	isochOutChannel = isochOutChan;
	transmitterSpeed = transmitterFWSpeed;
	doTransmitterIRM = doIRMForTransmitter;
	
	receiver = nil;
	transmitter = nil;

	numSubStreams = numSubStreamsInOutput;
	maxSubStreams = numSubStreamsInOutput;
	bcopy(pInputSubStreamIndexes,inputSubStreamIndexes,numSubStreams);
		
	transmitterCycleBufferFullSize = ((numSubStreams*4*kRemapperNumSourcePacketsPerCycle)+8);
}

////////////////////////////////////////////
// FWA_IORemapper Destructor
////////////////////////////////////////////
FWA_IORemapper::~FWA_IORemapper()
{
	// Should be safe to call stop() here, even if we aren't running!
	stop();
}

////////////////////////////////////////////
// FWA_IORemapper::start
////////////////////////////////////////////
IOReturn FWA_IORemapper::start(void)
{
    IOReturn result = kIOReturnSuccess ;

	//printf("Starting FWA_IORemapper!\n");
	
	lastReceiveCycle = 0xFFFFFFFF;
	
	for (unsigned int i = 0 ; i < kRemapperNumCyclesInTransmitProgram; i++)
		transmitCycleBufLen[i] = 8;
	
	result = CreateUniversalTransmitter(&transmitter,
										TransmitterDataPullProc,
										this,
										TransmitterMessageReceivedProc,
										this,
										nil, // &logger,
										nil,
										kRemapperNumCyclesPerUniversalTransmitSegment,
										kRemapperNumUniversalTransmitSegments,
										((kRemapperNumCyclesInTransmitProgram*transmitterCycleBufferFullSize)+8),
										doTransmitterIRM,
										(8+(32*4*numSubStreams)),	// Note: This assumes sample-rate up to 192KHz!
										0);	// No cycle-match value. Start immediately.
	if (!transmitter)
	{
		//printf("Error creating UniveralTransmitter object: %d\n",result);
		return kIOReturnNoResources;
	}
	
	// Set the channel to transmit on
	transmitter->setTransmitIsochChannel(isochOutChannel);
	//printf("Transmit on channel: %d\n",isochOutChannel);
	
	transmitter->setTransmitIsochSpeed(transmitterSpeed);
	
	// Save a pointer to the base of the transmitter's transmit buffer
	pTransmitterTransmitBuffer = transmitter->getClientBufferPointer();
	
	// Save a pointer to each cycle buffer in the transmitter buffer
	for (unsigned int i=0;i <kRemapperNumCyclesInTransmitProgram;i++)
		pTransmitCycleBuf[i] = &pTransmitterTransmitBuffer[transmitterCycleBufferFullSize*i];
	
	// Before we lock to an incoming stream, we need send CIP only packets. Initialize the CIP-only buffer
	pCIPOnlyBuffer = (UInt32*) &pTransmitterTransmitBuffer[transmitter->getClientBufferLen()-8];
	pCIPOnlyBuffer[0] = EndianU32_NtoB(0x01000060 + (numSubStreams << 16));	//TODO: should set the SID correctly!
	pCIPOnlyBuffer[1] = EndianU32_NtoB(0x90FFFFFF);
	
	// Start the transmitter
	transmitter->startTransmit();
	
	// Use the UniversalReceiver helper function to create the
	// UniversalReceiver object and dedicated real-time thread.
	result = CreateUniversalReceiver(&receiver,
									 nil,	// Don't pass in a default data-push callback, we'll specify one later!
									 this,
									 ReceiverMessageReceivedProc,
									 this,
									 nil, // &logger,
									 nil,
									 kRemapperNumCyclesPerUniversalReceiveSegment,
									 kRemapperNumUniversalReceiveSegments,
									 kUniversalReceiverDefaultBufferSize,
									 false,
									 kUniversalReceiverDefaultBufferSize);
	if (!receiver)
	{
		//printf("Error creating UniveralReceiver object: %d\n",result);
		return kIOReturnNoResources;
	}
	
	// Register a data-receive callback (using structure method and specifying only one callback per DCL program segment
	receiver->registerStructuredDataPushCallback(DataReceiveCallback, kCyclesPerUniversalReceiveSegment, this);
	
	// Register a no-data notification callback, for noitification of 5 seconds of no received isoch!
	receiver->registerNoDataNotificationCallback(ReceiverNoDataProc, this, 5000);
	
	// Set the channel to receive on
	receiver->setReceiveIsochChannel(isochInChannel);
	//printf("Receive on channel: %d\n",isochInChannel);
	
	// Start the receiver
	result = receiver->startReceive();
	
	return result;
}

////////////////////////////////////////////
// FWA_IORemapper::stop
////////////////////////////////////////////
IOReturn FWA_IORemapper::stop(void)
{
	if (receiver)
	{
		if (receiver->transportState == kUniversalReceiverTransportRecording)
			receiver->stopReceive();
		lastReceiveCycle = 0xFFFFFFFF;
		DestroyUniversalReceiver(receiver);
		receiver = nil;
	}

	if (transmitter)
	{
		if (transmitter->transportState == kUniversalTransmitterTransportPlaying)
			transmitter->stopTransmit();
		DestroyUniversalTransmitter(transmitter);
		transmitter = nil;
	}

	return kIOReturnSuccess;
}

////////////////////////////////////////////
// FWA_IORemapper::setNewIOMap
////////////////////////////////////////////
IOReturn FWA_IORemapper::setNewIOMap(UInt8 numSubStreamsInOutput, UInt8 *pInputSubStreamIndexes)
{
	// Make sure the new IO map doesn't have more signals than what can fit in our transmit buffers
	if (numSubStreamsInOutput > maxSubStreams)
		return kIOReturnNoSpace;
	
	// Copy the new IO map
	bcopy(pInputSubStreamIndexes,inputSubStreamIndexes,numSubStreams);
	numSubStreams = numSubStreamsInOutput;
	return kIOReturnSuccess;
}

//////////////////////////////////////////////////////
// FWA_IORemapper::ReceiverMessageReceivedProc
//////////////////////////////////////////////////////
void FWA_IORemapper::ReceiverMessageReceivedProc(UInt32 msg, UInt32 param1, UInt32 param2, void *pRefCont)
{
	//printf("Message from UniversalReceiver: %d\n",(int) msg);
	return;
}

//////////////////////////////////////////////////////
// FWA_IORemapper::TransmitterMessageReceivedProc
//////////////////////////////////////////////////////
void FWA_IORemapper::TransmitterMessageReceivedProc(UInt32 msg, UInt32 param1, UInt32 param2, void *pRefCon)
{
	FWA_IORemapper *pRemapper = (FWA_IORemapper*) pRefCon;
	
	//printf("Message from UniversalTransmitter: %d\n",(int) msg);
	
	// Handle DCL overruns here.
	if ((pRemapper->receiver) && (msg == kUniversalTransmitterPreparePacketFetcher))
	{
		pRemapper->receiver->stopReceive();
		pRemapper->lastReceiveCycle = 0xFFFFFFFF;
		pRemapper->receiver->startReceive();
	}

	return;
}


//////////////////////////////////////////////////////
// FWA_IORemapper::DataReceiveCallback
//////////////////////////////////////////////////////
IOReturn FWA_IORemapper::DataReceiveCallback(UInt32 CycleDataCount, UniversalReceiveCycleData *pCycleData, void *pRefCon)
{
	IOReturn result = kIOReturnSuccess ;
	CIPPacketParserStruct parsedPacket;
	UInt32 cycleBufIndex;
	
	FWA_IORemapper *pRemapper = (FWA_IORemapper*) pRefCon;

	for (unsigned int cycle=0;cycle<CycleDataCount;cycle++)
	{
		// Only process CIP-style packet. 
		if (IsCIPPacket(pCycleData[cycle].isochHeader))
		{
			result = ParseCIPPacket(pCycleData[cycle].pPayload, pCycleData[cycle].payloadLength, &parsedPacket);
			if (result != kIOReturnSuccess)
			{
				// The only way this can fail, is if there is more source
				// packets in the CIP-packet than what we ever expect.
				// (basically, just a sanity check here!)
				//printf("ParseCIPPacket failed. Result: 0x%08X\n",result);
				continue;
			}
			
			if (parsedPacket.fmt == k61883Fmt_AudioMidi)
			{
				//printf("Received AM824: fdf=0x%02X   numberOfSubStreams=%-3d   samplesPerSubStream=%-3d   FWReceivedCycle=%-4d\n",
				//	   parsedPacket.fdf,
				//	   (parsedPacket.sourcePacketSize/4),
				//	   parsedPacket.numSourcePackets,
				//	   ((pCycleData[cycle].fireWireTimeStamp & 0x1FFF000) >> 12));

				// Determine the number of substreams in the source packet 
				UInt32 numberSourceSubStreams = (parsedPacket.sourcePacketSize/4);
				
				// Determine the correct spot in the transmitters buffers for this received data
				cycleBufIndex = ((((pCycleData[cycle].fireWireTimeStamp & 0x1FFF000) >> 12)+kRemapperInToOutLatencyInCycles)%(kRemapperNumCyclesInTransmitProgram));
				
				// Copy the CIP header over
				bcopy( pCycleData[cycle].pPayload, pRemapper->pTransmitCycleBuf[cycleBufIndex],8);
				
				// Manipulate the CIP header in the transmit buffer to set the correct DBS
				UInt32 *pCIP = (UInt32*) pRemapper->pTransmitCycleBuf[cycleBufIndex];
				pCIP[0] = EndianU32_NtoB((EndianU32_BtoN(pCIP[0]) & 0xFF00FFFF) | (pRemapper->numSubStreams << 16)); 
				UInt32 *pDestPacketWord = &pCIP[2];
				
				// Copy samples from the source packets to the dest locations
				for (unsigned int sourcePacket=0;sourcePacket<parsedPacket.numSourcePackets;sourcePacket++)
				{
					UInt32 *pSourcePacketWord = (UInt32*) parsedPacket.pSourcePacket[sourcePacket];
					
					for (unsigned int subStream = 0; subStream < pRemapper->numSubStreams; subStream++)
					{
						if (pRemapper->inputSubStreamIndexes[subStream] < numberSourceSubStreams)
							*pDestPacketWord = pSourcePacketWord[pRemapper->inputSubStreamIndexes[subStream]]; 
						else
							*pDestPacketWord = EndianU32_NtoB(0x40000000);
						
						pDestPacketWord++;
					}
				}
				
				if (parsedPacket.numSourcePackets)
					pRemapper->transmitCycleBufLen[cycleBufIndex] = (8+(parsedPacket.numSourcePackets*pRemapper->numSubStreams*4));
				else
					pRemapper->transmitCycleBufLen[cycleBufIndex] = 8;
				pRemapper->lastReceiveCycle = ((pCycleData[cycle].fireWireTimeStamp & 0x1FFF000) >> 12);
			}
		}
	}
	
	return kIOReturnSuccess;
}

//////////////////////////////////////////////////////
// FWA_IORemapper::ReceiverNoDataProc
//////////////////////////////////////////////////////
IOReturn FWA_IORemapper::ReceiverNoDataProc(void *pRefCon)
{
	// Called when the univeral receiver doesn't get any data for the specified amount of time.
	FWA_IORemapper *pRemapper = (FWA_IORemapper*) pRefCon;
	
	//printf("ReceiverNoDataProc called!\n");
	pRemapper->lastReceiveCycle = 0xFFFFFFFF;
	return kIOReturnSuccess;
}

//////////////////////////////////////////////////////
// FWA_IORemapper::TransmitterDataPullProc
//////////////////////////////////////////////////////
IOReturn FWA_IORemapper::TransmitterDataPullProc(UniversalTransmitterCycleInfo *pCycleInfo, class UniversalTransmitter *pTransmitter, void *pRefCon)
{
	FWA_IORemapper *pRemapper = (FWA_IORemapper*) pRefCon;

	int receiverToTransmitterDelta;
	UInt32 cycle = ((pCycleInfo->expectedTransmitCycleTime & 0x1FFF000)>> 12);
	//printf("TransmitterDataPullProc: Pulling data for cycle:%-4d (last processed receive data for cycle: %d)\n",cycle,pRemapper->lastReceiveCycle);

	if ((pRemapper->lastReceiveCycle == 0xFFFFFFFF) || (pRemapper->transmitter->transportState == kUniversalTransmitterTransportStopped))
	{
		pCycleInfo->numRanges = 1;
		pCycleInfo->ranges[0].address = (IOVirtualAddress) pRemapper->pCIPOnlyBuffer;
		pCycleInfo->ranges[0].length = (IOByteCount) 8 ;
	}
	else
	{
		// Here' we delay (if needed) waiting to receive the data to be transmitted this cycle!
		while (1)
		{
			receiverToTransmitterDelta = cycle - pRemapper->lastReceiveCycle;
			if (receiverToTransmitterDelta < 0)
				receiverToTransmitterDelta += 8000;
			
			if (receiverToTransmitterDelta < kRemapperInToOutLatencyInCycles)
				break;

			// If we turned off the receiver, or if the receiver stopped receiving packets, we need
			// to bail outg of this loop. We detect that by looking for a lastReceiveCycle of 0xFFFFFFFF
			if (pRemapper->lastReceiveCycle == 0xFFFFFFFF)
				break;
			
			usleep(5);
		}
		
		// Check lastReceiveCycle once more.  
		if (pRemapper->lastReceiveCycle == 0xFFFFFFFF)
		{
			pCycleInfo->numRanges = 1;
			pCycleInfo->ranges[0].address = (IOVirtualAddress) pRemapper->pCIPOnlyBuffer;
			pCycleInfo->ranges[0].length = (IOByteCount) 8 ;
		}
		else
		{
			pCycleInfo->numRanges = 1;
			pCycleInfo->ranges[0].address = (IOVirtualAddress) pRemapper->pTransmitCycleBuf[cycle%(kRemapperNumCyclesInTransmitProgram)];
			pCycleInfo->ranges[0].length = (IOByteCount) pRemapper->transmitCycleBufLen[cycle%(kRemapperNumCyclesInTransmitProgram)];
		}
	}
	
	// Set sy and tag fields for this cycle
	pCycleInfo->sy = 0;
	pCycleInfo->tag = 1;
	
	//printf("TransmitterDataPullProc: Pulled data for cycle:%-4d (last processed receive data for cycle: %d)\n",cycle,pRemapper->lastReceiveCycle);
	
	return kIOReturnSuccess;
}

} // namespace AVS
