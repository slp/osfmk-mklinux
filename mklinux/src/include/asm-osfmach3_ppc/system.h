/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#ifndef _ASM_OSFMACH3_PPC_SYSTEM_H
#define _ASM_OSFMACH3_PPC_SYSTEM_H

#include <osfmach3/system.h>

#define mb()  __asm__ __volatile__ (""   : : :"memory")

#define _disable_interrupts() 0
#define _enable_interrupts(x) ((x)=0)

#define xchg(ptr,x) ((__typeof__(*(ptr)))__xchg((unsigned long)(x),(ptr),sizeof(*(ptr))))

/* this guy lives in arch/ppc/kernel */
extern inline unsigned long *xchg_u32(void *m, unsigned long val);

/*
 *  these guys don't exist.
 *  someone should create them.
 *              -- Cort
 */
extern void *xchg_u64(void *ptr, unsigned long val);
extern int xchg_u8(char *m, char val);

/*
 * This function doesn't exist, so you'll get a linker error
 * if something tries to do an invalid xchg().
 *
 * This only works if the compiler isn't horribly bad at optimizing.
 * gcc-2.5.8 reportedly can't handle this, but as that doesn't work
 * too well on the alpha anyway..
 */
extern void __xchg_called_with_bad_pointer(void);

static inline unsigned long __xchg(unsigned long x, void * ptr, int size)
{
	switch (size) {
		case 4:
			return (unsigned long )xchg_u32(ptr, x);
		case 8:
			return (unsigned long )xchg_u64(ptr, x);
	}
	__xchg_called_with_bad_pointer();
	return x;


}

extern inline int tas(char * m)
{
	return xchg_u8(m,1);
}

#endif	/* _ASM_OSFMACH3_PPC_SYSTEM_H */
