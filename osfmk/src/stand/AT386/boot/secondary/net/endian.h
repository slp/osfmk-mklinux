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
 * File : endian.h
 *
 * Author : Eric PAIRE (O.S.F. Research Institute)
 *
 * This file contains all declarations depending on machine endianness.
 */

#ifndef	__ENDIAN_H__
#define	__ENDIAN_H__

#define	BIG_ENDIAN	1234
#define	LITTLE_ENDIAN	4321

#if	defined(i386)
#define	BYTE_ORDER	LITTLE_ENDIAN
#endif

#define	ntohs(a)	htons(a)
#define	ntohl(a)	htonl(a)

#if	BYTE_ORDER == BIG_ENDIAN
#define	htons(a)	a
#define	htonl(a)	a
#endif

#if	BYTE_ORDER == LITTLE_ENDIAN
extern u16bits htons(u16bits);
extern u32bits htonl(u32bits);
#endif

#endif	/* __ENDIAN_H__ */
