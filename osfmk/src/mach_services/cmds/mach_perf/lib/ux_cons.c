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

#include <stdio.h>
#include <termio.h>

/*
 * Read a single character from stdin
 * (Used for more()).
 */

char
unix_read_char() {
	struct termio s, saved;
	char c;

	(void)ioctl(0, TCGETA, &s);
	saved = s;
	s.c_lflag &= ~(ICANON);
	s.c_cc[VMIN] = 1;
	(void)ioctl(0, TCSETAW, &s);
	read(0, &c, 1);
	(void)ioctl(0, TCSETAW, &saved);
	return(c);
}

/*
 * Set stdout to unbuffered mode
 */
void
unbuffered_output()
{
	setvbuf(stdout, 0, _IONBF, 0);
}
