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

#include <fddi.h>
#if	NFDDI > 0

#include <types.h>
#include <kern/spl.h>
#include <sys/time.h>
#include <kern/misc_protos.h>
#include <hp_pa/misc_protos.h>
#include <hp_pa/trap.h>
#include <hp_pa/regs.h>
#include <hp_pa/asp.h>

#include <mach/machine/vm_types.h>
#include <kern/time_out.h>
#include <device/device_types.h>
#include <device/driver_lock.h>
#include <device/errno.h>
#include <device/io_req.h>
#include <device/if_hdr.h>
#include <device/if_fddi.h>
#include <device/net_status.h>
#include <device/net_io.h>
#include <device/ds_routines.h>
#include <ipc/ipc_kmsg.h>
#include <device/if_llc.h>

#include <hp_pa/HP700/device.h>
#include <hp_pa/HP700/eisa_common.h>
#include <hp_pa/HP700/sh4811/iphase_cbrc.h>
#include <hp_pa/HP700/sh4811/sh4811_ring.h>
#include <hp_pa/HP700/sh4811/if_sh4811.h>
#include <hp_pa/HP700/sh4811/if_sh4811var.h>


static sh4811_softc_t	sh4811_softc[NFDDI];

/* The following are the possible IRQ values for the SeaHawk.
   The configuration registers for FSI and Timer/CPU use the last two
   bits for get/set the IRQ.
*/
static int sh4811_irq[] = {5, 15, 11, 9}; 

static int     sh4811timo = 100;

/* create a pool for seahawk to receive packets. Each packet is 
   PAGE_SIZE
*/
net_pool_t seahawk_pagepool;


/* ------------------------------------------------------------------------  */

#define	DSF_LOCK	1
#define	DSF_RUNNING	2

#define	FCR_write(unit,val)	{ if(!writeFCR(unit,val)) return(0); }
#define	CMR_write(unit,val1,val2)  { if(!writeCMR(unit,val1,val2)) return(0); }

int
sh4811probe(struct hp_device *dev)
{
        unsigned char *addr = (unsigned char *)dev->hp_addr;
        int unit = minor(dev->hp_unit);
	int boardid;

	if(!(boardid=geteisa_boardid((struct iomod *)addr, 0)))
		return(0);
	/* verify that board id matches that of SeaHawk Interphase board */
	if(boardid != INP_SEAHAWK)
		return(0);
	printf("Interphase Seahawk 4811 EISA/FDDI board - unit %d\n", unit);
	return(1);

}

void
sh4811attach(struct hp_device *dev)
{
        struct  ifnet   *ifp;
	int unit = minor(dev->hp_unit);
	sh4811_softc_t  *sp = &sh4811_softc[unit];
	int irq;

	sp->hwaddr = (unsigned char *)dev->hp_addr;
        sp->seated = TRUE;
        sp->configured = FALSE;

#if FDDITEST
	driver_lock_init(&sp->lock, unit, sh4811start, sh4811intr,
			 fddi_test_thread, sh4811_callback_receive);
#else
	driver_lock_init(&sp->lock, unit, sh4811start, sh4811intr,
			 (void (*)(int))0, sh4811_callback_receive);
#endif

	sh4811config_init(unit);
	sh4811hwrst(unit);
        queue_init(&sp->multicast_queue);

	/* initialize eisa irq settings */
	sh4811irq_init(unit);

	/* ifnet related */
        ifp = &(sp->ds_if);
        ifp->if_unit = unit;
        ifp->if_flags = IFF_SNAP | IFF_BROADCAST | IFF_MULTICAST;
        ifp->if_timer = 0;
        ifp->if_header_size = sizeof(struct fddi_header) + sizeof(struct llc);
	/* server considers mtu as follows */
        ifp->if_mtu = FDDIMTU;
        ifp->if_header_format = HDR_ETHERNET; /* not a typo, see RFC 1390 */
        ifp->if_address_size = FDDI_ADDR_SIZE;
        ifp->if_address = (char *)sp->can_mac_addr;
        if_init_queues(ifp);
	printf("IN sh4811attach- board reset and enabled\n");
}

void
sh4811reset(dev_t dev)
{
	int unit = minor(dev);
        sh4811_softc_t  *sp = &sh4811_softc[unit];
	driver_lock_state();

	(void)driver_lock(&sp->lock, DRIVER_OP_WAIT, FALSE);
        sp->ds_if.if_flags &= ~IFF_RUNNING;
#ifdef IFF_OACTIVE
        sp->ds_if.if_flags &= ~IFF_OACTIVE;
#endif
        sp->flags &= ~(DSF_LOCK|DSF_RUNNING);
	iphase_board_close(sp);
	sh4811hwrst(unit);
	sh4811init(unit);
	driver_unlock(&sp->lock);
}

