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
 * MkLinux
 */
/* 
 * Copyright (c) 1990, 1991 The University of Utah and
 * the Center for Software Science at the University of Utah (CSS).
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software is hereby
 * granted provided that (1) source code retains these copyright, permission,
 * and disclaimer notices, and (2) redistributions including binaries
 * reproduce the notices in supporting documentation, and (3) all advertising
 * materials mentioning features or use of this software display the following
 * acknowledgement: ``This product includes software developed by the Center
 * for Software Science at the University of Utah.''
 *
 * THE UNIVERSITY OF UTAH AND CSS ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSS DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSS requests users of this software to return to css-dist@cs.utah.edu any
 * improvements that they make and grant CSS redistribution rights.
 *
 * 	Utah $Hdr: debug.h 1.3 91/09/25$
 *	Author: Bob Wheeler, University of Utah CSS
 */

/*
 *	Debug support for assembly code.
 */

#define dump_save_registers(t1,t2) \
	stw	r1,SS_R1(t1) !\
	stw	r2,SS_R2(t1) !\
	stw	r3,SS_R3(t1) !\
	stw	r4,SS_R4(t1) !\
	stw	r5,SS_R5(t1) !\
	stw	r6,SS_R6(t1) !\
	stw	r7,SS_R7(t1) !\
	stw	r8,SS_R8(t1) !\
	stw	r9,SS_R9(t1) !\
	stw	r10,SS_R10(t1) !\
	stw	r11,SS_R11(t1) !\
	stw	r12,SS_R12(t1) !\
	stw	r13,SS_R13(t1) !\
	stw	r14,SS_R14(t1) !\
	stw	r15,SS_R15(t1) !\
	stw	r16,SS_R16(t1) !\
	stw	r17,SS_R17(t1) !\
	stw	r18,SS_R18(t1) !\
	stw	r19,SS_R19(t1) !\
	stw	r20,SS_R20(t1) !\
	stw	r21,SS_R21(t1) !\
	stw	r22,SS_R22(t1) !\
	stw	r23,SS_R23(t1) !\
	stw	r24,SS_R24(t1) !\
	stw	r25,SS_R25(t1) !\
	stw	r26,SS_R26(t1) !\
	stw	r27,SS_R27(t1) !\
	stw	r28,SS_R28(t1) !\
	stw	r29,SS_R29(t1) !\
	stw	r30,SS_R30(t1) !\
	stw	r31,SS_R31(t1)

#define dump_save_spaces(t1,t2) \
	mfsp	sr0,t2 !\
	stw	t2,SS_SR0(t1) !\
	mfsp	sr1,t2 !\
	stw	t2,SS_SR1(t1) !\
	mfsp	sr2,t2 !\
	stw	t2,SS_SR2(t1) !\
	mfsp	sr3,t2 !\
	stw	t2,SS_SR3(t1) !\
	mfsp	sr4,t2 !\
	stw	t2,SS_SR4(t1) !\
	mfsp	sr5,t2 !\
	stw	t2,SS_SR5(t1) !\
	mfsp	sr6,t2 !\
	stw	t2,SS_SR6(t1) !\
	mfsp	sr7,t2 !\
	stw	t2,SS_SR7(t1)

#define dump_save_control(t1,t2) \
	mfctl	cr8,t2 !\
	stw	t2,SS_CR8(t1) !\
	mfctl	cr9,t2 !\
	stw	t2,SS_CR9(t1) !\
	mfctl	cr11,t2 !\
	stw	t2,SS_CR11(t1) !\
	mfctl	cr12,t2 !\
	stw	t2,SS_CR12(t1) !\
	mfctl	cr13,t2 !\
	stw	t2,SS_CR13(t1) !\
	mfctl	cr15,t2 !\
	stw	t2,SS_CR15(t1) !\
	mfctl	cr19,t2 !\
	stw	t2,SS_CR19(t1) !\
	mfctl	cr20,t2 !\
	stw	t2,SS_CR20(t1) !\
	mfctl	cr21,t2 !\
	stw	t2,SS_CR21(t1) !\
	mfctl	cr22,t2 !\
	stw	t2,SS_CR22(t1)
