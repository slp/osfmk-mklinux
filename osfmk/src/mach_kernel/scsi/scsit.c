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

#include <mach_assert.h>
#include <mach_kdb.h>

#include <norma_scsi.h>
#if	NORMA_SCSI

#include <scsi.h>
#include <kernel_test.h>

#include <mach/std_types.h>
#include <scsi/scsi_defs.h>

#include <device/io_req.h>
#include <kern/zalloc.h>
#include <device/disk_status.h>
#include <scsi/scsit.h>
#include <vm/vm_kern.h>
#include <ddb/tr.h>
#include <kern/misc_protos.h>
#include <dipc/dipc_timer.h>

#if	KERNEL_TEST

/*
 *	The following are used to serialize and
 *	synchronize the kernel tests. 
 */

extern	boolean_t	kernel_test_thread_blocked;
extern	boolean_t	unit_test_thread_sync_done;
decl_simple_lock_data(extern, kernel_test_lock)
extern void		kernel_test_thread(void); 

#endif	/* KERNEL_TEST */

scsit_recv_complete_t scsit_recv_complete[SCSI_NLUNS];
scsit_target_select_t scsit_target_select[SCSI_NLUNS];
scsit_send_complete_t scsit_send_complete[SCSI_NLUNS];

#define HOTWIRE 1
zone_t scsit_handle_zone;
target_info_t *scsit_tgt_free_list = (target_info_t *)0;
decl_simple_lock_data(,scsit_tgt_lock_data)

int _scsit_node_self = -1;

#define scsit_tgt_lock() \
    MACRO_BEGIN \
    s = splbio(); \
    simple_lock(&scsit_tgt_lock_data); \
    MACRO_END
#define scsit_tgt_unlock() \
    MACRO_BEGIN \
    simple_unlock(&scsit_tgt_lock_data); \
    splx(s); \
    MACRO_END

scsi_softc_t *norma_scsi_softc = (scsi_softc_t *)0;

boolean_t scsit_initialized = FALSE;

io_req_t scsit_self_ior[SCSI_NLUNS];
decl_simple_lock_data(,scsit_self_lock_data)
#define scsit_self_lock() \
	MACRO_BEGIN \
	s = splbio(); \
	simple_lock(&scsit_self_lock_data); \
	MACRO_END
#define scsit_self_unlock() \
	MACRO_BEGIN \
	simple_unlock(&scsit_self_lock_data); \
	splx(s); \
	MACRO_END

#if	HOTWIRE
#define SCSI_SEND(tgtp,iorp) \
do { \
	 scsi_softc_t	*sc = scsi_softc[(unsigned char)(tgtp)->masterno]; \
	 (tgtp)->ior = (iorp); \
	 (tgtp)->cur_cmd = SCSI_CMD_SEND; \
	 (*sc->go)((tgtp), sizeof(scsi_cmd_write_t), \
		   ((iorp)->io_op&IO_PASSIVE)?(iorp)->io_count:0, FALSE); \
} while (0)
#else	/* HOTWIRE */
#define SCSI_SEND(x,y) scsi_send(x,y)
#endif	/* HOTWIRE */

#if	SCSIT_STATS
#define sstat_decl(variable)	unsigned int scsits_##variable = 0;
#define sstat_inc(variable)	scsits_##variable++
#define sstat_add(variable, amount)	scsits_##variable += amount
#define sstat_print(variable)	iprintf("%s = %d\n",#variable,scsits_##variable)
#else /* SCSIT_STATS */
#define sstat_decl(x)
#define sstat_inc(x)
#define sstat_add(x,y)
#define sstat_print(x)
#endif /* SCSIT_STATS */

sstat_decl(handle_alloc)
sstat_decl(handle_free)
sstat_decl(target)
sstat_decl(target_mismatch)
sstat_decl(initiator_mismatch)
sstat_decl(send)
sstat_decl(receive)

boolean_t	scsit_target_iodone(io_req_t ior);
boolean_t	scsit_initiator_iodone(io_req_t	ior);
boolean_t	scsit_remote_iodone(io_req_t ior);
boolean_t	scsit_target(target_info_t *self);

boolean_t	scsit_device = FALSE;

#if	SCSIT_STATS
void scsit_stats(void)
{
	iprintf("SCSIT stats:\n");
	indent +=2;
	sstat_print(handle_alloc);
	sstat_print(handle_free);
	sstat_print(target);
	sstat_print(target_mismatch);
	sstat_print(initiator_mismatch);
	sstat_print(send);
	sstat_print(receive);
	indent-=2;
}
#endif	/* SCSIT_STATS */

