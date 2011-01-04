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
 * Mach Operating System
 * Copyright (c) 1991,1990,1989,1988 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */

/*
 * Bootstrap task definitions
 */
#include <string.h>
#include <mach.h>
#include <sa_mach.h>
#include <cthreads.h>
#include <stdlib.h>
#include <stdio.h>
#include <device/device_types.h>
#include <mach/boot_info.h>
#include <mach/fs/file_system.h>
#include <mach/boolean.h>
#include <mach_debug/mach_debug.h>

/*
 * I/O console synchronization
 */
extern struct mutex	io_mutex;
extern struct condition	io_condition;
extern boolean_t	io_in_progress;

#define	BOOTSTRAP_IO_LOCK()					\
{								\
	mutex_lock(&io_mutex);					\
	while (io_in_progress)					\
		condition_wait(&io_condition, &io_mutex);	\
	io_in_progress = TRUE;					\
	mutex_unlock(&io_mutex);				\
}

#define	BOOTSTRAP_IO_UNLOCK()					\
{								\
	mutex_lock(&io_mutex);					\
	io_in_progress = FALSE;					\
	mutex_unlock(&io_mutex);				\
	condition_signal(&io_condition);			\
}

#define	HEADER_MAX	128

typedef struct objfmt *objfmt_t;
typedef struct objfmt_switch *objfmt_switch_t;

struct objfmt {
    struct loader_info info;
    objfmt_switch_t fmt;
};

struct objfmt_switch {
    const char *name;
    int (*recog)(struct file *, objfmt_t, void *);
    int (*load)(struct file *, objfmt_t, void *);
    void (*symload)(struct file *, mach_port_t, task_port_t, const char *,
		    objfmt_t);
};

extern objfmt_switch_t formats[];

/*
 * Base kernel address, max legal kernel addresss, and submap size for
 * loading the OSF/1 server into the kernel.  These will be initialized
 * during arg parging to machine-dependendent defaults, which can be
 * overridden with runtime flags.  See boot_dep.c
 */
extern unsigned long	unix_mapbase;
extern unsigned long	unix_mapend;

/* Should we automatically try to collocate all servers? (-k given) */
extern boolean_t	collocation_autotry;

/* Should we prohibit collocating any servers? (-u given) */
extern boolean_t	collocation_prohibit;

extern char *boot_device;
#define BOOT_DEVICE_NAME "boot_device"

/* Attempt to load the server into the kernel's address space (collocate).  */
#define SERVER_IN_KERNEL_F 0x1
/* Wait for the server to indicate that it's done with startup processing
   before loading other servers.  */
#define SERVER_SERIALIZE_F 0x2
/* Server died, a notification was received */
#define SERVER_DIED_F	   0x4
/* Server is a data filename */
#define	SERVER_DATA_F	   0x8

struct server {
	const char 	*symtab_name;
	const char 	*server_name;
        int		args_size;
        task_port_t	task_port;

	unsigned int	flags;
	unsigned int	bootstrap_completed; /* Set upon receit of a
					        "bootstrap_completed"
						message for this server.  */
	unsigned long   mapsize;
	unsigned long   mapbase;
	unsigned long   mapend;
};

typedef struct server *server_entry_t;

extern int boot_load_program(mach_port_t, mach_port_t,
			     task_port_t, thread_port_t, 
			     unsigned int, 
			     const char *,
			     const char *,
			     unsigned long);

extern void parse_args(int, char **, char *);
extern void map_init(struct server **, boolean_t);  
extern char *strbuild(char *, ...);
extern kern_return_t bootstrap_notify_dead_name(mach_port_t name);

extern void set_regs(mach_port_t, task_port_t, 
		     thread_port_t, struct loader_info *,
		     unsigned long, vm_size_t);
extern int ex_get_header(struct file *, objfmt_t);

extern boolean_t is_kernel_loadable(struct server *, struct objfmt *);

void service_init(void);
extern boolean_t service_demux(mach_msg_header_t *, mach_msg_header_t *);

extern char *program_name;
extern mach_port_t bootstrap_set;
extern mach_port_t bootstrap_notification_port;

#ifndef	PATH_MAX
#define PATH_MAX	512
#endif	/* PATH_MAX */
