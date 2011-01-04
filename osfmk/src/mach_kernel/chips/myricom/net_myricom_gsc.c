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

#include <hp_pa/HP700/device.h>
#include <hp_pa/iomod.h>
#include <hp_pa/asp.h>
#include <hp_pa/spl.h>
#include <chips/myricom/net_myricom.h>
#include <chips/myricom/net_myricom_gsc.h>
#include <chips/myricom/net_myricom_mcp3.h>
#include <kern/misc_protos.h>
#include <kern/kalloc.h>
#include <device/if_myrinet.h>
#include <device/net_device.h>
#include <mach/vm_param.h>
#include <mach/boolean.h>
#include <busses/iobus.h>
#include <busses/gsc/gsc.h>

/*
 * The on-board Memory is visible via 3 windows of 4K each. The two last
 * ones are reserved for MACH servers usage (first come, first served),
 * the first one for all other usage (servers when the two previous are
 * used and in-kernel x-kernel and kgdb modules).
 */

/*
 * Forward declarations
 */
void
net_myricom_gsc_control(net_device_t,
			enum net_myricom_flavor
    );

void
net_myricom_gsc_wakeup(net_device_t,
		       vm_offset_t,
		       unsigned int
    );

IOBUS_SHMEM_T
net_myricom_gsc_regmap(net_device_t,
		       unsigned int,
		       unsigned int *
    );

IOBUS_SHMEM_T
net_myricom_gsc_memmap(net_device_t,
		       vm_offset_t,
		       vm_size_t *
    );

boolean_t
net_myricom_gsc_reset(net_device_t
    );

void
net_myricom_gsc_attach(struct gsc_board *
    );

boolean_t
net_myricom_gsc_init(net_device_t
    );

net_bicintr_t
net_myricom_gsc_intr(net_device_t,
		     boolean_t *,
		     boolean_t *,
		     boolean_t *
    );

io_return_t
net_myricom_gsc_dinfo(net_device_t,
		      dev_flavor_t,
		      char *
    );

/*
 * net_myricom_gsc_control
 *
 * Purpose : Control LanAI chip
 * Returns : Nothing
 *
 * IN_arg  : netp   ==> Network generic interface structure
 *	     flavor ==> Flavor required
 */
void
net_myricom_gsc_control(
    net_device_t netp,
    enum net_myricom_flavor flavor)
{
    net_myricom_gsc_t myio = (net_myricom_gsc_t)netp->net_bic;

    switch (flavor) {
    case NET_MYRICOM_FLAVOR_INTR:
	if ((myio->myio_flags & NET_MYRICOM_GSC_FLAGS_INTERRUPT) == 0) {
	    /*
	     * Enable LANai chip interrupts
	     */
	    myio->myio_flags |= NET_MYRICOM_GSC_FLAGS_INTERRUPT;
	    SLIPSTREAM_SET_INTR(myio->myio_iotype,
				myio->myio_iobase, myio->myio_eir);

	    /*
	     * Small bug for LANai id < 4.1: The EIMR is reset when RST is set,
	     * so EIMR must be set after RST is set.
	     */
	    SLIPSTREAM_TSI(myio->myio_iotype, myio->myio_iobase,
			   NET_MYRICOM_GSC_REG_LANAI + NET_MYRICOM_REG_EIMR) =
		NET_MYRICOM_ISR_HOST;
	}
	break;

    case NET_MYRICOM_FLAVOR_START:
	if ((myio->myio_flags & NET_MYRICOM_GSC_FLAGS_RUNNING) == 0) {
	    /*
	     * Start LANai chip
	     */
	    myio->myio_flags |= NET_MYRICOM_GSC_FLAGS_RUNNING;
	    SLIPSTREAM_TSI(myio->myio_iotype, myio->myio_iobase,
			   NET_MYRICOM_GSC_REG_LANAI + NET_MYRICOM_REG_CLOCK) =
		myio->myio_clockval;
	    SLIPSTREAM_TSI(myio->myio_iotype,
			   myio->myio_iobase, NET_MYRICOM_GSC_REG_RESET) = 1;
	}
	break;
    }
}

/*
 * net_myricom_gsc_wakeup
 *
 * Purpose : Write a value to the on-board memory and wake up LanAI chip
 * Returns : Nothing
 *
 * IN_arg  : netp ==> Network generic interface structure
 *	     addr ==> Written address
 *	     val  ==> Written value
 */
