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

/* $Source: /u1/osc/rcs/mach_kernel/hp_pa/denormal.c,v $
 * $Revision: 1.1.9.4 $	$Author: emcmanus $
 * $State: Exp $   	$Locker:  $
 * $Date: 1996/02/02 12:13:03 $
 */

#include <hp_pa/spmath/float.h>
#include <hp_pa/spmath/sgl_float.h>
#include <hp_pa/spmath/dbl_float.h>
#include <hp_pa/spmath/hppa.h>

#undef Fpustatus_register
#define Fpustatus_register Fpu_register[0]

sgl_denormalize(sgl_opnd,inexactflag,rmode)
unsigned int *sgl_opnd;
boolean *inexactflag;
int rmode;
{
	unsigned int opnd;
	int sign, exponent;
	boolean guardbit = FALSE, stickybit, inexact;

	opnd = *sgl_opnd;
	stickybit = *inexactflag;
        exponent = Sgl_exponent(opnd) - SGL_WRAP;
        sign = Sgl_sign(opnd);
	Sgl_denormalize(opnd,exponent,guardbit,stickybit,inexact);
	if (inexact) {
	    switch (rmode) {
	      case ROUNDPLUS:
		if (sign == 0) {
			Sgl_increment(opnd);
		}
		break;
	      case ROUNDMINUS:
		if (sign != 0) {
			Sgl_increment(opnd);
		}
		break;
	      case ROUNDNEAREST:
		if (guardbit && (stickybit || 
		       Sgl_isone_lowmantissa(opnd))) {
			   Sgl_increment(opnd);
		}
		break;
	    }
	}
	Sgl_set_sign(opnd,sign);
	*sgl_opnd = opnd;
	*inexactflag = inexact;
	return;
}

dbl_denormalize(dbl_opndp1,dbl_opndp2,inexactflag,rmode)
unsigned int *dbl_opndp1, *dbl_opndp2;
boolean *inexactflag;
int rmode;
{
	unsigned int opndp1, opndp2;
	int sign, exponent;
	boolean guardbit = FALSE, stickybit, inexact;

	opndp1 = *dbl_opndp1;
	opndp2 = *dbl_opndp2;
	stickybit = *inexactflag;
	exponent = Dbl_exponent(opndp1) - DBL_WRAP;
	sign = Dbl_sign(opndp1);
	Dbl_denormalize(opndp1,opndp2,exponent,guardbit,stickybit,inexact);
	if (inexact) {
	    switch (rmode) {
	      case ROUNDPLUS:
		if (sign == 0) {
			Dbl_increment(opndp1,opndp2);
		}
		break;
	      case ROUNDMINUS:
		if (sign != 0) {
			Dbl_increment(opndp1,opndp2);
		}
		break;
	      case ROUNDNEAREST:
		if (guardbit && (stickybit || 
		       Dbl_isone_lowmantissap2(opndp2))) {
			   Dbl_increment(opndp1,opndp2);
		}
		break;
	    }
	}
	Dbl_set_sign(opndp1,sign);
	*dbl_opndp1 = opndp1;
	*dbl_opndp2 = opndp2;
	*inexactflag = inexact;
	return;
}
