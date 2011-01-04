/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

/* $Id: bitops.h,v 1.1.2.2 1997/11/26 13:56:59 bruel Exp $
 * bitops.h: Bit string operations on the Sparc.
 *
 * Copyright 1995, David S. Miller (davem@caip.rutgers.edu).
 */


#ifndef _ASM_OSFMACH3_MACHINE_BITOPS_H
#define _ASM_OSFMACH3_MACHINE_BITOPS_H

#include <asm/system.h>
#include <asm/byteorder.h>

/* Set bit 'nr' in 32-bit quantity at address 'addr' where bit '0'
 * is in the highest of the four bytes and bit '31' is the high bit
 * within the first byte. Sparc is BIG-Endian. Unless noted otherwise
 * all bit-ops return 0 if bit was previously clear and != 0 otherwise.
 */

extern __inline__ unsigned long set_bit(unsigned long nr,  void *addr)
{
	int mask;
	unsigned long *ADDR = (unsigned long *) addr;
	unsigned long oldbit;

	ADDR += nr >> 5;
	mask = 1 << (nr & 31);
	oldbit = (mask & *ADDR);
	*ADDR |= mask;
	return oldbit != 0;
}

extern __inline__ unsigned long clear_bit(unsigned long nr, void *addr)
{
	int mask;
	unsigned long *ADDR = (unsigned long *) addr;
	unsigned long oldbit;

	ADDR += nr >> 5;
	mask = 1 << (nr & 31);
	oldbit = (mask & *ADDR);
	*ADDR &= ~mask;
	return oldbit != 0;
}

extern __inline__ unsigned long change_bit(unsigned long nr, void *addr)
{
	int mask;
	unsigned long *ADDR = (unsigned long *) addr;
	unsigned long oldbit;

	ADDR += nr >> 5;
	mask = 1 << (nr & 31);
	oldbit = (mask & *ADDR);
	*ADDR ^= mask;
	return oldbit != 0;
}

extern __inline__ unsigned long test_bit(int nr, void *addr)
{
	int	mask;
	unsigned long *ADDR = (unsigned long *) addr;
	unsigned long oldbit;

	ADDR += nr >> 5;
	mask = 1 << (nr & 31);
	oldbit = (mask & *ADDR);
	return oldbit != 0;
}

/* The easy/cheese version for now. */
extern __inline__ unsigned long ffz(unsigned long word)
{
	unsigned long result = 0;

	while(word & 1) {
		result++;
		word >>= 1;
	}
	return result;
}

/* find_next_zero_bit() finds the first zero bit in a bit string of length
 * 'size' bits, starting the search at bit 'offset'. This is largely based
 * on Linus's ALPHA routines, which are pretty portable BTW.
 */

extern __inline__ unsigned long find_next_zero_bit(void *addr, unsigned long size, unsigned long offset)
{
	unsigned long *p = ((unsigned long *) addr) + (offset >> 5);
	unsigned long result = offset & ~31UL;
	unsigned long tmp;

	if (offset >= size)
		return size;
	size -= result;
	offset &= 31UL;
	if (offset) {
		tmp = *(p++);
		tmp |= ~0UL >> (32-offset);
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
	tmp |= ~0UL >> size;
found_middle:
	return result + ffz(tmp);
}

#define find_first_zero_bit(addr, size) \
        find_next_zero_bit((addr), (size), 0)

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

#endif /* _ASM_OSFMACH3_MACHINE_BITOPS_H */


