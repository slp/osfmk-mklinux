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
 * File : endian.c
 *
 * Author : Eric PAIRE (O.S.F. Research Institute)
 *
 * This file contains all definitions depending on machine endianness.
 */


#include <secondary/net.h>
#include <secondary/net/endian.h>

#if	BYTE_ORDER == LITTLE_ENDIAN
u16bits
htons(u16bits val)
{
	return (((val & 0xFF00) >> 8) |
		((val & 0x00FF) << 8));
}

u32bits
htonl(u32bits val)
{
	return (((val & 0xFF000000) >> 24) |
		((val & 0x00FF0000) >>  8) |
		((val & 0x0000FF00) <<  8) |
		((val & 0x000000FF) << 24));
}
#endif
