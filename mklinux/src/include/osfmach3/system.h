/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#ifndef __OSFMACH3_SYSTEM_H
#define __OSFMACH3_SYSTEM_H

#include <asm/segment.h>

#define sti()			while (0)
#define cli()			while (0)
#define save_flags(x)		(x) = 0
#define restore_flags(x)	(x) = 0

#endif	/* __OSFMACH3_SYSTEM_H */