void
net_myricom_gsc_wakeup(
    net_device_t netp,
    vm_offset_t addr,
    unsigned int val)
{
    net_myricom_gsc_t myio = (net_myricom_gsc_t)netp->net_bic;
    unsigned int i;
    boolean_t found;
    IOBUS_SHMEM_T shaddr;
    vm_offset_t page;

    for (i = 0; i < SLIPSTREAM_NPAGES; i++)
	if (trunc_page(addr) == myio->myio_page[i])
	    break;
    found = (i != SLIPSTREAM_NPAGES);
    if (!found) {
	/*
	 * Set up new page mapping
	 */
	i = 0;
	page = myio->myio_page[0];
	SLIPSTREAM_TSI(myio->myio_iotype, myio->myio_iobase,
		       NET_MYRICOM_GSC_REG_PNUM) = trunc_page(addr);
    }
    shaddr = myio->myio_iobase +
	PAGE_SIZE * (i + 1) + (addr - trunc_page(addr));

    /*
     * Set up mask value, write memory and reset mask value
     */
    SLIPSTREAM_TSI(myio->myio_iotype, myio->myio_iobase,
		   NET_MYRICOM_GSC_REG_MASK + i) =
	(NET_MYRICOM_GSC_REG_MASK_ENA | NET_MYRICOM_GSC_REG_MASK_MSK);
    *((volatile unsigned int *)shaddr) = val;
    SLIPSTREAM_TSI(myio->myio_iotype, myio->myio_iobase,
		   NET_MYRICOM_GSC_REG_MASK + i) = 0;

    if (!found) {
	/*
	 * Restore original page mapping
	 */
	SLIPSTREAM_TSI(myio->myio_iotype, myio->myio_iobase,
		       NET_MYRICOM_GSC_REG_PNUM) = page;
    }
}

/*
 * net_myricom_gsc_regmap
 *
 * Purpose : map lanAI registers
 * Returns : Address where the address range is mapped
 *
 * IN_arg  : netp    ==> Network generic interface structure
 *	     version ==> LanAI version
 *	     reg     ==> MYRICOM register to map
 * IO_arg  : num     ==> # LanAI registers to map
 */
IOBUS_SHMEM_T
net_myricom_gsc_regmap(
    net_device_t netp,
    unsigned int reg,
    unsigned int *num)
{
    net_myricom_gsc_t myio = (net_myricom_gsc_t)netp->net_bic;

    assert (reg < NET_MYRICOM_REG_CLOCK);
    *num = NET_MYRICOM_REG_CLOCK - reg;
    return ((IOBUS_SHMEM_T)&SLIPSTREAM_TSI(myio->myio_iotype,
					   myio->myio_iobase,
					   NET_MYRICOM_GSC_REG_LANAI + reg));
}
 
/*
 * net_myricom_gsc_memmap
 *
 * Purpose : map MYRICOM on-board memory
 * Returns : Address where the address range is mapped
 *
 * IN_arg  : netp ==> Network generic interface structure
 *	     addr ==> MYRICOM on-board memory address
 * IO_arg  : len  ==> Range of on-board memory
 */
