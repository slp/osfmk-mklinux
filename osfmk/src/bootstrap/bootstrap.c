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
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS 
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
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

/*
 * Bootstrap the various built-in servers.
 */

#include "bootstrap.h"

#include <ctype.h>
#include <stdlib.h>
#include <mach_error.h>

#include <mach/mach_traps.h>
#include <mach/mach_interface.h>
#include <mach/bootstrap.h>
#include <mach/host_info.h>
#include <mach/mach_host.h>
#include <mach/exc_server.h>
#include <mach/bootstrap_server.h>

#include <device/device_types.h>
#include <device/device.h>

#if	PARAGON860
mach_port_t	bootstrap_root_device_port;	/* local name */
#endif	/* PARAGON860 */
mach_port_t	bootstrap_master_device_port;	/* local name */
mach_port_t	bootstrap_master_host_port;	/* local name */
mach_port_t	bootstrap_notification_port;	/* local name */
mach_port_t	security_port;
mach_port_t	root_ledger_wired;
mach_port_t	root_ledger_paged;

const char default_config[] = "\
name_server name_server\n\
default_pager default_pager\n\
unix startup -s\n\
";

struct server *servers;
int nservers;

struct server *datadev;
int ndatadev;

static char my_name[] = "(bootstrap):";
char *program_name = (char *)"(bootstrap)";
char *data_name = (char *)"(data_server)";

/* static functions */
static void parse_path(struct server *, const char *);
static int parse_config_file(char *, size_t);
static char *parse_boot_args(char **data, struct server **sp);
static void data_device_loop(void);
static kern_return_t data_device_load_file(task_port_t, const char *,
					   vm_address_t *, vm_size_t *);

char	*boot_device = (char *)BOOT_DEVICE_NAME;

boolean_t bootstrap_demux(mach_msg_header_t *, mach_msg_header_t *);
void bootstrap_initialize(mach_port_t);
void start_bootstrap_thread(boolean_t);
boolean_t bootstrap_compat(mach_msg_header_t *, mach_msg_header_t *);

/*
 * Our task port.
 */
task_port_t	bootstrap_self;
mach_port_t	bootstrap_exception_port;

/*
 * Port set for bootstrap task.
 */
mach_port_t	bootstrap_set;

/*
 * We receive bootstrap requests on this port.
 */
mach_port_t	bootstrap_bootstrap_port;

/*
 * Security token for device operations
 */
security_token_t        bootstrap_security_token;

/*
 * Port for data_device thread
 */
thread_port_t bootstrap_data_device;

/*
 * Maximum message size for mach_msg_server.
 */
mach_msg_size_t bootstrap_msg_size = 128;

/*
 * Set when we should ask user for server path and arguments
 */
boolean_t	prompt = FALSE;

/*
 * Try to collocate all servers regardless of bootstrap.conf
 * flag (set via -k).
 */
boolean_t       collocation_autotry = FALSE;

/*
 * Prohibit using collocation, regardless of bootstrap.conf
 * flag (set via -u).
 */
boolean_t       collocation_prohibit = FALSE;

/*
 * I/O console synchronization
 */
struct mutex		io_mutex;
struct condition	io_condition;
boolean_t		io_in_progress;

/*
 * Bootstrap task.
 * Runs in user space.
 *
 * Called as 'bootstrap [bootstrap-flags]'
 *
 */
