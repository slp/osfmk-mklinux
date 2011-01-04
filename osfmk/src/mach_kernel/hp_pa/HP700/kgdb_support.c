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
/*	$CHeader: kgdb_support.c 1.9 93/12/26 22:26:22 $ */

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

/*
 *	kgdb_support.c
 *
 *	This module implements the device specific network interface for the
 *	kgdb debugger.
 */

#include <types.h>
#include <mach/port.h>
#include <hp_pa/asp.h>
#include <hp_pa/HP700/device.h>
#include <kern/task.h>
#include <kern/thread.h>
#include <kern/misc_protos.h>
#include <mach/boolean.h>
#include <kgdb/gdb_packet.h>
#include <kgdb/kgdb.h>
#include <kern/spl.h>
#include <machine/setjmp.h>
#include <hp_pa/HP700/kgdb_support.h>
#include <machine/trap.h>
#include <machine/endian.h>
#include <vm/vm_map.h>
#include <kgdb/kgdb_stub.h>
#include <hp_pa/HP700/if_i596var.h>

#ifdef TGDB_LEDS		/* XXX XYZZY */
#include "busconf.h"
#define TGDBLED(on,off,toggle) ledcontrol(on,off,toggle)
/*
 * LED4 - tgdb_request_ready
 * LED5 - tgdb_request_ready_lock
 * LED6 - 
 * LED7 - 
 */
#else
#define TGDBLED(on,off,toggle)
#endif

#ifdef KGDB_LEDS		/* XXX XYZZY */
#include "busconf.h"
#define KGDBLED(on,off,toggle) ledcontrol(on,off,toggle)
/*
 * LED4 - kgdb_request_ready
 * LED5 - kgdb_stop
 * LED6 - kgdb_active
 * LED7 - kgdb_connected
 */
#else
#define KGDBLED(on,off,toggle)
#endif

typedef struct {
  unsigned char ether[6];
  unsigned char ip[4];
  unsigned char port[2];
} net_addr;

static boolean_t 	    kgdb_attach_seen = FALSE;

/*
 * These are used to create the IP/UDP packet for responses.  The idea
 * is to build a packet that will go to the last address:port that we
 * received a packet from.  This, of course, assumes that the last
 * packet was from the same address:port as the one we're responding
 * to: FIXME XXX
 */

static net_addr kgdb_local_addr;            /* used by tftp server load */
static net_addr kgdb_foreign_addr;

#if TGDB
static net_addr tgdb_local_addr;
static net_addr tgdb_foreign_addr;
#endif

#if 0
/*
 * Format a "net_addr" into an ascii string for printing.
 */
static char *
addr_format(const void *Addr)
{
  const net_addr *addr = Addr;
  static char hex[16] = "0123456789abcdef";
  static char buf[64];
  char *p = buf;

#define X(v)  if (v > 0xf) *p++ = hex[v & 0xf]; \
                           *p++ = hex[(v >> 4) & 0xf]
#define D(v)  if ((v) > 99) *p++ = '0' + ((v) / 100); \
              if ((v) > 9)  *p++ = '0' + ((v) / 10) % 10; \
                            *p++ = '0' + ((v) % 10)

  X(addr->ether[0]);  *p++ = ':';
  X(addr->ether[1]);  *p++ = ':';
  X(addr->ether[2]);  *p++ = ':';
  X(addr->ether[3]);  *p++ = ':';
  X(addr->ether[4]);  *p++ = ':';
  X(addr->ether[5]);
  *p++ = ' ';
  D(addr->ip[0]);     *p++ = '.';
  D(addr->ip[1]);     *p++ = '.';
  D(addr->ip[2]);     *p++ = '.';
  D(addr->ip[3]);
  *p++ = ':';
  sprintf(p, "%d", ntohs(*(short*)addr->port));
  return buf;
}
#endif

/*
 *	Initialize the network stuff.
 */
void
kgdb_net_init(void)
{
	struct hp_device hd;

#if 0
	hd.hp_addr = (char *)CORE_LAN;
	hd.hp_unit = 0;
	pc586probe(&hd);
	pc586attach(&hd);
#endif
	if (!pc586init(0)) {
		printf("first init failed, try again...\n");
		delay(1000000);
		(void) pc586init(0);
	}
}	

#if TGDB
void
tgdb_net_init(void)
{
	pc586init(0);
}
#endif

/*
 *	Check for incoming network packet.
 */
void
kgdb_net_intr(void)
{
	spl_t s = splimp();
	pc586intr(0);
	splx(s);
}

/*
 *	Transmit network packet.
 */
static void 
kgdb_net_xmt(void *buf, int len)
{
	pc586debug_xmt(0, buf, len);
}	

#if TGDB
static void
tgdb_net_xmt(void *buffer, int length)
{
	pc586debug_xmt(0, buffer, length);
}	
#endif

