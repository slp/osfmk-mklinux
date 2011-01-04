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
 *	File:	dipc/dipc_kmsg.c
 *	Author:	Alan Langerman
 *	Date:	1994
 *
 *	Operations on remote kernel messages.
 */

#include <mach_kdb.h>

#include <ipc/ipc_space.h>
#include <dipc/dipc_error.h>
#include <dipc/dipc_kmsg.h>
#include <dipc/dipc_port.h>
#include <dipc/dipc_msg_progress.h>
#include <dipc/dipc_internal.h>
#include <dipc/dipc_counters.h>
#include <dipc/dipc_funcs.h>
#include <dipc/dipc_rpc.h>
#include <kern/misc_protos.h>
#include <ddb/tr.h>


dstat_decl(unsigned int c_dipc_kmsg_released = 0;)
dstat_decl(unsigned int c_dipc_kmsg_ool_released = 0;)
dstat_decl(unsigned int c_dipc_kmsg_clean_send = 0;)
dstat_decl(unsigned int c_dipc_kmsg_clean_recv_handle = 0;)
dstat_decl(unsigned int c_dipc_kmsg_clean_recv_waiting = 0;)
dstat_decl(unsigned int c_dipc_kmsg_clean_recv_inline = 0;)

extern vm_size_t	msg_ool_size_small;

/*
 *	This definition is an alternative view of a message
 *	that has one or more descriptors present.
 */
typedef struct complex_msg {
	mach_msg_header_t	h;
	mach_msg_body_t		b;
	mach_msg_descriptor_t	d[1];
} *complex_msg_t;


void	dipc_kmsg_abort(
		ipc_kmsg_t	kmsg);

/*
 *	Prepare a kmsg for transmission to a
 *	remote DIPC kernel.
 *
 *	An extra reference is acquired on the
 *	kmsg's remote_port; this reference must
 *	be released when the kmsg is released.
 */
