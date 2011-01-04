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
#include <cuda.h>
#include <platforms.h>

#include <mach_kdb.h>
#include <kern/spl.h>
#include <machine/machparam.h>          /* spl definitions */
#include <types.h>
#include <device/io_req.h>
#include <device/tty.h>
#include <device/conf.h>
#include <chips/busses.h>
#include <ppc/misc_protos.h>
#include <ppc/POWERMAC/powermac.h>
#include <ppc/POWERMAC/adb.h>
#include <ppc/POWERMAC/cuda_hdw.h>
#include <ppc/POWERMAC/interrupts.h>
#include <ppc/POWERMAC/powermac_gestalt.h>
#include <ppc/POWERMAC/device_tree.h>
#include <ppc/POWERMAC/pram.h>

//
//	CudaInterruptState - internal to CudaCore.c
//

enum CudaInterruptState
{
	CUDA_STATE_INTERRUPT_LIMBO	= -1,		//
	CUDA_STATE_IDLE			= 0,		//
	CUDA_STATE_ATTN_EXPECTED	= 1,		//
	CUDA_STATE_TRANSMIT_EXPECTED	= 2,		//
	CUDA_STATE_RECEIVE_EXPECTED	= 3			//
};

typedef enum CudaInterruptState CudaInterruptState;

//
//	CudaTransactionFlag - internal to CudaCore.c
//

enum CudaTransactionFlag
{
	CUDA_TS_NO_REQUEST	= 0x0000,
	CUDA_TS_SYNC_RESPONSE	= 0x0001,
	CUDA_TS_ASYNC_RESPONSE	= 0x0002
};

typedef enum CudaTransactionFlag CudaTransactionFlag;

//
//	contants - internal to CudaCore.c
//

volatile CudaInterruptState	cuda_interrupt_state;
volatile CudaTransactionFlag	cuda_transaction_state;

adb_request_t	*cuda_request = NULL, *cuda_collided = NULL;
adb_packet_t	cuda_unsolicited;
adb_packet_t	*cuda_current_response = NULL;

int		cuda_transfer_count = 0;
boolean_t	cuda_is_header_transfer = FALSE;
boolean_t	cuda_is_packet_type = FALSE;
static VIARegisterAddress	cuda_via_regs;
boolean_t	cuda_initted = FALSE;
static long 	cuda_state_transition_delay_ticks;
decl_simple_lock_data(,cuda_lock)

//
//	Local Function Prototype
//

static	void	cuda_process_response(void);
static	void	cuda_transmit_data(void);
static	void	cuda_expected_attention(void);
static	void	cuda_unexpected_attention(void);
static	void	cuda_receive_data(void);
static	void	cuda_receive_last_byte(void);
static	void	cuda_collision(void);
static	void	cuda_idle(void);
void		cuda_start(adb_request_t *packet);
void		cuda_poll(void);
int		cudaprobe(caddr_t addr, void *ui);
void		cudaattach(struct bus_device *bus);
void		cudaintr(void), cuda_error(void), cuda_handler(void);
void		cuda_send_request(adb_request_t *request);


//
// inline functions
//

static unsigned char cuda_read_data(void);

static __inline__ unsigned char cuda_read_data(void)
{
	volatile unsigned char val;

	val = *cuda_via_regs.shift; eieio();

	return val;
}

static unsigned char cuda_is_interrupt(void);

static __inline__ unsigned char cuda_is_interrupt(void)
{
	volatile unsigned char val;

	val = *cuda_via_regs.interruptEnable; eieio();

	return val & kCudaInterruptMask;
}

caddr_t	cuda_std[NCUDA] = { (caddr_t) 0 };
struct bus_device	*cuda_info[NCUDA];

struct bus_driver	cuda_driver = {
	cudaprobe, 0, cudaattach, 0, cuda_std, "cuda", cuda_info, 0, 0, 0 };

struct adb_ops cuda_ops = {
	cuda_start,
	cuda_poll
};

