/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#define __LANGUAGE_ASSEMBLY
#define __NO_UNDERSCORES__
#define ASSEMBLER

#include <machine/asm.h>

#define SYS_getpid	20
#define SYS_sigaction	46
#define SYS_write	4
#define SYS_read	3
#define SYS_ioctl	54
#define SYS_exit	1

#if	__STDC__
#define SYSCALL(x)			\
	ENTRY(parent_osf1_ ## x);	\
	movl	$(SYS_ ## x),%eax;	\
	SVC;				\
	jb	parent_osf1_cerror;\
	ret
#else	/* __STDC__ */
#define SYSCALL(x)			\
	ENTRY(parent_osf1/**/x);	\
	movl	$(SYS_/**/x),%eax;	\
	SVC;				\
	jb	parent_osf1_cerror;\
	ret
#endif	/* __STDC__ */

SYSCALL(getpid)

SYSCALL(sigaction)

SYSCALL(write)

SYSCALL(read)

SYSCALL(ioctl)

SYSCALL(exit)

	.text
	.globl	parent_osf1_cerror
parent_osf1_cerror:
	movl	%eax,errno
	movl	$-1,%eax
	ret
