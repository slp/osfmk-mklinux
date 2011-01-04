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

#ifndef	_MACHINE_SPL_H_
#define	_MACHINE_SPL_H_

#include <i386/ipl.h>

/*
 *	This file defines the interrupt priority levels used by
 *	machine-dependent code.
 */

typedef unsigned char		spl_t;

extern spl_t	(splhi)(void);

extern spl_t	(spl1)(void);

extern spl_t	(spl2)(void);

extern spl_t	(spl3)(void);

extern spl_t	(spl4)(void);
extern spl_t	(splhdw)(void);

extern spl_t	(spl5)(void);
extern spl_t	(spldcm)(void);

extern spl_t	(spl6)(void);

#endif	/* _MACHINE_SPL_H_ */
