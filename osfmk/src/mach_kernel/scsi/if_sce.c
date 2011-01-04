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

#include <sce.h>
#if	NSCE > 0

#include <kern/spl.h>
#include <ddb/tr.h>
#include <device/device_types.h>
#include <device/io_req.h>
#include <device/if_hdr.h>
#include <device/if_ether.h>
#include <device/net_status.h>
#include <device/net_io.h>
#include <device/ds_routines.h>
#include <kern/misc_protos.h>
#include <chips/busses.h>

#include <mach/kkt_types.h>
#include <scsi/scsit.h>
#include <scsi/if_sce.h>

typedef struct sce_softc {
	struct		ifnet sce_if;
	boolean_t	inuse;
	node_name	node;
	scsit_handle_t	recv_handle;
} * sce_softc_t;

struct sce_softc	sce_softc[NSCE];

#define NULL_SSC (sce_softc_t)0

char sce_fake_addr[6];
unsigned int scemtu = 0;
#define SCE_MTU scemtu

void		sceinit(void);
sce_softc_t	scegetssc(int unit);
void		scestart(int unit);
int		sceprobe(int			port,
			struct bus_device	*dev);
void		sceattach(struct bus_device	*dev);
void scerecv(sce_softc_t		ssc,
	     ipc_kmsg_t			new_kmsg,
	     vm_size_t			size);
void sce_recv_complete(
		       scsit_handle_t		handle,
		       void *			opaque,
		       char *			buffer,
		       unsigned int		count,
		       scsit_return_t		sr);
void sce_target_select(
		       node_name		node,
		       int			lun,
		       unsigned int		size);
void sce_send_complete(
		       scsit_handle_t		handle,
		       void *			opaque,
		       char *			buffer,
		       unsigned int		count,
		       scsit_return_t		sr);
boolean_t sce_prime(sce_softc_t ssc, ipc_kmsg_t old_kmsg);

unsigned int sce_lun;
static caddr_t sce_std[NSCE] = {0};
static struct bus_device *sce_info[NSCE];
struct	bus_driver	scedriver = 
	{(probe_t)sceprobe, 0, sceattach, 0, sce_std, "sce",
	     sce_info, 0, 0, 0};

int
sceprobe(int port,
	struct bus_device *dev)
{
	static boolean_t initialized = FALSE;
	if (!initialized) {
		initialized = TRUE;
		return (1);
	}
	return (0);
}

void
sceattach(struct bus_device	*dev)
{
	sceinit();
}

sce_softc_t
scegetssc(int unit)
{
	int i;
	for(i=0;i<NSCE;i++) {
		sce_softc_t ssc = &sce_softc[i];
		if (ssc->inuse && ssc->node == unit)
		    return ssc;
	}
	return NULL_SSC;
}

void
sceinit(void)
{
	node_name node;
	int i;
	scsit_return_t sr;

	for(i=0;i<NSCE;i++)
	    sce_softc[i].inuse = FALSE;
	scsit_init(64);
	sr = scsit_lun_allocate(sce_recv_complete,
				sce_target_select,
				sce_send_complete,
				&sce_lun);
	assert(sr == SCSIT_SUCCESS);
	SCE_MTU = ((NET_RCV_MAX - sizeof(struct ether_header))|1) - 1;
/*	SCE_MTU = ETHERMTU;*/
	node = scsit_node_self();
	printf(" mtu = 0x%x node = 0x%x", SCE_MTU, node);
	for(i=0;i<4;i++) {
		sce_fake_addr[5-i]= node % 256;
		node /= 256;
	}
	sce_fake_addr[0]=0x00;
	sce_fake_addr[1]=0x78;
	printf(" ethernet id [%x:%x:%x:%x:%x:%x]",
		sce_fake_addr[0], sce_fake_addr[1], sce_fake_addr[2],
		sce_fake_addr[3], sce_fake_addr[4], sce_fake_addr[5]);
}

