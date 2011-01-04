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
 *   a simple-minded procedure for mapping names to addresses
 *
 *
 */

#include <xkern/include/xkernel.h>
#include <xkern/include/romopt.h>

#define DNS_MAP_SIZE           16
#define MAX_XKHOST_NAMELENGTH  64

static struct namenumber {
    char	h_name[MAX_XKHOST_NAMELENGTH];
    IPhost	h_addr;
} hostnametable[DNS_MAP_SIZE];

static initialized = 0;


#define min(a,b) ((a)<(b) ? (a):(b))

static xkern_return_t
addnametotable(
    char	**str,
    int		nfields,
    int		line,
    void	*arg)
{
    char	hn[MAX_XKHOST_NAMELENGTH];
    int		i;

    bzero(hn, MAX_XKHOST_NAMELENGTH);
    strncpy(hn, str[1], min(strlen(str[1]), MAX_XKHOST_NAMELENGTH));
    for (i=0; i<DNS_MAP_SIZE; i++) {
	if (!strcmp(hostnametable[i].h_name, hn))
	    /*
	     * don't overwrite existing entries; should really print warning
	     */
	    break;
	else if (!hostnametable[i].h_name[0]) {
	    strcpy(hostnametable[i].h_name, hn);
	    str2ipHost(&hostnametable[i].h_addr, str[2]);
	    break;
	}
    }
    return XK_SUCCESS;
}

static xkern_return_t
xknameresolve(
    char	*n,
    IPhost	*a)
{
    int		i;

    for (i=0; i<DNS_MAP_SIZE; i++) {
	if (!strcmp(hostnametable[i].h_name, n)) {
	    *a = hostnametable[i].h_addr;
	    return XK_SUCCESS;
	}
    }
    return XK_FAILURE;
}

/* looking for lines like
 *
 *   dns umbra 192.12.69.97
 *
 */

xkern_return_t
xk_gethostbyname(
     char	*namestr,
     IPhost	*addr)
{
    char	hn[MAX_XKHOST_NAMELENGTH];
    RomOpt	options[] = { {"", 3, addnametotable} };

    if (!initialized) {
	bzero((char *)hostnametable, sizeof(hostnametable));
	findRomOpts("dns", options, 1, 0);
	initialized = 1;
    }
  
    bzero(hn, MAX_XKHOST_NAMELENGTH);
    strncpy(hn, namestr, min(strlen(namestr), MAX_XKHOST_NAMELENGTH));
    return xknameresolve(hn, addr);
}
