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
 *  (c) Copyright 1991 HEWLETT-PACKARD COMPANY
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


/* $Source: /u1/osc/rcs/mach_kernel/hp_pa/decode_exc.c,v $
 * $Revision: 1.1.9.4 $	$Author: emcmanus $
 * $State: Exp $   	$Locker:  $
 * $Date: 1996/02/02 12:13:00 $
 */

#ifdef HPE
#include "hfloat.asmassis.official"
#include "hsglflt.asmassis.official"
#include "hdblflt.asmassis.official"
#include "hcnvflt.asmassis.official"
#else /* HPE */
#include <hp_pa/spmath/float.h>
#include <hp_pa/spmath/sgl_float.h>
#include <hp_pa/spmath/dbl_float.h>
#include <hp_pa/spmath/cnv_float.h>
#endif /* HPE */

#undef Fpustatus_register
#define Fpustatus_register Fpu_register[0]

/* General definitions */
#define DOESTRAP 1
#define NOTRAP 0
#define copropbit	1<<31-2	/* bit position 2 */
#define opclass		9	/* bits 21 & 22 */
#define fmt		11	/* bits 19 & 20 */
#define df		13	/* bits 17 & 18 */
#define twobits		3	/* mask low-order 2 bits */
#define fivebits	31	/* mask low-order 5 bits */
#define MAX_EXCP_REG	7	/* number of excpeption registers to check */

/* Exception register definitions */
#define Excp_type(index) Exceptiontype(Fpu_register[index])
#define Excp_instr(index) Instructionfield(Fpu_register[index])
#define Clear_excp_register(index) Allexception(Fpu_register[index]) = 0
#define Excp_format() \
    (current_ir >> ((current_ir>>opclass & twobits)==1 ? df : fmt) & twobits)

/* Miscellaneous definitions */
#define Fpu_sgl(index) Fpu_register[index*2]

#define Fpu_dblp1(index) Fpu_register[index*2]
#define Fpu_dblp2(index) Fpu_register[(index*2)+1]

#define Fpu_quadp1(index) Fpu_register[index*2]
#define Fpu_quadp2(index) Fpu_register[(index*2)+1]
#define Fpu_quadp3(index) Fpu_register[(index*2)+2]
#define Fpu_quadp4(index) Fpu_register[(index*2)+3]

#ifdef notdef
/* Single precision floating-point definitions */
#define Sgl_decrement(sgl_value) Sall(sgl_value)--

/* Double precision floating-point definitions */
#define Dbl_decrement(dbl_valuep1,dbl_valuep2) \
    if ((Dallp2(dbl_valuep2)--) == 0) Dallp1(dbl_valuep1)-- 
#endif

/* External declarations */
extern fpudispatch();
extern sgl_denormalize();
extern dbl_denormalize();


#ifdef TIMEX_BUG
#ifdef MACH_KERNEL
#include <machine/regs.h>
#else
#include "machine/reg.h"
#endif
#endif

decode_fpu(Fpu_register)
unsigned int Fpu_register[];

