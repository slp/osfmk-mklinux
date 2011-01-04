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
 *	net_debug.h
 */
#ifndef	_NET_DEBUG_H_
#define	_NET_DEBUG_H_

extern void print_registers(void);
extern void debug_getchar(void);
extern void dump_packet(const char *banner, char *addr, int len);

#ifdef	NETDEBUG
extern int netdebug;
#define NPRINT(args)	if (netdebug) printf args ;
#define NDWAIT		if (netdebug) debug_getchar();
#else
#define NPRINT(args)	/* void */
#define NDWAIT		/* void */
#endif

#ifdef	PROTODEBUG
extern int protodebug;
#define PPRINT(args)	if (protodebug) printf args ;
#define PDWAIT		if (protodebug) debug_getchar();
#else
#define PPRINT(args)	/* void */
#define PDWAIT		/* void */
#endif

#endif	/* _NET_DEBUG_H_ */
