/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#ifndef _ASM_OSFMACH3_I386_UNISTD_H
#define _ASM_OSFMACH3_I386_UNISTD_H

#define kernel_thread	__kernel_thread
#include <asm-i386/unistd.h>
#undef kernel_thread

#include <linux/types.h>

extern pid_t kernel_thread(int (*fn)(void *),
			   void *arg,
			   unsigned long flags);

#endif	/* _ASM_OSFMACH3_I386_UNISTD_H */
