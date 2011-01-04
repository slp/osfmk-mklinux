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

/*
 * MYRICOM Control Program v3 NIC driver
 */

#include <busses/iobus.h>
#include <device/net_status.h>
#include <device/data_device.h>
#include <device/ds_routines.h>
#include <mach/boolean.h>
#include <mach/vm_param.h>
#include <kern/assert.h>
#include <kern/kalloc.h>
#include <kern/misc_protos.h>
#include <chips/myricom/net_myricom.h>
#include <chips/myricom/net_myricom_mcp3.h>
#include <machine/endian.h>

/*
 * To do: handle more than NET_MYRICOM_MCP3_GATHER_MAX fragments in transmission
 */

/*
 * Forward declarations
 */
void
net_myricom_mcp3_write(net_device_t,
		       io_req_t
    );

void
net_myricom_mcp3_close(net_device_t
    );

void
net_myricom_mcp3_netd_done(net_device_t,
			   net_done_t
    );

boolean_t
net_myricom_mcp3_prepin(net_device_t,
			int,
			vm_offset_t,
			vm_offset_t,
			vm_offset_t
    );

void
net_myricom_mcp3_prepout(net_device_t,
			 net_done_t,
			 vm_offset_t,
			 vm_size_t,
			 unsigned int);

io_return_t
net_myricom_mcp3_dinfo(net_device_t,
		       dev_flavor_t,
		       char *
    );

boolean_t
net_myricom_mcp3_reset(net_device_t
    );

boolean_t
net_myricom_mcp3_init(net_device_t
    );

net_nicout_t
net_myricom_mcp3_copyout(net_device_t,
			 net_done_t
    );

net_nicin_t
net_myricom_mcp3_copyin(net_device_t,
			int *,
			vm_offset_t *,
			vm_size_t *,
			unsigned int *
    );

int
net_myricom_mcp3_start(net_device_t,
		       net_done_t
    );

int
net_myricom_mcp3_timeout(net_device_t
    );

net_nicintr_t
net_myricom_mcp3_intr(net_device_t
    );

void
net_myricom_mcp3_multicast(net_device_t,
			   net_multicast_t *
    );

/*
 * net_myricom_mcp3_addr_broadcast
 *
 * Purpose : Tests whether a MYRINET address is a broadcast one or not
 * Returns : TRUE/FALSE whether the MYRINET address is a broadcast address
 *
 * IN_arg  : addr ==> MYRINET address to test
 */
boolean_t
net_myricom_mcp3_addr_broadcast(
    net_myricom_mcp3_addr_t addr)
{
    unsigned int i;

    for (i = 0; i < IF_MYRINET_MID_SIZE; i++)
	if (addr[i] != 0xFF)
	    return (FALSE);
    return (TRUE);
}

/*
 * net_myricom_mcp3_addr_compare
 *
 * Purpose : Compare two MYRINET addresses
 * returns : -1 if first < second
 *	     +1 if first > second
 *	      0 otherwise
 *
 * IN_arg  : addr1 ==> First MYRINET address
 *	     addr2 ==> Second MYRINET address
 */
int
net_myricom_mcp3_addr_compare(
    net_myricom_mcp3_addr_t addr1,
    net_myricom_mcp3_addr_t addr2)
{
    unsigned int i;

    for (i = 0; i < IF_MYRINET_MID_SIZE; i++) {
	if (addr1[i] < addr2[i])
	    return (-1);
	if (addr1[i] > addr2[i])
	    return (1);
    }
    return (0);
}

/*
 * net_myricom_mcp3_list_search
 *
 * Purpose : Search a MYRINET address in a list
 * Returns : TRUE/FALSE whether the address has been found or not
 *
 * IN_arg  : list     ==> generic list
 *	     addr     ==> MYRINET address to find
 *	     position ==> TRUE  : Index of the MYRINET address
 *			  FALSE : Index where to insert the MYRINET address
 */
boolean_t
net_myricom_mcp3_list_search(
    net_myricom_mcp3_list_t *list,
    net_myricom_mcp3_addr_t addr,
    unsigned *position)
{
    unsigned start, stop, half;
    net_myricom_mcp3_pair_t *pair = (net_myricom_mcp3_pair_t *)(list + 1);

    start = 0;
    stop = net_myricom_mcp3_32_get(&list->ls_length);
    while (start <= stop) {
	half = (stop - start) / 2;
	switch (net_myricom_mcp3_addr_compare(addr, pair[half].pa_address)) {
	case 0:
	    *position = half;
	    return (TRUE);

	case 1:
	    start = half + 1;
	    continue;

	case -1:
	   stop = half - 1;
	    continue;
	}
    }
    *position = half;
    return (FALSE);
}

/*
 * net_myricom_mcp3_list_get
 *
 * Purpose : Get a MYRINET address in a list
 * Returns : TRUE/FALSE whether the address has been found or not
 *
 * IN_arg  : list     ==> generic list
 *	     addr     ==> MYRINET address to get
 *	     value    ==> Associated value
 *	     position ==> TRUE  : Index of the MYRINET address
 *			  FALSE : Index where to insert the MYRINET address
 */
boolean_t
net_myricom_mcp3_list_get(
    net_myricom_mcp3_list_t *list,
    net_myricom_mcp3_addr_t addr,
    int *value,
    unsigned *position)
{
    unsigned i = net_myricom_mcp3_32_get(&list->ls_cache);
    net_myricom_mcp3_pair_t *pair = (net_myricom_mcp3_pair_t *)(list + 1);

    /*
     * First try in the cache
     */
    if (i < net_myricom_mcp3_32_get(&list->ls_length) &&
	net_myricom_mcp3_addr_compare(pair[i].pa_address, addr) == 0) {
	*value = net_myricom_mcp3_32_get(&pair[i].pa_value);
	*position = i;
	return (TRUE);
    }

    /*
     * Otherwise, search it
     */
    if (net_myricom_mcp3_list_search(list, addr, &i)) {
	*value = net_myricom_mcp3_32_get(&pair[i].pa_value);
	*position = i;
	net_myricom_mcp3_32_set(&list->ls_cache, i);
	return (TRUE);
    }

    return (FALSE);
}

/*
 * net_myricom_mcp3_list_put
 *
 * Purpose : Put a MYRINET address in a list
 * Returns : TRUE/FALSE whether the address has been inserted or not
 *
 * IN_arg  : list     ==> generic list
 *	     addr     ==> MYRINET address to put
 *	     value    ==> Associated value
 */
boolean_t
net_myricom_mcp3_list_put(
    net_myricom_mcp3_list_t *list,
    net_myricom_mcp3_addr_t addr,
    int value)
{
    unsigned p;
    unsigned i = net_myricom_mcp3_32_get(&list->ls_length);
    net_myricom_mcp3_pair_t *pair = (net_myricom_mcp3_pair_t *)(list + 1);

    /*
     * Search for the the position to fill
     */
    if (i == 0) {
	net_myricom_mcp3_32_inc(&list->ls_length);
	net_myricom_mcp3_addr_set(pair->pa_address, addr);

    } else if (!net_myricom_mcp3_list_search(list, addr, &p)) {
	if (i == net_myricom_mcp3_32_get(&list->ls_max))
	    return (FALSE);

	pair = pair + i - 1;
	while (i-- > p) {
	    *(pair + 1) = *pair;
	    pair--;
	}
	net_myricom_mcp3_32_inc(&list->ls_length);
	net_myricom_mcp3_addr_set(pair->pa_address, addr);
    }

    /*
     * Fill position
     */
    net_myricom_mcp3_32_set(&pair->pa_value, value);
    net_myricom_mcp3_32_set(&list->ls_cache, i);

    return (TRUE);
}

/*
 * net_myricom_mcp3_list_remove
 *
 * Purpose : Remove a MYRINET address from a list
 * Returns : TRUE/FALSE whether the address has been removed or not
 *
 * IN_arg  : list ==> generic list
 *	     addr ==> MYRINET address to remove
 */
