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
 	Firmware.s 

	Handle things that should be treated as an extension of the hardware

	Lovingly crafted by Bill Angell using traditional methods and only natural or recycled materials.
	No animal products are used other than rendered otter bile and deep fried pork lard.

*/

#include <ppc/asm.h>
#include <ppc/proc_reg.h>
#include <ppc/POWERMAC/mp/MPPlugIn.h>
#include <ppc/exception.h>
#include <mach/machine/vm_param.h>
#include <assym.s>

FWbase:
/*
 *			Here we generate the table of supported firmware calls 
 */
 
FWtable:

			.set	CutTrace,(.-FWtable)/4|0x80000000	/* Call number for CutTrace */
			.globl	CutTrace							/* Let everyone know 'bout it */
			.long	callUnimp							/* This was already handled in lowmem_vectors */

 #include	<ppc/FirmwareCalls.h>
 
			.set	FirmwareCnt, (.-FWtable)/4			/* Get the top number */

/*
 *			This routine handles the firmware call routine. It must be entered with IR and DR off,
 *			interruptions disabled, and in supervisor state.  Unless there is an error in the call,
 *			i.e., an invalid value in R2, return will always be made via an RFI using the state
 *			saved in the per_proc_info pointed to by SPRG 0.  Return is made via the LR in the case
 *			of a call error.  We can trash R1, R2, R3, the LR, and CR before the error return.
 *
 *			When we enter, we expect R0 to have call number, R2 to be per_proc_info, and LR
 *			to point to the return.  Also, all registers saved in per_proc_info.
 */

ENTRY(FirmwareCall, TAG_NO_FRAME_USED)
		
			mflr	r3								/* Save the return */
			bl		FWoff1							/* Get our address */
FWoff1:		rlwinm	r1,r0,2,1,29					/* Clear out bit 0 and multiply by 4 */
			mflr	r12								/* Get this for a base (we need to restore if error return) */
			cmplwi	r1,FirmwareCnt*4				/* Is it a valid firmware call number */
			subi	r12,r12,FWoff1-FWbase			/* Relocate to module base */
			ble+	goodCall						/* Yeah, it is... */
			
			mtlr	r3								/* Restore the LR */
			lwz		r12,PP_SAVE_R12(r2)				/* Restore the one register we can't trash */
			blr										/* Return for errors... */
			
goodCall:
			stw		r3,PP_TEMPWORK1(r2)				/* Save our return point */

#if 0
			li		r1,0x0A							/* (TEST/DEBUG) Linefeed */
			bl		sendchar						/* (TEST/DEBUG) Send it */
			li		r1,0x0D							/* (TEST/DEBUG) Carriage return */
			bl		sendchar						/* (TEST/DEBUG) Send it */
			li		r1,0x0D							/* (TEST/DEBUG) Carriage return */
			bl		sendchar						/* (TEST/DEBUG) Send it */
			li		r1,0x46							/* (TEST/DEBUG) 'F' */
			bl		sendchar						/* (TEST/DEBUG) Send it */
			li		r1,0x57							/* (TEST/DEBUG) 'W' */
			bl		sendchar						/* (TEST/DEBUG) Send it */
			li		r1,0x43							/* (TEST/DEBUG) 'C' */
			bl		sendchar						/* (TEST/DEBUG) Send it */
			li		r1,0x61							/* (TEST/DEBUG) 'a' */
			bl		sendchar						/* (TEST/DEBUG) Send it */
			li		r1,0x6C							/* (TEST/DEBUG) 'l' */
			bl		sendchar						/* (TEST/DEBUG) Send it */
			li		r1,0x6C							/* (TEST/DEBUG) 'l' */
			bl		sendchar						/* (TEST/DEBUG) Send it */
			li		r1,0x3A							/* (TEST/DEBUG) ':' */
			bl		sendchar						/* (TEST/DEBUG) Send it */
			li		r1,0x20							/* (TEST/DEBUG) ' ' */
			bl		sendchar						/* (TEST/DEBUG) Send it */
			mr		r2,r0							/* (TEST/DEBUG) Get the system call number */
			bl		sendword						/* (TEST/DEBUG) Send it out */
			rlwinm	r1,r0,2,1,29					/* (TEST/DEBUG) Restore R1 value */
			mfsprg	r2,	0							/* (TEST/DEBUG) Restore per_proc address */