int
cudaprobe(caddr_t addr, void *ui)
{
	device_node_t		*cudadev = NULL;
	unsigned char *cuda_base;

	if (cuda_initted) 
		return 1;


	switch (powermac_info.class) {
	case POWERMAC_CLASS_PDM:
		cuda_base = (unsigned char*)POWERMAC_IO(PDM_CUDA_BASE_PHYS);
		break;

	default:
	case POWERMAC_CLASS_PCI:
		cudadev = find_devices("via-cuda");

		if (cudadev == NULL) {
			return 0;
		}

		cuda_base = (unsigned char *) POWERMAC_IO(cudadev->addrs[0].address);
		break;

	}

	simple_lock_init(&cuda_lock, ETAP_IO_CHIP);

	cuda_initted = TRUE;

	cuda_via_regs.dataB			= (void*)cuda_base;
	cuda_via_regs.handshakeDataA		= (void*)(cuda_base+0x0200);
	cuda_via_regs.dataDirectionB		= (void*)(cuda_base+0x0400);
	cuda_via_regs.dataDirectionA		= (void*)(cuda_base+0x0600);
	cuda_via_regs.timer1CounterLow		= (void*)(cuda_base+0x0800);
	cuda_via_regs.timer1CounterHigh		= (void*)(cuda_base+0x0A00);
	cuda_via_regs.timer1LatchLow		= (void*)(cuda_base+0x0C00);
	cuda_via_regs.timer1LatchHigh		= (void*)(cuda_base+0x0E00);
	cuda_via_regs.timer2CounterLow		= (void*)(cuda_base+0x1000);
	cuda_via_regs.timer2CounterHigh		= (void*)(cuda_base+0x1200);
	cuda_via_regs.shift			= (void*)(cuda_base+0x1400);
	cuda_via_regs.auxillaryControl		= (void*)(cuda_base+0x1600);
	cuda_via_regs.peripheralControl		= (void*)(cuda_base+0x1800);
	cuda_via_regs.interruptFlag		= (void*)(cuda_base+0x1A00);
	cuda_via_regs.interruptEnable		= (void*)(cuda_base+0x1C00);
	cuda_via_regs.dataA			= (void*)(cuda_base+0x1E00);
	

	// we require delays of this duration between certain state transitions

	cuda_state_transition_delay_ticks = nsec_to_processor_clock_ticks(200);

	// Set the direction of the cuda signals.  ByteACk and TIP are output and
	// TREQ is an input

	*cuda_via_regs.dataDirectionB |= (kCudaByteAcknowledgeMask | kCudaTransferInProgressMask);

	*cuda_via_regs.dataDirectionB &= ~kCudaTransferRequestMask;


	// Set the clock control.  Set to shift data in by external clock CB1.

	*cuda_via_regs.auxillaryControl = (*cuda_via_regs.auxillaryControl |
											 kCudaTransferMode) &
											 kCudaSystemRecieve;

	// Clear any posible cuda interupt.
	
	if ( *cuda_via_regs.shift );

	// Initialize the internal data.

	cuda_interrupt_state	= CUDA_STATE_IDLE;
	cuda_transaction_state	= CUDA_TS_NO_REQUEST;
	cuda_is_header_transfer	= FALSE;
	cuda_is_packet_type	= FALSE;
	cuda_transfer_count	= 0;
	cuda_current_response	= NULL;

	// Terminate transaction and set idle state

	cuda_neg_tip_and_byteack();

	// we want to delay 4 mS for ADB reset to complete

	delay(400);

	// Clear pending interrupt if any...

	(void)cuda_read_data();

	// Issue a Sync Transaction, ByteAck asserted while TIP is negated.

	cuda_assert_byte_ack();

	// Wait for the Sync acknowledgement, cuda to assert TREQ

	cuda_wait_for_transfer_request_assert();

	// Wait for the Sync acknowledgement interrupt.

	cuda_wait_for_interrupt();

	// Clear pending interrupt

	(void)cuda_read_data();

	// Terminate the sync cycle by Negating ByteAck

	cuda_neg_byte_ack();

	// Wait for the Sync termination acknowledgement, cuda negates TREQ.

	cuda_wait_for_transfer_request_neg();

	// Wait for the Sync termination acknowledgement interrupt.

	cuda_wait_for_interrupt();

	// Terminate transaction and set idle state, TIP negate and ByteAck negate.
	cuda_neg_transfer_in_progress();

	// Clear pending interrupt, if there is one...
	(void)cuda_read_data();

	adb_hardware = &cuda_ops;

	pmac_register_int(PMAC_DEV_CUDA, SPLTTY,
					(void (*)(int, void *)) cudaintr);

	return	1;
}