{
    unsigned int current_ir, excp, sgl_opnd, dbl_opndp1, dbl_opndp2;
    int target, exception_index = 1;
    boolean inexact;
#ifdef TIMEX
    unsigned int excptype;
#endif

    /* exception_index is used to index the exception register queue.  It
     *   always points at the last register that contains a valid exception.  A
     *   zero value implies no exceptions (also the initialized value).  Setting
     *   the T-bit resets the exception_index to zero.
     */

    /*
     * Check for reserved-op exception.  A reserved-op exception does not 
     * set any exception registers nor does it set the T-bit.  If the T-bit
     * is not set then a reserved-op exception occurred.
     *
     * At some point, we may want to report reserved op exceptions as
     * illegal instructions.
     */
    if (!Is_tbit_set()) return(DOESTRAP);

#ifdef TIMEX_BUG
	if ((mfctl(CR_CCR) & 0xc0) &&
	        timex_fix(&Fpu_register[2], &Fpu_register[3], &Fpu_register[4],
		&Fpu_register[5]) == 0) {
			/*
			 * set bit 30 in exception register 1 to flag
			 * an uncorrectable error.
			 */
			Fpu_register[1] |= 0x2;
			return(DOESTRAP);
	}
#endif
    /* 
     * Is a coprocessor op. 
     *
     * Now we need to determine what type of exception occurred.
     */
     for (exception_index=1; exception_index<=MAX_EXCP_REG; exception_index++) {
	current_ir = Excp_instr(exception_index);
#ifdef TIMEX
	  /*
	   * Change from PA83: there are now 5 different unimplemented
	   * exception codes: 0x1, 0x9, 0xb, 0x3, and 0x23.  Only these have
	   * the low order bit set.
	   */
	  excptype = Excp_type(exception_index);
	  if (excptype & UNIMPLEMENTEDEXCEPTION) {
#else

	switch(Excp_type(exception_index)) {
	    case UNIMPLEMENTEDEXCEPTION:
#endif
		/*
		 * Clear T-bit and exception register so that
		 * we can tell if a trap really occurs while 
		 * emulating the instruction.
		 */
		Clear_tbit();
		Clear_excp_register(exception_index);
		/*
		 * Now emulate this instruction.  If a trap occurs,
		 * fpudispatch will return a non-zero number 
		 */
#ifdef TIMEX
		if (excp = fpudispatch(current_ir,excptype,0,Fpu_register)) {
#else
		if (excp = fpudispatch(current_ir,1,0,Fpu_register)) {
#endif
			/*
			 * We now need to make sure that the T-bit and the
			 * exception register contain the correct values
			 * before returning.
			 */
#ifdef TIMEX
			if (excp == UNIMPLEMENTEDEXCEPTION) {
				/*
			 	 * it is really unimplemented, so restore the
			 	 * TIMEX extended unimplemented exception code
			 	 */
				excp = excptype;
			}
#endif
			Set_tbit();
			Set_exceptiontype_and_instr_field(excp,current_ir,
			 Fpu_register[exception_index]);
			return(DOESTRAP);
		}
		/*
		 * Set t-bit since it might still be needed for a
		 * subsequent real trap
		 */
		Set_tbit();
		continue;
#ifdef TIMEX
	}

	  /*
	   * In PA89, the underflow exception has been extended to encode
	   * additional information.  The exception looks like pp01x0,
	   * where x is 1 if inexact and pp represent the inexact bit (I)
	   * and the round away bit (RA)
	   */
	  if (excptype & UNDERFLOWEXCEPTION) {
#else
	  case UNDERFLOWEXCEPTION:
#endif
		/* check for underflow trap enabled */
		if (Is_underflowtrap_enabled()) return(DOESTRAP);
		else {
		    /*
		     * Isn't a real trap; we need to 
		     * return the default value.
		     */
		    target = current_ir & fivebits;
#ifndef lint
		    if (Ibit(Fpu_register[exception_index])) inexact = TRUE;
		    else inexact = FALSE;
#endif
		    switch (Excp_format()) {
		      case SGL:
		        /*
		         * If ra (round-away) is set, will 
		         * want to undo the rounding done
		         * by the hardware.
		         */
		        if (Rabit(Fpu_register[exception_index])) 
				Sgl_decrement(Fpu_sgl(target));

			/* now denormalize */
			sgl_denormalize(&Fpu_sgl(target),&inexact,Rounding_mode());
		    	break;
		      case DBL:
		    	/*
		    	 * If ra (round-away) is set, will 
		    	 * want to undo the rounding done
		    	 * by the hardware.
		    	 */
		    	if (Rabit(Fpu_register[exception_index])) 
				Dbl_decrement(Fpu_dblp1(target),Fpu_dblp2(target));

			/* now denormalize */
			dbl_denormalize(&Fpu_dblp1(target),&Fpu_dblp2(target),
			  &inexact,Rounding_mode());
		    	break;
		    }
		    if (inexact) Set_underflowflag();
		    /* 
		     * Underflow can generate an inexact
		     * exception.  If inexact trap is enabled,
		     * want to do an inexact trap, otherwise 
		     * set inexact flag.
		     */
		    if (inexact & Is_inexacttrap_enabled()) {
		    	/*
		    	 * Set exception field of exception register
		    	 * to inexact, parm field to zero.
			 * Underflow bit should be cleared.
		    	 */
		    	Set_exceptiontype(Fpu_register[exception_index],
			 INEXACTEXCEPTION);
			Set_parmfield(Fpu_register[exception_index],0);
		    	return(DOESTRAP);
		    }
		    else {
		    	/*
		    	 * Exception register needs to be cleared.  
			 * Inexact flag needs to be set if inexact.
		    	 */
		    	Clear_excp_register(exception_index);
		    	if (inexact) Set_inexactflag();
		    }
		}
		continue;
#ifdef TIMEX
	}
	switch(Excp_type(exception_index)) {
#endif
	  case OVERFLOWEXCEPTION:
	  case OVERFLOWEXCEPTION | INEXACTEXCEPTION:
		/* check for overflow trap enabled */
		if (Is_overflowtrap_enabled()) return(DOESTRAP);
		else {
			/*
			 * Isn't a real trap; we need to 
			 * return the default value.
			 */
			target = current_ir & fivebits;
			switch (Excp_format()) {
			  case SGL: 
				Sgl_setoverflow(Fpu_sgl(target));
				break;
			  case DBL:
				Dbl_setoverflow(Fpu_dblp1(target),Fpu_dblp2(target));
				break;
			}
			Set_overflowflag();
			/* 
			 * Overflow always generates an inexact
			 * exception.  If inexact trap is enabled,
			 * want to do an inexact trap, otherwise 
			 * set inexact flag.
			 */
			if (Is_inexacttrap_enabled()) {
				/*
				 * Set exception field of exception
				 * register to inexact.  Overflow
				 * bit should be cleared.
				 */
				Set_exceptiontype(Fpu_register[exception_index],
				 INEXACTEXCEPTION);
				return(DOESTRAP);
			}
			else {
				/*
				 * Exception register needs to be cleared.  
				 * Inexact flag needs to be set.
				 */
				Clear_excp_register(exception_index);
				Set_inexactflag();
			}
		}
		break;
	  case INVALIDEXCEPTION:
	  case DIVISIONBYZEROEXCEPTION:
	  case INEXACTEXCEPTION:
	  default:
		return(DOESTRAP);
	  case NOEXCEPTION:	/* no exception */
		/*
		 * Clear exception register in case 
		 * other fields are non-zero.
		 */
		Clear_excp_register(exception_index);
		break;
	}
    }
    /*
     * No real exceptions occurred.
     */
    Clear_tbit();
    return(NOTRAP);
}


#ifdef MDSFU_SUPPORTED
/*ARGSUSED*/
decode_mdsfu(current_ir,Mdsfu_register)

unsigned int current_ir, Mdsfu_register[];
{
	/* 
	 * Is an sfu op.
	 *
	 * Now we need to determine what caused it, and what
	 * to do next.
	 * The two main causes of an exception trap in the mdsfu are:
	 *    1) retrieve, trap on overflow
	 *    2) partial implementation of multiply/divide sfu
	 * Currently we will expect only case 1.
	 */
	 return(DOESTRAP);
}
#endif /* MDSFU_SUPPORTED */
