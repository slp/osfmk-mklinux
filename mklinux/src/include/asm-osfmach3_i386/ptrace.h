/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#ifndef _ASM_OSFMACH3_I386_PTRACE_H
#define _ASM_OSFMACH3_I386_PTRACE_H

#include <asm-i386/ptrace.h>

#define INIT_PTREGS { \
	0,	/* ebx */ \
	0,	/* ecx */ \
	0,	/* edx */ \
	0,	/* esi */ \
	0,	/* edi */ \
	0,	/* ebp */ \
	0,	/* eax */ \
	0, 0,	/* ds */ \
	0, 0,	/* es */ \
	0, 0, 	/* fs */ \
	0, 0, 	/* gs */ \
	0,	/* orig_eax */ \
	0,	/* eip */ \
	0, 0,	/* cs */ \
	0,	/* eflags */ \
	0, 	/* esp */ \
	0, 0	/* ss */ \
}

#endif	/* _ASM_OSFMACH3_I386_PTRACE_H */