void
cudaattach(struct bus_device * device)
{
	printf(" alive\n");

}



//
//	StartCudaTransmission
//	Execute at secondary interrupt.
//

void
cuda_start(adb_request_t *request)
{
	spl_t	interruptState;
	adb_request_t	*next;

	if (!adb_polling) 
		interruptState = spltty();

	simple_lock(&cuda_lock);

	/* Check to see if the cuda is busy with another
	 * request .. typically a unsolicated request 
	 * (keyboard event, mouse event, etc.)
	 */

	if (cuda_request != NULL)  {
		printf("cuda already has request?!?\n");
		printf("Interrupt state = %x\n", cuda_interrupt_state);
		printf("%x\n", cuda_request);
		printf("cmd = %x, \n", cuda_request->a_cmd.a_header[0]);
		panic("cuda already has a request going?!?");
	}

	cuda_send_request(request);

	if (adb_polling) {
		simple_unlock(&cuda_lock);
		cuda_poll();
	} else {
		simple_unlock(&cuda_lock);
		splx(interruptState);
	}


	return;
}

void
cuda_send_request(adb_request_t *request)
{
	

	// The data register must written with the data byte 25uS
	// after examining TREQ or we run the risk of getting out of sync
	// with Cuda. So disable interrupts to protect critical area.

	// Check if we can commence with the packet transmission.  First, check if
	// Cuda can service our request now.  Second, check if Cuda wants to send
	// a response packet now.

	if ( cuda_interrupt_state == CUDA_STATE_IDLE 
	&& !cuda_is_transfer_in_progress() ) {
	// Set the shift register direction to output to Cuda by setting
	// the direction bit.

		cuda_request = request;
		cuda_request->a_reply.a_bcount = 0;

		cuda_set_data_direction_to_output();

		// Write the first byte to the shift register
		cuda_write_data(cuda_request->a_cmd.a_header[0]);

		// Set up the transfer state info here.

		cuda_is_header_transfer = TRUE;
		cuda_transfer_count = 1;

		// Make sure we're in idle state before transaction, and then
		// assert TIP to tell Cuda we're starting command
		cuda_neg_byte_ack();
		cuda_assert_transfer_in_progress();

		// The next state is going to be a transmit state, if there is
		// no collision.  This is a requested response but call it sync.

		cuda_interrupt_state = CUDA_STATE_TRANSMIT_EXPECTED;
		cuda_transaction_state = CUDA_TS_SYNC_RESPONSE;
	} else if (cuda_collided) {
		panic("CUDA Queue Error - Start Transmission, collision - two requests present.");
	} else {
		cuda_collided = request;
	}

}

void
cuda_poll(void)
{
	adb_request_t	*next;

	simple_lock(&cuda_lock);

	for (;;) {
		while (cuda_interrupt_state != CUDA_STATE_IDLE) {
			cuda_wait_for_interrupt();
			cuda_handler();
		}

		if (cuda_collided) {
			next = cuda_collided;
			cuda_collided = NULL;
			cuda_send_request(next);
		} else  if ((next = adb_next_request()) != NULL) {
			cuda_send_request(next);
		} else {
			simple_unlock(&cuda_lock);
			return;
		}
	}

	simple_unlock(&cuda_lock);
}

