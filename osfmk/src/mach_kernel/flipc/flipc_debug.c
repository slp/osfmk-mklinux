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

/* 
 * Use for routines that are here purely for debugging purposes.
 * Includes the flipc_debug.h file for compilation, and may include
 * kernel specific debug routines in the future.
 */

/*
 * As best I can tell, config isn't very smart and can only take a
 * single option to decide if it wants to include a file or not.  So
 * this file is officially a flipc file, and it won't have anything in
 * it if there aren't debug options set.
 */
#include <mach_kdb.h>
#include <mach_kgdb.h>
#include <flipc/flipc.h>

#if MACH_KDB || MACH_KGDB
#include <mach/flipc_debug.h>
#endif

/*
 * Forward declaration
 */
void flipcme_show_shared(void);

void flipcme_show_shared()
{
    flipc_epgroup_t epgroup_array;
    flipc_endpoint_t endpoint_array;
    int i;

    flipc_comm_buffer_ctl_t shared_cb_ctl =
	(flipc_comm_buffer_ctl_t) flipc_cb_base;

    printf("Shared buffer: 0x%x\n", flipc_cb_base);
    printf("\t%sreal_time_primitives, %smessage_engine_in_kernel, %sno_bus_locking\n",
	   BOOL(shared_cb_ctl->kernel_configuration.real_time_primitives),
	   BOOL(shared_cb_ctl->kernel_configuration.message_engine_in_kernel),
	   BOOL(shared_cb_ctl->kernel_configuration.no_bus_locking));
    printf("\tdata_buffer_size 0x%x, local_node_address=0x%x, null_destination=0x%x\n",
	   shared_cb_ctl->data_buffer_size,
	   shared_cb_ctl->local_node_address,
	   shared_cb_ctl->null_destination);
    printf("\tendpoint: start=0x%x, number=0x%x free=0x%x\n",
	   shared_cb_ctl->endpoint.start, shared_cb_ctl->endpoint.number,
	   shared_cb_ctl->endpoint.free);
    printf("\t\tc_type  s_dpb_en p_n_enpnt s_epgrp s_ns_enpnt\n");
    printf("\t\t  c_m_blst c_n_blst s_rels_ptr sh_acq_ptr p_proc_ptr\n");
    endpoint_array = FLIPC_ENDPOINT_PTR(shared_cb_ctl->endpoint.start);
    for (i = 0; i < shared_cb_ctl->endpoint.number; i++) {
	    printf("\t0x%-8x 0x%-8x 0x%-8x 0x%-8x 0x%-8x 0x%-8x 0x%-8x 0x%-8x\n",
		   (int)&endpoint_array[i] - (int)&endpoint_array[0],
		   endpoint_array[i].constda_type, 
		   endpoint_array[i].constdm_type, 
		   endpoint_array[i].sailda_dpb_or_enabled,
		   endpoint_array[i].saildm_dpb_or_enabled,
		   endpoint_array[i].pail_next_eg_endpoint,
		   endpoint_array[i].sail_epgroup,
		   endpoint_array[i].sail_next_send_endpoint);
	    printf("\t\t  0x%-8x 0x%-8x 0x%-8x 0x%-8x 0x%-8x 0x%-8x 0x%-8x \n",
		   endpoint_array[i].constda_my_buffer_list,
		   endpoint_array[i].constdm_my_buffer_list,
		   endpoint_array[i].constda_next_buffer_list,
		   endpoint_array[i].constdm_next_buffer_list,
		   endpoint_array[i].sail_release_ptr,
		   endpoint_array[i].shrd_acquire_ptr,
		   endpoint_array[i].sme_process_ptr);
    }

    printf("\tepgroup: start=0x%x, number=0x%x free=0x%x\n",
	   shared_cb_ctl->epgroup.start, shared_cb_ctl->epgroup.number,
	   shared_cb_ctl->epgroup.free);
    epgroup_array = FLIPC_EPGROUP_PTR(shared_cb_ctl->epgroup.start);
    printf("\t\tenabled version pf_endpoint pail_free\n");
    for (i = 0; i < shared_cb_ctl->epgroup.number; i++) {
	    printf("\t\t0x%-8x 0x%-8x 0x%-8x     0x%-8x\n",
		   epgroup_array[i].sail_enabled,
		   epgroup_array[i].pail_version,
		   epgroup_array[i].pail_first_endpoint,
		   epgroup_array[i].pail_free);
    }

    printf("\tbufferlist: start=0x%x, number=0x%x free=0x%x\n",
	   shared_cb_ctl->bufferlist.start, shared_cb_ctl->bufferlist.number,
	   shared_cb_ctl->bufferlist.free);
    printf("\tdata_buffer: start=0x%x, number=0x%x free=0x%x\n",
	   shared_cb_ctl->data_buffer.start, shared_cb_ctl->data_buffer.number,
	   shared_cb_ctl->data_buffer.free);
    printf("\tsail_request_sync=0x%x, sme_respond_sync=0x%x\n",
	   shared_cb_ctl->sail_request_sync ,shared_cb_ctl->sme_respond_sync);
    printf("\tsme_receive_in_progress=0x%x\n", 
	   shared_cb_ctl->sme_receive_in_progress);
    printf("\tsail_send_endpoint_list=0x%x\n", 
	   shared_cb_ctl->sail_send_endpoint_list);
}