boolean_t
net_myricom_mcp3_list_remove(
    net_myricom_mcp3_list_t *list,
    net_myricom_mcp3_addr_t addr)
{
    unsigned p;
    unsigned i = net_myricom_mcp3_32_get(&list->ls_length);
    net_myricom_mcp3_pair_t *pair = (net_myricom_mcp3_pair_t *)(list + 1);

    if (i == 0 || !net_myricom_mcp3_list_search(list, addr, &p))
	return (FALSE);

    pair = pair + p;
    while (++p < i) {
	*pair = *(pair + 1);
	pair++;
    }
    net_myricom_mcp3_32_dec(&list->ls_length);
    return (TRUE);
}

/*
 * net_myricom_mcp3_sendq_peekput
 *
 * Purpose : Get room on the send queue
 * Returns : Send item on the send queue
 *	     NET_MYRICOM_MCP3_SENDQ_MAX if queue is full
 *
 * IN_arg  : sendq ==> Send queue
 */
unsigned int
net_myricom_mcp3_sendq_peekput(
    net_myricom_mcp3_sendq_t *sendq)
{
    unsigned int head;
    unsigned int tail;

    head = net_myricom_mcp3_32_get(&sendq->sq_head);
    tail = net_myricom_mcp3_32_get(&sendq->sq_tail);

    assert(head < NET_MYRICOM_MCP3_SENDQ_MAX);
    assert(tail < NET_MYRICOM_MCP3_SENDQ_MAX);

    if (tail + 1 == head ||
	(tail == NET_MYRICOM_MCP3_SENDQ_MAX - 1 && head == 0)) {
	/*
	 * Queue is full
	 */
	return (NET_MYRICOM_MCP3_SENDQ_MAX);
    }
    return (tail);
}

/*
 * net_myricom_mcp3_sendq_put
 *
 * Purpose : Put a new item on the send queue
 * Returns : Nothing
 *
 * IN_arg  : sendq ==> Send queue
 */
void
net_myricom_mcp3_sendq_put(
    net_myricom_mcp3_sendq_t *sendq)
{
    unsigned int tail;

    tail = net_myricom_mcp3_32_get(&sendq->sq_tail);
    assert(tail < NET_MYRICOM_MCP3_SENDQ_MAX);

    if (tail == NET_MYRICOM_MCP3_SENDQ_MAX - 1)
	net_myricom_mcp3_32_set(&sendq->sq_tail, 0);
    else
	net_myricom_mcp3_32_set(&sendq->sq_tail, tail + 1);
}

/*
 * net_myricom_mcp3_sendq_free
 *
 * Purpose : Test if a send item on the send queue is free
 * Returns : TRUE/FALSE whether the send item is free
 *
 * IN_arg  : sendq ==> Send queue
 *	     index ==> Item index
 */
boolean_t
net_myricom_mcp3_sendq_free(
    net_myricom_mcp3_sendq_t *sendq,
    unsigned int index)
{
    unsigned int head;
    unsigned int tail;

    head = net_myricom_mcp3_32_get(&sendq->sq_head);
    tail = net_myricom_mcp3_32_get(&sendq->sq_tail);

    assert(tail < NET_MYRICOM_MCP3_SENDQ_MAX);
    assert(head < NET_MYRICOM_MCP3_SENDQ_MAX);
    assert(index < NET_MYRICOM_MCP3_SENDQ_MAX);

    if (head <= tail)
	return (index < head || index > tail);
    return (index < tail || index > head);
}

/*
 * net_myricom_mcp3_recvq_put
 *
 * Purpose : Put items to a receive queue
 * Returns : Nothing
 *
 * IN_arg  : recvq ==> Receive queue
 *	     recvi ==> Receive items to put
 */
void
net_myricom_mcp3_recvq_put(
    net_myricom_mcp3_recvq_t *recvq,
    net_myricom_mcp3_recvi_t *recvi)
{
    unsigned int tail;

    tail = net_myricom_mcp3_32_get(&recvq->rq_tail);
    recvq->rq_items[tail] = *recvi;
    if (tail == NET_MYRICOM_MCP3_RECVQ_MAX)
	tail = 0;
    else
	tail++;
    net_myricom_mcp3_32_set(&recvq->rq_tail, tail);
}

/*
 * net_myricom_mcp3_recvq_get
 *
 * Purpose : Get items from a receive queue
 * Returns : Nothing
 *
 * IN_arg  : recvq ==> Receive queue
 *	     recvi ==> Received items
 */
void
net_myricom_mcp3_recvq_get(
    net_myricom_mcp3_recvq_t *recvq,
    net_myricom_mcp3_recvi_t *recvi)
{
    unsigned int head;
    unsigned int tail;
    unsigned int index;

    head = net_myricom_mcp3_32_get(&recvq->rq_head);
    tail = net_myricom_mcp3_32_get(&recvq->rq_tail);

    *recvi = recvq->rq_items[head];
    if (head == NET_MYRICOM_MCP3_RECVQ_MAX)
	head = 0;
    else
	head++;
    net_myricom_mcp3_32_set(&recvq->rq_head, head);
}

/*
 * net_myricom_mcp3_recvq_full
 *
 * Purpose : Test if a receive queue if full
 * Returns : TRUE/FALSE whether it is full not not
 *
 * IN_arg  : recvq ==> Receive queue
 */
boolean_t
net_myricom_mcp3_recvq_full(
    net_myricom_mcp3_recvq_t *recvq)
{
    unsigned int head;
    unsigned int tail;

    head = net_myricom_mcp3_32_get(&recvq->rq_head);
    tail = net_myricom_mcp3_32_get(&recvq->rq_tail);

    return (tail + 1 == head ||
	    tail == NET_MYRICOM_MCP3_RECVQ_MAX && head == 0);
}

/*
 * net_myricom_mcp3_recvq_empty
 *
 * Purpose : Test if a receive queue if empty
 * Returns : TRUE/FALSE whether it is empty not not
 *
 * IN_arg  : recvq ==> Receive queue
*/
boolean_t
net_myricom_mcp3_recvq_empty(
    net_myricom_mcp3_recvq_t *recvq)
{
    unsigned int head;
    unsigned int tail;

    head = net_myricom_mcp3_32_get(&recvq->rq_head);
    tail = net_myricom_mcp3_32_get(&recvq->rq_tail);

    return (head == tail);
}

/*
 * net_myricom_mcp3_attach
 *
 * Purpose  : MCP3 attachment
 * Returns  : Network generic interface
 *
 * IN_args  : iotype  ==> I/O address type
 *            iobase  ==> I/O base address
 *	      eeprom  ==> Myricom board EEPROM
 *	      status  ==> EBUS DMA status
 *	      control ==> Control LANai chip function
 *	      wakeup  ==> Wake up LANai chip function
 *	      regmap  ==> Bus-dependent LANai register map function
 *	      memmap  ==> Bus-dependent shared memory map function
 */
