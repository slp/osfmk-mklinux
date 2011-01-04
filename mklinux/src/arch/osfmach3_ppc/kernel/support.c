/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#include <linux/kernel.h>
#include <asm/bitops.h>

unsigned long set_bit(unsigned long nr, void *addr)
{
	unsigned long old, t;
	unsigned long mask = 1 << (nr & 0x1f);
	unsigned long *p = ((unsigned long *)addr) + (nr >> 5);

	if ((unsigned long)addr & 3)
		printk(KERN_ERR "set_bit(%lx, %p)\n", nr, addr);
	__asm__ __volatile__("\n\
1:	lwarx	%0,0,%3
	or	%1,%0,%2
	stwcx.	%1,0,%3
	bne	1b"
	: "=&r" (old), "=&r" (t)	/*, "=m" (*p)*/
	: "r" (mask), "r" (p)
	: "cc");

	return (old & mask) != 0;
}

unsigned long clear_bit(unsigned long nr, void *addr)
{
	unsigned long old, t;
	unsigned long mask = 1 << (nr & 0x1f);
	unsigned long *p = ((unsigned long *)addr) + (nr >> 5);

	if ((unsigned long)addr & 3)
		printk(KERN_ERR "clear_bit(%lx, %p)\n", nr, addr);
	__asm__ __volatile__("\n\
1:	lwarx	%0,0,%3
	andc	%1,%0,%2
	stwcx.	%1,0,%3
	bne	1b"
	: "=&r" (old), "=&r" (t)	/*, "=m" (*p)*/
	: "r" (mask), "r" (p)
	: "cc");

	return (old & mask) != 0;
}

unsigned long change_bit(unsigned long nr, void *addr)
{
	unsigned long old, t;
	unsigned long mask = 1 << (nr & 0x1f);
	unsigned long *p = ((unsigned long *)addr) + (nr >> 5);

	if ((unsigned long)addr & 3)
		printk(KERN_ERR "change_bit(%lx, %p)\n", nr, addr);
	__asm__ __volatile__("\n\
1:	lwarx	%0,0,%3
	xor	%1,%0,%2
	stwcx.	%1,0,%3
	bne	1b"
	: "=&r" (old), "=&r" (t)	/*, "=m" (*p)*/
	: "r" (mask), "r" (p)
	: "cc");

	return (old & mask) != 0;
}

int ffz(unsigned int x)
{
	int n;

	x = ~x & (x+1);		/* set LS zero to 1, other bits to 0 */
	__asm__ ("cntlzw %0,%1" : "=r" (n) : "r" (x));
	return 31 - n;
}

/*
 * This implementation of find_{first,next}_zero_bit was stolen from
 * Linus' asm-alpha/bitops.h.
 */

unsigned long find_first_zero_bit(void * addr, unsigned long size)
{
	unsigned int * p = ((unsigned int *) addr);
	unsigned int result = 0;
	unsigned int tmp;

	if (size == 0)
		return 0;
	while (size & ~31UL) {
		if (~(tmp = *(p++)))
			goto found_middle;
		result += 32;
		size -= 32;
	}
	if (!size)
		return result;
	tmp = *p;
	tmp |= ~0UL << size;
found_middle:
	return result + ffz(tmp);
}

/*
 * Find next zero bit in a bitmap reasonably efficiently..
 */
unsigned long find_next_zero_bit(void * addr, unsigned long size,
				 unsigned long offset)
{
	unsigned int * p = ((unsigned int *) addr) + (offset >> 5);
	unsigned int result = offset & ~31UL;
	unsigned int tmp;

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
	tmp |= ~0UL << size;
found_middle:
	return result + ffz(tmp);
}
