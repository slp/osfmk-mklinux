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
/* CMU_HIST */
/*
 * Revision 2.3  93/03/11  14:06:18  danner
 * 	Corrected atm_probe argument types.
 * 	[93/03/09            danner]
 * 
 * Revision 2.2  93/01/21  12:20:27  danner
 * 	Created.
 * 	[93/01/19  16:29:30  bershad]
 * 
 * Revision 2.3  92/07/09  22:53:10  rvb
 * 	Minimal support for mapping atm fifo's.
 * 	[92/05/12  17:09:58  rvb]
 * 
 * Revision 2.2  92/04/03  12:08:12  rpd
 * 	Created
 * 	[92/03/23            rvb]
 */
/* CMU_ENDHIST */

#include <atm.h>

#if	NATM > 0

#include <vm/vm_kern.h>
#include <sys/types.h>
#include <machine/machparam.h>		/* spl definitions */
#include <kern/time_out.h>		/* ? maybe */
#include <device/errno.h>
#include <device/io_req.h>
#include <device/net_status.h>

#include <chips/busses.h>
#include <chips/atmreg.h>

#include <kern/eventcount.h>

#include <mips/mips_cpu.h>


struct bus_device *atm_info[NATM];
int atm_probe();
static void atm_attach();

struct bus_driver atm_driver =
       { atm_probe, 0, atm_attach, 0, /* csr */ 0, "atm", atm_info,
       	 "", 0, /* flags */ 0 };		/* ENABLED BUS INTR? */

/* XX        	 "", 0,  BUS_INTR_DISABLED}; */

typedef struct	atm_softc {
       struct atm_device        *atm_dev;		
       struct evc 		atm_eventcounter;
       mapped_atm_info_t	atm_mapped_info;
} atm_softc_t;


natural_t atm_nintrs = 0;

atm_softc_t atm_softc[NATM];

atm_probe(reg, ui)
	vm_offset_t reg;
	register struct bus_device *ui;
{
    register atm_softc_t *atm;
    mapped_atm_info_t info; /* info struct to hand to users */	
    vm_offset_t 	addr;
    int             unit = ui->unit;

    if (check_memory(reg, 0))  {
	return 0;
    }

    atm_info[unit] = ui;
    atm = &atm_softc[unit];
    atm->atm_dev = (struct atm_device *) reg; /* k1 address */

    evc_init(&atm->atm_eventcounter);
    
printf("evc_init of atm: event counter id is %d\n", atm->atm_eventcounter.ev_id);
    
    /* initialize the interface to deliver. No interrupts by default */
    atm->atm_dev->sreg = 0;
    atm->atm_dev->creg = (CR_RX_RESET | CR_TX_RESET);
    atm->atm_dev->creg = 0;
    atm->atm_dev->creg_set = (CR_RX_ENABLE | CR_TX_ENABLE);
#ifdef notdef	
    atm->atm_dev->rxthresh = 0;
    atm->atm_dev->rxtimerv = 0;
    atm->atm_dev->creg_s = RX_EOM_INTR; /* enable interrupt on end of message */
#endif	

    /*
     * Grab a page to be mapped later to users 
     */
    (void) kmem_alloc_wired(kernel_map, &addr, PAGE_SIZE); /* kseg2 */
    bzero(addr, PAGE_SIZE);
    addr = pmap_extract(pmap_kernel(), addr); /* phys */
    info = (mapped_atm_info_t) PHYS_TO_K0SEG(addr);
    atm->atm_mapped_info = info;

    /*
     * Set some permanent info
     */
    info->hello_world = 0xdeadbeef;
    info->interrupt_count = 0;
    info->wait_event = atm->atm_eventcounter.ev_id;
    info->saved_status_reg = 0;

    return 1;
}

static void
atm_attach(ui)
register struct bus_device *ui;
{
}

int atm_disable_interrupts_after_each = 1;


#define ATM_INTERRUPTS (RX_COUNT_INTR | RX_EOM_INTR | RX_TIME_INTR)