int
sh4811init(int unit)
{
        sh4811_softc_t  *sp = &sh4811_softc[unit];
        struct  ifnet   *ifp=&(sp->ds_if);
        unsigned long   oldpri;

	ifp->if_flags &= ~IFF_RUNNING;
        sp->flags &= ~(DSF_LOCK|DSF_RUNNING);

	sh4811_board_init(sp);

        oldpri = splimp();
#if 0
	untimeout((timeout_fcn_t)sh4811watch, &(ifp->if_unit));
	timeout((timeout_fcn_t)sh4811watch, &(ifp->if_unit), sh4811timo*hz);
	sp->timer = sh4811timo;
#endif
	ifp->if_flags |= IFF_RUNNING;
	sp->flags |= DSF_RUNNING;
#ifdef IFF_OACTIVE
	ifp->if_flags &= ~IFF_OACTIVE;
#else
	sp->tbusy = 0;
#endif
        splx(oldpri);

	while(1) {
        	io_req_t                m;

        	IF_DEQUEUE(&ifp->if_snd, m);
        	if (m) {
			FDDIPRINTF(FDDIDEBUG_DEBUG,
				   ("packet in send queue during init\n"));
        	} /* end if(m) */
        	if (queue_empty(&ifp->if_snd.ifq_head))
                	break;
	} /* end while */

#ifdef	FDDITEST
	sh4811FN();
#endif

        return(D_SUCCESS);
}

int
sh4811open(dev_t dev, dev_mode_t flag, io_req_t uio)
{
	sh4811_softc_t  *sp;
        register int    unit;
	driver_lock_state();

        unit = minor(dev);      /* XXX */
	sp = &sh4811_softc[unit];
        if (unit < 0 || unit >= NFDDI || !sp->seated)
            return(D_NO_SUCH_DEVICE);

	(void)driver_lock(&sp->lock, DRIVER_OP_WAIT, FALSE);
        sh4811_softc[unit].ds_if.if_flags |= IFF_UP;
        sh4811init(unit);
	driver_unlock(&sp->lock);

        return(D_SUCCESS);
}

void
sh4811close(dev_t dev)
{
        register int    unit;
        sh4811_softc_t  *sp = &sh4811_softc[unit];
        struct  ifnet   *ifp=&(sp->ds_if);
	driver_lock_state();

        unit = minor(dev);      /* XXX */
        if (unit < 0 || unit >= NFDDI || !sh4811_softc[unit].seated)
            return;

	(void)driver_lock(&sp->lock, DRIVER_OP_WAIT, FALSE);
 	iphase_board_close(sp);
        if(ifp->if_flags & IFF_UP)
        	ifp->if_flags &= ~IFF_UP;
	driver_unlock(&sp->lock);

        return;
}

void
sh4811_callback_receive(int unit)
{
	sh4811_softc_t *sp = &sh4811_softc[unit];
	unsigned int missed;
	spl_t s;
	driver_lock_state();

	s = splimp();
	if (driver_lock(&sp->lock, DRIVER_OP_CALLB, FALSE)) {
	    assert(sp->callback_missed > 0);
	    missed = sp->callback_missed;
	    sp->callback_missed = 0;
	    sh4811_setup_receive(sp, missed);
	    driver_unlock(&sp->lock);
	}
	splx(s);
}

void
sh4811hwrst(int unit)
{
	sh4811_softc_t  *sp = &sh4811_softc[unit];
        volatile unsigned char *addr = sp->hwaddr;
        unsigned long   oldpri;

	oldpri = splimp();

	/* set reset bit of board control register */
	*(addr+EISA_BOARD_CONTROL) = 0x04 ;
	/* hold reset bit for 1 millisec */
	delay(1000);
	/* clear reset bit and then enable board */
	*(addr+EISA_BOARD_CONTROL) = 0x00 ; 
	*(addr+EISA_BOARD_CONTROL) = 0x01 ; 

	splx(oldpri);

}

