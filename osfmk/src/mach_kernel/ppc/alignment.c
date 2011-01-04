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
 * Copyright 1991-1998 by Apple Computer, Inc. 
 *              All Rights Reserved 
 *  
 * Permission to use, copy, modify, and distribute this software and 
 * its documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appears in all copies and 
 * that both the copyright notice and this permission notice appear in 
 * supporting documentation. 
 *  
 * APPLE COMPUTER DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE. 
 *  
 * IN NO EVENT SHALL APPLE COMPUTER BE LIABLE FOR ANY SPECIAL, INDIRECT, OR 
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT, 
 * NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION 
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
 */
/*
 * MkLinux
 */

#include <mach_kgdb.h>
#include <debug.h>
#include <kern/thread.h>
#include <mach/thread_status.h>
#include <mach/boolean.h>
#include <kern/misc_protos.h>
#include <kgdb/kgdb_defs.h>	/* for kgdb_printf */
#include <ppc/proc_reg.h>
#include <ppc/fpu_protos.h>
#include <ppc/misc_protos.h>
#include <ppc/trap.h>
#include <ppc/spl.h>

#if DEBUG
/* These variable may be used to keep track of alignment exceptions */
int alignment_exception_count_user;
int alignment_exception_count_kernel;
#endif

#define	_AINST(x)	boolean_t  align_##x##(unsigned long dsisr,\
					       struct ppc_saved_state *ssp, \
					       struct ppc_float_state *fsp, \
					       unsigned long *align_buffer, \
					       unsigned long dar)


#define	_AFENTRY(name, r, b)	{ #name, align_##name##, r, b, TRUE }
#define	_AENTRY(name, r, b)	{ #name, align_##name##, r, b, FALSE }
#define	_ANIL			{ NULL, NULL, 0, 0 }

_AINST(lwz);
_AINST(stw);
_AINST(lhz);
_AINST(lha);
_AINST(sth);
_AINST(lfd);
_AINST(stfd);
_AINST(lwzu);
_AINST(stwu);
_AINST(lhzu);
_AINST(lhau);
_AINST(sthu);
_AINST(lfdu);
_AINST(stfdu);
_AINST(lwbrx);
_AINST(stwbrx);
_AINST(lhbrx);
_AINST(sthbrx);
_AINST(lwzx);
_AINST(stwx);
_AINST(lhzx);
_AINST(lhax);
_AINST(sthx);
_AINST(lfdx);
_AINST(stfdx);
_AINST(lwzux);
_AINST(stwux);
_AINST(lhzux);
_AINST(lhaux);
_AINST(sthux);
_AINST(lfdux);
_AINST(stfdux);

/*
 * Routines to set and get FPU registers.
 */

void GET_FPU_REG(struct ppc_float_state *fsp,
		 unsigned long reg,
		 unsigned long *value);
void SET_FPU_REG(struct ppc_float_state *fsp,
		 unsigned long reg,
		 unsigned long *value);

__inline__ void GET_FPU_REG(struct ppc_float_state *fsp,
			    unsigned long reg,
			    unsigned long *value)
{
	value[0] = ((unsigned long *) &fsp->fpregs[reg])[0];
	value[1] = ((unsigned long *) &fsp->fpregs[reg])[1];
}

__inline__ void SET_FPU_REG(struct ppc_float_state *fsp,
			    unsigned long reg, unsigned long *value)
{
	((unsigned long *) &fsp->fpregs[reg])[0] = value[0]; 
	((unsigned long *) &fsp->fpregs[reg])[1] = value[1];
}


/*
 * Macros to load and set registers according to 
 * a given cast type.
 */

#define	GET_REG(p, reg, value, cast) \
	{ *((cast *) value) = *((cast *) (&p->r0+reg)); }
#define	SET_REG(p, reg, value, cast) \
	{ *((cast *) (&p->r0+reg)) = *((cast *) value); }

/*
 * Macros to help decode the DSISR.
 */

#define	DSISR_BITS_15_16(bits)	((bits>>15) & 0x3)
#define	DSISR_BITS_17_21(bits)	((bits>>10) & 0x1f)	
#define	DSISR_BITS_REG(bits)	((bits>>5) & 0x1f)
#define	DSISR_BITS_RA(bits)	(bits & 0x1f)