scsit_return_t
scsit_lun_allocate(
		   scsit_recv_complete_t	recv_complete,
		   scsit_target_select_t	target_select,
		   scsit_send_complete_t	send_complete,
		   unsigned int			*lun)
{
	int i;

	for(i=0;i<SCSI_NLUNS;i++)
	    if (scsit_recv_complete[i] == (scsit_recv_complete_t)0)
		break;
	if (i == SCSI_NLUNS)
	    return SCSIT_RESOURCE_SHORTAGE;

	scsit_recv_complete[i] = recv_complete;
	scsit_target_select[i] = target_select;
	scsit_send_complete[i] = send_complete;

	*lun = i;
	return SCSIT_SUCCESS;
}

void
scsit_init(
	   unsigned int			max_pending)
{
	int handle_unit_size, handle_total_size;
	extern scsi_softc_t	scsi_softc_data[NSCSI];
	int i;

	if (scsit_initialized)
	    return;

	if (norma_scsi_softc == (scsi_softc_t *)0) {
		int i;
		for(i=0;i<NSCSI;i++) {
			if (scsi_softc[i] == &scsi_softc_data[i] &&
			    scsi_softc_data[i].tgt_setup !=
			    (void (*)(target_info_t *))0) {
				norma_scsi_softc = &scsi_softc_data[i];
			}
		}
		if (norma_scsi_softc == (scsi_softc_t *)0) {
			printf("\nNo SCSI Cluster adapters present\n\n");
		} else {
			scsit_device = TRUE;
		}
	}

	scsit_initialized = TRUE;

	if (scsit_device)
	    _scsit_node_self = norma_scsi_softc->initiator_id;
	else
	    _scsit_node_self = 0;

	handle_unit_size = sizeof(struct io_req) + sizeof(struct target_info);
	handle_total_size = (8*max_pending)*handle_unit_size;
	
	scsit_handle_zone = zinit(handle_unit_size,
				  handle_total_size,
				  handle_total_size,
				  "scsit handles");
	zone_change(scsit_handle_zone, Z_COLLECT, FALSE);
	zone_change(scsit_handle_zone, Z_EXPAND, FALSE);
	zone_change(scsit_handle_zone, Z_EXHAUST, TRUE);
	zone_enable_spl(scsit_handle_zone, splbio);

	i = zfill(scsit_handle_zone, 8 * max_pending);
	assert( i >= 8 * max_pending);

	simple_lock_init(&scsit_tgt_lock_data, ETAP_MISC_SCSIT_TGT);

	simple_lock_init(&scsit_self_lock_data, ETAP_MISC_SCSIT_SELF);

	for(i=0;i<SCSI_NLUNS;i++) {
		scsit_recv_complete[i] = (scsit_recv_complete_t)0;
		scsit_target_select[i] = (scsit_target_select_t)0;
		scsit_send_complete[i] = (scsit_send_complete_t)0;
		scsit_self_ior[i] = (io_req_t)0;
	}
}

scsit_return_t
scsit_handle_alloc(scsit_handle_t *handle)
{
	spl_t s;
	register target_info_t *tgt;
	target_info_t *mtgt;
	register scsi_softc_t *sc;
	TR_DECL("scsit_handle_alloc");

	if (scsit_device) {
		mtgt =  norma_scsi_softc->target[0];
		sc = scsi_softc[mtgt->masterno];
	}
	scsit_tgt_lock();
	sstat_inc(handle_alloc);
	tgt = scsit_tgt_free_list;
	if (tgt) {
		scsit_tgt_free_list = (target_info_t *)tgt->ior;
		scsit_tgt_unlock();
		tgt->ior = (io_req_t)(tgt + 1);
		tr3("Alloc old 0x%x 0x%x", tgt, tgt->ior);
	} else {
		scsit_tgt_unlock();
		tgt = (target_info_t *)zget(scsit_handle_zone);
		assert(tgt != (target_info_t *)0);

		if (scsit_device) {
			bcopy((const char *)mtgt, (char *)tgt,
			      sizeof(target_info_t));
			sc->tgt_setup(tgt);
		}

		tgt->ior = (io_req_t)(tgt + 1);
		tgt->ior->io_next = (io_req_t)0;
		tr3("Alloc new 0x%x 0x%x", tgt, tgt->ior);
	}

	*handle = (scsit_handle_t)tgt;
	return (SCSIT_SUCCESS);
}

scsit_return_t
scsit_handle_mismatch(scsit_handle_t handle)
{
	target_info_t *tgt = (target_info_t *)handle;
	tgt->ior->io_op ^= IO_INTERNAL;
	return SCSIT_SUCCESS;
}

scsit_return_t
scsit_handle_free(scsit_handle_t handle)
{
	spl_t s;
	target_info_t *tgt = (target_info_t *)handle;
	TR_DECL("scsit_handle_free");

	scsit_tgt_lock();
	sstat_inc(handle_free);
	assert(tgt->ior == (io_req_t)(tgt + 1));
	tr3("Free 0x%x 0x%x",tgt, tgt->ior);
	tgt->ior = (io_req_t)scsit_tgt_free_list;
	scsit_tgt_free_list = tgt;
	scsit_tgt_unlock();
	return (SCSIT_SUCCESS);
}