int
sh4811_board_init(sh4811_softc_t *sp)
{
	volatile unsigned char *addr;
	ipc_kmsg_t new_kmsg;
	extern net_pool_t inline_pagepool;
        unsigned long   oldpri;

        /*
         * Create a message pool for FDDI packets.
         */
        if (net_kmsg_create_pool(NET_POOL_INLINE,
                sizeof(struct seahawk_rcv_msg),
                SH_NUM_DESC*3,
                FALSE, "seahawk-pagepool",
                &seahawk_pagepool) != KERN_SUCCESS) {
                        panic("sh4811_board_init: cannot create message pool");
        }
	sp->rcv_msg_pool = seahawk_pagepool;
	net_kmsg_more(sp->rcv_msg_pool);

	sp->callback.ncb_fct = (void (*)(int))sh4811_callback_receive;
	sp->callback.ncb_arg = sp->ds_if.if_unit;

	if(!iphase_sram_info(sp))  {
		int i;
		volatile unsigned int *iaddr;
		FDDIPRINTF(FDDIDEBUG_DEBUG, ("\n\n"));
		for(i=0; i<4;i++) {
			iaddr = (volatile unsigned int *)
			    (0xFD000000+(i*0x1000));
			*iaddr;
			delay(1000);
			FDDIPRINTF(FDDIDEBUG_DEBUG,
				   ("A:%x, %x - ", iaddr, *iaddr));
		}
		FDDIPRINTF(FDDIDEBUG_DEBUG, ("\n\n"));
		return(0);
	}

	/* Enable edge sensitive timer/CPU interrupt states */
	addr = sp->hwaddr + FSI_MAC_ELM_INTERRUPT_CONFIG;
	*addr |= 0x28; 
	addr = sp->hwaddr + TIMER_CPU_INTERRUPT_CONFIG;
	*addr |= 0x38;

	sh4811_alloc_rings(sp); 
	iphase_board_init(sp); 
	sh4811_trace_smt(sp, SH4811_TRACE_ON);
	iphase_set_intr(sp); 
	iphase_req_stat(sp); 
	sh4811_get_addr(sp);

	oldpri=splimp();
	sh4811_create_rings(sp);
	assert(sp->indication == 0);
	sp->indication = FSI_CREATE_RING_RESP;
	while (sp->indication == FSI_CREATE_RING_RESP) {
		/* Wait for FSI response */
		assert_wait((event_t)&sp->indication, TRUE);
		driver_unlock(&sp->lock);
		thread_block((void (*)(void))0);
		(void)driver_lock(&sp->lock, DRIVER_OP_WAIT, FALSE);
	}
	assert(sp->indication == 0);
	sh4811_setup_receive(sp, SH_NUM_DESC);
	splx(oldpri);

	sh4811_trace_smt(sp, SH4811_TRACE_OFF); /* Remove SMT events for now */
	sh4811_connect_ring(sp, 0);
	sh4811_connect_ring(sp, 1);
	return(1);
}

void
sh4811config_init(int unit)
{
        sh4811_softc_t  *sp = &sh4811_softc[unit];
        volatile unsigned char *addr;
	unsigned int taddr=0;
	register int i;
	
	/* all of these values should be obtained from the EISA 
	   configuration file. On HPUX these are stored in EEPROM
	   and loaded on startup. Here we are hardcoding these values */
        addr = sp->hwaddr + 0xC8E;
        for(i=0;i<3;i++)
		taddr |= (*addr++ << ((i + 1) * BYTE_SIZE));
	sp->sram_paddr=taddr;
	sp->sram_vaddr = (unsigned char *)map_eisa_to_host(sp->sram_paddr);

        addr = sp->hwaddr;
        *(addr+FSI_A_PORT_ADDR) = 0x0;
        *(addr+FSI_A_PORT_ADDR+1) = 0x0;
        *(addr+STATIC_RAM_ADDR) = 0x0;
        *(addr+STATIC_RAM_ADDR+1) = 0x0;
        *(addr+ MAC_ELM_ADDR) = 0x0;
        *(addr+ MAC_ELM_ADDR+1) = 0x0;
	
        *(addr+FSI_A_PORT_MASK) = 0x3F;
        *(addr+FSI_A_PORT_MASK+1) = 0xF0;
        *(addr+STATIC_RAM_MASK) = 0x0;
        *(addr+STATIC_RAM_MASK+1) = 0x0;
        *(addr+ MAC_ELM_MASK) = 0x0;
        *(addr+ MAC_ELM_MASK+1) = 0x0;

        *(addr+MEM_ADDR_DECODE) = 0x3F;
        *(addr+FSI_A_PORT_DECODE) = 0x3F;
        *(addr+STATIC_RAM_DECODE) = 0x0;
        *(addr+ MAC_ELM_DECODE) = 0x0;

        *(addr+ ADDRESS_DECODE_ENABLE) = 0x21;
}

void
sh4811irq_init(int unit)
{
	sh4811_softc_t  *sp = &sh4811_softc[unit];
	int irq;

	/* Get current irq settings from board and make sure that
           it is not in conflict with an existing setting */
	irq = sh4811get_irq(unit);
	if(eisa_irq_in_use(sh4811_irq[irq])) {
		register int tries=0;
		register int numelem=sizeof(sh4811_irq)/sizeof(int);
	
		irq = ((irq == 1) ? 0 : irq+1);
		while(++tries <  numelem && eisa_irq_in_use(sh4811_irq[irq]))
			irq = ((irq == 1) ? 0 : irq + 1);
		if(tries >= numelem)
			panic("Unable to set EISA irq for SeaHawk board");
	}
	eisa_irq_register(unit, sh4811_irq[irq], sh4811intr);
	sp->irq=sh4811_irq[irq];
}