int
main(int argc, char **argv)
{
	register kern_return_t	kr;
	task_port_t		user_task;
	thread_port_t		user_thread;
	mach_port_t		prev_port;
	int			i;
	char			pathname[PATH_MAX];
	char			*ptr;
	char			*config_data;
	size_t			config_size;
	struct server	        *sp;
	int			result;
	boolean_t		retry;
	mach_msg_type_number_t	count;
	struct file		f;

	/*
	 * Initialize current cthread
	 */
	cthread_set_name(cthread_self(), "bootstrap_server");
	cthread_wire();

	/*
	 * Initialize synchronization
	 */
	condition_init(&io_condition);
	mutex_init(&io_mutex);

	/*
	 * Get master host and device ports
	 */
	kr = bootstrap_ports(bootstrap_port,
			     &bootstrap_master_host_port,
			     &bootstrap_master_device_port,
			     &root_ledger_wired,
			     &root_ledger_paged,
			     &security_port);
	if (kr != KERN_SUCCESS) 
	    panic(my_name);

	/*
	 * Initialize console and reboot services
	 */
	printf_init(bootstrap_master_device_port);
	panic_init(bootstrap_master_host_port);

	parse_args(argc, argv, pathname);

	/*
	 *	This task will become the bootstrap task.
	 */
	bootstrap_self = mach_task_self();

	/*
	 *	Initialize the exception port.
	 */
	kr = mach_port_allocate(bootstrap_self, MACH_PORT_RIGHT_RECEIVE,
				&bootstrap_exception_port);
	if (kr != KERN_SUCCESS)
	    panic(my_name);

	/*
	 *	Initialize the bootstrap port.
	 */
	kr = mach_port_allocate(bootstrap_self, MACH_PORT_RIGHT_RECEIVE,
				&bootstrap_bootstrap_port);
	if (kr != KERN_SUCCESS)
	    panic(my_name);

	kr = mach_port_allocate(bootstrap_self, MACH_PORT_RIGHT_PORT_SET,
				&bootstrap_set);
	if (kr != KERN_SUCCESS)
	    panic(my_name);

	kr = mach_port_move_member(bootstrap_self,
				   bootstrap_exception_port,
				   bootstrap_set);
	if (kr != KERN_SUCCESS)
	    panic(my_name);

	kr = mach_port_move_member(bootstrap_self,
				   bootstrap_bootstrap_port,
				   bootstrap_set);
	if (kr != KERN_SUCCESS)
	    panic(my_name);

	/*
	 * Initialize the security token for device operations
	 */
	count = TASK_SECURITY_TOKEN_COUNT;
	kr = task_info(bootstrap_self, TASK_SECURITY_TOKEN,
		       (task_info_t)&bootstrap_security_token, &count);
	if (kr != KERN_SUCCESS)
	    panic(my_name);

	/*
	 *  Have a reply port for dead-name notification.
	 *  XXX restart the service if it dies?
	 */
	kr = mach_port_allocate(bootstrap_self, MACH_PORT_RIGHT_RECEIVE,
				&bootstrap_notification_port);
	if (kr != KERN_SUCCESS)
		panic(my_name);

	kr = mach_port_move_member(bootstrap_self,
				   bootstrap_notification_port,
				   bootstrap_set);
	if (kr != KERN_SUCCESS)
		panic(my_name);

	/*
	 * Allocate the service server ports and add them to our
	 * port set.
	 */
	service_init();

	/*
	 * task_set_exception_ports and task_set_bootstrap_port
	 * both require a send right.
	 */
	(void) mach_port_insert_right(bootstrap_self, bootstrap_bootstrap_port,
				      bootstrap_bootstrap_port,
				      MACH_MSG_TYPE_MAKE_SEND);
	(void) mach_port_insert_right(bootstrap_self, bootstrap_exception_port,
				      bootstrap_exception_port,
				      MACH_MSG_TYPE_MAKE_SEND);

	/*
	 * Change our exception port.
	 */
	(void) task_set_exception_ports(bootstrap_self,
					EXC_MASK_ALL &
					~(EXC_MASK_SYSCALL |
			  EXC_MASK_MACH_SYSCALL | EXC_MASK_RPC_ALERT),
					bootstrap_exception_port,
					EXCEPTION_STATE_IDENTITY, 0);

	/*
	 * Read our configuration file.
	 */
	config_size = strlen(default_config);
	config_data = malloc(config_size+1);
	memcpy(config_data, default_config, config_size+1);
	retry = FALSE;

	for (;;) {
	    char *conf;
	    size_t size;
	    int line;

	    if (prompt || retry) {
		char newpath[PATH_MAX];

		BOOTSTRAP_IO_LOCK();
		printf("Configuration file (or 'builtin'): ");
		safe_gets(pathname, newpath, sizeof(newpath));
		BOOTSTRAP_IO_UNLOCK();
		if (strcmp(newpath, "builtin") == 0) {
		    config_size = strlen(default_config);
		    config_data = malloc(config_size+1);
		    memcpy(config_data, default_config, config_size+1);
		    if (parse_config_file(config_data, config_size) >= 0)
                        panic("Error in bootstrap builtin configuration\n");
		    break;
		}
		if (newpath[0] != '\0')
		    strcpy(pathname, newpath);
	    }
	    memset((char *)&f, 0, sizeof(f));
#if	PARAGON860
	    result = open_file(bootstrap_root_device_port, pathname, &f);
#else	/* PARAGON860 */
	    result = open_file(bootstrap_master_device_port, pathname, &f);
#endif	/* PARAGON860 */
	    if (result != 0) {
	        BOOTSTRAP_IO_LOCK();
		printf("\nERROR: bootstrap task cannot find configuration file, please make sure that\n");
		printf("       your boot device and partition is correctly specified.\n");
#if	ppc
		printf("       On PowerMacs this is specified from the MkLinux control panel.\n");
#endif /* ppc */
		printf("\n");
	        BOOTSTRAP_IO_UNLOCK();
		/* Loop back around, prompting for everything */
		retry=TRUE;
		continue;
	    }
	    size = file_size(&f);
	    conf = malloc(size+1);
	    conf[size] = 0;
	    result = read_file(&f, 0, (vm_offset_t) conf, size);
	    close_file(&f);
	    if (result != 0) {
		free(conf);
		BOOTSTRAP_IO_LOCK();
		printf("Error reading configuration file\n");
		BOOTSTRAP_IO_UNLOCK();
		if (!retry)
		    retry = TRUE;
		continue;
	    }

	    config_data = conf;
	    config_size = size;
            switch (line = parse_config_file(config_data, config_size)) {
            default:
                if (!retry)
                    retry = TRUE;
		free(conf);
		BOOTSTRAP_IO_LOCK();
                printf("Syntax error in configuration file (line %d)\n",
		       line);
		BOOTSTRAP_IO_UNLOCK();
                continue;

            case 0:
		BOOTSTRAP_IO_LOCK();
                printf("Empty bootstrap configuration file\n");
		BOOTSTRAP_IO_UNLOCK();
                break;

            case -1:
                break;
            }
            break;
 	}

	/*
	 * Start data_device thread
	 */
	if (ndatadev > 0) {
	    BOOTSTRAP_IO_LOCK();
	    printf("%s: started\n", data_name);
	    BOOTSTRAP_IO_UNLOCK();
	    cthread_detach(cthread_fork((cthread_fn_t)data_device_loop, 0));
	}

	/*
	 * Start all servers
	 */
	for (i = 0; i < nservers; i++) {
	    struct objfmt ofmt;
	    char newpath[PATH_MAX];

	    retry = FALSE;
Retry_Server:
	    sp = &servers[i];
	    /* If -a was seen in bootstrap args then prompt for
	     * the name and path of the server. 
	     */
	    if (prompt || retry) {
		char oldpath[PATH_MAX];
		const char *p1;
		char *p2;

		BOOTSTRAP_IO_LOCK();
		printf("%s: ", sp->symtab_name);
		for (p1 = sp->server_name, p2 = oldpath;
		     p1 < sp->server_name + sp->args_size;
		     p1++, p2++) {
			if (*p1 == '\0')
				*p2 = ' ';
			else if (*p1 == ' ') {
				*p2++ = '\\';
				*p2 = *p1;
			} else
				*p2 = *p1;
		}
		*p2 = '\0';		
		safe_gets(oldpath, newpath, sizeof(newpath));
		BOOTSTRAP_IO_UNLOCK();
	    } else {
		newpath[0] = 0;
	    }

	    parse_path(sp, newpath);

#if 0
	    /*
	     * Check if we really want to load this server
	     */
	    if (!(sp->flags & LOAD_SERVER_F))
		continue;
#endif

	    BOOTSTRAP_IO_LOCK();
	    printf("%s: loading %s\n", program_name, sp->server_name);
	    BOOTSTRAP_IO_UNLOCK();

	    /*
	     * Open the file and validate its format
	     */
	    memset((char *)&f, 0, sizeof (f));
#if	PARAGON860
	    result = open_file(bootstrap_root_device_port,
			       sp->server_name, &f);
#else	/* PARAGON860 */
	    result = open_file(bootstrap_master_device_port,
			       sp->server_name, &f);
#endif	/* PARAGON860 */
	    if (result != 0) {
		BOOTSTRAP_IO_LOCK();
		printf("%s: unable to open file (result = %d)\n",
		       program_name, result);
		BOOTSTRAP_IO_UNLOCK();
		retry = TRUE;
		goto Retry_Server;
	    }
	    result = ex_get_header(&f, &ofmt);
	    close_file(&f);
	    if (result) {
		BOOTSTRAP_IO_LOCK();
		printf("%s: unloadable file format (result = 0x%x)\n",
		       program_name, result);
		BOOTSTRAP_IO_UNLOCK();
		retry = TRUE;
		goto Retry_Server;
	    }

	    /*
	     * Create a task and thread to run a server
	     * If SERVER_IN_KERNEL_F is set, TRY to collocate the
	     * server.  If it is not loadable by the kernel, then
	     * turn off the flag and load it in user-space.
	     */
	    if (!(sp->flags & SERVER_IN_KERNEL_F))
		map_init(&sp, FALSE);
	    if (collocation_autotry || (sp->flags & SERVER_IN_KERNEL_F)) {
	        if (!collocation_prohibit) {
		    if (!(sp->flags & SERVER_IN_KERNEL_F))
			map_init(&sp, TRUE);
		    if (is_kernel_loadable(sp, &ofmt))
			sp->flags |= SERVER_IN_KERNEL_F;
		    else {
			sp->flags &= ~SERVER_IN_KERNEL_F;
			map_init(&sp, FALSE);
		    }
		}
		else {
		    sp->flags &= ~SERVER_IN_KERNEL_F;
		}
		if (sp->flags & SERVER_IN_KERNEL_F) {
		    kr = kernel_task_create(bootstrap_self, sp->mapbase,
					    sp->mapsize, &user_task);
		    sp->mapend = sp->mapbase + sp->mapsize;
	        }
		else {
		    kr = task_create(bootstrap_self, (ledger_port_array_t)0, 0,
				     FALSE, &user_task);
		}
	    } else {
		    kr = task_create(bootstrap_self, (ledger_port_array_t)0, 0,
				     FALSE, &user_task);
	    }
			
	    if (kr != KERN_SUCCESS)
		panic("[kernel_]task_create %d", kr);

	    sp->task_port = user_task;
	    kr = task_set_exception_ports(user_task,
					  EXC_MASK_ALL &
					  ~(EXC_MASK_SYSCALL |
					    EXC_MASK_MACH_SYSCALL |
					    EXC_MASK_RPC_ALERT),
					  bootstrap_exception_port,
					  EXCEPTION_STATE_IDENTITY, 
					  MACHINE_THREAD_STATE);
	    if (kr != KERN_SUCCESS) {
		    BOOTSTRAP_IO_LOCK();
		    printf("%s: task_set_exception_ports returned 0x%x",
			   program_name, kr);
		    BOOTSTRAP_IO_UNLOCK();
	    }
		    
	    kr = task_set_bootstrap_port(user_task,
					 bootstrap_bootstrap_port);

	    if (kr != KERN_SUCCESS)
		panic("task_set_bootstrap_port 0x%x", kr);

	    kr = thread_create(user_task, &user_thread);
	    if (kr != KERN_SUCCESS)
		panic("thread_create 0x%x", kr);

#if 0
	    strbuild(pathname, sp->symtab_name, "_name", (char *)0);
#else
	    strcpy(pathname, sp->symtab_name);
	    strcat(pathname, "_name");
#endif
	    if (ptr = getenv(pathname))
		parse_path(sp, ptr);

#ifdef DEBUG
	    BOOTSTRAP_IO_LOCK();
	    printf("%s: Task created for server %d: %s\n",
		   program_name, i, sp->server_name);
	    BOOTSTRAP_IO_UNLOCK();
#endif	    
	    /*
	     * Load the server.
	     */
	    kr = boot_load_program(bootstrap_master_host_port,
#if	PARAGON860
				   bootstrap_root_device_port,
#else	/* PARAGON860 */
				   bootstrap_master_device_port,
#endif	/* PARAGON860 */
				   user_task,
				   user_thread,
				   sp->flags,
				   sp->symtab_name, 
				   sp->server_name,
				   sp->mapend);

	    /*
	     * If we couldn't load the server, prompt and
	     * loop around for another try.
	     */
	    if (kr != 0) {
		BOOTSTRAP_IO_LOCK();
		printf("boot_load_program failed - %d\n", kr);
		BOOTSTRAP_IO_UNLOCK();
		kr = task_terminate(user_task);
		if (kr != KERN_SUCCESS) {
		    BOOTSTRAP_IO_LOCK();
		    printf("Failure terminating task - %d\n", kr);
		    BOOTSTRAP_IO_UNLOCK();
		}
		retry = TRUE;
		goto Retry_Server;
	    }
#ifdef DEBUG
	    BOOTSTRAP_IO_LOCK();
	    printf("%s: Server loaded %d: %s\n",
		   program_name, i, sp->server_name);
	    BOOTSTRAP_IO_UNLOCK();
#endif	    

	    /*
	     *  XXX restart the service if it dies?
	     */
	    kr = mach_port_request_notification(bootstrap_self,
						sp->task_port,
						MACH_NOTIFY_DEAD_NAME,
						0,
						bootstrap_notification_port,
						MACH_MSG_TYPE_MAKE_SEND_ONCE,
						&prev_port);
	    if (kr != KERN_SUCCESS)
		panic("request_notification %d", kr);

	    /*
	     * Start up the thread
	     */
	    kr = thread_resume(user_thread);
	    if (kr != KERN_SUCCESS)
		panic("thread_resume 0x%x", kr);

	    /*
	     * If we are supposed to wait for this server to startup,
	     * spin on handling messages until we get a bootstrap_completed
	     * message.
	     */
	    if (sp->flags & SERVER_SERIALIZE_F) {
#ifdef DEBUG
		int count = 0;
		BOOTSTRAP_IO_LOCK();
		printf("%s: Serializing wait on server %s\n",
		       program_name, sp->server_name);
		BOOTSTRAP_IO_UNLOCK();
#endif
		while (!sp->bootstrap_completed) {
		    (void) mach_msg_server_once(bootstrap_demux,
						bootstrap_msg_size,
						bootstrap_set,
						MACH_MSG_OPTION_NONE);
#ifdef DEBUG
		    count++;
#endif
		}
#ifdef DEBUG
		BOOTSTRAP_IO_LOCK();
		printf("%s: Serializing wait completed; message count %d\n",
		       program_name, count);
		BOOTSTRAP_IO_UNLOCK();
#endif
	    }
	    (void) mach_port_deallocate(bootstrap_self, user_thread);

	} /* end foreach server */

	/*
	 *	Deallocate the excess send rights.
	 */
	(void) mach_port_deallocate(bootstrap_self, bootstrap_exception_port);
	(void) mach_port_deallocate(bootstrap_self, bootstrap_bootstrap_port);

	BOOTSTRAP_IO_LOCK();
	printf("%s: started\n", program_name);
	BOOTSTRAP_IO_UNLOCK();
	for (;;) {
	    kr = mach_msg_server(bootstrap_demux,
				 bootstrap_msg_size,
				 bootstrap_set,
				 MACH_MSG_OPTION_NONE);
	    BOOTSTRAP_IO_LOCK();
	    printf("%s: mach_msg_server returned 0x%x\n", program_name, kr);
	    BOOTSTRAP_IO_UNLOCK();
	    panic(my_name);
	}
}

