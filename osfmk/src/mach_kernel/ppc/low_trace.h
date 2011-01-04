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
 *
 *	These are the structures and constants used for the low-level trace
 */






#ifndef _LOW_TRACE_H_
#define _LOW_TRACE_H_

typedef struct LowTraceRecord {

	unsigned short	LTR_cpu;			/* 0000 - CPU address */
	unsigned short	LTR_excpt;			/* 0002 - Exception code */
	unsigned int	LTR_timeHi;			/* 0004 - High order time */
	unsigned int	LTR_timeLo;			/* 0008 - Low order time */
	unsigned int	LTR_cr;				/* 000C - CR */
	unsigned int	LTR_srr0;			/* 0010 - SRR0 */
	unsigned int	LTR_srr1;			/* 0014 - SRR1 */
	unsigned int	LTR_dar;			/* 0018 - DAR */
	unsigned int	LTR_dsisr;			/* 001C - DSISR */
	
	unsigned int	LTR_lr;				/* 0020 - LR */
	unsigned int	LTR_ctr;			/* 0024 - CTR */
	unsigned int	LTR_r0;				/* 0028 - R0 */
	unsigned int	LTR_r1;				/* 002C - R1 */
	unsigned int	LTR_r2;				/* 0030 - R2 */
	unsigned int	LTR_r3;				/* 0034 - R3 */
	unsigned int	LTR_r4;				/* 0038 - R4 */
	unsigned int	LTR_r5;				/* 003C - R5 */

} LowTraceRecord;		


#define CutTrace 0x80000000

extern unsigned int traceStart, traceEnd, traceCurr;


#endif /* ifndef _LOW_TRACE_H_ */