int
sh4811get_irq(int unit)
{ 
	sh4811_softc_t  *sp = &sh4811_softc[unit];
        volatile unsigned char *addr ;
	int irq1, irq2;

        addr = sp->hwaddr + FSI_MAC_ELM_INTERRUPT_CONFIG;
	irq1=*addr & EISA_IRQ_ASSIGNMENT;
        addr = sp->hwaddr + TIMER_CPU_INTERRUPT_CONFIG;
	irq2=*addr & EISA_IRQ_ASSIGNMENT;
	if(irq2 != irq1)
		*addr = (*addr & 0xFC) | irq1 | 0x10;
	return(irq1);
}

#if 0
void
sh4811set_irq(dev_t unit, int irq)
{ 
	sh4811_softc_t  *sp = &sh4811_softc[unit];
        volatile unsigned char *addr ;

	/* Enable level sensitive FSI/MAC interrupt states */
	irq = 0;
        addr = sp->hwaddr + FSI_MAC_ELM_INTERRUPT_CONFIG;
	*addr = (*addr & 0xFC) | irq ;
	eisa_irq_register(unit, sh4811_irq[irq], (void (*)(void *))sh4811Fintr, 
			(void *)sp);
        addr = sp->hwaddr + TIMER_CPU_INTERRUPT_CONFIG;
	/* Enable edge sensitive timer/CPU interrupt states */
	irq = 2;
	*addr = (*addr & 0xFC) | irq ; 
	eisa_irq_register(unit, sh4811_irq[irq], (void (*)(void *))sh4811Cintr, 
			(void *)sp);
}

int
sh4811Fintr(sh4811_softc_t *sp)
{
	volatile unsigned int *iaddr, mask;
        unsigned long oldpri;

	sp->timer=-1;
	if(sp->configured == FALSE) {
		FDDIPRINTF(FDDIDEBUG_DEBUG, ("Got unconfigured F interrupt\n"));
		return(0);
	}
	if(is_fsi_intr(sp)) {
		sh4811_fsi_intr(sp);
	} else {
		FDDIPRINTF(FDDIDEBUG_DEBUG, ("Invalid F interrupt\n"));
		return(-1);
	}

        iaddr=(unsigned int *)(sp->hwaddr + FSI_INTR_MASK1);
        mask=FSI_INT_MASK;
        swap_long(iaddr, &mask);

	return(1);
}

int
sh4811Cintr(sh4811_softc_t *sp)
{
	sp->timer=-1;
	if(sp->configured == FALSE) {
		FDDIPRINTF(FDDIDEBUG_DEBUG, ("Got unconfigured C interrupt\n"));
		return(0);
	}
	if(is_cpu_intr(sp))
		return(sh4811_rc_intr(sp));
	FDDIPRINTF(FDDIDEBUG_DEBUG, ("Invalid C interrupt\n"));
	return(1);
}
#endif

void
sh4811intr(int unit)
{
        sh4811_softc_t *sp = &sh4811_softc[unit];
	boolean_t valid;
	driver_lock_state();

	if (!driver_lock(&sp->lock, DRIVER_OP_INTR, FALSE))
		return;

	sp->timer=-1;
	if(sp->configured == FALSE) {
		driver_unlock(&sp->lock);
		FDDIPRINTF(FDDIDEBUG_DEBUG, ("Got unconfigured interrupt\n"));
		return;
	}
	if(is_fsi_intr(sp)) {
		sh4811_fsi_intr(sp);
		valid = TRUE;
	} else
	    valid = FALSE;
	if(is_cpu_intr(sp)) {
		sh4811_rc_intr(sp);
		valid = TRUE;
	}
	driver_unlock(&sp->lock);
	if (!valid)
		FDDIPRINTF(FDDIDEBUG_DEBUG, ("Invalid fddi interrupt\n"));
	return;
}

int  
sh4811output(dev_t dev, io_req_t ior)
{
	register int    unit;

        unit = minor(dev);      /* XXX */
        if (unit < 0 || unit >= NFDDI || !sh4811_softc[unit].configured)
            return(D_NO_SUCH_DEVICE);
	return (net_write(&sh4811_softc[unit].ds_if, (void (*)(int))sh4811start, ior));
}