boolean_t
scsit_target_iodone(
	     io_req_t			ior)
{
	target_info_t *tgt = (target_info_t *)ior->io_dev_ptr;
	TR_DECL("scsit_target_iodone");
	tr3("enter: 0x%x 0x%x", (unsigned int)ior, (unsigned int)tgt);
	tgt->ior = ior; /* Restore:  stripped by iodone */
	if (ior->io_residual > 0 && !(ior->io_op & IO_INTERNAL)) {
		int sent = ior->io_count - ior->io_residual;

		/*
		 * We have a mismatch in send a receive sizes.  The sender
		 * sent less then we requested.  We need kick off another
		 * request here.
		 */
		sstat_inc(target_mismatch);
		tr2("Residual 0x%x",ior->io_residual);
		if (ior->io_op & IO_SCATTER) {
			io_scatter_t ios = (io_scatter_t)ior->io_data;
			while (1)
			    if (ios->ios_length >= sent) {
				    ios->ios_address += sent;
				    ios->ios_length -= sent;
				    break;
			    } else {
				    sent -= ios->ios_length;
				    ios++;
			    }
			ior->io_data = (io_buf_ptr_t)ios;
		} else
		    ior->io_data += sent;

		ior->io_count = ior->io_residual;
		ior->io_residual = 0;
		SCSI_SEND(tgt, ior );
		tr1("exit");
		return (TRUE);
	}
	if (ior->io_residual < 0)
	    ior->io_residual = 0;
	(*scsit_recv_complete[tgt->lun])((scsit_handle_t)tgt,
					 (void *)ior->io_uaddr, 
					 (char *)ior->io_sgp, 
					 ior->io_total - ior->io_residual,
					 (ior->io_error==KERN_SUCCESS)?
				       SCSIT_SUCCESS:SCSIT_FAILURE);
	tr1("exit");
	return (TRUE);
}

boolean_t
scsit_initiator_iodone(
	     io_req_t			ior)
{
	target_info_t *tgt = (target_info_t *)ior->io_dev_ptr;
	TR_DECL("scsit_initiator_iodone");
	tr3("enter: 0x%x 0x%x", (unsigned int)ior, (unsigned int)tgt);
	tgt->ior = ior; /* Restore:  stripped by iodone */
	if (ior->io_residual > 0 && !(ior->io_op & IO_INTERNAL)) {
		int sent = ior->io_count - ior->io_residual;
		/*
		 * We have a mismatch in send a receive sizes.  The receiver
		 * requested less then we sent.  We need kick off another
		 * request here
		 */
		sstat_inc(initiator_mismatch);
		tr2("Residual 0x%x",ior->io_residual);
		if (ior->io_op & IO_SCATTER) {
			io_scatter_t ios = (io_scatter_t)ior->io_data;
			while (1)
			    if (ios->ios_length >= sent) {
				    ios->ios_address += sent;
				    ios->ios_length -= sent;
				    break;
			    } else {
				    sent -= ios->ios_length;
				    ios++;
			    }
			ior->io_data = (io_buf_ptr_t)ios;
		} else
		    ior->io_data += sent;

		ior->io_op &= ~IO_DONE;
		ior->io_count = ior->io_residual;
		ior->io_residual = 0;
		SCSI_SEND(tgt, ior);
		tr1("exit");
		return (TRUE);
	}
	if (ior->io_residual < 0)
	    ior->io_residual = 0;
	(*scsit_send_complete[tgt->lun])((scsit_handle_t)tgt,
					 (void *)ior->io_uaddr, 
					 (char *)ior->io_sgp,
					 ior->io_total - ior->io_residual,
					 (ior->io_error==KERN_SUCCESS)?
				       SCSIT_SUCCESS:SCSIT_FAILURE);
	tr1("exit");
	return (TRUE);
}

scsit_return_t
scsit_node_setup(
		 node_name		node)
{
	target_info_t *tgt;
	scsi_inquiry_data_t	*inq;
	int i, retry;
	TR_DECL("scsit_node_setup");

	tr2("enter: 0x%x", (unsigned int)node);
	assert(scsit_initialized);

	if (node == scsit_node_self()) {
		tr1("exit");
		return SCSIT_SUCCESS;
	}

	if (!scsit_device) {
		tr1("exit: SCSIT_FAILURE(No Adapter)");
		return SCSIT_FAILURE;
	}

	tgt =  norma_scsi_softc->target[node];
	if (!tgt || 
	    ((tgt->flags & (TGT_ALIVE|TGT_FULLY_PROBED))
	     != (TGT_ALIVE|TGT_FULLY_PROBED)))
	    if (!scsi_probe(norma_scsi_softc, &tgt, node, 0, 0)) {
		    tr1("exit SCSIT_FAILURE");
		    return (SCSIT_FAILURE);
	    }
	if (tgt->dev_ops != &scsi_devsw[SCSI_CPU])
	    return (SCSIT_FAILURE);
	scsi_probe_luns(tgt);
	tr1("exit");
	return (SCSIT_SUCCESS);
	
}

