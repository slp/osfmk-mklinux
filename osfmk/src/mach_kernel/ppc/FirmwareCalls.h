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

 
#ifdef ASSEMBLER

#ifdef _FIRMWARECALLS_H_
#error Hey! You can only include FirmwareCalls.h in one assembler file, dude. And it should be Firmware.s!
#else /* _FIRMWARECALLS_H_ */

/*
 *			Entries for all firmware calls are in here (except for call 0x80000000 - CutTrace
 */

#define _FIRMWARECALLS_H_

#define	fwCallEnt(name, entrypt) \
 			.set	name,(.-FWtable)/4|0x80000000;	\
			.globl	name;							\
			.long	entrypt
			
/*
 *
 */
 
			fwCallEnt(MPgetProcCountCall, MPgetProcCountLL)	/* Call the MPgetProcCount routine */
			fwCallEnt(MPstartCall, MPstartLL)				/* Call the MPstart routine */
			fwCallEnt(MPexternalHookCall, MPexternalHookLL)	/* Get the address of the external interrupt handler */
			fwCallEnt(MPsignalCall, MPsignalLL)				/* Call the MPsignal routine */
			fwCallEnt(MPstopCall, MPstopLL)					/* Call the MPstop routine */
			fwCallEnt(dbgDispCall, dbgDispLL)				/* Write stuff to printer or modem port */
			fwCallEnt(dbgCkptCall, dbgCkptLL)				/* Save 128 bytes from r3 to 0x380 V=R mapping */
#if 0
			fwCallEnt(MPCPUAddressCall, 0)					/* Call the MPCPUAddress routine */
			fwCallEnt(MPresumeCall, 0)						/* Call the MPresume routine */
			fwCallEnt(MPresetCall, 0)						/* Call the MPreset routine */
			fwCallEnt(MPSenseCall, 0)						/* Call the MPSense routine */
			fwCallEnt(MPstoreStatusCall, 0)					/* Call the MPstoreStatus routine */
			fwCallEnt(MPSetStatusCall, 0)					/* Call the MPSetStatus routine */
			fwCallEnt(MPgetSignalCall, 0)					/* Call the MPgetSignal routine */
			fwCallEnt(MPsyncTBCall, 0)						/* Call the MPsyncTB routine */
			fwCallEnt(MPcheckPendingCall, 0)				/* Call the MPcheckPending routine */
#endif	
#endif	/* _FIRMWARECALLS_H_ */

#else /* ASSEMBLER */
	
/*
 *			The firmware function headers
 */
extern void			FirmwareStoreReal (unsigned int val, unsigned int addr);
extern void			CutTrace		(unsigned int item1, ...);

#endif /* ASSEMBLER */