void
sh4811start(int unit)
{
        struct  ifnet           *ifp;
        register sh4811_softc_t     *sp = &sh4811_softc[unit];
	driver_lock_state();

	if (!driver_lock(&sp->lock, DRIVER_OP_START, FALSE))
		return;

	FDDIPRINTF(FDDIDEBUG_START,
		   ("seahawk%d: start() busy %d\n", unit, sp->tbusy));

	ifp = &(sh4811_softc[unit].ds_if);
#ifdef IFF_OACTIVE
	ifp->if_flags |= IFF_OACTIVE;
#endif

	while(1) {
		io_req_t                ior;
#ifdef FDDITEST
		register int		count=0;

		if(++count > 6) {
			FDDIPRINTF(FDDIDEBUG_DEBUG,
				   ("fddi transmit count %d\n", count)); 
			driver_unlock(&sp->lock);
			return;
		}
		if(++sp->tbusy > 3) {
			FDDIPRINTF(FDDIDEBUG_DEBUG, ("tbusy %d\n", sp->tbusy));
		}
#endif
		IF_DEQUEUE(&ifp->if_snd, ior);
		if (ior == (io_req_t)0)
			break;

		if (!sh4811_txt(sp, ior)) {
       			IF_PREPEND(&ifp->if_snd, ior);
			break;
		}
	} /* end while */
#ifdef IFF_OACTIVE
	ifp->if_flags &= ~IFF_OACTIVE;
#endif
	driver_unlock(&sp->lock);
}

#if 0
void
sh4811watch(caddr_t b_ptr)
{
        register int    unit=*b_ptr;
        register sh4811_softc_t *sp=&sh4811_softc[unit];
	volatile unsigned char *addr;
	volatile unsigned int *iaddr, mask;
	unsigned long oldpri;
	io_req_t ior;
	static fsi_cmd *oldread;
	fsi_cmd *newread;

	oldpri = splimp();
	iaddr=(unsigned int *)(sp->hwaddr + FSI_INTR_MASK1);
	mask=FSI_INT_MASK;
	swap_long(iaddr, &mask);
	splx(oldpri);

	/* There is a problem with missing end of transmit interrupts
           on the Skyhawk board. All this is to deal with that.
        */
	newread = sp->txt_ring.read;
        if (sp->ds_if.if_flags & IFF_UP)  {
           if ((sp->ds_if.if_flags & IFF_UP) && sp->timer != -1 &&
			newread == oldread &&
			newread != sp->txt_ring.write) {
		oldpri = splimp();
                unmap_host_from_eisa((void *)sp->txt_list.read->io_ptr,
                                sp->txt_list.read->size);
		if(ior = (io_req_t)sp->txt_list.read->buf_ptr) {
			ior->io_residual = ior->io_count;
			ior->io_error = D_IO_ERROR;
			iodone(ior);
			sp->txt_list.read->buf_ptr=(unsigned char *)0;
                }
		INC_READ_PTR(sp->txt_ring);
                INC_READ_PTR(sp->txt_list);
	   	splx(oldpri);
	   }
        }
	timeout((timeout_fcn_t)sh4811watch, b_ptr, sh4811timo*hz);
	sp->timer=sh4811timo;
	oldread = newread;
	return;
}
#endif

int  
sh4811getstat(dev_t dev, dev_flavor_t flavor, dev_status_t status, 
		mach_msg_type_number_t *count) 
{
	register int    unit;
        register sh4811_softc_t *sp;

        unit = minor(dev);      /* XXX */
        sp = &sh4811_softc[unit];

        if (unit < 0 || unit >= NFDDI || !sp->seated)
            return (ENXIO);

        return (net_getstat(&(sp->ds_if), flavor, status, count));
}

int  
sh4811setstat(dev_t dev, dev_flavor_t flavor, dev_status_t status, 
		mach_msg_type_number_t count)
{ 
	register int    unit;
        register sh4811_softc_t *sp;
	unsigned long oldpri;
	driver_lock_state();

        unit = minor(dev);      /* XXX */
        sp = &sh4811_softc[unit];

        if (unit < 0 || unit >= NFDDI || !sp->seated)
            return (ENXIO);

        switch (flavor) {

          case NET_STATUS:
                break;

          case NET_ADDRESS:
                break;

	  case NET_ADDMULTI:
            {
                net_multicast_t *new;
                unsigned int size;

                if (count != FDDI_ADDR_SIZE * 2)
                    return (KERN_INVALID_VALUE);
                if ((((unsigned char *)status)[0] & 1) == 0 ||
                    (((unsigned char *)status)[6] & 1) == 0)
                    return (KERN_INVALID_VALUE);

                size = NET_MULTICAST_LENGTH(FDDI_ADDR_SIZE);
                new = (net_multicast_t *)kalloc(size);
                new->size = size;
                net_multicast_create(new, (unsigned char *)status,
                                     &((unsigned char *)
                                       status)[FDDI_ADDR_SIZE],
                                     FDDI_ADDR_SIZE);
                oldpri = splimp();
		(void)driver_lock(&sp->lock, DRIVER_OP_WAIT, FALSE);
                net_multicast_insert(&sp->multicast_queue, new);
                splx(oldpri);
		FDDIPRINTF(FDDIDEBUG_DEBUG, ("Add to cam %x %x %x\n",
					     *(unsigned short *)status,
					     *(unsigned short *)(status+1),
					     *(unsigned short *)(status+2)));
		sh4811_set_cam(sp, (unsigned char *)status, FDDI_SET_CAM_REQ);
		driver_unlock(&sp->lock);
                break;
            }

	   case NET_DELMULTI:
            {
                net_multicast_t *cur;

                if (count != FDDI_ADDR_SIZE * 2)
                    return (KERN_INVALID_VALUE);

                oldpri = splimp();
		(void)driver_lock(&sp->lock, DRIVER_OP_WAIT, FALSE);
                cur = net_multicast_remove(&sp->multicast_queue,
                                           (unsigned char *)status,
                                           &((unsigned char *)
                                             status)[FDDI_ADDR_SIZE],
                                           FDDI_ADDR_SIZE);
                if (cur == (net_multicast_t *)0) {
		    driver_unlock(&sp->lock);
                    splx(oldpri);
                    return (KERN_INVALID_VALUE);
                }
		sh4811_set_cam(sp, (unsigned char *)status, FDDI_CLEAR_CAM_REQ);
		driver_unlock(&sp->lock);
                splx(oldpri);

                kfree((vm_offset_t)cur, cur->size);
                break;
            }

          default:
                return D_INVALID_OPERATION;
        }

	return(D_SUCCESS);
}

