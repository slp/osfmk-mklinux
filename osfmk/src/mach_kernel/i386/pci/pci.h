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
/*
 * Taken from FREEBSD
**
**  Copyright (c) 1994	Wolfgang Stanglmeier, Koeln, Germany
**			<wolf@dentaro.GUN.de>
*/

#ifndef __PCI_H__
#define __PCI_H__

#include <busses/pci/pci.h>
/*
**  main pci initialization function.
**  called at boot time from autoconf.c
*/

void pci_configure(void);

/*
**  pci configuration id
**
**  is constructed from: bus, device & function numbers.
*/

typedef union {
	unsigned long cfg1;
        struct {
		 unsigned char   enable;
		 unsigned char   forward;
		 unsigned short  port;
	       } cfg2;
	} pcici_t;

#endif	/*__PCI_H__*/




