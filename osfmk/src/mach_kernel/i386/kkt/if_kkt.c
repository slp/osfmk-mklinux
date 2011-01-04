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
#include <kkt.h>
#if	NKKT > 0

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
#include <mach/kkt_request.h>
#include <chips/busses.h>

#include <i386/kkt/if_kkt.h>

typedef struct kt_softc {
	struct		ifnet kt_if;
	boolean_t	inuse;
	node_name	node;
} * kt_softc_t;

struct kt_softc	kt_softc[NKKT];

#define NULL_KSC (kt_softc_t)0

char kt_fake_addr[6];
struct kkt_properties kt_prop;
unsigned int ktmtu = 0;
#define KT_MTU ktmtu

extern request_block_t	dipc_get_kkt_req(boolean_t	wait);
extern void		dipc_free_kkt_req(request_block_t	req);

void		ktinit(void);
kt_softc_t	ktgetksc(int unit);
void		ktstart(int unit);
vm_offset_t 	ktalloc(void			*opaque,
			vm_size_t		size);
void		ktfree(void			*opaque,
		       vm_offset_t		data,
		       vm_size_t		size);
void		ktdeliver(channel_t		chan,
			  handle_t		handle,
			  endpoint_t		*ep,
			  vm_size_t		size,
			  boolean_t		inlinep,
			  boolean_t		thread_context);
int		ktprobe(int			port,
			struct bus_device	*dev);
void		ktattach(struct bus_device	*dev);
void		ktsenddone(kkt_return_t		kktr,
			   handle_t		handle,
			   request_block_t	req,
			   boolean_t		thread_context);
void		ktrecvdone(kkt_return_t		kktr,
			   handle_t		handle,
			   request_block_t	req,
			   boolean_t		thread_context);
void		ktrecv(kt_softc_t		ksc,
		       ipc_kmsg_t		new_kmsg,
		       vm_size_t		size);


static caddr_t kt_std[NKKT] = {0};
static struct bus_device *kt_info[NKKT];
struct	bus_driver	ktdriver = 
	{(probe_t)ktprobe, 0, ktattach, 0, kt_std, "kkt",
	     kt_info, 0, 0, 0};

