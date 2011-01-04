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
 * Revised Copyright Notice:
 *
 * Copyright 1993, 1994 CONVEX Computer Corporation.  All Rights Reserved.
 *
 * This software is licensed on a royalty-free basis for use or modification,
 * so long as each copy of the software displays the foregoing copyright notice.
 *
 * This software is provided "AS IS" without warranty of any kind.  Convex 
 * specifically disclaims the implied warranties of merchantability and
 * fitness for a particular purpose.
 */


#include <mach.h>
#include <mach/mach_traps.h>
#include <mach/message.h>
#include <mach/bootstrap.h>
#include <mach/mach_host.h>
#include <mach/host_info.h>
#include <device/device.h>
#include <device/device_types.h>
#include <device/net_status.h>
#include <tgdb.h>
#include <net_filter.h>
#include <gdb_packet.h>

extern void tgdb_request_loop(void);
void tgdb_set_policy(mach_port_t);

mach_port_t      tgdb_reply_port;
mach_port_t      device_server_port;
security_token_t security_token = {{0,0}};
mach_port_t	 if_port;

extern mach_port_t tgdb_host_priv;
extern mach_port_t tgdb_wait_port_set;

static struct gdb_response tgdb_response_buffer;

typedef struct {
  unsigned char ether[6];
  unsigned char ip[4];
  unsigned char port[2];
} net_addr;

static net_addr tgdb_local_addr;
static net_addr tgdb_foreign_addr;

thread_port_t exc_thread;
extern void tgdb_thread(void);
extern unsigned int tgdb_debug_flags;
extern int debug_flag;

int
main(int argc, char **argv)
{
	kern_return_t	rc;
	mach_port_t	security_port;
	mach_port_t	root_ledger_wired;
	mach_port_t	root_ledger_paged;

	if(argc > 1)
	{
		if(argv[1][0] == '-')
			if(argv[1][1] == 't')
			{
				tgdb_debug_flags = -1;
				debug_flag = 1;
			}
	}

	if (bootstrap_ports(bootstrap_port, &tgdb_host_priv,
			    &device_server_port, &root_ledger_wired,
			    &root_ledger_paged, &security_port))
		panic("bootstrap_ports");

	threads_init();
	printf_init(device_server_port);
	panic_init(tgdb_host_priv);

	tgdb_session_init();

	if (mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_PORT_SET,
			       &tgdb_wait_port_set) != KERN_SUCCESS) {
		printf("tgdb: can't allocate port set\n");
		return 1;
	}

	exc_thread = tgdb_thread_create((vm_offset_t) tgdb_thread);
	if(exc_thread == 0)
		printf("tgdb_thread_create failed\n");

	if((rc = thread_resume(exc_thread)) != KERN_SUCCESS) {
		printf("tgdb: thread_resume failed %x\n", rc);
	}

	/*
	 * Change our scheduling policy to round-robin, and boost our priority.
	 */
	tgdb_set_policy(tgdb_host_priv);

	tgdb_connect();

	return 1;	/* Only reached on error. */
}

int buf[8192];

struct gdb_request  tgdb_request_buffer;