dipc_return_t
dipc_kmsg_copyin(
	ipc_kmsg_t	kmsg)
{
	mach_msg_body_t			*body;
	mach_msg_descriptor_t		*saddr;
	mach_msg_descriptor_t		*eaddr;
	mach_msg_port_descriptor_t	*ool_ports;
	mach_msg_type_name_t		type;
	msg_progress_t			mp;
	dipc_copy_list_t		dclp, *dclpp;
	ipc_port_t			remote_port;
	ipc_port_t			local_port;
	ipc_port_t			port, *ports;
	mach_msg_bits_t			mbits;
	uid_t				uid;
	vm_size_t 			length;
	vm_size_t			ool_ports_length;
	vm_size_t			ool_length;
	dipc_return_t			dr;
	int				j;
	kern_return_t			kr;
	vm_map_copy_t			copy_obj;
	TR_DECL("dipc_kmsg_copyin");

	tr_start();
	tr2("enter: kmsg=0x%x", kmsg);

	assert(!KMSG_IS_META(kmsg));
	assert(!KMSG_IS_MIGRATING(kmsg));
	assert(!KMSG_HAS_WAITING(kmsg));
	assert(!KMSG_PLACEHOLDER(kmsg));

	/*
	 *	If the kmsg has already been converted,
	 *	nothing is left to be done.
	 */
	if (KMSG_IS_DIPC_FORMAT(kmsg)) {
		tr1("exit IS_DIPC_FORMAT");
		tr_stop();
		return DIPC_SUCCESS;
	}

	/*
	 *	Clear header of any junk that might interfere
	 *	with DIPC bits.
	 */
	mbits = (kmsg->ikm_header.msgh_bits &= ~MACH_MSGH_DIPC_BITS);

	KMSG_MSG_PROG_SEND_SET(kmsg, MSG_PROGRESS_NULL);

	/*
	 *	Do nothing with msgh_size.
	 */

	/*
	 *	Convert the destination port, msgh_remote_port.
	 *
	 *	The UID that comes back should match the UID
	 *	assigned in the port.
	 *
	 *	The conversion can't fail because if the port
	 *	isn't already in DIPC, dipc_port_copyin will
	 *	put it there and assign it a UID.
	 *
	 *	We discard the UID; later, it will be retrieved
	 *	from the port and used to send the message.
	 *
	 *	For the destination port ONLY, it is necessary to
	 *	acquire TWO extra references on the port.  The extra
	 *	references guarantee that the port won't disappear
	 *	as the result of dipc_port_copyin.  One reference
	 *	remains until the entire message is disposed, in
	 *	dipc_kmsg_release.  The other reference remains only
	 *	until the message is enqueued remotely, and is
	 *	released by dipc_port_enqueued.  There are two
	 *	references because the dipc_port_enqueued and
	 *	dipc_kmsg_release operations can race.
	 */
	remote_port = (ipc_port_t) kmsg->ikm_header.msgh_remote_port;
	ipc_port_reference(remote_port);
	ipc_port_reference(remote_port);
	dr = dipc_port_copyin(remote_port, MACH_MSGH_BITS_REMOTE(mbits), &uid);
	if (dr == DIPC_PORT_DEAD) {
		ipc_port_release(remote_port);
		ipc_port_release(remote_port);
		return dr;
	}
	assert(dr == DIPC_SUCCESS);
	assert(DIPC_UID_EQUAL(&remote_port->ip_dipc->dip_uid, &uid));

	/*
	 *	Convert the local port, msgh_local_port.
	 *	A UID contains 64 bits, which overflows the
	 *	size of a pointer on 32 bit machines, so we
	 *	steal the msgh_reserved field.  Set msgh_reserved
	 *	to a known value so we can tell later whether
	 *	there was a local port in this message; we could
	 *	check MACH_MSGH_BITS_LOCAL(msgh_bits) but that
	 *	requires a shift+mask, which is slower.
	 *
	 *	We depend here on MACH_PORT_NULL == IP_NULL,
	 *	given that ipc_kmsg_copyin will a pointer to
	 *	a port in msgh_local_port.
	 *
	 *	We assume here that UID (=origin,identifier)
	 *	overlays msgh_local_port,msgh_reserved.
	 *
	 *	The conversion can't fail because if the port
	 *	isn't already in DIPC, dipc_port_copyin will
	 *	put it there and assign it a UID.
	 *
	 *	Note that we must propagate NMS information to
	 *	the receiving node:  if local_port has NMS
	 *	enabled, set the appropriate bit in the message's
	 *	bitfield.
	 */
	local_port = (ipc_port_t) kmsg->ikm_header.msgh_local_port;
	if (local_port == IP_NULL)
		DIPC_UID_NULL((uid_t *) &kmsg->ikm_header.msgh_local_port);
	else {
		dr = dipc_port_copyin(local_port, MACH_MSGH_BITS_LOCAL(mbits),
				      (uid_t *)
				      &kmsg->ikm_header.msgh_local_port);
		assert(dr == DIPC_SUCCESS || dr == DIPC_PORT_DEAD);
		if (IP_NMS(local_port))
			kmsg->ikm_header.msgh_bits |= MACH_MSGH_BITS_LOCAL_NMS;
	}

	/*
	 *	DIPC doesn't touch the msgh_id.
	 */

	/*
	 *	Check for interesting objects in the message body.
	 *	If the message has none, then the conversion process
	 *	is complete.
	 */
	if (!(kmsg->ikm_header.msgh_bits & MACH_MSGH_BITS_COMPLEX)) {
		kmsg->ikm_header.msgh_bits |= MACH_MSGH_BITS_DIPC_FORMAT;
		tr1("exit done");
		tr_stop();
		return DIPC_SUCCESS;
	}

	/*
	 *	Walk the message body, translating inline ports,
	 *	ool memory and ool port arrays.
	 */

	body = (mach_msg_body_t *) (&kmsg->ikm_header + 1);
	saddr = (mach_msg_descriptor_t *) (body + 1);
	eaddr = saddr + body->msgh_descriptor_count;
	assert(saddr < eaddr && eaddr <= (mach_msg_descriptor_t *)
	       (&kmsg->ikm_header + kmsg->ikm_header.msgh_size));

	/*
	 *	Don't yet know whether there's any ool data
	 *	to worry about.
	 */
	mp = MSG_PROGRESS_NULL;
	dclp = DIPC_COPY_LIST_NULL;

    	for (; saddr < eaddr; ++saddr) {
		assert(DIPC_DSC(saddr, DIPC_TYPE_BITS) == 0);
		switch (saddr->type.type) {
		    case MACH_MSG_PORT_DESCRIPTOR: {
			mach_msg_port_descriptor_t 	*dsc;

			/*
			 *	Translate an outgoing port into
			 *	a UID.  Null and dead port names
			 *	require special handling.  Normal
			 *	ports require transit management.
			 *	The resulting UID is stored in the
			 *	(name,pad1) fields of the descriptor.
			 */
			dsc = &saddr->port;
			DIPC_MARK_DSC(dsc, DIPC_FORMAT_DESCRIPTOR);
			port = (ipc_port_t) dsc->name;
			assert(IP_NULL == (ipc_port_t) MACH_PORT_NULL);
			assert(IP_DEAD == (ipc_port_t) MACH_PORT_DEAD);
			if (port == IP_NULL)
				DIPC_UID_NULL((uid_t *) &dsc->name);
			else if (port == IP_DEAD)
				DIPC_UID_DEAD((uid_t *) &dsc->name);
			else {
				type = dsc->disposition;
				dr = dipc_port_copyin(port, type,
						      (uid_t *) &dsc->name);
				assert(dr==DIPC_SUCCESS || dr==DIPC_PORT_DEAD);
				if (IP_NMS(port))
					DIPC_MARK_DSC(dsc,
						      DIPC_PORT_TRANSIT_MGMT);
			}
			break;
		    }

		    case MACH_MSG_OOL_VOLATILE_DESCRIPTOR:
		    case MACH_MSG_OOL_DESCRIPTOR: {
			mach_msg_ool_descriptor_t 	*dsc;

			dsc = &saddr->out_of_line;
			if (dsc->size == 0)
				break;

			if (mp == MSG_PROGRESS_NULL) {
				mp = msg_progress_allocate(TRUE);
				mp->kmsg = kmsg;
				dclpp = &mp->copy_list;
			}

			/*
			 *	The region is kalloc'd, and not in a copy
			 *	object, it's directly pointed to by
			 *	dsc->address.  We must encapsulate it in
			 *	a KERNEL_BUFFER copy object.  Note that
			 *	when this copy_t is discarded, we must
			 *	first kfree the physical copy itself.
			 */
			if (dsc->copy == MACH_MSG_PHYSICAL_COPY
			    && dsc->size < msg_ool_size_small) {
				vm_map_copy_t	copy;

				copy = (vm_map_copy_t)
					kalloc(sizeof(struct vm_map_copy));
				assert(copy != VM_MAP_COPY_NULL);
				copy->type = VM_MAP_COPY_KERNEL_BUFFER;
				copy->size = dsc->size;
				copy->offset = 0;
				copy->cpy_kdata = (vm_offset_t)dsc->address;
				copy->cpy_kalloc_size =
					sizeof(struct vm_map_copy);
				dsc->address = (void *) copy;
			}

			/*
			 *	Build the chain of ool data elements.
			 *	dclpp always points to the place where
			 *	the next dipc_copy_list pointer should
			 *	be inserted.
			 */

			dclp = *dclpp = dipc_copy_list_allocate(TRUE);
			assert(dclp != (dipc_copy_list_t)0);
			dclpp = &dclp->next;

			dclp->next = DIPC_COPY_LIST_NULL;
			dclp->copy = (vm_map_copy_t) dsc->address;
			mp->msg_size += round_page(dclp->copy->size);

			/*
			 * DIPC expects either ENTRY_LIST or KERNEL_BUFFER.
			 * If we got an OBJECT or PAGE_LIST, convert to
			 * ENTRY_LIST.
			 */
			if (dclp->copy->type == VM_MAP_COPY_OBJECT) {
				vm_map_copy_t	new_copy;

				/*
				 *	Take a reference on the object to
				 *	donate to the new (entry-list)
				 *	copy object.
				 */
				vm_object_reference(dclp->copy->cpy_object);
				new_copy = vm_map_entry_list_from_object(
				    dclp->copy->cpy_object,
				    dclp->copy->offset,
				    dclp->copy->size,
				    current_task()->map->hdr.entries_pageable);
				assert(new_copy != VM_MAP_COPY_NULL);
				vm_map_copy_discard(dclp->copy);
				dclp->copy = new_copy;
				dsc->address = (void *) dclp->copy;
			} else if (dclp->copy->type == VM_MAP_COPY_PAGE_LIST) {
				kr = vm_map_convert_to_entry_list(dclp->copy,
				    current_task()->map->hdr.entries_pageable);
				assert(kr == KERN_SUCCESS);
			} else if (dclp->copy->type == VM_MAP_COPY_ENTRY_LIST) {
				vm_map_entry_t	tmp_entry;
				int i = dclp->copy->cpy_hdr.nentries;

				/*
				 * make sure all entries have objects.  Entry
				 * lists created above from object or page_list
				 * flavor copies will always have objects, so
				 * we only need to check entry lists that
				 * came from copyin.
				 */
				tmp_entry = vm_map_copy_first_entry(dclp->copy);
				while (i-- > 0) {
					if (tmp_entry->object.vm_object ==
					    VM_OBJECT_NULL) {
						tmp_entry->object.vm_object =
						  vm_object_allocate((vm_size_t)
							tmp_entry->vme_end -
							tmp_entry->vme_start);
						tmp_entry->offset = 0;
					}
					tmp_entry = tmp_entry->vme_next;
				}
			}

			assert(dclp->copy->type == VM_MAP_COPY_ENTRY_LIST ||
			       dclp->copy->type == VM_MAP_COPY_KERNEL_BUFFER);

			break;

		    }

		    case MACH_MSG_OOL_PORTS_DESCRIPTOR: {
			mach_msg_ool_ports_descriptor_t		*dsc;

			dsc = &saddr->ool_ports;
			length = dsc->count * sizeof(mach_port_t);

			if (length == 0) {
				DIPC_MARK_DSC(dsc, DIPC_FORMAT_DESCRIPTOR);
				break;
			}

			if (mp == MSG_PROGRESS_NULL) {
				mp = msg_progress_allocate(TRUE);
				mp->kmsg = kmsg;
				dclpp = &mp->copy_list;
			}

			/*
			 *	OOL ports are conveyed between nodes
			 *	as a sequence of type descriptors, one
			 *	for each port.  It would be more space
			 *	efficient to only send the UIDs, which
			 *	are smaller than the full-blown type
			 *	descriptors, but we also need to convey
			 *	the per-port NMS information.  Another
			 *	choice is to attach a bit vector specifying
			 *	which ports have NMS turned on; we leave
			 *	that as a future optimization.
			 */
			ool_ports_length = dsc->count *
				sizeof(mach_msg_descriptor_t);

			/*
			 * We allocate a KERNEL_BUFFER type vm_map_copy_t
			 * to hold the out of line port information
			 */

			copy_obj = (vm_map_copy_t)kalloc(ool_ports_length +
						   sizeof(struct vm_map_copy));
			assert(copy_obj != VM_MAP_COPY_NULL);

			ool_ports = (mach_msg_port_descriptor_t *)
				   (copy_obj + 1);

			copy_obj->type = VM_MAP_COPY_KERNEL_BUFFER;
			copy_obj->offset = 0;
			copy_obj->size = ool_ports_length;
			copy_obj->cpy_kdata = (vm_offset_t)ool_ports;
			copy_obj->cpy_kalloc_size = ool_ports_length +
						    sizeof(struct vm_map_copy);

			if (mp == MSG_PROGRESS_NULL) {
				mp = msg_progress_allocate(TRUE);
				mp->kmsg = kmsg;
				dclpp = &mp->copy_list;
			}

			type = dsc->disposition;
			ports = (ipc_port_t *) dsc->address;

			/*
			 *	Walk the existing port array,
			 *	obtaining each port's UID and
			 *	storing it in the new array of
			 *	type descriptors.
			 */
			for (j = 0; j < dsc->count; j++) {
				DIPC_MARK_DSC(&ool_ports[j],
					      DIPC_FORMAT_DESCRIPTOR);
				ool_ports[j].type = MACH_MSG_PORT_DESCRIPTOR;

				if (ports[j] == IP_NULL) {
				    DIPC_UID_NULL((uid_t *)&ool_ports[j].name);
				} else if (ports[j] == IP_DEAD) {
				    DIPC_UID_DEAD((uid_t *)&ool_ports[j].name);
				} else {
				    dr = dipc_port_copyin(ports[j], type,
					      (uid_t *) &ool_ports[j].name);
				    assert(dr == DIPC_SUCCESS ||
							dr == DIPC_PORT_DEAD);
				    if (IP_NMS(ports[j]))
					DIPC_MARK_DSC(&ool_ports[j],
						      DIPC_PORT_TRANSIT_MGMT);
				}
			}

			/*
			 *	Successful translation:  get rid of
			 *	the old port array.
			 */
			(void) kfree((vm_offset_t)ports, length);

			dsc->address = (void *) copy_obj;

			if (mp == MSG_PROGRESS_NULL) {
				mp = msg_progress_allocate(TRUE);
				mp->kmsg = kmsg;
			}

			/*
			 *	Build the chain of ool data elements.
			 *	dclpp always points to the place where
			 *	the next dipc_copy_list pointer should
			 *	be inserted.
			 */

			dclp = *dclpp = dipc_copy_list_allocate(TRUE);
			assert(dclp != (dipc_copy_list_t)0);
			dclpp = &dclp->next;

			dclp->next = DIPC_COPY_LIST_NULL;
			dclp->copy = (vm_map_copy_t) dsc->address;
			mp->msg_size += round_page(dclp->copy->size);
			DIPC_MARK_DSC(dsc, DIPC_FORMAT_DESCRIPTOR);

			break;
		    }

		    default:
			/*
			 * Invalid descriptor
			 */
			panic("dipc_kmsg_copyin:  kmsg[%d]=0x%x, type=%d",
			      kmsg,
			      saddr - (mach_msg_descriptor_t *) (body + 1),
			      saddr->type.type);
			break;
		}
	}
    
	/*
	 *	Conversion complete:  mark message.
	 */
	kmsg->ikm_header.msgh_bits |= MACH_MSGH_BITS_DIPC_FORMAT;
	if (mp != MSG_PROGRESS_NULL) {
		kmsg->ikm_header.msgh_bits |= MACH_MSGH_BITS_COMPLEX_OOL;
	}
	KMSG_MSG_PROG_SEND_SET(kmsg, mp);

	tr2("exit kmsg=0x%x", kmsg);
	tr_stop();
	return DIPC_SUCCESS;
}


