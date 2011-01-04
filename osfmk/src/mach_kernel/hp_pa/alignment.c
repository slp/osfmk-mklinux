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
/*
 *  (c) Copyright 1989 HEWLETT-PACKARD COMPANY
 *
 *  To anyone who acknowledges that this file is provided "AS IS"
 *  without any express or implied warranty:
 *      permission to use, copy, modify, and distribute this file
 *  for any purpose is hereby granted without fee, provided that
 *  the above copyright notice and this notice appears in all
 *  copies, and that the name of Hewlett-Packard Company not be
 *  used in advertising or publicity pertaining to distribution
 *  of the software without specific, written prior permission.
 *  Hewlett-Packard Company makes no representations about the
 *  suitability of this software for any purpose.
 */

/*
 * Copyright (c) 1990,1991,1992,1994 The University of Utah and
 * the Computer Systems Laboratory (CSL).  All rights reserved.
 *
 * THE UNIVERSITY OF UTAH AND CSL PROVIDE THIS SOFTWARE IN ITS "AS IS"
 * CONDITION, AND DISCLAIM ANY LIABILITY OF ANY KIND FOR ANY DAMAGES
 * WHATSOEVER RESULTING FROM ITS USE.
 *
 * CSL requests users of this software to return to csl-dist@cs.utah.edu any
 * improvements that they make and grant CSL redistribution rights.
 *
 * 	Utah $Hdr: alignment.c 1.49 94/12/14$
 */

int align_index[] = {
	0x0,		/* no alignment	    */
	0x1,		/* 2-byte alignment */
	0x3,		/* 4-byte alignment */
	0xf		/* f-byte alignment */
	};

int align_ldst[] = {
	0x0,		/* no alignment	    */
	0x1,		/* 2-byte alignment */
	0x3,		/* 4-byte alignment */
	0x3		/* 4-byte alignment */
	};

int align_coproc[] = {
	0x3,		/* 4-byte alignment */
	0x7		/* 8-byte alignment */
	};


#ifdef	HPOSFCOMPAT
#include <kern/thread.h>
#include <mach/exception.h>
#include <mach/vm_param.h>
#include <machine/trap.h>
#include <machine/psw.h>
#include <machine/regs.h>
#include <machine/spl.h>
#include <machine/break.h>
#include <mach_kdb.h>
#include <machine/pdc.h>
#include <machine/iomod.h>
#include <machine/thread_act.h>

#include <kern/misc_protos.h>
#include <hp_pa/misc_protos.h>

/*
 * Forwards.
 */
int align_load(unsigned long, unsigned long *, int);
int align_load_modify(unsigned long, unsigned long *);
int align_store(unsigned long, unsigned long *, int);
int align_store_modify(unsigned long, unsigned long *);
int align_load_indexed(unsigned long, unsigned long *);
int align_indexed_cload(unsigned long, unsigned long *, int);
int align_indexed_cstore(unsigned long, unsigned long *, int);
int align_short_cload(unsigned long, unsigned long *, int);
int align_short_cstore(unsigned long, unsigned long *, int);
int align_short_load(unsigned long, unsigned long *, int);
int align_short_store(unsigned long, unsigned long *, int);
int align_indexed_load(unsigned long, unsigned long *, int);

/*
 * aligment_fault - Decipher aligment fault.
 *
 * Data memory protection traps and unaligned reference traps come
 * in on the same trap number.  This routine is passed the save state
 * and figures out if an aligment fault occurred.
 */
boolean_t
alignment_fault(struct hp700_saved_state *ssp)
{
    boolean_t result = FALSE;

    if ((OPCODE_IS_STORE(ssp->iir)) || (OPCODE_IS_LOAD(ssp->iir))) {
	switch (OPCODE(ssp->iir)) {

	case OP_LDH:
	case OP_STH:
	    if (ssp->ior & 0x1) {
		result = TRUE;
	    }
	    break;

	case OP_LDW:
	case OP_LDWM:
	case OP_STW:
	case OP_STWM:
	    if (ssp->ior & 0x3) {
		result = TRUE;
	    }
	    break;

	case OP_LDSTX:
	    if (ssp->ior & align_index[SIZE_OP_LDSTX(ssp->iir)]) {
		result = TRUE;
	    }
	    break;
	case OP_COWD :
	case OP_CODWD :
	    if (ssp->ior & align_coproc[SIZE_OP_COPROC(ssp->iir)]) {
		result = TRUE;
	    }
	    break;			
	}
    }
    return(result);
}

