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
 * ip_host.h
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.1.8.1 $
 * $Date: 1995/02/23 15:25:48 $
 */

#ifndef ip_host_h
#define ip_host_h

typedef struct iphost {
    u_char 	a,b,c,d;
} IPhost;

#define IP_EQUAL(a1,a2) ( (a1).a == (a2).a && (a1).b == (a2).b &&	\
			  (a1).c == (a2).c && (a1).d == (a2).d )

#define IP_AND( x, y, z ) {	\
	(x).a = (y).a & (z).a;	\
	(x).b = (y).b & (z).b;	\
	(x).c = (y).c & (z).c;	\
	(x).d = (y).d & (z).d;	\
      }

#define IP_OR( x, y, z ) {	\
	(x).a = (y).a | (z).a;	\
	(x).b = (y).b | (z).b;	\
	(x).c = (y).c | (z).c;	\
	(x).d = (y).d | (z).d;	\
      }

#define IP_COMP( x, y ) {	\
	(x).a = ~(y).a;		\
	(x).b = ~(y).b;		\
	(x).c = ~(y).c;		\
	(x).d = ~(y).d;		\
      }

#endif
