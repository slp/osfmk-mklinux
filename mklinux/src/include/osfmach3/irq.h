/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#ifndef _OSFMACH3_IRQ_H
#define _OSFMACH3_IRQ_H

#include <linux/linkage.h>
#include <asm/segment.h>

#define NR_IRQS 0

extern void disable_irq(unsigned int);
extern void enable_irq(unsigned int);

#endif	/* _OSFMACH3_IRQ_H */