scsit_return_t
scsit_send(
	   scsit_handle_t	handle,
	   node_name		node,
	   int			lun,
	   void			*opaque_handle,
	   char			*buffer,
	   unsigned int		size,
	   boolean_t		sg)
{
	target_info_t *tgt;
	io_req_t ior;
	unsigned int real_size = size;
	spl_t s;
	TR_DECL("scsit_send");

	sstat_add(send, size);
	tr5("enter   : 0x%x 0x%x 0x%x 0x%x",
	    (unsigned int)node, (unsigned int)lun,
	    (unsigned int)opaque_handle, (unsigned int)buffer);
	tr3("enter(2): 0x%x 0x%x", (unsigned int)size, (unsigned int)sg);
	assert(scsit_initialized);

	tgt = (target_info_t *)handle;
	tgt->target_id = node;
	tgt->lun = lun;
	ior = tgt->ior;
	tr3("tgt = 0x%x ior = 0x%x",tgt,ior);
	ior->io_dev_ptr = (char *)tgt;
	ior->io_residual = 0;
	ior->io_error = KERN_SUCCESS;
	ior->io_op = IO_WRITE|IO_LOANED|(ior->io_op & IO_INTERNAL);
	if (sg) ior->io_op |= IO_SCATTER;
	ior->io_data = buffer;
	ior->io_count = size;
	ior->io_done = scsit_initiator_iodone;
	ior->io_sgp = (io_sglist_t)buffer;
	ior->io_total = real_size;
	ior->io_uaddr = (vm_address_t)opaque_handle;
	if (node == scsit_node_self()) {
		scsit_self_lock();
		if (scsit_self_ior[lun] == (io_req_t)0) {
			scsit_self_ior[lun] = ior;
			scsit_self_unlock();
			(*scsit_target_select)(node, lun, ior->io_count);
		} else {
			vm_offset_t wsize;
			io_req_t ior2 = scsit_self_ior[lun];
			scsit_self_ior[lun]=(io_req_t)0;
			scsit_self_unlock();
			assert((ior2->io_op & IO_PASSIVE));
			wsize = (ior->io_count > ior2->io_count) ?
			    ior2->io_count: ior->io_count;
			ior->io_residual = ior2->io_residual =
			    ior->io_count - ior2->io_count;
			if (sg)
			    if (ior2->io_op & IO_SCATTER)
				ios_copy((io_scatter_t)ior->io_data,
					 (io_scatter_t)ior2->io_data,
					 wsize);
			    else
				ios_copy_from((io_scatter_t)ior->io_data,
					      (char *)ior2->io_data,
					      wsize);
			else
			    if (ior2->io_op & IO_SCATTER)
				ios_copy_to((char *)ior->io_data,
					    (io_scatter_t)ior2->io_data,
					    wsize);
			    else
				bcopy((const char *)ior->io_data,
				      (char *)ior2->io_data,
				      wsize);
			(*ior->io_done)(ior);
			(*ior2->io_done)(ior2);
		}
	} else if (scsit_device) {
		SCSI_SEND(tgt, ior);
	} else {
		tr1("exit SCSIT_FAILURE");
		return (SCSIT_FAILURE);
	}
	tr1("exit");
	return (SCSIT_SUCCESS);
}