net_device_t
net_myricom_mcp3_attach(
    IOBUS_TYPE_T iotype,
    IOBUS_ADDR_T iobase,
    struct net_myricom_eeprom *eeprom,
    unsigned int status,
    void (*control)(net_device_t,
		    enum net_myricom_flavor),
    void (*wakeup)(net_device_t,
		   vm_offset_t,
		   unsigned int),
    IOBUS_SHMEM_T (*regmap)(net_device_t,
			    unsigned int,
			    unsigned int *),
    IOBUS_SHMEM_T (*memmap)(net_device_t,
			    vm_offset_t,
			    vm_size_t *))
{
    net_device_t netp;
    net_myricom_mcp3_t myri;
    unsigned int i;

    /*
     * Allocate a generic network interface structure
     */
    netp = net_driver_allocate("myri", -1, NET_MYRICOM_MCP3_SENDQ_MAX);

    /*
     * Allocate space for LANai structure
     */
    netp->net_nic = myri =
	(net_myricom_mcp3_t)kalloc(sizeof (struct net_myricom_mcp3));
    myri->myri_iotype = iotype;
    myri->myri_iobase = iobase;
    myri->myri_lanai = eeprom->myee_lanai;
    myri->myri_ramsize = eeprom->myee_ramsize;
    bzero((char *)&myri->myri_stats, sizeof (myri->myri_stats));
    myri->myri_regmap = regmap;
    myri->myri_memmap = memmap;
    myri->myri_control = control;
    myri->myri_wakeup = wakeup;
    myri->myri_status = status;
    myri->myri_flags = 0;
    myri->myri_numchan = 0;
    myri->myri_chan = NET_MYRICOM_MCP3_CHANNELS_MAX;
    myri->myri_reg = NET_MYRICOM_REG_CLOCK + 1;
    for (i = 0;
	 i < NET_MYRICOM_MCP3_CHANNELS_MAX * NET_MYRICOM_MCP3_SENDQ_MAX; i++)
	myri->myri_netd[i] = (net_done_t)0;

    switch (NET_MYRICOM_ID_VERSION(eeprom->myee_lanai)) {
    case 2:
	myri->myri_mcp = NET_MYRICOM_MCP3_LANAI_LOAD(2);
	myri->myri_mem = NET_MYRICOM_MCP3_L2_BASE;
	break;
    case 3:
	myri->myri_mcp = NET_MYRICOM_MCP3_LANAI_LOAD(3);
	myri->myri_mem = NET_MYRICOM_MCP3_L4_BASE;
	break;
    case 4:
	if (NET_MYRICOM_ID_REVISION(eeprom->myee_lanai) == 0) {
	    /*
	     * Load MCP program without partword load
	     */
	    myri->myri_mcp = NET_MYRICOM_MCP3_LANAI_LOAD(40);
	} else
	    myri->myri_mcp = NET_MYRICOM_MCP3_LANAI_LOAD(4);
	myri->myri_mem = NET_MYRICOM_MCP3_L4_BASE;
	break;
    default:
	panic("%s: Unexpected LANai ID 0x%x\n",
	      "net_myricom_mcp3_attach", eeprom->myee_lanai);
    }
    for (i = 0; i < NET_MYRICOM_MCP3_CHANNELS_MAX; i++) {
	myri->myri_module[i] = NET_MODULE_UNKNOWN;
	myri->myri_index[i] = 0;
    }

    for (i = 0;
	 i < NET_MYRICOM_MCP3_CHANNELS_MAX * (NET_MYRICOM_MCP3_RECVQ_MAX + 1);
	 i++)
	myri->myri_context[i] = (vm_offset_t)0;

    /*
     * Initialize interface NIC fields
     */
    netp->net_nic_ops.nic_init      = net_myricom_mcp3_init;
    netp->net_nic_ops.nic_reset     = net_myricom_mcp3_reset;
    netp->net_nic_ops.nic_start     = net_myricom_mcp3_start;
    netp->net_nic_ops.nic_intr      = net_myricom_mcp3_intr;
    netp->net_nic_ops.nic_timeout   = net_myricom_mcp3_timeout;
    netp->net_nic_ops.nic_copyin    = net_myricom_mcp3_copyin;
    netp->net_nic_ops.nic_copyout   = net_myricom_mcp3_copyout;
    netp->net_nic_ops.nic_multicast = net_myricom_mcp3_multicast;
    netp->net_nic_ops.nic_dinfo     = net_myricom_mcp3_dinfo;
    netp->net_nic_ops.nic_prepout   = net_myricom_mcp3_prepout;
    netp->net_nic_ops.nic_prepin    = net_myricom_mcp3_prepin;

    /*
     * Initialize other interface fields
     */
    netp->net_status.min_packet_size = sizeof (struct if_myrinet_eep);
    netp->net_status.max_packet_size =
	IF_MYRINET_EEP_MTU + sizeof (struct if_myrinet_eep);
    netp->net_status.header_format = HDR_ETHERNET;
    netp->net_status.header_size = sizeof (struct if_myrinet_eep);
    netp->net_status.address_size = IF_MYRINET_UID_SIZE;
    netp->net_status.flags |= NET_STATUS_BROADCAST /* | NET_STATUS_MULTICAST */;
    netp->net_status.media_type = NET_MEDIA_OTHERS;
    netp->net_status.media_speed = 640000000; /* in bits/sec */
    netp->net_status.high_media_speed = 640; /* in Mbits/sec */
    netp->net_status.max_queue_size = NET_MYRICOM_MCP3_SENDQ_MAX;
    netp->net_status.data_alignment = NET_MYRICOM_MCP3_ALIGNMENT;
    netp->net_address = myri->myri_uid;
    bzero((char *)myri->myri_uid, IF_MYRINET_UID_SIZE);
    netp->net_pool_min = NET_MYRICOM_MCP3_RECVQ_MAX;
    netp->net_spfstat = &myri->myri_stats;
    netp->net_spfcount = NET_MYRINET_STATS_COUNT;
    netp->net_msize[0] = netp->net_msize[1] = 0;
    netp->net_osize = sizeof (struct net_myricom_mcp3_done) +
	(round_page(IF_MYRINET_EEP_MTU + sizeof (struct if_myrinet_eep)) /
	 PAGE_SIZE) * sizeof (vm_offset_t);

    /*
     * Finally, reset NIC
     */
    if (!net_myricom_mcp3_reset(netp))
        panic("%s: cannot reset NIC on adapter %s%d\n",
              "net_myricom_mcp3_attach", NET_DEVICE_NAME, netp->net_generic);

    return (netp);
}

/*
 * net_myricom_mcp3_write
 *
 * Purpose : MCP program data handling
 * Returns : Nothing
 *
 * IN_arg  : netp ==> Network generic interface structure
 *           ior  ==> output I/O request
 */
void
net_myricom_mcp3_write(
    net_device_t netp,
    io_req_t ior)
{
    net_myricom_mcp3_t myri = (net_myricom_mcp3_t)netp->net_nic;
    vm_size_t size;
    io_buf_len_t count;
    recnum_t cur;
    IOBUS_SHMEM_T addr;
    vm_size_t len;

    assert((myri->myri_flags & NET_MYRICOM_MCP3_FLAGS_READY) == 0);

    len = 0;
    for (count = 0, cur = ior->io_recnum;
	 count < ior->io_count && cur < myri->myri_ramsize;
	 count += size, cur += size, len -= size, addr += size) {
	/*
	 * Compute size of data to copy
	 */
        size = PAGE_SIZE - (cur & PAGE_MASK);
        if (count + size > ior->io_count)
            size = ior->io_count - count;

	/*
	 * Map shared memory pages
	 */
	if (len < size) {
	    len = ior->io_count - count;
	    addr = (*myri->myri_memmap)(netp, (vm_offset_t)cur, &len);
	}

	/*
	 * Copy MCP segment
	 */
        bcopy(ior->io_data + count, (char *)addr, size);
    }

    if (ior->io_residual = ior->io_count - count) {
	ior->io_error = D_INVALID_SIZE;
	ior->io_op |= IO_ERROR;
    }
    io_completed(ior, TRUE);
}

/*
 * net_myricom_mcp3_close
 *
 * Purpose : Completion of MCP program loading
 * Returns : Nothing
 *
 * IN_arg  : netp ==> Network generic interface structure
 */
