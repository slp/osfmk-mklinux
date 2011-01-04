/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

/*
 * I/O 'port' access routines
 */

#include <linux/autoconf.h>

#ifdef	CONFIG_OSFMACH3
#include <linux/kernel.h>
#endif	/* CONFIG_OSFMACH3 */

/* This is really only correct for the MVME16xx (PreP)? */

#define _IO_BASE ((unsigned long)0x80000000)

unsigned char
inb(int port)
{
#ifdef	CONFIG_OSFMACH3
	printk("inb(%d) called from %p\n", port, __builtin_return_address(0));
	return (unsigned char) 0;
#else	/* CONFIG_OSFMACH3 */
	/* Ensure I/O operations complete */
	__asm__ volatile("eieio");
	return (*((unsigned  char *)(_IO_BASE+port)));
#endif	/* CONFIG_OSFMACH3 */
}

unsigned short
inw(int port)
{
#ifdef	CONFIG_OSFMACH3
	printk("inw(%d) called from %p\n", port, __builtin_return_address(0));
	return (unsigned short) 0;
#else	/* CONFIG_OSFMACH3 */
	/* Ensure I/O operations complete */
	__asm__ volatile("eieio");
	return (_LE_to_BE_short(*((unsigned short *)(_IO_BASE+port))));
#endif	/* CONFIG_OSFMACH3 */
}

unsigned long
inl(int port)
{
#ifdef	CONFIG_OSFMACH3
	printk("inl(%d) called from %p\n", port, __builtin_return_address(0));
	return (unsigned long) 0;
#else	/* CONFIG_OSFMACH3 */
	/* Ensure I/O operations complete */
	__asm__ volatile("eieio");
	return (_LE_to_BE_long(*((unsigned  long *)(_IO_BASE+port))));
#endif	/* CONFIG_OSFMACH3 */
}

void insb(int port, char *ptr, int len)
{
#ifdef	CONFIG_OSFMACH3
	printk("insb(%d) called from %p\n",
	       port, __builtin_return_address(0));
#else	/* CONFIG_OSFMACH3 */
	unsigned char *io_ptr = (unsigned char *)(_IO_BASE+port);
	/* Ensure I/O operations complete */
	__asm__ volatile("eieio");
	while (len-- > 0)
	{
		*ptr++ = *io_ptr;
	}
#endif	/* CONFIG_OSFMACH3 */
}

#if 0
void insw(int port, short *ptr, int len)
{
	unsigned short *io_ptr = (unsigned short *)(_IO_BASE+port);
	while (len-- > 0)
	{
		*ptr++ = _LE_to_BE_short(*io_ptr);
	}
}
#else
void insw(int port, short *ptr, int len)
{
#ifdef	CONFIG_OSFMACH3
	printk("insw(%d) called from %p\n",
	       port, __builtin_return_address(0));
#else	/* CONFIG_OSFMACH3 */
	unsigned short *io_ptr = (unsigned short *)(_IO_BASE+port);
	_insw(io_ptr, ptr, len);
#endif	/* CONFIG_OSFMACH3 */
}
#endif

void insw_unswapped(int port, short *ptr, int len)
{
#ifdef	CONFIG_OSFMACH3
	printk("insw_unswapped(%d) called from %p\n",
	       port, __builtin_return_address(0));
#else	/* CONFIG_OSFMACH3 */
	unsigned short *io_ptr = (unsigned short *)(_IO_BASE+port);
	/* Ensure I/O operations complete */
	__asm__ volatile("eieio");
	while (len-- > 0)
	{
		*ptr++ = *io_ptr;
	}
#endif	/* CONFIG_OSFMACH3 */
}

void insl(int port, long *ptr, int len)
{
#ifdef	CONFIG_OSFMACH3
	printk("insl(%d) called from %p\n",
	       port, __builtin_return_address(0));
#else	/* CONFIG_OSFMACH3 */
	unsigned long *io_ptr = (unsigned long *)(_IO_BASE+port);
	/* Ensure I/O operations complete */
	__asm__ volatile("eieio");
	while (len-- > 0)
	{
		*ptr++ = _LE_to_BE_long(*io_ptr);
	}
#endif	/* CONFIG_OSFMACH3 */
}

