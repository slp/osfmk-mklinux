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
/*        $CHeader: kgdb_stub.h 1.5 93/12/26 22:40:02 $ */

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

#ifndef _KGDB_KGDB_STUB_H_
#define _KGDB_KGDB_STUB_H_

#include <machine/setjmp.h>
#include <machine/thread.h>
#include <kgdb/gdb_packet.h>

extern jmp_buf_t       *kgdb_recover;

extern boolean_t 	kgdb_do_timing;
extern unsigned int 	kgdb_start_time;

/*
 * kgdb_debug_flags contains several bit fields for debugging:
 *
 *    1	kgdb_packet_input (dropped packets)
 *    2 kgdb_packet_input (packets kept)
 *    4 kgdb_get_request
 *    8 kgdb_send_packet
 *   16	kgdb_response
 *   32	kgdb_request
 *   64 kgdb_trap
 *  128	kgdb_bcopy (memory faults)
 *  256 kgdb_continue
 *  512 kgdb_single_step
 */
unsigned int kgdb_debug_flags;

void
kgdb_debug(unsigned int flag, char *format, int arg);

void
kgdb_mux_init(void);

void
kgdb_bcopy(const void *source, void *destination, unsigned int size);

void
kgdb_response(gdb_response_t response,
	      unsigned int data_size,
	      void *data);

void kgdb_trap(int type,
	       struct hp700_saved_state *ssp);

#endif /* _KGDB_KGDB_STUB_H_ */
