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

#include <mach.h>
#include <stdio.h>

void
safe_gets(char *mod, char *str, int maxlen)
{
	register char *lp;
	register int c;
	char *strmax = str + maxlen - 1; /* allow space for trailing 0 */

	lp = str;

	while(*mod)
	{
		*lp++ = *mod;
		printf("%c", *mod);
		mod++;
	}

	for (;;) {
	    c = getchar();
	    switch (c) {
		case '\n':
		case '\r':
		    printf("\n");
		    *lp++ = 0;
		    return;

		case '\b':
		case '#':
		case '\177':
		    if (lp > str) {
			printf("\b \b");
			lp--;
		    }
		    continue;
		case '@':
		case 'u'&037:
		    lp = str;
		    printf("\n\r");
		    continue;
		default:
		    if (c >= ' ' && c < '\177') {
			if (lp < strmax) {
			    *lp++ = c;
			    printf("%c", c);
			}
			else {
			    printf("%c", '\007'); /* beep */
			}
		    }
	    }
	}
}
