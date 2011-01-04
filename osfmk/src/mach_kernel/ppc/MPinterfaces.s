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

/* 																							
 	MPinterfaces.s 

	General interface to the MP hardware handlers anonymous

	Lovingly crafted by Bill Angell using traditional methods and only natural or recycled materials.
	No animal products are used other than rendered otter bile.

*/

#include <cpus.h>
#include <ppc/asm.h>
#include <ppc/proc_reg.h>
#include <ppc/POWERMAC/mp/MPPlugIn.h>
#include <mach/machine/vm_param.h>
#include <assym.s>

/*
 *			This first section is the glue for the high level C code.
 *			Anything that needs any kind of system services (e.g., VM) has to be done here.  The firmware
 *			code that implements the SC runs in real mode.
 */



/* #define	MPI_DEBUGGING	0 */
#define		MPI_DEBUGGING	0

/*
 *			The routine glues to the count CPU firmware call
 */

ENTRY(MPgetProcCount, TAG_NO_FRAME_USED)


#if 0
			lis		r3,KERNELBASE_TEXT_OFFSET@h				/* (TEST/DEBUG) Get location 0 */
			ori		r3,r3,KERNELBASE_TEXT_OFFSET@l			/* (TEST/DEBUG) Get location 0 */
			stmw	r0,0x680(r3)							/* (TEST/DEBUG) Save registers */
#endif

			mr		r12,r0									/* Keep R0 pristene */
			lis		r0,MPgetProcCountCall@h					/* Top half of MPgetProcCount firmware call number */
			ori		r0,r0,MPgetProcCountCall@l				/* Bottom half */
			sc												/* Go see how many processors we have */
			
#if			MPI_DEBUGGING
			lis		r0,CutTrace@h							/* Top half of trace entry maker call */
			ori		r0,r0,CutTrace@l						/* Bottom half of trace entry maker call */
			sc												/* Cut a backend trace entry */
#endif

			mr		r0,r12									/* Restore R0 */

#if 0
			lis		r3,KERNELBASE_TEXT_OFFSET@h				/* (TEST/DEBUG) Get location 0 */
			ori		r3,r3,KERNELBASE_TEXT_OFFSET@l			/* (TEST/DEBUG) Get location 0 */
			stmw	r0,0x780(r3)							/* (TEST/DEBUG) Save registers */
			
			lis		r3,0xF301								/* (TEST/DEBUG) Top part of printer port write data */
			ori		r3,r3,0x3010							/* (TEST/DEBUG) Bottom part */
			dcbst	0,r3									/* (TEST/DEBUG) Make sure this is outta there */
			sync											/* (TEST/DEBUG) Make sure */
			dcbi	0,r3									/* (TEST/DEBUG) Toss it completely */
			sync											/* (TEST/DEBUG) Make sure */
		
			mfmsr	r5										/* (TEST/DEBUG) Get the MSR */
			rlwinm	r11,r5,0,17,15							/* (TEST/DEBUG) Disable interruptions */
			rlwinm	r11,r11,0,28,26							/* (TEST/DEBUG) and DDAT also */
			mtmsr	r11										/* (TEST/DEBUG) Do it */
			isync											/* (TEST/DEBUG) Do it some more */
			mflr	r8										/* (TEST/DEBUG) */
			mr		r9,r1									/* (TEST/DEBUG) */
			mr		r10,r2									/* (TEST/DEBUG) */
			mr		r2,r3									/* (TEST/DEBUG) */
			mr		r11,r3									/* (TEST/DEBUG) */
			li		r1,0x0A									/* (TEST/DEBUG) Linefeed */
			bl		sendchar								/* (TEST/DEBUG) Send it */
			li		r1,0x0D									/* (TEST/DEBUG) Carriage return */
			bl		sendchar								/* (TEST/DEBUG) Send it */
			li		r1,0x0D									/* (TEST/DEBUG) Carriage return */
			bl		sendchar								/* (TEST/DEBUG) Send it */
			li		r1,0x52									/* (TEST/DEBUG) 'R' */
			bl		sendchar								/* (TEST/DEBUG) Send it */
			li		r1,0x65									/* (TEST/DEBUG) 'e' */
			bl		sendchar								/* (TEST/DEBUG) Send it */
			li		r1,0x74									/* (TEST/DEBUG) 't' */
			bl		sendchar								/* (TEST/DEBUG) Send it */
			li		r1,0x3A									/* (TEST/DEBUG) ':' */
			bl		sendchar								/* (TEST/DEBUG) Send it */
			li		r1,0x20									/* (TEST/DEBUG) ' ' */
			bl		sendchar								/* (TEST/DEBUG) Send it */
			bl		sendword								/* (TEST/DEBUG) Convert and send the return code */
			mr		r2,r8									/* (TEST/DEBUG) */
			bl		sendword								/* (TEST/DEBUG) Convert and send the return address */
			mr		r2,r5									/* (TEST/DEBUG) */
			bl		sendword								/* (TEST/DEBUG) Convert and send the MSR */

			li		r4,0									/* (TEST/DEBUG) */