/*
 * handle_alignment_fault - Handle user alignment fault.
 */
int
handle_alignment_fault(struct hp700_saved_state *ssp)
{
	int		result;
        unsigned long	opcode = ssp->iir;

	if (!alignment_fault(ssp))
		return(0);
	
        switch (OPCODE(opcode)) {
		
        case OP_LDW:
                result = align_load(opcode, (unsigned long*)ssp, 4);
                break;
		
        case OP_LDH:
                result = align_load(opcode, (unsigned long*)ssp, 2);
                break;
		
        case OP_LDWM:
                result = align_load_modify(opcode, (unsigned long*)ssp);
		break;

        case OP_STW:
                result = align_store(opcode, (unsigned long*)ssp, 4);
                break;
		
        case OP_STH:
                result = align_store(opcode, (unsigned long*)ssp, 2);
                break;

        case OP_STWM:
                result = align_store_modify(opcode, (unsigned long*)ssp);
		break;

        case OP_LDSTX:
                result = align_load_indexed(opcode, (unsigned long*)ssp);
		break;

	case OP_COWD:
                switch (OP_EXT_COPROC(opcode)) {
                case POP_CLDWX :
                        result = align_indexed_cload(opcode, (unsigned long*)ssp, 2);
                        break;
                case POP_CSTWX :
                        result = align_indexed_cstore(opcode, (unsigned long*)ssp, 2);
                        break;
                case POP_CLDWS :
                        result = align_short_cload(opcode, (unsigned long*)ssp, 4);
                        break;
                case POP_CSTWS :
                        result = align_short_cstore(opcode, (unsigned long*)ssp, 4);
                        break;
                default :
                        panic("alignment_handle : unknown COWD");
                        break;
                }
                break;
        case OP_CODWD :
                switch (OP_EXT_COPROC(opcode)) {
                case POP_CLDDX :
                        result = align_indexed_cload(opcode, (unsigned long*)ssp, 3);
                        break;
                case POP_CSTDX :
                        result = align_indexed_cstore(opcode, (unsigned long*)ssp, 3);
                        break;
                case POP_CLDDS :
                        result = align_short_cload(opcode, (unsigned long*)ssp, 8);
                        break;
                case POP_CSTDS :
                        result = align_short_cstore(opcode, (unsigned long*)ssp, 8);
                        break;
                default :
                        panic("alignment_handle : unknown CODWD");
                        break;
                }
                break;
	}
	
	if (!result) {
		ssp->ipsw |= PSW_N;
		return(1);
	}
        return(0);
}

/*
 * trap_instr_imm - Convert 2s complement (with sign bit on rhs) into integer.
 *
 * Mask specifies the size of the immediate.
 */
unsigned long
trap_instr_imm(
	unsigned long i,
	unsigned long mask)
{
	if (i & 0x1) 
                return(-(((~(i >> 1)) + 1) & mask));
	else 
		return((i >> 1) & mask);
}

/* 
 * align_load - Handle regular load alignment fault.
 */
int
align_load(
	unsigned long	opcode,
	unsigned long	*ssp,		/* XXX */
	int		size)
{
	char *reg_ptr;

	reg_ptr = (char *) &ssp[OPCODE_LMEM_TARGET(opcode)];
	
	if (size == 2) {
		/*
	 	 * Halfword load zeros upper 16 bits.
		 */
		bzero(reg_ptr, 4);
		reg_ptr += 2;
	}
        return(copyin((const char*)(ssp[OPCODE_LMEM_BASE(opcode)] +
		       trap_instr_imm(OPCODE_LMEM_IMM(opcode), OPCODE_L_IMASK)),
		      reg_ptr, size));
}

/* 
 * align_load_modify - Handle modified load alignment fault.
 */
