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
 * blast_mask32.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.1.8.1 $
 * $Date: 1995/02/23 15:29:59 $
 */

/* 
 * Header file for using 32-bit masks
 */

#ifndef blast_mask32_h
#define blast_mask32_h
 
/* 
 * non-ANSI compilers gripe about the _n##U usage while GCC gives
 * warning messages about the u_long cast.
 */
#   define UNSIGNED(_n)	_n##U

typedef u_long	BlastMask;
#define BLAST_MASK_PROTOTYPE	BlastMask

#define BLAST_MAX_FRAGS	32
#define BLAST_FULL_MASK(_m, _n)		(_m) = ( UNSIGNED(0xffffffff) >> (32 - (_n)) )
#define BLAST_MASK_SHIFT_LEFT(_m, _n)	(_m) <<= (_n)
#define BLAST_MASK_SHIFT_RIGHT(_m, _n)	(_m) >>= (_n)
#define BLAST_MASK_IS_BIT_SET(_m, _n)	( *(_m) & ( 1 << ((_n)-1 ) ) )
#define BLAST_MASK_SET_BIT(_m, _n)	( *(_m) |= ( 1 << ((_n)-1) ) )
#define BLAST_MASK_CLEAR(_m)		(_m) = 0
#define BLAST_MASK_IS_ZERO(_m)		( (_m) == 0 )
#define BLAST_MASK_OR(_m1, _m2)		(_m1) |= (_m2)
#define BLAST_MASK_EQ(_m1, _m2)		( (_m1) == (_m2) )
#define BLAST_MASK_NTOH(_tar, _src)	(_tar) = ntohl(_src);
#define BLAST_MASK_HTON(_tar, _src)	(_tar) = htonl(_src);

#define BLAST_PADDING	1

#endif /* ! blast_mask32_h */

