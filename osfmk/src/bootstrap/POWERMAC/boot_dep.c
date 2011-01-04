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
 * Copyright (c) 1991,1990,1989, 1988 Carnegie Mellon University
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

#include "bootstrap.h"
#include <mach/mach_host.h>
#include <mach/machine/vm_param.h>


extern boolean_t prompt;

/*
 * Is c part of a word?  (Newline is a delimiter, to cope with
 * bootmagic strings appended to normal boot_info by NORMA
 * kernel.)
 */
#define is_wordchar(c)	((c) != '\0' && (c) != '\n' && (c) != ' ')

/*
 * The min and max kernel address and submap size for loading the OSF/1 server
 * into the kernel.  The kernel addresses are exported by Mach.  The size
 * is big enough for an OSF/1 server that is using a 7.2MB buffer cache,
 * such as on a 36MB system.  The min address can be overridden by use of
 * -k#### and submap size can be set with -S###.
 */
#define UNIX_MAPBASE_DEFAULT	VM_MIN_KERNEL_LOADED_ADDRESS /* XXX */
#define UNIX_MAPEND_DEFAULT	VM_MAX_KERNEL_LOADED_ADDRESS /* XXX */
#define UNIX_MAPSIZE_DEFAULT	(48 * 1024 * 1024)	/* XXX */

/*
 * For the POWERMAC, we ask the kernel for the complete boot string
 */
#define DEFAULT_CONF_DIRECTORY "/dev/boot_device/mach_servers/"
#define DEFAULT_BOOT_FILE "bootstrap.conf"

extern boolean_t debug;
void
parse_args(int argc, char **argv, char *conf_file)
{
	char *arg, ch;
	int conf_file_set = 0;

	while (argc > 1 && *(arg = argv[1]) == '-') {
		switch (ch = *++arg) {
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
		case 'f':
			/* Requires an argument.  */
			if (argc <= 1) {
				printf("%s: -f switch lacks argument.\n",
				       program_name);
				break;
			}

			++argv; --argc;
			/* Use a different bootstrap.conf.  */
			if (*argv[1] == '/') {
				/* Full path name provided; don't do any
				   defaulting.  */
				strcpy(conf_file, argv[1]);
			} else {
				/* Path is relative to default directory.  */
				strcpy(conf_file, DEFAULT_CONF_DIRECTORY);
				strcat(conf_file, argv[1]);
			}
			conf_file_set = 1;
			break;
		case '-':
			return;
		default:
			printf("%s: Unrecognized switch '-%c'\n",
			       program_name, ch);
			break;
		}
		argv++;
		argc--;
	}
	if (!conf_file_set) {
		/* Default bootstrap configuration.  */
		strcpy(conf_file, DEFAULT_CONF_DIRECTORY);
		strcat(conf_file, DEFAULT_BOOT_FILE);
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

/*
 * Uses code similar to boot_load_program, but merely determines whether
 * or not the file is kernel-loadable, i.e. is linked at legal kernel
 * addresses.  Will become obsolete when we can relocate servers at load
 * time.
 *
 * Updates mapbase.
 */
boolean_t
is_kernel_loadable(struct server *sp, struct objfmt *ofmt)
{
        vm_offset_t	text_page_start;

	text_page_start = trunc_page(ofmt->info.text_start);
	if (text_page_start < sp->mapbase ||
	    text_page_start >= sp->mapend)
		return (FALSE);

	/* By default, load it at the base address it was linked for */
	sp->mapbase = text_page_start;
	return (TRUE);
}
