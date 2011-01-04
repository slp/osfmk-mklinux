/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

/*
 * MACH debugging.
 */

#ifndef	_OSFMACH3_MACH3_DEBUG_H_
#define _OSFMACH3_MACH3_DEBUG_H_

#include <mach/kern_return.h>

extern void Debugger(const char *mesg);

#include <osfmach3/macro_help.h>
#include <linux/kernel.h>

extern int mach3_debug;		/* in osfmach3/mach3_debug.c */
extern void mach3_debug_msg(int level, kern_return_t retcode);

#define MACH3_DEBUG(level, retcode, args)				\
MACRO_BEGIN								\
	if (level <= mach3_debug) {					\
		printk("<lev %d> ", level);				\
		printk args;						\
		mach3_debug_msg(level, retcode);			\
	}								\
MACRO_END

#endif	/* _OSFMACH3_MACH3_DEBUG_H_ */
