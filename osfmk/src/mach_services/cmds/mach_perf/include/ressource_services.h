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

#ifndef _RESSOURCE_SERVICES_H
#define _RESSOURCE_SERVICES_H

extern  int debug_ressources;
extern	int use_opt;
extern  int mem_use;		/* % of free memory we use */
#define MEM_USE_DEFAULT 60

struct ipc_space {
	int total;
	int send;
	int receive;
	int send_once;
	int port_set;
	int dead;
	int other;
};

struct vm_space {
	int regions;
	int pages;
	int rw;
	int r;
	int inh_share;
};

struct vm_stat {
	int real;
	int free;
	int active;
	int inactive;
	int wired;
	int zero_fill;
	int pagein;
	int pageout;
	int fault;
	int cow;
	int lookup;
};

extern struct vm_space d_vm_space_before, d_vm_space_after;
extern struct ipc_space d_ipc_space_before, d_ipc_space_after;
extern int debug_ressources_initialized;

#define vm_space_diff(before, after) \
	bcmp((char *)after, (char *)before, sizeof(struct vm_space))

#define ipc_space_diff(before, after) \
	bcmp((char *)after, (char *)before, sizeof(struct ipc_space))

#ifdef DEBUG_RESSOURCES
#define DEBUG_RESSOURCES_RESET() debug_ressources_reset()
#define DEBUG_RESSOURCES_TEST() debug_ressources_test(__FILE__, __LINE__)
#else	/* DEBUG_RESSOURCES */
#define DEBUG_RESSOURCES_RESET()
#define DEBUG_RESSOURCES_TEST()
#endif	/* DEBUG_RESSOURCES */

#endif /* _RESSOURCE_SERVICES_H */
	






