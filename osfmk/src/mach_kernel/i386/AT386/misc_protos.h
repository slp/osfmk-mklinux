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

#include <chips/busses.h>		/* for struct bus_device */
#include <types.h>			/* for dev_t */
#include <mach/machine/vm_types.h>	/* for vm_offset_t */

/*
 * i386/AT386/model_dep.c
 */

extern void		i386_init(void);
extern void		machine_init(void);
extern void		machine_startup(void);
extern vm_offset_t	utime_map(
				dev_t		dev,
				vm_offset_t	off,
				int		prot);

/*
 * i386/AT386/kd.c
 */

extern void		cninit(void);
extern void		kdreboot(void);
extern void		feep(void);

/*
 * i386/AT386/autoconf.c
 */

extern void		probeio(void);
extern void		take_dev_irq(
				struct bus_device *dev);

/*
 * i386/locore.s
 */

extern void		kdb_kintr(void);
extern void		kgdb_kintr(void);

/*
 * i386/db_interface.c
 */

extern void		kdb_console(void);

/*
 * i386/bcopy.s
 */

extern void		bcopy16(
				char		* from,
				char		* to,
				int		count);
