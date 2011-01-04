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
 * platform.h,v
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * platform.h,v
 * Revision 1.36.1.2.1.2  1994/09/07  04:18:12  menze
 * OSF modifications
 *
 * Revision 1.36.1.2  1994/09/01  04:17:05  menze
 * Subsystem initialization functions can fail
 * Removed xMalloc and xFree declarations
 *
 * Revision 1.36  1994/08/19  18:02:34  menze
 *   [ 1994/05/10         hkaram ]
 *   Added size_t definition for inkernel portion
 *
 *   [ 1994/04/01          menze ]
 *   added inclusion of x_libc.h and xk_assert.h
 *
 * Revision 1.35  1994/02/05  00:10:40  menze
 *   [ 1994/01/14          menze ]
 *   ROM declarations moved to xkernel/include/rom.h
 *
 * Revision 1.34  1994/01/10  18:14:38  menze
 *   [ 1994/01/04          menze ]
 *   Added inclusion of xk_arch.h
 *
 */

#ifndef platform_h
#define platform_h


#if defined(__GNUC__) && !XK_DEBUG
#  define XK_USE_INLINE
#endif

#include <xkern/include/domain.h>
#include <xkern/include/msg_s.h>
#include <string.h>
#include <stdlib.h>

xkern_return_t	evInit( int );

u_short inCkSum( Msg m, u_short *buf, int len );
u_short ocsum( u_short *buf, int count );

typedef long *Unspec;
typedef unsigned long ProcessId;
typedef unsigned long ContextId;

#ifndef NULL
#define NULL	0
#endif
#define MAXUNSIGNED	((unsigned) (-1)

#ifndef	MACH_KERNEL
#define splx(x)
#define spl7() 1
#define splnet spl7
#define splclk spl7
#endif  /* MACH_KERNEL */

#ifndef offsetof
#  define offsetof(_type, _member) ((size_t)&((_type *)0)->_member)
#endif

#define INIT_STACK_SIZE 1024

#define BYTES_PER_WORD	4

#define CLICKS_PER_SEC 100	/* Clock interrupts per second *//***/

#if !defined(ROUND4)
#define ROUND4(len)  ((len + 3) & ~3)
#endif
#define BETWEEN(A,B,C) ((A) <= (B) && (B) < (C))

typedef	char	*mapKeyPtr_t;
typedef	int	mapVal_t;

typedef char	*statePtr_t;

#ifndef	TRUE
#define	TRUE	1
#define FALSE	0
#endif

#define	SUCCESS_RET_VAL		0
#define	FAILURE_RET_VAL		(-1)

/* Used for numbers? */

#define	LO_BYTE_OF_2(word)	 ((unsigned char) (0xff & (unsigned) word))
#define HI_BYTE_OF_2(word)	 ((unsigned char) (((unsigned) word) >> 8 ))

#define CAT_BYTES(hiByte,loByte) ((((unsigned)hiByte)<<8) + (unsigned)loByte)

#include <machine/endian.h>

/* 
 * LONG_ALIGNMENT_REQUIRED indicates that int's must be 32-bit aligned
 * on this architecture.
 */
#define LONG_ALIGNMENT_REQUIRED
#define LONG_ALIGNED( address )  (! ((int)(address) & 0x3))
#define SHORT_ALIGNED( address ) ( ! ((int)(address) & 0x1))

#define xk_int8		bit8_t
#define xk_int16	bit16_t
#define xk_int32	bit32_t

#define xk_u_int8	u_bit8_t
#define xk_u_int16	u_bit16_t
#define xk_u_int32	u_bit32_t


#if defined(__hp700__) && defined(__GNUC__)

/*
 * read_itimer provides access to a high-resolution timer in the PA-RISC
 * processor, with minimal overhead.  The timer "is continually counting
 * up by 1 at a rate which is implementation-dependent and between twice
 * the "peak instruction rate" and half the "peak instruction rate",
 * according to the Architecture Reference Manual.
 * For the 720's it's incremented once every 1/50th usec and for the
 * 730's and 750's it's incremented once every 1/66th usec.
 */
#define read_itimer()	({u_int r; asm volatile("mfctl 16,%0" : "=r" (r)); r;})

#endif /* __hp700__ && __GNUC__ */

#endif
