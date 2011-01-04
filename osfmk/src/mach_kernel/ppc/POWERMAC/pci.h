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
 */
/*
 * Copyright 1991-1998 by Apple Computer, Inc. 
 *              All Rights Reserved 
 *  
 * Permission to use, copy, modify, and distribute this software and 
 * its documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appears in all copies and 
 * that both the copyright notice and this permission notice appear in 
 * supporting documentation. 
 *  
 * APPLE COMPUTER DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE. 
 *  
 * IN NO EVENT SHALL APPLE COMPUTER BE LIABLE FOR ANY SPECIAL, INDIRECT, OR 
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT, 
 * NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION 
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
 */
/*
 * MkLinux
 */

#ifndef _PPC_POWERMAC_PCI_H_
#define _PPC_POWERMAC_PCI_H_

#include <ppc/POWERMAC/device_tree.h>

boolean_t	powermac_is_coherent(void);
void		powermac_scan_bridges(unsigned int *first_avail);
unsigned long	powermac_pci_base(device_node_t *device);
void		powermac_display_pci_busses(void);
unsigned long	pci_io_base(int bus);
device_node_t	*pci_bus_device(int bus);
#endif
/*
 * @OSF_FREE_COPYRIGHT@
 * 
 */
/*
 * HISTORY
 * $Log: pci.h,v $
 * Revision 1.1.2.2  1998/01/12  10:34:20  stephen
 * 	Prototype changes
 * 	[1998/01/12  10:26:05  stephen]
 *
 * Revision 1.1.3.2  1998/01/12  10:26:05  stephen
 * 	Prototype changes
 *
 * Revision 1.1.2.1  1997/10/29  15:27:08  stephen
 * 	New file, for probing PCI
 * 	[1997/10/29  15:22:31  stephen]
 *
 * Revision 1.1.1.2  1997/10/29  15:22:31  stephen
 * 	New file, for probing PCI
 *
 */