static
int
cuda_get_result(unsigned char theStatus)
{
	int	status = ADB_RET_OK;
	

	if ( theStatus & kCudaTimeOutMask ) {
		status = ADB_RET_TIMEOUT;
	} else if ( theStatus & kCudaSRQAssertMask ) {
		status = ADB_RET_UNEXPECTED_RESULT;
	} else if ( theStatus & kCudaSRQErrorMask ) {
		status = ADB_RET_REQUEST_ERROR;
	} else if ( theStatus & kCudaBusErrorMask ) {
		status = ADB_RET_BUS_ERROR;
	}


	return status;
}


//
//	cuda_process_response
//	Execute at secondary interrupt.
//

void
cuda_process_response(void)
{
	adb_request_t	*request;


	// Almost ready for the next state, which should be a Idle state.
	// Just need to notifiy the client.

	if ( cuda_transaction_state == CUDA_TS_SYNC_RESPONSE ) {
		request = (adb_request_t *) cuda_request;
		request->a_result = cuda_get_result(cuda_request->a_reply.a_header[1]);
		cuda_request = NULL;
		simple_unlock(&cuda_lock);
		adb_done(request);
		simple_lock(&cuda_lock);
	} else if ( cuda_transaction_state == CUDA_TS_ASYNC_RESPONSE ) {
		simple_unlock(&cuda_lock);
		adb_unsolicited_done(cuda_current_response);
		simple_lock(&cuda_lock);
	}


	return;
}

void
cudaintr()
{
	if (adb_polling)
		return;

	simple_lock(&cuda_lock);
	cuda_handler();
	simple_unlock(&cuda_lock);
}

void
cuda_handler()
{
	unsigned char	interruptState;

	// Get the relevant signal in determining the cause of the interrupt:
	// the shift direction, the transfer request line and the transfer
	// request line.

	interruptState = cuda_get_interrupt_state();

	//printf("{%d}", interruptState);

	switch ( interruptState ) {
	case kCudaReceiveByte:
		cuda_receive_data();
		break;

	case kCudaReceiveLastByte:
		cuda_receive_last_byte();
		break;

	case kCudaTransmitByte:
		cuda_transmit_data();
		break;

	case kCudaUnexpectedAttention:
		cuda_unexpected_attention();
		break;

	case kCudaExpectedAttention:
		cuda_expected_attention();
		break;

	case kCudaIdleState:
		cuda_idle();
		break;

	case kCudaCollision:
		cuda_collision();
		break;

	// Unknown interrupt, clear it and leave.
	default:
		cuda_error();
		break;
	}
}

//
//	TransmitCudaData
//	Executes at hardware interrupt level.
//

static void cuda_transmit_data(void)
{

	// Clear the pending interrupt by reading the shift register.


	if ( cuda_is_header_transfer ) {
		// There are more header bytes, write one out.
		cuda_write_data(cuda_request->a_cmd.a_header[cuda_transfer_count++]);

		// Toggle the handshake line.
		if ( cuda_transfer_count >= cuda_request->a_cmd.a_hcount ) {
			cuda_is_header_transfer = FALSE;
			cuda_transfer_count = 0;
		}

		cuda_toggle_byte_ack();
	} else if ( cuda_transfer_count < cuda_request->a_cmd.a_bcount ) {
	// There are more command bytes, write one out and update the pointer
		cuda_write_data(cuda_request->a_cmd.a_buffer[cuda_transfer_count++]);

		// Toggle the handshake line.
		cuda_toggle_byte_ack();
	} else {
		(void)cuda_read_data();
		// There is no more command bytes, terminate the send transaction.
		// Cuda should send a expected attention interrupt soon.

		cuda_neg_tip_and_byteack();

		// The next interrupt should be a expected attention interrupt.

		cuda_interrupt_state = CUDA_STATE_ATTN_EXPECTED;
	}

}