int
align_load_modify(
	unsigned long	opcode,
	unsigned long	*ssp)		/* XXX */
{
        int		imm, result;
	unsigned long	addr;

	imm  = trap_instr_imm(OPCODE_LMEM_IMM(opcode), OPCODE_L_IMASK);
	addr = ssp[OPCODE_LMEM_BASE(opcode)];

	if (imm < 0)		/* Pre-decrement */
		addr += imm;

	result = copyin((const char*)addr, (char*)&ssp[OPCODE_LMEM_TARGET(opcode)], 4);

	if (imm > 0)		/* Post-increment */
		addr += imm;

	ssp[OPCODE_LMEM_BASE(opcode)] = addr;
	return(result);
}

/* 
 * align_store_modify - Handle modified store alignment fault.
 */
int
align_store_modify(
	unsigned long	opcode,
	unsigned long	*ssp)		/* XXX */
{
        int		imm, result;
	unsigned long	addr;

	imm  = trap_instr_imm(OPCODE_LMEM_IMM(opcode), OPCODE_L_IMASK);
	addr = ssp[OPCODE_LMEM_BASE(opcode)];
	
	if (imm < 0)		/* Pre-decrement */
		addr += imm;

	result = copyout((const char*)&ssp[OPCODE_LMEM_SOURCE(opcode)], (char*)addr, 4);
	
	if (imm > 0)		/* Post-increment */
		addr += imm;

	ssp[OPCODE_LMEM_BASE(opcode)] = addr;
	return(result);
}

/* 
 * trap_align_store - Handle regular store alignment fault.
 */
int
align_store(
	unsigned long 	opcode,
	unsigned long	*ssp,		/* XXX */
	int		size)
{
        return(copyout((const char*)((unsigned long)(&ssp[OPCODE_LMEM_SOURCE(opcode)]) +
			(4 - size)),
		       (char*)(ssp[OPCODE_LMEM_BASE(opcode)] +
			trap_instr_imm(OPCODE_LMEM_IMM(opcode),
				       OPCODE_L_IMASK)),
		       size));
}

int
align_load_indexed(
	unsigned long 	opcode,
	unsigned long	*ssp)		/* XXX */
{
	int 	result;
	
	if (BIT_19(opcode)) {
		switch (OP_EXT4_TYPE(opcode)) {
			
		case OP_EXT4_LDWS:
			result = align_short_load(opcode, ssp, 4);
			break;
			
		case OP_EXT4_LDHS:
			result = align_short_load(opcode, ssp, 2);
			break;
			
		case OP_EXT4_STWS :
			result = align_short_store(opcode, ssp, 4);
			break;
			
		case OP_EXT4_STHS :
			result = align_short_store(opcode, ssp, 2);
			break;
		}
	} else {
		switch (OP_EXT4_TYPE(opcode)) {
		case OP_EXT4_LDWX :
			result = align_indexed_load(opcode, ssp, 2);
			break;
		case OP_EXT4_LDHX :
			result = align_indexed_load(opcode, ssp, 1);
			break;
		} 
	}
	return(result);
}

/*
 * trap_align_short_load - Handle short load alignment fault.
 */
int
align_short_load(
	unsigned long	opcode,
	unsigned long	*ssp,		/* XXX */
	int 		size)
{
        int 		imm, result;
        unsigned long 	from;
	char 	*reg_ptr;

        imm  = trap_instr_imm(OPCODE_SLMEM_IMM(opcode), OPCODE_S_IMASK);
        from = ssp[OPCODE_SLMEM_BASE(opcode)];
	
	/* Modify base register before */
        if ((OPCODE_M_UPDATE(opcode) && OPCODE_A_UPDATE(opcode)) ||
	    (!OPCODE_M_UPDATE(opcode)))
                from += imm;

	reg_ptr = (char *)&ssp[OPCODE_SLMEM_TARGET(opcode)];

	if (size == 2) {
		/*
	 	 * Halfword load zeros upper 16 bits.
		 */
		bzero(reg_ptr, 4);
		reg_ptr += 2;
	}
	result = copyin((const char*)from, reg_ptr, size);
	
        if (!result && OPCODE_M_UPDATE(opcode)) {
		/* Modify base register after */		
                if (!OPCODE_A_UPDATE(opcode))
                        from += imm;
                ssp[OPCODE_SLMEM_BASE(opcode)] = from;
        }
        return(result);
}

