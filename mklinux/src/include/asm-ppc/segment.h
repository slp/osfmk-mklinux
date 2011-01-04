#ifndef _ASM_PPC_SEGMENT_H
#define _ASM_PPC_SEGMENT_H

#ifdef __KERNEL__
#include <linux/string.h>

extern void __put_user(unsigned long, void *, int);
extern unsigned long __get_user(const void *, int);
extern void memcpy_tofs(void *, void *, int);
extern void memcpy_fromfs(void *, void *, int);

#define put_user(x,ptr) __put_user((unsigned long)(x),(ptr),sizeof(*(ptr)))
#define get_user(ptr) ((__typeof__(*(ptr)))__get_user((ptr),sizeof(*(ptr))))
#define get_fs_byte(addr) __get_user((char *)(addr), sizeof(char))
#define get_fs_word(addr) __get_user((short *)(addr), sizeof(short))
#define get_fs_long(addr) __get_user((int *)(addr), sizeof(long))
#define put_fs_byte(x,addr) __put_user((x),(char *)(addr), sizeof(char))
#define put_fs_word(x,addr) __put_user((x),(short *)(addr), sizeof(short))
#define put_fs_long(x,addr) __put_user((x),(int *)(addr), sizeof(long))

/*
 * For segmented architectures, these are used to specify which segment
 * to use for the above functions.
 *
 * The powerpc is not segmented, so these are just dummies.
 */

#define KERNEL_DS 0
#define USER_DS 1

extern int get_fs(void);
extern int get_ds(void);
extern void set_fs(int);

#if 0
#define get_fs() 0
#define get_ds() 0
#define set_fs(x)
#endif
#endif

#endif /* _ASM_SEGMENT_H */
