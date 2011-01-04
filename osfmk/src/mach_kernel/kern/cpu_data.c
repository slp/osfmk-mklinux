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
 * MkLinux
 */

#include <cpus.h>
#include <cputypes.h>

#include <kern/kern_types.h>
#include <kern/cpu_number.h>
#include <kern/cpu_data.h>
#include <kern/thread.h>
#include <machine/spl.h>

#ifdef hp_pa
cpu_data_t	cpu_data[NCPUS] = {0};
#else
#ifdef	PPC
cpu_data_t	cpu_data[NCPUS] =
	{ { THREAD_NULL,		/* active_thread */
#if	MACH_RT
	    0,				/* preemption_level */
#endif	/* MACH_RT */
	    0,				/* simple_lock_cout */
	    SPLHIGH			/* interrupt_level */
	}, };
#else	/* PPC */
cpu_data_t	cpu_data[NCPUS];
#endif	/* PPC */
#endif