void
net_myricom_mcp3_close(
    net_device_t netp)
{
    net_myricom_mcp3_t myri = (net_myricom_mcp3_t)netp->net_nic;
    net_myricom_mcp3_channel_t *chan;
    unsigned int start;
    unsigned int stop;
    unsigned int i;
    vm_size_t len;
    net_myricom_mcp3_shmem_t *mem;
    net_myricom_mcp3_addr_t mid;

    assert((myri->myri_flags & NET_MYRICOM_MCP3_FLAGS_READY) == 0);

    /*
     * Initialize MYRICOM address
     */
    len = sizeof (net_myricom_mcp3_shmem_t);
    mem = (net_myricom_mcp3_shmem_t *)
	(*myri->myri_memmap)(netp, myri->myri_mem, &len);
    assert(len >= sizeof (net_myricom_mcp3_shmem_t));
    for (i = 0; i < IF_MYRINET_MID_SIZE - IF_MYRINET_UID_SIZE; i++)
	mid[i] = 0;
    for (i = 0; i < IF_MYRINET_UID_SIZE; i++)
	mid[i + IF_MYRINET_MID_SIZE - IF_MYRINET_UID_SIZE] = myri->myri_uid[i];
    net_myricom_mcp3_addr_set(mem->sh_address, mid);

    /*
     * Start LANai chip
     */
    (*myri->myri_control)(netp, NET_MYRICOM_FLAVOR_START);

    /*
     * Initialize DMA status
     */
    net_myricom_mcp3_32_set(&mem->sh_burst, myri->myri_status);

    /*
     * Enable all Interrupts
     */
    net_myricom_mcp3_32_set(&mem->sh_itmask, -1);

    /*
     * Handshake with MCP
     */
    chan = &mem->sh_channel[0];
    if (!NET_MYRICOM_HPI_WAS_RESET(chan)) {
	printf("%s%d: MCP3 handshake while LANai not reset\n",
	       NET_DEVICE_NAME, netp->net_generic);
	return;
    }

    start = net_myricom_mcp3_32_get(&mem->sh_debug[0]);
    for (i = 0; i < NET_MYRICOM_MCP3_SHAKE_TRIES; i++) {
	if (net_myricom_mcp3_32_get(&chan->ch_state) ==
	    NET_MYRICOM_MCP3_STATE_HOST) {
	    /*
	     * HOST is waiting for the NET
	     */
	    net_myricom_mcp3_32_set(&chan->ch_state,
				    NET_MYRICOM_MCP3_STATE_NET);
	}
	NET_MYRICOM_HPI_WAKE_SHAKE(netp, myri);

	if (net_myricom_mcp3_32_get(&chan->ch_state) ==
	    NET_MYRICOM_MCP3_STATE_READY)
	    break;

	delay(NET_MYRICOM_MCP3_SLEEP_USEC);
    }

    /*
     * Try to map all registers
     */
    i = NET_MYRICOM_REG_CLOCK;
    myri->myri_mapreg = (volatile unsigned int *)
	(*myri->myri_regmap)(netp, NET_MYRICOM_REG_IPF0, &i);
    if (i >= NET_MYRICOM_REG_CLOCK)
	myri->myri_flags |= NET_MYRICOM_MCP3_FLAGS_MAPREG;
    else
	myri->myri_reg = NET_MYRICOM_REG_IPF0;

    /*
     * Try to map all channels
     */
    myri->myri_flags |= NET_MYRICOM_MCP3_FLAGS_READY;
    myri->myri_numchan = net_myricom_mcp3_32_get(&mem->sh_num);
    len = myri->myri_numchan * sizeof (mem->sh_channel[0]);
    myri->myri_mapchan = (net_myricom_mcp3_channel_t *)
	(*myri->myri_memmap)(netp,
			     NET_MYRICOM_MCP3_GETCHANNEL(myri->myri_mem, 0),
			     &len);
    if (len >= myri->myri_numchan * sizeof (mem->sh_channel[0]))
	myri->myri_flags |= NET_MYRICOM_MCP3_FLAGS_MAPMEM;
    else
	myri->myri_chan = 0;

    if (i == NET_MYRICOM_MCP3_SHAKE_TRIES) {
	stop = net_myricom_mcp3_32_get(&mem->sh_debug[0]);
	printf("%s%d: %s (start == %d, stop == %d)\n",
	        NET_DEVICE_NAME, netp->net_generic,
	       "MCP3 initial handshake failed", start, stop);
	return;
    }

    /*
     * Set generic interface flags
     */
    assert((netp->net_status.flags & NET_STATUS_RUNNING) == 0);
    netp->net_status.flags |= NET_STATUS_RUNNING;

    /*
     * Now, it is safe to enable interrupts
     */
    myri->myri_module[0] = NET_MODULE_DEVICE;
    (*myri->myri_control)(netp, NET_MYRICOM_FLAVOR_INTR);
}

/*
 * net_myricom_mcp3_netd_done
 *
 * Purpose : Clean net done element
 * Returns : Nothing
 *
 * IN_arg  : netp ==> Network generic interface structure
 *           netd ==> Output packet
 */
void
net_myricom_mcp3_netd_done(
    net_device_t netp,
    net_done_t netd)
{
    net_myricom_mcp3_done_t mcpd = (net_myricom_mcp3_done_t)netd->netd_external;
    unsigned int count;

    /*
     * Free internal buffers
     */
    for (count = 0; count <= mcpd->mcpd_intnum; count++)
	kfree(mcpd->mcpd_intvirt[count], PAGE_SIZE);
}

/*
 * net_myricom_mcp3_prepin
 *
 * Purpose : Prepare copyin DMA actions
 * Returns : TRUE/FALSE whether the buffer is accepted or rejected
 *
 * IN_arg  : netp    ==> Network generic interface structure
 *	     module  ==> Module to give buffer to
 *	     context ==> Context to give back on copyin
 *           header  ==> Header virtual address
 *           data    ==> Data address
 *
 * Comment : Header is always a virtual address, data is either a virtual
 *	address or an io_sg_list depending on the NET_STATUS_SG flag.
 */
