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

#include <pmu.h>
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
#include <ppc/proc_reg.h>
#include <ppc/POWERMAC/powermac.h>
#include <ppc/POWERMAC/adb.h>
#include <ppc/POWERMAC/interrupts.h>
#include <ppc/POWERMAC/powermac_pci.h>
#include <ppc/POWERMAC/powermac_m2.h>
#include <ppc/POWERMAC/via6522.h>
#include <ppc/POWERMAC/pmu_defs.h>
#include <ppc/POWERMAC/device_tree.h>


///////////////////////////////////////////////////////////////////////////////
// function declarations
///////////////////////////////////////////////////////////////////////////////
void 		pmuattach( struct bus_device *device);
int 		pmuprobe(caddr_t addr, void *ui);
void		Handle_ADB_Interrupt(PMURequest theRequest);
void		Handle_ADB_Explicit(PMURequest theRequest);
void		Handle_ADB_Implicit(PMURequest theRequest);
void		pmuintr(void);
void 		panic_adb_request(adb_request_t *request);
void		pmu_handle_adb(adb_request_t *request);
void		pmu_pseudo_autopoll(adb_request_t *request);
void		pmu_ignore_request(adb_request_t *request);
void		pmu_set_device_list(adb_request_t *request);
void		pmu_get_real_time(adb_request_t *request);
void		pmu_get_pram(adb_request_t *request);
void            pmu_restart(void);
void            pmu_power_down(void);
void		pmu_handle_pseudo(adb_request_t *request);
void		pmu_send_request(adb_request_t *request);
void		pmu_start(adb_request_t *request);
void		pmu_poll(void);
void		ACK_PMU_Interrupt(void);

///////////////////////////////////////////////////////////////////////////////
// globals
///////////////////////////////////////////////////////////////////////////////

//
//	PMUTransactionFlag - internal to PMUCore.c
//

// enum PMUTransactionFlag
// {
// 	PMU_TS_NO_REQUEST	= 0x0000,
// 	PMU_TS_SYNC_RESPONSE	= 0x0001,
// 	PMU_TS_ASYNC_RESPONSE	= 0x0002
// };
//
// typedef enum PMUTransactionFlag PMUTransactionFlag;

volatile PMUInterruptState	pmu_interrupt_state;
volatile UInt8				pmu_last_adb_command;
UInt8						pmu_adb_flags=0;

adb_request_t	*pmu_request = NULL, *pmu_collided = NULL;

adb_packet_t	pmu_unsolicited;

int				pmu_transfer_count = 0;
boolean_t		pmu_is_header_transfer = FALSE;
boolean_t		pmu_is_packet_type = FALSE;
static long 	pmu_state_transition_delay_ticks;
UInt16			pmu_device_map;							// bitmap of ADB devices

boolean_t				pmu_initted = FALSE;
struct bus_device		*pmu_info[NPMU];
struct bus_driver		pmu_driver = { pmuprobe, 0, pmuattach, 0, NULL, "pmu", pmu_info, 0, 0, 0 };
struct adb_ops pmu_ops = 
{
	pmu_start,
	pmu_poll
};

VIARegisterAddress		gVIA1;
VIARegisterAddress		gVIA2;



SInt8			cmdLengthTable[256] = {
								-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,		// 0x00 - 0x0F
								 1, 1,-1,-1,-1,-1,-1,-1, 0, 0,-1,-1,-1,-1,-1, 0,		// 0x10 - 0x1F
								-1, 0, 2, 1, 1,-1,-1,-1, 0,-1,-1,-1,-1,-1,-1,-1,		// 0x20 - 0x2F
								 4,20,-1,-1,-1,-1,-1,-1, 0, 0, 2,-1,-1,-1,-1,-1,		// 0x30 - 0x3F
								 1, 1,-1,-1,-1,-1,-1,-1, 0, 0,-1,-1, 1,-1,-1,-1,		// 0x40 - 0x4F
								 1, 0, 2, 2,-1, 1, 3, 1, 0, 1, 0, 0, 0,-1,-1,-1,		// 0x50 - 0x5F
								 2,-1, 2, 0,-1,-1,-1,-1, 0, 0, 0, 0, 0, 0,-1,-1,		// 0x60 - 0x6F
								 1, 1, 1,-1,-1,-1,-1,-1, 0, 0,-1,-1,-1,-1, 4, 4,		// 0x70 - 0x7F
								 4,-1, 0,-1,-1,-1,-1,-1, 0,-1,-1,-1,-1,-1,-1,-1,		// 0x80 - 0x8F
								 1, 2,-1,-1,-1,-1,-1,-1, 0, 0,-1,-1,-1,-1,-1,-1,		// 0x90 - 0x9F
								 2, 2, 2, 4,-1, 0,-1,-1, 1, 1, 3, 2,-1,-1,-1,-1,		// 0xA0 - 0xAF
								-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,		// 0xB0 - 0xBF
								-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,		// 0xC0 - 0xCF
								 0,-1,-1,-1,-1,-1,-1,-1, 1, 1,-1,-1, 0, 0,-1,-1,		// 0xD0 - 0xDF
								-1, 4, 0,-1,-1,-1,-1,-1, 3,-1, 0,-1, 0,-1,-1, 0,		// 0xE0 - 0xEF
								-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1			// 0xF0 - 0xFF
						};