int  
sh4811setinput(dev_t dev, ipc_port_t rcv_port, int prio, filter_t filter[], unsigned int count, 
	       device_t device)
{
	register int    unit;
        register sh4811_softc_t *sp;

        unit = minor(dev);      /* XXX */
        sp = &sh4811_softc[unit];

        if (unit < 0 || unit >= NFDDI || !sp->seated)
            return (ENXIO);

        return (net_set_filter(&(sp->ds_if),
                               rcv_port,
                               prio,
                               filter,
                               count, 
			       device));
}

#ifdef	FDDITEST
#include <hp_pa/rtclock_entries.h>
extern void fddi_test_thread(void);

void 
sh4811FN(void)
{
	sh4811_softc_t *sp=&sh4811_softc[0];
	driver_lock_state();
	
	if (!driver_lock(&sp->lock, DRIVER_OP_TIMEO, FALSE))
		return;
        if(sp->status != SH4811_RING_CONNECT) {
                timeout((timeout_fcn_t) fddi_test_thread, (void *)0, 20*hz);
        }
	driver_unlock(&sp->lock);
}

static int FNon=0;
static tvalspec_t fddi_start_time, fddi_stop_time;

void
fddi_test_thread(void)
{
	sh4811_softc_t *sp=&sh4811_softc[0];
	struct  ifnet   *ifp=&(sp->ds_if);
	register int i,count,numtxt;
	driver_lock_state();
	
	if (!driver_lock(&sp->lock, DRIVER_OP_TIMEO, FALSE))
		return;
	if(sp->status != SH4811_RING_CONNECT) {
		delay(100);
	}
	else {
		if(FNon==0)
			rtc_gettime(&fddi_start_time);
		FNon++;
		sp->tbusy=0;
		numtxt=10;
		for(i=0; i<numtxt; i++) {
			sh4811_test_send(sp);
			sp->tbusy++;
			count=10;
			while(count-- && sp->tbusy > 0)
				delay(10);
		}
		if(FNon < 100) 
			timeout((timeout_fcn_t)fddi_test_thread, 
					(void *)0, 1);
		else {
			rtc_gettime(&fddi_stop_time);
			printf("start - sec %d,nsec %d\n",
			       fddi_start_time.tv_sec, fddi_start_time.tv_nsec);
			printf("stop - sec %d,nsec %d\n",
				fddi_stop_time.tv_sec, fddi_stop_time.tv_nsec);
			printf("transmit - count %d, size %d, err %d\n",
				ifp->if_opackets, sp->stats.txtsz, 
				ifp->if_oerrors);
			printf("receive - count %d, size %d, err %d\n",
				ifp->if_ipackets, sp->stats.rcvsz, 
				ifp->if_rcvdrops);
		}
	}
	driver_unlock(&sp->lock);
}
#endif

#if 0
/****************************************************************************/

/* Initialization of the ELM. See Motorola 68840 IFDDI User's Manual
   Chapter 11 page 1 and Table 7-9 for the details */