void
parse_path(struct server *sp, const char *np)
{
	char *newpath, *p, *slash;
	const char *cp;
	const char *op, *endop;

	op = sp->server_name;
	endop = sp->server_name + sp->args_size;
	p = newpath = malloc(PATH_MAX);

	/* before parsing path, check for any per-server boot options */
	np = parse_boot_args((char**) &np, &sp);

	while (isspace(*np))
	    np++;
	if (*np == '-') {
	    while (*p++ = *op++)
		;
	} else if (*np) {
	    if (*np == '/') {
		if (strncmp(np, "/dev/", 5) != 0) {
		    cp = "/dev/boot_device";
		    while (*cp)
			*p++ = *cp++;
		}
		while (*op++)
		    ;
	    } else {
		slash = p;
		while (*p = *op++)
		    if (*p++ == '/')
			slash = p;
		if (p == slash) {
		    cp = "/dev/boot_device/mach_servers/";
		    while (*cp)
			*p++ = *cp++;
		} else
		    p = slash;
	    }
	    while (*np && !isspace(*np))
		*p++ = *np++;
	    *p++ = 0;
	    while (isspace(*np))
		np++;
	}
	if (*np == 0) {
	    if (endop - op > 0) {
		memcpy(p, op, endop - op);
		p += endop - op;
	    }
	} else {
	    for (;;) {
		while (*np && !isspace(*np)) {
		    if (*np == '\\') {
			/* Skip the backslash and take next char blindly */
			np++;
			if (!*np)	/* ... except if it doesn't exist */
			    break;
		    } else if (*np == '"') {
			/* quoted string */
			np++;
			while (*np && *np != '"') {
			    *p++ = *np++;
			}
			np++;
			continue;
		    }
		    *p++ = *np++;
		}
		while (*np && isspace(*np))
		    np++;
		if (*np == 0)
		    break;
		*p++ = 0;
	    }
	}
	sp->server_name = newpath;
	sp->args_size = p - newpath;
}

