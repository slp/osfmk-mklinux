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

#include <busses/pci/pci.h>
#include <i386/pci/pci.h>
#include <i386/pci/pci_device.h>

#include <ncr810.h>
#if NNCR810>0
extern struct pci_driver ncr810device;
extern struct pci_driver ncr825device;
#endif

#include <ncr825.h>
#if NNCR825>0
extern struct pci_driver ncr825device;
#endif

#include <adsl.h>
#if NADSL>0
extern struct pci_driver adaptec7850_device;
extern struct pci_driver adaptec7870_device;
#endif

#include <fta.h>
#if NFTA > 0
extern struct pci_driver ftadevice;
#endif

#include <tu.h>
#if NTU > 0
extern struct pci_driver tudevice;
#endif

struct pci_device pci_devtab[] = {

#if NNCR810>0
	{&ncr810device},
	{&ncr825device},
#endif
#if NNCR825>0
	{&ncr825device},
#endif
#if NFTA > 0
	{ &ftadevice },
#endif
#if NTU > 0
	{ &tudevice },
#endif
#if NADSL>0
        {&adaptec7850_device},
        {&adaptec7870_device},
#endif
	{0}
};









