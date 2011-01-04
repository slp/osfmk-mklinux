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
 * ANSI C defines EDOM and ERANGE.  POSIX defines the remaining values.
 * We may at some stage want to surround the extra values with
 * #ifdef _POSIX_SOURCE.
 * By an extraordinary coincidence, nearly all the values defined here
 * correspond exactly to those in OSF/1 and in Linux.  Imagine that.
 * The exception is ETIMEDOUT, which has different values in the two
 * systems.  We use the OSF/1 value here.
 */

extern int *__mach_errno_addr(void);
#define errno (*__mach_errno_addr())

#define ESUCCESS	0		/* Success */
#define EPERM		1		/* Not owner */
#define ESRCH		3		/* No such process */
#define EIO		5		/* I/O error */
#define ENOMEM		12		/* Not enough core */
#define EBUSY		16		/* Mount device busy */
#define EINVAL		22		/* Invalid argument */
#define EDOM		33		/* Argument too large */
#define ERANGE		34		/* Result too large */
#define ETIMEDOUT	60		/* Connection timed out */