unsigned char  inb_p(int port) {return (inb(port)); }
unsigned short inw_p(int port) {return (inw(port)); }
unsigned long  inl_p(int port) {return (inl(port)); }

unsigned char
outb(unsigned char val,int port)
{
#ifdef	CONFIG_OSFMACH3
	printk("outb(%d) called from %p\n",
	       port, __builtin_return_address(0));
	return (unsigned char) 0;
#else	/* CONFIG_OSFMACH3 */
	/* Ensure I/O operations complete */
	__asm__ volatile("eieio");
	*((unsigned  char *)(_IO_BASE+port)) = (val);
	return (val);
#endif	/* CONFIG_OSFMACH3 */
}

unsigned short
outw(unsigned short val,int port)
{
#ifdef	CONFIG_OSFMACH3
	printk("outw(%d) called from %p\n",
	       port, __builtin_return_address(0));
	return (unsigned short) 0;
#else	/* CONFIG_OSFMACH3 */
	/* Ensure I/O operations complete */
	__asm__ volatile("eieio");
	*((unsigned  short *)(_IO_BASE+port)) = _LE_to_BE_short(val);
	return (val);
#endif	/* CONFIG_OSFMACH3 */
}

unsigned long
outl(unsigned long val,int port)
{
#ifdef	CONFIG_OSFMACH3
	printk("outl(%d) called from %p\n",
	       port, __builtin_return_address(0));
	return (unsigned long) 0;
#else	/* CONFIG_OSFMACH3 */
	/* Ensure I/O operations complete */
	__asm__ volatile("eieio");
	*((unsigned  long *)(_IO_BASE+port)) = _LE_to_BE_long(val);
	return (val);
#endif	/* CONFIG_OSFMACH3 */
}

void outsb(int port, char *ptr, int len)
{
#ifdef	CONFIG_OSFMACH3
	printk("outsb(%d) called from %p\n",
	       port, __builtin_return_address(0));
#else	/* CONFIG_OSFMACH3 */
	unsigned char *io_ptr = (unsigned char *)(_IO_BASE+port);
	/* Ensure I/O operations complete */
	__asm__ volatile("eieio");
	while (len-- > 0)
	{
		*io_ptr = *ptr++;
	}
#endif	/* CONFIG_OSFMACH3 */
}

#if 0
void outsw(int port, short *ptr, int len)
{
	unsigned short *io_ptr = (unsigned short *)(_IO_BASE+port);
	while (len-- > 0)
	{
		*io_ptr = _LE_to_BE_short(*ptr++);
	}
}
#else
void outsw(int port, short *ptr, int len)
{
#ifdef	CONFIG_OSFMACH3
	printk("outsw(%d) called from %p\n",
	       port, __builtin_return_address(0));
#else	/* CONFIG_OSFMACH3 */
	unsigned short *io_ptr = (unsigned short *)(_IO_BASE+port);
	_outsw(io_ptr, ptr, len);
#endif	/* CONFIG_OSFMACH3 */
}
#endif

void outsw_unswapped(int port, short *ptr, int len)
{
#ifdef	CONFIG_OSFMACH3
	printk("outsw_unswapped(%d) called from %p\n",
	       port, __builtin_return_address(0));
#else	/* CONFIG_OSFMACH3 */
	unsigned short *io_ptr = (unsigned short *)(_IO_BASE+port);
	/* Ensure I/O operations complete */
	__asm__ volatile("eieio");
	while (len-- > 0)
	{
		*io_ptr = *ptr++;
	}
#endif	/* CONFIG_OSFMACH3 */
}

void outsl(int port, long *ptr, int len)
{
#ifdef	CONFIG_OSFMACH3
	printk("outsl(%d) called from %p\n",
	       port, __builtin_return_address(0));
#else	/* CONFIG_OSFMACH3 */
	unsigned long *io_ptr = (unsigned long *)(_IO_BASE+port);
	/* Ensure I/O operations complete */
	__asm__ volatile("eieio");
	while (len-- > 0)
	{
		*io_ptr = _LE_to_BE_long(*ptr++);
	}
#endif	/* CONFIG_OSFMACH3 */
}

unsigned char  outb_p(unsigned char val,int port) { return (outb(val,port)); }
unsigned short outw_p(unsigned short val,int port) { return (outw(val,port)); }
unsigned long  outl_p(unsigned long val,int port) { return (outl(val,port)); }

