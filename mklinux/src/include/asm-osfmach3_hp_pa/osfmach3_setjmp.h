/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#ifndef	_ASM_OSFMACH3_MACHINE_OSFMACH3_SETJMP_H_
#define _ASM_OSFMACH3_MACHINE_OSFMACH3_SETJMP_H_

#include <mach_setjmp.h>

#define osfmach3_setjmp  mach_setjmp
#define osfmach3_longjmp  mach_longjmp

typedef int osfmach3_jmp_buf[_JBLEN];

#endif	/* _ASM_OSFMACH3_MACHINE_OSFMACH3_SETJMP_H_ */