boolean_t
net_myricom_mcp3_prepin(
    net_device_t netp,
    int module,
    vm_offset_t context,
    vm_offset_t header,
    vm_offset_t data)
{
    net_myricom_mcp3_t myri = (net_myricom_mcp3_t)netp->net_nic;
    net_myricom_mcp3_channel_t *chan;
    unsigned int n;
    unsigned int m;
    unsigned int index;
    struct io_sg_entry *iosge;
    vm_offset_t addr;
    vm_offset_t phys;
    vm_offset_t curr;
    vm_size_t size;
    vm_size_t length;
    net_myricom_mcp3_recvi_t recvi;

    /*
     * Validate module
     * XXX Only MSERVER module supported for now
     */
    if (module != NET_MODULE_DEVICE)
	return (FALSE);

    /*
     * Synchronize with MCP
     */
    NET_MYRICOM_MCP3_SETCHANNEL(netp, myri, chan, 0);
    if (NET_MYRICOM_HPI_WAS_RESET(chan))
	return (FALSE);

    /*
     * Don't fill up input context if there is no place
     */
    if (net_myricom_mcp3_recvq_full(&chan->ch_recv))
	return (FALSE);

    /*
     * Don't overwrite input context not yet consumed by the driver
     */
    index = net_myricom_mcp3_32_get(&chan->ch_recv.rq_tail);
    if (myri->myri_context[index] != (vm_offset_t)0)
	return (FALSE);

    /*
     * Compute how many pages will be given to the MCP module
     */
    assert((header & (NET_MYRICOM_MCP3_ALIGNMENT - 1)) == 0);
    net_myricom_mcp3_32_set(&recvi.ri_scatters[0].ga_pointer, kvtophys(header));
    net_myricom_mcp3_32_set(&recvi.ri_scatters[0].ga_length,
			    sizeof (struct if_myrinet_eep));
    IOBUS_PURGE_DCACHE(myri->myri_iotype, header,
		       sizeof (struct if_myrinet_eep));

    n = 1;
    if (netp->net_status.flags & NET_STATUS_SG) {
	iosge = ((io_sglist_t)data)->iosg_list;
	m = 0;
	while (m++ < ((io_sglist_t)data)->iosg_hdr.nentries &&
	       n < NET_MYRICOM_MCP3_SCATTER_MAX) {
	    if (m > 1 && phys + length == iosge->iosge_phys)
		length += iosge->iosge_length;
	    else {
		if (m > 1) {
		    assert((phys & (NET_MYRICOM_MCP3_ALIGNMENT - 1)) == 0);
		    net_myricom_mcp3_32_set(&recvi.ri_scatters[n].ga_pointer,
					    phys);
		    assert((length & (NET_MYRICOM_MCP3_ALIGNMENT - 1)) == 0);
		    net_myricom_mcp3_32_set(&recvi.ri_scatters[n++].ga_length,
					    length);
		}
		phys = iosge->iosge_phys;
		length = iosge->iosge_length;
	    }
	    iosge++;
	}
    } else {
	for (addr = data; (addr < data + IF_MYRINET_EEP_MTU &&
			   n < NET_MYRICOM_MCP3_SCATTER_MAX); addr += size) {
	    size = PAGE_SIZE - (addr - trunc_page(addr));
	    if (addr + size > data + IF_MYRINET_EEP_MTU)
		size = (data + IF_MYRINET_EEP_MTU) - addr;

	    curr = kvtophys(addr);
	    if (addr > data && phys + length == curr)
		length += size;
	    else {
		if (addr > data) {
		    assert((phys & (NET_MYRICOM_MCP3_ALIGNMENT - 1)) == 0);
		    net_myricom_mcp3_32_set(&recvi.ri_scatters[n].ga_pointer,
					    phys);
		    assert((length & (NET_MYRICOM_MCP3_ALIGNMENT - 1)) == 0);
		    net_myricom_mcp3_32_set(&recvi.ri_scatters[n++].ga_length,
					    length);
		}
		phys = curr;
		length = size;
	    }
	    IOBUS_PURGE_DCACHE(myri->myri_iotype, addr, size);
	}
    }
    assert (n < NET_MYRICOM_MCP3_SCATTER_MAX);

    assert((phys & (NET_MYRICOM_MCP3_ALIGNMENT - 1)) == 0);
    net_myricom_mcp3_32_set(&recvi.ri_scatters[n].ga_pointer, phys);
    assert((length & (NET_MYRICOM_MCP3_ALIGNMENT - 1)) == 0);
    net_myricom_mcp3_32_set(&recvi.ri_scatters[n++].ga_length, length);

    net_myricom_mcp3_32_set(&recvi.ri_num, n);
    net_myricom_mcp3_32_set(&recvi.ri_context, context);

#if MACH_ASSERT
    while (n < NET_MYRICOM_MCP3_SCATTER_MAX) {
	net_myricom_mcp3_32_set(&recvi.ri_scatters[n].ga_pointer, 0);
	net_myricom_mcp3_32_set(&recvi.ri_scatters[n++].ga_length, 0);
    }
#endif /* MACH_ASSERT */

    net_myricom_mcp3_recvq_put(&chan->ch_recv, &recvi);
    myri->myri_context[index] = context;
    return (TRUE);
}

/*
 * net_myricom_mcp3_prepout
 *
 * Purpose  : Prepare copyout DMA actions
 * Returns  : Nothing
 *
 * In_args  : netp  ==> Network generic interface structure
 *	      netd  ==> packet callback
 *	      addr  ==> Output buffer
 *	      len   ==> Output length
 *	      flags ==> Output flags
 */