atm_intr(unit, spllevel)
int unit;
int spllevel;
{
    register struct atm_softc	*atm = &atm_softc[unit];
    struct atm_device *atm_dev = atm->atm_dev;
    unsigned int intr;

    if (atm_dev == 0)  {
	printf("atm: stray interrupt\n");
	return;
    }

    /* Acknowledge interrupt request */
    intr = ATM_READ_REG(atm_dev->sreg);
    atm_dev->sreg = ~(intr & ATM_INTERRUPTS);

    /* clear the reason for the interrupt */
    if (atm_disable_interrupts_after_each)
	    atm_dev->creg &= ~intr;

    splx(spllevel); /* drop priority now */

    atm_intr_occurred();


    /* Pass status info up to user */
    if (atm->atm_mapped_info)  {
	atm->atm_mapped_info->interrupt_count++;
	atm->atm_mapped_info->saved_status_reg = intr;
    }

    /* Awake user thread */	

    evc_signal(&atm->atm_eventcounter);

    /* NOTE: INTERRUPTS ARE DISABLED. */
}

atm_intr_occurred()
{
    atm_nintrs++;
}


atm_output(dev, ior)
int		dev;
io_req_t	ior;
{
}

atm_start(unit)
int unit;
{
}

atm_open(dev, flag, ior)
	int		dev;
	int		flag;
	io_req_t	ior;
{
	register int	 	unit = dev;
	register atm_softc_t	*atm = &atm_softc[unit];
	
	if (unit >= NATM)
		return EINVAL;
	if (!atm->atm_dev)
		return ENXIO;

	return KERN_SUCCESS;
}

atm_close(dev, flag)
int	dev;
{
}

atm_read(dev, ior)
int		dev;
io_req_t	ior;
{
}

atm_write(dev, ior)
int		dev;
io_req_t	ior;
{
}

atm_get_status(dev, flavor, status, status_count)
	int		 dev;
	int		 flavor;
	dev_status_t	 status;	/* pointer to OUT array */
	natural_t	*status_count;		/* out */
{
    switch (flavor) {
    case NET_STATUS:
    {
	register struct net_status *ns = (struct net_status *)status;

	ns->min_packet_size = sizeof(struct sar_data);
	ns->max_packet_size = sizeof(struct sar_data);
	ns->header_format   = 999; /* XX */
	ns->header_size	    = sizeof(int); /* XX */
	ns->address_size    = 0;
	ns->flags	    = 0;
	ns->mapped_size	    = sizeof(struct atm_device) + PAGE_SIZE;
	*status_count = NET_STATUS_COUNT;
	break;
    }
    case NET_ADDRESS:
	/* This would be a good place for it */
    default:
	return (D_INVALID_OPERATION);
    }
    return (D_SUCCESS);    
}

atm_set_status(dev, flavor, status, status_count)
	int		dev;
	int		flavor;
	dev_status_t	status;
	natural_t	status_count;
{
}

atm_mmap(dev, off, prot)
	int		dev;
	vm_offset_t	off;
	int		prot;
{
	int 		unit = dev;
	vm_offset_t 	addr;

	/*
	 * Layout of mapped area is:
	 * 000 -- FA_END:			DEVICE
	 * FA_END -- FA_END + PAGE_SIZE:	SHARED STATE
	 */
	if (off < sizeof(struct atm_device))	 {
	    addr = K1SEG_TO_PHYS((vm_offset_t)(atm_softc[unit].atm_dev)) + off;
	} else if (off < sizeof(struct atm_device) + PAGE_SIZE) {
	    addr = K0SEG_TO_PHYS(atm_softc[unit].atm_mapped_info);
	} else return -1;
	return mips_btop(addr);
}

atm_setinput(dev, receive_port, priority, filter, filter_count)
	int		dev;
	ipc_port_t	receive_port;
	int		priority;
	/*filter_t	*filter;*/
	natural_t	filter_count;
{
}

atm_restart(ifp)
/*	register struct ifnet *ifp;	*/
{
}

atm_portdeath(dev, port)
	int		dev;
	mach_port_t	port;
{
}

#endif	/* NATM > 0 */