int tgdb_connect(void)
{
	kern_return_t	rc;
	filter_t filter[NET_MAX_FILTER];
	filter_t *pf;
	mach_port_t	port_set;



	if((rc = device_open(device_server_port, MACH_PORT_NULL, D_READ|D_WRITE,
			     security_token, (char *) "lan", &if_port))
	    != D_SUCCESS) {
		printf("tgdb: device_open failed %x\n", rc);
		return 0;	
	}

	pf = filter;
	/*
	 * Deal unly with IP packets.
	 */
	*pf++ = NETF_PUSHHDR+6;		
	*pf++ = NETF_PUSHLIT|NETF_CAND;	
	*pf++ = htons(ETHERTYPE_IP);	

	/*
	 * Check that this is a UDP packet on the tgdb port.
	 */	
	*pf++ = NETF_PUSHWORD+11;
	*pf++ = NETF_PUSHLIT|NETF_EQ;	
	*pf++ = htons(GDB_PORT);	

	if((rc = mach_port_allocate(mach_task_self(),
				    MACH_PORT_RIGHT_PORT_SET,
				    &port_set)) != KERN_SUCCESS) {
		printf("mach_port_allocate : failed %x\n", rc);
		return 0;
	}

	if(ether_setup_port(&tgdb_reply_port, port_set))
		printf("ether_setup_port failed\n");

	if((rc = device_set_filter(if_port, tgdb_reply_port,
				   MACH_MSG_TYPE_MAKE_SEND,
				   101,
				   filter,
				   pf - filter)) != D_SUCCESS) {
		printf("tgdb: device_set_filter returned %x\n", rc);
		return 0;
	}

	printf("remote task debugger started\n");

	while(1) {
		net_rcv_msg_t msg;
		gdb_request_t request = &tgdb_request_buffer;
		unsigned int request_size;
		gdb_response_t response = &tgdb_response_buffer;
		struct netipc_ether_header *ehp;
		struct netipc_udpip_header *uhp;
		tgdb_session_t session;

		msg = (net_rcv_msg_t)buf;

		rc = mach_msg(&msg->msg_hdr, MACH_RCV_MSG,
			      0, 8192, port_set,
			      MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);

		if (rc != MACH_MSG_SUCCESS)
			printf("get packet: kr returned %x\n", rc);

		ehp = (void *) &msg->header;
		uhp = (void *) (&msg->packet[0] + sizeof (int));

		if (ntohs(uhp->udp_dest_port) != GDB_PORT) {
			printf("WARNING NOT A GDB PORT\n");
			continue;
		}

		if (ntohs(uhp->ip_protocol) != UDP_PROTOCOL) {
			printf("WARNING NOT A UDP PACKET!\n");
			continue;
		}

		/*
		 * Copy packet to incoming buffer
		 */
		request_size = ntohs(uhp->udp_length) - 8;

		bcopy((&msg->packet[0] + sizeof(struct netipc_udpip_header) + sizeof(int))
		      ,(char *) &tgdb_request_buffer,
		      request_size);

		if (ntohl(request->task_id) == 0) {
			/*
			 * This request should have gone to the microkernel
			 * debugger stub.
			 */
			printf("tgdb got a request for kernel!\n");
			continue;
		}
			
		/*
		 * If this is just a request for status, drop it unless our
		 * status has changed, i.e., we've taken a breakpoint or something.
		 */
		if (ntohl(request->request) == 'w'
		    && (session = tgdb_lookup_task_id((int)ntohl(request->task_id), 0))
		    && !session->task_suspended) {
			continue;
		}

		/*
		 * Save all the address stuff for the response
		 */
		bcopy((char *) ehp->e_src, (char *) tgdb_foreign_addr.ether, 6);
		bcopy((char *) ehp->e_dest, (char *) tgdb_local_addr.ether, 6);
		bcopy((char *) &uhp->ip_src, (char *) tgdb_foreign_addr.ip, 4);
		bcopy((char *) &uhp->ip_dst, (char *) tgdb_local_addr.ip, 4);
		bcopy((char *) &uhp->udp_source_port, (char *) tgdb_foreign_addr.port, 2);
		bcopy((char *) &uhp->udp_dest_port, (char *) tgdb_local_addr.port, 2);
	
		tgdb_request(request, response);
	}
}


int
ether_setup_port(port, port_set)
	mach_port_t *port;
	mach_port_t port_set;
{
	kern_return_t rc;
        mach_port_limits_t limits;
	mach_msg_type_number_t len;

	*port = mach_reply_port();
	if (!MACH_PORT_VALID(*port))
		panic("ether_set_port: mach_reply_port() failed");
	rc = mach_port_move_member(mach_task_self(), *port, port_set);
	if (rc != KERN_SUCCESS) {
		printf("mach_port_move_member returned 0x%x\n", rc);
		(void)mach_port_destroy(mach_task_self(), *port);
		return 1;
	}

	len = MACH_PORT_LIMITS_INFO_COUNT;
	rc = mach_port_get_attributes(mach_task_self(), *port,
				      MACH_PORT_LIMITS_INFO,
				      (mach_port_info_t)&limits,
				      &len);
	if (rc != KERN_SUCCESS) {
		printf("mach_port_get_attributes returned 0x%x\n", rc);
		(void)mach_port_move_member(mach_task_self(), *port, port_set);
		(void)mach_port_destroy(mach_task_self(), *port);
		return 1;
	}

	limits.mpl_qlimit = MACH_PORT_QLIMIT_MAX;
	rc = mach_port_set_attributes(mach_task_self(), *port,
				      MACH_PORT_LIMITS_INFO,
				      (mach_port_info_t)&limits,
				      MACH_PORT_LIMITS_INFO_COUNT);
	if (rc != KERN_SUCCESS) {
		printf("%s: mach_port_set_attributes returned 0x%x\n",
		       "ether_set_filter", rc);
		(void)mach_port_move_member(mach_task_self(), *port, port_set);
		(void)mach_port_destroy(mach_task_self(), *port);
		return 1;
	}
	return 0;
}


