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

#include "bootstrap.h"
#include <mach/mach_host.h>
#include <mach/machine/vm_param.h>

/*
 * The min and max kernel address and submap size for loading the OSF/1 server
 * into the kernel.  The kernel addresses are exported by Mach.  The size
 * is big enough for an OSF/1 server that is using a 7.2MB buffer cache,
 * such as on a 36MB system.  The min address can be overridden by use of
 * -k#### and submap size can be set with -S###.
 */
#define UNIX_MAPBASE_DEFAULT	VM_MIN_KERNEL_LOADED_ADDRESS /* XXX */
#define UNIX_MAPSIZE_DEFAULT	(512 * 1024 * 1024)	/* XXX */
#define UNIX_MAPEND_DEFAULT	(VM_MIN_KERNEL_LOADED_ADDRESS + UNIX_MAPSIZE_DEFAULT) /* XXX */

extern boolean_t prompt;

void
parse_args(int argc, char **argv, char *conf_file)
{
	char *arg;

	strcpy(conf_file, "/dev/boot_device/mach_servers/bootstrap.conf");

	if (!argc)
		return;

	for (arg = (char*)argv; *arg; arg++) {
		switch(*arg) {
		case 'a':
			prompt = TRUE;
			break;			
		case 'k':
			/* Always try to load servers in kernel */
			collocation_autotry = TRUE;
			break;
		case 'u':
			/* Never try to load servers in kernel */
			collocation_prohibit = TRUE;
			break;
		default:
			BOOTSTRAP_IO_LOCK();
			printf("%s: Unrecognized switch '-%c'\n",
			       program_name, *arg);
			BOOTSTRAP_IO_UNLOCK();
			break;
		}
	}
}

void
map_init(struct server **sp, boolean_t doit)
{
	if (doit) {
		/* Initialize unix map defaults: */
		(*sp)->mapbase = UNIX_MAPBASE_DEFAULT;
		(*sp)->mapend = UNIX_MAPEND_DEFAULT;
		(*sp)->mapsize = UNIX_MAPSIZE_DEFAULT;
	}
	else {
		/* De-initialize unix map defaults: */
		(*sp)->mapbase = 0;
		(*sp)->mapend = 0;
		(*sp)->mapsize = 0;
	}
}

boolean_t
is_kernel_loadable(struct server *sp, struct objfmt *ofmt)
{
	return (strcmp(sp->symtab_name, "startup") ? FALSE : TRUE);
}
