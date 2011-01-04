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

#include <fddi.h>
#if	NFDDI > 0

#define	 INTERPHASE	0x25D0 /* INP in compressed form */

/************************************************************************/
/*
 * Prototypes.
 */
extern int  sh4811probe(struct hp_device *);
extern void sh4811attach(struct hp_device *);
extern int  sh4811init(int unit);
extern void sh4811reset(dev_t unit);
extern int  sh4811open(dev_t dev, dev_mode_t flag, io_req_t uio);
extern void sh4811close(dev_t dev);
extern void sh4811start(int unit);
extern void sh4811watch(caddr_t b_ptr);

extern void sh4811hwrst(int unit);
extern void sh4811config_init(int unit);
#if 0
extern void sh4811elm_init(dev_t unit);
extern void sh4811mac_init(dev_t unit);
extern int  sh4811fsi_init(dev_t unit);
extern void sh4811get_macaddr(dev_t unit, unsigned char *long_addr);
extern void sh4811set_macaddr(dev_t unit, unsigned char *long_addr);
#endif
extern void sh4811irq_init(int unit);
extern int  sh4811get_irq(int unit);
#if 0
extern void sh4811set_irq(dev_t unit, int irq);
#endif
extern void sh4811dumpconfig(int unit);

extern int  sh4811getstat(dev_t dev, dev_flavor_t flavor, dev_status_t status, 
			mach_msg_type_number_t *count);
extern int  sh4811setstat(dev_t dev, dev_flavor_t flavor, dev_status_t status, 
			mach_msg_type_number_t count);
extern int  sh4811setinput(dev_t unit, ipc_port_t rcv_port, int prio, filter_t filter[],
			   unsigned int count, device_t device);

#if 0
static int writeFCR(dev_t unit, int val);
static int writeCMR(dev_t unit, int val1, int val2);
#endif

/************************************************************************/

#endif
