/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#ifndef _ASM_OSFMACH3_I386_BUGS_H
#define _ASM_OSFMACH3_I386_BUGS_H

/*
 * This is included by init/main.c to check for architecture-dependent bugs.
 *
 * Needs:
 *	void check_bugs(void);
 */

#include <linux/config.h>

#define CONFIG_BUGosfmach3_i386

extern char x86;

static void check_bugs(void)
{
	system_utsname.machine[1] = '0' + x86;
}

#endif	/* _ASM_OSFMACH3_I386_BUGS_H */
