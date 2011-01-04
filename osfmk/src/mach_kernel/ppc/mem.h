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

#ifndef _PPC_MEM_H_
#define _PPC_MEM_H_

/* Various prototypes for things in/used by the low level memory
 * routines
 */

#include <mach_kdb.h>
#include <mach_kgdb.h>

#include <ppc/proc_reg.h>
#include <ppc/pmap.h>
#include <ppc/pmap_internals.h>
#include <mach/vm_types.h>

/* These statics are initialised by pmap.c */

extern vm_offset_t hash_table_base;
extern unsigned int hash_table_size;
extern vm_offset_t hash_table_mask;

void hash_table_init(vm_offset_t base, vm_offset_t size);
pte_t *find_or_allocate_pte(space_t	sid,
			    vm_offset_t v_addr,
			    boolean_t	allocate);
#if	MACH_KDB || MACH_KGDB
pte_t *db_find_or_allocate_pte(space_t	sid,
			       vm_offset_t v_addr,
			       boolean_t	allocate);
#endif	/* MACH_KDB || MACH_KGDB */

#endif /* _PPC_MEM_H_ */
