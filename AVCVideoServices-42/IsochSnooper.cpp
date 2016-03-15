/*
	File:		IsochSnooper.cpp
 
 Synopsis: This is a simple console mode application that shows an example of using
 the code in FireWireUniversalIsoch for seeing what isoch is active on the bus.
 
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

// Prototypes
void PrintLogMessage(char *pString);
void MessageReceivedProc(UInt32 msg, UInt32 param1, UInt32 param2, void *pRefCont);
IOReturn DataReceiveCallback(UInt32 CycleDataCount, UniversalReceiveCycleData *pCycleData, void *pRefCon);
IOReturn MyNoDataProc(void *pRefCon);

// Globals
volatile bool channelSnoopDone;
unsigned int currentIsochChannel;
unsigned int activeStreamCount;

// These parameters give us a total DCL program length of .5 second
#define kIsochSnooperCyclesPerSegment 100
#define kIsochSnooperNumSegments 40

//////////////////////////////////////////////////////
//
// Main
//
//////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
	// Local Vars
    IOReturn result = kIOReturnSuccess ;
	UniversalReceiver *receiver = nil;
	
	printf("Starting IsochSnooper!\n");
	
	// Alloacate a string logger object and pass it our callback func
	StringLogger logger(PrintLogMessage);

	// Use the UniversalReceiver helper function to create the
	// UniversalReceiver object and dedicated real-time thread.
	// Note: Here we are relying on a number of default parameters.
	result = CreateUniversalReceiver(&receiver,
									 nil,	// Don't pass in a default data-push callback, we'll specify one later
									 nil,
									 MessageReceivedProc,
									 nil,
									 &logger,
									 nil,
									 kIsochSnooperCyclesPerSegment,
									 kIsochSnooperNumSegments); 
	if (!receiver)
	{
		printf("Error creating UniveralReceiver object: %d\n",result);
		return -1;
	}
	
	// Register a data-receive callback (using structure method to allow only one callback per DCL program segment
	receiver->registerStructuredDataPushCallback(DataReceiveCallback, kIsochSnooperCyclesPerSegment, nil);
	
	// Register a no-data notification callback, for noitification of .25 seconds of no received isoch!
	receiver->registerNoDataNotificationCallback(MyNoDataProc, nil, 250);
	
	activeStreamCount = 0;
	
	for (currentIsochChannel=0;currentIsochChannel<64;currentIsochChannel++)
	{
		printf(".");
		receiver->setReceiveIsochChannel(currentIsochChannel);
		channelSnoopDone = false;
		receiver->startReceive();
		while (!channelSnoopDone);
		receiver->stopReceive();
	}
	
	// Delete the receiver object
	DestroyUniversalReceiver(receiver);
	
	// We're done!
	printf("\nIsochSnooper complete! Found %d active streams.\n",activeStreamCount);
	return result;
	
	return 0;
}

//////////////////////////////////////////////////////
// PrintLogMessage
//////////////////////////////////////////////////////
void PrintLogMessage(char *pString)
{
	printf("%s",pString);
}

//////////////////////////////////////////////////////
// MessageReceivedProc
//////////////////////////////////////////////////////
void MessageReceivedProc(UInt32 msg, UInt32 param1, UInt32 param2, void *pRefCont)
{
#if 0
	printf("Message from Isoch Receiver: %d\n",(int) msg);
	
	if (msg == kUniversalReceiverAllocateIsochPort)
		printf("Channel: %d, Speed %d\n",(int)param2,(int)param1);
#endif	
	return;
}


//////////////////////////////////////////////////////
// DataReceiveCallback
//////////////////////////////////////////////////////
IOReturn DataReceiveCallback(UInt32 CycleDataCount, UniversalReceiveCycleData *pCycleData, void *pRefCon)
{
	UInt8 fmt;
	UInt32 fdf;

#if 0	
	//  Test CIP Parser
	CIPPacketParserStruct cipPacket;
	for (int i=0;i<CycleDataCount;i++)
	{
		IOReturn res = ParseCIPPacket(pCycleData[i].pPayload, pCycleData[i].payloadLength, &cipPacket);

		printf("CIP Info: fmt = 0x%02X, fdf=0x%06X, syt=0x%04X, sourcePacketSize = %u, numSourcePackets=%u\n",
			   cipPacket.fmt,cipPacket.fdf,cipPacket.syt,cipPacket.sourcePacketSize,cipPacket.numSourcePackets);
	}
#endif
	
	// Parse packet, and report
	if (!channelSnoopDone)
	{
		if (IsCIPPacket(pCycleData[0].isochHeader))
		{
			fmt = (pCycleData[0].pPayload[4] & 0x3F);
			
			// The FDF is either 8-bits or 24-bits depending on the FMT
			if (pCycleData[0].pPayload[4] & 0x20)
			{
				fdf = (pCycleData[0].pPayload[5] << 16); 
				fdf += (pCycleData[0].pPayload[6] << 8); 
				fdf += pCycleData[0].pPayload[7]; 
			}
			else
			{
				fdf = pCycleData[0].pPayload[5]; 
			}
			
			printf("\nChannel %2u: Active, CIP-based packets\n",currentIsochChannel);

			// FMT specific data parsing
			switch (fmt)
			{
				case k61883Fmt_DV:	// DV
					printf("            FMT = DV (0x%02X)\n",fmt);
					switch (fdf & 0xFC)
					{
						case 0x84: // SDL_625_50
							printf("            DV Mode = SDL/PAL (FDF = 0x%02X)\n",fdf);
							break;

						case 0x04: // SDL_525_60
							printf("            DV Mode = SDL/NTSC (FDF = 0x%02X)\n",fdf);
							break;
						
						case 0x80: // SD_625_50
							printf("            DV Mode = SD/PAL (FDF = 0x%02X)\n",fdf);
							break;
						
						case 0x00: // SD_525_60
							printf("            DV Mode = SD/NTSC (FDF = 0x%02X)\n",fdf);
							break;
						
						case 0xF8: // DVCPro25_625_50
							printf("            DV Mode = DVCPro25/PAL (FDF = 0x%02X)\n",fdf);
							break;
						
						case 0x78: // DVCPro25_525_60
							printf("            DV Mode = DVCPro25/NTSC (FDF = 0x%02X)\n",fdf);
							break;
						
						case 0xF4: // DVCPro50_625_50
							printf("            DV Mode = DVCPro50/PAL (FDF = 0x%02X)\n",fdf);
							break;
						
						case 0x74: // DVCPro50_525_60
							printf("            DV Mode = DVCPro50/NTSC (FDF = 0x%02X)\n",fdf);
							break;
						
						case 0x88: // HD_1250_50
							printf("            DV Mode = HD/PAL (FDF = 0x%02X)\n",fdf);
							break;
						
						case 0x08: // HD_1125_60
							printf("            DV Mode = HD/NTSC (FDF = 0x%02X)\n",fdf);
							break;
						
						case 0xF0: // DVCPro100_50
							printf("            DV Mode = DVCProHD/PAL (FDF = 0x%02X)\n",fdf);
							break;
						
						case 0x70: // DVCPro100_60
							printf("            DV Mode = DVCProHD/NTSC (FDF = 0x%02X)\n",fdf);
							break;
						
						default:
							printf("            DV Mode = Unknown! (FDF = 0x%02X)\n",fdf);
							break;
					}
					break;

				case k61883Fmt_AudioMidi:	// AM
					// If this packet has an fdf of 0xFF, get the next packet's FDF.
					// Some AM824 packets have an fdf of 0xFF specifying no-data (somewhat
					// like the CIP-only packets present in DV/MPEG, but with garbage
					// source-packets following to keep the packet size constant). 
					// It's not typical to see sequential no-data packets in AM824,
					// so if this is one, we'll get the fdf from the next!
					if (fdf == 0xFF)
						fdf = pCycleData[0].pPayload[5]; 
					printf("            FMT = Audio/Midi (0x%02X)\n",fmt);
					switch (fdf & 0x07)
					{
						case 0:
							printf("            Sample-Rate = 32KHz (FDF = 0x%02X)\n",fdf);
							break;
							
						case 1:
							printf("            Sample-Rate = 44.1KHz (FDF = 0x%02X)\n",fdf);
							break;
							
						case 2:
							printf("            Sample-Rate = 48KHz (FDF = 0x%02X)\n",fdf);
							break;
							
						case 3:
							printf("            Sample-Rate = 88.2KHz (FDF = 0x%02X)\n",fdf);
							break;
							
						case 4:
							printf("            Sample-Rate = 96KHz (FDF = 0x%02X)\n",fdf);
							break;
							
						case 5:
							printf("            Sample-Rate = 176.4KHz (FDF = 0x%02X)\n",fdf);
							break;
							
						case 6:
							printf("            Sample-Rate = 192KHz (FDF = 0x%02X)\n",fdf);
							break;
							
						case 7:
							printf("            Sample-Rate = Reserved-Value! (FDF = 0x%02X)\n",fdf);
							break;
							
						default:
							printf("            Sample-Rate = Unknown! (FDF = 0x%02X)\n",fdf);
							break;
					}
					break;
					
				case k61883Fmt_MPEG2TS:	// MPEG
					printf("            FMT = MPEG2-TS (0x%02X)\n",fmt);
					break;
					
				default:
					printf("            Unknown FMT (0x%02X)\n",fmt);
					break;
			}
		}
		else
		{
			printf("Channel %2u: Active (non CIP-based packets)\n",currentIsochChannel);
		}
		
	}
	
	// Only bump the activeStreamCount if we haven't done so already for this channel
	if (channelSnoopDone != true)
		activeStreamCount += 1;
	channelSnoopDone = true;
	return kIOReturnSuccess;
}

//////////////////////////////////////////////////////
// MyNoDataProc
//////////////////////////////////////////////////////
IOReturn MyNoDataProc(void *pRefCon)
{
	// Report silence on this channel
	//printf("Channel %2u: Silent\n",currentIsochChannel);
	
	channelSnoopDone = true;
	return kIOReturnSuccess;
}

