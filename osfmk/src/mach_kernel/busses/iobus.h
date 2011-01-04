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
 * MkLinux
 */

#ifndef	_BUSSES_IOBUS_H_
#define	_BUSSES_IOBUS_H_

#include <machine/iobus.h>

/*
 * IOBUS_TYPE contains the type of bus, the bus number and some bus flags
 */
#define	IOBUS_TYPE_BUS(iotype)		(((iotype) >> 24) & 0xFF)
#define	IOBUS_TYPE_NUM(iotype)		(((iotype) >> 16) & 0xFF)
#define	IOBUS_TYPE_FLAGS(iotype)	 ((iotype) & 0xFFFF)
#define	IOBUS_TYPE(bus, num, flags)	((((bus) & 0xFF) << 24) | \
					 (((num) & 0xFF) << 16) | \
					 ((flags) & 0xFFFF))

#define	IOBUS_TYPE_BUS_UNKNOWN		0
#define	IOBUS_TYPE_BUS_ISA		1
#define	IOBUS_TYPE_BUS_EISA		2
#define	IOBUS_TYPE_BUS_PCI		3
#define	IOBUS_TYPE_BUS_GSC		4
#define	IOBUS_TYPE_BUS_CARDBUS		5
#define	IOBUS_TYPE_BUS_PCMCIA		6

#define	IOBUS_TYPE_NAME(iotype)						\
    (IOBUS_TYPE_BUS(iotype) == IOBUS_TYPE_BUS_CARDBUS ? "CARDBUS" :	\
     IOBUS_TYPE_BUS(iotype) == IOBUS_TYPE_BUS_PCMCIA  ? "PCMCIA"  :	\
     IOBUS_TYPE_BUS(iotype) == IOBUS_TYPE_BUS_GSC     ? "GSC"     :	\
     IOBUS_TYPE_BUS(iotype) == IOBUS_TYPE_BUS_PCI     ? "PCI"     :	\
     IOBUS_TYPE_BUS(iotype) == IOBUS_TYPE_BUS_EISA    ? "EISA"    :	\
     IOBUS_TYPE_BUS(iotype) == IOBUS_TYPE_BUS_ISA     ? "ISA"     : "UNKNOWN")

#define	IOBUS_TYPE_FLAGS_NOTCOHERENT	0x0001	/* I/O *NOT* cache coherent */

#endif /* _BUSSES_IOBUS_H_ */
