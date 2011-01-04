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
 *	Author: Bill Angell, Apple
 *	Date:	6/97
 *
 * exceptions and certain C functions write into a trace table which
 * can be examined via the machine 'lt' command under kdb
 */

#include <string.h>			/* For strcpy() */
#include <mach/boolean.h>
#include <machine/db_machdep.h>

#include <ddb/db_access.h>
#include <ddb/db_lex.h>
#include <ddb/db_output.h>
#include <ddb/db_command.h>
#include <ddb/db_sym.h>
#include <ddb/db_task_thread.h>
#include <ddb/db_command.h>		/* For db_option() */
#include <ddb/db_examine.h>
#include <ddb/db_expr.h>
#include <kern/thread.h>
#include <kern/task.h>
#include <mach/vm_param.h>
#include <ppc/low_trace.h>
#include <ppc/db_low_trace.h>

db_addr_t	db_low_trace_prev = 0;

/*
 *		Print out the low level trace table:
 *
 *		Displays the entry and 15 before it in newest to oldest order
 *		
 *		lt [entaddr]
 */
void db_low_trace(db_expr_t addr, int have_addr, db_expr_t count, char * modif) {

	int		c, i;
	unsigned int tempx;
	db_addr_t	next_addr;
	LowTraceRecord *xltr, *cxltr;
	unsigned char cmark;

	if(!addr) {
		addr=traceCurr-sizeof(LowTraceRecord);					/* Start at the newest */
		if((unsigned int)addr<traceStart)addr=traceEnd-sizeof(LowTraceRecord);	/* Wrap low back to high */
	}
	
	
	if((unsigned int)addr<traceStart||(unsigned int)addr>=traceEnd) {	/* In the table? */
		db_printf("address not in low memory trace table\n");	/* Tell the fool */
		return;													/* Leave... */
	}

	if((unsigned int)addr&0x0000003F) {							/* Proper alignment? */
		db_printf("address not aligned on trace entry boundary (0x40)\n");	/* Tell 'em */
		return;													/* Leave... */
	}
	
	xltr=(LowTraceRecord *)((unsigned int)addr+KERNELBASE_TEXT_OFFSET);		/* Point to the first entry to dump */

	cxltr=(LowTraceRecord *)((traceCurr==traceStart ? traceEnd : traceCurr)-sizeof(LowTraceRecord)+KERNELBASE_TEXT_OFFSET);	/* Get address of newest entry */

	db_low_trace_prev = addr;									/* Starting point */

	for(i=0; i<16; i++) {										/* Dump the 16 entries */
		
		db_printf("\n%s%08X  %1X  %08X %08X - %04X\n", (xltr!=cxltr ? " " : "*"), 
			(unsigned int)xltr-KERNELBASE_TEXT_OFFSET,
			xltr->LTR_cpu, xltr->LTR_timeHi, xltr->LTR_timeLo, 
			(xltr->LTR_excpt&0x8000 ? 0xFFFF : xltr->LTR_excpt*64));	/* Print the first line */
		db_printf("              %08X %08X %08X %08X %08X %08X %08X\n",
			xltr->LTR_cr, xltr->LTR_srr0, xltr->LTR_srr1, xltr->LTR_dar, xltr->LTR_dsisr, xltr->LTR_lr, xltr->LTR_ctr);
		db_printf("              %08X %08X %08X %08X %08X %08X\n",
			xltr->LTR_r0, xltr->LTR_r1, xltr->LTR_r2, xltr->LTR_r3, xltr->LTR_r4, xltr->LTR_r5);

		--xltr;													/* Back it on up */
		if((unsigned int)xltr-KERNELBASE_TEXT_OFFSET<traceStart)
			xltr=(LowTraceRecord *)(traceEnd-sizeof(LowTraceRecord)+KERNELBASE_TEXT_OFFSET);	/* Wrap low back to high */

	}
	db_next = (db_expr_t)((unsigned int)xltr-KERNELBASE_TEXT_OFFSET);
	return;
}