#endif


			addi	r1,r1,FWtable-FWbase			/* Get displacement into the call table */
			lis		r3,KERNELBASE_TEXT_OFFSET@h		/* Top half of virtual-to-physical conversion */
			lwzx	r1,r1,r12						/* Pick up the address of the routine */
			
			ori		r3,r3,KERNELBASE_TEXT_OFFSET@l	/* Low half of virtual-to-physical conversion */
			sub.	r1,r1,r3						/* Get the physical address of the firmware call handler routine */
			mtlr	r1								/* Put it in the LR */
			ble-	callUnimp						/* This one was unimplimented... */
			
			lwz		r3,PP_SAVE_R3(r2)				/* Get back the first parameter - all others should be the same as passed in */
			
			stw		r13,PP_SAVE_R13(r2)				/* Save the rest of the registers */
			stw		r14,PP_SAVE_R14(r2)				/* Save the rest of the registers */
			stw		r15,PP_SAVE_R15(r2)				/* Save the rest of the registers */
			stw		r16,PP_SAVE_R16(r2)				/* Save the rest of the registers */
			stw		r17,PP_SAVE_R17(r2)				/* Save the rest of the registers */
			stw		r18,PP_SAVE_R18(r2)				/* Save the rest of the registers */
			stw		r19,PP_SAVE_R19(r2)				/* Save the rest of the registers */
			stw		r20,PP_SAVE_R20(r2)				/* Save the rest of the registers */
			stw		r21,PP_SAVE_R21(r2)				/* Save the rest of the registers */
			stw		r22,PP_SAVE_R22(r2)				/* Save the rest of the registers */
			stw		r23,PP_SAVE_R23(r2)				/* Save the rest of the registers */
			stw		r24,PP_SAVE_R24(r2)				/* Save the rest of the registers */
			stw		r25,PP_SAVE_R25(r2)				/* Save the rest of the registers */
			stw		r26,PP_SAVE_R26(r2)				/* Save the rest of the registers */
			stw		r27,PP_SAVE_R27(r2)				/* Save the rest of the registers */
			stw		r28,PP_SAVE_R28(r2)				/* Save the rest of the registers */
			stw		r29,PP_SAVE_R29(r2)				/* Save the rest of the registers */
			stw		r30,PP_SAVE_R30(r2)				/* Save the rest of the registers */
			stw		r31,PP_SAVE_R31(r2)				/* Save the rest of the registers */
			
			blrl									/* Call the routine... */

/*			Note that for this section we use R31 as per_proc instead of R2 'cause we want to optimize cache use */

			li		r30,3*32						/* Get the third line of save area */
			mfsprg	r31,0							/* Get back the per processor area */
			li		r29,2*32						/* Get second line */
			
			la		r28,PP_TEMPWORK1(r31)			/* Get the address of a scratch area */
			dcbt	r30,r31							/* Tell HW to pull in the third line */
			
			stwcx.	r9,0,r28						/* Toss any pending reservations */
			
			dcbt	r29,r31							/* Tell HW to pull in second line */
			
			lwz		r9,PP_SAVE_SR0(r31)				/* Get the interrupt time SR0 */
			
			lwz		r0,PP_SAVE_R0(r31)				/* Restore */
			li		r30,4*32						/* Get the fourth line of save area */
			lwz		r1,PP_SAVE_R1(r31)				/* Restore */
			lwz		r2,PP_SAVE_R2(r31)				/* Restore */
			mtsr	0,r9							/* Restore the interrupt time SR0 */
