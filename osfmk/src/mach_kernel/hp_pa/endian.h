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
 */
/*
 * MkLinux
 */
/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 *
 *	@(#)endian.h	7.2 (Berkeley) 1/21/88
 */

#ifndef _MACHINE_ENDIAN_H_
#define	_MACHINE_ENDIAN_H_

/*
 * Definitions for byte order,
 * according to byte significance from low address to high.
 */
#define	LITTLE_ENDIAN	1234	/* least-significant byte first (vax) */
#define	BIG_ENDIAN	4321	/* most-significant byte first (IBM, net) */
#define	PDP_ENDIAN	3412	/* LSB first in word, MSW first in long (pdp) */

#define	BYTE_ORDER	BIG_ENDIAN	/* byte order on hp9000s800 */

/*
 * Macros for network/external number representation conversion.
 */
#if BYTE_ORDER == BIG_ENDIAN && !defined(lint)
#define	ntohl(x)	(x)
#define	ntohs(x)	(x)
#define	htonl(x)	(x)
#define	htons(x)	(x)

static __inline__ unsigned int byte_reverse_word(unsigned long word);
static __inline__ unsigned int byte_reverse_word(unsigned long word) {
	register int _ov = (word), result;

	__asm__ __volatile ("dep	%1,7,8,%0\n\t"
			"shd	%1,%1,8,%1\n\t"
			"dep	%1,15,8,%0\n\t"
			"shd	%1,%1,8,%1\n\t"
			"dep	%1,23,8,%0\n\t"
			"shd	%1,%1,8,%1\n\t"
			"dep	%1,31,8,%0"
			: "=&r" (result), "=r" (_ov) : "1" (_ov));
	return(result);
}

/* The above function is commutative, so we can use it for
 * translations in both directions (to/from little endianness)
 * Note that htolx and ltohx are probably identical, they are
 * included for completeness.
 */
#define htoll(x)  byte_reverse_word(x)
#define htols(x)  (byte_reverse_word(x) >> 16)
#define ltohl(x)  htoll(x)
#define ltohs(x)  htols(x)

#define htobl(x) (x)
#define htobs(x) (x)
#define btohl(x) (x)
#define btohs(x) (x)

#else
unsigned short	ntohs(), htons();
unsigned long	ntohl(), htonl();
#endif

/* This defines the order of elements in a bitfield,
 * it is principally used by the SCSI subsystem in
 * the definitions of mapped registers
 */
#define BYTE_MSF 1

#define NTOHL(x)	/* nothing */
#define NTOHS(x)	/* nothing */
#define HTONL(x)	/* nothing */
#define HTONS(x)	/* nothing */

#endif /* _MACHINE_ENDIAN_H_ */
