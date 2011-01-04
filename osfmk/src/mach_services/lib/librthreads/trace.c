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
 * Mach Operating System
 * Copyright (c) 1993,1992,1991,1990,1989 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
 * MkLinux
 */
/*
 * File: trace.c
 */

#include <trace.h>

#ifdef TRLOG

#include <stdio.h>
#include <rthreads.h>
#include <rthread_internals.h>

static int 	rthreads_log_ptr = 0;
unsigned int 	rthreads_tr_print_now = FALSE;

#define CLMS 80*4096
char rthreads_log[CLMS];

static void
rthreads_print_tr(
	rthreads_trace_event *cte, 
	int ti, 
	unsigned long show_extra)
{
	char *filename, *cp;
	if (cte->file == (char *) 0 || cte->funcname == (char *) 0 ||
	    cte->lineno == 0 || cte->fmt == 0) {
		printf("[%04x]\n", ti);
		return;
	}
	for (cp = cte->file; *cp; ++cp)
		if (*cp == '/')
			filename = cp + 1;
	printf("[%8x][%04x] %s", cte->sp,ti, cte->funcname);
	if (show_extra) {
		printf("(%s:%05d):\n\t", filename, cte->lineno);
	} else
		printf(":  ");
	printf(cte->fmt, cte->tag1, cte->tag2, cte->tag3, cte->tag4);
	printf("\n");
}

static void
rthreads_sprint_tr(
	rthreads_trace_event *cte, 
	int ti, 
        unsigned long show_extra)
{
	kern_return_t	r;
	char *filename, *cp;

	trace_lock();
	if (cte->file == (char *) 0 || cte->funcname == (char *) 0 ||
	    cte->lineno == 0 || cte->fmt == 0) {
		sprintf(&rthreads_log[rthreads_log_ptr],"[%04x]\n", ti);
		while(rthreads_log[rthreads_log_ptr]!='\0')
			rthreads_log_ptr++;
		trace_unlock();
		return;
	}
	for (cp = cte->file; *cp; ++cp)
		if (*cp == '/')
			filename = cp + 1;
	sprintf(&rthreads_log[rthreads_log_ptr],
		"[%8x][%04x] %s", cte->sp,ti, cte->funcname);
	while(rthreads_log[rthreads_log_ptr]!='\0')
		rthreads_log_ptr++;
	if (show_extra)
		sprintf(&rthreads_log[rthreads_log_ptr],
			"(%s:%05d):\n\t", filename, cte->lineno);
	else
		sprintf(&rthreads_log[rthreads_log_ptr], ":  ");
	while(rthreads_log[rthreads_log_ptr]!='\0')
		rthreads_log_ptr++;
	sprintf(&rthreads_log[rthreads_log_ptr],
		cte->fmt, cte->tag1, cte->tag2, cte->tag3, cte->tag4);
	while(rthreads_log[rthreads_log_ptr]!='\0')
		rthreads_log_ptr++;
	sprintf(&rthreads_log[rthreads_log_ptr],
		"\n");
	while(rthreads_log[rthreads_log_ptr]!='\0')
		rthreads_log_ptr++;
	if (rthreads_log_ptr >= CLMS-200)
		rthreads_log_ptr = 0;
	trace_unlock();
}

void 
tr(
	char *funcname, 
	char *file,
	unsigned int lineno,
	char *fmt,
	unsigned int tag1,
	unsigned int tag2,
	unsigned int tag3,
	unsigned int tag4)
{
	kern_return_t	r;
	struct rthreads_tr_struct *ctd = &rthreads_tr_data;

	if (ctd->trace_index >= TRACE_MAX)
		ctd->trace_index = 0;

	ctd->trace_buffer[ctd->trace_index].funcname = funcname;
	ctd->trace_buffer[ctd->trace_index].file = file;
	ctd->trace_buffer[ctd->trace_index].lineno = lineno;
	ctd->trace_buffer[ctd->trace_index].fmt = fmt;
	ctd->trace_buffer[ctd->trace_index].tag1 = tag1;
	ctd->trace_buffer[ctd->trace_index].tag2 = tag2;
	ctd->trace_buffer[ctd->trace_index].tag3 = tag3;
	ctd->trace_buffer[ctd->trace_index].tag4 = tag4;
	ctd->trace_buffer[ctd->trace_index].sp = rthread_sp();
	rthreads_sprint_tr(&(ctd->trace_buffer[ctd->trace_index]),
			   ctd->trace_index, 0);

	if (rthreads_tr_print_now) {
		if (trace_try_lock() != KERN_LOCK_OWNED) {
		    rthreads_print_tr(&(ctd->trace_buffer[ctd->trace_index]),
				      ctd->trace_index, 0);
		    trace_unlock();
	        }
	}
	++ctd->trace_index;
}

void
rthread_show_tr(
	struct rthreads_tr_struct *ctd,
	unsigned long	index,
	unsigned long	range,
	unsigned long	show_extra)
{
	kern_return_t	r;
	int 		i;


	if (ctd == 0) {
		ctd = &rthreads_tr_data;
		index = ctd->trace_index - (TRACE_WINDOW-4);
		range = TRACE_WINDOW;
		show_extra = 0;
	}

	if (index + range > TRACE_MAX)
		range = TRACE_MAX - index;

	for (i = index; i < index + range; ++i)
	    rthreads_print_tr(&(ctd->trace_buffer[i]),i, show_extra);
}

#else  /*TRLOG*/

int no_tracer;	/* not used: no_tracer only exists because ANSI C	*/
		/* forbids an empty source file 			*/

#endif /*TRLOG*/


