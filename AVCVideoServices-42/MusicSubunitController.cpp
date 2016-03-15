/*
	File:   MusicSubunitController.cpp
 
 Synopsis: This is the sourcecode for the MusicSubunitController Class
 
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

// Macros to simplify logging 
#define DoLoggerLog( x... ) { if (pLogger) pLogger->log( x ) ; }
#define DoIndentedLoggerLog( y,x... ) { if (pLogger) { DoIndentation(y,pLogger); pLogger->log( x ) ; } }

namespace AVS
{

#pragma mark -----------------------------------
#pragma mark MusicSubunitController Class Methods
#pragma mark -----------------------------------
	
///////////////////////////////
// Constructor
///////////////////////////////
MusicSubunitController::MusicSubunitController(AVCDevice *pDevice, UInt8 subUnitID, StringLogger *pStringLogger)
{
	pLogger = pStringLogger;
	pAVCDevice = pDevice;
	pAVCDeviceCommandInterface = nil;
	subunitTypeAndID = 0x48 + (subUnitID & 0x07);
	Initialize();
}

///////////////////////////////
// Alternate Constructor
///////////////////////////////
MusicSubunitController::MusicSubunitController(AVCDeviceCommandInterface *pDeviceCommandInterface, UInt8 subUnitID, StringLogger *pStringLogger)
{
	pLogger = pStringLogger;
	pAVCDeviceCommandInterface = pDeviceCommandInterface;
	pAVCDevice = pDeviceCommandInterface->GetAVCDevice();
	subunitTypeAndID = 0x48 + (subUnitID & 0x07);
	Initialize();
}

///////////////////////////////
// Destructor
///////////////////////////////
MusicSubunitController::~MusicSubunitController()
{
	CleanUpDiscoveryResources();
}

///////////////////////////////////////////////////////////
// MusicSubunitController::CleanUpDiscoveryResources
///////////////////////////////////////////////////////////
void MusicSubunitController::CleanUpDiscoveryResources(void)
{
	AVCInfoBlock *pAVCInfoBlock;
	
	if (pIsoInPlugs != nil)
	{
		delete [] pIsoInPlugs;
		pIsoInPlugs = nil;
	}
	
	if (pIsoOutPlugs != nil)
	{
		delete [] pIsoOutPlugs;
		pIsoOutPlugs = nil;
	}
	
	if (pExtInPlugs != nil)
	{
		delete [] pExtInPlugs;
		pExtInPlugs = nil;
	}
	
	if (pExtOutPlugs != nil)
	{
		delete [] pExtOutPlugs;
		pExtOutPlugs = nil;
	}
	
	if (pMusicSubunitDestPlugs != nil)
	{
		delete [] pMusicSubunitDestPlugs;
		pMusicSubunitDestPlugs = nil;
	}
	
	if (pMusicSubunitSourcePlugs != nil)
	{
		delete [] pMusicSubunitSourcePlugs;
		pMusicSubunitSourcePlugs = nil;
	}
	
	if (pAudioSubunitDestPlugs != nil)
	{
		delete [] pAudioSubunitDestPlugs;
		pAudioSubunitDestPlugs = nil;
	}
	
	if (pAudioSubunitSourcePlugs != nil)
	{
		delete [] pAudioSubunitSourcePlugs;
		pAudioSubunitSourcePlugs = nil;
	}
	
	if (pMusicSubunitStatusDescriptor)
	{
		delete [] pMusicSubunitStatusDescriptor;
		pMusicSubunitStatusDescriptor = nil;
	}
	
	while (!musicSubunitStatusDescriptorInfoBlocks.empty())
	{
		// Get a pointer to the element at the head of the queue
		pAVCInfoBlock = musicSubunitStatusDescriptorInfoBlocks.front();
		
		// Remove it from the queue
		musicSubunitStatusDescriptorInfoBlocks.pop_front();
		
		// Delete the object
		delete pAVCInfoBlock;
	}
	
	if (deviceConfigurationDictionary)
	{
		CFRelease(deviceConfigurationDictionary);
		deviceConfigurationDictionary = nil;
	}
	
	hasMusicSubunit = false;
	hasAudioSubunit = false;
	
	numIsoInPlugs = 0;
	numIsoOutPlugs = 0;
	
	numExtInPlugs = 0;
	numExtOutPlugs = 0;
	
	numMusicSubunitDestPlugs = 0;
	numMusicSubunitSourcePlugs = 0;
	numAudioSubunitDestPlugs = 0;
	numAudioSubunitSourcePlugs = 0;
	
	extendedStreamFormatCommandOpcode = 0xBF;	// Start with this. Will switch to 0x2F for legacy devices.
}

////////////////////////////////////////////
// MusicSubunitController::Initialize
////////////////////////////////////////////
IOReturn MusicSubunitController::Initialize(void)
{
	pIsoInPlugs = nil;
	pIsoOutPlugs = nil;
	pExtInPlugs = nil;
	pExtOutPlugs = nil;
	
	pMusicSubunitDestPlugs = nil;
	pMusicSubunitSourcePlugs = nil;
	
	pAudioSubunitDestPlugs = nil;
	pAudioSubunitSourcePlugs = nil;
	
	pMusicSubunitStatusDescriptor = nil;
	
	deviceConfigurationDictionary = nil;
	
	CleanUpDiscoveryResources();
	
	return kIOReturnSuccess;
}

/////////////////////////////////////////////////////////
// MusicSubunitController::DiscoverConfiguration
/////////////////////////////////////////////////////////
IOReturn MusicSubunitController::DiscoverConfiguration(void)
{
    UInt32 size;
    UInt8 cmd[512],response[512];
    IOReturn res;
    IOReturn result = kIOReturnSuccess;
	UInt32 i;
	CFNumberRef num; 
	CFMutableArrayRef array = NULL;
	CFMutableDictionaryRef level2Dict; 
	
	CleanUpDiscoveryResources();

	// Create the dictionary
	deviceConfigurationDictionary = CFDictionaryCreateMutable( kCFAllocatorDefault, 
											  0, 
											  &kCFTypeDictionaryKeyCallBacks, 
											  &kCFTypeDictionaryValueCallBacks ); 
	
	if (!deviceConfigurationDictionary)
		return kIOReturnNoMemory;
	
	UInt32 parserVersion = kMusicSubunitDeviceParserVersion;
	num = CFNumberCreate( kCFAllocatorDefault, 
						  kCFNumberIntType, 
						  &parserVersion ); 
	CFDictionarySetValue( deviceConfigurationDictionary, CFSTR("MusicSubunitDeviceParserVersion"), num ); 
	CFRelease( num ); 
	
	do
	{
		// Discover the unit plugs
		cmd[0] = kAVCStatusInquiryCommand;
		cmd[1] = 0xFF;
		cmd[2] = 0x02;
		cmd[3] = 0x00;
		cmd[4] = 0xFF;
		cmd[5] = 0xFF;
		cmd[6] = 0xFF;
		cmd[7] = 0xFF;
		size = 512;
		res = DoAVCCommand(cmd, 8, response, &size);
		if ((res == kIOReturnSuccess) && (response[0] == kAVCImplementedStatus))
		{
			numIsoInPlugs = response[4];
			numIsoOutPlugs = response[5];
			numExtInPlugs = response[6];
			numExtOutPlugs = response[7];
			
			DoLoggerLog("Number of Isochronous Input Plugs: %u\n",(unsigned int)numIsoInPlugs);
			DoLoggerLog("Number of Isochronous Output Plugs: %u\n",(unsigned int)numIsoOutPlugs);
			DoLoggerLog("Number of External Input Plugs: %u\n",(unsigned int)numExtInPlugs);
			DoLoggerLog("Number of External Output Plugs: %u\n",(unsigned int)numExtOutPlugs);

			num = CFNumberCreate( kCFAllocatorDefault, 
								  kCFNumberIntType, 
								  &numIsoInPlugs ); 
			CFDictionarySetValue( deviceConfigurationDictionary, CFSTR("NumIsochInputPlugs"), num ); 
			CFRelease( num ); 

			num = CFNumberCreate( kCFAllocatorDefault, 
								  kCFNumberIntType, 
								  &numIsoOutPlugs ); 
			CFDictionarySetValue( deviceConfigurationDictionary, CFSTR("NumIsochOutputPlugs"), num ); 
			CFRelease( num ); 

			num = CFNumberCreate( kCFAllocatorDefault, 
								  kCFNumberIntType, 
								  &numExtInPlugs ); 
			CFDictionarySetValue( deviceConfigurationDictionary, CFSTR("NumExternalInputPlugs"), num ); 
			CFRelease( num ); 

			num = CFNumberCreate( kCFAllocatorDefault, 
								  kCFNumberIntType, 
								  &numExtOutPlugs ); 
			CFDictionarySetValue( deviceConfigurationDictionary, CFSTR("NumExternalOutputPlugs"), num ); 
			CFRelease( num ); 
			
			// Create array of iso in plugs, if needed
			if (numIsoInPlugs > 0)
			{
				// Create a array for these plugs
				array = CFArrayCreateMutable( kCFAllocatorDefault,0,&kCFTypeArrayCallBacks);
				if (!array)
				{
					result = kIOReturnNoMemory;
					break;
				}
				
				pIsoInPlugs = new MusicDevicePlug[numIsoInPlugs];
				if (!pIsoInPlugs)
				{
					result = kIOReturnNoMemory;
					break;
				}
			}
			for (i=0;i<numIsoInPlugs;i++)
			{
				// Create the dictionary for this plug
				level2Dict = CFDictionaryCreateMutable( kCFAllocatorDefault, 
														0, 
														&kCFTypeDictionaryKeyCallBacks, 
														&kCFTypeDictionaryValueCallBacks ); 
				
				if (!level2Dict)
				{
					result = kIOReturnNoMemory;
					break;
				}
				
				pIsoInPlugs[i].pLogger = pLogger;
				
				pIsoInPlugs[i].pMusicSubunitController = this;
				pIsoInPlugs[i].subUnit = 0xFF;
				pIsoInPlugs[i].plugNum = i;
				pIsoInPlugs[i].isDest = false;
				
				pIsoInPlugs[i].GetCurrentStreamFormatInformation(level2Dict);
				pIsoInPlugs[i].GetSupportedStreamFormatInformation(level2Dict);
				
				// Append the plug dictionary to the array of plugs
				CFArrayAppendValue(array,level2Dict);
				CFRelease( level2Dict ); 
			}
			if (numIsoInPlugs > 0)
			{
				// Append the array of plugs to the top-level dictionary
				CFDictionarySetValue( deviceConfigurationDictionary, CFSTR("IsochInputPlugs"), array ); 
				CFRelease( array ); 
			}
		
			// Create array of iso out plugs, if needed
			if (numIsoOutPlugs > 0)
			{
				// Create a array for these plugs
				array = CFArrayCreateMutable( kCFAllocatorDefault,0,&kCFTypeArrayCallBacks);
				if (!array)
				{
					result = kIOReturnNoMemory;
					break;
				}
				
				pIsoOutPlugs = new MusicDevicePlug[numIsoOutPlugs];
				if (!pIsoOutPlugs)
				{
					result = kIOReturnNoMemory;
					break;
				}
			}
			for (i=0;i<numIsoOutPlugs;i++)
			{
				// Create the dictionary for this plug
				level2Dict = CFDictionaryCreateMutable( kCFAllocatorDefault, 
														0, 
														&kCFTypeDictionaryKeyCallBacks, 
														&kCFTypeDictionaryValueCallBacks ); 
				
				if (!level2Dict)
				{
					result = kIOReturnNoMemory;
					break;
				}
				
				pIsoOutPlugs[i].pLogger = pLogger;
				
				pIsoOutPlugs[i].pMusicSubunitController = this;
				pIsoOutPlugs[i].subUnit = 0xFF;
				pIsoOutPlugs[i].plugNum = i;
				pIsoOutPlugs[i].isDest = true;
				
				pIsoOutPlugs[i].GetSignalSource(level2Dict);
				pIsoOutPlugs[i].GetCurrentStreamFormatInformation(level2Dict);
				pIsoOutPlugs[i].GetSupportedStreamFormatInformation(level2Dict);
				
				// Append the plug dictionary to the array of plugs
				CFArrayAppendValue(array,level2Dict);
				CFRelease( level2Dict ); 
			}
			if (numIsoOutPlugs > 0)
			{
				// Append the array of plugs to the top-level dictionary
				CFDictionarySetValue( deviceConfigurationDictionary, CFSTR("IsochOutputPlugs"), array ); 
				CFRelease( array ); 
			}
			
			// Create array of external in plugs, if needed
			if (numExtInPlugs > 0)
			{
				// Create a array for these plugs
				array = CFArrayCreateMutable( kCFAllocatorDefault,0,&kCFTypeArrayCallBacks);
				if (!array)
				{
					result = kIOReturnNoMemory;
					break;
				}
				
				pExtInPlugs = new MusicDevicePlug[numExtInPlugs];
				if (!pExtInPlugs)
				{
					result = kIOReturnNoMemory;
					break;
				}
			}
			for (i=0;i<numExtInPlugs;i++)
			{
				// Create the dictionary for this plug
				level2Dict = CFDictionaryCreateMutable( kCFAllocatorDefault, 
														0, 
														&kCFTypeDictionaryKeyCallBacks, 
														&kCFTypeDictionaryValueCallBacks ); 
				
				if (!level2Dict)
				{
					result = kIOReturnNoMemory;
					break;
				}
				
				// Set values into this plug dictionary
				CFDictionarySetValue( level2Dict, CFSTR("TODO"), CFSTR("TODO") ); 
				
				pExtInPlugs[i].pLogger = pLogger;
				
				pExtInPlugs[i].pMusicSubunitController = this;
				pExtInPlugs[i].subUnit = 0xFF;
				pExtInPlugs[i].plugNum = 0x80 + i;
				pExtInPlugs[i].isDest = false;
				
				//pExtInPlugs[i].GetCurrentStreamFormatInformation();
				
				// Append the plug dictionary to the array of plugs
				CFArrayAppendValue(array,level2Dict);
				CFRelease( level2Dict ); 
			}
			if (numExtInPlugs > 0)
			{
				// Append the array of plugs to the top-level dictionary
				CFDictionarySetValue( deviceConfigurationDictionary, CFSTR("ExternalInputPlugs"), array ); 
				CFRelease( array ); 
			}
			
			// Create array of external out plugs, if needed
			if (numExtOutPlugs > 0)
			{
				// Create a array for these plugs
				array = CFArrayCreateMutable( kCFAllocatorDefault,0,&kCFTypeArrayCallBacks);
				if (!array)
				{
					result = kIOReturnNoMemory;
					break;
				}
				
				pExtOutPlugs = new MusicDevicePlug[numExtOutPlugs];
				if (!pExtOutPlugs)
				{
					result = kIOReturnNoMemory;
					break;
				}
			}
			for (i=0;i<numExtOutPlugs;i++)
			{
				// Create the dictionary for this plug
				level2Dict = CFDictionaryCreateMutable( kCFAllocatorDefault, 
														0, 
														&kCFTypeDictionaryKeyCallBacks, 
														&kCFTypeDictionaryValueCallBacks ); 
				
				if (!level2Dict)
				{
					result = kIOReturnNoMemory;
					break;
				}
				
				pExtOutPlugs[i].pLogger = pLogger;
				
				pExtOutPlugs[i].pMusicSubunitController = this;
				pExtOutPlugs[i].subUnit = 0xFF;
				pExtOutPlugs[i].plugNum = 0x80 + i;
				pExtOutPlugs[i].isDest = true;
				
				pExtOutPlugs[i].GetSignalSource(level2Dict);
				//pExtOutPlugs[i].GetCurrentStreamFormatInformation();
				
				// Append the plug dictionary to the array of plugs
				CFArrayAppendValue(array,level2Dict);
				CFRelease( level2Dict ); 
			}
			if (numExtOutPlugs > 0)
			{
				// Append the array of plugs to the top-level dictionary
				CFDictionarySetValue( deviceConfigurationDictionary, CFSTR("ExternalOutputPlugs"), array ); 
				CFRelease( array ); 
			}
		}
		
		// Discover the music subunit plugs
		if (pAVCDevice->hasMusicSubunit)
		{
			cmd[0] = kAVCStatusInquiryCommand;
			cmd[1] = 0x60;
			cmd[2] = 0x02;
			cmd[3] = 0x00;
			cmd[4] = 0xFF;
			cmd[5] = 0xFF;
			cmd[6] = 0xFF;
			cmd[7] = 0xFF;
			size = 512;
			res = DoAVCCommand(cmd, 8, response, &size);
			if ((res == kIOReturnSuccess) && (response[0] == kAVCImplementedStatus))
			{
				numMusicSubunitDestPlugs = response[4];
				numMusicSubunitSourcePlugs = response[5];
				
				DoLoggerLog("Number of Music Subunit Destination Plugs: %u\n",(unsigned int)numMusicSubunitDestPlugs);
				DoLoggerLog("Number of Music Subunit Source Plugs: %u\n",(unsigned int)numMusicSubunitSourcePlugs);
				
				num = CFNumberCreate( kCFAllocatorDefault, 
									  kCFNumberIntType, 
									  &numMusicSubunitSourcePlugs ); 
				CFDictionarySetValue( deviceConfigurationDictionary, CFSTR("NumMusicSubunitSourcePlugs"), num ); 
				CFRelease( num ); 
				
				num = CFNumberCreate( kCFAllocatorDefault, 
									  kCFNumberIntType, 
									  &numMusicSubunitDestPlugs ); 
				CFDictionarySetValue( deviceConfigurationDictionary, CFSTR("NumMusicSubunitDestPlugs"), num ); 
				CFRelease( num ); 
				
				// Create array of music subunit dest plugs, if needed
				if (numMusicSubunitDestPlugs > 0)
				{
					// Create a array for these plugs
					array = CFArrayCreateMutable( kCFAllocatorDefault,0,&kCFTypeArrayCallBacks);
					if (!array)
					{
						result = kIOReturnNoMemory;
						break;
					}
					
					pMusicSubunitDestPlugs = new MusicDevicePlug[numMusicSubunitDestPlugs];
					if (!pMusicSubunitDestPlugs)
					{
						result = kIOReturnNoMemory;
						break;
					}
				}
				for (i=0;i<numMusicSubunitDestPlugs;i++)
				{
					// Create the dictionary for this plug
					level2Dict = CFDictionaryCreateMutable( kCFAllocatorDefault, 
															0, 
															&kCFTypeDictionaryKeyCallBacks, 
															&kCFTypeDictionaryValueCallBacks ); 
					
					if (!level2Dict)
					{
						result = kIOReturnNoMemory;
						break;
					}
					
					pMusicSubunitDestPlugs[i].pLogger = pLogger;

					pMusicSubunitDestPlugs[i].pMusicSubunitController = this;
					pMusicSubunitDestPlugs[i].subUnit = 0x60;
					pMusicSubunitDestPlugs[i].plugNum = i;
					pMusicSubunitDestPlugs[i].isDest = true;
					
					pMusicSubunitDestPlugs[i].GetSignalSource(level2Dict);
					//pMusicSubunitDestPlugs[i].GetCurrentStreamFormatInformation();
					
					// Append the plug dictionary to the array of plugs
					CFArrayAppendValue(array,level2Dict);
					CFRelease( level2Dict ); 
				}
				if (numMusicSubunitDestPlugs > 0)
				{
					// Append the array of plugs to the top-level dictionary
					CFDictionarySetValue( deviceConfigurationDictionary, CFSTR("MusicSubunitDestPlugs"), array ); 
					CFRelease( array ); 
				}
				
				// Create array of music subunit source plugs, if needed
				if (numMusicSubunitSourcePlugs > 0)
				{
					// Create a array for these plugs
					array = CFArrayCreateMutable( kCFAllocatorDefault,0,&kCFTypeArrayCallBacks);
					if (!array)
					{
						result = kIOReturnNoMemory;
						break;
					}
					
					pMusicSubunitSourcePlugs = new MusicDevicePlug[numMusicSubunitSourcePlugs];
					if (!pMusicSubunitSourcePlugs)
					{
						result = kIOReturnNoMemory;
						break;
					}
				}
				for (i=0;i<numMusicSubunitSourcePlugs;i++)
				{
					// Create the dictionary for this plug
					level2Dict = CFDictionaryCreateMutable( kCFAllocatorDefault, 
															0, 
															&kCFTypeDictionaryKeyCallBacks, 
															&kCFTypeDictionaryValueCallBacks ); 
					
					if (!level2Dict)
					{
						result = kIOReturnNoMemory;
						break;
					}
					
					// Set values into this plug dictionary
					CFDictionarySetValue( level2Dict, CFSTR("TODO"), CFSTR("TODO") ); 
					
					pMusicSubunitSourcePlugs[i].pLogger = pLogger;

					pMusicSubunitSourcePlugs[i].pMusicSubunitController = this;
					pMusicSubunitSourcePlugs[i].subUnit = 0x60;
					pMusicSubunitSourcePlugs[i].plugNum = i;
					pMusicSubunitSourcePlugs[i].isDest = false;
					
					//pMusicSubunitSourcePlugs[i].GetCurrentStreamFormatInformation();
					
					// Append the plug dictionary to the array of plugs
					CFArrayAppendValue(array,level2Dict);
					CFRelease( level2Dict ); 
				}
				if (numMusicSubunitSourcePlugs > 0)
				{
					// Append the array of plugs to the top-level dictionary
					CFDictionarySetValue( deviceConfigurationDictionary, CFSTR("MusicSubunitSourcePlugs"), array ); 
					CFRelease( array ); 
				}
			}
			
			// Read/Parse the Music Subunit Status Descriptor
			res = GetMusicSubunitStatusDescriptor();
			if ((res == kIOReturnSuccess) && (pMusicSubunitStatusDescriptor))
				ParseMusicSubunitStatusDescriptor(deviceConfigurationDictionary);
		}
		
		// Discover the audio subunit plugs
		if (pAVCDevice->hasAudioSubunit)
		{
			cmd[0] = kAVCStatusInquiryCommand;
			cmd[1] = 0x08;
			cmd[2] = 0x02;
			cmd[3] = 0x00;
			cmd[4] = 0xFF;
			cmd[5] = 0xFF;
			cmd[6] = 0xFF;
			cmd[7] = 0xFF;
			size = 512;
			res = DoAVCCommand(cmd, 8, response, &size);
			if ((res == kIOReturnSuccess) && (response[0] == kAVCImplementedStatus))
			{
				numAudioSubunitDestPlugs = response[4];
				numAudioSubunitSourcePlugs = response[5];
				
				DoLoggerLog("Number of Audio Subunit Destination Plugs: %u\n",(unsigned int)numAudioSubunitDestPlugs);
				DoLoggerLog("Number of Audio Subunit Source Plugs: %u\n",(unsigned int)numAudioSubunitSourcePlugs);
	
				num = CFNumberCreate( kCFAllocatorDefault, 
									  kCFNumberIntType, 
									  &numAudioSubunitSourcePlugs ); 
				CFDictionarySetValue( deviceConfigurationDictionary, CFSTR("NumAudioSubunitSourcePlugs"), num ); 
				CFRelease( num ); 
				
				num = CFNumberCreate( kCFAllocatorDefault, 
									  kCFNumberIntType, 
									  &numAudioSubunitDestPlugs ); 
				CFDictionarySetValue( deviceConfigurationDictionary, CFSTR("NumAudioSubunitDestPlugs"), num ); 
				CFRelease( num ); 
				
				// Create array of audio subunit dest plugs, if needed
				if (numAudioSubunitDestPlugs > 0)
				{
					// Create a array for these plugs
					array = CFArrayCreateMutable( kCFAllocatorDefault,0,&kCFTypeArrayCallBacks);
					if (!array)
					{
						result = kIOReturnNoMemory;
						break;
					}
					
					pAudioSubunitDestPlugs = new MusicDevicePlug[numAudioSubunitDestPlugs];
					if (!pAudioSubunitDestPlugs)
					{
						result = kIOReturnNoMemory;
						break;
					}
				}
				for (i=0;i<numAudioSubunitDestPlugs;i++)
				{
					// Create the dictionary for this plug
					level2Dict = CFDictionaryCreateMutable( kCFAllocatorDefault, 
															0, 
															&kCFTypeDictionaryKeyCallBacks, 
															&kCFTypeDictionaryValueCallBacks ); 
					
					if (!level2Dict)
					{
						result = kIOReturnNoMemory;
						break;
					}
					
					pAudioSubunitDestPlugs[i].pLogger = pLogger;

					pAudioSubunitDestPlugs[i].pMusicSubunitController = this;
					pAudioSubunitDestPlugs[i].subUnit = 0x08;
					pAudioSubunitDestPlugs[i].plugNum = i;
					pAudioSubunitDestPlugs[i].isDest = true;
					
					pAudioSubunitDestPlugs[i].GetSignalSource(level2Dict);
					//pAudioSubunitDestPlugs[i].GetCurrentStreamFormatInformation();

					// Append the plug dictionary to the array of plugs
					CFArrayAppendValue(array,level2Dict);
					CFRelease( level2Dict ); 
				}
				if (numAudioSubunitDestPlugs > 0)
				{
					// Append the array of plugs to the top-level dictionary
					CFDictionarySetValue( deviceConfigurationDictionary, CFSTR("AudioSubunitDestPlugs"), array ); 
					CFRelease( array ); 
				}
				
				// Create array of audio subunit source plugs, if needed
				if (numAudioSubunitSourcePlugs > 0)
				{
					// Create a array for these plugs
					array = CFArrayCreateMutable( kCFAllocatorDefault,0,&kCFTypeArrayCallBacks);
					if (!array)
					{
						result = kIOReturnNoMemory;
						break;
					}
					
					pAudioSubunitSourcePlugs = new MusicDevicePlug[numAudioSubunitSourcePlugs];
					if (!pAudioSubunitSourcePlugs)
					{
						result = kIOReturnNoMemory;
						break;
					}
				}
				for (i=0;i<numAudioSubunitSourcePlugs;i++)
				{
					// Create the dictionary for this plug
					level2Dict = CFDictionaryCreateMutable( kCFAllocatorDefault, 
															0, 
															&kCFTypeDictionaryKeyCallBacks, 
															&kCFTypeDictionaryValueCallBacks ); 
					
					if (!level2Dict)
					{
						result = kIOReturnNoMemory;
						break;
					}
					
					// Set values into this plug dictionary
					CFDictionarySetValue( level2Dict, CFSTR("TODO"), CFSTR("TODO") ); 
					
					pAudioSubunitSourcePlugs[i].pLogger = pLogger;

					pAudioSubunitSourcePlugs[i].pMusicSubunitController = this;
					pAudioSubunitSourcePlugs[i].subUnit = 0x08;
					pAudioSubunitSourcePlugs[i].plugNum = i;
					pAudioSubunitSourcePlugs[i].isDest = false;

					//pAudioSubunitSourcePlugs[i].GetCurrentStreamFormatInformation();

					// Append the plug dictionary to the array of plugs
					CFArrayAppendValue(array,level2Dict);
					CFRelease( level2Dict ); 
				}
				if (numAudioSubunitSourcePlugs > 0)
				{
					// Append the array of plugs to the top-level dictionary
					CFDictionarySetValue( deviceConfigurationDictionary, CFSTR("AudioSubunitSourcePlugs"), array ); 
					CFRelease( array ); 
				}
			}
		}
		
	}while(0);
	
#if 1	
	{
		// AY_DEBUG: Create a plist file from the deviceConfigurationDictionary
		CFURLRef fileURL; 
		CFDataRef xmlData; 
		Boolean status; 
		SInt32 errorCode; 
		fileURL = CFURLCreateWithFileSystemPath( kCFAllocatorDefault, 
												 CFSTR("AVCMusicDevice.plist"), // file path name 
												 kCFURLPOSIXPathStyle, // interpret as POSIX path 
												 false ); // is it a directory? 
		
		// Convert the property list into XML data. 
		xmlData = CFPropertyListCreateXMLData( kCFAllocatorDefault, deviceConfigurationDictionary ); 
		// Write the XML data to the file. 
		status = CFURLWriteDataAndPropertiesToResource ( 
														 fileURL, // URL to use 
														 xmlData, // data to write 
														 NULL, 
														 &errorCode); 
		CFRelease(xmlData); 
	}
#endif	
	
	return result;
}

/////////////////////////////////////////////////////////
// MusicSubunitController::CopyDeviceDiscoveryDictionary
/////////////////////////////////////////////////////////
CFDictionaryRef MusicSubunitController::CopyDeviceDiscoveryDictionary(void)
{
	if (deviceConfigurationDictionary)
		return CFDictionaryCreateCopy(kCFAllocatorDefault,deviceConfigurationDictionary);
	else
		return NULL;
}

/////////////////////////////////////////////////////////
// MusicSubunitController::SetPlugStreamFormatWithListIndex
/////////////////////////////////////////////////////////
IOReturn MusicSubunitController::SetPlugStreamFormatWithListIndex(bool isInputPlug, UInt32 plugNum, UInt32 listIndex)
{
	MusicDevicePlug *pPlug = NULL;
	ExtendedStreamFormatInfo *pFormat;
	
	if (isInputPlug)
	{
		if (plugNum < numIsoInPlugs)
			pPlug = &pIsoInPlugs[plugNum];
	}
	else
	{
		if (plugNum < numIsoOutPlugs)
			pPlug = &pIsoInPlugs[plugNum];
	}

	if (pPlug)
	{
		pFormat = pPlug->supportedFormats.at(listIndex);
		if (pFormat)
			return pPlug->SetCurrentStreamFormat(pFormat);
		else
			return kIOReturnBadArgument;
	}
	else
		return kIOReturnBadArgument;
}

/////////////////////////////////////////
// MusicSubunitController::DoAVCCommand
/////////////////////////////////////////
IOReturn MusicSubunitController::DoAVCCommand(const UInt8 *command, UInt32 cmdLen, UInt8 *response, UInt32 *responseLen)
{
	if (pAVCDeviceCommandInterface)
		return pAVCDeviceCommandInterface->AVCCommand(command,cmdLen,response,responseLen);
	if (pAVCDevice)
		return pAVCDevice->AVCCommand(command,cmdLen,response,responseLen);
	else
		return kIOReturnNoDevice;
}

///////////////////////////////////////////////////////////////
// MusicSubunitController::GetMusicSubunitStatusDescriptor
///////////////////////////////////////////////////////////////
IOReturn MusicSubunitController::GetMusicSubunitStatusDescriptor(void)
{
    UInt32 size;
    UInt8 cmd[10],response[512];
	IOReturn res = kIOReturnSuccess;
	UInt32 descriptorLen;
	UInt32 numDescriptorBytesRead;
	UInt32 descriptorBytesSoFar = 0;
	bool descriptorOpened = false;

	// If we already have a copy, delete it
	if (pMusicSubunitStatusDescriptor)
	{
		delete [] pMusicSubunitStatusDescriptor;
		pMusicSubunitStatusDescriptor = nil;
	}

	do
	{
		// Attempt to open the descriptor
		cmd[0] = kAVCControlCommand;
		cmd[1] = 0x60;
		cmd[2] = 0x08;
		cmd[3] = 0x80;
		cmd[4] = 0x01;
		cmd[5] = 0x00;
		
		size = 512;
		res = DoAVCCommand(cmd, 6, response, &size);
		if ((res == kIOReturnSuccess) && ((response[0] == kAVCAcceptedStatus) || (response[0] == kAVCImplementedStatus)))
		{
			descriptorOpened = true;
		}
		else
		{
			res = kIOReturnError;
			break;
		}
		
		// Read the first part of the descriptor
		cmd[0] = kAVCControlCommand;
		cmd[1] = 0x60;
		cmd[2] = 0x09;
		cmd[3] = 0x80;
		cmd[4] = 0xFF;
		cmd[5] = 0x00;
		cmd[6] = 0x00;
		cmd[7] = 0x80;
		cmd[8] = 0x00;
		cmd[9] = 0x00;
		size = 512;
		res = DoAVCCommand(cmd, 10, response, &size);
		if ((res == kIOReturnSuccess) && ((response[0] == kAVCAcceptedStatus) || (response[0] == kAVCImplementedStatus)))
		{
			if (size > 12)
			{
				// Determine the total length off the descriptor
				descriptorLen = (response[10]*256)+response[11]+2;

				// Allocate memory for the descriptor
				pMusicSubunitStatusDescriptor = new UInt8[descriptorLen];
				if (!pMusicSubunitStatusDescriptor)
				{
					res = kIOReturnNoMemory;
					break;
				}
				else
				{
					// Determine number of descriptor bytes returned in this command
					numDescriptorBytesRead = (response[6]*256)+response[7];

					// Copy the first part of the descriptor
					bcopy(&response[10],pMusicSubunitStatusDescriptor,numDescriptorBytesRead);
					descriptorBytesSoFar += numDescriptorBytesRead;

					// Check to see if there's more to come....
					// Some devices don't report the read_result_status correctly, 
					// so use a length check instead
					//while (response[4] == 0x11)
					while (descriptorBytesSoFar < descriptorLen)
					{
						cmd[8] = (descriptorBytesSoFar >> 8);
						cmd[9] = (descriptorBytesSoFar & 0xFF);
						size = 512;
						res = DoAVCCommand(cmd, 10, response, &size);
						if ((res == kIOReturnSuccess) && ((response[0] == kAVCAcceptedStatus) || (response[0] == kAVCImplementedStatus)))
						{
							// Determine number of descriptor bytes returned in this command
							numDescriptorBytesRead = (response[6]*256)+response[7];
							
							if ((descriptorBytesSoFar + numDescriptorBytesRead) <= descriptorLen)
							{
								// Copy the part of the descriptor
								bcopy(&response[10],&pMusicSubunitStatusDescriptor[descriptorBytesSoFar],numDescriptorBytesRead);
								descriptorBytesSoFar += numDescriptorBytesRead;
							}
							else
							{
								// Too many bytes. This error condition will be handled below.
								break;
							}
						}
						else
						{
							res = kIOReturnError;
							break;
						}
					};
					
					if (descriptorBytesSoFar != descriptorLen)
					{
						DoLoggerLog("ERROR: MusicDevicePlug::GetMusicSubunitStatusDescriptor - Received incorrect number of descriptor bytes (expected: %u, actual: %u)\n",
								   (unsigned int)descriptorLen,
								   (unsigned int)descriptorBytesSoFar);
						res = kIOReturnError;
						break;
					}
				}
			}
			else
			{
				res = kIOReturnError;
				break;
			}
		}
		else
		{
			res = kIOReturnError;
			break;
		}
		
	}while(0);
	
	if (descriptorOpened)
	{
		// Close the descriptor
		cmd[0] = kAVCControlCommand;
		cmd[1] = 0x60;
		cmd[2] = 0x08;
		cmd[3] = 0x80;
		cmd[4] = 0x00;
		cmd[5] = 0x00;
		size = 512;
		DoAVCCommand(cmd, 6, response, &size);
	}
	
	// If we have an error, and we had allocated memory for the descriptor, delete it now
	if (res != kIOReturnSuccess)
	{
		delete [] pMusicSubunitStatusDescriptor;
		pMusicSubunitStatusDescriptor = nil;
	}
	
	return res;
}

static char* MusicSubunitInfoBlockTypeDescriptions(UInt32 infoBlockType)
{
	switch (infoBlockType)
	{
		case 0x8100:
			return "General Music Subunit Status Area Info Block";
			break;

		case 0x8101:
			return "Music Output Plug Status Area Info Block";
			break;

		case 0x8102:
			return "Source Plug Status Info Block";
			break;

		case 0x8103:
			return "Audio Info Block";
			break;

		case 0x8104:
			return "MIDI Info Block";
			break;

		case 0x8105:
			return "SMPTE Time Code Info Block";
			break;
			
		case 0x8106:
			return "Sample Count Info Block";
			break;
			
		case 0x8107:
			return "Audio SYNC Info Block";
			break;
			
		case 0x8108:
			return "Routing Status Info Block";
			break;
			
		case 0x8109:
			return "Subunit Plug Info Block";
			break;
			
		case 0x810A:
			return "Cluster Info Block";
			break;
			
		case 0x810B:
			return "Music Plug Info Block";
			break;
			
		case 0x000B:
			return "Name Info Block";
			break;

		case 0x000A:
			return "Raw Text Info Block";
			break;
			
		default:
			return "Unknown";
			break;
	};

	return "INTERNAL ERROR"; // To fix a warning. This cannot happen!
}

///////////////////////////////////////////////////////////////
// MusicSubunitController::ParseMusicSubunitStatusDescriptor
///////////////////////////////////////////////////////////////
IOReturn MusicSubunitController::ParseMusicSubunitStatusDescriptor(CFMutableDictionaryRef dict)
{
	IOReturn res = kIOReturnSuccess;
	UInt8 *pInfoBlockBytes;
	UInt32 sizeOfCurrentInfoBlock;
	UInt32 currentInfoBlockType;
	AVCInfoBlock *pAVCInfoBlock;
	UInt32 i;
	CFMutableArrayRef infoBlockArray;

	
	if (!pMusicSubunitStatusDescriptor)
		return kIOReturnNoResources;
	
	UInt32 totalInfoBlocksLength = ((pMusicSubunitStatusDescriptor[0]*256)+pMusicSubunitStatusDescriptor[1]);
	
	DoLoggerLog("Music subunit status descriptor length: %u\n",(unsigned int)totalInfoBlocksLength+2);

	// The music subunit status descriptor is just a series of info blocks (each with optional nested info blocks).
	// Traverse the top-level info-blocks
	
	pInfoBlockBytes = &pMusicSubunitStatusDescriptor[2];
	do
	{
		sizeOfCurrentInfoBlock = ((pInfoBlockBytes[0]*256)+pInfoBlockBytes[1]+2);
		currentInfoBlockType =  ((pInfoBlockBytes[2]*256)+pInfoBlockBytes[3]);
		DoLoggerLog("Info-Block: 0x%04X (%s) Length: %u\n",
				   (unsigned int)currentInfoBlockType,
				   MusicSubunitInfoBlockTypeDescriptions(currentInfoBlockType),
				   (unsigned int)sizeOfCurrentInfoBlock);
		
		pAVCInfoBlock = new AVCInfoBlock(pInfoBlockBytes,pLogger);
		if (pAVCInfoBlock)
			musicSubunitStatusDescriptorInfoBlocks.push_back(pAVCInfoBlock);
		
		pInfoBlockBytes += sizeOfCurrentInfoBlock;
	}while(pInfoBlockBytes < (pMusicSubunitStatusDescriptor+totalInfoBlocksLength+2));

	infoBlockArray = CFArrayCreateMutable( kCFAllocatorDefault,0,&kCFTypeArrayCallBacks);
	if (!infoBlockArray)
		return kIOReturnNoMemory;
	
	for (i=0;i<musicSubunitStatusDescriptorInfoBlocks.size();i++)
	{
		pAVCInfoBlock = musicSubunitStatusDescriptorInfoBlocks.at(i);
		pAVCInfoBlock->ParseInfoBlock(0,infoBlockArray);
		
#if 0		
		// AY_DEBUG: Test some additional AVCInfoBlock APIs
		if (pAVCInfoBlock->InfoBlockType() == 0x8108)
		{
			AVCInfoBlock *pSubunitPlugInfoBlock;
			UInt32 subunitPlugInfoBlockCount = 0;
			do 
			{
				pSubunitPlugInfoBlock = pAVCInfoBlock->GetNestedInfoBlock(0x8109,subunitPlugInfoBlockCount);
				if (pSubunitPlugInfoBlock)
				{
					subunitPlugInfoBlockCount += 1;
					UInt8 testBuffer[512];
					bool bResult;
					UInt16 bufLen = 512;
					bResult = pSubunitPlugInfoBlock->GetInfoBlockBytes(testBuffer, &bufLen);
					if (bResult)
					{
						DoLoggerLog("AY_DEBUG: Got %u bytets for a Subunit Plug Info Block!\n",(unsigned int)bufLen);
						if (((testBuffer[0]*256)+testBuffer[1]+2) != bufLen)
								DoLoggerLog("AY_DEBUG: Sanity Check of retrieved GetNestedInfoBlock() bytes failed\n");
					}
					else
					{
						DoLoggerLog("AY_DEBUG: Faild to get the bytets for a Subunit Plug Info Block!\n");
					}
					bufLen = 512;
					bResult = pSubunitPlugInfoBlock->GetPrimaryFields(testBuffer, &bufLen);
					if (bResult)
					{
						DoLoggerLog("AY_DEBUG: Got %u bytets for a Subunit Plug Info Block's Primary Field!\n",(unsigned int)bufLen);
						if (pSubunitPlugInfoBlock->PrimaryFieldsLength() != bufLen)
						{
							DoLoggerLog("AY_DEBUG: Sanity Check of retrieved GetPrimaryFields() bytes failed\n");
						}
						else
						{
							DoLoggerLog("AY_DEBUG: subunit_plug_id: %u\n",(unsigned int)testBuffer[0]);
						}
					}
					else
					{
						DoLoggerLog("AY_DEBUG: Faild to get the bytets for a Subunit Plug Info Block's Primary Field!\n");
					}
				}
			}while (pSubunitPlugInfoBlock);
			DoLoggerLog("AY_DEBUG: Found a total of %u Subunit Plug Info Blocks in the Routing Status Info Block\n",(unsigned int) subunitPlugInfoBlockCount);
		}
#endif		
		
	}
	
	if (dict)
		CFDictionarySetValue( dict, CFSTR("MusicSubunitStatusDescriptorInfoBlocks"), infoBlockArray ); 
	CFRelease(infoBlockArray);

	return res;
}


#pragma mark -----------------------------------
#pragma mark ExtendedStreamFormatInfo Class Methods
#pragma mark -----------------------------------

/////////////////////////////////////////
// ExtendedStreamFormatInfo Constructor
/////////////////////////////////////////
ExtendedStreamFormatInfo::ExtendedStreamFormatInfo(StringLogger *pStringLogger)
{
	pLogger = pStringLogger;
	len = 0;
	pFormatInfoBytes = nil;
}

/////////////////////////////////////////
// ExtendedStreamFormatInfo Destructor
/////////////////////////////////////////
ExtendedStreamFormatInfo::~ExtendedStreamFormatInfo()
{
	if (pFormatInfoBytes)
		delete [] pFormatInfoBytes;
}

/////////////////////////////////////////
// ExtendedStreamFormatInfo::InitWithFormatInfo
/////////////////////////////////////////
IOReturn ExtendedStreamFormatInfo::InitWithFormatInfo(UInt8 *pFormatInfo, UInt32 formatInfoLen)
{
	// Can only init once!
	if (pFormatInfoBytes)
		return kIOReturnNotPermitted;
	
	len = 0;

	pFormatInfoBytes = new UInt8[formatInfoLen];
	if (pFormatInfoBytes)
	{
		bcopy(pFormatInfo,pFormatInfoBytes,formatInfoLen);
		len = formatInfoLen;
		return kIOReturnSuccess;
	}
	else
		return kIOReturnNoMemory;
}	

/////////////////////////////////////////
// ExtendedStreamFormatInfo::PrintFormatInformation
/////////////////////////////////////////
void ExtendedStreamFormatInfo:: PrintFormatInformation(CFMutableDictionaryRef dict)
{
	UInt32 numFormatInfoFields;
	UInt32 i;
	CFNumberRef num; 
	CFMutableArrayRef array;
	CFDataRef data;
	
	if (dict)
	{
		data = CFDataCreate( kCFAllocatorDefault, pFormatInfoBytes, len ); 
		if (!data)
			return;
		CFDictionarySetValue( dict, CFSTR("RawStreamFormatInfoBytes"), data ); 
		CFRelease( data ); 
	}
	
	DoLoggerLog("===== Stream Format Information =====\n");

	if ((pFormatInfoBytes[0] == 0x90) && (pFormatInfoBytes[1] == 0x40))
	{
		// Format is "compound AM824 stream"
		DoLoggerLog("    Compound AM824 stream\n")
		
		numFormatInfoFields = pFormatInfoBytes[4];
		
		// Sample Rate
		switch (pFormatInfoBytes[2])
		{
			case 0x00:
				DoLoggerLog("    22.05KHz\n")
				break;
				
			case 0x01:
				DoLoggerLog("    24KHz\n")
				break;
				
			case 0x02:
				DoLoggerLog("    32KHz\n")
				break;
				
			case 0x03:
				DoLoggerLog("    44.1KHz\n")
				break;
				
			case 0x04:
				DoLoggerLog("    48KHz\n")
				break;
				
			case 0x05:
				DoLoggerLog("    96KHz\n")
				break;
				
			case 0x06:
				DoLoggerLog("    176.4KHz\n")
				break;
				
			case 0x07:
				DoLoggerLog("    192KHz\n")
				break;
				
			case 0x0A:
				DoLoggerLog("    88.2KHz\n")
				break;
				
			default:
				DoLoggerLog("    Unknown Sample Rate\n")
				break;
				
		};
		
		// Possible Sync Source
		if (pFormatInfoBytes[3] & 0x04)
		{
			DoLoggerLog("    Plug claims to be a possible sync source\n");
		}
		else
		{
			DoLoggerLog("    Plug does not claims to be a possible sync source\n");
		}
		
		if (dict)
		{
			CFDictionarySetValue( dict, CFSTR("StreamFormat"), CFSTR("CompoundAM824") ); 
			
			UInt32 intVal = pFormatInfoBytes[2];
			num = CFNumberCreate( kCFAllocatorDefault, 
								  kCFNumberIntType, 
								  &intVal ); 
			CFDictionarySetValue( dict, CFSTR("SampleRate"), num ); 
			CFRelease( num ); 
			
			if (pFormatInfoBytes[3] & 0x04)
				CFDictionarySetValue( dict, CFSTR("SyncSourceFlag"), CFSTR("YES") ); 
			else
				CFDictionarySetValue( dict, CFSTR("SyncSourceFlag"), CFSTR("NO") ); 

			num = CFNumberCreate( kCFAllocatorDefault, 
								  kCFNumberIntType, 
								  &numFormatInfoFields ); 
			CFDictionarySetValue( dict, CFSTR("NumFormatInfoFields"), num ); 
			CFRelease( num ); 
		}
		
		// Signal Formats
		if (numFormatInfoFields > 0)
		{
			array = CFArrayCreateMutable( kCFAllocatorDefault,0,&kCFTypeArrayCallBacks);
			
			DoLoggerLog("    numFormatInfoFields: %u\n",(unsigned int)numFormatInfoFields);
			for (i=0;i<numFormatInfoFields;i++)
			{
				DoLoggerLog("      %u channels of ",pFormatInfoBytes[5+(i*2)]);

				if (array)
				{
					UInt32 fmtAndCount = (pFormatInfoBytes[5+(i*2)] << 8) + (pFormatInfoBytes[6+(i*2)]);
					num = CFNumberCreate( kCFAllocatorDefault, 
										  kCFNumberIntType, 
										  &fmtAndCount ); 
					CFArrayAppendValue(array,num);
					CFRelease( num ); 
				}
				
				switch(pFormatInfoBytes[6+(i*2)])
				{
					case 0x00:
						DoLoggerLog("IEC60958-3\n");
						break;
						
					case 0x01:
						DoLoggerLog("IEC61937-3\n");
						break;
						
					case 0x02:
						DoLoggerLog("IEC61937-4\n");
						break;
						
					case 0x03:
						DoLoggerLog("IEC61937-5\n");
						break;
						
					case 0x04:
						DoLoggerLog("IEC61937-6\n");
						break;
						
					case 0x05:
						DoLoggerLog("IEC61937-7\n");
						break;
						
					case 0x06:
						DoLoggerLog("MBLA\n");
						break;
						
					case 0x07:
						DoLoggerLog("MBLA(DVD)\n");
						break;
						
					case 0x08:
						DoLoggerLog("OneBitAudio(Raw)\n");
						break;
						
					case 0x09:
						DoLoggerLog("OneBitAudio(SACD)\n");
						break;
						
					case 0x0A:
						DoLoggerLog("OneBitAudio(EncodedRaw)\n");
						break;
						
					case 0x0B:
						DoLoggerLog("OneBitAudio(EncodedSACD)\n");
						break;
						
					case 0x0C:
						DoLoggerLog("HPMBLA\n");
						break;
						
					case 0x0D:
						DoLoggerLog("MIDI\n");
						break;
						
					case 0x0E:
						DoLoggerLog("SMPTE-TimeCode\n");
						break;
						
					case 0x0F:
						DoLoggerLog("SampleCount\n");
						break;
						
					case 0x10:
						DoLoggerLog("AncillaryData\n");
						break;
						
					case 0x40:
						DoLoggerLog("SyncStream\n");
						break;
						
					default:
						DoLoggerLog("Unknown\n");
						break;
				};
			}
			
			if ((array) && (dict))
			{
				CFDictionarySetValue( dict, CFSTR("Formats"), array );
				CFRelease(array);
			}
		}
	}
	else if ((pFormatInfoBytes[0] == 0x90) && (pFormatInfoBytes[1] == 0x00))
	{
		// Format is AM824 single-stream (not compound)
		DoLoggerLog("    AM824 stream (not compound)\n")
		
		// Sample Rate
		switch(((pFormatInfoBytes[4] & 0xF0) >> 4))
		{
			case 0x00:
				DoLoggerLog("    22.05KHz\n")
				break;
				
			case 0x01:
				DoLoggerLog("    24KHz\n")
				break;
				
			case 0x02:
				DoLoggerLog("    32KHz\n")
				break;
				
			case 0x03:
				DoLoggerLog("    44.1KHz\n")
				break;
				
			case 0x04:
				DoLoggerLog("    48KHz\n")
				break;
				
			case 0x05:
				DoLoggerLog("    96KHz\n")
				break;
				
			case 0x06:
				DoLoggerLog("    176.4KHz\n")
				break;
				
			case 0x07:
				DoLoggerLog("    192KHz\n")
				break;
				
			case 0x0A:
				DoLoggerLog("    88.2KHz\n")
				break;
				
			default:
				DoLoggerLog("    Unknown Sample Rate\n")
				break;
				
		};
		
		// Signal Format
		numFormatInfoFields = 1;
		switch(pFormatInfoBytes[2])
		{
			case 0x00:
				DoLoggerLog("    IEC60958-3\n");
				break;
				
			case 0x01:
				DoLoggerLog("    IEC61937-3\n");
				break;
				
			case 0x02:
				DoLoggerLog("    IEC61937-4\n");
				break;
				
			case 0x03:
				DoLoggerLog("    IEC61937-5\n");
				break;
				
			case 0x04:
				DoLoggerLog("    IEC61937-6\n");
				break;
				
			case 0x05:
				DoLoggerLog("    IEC61937-7\n");
				break;
				
			case 0x06:
				DoLoggerLog("    MBLA\n");
				break;
				
			case 0x07:
				DoLoggerLog("    MBLA(DVD)\n");
				break;
				
			case 0x08:
				DoLoggerLog("    OneBitAudio(Raw)\n");
				break;
				
			case 0x09:
				DoLoggerLog("    OneBitAudio(SACD)\n");
				break;
				
			case 0x0A:
				DoLoggerLog("    OneBitAudio(EncodedRaw)\n");
				break;
				
			case 0x0B:
				DoLoggerLog("    OneBitAudio(EncodedSACD)\n");
				break;
				
			case 0x0C:
				DoLoggerLog("    HPMBLA\n");
				break;
				
			case 0x0D:
				DoLoggerLog("    MIDI\n");
				break;
				
			case 0x0E:
				DoLoggerLog("    SMPTE-TimeCode\n");
				break;
				
			case 0x0F:
				DoLoggerLog("    SampleCount\n");
				break;
				
			case 0x10:
				DoLoggerLog("    AncillaryData\n");
				break;
				
			case 0x40:
				DoLoggerLog("    SyncStream\n");
				break;
				
			default:
				DoLoggerLog("    Unknown\n");
				break;
		};
		
		if (dict)
		{
			CFDictionarySetValue( dict, CFSTR("StreamFormat"), CFSTR("AM824") ); 

			UInt32 sampleRate = ((pFormatInfoBytes[4] & 0xF0) >> 4);
			num = CFNumberCreate( kCFAllocatorDefault, 
								  kCFNumberIntType, 
								  &sampleRate ); 
			CFDictionarySetValue( dict, CFSTR("SampleRate"), num ); 
			CFRelease( num ); 
			
			UInt32 intVal = pFormatInfoBytes[2];
			num = CFNumberCreate( kCFAllocatorDefault, 
								  kCFNumberIntType, 
								  &intVal ); 
			CFDictionarySetValue( dict, CFSTR("AM824Format"), num ); 
			CFRelease( num ); 
		}
	}

	DoLoggerLog("=====================================\n")
}

#pragma mark -----------------------------------
#pragma mark MusicDevicePlug Class Methods
#pragma mark -----------------------------------

/////////////////////////////////////////
// MusicDevicePlug Constructor
/////////////////////////////////////////
MusicDevicePlug::MusicDevicePlug()
{
	pLogger = nil;

	sourceSubUnit = 0xEE;
	sourcePlugNum = 0xEE;
	sourcePlugStatus = 0xEE;
	
	pCurrentFormat = nil;
}

/////////////////////////////////////////
// MusicDevicePlug Destructor
/////////////////////////////////////////
MusicDevicePlug::~MusicDevicePlug() 
{
	ExtendedStreamFormatInfo *pFormatInfo;
	
	if (pCurrentFormat)
		delete pCurrentFormat;
	
	while (!supportedFormats.empty())
	{
		// Get a pointer to the element at the head of the queue
		pFormatInfo = supportedFormats.front();
		
		// Remove it from the queue
		supportedFormats.pop_front();
		
		// Delete the object
		delete pFormatInfo;
	}
}

/////////////////////////////////////////
// MusicDevicePlug::GetSignalSource
/////////////////////////////////////////
IOReturn MusicDevicePlug::GetSignalSource(CFMutableDictionaryRef dict)
{
    UInt32 size;
    UInt8 cmd[8],response[8];
    IOReturn res;
	CFMutableDictionaryRef signalSourceDict;
	CFNumberRef num; 
	
	// This function is only supported for dest plugs 
	if (!isDest)
		return kIOReturnUnsupported;
	
	// Preset the source values to "no connection" indication
	sourceSubUnit = 0xEE;
	sourcePlugNum = 0xEE;
	sourcePlugStatus = 0xEE;
	
	cmd[0] = kAVCStatusInquiryCommand;
	cmd[1] = 0xFF;
	cmd[2] = 0x1A;
	cmd[3] = 0xFF;
	cmd[4] = 0xFF;
	cmd[5] = 0xFE;
	cmd[6] = subUnit;
	cmd[7] = plugNum;
	size = 8;
	res = pMusicSubunitController->DoAVCCommand(cmd, 8, response, &size);
	if ((res == kIOReturnSuccess) && (response[0] == kAVCImplementedStatus))
	{
		sourceSubUnit = response[4];
		sourcePlugNum = response[5];
		sourcePlugStatus = response[3];
		
		DoLoggerLog("Source plug 0x%02X/0x%02X connected to destination plug 0x%02X/0x%02X (status = 0x%02X)\n",
				   sourceSubUnit,sourcePlugNum,subUnit,plugNum,sourcePlugStatus);
		
		if (dict)
		{
			signalSourceDict = CFDictionaryCreateMutable( kCFAllocatorDefault, 
														  0, 
														  &kCFTypeDictionaryKeyCallBacks, 
														  &kCFTypeDictionaryValueCallBacks ); 
			if (signalSourceDict)
			{
				UInt32 intVal = sourceSubUnit;
				num = CFNumberCreate( kCFAllocatorDefault, 
									  kCFNumberIntType, 
									  &intVal ); 
				CFDictionarySetValue( signalSourceDict, CFSTR("SourceSubUnit"), num ); 
				CFRelease( num ); 

				intVal = sourcePlugNum;
				num = CFNumberCreate( kCFAllocatorDefault, 
									  kCFNumberIntType, 
									  &intVal ); 
				CFDictionarySetValue( signalSourceDict, CFSTR("SourcePlugNum"), num ); 
				CFRelease( num ); 

				intVal = sourcePlugStatus;
				num = CFNumberCreate( kCFAllocatorDefault, 
									  kCFNumberIntType, 
									  &intVal ); 
				CFDictionarySetValue( signalSourceDict, CFSTR("SourcePlugStatus"), num ); 
				CFRelease( num ); 
				
				CFDictionarySetValue( dict, CFSTR("SignalSource"), signalSourceDict ); 
				CFRelease(signalSourceDict);
			}
		}
	}
	else
	{
		res = kIOReturnError;
	}
	return res;
}

/////////////////////////////////////////
// MusicDevicePlug::GetCurrentStreamFormatInformation
/////////////////////////////////////////
IOReturn MusicDevicePlug::GetCurrentStreamFormatInformation(CFMutableDictionaryRef dict)
{
    UInt32 size;
    UInt8 cmd[11],response[512];
    IOReturn res;
	UInt32 formatInformationSize;
	UInt32 numFormatInfoFields;
	CFMutableDictionaryRef currentStreamFormatInfoDict;
	
	currentStreamFormatInfoDict = CFDictionaryCreateMutable( kCFAllocatorDefault, 
															 0, 
															 &kCFTypeDictionaryKeyCallBacks, 
															 &kCFTypeDictionaryValueCallBacks ); 
	if (!currentStreamFormatInfoDict)
		return kIOReturnNoMemory;
		
	if (pCurrentFormat)
	{
		delete pCurrentFormat;
		pCurrentFormat = nil;
	}
	
	cmd[0] = kAVCStatusInquiryCommand;
	cmd[1] = subUnit;
	cmd[2] = pMusicSubunitController->extendedStreamFormatCommandOpcode;
	cmd[3] = 0xC0;
	cmd[9] = 0xFF;
	cmd[10] = 0xFF;

	if (subUnit = 0xFF)
	{
		// A unit plugs
		if (isDest)
			cmd[4] = 0x01;	// Unit dest plugs are outputs
		else
			cmd[4] = 0x00; 
		cmd[5] = 0x00;		// A unit plug
		if (plugNum < 0x80)
			cmd[6] = 0x00;	// A PCR
		else
			cmd[6] = 0x01;	// An external plug
		cmd[7] = plugNum;
		cmd[8] = 0xFF;
	}
	else
	{
		// A subunit plug
		if (isDest)
			cmd[4] = 0x00;	// Subunit dest plugs are inputs
		else
			cmd[4] = 0x01;
		cmd[5] = 0x01;		// A subunit plug
		cmd[6] = plugNum;
		cmd[7] = 0xFF; 
		cmd[8] = 0xFF;
	}

	size = 512;
	res = pMusicSubunitController->DoAVCCommand(cmd, 11, response, &size);
	
	// If the AVC command completed, but with an not-implemented response, and we haven't tried the old extendedStreamFormatCommandOpcode, do so now
	if ((res == kIOReturnSuccess) && (response[0] == kAVCNotImplementedStatus) && (pMusicSubunitController->extendedStreamFormatCommandOpcode == 0xBF))
	{
		DoLoggerLog("extendedStreamFormatCommandOpcode of 0xBF not supported, switching to 0x2F\n");
		pMusicSubunitController->extendedStreamFormatCommandOpcode = 0x2F;
		cmd[2] = pMusicSubunitController->extendedStreamFormatCommandOpcode;
		size = 512;
		res = pMusicSubunitController->DoAVCCommand(cmd, 11, response, &size);
	}
	
	if ((res == kIOReturnSuccess) && (response[0] == kAVCImplementedStatus))
	{
		if (subUnit == 0xFF)
		{
			DoLoggerLog("Got current stream format information for %s plug 0x%02X/0x%02X\n",(isDest == true) ? "unit output" : "unit input",subUnit,plugNum);
		}
		else
		{
			DoLoggerLog("Got current stream format information for %s plug 0x%02X/0x%02X\n",(isDest == true) ? "subunit dest" : "subunit source",subUnit,plugNum);
		}
		
		formatInformationSize = size - 10;

		if ((formatInformationSize > 1) && (response[10] == 0x90) && (response[11] == 0x40))
		{
			// Format is "compound AM824 stream"

			// Make sure formatInformationSize is reasonable
			if (formatInformationSize > 5)
			{
				numFormatInfoFields = response[14];
				//if (formatInformationSize == (5+(2*numFormatInfoFields)))
				if (formatInformationSize >= (5+(2*numFormatInfoFields)))	// Work around a particular device that sends too many response bytes!

				{
					// formatInformationSize is correct. 
					
					// Save a copy
					pCurrentFormat = new ExtendedStreamFormatInfo(pLogger);
					if (pCurrentFormat)
					{
						pCurrentFormat->InitWithFormatInfo(&response[10],formatInformationSize);
						pCurrentFormat->PrintFormatInformation(currentStreamFormatInfoDict);
					}
				}
				else
				{
					DoLoggerLog("ERROR: Format information field is invalid length in MusicDevicePlug::GetCurrentStreamFormatInformation\n");
				}
			}
			else
			{
				DoLoggerLog("ERROR: Format information field is invalid length in MusicDevicePlug::GetCurrentStreamFormatInformation\n");
			}
		}
		else if ((formatInformationSize == 6) && (response[10] == 0x90) && (response[11] == 0x00))
		{
			// Format is AM824 single-stream (not compound)
			
			// Save a copy
			pCurrentFormat = new ExtendedStreamFormatInfo(pLogger);
			if (pCurrentFormat)
			{
				pCurrentFormat->InitWithFormatInfo(&response[10],formatInformationSize);
				pCurrentFormat->PrintFormatInformation(currentStreamFormatInfoDict);
			}
		}
	}	
	else
	{
		if (subUnit ==  0xFF)
		{
			DoLoggerLog("Unsucessful attempt at retrieving stream format information for %s plug 0x%02X/0x%02X\n",
				   (isDest == true) ? "unit output" : "unit input",subUnit,plugNum);

		}
		else
		{
			DoLoggerLog("Unsucessful attempt at retrieving stream format information for %s plug 0x%02X/0x%02X\n",
					   (isDest == true) ? "subunit dest" : "subunit source",subUnit,plugNum);
		}
		res = kIOReturnError;
	}
	
	if (dict)
	{
		CFDictionarySetValue( dict, CFSTR("CurrentStreamFormatInformation"), currentStreamFormatInfoDict ); 
	}
	CFRelease(currentStreamFormatInfoDict);
	
	return res;
}

/////////////////////////////////////////
// MusicDevicePlug::GetSupportedStreamFormatInformation
/////////////////////////////////////////
IOReturn MusicDevicePlug::GetSupportedStreamFormatInformation(CFMutableDictionaryRef dict)
{
    UInt32 size;
    UInt8 cmd[11],response[512];
    IOReturn res;
	UInt32 formatInformationSize;
	UInt32 numFormatInfoFields;
	ExtendedStreamFormatInfo *pFormatInfo;
	bool done = false;
	UInt8 index = 0;
	UInt32 i;
	CFMutableArrayRef array;
	CFMutableDictionaryRef streamFmtDict;

	while (!supportedFormats.empty())
	{
		// Get a pointer to the element at the head of the queue
		pFormatInfo = supportedFormats.front();
		
		// Remove it from the queue
		supportedFormats.pop_front();
		
		// Delete the object
		delete pFormatInfo;
	}

	cmd[0] = kAVCStatusInquiryCommand;
	cmd[1] = subUnit;
	cmd[2] = pMusicSubunitController->extendedStreamFormatCommandOpcode;
	cmd[3] = 0xC1;
	cmd[9] = 0xFF;
	
	if (subUnit = 0xFF)
	{
		// A unit plugs
		if (isDest)
			cmd[4] = 0x01;	// Unit dest plugs are outputs
		else
			cmd[4] = 0x00; 
		cmd[5] = 0x00;		// A unit plug
		if (plugNum < 0x80)
			cmd[6] = 0x00;	// A PCR
		else
			cmd[6] = 0x01;	// An external plug
		cmd[7] = plugNum;
		cmd[8] = 0xFF;
	}
	else
	{
		// A subunit plug
		if (isDest)
			cmd[4] = 0x00;	// Subunit dest plugs are inputs
		else
			cmd[4] = 0x01;
		cmd[5] = 0x01;		// A subunit plug
		cmd[6] = plugNum;
		cmd[7] = 0xFF; 
		cmd[8] = 0xFF;
	}
	
	while (!done)
	{
		cmd[10] = index;

		size = 512;
		res = pMusicSubunitController->DoAVCCommand(cmd, 11, response, &size);
		
		// If the AVC command completed, but with an not-implemented response, and we haven't tried the old extendedStreamFormatCommandOpcode, do so now
		if ((res == kIOReturnSuccess) && (response[0] == kAVCNotImplementedStatus) && (pMusicSubunitController->extendedStreamFormatCommandOpcode == 0xBF))
		{
			DoLoggerLog("extendedStreamFormatCommandOpcode of 0xBF not supported, switching to 0x2F\n");
			pMusicSubunitController->extendedStreamFormatCommandOpcode = 0x2F;
			cmd[2] = pMusicSubunitController->extendedStreamFormatCommandOpcode;
			size = 512;
			res = pMusicSubunitController->DoAVCCommand(cmd, 11, response, &size);
		}
		
		if ((res == kIOReturnSuccess) && (response[0] == kAVCImplementedStatus))
		{
			if (subUnit == 0xFF)
			{
				DoLoggerLog("Got supported stream format information for %s plug 0x%02X/0x%02X (index=%u)\n",
						   (isDest == true) ? "unit output" : "unit input",
						   subUnit,
						   plugNum,
						   (unsigned int)index);
			}
			else
			{
				DoLoggerLog("Got supported stream format information for %s plug 0x%02X/0x%02X (index=%u)\n",
						   (isDest == true) ? "subunit dest" : "subunit source",
						   subUnit,
						   plugNum,
						   (unsigned int)index);
			}
			
			formatInformationSize = size - 11;
			
			if ((formatInformationSize > 1) && (response[11] == 0x90) && (response[12] == 0x40))
			{
				// Format is "compound AM824 stream"
				
				// Make sure formatInformationSize is reasonable
				if (formatInformationSize > 5)
				{
					numFormatInfoFields = response[15];
					//if (formatInformationSize == (5+(2*numFormatInfoFields)))
					if (formatInformationSize >= (5+(2*numFormatInfoFields)))	// Work around a particular device that sends too many response bytes!
					{
						// formatInformationSize is correct. 
						
						// Create a copy
						pFormatInfo = new ExtendedStreamFormatInfo(pLogger);
						if (pFormatInfo)
						{
							pFormatInfo->InitWithFormatInfo(&response[11],formatInformationSize);
							//pFormatInfo->PrintFormatInformation();
							supportedFormats.push_back(pFormatInfo);
						}
					}
					else
					{
						DoLoggerLog("ERROR: Format information field is invalid length in MusicDevicePlug::GetSupportedStreamFormatInformation\n");
					}
				}
				else
				{
					DoLoggerLog("ERROR: Format information field is invalid length in MusicDevicePlug::GetSupportedStreamFormatInformation\n");
				}
			}
			else if ((formatInformationSize == 6) && (response[11] == 0x90) && (response[12] == 0x00))
			{
				// Format is AM824 single-stream (not compound)
				
				// Save a copy
				pFormatInfo = new ExtendedStreamFormatInfo(pLogger);
				if (pFormatInfo)
				{
					pFormatInfo->InitWithFormatInfo(&response[11],formatInformationSize);
					//pFormatInfo->PrintFormatInformation();
					supportedFormats.push_back(pFormatInfo);
				}
			}
		}
		else
			done = true;	// We got an error, so we must be done!
		
		index += 1;
	};
	
	DoLoggerLog("Total supported stream formats detected:%u\n",(unsigned int)supportedFormats.size());
	
	if (supportedFormats.size())
	{
		array = CFArrayCreateMutable( kCFAllocatorDefault,0,&kCFTypeArrayCallBacks);

		for (i=0;i<supportedFormats.size();i++)
		{
			streamFmtDict = CFDictionaryCreateMutable( kCFAllocatorDefault, 
													  0, 
													  &kCFTypeDictionaryKeyCallBacks, 
													  &kCFTypeDictionaryValueCallBacks ); 
			
			pFormatInfo = supportedFormats.at(i);
			pFormatInfo->PrintFormatInformation(streamFmtDict);

			if ((array) && (streamFmtDict))
			{
				CFArrayAppendValue(array,streamFmtDict);
				CFRelease(streamFmtDict);
			}
		}

		if ((array) && (dict))
		{
			CFDictionarySetValue( dict, CFSTR("SupportedStreamFormatInformation"), array ); 
			CFRelease(array);
		}	
	}
	
	return res;
}

/////////////////////////////////////////
// MusicDevicePlug::SetCurrentStreamFormat
/////////////////////////////////////////
IOReturn MusicDevicePlug::SetCurrentStreamFormat(ExtendedStreamFormatInfo *pFormat)
{
    UInt32 size;
    UInt8 cmd[10+pFormat->len],response[512];
    IOReturn res;
	
	cmd[0] = kAVCControlCommand;
	cmd[1] = subUnit;
	cmd[2] = pMusicSubunitController->extendedStreamFormatCommandOpcode;
	cmd[3] = 0xC0;
	cmd[9] = 0xFF;
	
	if (subUnit = 0xFF)
	{
		// A unit plugs
		if (isDest)
			cmd[4] = 0x01;	// Unit dest plugs are outputs
		else
			cmd[4] = 0x00; 
		cmd[5] = 0x00;		// A unit plug
		if (plugNum < 0x80)
			cmd[6] = 0x00;	// A PCR
		else
			cmd[6] = 0x01;	// An external plug
		cmd[7] = plugNum;
		cmd[8] = 0xFF;
	}
	else
	{
		// A subunit plug
		if (isDest)
			cmd[4] = 0x00;	// Subunit dest plugs are inputs
		else
			cmd[4] = 0x01;
		cmd[5] = 0x01;		// A subunit plug
		cmd[6] = plugNum;
		cmd[7] = 0xFF; 
		cmd[8] = 0xFF;
	}
	
	// Copy the stream format info into the command
	bcopy(pFormat->pFormatInfoBytes,&cmd[10],pFormat->len);
	
	size = 512;
	res = pMusicSubunitController->DoAVCCommand(cmd, 10+pFormat->len, response, &size);
	if ((res == kIOReturnSuccess) && ((response[0] == kAVCAcceptedStatus) || (response[0] == kAVCImplementedStatus)))
	{
		// Succeeded, update the current
		DoLoggerLog("MusicDevicePlug::SetCurrentStreamFormat succeeded\n");
		GetCurrentStreamFormatInformation();
	}
	else
	{
		DoLoggerLog("ERROR: MusicDevicePlug::SetCurrentStreamFormat failed\n");
		res = kIOReturnError;
	}

	return res;
}

#pragma mark -----------------------------------
#pragma mark AVCInfoBlock Class Methods
#pragma mark -----------------------------------

///////////////////////////////
// Constructor
///////////////////////////////
AVCInfoBlock::AVCInfoBlock(UInt8 *pInfoBlockBytes, StringLogger *pStringLogger)
{
	UInt32 sizeOfInfoBlock = ((pInfoBlockBytes[0]*256)+pInfoBlockBytes[1]+2);
	AVCInfoBlock *pNestedInfoBlock;
	UInt8 *pStartOfNestedInfoBlock;

	pLogger = pStringLogger;
	
	pBlockBytes = new UInt8[sizeOfInfoBlock];
	if (pBlockBytes)
	{
		bcopy(pInfoBlockBytes,pBlockBytes,sizeOfInfoBlock);
		
		// Extract the block type
		infoBlockType = ((pBlockBytes[2]*256)+pBlockBytes[3]);
		
		// Discover Nested Info Blocks
		pStartOfNestedInfoBlock = (pBlockBytes + 6 + PrimaryFieldsLength());
		while (pStartOfNestedInfoBlock < (pBlockBytes + CompoundLength() + 2))
		{
			pNestedInfoBlock = new AVCInfoBlock(pStartOfNestedInfoBlock,pLogger);
			if (pNestedInfoBlock)
				nestedInfoBlockList.push_back(pNestedInfoBlock);
			
			pStartOfNestedInfoBlock += (pNestedInfoBlock->CompoundLength() + 2);
		}
	}
}

///////////////////////////////
// Destructor
///////////////////////////////
AVCInfoBlock::~AVCInfoBlock()
{
	AVCInfoBlock *pNestedAVCInfoBlock;
	
	// Delete Nested Info Blocks
	while (!nestedInfoBlockList.empty())
	{
		// Get a pointer to the element at the head of the queue
		pNestedAVCInfoBlock = nestedInfoBlockList.front();
		
		// Remove it from the queue
		nestedInfoBlockList.pop_front();
		
		// Delete the object
		delete pNestedAVCInfoBlock;
	}
	
	if (pBlockBytes)
		delete [] pBlockBytes;
}

////////////////////////////////////////
// AVCInfoBlock::CompoundLength
////////////////////////////////////////
UInt16 AVCInfoBlock::CompoundLength(void)
{
	return ((pBlockBytes[0]*256)+pBlockBytes[1]);
}

////////////////////////////////////////
// AVCInfoBlock::PrimaryFieldsLength
////////////////////////////////////////
UInt16 AVCInfoBlock::PrimaryFieldsLength(void)
{
	return ((pBlockBytes[4]*256)+pBlockBytes[5]);
}

////////////////////////////////////////
// AVCInfoBlock::InfoBlockType
////////////////////////////////////////
UInt16 AVCInfoBlock::InfoBlockType(void)
{
	return infoBlockType;
}

////////////////////////////////////////
// AVCInfoBlock::GetInfoBlockBytes
////////////////////////////////////////
bool AVCInfoBlock::GetInfoBlockBytes(UInt8 *pBuffer, UInt16 *pLength)
{
	if (*pLength < (CompoundLength()+2))
		return false;
	else
	{
		bcopy(pBlockBytes, pBuffer,(CompoundLength()+2));
		*pLength = (CompoundLength()+2);
		
	}
	return true;
}

////////////////////////////////////////
// AVCInfoBlock::GetPrimaryFields
////////////////////////////////////////
bool AVCInfoBlock::GetPrimaryFields(UInt8 *pPrimaryFieldsBytes, UInt16 *pLength)
{
	if (*pLength < PrimaryFieldsLength())
		return false;
	else
	{
		if (PrimaryFieldsLength() > 0)
			bcopy(&pBlockBytes[6], pPrimaryFieldsBytes,PrimaryFieldsLength());
		*pLength = PrimaryFieldsLength();
	}
	return true;
}

////////////////////////////////////////
// AVCInfoBlock::GetNestedInfoBlock
////////////////////////////////////////
AVCInfoBlock* AVCInfoBlock::GetNestedInfoBlock(unsigned short info_block_type,UInt16 index)
{
	UInt32 i;
	UInt32 foundSoFar = 0;
	AVCInfoBlock *pNestedAVCInfoBlock;
	
	for (i=0;i<nestedInfoBlockList.size();i++)
	{
		pNestedAVCInfoBlock = nestedInfoBlockList.at(i);
		if (pNestedAVCInfoBlock->InfoBlockType() == info_block_type)
		{
			if (foundSoFar == index)
				return pNestedAVCInfoBlock;	// Found the one we're looking for!
			else
				foundSoFar += 1;
		}
	}
	return nil;
}

static void DoIndentation(UInt32 depth, StringLogger *pLogger)
{
	UInt32 i;
	
	for (i=0;i<depth;i++)
	{
		DoLoggerLog("    ");
	}
}

////////////////////////////////////////
// AVCInfoBlock::ParseInfoBlock
////////////////////////////////////////
IOReturn AVCInfoBlock::ParseInfoBlock(UInt32 nestingDepth,CFMutableArrayRef array)
{
	AVCInfoBlock *pNestedAVCInfoBlock;
	UInt32 i;
	UInt32 remainingBytesInPrimaryField;
	UInt8 *pNameInfoBlockNestedInfoBlockBytes;
	CFMutableArrayRef nestedArray;
	CFMutableDictionaryRef infoBlockDict;
	CFNumberRef num;
	UInt32 intVal;
	CFMutableArrayRef signalArray;
	CFMutableDictionaryRef signalDict;
	CFDataRef data;
	
	infoBlockDict = CFDictionaryCreateMutable( kCFAllocatorDefault, 
											   0, 
											   &kCFTypeDictionaryKeyCallBacks, 
											   &kCFTypeDictionaryValueCallBacks ); 
	if (!infoBlockDict)
		return kIOReturnNoMemory;
	
	UInt16 rawBytesBufLen = CompoundLength()+2;
	UInt8 *pRawBytes = new UInt8[rawBytesBufLen];
	if (!pRawBytes)
		return kIOReturnNoMemory;
	GetInfoBlockBytes(pRawBytes,&rawBytesBufLen);		
	data = CFDataCreate( kCFAllocatorDefault, pRawBytes, rawBytesBufLen ); 
	if (!data)
		return kIOReturnNoMemory;
	CFDictionarySetValue( infoBlockDict, CFSTR("RawInfoBlockBytes"), data ); 
	CFRelease( data ); 
	delete [] pRawBytes;
	
	DoIndentedLoggerLog(nestingDepth,"{\n");

	DoIndentedLoggerLog(nestingDepth+1,"AVCInfoBlock Type: 0x%04X (%s)\n",infoBlockType,MusicSubunitInfoBlockTypeDescriptions(infoBlockType));
	DoIndentedLoggerLog(nestingDepth+1,"CompoundLength: %u, PrimaryFieldsLength: %u\n",
			   (unsigned int)CompoundLength(),
			   (unsigned int)PrimaryFieldsLength());

	intVal = infoBlockType;
	num = CFNumberCreate( kCFAllocatorDefault, 
						  kCFNumberIntType, 
						  &intVal ); 
	CFDictionarySetValue( infoBlockDict, CFSTR("InfoBlockType"), num ); 
	CFRelease( num ); 

	intVal = CompoundLength();
	num = CFNumberCreate( kCFAllocatorDefault, 
						  kCFNumberIntType, 
						  &intVal ); 
	CFDictionarySetValue( infoBlockDict, CFSTR("CompoundLength"), num ); 
	CFRelease( num ); 

	intVal = PrimaryFieldsLength();
	num = CFNumberCreate( kCFAllocatorDefault, 
						  kCFNumberIntType, 
						  &intVal ); 
	CFDictionarySetValue( infoBlockDict, CFSTR("PrimaryFieldsLength"), num ); 
	CFRelease( num ); 
	
	// Decode primary fields of specific info blocks 
	switch (infoBlockType)
	{
		// "General Music Subunit Status Area Info Block"
		case 0x8100:
			DoIndentedLoggerLog(nestingDepth+1,"current_transmit_capability: 0x%02X\n",
					   (unsigned int)pBlockBytes[6]);
			
			intVal = pBlockBytes[6];
			num = CFNumberCreate( kCFAllocatorDefault, 
								  kCFNumberIntType, 
								  &intVal ); 
			CFDictionarySetValue( infoBlockDict, CFSTR("CurrentTransmitCapability"), num ); 
			CFRelease( num ); 
			
			DoIndentedLoggerLog(nestingDepth+1,"current_receive_capability: 0x%02X\n",
					   (unsigned int)pBlockBytes[7]);

			intVal = pBlockBytes[7];
			num = CFNumberCreate( kCFAllocatorDefault, 
								  kCFNumberIntType, 
								  &intVal ); 
			CFDictionarySetValue( infoBlockDict, CFSTR("CurrentReceiveCapability"), num ); 
			CFRelease( num ); 
			
			DoIndentedLoggerLog(nestingDepth+1,"current_latency_capability: 0x%02X%02X%02X%02X\n",
					   (unsigned int)pBlockBytes[8],(unsigned int)pBlockBytes[9],(unsigned int)pBlockBytes[10],(unsigned int)pBlockBytes[11]);
			
			intVal = ((unsigned int)pBlockBytes[8] << 24) + ((unsigned int)pBlockBytes[9] << 16) + ((unsigned int)pBlockBytes[10] << 8) + (unsigned int)pBlockBytes[11];
			num = CFNumberCreate( kCFAllocatorDefault, 
								  kCFNumberIntType, 
								  &intVal ); 
			CFDictionarySetValue( infoBlockDict, CFSTR("CurrentLatencyCapability"), num ); 
			CFRelease( num ); 
			break;
			
		// "Music Output Plug Status Area Info Block"
		case 0x8101:
			DoIndentedLoggerLog(nestingDepth+1,"number_of_source_plugs: %u\n",
					   (unsigned int)pBlockBytes[6]);
			
			intVal = pBlockBytes[6];
			num = CFNumberCreate( kCFAllocatorDefault, 
								  kCFNumberIntType, 
								  &intVal ); 
			CFDictionarySetValue( infoBlockDict, CFSTR("NumSourcePlugs"), num ); 
			CFRelease( num ); 
			break;
			
		// "Source Plug Status Info Block"
		case 0x8102:
			DoIndentedLoggerLog(nestingDepth+1,"source_plugs_number: %u\n",
							   (unsigned int)pBlockBytes[6]);
			
			intVal = pBlockBytes[6];
			num = CFNumberCreate( kCFAllocatorDefault, 
								  kCFNumberIntType, 
								  &intVal ); 
			CFDictionarySetValue( infoBlockDict, CFSTR("SourcePlugNum"), num ); 
			CFRelease( num ); 
			break;
			
		// "Audio Info Block"
		case 0x8103:
			DoIndentedLoggerLog(nestingDepth+1,"number_of_audio_streams: %u\n",
							   (unsigned int)pBlockBytes[6]);
			
			intVal = pBlockBytes[6];
			num = CFNumberCreate( kCFAllocatorDefault, 
								  kCFNumberIntType, 
								  &intVal ); 
			CFDictionarySetValue( infoBlockDict, CFSTR("NumAudioStreams"), num ); 
			CFRelease( num ); 
			break;
			
		// "MIDI Info Block"
		case 0x8104:
			DoIndentedLoggerLog(nestingDepth+1,"number_of_MIDI_streams: %u\n",
							   (unsigned int)pBlockBytes[6]);
			
			intVal = pBlockBytes[6];
			num = CFNumberCreate( kCFAllocatorDefault, 
								  kCFNumberIntType, 
								  &intVal ); 
			CFDictionarySetValue( infoBlockDict, CFSTR("NumMIDIStreams"), num ); 
			CFRelease( num ); 
			break;
			
		// "SMPTE Time Code Info Block"
		case 0x8105:
			DoIndentedLoggerLog(nestingDepth+1,"SMPTE_time_code_activity: 0x%02X\n",
							   (unsigned int)pBlockBytes[6]);

			intVal = pBlockBytes[6];
			num = CFNumberCreate( kCFAllocatorDefault, 
								  kCFNumberIntType, 
								  &intVal ); 
			CFDictionarySetValue( infoBlockDict, CFSTR("SMPTETimeCodeActivity"), num ); 
			CFRelease( num ); 
			break;
			
		// "Sample Count Info Block"
		case 0x8106:
			DoIndentedLoggerLog(nestingDepth+1,"sample_count_activity: 0x%02X\n",
							   (unsigned int)pBlockBytes[6]);

			intVal = pBlockBytes[6];
			num = CFNumberCreate( kCFAllocatorDefault, 
								  kCFNumberIntType, 
								  &intVal ); 
			CFDictionarySetValue( infoBlockDict, CFSTR("SampleCountActivity"), num ); 
			CFRelease( num ); 
			break;
			
		// "Audio SYNC Info Block"
		case 0x8107:
			DoIndentedLoggerLog(nestingDepth+1,"audio_sync_activity: 0x%02X\n",
							   (unsigned int)pBlockBytes[6]);
			
			intVal = pBlockBytes[6];
			num = CFNumberCreate( kCFAllocatorDefault, 
								  kCFNumberIntType, 
								  &intVal ); 
			CFDictionarySetValue( infoBlockDict, CFSTR("AudioSyncActivity"), num ); 
			CFRelease( num ); 
			break;
			
		// "Routing Status Info Block"
		case 0x8108:
			DoIndentedLoggerLog(nestingDepth+1,"number_of_subunit_dest_plugs: %u\n",
							   (unsigned int)pBlockBytes[6]);
			DoIndentedLoggerLog(nestingDepth+1,"number_of_subunit_source_plugs: %u\n",
							   (unsigned int)pBlockBytes[7]);
			DoIndentedLoggerLog(nestingDepth+1,"number_of_music_plugs: %u\n",
							   (unsigned int)((pBlockBytes[8]*256)+pBlockBytes[9]));
			
			intVal = pBlockBytes[6];
			num = CFNumberCreate( kCFAllocatorDefault, 
								  kCFNumberIntType, 
								  &intVal ); 
			CFDictionarySetValue( infoBlockDict, CFSTR("NumSubunitDestPlugs"), num ); 
			CFRelease( num ); 
			
			intVal = pBlockBytes[7];
			num = CFNumberCreate( kCFAllocatorDefault, 
								  kCFNumberIntType, 
								  &intVal ); 
			CFDictionarySetValue( infoBlockDict, CFSTR("NumSubunitSourcePlugs"), num ); 
			CFRelease( num ); 

			intVal = ((pBlockBytes[8]*256)+pBlockBytes[9]);
			num = CFNumberCreate( kCFAllocatorDefault, 
								  kCFNumberIntType, 
								  &intVal ); 
			CFDictionarySetValue( infoBlockDict, CFSTR("NumMusicPlugs"), num ); 
			CFRelease( num ); 
			break;
			
		// "Subunit Plug Info Block"
		case 0x8109:
			DoIndentedLoggerLog(nestingDepth+1,"subunit_plug_id: %u\n",
							   (unsigned int)pBlockBytes[6]);
			DoIndentedLoggerLog(nestingDepth+1,"signal_format: 0x%02X%02X\n",
							   (unsigned int)pBlockBytes[7],(unsigned int)pBlockBytes[8]);
			DoIndentedLoggerLog(nestingDepth+1,"plug_type: 0x%02X\n",
							   (unsigned int)pBlockBytes[9]);
			DoIndentedLoggerLog(nestingDepth+1,"number_of_clusters: %u\n",
							   (unsigned int)((pBlockBytes[10]*256)+pBlockBytes[11]));
			DoIndentedLoggerLog(nestingDepth+1,"number_of_channels: %u\n",
							   (unsigned int)((pBlockBytes[12]*256)+pBlockBytes[13]));
			
			intVal = pBlockBytes[6];
			num = CFNumberCreate( kCFAllocatorDefault, 
								  kCFNumberIntType, 
								  &intVal ); 
			CFDictionarySetValue( infoBlockDict, CFSTR("SubunitPlugID"), num ); 
			CFRelease( num ); 
			
			intVal = ((unsigned int)pBlockBytes[7] << 8) + (unsigned int)pBlockBytes[8];
			num = CFNumberCreate( kCFAllocatorDefault, 
								  kCFNumberIntType, 
								  &intVal ); 
			CFDictionarySetValue( infoBlockDict, CFSTR("SignalFormat"), num ); 
			CFRelease( num ); 
			
			intVal = pBlockBytes[9];
			num = CFNumberCreate( kCFAllocatorDefault, 
								  kCFNumberIntType, 
								  &intVal ); 
			CFDictionarySetValue( infoBlockDict, CFSTR("PlugType"), num ); 
			CFRelease( num ); 
			
			intVal = ((pBlockBytes[10]*256)+pBlockBytes[11]);
			num = CFNumberCreate( kCFAllocatorDefault, 
								  kCFNumberIntType, 
								  &intVal ); 
			CFDictionarySetValue( infoBlockDict, CFSTR("NumClusters"), num ); 
			CFRelease( num ); 
			
			intVal = ((pBlockBytes[12]*256)+pBlockBytes[13]);
			num = CFNumberCreate( kCFAllocatorDefault, 
								  kCFNumberIntType, 
								  &intVal ); 
			CFDictionarySetValue( infoBlockDict, CFSTR("NumChannels"), num ); 
			CFRelease( num ); 
			break;
			
		// "Cluster Info Block"
		case 0x810A:
			DoIndentedLoggerLog(nestingDepth+1,"stream_format: 0x%02X\n",
							   (unsigned int)pBlockBytes[6]);
			DoIndentedLoggerLog(nestingDepth+1,"port_type: 0x%02X\n",
							   (unsigned int)pBlockBytes[7]);
			DoIndentedLoggerLog(nestingDepth+1,"number_of_signals: %u\n",
							   (unsigned int)pBlockBytes[8]);
			
			intVal = pBlockBytes[6];
			num = CFNumberCreate( kCFAllocatorDefault, 
								  kCFNumberIntType, 
								  &intVal ); 
			CFDictionarySetValue( infoBlockDict, CFSTR("StreamFormat"), num ); 
			CFRelease( num ); 
			
			intVal = pBlockBytes[7];
			num = CFNumberCreate( kCFAllocatorDefault, 
								  kCFNumberIntType, 
								  &intVal ); 
			CFDictionarySetValue( infoBlockDict, CFSTR("PortType"), num ); 
			CFRelease( num ); 
			
			intVal = pBlockBytes[8];
			num = CFNumberCreate( kCFAllocatorDefault, 
								  kCFNumberIntType, 
								  &intVal ); 
			CFDictionarySetValue( infoBlockDict, CFSTR("NumSignals"), num ); 
			CFRelease( num ); 
			
			if (pBlockBytes[8])
			{
				signalArray = CFArrayCreateMutable( kCFAllocatorDefault,0,&kCFTypeArrayCallBacks);
				if (!signalArray)
					return kIOReturnNoMemory;
				
				for (i=0;i<pBlockBytes[8];i++)
				{
					signalDict = CFDictionaryCreateMutable( kCFAllocatorDefault, 
															0, 
															&kCFTypeDictionaryKeyCallBacks, 
															&kCFTypeDictionaryValueCallBacks ); 
					if (!signalDict)
						return kIOReturnNoMemory;
					
					DoIndentedLoggerLog(nestingDepth+1,"signal %u:\n",
										(unsigned int)i);
					DoIndentedLoggerLog(nestingDepth+1," music_plug_id: 0x%02X%02X\n",
										(unsigned int)pBlockBytes[9+(i*4)],(unsigned int)pBlockBytes[10+(i*4)]);
					DoIndentedLoggerLog(nestingDepth+1," stream_position: %u\n",
										(unsigned int)pBlockBytes[11+(i*4)]);
					DoIndentedLoggerLog(nestingDepth+1," stream_location: %u\n",
										(unsigned int)pBlockBytes[12+(i*4)]);

					intVal = i;
					num = CFNumberCreate( kCFAllocatorDefault, 
										  kCFNumberIntType, 
										  &intVal ); 
					CFDictionarySetValue( signalDict, CFSTR("Signal"), num ); 
					CFRelease( num ); 
					
					intVal = ((unsigned int)pBlockBytes[9+(i*4)] << 8) + (unsigned int)pBlockBytes[10+(i*4)];
					num = CFNumberCreate( kCFAllocatorDefault, 
										  kCFNumberIntType, 
										  &intVal ); 
					CFDictionarySetValue( signalDict, CFSTR("MusicPlugID"), num ); 
					CFRelease( num ); 
					
					intVal = pBlockBytes[11+(i*4)];
					num = CFNumberCreate( kCFAllocatorDefault, 
										  kCFNumberIntType, 
										  &intVal ); 
					CFDictionarySetValue( signalDict, CFSTR("StreamPosition"), num ); 
					CFRelease( num ); 
					
					intVal = pBlockBytes[12+(i*4)];
					num = CFNumberCreate( kCFAllocatorDefault, 
										  kCFNumberIntType, 
										  &intVal ); 
					CFDictionarySetValue( signalDict, CFSTR("StreamLocation"), num ); 
					CFRelease( num ); 
					
					CFArrayAppendValue(signalArray,signalDict);
					CFRelease(signalDict);

				}
				CFDictionarySetValue( infoBlockDict, CFSTR("Signals"), signalArray ); 
				CFRelease(signalArray);
			}
			break;
			
		// "Music Plug Info Block"
		case 0x810B:
			DoIndentedLoggerLog(nestingDepth+1,"music_plug_type: 0x%02X\n",
							   (unsigned int)pBlockBytes[6]);
			intVal = pBlockBytes[6];
			num = CFNumberCreate( kCFAllocatorDefault, 
								  kCFNumberIntType, 
								  &intVal ); 
			CFDictionarySetValue( infoBlockDict, CFSTR("MusicPlugType"), num ); 
			CFRelease( num ); 
			
			DoIndentedLoggerLog(nestingDepth+1,"music_plug_id: 0x%02X%02X\n",
							   (unsigned int)pBlockBytes[7],(unsigned int)pBlockBytes[8]);
			intVal = ((unsigned int)pBlockBytes[7] << 8)+ (unsigned int)pBlockBytes[8];
			num = CFNumberCreate( kCFAllocatorDefault, 
								  kCFNumberIntType, 
								  &intVal ); 
			CFDictionarySetValue( infoBlockDict, CFSTR("MusicPlugID"), num ); 
			CFRelease( num ); 
			
			DoIndentedLoggerLog(nestingDepth+1,"routing_support: 0x%02X\n",
							   (unsigned int)pBlockBytes[9]);
			intVal = pBlockBytes[9];
			num = CFNumberCreate( kCFAllocatorDefault, 
								  kCFNumberIntType, 
								  &intVal ); 
			CFDictionarySetValue( infoBlockDict, CFSTR("RoutingSupport"), num ); 
			CFRelease( num ); 
			
			DoIndentedLoggerLog(nestingDepth+1,"source:\n");
			
			DoIndentedLoggerLog(nestingDepth+1," plug_function_type: 0x%02X\n",
							   (unsigned int)pBlockBytes[10]);
			intVal = pBlockBytes[10];
			num = CFNumberCreate( kCFAllocatorDefault, 
								  kCFNumberIntType, 
								  &intVal ); 
			CFDictionarySetValue( infoBlockDict, CFSTR("SourcePlugFunctionType"), num ); 
			CFRelease( num ); 
			
			DoIndentedLoggerLog(nestingDepth+1," plug_id: 0x%02X\n",
							   (unsigned int)pBlockBytes[11]);
			intVal = pBlockBytes[11];
			num = CFNumberCreate( kCFAllocatorDefault, 
								  kCFNumberIntType, 
								  &intVal ); 
			CFDictionarySetValue( infoBlockDict, CFSTR("SourcePlugID"), num ); 
			CFRelease( num ); 
			
			DoIndentedLoggerLog(nestingDepth+1," plug_function_block_id: 0x%02X\n",
							   (unsigned int)pBlockBytes[12]);
			intVal = pBlockBytes[12];
			num = CFNumberCreate( kCFAllocatorDefault, 
								  kCFNumberIntType, 
								  &intVal ); 
			CFDictionarySetValue( infoBlockDict, CFSTR("SourcePlugFunctionBlockID"), num ); 
			CFRelease( num ); 
						
			DoIndentedLoggerLog(nestingDepth+1," stream_position: 0x%02X\n",
							   (unsigned int)pBlockBytes[13]);
			intVal = pBlockBytes[13];
			num = CFNumberCreate( kCFAllocatorDefault, 
								  kCFNumberIntType, 
								  &intVal ); 
			CFDictionarySetValue( infoBlockDict, CFSTR("SourcePlugStreamPosition"), num ); 
			CFRelease( num ); 
			
			DoIndentedLoggerLog(nestingDepth+1," stream_location: 0x%02X\n",
							   (unsigned int)pBlockBytes[14]);
			intVal = pBlockBytes[14];
			num = CFNumberCreate( kCFAllocatorDefault, 
								  kCFNumberIntType, 
								  &intVal ); 
			CFDictionarySetValue( infoBlockDict, CFSTR("SourcePlugStreamLocation"), num ); 
			CFRelease( num ); 
			
			DoIndentedLoggerLog(nestingDepth+1,"dest:\n");
			
			DoIndentedLoggerLog(nestingDepth+1," plug_function_type: 0x%02X\n",
							   (unsigned int)pBlockBytes[15]);
			intVal = pBlockBytes[15];
			num = CFNumberCreate( kCFAllocatorDefault, 
								  kCFNumberIntType, 
								  &intVal ); 
			CFDictionarySetValue( infoBlockDict, CFSTR("DestPlugFunctionType"), num ); 
			CFRelease( num ); 
			
			DoIndentedLoggerLog(nestingDepth+1," plug_id: 0x%02X\n",
							   (unsigned int)pBlockBytes[16]);
			intVal = pBlockBytes[16];
			num = CFNumberCreate( kCFAllocatorDefault, 
								  kCFNumberIntType, 
								  &intVal ); 
			CFDictionarySetValue( infoBlockDict, CFSTR("DestPlugID"), num ); 
			CFRelease( num ); 
			
			DoIndentedLoggerLog(nestingDepth+1," plug_function_block_id: 0x%02X\n",
							   (unsigned int)pBlockBytes[17]);
			intVal = pBlockBytes[17];
			num = CFNumberCreate( kCFAllocatorDefault, 
								  kCFNumberIntType, 
								  &intVal ); 
			CFDictionarySetValue( infoBlockDict, CFSTR("DestPlugFunctionBlockID"), num ); 
			CFRelease( num ); 
			
			DoIndentedLoggerLog(nestingDepth+1," stream_position: 0x%02X\n",
							   (unsigned int)pBlockBytes[18]);
			intVal = pBlockBytes[18];
			num = CFNumberCreate( kCFAllocatorDefault, 
								  kCFNumberIntType, 
								  &intVal ); 
			CFDictionarySetValue( infoBlockDict, CFSTR("DestPlugStreamPosition"), num ); 
			CFRelease( num ); 
			
			DoIndentedLoggerLog(nestingDepth+1," stream_location: 0x%02X\n",
							   (unsigned int)pBlockBytes[19]);
			intVal = pBlockBytes[19];
			num = CFNumberCreate( kCFAllocatorDefault, 
								  kCFNumberIntType, 
								  &intVal ); 
			CFDictionarySetValue( infoBlockDict, CFSTR("DestPlugStreamLocation"), num ); 
			CFRelease( num ); 
			
			break;
			
		// "Name Info Block"
		case 0x000B:
			DoIndentedLoggerLog(nestingDepth+1,"name_data_reference_type: 0x%02X\n",
							   (unsigned int)pBlockBytes[6]);
			intVal = pBlockBytes[6];
			num = CFNumberCreate( kCFAllocatorDefault, 
								  kCFNumberIntType, 
								  &intVal ); 
			CFDictionarySetValue( infoBlockDict, CFSTR("NameDataReferenceType"), num ); 
			CFRelease( num ); 
			
			if ((PrimaryFieldsLength() > 1) && (pBlockBytes[6] == 0x00))
			{
				DoIndentedLoggerLog(nestingDepth+1,"name_data_attributes: 0x%02X\n",
								   (unsigned int)pBlockBytes[7]);
				intVal = pBlockBytes[7];
				num = CFNumberCreate( kCFAllocatorDefault, 
									  kCFNumberIntType, 
									  &intVal ); 
				CFDictionarySetValue( infoBlockDict, CFSTR("NameDataAttributes"), num ); 
				CFRelease( num ); 
				
				DoIndentedLoggerLog(nestingDepth+1,"maximum_number_of_characters: %u\n",
								   (unsigned int)((pBlockBytes[8]*256)+pBlockBytes[9]));
				intVal = ((pBlockBytes[8]*256)+pBlockBytes[9]);
				num = CFNumberCreate( kCFAllocatorDefault, 
									  kCFNumberIntType, 
									  &intVal ); 
				CFDictionarySetValue( infoBlockDict, CFSTR("MaxNumCharacters"), num ); 
				CFRelease( num ); 
				
				remainingBytesInPrimaryField = PrimaryFieldsLength() - 4;
				
				// The rest of this primary field is just a sequence of info-blocks
				pNameInfoBlockNestedInfoBlockBytes = &pBlockBytes[10];
				
				if (remainingBytesInPrimaryField)
				{
					nestedArray = CFArrayCreateMutable( kCFAllocatorDefault,0,&kCFTypeArrayCallBacks);
					if (!nestedArray)
						return kIOReturnNoMemory;
				}
				else
					nestedArray = NULL;
				
				while (remainingBytesInPrimaryField > 0)
				{
					pNestedAVCInfoBlock = new AVCInfoBlock(pNameInfoBlockNestedInfoBlockBytes,pLogger);
					pNestedAVCInfoBlock->ParseInfoBlock(nestingDepth+1,nestedArray);
					
					remainingBytesInPrimaryField -= (pNestedAVCInfoBlock->CompoundLength()+2);
					pNameInfoBlockNestedInfoBlockBytes += (pNestedAVCInfoBlock->CompoundLength()+2);
					delete pNestedAVCInfoBlock;
				}
				
				if (nestedArray)
				{
					CFDictionarySetValue( infoBlockDict, CFSTR("NestedInfoBlocks"), nestedArray ); 
					CFRelease(nestedArray);
				}
			}
			break;
			
		// "Raw Text Info Block"
		case 0x000A:
			DoIndentedLoggerLog(nestingDepth+1,"raw_text: ");
			
			CFMutableStringRef rawTextString = CFStringCreateMutable(kCFAllocatorDefault,0);
			if (!rawTextString)
				return kIOReturnNoMemory;
			
			for (i=0;i<PrimaryFieldsLength();i++)
			{
				char c = ((pBlockBytes[6+i] >= 32) && (pBlockBytes[6+i] <= 126)) ? pBlockBytes[6+i] : ' ';
				DoLoggerLog("%c",c);
				CFStringAppendFormat (rawTextString,NULL,CFSTR("%c"),c);
			}
			DoLoggerLog("\n");

			CFDictionarySetValue( infoBlockDict, CFSTR("RawText"), rawTextString ); 
			CFRelease(rawTextString);
			break;
			
		// "Unknown"
		default:
			break;
	};
	
	// If we have nested info blocks, create an array to hold them, then recursively parse them
	if (nestedInfoBlockList.size())
	{
		nestedArray = CFArrayCreateMutable( kCFAllocatorDefault,0,&kCFTypeArrayCallBacks);
		if (!nestedArray)
			return kIOReturnNoMemory;
		
		for (i=0;i<nestedInfoBlockList.size();i++)
		{
			pNestedAVCInfoBlock = nestedInfoBlockList.at(i);
			pNestedAVCInfoBlock->ParseInfoBlock(nestingDepth+1,nestedArray);
		}
	
		CFDictionarySetValue( infoBlockDict, CFSTR("NestedInfoBlocks"), nestedArray ); 
		CFRelease(nestedArray);
	}
	
	DoIndentedLoggerLog(nestingDepth,"}\n");
	
	if (array)
		CFArrayAppendValue(array,infoBlockDict);
	CFRelease(infoBlockDict);
	
	return kIOReturnSuccess;
}


} // namespace AVS	