struct ppc_align_instruction {
	char		*name;
	boolean_t	(*a_instruct)(unsigned long,
				      struct ppc_saved_state *,
				      struct ppc_float_state *,
				      unsigned long *,
				      unsigned long );
	int		a_readbytes;
	int		a_writebytes;
	boolean_t	a_is_float;
} align_table00[] = {
_AENTRY(lwz, 4, 0),	/* 00 0 0000 */
_ANIL,			/* 00 0 0001 */
_AENTRY(stw, 0, 4),	/* 00 0 0010 */
_ANIL,			/* 00 0 0011 */
_AENTRY(lhz, 2, 0),	/* 00 0 0100 */
_AENTRY(lha, 2, 0),	/* 00 0 0101 */
_AENTRY(sth, 0, 2),	/* 00 0 0110 */
_ANIL,			/* 00 0 0111 - lmw */
_ANIL,			/* 00 0 1000 - lfs */
_AFENTRY(lfd, 8, 0),	/* 00 0 1001 */
_ANIL,			/* 00 0 1010 - stfs */
_AFENTRY(stfd, 0, 8),	/* 00 0 1011 */
_ANIL,			/* 00 0 1100 ?*/
_ANIL,			/* 00 0 1101 ?*/
_ANIL,			/* 00 0 1110 ?*/
_ANIL,			/* 00 0 1111 ?*/
_AENTRY(lwzu, 4, 0),	/* 00 1 0000 */
_ANIL,			/* 00 1 0001 ?*/
_AENTRY(stwu, 0, 4),	/* 00 1 0010 */
_ANIL,			/* 00 1 0011 */
_AENTRY(lhzu, 2, 0),	/* 00 1 0100 */
_AENTRY(lhau, 2, 0),	/* 00 1 0101 */
_AENTRY(sthu, 0, 2),	/* 00 1 0110 */
_ANIL,			/* 00 1 0111 - stmw */
_ANIL,			/* 00 1 1000 - lfsu */
_AFENTRY(lfdu, 8, 0),	/* 00 1 1001 */
_ANIL,			/* 00 1 1010 */
_AFENTRY(stfdu, 0, 8),	/* 00 1 1011 */
};

struct ppc_align_instruction align_table10[] = {
_ANIL,				/* 0 0000 ?*/
_ANIL,				/* 0 0001 ?*/
_ANIL,				/* 0 0010 - stwcx */
_ANIL,				/* 0 0011 ?*/
_ANIL,				/* 0 0100 ?*/
_ANIL,				/* 0 0101 ?*/
_ANIL,				/* 0 0110 ?*/
_ANIL,				/* 0 0111 ?*/
_ANIL,				/* 0 1000 - lwbrx*/
_ANIL,				/* 0 1001 ?*/
_AENTRY(stwbrx, 0, 4),		/* 0 1010 */
_ANIL,				/* 0 1011 */
_ANIL,				/* 0 1100 ?*/
_ANIL,				/* 0 1101 ?*/
_AENTRY(lhbrx, 2, 0),		/* 0 1110 */
};

struct ppc_align_instruction align_table11[] = {
_AENTRY(lwzx, 4, 0),		/* 0 0000 */
_ANIL,				/* 0 0001 ?*/
_AENTRY(stwx, 0, 4),		/* 0 0010 */
_ANIL,				/* 0 0011 */
_AENTRY(lhzx, 2, 0),		/* 0 0100 */
_AENTRY(lhax, 2, 0),		/* 0 0101 */
_AENTRY(sthx, 0, 2),		/* 0 0110 */
_ANIL,				/* 0 0111?*/
_ANIL,				/* 0 1000 - lfsx */
_AFENTRY(lfdx, 8, 0),		/* 0 1001 */
_ANIL,				/* 0 1010 - stfsx */
_AFENTRY(stfdx, 0, 8),		/* 0 1011 */
_ANIL,				/* 0 1100 ?*/
_ANIL,				/* 0 1101 ?*/
_ANIL,				/* 0 1110 ?*/
_ANIL,				/* 0 1111 ?*/
_AENTRY(lwzux, 4, 0),		/* 1 0000 */
_ANIL,				/* 1 0001 ?*/
_AENTRY(stwux, 0, 4),		/* 1 0010 */
_ANIL,				/* 1 0011 */
_AENTRY(lhzux, 4, 0),		/* 1 0100 */
_AENTRY(lhaux, 4, 0),		/* 1 0101 */
_AENTRY(sthux, 0, 4),		/* 1 0110 */
_ANIL,				/* 1 0111 ?*/
_ANIL,				/* 1 1000 - lfsux */
_AFENTRY(lfdux, 0, 8),		/* 1 1001 */
_ANIL,				/* 1 1010 */
_AFENTRY(stfdux, 0, 8),		/* 1 1011 */
};


