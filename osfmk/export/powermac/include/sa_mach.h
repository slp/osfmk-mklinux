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

#ifndef _SA_MACH_H_
#define	_SA_MACH_H_

#include <mach.h>

/*
 * Prototypes defined in libsa_mach.a
 */

#if __STDC__ || defined(__cplusplus)
#define __(args)        args
#else /* __STDC__ || defined(__cplusplus) */
#define __(args)        ()
#endif /* __STDC__ || defined(__cplusplus) */

#if defined(__cplusplus)
extern "C" {
#endif /* defined(__cplusplus) */

extern int      isascii __((int));
extern int      toascii __((int));
extern void	printf_init __((mach_port_t));
extern void	_exit __((int));
extern void	sleep(int);
extern void	usleep(int);

#if defined(__cplusplus)
}
#endif /* defined(__cplusplus) */

#endif /* _SA_MACH_H_ */
