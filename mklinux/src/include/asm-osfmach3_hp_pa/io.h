/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */


#ifndef _ASM_OSFMACH3_MACHINE_IO_H
#define _ASM_OSFMACH3_MACHINE_IO_H

unsigned char inb(int port);
unsigned char outb(unsigned char val,int port);

extern inline unsigned char  inb_p(int port) {return (inb(port)); }
extern inline unsigned char  outb_p(unsigned char val,int port) { return (outb(val,port)); }

#define readb(addr) (*(volatile unsigned char *) (addr))
#define readw(addr) (*(volatile unsigned short *) (addr))
#define readl(addr) (*(volatile unsigned int *) (addr))

#define writeb(b,addr) ((*(volatile unsigned char *) (addr)) = (b))
#define writew(b,addr) ((*(volatile unsigned short *) (addr)) = (b))
#define writel(b,addr) ((*(volatile unsigned int *) (addr)) = (b))

#endif	/* _ASM_OSFMACH3_MACHINE_IO_H */