/*	Don't	lwz		r3,PP_SAVE_R3(r31)				; Register 3 is passed back to the caller intact */
			lwz		r29,PP_SAVE_CR(r31)				/* Get the CR */
			lwz		r4,PP_SAVE_R4(r31)				/* Restore */
			dcbt	r30,r31							/* Bring in line 4 */
			mtcr	r29								/* Restore */
			lwz		r5,PP_SAVE_R5(r31)				/* Restore */
			lwz		r29,PP_SAVE_CTR(r31)			/* Get the CTR */
			lwz		r6,PP_SAVE_R6(r31)				/* Restore */
			mtctr	r29								/* Restore */
			lwz		r7,PP_SAVE_R7(r31)				/* Restore */

			lwz		r29,PP_SAVE_LR(r31)				/* Get the LR */
			lwz		r8,PP_SAVE_R8(r31)				/* Restore */
			mtlr	r29								/* Restore */
			lwz		r9,PP_SAVE_R9(r31)				/* Restore */
			li		r30,5*32						/* Get the fifth line of save area */
			lwz		r29,PP_SAVE_XER(r31)			/* Get the XER */
			lwz		r10,PP_SAVE_R10(r31)			/* Restore */
			lwz		r11,PP_SAVE_R11(r31)			/* Restore */
			lwz		r12,PP_SAVE_R12(r31)			/* Restore */
			lwz		r13,PP_SAVE_R13(r31)			/* Restore */
			dcbt	r30,r31							/* Bring in line 5 */
			lwz		r14,PP_SAVE_R14(r31)			/* Restore */
			lwz		r15,PP_SAVE_R15(r31)			/* Restore */
			mtxer	r29								/* Restore */

			lwz		r16,PP_SAVE_R16(r31)			/* Restore */
			lwz		r17,PP_SAVE_R17(r31)			/* Restore */
			li		r30,6*32						/* Get the sixth line of save area */
			lwz		r18,PP_SAVE_R18(r31)			/* Restore */
			lwz		r19,PP_SAVE_R19(r31)			/* Restore */
			dcbt	r30,r31							/* Bring in line 6 */
			lwz		r20,PP_SAVE_R20(r31)			/* Restore */
			lwz		r21,PP_SAVE_R21(r31)			/* Restore */
			lwz		r22,PP_SAVE_R22(r31)			/* Restore */
			lwz		r23,PP_SAVE_R23(r31)			/* Restore */

			lwz		r24,PP_SAVE_R24(r31)			/* Restore */
			lwz		r25,PP_SAVE_R25(r31)			/* Restore */
			lwz		r29,PP_SAVE_SRR0(r31)			/* Get the SSR0 */
			lwz		r26,PP_SAVE_R26(r31)			/* Restore */
			mtsrr0	r29								/* Restore */
			lwz		r27,PP_SAVE_R27(r31)			/* Restore */
			lwz		r29,PP_SAVE_SRR1(r31)			/* Get the SRR1 */
			lwz		r28,PP_SAVE_R28(r31)			/* Restore */
			mtsrr1	r29								/* Restore */
			lwz		r30,PP_SAVE_R30(r31)			/* Restore */
		
			lwz		r29,PP_SAVE_R29(r31)			/* Restore */
			lwz		r31,PP_SAVE_R31(r31)			/* Restore */
			
			rfi										/* Return to firmware caller */
			.long	0								/* For old 601 bug */
			.long	0
			.long	0
			.long	0
			.long	0
			.long	0
			.long	0
			.long	0
			
	
callUnimp:	mfsprg	r2,0							/* Restore the per cpu block */
			lwz		r3,PP_TEMPWORK1(r2)				/* Restore the return address */
			lwz		r12,PP_SAVE_R12(r2)				/* Restore the one register we can't trash */
			mtlr	r3								/* Restore the LR */
			blr										/* Return for errors... */


/*
 *			This routine is used to store using a real address. It stores parmeter1 at parameter2.
 *			Used primarily for debugging
 */

ENTRY(FirmwareStoreReal, TAG_NO_FRAME_USED)

			mfmsr	r5								/* Get the MSR */
			rlwinm	r6,r5,0,28,26					/* Clear the DR bit */
			rlwinm	r6,r6,0,17,15					/* Clear the EE bit */
			mtmsr	r6								/* Turn off DR and EE */
			isync									/* Just do it */

			stw		r3,0(r4)						/* Store the word */

			mtmsr	r5								/* Turn on DR and EE */
			isync									/* Just do it */
			blr										/* Leave... */


/*
 *			This routine is used to write debug output to either the modem or printer port.
 *			parm 1 is printer (0) or modem (1); parm 2 is ID (printed directly); parm 3 converted to hex
 */

ENTRY(dbgDisp, TAG_NO_FRAME_USED)

			mr		r12,r0									/* Keep R0 pristene */
			lis		r0,dbgDispCall@h						/* Top half of dbgDispCall firmware call number */
			ori		r0,r0,dbgDispCall@l						/* Bottom half */

			sc												/* Go display the stuff */

			mr		r0,r12									/* Restore R0 */
			blr												/* Return... */
			
/*			Here's the low-level part of dbgDisp			*/