IOBUS_SHMEM_T
net_myricom_gsc_memmap(
    net_device_t netp,
    vm_offset_t addr,
    vm_size_t *len)
{
    net_myricom_gsc_t myio = (net_myricom_gsc_t)netp->net_bic;
    vm_offset_t start;
    vm_offset_t cur;
    vm_offset_t end;
    unsigned int i;
    unsigned int j;
    unsigned int n;
    unsigned int ref;
    IOBUS_SHMEM_T map;

    /*
     * The mapping pages are kept in address growing order, so that mapping
     * N contiguous pages results as N contiguous mapped pages. The reference
     * count is incremented each time a page is mapped and if a page is
     * unmapped, the reference count of all other pages is decremented by
     * its reference count.
     */
    start = trunc_page(addr);
    end = round_page(addr + *len);
    if (end - start > SLIPSTREAM_NPAGES * PAGE_SIZE)
	end = start + SLIPSTREAM_NPAGES * PAGE_SIZE;
    *len = (end - start) - (addr - start);

    /*
     * Area to map requires more than 1 page
     */
    for (cur = start; cur < end; cur += PAGE_SIZE) {
	j = SLIPSTREAM_NPAGES;
	for (i = 0; i < SLIPSTREAM_NPAGES; i++) {
	    if (myio->myio_page[i] == cur)
		break;
	    if ((myio->myio_page[i] < start || myio->myio_page[i] >= end) &&
		(j == SLIPSTREAM_NPAGES || ref < myio->myio_ref[i]))
		ref = myio->myio_ref[j = i];
	}
	if (i < SLIPSTREAM_NPAGES)
	    continue;

	/*
	 * Map new page
	 */
	assert(j < SLIPSTREAM_NPAGES);
	if (myio->myio_page[j] < cur) {
	    for (i = j; i < SLIPSTREAM_NPAGES - 1; i++) {
		if (myio->myio_page[i + 1] > cur)
		    break;
		SLIPSTREAM_TSI(myio->myio_iotype, myio->myio_iobase,
			       NET_MYRICOM_GSC_REG_PNUM + i) =
		    myio->myio_page[i] = myio->myio_page[i + 1];
		myio->myio_ref[i] = myio->myio_ref[i + 1];
	    }
	} else {
	    for (i = j; i > 0; i--) {
		if (myio->myio_page[i - 1] < cur)
		    break;
		SLIPSTREAM_TSI(myio->myio_iotype, myio->myio_iobase,
			       NET_MYRICOM_GSC_REG_PNUM + i - 1) =
		    myio->myio_page[i - 1] = myio->myio_page[i];
		myio->myio_ref[i - 1] = myio->myio_ref[i];
	    }
	}
	SLIPSTREAM_TSI(myio->myio_iotype, myio->myio_iobase,
		       NET_MYRICOM_GSC_REG_PNUM + i) =
	    myio->myio_page[i] = cur;

	/*
	 * Update reference counts
	 */
	for (i = 0; i < SLIPSTREAM_NPAGES; i++) {
	    if (ref >= myio->myio_ref[i])
		myio->myio_ref[i] -= ref;
	    else if (myio->myio_ref[i] != 0)
		myio->myio_ref[i] = 0;
	}
    }

    for (i = 0; i < SLIPSTREAM_NPAGES && myio->myio_page[i] < end; i++) {
	if (myio->myio_page[i] == start)
	    map = myio->myio_iobase + (i + 1) * PAGE_SIZE + (addr - start);
	else if (myio->myio_page[i] > start)
	    assert(myio->myio_page[i - 1] + PAGE_SIZE == myio->myio_page[i]);
	myio->myio_ref[i]++;
    }
    return (map);
}

/*
 * net_myricom_gsc_probe
 *
 * Purpose : Probe a GSC MYRICOM board
 * Returns : TRUE/FALSE whether the board is recognized or not
 *
 * IN_args : iotype ==> GSC bus type
 *	     iobase ==> GSC bus base address
 */
gsc_attach_t
net_myricom_gsc_probe(
    IOBUS_TYPE_T iotype,
    IOBUS_ADDR_T iobase)
{
    struct net_myricom_eeprom eeprom;
    unsigned int i;
    unsigned char *p;
    volatile unsigned int *rw;

    /*
     * Read the MYRICOM EEPROM structure on the board
     */
    p = (unsigned char *)&eeprom;
    rw = &((struct iomod *)IOBUS_ADDR(iotype, iobase))->io_dc_rw;
    for (i = 0; i < sizeof (eeprom); i++) {
	*rw = i + NET_MYRICOM_GSC_IODC_OFFSET;
	*p++ = *rw;
    }

    /*
     * Validate LanAI version.revision
     */
    switch (eeprom.myee_lanai) {
    case NET_MYRICOM_ID_LANAI32:
	break;

    case NET_MYRICOM_ID_LANAI40:
	break;

    case NET_MYRICOM_ID_LANAI41:
	break;

    default:
	printf("%s: Unsupported MYRICOM LanAI 0x%x\n",
	       "net_myricom_gsc_probe", eeprom.myee_lanai);
	return ((gsc_attach_t)0);
    }
    return (net_myricom_gsc_attach);
}


/*
 * net_myricom_gsc_attach
 *
 * Purpose : Bus attachment and first initialization
 * Returns : Nothing
 *
 * In_args : gp ==> GSC board
 */
void
net_myricom_gsc_attach(
    struct gsc_board *gp)
{
    struct net_myricom_eeprom eeprom;
    unsigned int i;
    unsigned int unit;
    unsigned char *p;
    volatile unsigned int *rw;
    struct net_device *netp;
    net_myricom_gsc_t myio;

    /*
     * Read the MYRICOM EEPROM structure on the board
     */
    p = (unsigned char *)&eeprom;
    rw = &((struct iomod *)IOBUS_ADDR(gp->gsc_iotype,
				      gp->gsc_iobase))->io_dc_rw;
    for (i = 0; i < sizeof (eeprom); i++) {
	*rw = i + NET_MYRICOM_GSC_IODC_OFFSET;
	*p++ = *rw;
    }