//  This table is used to determine how to handle the reply:

//	=0:	no reply should be expected.
//	=1: only a reply byte will be sent (this is a special case for a couple of commands)
//	<0:	a reply is expected and the PMGR will send a count byte.
//	>1:	a reply is expected and the PMGR will not send a count byte,
//		but the count will be (value-1).

SInt8					rspLengthTable[256] = {
																						
								 0, 0, 0, 0, 0, 0, 0, 0,-1,-1,-1,-1,-1,-1,-1,-1,			// 0x00 - 0x0F
								 0, 0, 0, 0, 0, 0, 0, 0, 2, 2,-1,-1,-1,-1,-1, 0,			// 0x10 - 0x1F
								 0, 0, 0, 0, 0, 0, 0, 0,-1,-1,-1,-1,-1,-1,-1,-1,			// 0x20 - 0x2F
								 0, 0, 0, 0, 0, 0, 0, 0, 5,21,-1,-1,-1,-1,-1,-1,			// 0x30 - 0x3F
								 0, 0, 0, 0, 0, 0, 0, 0, 2, 2,-1,-1, 0,-1,-1,-1,			// 0x40 - 0x4F
								 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 3, 3,-1,-1,-1,-1,			// 0x50 - 0x5F
								 0, 0, 0, 3, 0, 0, 0, 0, 4, 4, 3, 9,-1,-1,-1,-1,			// 0x60 - 0x6F
								 0, 0, 0, 0, 0, 0, 0, 0,-1,-1,-1,-1,-1,-1, 1, 1,			// 0x70 - 0x7F
								 0, 0, 0, 0, 0, 0, 0, 0, 6,-1,-1,-1,-1,-1,-1,-1,			// 0x80 - 0x8F
								 0, 0, 0, 0, 0, 0, 0, 0, 2, 2,-1,-1,-1,-1,-1,-1,			// 0x90 - 0x9F
								 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0,-1,-1,-1,-1,			// 0xA0 - 0xAF
								 0, 0, 0, 0, 0, 0, 0, 0,-1,-1,-1,-1,-1,-1,-1,-1,			// 0xB0 - 0xBF
								 0, 0, 0, 0, 0, 0, 0, 0,-1,-1,-1,-1,-1,-1,-1,-1,			// 0xC0 - 0xCF
								 0, 0, 0, 0, 0, 0, 0, 0, 2, 2,-1,-1, 2,-1,-1,-1,			// 0xD0 - 0xDF
								 0, 0, 1, 0, 0, 0, 0, 0,-1,-1, 2,-1,-1,-1,-1, 0,			// 0xE0 - 0xEF
								 0, 0, 0, 0, 0, 0, 0, 0,-1,-1,-1,-1,-1,-1,-1,-1				// 0xF0 - 0xFF
						};


extern boolean_t	pmu_lock_initted;
static device_node_t	*pmudev;

decl_simple_lock_data(,pmu_lock)

long	v2PMreq, v2PMack;

///////////////////////////////////////////////////////////////////////////////
// function definitions
///////////////////////////////////////////////////////////////////////////////
int 
pmuprobe(caddr_t addr, void *ui)
{

	if (pmu_initted) 
		return 1;

	switch (powermac_info.class) {
	case	POWERMAC_CLASS_POWERBOOK:
	
		if (InitializePMU((UInt8*)POWERMAC_IO(M2_PMU_BASE_PHYS))) {
			printf("** PowerBook PMU failed initialize\n");
			return 0;
		}
		break;

	default:
		if ((pmudev = find_devices("via-pmu")) == NULL) 
			return 0;
		

		if (InitializePMU((UInt8*)(POWERMAC_IO(pmudev->addrs[0].address))) != kPMUNoError)
		{
			printf("PMU: Init failed!\n");
			return 0;
		}
	}

	// Initialize the internal data.

	simple_lock_init(&pmu_lock, ETAP_IO_CHIP);

	pmu_interrupt_state	= PMU_STATE_IDLE;
	pmu_is_header_transfer	= FALSE;
	pmu_is_packet_type	= FALSE;
	pmu_transfer_count	= 0;

	pmu_initted = TRUE;

	adb_hardware = &pmu_ops;

	pmac_register_int(PMAC_DEV_PMU, SPLTTY, (void (*)(int, void *)) pmuintr);

	return 1;
}