dumpregs:	li		r1,0x0A									/* (TEST/DEBUG) Linefeed */
			bl		sendchar								/* (TEST/DEBUG) Send it */
			li		r1,0x0D									/* (TEST/DEBUG) Carriage return */
			bl		sendchar								/* (TEST/DEBUG) Send it */
			li		r1,0x0D									/* (TEST/DEBUG) Carriage return */
			bl		sendchar								/* (TEST/DEBUG) Send it */
			
			lwz		r2,0x680(r4)							/* (TEST/DEBUG) Get original word */
			bl		sendword								/* (TEST/DEBUG) Send it */
			lwz		r2,0x780(r4)							/* (TEST/DEBUG) Get now word */
			bl		sendword								/* (TEST/DEBUG) Send it */
			addi	r4,r4,4									/* (TEST/DEBUG) Point to the next */
			cmplwi	cr0,r4,32*4								/* (TEST/DEBUG) Have we seen them all? */
			blt+	dumpregs								/* (TEST/DEBUG) Nope... */
		
			lis		r3,0xF301								/* (TEST/DEBUG) Top part of printer port write data */
			ori		r3,r3,0x3010							/* (TEST/DEBUG) Bottom part */
			dcbst	0,r3									/* (TEST/DEBUG) Make sure this is outta there */
			sync											/* (TEST/DEBUG) Make sure */
			dcbi	0,r3									/* (TEST/DEBUG) Toss it completely */
			sync											/* (TEST/DEBUG) Make sure */
		
			mr		r1,r9									/* (TEST/DEBUG) */
			mr		r2,r10									/* (TEST/DEBUG) */
			mr		r3,r11									/* (TEST/DEBUG) */
			mtlr	r8										/* (TEST/DEBUG) */
			mtmsr	r5										/* (TEST/DEBUG) Restore the MSR */
			mr		r0,r12									/* (TEST/DEBUG) restore R0 */
			isync											/* (TEST/DEBUG) */
#endif			
			
			blr												/* Return, pass back R3... */

/*
 *			The routine glues to the start CPU firmware call - actually it's really a boot
 *			The first parameter is the CPU number to start
 *			The second parameter is the real address of the code used to boot the processor
 *			The third parameter is the real addess of the CSA for the subject processor
 */

ENTRY(MPstart, TAG_NO_FRAME_USED)

			mr		r12,r0									/* Keep R0 pristene */
			lis		r0,MPstartCall@h						/* Top half of MPstartCall firmware call number */
			ori		r0,r0,MPstartCall@l						/* Bottom half */
			sc												/* Go see how many processors we have */
			
#if			MPI_DEBUGGING
			lis		r0,CutTrace@h							/* Top half of trace entry maker call */
			ori		r0,r0,CutTrace@l						/* Bottom half of trace entry maker call */
			sc												/* Cut a backend trace entry */
#endif

			mr		r0,r12									/* Restore R0 */
			blr												/* Return... */

/*
 *			This routine glues to the get external interrupt handler physical address
 */

ENTRY(MPexternalHook, TAG_NO_FRAME_USED)

			mr		r12,r0									/* Keep R0 pristene */
			lis		r0,MPexternalHookCall@h					/* Top half of MPexternalHookCall firmware call number */
			ori		r0,r0,MPexternalHookCall@l				/* Bottom half */
			sc												/* Go see how many processors we have */
			
#if			MPI_DEBUGGING
			lis		r0,CutTrace@h							/* Top half of trace entry maker call */
			ori		r0,r0,CutTrace@l						/* Bottom half of trace entry maker call */
			sc												/* Cut a backend trace entry */
#endif

			mr		r0,r12									/* Restore R0 */
			blr												/* Return... */


/*
 *			This routine glues to the signal processor routine
 */