void
sh4811elm_init(dev_t unit)
{
        sh4811_softc_t  *sp = &sh4811_softc[unit];
        unsigned char *addr=sp->hwaddr;
        unsigned short *saddr;

        /* enable EISA access to MAC/ELM registers */
        *(addr+MISCELLANEOUS_OUTPUT_LATCH) = 0x00;

	/* set up for SAS - single attach station */
	*(saddr = (unsigned short *)(addr + ELM_CTRL_A)) = SAS_CTRL_A_VALUE;
	*(saddr = (unsigned short *)(addr + ELM_CTRL_B)) = SAS_CTRL_B_VALUE;
	
	/* setup PCM(Physical Connection Management) timers */
	*(saddr = (unsigned short *)(addr + PCM_ACQ_MAX)) = PCM_ACQ_MAX_VALUE;
	*(saddr = (unsigned short *)(addr + PCM_LSCHANGE_MAX)) = 
				PCM_LSCHANGE_MAX_VALUE;
	*(saddr = (unsigned short *)(addr + PCM_BREAK_MIN)) = 
				PCM_BREAK_MIN_VALUE;
	*(saddr = (unsigned short *)(addr + PCM_SIG_TIMEOUT)) = 
				PCM_SIG_TIMEOUT_VALUE;
	*(saddr = (unsigned short *)(addr + PCM_LC_SHORT)) = PCM_LC_SHORT_VALUE;
	*(saddr = (unsigned short *)(addr + PCM_SCRUB)) = PCM_SCRUB_VALUE;
	*(saddr = (unsigned short *)(addr + PCM_NOISE)) = PCM_NOISE_VALUE;

	/* clear the following by reading them */
	*(saddr = (unsigned short *)(addr + ELM_INT_EVENT));
	*(saddr = (unsigned short *)(addr + ELM_VIOLATE_SYM));
	*(saddr = (unsigned short *)(addr + ELM_MIN_IDLE));
	*(saddr = (unsigned short *)(addr + ELM_LINK_ERROR));

	/* initialze the ELM interrupt mask register */
	*(saddr = (unsigned short *)(addr + ELM_INTR_MASK)) = 
				ELM_INTR_MASK_VALUE;
	/* move PCM to break state */
	*(saddr = (unsigned short *)(addr + ELM_CTRL_B)) |= 0x0100;
}

/* Initialization of the MAC. See Motorola 68840 IFDDI User's Manual
    Chapter 11 page 2 for the details */
void
sh4811mac_init(dev_t unit)
{
        sh4811_softc_t  *sp = &sh4811_softc[unit];
        unsigned char *addr;
        unsigned short *saddr;

	addr = sp->hwaddr;
	/* read some MAC registers to initialize them */
	*(saddr = (unsigned short *)(addr + FRAME_COUNT));
	*(saddr = (unsigned short *)(addr + LOST_COUNT));
	*(saddr = (unsigned short *)(addr + INT_EVENT_A));
	*(saddr = (unsigned short *)(addr + INT_EVENT_B));
	*(saddr = (unsigned short *)(addr + INT_EVENT_C));

	/* get mac addr from factory settings */
	sh4811get_macaddr(unit, sp->noncan_mac_addr); 
	/* set mac addr in the MAC registers of the Motorola chipset */
	sh4811set_macaddr(unit, sp->noncan_mac_addr); 

	/* set TTRT to typical value (from THE book Table 11-1) */
	*(saddr = (unsigned short *)(addr + REQ_TTRT)) = REQ_TTRT_VALUE;
	/* set Intial and max TRT values (Table 11-1) */
	*(saddr = (unsigned short *)(addr + INIT_MAX_TTRT)) 
			= INIT_MAX_TTRT_VALUE;

	/* turn it on !!! */
	saddr = (unsigned short *)(addr + MAC_CTRL_A);
	*saddr |=  MAC_ON;

        /* disable EISA access to MAC/ELM registers */
        *(addr+MISCELLANEOUS_OUTPUT_LATCH) = 0x20;
}

void
sh4811get_macaddr(dev_t unit, unsigned char *long_addr)
{
        sh4811_softc_t  *sp = &sh4811_softc[unit];
        unsigned char *addr=sp->hwaddr;
        register int i;

        /* enable access to factory node addr */
        *(addr+FACTORY_TEST_OUTPUT_LATCH) = 0x03;

	addr += SH4811_NODE_ADDR;
        for(i=0;i<FDDI_ADDR_SIZE;i++) 
                *long_addr++ = *addr++;

        /* disable access to factory node addr */
        *(sp->hwaddr+FACTORY_TEST_OUTPUT_LATCH) = 0x00;
}

void 
sh4811set_macaddr(dev_t unit, unsigned char *long_addr)
{
	sh4811_softc_t  *sp = &sh4811_softc[unit];
	register int i;
	unsigned short *saddr;
	unsigned char *addr;

	addr = long_addr;
	saddr = (unsigned short *)(sp->hwaddr + LONG_ADDRESS_A);
	for(i=0;i<3;i++)
		*saddr++ = (*addr++ << BYTE_SIZE) | *addr++ ;
}
#endif

