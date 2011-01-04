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

#ifndef	_HP700_TGDB_H
#define	_HP700_TGDB_H

#define	FLUSH_INFERIOR_CACHE

/*
 * Total amount of space needed to store our copies of the machine's
 * register state.  This corresponds to a #define in the gdb sources.
 */
#define NUM_REGS 124

#define REGISTER_BYTES (NUM_REGS * 4)

struct gdb_registers {

  /*
   * NOTE: This structure *must* conform exactly to gdb's register
   * layout (in gdb/config/pa/tm-hppa.c).  So you can't change that
   * without changing this.  The *names* in tis struct correspond to
   * what's used in the microkernel.  Also, all of the fpregs in here
   * are doubles, whereas gdb refers to fr0-fr3 as their unsigned int
   * values (i.e. gdb has eight fields of 32 bits whereas this has 4
   * fields of 64 bits).  In addition, the float registers are stored
   * here as long longs instead of doubles.  This allows us to use the
   * scalar regs for the copies.
   */

  unsigned	flags;
  unsigned	r1;
  unsigned	rp;		/* r2 - return pointer			*/
  unsigned	r3;
  unsigned	r4;
  unsigned	r5;
  unsigned	r6;
  unsigned	r7;
  unsigned	r8;
  unsigned	r9;
  unsigned	r10;
  unsigned	r11;
  unsigned	r12;
  unsigned	r13;
  unsigned	r14;
  unsigned	r15;
  unsigned	r16;
  unsigned	r17;
  unsigned	r18;
  unsigned	t4;		/* r19 - temp #4                       */
  unsigned	t3;		/* r20 - temp #3                       */
  unsigned	t2;		/* r21 - temp #2                       */
  unsigned	t1;		/* r22 - temp #1                       */
  unsigned	arg3;		/* r23 - argument #3			*/
  unsigned	arg2;		/* r24 - argument #2			*/
  unsigned	arg1;		/* r25 - argument #1			*/
  unsigned	arg0;		/* r26 - argument #0			*/
  unsigned	dp;		/* r27 - data pointer			*/
  unsigned	ret0;		/* r28 - return value 0			*/
  unsigned	ret1;		/* r29 - return value 1			*/
  unsigned	sp;		/* r30 - stack pointer			*/
  unsigned	r31;
  unsigned	sar;		/* cr11 - shift amount register */
  unsigned	iioq_head;
  unsigned	iisq_head;
  unsigned	iioq_tail;
  unsigned	iisq_tail;
  unsigned	eiem;		/* cr15 - external interrupt mask */
  unsigned	iir;		/* cr19 - interrupt instruction register */
  unsigned	isr;		/* cr20 - interrupt space register */
  unsigned	ior;		/* cr21 - interrupt offset register */
  unsigned	ipsw;		/* cr22 - interrupt psw */
  unsigned	sr4;
  unsigned	sr0;
  unsigned	sr1;
  unsigned	sr2;
  unsigned	sr3;
  unsigned	sr5;
  unsigned	sr6;
  unsigned	sr7;
  unsigned	rctr;		/* cr0 - recovery counter */
  unsigned	pidr1;		/* cr8 - protection id 1 */
  unsigned	pidr2;		/* cr9 - protection id 2 */
  unsigned	ccr;		/* cr10 - not used in Mach */
  unsigned	pidr3;		/* cr12 - protection id 3 */
  unsigned	pidr4;		/* cr13 - protection id 4 */
  unsigned	isp;
  unsigned	thrd;
  unsigned	usr0;		/* cr26 - user thread register */
  unsigned      fpu;
  double 	fr0;
  double 	fr1;
  double 	fr2;
  double 	fr3;
  double 	farg0;		/* fr4 - fp argument # 0 */
  double 	farg1;		/* fr5 - fp argument # 1 */
  double 	farg2;		/* fr6 - fp argument # 2 */
  double 	farg3;		/* fr7 - fp argument # 3 */
  double 	fr8;
  double 	fr9;
  double 	fr10;
  double 	fr11;
  double 	fr12;
  double 	fr13;
  double 	fr14;
  double 	fr15;
  double 	fr16;
  double 	fr17;
  double 	fr18;
  double 	fr19;
  double 	fr20;
  double 	fr21;
  double 	fr22;
  double 	fr23;
  double 	fr24;
  double 	fr25;
  double 	fr26;
  double 	fr27;
  double 	fr28;
  double 	fr29;
  double 	fr30;
  double  	fr31;
};

void tgdb_get_registers(mach_port_t thread, struct hp700_thread_state *ss,
                        struct hp700_float_state *fs);
void tgdb_set_registers(mach_port_t thread, struct hp700_thread_state *ss,
                        struct hp700_float_state *fs);

void hp700ts_to_gdb(struct hp700_thread_state *hp, struct hp700_float_state *fp,
                    struct gdb_registers *gdb);
void gdb_to_hp700ts(struct gdb_registers *gdb, struct hp700_thread_state *hp,
                    struct hp700_float_state *fp);

#endif /* HP700_TGDB_H */