//
//	cuda_expected_attention
//	Executes at hardware interrupt level.
//

static void cuda_expected_attention(void)
{
	int	i;


	// Clear the pending interrupt by reading the shift register.

	(void)cuda_read_data();

	// Allow the VIA to settle directions.. else the possibility of
	// data corruption.
	tick_delay(cuda_state_transition_delay_ticks);

	if ( cuda_transaction_state ==  CUDA_TS_SYNC_RESPONSE) {
		cuda_current_response = (adb_packet_t*)&cuda_request->a_reply;
	} else {
		cuda_unsolicited.a_hcount = 0;
		cuda_unsolicited.a_bcount = 0;
		cuda_unsolicited.a_bsize = sizeof(cuda_unsolicited.a_buffer);
		cuda_current_response = &cuda_unsolicited;
	}

	cuda_is_header_transfer = TRUE;
	cuda_is_packet_type = TRUE;
	cuda_transfer_count = 0;

	// Set the shift register direction to input.
	cuda_set_data_direction_to_input();

	// Start the response packet transaction.
	cuda_assert_transfer_in_progress();

	// The next interrupt should be a receive data interrupt.
	cuda_interrupt_state = CUDA_STATE_RECEIVE_EXPECTED;


}

//
//	cuda_unexpected_attention
//	Executes at hardware interrupt level.
//

static void cuda_unexpected_attention(void)
{

	// Clear the pending interrupt by reading the shift register.
	(void)cuda_read_data();

	// Get ready for a unsolicited response.

	cuda_unsolicited.a_hcount = 0;
	cuda_unsolicited.a_bcount = 0;
	cuda_unsolicited.a_bsize = sizeof(cuda_unsolicited.a_buffer);

	cuda_current_response = &cuda_unsolicited;

	cuda_is_header_transfer = TRUE;
	cuda_is_packet_type = TRUE;
	cuda_transfer_count = 0;


	// Start the response packet transaction, Transaction In Progress
	cuda_assert_transfer_in_progress();

	// The next interrupt should be a receive data interrupt and the next
	// response should be an async response.

	cuda_interrupt_state = CUDA_STATE_RECEIVE_EXPECTED;

	cuda_transaction_state = CUDA_TS_ASYNC_RESPONSE;

}

//
//	cuda_receive_data
//	Executes at hardware interrupt level.
//

static void cuda_receive_data(void)
{

	if ( cuda_is_packet_type ) {
		unsigned char packetType;

		packetType = cuda_read_data();
		cuda_current_response->a_header[cuda_transfer_count++] = packetType;

		if ( packetType == ADB_PACKET_ERROR) {
			cuda_current_response->a_hcount = 4;
		} else {
			cuda_current_response->a_hcount = 3;
		}

		cuda_is_packet_type = FALSE;

		cuda_toggle_byte_ack();
	} else if ( cuda_is_header_transfer ) {

		cuda_current_response->a_header[cuda_transfer_count++] =
				cuda_read_data();

		if (cuda_transfer_count >= cuda_current_response->a_hcount) {
			cuda_is_header_transfer = FALSE;
			cuda_transfer_count = 0;
		}

		cuda_toggle_byte_ack();
	} else if ( cuda_transfer_count < cuda_current_response->a_bsize ) {
		// Still room for more bytes. Get the byte and tell Cuda to continue.
		// Toggle the handshake line, ByteAck, to acknowledge receive.

		cuda_current_response->a_buffer[cuda_transfer_count++] =
				cuda_read_data();

		cuda_toggle_byte_ack();
	} else {
		// Cuda is still sending data but the buffer is full.
		// Normally should not get here.  The only exceptions are open ended
		// request such as  PRAM read...  In any event time to exit.

		cuda_current_response->a_bcount = cuda_transfer_count;

		cuda_read_data();

		cuda_process_response();
		cuda_neg_tip_and_byteack();
	}


}

