/*
 * Copyright 1991-1998 by Open Software Foundation, Inc. 
 *              All Rights Reserved 
 *  
 * Permission to use, copy, modify, and distribute this software and 
 * its documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appears in all copies and 
 * that both the copyright notice and this permission notice appear in 
 * supporting documentation. 
 *  
 * OSF DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE. 
 *  
 * IN NO EVENT SHALL OSF BE LIABLE FOR ANY SPECIAL, INDIRECT, OR 
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT, 
 * NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION 
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
 * 
 */
/*
 * MkLinux
 */

/*
 *   AWACs - Audio Waveform Amplifier and Converter
 *
 *   Notes:
 *
 *   1. All values written to (or read from) hardware must be endian reversed
 *      when using the constants from this file.
 *   2. The hardware uses a serial-bus time multiplexed scheme to communicate
 *      with up to four addressable devices (subframes). AWACs currently only uses 
 *      'SubFrame 0'.  When a register field refers to a  subframe, then the SubFrame0 
 *      value  should be used.
 *   3. The CodecControl is used to as a pointer/data reg to access four 12-bit
 *      registers. The kCodecCtlAddress* values indicate the 12-bit register to be
 *      written and the kCodecCtlDataMask indicates the 12-bits to be written. 
 *      kCodecCtlBusy is set/cleared by hardware when a pending value waiting to be
 *      written to one of the 12-bit registers. kCodecCtlEMSelect* indicates which
 *      SubFrame the data will go out on. (See Note 2)
 *  4.  The SoundControl is used to set the data rate and indicates the input,
 *      output and status subframes. (See Note 2).
 *  5.  The CodecStatus indicate when there was an error, or a change of input
 *      or output sources, i.e. a mic, line, or headphones were plugged in.
 *  6.  The ClippingCount  is incremented on a left/right channel overflow. It
 *      is reset by reading its contents.
 *
 */

/*----------------------------- AWACs register mapping structure -------------------------*/

struct _awacs_regmap_t
{
	volatile uchar_t		       	CodecControl[3];
	uchar_t					pad0;
	volatile uchar_t	       		CodecStatus[3];
	uchar_t					pad1;
	volatile ushort_t	       		BufferSize;
	ushort_t				pad2;
	volatile ulong_t	       		Phase;
	volatile uchar_t		       	SoundControl[2];
	ushort_t				pad3;
	volatile uchar_t		       	DMAIn;
	uchar_t					pad4[3];
	volatile uchar_t		       	DMAOut;
	uchar_t					pad5[3];
};
typedef  struct _awacs_regmap_t awacs_regmap_t;


/*------------------------ AWACs CODEC Control register constants -----------------------*/

enum AWACsCODEC_ControlRegisterGeneralConstants 
{
	kCodecCtlDataMask	=	0x00000FFF,	        /*CODEC control data bits*/

	kCodecCtlAddress0	=	0x00000000,		/*Select address 0*/
	kCodecCtlAddress1	=	0x00001000,		/*Select address 1*/
	kCodecCtlAddress2	=	0x00002000,		/*Select address 2*/
	kCodecCtlAddress4	=	0x00004000,		/*Select address 4*/
	
	kCodecCtlEMSelect0	=	0x00000000,		/*Select subframe 0*/

	kCodecCtlBusy		=	0x00400000, 		/*AWACS busy bit */    
	kCommand		=	0x00400000,		/*Write to command [register] */
	kExpand			=	0x00800000		/*AWACS mode (not Singer) */
};

enum AWACsCODEC_ControlRegister0Constants 
{
	kLeftInputGainMask		=	0x000000F0,	/*Bits used for left input gain*/
	kRightInputGainMask		=	0x0000000F,	/*Bits used for right input gain*/
	
	kDefaultMicGain			=	0x000000CC,	/*Default right & left gain for a mic*/
	kDefaultCDGain			=	0x000000BB,	/*Default right & left gain for a CD*/
	
	kNotMicPreamp			=	0x00000100,	/*Select preamp on or off*/
	
	kInputMuxMask			=	0x00000E00,	/*Bits used to set which input to use */
	kCDInput		       	=	0x00000200,	/*Bit 9 = CD ROM audio input*/
	kMicInput		       	=	0x00000400,	/*Bit 10 = external mic input*/
	kUnusedInput			=	0x00000800,	/*Bit 11 = unused input*/
	kInitialAwacsR0Value    	=	0x00000000	/*Initial value set on boot*/
};


enum AWACsCODEC_ControlRegister1Constants 
{
	kReservedR1Bit0			=	0x00000001,	/*Reserved - always set to zero*/
	kReservedR1Bit1			=	0x00000002,	/*Reserved - always set to zero*/
	kRecalibrate			=	0x00000004,	/*Recalibrate AWACs*/
	kExtraSampleRateMask    	=	0x00000038,	/*!!!Do these bits do anything???*/
	kLoopThruEnable			=	0x00000040,	/*Loop thru enable*/
	kMuteInternalSpeaker	        =	0x00000080,	/*Internal speaker mute*/
	kReservedR1Bit8			=	0x00000100,	/*Reserved - always set to zero*/
	kMuteHeadphone			=	0x00000200,	/*Headphone mute*/
	kParallelOutMask		=	0x0000C000,	/*!!!What do these bits do???*/
	kInitialAwacsR1Value     	=	0x00000000	/*Initial value set on boot*/
};