/*
 *	Convert a message from network format to local
 *	format; then the message can be handed to local
 *	IPC for delivery.
 *
 *	Note that additional optimizations are possible
 *	by combining some or all of this code directly
 *	into the local IPC copyout path.  For instance,
 *	ool data could be received directly into the
 *	user's address space; currently, the ool data
 *	is received into pageable memory that is then
 *	inserted into the user's address space.
 *
 *	For now we will move all non ool-engine activities
 *	the second pass so the ool-engine can complete ASAP.
 */
dipc_return_t
dipc_kmsg_copyout(
	ipc_kmsg_t	kmsg,
	vm_map_t	map,
	mach_msg_body_t	*slist)
{
	dipc_copy_list_t		dclp, *dclpp;
	dipc_return_t			dr;
	mach_msg_bits_t			mbits;
	ipc_port_t			rport;
	node_name			remote_node;
	ipc_port_t			remote_port;
	mach_msg_descriptor_t		*sstart, *send;
	complex_msg_t			msg;
	int				i;
	vm_object_t			obj;
	vm_offset_t			offset;
	msg_progress_t			mp;
	ipc_kmsg_t			old_prev;
	TR_DECL("dipc_kmsg_copyout");

	tr_start();
	tr2("enter: kmsg=0x%x", kmsg);

	assert(KMSG_IS_DIPC_FORMAT(kmsg));
	assert(!KMSG_IS_META(kmsg));

	remote_port = (ipc_port_t)kmsg->ikm_header.msgh_remote_port;

	assert((remote_port == MACH_PORT_NULL) ||
	       DIPC_IS_DIPC_PORT(remote_port));
	assert((remote_port == MACH_PORT_NULL) ||
		!DIPC_IS_PROXY(remote_port));

	mbits = kmsg->ikm_header.msgh_bits;

	/*
	 * We need to extract the remote_node from the kmsg.  This
	 * is either gotten from the handle, or it is in the handle
	 * field if the handle has been freed.  We can tell this from
	 * the bit MACH_MSGH_BITS_COMPLEX_OOL -- on the receive side.
	 * On rare occasions, we can copy out a message that was
	 * never really sent over the wire.  In this case, the message
	 * requires slightly special treatment.
	 */

	if (!KMSG_RECEIVING(kmsg)) {
		/*
		 *	Send-side kmsg.  Sending node is node_self.
		 */
		remote_node = dipc_node_self();
	} else if (KMSG_HAS_HANDLE(kmsg)) {
		kkt_return_t kktr;
		struct handle_info info;
		/*
		 *	Receive-side kmsg with handle.  Should
		 *	have ool data as well.  (Meta-kmsgs
		 *	shouldn't be present at this time.)
		 */
		assert(mbits & MACH_MSGH_BITS_COMPLEX_OOL);
		kktr = KKT_HANDLE_INFO(kmsg->ikm_handle, &info);
		remote_node = info.node;
	} else {
		/*
		 *	Receive-side kmsg with no handle.
		 *	The kmsg contents should be here in
		 *	their entirety.
		 */
		assert(!(mbits & MACH_MSGH_BITS_COMPLEX_OOL));
		remote_node = (node_name)kmsg->ikm_handle;
	}

	/*
	 * First we copy out the remote port to correctly
	 * deal with rights management
	 */

	if (remote_port != MACH_PORT_NULL) {
		dr = dipc_port_copyout(&remote_port->ip_uid, 
				       MACH_MSGH_BITS_REMOTE(mbits),
				       remote_node,
				       TRUE,
				       IP_NMS(remote_port),
				       &rport);
		/*
		 *	If the kmsg originated on the receive-side,
		 *	then DIPC acquired a reference for it along
		 *	the message delivery path.  We want to INHERIT
		 *	that reference; that is to say, any message
		 *	emerging from the combination of ipc_mqueue_receive
		 *	followed by dipc_kmsg_copyout should have ONE
		 *	reference on the port for the message (not two).
		 */
		if (KMSG_RECEIVING(kmsg)) {
			assert(remote_port->ip_references > 2);
			ipc_port_release(remote_port);
		}
		assert(dr == DIPC_SUCCESS);
		assert(rport == remote_port);
	}

	/*
	 * The local/reply port is not copied out until after the entire
	 * kmsg has been received so that in the event of transport errors,
	 * the port copyout does not need to be undone.
	 */

	/*
	 * Initialize msg_progress pointer if this message is really
	 * being received.  Otherwise, it's being destroyed on the send
	 * side and we can't overwrite ikm_prev with the mp pointer.
	 */
	if (KMSG_RECEIVING(kmsg)) {
		/*
		 *	XXX Grossness XXX
		 *	The receive-side message progress pointer
		 *	is stored in the ikm_prev field.  But some
		 *	code (at least under MACH_ASSERT) cares
		 *	about what was in that field.
		 */
		old_prev = kmsg->ikm_prev;
		KMSG_MSG_PROG_RECV_SET(kmsg, MSG_PROGRESS_NULL);
	}


	/*
	 * Any more work?  The other side always sets this bit
	 * if there is more work to be done
	 */

	if (!(mbits & MACH_MSGH_BITS_COMPLEX))
		goto simple_message;

	/*
	 * This will take two passes.  In the first we set up all of
	 * the vm_map_copy objects to get the ool data.  In the second
	 * pass we translate inline ports and parse the regions containing
	 * out of line ports and translate them.
	 */
	msg = (complex_msg_t) &kmsg->ikm_header;
	obj = VM_OBJECT_NULL;
	offset = 0;
	mp = MSG_PROGRESS_NULL;

	assert(msg->b.msgh_descriptor_count > 0);

	/* set up scatter list */
	if (slist != MACH_MSG_BODY_NULL) {
		sstart = (mach_msg_descriptor_t *) (slist + 1);
		send = sstart + slist->msgh_descriptor_count;
	} else {
		sstart = MACH_MSG_DESCRIPTOR_NULL;
	}

	/*
	 *	If the kmsg originated locally, it is being
	 *	copied out again because port migration brought
	 *	the receive right back here before the message
	 *	could be sent or because we are destroying the
	 *	message.  All of the regions are in an understandable
	 *	format that can be copied out by local IPC; we only
	 *	must translate ports back into local IPC format.
	 */
	if (!KMSG_RECEIVING(kmsg))
		goto loopback_copyout;

	/*
	 * We will need a msg_progress structure for the ool engine
	 */
	if (mbits & MACH_MSGH_BITS_COMPLEX_OOL) {
		mp = msg_progress_allocate(TRUE);
		mp->kmsg = kmsg;
		dclpp = &mp->copy_list;
		KMSG_MSG_PROG_RECV_SET(kmsg, mp);
	}

	for(i=0;i<msg->b.msgh_descriptor_count;i++) {
		switch (msg->d[i].type.type & ~DIPC_TYPE_BITS) {

		    case MACH_MSG_PORT_DESCRIPTOR:
			break;

		    case MACH_MSG_OOL_VOLATILE_DESCRIPTOR:
		    case MACH_MSG_OOL_DESCRIPTOR: {
			mach_msg_ool_descriptor_t *dsc;
			vm_map_copy_t vmc = VM_MAP_COPY_NULL;

			/* skip port descriptors in scatter list */
			SKIP_PORT_DESCRIPTORS(sstart, send);

			/*
			 * This is not all that hard either.  For
			 * now we will use entry list copy_ts
			 * and a single object for all ool regions.
			 * In the future, we should use KERNEL_BUFFER
			 * copy_ts for small ool regions.  We will
			 * have to keep track of the total kalloc
			 * consumption for this message when we
			 * do that
			 */

			dsc = &msg->d[i].out_of_line;
			if (dsc->size == 0) {
				INCREMENT_SCATTER(sstart);
				break;
			}

			assert(mbits & MACH_MSGH_BITS_COMPLEX_OOL);

			/*
			 * Receiver requested overwrite for this
			 * descriptor.  Try to set up the entry to
			 * point at the receiver's address space.
			 */
			if ((sstart != MACH_MSG_DESCRIPTOR_NULL) &&
			    (sstart->out_of_line.copy == MACH_MSG_OVERWRITE)) {
				assert(map != VM_MAP_NULL);
				vmc = vm_map_copy_overwrite_recv(map,
						(vm_offset_t)
						  sstart->out_of_line.address,
						(vm_size_t)
						  dsc->size);
					  /* was sstart->out_of_line.size */
				if (vmc != VM_MAP_COPY_NULL)
					dsc->copy = MACH_MSG_OVERWRITE_DIPC;
			}

			/*
			 * Either we're not overwriting, or we tried
			 * to overwrite directly but failed.
			 */
			if (vmc == VM_MAP_COPY_NULL) {
				if (obj == VM_OBJECT_NULL) {
					/*
					 * We only need one of these
					 */
					obj = vm_object_allocate(
							round_page(dsc->size));
					assert(obj != VM_OBJECT_NULL);
				} else {
					/*
					 *	Add an additional reference
					 *	for each region that's going
					 *	to be pointing to this object.
					 */
					vm_object_reference(obj);
					obj->size += round_page(dsc->size);
				}
				assert(map != VM_MAP_NULL);
				vmc = (vm_map_copy_t)
					vm_map_entry_list_from_object(
						obj,
				   		offset,
				       		dsc->size,
						map->hdr.entries_pageable);
				assert(vmc != VM_MAP_COPY_NULL);

				offset += round_page(dsc->size);
			}

			/*
			 * Note that we have not supported matching
			 * send side and receive side alignment of
			 * ool data
			 */

			dclp = *dclpp = dipc_copy_list_allocate(TRUE);
			assert(dclp != (dipc_copy_list_t)0);
			dclpp = &dclp->next;

			dclp->next = DIPC_COPY_LIST_NULL;
			if (dsc->copy == MACH_MSG_OVERWRITE_DIPC)
				dclp->flags |= DCL_OVERWRITE;
			dsc->type &= ~DIPC_TYPE_BITS;
			dsc->address = (void *)vmc;
			dclp->copy = vmc;
			mp->msg_size += round_page(dsc->size);

			INCREMENT_SCATTER(sstart);
			break;
		    }

		    case MACH_MSG_OOL_PORTS_DESCRIPTOR: {
			mach_msg_ool_ports_descriptor_t *dsc;
			vm_map_copy_t vmc;
			int ool_ports_size;

			dsc = &msg->d[i].ool_ports;

			/* we're not directly overwriting ool ports */
			SKIP_PORT_DESCRIPTORS(sstart, send);
			INCREMENT_SCATTER(sstart);

			if (dsc->count == 0)
				break;

			assert(mbits & MACH_MSGH_BITS_COMPLEX_OOL);

			/*
			 * We use a KERNEL_BUFFER copy_t to get the
			 * region.  Then we take a second pass to
			 * convert to the expected format
			 */

			ool_ports_size = sizeof(mach_msg_descriptor_t)*
				dsc->count;

			vmc = (vm_map_copy_t)kalloc(ool_ports_size +
						  sizeof(struct vm_map_copy));
			vmc->type = VM_MAP_COPY_KERNEL_BUFFER;
			vmc->offset = 0;
			vmc->size = ool_ports_size;
			vmc->cpy_kdata = (vm_offset_t)(vmc+1);
			vmc->cpy_kalloc_size = ool_ports_size +
				sizeof(struct vm_map_copy);

			dsc->address = (void *)vmc;

			dclp = *dclpp = dipc_copy_list_allocate(TRUE);
			assert(dclp != (dipc_copy_list_t)0);
			dclpp = &dclp->next;

			dclp->copy = vmc;
			dclp->next = DIPC_COPY_LIST_NULL;
			mp->msg_size += round_page(ool_ports_size);

			break;
		    }

		    default:
			assert(FALSE);
		}
	}


	/*
	 * Tell the ool engine to suck the data
	 */
	if (mbits & MACH_MSGH_BITS_COMPLEX_OOL) {
		dr = dipc_receive_ool(mp);
		assert(dr == DIPC_SUCCESS ||
		       dr == DIPC_TRANSPORT_ERROR);
		/*
		 *	dipc_receive_ool (really dipc_ool_xfer)
		 *	will free up the handle in all cases.
		 */
		assert(!KMSG_HAS_HANDLE(kmsg));
		if (dr == DIPC_TRANSPORT_ERROR) {
			/*
			 * error occurred during transmission.
			 * discard any ool data already received.
			 */
			for (i=0;i<msg->b.msgh_descriptor_count;i++) {
				switch (msg->d[i].type.type & ~DIPC_TYPE_BITS) {
				    case MACH_MSG_PORT_DESCRIPTOR:
					break;
				    case MACH_MSG_OOL_VOLATILE_DESCRIPTOR:
				    case MACH_MSG_OOL_DESCRIPTOR: {
					mach_msg_ool_descriptor_t *dsc;
					dsc = &msg->d[i].out_of_line;
					if (dsc->size == 0)
						break;
					vm_map_copy_discard(
						(vm_map_copy_t)dsc->address);
					break;
				    }
				    case MACH_MSG_OOL_PORTS_DESCRIPTOR: {
					mach_msg_ool_ports_descriptor_t *dsc;
					dsc = &msg->d[i].ool_ports;
					if (dsc->count == 0)
						break;
					vm_map_copy_discard(
						(vm_map_copy_t)dsc->address);
					break;
				      }
				  default:
					assert(FALSE);
				}
			}
			msg_progress_deallocate(mp);

			/*
			 * Now clean out the message except for the
			 * destination port.
			 */
			kmsg->ikm_header.msgh_local_port = MACH_PORT_NULL;
			kmsg->ikm_header.msgh_bits &=
				~(MACH_MSGH_BITS_COMPLEX |
				  MACH_MSGH_BITS_COMPLEX_OOL);
			if (KMSG_RECEIVING(kmsg))
				kmsg->ikm_prev = old_prev;
			return(DIPC_TRANSPORT_ERROR);
		}
		msg_progress_deallocate(mp);
		mp = MSG_PROGRESS_NULL;
	}

    loopback_copyout:

	assert(mp == MSG_PROGRESS_NULL);
	/*
	 * This pass is for all ports and for PHYSICAL_COPY
	 * descriptors in messages that never went anywhere.
	 */
		
	for(i=0;i<msg->b.msgh_descriptor_count;i++) {
		switch (msg->d[i].type.type & ~DIPC_TYPE_BITS) {
		    case MACH_MSG_PORT_DESCRIPTOR: {
			mach_msg_port_descriptor_t *dsc;

			/*
			 * This is the easiest.  Just copyout
			 * the port and insert the port in the kmsg
			 */
			dsc = &msg->d[i].port;
			dr = dipc_port_copyout((uid_t *)&dsc->name,
					       dsc->disposition,
					       remote_node,
					       FALSE,
					       DIPC_DSC(dsc,
							DIPC_PORT_TRANSIT_MGMT),
					       &rport);
			assert(dr == DIPC_SUCCESS);
			dsc->name = (mach_port_t)rport;
			dsc->type &= ~DIPC_TYPE_BITS;
			break;
		    }

		    case MACH_MSG_OOL_PORTS_DESCRIPTOR: {
			mach_msg_ool_ports_descriptor_t *dsc;
			mach_msg_port_descriptor_t *pdsc;
			int j;
			ipc_port_t *array;
				
			dsc = &msg->d[i].ool_ports;
			pdsc = (mach_msg_port_descriptor_t *)
				(((vm_map_copy_t)dsc->address)+1);
			array = (ipc_port_t *)
				kalloc(sizeof(ipc_port_t)*dsc->count);
			/*
			 * The ipc_copyout expects a kalloced buffer
			 * here
			 */
			assert(array != (ipc_port_t *)0);
			for(j=0;j<dsc->count;j++,pdsc++) {
				dr = dipc_port_copyout(
						(uid_t *)&pdsc->name,
						dsc->disposition,
						remote_node,
						FALSE,
						!!(DIPC_DSC(pdsc,
						      DIPC_PORT_TRANSIT_MGMT)),
						&rport);
				assert(dr == DIPC_SUCCESS);
				array[j]=rport;
			}
			vm_map_copy_discard(dsc->address);
			dsc->address = (void *)array;
			dsc->type &= ~DIPC_TYPE_BITS;
			break;
		    }

		    case MACH_MSG_OOL_VOLATILE_DESCRIPTOR:
		    case MACH_MSG_OOL_DESCRIPTOR: {
			mach_msg_ool_descriptor_t 	*dsc;

			dsc = &msg->d[i].out_of_line;

			/*
			 * PHYSICAL_COPY is meaningless on the receive
			 * side; worse, it causes the local IPC copyout
			 * code to treat dsc->address as a kalloc'ed buffer
			 * instead of a vm_map_copy_t.  Just switch the type
			 * to virtual copy.
			 */
			if ((dsc->copy == MACH_MSG_PHYSICAL_COPY) &&
			    KMSG_RECEIVING(kmsg))
				dsc->copy = MACH_MSG_VIRTUAL_COPY;

			if (dsc->size == 0)
				break;

			/*
			 *	For kmsgs that were converted
			 *	to DIPC_FORMAT but never sent,
			 *	we must reconvert the region
			 *	back to the format local IPC
			 *	expects:  a direct pointer to
			 *	the buffer.  We must also free
			 *	the KERNEL_BUFFER copy object.
			 *	We already know that this message
			 *	is on the send side since we turned
			 *	off PHYSICAL_COPY for receive above.
			 */
			if (dsc->copy == MACH_MSG_PHYSICAL_COPY
			    && dsc->size < msg_ool_size_small) {
				vm_map_copy_t	copy;

				copy = (vm_map_copy_t) dsc->address;
				assert(copy != VM_MAP_COPY_NULL);
				assert(copy->type == VM_MAP_COPY_KERNEL_BUFFER);
				assert(dsc->size == copy->size);
				dsc->address = (void *) copy->cpy_kdata;
				vm_map_copy_discard(copy);
			}
			break;

		    }
		}
	}

    simple_message:
	/*
	 * Finally, we copyout the local/reply port
	 */
	rport = 0;
	if (!DIPC_IS_UID_NULL((uid_t *) &kmsg->ikm_header.msgh_local_port)) {
		dr = dipc_port_copyout(
				(uid_t *) &kmsg->ikm_header.msgh_local_port,
				MACH_MSGH_BITS_LOCAL(mbits),
				remote_node,
				FALSE,
				mbits & MACH_MSGH_BITS_LOCAL_NMS,
				&rport);
		assert(dr == DIPC_SUCCESS);
	}
	/*
	 * Insert port into kmsg
	 */
	kmsg->ikm_header.msgh_local_port = (mach_port_t)rport;

	assert(!KMSG_HAS_HANDLE(kmsg));
	if (KMSG_RECEIVING(kmsg))
		kmsg->ikm_prev = old_prev;
	kmsg->ikm_header.msgh_bits &= ~MACH_MSGH_DIPC_BITS;
	tr1("exit");
	tr_stop();
	return DIPC_SUCCESS;
}