void
net_myricom_mcp3_prepout(
    net_device_t netp,
    net_done_t netd,
    vm_offset_t addr,
    vm_size_t len,
    unsigned int flags)
{
    net_myricom_mcp3_done_t mcpd = (net_myricom_mcp3_done_t)netd->netd_external;
    net_myricom_mcp3_t myri = (net_myricom_mcp3_t)netp->net_nic;
    unsigned int count;
    vm_offset_t phys;
    vm_size_t size;
    vm_offset_t int_virt;
    vm_offset_t int_phys;
    vm_offset_t dma_virt;
    vm_offset_t dma_phys;
    unsigned char *header;
    unsigned int i;
    unsigned char *p;

    assert((flags & NET_BICOUT_FLAGS_PADDING) == 0 && len > 0);
    if (flags & NET_BICOUT_FLAGS_FIRST) {
	/*
	 * Packet callback initialization
	 */
	bzero((char *)mcpd, netp->net_osize);
	mcpd->mcpd_num = mcpd->mcpd_intnum = -1;

	/*
	 * Extract MYRINET destination address
	 */
	if (netd->netd_ptype == NET_PACKET_MULTICAST ||
	    netd->netd_ptype == NET_PACKET_BROADCAST) {
	    /*
	     * Non unicast packets must be sent to all Myrinet boards
	     */
	    for (i = 0; i < IF_MYRINET_MID_SIZE; i++)
		mcpd->mcpd_dmid[i] = 0xFF;
	} else {
	    assert(netd->netd_ptype == NET_PACKET_UNICAST);
	    header = ((struct if_myrinet_eep *)addr)->myri_ether.ether_dhost;
	    p = mcpd->mcpd_dmid;
	    for (i = 0; i < IF_MYRINET_MID_SIZE - IF_MYRINET_UID_SIZE; i++)
		*p++ = 0;
	    if ((flags & NET_BICOUT_FLAGS_VIRTUAL) == 0)
		pmap_copy_part_page((vm_offset_t)header, 0,
				    kvtophys((vm_offset_t)p), 0,
				    IF_MYRINET_UID_SIZE);
	    else
		bcopy((const char *)header, (char *)p, IF_MYRINET_UID_SIZE);
	}

    } else {
	/*
	 * Compute relative positions of last DMA buffer
	 */
	assert (mcpd->mcpd_num >= 0);
	dma_phys = (mcpd->mcpd_phys[mcpd->mcpd_num] +
		    mcpd->mcpd_size[mcpd->mcpd_num]);
	if (mcpd->mcpd_flags & (NET_MYRICOM_MCP3_DONE_INTERNAL |
				NET_MYRICOM_MCP3_DONE_VIRTUAL))
	    dma_virt = (mcpd->mcpd_virt[mcpd->mcpd_num] +
			mcpd->mcpd_size[mcpd->mcpd_num]);
	if (mcpd->mcpd_intnum >= 0) {
	    /*
	     * Compute relative positions of last internal buffer
	     */
	    int_virt = (mcpd->mcpd_intvirt[mcpd->mcpd_intnum] +
			mcpd->mcpd_intsize);
	    int_phys = kvtophys(int_virt);
	}
    }

    /*
     * This loop must be run while all the length of the current buffer has
     * not been handled or if there is some unhandled size in the last buffer
     */
    while (len > 0 ||
	   ((flags & NET_BICOUT_FLAGS_LAST) && mcpd->mcpd_rest > 0)) {
	/*
	 * Compute the address/size of the data area to validate for DMA
	 */
	if (flags & NET_BICOUT_FLAGS_VIRTUAL) {
	    phys = kvtophys(addr);
	    size = PAGE_SIZE - (phys - trunc_page(phys));
	    if (size > len)
		size = len;
	} else {
	    phys = addr;
	    size = len;
	}
	if ((mcpd->mcpd_intsize & PAGE_MASK) +
	    mcpd->mcpd_rest + size > PAGE_SIZE)
	    size = PAGE_SIZE - (mcpd->mcpd_intsize & PAGE_MASK);

	/*
	 * Deal with relative positions of last and current buffers
	 */
	if (mcpd->mcpd_num >= 0 && size > 0 &&
	    dma_phys + mcpd->mcpd_rest == phys &&
	    ((flags & NET_BICOUT_FLAGS_VIRTUAL) == 0) ==
	    ((mcpd->mcpd_flags & NET_MYRICOM_MCP3_DONE_VIRTUAL) == 0)) {
	    /*
	     * Current buffer is contiguous with the previous one
	     */
	    count = ((mcpd->mcpd_rest + size) &
		     ~(NET_MYRICOM_MCP3_ALIGNMENT - 1));
	    if (count > 0) {
		if (mcpd->mcpd_flags & NET_MYRICOM_MCP3_DONE_INTERNAL) {
		    /*
		     * Keep on copying into the internal buffer
		     */
		    if (flags & NET_BICOUT_FLAGS_VIRTUAL) {
			bcopy((const char *)dma_virt, (char *)int_virt, count);
			IOBUS_FLUSH_DCACHE(myri->myri_iotype, int_virt, count);
		    } else
			pmap_copy_part_page(dma_phys, 0, int_phys, 0, count);

		    mcpd->mcpd_intsize += count;
		    int_virt += count;
		    int_phys += count;

		} else if (flags & NET_BICOUT_FLAGS_VIRTUAL)
		    IOBUS_FLUSH_DCACHE(myri->myri_iotype, dma_virt, count);

		if (flags & NET_BICOUT_FLAGS_VIRTUAL)
		    dma_virt += count;
		dma_phys += count;

		mcpd->mcpd_size[mcpd->mcpd_num] += count;
	    }
	    mcpd->mcpd_rest = (size + mcpd->mcpd_rest) - count;

	    addr += size;
	    len -= size;
	    continue;
	}

	/*
	 * Previous and current buffers are not contiguous
	 */
	if (mcpd->mcpd_rest > 0) {
	    /*
	     * First, complete handling of the previous buffer
	     */
	    if (mcpd->mcpd_intnum < 0 ||
		(mcpd->mcpd_intsize & PAGE_MASK) == 0) {
		/*
		 * Allocate a new internal buffer
		 */
		if (netd->netd_done == (void (*)(net_device_t, net_done_t))0)
		    netd->netd_done = net_myricom_mcp3_netd_done;
		int_virt = mcpd->mcpd_intvirt[++mcpd->mcpd_intnum] =
		    kalloc(PAGE_SIZE);
		int_phys = kvtophys(int_virt);
	    }

	    /*
	     * Copy the unhandled part of the previous buffer
	     */
	    if (mcpd->mcpd_flags & NET_MYRICOM_MCP3_DONE_VIRTUAL) {
		bcopy((const char *)dma_virt,
		      (char *)int_virt, mcpd->mcpd_rest);
		IOBUS_FLUSH_DCACHE(myri->myri_iotype,
				   int_virt, mcpd->mcpd_rest);
	    } else
		pmap_copy_part_page(dma_phys, 0, int_phys, 0, mcpd->mcpd_rest);

	    /*
	     * Update copyout current state
	     */
	    if ((mcpd->mcpd_flags & NET_MYRICOM_MCP3_DONE_INTERNAL) == 0 ||
		int_phys != dma_phys + mcpd->mcpd_rest) {
		/*
		 * Initialize a new gather entry
		 */
		assert(mcpd->mcpd_num < NET_MYRICOM_MCP3_GATHER_MAX - 1);
		mcpd->mcpd_virt[++mcpd->mcpd_num] = dma_virt = int_virt;
		mcpd->mcpd_phys[mcpd->mcpd_num] = dma_phys = int_phys;
	    }
	    mcpd->mcpd_intsize += mcpd->mcpd_rest;
	    mcpd->mcpd_size[mcpd->mcpd_num] += mcpd->mcpd_rest;
	    dma_phys += mcpd->mcpd_rest;
	    int_phys += mcpd->mcpd_rest;
	    int_virt += mcpd->mcpd_rest;

	    if (mcpd->mcpd_flags & NET_MYRICOM_MCP3_DONE_VIRTUAL)
		dma_virt += mcpd->mcpd_rest;

	    if ((phys + mcpd->mcpd_rest) &
		(NET_MYRICOM_MCP3_ALIGNMENT - 1) == 0) {
		/*
		 * Current buffer position leads back to a correct alignment
		 */
		count = NET_MYRICOM_MCP3_ALIGNMENT - mcpd->mcpd_rest;

		/*
		 * Copy the unaligned part of current buffer
		 */
		if (flags & NET_BICOUT_FLAGS_VIRTUAL) {
		    bcopy((const char *)addr, (char *)int_virt, count);
		    IOBUS_FLUSH_DCACHE(myri->myri_iotype, int_virt, count);
		    addr += count;
		} else
		    pmap_copy_part_page(phys, 0, int_phys, 0, count);

		/*
		 * Adjust pointer/size values
		 */
		mcpd->mcpd_intsize += count;
		mcpd->mcpd_size[mcpd->mcpd_num] += count;
		dma_phys += count;
		int_phys += count;
		int_virt += count;

		addr += count;
		phys += count;
		len -= count;
		size -= count;
	    }

	    mcpd->mcpd_rest = 0;
	    if ((mcpd->mcpd_flags & NET_MYRICOM_MCP3_DONE_INTERNAL) == 0)
		mcpd->mcpd_flags |= NET_MYRICOM_MCP3_DONE_INTERNAL;
	}

	/*
	 * Now handle the current buffer
	 */
	if ((phys & (NET_MYRICOM_MCP3_ALIGNMENT - 1)) == 0) {
	    /*
	     * Current buffer is correctly aligned
	     */
	    count = size & ~(NET_MYRICOM_MCP3_ALIGNMENT - 1);
	    if (count == 0)
		continue;

	    mcpd->mcpd_rest = size - count;
	    assert(mcpd->mcpd_num < NET_MYRICOM_MCP3_GATHER_MAX - 1);
	    mcpd->mcpd_phys[++mcpd->mcpd_num] = phys;
	    mcpd->mcpd_size[mcpd->mcpd_num] = count;
	    dma_phys = phys + count;

	    if (mcpd->mcpd_flags & NET_MYRICOM_MCP3_DONE_INTERNAL)
		mcpd->mcpd_flags &= ~NET_MYRICOM_MCP3_DONE_INTERNAL;
	    if (flags & NET_BICOUT_FLAGS_VIRTUAL) {
		mcpd->mcpd_virt[mcpd->mcpd_num] = addr;
		IOBUS_FLUSH_DCACHE(myri->myri_iotype, addr, count);
		dma_virt = addr + count;
		if ((mcpd->mcpd_flags & NET_MYRICOM_MCP3_DONE_VIRTUAL) == 0)
		    mcpd->mcpd_flags |= NET_MYRICOM_MCP3_DONE_VIRTUAL;
	    } else if (mcpd->mcpd_flags & NET_MYRICOM_MCP3_DONE_VIRTUAL)
		mcpd->mcpd_flags &= ~NET_MYRICOM_MCP3_DONE_VIRTUAL;

	    addr += size;
	    len -= size;
	    continue;
	}

	/*
	 * Current buffer is not correctly aligned
	 */
	if (size == 0)
	    continue;

	if (mcpd->mcpd_intnum < 0 || (mcpd->mcpd_intsize & PAGE_MASK) == 0) {
	    /*
	     * Allocate a new internal buffer
	     */
	    if (netd->netd_done == (void (*)(net_device_t, net_done_t))0)
		netd->netd_done = net_myricom_mcp3_netd_done;
	    int_virt = mcpd->mcpd_intvirt[++mcpd->mcpd_intnum] =
		kalloc(PAGE_SIZE);
	    int_phys = kvtophys(int_virt);
	}

	if ((mcpd->mcpd_flags & NET_MYRICOM_MCP3_DONE_INTERNAL) == 0 ||
	    int_phys != dma_phys) {
	    /*
	     * Initialize a new gather entry
	     */
	    assert(mcpd->mcpd_num < NET_MYRICOM_MCP3_GATHER_MAX - 1);
	    mcpd->mcpd_virt[++mcpd->mcpd_num] = dma_virt = int_virt;
	    mcpd->mcpd_phys[mcpd->mcpd_num] = dma_phys = int_phys;
	}

	/*
	 * Copy unaligned buffer into an internal buffer
	 */
	if (flags & NET_BICOUT_FLAGS_VIRTUAL) {
	    bcopy((const char *)addr, (char *)int_virt, size);
	    IOBUS_FLUSH_DCACHE(myri->myri_iotype, int_virt, size);
	    dma_virt += size;
	} else
	    pmap_copy_part_page(phys, 0, int_phys, 0, size);
	dma_phys += size;
	addr += size;
	len -= size;

	int_virt += size;
	int_phys += size;
	mcpd->mcpd_rest = 0;
	mcpd->mcpd_size[mcpd->mcpd_num] += size;
	mcpd->mcpd_intsize += size;

	if ((mcpd->mcpd_flags & NET_MYRICOM_MCP3_DONE_INTERNAL) == 0)
	    mcpd->mcpd_flags |= NET_MYRICOM_MCP3_DONE_INTERNAL;
    }

    if ((flags & NET_BICOUT_FLAGS_LAST) &&
	(netd->netd_length & (NET_MYRICOM_MCP3_ALIGNMENT - 1))) {
	/*
	 * Complete with trailing zeroes
	 */
	assert(mcpd->mcpd_flags & NET_MYRICOM_MCP3_DONE_INTERNAL);
	count = NET_MYRICOM_MCP3_ALIGNMENT -
	    (netd->netd_length & (NET_MYRICOM_MCP3_ALIGNMENT - 1));
	assert(((int_virt + count) & (NET_MYRICOM_MCP3_ALIGNMENT - 1)) == 0);
	assert(mcpd->mcpd_size[mcpd->mcpd_num] + count <= PAGE_SIZE);

	bzero((char *)int_virt, count);
	IOBUS_FLUSH_DCACHE(myri->myri_iotype, int_virt, count);
	mcpd->mcpd_size[mcpd->mcpd_num] += count;
	mcpd->mcpd_intsize += count;
    }
}

