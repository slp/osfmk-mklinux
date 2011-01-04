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
 */
/*
 * Copyright 1991-1998 by Apple Computer, Inc. 
 *              All Rights Reserved 
 *  
 * Permission to use, copy, modify, and distribute this software and 
 * its documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appears in all copies and 
 * that both the copyright notice and this permission notice appear in 
 * supporting documentation. 
 *  
 * APPLE COMPUTER DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE. 
 *  
 * IN NO EVENT SHALL APPLE COMPUTER BE LIABLE FOR ANY SPECIAL, INDIRECT, OR 
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT, 
 * NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION 
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
 */
/*
 * MkLinux
 */

#ifndef _SYS_TYPES_H_
#include <types.h>

//----------------------------
//	Command dispatching
//----------------------------

enum {
		kPluginInitialize			= 0,
		kPluginCommand				= 1
};




//----------------------------
//	PMUStatus
//----------------------------

typedef	int			PMUStatus;
typedef uchar_t		UInt8;
typedef signed char	SInt8;
typedef ushort_t	UInt16;
typedef ulong_t		UInt32;
typedef boolean_t	Boolean;

enum {
		kPMUNoError					= 0,
		kPMUInitError				= 1,	// Failed to initialize library
		kPMUParameterError			= 2,	// NULL/malformed PMURequest block
		kPMURequestError			= 3,	// PMUStartIO before ClientInit
		kPMUIOError					= 4,	// Generic I/O failure
		kPMUsendEndErr				= 5,	//
		kPMUsendStartErr			= 6,	//
		kPMUrecvEndErr				= 7,	//
		kPMUrecvStartErr			= 8,	//
		kPMUPacketCommandError		= 9,	// Packet Cmd invalid
		kPMUPacketSizeError			= 10	// Packet Size invalid
};


//----------------------------
//  PMURequest
//----------------------------
	
struct PMURequest {
	PMUStatus					pmStatus;
	UInt16						pmCommand;
	UInt8 * 					pmSBuffer1;
	UInt16						pmSLength1;
	UInt8 * 					pmSBuffer2;
	UInt16						pmSLength2;
	UInt8 *						pmRBuffer;
	UInt16						pmRLength;
	UInt8						pmMisc[8];
};

typedef struct PMURequest PMURequest;
			

//----------------------------
//  command specification in PMURequest
//----------------------------

enum {
			kPMUpMgrADB   		= 			0x20,   			// send ADB command
			kPMUpMgrADBoff   	= 			0x21,   			// turn ADB auto-poll off
			kPMUxPramWrite		= 			0x32,   			// write extended PRAM byte(s)
			kPMUtimeRead  		= 			0x38,   			// read the time from the clock chip
			kPMUxPramRead 		= 			0x3A,   			// read extended PRAM byte(s)

			kPMUmaskInts		=			0x70,				//		
			kPMUreadINT   		= 			0x78,   			// get PMGR interrupt data
			kPMUPmgrPWRoff  	= 			0x7E,  				// turn system power off
			kPMUresetCPU		=			0xD0   			// reset the CPU
};


//
//	PMUInterruptState - internal to PMUCore.c
//

enum PMUInterruptState
{
	PMU_STATE_INTERRUPT_LIMBO	= -1,		//	Oh-oh
	PMU_STATE_IDLE				= 0,		//	nothing to do..
	PMU_STATE_CMD_START			= 1,		//	sending a command
	PMU_STATE_RESPONSE_EXPECTED	= 2			//	waiting for a response (ADB)
};

typedef enum PMUInterruptState PMUInterruptState;

enum
{
	bPMU_ADB_INT			=	4,
	
	PMU_ADB_INT_MASK		=	(1 << bPMU_ADB_INT)
};


//----------------------------
//	VIA registers
//----------------------------

struct VIARegisterAddress {	
		volatile UInt8*		dataA;
		volatile UInt8* 	directionA;
		volatile UInt8*		dataB;
		volatile UInt8* 	directionB;
		volatile UInt8* 	shift;
		volatile UInt8* 	auxillaryControl;
		volatile UInt8* 	interruptFlag;
		volatile UInt8* 	interruptEnable;
};
typedef struct VIARegisterAddress VIARegisterAddress;


//----------------------------
//	VIA register contents
//----------------------------

#if 0
enum {											// port B
			v2PMreq				= 			4, 					// Power manager handshake request
			v2PMack				=			3 					// Power manager handshake acknowledge
};
#endif

enum {											// IFR/IER
			ifCA2 				= 			0, 					// CA2 interrupt
			ifCA1 				= 			1, 					// CA1 interrupt
			ifSR  				= 			2, 					// SR shift register done
			ifCB2 				= 			3, 					// CB2 interrupt
			ifCB1 				= 			4, 					// CB1 interrupt
			ifT2  				= 			5, 					// T2 timer2 interrupt
			ifT1  				= 			6, 					// T1 timer1 interrupt
			ifIRQ 				= 			7 					// any interrupt
};


//----------------------------
//	Timers
//----------------------------


enum
{
	CONSTANT			= 100,					// nanoseconds per timebase period
	TIMEOUT32MS			= 32000000/CONSTANT,	// 32ms
	TIMEOUT400NS		= 400/CONSTANT			// 400ns
};

typedef struct		// holds the end-time of a timer
{
	UInt32 upper;
	UInt32 lower;
} Timer;


//----------------------------
//
//	Function Prototypes
//
//----------------------------

PMUStatus				InitializePMU(void *);
PMUStatus 				SendCommand(PMURequest *theRequest);
Boolean					waitPMUnotBusy(void);
PMUStatus				SendPMUByte(UInt8 theByte);
PMUStatus				ReadPMUByte(UInt8 *theByte);
Boolean					WaitForAckHi(void);
Boolean					WaitForAckLo(void);
void 					startTimer(Timer *, UInt32);
Boolean 				PMUtimeout(Timer *);

#endif
