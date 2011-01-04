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

#ifndef	_KERN_STARTUP_H_
#define	_KERN_STARTUP_H_

#include <cpus.h>

/*
 * Kernel and machine startup declarations
 */

/* Initialize kernel */
extern void	setup_main(void);

/* Initialize machine dependent stuff */
extern void	machine_init(void);

#if	NCPUS > 1

extern void	slave_main(void);

/*
 * The following must be implemented in machine dependent code.
 */

/* Slave cpu initialization */
extern void	slave_machine_init(void);

/* Start slave processors */
extern void	start_other_cpus(void);

#endif	/* NCPUS > 1 */
#endif	/* _KERN_STARTUP_H_ */