struct ppc_align_instruction_table {
	struct ppc_align_instruction	*table;
	int				size;
} align_tables[4] = {
	align_table00, 	sizeof(align_table00)/
			sizeof(struct ppc_align_instruction),

	NULL, 		0,

	align_table10, 	sizeof(align_table10)/
			sizeof(struct ppc_align_instruction),

	align_table11, 	sizeof(align_table11)/
			sizeof(struct ppc_align_instruction)
};


/*
 * Alignment Exception Handler
 *
 *
 * This handler is called when the chip attempts
 * to execute an instruction which causes page
 * boundaries to be crossed. Typically, this will
 * happen on stfd* and lfd* instructions.
 * (A request has been made for GNU C compiler
 * NOT to make use of these instructions to 
 * load and store 8 bytes at a time.)
 *
 * This is a *SLOW* handler. There is room for vast
 * improvement. However, it is expected that alignment
 * exceptions will be very infrequent.
 *
 * Not all of the 64 instructions (as listed in 
 * PowerPC Microprocessor Family book under the Alignment
 * Exception section) are handled yet.
 * Only the most common ones which are expected to
 * happen.
 * 
 * -- Michael Burg, Apple Computer, Inc. 1996
 *
 * TODO NMGS finish handler
 */

boolean_t
alignment(unsigned long dsisr, unsigned long dar,
	       struct ppc_saved_state *ssp)
{
	struct ppc_align_instruction_table	*table;
	struct ppc_align_instruction		*entry;
	unsigned long		align_buffer[2];
	boolean_t		success;
	spl_t			s;

#if	DEBUG
	if (ssp->srr0 & MASK(MSR_PR))
		alignment_exception_count_user++;
	else
		alignment_exception_count_kernel++;
#endif

	table = &align_tables[DSISR_BITS_15_16(dsisr)];

	if (table == NULL
	|| table->size < DSISR_BITS_17_21(dsisr)) {
#if	DEBUG
		printf("EXCEPTION NOT HANDLED: Out of range.\n");
#endif
		goto out;
	}

	entry = &table->table[DSISR_BITS_17_21(dsisr)];

	if (entry->a_instruct == NULL) {
#if	DEBUG
		printf("EXCEPTION NOT HANDLED: Out of range.\n");
#endif
		goto out;
	}

	/*
	 * Check to see if the instruction is a 
	 * floating point operation. Save off
	 * the FPU register set ...
	 */

	if (entry->a_is_float)
		fpu_save();

	/*
	 * Pull in any bytes which are going to be
	 * read.
	 */

	if (entry->a_readbytes) {
		if (USER_MODE(ssp->srr1)) {
			if (copyin((char *) dar,
				   (char *) align_buffer,
				   entry->a_readbytes)) {
				return	TRUE;
			}
		} else {
			bcopy((char *) dar,
			      (char *) align_buffer,
			      entry->a_readbytes);
		}
	}

#if	0 && DEBUG
	printf("Alignment exception: %s %d,0x%x (r%d/w%d) (tmp %x/%x)\n",
	       entry->name, DSISR_BITS_REG(dsisr),
	       dar, entry->a_readbytes, entry->a_writebytes, 
	       align_buffer[0], align_buffer[1]);
#endif

