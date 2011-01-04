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
 * Copyright (c) 1992 The University of Utah and
 * the Center for Software Science (CSS).  All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * THE UNIVERSITY OF UTAH AND CSS ALLOW FREE USE OF THIS SOFTWARE IN ITS "AS
 * IS" CONDITION.  THE UNIVERSITY OF UTAH AND CSS DISCLAIM ANY LIABILITY OF
 * ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * CSS requests users of this software to return to css-dist@cs.utah.edu any
 * improvements that they make and grant CSS redistribution rights.
 *
 *      Utah $Hdr$
 */

/*
 *	Autoconfigure devices
 */

#include <types.h>
#include <machine/asp.h>
#include <machine/pdc.h>
#include <device/conf.h>
#include <device/ds_routines.h>
#include <hp_pa/HP700/device.h>
#include <hp_pa/boot.h>

#include <string.h>
#include <hp_pa/HP700/cons.h>
#include <kern/misc_protos.h>
#include <hp_pa/misc_protos.h>

void
autoconf(void)
{
	extern int pdc_alive;

	ioconf();
	/*
	 * Turn off PDC console before turning on serial console.
	 */
	pdc_alive = 0;
	cninit();
}

char root_name[8] = "sd6a";
#ifdef PDCDUMP
char dump_name[8] = "sd6d";
#endif

extern int boothowto;

void
get_root_device(void)
{
    dev_ops_t boot_dev_ops;
    int unit;
    extern struct device_path boot_dp;
    boolean_t ask = (boothowto & RB_ASKNAME);
    int dev;
    
   dev = boot_dp.dp_layers[0];
   if (dev > 9) {
	root_name[2] = '0' + (dev / 10);
	root_name[3] = '0' + (dev % 10);
	root_name[4] = 'a';
   } else
        root_name[2] = '0' + dev;
		
#ifdef PDCDUMP
   if (dev > 9) {
	dump_name[2] = '0' + (dev / 10);
	dump_name[3] = '0' + (dev % 10);
	dump_name[4] = 'd';
   } else
        dump_name[2] = '0' + dev;
#endif

    for (;;)
    {
	char name[8];
	dev_ops_t boot_dev_ops;

	if(ask)
	{
	    printf("root device [%s]? ", root_name);
	    safe_gets(name, sizeof name);
	    if (*name != 0)
		strcpy(root_name, name);
	}

	if(dev_name_lookup(root_name, &boot_dev_ops, &unit))
	{
	    dev_set_indirection("boot_device", boot_dev_ops, unit);
	    break;
	}
	ask = TRUE;
    }

    printf("root on %s\n", root_name);
}

static void
get_howto_flags(char **str)
{
	register char *cp;
	register int	bflag;
	cp = *str;
	*cp++ = '-';

	bflag = boothowto;

	if (bflag & RB_ASKNAME)
	    *cp++ = 'q';
	if (bflag & RB_SINGLE)
	    *cp++ = 's';
#if	MACH_KDB
	if (bflag & RB_KDB)
	    *cp++ = 'd';
#endif	/* MACH_KDB */
	if (bflag & RB_HALT)
	    *cp++ = 'h';
	if (bflag & RB_INITNAME)
	    *cp++ = 'n';
	if (bflag & RB_ALTBOOT)
	    *cp++ = 'v';

	if (cp > (*str) + 1) {		/* any flags ? */
		*cp = 0;
		*str = cp;
	}
}

char *
machine_boot_info(
	char		*buf,
	vm_size_t	buf_len)
{
	char *p = buf;
	char *endp = p + buf_len - 1;
	char *q;
	vm_size_t len;
	extern char boot_string[];

	/*
	 * First copy the root name
	 */
	q = root_name;
	while (p < endp && *q != 0)
		*p++ = *q++;
	*p++ = ' ';
	*p = 0;

	/*
	 * Get the boothowto flags
	 */
	get_howto_flags(&p);

	/*
	 * Get the remaining flags
	 */
	len = strlen(boot_string);
	if (len != 0) {
		if (*(p - 1) != ' ')
			*p++ = ' ';
		q = boot_string;
		while (p < endp && *q != 0)
			*p++ = *q++;
	} else {
		if (*(p - 1) == ' ')
			*(p - 1) = 0;
	}

	return(buf);
}