//
//	cuda_receive_last_byte
//	Executes at hardware interrupt level.
//

static void cuda_receive_last_byte(void)
{

	if ( cuda_is_header_transfer ) {
		cuda_current_response->a_header[cuda_transfer_count++]	=
			cuda_read_data();

		cuda_transfer_count = 0;
	} else if ( cuda_transfer_count < cuda_current_response->a_bsize ) {
		cuda_current_response->a_buffer[cuda_transfer_count++] =
			cuda_read_data();
	} else {
		/* Overrun -- ignore data */
		(void) cuda_read_data();
	}

	cuda_current_response->a_bcount = cuda_transfer_count;

	cuda_process_response();
	cuda_neg_tip_and_byteack();

}

//
//	cuda_collision
//	Executes at hardware interrupt level.
//

static void cuda_collision(void)
{

	// Clear the pending interrupt by reading the shift register.
	(void)cuda_read_data();

	// Negate TIP to abort the send.  Cuda should send a second attention
	// interrupt to acknowledge the abort cycle.
	cuda_neg_transfer_in_progress();

	// The next interrupt should be an expected attention and the next
	// response packet should be an async response.

	cuda_interrupt_state = CUDA_STATE_ATTN_EXPECTED;
	cuda_transaction_state = CUDA_TS_ASYNC_RESPONSE;

	/* queue the request */
	cuda_collided = cuda_request;
	cuda_request = NULL;
	cuda_is_header_transfer = FALSE;
	cuda_transfer_count = 0;

}

//
//	cuda_idle
//	Executes at hardware interrupt level.
//

static void cuda_idle(void)
{
	adb_request_t	*next;


	// Clear the pending interrupt by reading the shift register.
	(void)cuda_read_data();

	// Set to the idle state.
	cuda_interrupt_state = CUDA_STATE_IDLE;

	// See if there are any pending requests.  There may be a pending request
	// if a collision has occured.  If so do the resend.

	if (adb_polling) {
		return;		/* Prevent recursion */
	}

	if ((next = cuda_collided) != NULL) {
		cuda_collided = NULL;
		simple_unlock(&cuda_lock);
		cuda_start(next);
		simple_lock(&cuda_lock);
	} else if ((next = adb_next_request()) != NULL) {
		simple_unlock(&cuda_lock);
		cuda_start(next);
		simple_lock(&cuda_lock);
	}

}

void
cuda_error(void)
{
	int	i;


	//printf("{Error %d}", cuda_transaction_state);

	switch (cuda_transaction_state) {
	case	CUDA_STATE_IDLE:
		cuda_neg_tip_and_byteack();
		break;

	case	CUDA_STATE_TRANSMIT_EXPECTED:
		if (cuda_is_header_transfer && cuda_transfer_count <= 1) {
			tick_delay(cuda_state_transition_delay_ticks);
			cuda_neg_transfer_in_progress();
			cuda_set_data_direction_to_input();
			panic ("CUDA - TODO FORCE COMMAND BACK UP!\n");
		} else {
			cuda_transaction_state = CUDA_STATE_ATTN_EXPECTED;
			cuda_neg_tip_and_byteack();
		}
		break;

	case	CUDA_STATE_ATTN_EXPECTED:
		cuda_assert_transfer_in_progress();

		tick_delay(cuda_state_transition_delay_ticks);
		cuda_set_data_direction_to_input();
		cuda_neg_transfer_in_progress();
		panic("CUDA - TODO CHECK FOR TRANSACTION TYPE AND ERROR");
		break;

	case	CUDA_STATE_RECEIVE_EXPECTED:
		cuda_neg_tip_and_byteack();
		panic("Cuda - todo check for transaction type and error");
		break;

	default:
		cuda_set_data_direction_to_input();
		cuda_neg_tip_and_byteack();
		break;
	}

}