	success = entry->a_instruct(dsisr,
				    ssp,
				    &current_act()->mact.pcb->fs,
				    align_buffer,
				    dar);

	if (success) {
		if (entry->a_writebytes) {
			if (USER_MODE(ssp->srr1)) {
				if (copyout((char *) align_buffer,
					    (char *) dar,
					    entry->a_writebytes)) {
					return	TRUE;
				}
			} else {
				bcopy((char *) align_buffer,
				      (char *) dar,
				      entry->a_writebytes);
			}
		}

		ssp->srr0 += 4;	/* Skip the instruction .. */
	}

	return	!success;

out:
#if	0 && DEBUG
	printf("ALIGNMENT EXCEPTION: (dsisr 0x%x) table %d 0x%x\n",
		dsisr, DSISR_BITS_15_16(dsisr), DSISR_BITS_17_21(dsisr));
#endif

	return	TRUE;
}

_AINST(lwz)
{
	SET_REG(ssp, DSISR_BITS_REG(dsisr), align_buffer, unsigned long);

	return	TRUE;
}

_AINST(stw)
{
	GET_REG(ssp, DSISR_BITS_REG(dsisr), align_buffer, unsigned long);

	return	TRUE;
}

_AINST(lhz)
{
	unsigned long value = *((unsigned short *) align_buffer);

	SET_REG(ssp, DSISR_BITS_REG(dsisr), &value, unsigned long);

	return	TRUE;
}

_AINST(lha)
{
	long value = *((short *) align_buffer);

	SET_REG(ssp, DSISR_BITS_REG(dsisr), &value, unsigned long);

	return	TRUE;
}

_AINST(sth)
{
	GET_REG(ssp, DSISR_BITS_REG(dsisr), align_buffer, unsigned short);

	return	TRUE;
}

_AINST(lfd)
{
	SET_FPU_REG(fsp, DSISR_BITS_REG(dsisr), align_buffer);
	return	TRUE;
}

_AINST(stfd)
{
	GET_FPU_REG(fsp, DSISR_BITS_REG(dsisr), align_buffer);
	return TRUE;
}

_AINST(lwzu)
{
	SET_REG(ssp, DSISR_BITS_REG(dsisr), align_buffer, unsigned long)
	SET_REG(ssp, DSISR_BITS_RA(dsisr), &dar, unsigned long);
	return TRUE;
}

_AINST(stwu)
{
	GET_REG(ssp, DSISR_BITS_REG(dsisr), align_buffer, unsigned long)
	SET_REG(ssp, DSISR_BITS_RA(dsisr), &dar, unsigned long);
	return TRUE;
}


_AINST(lhzu)
{
	SET_REG(ssp, DSISR_BITS_REG(dsisr), align_buffer, unsigned short)
	SET_REG(ssp, DSISR_BITS_RA(dsisr), &dar, unsigned long);
	return TRUE;
}

_AINST(lhau)
{
	long	value = *((short *) align_buffer);

	SET_REG(ssp, DSISR_BITS_REG(dsisr), &value, unsigned long);
	SET_REG(ssp, DSISR_BITS_RA(dsisr), &dar, unsigned long);

	return	TRUE;
}

_AINST(sthu)
{
	GET_REG(ssp, DSISR_BITS_REG(dsisr), align_buffer, unsigned short)
	SET_REG(ssp, DSISR_BITS_RA(dsisr), &dar, unsigned long);
	return	TRUE;
}

_AINST(lfdu)
{
	SET_FPU_REG(fsp, DSISR_BITS_REG(dsisr), align_buffer);
	SET_REG(ssp, DSISR_BITS_RA(dsisr), &dar, unsigned long);

	return	TRUE;
}

_AINST(stfdu)
{
	GET_FPU_REG(fsp, DSISR_BITS_REG(dsisr), align_buffer);
	SET_REG(ssp, DSISR_BITS_RA(dsisr), &dar, unsigned long);

	return	TRUE;
}

_AINST(lwbrx)
{
	unsigned long 	new_value;

	__asm__ volatile("lwbrx	%0,0,%1" : : "b" (new_value),
			"b" (&align_buffer[0]));

	SET_REG(ssp, DSISR_BITS_REG(dsisr), &new_value, unsigned long);

	return	TRUE;
}