ENTRY(MPsignal, TAG_NO_FRAME_USED)

			mr		r12,r0									/* Keep R0 pristene */
			lis		r0,MPsignalCall@h						/* Top half of MPsignalCall firmware call number */
			ori		r0,r0,MPsignalCall@l					/* Bottom half */
			sc												/* Go kick the other guy */
			
#if			MPI_DEBUGGING
			lis		r0,CutTrace@h							/* Top half of trace entry maker call */
			ori		r0,r0,CutTrace@l						/* Bottom half of trace entry maker call */
			sc												/* Cut a backend trace entry */
#endif

			mr		r0,r12									/* Restore R0 */
			blr												/* Return... */


/*
 *			This routine glues to the stop processor routine
 */

ENTRY(MPstop, TAG_NO_FRAME_USED)

			mr		r12,r0									/* Keep R0 pristene */
			lis		r0,MPstopCall@h							/* Top half of MPsignalCall firmware call number */
			ori		r0,r0,MPstopCall@l						/* Bottom half */
			sc												/* Stop the other guy cold */
			
#if			MPI_DEBUGGING
			lis		r0,CutTrace@h							/* Top half of trace entry maker call */
			ori		r0,r0,CutTrace@l						/* Bottom half of trace entry maker call */
			sc												/* Cut a backend trace entry */
#endif

			mr		r0,r12									/* Restore R0 */
			blr												/* Return... */


/* *************************************************************************************************************
 *
 *			This second section is the glue for the low level stuff directly into the PM plugin.
 *			At this point every register in existence should be saved
 *
 ***************************************************************************************************************/


/*
 *			See how many physical processors we have
 */
 
ENTRY(MPgetProcCountLL, TAG_NO_FRAME_USED)

			lis		r11,MPEntries@h							/* Get the address of the MP entry block  (in the V=R area) */
			ori		r11,r11,MPEntries@l						/* Get the bottom of the MP spec area */
			lwz		r10,kCountProcessors*4(r11)				/* Get the routine entry point */
			mflr	r13										/* Save the return in an unused register */
			mtlr	r10										/* Set it */
			blrl											/* Call the routine */
			mtlr	r13										/* Restore firmware caller address */
			blr												/* Leave... */

/*
 *			Start up a processor
 */

ENTRY(MPstartLL, TAG_NO_FRAME_USED)

			lis		r11,MPEntries@h							/* Get the address of the MP entry block  (in the V=R area) */
			ori		r11,r11,MPEntries@l						/* Get the bottom of the MP spec area */
			lwz		r10,kStartProcessor*4(r11)				/* Get the routine entry point */
			mflr	r13										/* Save the return in an unused register */
			mtlr	r10										/* Set it */
			blrl											/* Call the routine */
			mtlr	r13										/* Restore firmware caller address */
			blr												/* Leave... */

/*
 *			Get physical address of SIGP external handler
 */

ENTRY(MPexternalHookLL, TAG_NO_FRAME_USED)

			lis		r11,MPEntries@h							/* Get the address of the MP entry block  (in the V=R area) */
			ori		r11,r11,MPEntries@l						/* Get the bottom of the MP spec area */
			lwz		r10,kExternalHook*4(r11)				/* Get the routine entry point */
			mflr	r13										/* Save the return in an unused register */
			mtlr	r10										/* Set it */
			blrl											/* Call the routine */
			mtlr	r13										/* Restore firmware caller address */
			blr												/* Leave... */



/*
 *			Send a signal to another processor
 */

ENTRY(MPsignalLL, TAG_NO_FRAME_USED)

			lis		r11,MPEntries@h							/* Get the address of the MP entry block  (in the V=R area) */
			ori		r11,r11,MPEntries@l						/* Get the bottom of the MP spec area */
			lwz		r10,kSignalProcessor*4(r11)				/* Get the routine entry point */
			mflr	r13										/* Save the return in an unused register */
			mtlr	r10										/* Set it */
			blrl											/* Call the routine */
			mtlr	r13										/* Restore firmware caller address */
			blr												/* Leave... */



/*
 *			Stop another processor
 */

ENTRY(MPstopLL, TAG_NO_FRAME_USED)

			lis		r11,MPEntries@h							/* Get the address of the MP entry block  (in the V=R area) */
			ori		r11,r11,MPEntries@l						/* Get the bottom of the MP spec area */
			lwz		r10,kStopProcessor*4(r11)				/* Get the routine entry point */
			mflr	r13										/* Save the return in an unused register */
			mtlr	r10										/* Set it */
			blrl											/* Call the routine */
			mtlr	r13										/* Restore firmware caller address */
			blr												/* Leave... */


