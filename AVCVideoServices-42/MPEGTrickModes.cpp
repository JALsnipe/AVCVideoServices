/*
	File:		MPEGTrickModes.cpp
 
 Synopsis: This is the source file for the MPEGTrickModes functions.
 
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

/////////////////////////////////////////////////////////////////////////////////////////////
// FramesPerSecond: Convert from MPEGFrameRate to a approximate number of frames per second
/////////////////////////////////////////////////////////////////////////////////////////////
UInt32 FramesPerSecond(MPEGFrameRate frameRate)
{
	UInt32 count = 0;
	
	switch (frameRate)
	{
		case MPEGFrameRate_23_976:
		case MPEGFrameRate_24:
			count = 24;
			break;
			
		case MPEGFrameRate_25:
			count = 25;
			break;
			
		case MPEGFrameRate_50:
			count = 50;
			break;
			
		case MPEGFrameRate_59_94:
		case MPEGFrameRate_60:
			count = 60;
			break;
			
		case MPEGFrameRate_30:
		case MPEGFrameRate_29_97:
		case MPEGFrameRate_Unknown:
		default:
			count = 30;
			break;
	};
	
	return count;
}


/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
///////
///////  
///////  MPEGNaviFileReader Class Object
///////
///////
///////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////
// MPEGNaviFileReader::MPEGNaviFileReader
/////////////////////////////////////////////////////////////////////////////////////////////
MPEGNaviFileReader::MPEGNaviFileReader()
{
	tsFile = nil;
	naviFile = nil;
	pNaviBuf = nil;
	pNaviFileName = nil;
	frameHorizontalSize = 0;
	frameVerticalSize = 0;
	bitRate = 0;
	frameRate = MPEGFrameRate_Unknown;
	numFrames = 0;
	numTSPacketsInFile = 0;
	pFrameInfo = nil;
	pStreamInfo = nil;
	currentOffsetInTSPackets = 0;
	currentOffsetInFrames = 0;
	hasNaviFile = false;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// MPEGNaviFileReader::~MPEGNaviFileReader
/////////////////////////////////////////////////////////////////////////////////////////////
MPEGNaviFileReader::~MPEGNaviFileReader()
{
	if (tsFile)
		fclose(tsFile);
	
	if (naviFile)
		fclose(naviFile);
	
	if (pNaviFileName)
		delete [] pNaviFileName;
	
	if (pNaviBuf)
		delete [] pNaviBuf;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// MPEGNaviFileReader::InitWithTSFile
/////////////////////////////////////////////////////////////////////////////////////////////
IOReturn MPEGNaviFileReader::InitWithTSFile(char *pTSFileName, bool failIfNoNaviFile)
{
	unsigned int cnt;
	UInt64 tsFileSize;
	
	pNaviFileName = new char[strlen(pTSFileName)+10]; // Slightly overallocate
	if (!pNaviFileName)
		return kIOReturnNoMemory;
	
	// Open the TS file
	tsFile = fopen(pTSFileName,"rb");
	if (tsFile == nil)
		return kIOReturnError;
	
	// Seek to the end of the TS file and determine its length
	fseeko(tsFile,0,SEEK_END);
	tsFileSize = ftello(tsFile);
	numTSPacketsInFile = tsFileSize/kMPEG2TSPacketSize;
	
	// Seek back to the beginning of the TS file
	fseeko(tsFile,0,SEEK_SET);
	
	// Determine the navi filename
	strcpy(pNaviFileName,pTSFileName);
	strcat(pNaviFileName,".tsnavi");
	
	// Open the Navi file
	naviFile = fopen(pNaviFileName,"rb");
	if (naviFile == nil)
	{
		hasNaviFile = false;

		if (failIfNoNaviFile == true)
			return kIOReturnError;
		
		// Fake the stream info, since we have no navi file
		frameHorizontalSize = 0;
		frameVerticalSize = 0;
		bitRate = 0;
		frameRate = MPEGFrameRate_Unknown;
		numFrames = 0;
		naviFileSize = 0;
		pFrameInfo = nil;
		pStreamInfo = nil;
	}
	else
	{
		// Seek to the end of the navi file and determine its length
		fseeko(naviFile,0,SEEK_END);
		naviFileSize = ftello(naviFile);
		
		// Seek back to the beginning of the navi file
		fseeko(naviFile,0,SEEK_SET);
		
		// Only use the navi file if it has a NaviFileStreamInfo and at least one NaviFileFrameInfo
		if (naviFileSize >= (sizeof(NaviFileStreamInfo)+sizeof(NaviFileFrameInfo)))
		{
			hasNaviFile = true;
			
			// Allocate memory to store the entire contents of the navi file in memory
			pNaviBuf = new UInt8[naviFileSize];
			if (!pNaviBuf)
				return kIOReturnNoMemory;
			
			// Read the navi file into memory
			cnt = fread(pNaviBuf,1,naviFileSize,naviFile);
			if (cnt != naviFileSize)
				return kIOReturnError;
			
			// Set the navi file frame pointer
			pFrameInfo = (NaviFileFrameInfo*) (pNaviBuf + sizeof(NaviFileStreamInfo));
			
			// Set the stream info pointer
			pStreamInfo = (NaviFileStreamInfo*) pNaviBuf;
			
			// Calculate number of frames
			numFrames = (naviFileSize - sizeof(NaviFileStreamInfo)) / sizeof(NaviFileFrameInfo);

			// Ensure that the navi data file is converted into native byte order!
			pStreamInfo->naviFileStructureRevision = EndianU32_BtoN(pStreamInfo->naviFileStructureRevision);
			pStreamInfo->frameHorizontalSize = EndianU32_BtoN(pStreamInfo->frameHorizontalSize);
			pStreamInfo->frameVerticalSize = EndianU32_BtoN(pStreamInfo->frameVerticalSize);
			pStreamInfo->bitRate = EndianU32_BtoN(pStreamInfo->bitRate);
			pStreamInfo->frameRate = (MPEGFrameRate) EndianU32_BtoN(pStreamInfo->frameRate);
			for (UInt32 i = 0; i < numFrames; i++)
			{
				pFrameInfo[i].frameTSPacketOffset = EndianU32_BtoN(pFrameInfo[i].frameTSPacketOffset);
				pFrameInfo[i].frameType = (MPEGFrameType) EndianU32_BtoN(pFrameInfo[i].frameType);
			}
			
			// Extract the stream info
			frameHorizontalSize = pStreamInfo->frameHorizontalSize;
			frameVerticalSize = pStreamInfo->frameVerticalSize;
			bitRate = pStreamInfo->bitRate;
			frameRate = pStreamInfo->frameRate;
		}
		else
		{
			hasNaviFile = false;
			
			if (failIfNoNaviFile == true)
				return kIOReturnError;
			
			// Fake the stream info, since we have no navi file
			frameHorizontalSize = 0;
			frameVerticalSize = 0;
			bitRate = 0;
			frameRate = MPEGFrameRate_Unknown;
			numFrames = 0;
			naviFileSize = 0;
			pFrameInfo = nil;
			pStreamInfo = nil;
		}
	}
	
	currentOffsetInTSPackets = 0;
	currentOffsetInFrames = 0;
	
	return kIOReturnSuccess; 
}

/////////////////////////////////////////////////////////////////////////////////////////////
// MPEGNaviFileReader::ReadNextTSPackets
/////////////////////////////////////////////////////////////////////////////////////////////
UInt32 MPEGNaviFileReader::ReadNextTSPackets(void *pBuffer, UInt32 numTSPackets)
{
	unsigned int cnt = 0;
	
	if (tsFile)
	{
		cnt = fread(pBuffer,188,numTSPackets,tsFile);
		
		// Update current offsets, timecode.
		currentOffsetInTSPackets += cnt;
		
		if (hasNaviFile == true)
		{
			for (;;)
			{
				if ((currentOffsetInFrames < numFrames) && (currentOffsetInTSPackets >= pFrameInfo[currentOffsetInFrames+1].frameTSPacketOffset))
					currentOffsetInFrames += 1;
				else
					break;
			}
		}
	}

	return cnt;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// MPEGNaviFileReader::SeekForwards
/////////////////////////////////////////////////////////////////////////////////////////////
IOReturn MPEGNaviFileReader::SeekForwards(UInt32 seconds)
{
	IOReturn result = kIOReturnError;
	UInt32 framesToSkip = seconds * FramesPerSecond(frameRate);
	UInt32 i = 0;
	UInt64 fileOffset;
	
	if (hasNaviFile == true)
	{
		if ((currentOffsetInFrames + framesToSkip) < numFrames)
		{
			while ((currentOffsetInFrames + framesToSkip + i) < numFrames)
			{
				if (pFrameInfo[currentOffsetInFrames + framesToSkip + i].frameType == kMPEGFrameType_I)
				{
					// We've found our target frame, seek to it, and set the correct time-code and file offset
					fileOffset = (UInt64) pFrameInfo[currentOffsetInFrames + framesToSkip + i].frameTSPacketOffset * (UInt64) kMPEG2TSPacketSize;
					fseeko(tsFile,fileOffset,SEEK_SET);
					currentOffsetInTSPackets = pFrameInfo[currentOffsetInFrames + framesToSkip + i].frameTSPacketOffset;
					currentOffsetInFrames = currentOffsetInFrames + framesToSkip + i;
					result = kIOReturnSuccess;
					break;
				}
				else
					i += 1;
			}
		}
	}
	
	return result;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// MPEGNaviFileReader::SeekBackwards
/////////////////////////////////////////////////////////////////////////////////////////////
IOReturn MPEGNaviFileReader::SeekBackwards(UInt32 seconds)
{
	IOReturn result = kIOReturnError;
	UInt32 framesToSkip = seconds * FramesPerSecond(frameRate);
	UInt32 i = 0;
	UInt64 fileOffset;
	
	if (hasNaviFile == true)
	{
		if ((int)(currentOffsetInFrames - framesToSkip) >= 0)
		{
			while ((currentOffsetInFrames - framesToSkip + i) < numFrames)
			{
				if (pFrameInfo[currentOffsetInFrames - framesToSkip + i].frameType == kMPEGFrameType_I)
				{
					// We've found our target frame, seek to it, and set the correct time-code and file offset
					fileOffset = (UInt64) pFrameInfo[currentOffsetInFrames - framesToSkip + i].frameTSPacketOffset * (UInt64) kMPEG2TSPacketSize;
					fseeko(tsFile,fileOffset,SEEK_SET);
					currentOffsetInTSPackets = pFrameInfo[currentOffsetInFrames - framesToSkip + i].frameTSPacketOffset;
					currentOffsetInFrames = currentOffsetInFrames - framesToSkip + i;
					result = kIOReturnSuccess;
					break;
				}
				else
					i += 1;
			}
		}
		else
		{
			SeekToBeginning();
			result = kIOReturnSuccess;
		}
	}
	
	return result;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// MPEGNaviFileReader::SeekToSpecificFrame
/////////////////////////////////////////////////////////////////////////////////////////////
IOReturn MPEGNaviFileReader::SeekToSpecificFrame(UInt32 frameOffset)
{
	IOReturn result = kIOReturnError;
	UInt32 i = 0;
	UInt64 fileOffset;
	
	if (hasNaviFile == true)
	{
		if (frameOffset < numFrames)
		{
			while ((frameOffset + i) < numFrames)
			{
				if (pFrameInfo[frameOffset + i].frameType == kMPEGFrameType_I)
				{
					// We've found our target frame, seek to it, and set the correct time-code and file offset
					fileOffset = (UInt64) pFrameInfo[frameOffset + i].frameTSPacketOffset * (UInt64) kMPEG2TSPacketSize;
					fseeko(tsFile,fileOffset,SEEK_SET);
					currentOffsetInTSPackets = pFrameInfo[frameOffset + i].frameTSPacketOffset;
					currentOffsetInFrames = frameOffset + i;
					result = kIOReturnSuccess;
					break;
				}
				else
					i += 1;
			}
		}
	}
	
	return result;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// MPEGNaviFileReader::SeekToBeginning
/////////////////////////////////////////////////////////////////////////////////////////////
IOReturn MPEGNaviFileReader::SeekToBeginning(void)
{
	fseeko(tsFile,0,SEEK_SET);
	currentOffsetInTSPackets = 0;
	currentOffsetInFrames = 0;

	return kIOReturnSuccess;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// MPEGNaviFileReader::GetCurrentTimeCodePositionInFrames
/////////////////////////////////////////////////////////////////////////////////////////////
UInt32 MPEGNaviFileReader::GetCurrentTimeCodePositionInFrames(void)
{
	return currentOffsetInFrames;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// MPEGNaviFileReader::isIFrameBoundary
/////////////////////////////////////////////////////////////////////////////////////////////
bool MPEGNaviFileReader::isIFrameBoundary(void)
{
	if (hasNaviFile == true)
	{
		if ((pFrameInfo[currentOffsetInFrames].frameType == kMPEGFrameType_I) && 
			(pFrameInfo[currentOffsetInFrames].frameTSPacketOffset == currentOffsetInTSPackets))
			return true;
		else
			return false;
	}
	else
		return false;	// If no navi file, we cannot deterimne frame boundaries.
}

/////////////////////////////////////////////////////////////////////////////////////////////
// MPEGNaviFileReader::GetStreamInfo
/////////////////////////////////////////////////////////////////////////////////////////////
IOReturn MPEGNaviFileReader::GetStreamInfo(UInt32 *pHorizontalResolution, 
										   UInt32 *pVerticalResolution, 
										   MPEGFrameRate *pFrameRate, 
										   UInt32 *pBitRate,
										   UInt32 *pNumFrames,
										   UInt32 *pNumTSPackets)
{
	*pHorizontalResolution = frameHorizontalSize;
	*pVerticalResolution = frameVerticalSize;
	*pBitRate = bitRate;
	*pFrameRate = frameRate;
	*pNumFrames = numFrames;
	*pNumTSPackets = numTSPacketsInFile;
	
	return kIOReturnSuccess; 
}

/////////////////////////////////////////////////////////////////////////////////////////////
// MPEGNaviFileReader::FileLenInTSPackets
/////////////////////////////////////////////////////////////////////////////////////////////
UInt32 MPEGNaviFileReader::FileLenInTSPackets(void)
{
	return numTSPacketsInFile;
}

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
///////
///////  
///////  MPEGNaviFileWriter Class Object
///////
///////
///////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////
// MPEGNaviFileWriter::MPEGNaviFileWriter
/////////////////////////////////////////////////////////////////////////////////////////////
MPEGNaviFileWriter::MPEGNaviFileWriter()
{
	tsFile = nil;
	naviFile = nil;
	frameHorizontalSize = 0;
	frameVerticalSize = 0;
	bitRate = 0;
	frameRate = MPEGFrameRate_Unknown;
	numFrames = 0;
	numTSPacketsStored = 0;
	firstIFrameFound = false;
	tsDemuxerHadFileWriteError = false;
	pTSDemuxer = nil;
	programIndex = 1;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// MPEGNaviFileWriter::~MPEGNaviFileWriter
/////////////////////////////////////////////////////////////////////////////////////////////
MPEGNaviFileWriter::~MPEGNaviFileWriter()
{
	if (tsFile)
		fclose(tsFile);
	
	if (naviFile)
		fclose(naviFile);

	if (pTSDemuxer)
		delete pTSDemuxer;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// MPEGNaviFileWriter::InitWithTSFile
/////////////////////////////////////////////////////////////////////////////////////////////
IOReturn MPEGNaviFileWriter::InitWithTSFile(char *pTSFileName, bool alsoCreateNaviFile)
{
	IOReturn result = kIOReturnSuccess;

	// First, make sure the files are not open
	if ((tsFile) || (naviFile))
		return kIOReturnExclusiveAccess;
	
	// Allocate memory for the navi filename string
	char *pNaviFileName = new char[strlen(pTSFileName)+10]; // Slightly overallocate
	if (!pNaviFileName)
		result = kIOReturnNoMemory;
	
	// Create the TS file
	if (result == kIOReturnSuccess)
	{
		tsFile = fopen(pTSFileName,"wb");
		if (tsFile == nil)
			result = kIOReturnError;
	}
	
	if (alsoCreateNaviFile == true)
	{
		// Create the navi file
		if (result == kIOReturnSuccess)
		{
			strcpy(pNaviFileName,pTSFileName);
			strcat(pNaviFileName,".tsnavi");
			naviFile = fopen(pNaviFileName,"wb");
			if (naviFile == nil)
				result = kIOReturnError;
		}
		
		// Create the demuxer
		if (result == kIOReturnSuccess)
		{
			pTSDemuxer = new TSDemuxer(MPEGNaviFileWriter::PESCallback,
									   this,
									   nil,
									   nil,
									   programIndex,
									   564, // Only need to have the first few TS packets in the frame to determine frame type
									   0);	// We don't need audio PES packets
			
			if (!pTSDemuxer)
				result = kIOReturnNoMemory;
			else
			{
				pTSDemuxer->SetDemuxerConfigurationBits(kDemuxerConfig_PartialDemux);
				firstIFrameFound = false;
				tsDemuxerHadFileWriteError = false;
				frameHorizontalSize = 0;
				frameVerticalSize = 0;
				bitRate = 0;
				frameRate = MPEGFrameRate_Unknown;
				numFrames = 0;
				numTSPacketsStored = 0;
			}
		}
	}
	
	if (pNaviFileName)
		delete [] pNaviFileName;

	return result;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// MPEGNaviFileWriter::WriteNextTSPackets
/////////////////////////////////////////////////////////////////////////////////////////////
UInt32 MPEGNaviFileWriter::WriteNextTSPackets(void *pBuffer, UInt32 numTSPackets)
{
	UInt32 i;
	unsigned int cnt = 0;
	IOReturn result = kIOReturnSuccess;
	UInt8 *pByteBuf = (UInt8*) pBuffer;
	
	// First, make sure the file is open
	if (!tsFile)
		return 0;
	
	// Loop for each TS packet
	for (i=0;i<numTSPackets;i++)
	{
		// Write the TS packet to the transport stream file
		cnt = fwrite(&pByteBuf[188*i],kMPEG2TSPacketSize,1,tsFile);
		if (cnt != 1)
		{
			result = kIOReturnError;
			break;
		}
		else
		{
			if (pTSDemuxer != nil)
			{
				// Send the TS packet to the demuxer
				pTSDemuxer->nextTSPacket(&pByteBuf[188*i],numTSPacketsStored);
				
				// See if the demuxer had a file write error
				if (tsDemuxerHadFileWriteError == true)
				{
					result = kIOReturnError;
					break;
				}
				
			}
			
			// Bump the packet count
			numTSPacketsStored += 1;
		}
	}
	
	if (result == kIOReturnSuccess)
		return cnt;
	else
		return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// MPEGNaviFileWriter::CloseFiles
/////////////////////////////////////////////////////////////////////////////////////////////
IOReturn MPEGNaviFileWriter::CloseFiles(void)
{
	// First, make sure the files are open
	if ((!tsFile) || (!naviFile))
		return kIOReturnNotOpen;
	
	if (tsFile)
	{
		fclose(tsFile);
		tsFile = nil;
	}
	
	if (naviFile)
	{
		fclose(naviFile);
		naviFile = nil;
	}
	
	if (pTSDemuxer)
	{
		delete pTSDemuxer;
		pTSDemuxer = nil;
	}
	
	return kIOReturnSuccess;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// MPEGNaviFileWriter::GetCurrentTimeCodePositionInFrames
/////////////////////////////////////////////////////////////////////////////////////////////
UInt32 MPEGNaviFileWriter::GetCurrentTimeCodePositionInFrames(void)
{
	return numFrames;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// MPEGNaviFileWriter::GetStreamInfo
/////////////////////////////////////////////////////////////////////////////////////////////
IOReturn MPEGNaviFileWriter::GetStreamInfo(UInt32 *pHorizontalResolution, 
										   UInt32 *pVerticalResolution, 
										   MPEGFrameRate *pFrameRate, 
										   UInt32 *pBitRate, 
										   UInt32 *pNumFrames,
										   UInt32 *pNumTSPackets)
{
	*pHorizontalResolution = frameHorizontalSize;
	*pVerticalResolution = frameVerticalSize;
	*pBitRate = bitRate;
	*pFrameRate = frameRate;
	*pNumFrames = numFrames;
	*pNumTSPackets = numTSPacketsStored;
	
	return kIOReturnSuccess; 
}

/////////////////////////////////////////////////////////////////////////////////////////////
// MPEGNaviFileWriter::FileLenInTSPackets
/////////////////////////////////////////////////////////////////////////////////////////////
UInt32 MPEGNaviFileWriter::FileLenInTSPackets(void)
{
	return numTSPacketsStored;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// MPEGNaviFileWriter::PESCallback
/////////////////////////////////////////////////////////////////////////////////////////////
IOReturn MPEGNaviFileWriter::PESCallback(TSDemuxerMessage msg, PESPacketBuf* pPESPacket,void *pRefCon)
{
	MPEGNaviFileWriter *pFileWriter = (MPEGNaviFileWriter*) pRefCon;
	
	TSDemuxerStreamType streamType = pPESPacket->streamType;
	UInt8 *pPESBuf = pPESPacket->pPESBuf;
	UInt32 pesBufLen = pPESPacket->pesBufLen;
	UInt32 i;
	UInt32 strid = 0;
	NaviFileStreamInfo streamInfo;
	NaviFileStreamInfo streamInfoBigEndian;
	NaviFileFrameInfo frameInfo;
	NaviFileFrameInfo frameInfoBigEndian;
	unsigned int cnt;
	
	streamInfo.naviFileStructureRevision = kNaviFileStructureRevision_1;
	streamInfo.frameHorizontalSize = 0;
	streamInfo.frameVerticalSize = 0;
	streamInfo.bitRate = 0;
	streamInfo.frameRate = MPEGFrameRate_Unknown;
	
	frameInfo.frameTSPacketOffset = pFileWriter->numTSPacketsStored;
	frameInfo.frameType = kMPEGFrameType_Unknown;
	
	if (((msg == kTSDemuxerPESReceived) || (msg == kTSDemuxerPESLargerThanAllocatedBuffer)) && (streamType == kTSDemuxerStreamTypeVideo))
	{
		// Get info about this frame
		for (i = 9+pPESBuf[8]; i < pesBufLen-4; i++)
		{
			strid = (strid << 8) | pPESBuf[i];
			if (strid == 0x000001B3) // sequence_header_code
			{
				// Found a Sequence Header!
				
				// Get the frame resolution
				streamInfo.frameHorizontalSize = (int)( ((UInt32)pPESBuf[i+1] << 4) + ((UInt32)pPESBuf[i+2] >> 4));
				streamInfo.frameVerticalSize = (int)( (((UInt32)pPESBuf[i+2] & 0x0F) << 8) + (UInt32)pPESBuf[i+3]);
				pFileWriter->frameHorizontalSize = streamInfo.frameHorizontalSize;
				pFileWriter->frameVerticalSize = streamInfo.frameVerticalSize;
				
				// Get the frame rate
				switch((pPESBuf[i+4] & 0x0F))
				{
					case 1:	// Frame Rate:      23.976 fps
						streamInfo.frameRate = MPEGFrameRate_23_976;
						break;
						
					case 2: // Frame Rate:      24 fps
						streamInfo.frameRate = MPEGFrameRate_24;
						break;
						
					case 3: // Frame Rate:      25 fps
						streamInfo.frameRate = MPEGFrameRate_25;
						break;
						
					case 4: // Frame Rate:      29.97 fps
						streamInfo.frameRate = MPEGFrameRate_29_97;
						break;
						
					case 5: // Frame Rate:      30 fps
						streamInfo.frameRate = MPEGFrameRate_30;
						break;
						
					case 6: // Frame Rate:      50 fps
						streamInfo.frameRate = MPEGFrameRate_50;
						break;
						
					case 7: // Frame Rate:      59.94 fps
						streamInfo.frameRate = MPEGFrameRate_59_94;
						break;
						
					case 8:  // Frame Rate:      60 fps
						streamInfo.frameRate = MPEGFrameRate_60;
						break;
						
					default:
						break;
				};
				pFileWriter->frameRate = streamInfo.frameRate;
				
				// Get the bit-rate code
				streamInfo.bitRate = ((pPESBuf[i+5] << 10)+(pPESBuf[i+6] << 2)+((pPESBuf[i+7] & 0xC0) >> 6))*400;
				pFileWriter->bitRate = streamInfo.bitRate;
			}
			else if (strid == 0x00000100) // picture_start_code
			{
				// picture_header, determine what kind of picture it is
				if (i<(pesBufLen-3))
				{
					// check if picture_coding_type == 1
					if ((pPESBuf[i+2] & (0x7 << 3)) == (1 << 3))
					{
						frameInfo.frameType = kMPEGFrameType_I;
						break; // We can break out of the for loop now
					}
					else if ((pPESBuf[i+2] & (0x7 << 3)) == (2 << 3))
					{
						frameInfo.frameType = kMPEGFrameType_P;
						break; // We can break out of the for loop now
					}
					else if ((pPESBuf[i+2] & (0x7 << 3)) == (3 << 3))
					{
						frameInfo.frameType =kMPEGFrameType_B;
						break; // We can break out of the for loop now
					}
				}
			}
		}
		
		if (pFileWriter->firstIFrameFound == false)
		{
			if (frameInfo.frameType == kMPEGFrameType_I)
			{
				// We've found our first I Frame
				
				// Write the stream info to the file
				
				// First, ensure the navi file data is saved in big-endian format
				streamInfoBigEndian.naviFileStructureRevision = EndianU32_NtoB(streamInfo.naviFileStructureRevision);
				streamInfoBigEndian.frameHorizontalSize = EndianU32_NtoB(streamInfo.frameHorizontalSize);
				streamInfoBigEndian.frameVerticalSize = EndianU32_NtoB(streamInfo.frameVerticalSize);
				streamInfoBigEndian.bitRate = EndianU32_NtoB(streamInfo.bitRate);
				streamInfoBigEndian.frameRate = (MPEGFrameRate) EndianU32_NtoB(streamInfo.frameRate);
				
				// Write To Navigation File
				cnt = fwrite(&streamInfoBigEndian,sizeof(NaviFileStreamInfo),1,pFileWriter->naviFile);
				if (cnt != 1)
				{
					pFileWriter->tsDemuxerHadFileWriteError = true;
				}
				else
				{
					pFileWriter->firstIFrameFound = true;
				}
			}
		}
		
		if ((pFileWriter->firstIFrameFound == true) && (pFileWriter->tsDemuxerHadFileWriteError == false) && (frameInfo.frameType != kMPEGFrameType_Unknown))
		{
			// First, ensure the navi file data is saved in big-endian format
			frameInfoBigEndian.frameTSPacketOffset = EndianU32_NtoB(frameInfo.frameTSPacketOffset);
			frameInfoBigEndian.frameType = (MPEGFrameType) EndianU32_NtoB(frameInfo.frameType);
			
			// Write this frame info tho the file
			cnt = fwrite(&frameInfoBigEndian,sizeof(NaviFileFrameInfo),1,pFileWriter->naviFile);
			if (cnt != 1)
			{
				pFileWriter->tsDemuxerHadFileWriteError = true;
			}
			else
			{
				// Bump the frame count
				pFileWriter->numFrames += 1;
			}
		}
	}
	else if (msg == kTSDemuxerRescanningForPSI)
	{
		pFileWriter->programIndex += 1;
		if (pFileWriter->programIndex > kMaxAutoPSIDetectProgramIndex)
			pFileWriter->programIndex = 1; // Note that the PSITables program index is one based!
		printf("MPEGNaviFileWriter auto-PSI switching to next PAT program (%d)\n",(int)pFileWriter->programIndex);
		pPESPacket->pTSDemuxer->psiTables->selectProgram(pFileWriter->programIndex); 
		pPESPacket->pTSDemuxer->psiTables->ResetPSITables();
	}
	
	pPESPacket->pTSDemuxer->ReleasePESPacketBuf(pPESPacket);
	return kIOReturnSuccess;
}

/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
///////
///////  
///////  NaviFileCreator Class Object
///////
///////
///////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////
// NaviFileCreator::NaviFileCreator
/////////////////////////////////////////////////////////////////////////////////////////////
NaviFileCreator::NaviFileCreator()
{
	percentageComplete = 0;
	previousPercentageComplete = 0;
	clientProgressCallback = nil;
	pClientRefCon = nil;
	horizontalResolution = 0;
	verticalResolution = 0;
	frameRate = MPEGFrameRate_Unknown;
	iFrames = 0;
	pFrames = 0;
	bFrames = 0;
	bitRate = 0;
	pInFile = nil;
	pOutFile = nil;
	firstIFrameFound = false;
	streamTSPacketNumber = 0;
	tsDemuxerHadFileWriteError = false;
	tsFileSize = 0;
	tsFileSizeInPackets = 0;
	programIndex = 1;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// NaviFileCreator::~NaviFileCreator
/////////////////////////////////////////////////////////////////////////////////////////////
NaviFileCreator::~NaviFileCreator()
{
	
}

/////////////////////////////////////////////////////////////////////////////////////////////
// NaviFileCreator::~RegisterProgressNotificationCallback
/////////////////////////////////////////////////////////////////////////////////////////////
void NaviFileCreator::RegisterProgressNotificationCallback(NaviFileCreatorProgressCallback progressCallback, void *pRefCon)
{
	clientProgressCallback = progressCallback;
	pClientRefCon = pRefCon;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// NaviFileCreator::CreateMPEGNavigationFileForTSFile
/////////////////////////////////////////////////////////////////////////////////////////////
IOReturn NaviFileCreator::CreateMPEGNavigationFileForTSFile(char *pTSFilename)
{
	IOReturn result = kIOReturnSuccess;
	TSDemuxer *pTSDemuxer = nil;
	unsigned int cnt;
	UInt8 *pTSPacket = nil;
	
	percentageComplete = 0;
	previousPercentageComplete = 0;
	
	char *pOutFileName = new char[strlen(pTSFilename)+10]; // Slightly overallocate
	if (!pOutFileName)
		result = kIOReturnNoMemory;
	
	if (result == kIOReturnSuccess)
	{
		// Create the file read buffer
		pTSPacket = new UInt8[kMPEG2TSPacketSize];
		if (!pTSPacket)
			result = kIOReturnNoMemory;
	}
	
	if (result == kIOReturnSuccess)
	{
		pInFile = fopen(pTSFilename,"rb");
		if (pInFile == nil)
			result = kIOReturnError;
	}
	
	if (result == kIOReturnSuccess)
	{
		// Seek to the end of the TS file and determine its length
		fseeko(pInFile,0,SEEK_END);
		tsFileSize = ftello(pInFile);
		tsFileSizeInPackets = tsFileSize/kMPEG2TSPacketSize;
	
		// Seek back to the beginning of the TS file
		fseeko(pInFile,0,SEEK_SET);
	}
	
	if (result == kIOReturnSuccess)
	{
		strcpy(pOutFileName,pTSFilename);
		strcat(pOutFileName,".tsnavi");
		pOutFile = fopen(pOutFileName,"wb");
		if (pOutFile == nil)
			result = kIOReturnError;
	}
	
	if (result == kIOReturnSuccess)
	{
		// Create TS demuxer
		pTSDemuxer = new TSDemuxer(NaviFileCreator::PESCallback,
								   this,
								   nil,
								   nil,
								   programIndex,
								   564, // Only need to have the first few TS packets in the frame to determine frame type
								   0);	// We don't need audio PES packets
		
		if (!pTSDemuxer)
			result = kIOReturnNoMemory;
		else
		{
			pTSDemuxer->SetDemuxerConfigurationBits(kDemuxerConfig_PartialDemux);
			firstIFrameFound = false;
			streamTSPacketNumber = 0;
			tsDemuxerHadFileWriteError = false;
			horizontalResolution = 0;
			verticalResolution = 0;
			frameRate = MPEGFrameRate_Unknown;
			iFrames = 0;
			pFrames = 0;
			bFrames = 0;
		}
	}
	
	if (result == kIOReturnSuccess)
	{
		for (;;)
		{
			// Read the next packet
			cnt = fread(pTSPacket,1,kMPEG2TSPacketSize,pInFile);
			if (cnt != kMPEG2TSPacketSize)
			{
				// Got a read error, so we've most likely reached the end of the file
				break;
			}
			else
			{
				// Pass the packet to the demuxer
				pTSDemuxer->nextTSPacket(pTSPacket,streamTSPacketNumber);
				
				if (tsDemuxerHadFileWriteError == true)
				{
					result = kIOReturnError;
					break;
				}
				
				// Bump the packet count
				streamTSPacketNumber += 1;
				
				// Calculate new percentageComplete
				percentageComplete = (UInt32)(((1.0*streamTSPacketNumber)/(1.0*tsFileSizeInPackets))*100);
				
				// See if we should notify the client of progress
				if ((clientProgressCallback != nil) && (percentageComplete != previousPercentageComplete))
				{
					clientProgressCallback(percentageComplete,pClientRefCon);
					previousPercentageComplete = percentageComplete;
				}
			}
		}
	}
	
	if (pOutFile)
		fclose(pOutFile);
	
	if (pInFile)
		fclose(pInFile);
	
	if (pTSPacket)
		delete [] pTSPacket;
	
	if (pOutFileName)
		delete [] pOutFileName;
	
	if (pTSDemuxer)
		delete pTSDemuxer;
	
	return result;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// NaviFileCreator::PESCallback
/////////////////////////////////////////////////////////////////////////////////////////////
IOReturn NaviFileCreator::PESCallback(TSDemuxerMessage msg, PESPacketBuf* pPESPacket,void *pRefCon)
{
	TSDemuxerStreamType streamType = pPESPacket->streamType;
	UInt8 *pPESBuf = pPESPacket->pPESBuf;
	UInt32 pesBufLen = pPESPacket->pesBufLen;
	UInt32 i;
	UInt32 strid = 0;
	NaviFileStreamInfo streamInfo;
	NaviFileStreamInfo streamInfoBigEndian;
	NaviFileFrameInfo frameInfo;
	NaviFileFrameInfo frameInfoBigEndian;
	unsigned int cnt;
	NaviFileCreator *pCreator = (NaviFileCreator*) pRefCon;
	
	streamInfo.naviFileStructureRevision = kNaviFileStructureRevision_1;
	streamInfo.frameHorizontalSize = 0;
	streamInfo.frameVerticalSize = 0;
	streamInfo.bitRate = 0;
	streamInfo.frameRate = MPEGFrameRate_Unknown;
	
	frameInfo.frameTSPacketOffset = pCreator->streamTSPacketNumber;
	frameInfo.frameType = kMPEGFrameType_Unknown;
	
	
	if (((msg == kTSDemuxerPESReceived) || (msg == kTSDemuxerPESLargerThanAllocatedBuffer)) && (streamType == kTSDemuxerStreamTypeVideo))
	{
		// Get info about this frame
		for (i = 9+pPESBuf[8]; i < pesBufLen-4; i++)
		{
			strid = (strid << 8) | pPESBuf[i];
			if (strid == 0x000001B3) // sequence_header_code
			{
				// Found a Sequence Header!
				
				// Get the frame resolution
				streamInfo.frameHorizontalSize = (int)( ((UInt32)pPESBuf[i+1] << 4) + ((UInt32)pPESBuf[i+2] >> 4));
				streamInfo.frameVerticalSize = (int)( (((UInt32)pPESBuf[i+2] & 0x0F) << 8) + (UInt32)pPESBuf[i+3]);
				
				pCreator->horizontalResolution = streamInfo.frameHorizontalSize;
				pCreator->verticalResolution = streamInfo.frameVerticalSize;

				// Get the frame rate
				switch((pPESBuf[i+4] & 0x0F))
				{
					case 1:	// Frame Rate:      23.976 fps
						streamInfo.frameRate = MPEGFrameRate_23_976;
						break;
						
					case 2: // Frame Rate:      24 fps
						streamInfo.frameRate = MPEGFrameRate_24;
						break;
						
					case 3: // Frame Rate:      25 fps
						streamInfo.frameRate = MPEGFrameRate_25;
						break;
						
					case 4: // Frame Rate:      29.97 fps
						streamInfo.frameRate = MPEGFrameRate_29_97;
						break;
						
					case 5: // Frame Rate:      30 fps
						streamInfo.frameRate = MPEGFrameRate_30;
						break;
						
					case 6: // Frame Rate:      50 fps
						streamInfo.frameRate = MPEGFrameRate_50;
						break;
						
					case 7: // Frame Rate:      59.94 fps
						streamInfo.frameRate = MPEGFrameRate_59_94;
						break;
						
					case 8:  // Frame Rate:      60 fps
						streamInfo.frameRate = MPEGFrameRate_60;
						break;
						
					default:
						break;
				};
				
				pCreator->frameRate = streamInfo.frameRate;
				
				// Get the bit-rate code
				streamInfo.bitRate = ((pPESBuf[i+5] << 10)+(pPESBuf[i+6] << 2)+((pPESBuf[i+7] & 0xC0) >> 6))*400;
				pCreator->bitRate = streamInfo.bitRate;

			}
			else if (strid == 0x00000100) // picture_start_code
			{
				// picture_header, determine what kind of picture it is
				if (i<(pesBufLen-3))
				{
					// check if picture_coding_type == 1
					if ((pPESBuf[i+2] & (0x7 << 3)) == (1 << 3))
					{
						frameInfo.frameType = kMPEGFrameType_I;
						pCreator->iFrames += 1;
						break; // We can break out of the for loop now
					}
					else if ((pPESBuf[i+2] & (0x7 << 3)) == (2 << 3))
					{
						frameInfo.frameType = kMPEGFrameType_P;
						if (pCreator->firstIFrameFound == true)
							pCreator->pFrames += 1;
						break; // We can break out of the for loop now
					}
					else if ((pPESBuf[i+2] & (0x7 << 3)) == (3 << 3))
					{
						frameInfo.frameType =kMPEGFrameType_B;
						if (pCreator->firstIFrameFound == true)
							pCreator->bFrames += 1;
						break; // We can break out of the for loop now
					}
				}
			}
		}
		
		if (pCreator->firstIFrameFound == false)
		{
			if (frameInfo.frameType == kMPEGFrameType_I)
			{
				// We've found our first I Frame
				
				// Write the stream info to the file
				
				// First, ensure the navi file data is saved in big-endian format
				streamInfoBigEndian.naviFileStructureRevision = EndianU32_NtoB(streamInfo.naviFileStructureRevision);
				streamInfoBigEndian.frameHorizontalSize = EndianU32_NtoB(streamInfo.frameHorizontalSize);
				streamInfoBigEndian.frameVerticalSize = EndianU32_NtoB(streamInfo.frameVerticalSize);
				streamInfoBigEndian.bitRate = EndianU32_NtoB(streamInfo.bitRate);
				streamInfoBigEndian.frameRate = (MPEGFrameRate) EndianU32_NtoB(streamInfo.frameRate);
				
				// Write To Navigation File
				cnt = fwrite(&streamInfoBigEndian,sizeof(NaviFileStreamInfo),1,pCreator->pOutFile);
				if (cnt != 1)
				{
					pCreator->tsDemuxerHadFileWriteError = true;
				}
				else
				{
					pCreator->firstIFrameFound = true;
				}
			}
		}
		
		if ((pCreator->firstIFrameFound == true) && (pCreator->tsDemuxerHadFileWriteError == false) && (frameInfo.frameType != kMPEGFrameType_Unknown))
		{
			// First, ensure the navi file data is saved in big-endian format
			frameInfoBigEndian.frameTSPacketOffset = EndianU32_NtoB(frameInfo.frameTSPacketOffset);
			frameInfoBigEndian.frameType = (MPEGFrameType) EndianU32_NtoB(frameInfo.frameType);
			
			// Write this frame info tho the file
			cnt = fwrite(&frameInfoBigEndian,sizeof(NaviFileFrameInfo),1,pCreator->pOutFile);
			if (cnt != 1)
			{
				pCreator->tsDemuxerHadFileWriteError = true;
			}
		}
	}
	else if (msg == kTSDemuxerRescanningForPSI)
	{
		pCreator->programIndex += 1;
		if (pCreator->programIndex > kMaxAutoPSIDetectProgramIndex)
			pCreator->programIndex = 1; // Note that the PSITables program index is one based!
		printf("NaviFileCreator auto-PSI switching to next PAT program (%d)\n",(int)pCreator->programIndex);
		pPESPacket->pTSDemuxer->psiTables->selectProgram(pCreator->programIndex); 
		pPESPacket->pTSDemuxer->psiTables->ResetPSITables();
	}
	
	pPESPacket->pTSDemuxer->ReleasePESPacketBuf(pPESPacket);
	return kIOReturnSuccess;
}


/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
///////
///////  
///////  MPEGTrickMode_RepositionFilePointerForward
///////
///////
///////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////

static IOReturn RepositionFilePointerForward_PESCallback(TSDemuxerMessage msg, PESPacketBuf* pPESPacket,void *pRefCon);

// Local Structures
typedef struct RepositionFilePointerForward_PrivStruct
{
	FILE *pInFile;
	UInt32 timeInSeconds;
	bool repositionDone;
	UInt8 frameRateInFrames;
	bool firstIFrameFound;
	bool searchingForTargetIFrame;
	UInt32 totalFramesToSkip;
	UInt32 numSkippedFrames;
	UInt64 seekLocation;
}RepositionFilePointerForward_Priv;

/////////////////////////////////////////////////////////////////////////////////////////////
// MPEGTrickMode_RepositionFilePointerForward
/////////////////////////////////////////////////////////////////////////////////////////////
IOReturn MPEGTrickMode_RepositionFilePointerForward(FILE *pInFile, UInt32 timeInSeconds, UInt32 *pTotalSkippedFrames)
{
	IOReturn result = kIOReturnSuccess;
	TSDemuxer *pTSDemuxer = nil;
	UInt8 *pTSPacket;
	unsigned int cnt;
	UInt64 currentFilePositionInBytes;
	UInt32 currentFilePositionInTSPackets;
	
	*pTotalSkippedFrames = 0;
	
	// Create the file read buffer
	pTSPacket = new UInt8[kMPEG2TSPacketSize];
	if (!pTSPacket)
		return kIOReturnNoMemory;
	
	// Create private data struct;
	RepositionFilePointerForward_Priv *pPriv = new RepositionFilePointerForward_Priv;
	if (!pPriv)
		result = kIOReturnNoMemory;
	else
	{
		// Initialize the private data struct
		pPriv->timeInSeconds = timeInSeconds;
		pPriv->pInFile = pInFile;
		pPriv->repositionDone = false;
		pPriv->frameRateInFrames = 30;
		pPriv->firstIFrameFound = false;
		pPriv->searchingForTargetIFrame = false;
		pPriv->totalFramesToSkip = 0;
		pPriv->numSkippedFrames = 0;
		pPriv->seekLocation = 0;
	}
	
	if (result == kIOReturnSuccess)
	{
		// Create TS demuxer
		pTSDemuxer = new TSDemuxer(RepositionFilePointerForward_PESCallback,
								   pPriv,
								   nil,
								   nil,
								   1,	// programNum
								   564, // Only need to have the first few TS packets in the frame to determine frame type
								   0);	// We don't need audio PES packets
		
		if (!pTSDemuxer)
			result = kIOReturnNoMemory;
		else
		{
			pTSDemuxer->SetDemuxerConfigurationBits(kDemuxerConfig_PartialDemux);
		}
	}
	
	if (result == kIOReturnSuccess)
	{
		// Get the current file position
		currentFilePositionInBytes = ftello(pInFile);
		currentFilePositionInTSPackets = currentFilePositionInBytes / kMPEG2TSPacketSize;
		
		for (;;)
		{
			// Read the next packet
			cnt = fread(pTSPacket,1,kMPEG2TSPacketSize,pInFile);
			if (cnt != kMPEG2TSPacketSize)
			{
				// Got a read error, so we've most likely reached the end of the file
				result = kIOReturnNotReadable;
				break;
			}
			else
			{
				// Pass the packet to the demuxer
				pTSDemuxer->nextTSPacket(pTSPacket,currentFilePositionInTSPackets);
				
				currentFilePositionInTSPackets++;
				
				// Are we done (this var set in the demuxer PES callback)
				if (pPriv->repositionDone == true)
				{
					*pTotalSkippedFrames = pPriv->numSkippedFrames;
					break;
				}
			}
		}
	}
	
	if (pTSDemuxer)
		delete pTSDemuxer;
	
	if (pPriv)
		delete pPriv;
	
	if (pTSPacket)
		delete [] pTSPacket;
	
	return result;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// RepositionFilePointerForward_PESCallback
/////////////////////////////////////////////////////////////////////////////////////////////
IOReturn RepositionFilePointerForward_PESCallback(TSDemuxerMessage msg, PESPacketBuf* pPESPacket,void *pRefCon)
{
	RepositionFilePointerForward_Priv *pPriv = (RepositionFilePointerForward_Priv*) pRefCon;
	TSDemuxerStreamType streamType = pPESPacket->streamType;
	UInt8 *pPESBuf = pPESPacket->pPESBuf;
	UInt32 pesBufLen = pPESPacket->pesBufLen;
	UInt32 i;
	UInt32 strid = 0;
	
	/////////////////////////////////////////////////////////////////////////////////////////////
	// First, we wait for an I frame.
	// Then when we get an I frame we determine the number of frames per second
	// and calculate the number of frames we need to skip.
	// When we've skipped the right number of frames, we need to determine the offset of the
	// first TS packet corresonding to the next I frame (if its not the current frame we need
	// to keep searching for the next I frame).
	// Then when we have determined the offset of the desired TS packet, we use fseeko
	// to reposition the file, and set the flag indicating we're done.
	/////////////////////////////////////////////////////////////////////////////////////////////
	
	if (((msg == kTSDemuxerPESReceived) || (msg == kTSDemuxerPESLargerThanAllocatedBuffer)) && (streamType == kTSDemuxerStreamTypeVideo))
	{
		// Account for this frame
		pPriv->numSkippedFrames += 1;
		
		if (pPriv->firstIFrameFound == false)
		{
			// Look for an I frame, and get the framerate
			for (i = 9+pPESBuf[8]; i < pesBufLen-4; i++)
			{
				strid = (strid << 8) | pPESBuf[i];
				if (strid == 0x000001B3) // sequence_header_code
				{
					// Found a Sequence Header!
					
					// Get the frame rate
					switch((pPESBuf[i+4] & 0x0F))
					{
						case 1:	// Frame Rate:      23.976 fps
						case 2: // Frame Rate:      24 fps
							pPriv->frameRateInFrames = 24;
							break;
							
						case 3: // Frame Rate:      25 fps
							pPriv->frameRateInFrames = 25;
							break;
							
						case 4: // Frame Rate:      29.97 fps
						case 5: // Frame Rate:      30 fps
							pPriv->frameRateInFrames = 30;
							break;
							
						case 6: // Frame Rate:      50 fps
							pPriv->frameRateInFrames = 50;
							break;
							
						case 7: // Frame Rate:      59.94 fps
						case 8:  // Frame Rate:      60 fps
							pPriv->frameRateInFrames = 60;
							break;
							
						default:
							break;
					};
				}
				else if (strid == 0x00000100) // picture_start_code
				{
					// picture_header, determine what kind of picture it is
					if (i<(pesBufLen-3))
					{
						// check if picture_coding_type == 1
						if ((pPESBuf[i+2] & (0x7 << 3)) == (1 << 3))
						{
							// Frame is an I Frame
							
							pPriv->firstIFrameFound = true;
							
							// Calculate the number of frames to skip
							pPriv->totalFramesToSkip = pPriv->frameRateInFrames * pPriv->timeInSeconds;
							
							// Account for this frame
							pPriv->numSkippedFrames = 1;
							
							// We can break out of the for loop now
							break;
						}
						else
						{
							// Not an I Frame
							
							// We can break out of the for loop now
							break;
						}
					}
				}
			}
		}
		else
		{
			if (pPriv->numSkippedFrames > pPriv->totalFramesToSkip)
				pPriv->searchingForTargetIFrame = true;
		}
		
		if (pPriv->searchingForTargetIFrame == true)
		{
			// Look for an I frame
			for (i = 9+pPESBuf[8]; i < pesBufLen-4; i++)
			{
				strid = (strid << 8) | pPESBuf[i];
				if (strid == 0x00000100) // picture_start_code
				{
					// picture_header, determine what kind of picture it is
					if (i<(pesBufLen-3))
					{
						// check if picture_coding_type == 1
						if ((pPESBuf[i+2] & (0x7 << 3)) == (1 << 3))
						{
							// We found the target I Frame !
							
							// Calculate file position for the first packet of this frame
							pPriv->seekLocation = (UInt64) pPESPacket->startTSPacketTimeStamp * kMPEG2TSPacketSize;
							
							// Seek to the desired file posiiton
							fseeko(pPriv->pInFile,pPriv->seekLocation,SEEK_SET);
							
							// We're done !!!!
							pPriv->repositionDone = true;
							
							// We can break out of the for loop now
							break;
						}
						else
						{
							// Not an I Frame
							
							// We can break out of the for loop now
							break;
						}
					}
				}
			}
		}
	}
	
	pPESPacket->pTSDemuxer->ReleasePESPacketBuf(pPESPacket);
	return kIOReturnSuccess;
}

} // namespace AVS