_AINST(stwbrx)
{
	unsigned long	value;

	GET_REG(ssp, DSISR_BITS_REG(dsisr), &value, unsigned long);
	__asm__ volatile("stwbrx	%0,0,%1" : : "b" (value), "b" (&align_buffer[0]));

	return	TRUE;
}

_AINST(lhbrx)
{
	unsigned short	value;

	__asm__ volatile("lhbrx %0,0,%1" : : "b" (value), "b" (&align_buffer[0]));

	SET_REG(ssp, DSISR_BITS_REG(dsisr), &value, unsigned short);

	return	TRUE;
}

_AINST(sthbrx)
{
	unsigned short value;

	GET_REG(ssp, DSISR_BITS_REG(dsisr), &value, unsigned short);
	__asm__ volatile("sthbrx %0,0,%1" : : "b" (value), "b" (&align_buffer[0]));

	return	TRUE;
}

_AINST(lwzx)
{
	SET_REG(ssp, DSISR_BITS_REG(dsisr), &align_buffer[0], unsigned long);

	return	TRUE;
}

_AINST(stwx)
{
	GET_REG(ssp, DSISR_BITS_REG(dsisr), &align_buffer[0], unsigned long);

	return	TRUE;
}

_AINST(lhzx)
{
	SET_REG(ssp, DSISR_BITS_REG(dsisr), &align_buffer[0], unsigned short);

	return	TRUE;
}

_AINST(lhax)
{
	long	value	= *((short *) &align_buffer[0]);

	SET_REG(ssp, DSISR_BITS_REG(dsisr), &value, unsigned long);

	return	TRUE;
}

_AINST(sthx)
{
	GET_REG(ssp, DSISR_BITS_REG(dsisr), &align_buffer[0], unsigned short);

	return	TRUE;
}

_AINST(lfdx)
{
	SET_FPU_REG(fsp, DSISR_BITS_REG(dsisr), align_buffer);

	return	TRUE;
}

_AINST(stfdx)
{
	GET_FPU_REG(fsp, DSISR_BITS_REG(dsisr), align_buffer);

	return	TRUE;
}

_AINST(lwzux)
{
	SET_REG(ssp, DSISR_BITS_REG(dsisr), &align_buffer[0], unsigned long);
	SET_REG(ssp, DSISR_BITS_RA(dsisr), &dar, unsigned long);

	return	TRUE;
}

_AINST(stwux)
{
	GET_REG(ssp, DSISR_BITS_REG(dsisr), &align_buffer[0], unsigned long);
	SET_REG(ssp, DSISR_BITS_RA(dsisr), &dar, unsigned long);

	return	TRUE;
}

_AINST(lhzux)
{
	unsigned long value = *((unsigned short *)&align_buffer[0]);

	SET_REG(ssp, DSISR_BITS_REG(dsisr), &value, unsigned long);
	SET_REG(ssp, DSISR_BITS_RA(dsisr), &dar, unsigned long);

	return	TRUE;
}

_AINST(lhaux)
{
	long value = *((short *) &align_buffer[0]);

	SET_REG(ssp, DSISR_BITS_REG(dsisr), &value, unsigned long);
	SET_REG(ssp, DSISR_BITS_RA(dsisr), &dar, unsigned long);

	return	TRUE;
}

_AINST(sthux)
{
	GET_REG(ssp, DSISR_BITS_REG(dsisr), &align_buffer[0], unsigned short);
	SET_REG(ssp, DSISR_BITS_RA(dsisr), &dar, unsigned long);

	return	TRUE;
}

_AINST(lfdux)
{
	SET_FPU_REG(fsp, DSISR_BITS_REG(dsisr), &align_buffer[0]);
	SET_REG(ssp, DSISR_BITS_RA(dsisr), &dar, unsigned long);

	return	TRUE;
}


_AINST(stfdux)
{
	GET_FPU_REG(fsp, DSISR_BITS_REG(dsisr), &align_buffer[0]);
	SET_REG(ssp, DSISR_BITS_RA(dsisr), &dar, unsigned long);

	return	TRUE;
}
