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
 *	file: sqrt.c
 *
 *	Floating-point emulation package for sqrt.
 */

/*
 * Copyright (c) 1985, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Some IEEE standard 754 recommended functions and remainder and sqrt for
 * supporting the C elementary functions.
 ******************************************************************************
 * WARNING:
 *      These codes are developed (in double) to support the C elementary
 * functions temporarily. They are not universal, and some of them are very
 * slow (in particular, drem and sqrt is extremely inefficient). Each
 * computer system should have its implementation of these functions using
 * its own assembler.
 ******************************************************************************
 *
 * IEEE 754 required operations:
 *     drem(x,p)
 *              returns  x REM y  =  x - [x/y]*y , where [x/y] is the integer
 *              nearest x/y; in half way case, choose the even one.
 *     sqrt(x)
 *              returns the square root of x correctly rounded according to
 *		the rounding mod.
 *
 * IEEE 754 recommended functions:
 * (a) copysign(x,y)
 *              returns x with the sign of y.
 * (b) scalb(x,N)
 *              returns  x * (2**N), for integer values N.
 * (c) logb(x)
 *              returns the unbiased exponent of x, a signed integer in
 *              double precision, except that logb(0) is -INF, logb(INF)
 *              is +INF, and logb(NAN) is that NAN.
 * (d) finite(x)
 *              returns the value TRUE if -INF < x < +INF and returns
 *              FALSE otherwise.
 *
 *
 * CODED IN C BY K.C. NG, 11/25/84;
 * REVISED BY K.C. NG on 1/22/85, 2/13/85, 3/24/85.
 */

static const unsigned short msign=0x7fff, mexp =0x7ff0  ;
static const short prep1=54, gap=4, bias=1023           ;
static const double novf=1.7E308, nunf=3.0E-308,zero=0.0;

extern double scalb(double, int);
double scalb(x,N)
double x; int N;
{
        int k;

        unsigned short *px=(unsigned short *) &x;

        if( x == zero )  return(x);

        if( (k= *px & mexp ) != mexp ) {
            if( N<-2100) return(nunf*nunf); else if(N>2100) return(novf+novf);
            if( k == 0 ) {
                 x *= scalb(1.0,(int)prep1);  N -= prep1; return(scalb(x,N));}

            if((k = (k>>gap)+ N) > 0 )
                if( k < (mexp>>gap) ) *px = (*px&~mexp) | (k<<gap);
                else x=novf+novf;               /* overflow */
            else
                if( k > -prep1 )
                                        /* gradual underflow */
                    {*px=(*px&~mexp)|(short)(1<<gap); x *= scalb(1.0,k-1);}
                else
                return(nunf*nunf);
            }
        return(x);
}

extern double logb(double);
double logb(x)
double x;
{
        short *px=(short *) &x, k;

        if( (k= *px & mexp ) != mexp )
            if ( k != 0 )
                return ( (k>>gap) - bias );
            else if( x != zero)
                return ( -1022.0 );
            else
                return(-(1.0/zero));
        else if(x != x)
            return(x);
        else
            {*px &= msign; return(x);}
}

extern int finite(double);
finite(x)
double x;
{
        return( (*((short *) &x ) & mexp ) != mexp );
}

extern double drem(double, double);
double drem(x,p)
double x,p;
{
        short sign;
        double hp,dp,tmp;
        unsigned short  k;
        unsigned short
              *px=(unsigned short *) &x  ,
              *pp=(unsigned short *) &p  ,
              *pd=(unsigned short *) &dp ,
              *pt=(unsigned short *) &tmp;

        *pp &= msign ;

        if( ( *px & mexp ) == mexp )
		return  (x-p)-(x-p);	/* create nan if x is inf */
	if (p == zero) {
		return zero/zero;
	}

        if( ( *pp & mexp ) == mexp )
		{ if (p != p) return p; else return x;}

        else  if ( ((*pp & mexp)>>gap) <= 1 )
                /* subnormal p, or almost subnormal p */
            { double b; b=scalb(1.0,(int)prep1);
              p *= b; x = drem(x,p); x *= b; return(drem(x,p)/b);}
        else  if ( p >= novf/2)
            { p /= 2 ; x /= 2; return(drem(x,p)*2);}
        else
            {
                dp=p+p; hp=p/2;
                sign= *px & ~msign ;
                *px &= msign       ;
                while ( x > dp )
                    {
                        k=(*px & mexp) - (*pd & mexp) ;
                        tmp = dp ;
                        *pt += k ;

                        if( x < tmp ) *pt -= 16 ;

                        x -= tmp ;
                    }
                if ( x > hp )
                    { x -= p ;  if ( x >= hp ) x -= p ; }

			*px ^= sign;
                return( x);

            }
}

extern double sqrt(double);
double sqrt(x)
double x;
{
        double q,s,b,r;
        double t;
	double const zero=0.0;
        int m,n,i;
        int k=51;

    /* sqrt(NaN) is NaN, sqrt(+-0) = +-0 */
        if(x!=x||x==zero) return(x);

    /* sqrt(negative) is invalid */
        if(x<zero) {
		return(zero/zero);
	}

    /* sqrt(INF) is INF */
        if(!finite(x)) return(x);

    /* scale x to [1,4) */
        n=logb(x);
        x=scalb(x,-n);
        if((m=logb(x))!=0) x=scalb(x,-m);       /* subnormal number */
        m += n;
        n = m/2;
        if((n+n)!=m) {x *= 2; m -=1; n=m/2;}

    /* generate sqrt(x) bit by bit (accumulating in q) */
            q=1.0; s=4.0; x -= 1.0; r=1;
            for(i=1;i<=k;i++) {
                t=s+1; x *= 4; r /= 2;
                if(t<=x) {
                    s=t+t+2, x -= t; q += r;}
                else
                    s *= 2;
                }

    /* generate the last bit and determine the final rounding */
            r/=2; x *= 4;
            if(x==zero) goto end; 100+r; /* trigger inexact flag */
            if(s<x) {
                q+=r; x -=s; s += 2; s *= 2; x *= 4;
                t = (x-s)-5;
                b=1.0+3*r/4; if(b==1.0) goto end; /* b==1 : Round-to-zero */
                b=1.0+r/4;   if(b>1.0) t=1;	/* b>1 : Round-to-(+INF) */
                if(t>=0) q+=r; }	      /* else: Round-to-nearest */
            else {
                s *= 2; x *= 4;
                t = (x-s)-1;
                b=1.0+3*r/4; if(b==1.0) goto end;
                b=1.0+r/4;   if(b>1.0) t=1;
                if(t>=0) q+=r; }

end:        return(scalb(q,n));
}
