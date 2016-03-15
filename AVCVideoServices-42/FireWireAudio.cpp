/*
	File:		FireWireAudio.cpp
 
 Synopsis: This is the top source file for the FireWireAudio class.
 
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

#include "FWAUserLib/AppleFWAudioUserLib.h"
#include "FireWireAudio.h"

namespace AVS
{

//////////////////////////////////////////////////////////////////////////////
// FireWireAudio::FireWireAudio
//////////////////////////////////////////////////////////////////////////////
FireWireAudio::FireWireAudio(UInt32 numAudioOutputs,
								 UInt32 numAudioInputs,
								 bool hasMIDIOutput,
								 bool hasMIDIInput)
{
	UInt32 i;
	
	receiverStarted = false;
	transmitterStarted = false;

	// Set the initial sample rate
	deviceSampleRate = 48000;
	subunitSampleRate = 2;
	
	if (numAudioInputs < kMaxAudioStreams)
		inputAudioChannels = numAudioInputs;
	else
		inputAudioChannels = kMaxAudioStreams;
		
	if (numAudioOutputs < kMaxAudioStreams)
		outputAudioChannels = numAudioOutputs;
	else
		outputAudioChannels = kMaxAudioStreams;

	inputMIDISequences  = hasMIDIInput;
	outputMIDISequences = hasMIDIOutput;
	
	_FWARef = nil;
	deviceRef = nil;
	audioEngine = nil;
	
	outputIsochStreamRef = nil;
	inputIsochStreamRef = nil;
	outMIDIStreamRef = nil;
	inMIDIStreamRef = nil;
	outMIDIPlugRef = nil;
	inMIDIPlugRef = nil;
	for (i=0;i<kMaxAudioStreams;i++)
		outputStreamRefs[i] = nil;
	for (i=0;i<kMaxAudioStreams;i++)
		inputStreamRefs[i] = nil;
}

//////////////////////////////////////////////////////////////////////////////
// FireWireAudio::~FireWireAudio
//////////////////////////////////////////////////////////////////////////////
FireWireAudio::~FireWireAudio()
{
	OSStatus result;

	if (transmitterStarted == true)
		StopFireWireAudioTransmitter();
	
	if (receiverStarted == true)
		StopFireWireAudioReceiver();
	
	// Dispose of the FireWireAudio engine
	if (audioEngine)
		result = FWADisposeFWAudioEngine(_FWARef, audioEngine);
	
	// Dispose of the FireWireAudio device
	if (deviceRef)
		result =  FWADisposeFWAudioDevice(_FWARef,deviceRef);	
	
	// Close the FireWireAudio user-client
	if (_FWARef	)
		FWAClose(_FWARef);
}

//////////////////////////////////////////////////////////////////////////////
// FireWireAudio::setupFireWireAudio(void)
//////////////////////////////////////////////////////////////////////////////
IOReturn FireWireAudio::setupFireWireAudio(void)
{
	IOReturn result ;
	char s[64] = {0};
	UInt32 vendorID;
	UInt64 guid;
	
	// Open the FireWireAudio user client
	result = FWAOpenLocal(&_FWARef);
	if (result != kIOReturnSuccess )
		return result;
	
	// Get the Vendor ID
	result = FWAGetVendorID(_FWARef,&vendorID);
	if (result != kIOReturnSuccess )
		return result;
	
	// Get the local GUID, and create a string representing the GUID
	result = FWAGetMacGUID(_FWARef,&guid);
	if (result != kIOReturnSuccess )
		return result;
	sprintf(s,"%llx",guid);
	
	// Create the core-audio device
	result =  FWACreateFWAudioDevice(_FWARef,"Macintosh", vendorID, s, &deviceRef);
	if (result != kIOReturnSuccess )
		return result;

	// Create the core-audio engine
	result = FWACreateFWAudioEngine(_FWARef, deviceRef, (inputAudioChannels != 0), (outputAudioChannels != 0), &audioEngine);
		
	return kIOReturnSuccess;
}

//////////////////////////////////////////////////////
// FireWireAudio::SetAudioSampleRate
//////////////////////////////////////////////////////
void FireWireAudio::SetAudioSampleRate(MusicSubunitSampleRate sampleRate)
{
	subunitSampleRate = sampleRate;
	deviceSampleRate = MusicSubunitSampleRateToCoreAudioSampleRate(sampleRate);

#if 0
	
	// TODO: This algorithm for changing the sample rate on an existing
	// core-audio engine doesn't seem to work, so I've moved the functionality
	// to the app level (where the entire FireWireAudio object is deleted, and 
	// a new one is created
	
	// If we're transmitting or receiving, change the core-audio sample rate
	if (receiverStarted || transmitterStarted)
	{
		// Stop the audio device, set the new sample rate, then restart the audio device
		FWAStopFWAudioDevice(_FWARef,deviceRef);
		FWASetIsochStreamSampleRate(_FWARef,outputIsochStreamRef,deviceSampleRate);
		FWASetIsochStreamSampleRate(_FWARef,inputIsochStreamRef,deviceSampleRate);
		FWAStartFWAudioDevice(_FWARef,deviceRef);
	}
#endif	
}

//////////////////////////////////////////////////////
// FireWireAudio::MusicSubunitSampleRateToCoreAudioSampleRate
//////////////////////////////////////////////////////
UInt32 FireWireAudio::MusicSubunitSampleRateToCoreAudioSampleRate(MusicSubunitSampleRate sampleRate)
{
	UInt32 returnVal;
	
	switch (sampleRate)
	{
		case kMusicSubunitSampleRate_32000: returnVal = 32000; break;
		case kMusicSubunitSampleRate_44100: returnVal = 32000; break;
		case kMusicSubunitSampleRate_88200: returnVal = 88200; break;
		case kMusicSubunitSampleRate_96000: returnVal = 96000; break;
		case kMusicSubunitSampleRate_176400: returnVal = 176400; break;
		case kMusicSubunitSampleRate_192000: returnVal = 192000; break;
		case kMusicSubunitSampleRate_48000:
		default: returnVal = 48000; break;
	};

	return returnVal;
}

//////////////////////////////////////////////////////
// FireWireAudio::StartFireWireAudioTransmitter
//////////////////////////////////////////////////////
void FireWireAudio::StartFireWireAudioTransmitter(MusicSubunitSampleRate sampleRate, UInt8 isochChannel, IOFWSpeed isochSpeed )
{
	OSStatus result;
	UInt32 index =0;
	char name[32] = {0};
	char ident[64] = {0};
	UInt32 plugs;
	
	// Don't do this if we're already started
	if (transmitterStarted == true)
		return;
	
	printf("+StartFireWireAudioTransmitter: SampleRate=%d,  Isoch-Channel=%d,  IsochSpeed=%d\n", sampleRate, isochChannel,isochSpeed);
	
	// Create the isoch output stream
	result =  FWACreateIsochStream(_FWARef,isochChannel,kFWAStreamOut,outputAudioChannels,outputMIDISequences,&outputIsochStreamRef);
	if (result == kIOReturnSuccess)
	{
		// Set the sample rate for the isoch stream
		result = FWASetIsochStreamSampleRate(_FWARef,outputIsochStreamRef,deviceSampleRate);
		
		// Create a stream for each audio signal in the isoch output stream
		for (index = 0; index < outputAudioChannels; index++)
		{
			// Create a unique name for this audio stream
			sprintf(name,"Audio %lu",index+1);
			sprintf(ident,"ident %lu",index+1);
			result = FWACreateFWAudioStream(_FWARef, outputIsochStreamRef, index,kFWAStreamOut,1,name,(UInt8*)ident, &outputStreamRefs[index]);
		}
		
		// assume one MIDI
		if (outputMIDISequences)
		{
			// Create the MIDI stream object (which can hold up to 8 MIDI "plugs")
			result =  FWACreateFWAudioMIDIStream(_FWARef, outputIsochStreamRef, index,kFWAStreamOut,&outMIDIStreamRef);
			if (kIOReturnSuccess == result)
			{
				// Name the MIDI out
				index = 0;
				sprintf(name,"out %lu",index);
				
				// Create one MIDI plug for this MIDI stream
				result =  FWACreateFWAudioMIDIPlug(_FWARef, outMIDIStreamRef, 0,name,(UInt8*)name,&outMIDIPlugRef);
			}
			
			// For debugging, print the number of MIDI plugs
			result = FWAGetNumMIDIOutputPlugs(_FWARef, &plugs);
			printf("we have %lu output MIDI Plugs\n",plugs);
		}
	}
	else
	{
		printf("FWACreateIsochStream Error:0x%08X\n",(int)result);
	}
	
	// Set the flag signifying that the transmitter has been started
	transmitterStarted = true;
	
	// Only start the core-audio device if both input and output have been started
	if ( receiverStarted && transmitterStarted)
	{
		printf("starting device\n");
		FWAStartFWAudioDevice(_FWARef,deviceRef);
	}
	printf("-StartFireWireAudioTransmitter \n");
}

//////////////////////////////////////////////////////
// FireWireAudio::StopFireWireAudioTransmitter
//////////////////////////////////////////////////////
void FireWireAudio::StopFireWireAudioTransmitter(void)
{
	printf("StopFireWireAudioTransmitter\n");
	
	// Don't do this if we're already stopped
	if (transmitterStarted == false)
		return;
	
	// Clear the flag signifying that the transmitter has been started
	transmitterStarted = false;
	
	// Only tear-down the core-audio device if both input and output have been stopped
	if ( !receiverStarted && !transmitterStarted)
	{
		printf("Tear down core-audio device\n");
		FWAStopFWAudioDevice(_FWARef,deviceRef);
		CleanUpDriverResources();
	}
}

//////////////////////////////////////////////////////
// FireWireAudio::StartFireWireAudioReceiver
//////////////////////////////////////////////////////
void FireWireAudio::StartFireWireAudioReceiver(MusicSubunitSampleRate sampleRate, UInt8 isochChannel, IOFWSpeed isochSpeed )
{
	OSStatus result;
	UInt32 index;
	char name[32] = {0};
	UInt32 plugs;
	
	// Don't do this if we're already started
	if (receiverStarted == true)
		return;
	
	printf("+StartFireWireAudioReceiver: SampleRate=%d,  Isoch-Channel=%d,  IsochSpeed=%d\n", sampleRate, isochChannel,isochSpeed);
	
	// Create the isoch input stream
	result = FWACreateIsochStream(_FWARef,isochChannel,kFWAStreamIn,inputAudioChannels,inputMIDISequences, &inputIsochStreamRef);
	if (kIOReturnSuccess == result)
	{
		// Set the sample rate for the isoch stream
		result = FWASetIsochStreamSampleRate(_FWARef,inputIsochStreamRef,deviceSampleRate);
		
		// Set the clock source for the virutal core-audio device to sync to this input stream
		// TODO: This seems to cause problems, so I've removed it for now 
		//result = FWASetClockSource(_FWARef, inputIsochStreamRef,0);
		
		// Create a stream for each audio signal in the isoch input stream
		for (index = 0 ; index < inputAudioChannels; ++index)
		{	
			// Create a unique name for this audio stream
			sprintf(name,"in %lu",index);					
			result =  FWACreateFWAudioStream(_FWARef, inputIsochStreamRef, index,kFWAStreamIn,1,name,(UInt8*)name,&inputStreamRefs[index]);
		}
		
		// assume one MIDI
		if (inputMIDISequences)
		{
			// Create the MIDI stream object (which can hold up to 8 MIDI "plugs")
			result = FWACreateFWAudioMIDIStream(_FWARef, inputIsochStreamRef, index,kFWAStreamIn,&inMIDIStreamRef);
			if (kIOReturnSuccess == result)
			{
				// Name the MIDI out
				index = 0;
				sprintf(name,"in %lu",index);
				
				// Create one MIDI plug for this MIDI stream
				result =  FWACreateFWAudioMIDIPlug(_FWARef, inMIDIStreamRef, 0,name,(UInt8*)name,&inMIDIPlugRef);
			}				
			
			// For debugging, print the number of MIDI plugs
			result = FWAGetNumMIDIInputPlugs(_FWARef, &plugs);
			printf("we have %lu input MIDI Plugs\n",plugs);
		}
	}
	else
	{
		printf("FWACreateIsochStream Error:0x%08X\n",(int)result);
	}
	
	// Set the flag signifying that the receiver has been started
	receiverStarted = true;
	
	// Only start the core-audio device if both input and output have been started
	if ( receiverStarted && transmitterStarted)
	{
		printf("starting device\n");
		FWAStartFWAudioDevice(_FWARef,deviceRef);
	}
	
	printf("-StartFireWireAudioReceiver \n");
}

//////////////////////////////////////////////////////
// FireWireAudio::StopFireWireAudioReceiver
//////////////////////////////////////////////////////
void FireWireAudio::StopFireWireAudioReceiver(void)
{
	printf("StopFireWireAudioReceiver\n");
	
	// Don't do this if we're already stopped
	if (receiverStarted == false)
		return;
	
	// Clear the flag signifying that the receiver has been started
	receiverStarted = false; 
	
	// Only tear-down the core-audio device if both input and output have been stopped
	if ( !receiverStarted && !transmitterStarted)
	{
		printf("Tear down core-audio device\n");
		FWAStopFWAudioDevice(_FWARef,deviceRef);
		CleanUpDriverResources();
	}
}

//////////////////////////////////////////////////////////////////////////////
// FireWireAudio::CleanUpDriverResources(void)
//////////////////////////////////////////////////////////////////////////////
void  FireWireAudio::CleanUpDriverResources()
{
	printf("CleanUpDriverResources \n");
	
	OSStatus result;
	UInt32 index;
	
	// Dispose of the input audio signals
	for (index = 0 ; index < inputAudioChannels; ++index)
	{
		if (inputStreamRefs[index])
		{
			printf ("disposing inputStreamRefs[%d] = %p\n",(int)index,inputStreamRefs[index]);
			result =  FWADisposeFWAudioStream(_FWARef,inputStreamRefs[index]);
			inputStreamRefs[index] = NULL;
		}
	}
	
	// Dispose of the output audio signals
	for (index = 0 ; index < outputAudioChannels; ++index)
	{
		if (outputStreamRefs[index])
		{
			printf ("disposing outputStreamRefs[%d] = %p\n",(int)index,outputStreamRefs[index]);
			result =  FWADisposeFWAudioStream(_FWARef,outputStreamRefs[index]);
			outputStreamRefs[index] = NULL;
		}
	}
	
	// Dispose of the MIDI out plug
	if (outMIDIPlugRef)
	{
		printf ("disposing outMIDIPlugRef = %p\n",outMIDIPlugRef);
		
		result = FWADisposeFWAudioMIDIPlug(_FWARef,outMIDIPlugRef );
		outMIDIPlugRef = NULL;
	}
	
	// Dispose of the MIDI out stream
	if (outMIDIStreamRef)
	{
		printf ("disposing outMIDIStreamRef = %p\n",outMIDIStreamRef);
		result = FWADisposeFWAudioMIDIStream(_FWARef,outMIDIStreamRef );
		outMIDIStreamRef = NULL;
	}
	
	// Dispose of the MIDI in plug
	if (inMIDIPlugRef)
	{		
		printf ("disposing inMIDIPlugRef = %p\n",inMIDIPlugRef);
		result = FWADisposeFWAudioMIDIPlug(_FWARef,inMIDIPlugRef );
		inMIDIPlugRef = NULL;
	}
	
	// Dispose of the MIDI in stream
	if (inMIDIStreamRef )
	{
		printf ("disposing inMIDIStreamRef = %p\n",inMIDIStreamRef);
		result = FWADisposeFWAudioMIDIStream(_FWARef,inMIDIStreamRef );
		inMIDIStreamRef = NULL;
	}
	
	// Dispose of the ouput isoch stream
	if (outputIsochStreamRef)
	{
		printf ("disposing outputIsochStreamRef = %p\n",outputIsochStreamRef);
		result = FWADisposeIsochStream(_FWARef,outputIsochStreamRef );
		outputIsochStreamRef = NULL;
	}
	
	// Dispose of the input isoch stream
	if (inputIsochStreamRef)
	{
		printf ("disposing inputIsochStreamRef = %p\n",inputIsochStreamRef);
		result = FWADisposeIsochStream(_FWARef,inputIsochStreamRef );
		inputIsochStreamRef = NULL;
	}
}

} // namespace AVS
	