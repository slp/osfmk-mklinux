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
 * setjmp longjmp for ppc
 */
#ifndef	_PPC_SETJMP_H_
#define	_PPC_SETJMP_H_

/*
 * We save the following registers (marked as non-volatile in the ELF spec)
 *
 * r1      - stack pointer
 * r13     - small data area pointer
 * r14-r30 - local variables
 * r31     - local variable/environment pointer
 * 
 * cr      - condition register
 * lr      - link register (to know where to jump back to)
 * xer     - fixed point exception register
 *
 * fpscr   - floating point status and control
 * f14-f31 - local variables
 *
 * which comes to 57 words. We round up to 64 for good measure.
 */

typedef	struct jmp_buf {
	int	jmp_buf[64];
} jmp_buf_t;

#endif	/* _PPC_SETJMP_H_ */