/*
 * Build an IP/UDP packet and copy the buffer into it.  The packet is
 * addressed to the machine from which the most recent request came.
 * This is a problem if remote debugging sessions originate from multiple
 * machines; the source of the request should really be saved with the
 * session.
 *
 * We send the packet using the kgdb hooks in the ethernet driver.
 */
void
tgdb_send_packet(void *buffer, unsigned int size)
{
	static char packet_buffer[ETHERMTU];
	struct netipc_ether_header eh;
	struct netipc_udpip_header ih;
	struct netipc_ether_header *ehp = (void *) packet_buffer;
	struct netipc_udpip_header *uhp = (void *) (ehp + 1);
	char *dp = (char *) (uhp + 1);
	int i, total_length, udp_length;
	unsigned long checksum;
	kern_return_t kr;
	mach_msg_type_number_t	data_count;
	
	udp_length = UDP_OVERHEAD + size;
	total_length = EHLEN + IP_OVERHEAD + udp_length;
	
	bcopy((char *) tgdb_foreign_addr.ether, (char *) eh.e_dest, 6);
	bcopy((char *) tgdb_local_addr.ether, (char *) eh.e_src, 6);
	eh.e_ptype = htons(ETHERTYPE_IP);
	
#if 0
	ih.ip_version = IPVERSION;
	ih.ip_header_length = 5;
#else
	ih.ip_vhl = 0x45;
#endif
	ih.ip_type_of_service = 0;
	ih.ip_total_length = htons(udp_length + IP_OVERHEAD);
	ih.ip_id = htons(0);
	ih.ip_fragment_offset = htons(0);
	ih.ip_time_to_live = 0xff;
	ih.ip_protocol = UDP_PROTOCOL;
	ih.ip_checksum = 0;


	bcopy((char *) tgdb_local_addr.ip, (char *) &ih.ip_src, 4);
	bcopy((char *) tgdb_foreign_addr.ip, (char *) &ih.ip_dst, 4);
	
	for (checksum = i = 0; i < 10; i++) {
		checksum += ((unsigned short *) &ih)[i];
	}
	checksum = (checksum & 0xffff) + (checksum >> 16);
	checksum = (checksum & 0xffff) + (checksum >> 16);
	ih.ip_checksum = (~checksum & 0xffff);
	
	bcopy((char *) tgdb_local_addr.port, (char *) &ih.udp_source_port, 2);
	bcopy((char *) tgdb_foreign_addr.port, (char *) &ih.udp_dest_port, 2);
	ih.udp_length = htons(udp_length);
	ih.udp_checksum = 0;

	bcopy((char *) &eh, (char *) ehp, sizeof(eh));
	bcopy((char *) &ih, (char *) uhp, sizeof(ih));
	bcopy(buffer, dp, size);
	
	while (total_length < 64) {
		packet_buffer[total_length] = 0;
		total_length++;
	}

	if((kr = device_write(if_port, 0, 0, packet_buffer, total_length, (int *)&data_count))
	   != KERN_SUCCESS)
		printf("tgdb: device_write failed %x\n", kr);

}


void
tgdb_set_policy(
	mach_port_t master_host_port)
{
	host_priority_info_data_t	host_pri_info;
	mach_port_t		default_processor_set_name;
	mach_port_t		default_processor_set;
	policy_timeshare_base_data_t	ts_base;
	policy_timeshare_limit_data_t	ts_limit;
	kern_return_t		r;
	unsigned		count;

	count = HOST_PRIORITY_INFO_COUNT;
	r = host_info(mach_host_self(),
		      HOST_PRIORITY_INFO,
		      (host_info_t) &host_pri_info,
		      &count);
	if (r != KERN_SUCCESS)
		printf("Could not get host priority info.  Error = 0x%x\n", r);

	ts_base.base_priority = host_pri_info.system_priority;
	ts_limit.max_priority = host_pri_info.maximum_priority;

	(void)processor_set_default(mach_host_self(),
				    &default_processor_set_name);
	(void)host_processor_set_priv(master_host_port,
				      default_processor_set_name,
				      &default_processor_set);

	r = task_set_policy(mach_task_self(), default_processor_set,
			    POLICY_TIMESHARE,
			    (policy_base_t) & ts_base, POLICY_TIMESHARE_BASE_COUNT,
			    (policy_limit_t) & ts_limit, POLICY_TIMESHARE_LIMIT_COUNT,
			    TRUE);
	if (r != KERN_SUCCESS)
		printf("task_set_policy returned 0x%x\n", r);
}