/*
 *			Third section: Miscellaneous MP related routines
 */



/*
 *			All non-primary CPUs start here.
 *			We are dispatched by the SMP driver. Addressing is real (no DR or IR), 
 *			interruptions disabled, etc.  R3 points to the CPUStatusArea (CSA) which contains
 *			most of the state for the processor.  This is set up by the primary.  Note that we 
 *			do not use everything in the CSA.  Caches should be clear and coherent with 
 *			no paradoxies (well, maybe one doxie, a pair would be pushing it).
 */
	
ENTRY(start_secondary,TAG_NO_FRAME_USED)

			mr		r31,r3							/* Get the pointer to the CSA */
			
			lis		r21,SpinTimeOut@h				/* Get the top part of the spin timeout */
			ori		r21,r21,SpinTimeOut@l			/* Slam in the bottom part */
			
GetValid:	lbz		r10,CSAregsAreValid(r31)		/* Get the CSA validity value */

			stw		r21,28(r0)						/* (TEST/DEBUG) */

			mr.		r10,r10							/* Is the area valid yet? */
			bne		GotValid						/* Yeah... */
			addic.	r21,r21,-1						/* Count the try */
			isync									/* Make sure we don't prefetch the valid flag */
			bge+	GetValid						/* Still more tries left... */
			blr										/* Return and cancel startup request... */
				
GotValid:	li		r21,0							/* Set the valid flag off (the won't be after the RFI) */
			lwz		r10,CSAdec(r31)					/* Get the decrimenter */
			stb		r21,CSAregsAreValid(r31)		/* Clear that validity flag */
			
			lwz		r11,CSAdbat+(0*8)+0(r31)		/* Get the first DBAT */
			lwz		r12,CSAdbat+(0*8)+4(r31)		/* Get the first DBAT */
			lwz		r13,CSAdbat+(1*8)+0(r31)		/* Get the second DBAT */
			mtdec	r10								/* Set the decrimenter */
			lwz		r14,CSAdbat+(1*8)+4(r31)		/* Get the second DBAT */
			mtdbatu	0,r11							/* Set top part of DBAT 0 */
			lwz		r15,CSAdbat+(2*8)+0(r31)		/* Get the third DBAT */
			mtdbatl	0,r12							/* Set lower part of DBAT 0 */
			lwz		r16,CSAdbat+(2*8)+4(r31)		/* Get the third DBAT */
			mtdbatu	1,r13							/* Set top part of DBAT 1 */
			lwz		r17,CSAdbat+(3*8)+0(r31)		/* Get the fourth DBAT */
			mtdbatl	1,r14							/* Set lower part of DBAT 1 */
			lwz		r18,CSAdbat+(3*8)+4(r31)		/* Get the fourth DBAT */
			mtdbatu	2,r15							/* Set top part of DBAT 2 */			
			lwz		r11,CSAibat+(0*8)+0(r31)		/* Get the first IBAT */
			mtdbatl	2,r16							/* Set lower part of DBAT 2 */
			lwz		r12,CSAibat+(0*8)+4(r31)		/* Get the first IBAT */
			mtdbatu	3,r17							/* Set top part of DBAT 3 */
			lwz		r13,CSAibat+(1*8)+0(r31)		/* Get the second IBAT */
			mtdbatl	3,r18							/* Set lower part of DBAT 3 */
			lwz		r14,CSAibat+(1*8)+4(r31)		/* Get the second IBAT */
			mtibatu	0,r11							/* Set top part of IBAT 0 */
			lwz		r15,CSAibat+(2*8)+0(r31)		/* Get the third IBAT */
			mtibatl	0,r12							/* Set lower part of IBAT 0 */
			lwz		r16,CSAibat+(2*8)+4(r31)		/* Get the third IBAT */
			mtibatu	1,r13							/* Set top part of IBAT 1 */
			lwz		r17,CSAibat+(3*8)+0(r31)		/* Get the fourth IBAT */
			mtibatl	1,r14							/* Set lower part of IBAT 1 */
			lwz		r18,CSAibat+(3*8)+4(r31)		/* Get the fourth IBAT */
			mtibatu	2,r15							/* Set top part of IBAT 2 */
			lwz		r11,CSAsdr1(r31)				/* Get the SDR1 value */
			mtibatl	2,r16							/* Set lower part of IBAT 2 */
			lwz		r12,CSAsprg(r31)				/* Get SPRG0 (the per_proc_info address) */
			mtibatu	3,r17							/* Set top part of IBAT 3 */
			lwz		r13,CSAmsr(r31)					/* Get the MSR */
			mtibatl	3,r18							/* Set lower part of IBAT 3 */
			lwz		r14,CSApc(r31)					/* Get the PC */
			sync									/* Sync up */
			mtsdr1	r11								/* Set the SDR1 value */
			sync									/* Sync up */
			
			la		r10,CSAsr-4(r31)				/* Point to SR 0  - 4 */
			li		r9,0							/* Start at SR 0 */