boolean_t
sce_prime(sce_softc_t ssc, ipc_kmsg_t old_kmsg)
{
	ipc_kmsg_t new_kmsg;
	sce_softc_t *pssc;
	register struct ether_header *ehp;
	register struct packet_header *pkt;
	scsit_return_t sr;

	new_kmsg = net_kmsg_get();
	if (new_kmsg == IKM_NULL) {
		assert(old_kmsg != IKM_NULL);
		new_kmsg = old_kmsg;
	}
	pssc = (sce_softc_t *)&net_kmsg(new_kmsg)->header[0];
	*pssc = ssc;
	ehp = ((struct ether_header *) (&net_kmsg(new_kmsg)->packet[0])) - 1;
	pkt = ((struct packet_header *)ehp) + 1;

	sr = scsit_receive(ssc->recv_handle,
			   ssc->node,
			   sce_lun,
			   (void *)new_kmsg,
			   (char *)pkt,
			   NET_RCV_MAX,
			   0);
	assert(sr == SCSIT_SUCCESS);
	return(new_kmsg != old_kmsg);
}

io_return_t
sceopen(
       dev_t			dev,
       dev_mode_t		flag,
       io_req_t			ior)
{
	struct	ifnet	*ifp;
	int i, unit;
	sce_softc_t ssc;
	scsit_return_t sr;
	TR_DECL("sceopen");

	tr4("enter: dev = 0x%x flag = 0x%x ior = 0x%x", dev, flag, ior);
	unit = minor(dev);

	if (scegetssc(unit) != NULL_SSC) {
		tr1("exit: D_ALREADY_OPEN");
		return (D_ALREADY_OPEN);
	}

	for(i=0;i<NSCE;i++) {
		ssc = &sce_softc[i];
		if (!ssc->inuse)
		    break;
	}
	if (ssc->inuse) {
		tr1("exit: D_WOULD_BLOCK");
		return (D_WOULD_BLOCK);
	}

	ssc->inuse = TRUE;
	ssc->node = unit;
	sr = scsit_handle_alloc(&ssc->recv_handle);
	assert(sr == SCSIT_SUCCESS);
	sr = scsit_handle_mismatch(ssc->recv_handle);
	assert(sr == SCSIT_SUCCESS);

	ifp = &(ssc->sce_if);
	ifp->if_unit = unit;
	ifp->if_mtu = SCE_MTU;
	ifp->if_flags = IFF_UP | IFF_RUNNING | IFF_BROADCAST;
	ifp->if_header_size = sizeof(struct ether_header);
	ifp->if_header_format = HDR_ETHERNET;
	ifp->if_address_size = 6;
	ifp->if_address = sce_fake_addr;
	if_init_queues(ifp);

	(void)sce_prime(ssc, IKM_NULL);
	tr1("exit: D_SUCCESS");
	return (D_SUCCESS);
}

void
scestart(int unit)
{
	sce_softc_t ssc = scegetssc(unit);
	scsit_return_t sr;
	struct ifnet *ifp;
	io_req_t ior;
	TR_DECL("scestart");

	tr2("enter: unit = 0x%x", unit);

	assert(ssc != NULL_SSC);

	ifp = &ssc->sce_if;
	IF_DEQUEUE(&ifp->if_snd, ior);
      dequeued:
	while(ior) {
		struct ether_header *ehp = (struct ether_header *)ior->io_data;
		scsit_handle_t handle;

		bcopy((const char *)sce_fake_addr,(char *)&ehp->ether_shost,6);

		sr = scsit_handle_alloc(&handle);
		assert(sr == SCSIT_SUCCESS);

		tr1("sending");
		sr = scsit_send(handle,
				ssc->node,
				sce_lun,
				(void *)ior,
				(char *)ehp,
				ior->io_count,
				FALSE);
		assert(sr == SCSIT_SUCCESS);
		IF_DEQUEUE(&ifp->if_snd, ior);
	}
	tr1("exit");
}

