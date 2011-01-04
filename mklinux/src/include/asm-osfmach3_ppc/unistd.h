/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#ifndef _ASM_OSFMACH3_PPC_UNISTD_H
#define _ASM_OSFMACH3_PPC_UNISTD_H

#undef __KERNEL_SYSCALLS__
#include <asm-ppc/unistd.h>

#undef _syscall0
#undef _syscall1
#undef _syscall2
#undef _syscall3
#undef _syscall4
#undef _syscall5

#define _syscall0(type,name) \
type name(void) \
{ \
    long retval; \
    __asm__  ( \
	      "li 0, %1 \n\t" \
	      "sc \n\t" \
	      "mr %0,3 \n\t" \
	      "bns 10f \n\t" \
	      "mr 0,3 \n\t" \
	      "lis 3,errno@ha \n\t" \
	      "stw 0,errno@l(3) \n\t" \
	      "li %0,-1 \n\t" \
	      "10: \n\t" \
	      : "=r" (retval) \
	      : "i" (__NR_##name) \
	      : "0", "3", "cc", "memory" \
	      );  \
    return(retval);\
}

#define _syscall1(type,name,type1,arg1) \
type name(type1 arg1) \
{ \
    long retval; \
    __asm__  ( \
	      "li 0, %1 \n\t" \
	      "sc \n\t" \
	      "mr %0,3 \n\t" \
	      "bns 10f \n\t" \
	      "mr 0,3 \n\t" \
	      "lis 3,errno@ha \n\t" \
	      "stw 0,errno@l(3) \n\t" \
	      "li %0,-1 \n\t" \
	      "10: \n\t" \
	      : "=r" (retval) \
	      : "i" (__NR_##name) \
	      : "0", "3", "cc", "memory" \
	      );  \
    return(retval); \
}

#define _syscall2(type,name,type1,arg1,type2,arg2) \
type name(type1 arg1,type2 arg2) \
{ \
    long retval; \
    __asm__  ( \
	      "li 0, %1 \n\t" \
	      "sc \n\t" \
	      "mr %0,3 \n\t" \
	      "bns 10f \n\t" \
	      "mr 0,3 \n\t" \
	      "lis 3,errno@ha \n\t" \
	      "stw 0,errno@l(3) \n\t" \
	      "li %0,-1 \n\t" \
	      "10: \n\t" \
	      : "=r" (retval) \
	      : "i" (__NR_##name) \
	      : "0", "3", "cc", "memory" \
	      );  \
    return(retval); \
}


#define _syscall3(type,name,type1,arg1,type2,arg2,type3,arg3) \
type name(type1 arg1,type2 arg2, type3 arg3) \
{ \
    long retval; \
    __asm__  ( \
	      "li 0, %1 \n\t" \
	      "sc \n\t" \
	      "mr %0,3 \n\t" \
	      "bns 10f \n\t" \
	      "mr 0,3 \n\t" \
	      "lis 3,errno@ha \n\t" \
	      "stw 0,errno@l(3) \n\t" \
	      "li %0,-1 \n\t" \
	      "10: \n\t" \
	      : "=r" (retval) \
	      : "i" (__NR_##name) \
	      : "0", "3", "cc", "memory" \
	      );  \
    return(retval); \
}

#define _syscall4(type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4) \
type name (type1 arg1, type2 arg2, type3 arg3, type4 arg4) \
{ \
    long retval; \
    __asm__  ( \
	      "li 0, %1 \n\t" \
	      "sc \n\t" \
	      "mr %0,3 \n\t" \
	      "bns 10f \n\t" \
	      "mr 0,3 \n\t" \
	      "lis 3,errno@ha \n\t" \
	      "stw 0,errno@l(3) \n\t" \
	      "li %0,-1 \n\t" \
	      "10: \n\t" \
	      : "=r" (retval) \
	      : "i" (__NR_##name) \
	      : "0", "3", "cc", "memory" \
	      );  \
    return(retval); \
}

#define _syscall5(type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4, \
	  type5,arg5) \
type name (type1 arg1,type2 arg2,type3 arg3,type4 arg4,type5 arg5) \
{ \
    long retval; \
    __asm__  ( \
	      "li 0, %1 \n\t" \
	      "sc \n\t" \
	      "mr %0,3 \n\t" \
	      "bns 10f \n\t" \
	      "mr 0,3 \n\t" \
	      "lis 3,errno@ha \n\t" \
	      "stw 0,errno@l(3) \n\t" \
	      "li %0,-1 \n\t" \
	      "10: \n\t" \
	      : "=r" (retval) \
	      : "i" (__NR_##name) \
	      : "0", "3", "cc", "memory" \
	      );  \
    return(retval); \
}

#include <linux/types.h>

extern pid_t kernel_thread(int (*fn)(void *),
			   void *arg,
			   unsigned long flags);

#endif	/* _ASM_OSFMACH3_PPC_UNISTD_H */