/*
 * Handle an incoming packet from the network.  Called from network
 * driver interrupt handler.  The driver drops everything except
 * debugger packets before calling here.
 */
boolean_t
kgdb_packet_input(void *buffer, unsigned int size)
{
        netipc_ether_header_t ehp = (netipc_ether_header_t) buffer;
	netipc_udpip_header_t uhp = (netipc_udpip_header_t) (ehp + 1);
	char *dp = (char *) (uhp + 1);
	unsigned int request_size;
	gdb_request_t request = &kgdb_request_buffer;

	if (ntohs(uhp->udp_dest_port) != GDB_PORT) {
		return FALSE;
	}

	/*
	 * If we're not initialized or we are already working on a packet,
	 * drop this one.
	 */
	if (!kgdb_initialized || kgdb_request_ready) {
		kgdb_debug(1, "kgdb_packet_input: dropped (not initialized)\n",
			   0);
		return FALSE;
	}

	/*
	 * Copy packet to incoming buffer
	 */
	request_size = ntohs(uhp->udp_length) - 8;
	if(request_size > sizeof(kgdb_request_buffer)) 
		return FALSE;

	kgdb_bcopy(dp, (char *) &kgdb_request_buffer, request_size);

	/*
	 * If it's not for the microkernel stub (task != 0) and the
	 * microkernel stub is not active, pass it up to the task stub.
	 */
	if (ntohl(request->task_id) != 0) {
		if (kgdb_active) {
			return TRUE;	/* we're active; swallow it */
		}
		return FALSE;
	}

	if (ntohl(request->request) == 'a') {
	  kgdb_attach_seen =  TRUE;
	}

	/*
	 * Save all the address stuff for the response
	 */
	kgdb_bcopy((char *) ehp->e_src, kgdb_foreign_addr.ether, 6);
	kgdb_bcopy((char *) ehp->e_dest, kgdb_local_addr.ether, 6);
	kgdb_bcopy((char *) &uhp->ip_src, kgdb_foreign_addr.ip, 4);
	kgdb_bcopy((char *) &uhp->ip_dst, kgdb_local_addr.ip, 4);
	kgdb_bcopy((char *) &uhp->udp_source_port, kgdb_foreign_addr.port, 2);
	kgdb_bcopy((char *) &uhp->udp_dest_port, kgdb_local_addr.port, 2);
	
	if (kgdb_attach_seen) {
	  /*
	   * If this is just a request for status, drop it unless our
	   * status has changed, i.e., we've taken a breakpoint or something.
	   */
	  if (!kgdb_active && ntohl(request->request) == 'w') {
		kgdb_debug (1, "kgdb_packet_input: dropped (no status change)\n",
			    0);
		return TRUE;
	  }
	}
	else {
	  if (ntohl(request->request) == 'w') {
	      kgdb_response_buffer.xid = request->xid;
	      kgdb_response_buffer.return_code = -1;
	      kgdb_response(&kgdb_response_buffer, 0, 0);
	      return TRUE;
	  }
        }

	if (!kgdb_active) {
		kgdb_stop = TRUE;	/* make driver stop later */
	}

	kgdb_debug(2, "kgdb_packet_input: %d bytes received\n", request_size);

	kgdb_connected = TRUE;
	kgdb_request_ready = TRUE;

	return TRUE;
}

/*
 * Take care of all the grungy network packet details and send the packet
 * to the remote machine.
 */
void
kgdb_send_packet(void *buffer, unsigned int size)
{
	static char packet_buffer[ETHERMTU];
	struct netipc_ether_header eh;
	struct netipc_udpip_header ih;
	netipc_ether_header_t ehp = (netipc_ether_header_t) packet_buffer;
	netipc_udpip_header_t uhp = (netipc_udpip_header_t) (ehp + 1);
	char *dp = (char *) (uhp + 1);
	int i, total_length, udp_length;
	unsigned long checksum;
	
	kgdb_debug(8, "kgdb_send_packet: %d bytes\n", size);

	udp_length = UDP_OVERHEAD + size;
	total_length = EHLEN + IP_OVERHEAD + udp_length;
	
	kgdb_bcopy(kgdb_foreign_addr.ether, (char *) eh.e_dest, 6);
	kgdb_bcopy(kgdb_local_addr.ether, (char *) eh.e_src, 6);
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
	
	kgdb_bcopy(kgdb_local_addr.ip, (char *) &ih.ip_src, 4);
	kgdb_bcopy(kgdb_foreign_addr.ip, (char *) &ih.ip_dst, 4);
	
	for (checksum = i = 0; i < 10; i++) {
		checksum += ((unsigned short *) &ih)[i];
	}
	checksum = (checksum & 0xffff) + (checksum >> 16);
	checksum = (checksum & 0xffff) + (checksum >> 16);
	ih.ip_checksum = (~checksum & 0xffff);
	
	kgdb_bcopy(kgdb_local_addr.port, (char *) &ih.udp_source_port, 2);
	kgdb_bcopy(kgdb_foreign_addr.port, (char *) &ih.udp_dest_port, 2);
	ih.udp_length = htons(udp_length);
	ih.udp_checksum = 0;

	kgdb_bcopy((char *) &eh, (char *) ehp, sizeof(eh));
	kgdb_bcopy((char *) &ih, (char *) uhp, sizeof(ih));
	kgdb_bcopy(buffer, dp, size);
	
	while (total_length < 64) {
		packet_buffer[total_length] = 0;
		total_length++;
	}

	kgdb_net_xmt(packet_buffer, total_length);
}