/* Parse the per-server/request bootstrap arguments */
char* 
parse_boot_args(char **data, struct server **sp)
{
    char* ptr = *data;
	unsigned int val;

    while (TRUE) {
	if (*ptr != '-')
	    return ptr;       /* Done with bootstrap args */
	else
	    ptr++;

        switch (*ptr++) {
	    case 'S':
		/* collocated server mapsize - implies -k */
		val = strtoul(ptr, &ptr, 0); 
		if (collocation_prohibit)
		    break;
		if (!((*sp)->flags & SERVER_IN_KERNEL_F)) {
		    (*sp)->flags |= SERVER_IN_KERNEL_F;
		    (void) map_init(sp, TRUE);  /* initialize kernel map info */
		}
		(*sp)->mapsize = round_page(val);
		break;
	    case 'k':
		/* collocation indicator (only - text start becomes mapbase) */
		if (collocation_prohibit)
			break;
		if (!((*sp)->flags & SERVER_IN_KERNEL_F)) {
		    (*sp)->flags |= SERVER_IN_KERNEL_F;
		    (void) map_init(sp, TRUE);  /* set kernel map info */
		}
		break;
	    case 'w':
		/* Wait for this server to finish startup before continuing.  */
		(*sp)->flags |= SERVER_SERIALIZE_F;
		break;
	    case 'd':
		/* Data request line */
		(*sp)->flags |= SERVER_DATA_F;
		break;
	    default:
		BOOTSTRAP_IO_LOCK();
		printf("%s: Unrecognized switch '-%c'\n",
		           program_name, *(ptr-1));
		BOOTSTRAP_IO_UNLOCK();
		break;
	}

	while (isspace(*ptr))
	    ptr++;
    }
}

