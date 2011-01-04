/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#ifndef	_OSFMACH3_SETJMP_H_
#define _OSFMACH3_SETJMP_H_

#include <asm/osfmach3_setjmp.h>

extern int osfmach3_setjmp(osfmach3_jmp_buf *jmp_buf);
extern void osfmach3_longjmp(osfmach3_jmp_buf *jmp_buf, int val);

#endif	/* _OSFMACH3_SETJMP_H_ */
