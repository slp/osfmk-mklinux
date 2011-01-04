/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

/*
 * Handy assert macro.
 */

#ifndef	_OSFMACH3_ASSERT_H_
#define _OSFMACH3_ASSERT_H_

#include <linux/autoconf.h>

#if	CONFIG_OSFMACH3_DEBUG

#include <osfmach3/macro_help.h>

#include <linux/kernel.h>

extern void Debugger(const char *);

#define ASSERT(ex)	assert(ex)
#if defined(__STDC__)
#define assert(ex)							\
MACRO_BEGIN								\
	if (!(ex)) {							\
		printk("Assertion failed: file: \"%s\", line: %d test: %s\n", \
		       __FILE__, __LINE__, # ex );			\
		Debugger("assertion failure");				\
	}								\
MACRO_END
#else	/* __STDC__ */
#define assert(ex)							\
MACRO_BEGIN								\
	if (!(ex)) {							\
		printk("Assertion failed: file: \"%s\", line: %d test: %s\n", \
		       __FILE__, __LINE__, "ex");			\
		Debugger("assertion failure");				\
	}								\
MACRO_END
#endif	/* __STDC__ */

#define assert_static(x)	assert(x)

#else	/* CONFIG_OSFMACH3_DEBUG */

#define ASSERT(ex)
#define assert(ex)
#define assert_static(ex)

#endif	/* CONFIG_OSFMACH3_DEBUG */

#endif	/* _OSFMACH3_ASSERT_H_ */