scsit_return_t
scsit_receive(
	      scsit_handle_t		handle,
	      node_name			node,
	      int			lun,
	      void			*opaque_handle,
	      char			*buffer,
	      unsigned int		size,
	      boolean_t			sg)
{
	target_info_t *tgt;
	io_req_t ior;
	unsigned int real_size = size;
	spl_t s;
	TR_DECL("scsit_receive");

	sstat_add(receive, size);
	tr5("enter   : 0x%x 0x%x 0x%x 0x%x",
	    (unsigned int)node, (unsigned int)lun,
	    (unsigned int)opaque_handle, (unsigned int)buffer);
	tr3("enter(2): 0x%x 0x%x", (unsigned int)size, (unsigned int)sg);
	assert(scsit_initialized);

	tgt = (target_info_t *)handle;
	tgt->target_id = node;
	tgt->lun = lun;
	ior = tgt->ior;
	tr3("tgt = 0x%x ior = 0x%x",tgt,ior);
	ior->io_dev_ptr = (char *)tgt;
	ior->io_residual = 0;
	ior->io_error = KERN_SUCCESS;
	ior->io_op = IO_LOANED|IO_WRITE|IO_PASSIVE|(ior->io_op & IO_INTERNAL);
	if (sg) ior->io_op |= IO_SCATTER;
	ior->io_data = buffer;
	ior->io_count = size;
	ior->io_done = scsit_target_iodone;
	ior->io_sgp = (io_sglist_t)buffer;
	ior->io_total = real_size;
	ior->io_uaddr = (vm_address_t)opaque_handle;

	if (node == scsit_node_self()) {
		scsit_self_lock();
		if (scsit_self_ior[lun] == (io_req_t)0) {
			scsit_self_ior[lun] = ior;
			scsit_self_unlock();
		} else {
			vm_offset_t wsize;
			io_req_t ior2 = scsit_self_ior[lun];
			scsit_self_ior[lun]=(io_req_t)0;
			scsit_self_unlock();
			assert(!(ior2->io_op & IO_PASSIVE));
			wsize = (ior->io_count > ior2->io_count) ?
			    ior2->io_count: ior->io_count;
			ior->io_residual = ior2->io_residual =
			    ior2->io_count - ior->io_count;
			if (sg)
			    if (ior2->io_op & IO_SCATTER)
				ios_copy((io_scatter_t)ior2->io_data,
					 (io_scatter_t)ior->io_data,
					 wsize);
			    else
				ios_copy_to((char *)ior2->io_data,
					    (io_scatter_t)ior->io_data,
					    wsize);
			else
			    if (ior2->io_op & IO_SCATTER)
				ios_copy_from((io_scatter_t)ior2->io_data,
					      (char *)ior->io_data,
					      wsize);
			    else
				bcopy((const char *)ior2->io_data,
				      (char *)ior->io_data,
				      wsize);
			(*ior2->io_done)(ior2);
			(*ior->io_done)(ior);
		}
	} else if (scsit_device) {
		SCSI_SEND(tgt, ior);
	} else {
		tr1("exit SCSIT_FAILURE");
		return (SCSIT_FAILURE);
	}
	tr1("exit");
	return (SCSIT_SUCCESS);
}

boolean_t
scsit_remote_iodone(
		    io_req_t			ior)
{
	target_info_t *tgt = (target_info_t *)ior->io_dev_ptr;
	TR_DECL("scsit_remote_iodone");
	tr3("enter: ior = 0x%x tgt = 0x%x",(unsigned int)ior,(unsigned int)tgt);
	tgt->ior = ior; /* Restore:  stripped by iodone */
	(*(scsit_remote_complete_t)ior->io_total)
	    ((scsit_handle_t)tgt,
	     (node_name)ior->io_uaddr,
	     ior->io_data,
	     ior->io_count,
	     SCSIT_SUCCESS);

	tr1("exit");
	return (TRUE);
}

scsit_return_t
scsit_remote_op(
		scsit_handle_t	handle,
		node_name	node,
		int		op,
		unsigned int	remote_pa,
		char		*buffer,
		unsigned int	size,
		scsit_remote_complete_t complete)
{
	target_info_t *tgt;
	io_req_t ior;
	register scsi_softc_t	*sc;
	int aop;
	TR_DECL("scsit_remote_op");

	tr5("node = 0x%x op = 0x%x remote_pa = 0x%x size = 0x%x",
	    node, op, remote_pa, size);

	assert(scsit_initialized);

	tgt = (target_info_t *)handle;
	tgt->target_id = node;
	ior = tgt->ior;

	ior->io_residual = 0;
	ior->io_error = KERN_SUCCESS;
	ior->io_op = IO_LOANED|(ior->io_op & IO_INTERNAL);
	ior->io_dev_ptr = (char *)tgt;
	ior->io_data = buffer;
	ior->io_count = size;
	ior->io_done = scsit_remote_iodone;
	ior->io_uaddr = (vm_address_t)node;
	ior->io_total = (vm_size_t)complete;

	aop = op | (complete?0:8);
	sc = scsi_softc[(unsigned char)tgt->masterno];
	(*sc->remote_op)(tgt, aop, remote_pa, buffer, size);

	tr1("exit");
	return SCSIT_SUCCESS;
}

boolean_t
scsit_target(
	     target_info_t		*self)
{
	TR_DECL("scsit_target");
	tr2("enter: 0x%x", (unsigned int)self);
	sstat_inc(target);
	assert(scsit_initialized);
	assert(self->dev_info.cpu.req_cmd == SCSI_CMD_SEND);
	self->dev_info.cpu.req_pending = FALSE;
	self->ior = (io_req_t)0;

	(*scsit_target_select[self->dev_info.cpu.req_lun])
	    (self->dev_info.cpu.req_id,
	     self->dev_info.cpu.req_lun,
	     self->dev_info.cpu.req_len);

	tr1("exit");
	return TRUE;
}