ENTRY(dbgDispLL, TAG_NO_FRAME_USED)

			mr.		r3,r3									/* See if printer or modem */
			lis		r10,0xF301								/* Set the top part */
			mr		r3,r4									/* Copy the ID parameter */
			ori		r10,r10,0x3010							/* Assume the printer (this is the normal one) */
			mflr	r11										/* Save the link register */
			beq+	dbgDprintr								/* It sure are... */
			ori		r10,r10,0x0020							/* Move it over to the modem port */
			
/*			Print the ID parameter							*/
			
dbgDprintr:	rlwinm	r3,r3,8,0,31							/* Get the first character */
			bl		dbgDchar								/* Print it */
			rlwinm	r3,r3,8,0,31							/* Get the second character */
			bl		dbgDchar								/* Print it */
			rlwinm	r3,r3,8,0,31							/* Get the third character */
			bl		dbgDchar								/* Print it */
			rlwinm	r3,r3,8,0,31							/* Get the fourth character */
			bl		dbgDchar								/* Print it */
			
			li		r3,0x20									/* Get a space for a separator */
			bl		dbgDchar								/* Print it */
			
/*			Print the data parameter						*/
			
			li		r6,8									/* Set number of hex digits to dump */
						
dbgDnext:	rlwinm	r5,r5,4,0,31							/* Rotate a nybble */
			subi	r6,r6,1									/* Back down the count */
			rlwinm	r3,r5,0,28,31							/* Isolate the last nybble */
			lbz		r3,0x020C(r3)							/* Convert to ascii */
			bl		dbgDchar								/* Print it */
			mr.		r6,r6									/* Any more? */
			bne+	dbgDnext								/* Convert 'em all... */

			li		r3,0x0A									/* Linefeed */
			bl		dbgDchar								/* Send it */
			li		r3,0x0D									/* Carriage return */
			bl		dbgDchar								/* Send it */
			
			mtlr	r11										/* Get back the return */
			dcbi	0,r10									/* Make sure the SCC port has no paradox */
			blr												/* Leave... */
			
/*			Write to whichever serial port.  Try to leave it clean, but not too hard (this is a hack) */
			
dbgDchar:	stb		r3,0(r10)								/* Send to printer port */
			dcbf	0,r10									/* Force it out */
			sync 											/* Make sure it's out there */

			lis		r7,3									/* Get enough for about 1ms */

dbgDchar0:	addi	r7,r7,-1								/* Count down */
			mr.		r7,r7									/* Waited long enough? */
			bgt+	dbgDchar0								/* Nope... */
			blr												/* Return */
			
			
/*
 *			Used for debugging to leave stuff in 0x380-0x3FF (128 bytes).
 *			Mapping is V=R.  Stores and loads are real.
 */

ENTRY(dbgCkpt, TAG_NO_FRAME_USED)

			mr		r12,r0									/* Keep R0 pristene */
			lis		r0,dbgCkptCall@h						/* Top half of dbgCkptCall firmware call number */
			ori		r0,r0,dbgCkptCall@l						/* Bottom half */

			sc												/* Go stash the stuff */

			mr		r0,r12									/* Restore R0 */
			blr												/* Return... */
			
/*			Here's the low-level part of dbgCkpt			*/

