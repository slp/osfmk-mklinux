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

#ifndef _MACHINE_ENDIAN_H_
#define _MACHINE_ENDIAN_H_

/*
 * Definitions for byte order,
 * according to byte significance from low address to high.
 */
#define	LITTLE_ENDIAN	1234	/* least-significant byte first (vax) */
#define	BIG_ENDIAN	4321	/* most-significant byte first (IBM, net) */
#define	PDP_ENDIAN	3412	/* LSB first in word, MSW first in long (pdp) */

#if _BIG_ENDIAN			/* Predefined by compiler */
#define	BYTE_ORDER	BIG_ENDIAN	/* byte order we use on ppc */
#define ENDIAN		BIG
#else
#error code has not been ported to little endian targets yet
#endif

/*
 * Macros for network/external number representation conversion.
 */
#if BYTE_ORDER == BIG_ENDIAN && !defined(lint)
#define	ntohl(x)	(x)
#define	ntohs(x)	(x)
#define	htonl(x)	(x)
#define	htons(x)	(x)

static __inline__ unsigned int byte_reverse_word(unsigned int word);
static __inline__ unsigned int byte_reverse_word(unsigned int word) {
	unsigned int result;
	__asm__ volatile("lwbrx	%0, 0, %1" : "=r" (result) : "r" (&word));
	return result;
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

#endif /* _MACHINE_ENDIAN_H_ */
