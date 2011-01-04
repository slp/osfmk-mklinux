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

#define	BYTE_ORDER	LITTLE_ENDIAN	/* byte order on i386 */
#define ENDIAN		LITTLE

/*
 * Macros for network/external number representation conversion.
 */
#ifdef _NO_PROTO
unsigned short	ntohs(), htons();
unsigned long	ntohl(), htonl();
#else
unsigned short	ntohs(unsigned short), htons(unsigned short);
unsigned long	ntohl(unsigned long), htonl(unsigned long);
#endif

#if !defined(lint) && defined(__GNUC__)

/*
 * Use GNUC support to inline the byteswappers.
 */

#ifdef _NO_PROTO
extern __inline__
unsigned short
ntohs(w_int)
int w_int;
{
	register unsigned short w = w_int;
	__asm__("rorw	$8, %0" : "=r" (w) : "0" (w));
	return (unsigned long)(w);	/* zero-extend for compat */
}

#else	/* _NO_PROTO */
extern __inline__
unsigned short
ntohs(register unsigned short w)
{
	__asm__("rorw	$8, %0" : "=r" (w) : "0" (w));
	return (unsigned long)(w);	/* zero-extend for compat */
}

#endif	/* _NO_PROTO */
#define	htons	ntohs

extern __inline__
unsigned long
ntohl(register unsigned long l)
{
	__asm__("rorw	$8, %w0" : "=r" (l) : "0" (l));
	__asm__("ror	$16, %0" : "=r" (l) : "0" (l));
	__asm__("rorw	$8, %w0" : "=r" (l) : "0" (l));
	return l;
}
#define htonl	ntohl

#endif	/* __GNUC__ inlines */

#define NTOHL(x)	(x) = ntohl((unsigned long)x)
#define NTOHS(x)	(x) = ntohs((unsigned short)x)
#define HTONL(x)	(x) = htonl((unsigned long)x)
#define HTONS(x)	(x) = htons((unsigned short)x)

#endif