int
ktprobe(int port,
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
ktattach(struct bus_device	*dev)
{
	ktinit();
}

vm_offset_t
ktalloc(void		*opaque,
	vm_size_t	size)
{
	return (kalloc(size));
}

void
ktfree(void		*opaque,
       vm_offset_t	data,
       vm_size_t	size)
{
	kfree(data, size);
}

kt_softc_t
ktgetksc(int unit)
{
	int i;
	for(i=0;i<NKKT;i++) {
		kt_softc_t ksc = &kt_softc[i];
		if (ksc->inuse && ksc->node == unit)
		    return ksc;
	}
	return NULL_KSC;
}

void
ktinit(void)
{
	node_name node;
	int i;
	kkt_return_t kktr;

	for(i=0;i<NKKT;i++)
	    kt_softc[i].inuse = FALSE;
	kkt_bootstrap_init();
	KT_MTU = ((NET_RCV_MAX - sizeof(struct ether_header))|1) - 1;
/*	KT_MTU = ETHERMTU;*/
	kktr = KKT_CHANNEL_INIT(CHAN_DEVICE_KT,
				NKKT*20,
				NKKT*60,
				(kkt_channel_deliver_t)ktdeliver,
				(kkt_malloc_t)ktalloc,
				(kkt_free_t)ktfree);
	assert(kktr == KKT_SUCCESS);
	node = KKT_NODE_SELF();
	printf(" mtu = 0x%x node = 0x%x", KT_MTU, node);
	for(i=0;i<4;i++) {
		kt_fake_addr[5-i]= node % 256;
		node /= 256;
	}
	kt_fake_addr[0]=0x00;
	kt_fake_addr[1]=0x77;
	printf(" ethernet id [%x:%x:%x:%x:%x:%x]",
		kt_fake_addr[0], kt_fake_addr[1], kt_fake_addr[2],
		kt_fake_addr[3], kt_fake_addr[4], kt_fake_addr[5]);
}

io_return_t
ktopen(
       dev_t			dev,
       dev_mode_t		flag,
       io_req_t			ior)
{
	struct	ifnet	*ifp;
	int i, unit;
	kt_softc_t ksc;
	kkt_return_t kktr;
	TR_DECL("ktopen");

	tr4("enter: dev = 0x%x flag = 0x%x ior = 0x%x", dev, flag, ior);
	unit = minor(dev);

	if (ktgetksc(unit) != NULL_KSC) {
		tr1("exit: D_ALREADY_OPEN");
		return (D_ALREADY_OPEN);
	}

	for(i=0;i<NKKT;i++) {
		ksc = &kt_softc[i];
		if (!ksc->inuse)
		    break;
	}
	if (ksc->inuse) {
		tr1("exit: D_WOULD_BLOCK");
		return (D_WOULD_BLOCK);
	}

	ksc->inuse = TRUE;
	ksc->node = unit;

	ifp = &(ksc->kt_if);
	ifp->if_unit = unit;
	ifp->if_mtu = KT_MTU;
	ifp->if_flags = IFF_UP | IFF_RUNNING | IFF_BROADCAST;
	ifp->if_header_size = sizeof(struct ether_header);
	ifp->if_header_format = HDR_ETHERNET;
	ifp->if_address_size = 6;
	ifp->if_address = kt_fake_addr;
	if_init_queues(ifp);

	tr1("exit: D_SUCCESS");
	return (D_SUCCESS);
}

void
ktstart(int unit)
{
	kt_softc_t ksc = ktgetksc(unit);
	kkt_return_t kktr;
	struct ifnet *ifp;
	io_req_t ior;
	TR_DECL("ktstart");

	tr2("enter: unit = 0x%x", unit);

	assert(ksc != NULL_KSC);

	ifp = &ksc->kt_if;
	IF_DEQUEUE(&ifp->if_snd, ior);
      dequeued:
	while(ior) {
		struct ether_header *ehp = (struct ether_header *)ior->io_data;
		handle_t handle;
		kern_return_t kr[2];
		request_block_t req;

		kktr = KKT_HANDLE_ALLOC(CHAN_DEVICE_KT,
					&handle,
					(kkt_error_t)0,
					FALSE,
					FALSE);

		if (kktr != KKT_SUCCESS)
		    break;

		req = dipc_get_kkt_req(FALSE);
		assert(req != NULL_REQUEST);

		req->request_type = REQUEST_SEND;
		req->data.address = (vm_offset_t)ior->io_data;
		req->data_length = ior->io_count;
		req->callback = ktsenddone;
		req->args[0] = (unsigned int)ior;

		bcopy((const char *)kt_fake_addr, (char *)&ehp->ether_shost,6);

		tr1("sending");
		kktr = KKT_SEND_CONNECT(handle,
					unit,
					0,
					req,
					FALSE,
					kr);
		if (kktr == KKT_DATA_SENT || kktr == KKT_ABORT) {
			kktr = KKT_HANDLE_FREE(handle);
			assert(kktr == KKT_SUCCESS);
			dipc_free_kkt_req(req);
			iodone(ior);
		} else
		    assert(kktr == KKT_SUCCESS);
		IF_DEQUEUE(&ifp->if_snd, ior);
	}
	tr1("exit");
}

io_return_t
ktoutput(
	 dev_t			dev,
	 io_req_t		ior)
{
	kt_softc_t ksc = ktgetksc(minor(dev));
	if (ksc == NULL_KSC)
	    return (D_NO_SUCH_DEVICE);
	return (net_write(&ksc->kt_if, ktstart, ior));
}

void
ktsenddone(kkt_return_t		kktr,
	   handle_t		handle,
	   request_block_t	req,
	   boolean_t		thread_context)
{
	io_req_t ior = (io_req_t)req->args[0];
	TR_DECL("ktsenddone");

	tr1("enter");
	kktr = KKT_HANDLE_FREE(handle);
	assert(kktr == KKT_SUCCESS);
	dipc_free_kkt_req(req);
	iodone(ior);
}

void
ktrecvdone(kkt_return_t		kktr,
	   handle_t		handle,
	   request_block_t	req,
	   boolean_t		thread_context)
{
	ipc_kmsg_t new_kmsg = (ipc_kmsg_t)req->args[0];
	kt_softc_t ksc = (kt_softc_t)req->args[1];
	vm_size_t size = req->data_length;
	TR_DECL("ktrecvdone");

	tr1("enter");
	kktr = KKT_HANDLE_FREE(handle);
	assert(kktr == KKT_SUCCESS);
	dipc_free_kkt_req(req);
	ktrecv(ksc, new_kmsg, size);
}

void
ktrecv(kt_softc_t		ksc,
       ipc_kmsg_t		new_kmsg,
       vm_size_t		size)
{
	register struct ether_header *ehp, *ehp2;
	register struct packet_header *pkt, *pkt2;
	TR_DECL("ktrecv");

	tr2("size = 0x%x", size);
	ehp = (struct ether_header *)(&net_kmsg(new_kmsg)->header[0]);
	pkt = (struct packet_header *)(&net_kmsg(new_kmsg)->packet[0]);
	ehp2 = ((struct ether_header *)(&net_kmsg(new_kmsg)->packet[0]));
	ehp2--;
	pkt2 = (struct packet_header *)ehp2;
	pkt2++;

	bcopy((const char *)pkt2, (char *)ehp, sizeof(struct ether_header));
		
	pkt->type = ehp->ether_type;
	pkt->length = size - sizeof(struct ether_header) + 
	    sizeof(struct packet_header);

	net_packet(&ksc->kt_if, new_kmsg, pkt->length,
		   ethernet_priority(new_kmsg), (io_req_t)0);
}

void
ktdeliver(channel_t		chan,
	  handle_t		handle,
	  endpoint_t		*ep,
	  vm_size_t		size,
	  boolean_t		inlinep,
	  boolean_t		thread_context)
{
	struct handle_info hi;
	kkt_return_t kktr;
	kt_softc_t ksc;
	ipc_kmsg_t new_kmsg;
	char * buf;
	register struct ether_header *ehp;
	register struct packet_header *pkt;
	TR_DECL("ktdeliver");

	tr1("enter");
	kktr = KKT_HANDLE_INFO(handle, &hi);
	assert(kktr == KKT_SUCCESS);

	ksc = ktgetksc(hi.node);

	if (ksc != NULL_KSC) {
		new_kmsg = net_kmsg_get();
		if (new_kmsg != IKM_NULL) {
			ehp = ((struct ether_header *)
				(&net_kmsg(new_kmsg)->packet[0]));
			ehp--;
			pkt = (struct packet_header *)ehp;
			pkt++;

			if (inlinep) {
				tr1("inline");
				kktr = KKT_COPYOUT_RECV_BUFFER(handle,
							       (char *)pkt);
				assert(kktr == KKT_SUCCESS);
				kktr = KKT_CONNECT_REPLY(handle, 0, 0);
				assert(kktr == KKT_SUCCESS);
				kktr = KKT_HANDLE_FREE(handle);
				assert(kktr == KKT_SUCCESS);
				ktrecv(ksc, new_kmsg, size);
			} else {
				request_block_t req = dipc_get_kkt_req(FALSE);
				assert(req != NULL_REQUEST);

				tr1("requesting");

				kktr = KKT_CONNECT_REPLY(handle, 0, 0);
				assert(kktr == KKT_SUCCESS);

				req->request_type = REQUEST_RECV;
				req->data.address = (vm_offset_t)pkt;
				req->data_length = size;
				req->callback = ktrecvdone;
				req->args[0] = (unsigned int)new_kmsg;
				req->args[1] = (unsigned int)ksc;

				kktr = KKT_REQUEST(handle, req);
				assert(kktr == KKT_SUCCESS);
			}
			return;
		}
		tr1("no kmsg");
	} else {
		tr1("no ksc");
	}
	kktr = KKT_HANDLE_ABORT(handle, 0);
	assert(kktr == KKT_SUCCESS);
	kktr = KKT_HANDLE_FREE(handle);
	assert(kktr == KKT_SUCCESS);
}

io_return_t
ktgetstat(
	  dev_t			dev,
	  dev_flavor_t		flavor,
	  dev_status_t		data,
	  natural_t		*count)
{
	kt_softc_t ksc = ktgetksc(minor(dev));
	if (ksc == NULL_KSC)
	    return (D_NO_SUCH_DEVICE);
	return (net_getstat(&ksc->kt_if, flavor, data, count));
}

io_return_t
ktsetstat(
	  dev_t			dev,
	  dev_flavor_t		flavor,
	  dev_status_t		data,
	  natural_t		count)
{
	kt_softc_t ksc = ktgetksc(minor(dev));
	if (ksc == NULL_KSC)
	    return (D_NO_SUCH_DEVICE);
	return (D_INVALID_OPERATION);
}

io_return_t
ktsetinput(
	   dev_t		dev,
	   ipc_port_t		port,
	   int			pri,
	   filter_t		filter[],
	   unsigned int		fcount,
	   device_t		device)
{
	kt_softc_t ksc = ktgetksc(minor(dev));
	if (ksc == NULL_KSC)
	    return (D_NO_SUCH_DEVICE);
	return (net_set_filter(&ksc->kt_if, port, pri, filter, fcount, device));
}
#endif	/* NKKT > 0 */