enum {
	kPMUMD0Int	= 0x01,	/* interrupt type 0 (machine-specific)*/
	kPMUMD1Int	= 0x02,	/* interrupt type 1 (machine-specific)*/
	kPMUMD2Int	= 0x04,	/* interrupt type 2 (machine-specific)*/
	kPMUbrightnessInt	= 0x08,	/* brightness button has been pressed, value changed*/
	kPMUADBint	= 0x10,		/* ADB*/
	kPMUbattInt	= 0x20,		/* battery*/
	kPMUenvironmentInt	= 0x40,		/* environment*/
	kPMUoneSecInt	= 0x80		/* one second interrupt*/
};

void 
pmuattach( struct bus_device *device)
{
	PMURequest	theRequest;
	spl_t		s;

	s = spltty();
	simple_lock(&pmu_lock);

	ACK_PMU_Interrupt();		// clear any pending interrupt

	theRequest.pmCommand = kPMUmaskInts;
	theRequest.pmSLength1 = 1;	
	theRequest.pmSLength2 = 0;
	theRequest.pmMisc[0] = kPMUMD2Int | kPMUbrightnessInt | kPMUADBint;
	theRequest.pmSBuffer1 = &theRequest.pmMisc[0];
	theRequest.pmRBuffer = NULL;
	SendCommand(&theRequest);

	ACK_PMU_Interrupt();		// clear any pending interrupt

	theRequest.pmCommand = kPMUreadINT;	// read any pending interrupt from PGE
	theRequest.pmSLength1 = 0;	// just to clear it
	theRequest.pmSLength2 = 0;
	theRequest.pmRBuffer = &theRequest.pmMisc[0];
	SendCommand(&theRequest);
	simple_unlock(&pmu_lock);
	splx(s);

	printf(" attached");
}

void
pmuintr(void)
{
	
	PMURequest		theRequest;
	UInt8			interruptState[12];

	simple_lock(&pmu_lock);

	interruptState[0] = interruptState[1] = interruptState[2] = 0;
	//ACK_PMU_Interrupt();
	theRequest.pmCommand = kPMUreadINT;				// read any pending interrupt from PGE
	theRequest.pmSLength1 = 0;						// just to clear it
	theRequest.pmSLength2 = 0;
	theRequest.pmRBuffer = interruptState;

	SendCommand(&theRequest);				       	// see what interrupt is waiting

	if (theRequest.pmRLength < 1)
		panic("pmuintr: bad ReadINT length");
	

	if (interruptState[0] & PMU_ADB_INT_MASK)
	{
		Handle_ADB_Interrupt(theRequest);
	}


	simple_unlock(&pmu_lock);
}



void		
panic_adb_request(adb_request_t *request) {
	int		i;
	

	printf("PMU start, request = %08x\na_cmd\n\ta_header\t", request);
	for(i=0; i<8; i++)
		printf(" %02x", request->a_cmd.a_header[i]);
	printf("\n\ta_hcount\t%x\n\ta_buffer\t", request->a_cmd.a_hcount);
	for (i=0; i<32; i++)
		printf(" %02x", request->a_cmd.a_buffer[i]);
	printf("\n\ta_bcount\t%x\n\ta_bsize\t%x\na_reply\n\ta_header\t", request->a_cmd.a_bcount, request->a_cmd.a_bsize);
	for(i=0; i<8; i++)
		printf(" %02x", request->a_reply.a_header[i]);
	printf("\n\ta_hcount\t%x\n\ta_buffer\t", request->a_reply.a_hcount);
	for (i=0; i<32; i++)
		printf(" %02x", request->a_reply.a_buffer[i]);
	printf("\n\ta_bcount\t%x\n\ta_bsize\t%x\n", request->a_reply.a_bcount, request->a_reply.a_bsize);
	panic("pmu unknown request");
}


void
pmu_ignore_request(adb_request_t *request)
{

	pmu_interrupt_state = PMU_STATE_IDLE;
	pmu_request = NULL;
	pmu_last_adb_command = 0;
	adb_done(request);

}


void
pmu_set_device_list(adb_request_t *request)
{

	pmu_device_map = (request->a_cmd.a_header[2] << 8) + request->a_cmd.a_header[3];
	pmu_interrupt_state = PMU_STATE_IDLE;
	pmu_request = NULL;
	pmu_last_adb_command = 0;
	adb_done(request);

}