ENTRY(dbgCkptLL, TAG_NO_FRAME_USED)

			li		r12,0x380								/* Point to output area */
			li		r1,32									/* Get line size */
			dcbz	0,r12									/* Make sure we don't fetch a cache line */

			lwz		r4,0x00(r3)								/* Load up storage to checkpoint */
			
			dcbt	r1,r3									/* Start in the next line */
			
			lwz		r5,0x04(r3)								/* Load up storage to checkpoint */
			lwz		r6,0x08(r3)								/* Load up storage to checkpoint */
			lwz		r7,0x0C(r3)								/* Load up storage to checkpoint */
			lwz		r8,0x10(r3)								/* Load up storage to checkpoint */
			lwz		r9,0x14(r3)								/* Load up storage to checkpoint */
			lwz		r10,0x18(r3)							/* Load up storage to checkpoint */
			lwz		r11,0x1C(r3)							/* Load up storage to checkpoint */
			
			add		r3,r3,r1								/* Bump input */
			
			stw		r4,0x00(r12)							/* Store it */
			stw		r5,0x04(r12)							/* Store it */
			stw		r6,0x08(r12)							/* Store it */
			stw		r7,0x0C(r12)							/* Store it */
			stw		r8,0x10(r12)							/* Store it */
			stw		r9,0x14(r12)							/* Store it */
			stw		r10,0x18(r12)							/* Store it */
			stw		r11,0x1C(r12)							/* Store it */
			
			dcbz	r1,r12									/* Clear the next line */
			add		r12,r12,r1								/* Point to next output line */

			lwz		r4,0x00(r3)								/* Load up storage to checkpoint */
			lwz		r5,0x04(r3)								/* Load up storage to checkpoint */
			lwz		r6,0x08(r3)								/* Load up storage to checkpoint */
			lwz		r7,0x0C(r3)								/* Load up storage to checkpoint */
			lwz		r8,0x10(r3)								/* Load up storage to checkpoint */
			lwz		r9,0x14(r3)								/* Load up storage to checkpoint */
			lwz		r10,0x18(r3)							/* Load up storage to checkpoint */
			lwz		r11,0x1C(r3)							/* Load up storage to checkpoint */
			
			dcbt	r1,r3									/* Touch the next line */
			add		r3,r3,r1								/* Point to next input line */
				
			stw		r4,0x00(r12)							/* Store it */
			stw		r5,0x04(r12)							/* Store it */
			stw		r6,0x08(r12)							/* Store it */
			stw		r7,0x0C(r12)							/* Store it */
			stw		r8,0x10(r12)							/* Store it */
			stw		r9,0x14(r12)							/* Store it */
			stw		r10,0x18(r12)							/* Store it */
			stw		r11,0x1C(r12)							/* Store it */

			dcbz	r1,r12									/* Clear the next line */
			add		r12,r12,r1								/* Point to next output line */

			lwz		r4,0x00(r3)								/* Load up storage to checkpoint */
			lwz		r5,0x04(r3)								/* Load up storage to checkpoint */
			lwz		r6,0x08(r3)								/* Load up storage to checkpoint */
			lwz		r7,0x0C(r3)								/* Load up storage to checkpoint */
			lwz		r8,0x10(r3)								/* Load up storage to checkpoint */
			lwz		r9,0x14(r3)								/* Load up storage to checkpoint */
			lwz		r10,0x18(r3)							/* Load up storage to checkpoint */
			lwz		r11,0x1C(r3)							/* Load up storage to checkpoint */
			
			dcbt	r1,r3									/* Touch the next line */
			add		r3,r3,r1								/* Point to next input line */
				
			stw		r4,0x00(r12)							/* Store it */
			stw		r5,0x04(r12)							/* Store it */
			stw		r6,0x08(r12)							/* Store it */
			stw		r7,0x0C(r12)							/* Store it */
			stw		r8,0x10(r12)							/* Store it */
			stw		r9,0x14(r12)							/* Store it */
			stw		r10,0x18(r12)							/* Store it */
			stw		r11,0x1C(r12)							/* Store it */

			dcbz	r1,r12									/* Clear the next line */
			add		r12,r12,r1								/* Point to next output line */

			lwz		r4,0x00(r3)								/* Load up storage to checkpoint */
			lwz		r5,0x04(r3)								/* Load up storage to checkpoint */
			lwz		r6,0x08(r3)								/* Load up storage to checkpoint */
			lwz		r7,0x0C(r3)								/* Load up storage to checkpoint */
			lwz		r8,0x10(r3)								/* Load up storage to checkpoint */
			lwz		r9,0x14(r3)								/* Load up storage to checkpoint */
			lwz		r10,0x18(r3)							/* Load up storage to checkpoint */
			lwz		r11,0x1C(r3)							/* Load up storage to checkpoint */
			
			stw		r4,0x00(r12)							/* Store it */
			stw		r5,0x04(r12)							/* Store it */
			stw		r6,0x08(r12)							/* Store it */
			stw		r7,0x0C(r12)							/* Store it */
			stw		r8,0x10(r12)							/* Store it */
			stw		r9,0x14(r12)							/* Store it */
			stw		r10,0x18(r12)							/* Store it */
			stw		r11,0x1C(r12)							/* Store it */
			
			blr


#if 0
/*
 *			(TEST/DEBUG)
 */



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

			lis		r1,3							/* (TEST/DEBUG) Get enough for about 1ms */
sendchar0:	addi	r1,r1,-1						/* (TEST/DEBUG) Count down */
			mr.		r1,r1							/* (TEST/DEBUG) Waited long enough? */
			bgt+	sendchar0						/* (TEST/DEBUG) Nope... */
			blr										/* (TEST/DEBUG) Return */
#endif
