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

#ifndef _PPC_MISC_PROTOS_H_
#define _PPC_MISC_PROTOS_H_

#include <cpus.h>
#include <debug.h>
#include <mach_kdb.h>
#include <mach_kgdb.h>
#include <mach_debug.h>

#include <ppc/thread.h>
#include <ppc/boot.h>
#include <ppc/thread_act.h>
#include <mach/vm_types.h>
#include <mach/ppc/thread_status.h>
#include <stdarg.h>
#include <ppc/kgdb_defs.h>

extern int strcmp(const char *s1, const char *s2);
extern int strncmp(const char *s1, const char *s2, unsigned long n);
extern char *strcat(char *dest, const char *src);
extern char *strcpy(char *dest, const char *src);

extern void vprintf(const char *fmt, va_list args);
extern void printf(const char *fmt, ...);

extern void bcopy_nc(char *from, char *to, int size); /* uncached-safe */

extern void go(unsigned int mem_size, boot_args *args);
extern void call_kgdb_with_ctx(int type,int code,struct ppc_saved_state *ssp);
extern struct ppc_saved_state *enterDebugger(unsigned int trap,
				      struct ppc_saved_state *state,
				      unsigned int dsisr);

extern void ppc_vm_init(unsigned int mem_size, boot_args *args);
extern void regDump(struct ppc_saved_state *state);

extern void autoconf(void);
extern void machine_init(void);
extern void machine_conf(void);
extern void probeio(void);
extern int  cons_find(boolean_t);
extern void machine_startup(unsigned int memory_size, boot_args *args);

extern void interrupt_init(void);
extern void interrupt_enable(void);
extern void interrupt_disable(void);
#if	MACH_KDB
extern void db_interrupt_enable(void);
extern void db_interrupt_disable(void);
#endif	/* MACH_KDB */
extern void amic_init(void);

extern void phys_zero(vm_offset_t, vm_size_t);
extern void phys_copy(vm_offset_t, vm_offset_t, vm_size_t);

extern void Load_context(thread_t th);

extern struct thread_shuttle *Switch_context(struct thread_shuttle   *old,
				      void                    (*cont)(void),
				      struct thread_shuttle   *new);

extern vm_offset_t utime_map(dev_t dev, vm_offset_t off, int prot);

extern int nsec_to_processor_clock_ticks(int nsec);

extern void tick_delay(int ticks);

/* TODO DPRINTF will be more complex one day */
#if	DEBUG && MACH_KGDB
extern int kgdb_debug;
#define DPRINTF(x) if (kgdb_debug) { printf("%s : ",__FUNCTION__);printf x; }
#else	/* DEBUG && MACH_KGDB */
#define DPRINTF(x)
#endif	/* DEBUG && MACH_KGDB */

#if MACH_ASSERT
extern void dump_pcb(pcb_t pcb);
extern void dump_thread(thread_t th);
#endif 

#if	NCPUS > 1
extern void mp_probe_cpus(void);
#if	MACH_KDB
extern void remote_kdb(void);
extern void clear_kdb_intr(void);
extern void kdb_console(void);
#endif	/* MACH_KDB */
#endif	/* NCPUS > 1 */

#endif /* _PPC_MISC_PROTOS_H_ */
