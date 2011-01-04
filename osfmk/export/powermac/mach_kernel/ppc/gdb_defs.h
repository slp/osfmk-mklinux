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

#ifndef	_PPC_GDB_DEFS_H_
#define	_PPC_GDB_DEFS_H_

#include <mach/boolean.h>
/*
 * GDB DEPENDENT DEFINITIONS
 *
 * The following definitions match data descriptions in the gdb source file
 * gdb/config/rs6000/tm-rs6000.h (which is used by powerpc/tm-powerpc-eabi.h)
 */

typedef struct {
	unsigned int	r0;
	unsigned int	r1;
	unsigned int	r2;
	unsigned int	r3;
	unsigned int	r4;
	unsigned int	r5;
	unsigned int	r6;
	unsigned int	r7;
	unsigned int	r8;
	unsigned int	r9;
	unsigned int	r10;
	unsigned int	r11;
	unsigned int	r12;
	unsigned int	r13;
	unsigned int	r14;
	unsigned int	r15;
	unsigned int	r16;
	unsigned int	r17;
	unsigned int	r18;
	unsigned int	r19;
	unsigned int	r20;
	unsigned int	r21;
	unsigned int	r22;
	unsigned int	r23;
	unsigned int	r24;
	unsigned int	r25;
	unsigned int	r26;
	unsigned int	r27;
	unsigned int	r28;
	unsigned int	r29;
	unsigned int	r30;
	unsigned int    r31;

	double		f0;
	double		f1;
	double		f2;
	double		f3;
	double		f4;
	double		f5;
	double		f6;
	double		f7;
	double		f8;
	double		f9;
	double		f10;
	double		f11;
	double		f12;
	double		f13;
	double		f14;
	double		f15;
	double		f16;
	double		f17;
	double		f18;
	double		f19;
	double		f20;
	double		f21;
	double		f22;
	double		f23;
	double		f24;
	double		f25;
	double		f26;
	double		f27;
	double		f28;
	double		f29;
	double		f30;
	double	    	f31;

	unsigned int	srr0; /* named iar in gdb.. should be ok? */
	unsigned int	srr1; /* named msr in gdb.. hrmm.. */
	unsigned int	cr;
	unsigned int    lr;
	unsigned int	ctr;
	unsigned int	xer;
	unsigned int	mq;   /* gdb supports this for the 601 */


	unsigned char	reason; /* Stores gdb reason for thread suspension */
} kgdb_regs_t;

/* Some (almost) machine independant code (eg. kgdb_entry.c) needs to
 * be able to modify an "instruction pointer register" which must be
 * defined. We use, by convention, the name of the register on the intel
 * platform.
 */

#define eip srr0	/* in fact, this is the saved iar */


#define REGISTER_BYTES (sizeof(kgdb_regs_t)) /* Should be 420 */

/* Provide the byte offset into kgdb_regs_t for register number num */
/* The PowerPc complicates life in that registers 32-63 are doubles */

#define REGISTER_OFFSET(reg) ( (reg < 32) ? (reg * sizeof(int))		     \
			      : ((reg < 64) ? ((reg - 32) * sizeof (double)+ \
					       (32 * sizeof (int)))	     \
				 : ((reg * sizeof(int)) +		     \
				    (32 * (sizeof(double)-sizeof(int))))))

extern boolean_t kgdb_kernel_in_pmap; /* Set to TRUE in ppc_init.c */

#endif	/* _PPC_GDB_DEFS_H_ */