void		
pmu_pseudo_autopoll(adb_request_t *request) 
{
	PMURequest		theRequest;
	UInt8			cmd_buf[4];


	if (request->a_cmd.a_header[2])
	{
		theRequest.pmCommand = kPMUpMgrADB;
		cmd_buf[0] = 0;										// don't care
		cmd_buf[1] = 0x86;									// magic
		cmd_buf[2] = (UInt8)(pmu_device_map >> 8);			// upper 8 bits
		cmd_buf[3] = (UInt8)pmu_device_map;					// lower
		theRequest.pmSLength1 = 4;
		theRequest.pmSBuffer1 = cmd_buf;
		pmu_adb_flags = 0x02;
	}
	else
	{
		theRequest.pmCommand = kPMUpMgrADBoff;				// command to turn auto-poll off
		theRequest.pmSLength1 = 0;							// just to clear it
		theRequest.pmSBuffer1 = NULL;
		pmu_adb_flags = 0;
	}
	theRequest.pmSLength2 = 0;
	theRequest.pmSBuffer2 = NULL;
	theRequest.pmRBuffer = NULL;
	request->a_result = SendCommand(&theRequest);

	if (!(request->a_cmd.a_header[2]))
	{
		UInt8		interruptState[12];
		// for poll off case, I want to clear any pending interrupt
		do {
			ACK_PMU_Interrupt();							// clear any pending interrupt
			theRequest.pmCommand = kPMUreadINT;				// read any pending interrupt from PGE
			theRequest.pmSLength1 = 0;						// just to clear it
			theRequest.pmSLength2 = 0;
			theRequest.pmRBuffer = interruptState;
			SendCommand(&theRequest);
			} while (interruptState[0]);
	}

	pmu_interrupt_state = PMU_STATE_IDLE;
	pmu_request = NULL;
	pmu_last_adb_command = 0;
	adb_done(request);

}


void
Handle_ADB_Interrupt(PMURequest theRequest)
{
	int length = theRequest.pmRLength;
	

	if (length < 1)
		panic("Handle_ADB_Interrupt: bad length");


	if (theRequest.pmRBuffer[0] & 0x04)
		Handle_ADB_Implicit(theRequest);
	else
		Handle_ADB_Explicit(theRequest);
		
}



void
Handle_ADB_Explicit(PMURequest theRequest)
{
	int length = theRequest.pmRLength;
	int	i;
	adb_request_t 			*ADBrequest = pmu_request;
	


	if (pmu_interrupt_state == PMU_STATE_IDLE)
		panic("\nHandle_ADB_Explicit: idle\n");
		
	if (!ADBrequest)
		panic("\nHandle_ADB_Explicit: no request\n");

	if (length < 2)
		panic("Handle_ADB_Explicit: bad length");

	if (pmu_interrupt_state != PMU_STATE_RESPONSE_EXPECTED)
		panic("Handle_ADB_Explicit: unexpected");
		
	if (theRequest.pmRBuffer[1] != pmu_last_adb_command)
	{
		printf("Handle_ADB_Explicit: expected %02x got %02x\n", pmu_last_adb_command, theRequest.pmRBuffer[1]);
		panic("what now?");
		return;	
	}
	
	if (length > 2)
		ADBrequest->a_result = ADB_RET_OK;
	else
		ADBrequest->a_result = ADB_RET_TIMEOUT;
		
	ADBrequest->a_reply.a_bcount = length - 2;

	for (i = 0; i < length-2; i++)
	{
		ADBrequest->a_reply.a_buffer[i] = theRequest.pmRBuffer[i+2];

	}

	pmu_request = NULL;
	pmu_interrupt_state = PMU_STATE_IDLE;
	pmu_last_adb_command = 0;
	adb_done(ADBrequest);

}



void
Handle_ADB_Implicit(PMURequest theRequest)
{
	int length = theRequest.pmRLength;
	int	i;
	

	if ((length < 2) || (length > 34))
		panic("Handle_ADB_Implicit: bad length");

	pmu_unsolicited.a_header[1] = theRequest.pmRBuffer[0];		// flags
	pmu_unsolicited.a_header[2] = theRequest.pmRBuffer[1];		// adb command
	pmu_unsolicited.a_hcount = 3;
	pmu_unsolicited.a_bcount = length-2;
	pmu_unsolicited.a_bsize = 32;
	for (i=0; i < (length-2); i++)
		pmu_unsolicited.a_buffer[i] = theRequest.pmRBuffer[i+2];
	adb_unsolicited_done(&pmu_unsolicited);

}



