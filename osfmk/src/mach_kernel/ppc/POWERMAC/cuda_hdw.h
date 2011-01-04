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
#include	<ppc/POWERMAC/via6522.h>

typedef struct VIARegisterAddress VIARegisterAddress;

struct VIARegisterAddress
{	
		volatile unsigned char*		dataB;
		volatile unsigned char* 	handshakeDataA;
		volatile unsigned char* 	dataDirectionB;
		volatile unsigned char* 	dataDirectionA;
		volatile unsigned char* 	timer1CounterLow;
		volatile unsigned char* 	timer1CounterHigh;
		volatile unsigned char* 	timer1LatchLow;
		volatile unsigned char* 	timer1LatchHigh;
		volatile unsigned char* 	timer2CounterLow;
		volatile unsigned char* 	timer2CounterHigh;
		volatile unsigned char* 	shift;
		volatile unsigned char* 	auxillaryControl;
		volatile unsigned char* 	peripheralControl;
		volatile unsigned char* 	interruptFlag;
		volatile unsigned char* 	interruptEnable;
		volatile unsigned char* 	dataA;
};



//	Cuda to VIA signal definition.  They are all active low.

enum
{
	kCudaTransferRequestMask		= EVRB_XCVR,		// TREQ			(input)	
	kCudaNegateTransferRequest		= EVRB_XCVR,		// TREQ
	kCudaAssertTransferRequest		= ~EVRB_XCVR,		// /TREQ

	kCudaByteAcknowledgeMask		= EVRB_FULL,		// ByteAck		(output)
	kCudaNegateByteAcknowledge		= EVRB_FULL,		// ByteAck
	kCudaAssertByteAcknowledge		= ~EVRB_FULL,		// /ByteAck
	
	kCudaTransferInProgressMask		= EVRB_SYSES,		// TIP			(output)
	kCudaNegateTransferInProgress	= EVRB_SYSES,		// TIP
	kCudaAssertTransferInProgress	= ~EVRB_SYSES,		// /TIP

	kCudaTransferMode				= VAC_SRMD3,		// 

	kCudaDirectionMask				= VAC_SRMD4,		// 
	kCudaSystemSend					= VAC_SRMD4,		// 
	kCudaSystemRecieve				= ~VAC_SRMD4,		// 
	
	kCudaInterruptMask				= VIE_SR,
	kCudaInterruptDisable			= VIE_CLEAR | VIE_SR,
	kCudaInterruptEnable			= VIE_SET | VIE_SR
};

//	The bits from Cuda that determines the cause of an interrupt

enum
{
		kCudaInterruptStateMask		= kCudaTransferInProgressMask |
									  kCudaTransferRequestMask
};

// Interrupt states.  Determined by kTransferRequest, kTransferInProgress and 
// kCudaDirection.  The names are from the view of the system.

enum
{
		kCudaReceiveByte			= 0,							// 0x00
		kCudaReceiveLastByte		= kCudaNegateTransferRequest,	// 0x08
		kCudaCollision				= kCudaSystemSend,				// 0x10
		kCudaTransmitByte			= kCudaSystemSend | 	
									  kCudaNegateTransferRequest, 	// 0x18
		kCudaUnexpectedAttention	= kCudaNegateTransferInProgress,// 0x20
		kCudaIdleState				= kCudaNegateTransferInProgress |  
									  kCudaNegateTransferRequest,	// 0x28
		kCudaExpectedAttention		= kCudaSystemSend |
									  kCudaNegateTransferInProgress,// 0x30
		kCudaIllegalState		 	= kCudaSystemSend |
									  kCudaNegateTransferInProgress	|
									  kCudaNegateTransferRequest	// 0x38
};

enum
{
		kCudaSRQAssertMask			= 0x01,			// inactive device asserted SRQ
		kCudaTimeOutMask			= 0x02,			// active device did not have data available
		kCudaSRQErrorMask			= 0x04,			// device asserted excessive SRQ period
		kCudaBusErrorMask			= 0x08,			// timing error in bit cell was detected
		kCudaAutoPollMask			= 0x40,			// data is from an AutoPoll
		kCudaResponseMask			= 0x80			// response Packet in progress
};

#define	cuda_write_data(theByte)  {*cuda_via_regs.shift = theByte; eieio(); }
#define cuda_set_data_direction_to_input()  {*cuda_via_regs.auxillaryControl &= kCudaSystemRecieve; eieio(); }
#define	cuda_set_data_direction_to_output()  {*cuda_via_regs.auxillaryControl |= kCudaSystemSend; eieio(); }
#define cuda_assert_transfer_in_progress()  {*cuda_via_regs.dataB &= kCudaAssertTransferInProgress; eieio(); }
#define cuda_neg_transfer_in_progress()  {*cuda_via_regs.dataB |= kCudaNegateTransferInProgress; eieio(); }
#define cuda_neg_tip_and_byteack()	 {*cuda_via_regs.dataB |= kCudaNegateByteAcknowledge | kCudaNegateTransferInProgress; eieio(); }
#define cuda_toggle_byte_ack()  {*cuda_via_regs.dataB ^= kCudaByteAcknowledgeMask; eieio(); }
#define cuda_assert_byte_ack()   {*cuda_via_regs.dataB &= kCudaAssertByteAcknowledge; eieio(); }
#define cuda_neg_byte_ack()  {*cuda_via_regs.dataB |= kCudaNegateByteAcknowledge; eieio(); }
#define cuda_is_transfer_in_progress() ((*cuda_via_regs.dataB & kCudaTransferRequestMask) == 0 )
#define cuda_disable_interrupt()  {*cuda_via_regs.interruptEnable = kCudaInterruptDisable; eieio(); }
#define cuda_enable_interrupt()  {*cuda_via_regs.interruptEnable = kCudaInterruptEnable; eieio(); }
#define cuda_get_interrupt_state()  (*cuda_via_regs.dataB & kCudaInterruptStateMask) | \
			   (*cuda_via_regs.auxillaryControl & kCudaDirectionMask)
#define cuda_wait_for_transfer_request_assert() while ( (*cuda_via_regs.dataB & kCudaTransferRequestMask) != 0 ) { eieio(); } ; eieio()
#define cuda_wait_for_transfer_request_neg() while ( (*cuda_via_regs.dataB & kCudaTransferRequestMask) == 0 ) { eieio(); } ; eieio()
#define cuda_wait_for_interrupt() while ( (*cuda_via_regs.interruptFlag & kCudaInterruptMask) == 0 ) { eieio(); } ; eieio()
