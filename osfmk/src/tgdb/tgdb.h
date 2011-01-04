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


#include <mach/exception.h>

#define TGDB_SESSIONS 10

struct slock {
	int		lock_data[4];	
};

struct tgdb_session {
	boolean_t	        valid;
	unsigned int	        task_id;
	mach_port_t	        task;
	mach_port_t	        exception_port;
	mach_port_t	        original_exception_ports[EXC_TYPES_COUNT];
	exception_mask_t        old_exception_masks[EXC_TYPES_COUNT];
	mach_port_t	        original_notify_port;
	mach_port_t	        current_thread;
	boolean_t	        task_suspended;
	int		        exception;
	boolean_t	        single_stepping;
	boolean_t	        one_thread_cont;
	unsigned int	        thread_count;
	mach_port_t	        *thread;
	struct slock            lock; 
};	

typedef struct tgdb_session *tgdb_session_t;

extern tgdb_session_t tgdb_lookup_task_id(unsigned int, unsigned int);

#define assert(a) ((void) 0)

/* Prototypes internal to tgdb. */
struct gdb_request;
struct gdb_response;
struct gdb_registers;
struct mach_thread;
void threads_init(void);
void tgdb_session_init(void);
thread_port_t tgdb_thread_create(vm_offset_t entry);
int tgdb_connect(void);
int ether_setup_port(mach_port_t *portp, mach_port_t port_set);
void tgdb_request(struct gdb_request *request, struct gdb_response *response);
void tgdb_send_packet(void *buffer, unsigned int size);
void tgdb_debug(unsigned int flag, const char *format, int arg);
void simple_lock(struct slock *l);
void simple_unlock(struct slock *l);
int  spin_try_lock(struct slock *l);
void spin_unlock(struct slock *l);
tgdb_session_t tgdb_lookup_task(mach_port_t task);
void tgdb_detach(tgdb_session_t session);
kern_return_t tgdb_vm_read(mach_port_t task, vm_offset_t from, vm_size_t size,
			   vm_offset_t to);
int tgdb_get_break_addr(mach_port_t thread);
void tgdb_clear_trace(mach_port_t thread);
void tgdb_basic_thread_info(mach_port_t thread, mach_port_t task,
			    unsigned int *buffer);
void tgdb_set_trace(mach_port_t thread);
void set_thread_self(struct mach_thread *thread);
void thread_set_regs(vm_offset_t entry, struct mach_thread *th);
boolean_t tgdb_demux(mach_msg_header_t *InHeadP, mach_msg_header_t *OutHeadP);
void tgdb_thread(void);
unsigned int get_dp(void);


#include <machine_tgdb.h>


/* Prototypes from libmach.  These should really be in an exported .h. */
void panic(const char *s, ...);
void printf_init(mach_port_t device_server_port);
void panic_init(mach_port_t port);
void printf(const char *fmt, ...);
void bcopy(const void *src, void *dst, unsigned len);
mach_msg_return_t mach_msg_server(boolean_t (*demux)(mach_msg_header_t *,
						     mach_msg_header_t *),
				  mach_msg_size_t max_size,
				  mach_port_t rcv_name,
				  mach_msg_options_t options);
void bzero(void *dst, size_t len);
