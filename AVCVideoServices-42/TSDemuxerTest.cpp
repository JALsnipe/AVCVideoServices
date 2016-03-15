/*
	File:		TSDemuxerTest.cpp

   Synopsis: This is a simple console mode application that tests the MPEG2 transport
             stream demuxer

	Copyright: 	© Copyright 2001-2003 Apple Computer, Inc. All rights reserved.

	Written by: ayanowitz

 Disclaimer:	IMPORTANT:  This Apple software is supplied to you by Apple Computer, Inc.
 ("Apple") in consideration of your agreement to the following terms, and your
 use, installation, modification or redistribution of this Apple software
 constitutes acceptance of these terms.  If you do not agree with these terms,
 please do not use, install, modify or redistribute this Apple software.

 In consideration of your agreement to abide by the following terms, and subject
 to these terms, Apple grants you a personal, non-exclusive license, under AppleÕs
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

void PrintLogMessage(char *pString);
IOReturn PESCallback(TSDemuxerMessage msg, PESPacketBuf* pPESPacket,void *pRefCon);
void HDV2_VAUXCallback(HDV2VideoFramePack *pVAux, void *pRefCon);
void HDV1_PackDataCallback(HDV1PackData *pPack, void *pRefCon);

static int hexStringToInt(char* str);

#if 0
// Enable this for testing with AVCHD (or BD) transport streams
#define kPacketPrefixSize 4
#else
// Setting for testing with standard MPEG-2 transport streams
#define kPacketPrefixSize 0
#endif

// Globals
TSDemuxer *deMux;
FILE *inFile;
UInt8 tsPacketBuf[kMPEG2TSPacketSize+kPacketPrefixSize];
UInt32 videoPid;
UInt32 audioPid;
UInt32 programNum;
UInt32 videoPESPacketCount = 0;
UInt32 audioPESPacketCount = 0;
UInt32 verboseLevel = 0;

//////////////////////////////////////////////////////
//
// Main
//
//////////////////////////////////////////////////////
int main(int argc, char* argv[])
{
	// Local Vars
    IOReturn result = kIOReturnSuccess ;
	unsigned int cnt;
	unsigned int tsPacketCount = 0;
	
	// Parse the command line
	if ((argc != 5) && (argc != 4) && (argc != 2))
	{
		printf("Usage: %s inFileName videoPid audioPid verboseLevel\n",argv[0]);
		printf("  or\n");
		printf("Usage: %s inFileName programNumIndex(one-based) verboseLevel\n",argv[0]);
		printf("  or\n");
		printf("Usage: %s inFileName (same as inFileName 1 1)\n",argv[0]);
		
		return -1;
	}

	// Open the input file
	inFile = fopen(argv[1],"rb");
	if (inFile == nil)
	{
		printf("Unable to open input file: %s\n",argv[1]);
		return -1;
	}

	printf("TSDemuxerTest\n");
	printf("-------------\n");
	printf("InputFile: %s\n",argv[1]);
	printf("\n");

	// Alloacate a string logger object and pass it our callback func
	StringLogger logger(PrintLogMessage);

	if (argc == 5)
	{
		// Get the video PID
		videoPid = hexStringToInt(argv[2]);

		// Get the audio PID
		audioPid = hexStringToInt(argv[3]);

		// Get the verbose level
		verboseLevel = hexStringToInt(argv[4]);

		deMux = new TSDemuxer(videoPid,
						audioPid,
						kIgnoreStream,
						PESCallback,
						nil,
						nil,
						nil,
						kMaxVideoPESSizeDefault,
						kMaxAudioPESSizeDefault,
						kDefaultVideoPESBufferCount,
						kDefaultAudioPESBufferCount,
						&logger);
	}
	else
	{
		// Get the program num
		if (argc == 2)
			programNum =  1;
		else
			programNum =  hexStringToInt(argv[2]);

		// Get the verbose level
		if (argc == 2)
			verboseLevel =  1;
		else
			verboseLevel = hexStringToInt(argv[3]);

		deMux = new TSDemuxer(PESCallback,
						nil,
						nil,
						nil,
						programNum,
						kMaxVideoPESSizeDefault,
						kMaxAudioPESSizeDefault,
						kDefaultVideoPESBufferCount,
						kDefaultAudioPESBufferCount,
						&logger);
	}

	if (!deMux)
	{
		printf("Error Allocating Demux Object\n");
		return -1;
	}
	
	// Install a handler for HDV2 VAux data
	deMux->InstallHDV2VAuxCallback(HDV2_VAUXCallback, nil);

	// Install a handler for HDV1 Embedded pack data
	deMux->InstallHDV1PackDataCallback(HDV1_PackDataCallback,nil);
	
	// Enable demuxer feature that saves the raw TS packets for each PES in an array
	deMux->SetDemuxerConfigurationBits(kDemuxerConfig_KeepTSPackets);
	
	// Demux it!
	for(;;)
	{
		cnt = fread(tsPacketBuf,1,kMPEG2TSPacketSize+kPacketPrefixSize,inFile);
		if (cnt != kMPEG2TSPacketSize+kPacketPrefixSize)
			break;
		else
		{
			deMux->nextTSPacket(tsPacketBuf+kPacketPrefixSize,tsPacketCount);
			tsPacketCount += 1;
		}
	}
	
	fclose(inFile);

	printf("\n");
	printf("Video PES Packet Count: %d\n",(int)videoPESPacketCount);
	printf("Audio PES Packet Count: %d\n",(int)audioPESPacketCount);
	
	return result;
}

//////////////////////////////////////////////////////
// PrintLogMessage
//////////////////////////////////////////////////////
void PrintLogMessage(char *pString)
{
	printf("%s",pString);
}

//////////////////////////////////////////////////////
// PESCallback
//////////////////////////////////////////////////////
IOReturn PESCallback(TSDemuxerMessage msg, PESPacketBuf* pPESPacket,void *pRefCon)
{
	SInt64 pts,dts;
	UInt32 strid = 0;
	UInt32 i;
	UInt32 bitRate;
	unsigned char *pDescriptorByte;
	static bool isAC3Audio = false;
	PSITables *pPSITables = pPESPacket->pTSDemuxer->psiTables;

	TSDemuxerStreamType streamType = pPESPacket->streamType;
	UInt8 *pPESBuf = pPESPacket->pPESBuf;
	UInt32 pesBufLen = pPESPacket->pesBufLen;
	
	if (msg == kTSDemuxerPESReceived)
	{
		if (verboseLevel > 0)
		{
			// If this is the first callback for the stream, 
			// Dump the PMT descriptors
			if ( (videoPESPacketCount == 0) && (audioPESPacketCount == 0))
			{
				printf("\n");
				
				if (deMux->psiTables->primaryProgramDescriptorsLen > 0)
				{
					printf("PMT Program Descriptors Length: %d\n",deMux->psiTables->primaryProgramDescriptorsLen);
					pDescriptorByte = deMux->psiTables->pPrimaryProgramDescriptors;
					do
					{
						printf("    Descriptor --> Tag: %d Len: %d ",pDescriptorByte[0],pDescriptorByte[1]);
						
						if (pDescriptorByte[1] > 0)
						{
							printf("( ");
							for (i=0;i<pDescriptorByte[1];i++)
								printf("%02X ",pDescriptorByte[2+i]);
							printf(")\n");
						}
						else	
							printf("\n");
						
						// Special decoding of registration-descriptor
						if (pDescriptorByte[0] == 5)
						{
							printf("        Found: registration_descriptor. Format Identifier = 0x%08X\n",
								   (pDescriptorByte[2] << 24) + (pDescriptorByte[3] << 16) + (pDescriptorByte[4] << 8) + (pDescriptorByte[5]));
						}
						
						// Bump past this descriptor
						pDescriptorByte += (2+pDescriptorByte[1]);
					}while (pDescriptorByte < (deMux->psiTables->pPrimaryProgramDescriptors + deMux->psiTables->primaryProgramDescriptorsLen));
					printf("\n");
				}
				
				if (deMux->psiTables->primaryVideoESDescriptorsLen > 0)
				{
					printf("PMT Video ES Descriptors Length: %d\n",deMux->psiTables->primaryVideoESDescriptorsLen);
					pDescriptorByte = deMux->psiTables->pPrimaryVideoESDescriptors;
					do
					{
						printf("    Descriptor --> Tag: %d Len: %d ",pDescriptorByte[0],pDescriptorByte[1]);
						
						if (pDescriptorByte[1] > 0)
						{
							printf("( ");
							for (i=0;i<pDescriptorByte[1];i++)
								printf("%02X ",pDescriptorByte[2+i]);
							printf(")\n");
						}
						else	
							printf("\n");
						
						// Bump past this descriptor
						pDescriptorByte += (2+pDescriptorByte[1]);
					}while (pDescriptorByte < (deMux->psiTables->pPrimaryVideoESDescriptors + deMux->psiTables->primaryVideoESDescriptorsLen));
					printf("\n");
					
				}
				
				if (deMux->psiTables->primaryAudioESDescriptorsLen > 0)
				{
					printf("PMT Audio ES Descriptors Length: %d\n",deMux->psiTables->primaryAudioESDescriptorsLen);
					pDescriptorByte = deMux->psiTables->pPrimaryAudioESDescriptors;
					do
					{
						printf("    Descriptor --> Tag: %d Len: %d ",pDescriptorByte[0],pDescriptorByte[1]);
						
						if (pDescriptorByte[1] > 0)
						{
							printf("( ");
							for (i=0;i<pDescriptorByte[1];i++)
								printf("%02X ",pDescriptorByte[2+i]);
							printf(")\n");
							
							// Look for AC-3 registration descriptor
							if ((pDescriptorByte[0] == 0x05) && (pDescriptorByte[1] == 0x04) && 
								(pDescriptorByte[2] == 0x41) &&(pDescriptorByte[3] == 0x43) &&(pDescriptorByte[4] == 0x2D) &&(pDescriptorByte[5] == 0x33))
							{
								isAC3Audio = true;		
							}
								
							// Decode AC-3 descriptor
							if ((pDescriptorByte[0] == 0x81) && (isAC3Audio == true))
							{
								printf("      AC-3 Audio Descriptor\n");
								switch (((pDescriptorByte[2] & 0xE0) >> 5))
								{
									case 0:
										printf("        Sample Rate: 48KHz\n");
										break;
										
									case 1:
										printf("        Sample Rate: 44.1KHz\n");
										break;
										
									case 2:
										printf("        Sample Rate: 32 KHz\n");
										break;
										
									case 3:
										printf("        Sample Rate: Reserved\n");
										break;
										
									case 4:
										printf("        Sample Rate: 48KHz, or 44.1KHz\n");
										break;
										
									case 5:
										printf("        Sample Rate: 48KHz, or 32KHz\n");
										break;
										
									case 6:
										printf("        Sample Rate: 44.1KHz, or 32KHz\n");
										break;
										
									case 7:
										printf("        Sample Rate: 48KHz, or 44.1KHz, or 32KHz\n");
										break;

									default:
										printf("        Sample Rate: Unknown\n");
										break;
								};
								printf("        bsid: %u\n",(unsigned int)(pDescriptorByte[2] & 0x1F));
								switch (((pDescriptorByte[3] & 0xFC) >> 2))
								{
									case 0:
										printf("        Bit-Rate: 32 kbit/sec\n");
										break;
										
									case 1:
										printf("        Bit-Rate: 40 kbit/sec\n");
										break;
										
									case 2:
										printf("        Bit-Rate: 48 kbit/sec\n");
										break;
										
									case 3:
										printf("        Bit-Rate: 56 kbit/sec\n");
										break;
										
									case 4:
										printf("        Bit-Rate: 64 kbit/sec\n");
										break;
										
									case 5:
										printf("        Bit-Rate: 80 kbit/sec\n");
										break;
										
									case 6:
										printf("        Bit-Rate: 96 kbit/sec\n");
										break;
										
									case 7:
										printf("        Bit-Rate: 112 kbit/sec\n");
										break;
										
									case 8:
										printf("        Bit-Rate: 128 kbit/sec\n");
										break;
										
									case 9:
										printf("        Bit-Rate: 160 kbit/sec\n");
										break;
										
									case 10:
										printf("        Bit-Rate: 192 kbit/sec\n");
										break;
										
									case 11:
										printf("        Bit-Rate: 224 kbit/sec\n");
										break;
										
									case 12:
										printf("        Bit-Rate: 256 kbit/sec\n");
										break;
										
									case 13:
										printf("        Bit-Rate: 320 kbit/sec\n");
										break;
										
									case 14:
										printf("        Bit-Rate: 384 kbit/sec\n");
										break;
										
									case 15:
										printf("        Bit-Rate: 448 kbit/sec\n");
										break;
										
									case 16:
										printf("        Bit-Rate: 512 kbit/sec\n");
										break;
										
									case 17:
										printf("        Bit-Rate: 576 kbit/sec\n");
										break;

									case 18:
										printf("        Bit-Rate: 640 kbit/sec\n");
										break;
										
									case 32:
										printf("        Bit-Rate (upper-limit): 32 kbit/sec\n");
										break;
										
									case 33:
										printf("        Bit-Rate (upper-limit): 40 kbit/sec\n");
										break;
										
									case 34:
										printf("        Bit-Rate (upper-limit): 48 kbit/sec\n");
										break;
										
									case 35:
										printf("        Bit-Rate (upper-limit): 56 kbit/sec\n");
										break;
										
									case 36:
										printf("        Bit-Rate (upper-limit): 64 kbit/sec\n");
										break;
										
									case 37:
										printf("        Bit-Rate (upper-limit): 80 kbit/sec\n");
										break;
										
									case 38:
										printf("        Bit-Rate (upper-limit): 96 kbit/sec\n");
										break;
										
									case 39:
										printf("        Bit-Rate (upper-limit): 112 kbit/sec\n");
										break;
										
									case 40:
										printf("        Bit-Rate (upper-limit): 128 kbit/sec\n");
										break;
										
									case 41:
										printf("        Bit-Rate (upper-limit): 160 kbit/sec\n");
										break;
										
									case 42:
										printf("        Bit-Rate (upper-limit): 192 kbit/sec\n");
										break;
										
									case 43:
										printf("        Bit-Rate (upper-limit): 224 kbit/sec\n");
										break;
										
									case 44:
										printf("        Bit-Rate (upper-limit): 256 kbit/sec\n");
										break;
										
									case 45:
										printf("        Bit-Rate (upper-limit): 320 kbit/sec\n");
										break;
										
									case 46:
										printf("        Bit-Rate (upper-limit): 384 kbit/sec\n");
										break;
										
									case 47:
										printf("        Bit-Rate (upper-limit): 448 kbit/sec\n");
										break;
										
									case 48:
										printf("        Bit-Rate (upper-limit): 512 kbit/sec\n");
										break;
										
									case 49:
										printf("        Bit-Rate (upper-limit): 576 kbit/sec\n");
										break;
										
									case 50:
										printf("        Bit-Rate (upper-limit): 640 kbit/sec\n");
										break;
										
									default:
										printf("        Bit-Rate: Unknown\n");
										break;
								};
								
								switch (pDescriptorByte[3] & 0x03)
								{
									case 0:
										printf("        Surrround Mode: Not indicated\n");
										break;

									case 1:
										printf("        Surrround Mode: Not Dolby surround encoded\n");
										break;
										
									case 2:
										printf("        Surrround Mode: Dolby surround encoded\n");
										break;
										
									case 3:
									default:
										printf("        Surrround Mode: Reserved\n");
										break;
								};
										
								printf("        bsmod: %u\n",(unsigned int)((pDescriptorByte[4] & 0xE0) >> 5));

								switch (((pDescriptorByte[4] & 0x1E) >> 1))
								{
									case 0:
										printf("        Audio Coding Mode: 1+1\n");
										break;

									case 1:
										printf("        Audio Coding Mode: 1/0\n");
										break;
										
									case 2:
										printf("        Audio Coding Mode: 2/0\n");
										break;
										
									case 3:
										printf("        Audio Coding Mode: 3/0\n");
										break;
										
									case 4:
										printf("        Audio Coding Mode: 2/1\n");
										break;
										
									case 5:
										printf("        Audio Coding Mode: 3/1\n");
										break;
										
									case 6:
										printf("        Audio Coding Mode: 2/2\n");
										break;
										
									case 7:
										printf("        Audio Coding Mode: 3/2\n");
										break;
										
									case 8:
										printf("        Num Channels: 1\n");
										break;
										
									case 9:
										printf("        Num Channels: <=2\n");
										break;
										
									case 10:
										printf("        Num Channels: <=3\n");
										break;
										
									case 11:
										printf("        Num Channels: <=4\n");
										break;
										
									case 12:
										printf("        Num Channels: <=5\n");
										break;
										
									case 13:
										printf("        Num Channels: <=6\n");
										break;
										
									case 14:
									case 15:
									default:
										printf("        Num Channels: Reserved\n");
										break;
								};
								printf("        %s service\n",((pDescriptorByte[4] & 0x1) == 0x1) ? "Full" : "Partial");
							}
						}
						else	
							printf("\n");
						
						// Bump past this descriptor
						pDescriptorByte += (2+pDescriptorByte[1]);
					}while (pDescriptorByte < (deMux->psiTables->pPrimaryAudioESDescriptors + deMux->psiTables->primaryAudioESDescriptorsLen));
					printf("\n");
				}
			}
			
			// Dump PES Header Info
			if (streamType == kTSDemuxerStreamTypeVideo)
				printf("PES Packet (Video Packet %d) Len: %d\n",(int)videoPESPacketCount,(int)pesBufLen);
			else
				printf("PES Packet (Audio Packet %d) Len: %d\n",(int)audioPESPacketCount,(int)pesBufLen);
			
			printf("First TS Packet for this PES: %d\n",(int)pPESPacket->startTSPacketTimeStamp);

			if (pPESPacket->tsPacketArray)
				printf("Number of TS packets: %u\n",(unsigned int)CFArrayGetCount(pPESPacket->tsPacketArray));
				
			printf("Packet Start Code Prefix: 0x%02X%02X%02X\n",pPESBuf[0],pPESBuf[1],pPESBuf[2]);
			printf("Stream ID: 0x%02X\n",pPESBuf[3]);
			printf("Packet Len: %d\n",(pPESBuf[4] << 8)+pPESBuf[5]);
			
			// For the audio stream, the packet length field in the PES header should match the actual PES
			// buf size minus the size of the PES header. Display a warning if this doesn't match!
			if ((streamType == kTSDemuxerStreamTypeAudio) && ((int)(pesBufLen-6) != (int)((pPESBuf[4] << 8)+pPESBuf[5])))
				printf("Warning: Packet Len field doesn't match actual buf length. Actual=%d, Specified Len=%d\n",
					   (int)(pesBufLen-6), (int)((pPESBuf[4] << 8)+pPESBuf[5]) );
			
			printf("PES Scrambling Control: %d\n",((pPESBuf[6] & 0x30)>>4));
			printf("PES Priority: %d\n",((pPESBuf[6] & 0x08)>>3));
			printf("Data Alignment Indicator: %d\n",((pPESBuf[6] & 0x04)>>2));
			printf("Copyright: %d\n",((pPESBuf[6] & 0x02)>>1));
			printf("Original or Copy: %d\n",(pPESBuf[6] & 0x01));
			
			printf("PTS/DTS Flags: %d\n",((pPESBuf[7] & 0xC0)>>6));
			printf("ESCR Flag: %d\n",((pPESBuf[7] & 0x20)>>5));
			printf("ES_Rate_Flag: %d\n",((pPESBuf[7] & 0x10)>>4));
			printf("DSM_trick_mode_flag: %d\n",((pPESBuf[7] & 0x08)>>3));
			printf("Additional copy info flag: %d\n",((pPESBuf[7] & 0x04)>>2));
			printf("PES_CRC_Flag: %d\n",((pPESBuf[7] & 0x02)>>1));
			printf("PES Extension Flag: %d\n",(pPESBuf[7] & 0x01));
			
			printf("PES Header Data Length: %d\n",pPESBuf[8]);
			
			if (((pPESBuf[7] & 0xC0)>>6) == 2)
			{
				// Has PTS
				pts = (
					   ((((UInt64)pPESBuf[9] & 0x0F)  >> 1) << 30) |
					   ((UInt64)pPESBuf[10]  << 22) |
					   (((UInt64)pPESBuf[11]  >> 1) << 15) |
					   ((UInt64)pPESBuf[12]  << 7) |
					   (((UInt64)pPESBuf[13]  >> 1)));
				printf("PTS = %llu (time=%f)\n", pts,((1.0/90000.0)*pts));
			}
			else if (((pPESBuf[7] & 0xC0)>>6) == 3)
			{
				// Has PTS and DTS
				pts = (
					   ((((UInt64)pPESBuf[9] & 0x0F)  >> 1) << 30) |
					   ((UInt64)pPESBuf[10]  << 22) |
					   (((UInt64)pPESBuf[11]  >> 1) << 15) |
					   ((UInt64)pPESBuf[12]  << 7) |
					   (((UInt64)pPESBuf[13]  >> 1)));
				printf("PTS = %llu (time=%f)\n", pts,((1.0/90000.0)*pts));
				
				dts = (
					   ((((UInt64)pPESBuf[14] & 0x0F)  >> 1) << 30) |
					   ((UInt64)pPESBuf[15]  << 22) |
					   (((UInt64)pPESBuf[16]  >> 1) << 15) |
					   ((UInt64)pPESBuf[17]  << 7) |
					   (((UInt64)pPESBuf[18]  >> 1)));
				printf("DTS = %llu (time=%f)\n", dts,((1.0/90000.0)*dts));
			}
			
			// For MPEG-2 video PES packets, find Sequence Headers, GOP headers and Picture Headers in PES Packet
			if ((streamType == kTSDemuxerStreamTypeVideo) && (pPSITables) && (pPSITables->primaryProgramVideoStreamType == 0x02))
			{
				// Look for a GOP Header
				for (i = 9+pPESBuf[8]; i < pesBufLen; i++)
				{
					strid = (strid << 8) | pPESBuf[i];
					if (strid == 0x000001B3) // sequence_header_code
					{
						// found a Group of Pictures header
						printf("Sequence Header\n");
						
						// Get the frame resolution
						printf("  Horizontal Size: %d\n",
							   (int)( ((UInt32)pPESBuf[i+1] << 4) + ((UInt32)pPESBuf[i+2] >> 4)));
						printf("  Vertical Size:   %d\n",
							   (int)( (((UInt32)pPESBuf[i+2] & 0x0F) << 8) + (UInt32)pPESBuf[i+3]));
						
						// Get the frame rate
						switch((pPESBuf[i+4] & 0x0F))
						{
							case 1:
								printf("  Frame Rate:      23.976 fps\n");
								break;
							case 2:
								printf("  Frame Rate:      24 fps\n");
								break;
							case 3:
								printf("  Frame Rate:      25 fps\n");
								break;
							case 4:
								printf("  Frame Rate:      29.97 fps\n");
								break;
							case 5:
								printf("  Frame Rate:      30 fps\n");
								break;
							case 6:
								printf("  Frame Rate:      50 fps\n");
								break;
							case 7:
								printf("  Frame Rate:      59.94 fps\n");
								break;
							case 8:
								printf("  Frame Rate:      60 fps\n");
								break;
							default:
								printf("  Frame Rate:      Unknown Value\n");
								break;
						};
						
						// Get the bit-rate code
						bitRate = ((pPESBuf[i+5] << 10)+(pPESBuf[i+6] << 2)+((pPESBuf[i+7] & 0xC0) >> 6))*400;
						printf("  Bit-Rate:        %d bits/sec\n",(int)bitRate);
					}
					else if (strid == 0x000001B8) // group_start_code
					{
						// found a Group of Pictures header
						printf("GOP Header\n");
						
						UInt32 hours, minutes, seconds, frames;
						
						hours = (pPESBuf[i+1] & 0x7C) >> 2;
						minutes = ((pPESBuf[i+1] & 0x03) << 4) + ((pPESBuf[i+2] & 0xF0) >> 4);
						seconds = ((pPESBuf[i+2] & 0x07) << 3) + ((pPESBuf[i+3] & 0xE0) >> 5);
						frames = ((pPESBuf[i+3] & 0x1F) << 1) + ((pPESBuf[i+4] & 0x80) >> 7);
						
						printf("  Timecode: %d:%d:%d:%d\n",(int)hours,(int)minutes,(int)seconds,(int)frames);
						
						
					}
					else if (strid == 0x00000100) // picture_start_code
					{
						// picture_header, determine what kind of picture it is
						if (i<(pesBufLen-3))
						{
							// check if picture_coding_type == 1
							if ((pPESBuf[i+2] & (0x7 << 3)) == (1 << 3))
								printf("Picture Header: I-Frame Picture\n");
							else if ((pPESBuf[i+2] & (0x7 << 3)) == (2 << 3))
								printf("Picture Header: P-Frame Picture\n");
							else if ((pPESBuf[i+2] & (0x7 << 3)) == (3 << 3))
								printf("Picture Header: B-Frame Picture\n");
							else
								printf("Picture Header: Unknown Picture Type\n");
						}
					}
				}
			}
			
			printf ("\n");
		}
	}
	else
	{
		if (streamType == kTSDemuxerStreamTypeVideo)
			printf("PES Packet (Video Packet %d) Error: %d\n\n",(int)videoPESPacketCount,(int)msg);
		else
			printf("PES Packet (Audio Packet %d) Error: %d\n\n",(int)audioPESPacketCount,(int)msg);
	}

	if (streamType == kTSDemuxerStreamTypeVideo)
		videoPESPacketCount += 1;
	else if (streamType == kTSDemuxerStreamTypeAudio)
		audioPESPacketCount += 1;
	
	// Don't forget to release this PES buffer
	deMux->ReleasePESPacketBuf(pPESPacket);

	return kIOReturnSuccess;
}

//////////////////////////////////////////////////////
// HDV1_PackDataCallback
//////////////////////////////////////////////////////
void HDV1_PackDataCallback(HDV1PackData *pPack, void *pRefCon)
{
	printf("HDV1 Pack Data (Length = %u)\n\n",(unsigned int) pPack->packDataLen);

	printf("ID String: 0x%08X\n",(unsigned int)pPack->idString);
	
	printf("pPack->seamlessPlayBackPoint: %d\n",pPack->seamlessPlayBackPoint);
	printf("pPack->has_2_3_pullDown: %d\n",pPack->has_2_3_pullDown);
	printf("pPack->pullDownRepetition: %d\n",pPack->pullDownRepetition);
	printf("\n");

	if (pPack->hasMPEGSourcePack)
	{
		printf("pPack->serviceID: 0x%04X\n",pPack->serviceID);
		printf("pPack->sourceCode: %d\n",pPack->sourceCode);
		printf("pPack->is50HzSystem: %d\n",pPack->is50HzSystem);
		printf("pPack->sType: %d\n",pPack->sType);
		printf("pPack->tunerCategory: %d\n",pPack->tunerCategory);
		printf("\n");
	}
	
	if (pPack->hasMPEGSourceControlPack)
	{
		printf("pPack->cgms: %d\n",pPack->cgms);
		printf("pPack->tph: %d\n",pPack->tph);
		printf("pPack->tpl: %d\n",pPack->tpl);
		printf("pPack->ss: %d\n",pPack->ss);
		printf("pPack->recST: %d\n",pPack->recST);
		printf("pPack->recMode: %d\n",pPack->recMode);
		printf("pPack->mr: %d\n",pPack->mr);
		printf("pPack->isHD: %d\n",pPack->isHD);
		printf("pPack->audMode: %d\n",pPack->audMode);
		printf("pPack->maxBitRate: 0x%02X\n",pPack->maxBitRate);
		printf("pPack->recEnd: %d\n",pPack->recEnd);
		printf("pPack->genreCategory: %d\n",pPack->genreCategory);
		printf("\n");
	}
	
	if (pPack->hasRecDatePack)
	{
		printf("pPack->recTimeZone: %d\n",pPack->recTimeZone);
		printf("pPack->recDay: %d\n",pPack->recDay);
		printf("pPack->recWeek: %d\n",pPack->recWeek);
		printf("pPack->recMonth: %d\n",pPack->recMonth);
		printf("pPack->recYear: %d\n",pPack->recYear);
		printf("\n");
	}
	
	if (pPack->hasRecTimePack)
	{
		printf("pPack->recFrames: %d\n",pPack->recFrames);
		printf("pPack->recSeconds: %d\n",pPack->recSeconds);
		printf("pPack->recMinutes: %d\n",pPack->recMinutes);
		printf("pPack->recHours: %d\n",pPack->recHours);
		printf("\n");
	}
	
	if (pPack->hasStreamPack)
	{
		printf("pPack->streamType: 0x%02X\n",pPack->streamType);
		printf("pPack->elementaryPID: 0x%04X\n",pPack->elementaryPID);
		printf("pPack->pidType: %d\n",pPack->pidType);
		printf("\n");
	}	
	
	if (pPack->hasTitleTimeCodePack)
	{
		printf("pPack->ttcFrames: %d\n",pPack->ttcFrames);
		printf("pPack->ttcSeconds: %d\n",pPack->ttcSeconds);
		printf("pPack->ttcMinutes: %d\n",pPack->ttcMinutes);
		printf("pPack->ttcHours: %d\n",pPack->ttcHours);
		printf("pPack->s1: %d\n",pPack->s1);
		printf("pPack->s2: %d\n",pPack->s2);
		printf("pPack->s3: %d\n",pPack->s3);
		printf("pPack->s4: %d\n",pPack->s4);
		printf("pPack->s5: %d\n",pPack->s5);
		printf("pPack->s6: %d\n",pPack->s6);
		printf("\n");
	}
	
	if (pPack->hasBinaryGroupPack)
	{
		printf("pPack->binaryGroup1: %d\n",pPack->binaryGroup1);
		printf("pPack->binaryGroup2: %d\n",pPack->binaryGroup2);
		printf("pPack->binaryGroup3: %d\n",pPack->binaryGroup3);
		printf("pPack->binaryGroup4: %d\n",pPack->binaryGroup4);
		printf("pPack->binaryGroup5: %d\n",pPack->binaryGroup5);
		printf("pPack->binaryGroup6: %d\n",pPack->binaryGroup6);
		printf("pPack->binaryGroup7: %d\n",pPack->binaryGroup7);
		printf("pPack->binaryGroup8: %d\n",pPack->binaryGroup8);
		printf("\n");
	}
}	

//////////////////////////////////////////////////////
// HDV2_VAUXCallback
//////////////////////////////////////////////////////
void HDV2_VAUXCallback(HDV2VideoFramePack *pVAux, void *pRefCon)
{
	if (pVAux->keyWord != 0x44)
	{
		// This is not HDV2 VAux, since the keyword didn't match!
		return;
	}
	printf("HDV2 V-Aux Video Frame Pack\n\n");

	printf("keyWord = 0x%02X\n",pVAux->keyWord);
	printf("length = %u\n", pVAux->length);
	printf("extendedTrackNumber = %u\n", (int) pVAux->extendedTrackNumber);
	printf("numVideoFrames = %u\n", pVAux->numVideoFrames);
	printf("dataH = %u ", pVAux->dataH);
	switch (pVAux->dataH)
	{
		case 0:
			printf("(Stuffing)\n");
			break;
		case 1:
			printf("(I-Picture)\n");
			break;		
		case 2:
			printf("(B-Picture)\n");
			break;		
		case 3:
			printf("(P-Picture)\n");
			break;
		case 4:
			printf("(Copy-Picture)\n");
			break;
		case 5:
			printf("(V-END)\n");
			break;
		case 7:
			printf("(No Information)\n");
			break;
		case 8:
			printf("(No Picture)\n");
			break;
		case 9:
			printf("(No Editable)\n");
			break;
		default:
			printf("(Reserved Value)\n");
			break;
	};
	printf("vbvDelay = %u\n", pVAux->vbvDelay);
	printf("headerSize = %u\n", pVAux->headerSize);
	printf("dts = %llu (time=%f)\n", pVAux->dts,((1.0/90000.0)*pVAux->dts));
	printf("progressive = %u\n", pVAux->progressive);
	printf("topFieldFirst = %u\n", pVAux->topFieldFirst);
	printf("repeatFirstField = %u\n", pVAux->repeatFirstField);
	printf("sourceFrameRate = %u ", pVAux->sourceFrameRate);
	switch (pVAux->sourceFrameRate)
	{
		case 4:
			printf("(29.97 Hz)\n");
			break;
		case 3:
			printf("(25 Hz)\n");
			break;
		case 1:
			printf("(23.98 Hz)\n");
			break;
		default:
			printf("(Reserved Value)\n");
			break;
			
	};
	printf("searchDataMode = 0x%02X\n", pVAux->searchDataMode);
	printf("horizontalSize = %u\n", pVAux->horizontalSize);
	printf("verticalSize = %u\n", pVAux->verticalSize);
	printf("aspectRatio = %u ", pVAux->aspectRatio);
	switch (pVAux->aspectRatio)
	{
		case 3:
			printf("(16:9)\n");
			break;
		default:
			printf("(Reserved Value)\n");
			break;
	};
	printf("frameRate = %u ", pVAux->frameRate);
	switch (pVAux->frameRate)
	{
		case 3:
			printf("(25 fps)\n");
			break;
		case 4:
			printf("(29.97 fps)\n");
			break;
		default:
			printf("(Reserved Value)\n");
			break;
	};
	printf("bitRate = %u (%u bits/sec)\n", (int) pVAux->bitRate, (int)(pVAux->bitRate*400));
	printf("vbvBufferSize = %u (%u bits)\n", pVAux->vbvBufferSize, (pVAux->vbvBufferSize*16*1024));
	printf("mpegProfile = %u (%s)\n", pVAux->mpegProfile,(pVAux->mpegProfile == 4) ? "Main" : "Reserved Value");
	printf("mpegLevel = %u (%s)\n", pVAux->mpegLevel,(pVAux->mpegLevel == 6) ? "High-1440" : "Reserved Value");
	printf("videoFormat = %u (%s)\n", pVAux->videoFormat,(pVAux->videoFormat == 0) ? "Component" : "Reserved Value");
	printf("chroma = %u (%s)\n", pVAux->chroma,(pVAux->chroma == 1) ? "4:2:0" : "Reserved Value");
	printf("gopN = %u ", pVAux->gopN);
	switch (pVAux->gopN)
	{
		case 12:
			printf("(1080i/50 system)\n");
			break;
		case 15:
			printf("(1080i/60)\n");
			break;
		default:
			printf("(Reserved Value)\n");
			break;
	};
	printf("gopM = %u (%s)\n", pVAux->gopM,(pVAux->gopM == 3) ? "3" : "Reserved Value");
	printf("packDataEnable0 = %u (Time-Code%sValid)\n", pVAux->packDataEnable0, (pVAux->packDataEnable0 == true) ? " " : " Not ");
	printf("packDataEnable1 = %u (Rec-Date%sValid)\n", pVAux->packDataEnable1, (pVAux->packDataEnable1 == true) ? " " : " Not ");
	printf("packDataEnable2 = %u (Rec-Time%sValid)\n", pVAux->packDataEnable2, (pVAux->packDataEnable2 == true) ? " " : " Not ");
	printf("timeCode: %02u:%02u:%02u.%02u (bf=%u, ttc_df=%u)\n",
		   pVAux->tc_hours,
		   pVAux->tc_minutes,
		   pVAux->tc_seconds,
		   pVAux->tc_frames,
		   pVAux->bf,
		   pVAux->ttc_df);
	printf("recordDate: %02u/%02u/%04u (ds=%u, tm=%u, timeZone=%u, week=%u)\n",
		   pVAux->recDate_month,
		   pVAux->recDate_day,
		   pVAux->recDate_year,
		   pVAux->ds,pVAux->tm,pVAux->recDate_timeZone,pVAux->recDate_week);
	printf("recordTime: %02u:%02u:%02u.%02u\n",
		   pVAux->recTime_hours,
		   pVAux->recTime_minutes,
		   pVAux->recTime_seconds,
		   pVAux->recTime_frames);
	printf("rec_st=%u, atn_bf=%u\n",
		   pVAux->rec_st,
		   pVAux->atn_bf);
	printf("copyGenerationManagementSystem = %u ", pVAux->copyGenerationManagementSystem);
	switch (pVAux->copyGenerationManagementSystem)
	{
		case 0:
			printf("(Copying permitted without restriction)\n");
			break;
		case 2:
			printf("(One generation of copies permitted)\n");
			break;
		case 3:
			printf("(No copying permitted)\n");
			break;
		case 1:
		default:
			printf("(Not Used)\n");
			break;
	};
	printf("extendedDVPack1 = 0x%02X%02X%02X%02X%02X\n",
		   pVAux->extendedDVPack1[0], pVAux->extendedDVPack1[1], pVAux->extendedDVPack1[2], pVAux->extendedDVPack1[3], pVAux->extendedDVPack1[4]);
	printf("extendedDVPack2 = 0x%02X%02X%02X%02X%02X\n",
		   pVAux->extendedDVPack2[0], pVAux->extendedDVPack2[1], pVAux->extendedDVPack2[2], pVAux->extendedDVPack2[3], pVAux->extendedDVPack2[4]);
	printf("extendedDVPack3 = 0x%02X%02X%02X%02X%02X\n",
		   pVAux->extendedDVPack3[0], pVAux->extendedDVPack3[1], pVAux->extendedDVPack3[2], pVAux->extendedDVPack3[3], pVAux->extendedDVPack3[4]);
	
	if (pVAux->hasMakerCodeAndImagingFrameRate)
	{
		printf("Maker Code = 0x%02X\n",pVAux->makerCode);
		printf("Imaging Frame Rate = 0x%02X\n",pVAux->imagingFrameRate);
	}
	
	printf("\n");
	
}

//////////////////////////////////////////////////////
// hexStringToInt
//////////////////////////////////////////////////////
static int hexStringToInt(char* str)
{
	if (strncmp(str, "0x", 2) != 0)
		return atol(str);

	int v = 0;
	char *p = str + 2;
	while (*p && *p != ' ')
	{
		if (*p >= '0' && *p <= '9')
			v = (v << 4) | (*p - '0');
		else if (*p >= 'a' && *p <= 'f')
			v = (v << 4) | ((*p - 'a') + 10);
		else if (*p >= 'A' && *p <= 'F')
			v = (v << 4) | ((*p - 'A') + 10);
		p++;
	}

	return v;
}

