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
 * blast_mask64.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.1.10.1 $
 * $Date: 1995/02/23 15:30:04 $
 */

/* 
 * Header file for using 64-bit masks
 */

#ifndef blast_mask64_h
#define blast_mask64_h
 

typedef struct {
    u_long	hi;
    u_long	lo;
} BlastMask;

#define BLAST_MASK_PROTOTYPE	BlastMask

/* 
 * non-ANSI compilers gripe about the _n##U usage while GCC gives
 * warning messages about the u_long cast.
 */
#define UNSIGNED(_n)	_n##U

#define BLAST_MAX_FRAGS	64
#define BLAST_FULL_MASK(_m, _n)				\
{							\
    if ( _n <= 32 ) {					\
	(_m).hi = 0; 					\
	(_m).lo = ( UNSIGNED(0xffffffff) >> (32 - (_n)) );	\
    } else {						\
	(_m).hi = ( UNSIGNED(0xffffffff) >> (64 - (_n)) );	\
	(_m).lo = UNSIGNED(0xffffffff);			\
    }							\
}    

#define BLAST_MASK_SHIFT_LEFT(_m, _n)			\
{							\
    (_m).hi <<= (_n);					\
    if ( (_m).lo & UNSIGNED(0x80000000) ) {			\
	(_m).hi |= 0x01;				\
    }							\
    (_m).lo <<= (_n);					\
}							


#define BLAST_MASK_SHIFT_RIGHT(_m, _n)			\
{							\
    (_m).lo >>= (_n);					\
    if ( (_m).hi & 0x01 ) {				\
	(_m).lo |= UNSIGNED(0x80000000);		\
    }							\
    (_m).hi >>= (_n);					\
}		

#if !XK_DEBUG && ! defined(__GNUC__)
/*
 * These macros cause gcc2 to generate spurious warnings, thus the inline
 * functions below
 */

#define BLAST_MASK_IS_BIT_SET(_m, _n)	\
  ( (_n) > 32 ? ((_m)->hi & (1 << ((_n)-33))) : ((_m)->lo & (1 << ((_n)-1))) )
#define BLAST_MASK_SET_BIT(_m, _n)	\
 ( (_n) > 32 ? ((_m)->hi |= (1 << ((_n)-33))) : ((_m)->lo |= (1 << ((_n)-1))) )

#else	/* GNUC || XK_DEBUG */
#  if defined(__GNUC__)
#      define FUNCTYPE	static __inline__
#  else
#      define FUNCTYPE	static 
#  endif /* GNUC */

FUNCTYPE int	BLAST_MASK_IS_BIT_SET( BlastMask *, int );
FUNCTYPE void	BLAST_MASK_SET_BIT( BlastMask *, int );

FUNCTYPE int
BLAST_MASK_IS_BIT_SET( m, n )
    BlastMask	*m;
    int		n;
{
    if ( n > 32 ) {
	return m->hi & (1 << (n - 33));
    } else {
	return m->lo & (1 << (n - 1));
    }
}

FUNCTYPE void
BLAST_MASK_SET_BIT( m, n )
    BlastMask	*m;
    int		n;
{
    if ( n > 32 ) {
	m->hi |= (1 << (n - 33));
    } else {
	m->lo |= (1 << (n - 1));
    }
}

#endif   /* !XK_DEBUG && ! defined(__GNUC__) */


#define BLAST_MASK_CLEAR(_m)		{ (_m).hi = 0; (_m).lo = 0; }
#define BLAST_MASK_IS_ZERO(_m)		( (_m).hi == 0 && (_m).lo == 0 )
#define BLAST_MASK_OR(_m1, _m2)		{ (_m1).hi |= (_m2).hi;		\
					  (_m1).lo |= (_m2).lo; }
#define BLAST_MASK_EQ(_m1, _m2)		( (_m1).hi == (_m2).hi &&	\
					  (_m1).lo == (_m2).lo )
#define BLAST_MASK_NTOH(_tar, _src)	{ (_tar).hi = ntohl((_src).hi); \
					  (_tar).lo = ntohl((_src).lo); }
#define BLAST_MASK_HTON(_tar, _src)	{ (_tar).hi = htonl((_src).hi); \
					  (_tar).lo = htonl((_src).lo); }


#define BLAST_PADDING	1

#endif /* ! blast_mask64_h */