/*
 *	Free up all kmsg resources after a successful
 *	kmsg send.  All ool copy objects, ool ports,
 *	and inline ports are "freed up".  In the case
 *	of ports, reference counts are released, while
 *	for ool memory copy objects are deallocated.
 *
 *	There is no counterpart for this operation in
 *	local IPC because local IPC has no concept of
 *	a successful message send operation that results
 *	in a dead kmsg.
 *
 *	Generally, this routine should only be called from
 *	send-side IPC.  It's hard to think of a receive-side
 *	case that needs to release the kmsg without
 *	destroying it.
 */
void
dipc_kmsg_release(
	ipc_kmsg_t	kmsg)
{
	ipc_port_t	remote_port;
	vm_map_copy_t	copy;
	TR_DECL("dipc_kmsg_release");

	tr_start();
	tr2("enter: kmsg=0x%x", kmsg);

	if (!KMSG_IS_DIPC_FORMAT(kmsg)) {
		printf("dipc_kmsg_release:  !DIPC_FORMAT kmsg 0x%x?\n", kmsg);
		ipc_kmsg_free(kmsg);
		tr1("exit !DIPC_FORMAT");
		tr_stop();
		return;
	}

	/*
	 *	Destination port has an extra reference on it,
	 *	left behind by dipc_kmsg_copyin.
	 */
	remote_port = (ipc_port_t) kmsg->ikm_header.msgh_remote_port;
	ipc_port_release(remote_port);

	/*
	 *	Local port, if any, has been converted to a UID.
	 *	There is nothing left to do to it.
	 */

	if (kmsg->ikm_header.msgh_bits & MACH_MSGH_BITS_COMPLEX_OOL) {
		struct foo {
			mach_msg_header_t h;
			mach_msg_body_t b;
			mach_msg_descriptor_t d[1];
		} *msg = (struct foo *)&kmsg->ikm_header;
		int i;
		msg_progress_t mp;

		for(i=0;i<msg->b.msgh_descriptor_count;i++) {
			switch (msg->d[i].type.type & ~DIPC_TYPE_BITS) {
			    case MACH_MSG_PORT_DESCRIPTOR:
				/* Nothing needs to be done */
				break;
			    case MACH_MSG_OOL_VOLATILE_DESCRIPTOR:
			    case MACH_MSG_OOL_DESCRIPTOR:
				copy = (vm_map_copy_t)
					msg->d[i].out_of_line.address;
				if (copy == VM_MAP_COPY_NULL)
					break;
				assert(copy->type==VM_MAP_COPY_KERNEL_BUFFER
				       || copy->type==VM_MAP_COPY_ENTRY_LIST);
				if ((msg->d[i].out_of_line.copy ==
				     MACH_MSG_PHYSICAL_COPY) &&
				    (msg->d[i].out_of_line.size <
				     msg_ool_size_small)) {
					assert(copy->type ==
						VM_MAP_COPY_KERNEL_BUFFER);
					kfree((vm_offset_t) copy->cpy_kdata,
						copy->size);
				}
				vm_map_copy_discard(copy);
				break;
			    case MACH_MSG_OOL_PORTS_DESCRIPTOR:
				copy = (vm_map_copy_t)
					msg->d[i].out_of_line.address;
				if (copy == VM_MAP_COPY_NULL)
					break;
				assert(copy->type==VM_MAP_COPY_KERNEL_BUFFER);
				vm_map_copy_discard(copy);
				break;
			    default:
				assert(FALSE);
			}
		}
		mp = KMSG_MSG_PROG_SEND(kmsg);
		assert(mp != MSG_PROGRESS_NULL);
		assert((mp->flags & MP_F_RECV) == 0);
		msg_progress_deallocate(mp);
		dstat(++c_dipc_kmsg_ool_released);
	}
	ipc_kmsg_free(kmsg);
	dstat(++c_dipc_kmsg_released);
	tr1("exit");
	tr_stop();
}


