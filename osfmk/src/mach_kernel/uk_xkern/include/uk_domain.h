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
 * Header files w.r.t. the MACH Kernel Domain
 */

#ifndef uk_domain_h
#define uk_domain_h

#include <cpus.h>
#include <xk_debug.h>
#include <norma_ether.h>
#include <mach_assert.h>
#include <mach_kdb.h>
#include <mach_kgdb.h>
#include <kgdb.h>
#include <stdarg.h>
#include <string.h>
#include <types.h>      /* mach_kernel/sys/types.h */
#include <kern/kalloc.h>
#include <kern/thread.h>
#include <kern/lock.h>
#include <kern/syscall_subr.h>
#include <kern/misc_protos.h>
#include <uk_xkern/include/utils.h>
#include <uk_xkern/include/uk_process.h>
#include <uk_xkern/include/assert.h>

#define         XKSTATS   (XK_DEBUG || (MACH_ASSERT && (MACH_KDB || MACH_KGDB || KGDB)))

#endif /* uk_domain_h */