io_return_t
sceoutput(
	 dev_t			dev,
	 io_req_t		ior)
{
	sce_softc_t ssc = scegetssc(minor(dev));
	if (ssc == NULL_SSC)
	    return (D_NO_SUCH_DEVICE);
	return (net_write(&ssc->sce_if, scestart, ior));
}

void
sce_send_complete(
		  scsit_handle_t	handle,
		  void *		opaque,
		  char *		buffer,
		  unsigned int		count,
		  scsit_return_t	sr)
{
	io_req_t ior = (io_req_t)opaque;
	TR_DECL("sce_send_complete");

	tr1("enter");
	sr = scsit_handle_free(handle);
	assert(sr == SCSIT_SUCCESS);
	iodone(ior);
}

void
sce_target_select(
		  node_name		node,
		  int			lun,
		  unsigned int		size)
{
	assert(FALSE);
}

void
sce_recv_complete(
		  scsit_handle_t	handle,
		  void *		opaque,
		  char *		buffer,
		  unsigned int		count,
		  scsit_return_t	sr)
{
	ipc_kmsg_t new_kmsg = (ipc_kmsg_t)opaque;
	sce_softc_t ssc = *((sce_softc_t *)(&net_kmsg(new_kmsg)->header[0]));
	TR_DECL("sce_recv_complete");

	tr5("enter 0x%x 0x%x 0x%x 0x%x", handle, opaque, buffer, count);
	if (sce_prime(ssc, new_kmsg))
	    scerecv(ssc, new_kmsg, count);
}

void
scerecv(sce_softc_t		ssc,
       ipc_kmsg_t		new_kmsg,
       vm_size_t		size)
{
	register struct ether_header *ehp, *ehp2;
	register struct packet_header *psce, *psce2;
	TR_DECL("scerecv");

	tr2("size = 0x%x", size);
	ehp = (struct ether_header *)(&net_kmsg(new_kmsg)->header[0]);
	psce = (struct packet_header *)(&net_kmsg(new_kmsg)->packet[0]);
	ehp2 = ((struct ether_header *)(&net_kmsg(new_kmsg)->packet[0])) - 1;
	psce2 = ((struct packet_header *)ehp2) + 1;

	bcopy((const char *)psce2, (char *)ehp, sizeof(struct ether_header));
		
	psce->type = ehp->ether_type;
	psce->length = size - sizeof(struct ether_header) + 
	    sizeof(struct packet_header);

	net_packet(&ssc->sce_if, new_kmsg, psce->length,
		   ethernet_priority(new_kmsg), (io_req_t)0);
}

io_return_t
scegetstat(
	  dev_t			dev,
	  dev_flavor_t		flavor,
	  dev_status_t		data,
	  natural_t		*count)
{
	sce_softc_t ssc = scegetssc(minor(dev));
	if (ssc == NULL_SSC)
	    return (D_NO_SUCH_DEVICE);
	return (net_getstat(&ssc->sce_if, flavor, data, count));
}

io_return_t
scesetstat(
	  dev_t			dev,
	  dev_flavor_t		flavor,
	  dev_status_t		data,
	  natural_t		count)
{
	sce_softc_t ssc = scegetssc(minor(dev));
	if (ssc == NULL_SSC)
	    return (D_NO_SUCH_DEVICE);
	return (D_INVALID_OPERATION);
}

io_return_t
scesetinput(
	   dev_t		dev,
	   ipc_port_t		port,
	   int			pri,
	   filter_t		filter[],
	   unsigned int		fcount,
	   device_t		device)
{
	sce_softc_t ssc = scegetssc(minor(dev));
	if (ssc == NULL_SSC)
	    return (D_NO_SUCH_DEVICE);
	return (net_set_filter(&ssc->sce_if, port, pri, filter, fcount, device));
}
#endif	/* NSCE > 0 */