void
sh4811dumpconfig(int unit)
{
	sh4811_softc_t  *sp = &sh4811_softc[unit];
	register int i;
	unsigned char *addr=sp->hwaddr;
	unsigned short *saddr;
	unsigned int *iaddr;
	unsigned int sval;
	unsigned int ival;
	

        /* enable access to factory node addr */
        *(addr+FACTORY_TEST_OUTPUT_LATCH) = 0x03;
        /* enable EISA access to MAC/ELM registers */
        *(addr+MISCELLANEOUS_OUTPUT_LATCH) = 0x00;

	FDDIPRINTF(FDDIDEBUG_DUMP, ("control register %x - %x\n",
				    (addr + 0xC84), *(addr + 0xC84)));

	addr += 0xC88;
	FDDIPRINTF(FDDIDEBUG_DUMP, ("dump %x to C8A\n", addr));
	for(i=0;i<3;i++)
		FDDIPRINTF(FDDIDEBUG_DUMP, ("%x   ", *addr++));
	FDDIPRINTF(FDDIDEBUG_DUMP, ("\n"));

	addr = sp->hwaddr + 0xC8E;
	FDDIPRINTF(FDDIDEBUG_DUMP, ("dump %x to C90 \n", addr));
	for(i=0;i<3;i++)
		FDDIPRINTF(FDDIDEBUG_DUMP, ("%x   ", *addr++));
	FDDIPRINTF(FDDIDEBUG_DUMP, ("\n"));

	addr = sp->hwaddr + 0xCA8;
	FDDIPRINTF(FDDIDEBUG_DUMP, ("dump %x to CAE \n", addr));
	for(i=0;i<7;i++)
		FDDIPRINTF(FDDIDEBUG_DUMP, ("%x   ", *addr++));
	FDDIPRINTF(FDDIDEBUG_DUMP, ("\n"));

	addr = sp->hwaddr + 0xCC0;
	FDDIPRINTF(FDDIDEBUG_DUMP, ("dump %x to CC9 \n", addr));
	for(i=0;i<10;i++)
		FDDIPRINTF(FDDIDEBUG_DUMP, ("%x   ", *addr++));
	FDDIPRINTF(FDDIDEBUG_DUMP, ("\n"));

	iaddr = (unsigned int *)(sp->hwaddr + FSI_B_PORT);
	*iaddr;
	delay(50);
	FDDIPRINTF(FDDIDEBUG_DUMP, ("FSI B-port registers %x\n", iaddr));
	for(i=0;i<16;i++, iaddr++)
		FDDIPRINTF(FDDIDEBUG_DUMP, ("%x   ", int_swap(*iaddr, ival)));
	FDDIPRINTF(FDDIDEBUG_DUMP, ("\n"));

	saddr = (unsigned short *)(sp->hwaddr + MAC_CTRL_A);
	*saddr;
	delay(50);
	FDDIPRINTF(FDDIDEBUG_DUMP, ("MAC registers %x\n", saddr));
	for(i=0;i<5;i++,saddr++) 
		FDDIPRINTF(FDDIDEBUG_DUMP, ("%x ", short_swap(*saddr, sval)));
	FDDIPRINTF(FDDIDEBUG_DUMP, ("\n"));
	saddr = (unsigned short *)(sp->hwaddr + 0x420);
	FDDIPRINTF(FDDIDEBUG_DUMP, ("MAC registers %x\n", saddr));
	for(i=0;i<6;i++,saddr++) 
		FDDIPRINTF(FDDIDEBUG_DUMP, ("%x ", short_swap(*saddr, sval)));
	FDDIPRINTF(FDDIDEBUG_DUMP, ("\n"));
	saddr = (unsigned short *)(sp->hwaddr + 0x438);
	FDDIPRINTF(FDDIDEBUG_DUMP, ("MAC registers %x\n", saddr));
	for(i=0;i<25;i++,saddr++) 
		FDDIPRINTF(FDDIDEBUG_DUMP, ("%x ", short_swap(*saddr, sval)));
	FDDIPRINTF(FDDIDEBUG_DUMP, ("\n"));

	saddr = (unsigned short *)(sp->hwaddr + ELM_CTRL_A);
	*saddr;
	delay(50);
	FDDIPRINTF(FDDIDEBUG_DUMP, ("ELM registers %x\n", saddr));
	for(i=0;i<27;i++,saddr++)
		FDDIPRINTF(FDDIDEBUG_DUMP, ("%x ", short_swap(*saddr, sval)));
	FDDIPRINTF(FDDIDEBUG_DUMP, ("\n"));

        /* disable access to factory node addr */
        *(sp->hwaddr+FACTORY_TEST_OUTPUT_LATCH) = 0x00;
        /* disable EISA access to MAC/ELM registers */
        *(addr+MISCELLANEOUS_OUTPUT_LATCH) = 0x20;
}

#endif