/*
 * net_myricom_mcp3_dinfo
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
net_myricom_mcp3_dinfo(
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

/*
 * net_myricom_mcp3_reset
 *
 * Purpose  : NIC reset
 * Returns  : TRUE/FALSE if device successfully reset
 *
 * IN_args  : netp ==> Generic net structure
 */
boolean_t
net_myricom_mcp3_reset(
    net_device_t netp)
{
    net_myricom_mcp3_t myri = (net_myricom_mcp3_t)netp->net_nic;
    unsigned int i;
    unsigned int j;
    unsigned int index;

    /*
     * Reset flags
     */
    myri->myri_flags = 0;

    /*
     * Dequeue all outstanding transmissions
     */
    index = 0;
    for (i = 0; i < myri->myri_numchan; i++) {
	if (myri->myri_module[i] == NET_MODULE_UNKNOWN)
	    index += NET_MYRICOM_MCP3_SENDQ_MAX;
	else {
	    for (j = 0; j < NET_MYRICOM_MCP3_SENDQ_MAX; j++) {
		if (myri->myri_netd[index] != (net_done_t)0) {
		    mpenqueue_tail(&netp->net_done,
				   &(myri->myri_netd[index])->netd_queue);
		    myri->myri_netd[index] = (net_done_t)0;
		}
		index++;
	    }
	}
    }

    /*
     * Free all prepared input DMA buffers
     */
    index = 0;
    for (i = 0; i < myri->myri_numchan; i++) {
	myri->myri_index[i] = 0;
	if (myri->myri_module[i] == NET_MODULE_UNKNOWN)
	    index += NET_MYRICOM_MCP3_RECVQ_MAX + 1;
	else {
	    for (j = 0; j < NET_MYRICOM_MCP3_RECVQ_MAX + 1; j++) {
		if (myri->myri_context[index] != (vm_offset_t)0) {
		    net_driver_unprepare(netp, myri->myri_module[i],
					 myri->myri_context[index]);
		    myri->myri_context[index] = (vm_offset_t)0;
		}
		index++;
	    }
	}
    }

    /*
     * Reset all channel status
     */
    for (i = 0; i < myri->myri_numchan; i++)
	if (myri->myri_module[i] != NET_MODULE_UNKNOWN)
	    myri->myri_module[i] = NET_MODULE_UNKNOWN;

    return (TRUE);
}

/*
 * net_myricom_mcp3_init
 *
 * Purpose  : NIC initialization
 * Returns  : TRUE/FALSE if device successfully initialized
 *
 * IN_args  : netp ==> Generic net structure
 */
boolean_t
net_myricom_mcp3_init(
    net_device_t netp)
{
    net_myricom_mcp3_t myri = (net_myricom_mcp3_t)netp->net_nic;

    /*
     * Ask for MCP program load
     */
    DATADEV_INIT(&myri->myri_datadev, myri->myri_mcp,
		 net_myricom_mcp3_close, net_myricom_mcp3_write, netp);
    datadev_request(&myri->myri_datadev);

    return (TRUE);
}

/*
 * net_myricom_mcp3_copyout
 *
 * Purpose  : Try to copy a packet to the adapter memory
 * Returns  : NET_NICOUT_ERROR     if an error occured
 *	      NET_NICOUT_COMPLETED if a packet has been copied out on place
 *	      NET_NICOUT_SUCCESS   if a packet can be copied out
 *            NET_NICOUT_DELAYED   if no available space
 *
 * IN_args  : netp ==> Network generic interface structure
 *            netd ==> packet callback
 */
/*ARGSUSED*/
net_nicout_t
net_myricom_mcp3_copyout(
    net_device_t netp,
    net_done_t netd)
{
    net_myricom_mcp3_t myri = (net_myricom_mcp3_t)netp->net_nic;
    net_myricom_mcp3_done_t mcpd;
    net_myricom_mcp3_channel_t *chan;
    unsigned int i;
    unsigned int len;
    net_myricom_mcp3_sendi_t *sendi;
    net_done_t *netdd;

    /*
     * Validate channel
     */
    mcpd = (net_myricom_mcp3_done_t)netd->netd_external;
    if (mcpd->mcpd_schannel >= myri->myri_numchan)
	return (NET_NICOUT_ERROR);

    /*
     * Synchronize with MCP
     */
    NET_MYRICOM_MCP3_SETCHANNEL(netp, myri, chan, mcpd->mcpd_schannel);
    if (NET_MYRICOM_HPI_WAS_RESET(chan))
	return (NET_NICOUT_ERROR);

    /*
     * Get an item from the send queue
     */
    i = net_myricom_mcp3_sendq_peekput(&chan->ch_send);
    if (i == NET_MYRICOM_MCP3_SENDQ_MAX) {
	printf("net_myricom_mcp3_copyout: no more place\n");
	return (NET_NICOUT_DELAYED);
    }
    sendi = &chan->ch_send.sq_items[i];

    /*
     * Record Output done queue element sent
     */
    netdd =
	&myri->myri_netd[mcpd->mcpd_schannel * NET_MYRICOM_MCP3_SENDQ_MAX + i];
    if (*netdd != (net_done_t)0)
	mpenqueue_tail(&netp->net_done, &(*netdd)->netd_queue);
    *netdd = netd;

    /*
     * Initialize the send item
     */
    assert(netd->netd_length <= (IF_MYRINET_EEP_MTU +
				 sizeof (struct if_myrinet_eep)));
    len = (netd->netd_length + NET_MYRICOM_MCP3_ALIGNMENT - 1) &
	~(NET_MYRICOM_MCP3_ALIGNMENT - 1);
    net_myricom_mcp3_32_set(&sendi->si_length, len);
    net_myricom_mcp3_addr_set(sendi->si_address, mcpd->mcpd_dmid);
    net_myricom_mcp3_32_set(&sendi->si_channel, mcpd->mcpd_dchannel);
    net_myricom_mcp3_32_set(&sendi->si_num, mcpd->mcpd_num + 1);
    net_myricom_mcp3_32_set(&sendi->si_offset, NET_MYRICOM_MCP3_NO_CHECKSUM);
    assert(mcpd->mcpd_num < NET_MYRICOM_MCP3_GATHER_MAX - 1);

    for (i = 0; i <= mcpd->mcpd_num; i++) {
	assert((mcpd->mcpd_phys[i] & (NET_MYRICOM_MCP3_ALIGNMENT - 1)) == 0);
	net_myricom_mcp3_32_set(&sendi->si_gathers[i].ga_pointer,
				mcpd->mcpd_phys[i]);
	assert((mcpd->mcpd_size[i] & (NET_MYRICOM_MCP3_ALIGNMENT - 1)) == 0);
	net_myricom_mcp3_32_set(&sendi->si_gathers[i].ga_length,
				mcpd->mcpd_size[i]);
    }

    /*
     * Update sendq pointer
     */
    net_myricom_mcp3_sendq_put(&chan->ch_send);
    return (NET_NICOUT_COMPLETED);
}