void
pmu_handle_adb(adb_request_t *request)
{
	PMURequest		theRequest;
	int				total_count = request->a_cmd.a_hcount+request->a_cmd.a_bcount;
	UInt8			command_buf[3];


	if (total_count < 2)
		panic("pmu_handle_adb: invalid count");
		
	theRequest.pmCommand = kPMUpMgrADB;				// ADB command coming in...
	
	// in buffer 1, I build the minimum 3 byte adb command [cmd] [flags] [length]
	command_buf[0] = request->a_cmd.a_header[1];
	command_buf[1] = pmu_adb_flags;
	command_buf[2] = total_count - 2;			// extra bytes in the original command
	theRequest.pmSBuffer1 = command_buf;
	theRequest.pmSLength1 = 3;
	
	// now append the stuff passed in
	theRequest.pmSLength2 = command_buf[2];
	if (theRequest.pmSLength2)
	{
		if (request->a_cmd.a_hcount == 2)
			theRequest.pmSBuffer2 = request->a_cmd.a_buffer;
		else
			theRequest.pmSBuffer2 = &(request->a_cmd.a_header[2]);
	}
	theRequest.pmRBuffer = NULL;				// no receive on ADB commands
	
	pmu_interrupt_state = PMU_STATE_RESPONSE_EXPECTED;
	pmu_last_adb_command = request->a_cmd.a_header[1];
	SendCommand(&theRequest);

}



void
pmu_get_real_time(adb_request_t *request)
{
	int	x;
	spl_t			s;
	PMURequest		theRequest;
	
	s = spltty();

	theRequest.pmCommand = kPMUtimeRead;
	theRequest.pmSBuffer1 = theRequest.pmSBuffer2 = NULL;
	theRequest.pmSLength1 = theRequest.pmSLength2 = 0;
	theRequest.pmRBuffer = request->a_reply.a_buffer;
	request->a_result = SendCommand(&theRequest);
	request->a_reply.a_bcount = theRequest.pmRLength;
	pmu_interrupt_state = PMU_STATE_IDLE;
	pmu_request = NULL;
	pmu_last_adb_command = 0;
	
	splx(s);

	adb_done(request);
}



void
pmu_get_pram(adb_request_t *request)
{
	PMURequest		theRequest;
	UInt8			command_buf[2];
	

	theRequest.pmCommand = kPMUxPramRead;
	
	// PMU only supports offsets up to 0xFF
	if (request->a_cmd.a_header[2])
		panic("pmu: only supports offsets to 0xff");
	
	// only support 255 byte transfers
	if ((request->a_reply.a_bsize < 0) || (request->a_reply.a_bsize > 255))
		panic("pmu: only support transfers up to 255");
		
	command_buf[0] = request->a_cmd.a_header[2];						// offset to read from
	command_buf[1] = (UInt8)request->a_reply.a_bsize;
	
	// adjust for bytes at the end
	if ((command_buf[0] + command_buf[1]) > 255)
	{
		printf("pmu: adjusting length");
		command_buf[1] = 255-command_buf[0];
	}
	
	theRequest.pmSBuffer1 = command_buf;
	theRequest.pmSLength1 = request->a_reply.a_bsize;
	theRequest.pmSBuffer2 = NULL;
	theRequest.pmSLength2 = 0;
	theRequest.pmRBuffer = request->a_reply.a_buffer;
	request->a_result = SendCommand(&theRequest);
	request->a_reply.a_bcount = theRequest.pmRLength;
	pmu_interrupt_state = PMU_STATE_IDLE;
	pmu_request = NULL;
	pmu_last_adb_command = 0;
	adb_done(request);
	
}



void
pmu_restart(void)
{
  PMURequest       theRequest;
  

  theRequest.pmCommand = kPMUresetCPU;
  theRequest.pmSLength1 = theRequest.pmSLength2 = 0;
  theRequest.pmSBuffer1 = theRequest.pmSBuffer2 = NULL;
  theRequest.pmRBuffer = NULL;
  SendCommand(&theRequest);


  for(;;)                                   // just spin
    ;
}

void
pmu_power_down(void)
{
  PMURequest       theRequest;
  UInt8            sig[4];
  UInt8            buf[16];


  sig[0]='M';sig[1]='A';sig[2]=sig[3]='T';
  theRequest.pmCommand = kPMUPmgrPWRoff;
  theRequest.pmSLength1 = 4;
  theRequest.pmSBuffer1 = sig;
  theRequest.pmSLength2 = 0;
  theRequest.pmSBuffer2 = NULL;
  theRequest.pmRBuffer = buf;
  SendCommand(&theRequest);
  if (theRequest.pmRBuffer[0] == 0x70)
  {
    for (;;)
      ;
  }
  else if (theRequest.pmRBuffer[0] == 0xAA)
  	printf("denied\n");
  else
    printf("\nUnknown: %02x\n", theRequest.pmRBuffer[0]);

}


