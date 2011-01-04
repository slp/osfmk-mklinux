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
/*         $CHeader: kgdb.h 1.13 93/12/26 22:05:25 $	*/

/* 
 * Revised Copyright Notice:
 *
 * Copyright 1993, 1994 CONVEX Computer Corporation.  All Rights Reserved.
 *
 * This software is licensed on a royalty-free basis for use or modification,
 * so long as each copy of the software displays the foregoing copyright notice.
 *
 * This software is provided "AS IS" without warranty of any kind.  Convex 
 * specifically disclaims the implied warranties of merchantability and
 * fitness for a particular purpose.
 */


#ifndef	_KGDB_KGDB_H_
#define _KGDB_KGDB_H_

#include <machine/thread.h>
#include <mach/ndr.h>
#include <kgdb/types.h>

extern volatile boolean_t  kgdb_request_ready;
extern struct gdb_request  kgdb_request_buffer;
extern struct gdb_response kgdb_response_buffer;
			   
extern boolean_t	kgdb_debug_init; /* !0 waits for host at bootstrap */
extern boolean_t 	kgdb_connected;	/* do we know where responses go? */
extern boolean_t 	kgdb_active; /* is kgdb polling? */
extern boolean_t 	kgdb_stop; /* should we stop in ether intr()? */

extern int 		kgdb_dev;

unsigned int
kgdb_read_timer(void);

void
kgdb_break(void);

void
kgdb_flush_cache(vm_offset_t offset, vm_size_t length);

void
kgdb_continue(int type,
	      struct hp700_saved_state *ssp,
	      vm_offset_t addr);

void
kgdb_single_step(int type,
		 struct hp700_saved_state *ssp,
		 vm_offset_t addr);

void
kgdb_fixup_pc(int type, struct hp700_saved_state *ssp);

#endif	/* _KGDB_KGDB_H_ */

