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
 * net_debug.c
 */
#if	defined(NETDEBUG) || defined(PROTODEBUG)

#include <secondary/net.h>
#include <secondary/net_debug.h>

int netdebug = 1;
int protodebug = 1;

void dump_packet(const char *banner, char *addr, int len);
void debug_getchar(void);

void
dump_packet(const char *banner, char *addr, int len)
{
	register int i;
	printf("%s: ",banner);
	for (i = 0; i < 128; i++) {
		u8bits val = ((char *)addr)[i];
		if ((val >> 4) >= 10)
			putchar((val >> 4) - 10 + 'A');
		else
			putchar((val >> 4) + '0');
		if ((val & 0xF) > 9)
			putchar((val & 0xF) - 10 + 'A');
		else
			putchar((val & 0xF) + '0');
		putchar(' ');
	}
	putchar('\n');
}

void
debug_getchar(void)
{
	printf("Press a key to continue...");\
        getchar();
	putchar('\n');
}
#endif