void		
pmu_handle_pseudo(adb_request_t *request) 
{

	if (request->a_cmd.a_hcount < 2)
	{
		printf("pmu_handle_pseudo: bad length\n");
		panic_adb_request(request);
	}
	switch(request->a_cmd.a_header[1])
	{
		case ADB_PSEUDOCMD_START_STOP_AUTO_POLL:
			pmu_pseudo_autopoll(request);
			break;

		case ADB_PSEUDOCMD_SET_AUTO_RATE:
			pmu_ignore_request(request);
			break;

		case ADB_PSEUDOCMD_SET_DEVICE_LIST:
			pmu_set_device_list(request);
			break;

		case ADB_PSEUDOCMD_GET_REAL_TIME:
			pmu_get_real_time(request);
			break;
			
		case ADB_PSEUDOCMD_GET_PRAM:
			pmu_get_pram(request);
			break;
		
	        case ADB_PSEUDOCMD_POWER_DOWN:
		        pmu_power_down();
			break;

	        case ADB_PSEUDOCMD_RESTART_SYSTEM:
		        pmu_restart();
                        break;

	        default:
			printf("pmu_handle_pseudo: unknown command\n");
			panic_adb_request(request);
	}

}


void
pmu_send_request(adb_request_t *request)
{
	
	if ( pmu_interrupt_state == PMU_STATE_IDLE ) 
	{
		pmu_request = request;
		// adb request from the adb driver, which thinks it is talking to a cuda, so we need to 
		// convert cuda requests to pmu requests
	
		pmu_interrupt_state = PMU_STATE_CMD_START;
		switch(request->a_cmd.a_header[0])
		{
			case ADB_PACKET_PSEUDO:
				pmu_handle_pseudo(request);
				break;
	
			case ADB_PACKET_ADB:
				pmu_handle_adb(request);
				break;
				
			default:
				printf("pmu_start: unknown request\n");
				panic_adb_request(request);
		}
	} else if (pmu_collided)
		panic("PMU Queue Error - Start Transmission, collision - two requests present.");
	else
		pmu_collided = request;

}



void		
pmu_start(adb_request_t *request) 
{
	spl_t			interruptState;
	adb_request_t	*next;


	/* Check to see if the pmu is busy with another
	 * request .. typically a unsolicated request 
	 * (keyboard event, mouse event, etc.)
	 */

	if (pmu_request != NULL)  {
		printf("pmu already has request?!?\n");
		printf("Interrupt state = %x\n", pmu_interrupt_state);
		printf("%x\n", pmu_request);
		printf("cmd = %x, \n", pmu_request->a_cmd.a_header[0]);
		panic("pmu already has a request going?!?");
	}

	if (!adb_polling) 
		interruptState = spltty();

	simple_lock(&pmu_lock);

	pmu_send_request(request);

	if (adb_polling) {
		simple_unlock(&pmu_lock);
		pmu_poll();
	} else {
		simple_unlock(&pmu_lock);
		splx(interruptState);
	}


	return;
}


void	pmu_wait_for_interrupt(void);
void
pmu_wait_for_interrupt()
{
	while (!(*gVIA1.interruptFlag & (1 << ifCB1)))
		eieio();
	eieio();
}



void
pmu_poll(void)
{
	adb_request_t	*next;

	simple_lock(&pmu_lock);

	for (;;) {
		while (pmu_interrupt_state != PMU_STATE_IDLE) {
			pmu_wait_for_interrupt();
			simple_unlock(&pmu_lock);
			pmuintr();
			simple_lock(&pmu_lock);
		}

		if (pmu_collided) {
			next = pmu_collided;
			pmu_collided = NULL;
			pmu_send_request(next);
		} else  if ((next = adb_next_request()) != NULL) {
			pmu_send_request(next);
		} else {
			simple_unlock(&pmu_lock);
			return;
		}
	}

	simple_unlock(&pmu_lock);
}

//еееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееее
//	InitializePMU
//	Initialize hardware
//еееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееее

