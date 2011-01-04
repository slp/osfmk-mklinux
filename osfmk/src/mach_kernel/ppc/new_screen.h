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

#ifndef _NEW_SCREEN_H_
#define _NEW_SCREEN_H_

#include <ppc/boot.h>

/* AV and HPV cards */
#define	AV_BUFFER_START	   0xE0000000
#define	AV_BUFFER_END	   0xE0500000
#define	HPV_BUFFER_START   0xFE000000
#define	HPV_BUFFER_END	   0xFF000000

extern void clear_RGB16(int color);
extern void adj_position(unsigned char C);
extern void put_cursor(int color);
extern void screen_put_char(unsigned char C);
extern void initialize_screen(void *);
#endif /* _NEW_SCREEN_H_ */