#if TGDB
/*
 * Handle an incoming packet from the network.  This is called from the
 * network driver interrupt handler.
 */
boolean_t
tgdb_packet_input(void *buffer, unsigned int size)
{
        netipc_ether_header_t ehp = (netipc_ether_header_t) buffer;
	netipc_udpip_header_t uhp = (netipc_udpip_header_t) (ehp + 1);
	char *dp = (char *) (uhp + 1);
	unsigned int request_size;
	gdb_request_t request = &tgdb_request_buffer;
	tgdb_session_t session;
	int retval;

	if (ntohs(uhp->udp_dest_port) != GDB_PORT) {
		return FALSE;
	}

	simple_lock(&tgdb_request_ready_lock);
	TGDBLED(LED5,0,0);

	/*
	 * If we're not initialized or we are already working on a packet,
	 * drop this one.
	 */
	if (!tgdb_initialized || tgdb_request_ready) {
		tgdb_debug(1, "tgdb_packet_input: dropped (%s)\n",
			   !tgdb_initialized
			   ? (int)"not initialized"
			   : (int)"tgdb_request_ready");
		simple_unlock(&tgdb_request_ready_lock);
		TGDBLED(0,LED5,0);
		return FALSE;
	}

	/*
	 * Lock the tgdb_request_* variables so that we can put stuff
	 * into them.  Refer to the comments in tgdb_stub.h for a
	 * description of `tgdb_request_ready'. 
	 *
	 * DANGER WILL ROBINSON:  From here on, if we have to eat the
	 * packet, set `retval' to the return value and then "goto
	 * dropit".  That takes care of unlocking this thing.
	 */
	tgdb_request_ready = TRUE;
	TGDBLED(LED4,0,0);
	simple_unlock(&tgdb_request_ready_lock);
	TGDBLED(0,LED5,0);

	/*
	 * Copy packet to incoming buffer
	 */
	request_size = ntohs(uhp->udp_length) - 8;
	if (request->size > sizeof(tgdb_request_buffer)) {
	  retval = FALSE;
	  goto dropit;
	}
	bcopy(dp, (char *) &tgdb_request_buffer, request_size);	

	if (ntohl(request->task_id) == 0) {
		/*
		 * This request should have gone to the microkernel
		 * debugger stub.
		 */
		retval = FALSE;
		goto dropit;
	}

	/*
	 * If this is just a request for status, drop it unless our
	 * status has changed, i.e., we've taken a breakpoint or something.
	 */
	if (ntohl(request->request) == 'w'
	    && (session = tgdb_lookup_task_id(ntohl(request->task_id), 0))
	    && !session->task_suspended) {
		retval = TRUE;	/* drop the packet */
		goto dropit;
	};

	/*
	 * Save all the address stuff for the response
	 */
	bcopy((char *) ehp->e_src, (char *) tgdb_foreign_addr.ether, 6);
	bcopy((char *) ehp->e_dest, (char *) tgdb_local_addr.ether, 6);
	bcopy((char *) &uhp->ip_src, (char *) tgdb_foreign_addr.ip, 4);
	bcopy((char *) &uhp->ip_dst, (char *) tgdb_local_addr.ip, 4);
	bcopy((char *) &uhp->udp_source_port,
	      (char *) tgdb_foreign_addr.port, 2);
	bcopy((char *) &uhp->udp_dest_port, (char *) tgdb_local_addr.port, 2);
	
	thread_wakeup((int) &tgdb_request_ready);
	
	tgdb_debug(2, "tgdb_packet_input: %d bytes received\n", 
		   request_size);

	return TRUE;

dropit:
	simple_lock(&tgdb_request_ready_lock);
	TGDBLED(LED5,0,0);
	tgdb_request_ready = FALSE;
	TGDBLED(0,LED4,0);
	simple_unlock(&tgdb_request_ready_lock);
	TGDBLED(0,LED5,0);
	return retval;
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
	netipc_ether_header_t ehp = (netipc_ether_header_t) packet_buffer;
	netipc_udpip_header_t uhp = (netipc_udpip_header_t) (ehp + 1);
	char *dp = (char *) (uhp + 1);
	int i, total_length, udp_length;
	unsigned long checksum;
	int s;
	
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

	s = spllan();
	tgdb_net_xmt(packet_buffer, total_length);
	splx(s);
}
#endif /* TGDB */
