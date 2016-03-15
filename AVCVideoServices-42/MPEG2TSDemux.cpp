/*
	File:		MPEG2TSDemux.cpp

 Synopsis: This is a console mode application that converts a MPEG-2 transport stream
           captured from either a Sony MicroMV, or the JVC Mini-HD camcorder to
		   two elementary stream files, one for video, the other for audio.
 
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

// Global Vars
TSDemuxer *deMux;
FILE *inFile;
FILE *outVideoFile;
FILE *outAudioFile;
UInt8 tsPacketBuf[kMPEG2TSPacketSize];
bool foundFirstIFrame = false;
UInt64 lastDTS = 0;
UInt64 lastAudioDTS = 0;

UInt32 framesBetweenEmptyPES = 0;

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
	char outVideoFileName[255];
	char outAudioFileName[255];
	
	// Parse the command line
	if (argc != 2)
	{
		printf("Usage: %s inFileName\n",argv[0]);
		return -1;
	}

	// Open the input file
	inFile = fopen(argv[1],"rb");
	if (inFile == nil)
	{
		printf("Unable to open input file: %s\n",argv[1]);
		return -1;
	}

	// Generate the outputfile names
	// by removing any file extension from
	// the input file, and replacing it
	// with .mpv for video, and .mp3 for audio
	strncpy(outVideoFileName,argv[1],255);
	strncpy(outAudioFileName,argv[1],255);
	strtok(outVideoFileName,".");
	strtok(outAudioFileName,".");
	strcat(outVideoFileName,".mpv");
	strcat(outAudioFileName,".mp3");

	// Open the output files
	outVideoFile = fopen(outVideoFileName,"wb");
	if (outVideoFile == nil)
	{
		printf("Unable to open output video file: %s\n",argv[2]);
		return -1;
	}
	outAudioFile = fopen(outAudioFileName,"wb");
	if (outAudioFile == nil)
	{
		printf("Unable to open output audio file: %s\n",argv[2]);
		return -1;
	}
	
	// Alloacate a string logger object and pass it our callback func
	StringLogger logger(PrintLogMessage);

	deMux = new TSDemuxer(PESCallback,
					   nil,
					   nil,
					   nil,
					   1,
					   kMaxVideoPESSizeDefault,
					   kMaxAudioPESSizeDefault,
					   kDefaultVideoPESBufferCount,
					   kDefaultAudioPESBufferCount,
					   &logger);
	if (!deMux)
	{
		printf("Error Allocating Demux Object\n");
		return -1;
	}

	// Demux it!
	for(;;)
	{
		cnt = fread(tsPacketBuf,1,kMPEG2TSPacketSize,inFile);
		if (cnt != kMPEG2TSPacketSize)
			break;
		else
			deMux->nextTSPacket(tsPacketBuf);
	}
	
	// Close files
	fclose(outVideoFile);
	fclose(outAudioFile);
	fclose(inFile);
	
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
	// Local Vars
	UInt32 i;
	UInt32 strid = 0;
	unsigned int cnt;
	UInt32 pesHeaderLen;

	TSDemuxerStreamType streamType = pPESPacket->streamType;
	UInt8 *pPESBuf = pPESPacket->pPESBuf;
	UInt32 pesBufLen = pPESPacket->pesBufLen;
	
	if (msg == kTSDemuxerPESReceived)
	{
		// Throw away all PES packets until we find the first I-Frame packet
		if (foundFirstIFrame != true)
		{
			if (streamType == kTSDemuxerStreamTypeVideo)
			{
				// Look for a GOP Header. For MicroMV and JVC Mini-HD Streams, 
				// all I-frames start with a GOP header
				for (i = 9+pPESBuf[8]; i < pesBufLen; i++)
				{
					strid = (strid << 8) | pPESBuf[i];
					if (strid == 0x000001B8) // group_start_code
					{
						printf("Found First I-Frame\n");
						foundFirstIFrame = true;
						break;
					}
				}
			}
		}
		
		// If we've found the first I-Frame, add this PES
		// packet to the PS mux.
		if (foundFirstIFrame == true)
		{
			if (streamType == kTSDemuxerStreamTypeVideo)
			{
				// Write PES packet payload to video es file
				
				pesHeaderLen = 9+pPESBuf[8];
				
				cnt = fwrite(pPESBuf+pesHeaderLen,1,pesBufLen-pesHeaderLen,outVideoFile);
				if (cnt != pesBufLen-pesHeaderLen)
				{
					printf("Error Writing to output video file!\n");
					exit(1);
				}
			}
			else if (streamType == kTSDemuxerStreamTypeAudio)
			{
				// Write PES packet payload to audio es file
				
				pesHeaderLen = 9+pPESBuf[8];
				
				cnt = fwrite(pPESBuf+pesHeaderLen,1,pesBufLen-pesHeaderLen,outAudioFile);
				if (cnt != pesBufLen-pesHeaderLen)
				{
					printf("Error Writing to output audio file!\n");
					exit(1);
				}
			}
		}
	}
	else
	{
		if (streamType == kTSDemuxerStreamTypeVideo)
			printf("Video PES Packet Error: %d\n\n",(int)msg);
		else
			printf("Audio PES Packet Error: %d\n\n",(int)msg);
	}

	// Don't forget to release this PES buffer
	deMux->ReleasePESPacketBuf(pPESPacket);
		
	return kIOReturnSuccess;
}	