LoadSRs:	lwz	r8,4(r10)						/* Get the next SR in line */
	addi	r10,	r10,4
			mtsrin	r8,r9							/* Load up the SR */
			addis	r9,r9,0x1000					/* Bump to the next SR */
			mr.		r9,r9							/* See if we wrapped back to 0 */
			bne+	LoadSRs							/* Not yet... */
						
			lwz		r0,CSAgpr+(0*4)(r31)			/* Get a GPR */
			mtsprg	0,r12							/* Set the SPRG0 (per_proc_into) value */
			lwz		r1,CSAgpr+(1*4)(r31)			/* Get a GPR */
			mtsrr1	r13								/* Set the MSR to dispatch */
			lwz		r2,CSAgpr+(2*4)(r31)			/* Get a GPR */
			mtsrr0	r14								/* Set the PC to dispatch */
			lwz		r3,CSAgpr+(3*4)(r31)			/* Get a GPR */
			lwz		r4,CSAgpr+(4*4)(r31)			/* Get a GPR */
			lwz		r5,CSAgpr+(5*4)(r31)			/* Get a GPR */
			lwz		r6,CSAgpr+(6*4)(r31)			/* Get a GPR */
			lwz		r7,CSAgpr+(7*4)(r31)			/* Get a GPR */
			lwz		r8,CSAgpr+(8*4)(r31)			/* Get a GPR */
			lwz		r9,CSAgpr+(9*4)(r31)			/* Get a GPR */
			lwz		r10,CSAgpr+(10*4)(r31)			/* Get a GPR */
			lwz		r11,CSAgpr+(11*4)(r31)			/* Get a GPR */
			lwz		r12,CSAgpr+(12*4)(r31)			/* Get a GPR */
			lwz		r13,CSAgpr+(13*4)(r31)			/* Get a GPR */
			lwz		r14,CSAgpr+(14*4)(r31)			/* Get a GPR */
			lwz		r15,CSAgpr+(15*4)(r31)			/* Get a GPR */
			lwz		r16,CSAgpr+(16*4)(r31)			/* Get a GPR */
			lwz		r17,CSAgpr+(17*4)(r31)			/* Get a GPR */
			lwz		r18,CSAgpr+(18*4)(r31)			/* Get a GPR */
			lwz		r19,CSAgpr+(19*4)(r31)			/* Get a GPR */
			lwz		r20,CSAgpr+(20*4)(r31)			/* Get a GPR */
			lwz		r21,CSAgpr+(21*4)(r31)			/* Get a GPR */
			lwz		r22,CSAgpr+(22*4)(r31)			/* Get a GPR */
			lwz		r23,CSAgpr+(23*4)(r31)			/* Get a GPR */
			lwz		r24,CSAgpr+(24*4)(r31)			/* Get a GPR */
			lwz		r25,CSAgpr+(25*4)(r31)			/* Get a GPR */
			lwz		r26,CSAgpr+(26*4)(r31)			/* Get a GPR */
			lwz		r27,CSAgpr+(27*4)(r31)			/* Get a GPR */
			lwz		r28,CSAgpr+(28*4)(r31)			/* Get a GPR */
			lwz		r29,CSAgpr+(29*4)(r31)			/* Get a GPR */
			lwz		r30,CSAgpr+(30*4)(r31)			/* Get a GPR */
			lwz		r31,CSAgpr+(31*4)(r31)			/* Get a GPR */
			
			sync									/* Make sure we're sunk */

			rfi										/* Get the whole shebang going... */

			.long	0
			.long	0
			.long	0
			.long	0
			.long	0
			.long	0
			.long	0
			.long	0




