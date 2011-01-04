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

#ifndef	_MATH_H_
#define	_MATH_H_ 1

double acos (double);
double acosh (double);
double asin (double);
double asinh (double);
double atan (double);
double atanh (double);
double atan2 (double, double);
double cbrt (double);
double ceil (double);
double copysign (double, double);
double cos (double);
double cosh (double);
double drem (double);
double exp (double);
double expm1 (double);
double fabs (double);
int    finite (double);
double floor (double);
double fmod (double, double);
double frexp (double, int *);
int    ilogb (double);
int    isnan(double);
double ldexp (double, int);
double log (double);
double log10 (double);
double log1p (double);
double logb (double);
double modf (double, double *);
double nextafter (double, double);
double pow (double, double);
double remainder (double, double);
double rint (double);
double scalb (double, double);
double sin (double);
double sinh (double);
double sqrt (double);
double tan (double);
double tanh (double);

#include <machine/math.h>

#endif	/* _MATH_H_ */
