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

#include <types.h>
#include <machine/thread_act.h>

extern void delay_init(void);

extern void fprinit(int*);
extern void doexception(int, int, int);
extern void machine_conf(void);
extern void getdumpdev(void);
extern void configure(void);
extern void autoconf(void);
extern void ioconf(void);
extern void machine_init(void);

extern unsigned long trap_instr_imm(unsigned long, unsigned long);

extern void get_root_device(void);
extern void dumpconf(void);
extern void fcacheall(void);
extern void fastboot(int);
extern void dumpsys(void);
extern void savestk(void);
extern void dispstk(void);
extern int* getpc(void);
extern int* getfp(void);
extern void doadump(void);
extern int ffset(unsigned int);
extern void grfconfig(void);
extern void Load_context(thread_t);
extern void kern_print(int, struct hp700_saved_state *);
extern vm_offset_t utime_map(dev_t, vm_offset_t, int);

extern void dump_ssp (struct hp700_saved_state *);

#if MACH_DEBUG
extern void stack_statistics(unsigned int *, vm_size_t *);
extern void dump_fp(struct hp700_float_state *);
extern void dump_pcb(pcb_t);
extern void dump_thread(thread_t);
#endif 