/*
 * align_short_store - Handle short store alignment fault.
 */
int
align_short_store(
	unsigned long	opcode,
	unsigned long	*ssp,		/* XXX */
	int 		size)
{
        int 		imm, result;
        unsigned long 	to;

        imm = trap_instr_imm(OPCODE_SSMEM_IMM(opcode), OPCODE_S_IMASK);
        to  = ssp[OPCODE_SSMEM_BASE(opcode)];
	
	/* Modify base register before */
        if ((OPCODE_M_UPDATE(opcode) && OPCODE_A_UPDATE(opcode)) ||
	    (!OPCODE_M_UPDATE(opcode)))
                to += imm;

	result = copyout((const char*)((unsigned long)(&ssp[OPCODE_SSMEM_SOURCE(opcode)]) +
			  (4 - size)),
			 (char*)to, size);

	if (!result && OPCODE_M_UPDATE(opcode)) {
		/* Modify base register after */		
                if (!OPCODE_A_UPDATE(opcode))
                        to += imm;
                ssp[OPCODE_SSMEM_BASE(opcode)] = to;
        }
        return(result);
}

/*
 * align_indexed_load - Align indexed load fault.
 */
int
align_indexed_load(
	unsigned long	opcode,
	unsigned long	*ssp,		/* XXX */
	int 		size)
{
        unsigned long	index, from;
        int 		result;
	char 	*reg_ptr;

        index = (unsigned long)ssp[OPCODE_IMEM_X(opcode)];
        if (OPCODE_U_UPDATE(opcode))
                index <<= size;
	
        from = ssp[OPCODE_IMEM_BASE(opcode)];
	if (!OPCODE_M_UPDATE(opcode))		/* Pre-modify */
		from += index;

	reg_ptr = (char *)&ssp[OPCODE_IMEM_TARGET(opcode)];
	
	if (size == 1) {
		/*
	 	 * Halfword load zeros upper 16 bits.
		 */
		bzero(reg_ptr, 4);
		reg_ptr += 2;
	}
        result = copyin((const char*)from, reg_ptr, (1 << size));

        if (!result && OPCODE_M_UPDATE(opcode))	/* Post-modify and update */
                ssp[OPCODE_IMEM_BASE(opcode)] = from + index;

        return(result);
}

/*
 * align_indexed_cstore - Align indexed coprocessor store fault.
 *
 * Returns 0 if trap handled correctly.
 */
int
align_indexed_cstore(
		     unsigned long opcode,
		     unsigned long *ssp,
		     int size)
{
        unsigned long index;
        unsigned long t;
        int result;
	unsigned char *reg_ptr;
	double *fp_reg;

	fp_reg = (double*)&((pcb_t)ssp)->sf;

        index = ssp[OPCODE_CMEM_X(opcode)];
        if (OPCODE_U_UPDATE(opcode)) {
                index <<= size;
        }
        t = ssp[OPCODE_CMEM_BASE(opcode)];
	if (!OPCODE_M_UPDATE(opcode)) {         /* Pre-modify */
		t += index;
	}
	reg_ptr = (unsigned char *)&fp_reg[OPCODE_CMEM_SOURCE(opcode)];
        /*
         * Left or Right register file selection (PA89).
         */
        if ((size == 2) && (OP_COP_UID(opcode) == COP_UID_FRR)) {
                reg_ptr += 4;
        }
        if (!(result = copyout((const char*)reg_ptr, (char*)t, (1 << size))) && OPCODE_M_UPDATE(opcode))
	{   /* Post-modify and update */
                ssp[OPCODE_CMEM_BASE(opcode)] = t + index;
        }
        return(result);
}

/*
 * align_indexed_cload - Align indexed coprocessor load fault.
 *
 * Returns 0 if trap handled correctly.
 */