#if	KERNEL_TEST
node_name	scsit_test_node;
char		*scsit_remote_timeri = "SCSI rem_op w/INT";
char		*scsit_remote_timer = "SCSI rem_op";
char		*scsit_send_timer = "SCSI send";
char		*scsit_rpci_timer = "SCSI rpc intr";
char		*scsit_rpct_timer = "SCSI rpc thrd";
char		*scsit_cram_timer = "SCSI cram";
unsigned int	scsit_remote_count;
vm_offset_t	scsit_remote_pa;
#define SCSIT_TEST_BUFFER_SIZE 64*1024
#define SCSIT_TEST_SIZE 256
char scsit_test_buffer[SCSIT_TEST_BUFFER_SIZE];

scsit_handle_t scsit_handle;

boolean_t	scsit_test_initiator = FALSE;

#define SCSIT_TEST_LUN (SCSI_NLUNS - 1)

#define SCSIT_SEND 0
#define SCSIT_RPCI 1
#define SCSIT_RPCT 2
#define SCSIT_CRAM 3


void scsit_test_thread(void);
kern_return_t scsit_test(node_name	node);
void scsit_test_init(void);
void scsit_test_remote_complete(scsit_handle_t	handle,
				node_name	node,
				char		*buffer,
				unsigned int	size,
				scsit_return_t	sr);

void scsit_test_recv_complete(
			      scsit_handle_t		handle,
			      void *			opaque,
			      char *			buffer,
			      unsigned int		count,
			      scsit_return_t		sr);
void scsit_test_target_select(
			      node_name			node,
			      int			lun,
			      unsigned int		size);
void scsit_test_send_complete(
			      scsit_handle_t		handle,
			      void *			opaque,
			      char *			buffer,
			      unsigned int		count,
			      scsit_return_t		sr);
void transport_send_test(void);
void transport_recv_test(void);
void transport_test_setup(void);

void
scsit_test_remote_complete(scsit_handle_t	handle,
			   node_name		node,
			   char			*buffer,
			   unsigned int		size,
			   scsit_return_t	result)
{
	scsit_return_t sr;
	timer_stop((unsigned int)scsit_remote_timeri, 0);
	if (--scsit_remote_count) {
		timer_start((unsigned int)scsit_remote_timeri);
		sr = scsit_remote_op(handle,
				     scsit_test_node,
				     0,
				     scsit_remote_pa,
				     (char *)scsit_test_buffer,
				     size,
				     scsit_test_remote_complete);
		assert(sr == SCSIT_SUCCESS);
	} else {
		thread_wakeup_one((event_t)scsit_test_thread);
	}
}

void scsit_test_recv_complete(
			      scsit_handle_t		handle,
			      void *			opaque,
			      char *			buffer,
			      unsigned int		count,
			      scsit_return_t		sr)
{
	if (scsit_test_initiator) {
		if (buffer[0] == SCSIT_RPCI) {
			timer_stop((unsigned int)scsit_rpci_timer, 0);
			if (--scsit_remote_count) {
				timer_start((unsigned int)scsit_rpci_timer);
				sr = scsit_send(scsit_handle,
						(node_name)opaque,
						SCSIT_TEST_LUN, opaque,
						buffer, count, FALSE);
				assert(sr == SCSIT_SUCCESS);
			} else {
				thread_wakeup_one((event_t)scsit_test_thread);
			}
		} else {
			assert(buffer[0] == SCSIT_RPCT);
			thread_wakeup_one((event_t)scsit_test_thread);
		}
		sr = scsit_receive(handle, (node_name)opaque,
				   SCSIT_TEST_LUN, opaque,
				   buffer, count, FALSE);
		assert(sr == SCSIT_SUCCESS);
	} else {
		if (buffer[0] == SCSIT_CRAM) {
			if (buffer[1] == 1) {
				if (count == SCSIT_TEST_BUFFER_SIZE) {
					count = SCSIT_TEST_SIZE;
				} else {
					count *= 2;
				}
			}
			sr = scsit_receive(handle, (node_name)opaque,
					   SCSIT_TEST_LUN, opaque,
					   buffer, count, FALSE);
		} else if (buffer[0] != SCSIT_SEND) {
			sr = scsit_send(handle, (node_name)opaque,
					SCSIT_TEST_LUN, opaque,
					buffer, count, FALSE);
			assert(sr == SCSIT_SUCCESS);
		} else {
			sr = scsit_receive(handle, (node_name)opaque,
					   SCSIT_TEST_LUN, opaque,
					   buffer, count, FALSE);
			assert(sr == SCSIT_SUCCESS);
		}
	}
}

void scsit_test_target_select(
			      node_name			node,
			      int			lun,
			      unsigned int		size)
{
	assert(FALSE);
}