static int
parse_config_file(char *conf, size_t size)
{
	char *pptr, *ptr = conf;
	char *endp = conf + size;
	int count = 0;
	int line;
	char c;
	struct server *sp, *spp;
	struct server s;

	while (ptr < endp) {
	    while (isspace(*ptr) && *ptr != '\n' && *ptr != '\r')
		ptr++;
	    if (*ptr == '#') {
		*ptr++ = ' ';
		while (ptr < endp && *ptr != '\n' && *ptr != '\r')
		    *ptr++ = ' ';
		if (ptr == endp)
		    break;
	    }
	    if (*ptr == '\n' || *ptr == '\r') {
		ptr++;
		continue;
	    }
	    count++;
	    while (ptr < endp && *ptr != '\n' && *ptr != '\r' && *ptr != '#')
		ptr++;
	    if (ptr == endp)
		break;
	}
	if (count == 0)
	    return 0;

	line = 1;
	servers = (struct server *) malloc(count * sizeof (struct server));
	nservers = count;
	pptr = conf;
	for (sp = servers; sp - servers < count; sp++) {

	    /* initialize server map info & bootstrap_completed flag */
	    sp->mapend = 0;
	    sp->mapbase = 0;
	    sp->mapsize = 0;
	    sp->flags = 0;
	    sp->bootstrap_completed = 0;

	    for (;;) {
		ptr = pptr;
		while (*pptr != '\n' && *ptr != '\r' && *pptr != '\0')
		    pptr++;
		c = *pptr;
		if (*pptr == '\n' || *pptr == '\r')
		    *pptr = '\0';
		while (isspace(*ptr))
		    ptr++;
		if (ptr != pptr++)
		    break;
		if (c == '\n' || c == '\r')
		    line++;
	    }

	    /* first see if any boot args exist for the server */
	    ptr = parse_boot_args(&ptr, &sp);

	    while (isspace(*ptr))
		ptr++;
	    sp->symtab_name = ptr;
	    while (*ptr && !isspace(*ptr))
		ptr++;
	    if (*ptr == '\0') {
		free(servers);
		return line;
	    }
	    *ptr++ = 0;
	    while (isspace(*ptr))
		ptr++;
	    if (*ptr == '\0' || *ptr == '-') {
		free(servers);
		return line;
	    }
	    sp->server_name = "";
	    sp->args_size = 1;
	    parse_path(sp, ptr);
	    if (c == '\n' || c == '\r')
		line++;
	}

	/*
	 * Finally isolate server names from data_device requests
	 */
	datadev = sp = &servers[nservers];
	while (sp > servers) {
	    sp--;
	    if ((sp->flags & SERVER_DATA_F) == 0)
		continue;
	    s = *sp;
	    for (spp = sp; spp < datadev; spp++)
		*spp = *(spp + 1);
	    *--datadev = s;
	    ndatadev++;
	    nservers--;
	}

	return -1;
}

/*
 *	We have three types of requests:
 *
 *		bootstrap requests from our "children"
 *		exceptions from ourself, or our "children"
 *		service requests from our "children"
 */

boolean_t
bootstrap_demux(mach_msg_header_t *in, mach_msg_header_t *out)
{
	if (in->msgh_local_port == bootstrap_bootstrap_port)
		return bootstrap_server(in, out);
	if (in->msgh_local_port == bootstrap_exception_port)
		return exc_server(in, out);
	return service_demux(in, out);
}

/*
 *	Catch exceptions.
 */

kern_return_t
catch_exception_raise(mach_port_t exception_port,
		      mach_port_t thread,
		      mach_port_t task,
		      exception_type_t exception,
		      exception_data_t codes,
		      mach_msg_type_number_t codeCnt)
{
	BOOTSTRAP_IO_LOCK();
	printf("%s: catch_exception_raise(%d)\n",
	       program_name, exception);
	BOOTSTRAP_IO_UNLOCK();
	panic(my_name);

	/* mach_msg_server will deallocate thread/task for us */

	return KERN_FAILURE;
}

kern_return_t
catch_exception_raise_state(mach_port_t port,
			    exception_type_t exc_type,
			    exception_data_t ecode,
			    mach_msg_type_number_t code_count,
			    int *flavor,
			    thread_state_t in_state,
			    mach_msg_type_number_t icnt,
			    thread_state_t out_state,
			    mach_msg_type_number_t *ocnt)
{
	BOOTSTRAP_IO_LOCK();
	printf("%s: catch_exception_raise_state(%d)\n",
	       program_name, exc_type);
	BOOTSTRAP_IO_UNLOCK();
	panic(my_name);

	/* mach_msg_server will deallocate thread/task for us */

	return KERN_FAILURE;
}