/*
 * net_myricom_mcp3_copyin
 *
 * Purpose  : Try to copy a packet from the adapter memory
 * Returns  : NET_NICIN_ERROR     if an error occured
 *            NET_NICIN_COMPLETED if a packet can be copied in on place
 *	      NET_NICIN_SUCCESS   if a packet can be copied in
 *            NET_NICIN_EMPTY     if no pending packet
 *
 * IN_args  : netp   ==> Network generic interface structure
 *
 * OUT_args : module ==> Receiver module identification
 *            addr   ==> RAM address where to get packet (if return == TRUE)
 *            len    ==> Packet length (len == 0 ==> end of packet)
 *            type   ==> Input destination address type
 */
net_nicin_t
net_myricom_mcp3_copyin(
    net_device_t netp,
    int *module,
    vm_offset_t *addr,
    vm_size_t *len,
    unsigned int *type)
{
    net_myricom_mcp3_t myri = (net_myricom_mcp3_t)netp->net_nic;
    net_myricom_mcp3_channel_t *chan;
    unsigned int i;
    unsigned int n;
    unsigned int index;
    net_myricom_mcp3_recvi_t recvi;

    for (i = 0; i < myri->myri_numchan; i++) {
	if (myri->myri_module[i] == NET_MODULE_UNKNOWN)
	    continue;
	NET_MYRICOM_MCP3_SETCHANNEL(netp, myri, chan, i);
	if (NET_MYRICOM_HPI_WAS_RESET(chan))
	    return (NET_NICIN_ERROR);
	if (!net_myricom_mcp3_recvq_empty(&chan->ch_recvack)) {
	    net_myricom_mcp3_recvq_get(&chan->ch_recvack, &recvi);
	    break;
	}
    }

    if (i == myri->myri_numchan)
	return (NET_NICIN_EMPTY);

    /*
     * Got a packet
     */
    *module = NET_MODULE_DEVICE;
    *type = NET_PACKET_PERFECT;
    *len = 0;

    index = myri->myri_index[i];
    assert(myri->myri_context[index] != (vm_offset_t)0);
    assert(myri->myri_context[index] ==
	   net_myricom_mcp3_32_get(&recvi.ri_context));
    *addr = myri->myri_context[index];
    myri->myri_context[index] = (vm_offset_t)0;
    if (index == NET_MYRICOM_MCP3_RECVQ_MAX)
	myri->myri_index[i] = 0;
    else
	myri->myri_index[i]++;

    n = net_myricom_mcp3_32_get(&recvi.ri_num);
    for (i = 0; i < n; i++)
	*len += net_myricom_mcp3_32_get(&recvi.ri_scatters[i].ga_length);
    assert(*len <= IF_MYRINET_EEP_MTU + sizeof (struct if_myrinet_eep));

    return (NET_NICIN_COMPLETED);
}

/*
 * net_myricom_mcp3_start
 *
 * Purpose  : Start sending last copied out packet to the network
 * Returns  : -1 if error occured
 *             0 if NIC already started
 *             1 if NIC was started
 *
 * IN_args  : netp ==> network generic interface structure
 *            netd ==> Packet to send
 */
/*ARGSUSED*/
int
net_myricom_mcp3_start(
    net_device_t netp,
    net_done_t netd)
{
    net_myricom_mcp3_t myri = (net_myricom_mcp3_t)netp->net_nic;

    NET_MYRICOM_HPI_WAKE_SEND(netp, myri);
    return (1);
}

/*
 * net_myricom_mcp3_timeout
 *
 * Purpose : manage timeout for the NIC
 * Returns : -1 if adapter should be reset
 *            0 otherwise
 *
 * IN_args  : netp ==> Network generic interface structure
 */
int
net_myricom_mcp3_timeout(
    net_device_t netp)
{
    net_myricom_mcp3_t myri = (net_myricom_mcp3_t)netp->net_nic;

    return (0);
}

/*
 * net_myricom_mcp3_intr
 *
 * Purpose  : Handle interrupts
 * Returns  : NET_NICINTR_ERROR   if adapter should be reset
 *            NET_NICINTR_NOTHING if no input/output event occured
 *            NET_NICINTR_EVENT   if any input/output event occured
 *
 * IN_args  : netp ==> network generic interface structure
 */
net_nicintr_t
net_myricom_mcp3_intr(
    net_device_t netp)
{
    net_myricom_mcp3_t myri = (net_myricom_mcp3_t)netp->net_nic;
    net_nicintr_t ret = NET_NICINTR_NOTHING;
    unsigned int i;
    unsigned int j;
    unsigned int index;
    net_myricom_mcp3_channel_t *chan;
    volatile unsigned int *reg;
    unsigned int len;

    /*
     * Look for end of transmission
     */
    index = 0;
    for (i = 0; i < myri->myri_numchan; i++) {
	if (myri->myri_module[i] == NET_MODULE_UNKNOWN)
	    index += NET_MYRICOM_MCP3_SENDQ_MAX;
	else {
	    NET_MYRICOM_MCP3_SETCHANNEL(netp, myri, chan, i);
	    if (NET_MYRICOM_HPI_WAS_RESET(chan))
		return (NET_NICINTR_ERROR);
	    for (j = 0; j < NET_MYRICOM_MCP3_SENDQ_MAX; j++) {
		if (myri->myri_netd[index] != (net_done_t)0 &&
		    net_myricom_mcp3_sendq_free(&chan->ch_send, j)) {
		    if (ret == NET_NICINTR_NOTHING)
			ret = NET_NICINTR_EVENT;
		    mpenqueue_tail(&netp->net_done,
				   &(myri->myri_netd[index])->netd_queue);
		    myri->myri_netd[index] = (net_done_t)0;
		}
		index++;
	    }
	}
    }
    if (ret != NET_NICINTR_NOTHING)
	return (ret);

    /*
     * Look for pending received packets
     */
    if (!net_myricom_mcp3_recvq_empty(&chan->ch_recvack))
	return (NET_NICINTR_EVENT);

    /*
     * No event to report ==> Acknowledge IT
     */
    if (myri->myri_flags & NET_MYRICOM_MCP3_FLAGS_MAPREG)
	reg = &myri->myri_mapreg[NET_MYRICOM_REG_ISR];
    else if (myri->myri_reg == NET_MYRICOM_REG_ISR)
	reg = myri->myri_mapreg;
    else {
	len = 1;
	reg = (volatile unsigned int *)
	    (*myri->myri_regmap)(netp, NET_MYRICOM_REG_ISR, &len);
	assert (len >= 1);
	myri->myri_mapreg = reg;
	myri->myri_reg = NET_MYRICOM_REG_ISR;
    }
    *reg = NET_MYRICOM_ISR_HOST;

    return (NET_NICINTR_NOTHING);
}

/*
 * net_myricom_mcp3_multicast
 *
 * Purpose : manage multicast entries at NIC level
 * Returns : Nothing
 *
 * Args_in : netp ==> Network generic interface structure
 *           mp   ==> == NULL: recompute multicast filtering
 *                    != NULL: multicast entry to add
 */
void
net_myricom_mcp3_multicast(
    net_device_t netp,
    net_multicast_t *mp)
{
    net_myricom_mcp3_t myri = (net_myricom_mcp3_t)netp->net_nic;
}
