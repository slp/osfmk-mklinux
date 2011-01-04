/*
 * Copyright 1991-1998 by Open Software Foundation, Inc. 
 *              All Rights Reserved 
 *  
 * Permission to use, copy, modify, and distribute this software and 
 * its documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appears in all copies and 
 * that both the copyright notice and this permission notice appear in 
 * supporting documentation. 
 *  
 * OSF DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE. 
 *  
 * IN NO EVENT SHALL OSF BE LIABLE FOR ANY SPECIAL, INDIRECT, OR 
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT, 
 * NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION 
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
 * 
 */
/*
 * MkLinux
 */

#ifndef	_POWERMAC_IO_H_
#define _POWERMAC_IO_H_

static void __inline__
outb(unsigned long addr, volatile unsigned char value)
{
	*(volatile unsigned char *) addr  = value;
	eieio();
}

static volatile unsigned char __inline__
inb(unsigned long addr)
{
	volatile unsigned char value;

	value = *(volatile unsigned char *) addr;
	eieio();

	return value;
}

static void __inline__
outw(unsigned long addr, volatile unsigned short value)
{
	*(volatile unsigned short *) addr = value; eieio();

}

static void __inline__
outw_le(unsigned long addr, unsigned short value)
{
	__asm__ volatile("sthbrx %0,0,%1" : :  "r" (value), "r" (addr) : "memory"); eieio();
}

static volatile unsigned short __inline__
inw_le(unsigned long addr)
{
	volatile unsigned short value;

	__asm__ volatile("lhbrx %0,0,%1" : "=r" (value) : "r" (addr)); eieio();

	return value;
}

static volatile unsigned long __inline__
inl_le(volatile unsigned long addr)
{
	volatile unsigned long value;

	__asm__ volatile("lwbrx %0,0,%1" : "=r" (value) : "r" (addr)); eieio();

	return value;
}

static void  __inline__
outl_le(volatile unsigned long addr, volatile unsigned long value)
{
	__asm__ volatile("stwbrx %0,0,%1" : : "r" (value) , "r" (addr) : "memory"); eieio();

	return;
}


static volatile unsigned short __inline__
inw(unsigned long addr)
{
	volatile unsigned short value;

	value = *(volatile unsigned short *) addr; eieio();

	return value;
}

static void __inline__
outsw(unsigned long addr, void *_value, long count)
{
	volatile unsigned short *value = _value;

	while (count-- > 0)
		outw(addr, *value++);
}

static void __inline__
insw_le(unsigned long addr, void *_value, long count)
{
	volatile unsigned short *value = _value;

	while (count-- > 0)
		*value++ = inw_le(addr);
}

static void __inline
insw(unsigned long addr, void *_value, long count)
{
	volatile unsigned short *value = _value;

	while (count-- > 0)
		*value++ = inw(addr);
}

static void __inline__
outsl(unsigned long addr, void *_value, long count)
{
	volatile unsigned long *value = _value;

	while (count-- > 0) {
		*(volatile unsigned long  *) addr = *value++; eieio();
	}
}

static void __inline__
insl_le(unsigned long addr, void *_value, long count)
{
	volatile unsigned long *value = _value;

	while (count-- > 0) {
		__asm__ volatile("lwbrx %0,0,%1" : "=r" (*value) : "r" (addr)); eieio();
		value++;
	}
}

static void __inline__
insl(unsigned long addr, void *_value, long count)
{
	volatile unsigned long *value = _value;

	while (count-- > 0) {
		*value++ = *(volatile unsigned long *)addr;
		eieio();
	}
}


static unsigned long __inline__
inl(volatile unsigned long addr)
{
	return	*(volatile unsigned long *) addr;
}

static void __inline__
outl(volatile unsigned long addr, unsigned long value)
{
	*(volatile unsigned long *) addr = value;
	eieio();
}

#endif
