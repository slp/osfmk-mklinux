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
 * hoststr.c
 *
 * x-kernel v3.2
 *
 * Copyright (c) 1993,1991,1990  Arizona Board of Regents
 *
 *
 * $Revision: 1.1.8.1 $
 * $Date: 1995/02/23 15:21:33 $
 */

#include <xkern/include/domain.h>
#include <xkern/include/xkernel.h>
#include <xkern/include/prot/ip.h>
#include <xkern/include/prot/eth.h>

xkern_return_t
str2ipHost(
	IPhost *h,
	char *s)
 {
    int a[4], i;

    for ( i=0; i < 4; i++ ) {
        a[i] = 0;
	while (*s >= '0' && *s <= '9') {
	    a[i] = 10*(a[i]) + (*s++ - '0');
	}
	if (i < 3 && *s++ != '.') {
	    return XK_FAILURE;
	}
	if ( a[i] > 255 ) {
	    return XK_FAILURE;
	}
    }
    h->a = a[0];
    h->b = a[1];
    h->c = a[2];
    h->d = a[3];
    return XK_SUCCESS;
}


char *
ipHostStr(
	IPhost *h)
{
    static char str[4][32];
    static int i=0;
    
    i = ++i % 4;
    if (h == 0) {
	return "<null>";
    }
    if (h == (IPhost *)ANY_HOST) {
	return "ANY_HOST";
    }
    sprintf(str[i], "%d.%d.%d.%d", h->a, h->b, h->c, h->d);
    return str[i];
}


static int
hexdigit(
	char a)
{
    if (a >= '0' && a <= '9') 
	return(a-'0');
    if (a >= 'a' && a <= 'f') 
	return(a-'a'+10);
    if (a >= 'A' && a <= 'F') 
	return(a-'A'+10);
    return(-1);
}


xkern_return_t
str2ethHost(
	ETHhost *h,
	char *s)
{
    int	a[6], i, n;
     
    for ( i=0; i < 6; i++ ) {
        a[i] = 0;
	while( (n=hexdigit(*s)) >= 0 ) {
	    a[i] = 16*(a[i]) + n;
	    s++;
	}
	if (i < 5 && *s++ != ':') {
	    return XK_FAILURE;
	}
	if (a[i] > 0xff) {
	    return XK_FAILURE;
	}
    }
    h->high = htons((a[0] << 8) + a[1]);
    h->mid = htons((a[2] << 8) + a[3]);
    h->low = htons((a[4] << 8) + a[5]);
    return XK_SUCCESS;
}


char *
ethHostStr(
	ETHhost *h)
{
    static	char str[4][32];
    static	int i=0;
    u_char	*hc = (u_char *)h;
    
    if (h == 0) {
	return "<null>";
    }
    if (h == (ETHhost *)ANY_HOST) {
	return "ANY_HOST";
    }
    i = ++i % 4;
    sprintf(str[i], "%02x:%02x:%02x:%02x:%02x:%02x",
	    hc[0], hc[1], hc[2], hc[3], hc[4], hc[5]);
    return str[i];
}