/*
 *	Convert a DIPC_FORMAT kmsg back to a local IPC kmsg,
 *	in preparation for destroying it.  This is typically
 *	called via ipc_kmsg_clean.  In some cases, the caller
 *	shouldn't even do local IPC cleaning.
 *
 *	The caller is responsible for doing local IPC cleaning
 *	and then deallocating the kmsg.
 *
 *	Cases:
 *		meta-kmsg:
 *			Abort the kmsg, which notifies the sender
 *			to destroy the kmsg, then deallocates the
 *			handle.  The caller frees the meta-kmsg.
 *
 *		inline kmsg:
 *			On the receiver, we have the only copy.
 *			Copyout the message to reclaim transit
 *			information.
 *
 *			On the sender, call dipc_kmsg_copyout
 *			to convert the kmsg... XXXX
 *
 *		inline kmsg + handle:
 *			send-side:	copyout, let caller destroy
 *			recv-side:	tell sender to destroy original
 *					abort handle
 *					return FALSE so caller just frees kmsg
 *		--- do we have to do anything about the dest port? ---
 *
 *	Returns:
 *		TRUE	all cleaning done, just deallocate
 *		FALSE	caller must clean, then deallocate
 */
#if	MACH_ASSERT
#define	map	current_map()
#endif	/* MACH_ASSERT */
boolean_t
dipc_kmsg_clean(
	ipc_kmsg_t	kmsg)
{
	ipc_port_t			dest;
	dipc_return_t			dr;
	kkt_return_t			kktr;
	mach_msg_dipc_trailer_t		*trailer;
	unsigned int			waitchan;
	node_name			node;
	TR_DECL("dipc_kmsg_clean");

	assert(KMSG_IS_DIPC_FORMAT(kmsg) || KMSG_IS_META(kmsg));
	assert(!(kmsg->ikm_header.msgh_bits & MACH_MSGH_BITS_REF_CONVERT));
	tr_start();
	tr2("cleaning kmsg 0x%x", kmsg);

	if (KMSG_RECEIVING(kmsg)) {
		dest = (ipc_port_t) kmsg->ikm_header.msgh_remote_port;
		if (KMSG_HAS_HANDLE(kmsg)) {
			/*
			 *	Abort the handle, causing the
			 *	sender to drop the message.  No further
			 *	processing required by the receiver;
			 *	just deallocate the message.
			 *
			 *	This path is valid for both meta-kmsgs
			 *	and DIPC_FORMAT messages.
			 */
			dipc_kmsg_abort(kmsg);
			assert(!KMSG_HAS_HANDLE(kmsg));
			kmsg->ikm_header.msgh_bits &=
				~(MACH_MSGH_BITS_COMPLEX |
				  MACH_MSGH_BITS_COMPLEX_OOL |
				  MACH_MSGH_BITS_DIPC_FORMAT);
			dstat(++c_dipc_kmsg_clean_recv_handle);
			tr_stop();
			return FALSE;
		} else if (KMSG_HAS_WAITING(kmsg) && (dest != IP_NULL) &&
			   (dest->ip_receiver == ipc_space_kernel)) {
			/*
			 *	Just wakeup sender with an abort code,
			 *	causing the sender to drop the message.
			 *	No further processing required by the
			 *	receiver; just deallocate the message.
			 */
			assert(!KMSG_IS_META(kmsg));
			trailer = KMSG_DIPC_TRAILER(kmsg);
			waitchan = trailer->dipc_sender_kmsg;
			node = kmsg->ikm_handle;
			assert(dipc_node_is_valid(node));
			kktr = drpc_wakeup_sender(node, waitchan,
						  DIPC_MSG_DESTROYED, &dr);
			assert(kktr == KKT_SUCCESS);
			assert(dr == DIPC_SUCCESS);
			kmsg->ikm_header.msgh_bits &=
				~(MACH_MSGH_BITS_COMPLEX |
				  MACH_MSGH_BITS_COMPLEX_OOL |
				  MACH_MSGH_BITS_DIPC_FORMAT);
			tr_stop();
			dstat(++c_dipc_kmsg_clean_recv_waiting);
			return FALSE;
		} else {
			/*
			 *	Complete inline kmsg in DIPC_FORMAT.
			 *	We must copyout the message to reclaim
			 *	any transits in it.
			 *
			 *	We don't need any map for dipc_kmsg_copyout
			 *	because all of the data for the message are
			 *	already on this node.  The same rationale
			 *	applies to the unneeded scatter list.
			 */
			dr = dipc_kmsg_copyout(kmsg, VM_MAP_NULL,
					       MACH_MSG_BODY_NULL);
			assert(dr == DIPC_SUCCESS);
			assert(!KMSG_IS_DIPC_FORMAT(kmsg));
			assert(!KMSG_HAS_HANDLE(kmsg));
			dstat(++c_dipc_kmsg_clean_recv_inline);
			tr_stop();
			return TRUE;
		}
	} else {
		/*
		 *	Send-side kmsg
		 */
		assert(!KMSG_HAS_HANDLE(kmsg));
		/*
		 *	We don't need any map for dipc_kmsg_copyout
		 *	because all of the data for the message are
		 *	already on this node.  The same rationale
		 *	applies to the unneeded scatter list.
		 */
		dr = dipc_kmsg_copyout(kmsg, VM_MAP_NULL, MACH_MSG_BODY_NULL);
		assert(dr == DIPC_SUCCESS);
		assert(!KMSG_IS_DIPC_FORMAT(kmsg));
		assert(!KMSG_HAS_HANDLE(kmsg));
		dstat(++c_dipc_kmsg_clean_send);
		tr_stop();
		return TRUE;
	}
	/* NOTREACHED */
}


