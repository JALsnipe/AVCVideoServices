/*
	File:		SimpleVirtualMPEGTapePlayer.h
 
 Synopsis: This is the header file for the SimpleVirtualMPEGTapePlayer class.
 
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

//////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// SimpleVirtualMPEGTapePlayer - This class is an example of one simple way to use both a VirtualTapeSubunit
// object and a MPEG2Transmitter object together to create a playback only VirtualDVHS. For simplicity,
// transport states are limited to either wind/stop (when no p2p connection exist), and play/fwd (when a p2p
// connection exists). All AV/C transport control commands are rejected. The client just provides the name 
// of the file, and this object does the rest, with no client involvement needed.
//
// Notes:
//
//  1) Client creates a SimpleVirtualMPEGTapePlayer object.
//
//  2) Client calls initWithFileName(...) on the new object, with the name of the source MPEG2-TS file.
//
//	3) Mac then emulates an VirtualDVHS type AV/C device. 
//      Note: This device currently rejects all transport control commands, and doesn't increment time-code.
//
//  4) When an external device (TV, STB) makes a p2p connection to the VirtualDVHS's isoch output plug, a
//     MPEG2Transmitter object will be created and start transmitting on the designated isoch channel
//     from the source file.
//
//  5) If an external device (TV, STB) disconnects from the VirtualDVHS's isoch output plug, the
//     MPEG2Transmitter object will be stopped and deleted.
//
//  6) The client has access to a few parameters that it can poll to deteremine if a p2p connection
//     exists and if the entire file has been transmitted.
//
//  7) When the transmitter has consumed the entire file, transmitDone will be set to true, and the 
//     transmitter will continue to send CIP only packets while the p2p connection (and this object)
//     exists.
//
//  8) The client can call setLoopModeState to enable a loop playback mode for the input file.
//     The file will be looped as long as no file i/o error occurs. If one does, loopMode will
//     be disabled, and the transmit completed.
// 
//  9) The client can call restartTransmit to rewind to the beginning of the transmit file. If
//     transmitDone is already true, it will be set to false to restart transmission.
//
// 10) Deleting the SimpleVirtualMPEGTapePlayer object deletes the VirtualTapeSubunit object and any
//     MPEG2Transmitter object, and closes the input file.
//
// 11) This class generates a pretty accurate time-code by partially demuxing, with an instance of a
//     TSDemuxer object, to count frame-starts.
//
// 12) TODO: This class could provide the client more file pointer manipulation than just restartTransmit
//
// 13) TODO: This class should allow for AV/C transport control for at least play-pause, 
//     play-normal, and stop states.
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////

using namespace AVS;

class SimpleVirtualMPEGTapePlayer
{
	
public:
	SimpleVirtualMPEGTapePlayer();
	~SimpleVirtualMPEGTapePlayer();
	
	// This object must be initialized before it does anything useful
	IOReturn initWithFileName(char* pMpegTSFileName, AVCDevice *pAVCDevice = nil);
	
	// This function enables the client to enable or disable looping mode
	void setLoopModeState(bool enableLoopMode);
	
	// This function allows the client to rewind to the beginning of the file
	// Note: If transmitDone is already true, it will be set to false.
	void  restartTransmit(void);
	
	// This function enables the client to get access to the virtual tape 
	// subunit's current time-code frame-count
	UInt32 getTapeSubunitTimeCodeFrameCount();
	
	// This function enables the client to get access to the virtual tape 
	// subunit's current time-code in hours, minutes, seconds, and frames
	void getTapeSubunitTimeCodeFrameCountInHMSF(UInt32 *pHours, UInt32 *pMinutes, UInt32 *pSeconds, UInt32 *pFrames);
	
	// Client can poll this value to determine how many packets have been fetched
	// by the transmitter
	unsigned int packetsConsumed;
	
	// Client can poll this value to determine if the entire file has been transmitted
	bool transmitDone;
	
	// Client can poll this value to determine if the sink device (the TV) has made a
	// p2p connection to the Mac's isoch output plug used by this virtual tape device
	bool p2pConnectionExists;
		
private:
		
	// Callbacks from tape subunit object
	static IOReturn MyVirtualTapeTransportStateChangeHandler(void *pRefCon, UInt8 newTransportMode, UInt8 newTransportState); 
	static IOReturn MyVirtualTapeSignalModeChangeHandler(void *pRefCon, UInt8 newSignalMode);
	static IOReturn MyVirtualTapeTimeCodeRepositionHandler(void *pRefCon, UInt32 newTimeCode);
	static void MyVirtualTapeCMPConnectionHandler(void *pRefCon, bool isInputPlug, UInt8 isochChannel, UInt8 isochSpeed, UInt8 p2pCount);
	
	// Callbacks for mpeg transmitter object
	static void MessageReceivedProc(UInt32 msg, UInt32 param1, UInt32 param2, void *pRefCon);
	static IOReturn MpegTransmitCallback(UInt32 **ppBuf, bool *pDiscontinuityFlag, void *pRefCon);
	
	// Callback for TS demuxer object (for frame detection)
	static IOReturn PESCallback(TSDemuxerMessage msg, PESPacketBuf* pPESPacket,void *pRefCon);
	
	FILE *inFile;
	char tsPacketBuf[kMPEG2TSPacketSize];
	MPEG2Transmitter *pTransmitter;
	VirtualTapeSubunit *pTapeSubunit;
	TSDemuxer *pTSDemuxer;
	bool loopMode;
	bool flushMode;
	unsigned int flushCnt;

};
