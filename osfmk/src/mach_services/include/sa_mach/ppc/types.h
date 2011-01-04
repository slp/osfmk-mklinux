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

#ifndef	_MACH_MACHINE_TYPES_H_
#define _MACH_MACHINE_TYPES_H_ 1

typedef long		dev_t;		/* device number (major+minor) */

typedef signed char	bit8_t;		/* signed 8-bit quantity */
typedef unsigned char	u_bit8_t;	/* unsigned 8-bit quantity */

typedef short		bit16_t;	/* signed 16-bit quantity */
typedef unsigned short	u_bit16_t;	/* unsigned 16-bit quantity */

typedef int		bit32_t;	/* signed 32-bit quantity */
typedef unsigned int	u_bit32_t;	/* unsigned 32-bit quantity */

/* Only 32 bits of the "bit64_t" are significant on this 32-bit machine */
typedef struct { int __val[2]; } bit64_t;	/* signed 64-bit quantity */
typedef struct { unsigned int __val[2]; } u_bit64_t;/* unsigned 64-bit quantity */
#define	_SIG64_BITS	__val[1]	/* bits of interest (32) */

#endif /*  _MACH_MACHINE_TYPES_H_ */
