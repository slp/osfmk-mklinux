/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#ifndef _OSFMACH3_SEGMENT_H
#define _OSFMACH3_SEGMENT_H

#include <mach/mach_types.h>

#define KERNEL_DS	0x100
#define USER_DS		0x101

extern void memcpy_tofs(void * to, const void * from, unsigned long n);
extern void memcpy_fromfs(void * to, const void * from, unsigned long n);
extern int strlen_fromfs(const char *addr);
extern int copystr_fromfs(char *to, const char *from, int n);
extern void copyout(char *from, vm_address_t to, unsigned long count);
extern void copyin(vm_address_t from, char *to, unsigned long count);

extern unsigned long get_fs(void);
extern void set_fs(unsigned long val);
static __inline__ unsigned long
get_ds(void)
{
	return KERNEL_DS;
}

#endif /* _OSFMACH3_SEGMENT_H */