    /*
     * Update eeprom values to correct burnt ones
     */
    switch (eeprom.myee_lanai) {
    case NET_MYRICOM_ID_LANAI32:
	eeprom.myee_clockval = NET_MYRICOM_CLOCKVAL32;
	eeprom.myee_ramsize = 128 * 1024;
	break;

    case NET_MYRICOM_ID_LANAI41:
	eeprom.myee_clockval = NET_MYRICOM_CLOCKVAL41;
	break;
    }

    /*
     * Attach MCP3 interface
     */
    netp = net_myricom_mcp3_attach(gp->gsc_iotype, gp->gsc_iobase, &eeprom,
				   (NET_MYRICOM_DMA_STS_08 |
				    NET_MYRICOM_DMA_STS_04 |
				    NET_MYRICOM_DMA_STS_02),
				   net_myricom_gsc_control,
				   net_myricom_gsc_wakeup,
				   net_myricom_gsc_regmap,
				   net_myricom_gsc_memmap);

    /*
     * Allocate BIC structure
     */
    netp->net_bic = myio =
	(net_myricom_gsc_t)kalloc(sizeof (struct net_myricom_gsc));
    myio->myio_iotype = gp->gsc_iotype;
    myio->myio_iobase = gp->gsc_iobase;
    myio->myio_clockval = eeprom.myee_clockval;
    myio->myio_ramsize = eeprom.myee_ramsize;
    myio->myio_lanai = eeprom.myee_lanai;
    myio->myio_eir = gp->gsc_eir;
    myio->myio_flags = 0;

    /*
     * Initialize MYRICOM address
     */
    for (i = 0; i < IF_MYRINET_UID_SIZE; i++)
	((unsigned char *)netp->net_address)[i] = eeprom.myee_addr[i];

    /*
     * Complete BIC initialization
     */
    netp->net_bic_ops.bic_init    = net_myricom_gsc_init;
    netp->net_bic_ops.bic_reset   = net_myricom_gsc_reset;
    netp->net_bic_ops.bic_intr    = net_myricom_gsc_intr;
    netp->net_bic_ops.bic_dinfo   = net_myricom_gsc_dinfo;
    netp->net_bic_ops.bic_copyout = (net_bicout_t (*)(net_device_t,
						      net_done_t,
						      vm_offset_t,
						      vm_size_t,
						      unsigned int))0;
    netp->net_bic_ops.bic_copyin  = (net_bicin_t (*)(net_device_t,
						     int,
						     vm_offset_t,
						     vm_offset_t,
						     vm_size_t,
						     boolean_t))0;

    /*
     * Advertise MYRICOM board
     */
    printf("%s%d: on GSC%d at 0x%x, EIR = %d",
	   NET_DEVICE_NAME, netp->net_generic,
	   IOBUS_TYPE_NUM(gp->gsc_iotype), gp->gsc_iobase, gp->gsc_eir);
    net_driver_alias(netp);
    printf("\n%s%d: LanAI %d.%d, UID [%02x:%02x:%02x:%02x:%02x:%02x]",
	   NET_DEVICE_NAME, netp->net_generic,
	   NET_MYRICOM_ID_VERSION(eeprom.myee_lanai),
	   NET_MYRICOM_ID_REVISION(eeprom.myee_lanai),
	   eeprom.myee_addr[0], eeprom.myee_addr[1], eeprom.myee_addr[2],
	   eeprom.myee_addr[3], eeprom.myee_addr[4], eeprom.myee_addr[5]);
    printf(", clockval = 0x%x, RAM = %dKB\n",
	   eeprom.myee_clockval, eeprom.myee_ramsize / 1024);

    /*
     * Complete GSC board
     */
    gp->gsc_intr = net_driver_intr;
    gp->gsc_unit = netp->net_generic;

    /*
     * Finally, reset adapter
     */
    if (!net_myricom_gsc_reset(netp))
        panic("%s: cannot reset BIC on adapter %s%d\n",
              "net_myrinet_gsc_attach", NET_DEVICE_NAME, netp->net_generic);
}

/*
 * net_myricom_gsc_reset
 *
 * Purpose : BIC reset
 * Returns : TRUE/FALSE if the reset sucessfully completed
 *
 * IN_arg  : netp ==> Network generic interface structure
 */
