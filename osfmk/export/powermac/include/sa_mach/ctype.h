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

#ifndef _CTYPE_H_
#define _CTYPE_H_

extern int	isalpha(int);
extern int	isalnum(int);
extern int	iscntrl(int);
extern int	isdigit(int);
extern int	isgraph(int);
extern int	islower(int);
extern int	isprint(int);
extern int	ispunct(int);
extern int	isspace(int);
extern int	isupper(int);
extern int	isxdigit(int);
extern int	toupper(int);
extern int	tolower(int);

extern int	isascii(int);
extern int	toascii(int);

extern int	(_toupper)(int);
extern int	(_tolower)(int);

#endif /* _CTYPE_H_ */