enum AWACsCODEC_ControlRegister2Constants 
{
	kRightHeadphoneAttenMask	=	0x0000000F,	/*Attenuation for right headphone speaker*/
	kRightHeadphoneAttenShift	=	0,		/*Bit shift from LSB position*/
	kReservedR2Bit4			=	0x00000010,	/*Reserved - always set to zero*/
	kReservedR2Bit5			=	0x00000020,	/*Reserved - always set to zero*/
	kLefttHeadphoneAttenMask	=	0x000003C0,	/*Attenuation for left headphone speaker*/
	kLeftHeadphoneAttenShift	=	6,		/*Bit shift from LSB position*/
	kReservedR2Bit10		=	0x00000400,	/*Reserved - always set to zero*/
	kReservedR2Bit11		=	0x00000800,	/*Reserved - always set to zero*/
	kHeadphoneAttenMask		=	(kRightHeadphoneAttenMask | kLefttHeadphoneAttenMask),
	kInitialAwacsR2Value	       	=	0x00000000	/*Initial value set on boot*/
};


enum AWACsCODEC_ControlRegister4Constants 
{
	kRightSpeakerAttenMask		=	0x0000000F,	/*Attenuation for right internal speaker*/
	kRightSpeakerAttenShift		=	0,	       	/*Bit shift from LSB position*/
	kReservedR4Bit4			=	0x00000010,	/*Reserved - always set to zero*/
	kReservedR4Bit5			=	0x00000020,	/*Reserved - always set to zero*/
	kLeftSpeakerAttenMask		=	0x000003C0,	/*Attenuation for left internal speaker*/
	kLeftSpeakerAttenShift		=	6,	       	/*Bit shift from LSB position*/
	kReservedR4Bit10		=	0x00000400,	/*Reserved - always set to zero*/
	kReservedR4Bit11		=	0x00000800,	/*Reserved - always set to zero*/
	kSpeakerAttenMask		=	(kRightSpeakerAttenMask | kLeftSpeakerAttenMask),
	kInitialAwacsR4Value		=	0x00000000	/*Initial value set on boot*/
};

/*----------------- AWACs CODEC Status Register constants -------------------------------*/

enum AWACsCODEC_StatusRegisterConstants 
{
	kNotMicSense	        	=	0x00000001,	/*This bit is 0 when a mic(input) is plugged in*/
	kLineInSense	          	=	0x00000002,	/*This bit is 1, when a line(input) is pluggen in*/
	kHeadphoneSense	        	=	0x00000008,	/*This bit is 1 when headphone is plugged in*/
	kManufacturerIDMask	        =	0x00000F00,	/*AWACS chip manufacturer ID bits*/
	kManufacturerIDShft	        =	8,	       	/*Bits to shift right to get ID in LSB position*/
	kRevisionNumberMask	        =	0x0000F000,	/*AWACS chip revision bits*/
	kRevisionNumberShft     	=	12,	       	/*Bits to shift right to get rev in LSB position*/
	kAwacsErrorStatus	      	=	0x000F0000,	/*AWACS error status bits*/
	kOverflowRight                  =       0x00100000,
	kOverflowLeft                   =       0x00200000,
	kValidData                      =       0x00400000,
	kExtend                         =       0x00800000
};

/*--------------------- AWACs Sound Control register ------------------------------------*/
	
enum AWACsSoundControlRegisterConstants 
{
	kInSubFrameMask			=	0x00000003,	/*All of the input subframe bits*/
	kInSubFrame0			=	0x00000000,	/*Input subframe 0*/
	kInSubFrame1			=	0x00000001,	/*Input subframe 1*/
	kInSubFrame2			=	0x00000002,	/*Input subframe 2*/
	kInSubFrame3			=	0x00000003,	/*Input subframe 3*/
	
	kOutSubFrameMask		= 	0x0000003C, 	/*All of the output subframe bits*/
	kOutSubFrame0			=	0x00000004,	/*Output subframe 0*/
	kOutSubFrame1			=	0x00000008,	/*Output subframe 1*/
	kOutSubFrame2			=	0x00000010,	/*Output subframe 2*/
	kOutSubFrame3			=	0x00000020, 	/*Output subframe 3*/
	
	kHWRateMask	       		=	0x00000600,	/*All of the hardware sampling rate bits*/
	kHWRate44100			=	0x00000400,	/*Hardware sampling bits for 44100 Hz*/
	kHWRate29400			=	0x00000200,	/*Hardware sampling bits for 29400 Hz*/
	kHWRate22050			=	0x00000000,	/*Hardware sampling bits for 22050 Hz*/

	kHWRateShift                    =       9,

	kDMAOutEnable			=	0x00000100	/* Output DMA enabled */
};

/*--------------------- AWACs Clipping Count register ------------------------------------*/

enum AWACsClippingCountRegisterConstants 
{
       kRightClippingCount              =       0x000000FF,
       kLeftClippingCount               =       0x0000FF00
};

enum AWACSDMAOutControlRegisterConstants
{
	DMA_IF0	= 0x80,
	DMA_IF1	= 0x40,
	DMA_ERR	= 0x20,
	DMA_STAT= 0x10,
	DMA_IE0	= 0x08,
	DMA_IE1 = 0x04,
	DMA_IER = 0x02
};