kern_return_t
catch_exception_raise_state_identity(mach_port_t port,
				     mach_port_t thread_port,
				     mach_port_t task_port,
				     exception_type_t exc_type,
				     exception_data_t code,
				     mach_msg_type_number_t code_count,
				     int *flavor,
				     thread_state_t in_state,
				     mach_msg_type_number_t icnt,
				     thread_state_t out_state,
				     mach_msg_type_number_t *ocnt)
{
	int			i;
	kern_return_t		kr;
	int			pset_id, task_id, thread_id;
	int			n_pset, n_task, n_thread;
	processor_set_name_port_array_t	pset_list;
	mach_msg_type_number_t 	pset_count;
	task_port_array_t	task_list;
	mach_msg_type_number_t	task_count;
	thread_port_array_t	thread_list;
	mach_msg_type_number_t	thread_count;

	BOOTSTRAP_IO_LOCK();
	printf("%s: task 0x%x exception %d codes",
	       program_name, task_port, exc_type);
	for (i = 0; i < code_count; i++)
		printf(" %d", code[i]);
	printf("\n");
	BOOTSTRAP_IO_UNLOCK();

	/* copy input state to output state */
	*ocnt = icnt;
	memcpy((char *) out_state, (char *) in_state, icnt * sizeof (int));
	BOOTSTRAP_IO_LOCK();
	printf("State is at 0x%x (count = %d)\n", (int) out_state, *ocnt);
	for (i = 0; i < icnt; i++) {
		if (i % 4)
			printf("\n");
		printf("  %d", in_state[i]);
	}
	printf("\n");
	BOOTSTRAP_IO_UNLOCK();

	/* try and find out more about the task and thread */
	pset_id = -1;
	task_id = -1;
	thread_id = -1;
	kr = host_processor_sets(bootstrap_master_host_port,
				 &pset_list,
				 &pset_count);
	if (kr != KERN_SUCCESS) {
		BOOTSTRAP_IO_LOCK();
		printf("host_processor_sets failed 0x%x %s\n",
		       kr, mach_error_string(kr));
		BOOTSTRAP_IO_UNLOCK();
	} else for (n_pset = 0; n_pset < pset_count; n_pset++) {
		kr = processor_set_tasks(pset_list[n_pset],
					 &task_list,
					 &task_count);
		if (kr != KERN_SUCCESS) {
			BOOTSTRAP_IO_LOCK();
			printf("processor_set_tasks failed 0x%x %s\n",
			       kr, mach_error_string(kr));
			BOOTSTRAP_IO_UNLOCK();
			continue;
		}
		for (n_task = 0; n_task < task_count; n_task++) {
			if (task_list[n_task] == task_port) {
				/* found the task */
				task_id = n_task;
				kr = task_threads(task_port,
						  &thread_list,
						  &thread_count);
				if (kr == KERN_SUCCESS) {
					BOOTSTRAP_IO_LOCK();
					printf("task_threads failed 0x%x %s\n",
					       kr, mach_error_string(kr));
					BOOTSTRAP_IO_UNLOCK();
					continue;
				}
				for (n_thread = 0; n_thread < thread_count;
				     n_thread++) {
					if (thread_list[n_thread] ==
					    thread_port) {
						/* found thread */
						thread_id = n_thread;
					}
					(void) mach_port_deallocate(
						mach_task_self(),
						thread_list[n_thread]);
				}
				(void) vm_deallocate(mach_task_self(),
						     (vm_offset_t) thread_list,
						     thread_count * 
						     sizeof (thread_list[0]));
			}
			(void) mach_port_deallocate(mach_task_self(),
						    task_list[n_task]);
		}
		(void) mach_port_deallocate(mach_task_self(),
					    pset_list[n_pset]);
		(void) vm_deallocate(mach_task_self(),
				     (vm_offset_t) task_list,
				     task_count * sizeof (task_list[0]));
	}
	(void) vm_deallocate(mach_task_self(),
			     (vm_offset_t) pset_list,
			     pset_count * sizeof (pset_list[0]));
	BOOTSTRAP_IO_LOCK();
	printf("processor_set #%d, task #%d, thread #%d\n",
	       pset_id, task_id, thread_id);
	BOOTSTRAP_IO_UNLOCK();

	panic(my_name);

	/* mach_msg_server will deallocate thread/task for us */

	return KERN_FAILURE;
}

/*
 *	Handle bootstrap requests.
 */
kern_return_t
do_bootstrap_ports(mach_port_t bootstrap,
		   mach_port_t *hostp,
		   mach_port_t *devicep,
		   mach_port_t *root_ledger_wiredp,
		   mach_port_t *root_ledger_pagedp,
		   mach_port_t *security_portp)
{
	*hostp = bootstrap_master_host_port;
	*devicep = bootstrap_master_device_port;
	*root_ledger_wiredp = root_ledger_wired;
	*root_ledger_pagedp = root_ledger_paged;
	*security_portp = security_port;
	return KERN_SUCCESS;
}

kern_return_t
do_bootstrap_arguments(mach_port_t bootstrap,
		       task_port_t task_port,
		       vm_offset_t *args,
		       mach_msg_type_number_t *args_count)
{
	kern_return_t kr;
	vm_size_t args_size;
	int i;
	struct server *sp;

	for (i = 0, sp = servers; i < nservers; i++, sp++) {
	    if (task_port == sp->task_port)
		break;
	}
	if (i == nservers)
	    return KERN_INVALID_ARGUMENT;
	args_size = round_page(sp->args_size);
	kr = vm_allocate(mach_task_self(), args, args_size, TRUE);
	if (kr != KERN_SUCCESS)
		return kr;
	memcpy((char *)(*args), (char *)sp->server_name, sp->args_size);
	*args_count = sp->args_size;
	return KERN_SUCCESS;
}

extern char **__environment;


kern_return_t
do_bootstrap_environment(mach_port_t bootstrap,
			 task_port_t task_port,
			 vm_offset_t *env,
			 mach_msg_type_number_t *env_count)
{
	kern_return_t kr;
	vm_size_t env_size, alloc_size;
	char **ep;
	char *p1, *p2;

	env_size = 0;
	for (ep = __environment; *ep; ep++)
		env_size += strlen(*ep) + 1;
	if (env_size == 0)
		alloc_size = vm_page_size;
	else
		alloc_size = round_page(env_size);
	kr = vm_allocate(mach_task_self(), env, alloc_size, TRUE);
	if (kr != KERN_SUCCESS)
		return kr;
	p1 = (char *)(*env);
	for (ep = __environment; *ep; ep++) {
		p2 = *ep;
		while (*p1++ = *p2++)
			;
	}
	*env_count = env_size;
	return KERN_SUCCESS;
}

