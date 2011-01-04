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

#ifndef	_i386_TGDB_H
#define	_i386_TGDB_H

#undef	FLUSH_CACHE


/*
 * Total amount of space needed to store our copies of the machine's
 * register state.  This corresponds to a #define in the gdb sources.
 */
#define NUM_REGS 16

#define REGISTER_BYTES (NUM_REGS * 4)

struct gdb_registers {

  /*
   * NOTE: This structure *must* conform exactly to gdb's register
   * layout (in gdb/config/i386/tm-i386.c).  So you can't change that
   * without changing this.  The *names* in this struct correspond to
   * what's used in the microkernel.  
   */
        unsigned int    eax;
        unsigned int    ecx;
        unsigned int    edx;
        unsigned int    ebx;
        unsigned int    esp;
        unsigned int    ebp;
        unsigned int    esi;
        unsigned int    edi;
        unsigned int    eip;
        unsigned int    efl;
        unsigned int    cs;
        unsigned int    ss;
        unsigned int    ds;
        unsigned int    es;
        unsigned int    fs;
        unsigned int    gs;

};

void tgdb_get_registers(mach_port_t thread, struct i386_thread_state *ss,
                        struct i386_float_state *fs);
void tgdb_set_registers(mach_port_t thread, struct i386_thread_state *ss,
                        struct i386_float_state *fs);
void i386ts_to_gdb(struct i386_thread_state *in, struct i386_float_state *fp,
                    struct gdb_registers *gdb);
void gdb_to_i386ts(struct gdb_registers *gdb, struct i386_thread_state *in,
                    struct i386_float_state *fp);

#endif
