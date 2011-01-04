#ifndef _ASM_PPC_BITOPS_H_
#define _ASM_PPC_BITOPS_H_

/*
 * Copyright 1996, Paul Mackerras.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <asm/system.h>
#include <asm/byteorder.h>

extern unsigned long set_bit(unsigned long nr, void *addr);
extern unsigned long clear_bit(unsigned long nr, void *addr);
extern unsigned long change_bit(unsigned long nr, void *addr);
extern int ffz(unsigned int);
extern unsigned long find_first_zero_bit(void * addr, unsigned long size);
extern unsigned long find_next_zero_bit(void * addr, unsigned long size,
					unsigned long offset);

extern __inline__ unsigned long test_bit(int nr, __const__ void *addr)
{
	__const__ unsigned int *p = (__const__ unsigned int *) addr;

	return (p[nr >> 5] >> (nr & 0x1f)) & 1;
}

#define _EXT2_HAVE_ASM_BITOPS_

#define ext2_set_bit(nr, addr)		set_bit((nr) ^ 0x18, addr)
#define ext2_clear_bit(nr, addr)	clear_bit((nr) ^ 0x18, addr)

extern __inline__ int ext2_test_bit(int nr, __const__ void * addr)
{
	__const__ unsigned char	*ADDR = (__const__ unsigned char *) addr;

	return (ADDR[nr >> 3] >> (nr & 7)) & 1;
}

/*
 * This implementation of ext2_find_{first,next}_zero_bit was stolen from
 * Linus' asm-alpha/bitops.h.
 */

#define ext2_find_first_zero_bit(addr, size) \
        ext2_find_next_zero_bit((addr), (size), 0)

extern __inline__ unsigned long ext2_find_next_zero_bit(void *addr,
	unsigned long size, unsigned long offset)
{
	unsigned long *p = ((unsigned long *) addr) + (offset >> 5);
	unsigned long result = offset & ~31UL;
	unsigned long tmp;

	if (offset >= size)
		return size;
	size -= result;
	offset &= 31UL;
	if(offset) {
		tmp = *(p++);
		tmp |= le32_to_cpu(~0UL >> (32-offset));
		if (size < 32)
			goto found_first;
		if (~tmp)
			goto found_middle;
		size -= 32;
		result += 32;
	}
	while (size & ~31UL) {
		if (~(tmp = *(p++)))
			goto found_middle;
		result += 32;
		size -= 32;
	}
	if (!size)
		return result;
	tmp = *p;

found_first:
	return result + ffz(le32_to_cpu(tmp) | (~0UL << size));
found_middle:
	return result + ffz(le32_to_cpu(tmp));
}

#ifdef __KERNEL__
#define minix_set_bit(nr, addr)		set_bit((nr), (addr))
#define minix_clear_bit(nr, addr)	clear_bit((nr), (addr))
#define minix_find_first_zero_bit(adr, sz) find_first_zero_bit((adr), (sz))
#endif /* __KERNEL__ */

#endif /* _ASM_PPC_BITOPS_H */