/*
 *	Cause the sender of a kmsg to destroy it.
 *	Free up the kmsg's handle.  This call works
 *	for meta- and DIPC_FORMAT-kmsgs.
 */
void
dipc_kmsg_abort(
	ipc_kmsg_t	kmsg)
{
	kkt_return_t	kktr;
	TR_DECL("dipc_kmsg_abort");

	tr2("kmsg 0x%x", kmsg);
	printf("dipc_kmsg_abort:  kmsg 0x%x\n", kmsg);

	assert(KMSG_IS_META(kmsg) || KMSG_IS_DIPC_FORMAT(kmsg));
	assert(KMSG_HAS_HANDLE(kmsg));

	/*
	 *	Abort our end of the connection, and tell the
	 *	sender that we aborted and why.
	 */
	kktr = KKT_HANDLE_ABORT(kmsg->ikm_handle, DIPC_MSG_DESTROYED);
	assert(kktr == KKT_SUCCESS);

	/*
	 *	This handle is USELESS now and NO operations are in
	 *	progress on it anywhere.
	 */
	KMSG_DROP_HANDLE(kmsg, kmsg->ikm_handle);
}


#if	MACH_KDB
#include <ddb/db_output.h>
extern void	ipc_print_type_name(int type_name);
extern char	*mm_copy_options_string(mach_msg_copy_options_t	option);