kern_return_t
do_bootstrap_completed(mach_port_t bootstrap,
		       task_port_t task_port)
{
	unsigned i;
	struct server *sp;

	if (task_port == MACH_PORT_DEAD)
	    return KERN_INVALID_ARGUMENT;

	for (i = 0, sp = servers; i < nservers; i++, sp++) {
	    if (task_port == sp->task_port)
		break;
	}
	if (i == nservers)
	    return KERN_INVALID_ARGUMENT;
	sp->bootstrap_completed = 1;
	return KERN_SUCCESS;
}

kern_return_t
bootstrap_notify_dead_name(mach_port_t name)
{
	int i;
	struct server *sp;

	for (i = 0, sp = servers; i < nservers; i++, sp++) {
	    if (name == sp->task_port)
		break;
	}
	if (i == nservers)
	    return KERN_FAILURE;

	BOOTSTRAP_IO_LOCK();
	printf("%s: '%s' task terminated\n", program_name, sp->server_name);
	BOOTSTRAP_IO_UNLOCK();

	if (sp->flags & SERVER_SERIALIZE_F) {
	    sp->flags |= SERVER_DIED_F;
	    sp->bootstrap_completed = 1;
	}

	return KERN_SUCCESS;
}

/*
 * Code only run be the second thread of the bootstrap task
 * There is no need for any locking since the 2 threads only share
 * bootstrap ports.
 */
static void
data_device_loop()
{
    io_buf_ptr_t local_addr;
    mach_msg_type_number_t local_size;
    mach_msg_type_number_t iosize;
    io_buf_len_t bytes_wanted;
    vm_address_t addr;
    vm_size_t size;
    io_return_t kr;
    unsigned i;
    struct server *sp;
    mach_port_t device_port;

    /*
     * Initialize current cthread
     */
    cthread_set_name(cthread_self(), "data_request");
    cthread_wire();

    /*
     * Loop through all data requests
     */
    local_addr = (io_buf_ptr_t)0;
    bytes_wanted = vm_page_size;
    for (;;) {
	/*
	 * Open data_device
	 */
#if	PARAGON860
	kr = device_open(bootstrap_root_device_port, MACH_PORT_NULL,
			 D_READ|D_WRITE, bootstrap_security_token,
			 (char *)"data_device", &device_port);
#else	/* PARAGON860 */
	kr = device_open(bootstrap_master_device_port, MACH_PORT_NULL,
			 D_READ|D_WRITE, bootstrap_security_token,
			 (char *)"data_device", &device_port);
#endif	/* PARAGON860 */
	if (kr == D_NO_SUCH_DEVICE) {
	    BOOTSTRAP_IO_LOCK();
	    printf("%s: '/dev/data_device' not configured (fatal)\n",
		   data_name);
	    BOOTSTRAP_IO_UNLOCK();
	    cthread_exit(0);
	}
	if (kr != KERN_SUCCESS) {
	    BOOTSTRAP_IO_LOCK();
	    printf("%s: '/dev/data_device' open returned %d (fatal)\n",
		   data_name, kr);
	    BOOTSTRAP_IO_UNLOCK();
	    cthread_exit(0);
	}

	/*
	 * Free previous local_addr
	 */
	if (local_addr != (io_buf_ptr_t)0) {
	    kr = vm_deallocate(bootstrap_self,
			       (vm_address_t)local_addr, local_size);
	    if (kr != KERN_SUCCESS) {
		BOOTSTRAP_IO_LOCK();
		printf("%s: vm_deallocate returned %d (skipped)\n",
		       data_name, kr);
		BOOTSTRAP_IO_UNLOCK();
	    }
	    local_addr = (io_buf_ptr_t)0;
	}

	/*
	 * Get data request
	 */
	kr = device_read(device_port, 0, 0,
			 bytes_wanted, &local_addr, &local_size);
	if (kr == D_INVALID_SIZE) {
	    kr = device_close(device_port);
	    if (kr != KERN_SUCCESS)
		break;
	    bytes_wanted += vm_page_size;
	    continue;
	}
	if (kr != KERN_SUCCESS) {
	    BOOTSTRAP_IO_LOCK();
	    printf("%s: device_read returned %d (fatal)\n", data_name, kr);
	    BOOTSTRAP_IO_UNLOCK();
	    cthread_exit(0);
	}

	/*
	 * Look for the requested file
	 */
	for (i = 0, sp = datadev; i < ndatadev; i++, sp++)
	    if (!strcmp(sp->symtab_name, local_addr))
		break;
	if (i == ndatadev) {
	    BOOTSTRAP_IO_LOCK();
	    printf("%s: Unknown data request '%s' (skipped)\n",
		   data_name, local_addr);
	    BOOTSTRAP_IO_UNLOCK();
	    kr = device_close(device_port);
	    if (kr != KERN_SUCCESS)
		break;
	    continue;
	}

	/*
	 * Load requested file
	 */
	if (data_device_load_file(bootstrap_self, sp->server_name,
				  &addr, &size) != KERN_SUCCESS) {
	    kr = device_close(device_port);
	    if (kr != KERN_SUCCESS)
		break;
	    continue;
	}
	sp->bootstrap_completed = 1;
	sp->mapbase = addr;
	sp->mapsize = size;

	/*
	 * Check for null file size
	 */
	if (size == 0) {
	    BOOTSTRAP_IO_LOCK();
	    printf("%s: NULL file size '%s' (skipped)\n",
		   data_name, sp->server_name);
	    BOOTSTRAP_IO_UNLOCK();
	    kr = device_close(device_port);
	    if (kr != KERN_SUCCESS)
		break;
	    continue;
	}

	/*
	 * Write requested file into the kernel
	 */
	kr = device_write(device_port, 0, 0,
			  (io_buf_ptr_t)sp->mapbase, sp->mapsize,
			  (io_buf_len_t *)&iosize);
	if (kr != KERN_SUCCESS) {
	    BOOTSTRAP_IO_LOCK();
	    printf("%s : Device_write failed on '%s' (%d) (skipped)\n",
		   data_name, sp->server_name, kr);
	    BOOTSTRAP_IO_UNLOCK();
	    kr = device_close(device_port);
	    if (kr != KERN_SUCCESS)
		break;
	    continue;
	}

	/*
	 * Deallocate memory
	 */
	kr = vm_deallocate(bootstrap_self, sp->mapbase, sp->mapsize);
	if (kr != KERN_SUCCESS) {
	    BOOTSTRAP_IO_LOCK();
	    printf("%s: vm_deallocate returned %d (skipped)\n",
		   data_name, kr);
	    BOOTSTRAP_IO_UNLOCK();
	}

	/*
	 * Advertise kernel data request service
	 */
	BOOTSTRAP_IO_LOCK();
	printf("%s: loaded %s\n", data_name, sp->server_name);
	BOOTSTRAP_IO_UNLOCK();

	/*
	 * Close device
	 */
	kr = device_close(device_port);
	if (kr != KERN_SUCCESS)
	    break;
    }

    /*
     * Advertise error on device_close()
     */
    BOOTSTRAP_IO_LOCK();
    printf("%s: device_close returned %d (fatal)\n",
	   data_name, kr);
    BOOTSTRAP_IO_UNLOCK();
    cthread_exit(0);
}

