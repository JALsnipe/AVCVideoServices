/*
	File:		FireWireAudio.h
 
 Synopsis: This is the top level header file for the FireWireAudio class.
 
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

#ifndef __AVCVIDEOSERVICES_FIREWIREAUDIO__
#define __AVCVIDEOSERVICES_FIREWIREAUDIO__

namespace AVS
{

#define kMaxAudioStreams 64
	
/////////////////////////////////////////
//
// FireWireAudio Class definition
//
/////////////////////////////////////////
class FireWireAudio
{
	
public:
	// Constructor
	FireWireAudio(UInt32 numAudioOutputs = 6,
				  UInt32 numAudioInputs = 6,
				  bool hasMIDIOutput = true,
				  bool hasMIDIInput = true);
	
	// Destructor
	~FireWireAudio();
	
	// Setup funcion that finds the first available local node
    IOReturn setupFireWireAudio(void);

	// Stream start/stop routines
	void StartFireWireAudioTransmitter(MusicSubunitSampleRate sampleRate, UInt8 isochChannel, IOFWSpeed isochSpeed );
	void StopFireWireAudioTransmitter(void);
	void StartFireWireAudioReceiver(MusicSubunitSampleRate sampleRate, UInt8 isochChannel, IOFWSpeed isochSpeed );
	void StopFireWireAudioReceiver(void);

	// Change the sample rate
	void SetAudioSampleRate(MusicSubunitSampleRate sampleRate);
	
private:	
		
	void CleanUpDriverResources();

	UInt32 MusicSubunitSampleRateToCoreAudioSampleRate(MusicSubunitSampleRate sampleRate);

	MusicSubunitSampleRate subunitSampleRate;
		
	UInt32 deviceSampleRate;
	UInt32 inputAudioChannels;
	UInt32 outputAudioChannels;
	UInt32 inputMIDISequences;
	UInt32 outputMIDISequences;
	FWARef _FWARef;
	FWADeviceRef deviceRef;
	FWAEngineRef audioEngine;
	
	FWAIsochStreamRef outputIsochStreamRef;
	FWAIsochStreamRef inputIsochStreamRef;
	FWAAudioStreamRef outputStreamRefs[kMaxAudioStreams];
	FWAAudioStreamRef inputStreamRefs[kMaxAudioStreams];
	FWAMIDIStreamRef outMIDIStreamRef;
	FWAMIDIStreamRef inMIDIStreamRef;
	FWAMIDIPlugRef outMIDIPlugRef;
	FWAMIDIPlugRef inMIDIPlugRef;
	
	bool receiverStarted;
	bool transmitterStarted;
	
};	

} // namespace AVS

#endif // __AVCVIDEOSERVICES_FIREWIREAUDIO__