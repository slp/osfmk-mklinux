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
 * Taken from
 *
 *  Copyright (c) 1994	Wolfgang Stanglmeier, Koeln, Germany
 *			<wolf@dentaro.GUN.de>
 */

#ifndef __PCIBIOS_H__
#define __PCIBIOS_H__

/*
 *	the availability of a pci bus.
 *	configuration mode (1 or 2)
 *	0 if no pci bus found.
*/

int pci_conf_mode (void);

/*
 *	get a "ticket" for accessing a pci device
 *	configuration space.
*/

pcici_t pcitag (unsigned char bus,
		unsigned char device,
                unsigned char func);

/*
 *	read or write the configuration space.
*/

unsigned long pci_conf_read(pcici_t tag, unsigned long reg);
void   pci_conf_write(pcici_t tag, unsigned long reg, unsigned long data);

#endif