void scsit_test_send_complete(
			      scsit_handle_t		handle,
			      void *			opaque,
			      char *			buffer,
			      unsigned int		count,
			      scsit_return_t		sr)
{
	if (scsit_test_initiator) {
		if (buffer[0] == SCSIT_SEND) {
			timer_stop((unsigned int)scsit_send_timer, 0);
			if (--scsit_remote_count) {
				timer_start((unsigned int)scsit_send_timer);
				sr = scsit_send(handle, (node_name)opaque,
						SCSIT_TEST_LUN, opaque,
						buffer, count, FALSE);
				assert(sr == SCSIT_SUCCESS);
			} else {
				thread_wakeup_one((event_t)scsit_test_thread);
			}
		} else if (buffer[0] == SCSIT_CRAM) {
			timer_stop((unsigned int)scsit_cram_timer, count);
			if (--buffer[1] == 0) {
				thread_wakeup_one((event_t)scsit_test_thread);
#if	MACH_KDB
				db_show_one_timer((unsigned int)
						  scsit_cram_timer);
#endif	/* MACH_KDB */
			} else {
				timer_start((unsigned int)scsit_cram_timer);
				sr = scsit_send(handle, (node_name)opaque,
						SCSIT_TEST_LUN, 
						opaque, buffer, count, FALSE);
				assert(sr == SCSIT_SUCCESS);
			}
		} else {
			assert(FALSE);
		}
	} else {
		assert(buffer[0]==SCSIT_RPCI ||
		       buffer[0]==SCSIT_RPCT);
		sr = scsit_receive(handle, (node_name)opaque,
				   SCSIT_TEST_LUN, opaque,
				   buffer, count, FALSE);
		assert(sr == SCSIT_SUCCESS);
	}
}

