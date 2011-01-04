/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#ifndef _ASM_OSFMACH3_I386_SEGMENT_H
#define _ASM_OSFMACH3_I386_SEGMENT_H

#include <osfmach3/segment.h>

#ifndef	__ASSEMBLY__

/*
 * Uh, these should become the main single-value transfer routines..
 * They automatically use the right size if we just have the right
 * pointer type..
 */
#define put_user(x,ptr) __put_user((unsigned long)(x),(ptr),sizeof(*(ptr)))
#define get_user(ptr) ((__typeof__(*(ptr)))__get_user((ptr),sizeof(*(ptr))))

/*
 * This is a silly but good way to make sure that
 * the __put_user function is indeed always optimized,
 * and that we use the correct sizes..
 */
extern int bad_user_access_length(void);

static inline void __put_user(unsigned long x, void * y, int size)
{
	switch (size) {
		case 1:
			if (get_fs() == get_ds()) {
				*(unsigned char *)y = (unsigned char) x;
			} else {
				unsigned char c;

				c = (unsigned char) x;
				copyout((char *) &c,
					(vm_address_t) y,
					size);
			}
			break;
		case 2:
			if (get_fs() == get_ds()) {
				*(unsigned short *)y = (unsigned short) x;
			} else {
				unsigned short s;

				s = (unsigned short) x;
				copyout((char *) &s,
					(vm_address_t) y,
					size);
			}
			break;
		case 4:
			if (get_fs() == get_ds()) {
				*(unsigned long *)y = (unsigned long) x;
			} else {
				copyout((char *) &x,
					(vm_address_t) y,
					size);
			}
			break;
		default:
			bad_user_access_length();
	}
}

static inline unsigned long __get_user(const void * y, int size)
{
	unsigned long result;

	switch (size) {
		case 1:
			if (get_fs() == get_ds()) {
				result = (unsigned long)*((unsigned char *) y);
			} else {
				unsigned char val;
				copyin((vm_address_t) y, (char *) &val, size);
				result = (unsigned long) val;
			}
			return (unsigned char) result;
		case 2:
			if (get_fs() == get_ds()) {
				result = (unsigned long)*((unsigned short *) y);
			} else {
				unsigned short val;
				copyin((vm_address_t) y, (char *) &val, size);
				result = (unsigned long) val;
			}
			return (unsigned short) result;
		case 4:
			if (get_fs() == get_ds()) {
				result = (unsigned long)*((unsigned long *) y);
			} else {
				copyin((vm_address_t) y, (char *)&result, size);
			}
			return result;
		default:
			return bad_user_access_length();
	}
}

/*
 * These are deprecated..
 *
 * Use "put_user()" and "get_user()" with the proper pointer types instead.
 */

#define get_fs_byte(addr) __get_user((const unsigned char *)(addr),1)
#define get_fs_word(addr) __get_user((const unsigned short *)(addr),2)
#define get_fs_long(addr) __get_user((const unsigned int *)(addr),4)

#define put_fs_byte(x,addr) __put_user((x),(unsigned char *)(addr),1)
#define put_fs_word(x,addr) __put_user((x),(unsigned short *)(addr),2)
#define put_fs_long(x,addr) __put_user((x),(unsigned int *)(addr),4)

#endif	/* __ASSEMBLY__ */

#endif	/* _ASM_OSFMACH3_I386_SEGMENT_H */