PMUStatus 
InitializePMU(void * physAddr)
{	
	PMUStatus		status;
	PMURequest		theRequest;
	UInt8			interruptState[12];
	UInt8			mask_buf[1];


	gVIA1.shift				= (UInt8 *)physAddr + 0x1400;
	gVIA1.auxillaryControl	= (UInt8 *)physAddr + 0x1600;
	gVIA1.interruptFlag		= (UInt8 *)physAddr + 0x1A00;
	gVIA1.interruptEnable	= (UInt8 *)physAddr + 0x1C00;
				
	switch (powermac_info.class) {
	case	POWERMAC_CLASS_POWERBOOK:
		gVIA2.dataB	= (UInt8 *)physAddr + 0x2000;
		v2PMack = 1;
		v2PMreq = 2;
		break;

	default:
		gVIA2.dataB	= (UInt8 *)physAddr + 0x0000;
		v2PMack = 3;
		v2PMreq = 4;
		break;
	}

	theRequest.pmCommand = kPMUmaskInts;
	mask_buf[0] = PMU_ADB_INT_MASK;					// mask out all ints except ADB
	theRequest.pmSLength1 = 1;
	theRequest.pmSBuffer1 = mask_buf;
	theRequest.pmRBuffer = NULL;
	SendCommand(&theRequest);
	
	do {
		ACK_PMU_Interrupt();							// clear any pending interrupt
		theRequest.pmCommand = kPMUreadINT;				// read any pending interrupt from PGE
		theRequest.pmSLength1 = 0;						// just to clear it
		theRequest.pmSLength2 = 0;
		theRequest.pmRBuffer = interruptState;
		SendCommand(&theRequest);
		} while (interruptState[0]);


	return kPMUNoError;
}

//еееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееее
//	SendCommand
//еееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееее

PMUStatus 
SendCommand(PMURequest * plugInMessage)
{
	UInt16			retries,i;
	UInt8			firstChar, receivedByte, charCountS1, charCountS2;
	UInt8			*dataPointer, *dataPointer1, *dataPointer2;
	SInt8			charCountR;
	Timer			timer;
	PMUStatus		status;


	retries = 0;
	firstChar = plugInMessage->pmCommand;				// get command byte
	charCountS1 = plugInMessage->pmSLength1;			// get caller's length counters
	charCountS2 = plugInMessage->pmSLength2;
	dataPointer1 = plugInMessage->pmSBuffer1;			// and transmit data pointers
	dataPointer2 = plugInMessage->pmSBuffer2;
	
	charCountR = rspLengthTable[firstChar];				// get response length from table

	if ( ((plugInMessage->pmSLength1 != 0) && (plugInMessage->pmSBuffer1 == NULL)) ||		// validate pointers
		((plugInMessage->pmSLength2 != 0) && (plugInMessage->pmSBuffer2 == NULL)) ||
		(((charCountR < 0) || (charCountR > 1)) && (plugInMessage->pmRBuffer == NULL)) ) {
		return (plugInMessage->pmStatus);
	}
	if ( (firstChar != 0xE1) &&												// validate length
		 (cmdLengthTable[firstChar] != -1) &&
		 (cmdLengthTable[firstChar] != (charCountS1 + charCountS2)) ) {
		 return(plugInMessage->pmStatus);
	}
	
	plugInMessage->pmStatus = kPMUIOError;
	
	WaitForAckHi();

	while ( 1 ) {
		status = SendPMUByte(firstChar);				// send command byte
		if ( status == kPMUNoError ) break;
		if ( ++retries >= 512 ) {
			return(plugInMessage->pmStatus);
		}

		startTimer(&timer,TIMEOUT32MS);
		while ( 1 )										// spin for 32 milliseconds
			if ( PMUtimeout(&timer) ) break;
	}

	if ( cmdLengthTable[firstChar] < 0 ) {				// should we send a length byte?
		SendPMUByte((UInt8)charCountS1 + charCountS2);	// yes, do it
	}

	for ( i = 0; i < charCountS1; i++ ) {				// send data bytes
		SendPMUByte(*dataPointer1++);
	}

	for ( i = 0; i < charCountS2; i++ ) {				// send more data bytes
		SendPMUByte(*dataPointer2++);
	}
/* charCountR ==	0:	no reply at all
				1:	only a reply byte will be sent by the PGE
				<0: a length byte and a reply will be sent
				>1: a reply will be sent, but no length byte
					 (length is charCount - 1)
*/

	plugInMessage->pmRLength = 0;
	if ( charCountR ) {									// receive the reply byte
		if ( charCountR == 1 ) {
			ReadPMUByte(&receivedByte);
			*(plugInMessage->pmRBuffer) = receivedByte;
		} else {
			if ( charCountR < 0 ) {					// receive the length byte
				ReadPMUByte(&receivedByte);
				charCountR = receivedByte;
			} else {
				charCountR--;
			}
			plugInMessage->pmRLength = charCountR;
			dataPointer = plugInMessage->pmRBuffer;
			for ( i = 0; i < charCountR; i++ ) {
				ReadPMUByte(dataPointer++);				// receive the rest of the reply
			}
		}
	}
	plugInMessage->pmStatus = kPMUNoError;


	return(plugInMessage->pmStatus);
}


//еееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееее
//	SendPMUByte
//еееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееее

PMUStatus 
SendPMUByte(UInt8 theByte)
{
	PMUStatus status = kPMUNoError;


	*gVIA1.auxillaryControl |= 0x1C;			// set shift register to output
	eieio();
	*gVIA1.shift = theByte;						// give it the byte
	eieio();
	*gVIA2.dataB &= ~(1<<v2PMreq);				// assert /REQ
	eieio();
	if ( WaitForAckLo() ) {						// ack now low
		*gVIA2.dataB |= (1<<v2PMreq);			// deassert /REQ line
		eieio();
		if ( !WaitForAckHi() ) {
			status = kPMUsendEndErr;
		}
	}
	else {
		*gVIA2.dataB |= (1<<v2PMreq);			// deassert /REQ line
		eieio();
		status = kPMUsendStartErr;
	}


	return ( status );
}

//еееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееее
//	ReadPMUByte
//еееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееее

PMUStatus
ReadPMUByte(UInt8 *theByte)
{
	PMUStatus status = kPMUNoError;


	*gVIA1.auxillaryControl |= 0x0C; eieio();	// set shift register to input
	*gVIA1.auxillaryControl &= ~0x10; eieio();
	*theByte = *gVIA1.shift;	// read a byte to reset shift reg
	eieio();
	*gVIA2.dataB &= ~(1<<v2PMreq);	// assert /REQ
	eieio();
	if ( WaitForAckLo() ) {			// ack now low
		*gVIA2.dataB |= (1<<v2PMreq);		// deassert /REQ line
		eieio();
		if ( WaitForAckHi() ) {					// wait for /ACK high
			*theByte = *gVIA1.shift;			// got it, read the byte
			eieio();
		}
		else {
			status = kPMUrecvEndErr;
		}
	}
	else {
		*gVIA2.dataB |= (1<<v2PMreq);			// deassert /REQ line
		eieio();
		status = kPMUrecvStartErr;
	}


	return ( status );
}

//еееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееее
//	WaitForAckLo
//еееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееее

Boolean WaitForAckLo(void)
{
	Timer timer;


// wait up to 32 milliseconds for Ack signal from PG&E to go low

	startTimer(&timer,TIMEOUT32MS);
	while ( TRUE ) {
		if ( !(*gVIA2.dataB & (1<<v2PMack)) ) {
			return ( TRUE );
		}
		if ( PMUtimeout(&timer) ) {
			return ( FALSE );
		}
	}

}

//еееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееее
//	WaitForAckHi
//еееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееееее

Boolean WaitForAckHi(void)
{
Timer timer;


// wait up to 32 milliseconds for Ack signal from PG&E to go high

	startTimer(&timer,TIMEOUT32MS);
	while ( TRUE ) {
		if ( *gVIA2.dataB & (1<<v2PMack) ) {
			return ( TRUE );
		}
		if ( PMUtimeout(&timer) ) {
			return ( FALSE);
		}
	}

}

// *************************************************************************************
// Start a timer by copying the current value of the timebase
// *************************************************************************************

void startTimer(Timer * timer, UInt32 duration)
{
UInt32 temp;


	while ( 1 )
	{
		timer->upper = mftbu();
		timer->lower = mftb();
		temp = mftbu();
		if ( temp == timer->upper ) break;
	}
	if ( (timer->lower += duration) < timer->lower ) ++timer->upper;
	timer->lower += duration;

}


// *************************************************************************************
// Check to see if a timer has expired by comparing the sum of its start value
// and its duration to the current value of the timebase
// *************************************************************************************

Boolean 
PMUtimeout(Timer * timer)
{
	UInt32 currentUpper;
	UInt32 currentLower;
	UInt32 temp;


	while ( 1 )
	{
		currentUpper = mftbu();
		currentLower = mftb();
		temp = mftbu();
		if ( temp == currentUpper ) break;
	}
	if ( currentLower > timer->lower )
	{
		if ( currentUpper < timer->upper ) {
			return FALSE;
		} else {
			return TRUE;
		}
	}
	if ( currentUpper > timer->upper ) {
		return TRUE;
	} else {
		return FALSE;
	}
}



void
ACK_PMU_Interrupt()
{

	*gVIA1.interruptFlag = ((1<<ifIRQ) | (1 << ifCB1));
	eieio();
}