int
align_indexed_cload(
		    unsigned long opcode,
		    unsigned long *ssp,
		    int size)
{
        unsigned long index;
        unsigned long f;
        int result;
	char *reg_ptr;
	double *fp_reg;

	fp_reg = (double*)&((pcb_t)ssp)->sf;

        index = ssp[OPCODE_CMEM_X(opcode)];
        if (OPCODE_U_UPDATE(opcode)) {
                index <<= size;
        }
        f = ssp[OPCODE_CMEM_BASE(opcode)];
	if (!OPCODE_M_UPDATE(opcode)) {         /* Pre-modify */
		f += index;
	}
	reg_ptr = (char *)&fp_reg[OPCODE_CMEM_TARGET(opcode)];
        /*
         * Left or Right register file selection (PA89).
         */
        if ((size == 2) && (OP_COP_UID(opcode) == COP_UID_FRR)) {
                reg_ptr += 4;
        }
        if (!(result = copyin((const char*)f, reg_ptr, (1 << size))) && OPCODE_M_UPDATE(opcode))
	{   /* Post-modify and update */
                ssp[OPCODE_CMEM_BASE(opcode)] = f + index;
        }
        return(result);
}

/*
 * align_short_cload - Handle short load coprocessor alignment fault.
 * 
 * Returns 0 if trap handled correctly.
 */
int
align_short_cload(
		  unsigned long opcode,
		  unsigned long *ssp,
		  int size)
{
        int im;
        unsigned long from;
        int result;
	char *reg_ptr;
	double *fp_reg;

	fp_reg = (double*)&((pcb_t)ssp)->sf;

        im = trap_instr_imm(OPCODE_CSMEM_IMM(opcode), OPCODE_S_IMASK);
        from = ssp[OPCODE_CSMEM_BASE(opcode)];
        if ((OPCODE_M_UPDATE(opcode) && OPCODE_A_UPDATE(opcode)) || (!OPCODE_M_UPDATE(opcode))) {
                from += im;
        }

        reg_ptr = (char *)&fp_reg[OPCODE_CSMEM_TARGET(opcode)];

        /*
         * Left or Right register file selection (PA89).
         */
        if ((size == 4) && (OP_COP_UID(opcode) == COP_UID_FRR)) {
                reg_ptr += 4;
        }
        if (!(result = copyin((const char*)from, reg_ptr, size)) && OPCODE_M_UPDATE(opcode)) {
                if (!OPCODE_A_UPDATE(opcode)) {                    /* Modify base register after */
                        from += im;
                }
                ssp[OPCODE_CSMEM_BASE(opcode)] = from;
        }
        return(result);
}

/*
 * align_short_cstore - Handle short store coprocessor alignment fault.
 * 
 * Returns 0 if trap handled correctly.
 */
int
align_short_cstore(
		   unsigned long opcode,
		   unsigned long *ssp,
		   int size)
{
        int im;
        unsigned long to;
        int result;
	unsigned char *reg_ptr;
	double *fp_reg;

	fp_reg = (double*)&((pcb_t)ssp)->sf;

        im = trap_instr_imm(OPCODE_CSMEM_IMM(opcode), OPCODE_S_IMASK);
        to = ssp[OPCODE_CSMEM_BASE(opcode)];
        if ((OPCODE_M_UPDATE(opcode) && OPCODE_A_UPDATE(opcode)) || (!OPCODE_M_UPDATE(opcode))) { 
                to += im;
        }
        reg_ptr = (unsigned char *)&fp_reg[OPCODE_CSMEM_SOURCE(opcode)];
        /*
         * Left or Right register file selection (PA89).
         */
        if ((size == 4) && (OP_COP_UID(opcode) == COP_UID_FRR)) {
                reg_ptr += 4;
        }
        if (!(result = copyout((const char*)reg_ptr, (char*)to, size)) && OPCODE_M_UPDATE(opcode)) {
                if (!OPCODE_A_UPDATE(opcode)) {                     /* Modify base register after */
                        to += im;
                }
                ssp[OPCODE_CSMEM_BASE(opcode)] = to;
        }
        return(result);
}

#endif /* HPOSFCOMPAT */