boolean_t
net_myricom_gsc_reset(
    net_device_t netp)
{
    net_myricom_gsc_t myio = (net_myricom_gsc_t)netp->net_bic;

    if (myio->myio_flags & NET_MYRICOM_GSC_FLAGS_RUNNING) {
	/*
	 * Stop LANai chip
	 */
	myio->myio_flags &= ~NET_MYRICOM_GSC_FLAGS_RUNNING;
	SLIPSTREAM_RESET(myio->myio_iotype, myio->myio_iobase,
			 "net_myricom_gsc_reset");
    }

    if (myio->myio_flags & NET_MYRICOM_GSC_FLAGS_INTERRUPT) {
	/*
	 * Disable LANai interrupts
	 */
	myio->myio_flags &= ~NET_MYRICOM_GSC_FLAGS_INTERRUPT;
	SLIPSTREAM_MSK_INTR(myio->myio_iotype, myio->myio_iobase);
    }

    if (SLIPSTREAM_GET_INTR(myio->myio_iotype, myio->myio_iobase)) {
	/*
	 * Cleanup pending interrupts
	 */
	SLIPSTREAM_CLR_INTR(myio->myio_iotype, myio->myio_iobase);
    }
    return (TRUE);
}

/*
 * net_myricom_gsc_init
 *
 * Purpose : BIC initialization
 * Returns : TRUE/FALSE if the initialization sucessfully completed
 *
 * IN_arg  : netp ==> Network generic interface structure
 */
boolean_t
net_myricom_gsc_init(
    net_device_t netp)
{
    net_myricom_gsc_t myio = (net_myricom_gsc_t)netp->net_bic;
    unsigned int i;

    assert((myio->myio_flags & NET_MYRICOM_GSC_FLAGS_RUNNING) == 0);

    /*
     * Initialize Slipstream
     */
    SLIPSTREAM_INIT(myio->myio_iotype, myio->myio_iobase);
    SLIPSTREAM_TRS(myio->myio_iotype, myio->myio_iobase)->sl_cntl =
	SLIPSTREAM_EXPAND_IO | SLIPSTREAM_WIPER_CHIP;
    for (i = 0; i < SLIPSTREAM_NPAGES; i++) {
	SLIPSTREAM_TSI(myio->myio_iotype, myio->myio_iobase,
		       NET_MYRICOM_GSC_REG_PNUM + i) =
	    myio->myio_page[i] = i * PAGE_SIZE;
	SLIPSTREAM_TSI(myio->myio_iotype, myio->myio_iobase,
		       NET_MYRICOM_GSC_REG_MASK + i) = 0;
	myio->myio_ref[i] = 0;
    }

    /*
     * Cleanup on-board memory and load LanAI Control Program
     */
    for (i = 0; i < myio->myio_ramsize; i += PAGE_SIZE) {
        if (myio->myio_page[0] != i)
            SLIPSTREAM_TSI(myio->myio_iotype, myio->myio_iobase,
			   NET_MYRICOM_GSC_REG_PNUM) = myio->myio_page[0] = i;
        bzero((char *)myio->myio_iobase + PAGE_SIZE, PAGE_SIZE);
    }

    return (TRUE);
}

/*
 * net_myricom_gsc_intr
 *
 * Purpose : Interrupt management
 * Returns : NET_BICINTR_ERROR if error occured
 *           NET_BICINTR_NOTHING if no event to report
 *           NET_BICINTR_EVENT if copy in/out completed
 *
 * IN_arg  : netp      ==> Network generic interface structure
 * OUT_arg : copyin    ==> TRUE/FALSE if the completed event was a copyin
 *           busy      ==> TRUE/FALSE if the BIC is busy
 *           nic_event ==> TRUE/FALSE if the NIC has a pending IT
 */
/*ARGSUSED*/
net_bicintr_t
net_myricom_gsc_intr(
    net_device_t netp,
    boolean_t *copyin,
    boolean_t *busy,
    boolean_t *nic_event)
{
    net_myricom_gsc_t myio = (net_myricom_gsc_t)netp->net_bic;

    if (*nic_event = SLIPSTREAM_GET_INTR(myio->myio_iotype,
					 myio->myio_iobase))
	SLIPSTREAM_CLR_INTR(myio->myio_iotype, myio->myio_iobase);
    *busy = (myio->myio_flags & NET_MYRICOM_GSC_FLAGS_RUNNING) == 0;
    *copyin = FALSE;
    return (NET_BICINTR_NOTHING);
}

/*
 * net_myricom_gsc_dinfo
 *
 * Purpose : Get info for kernel/Set info from kernel.
 * Returns : D_SUCCESS = success
 *           D_INVALID_OPERATION = invalid operation
 *
 * IN_arg  : netp   ==> Network generic interface structure
 *           flavor ==> type of information
 *           info   ==> information associated to the flavor
 */
/*ARGSUSED*/
io_return_t
net_myricom_gsc_dinfo(
    net_device_t netp,
    dev_flavor_t flavor,
    char *info)
{
    switch (flavor) {
    default:
        return (D_INVALID_OPERATION);
    }
    return (D_SUCCESS);
}
