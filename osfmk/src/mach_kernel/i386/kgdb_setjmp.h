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
 * Buffer for x86 saved state used by kgdb_setjmp/kgdb_longjmp
 */
#ifndef	_KGDB_SETJMP_H_
#define	_KGDB_SETJMP_H_

typedef	struct kgdb_jmp_buf {
	int	jmp_buf[6];	/* x86 registers */
} kgdb_jmp_buf_t;

#define	JMP_BUF_NULL (kgdb_jmp_buf_t *) 0

extern int kgdb_setjmp(
	kgdb_jmp_buf_t	*kgdb_jmp_buf);

extern int kgdb_longjmp(
	kgdb_jmp_buf_t	*kgdb_jmp_buf,
	int		value);

#endif	/* _KGDB_SETJMP_H_ */