#define	printf	kdbprintf
extern int	indent;


void
dipc_descriptor_print(
	mach_msg_descriptor_t	*descrip)
{
	mach_msg_descriptor_type_t	type;
	unsigned int			port_transit;

	port_transit = DIPC_DSC(descrip, DIPC_PORT_TRANSIT_MGMT);
	type = descrip->type.type & ~DIPC_TYPE_BITS;

	switch (type) {
	    case MACH_MSG_PORT_DESCRIPTOR: {
		mach_msg_port_descriptor_t *dsc;

		dsc = &descrip->port;
		db_printf("-- DIPC PORT name = (0x%x,0x%x) %s disp = ",
			  dsc->name, dsc->pad1,
			  port_transit ? "(TRANSIT MGMT)" : "");
		ipc_print_type_name(dsc->disposition);
		db_printf("\n");
		break;
	    }

	    case MACH_MSG_OOL_VOLATILE_DESCRIPTOR:
	    case MACH_MSG_OOL_DESCRIPTOR: {
		mach_msg_ool_descriptor_t *dsc;
		
		dsc = &descrip->out_of_line;
		db_printf("-- OOL%s addr = 0x%x size = 0x%x copy = %s %s\n",
			  type == MACH_MSG_OOL_DESCRIPTOR ? "": " VOLATILE",
		          dsc->address, dsc->size,
			  mm_copy_options_string(dsc->copy),
			  dsc->deallocate ? "DEALLOC" : "");
		break;
	    } 

	    case MACH_MSG_OOL_PORTS_DESCRIPTOR: {
		mach_msg_ool_ports_descriptor_t *dsc;

		dsc = &descrip->ool_ports;

		db_printf("-- OOL_PORTS addr = 0x%x count = 0x%x ",
		          dsc->address, dsc->count);
		db_printf("disp = ");
		ipc_print_type_name(dsc->disposition);
		db_printf("copy = %s %s\n",
			  mm_copy_options_string(dsc->copy),
			  dsc->deallocate ? "DEALLOC" : "");
		break;
	    }

	    default: {
		printf("-- UNKNOWN DESCRIPTOR 0x%x\n", type);
		break;
	    }
	}
}


/*
 *	Don't call this routine directly, use ipc_kmsg_print instead.
 */
void
dipc_kmsg_print(
	ipc_kmsg_t	kmsg)
{
	mach_msg_dipc_trailer_t		*trailer;

	printf("handle=0x%x\n", kmsg->ikm_handle);
	if (kmsg->ikm_header.msgh_bits & MACH_MSGH_BITS_COMPLEX_OOL) {
		iprintf("send-mp=0x%x recv-mp=0x%x\n",
			KMSG_MSG_PROG_SEND(kmsg),
			KMSG_MSG_PROG_RECV(kmsg));
	}
	trailer = KMSG_DIPC_TRAILER(kmsg);
	/*
	 *	ipc_kmsg_print, the caller of this routine, will
	 *	put out the newline for this printf.
	 */
	iprintf("trailer at 0x%x:  (recv sender_kmsg)/(send flags) 0x%x",
		trailer, trailer->dipc_sender_kmsg);
}
#endif	/* MACH_KDB */