/*
 * Load a file into the vm of the task argument
 * XXX For now, only the bootstrap_task uses it
 */
static kern_return_t
data_device_load_file(
    task_port_t task_port,
    const char *filename,
    vm_address_t *addr,
    vm_size_t *size)
{
	int result;
	kern_return_t kr;
	kern_return_t ret;
	vm_address_t faddr;
	vm_size_t fsize;
	struct file f;

	/*
	 * Open the file.
	 */
	memset((char *)&f, 0, sizeof (f));
#if	PARAGON860
	result = open_file(bootstrap_root_device_port, filename, &f);
#else	/* PARAGON860 */
	result = open_file(bootstrap_master_device_port, filename, &f);
#endif	/* PARAGON860 */
	if (result != 0)
	    return (KERN_INVALID_ARGUMENT);
	fsize = file_size(&f);
	if (fsize == 0) {
	    *size = 0;
	    *addr = 0;
	    close_file(&f);
	    return (KERN_SUCCESS);
	}

	/*
	 * Allocate the vm_region in the target task
	 */
	kr = vm_allocate(task_port, &faddr, fsize, TRUE);
	if (kr != KERN_SUCCESS) {
	    close_file(&f);
	    return (kr);
	}

	if (task_port == bootstrap_self) {
	    /*
	     * Read the file contents
	     */
	    result = read_file(&f, 0, faddr, fsize);
	    if (result) {
		ret = vm_deallocate(task_port, faddr, fsize);
		if (ret != KERN_SUCCESS) {
		    BOOTSTRAP_IO_LOCK();
		    printf("%s: vm_deallocate returned 0x%x\n",
			   data_name, ret);
		    BOOTSTRAP_IO_UNLOCK();
		}
		close_file(&f);
		return (KERN_FAILURE);
	    }

	} else {
	    vm_address_t laddr;
	    vm_size_t lsize;
	    vm_size_t count;
	    vm_size_t rsize;

	    /*
	     * Allocate the vm_region in the local task
	     */
	    lsize = fsize;
	    do {
		lsize = ((lsize +
			  vm_page_size - 1) / vm_page_size) * vm_page_size;
		kr = vm_allocate(bootstrap_self, &laddr, lsize, TRUE);
		if (kr == KERN_SUCCESS)
		    break;
		lsize = (lsize + 1) >> 1;
	    } while (vm_page_size < lsize);

	    if (kr != KERN_SUCCESS) {
		ret = vm_deallocate(task_port, faddr, fsize);
		if (ret != KERN_SUCCESS) {
		    BOOTSTRAP_IO_LOCK();
		    printf("%s: vm_deallocate returned 0x%x\n",
			   program_name, ret);
		    BOOTSTRAP_IO_UNLOCK();
		}
		close_file(&f);
		return (kr);
	    }

	    /*
	     * Read the file contents and write it to the target task vm space
	     */
	    for (count = 0; count < fsize; count += rsize) {
		rsize = (count + lsize <= fsize ? lsize : fsize - count);

		result = read_file(&f, count, laddr, rsize);
		if (result)
		    kr = KERN_FAILURE;
		else {
		    kr = vm_write(task_port, faddr + count, laddr, rsize);
		    if (kr == KERN_SUCCESS)
			continue;
		}

		ret = vm_deallocate(bootstrap_self, laddr, lsize);
		if (ret != KERN_SUCCESS) {
		    BOOTSTRAP_IO_LOCK();
		    printf("%s: vm_deallocate returned 0x%x\n",
			   program_name, ret);
		    BOOTSTRAP_IO_UNLOCK();
		}
		ret = vm_deallocate(task_port, faddr, fsize);
		if (ret != KERN_SUCCESS) {
		    BOOTSTRAP_IO_LOCK();
		    printf("%s: vm_deallocate returned 0x%x\n",
			   program_name, ret);
		    BOOTSTRAP_IO_UNLOCK();
		}
		close_file(&f);
		return (kr);
	    }

	    /*
	     * Deallocate local space
	     */
	    ret = vm_deallocate(bootstrap_self, laddr, lsize);
	    if (ret != KERN_SUCCESS) {
		BOOTSTRAP_IO_LOCK();
		printf("%s: vm_deallocate returned 0x%x\n",
		       program_name, ret);
		BOOTSTRAP_IO_UNLOCK();
	    }
	}

	/*
	 * clean up and return
	 */
	close_file(&f);
	*size = fsize;
	*addr = faddr;
	return (KERN_SUCCESS);
}