/*
 *			This routine handles requests to firmware from another processor.  It is actually the second level
 *			of a three level signaling protocol.  The first level is handled in the physical MP driver. It is the 
 *			basic physical control for the processor, e.g., physical stop, reset, start.  The second level (this
 *			one) handles cross-processor firmware requests, e.g., complete TLB purges.  The last are AST requests
 *			which are handled directly by mach.
 *
 *			If this code handles the request (based upon MPPICParm0BU which is valid until the next SIGP happens -
 *			actually, don't count on it once you enable) it will RFI back to the 
 *			interrupted code.  If not, it will return and let the higher level interrupt handler be called.
 *
 *			We need to worry about registers we use here, check in lowmem_vectors to see what is boten and verboten.
 *
 *			Note that there are no functions implemented yet.
 */


ENTRY(MPsignalFW, TAG_NO_FRAME_USED)


			mfspr	r7,pir							/* Get the processor address */
			lis		r6,MPPICPUs@h					/* Get high part of CPU control block array */
			rlwinm	r7,r7,5,23,26					/* Get index into CPU array */
			ori		r6,r6,MPPICPUs@h				/* Get low part of CPU control block array */
			add		r7,r7,r6						/* Point to the control block for this processor */
			lwz		r6,MPPICParm0BU(r7)				/* Just pick this up for now */
			blr										/* Leave... */



#if 0
sendword:	mflr	r7								/* (TEST/DEBUG) Save return */
			li		r6,8							/* (TEST/DEBUG) Get characters in a word */
		
sendword0:	rlwinm	r2,r2,4,0,31					/* (TEST/DEBUG) Rotate all around */
			rlwinm	r1,r2,0,28,31					/* (TEST/DEBUG) Int code 1st digit */
			lbz		r1,0x020C(r1)					/* (TEST/DEBUG) Convert to ascii */
			bl		sendchar						/* (TEST/DEBUG) Send it on out */
			
			addi	r6,r6,-1						/* (TEST/DEBUG) Count the character */
			mr.		r6,r6							/* (TEST/DEBUG) Done yet? */
			bgt+	sendword0						/* (TEST/DEBUG) Nope... */
			
			li		r1,0x20							/* (TEST/DEBUG) Get a space */
			bl		sendchar						/* (TEST/DEBUG) Send it out */
			
			mtlr	r7								/* (TEST/DEBUG) Restore return */
			blr										/* (TEST/DEBUG) Leave... */
		
sendbyte:	mflr	r7								/* (TEST/DEBUG) Save return */
	
			rlwinm	r1,r2,28,28,31					/* (TEST/DEBUG) Int code 1st digit */
			lbz		r1,0x020C(r1)					/* (TEST/DEBUG) Convert to ascii */
			bl		sendchar						/* (TEST/DEBUG) Send it on out */
		
			rlwinm	r1,r2,0,28,31					/* (TEST/DEBUG) Int code 2nd digit */
			lbz		r1,0x020C(r1)					/* (TEST/DEBUG) Convert to ascii */
			bl		sendchar						/* (TEST/DEBUG) Send it on out */
	
			li		r1,0x20							/* (TEST/DEBUG) Get a space */
			bl		sendchar						/* (TEST/DEBUG) Send it out */
			
			mtlr	r7								/* (TEST/DEBUG) Restore return */
			blr										/* (TEST/DEBUG) Return */
			
sendchar:	lis		r3,0xF301						/* (TEST/DEBUG) Top part of printer port write data */
			ori		r3,r3,0x3010					/* (TEST/DEBUG) Bottom part */
			stb		r1,0(r3)						/* (TEST/DEBUG) Send to printer port */
			dcbf	0,r3							/* (TEST/DEBUG) Force it out */
			sync									/* (TEST/DEBUG) Make sure */

			lis		r1,3							/* (TEST/DEBUG) Get enough for about 1ms */
sendchar0:	addi	r1,r1,-1						/* (TEST/DEBUG) Count down */
			mr.		r1,r1							/* (TEST/DEBUG) Waited long enough? */
			bgt+	sendchar0						/* (TEST/DEBUG) Nope... */
			blr										/* (TEST/DEBUG) Return */
#endif


/*
 *			Make space for the maximum supported CPUs in the data section
 */
			.section ".data"
			.align	5
EXT(CSA):
			.set	., .+(CSAsize*NCPUS)
			.type	EXT(CSA), @object
			.size	EXT(CSA), CSAsize*NCPUS
			.globl	EXT(CSA)