void
scsit_test_thread(void)
{
	scsit_return_t sr;
	int i;
	scsit_handle_t rhandle[MAX_SCSI_TARGETS];
	spl_t s;

	assert(scsit_recv_complete[SCSIT_TEST_LUN]==(scsit_recv_complete_t)0);

	scsit_recv_complete[SCSIT_TEST_LUN] = scsit_test_recv_complete;
	scsit_target_select[SCSIT_TEST_LUN] = scsit_test_target_select;
	scsit_send_complete[SCSIT_TEST_LUN] = scsit_test_send_complete;

	if (scsit_device) {
		s = splbio();

		for(i=0;i<MAX_SCSI_TARGETS;i++)
		    if (i != scsit_node_self()) {
			    sr = scsit_handle_alloc(&rhandle[i]);
			    assert(sr == SCSIT_SUCCESS);
			    sr = scsit_receive(rhandle[i], i, SCSIT_TEST_LUN,
					       (void *)i, scsit_test_buffer,
					       SCSIT_TEST_SIZE, FALSE);
			    assert(sr == SCSIT_SUCCESS);
		    }
		splx(s);
	}

	printf("scsit_test: Initialized\n");
	
	for(;;) {
		scsit_test_initiator = FALSE;
		/* 
	 	 *	It is now safe to wake up the thread suspended in
	 	 *	scsit_test_init(). 
	 	 */
		s = splsched();
		simple_lock(&kernel_test_lock);
		unit_test_thread_sync_done = TRUE;
		if(kernel_test_thread_blocked)
			thread_wakeup((event_t)&kernel_test_thread);
		simple_unlock(&kernel_test_lock);
		splx(s);

		assert_wait((event_t)scsit_test_thread, FALSE);
		thread_block((void (*)(void)) 0);
		if (!scsit_device) continue;

		scsit_test_initiator = TRUE;

		timer_set((unsigned int)scsit_send_timer,
			   scsit_send_timer);

		timer_set((unsigned int)scsit_rpci_timer,
			   scsit_rpci_timer);

		timer_set((unsigned int)scsit_rpct_timer,
			   scsit_rpct_timer);

		timer_set((unsigned int)scsit_remote_timeri,
			   scsit_remote_timeri);

		timer_set((unsigned int)scsit_remote_timer,
			   scsit_remote_timer);

		sr = scsit_handle_alloc(&scsit_handle);
		assert(sr == SCSIT_SUCCESS);

		scsit_remote_count = 10000;

		scsit_test_buffer[0]=SCSIT_SEND;

		s = splbio();
		timer_start((unsigned int)scsit_send_timer);
		sr = scsit_send(scsit_handle, scsit_test_node, SCSIT_TEST_LUN,
				(void *)scsit_test_node, scsit_test_buffer,
				SCSIT_TEST_SIZE, FALSE);
		assert(sr == SCSIT_SUCCESS);

		assert_wait((event_t)scsit_test_thread, FALSE);
		splx(s);
		thread_block((void (*)(void)) 0);
#if	MACH_KDB
		db_show_one_timer((unsigned int)scsit_send_timer);
#endif	/* MACH_KDB */

		scsit_remote_count = 10000;

		scsit_test_buffer[0]=SCSIT_RPCI;

		s = splbio();
		timer_start((unsigned int)scsit_rpci_timer);
		sr = scsit_send(scsit_handle, scsit_test_node, SCSIT_TEST_LUN,
				(void *)scsit_test_node, scsit_test_buffer,
				SCSIT_TEST_SIZE, FALSE);
		assert(sr == SCSIT_SUCCESS);

		assert_wait((event_t)scsit_test_thread, FALSE);
		splx(s);
		thread_block((void (*)(void)) 0);
#if	MACH_KDB
		db_show_one_timer((unsigned int)scsit_rpci_timer);
#endif	/* MACH_KDB */

		scsit_test_buffer[0]=SCSIT_RPCT;

		for(i=0;i<10000;i++) {
			assert_wait((event_t)scsit_test_thread, FALSE);
			s = splbio();
			timer_start((unsigned int)scsit_rpct_timer);
			sr = scsit_send(scsit_handle, scsit_test_node,
					SCSIT_TEST_LUN, 
					(void *)scsit_test_node,
					scsit_test_buffer,
					SCSIT_TEST_SIZE, FALSE);
			assert(sr == SCSIT_SUCCESS);

			splx(s);
			thread_block((void (*)(void)) 0);
			timer_stop((unsigned int)scsit_rpct_timer, 0);
		}

#if	MACH_KDB
		db_show_one_timer((unsigned int)scsit_rpct_timer);
#endif	/* MACH_KDB */

		scsit_test_buffer[0]=SCSIT_CRAM;

		for (i = SCSIT_TEST_SIZE; i< SCSIT_TEST_BUFFER_SIZE +1; i*= 2) {
			assert_wait((event_t)scsit_test_thread, FALSE);
			s = splbio();
			timer_set((unsigned int)scsit_cram_timer,
				  scsit_cram_timer);
			scsit_test_buffer[1]=100;
			timer_start((unsigned int)scsit_cram_timer);
			sr = scsit_send(scsit_handle, scsit_test_node,
					SCSIT_TEST_LUN, 
					(void *)scsit_test_node,
					scsit_test_buffer,
					i, FALSE);
			assert(sr == SCSIT_SUCCESS);
			splx(s);
			thread_block((void (*)(void)) 0);
		}

		for(i=0;i<10000;i++) {
			s = splbio();
			timer_start((unsigned int)scsit_remote_timer);
			sr = scsit_remote_op(scsit_handle, scsit_test_node, 0,
					     scsit_remote_pa, 
					     scsit_test_buffer, 1,
					     (scsit_remote_complete_t)0);
			timer_stop((unsigned int)scsit_remote_timer, 0);
			splx(s);
		}

#if	MACH_KDB
		db_show_one_timer((unsigned int)scsit_remote_timer);
#endif	/* MACH_KDB */
		scsit_remote_count = 10000;

		s = splbio();
		timer_start((unsigned int)scsit_remote_timeri);
		sr = scsit_remote_op(scsit_handle, scsit_test_node, 0,
				     scsit_remote_pa, scsit_test_buffer, 1,
				     scsit_test_remote_complete);
		assert(sr == SCSIT_SUCCESS);

		assert_wait((event_t)scsit_test_thread, FALSE);
		splx(s);
		thread_block((void (*)(void)) 0);
#if	MACH_KDB
		db_show_one_timer((unsigned int)scsit_remote_timeri);
#endif	/* MACH_KDB */
		sr = scsit_handle_free(scsit_handle);
		assert(sr == SCSIT_SUCCESS);
	}
}

kern_return_t
scsit_test(node_name	node)
{
	scsit_test_node = node;
	thread_wakeup_one((event_t)scsit_test_thread);
	if (!scsit_device) {
		printf("This is pretty pointless w/o an adapter\n");
	}
	return (KERN_SUCCESS);
}

void
scsit_test_init(void)
{
	spl_t s;

	kernel_thread(kernel_task, scsit_test_thread, (char *)0);
	/*
	 *	Suspend this thread until we get woken up from inside
	 *	scsit_test_thread().
	 */
	s = splsched();
	simple_lock(&kernel_test_lock);
	while(!unit_test_thread_sync_done){
		assert_wait((event_t)&kernel_test_thread, FALSE);
		kernel_test_thread_blocked = TRUE;
		simple_unlock(&kernel_test_lock);
		splx(s);
		thread_block((void (*)(void)) 0);
		s = splsched();
		simple_lock(&kernel_test_lock);
		kernel_test_thread_blocked = FALSE;
	}
	unit_test_thread_sync_done = FALSE; /* Reset for next use */
	simple_unlock(&kernel_test_lock);
	splx(s);

	scsit_remote_pa = kvtophys((vm_offset_t)scsit_test_buffer);
}

void
transport_send_test(void)
{}

void
transport_recv_test(void)
{}

void
transport_test_setup(void)
{}

#endif	/*KERNEL_TEST*/
#endif	/*NORMA_SCSI*/
