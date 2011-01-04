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
 *  ETAP build options are selected using the config.debug configuration file.
 *
 *  ETAP options are:
 *	ETAP_LOCK_ACCUMULATE	- Cumulative lock tracing
 *	ETAP_LOCK_MONITOR	- Monitor lock behavior
 *	ETAP_EVENT_MONITOR	- Monitor general events
 *
 * Derived options are:
 *	ETAP_LOCK_TRACE		- Equals one if either cumulative or monitored
 *				  lock tracing is configured (zero otherwise).
 *	ETAP_MONITOR		- Equals one if either lock or event monitoring
 *				  is configured (zero otherwise).
 */

#ifndef	_KERN_ETAP_OPTIONS_H_
#define _KERN_ETAP_OPTIONS_H_

#include <etap.h>
#include <etap_lock_monitor.h>
#include <etap_lock_accumulate.h>
#include <etap_event_monitor.h>

#if	ETAP_LOCK_MONITOR || ETAP_LOCK_ACCUMULATE
#define	ETAP_LOCK_TRACE		1
#else	/* ETAP_LOCK_MONITOR || ETAP_LOCK_ACCUMULATE */
#define	ETAP_LOCK_TRACE		0		
#endif  /* ETAP_LOCK_MONITOR || ETAP_LOCK_ACCUMULATE */

#if	ETAP_LOCK_MONITOR || ETAP_EVENT_MONITOR
#define ETAP_MONITOR		1
#else	/* ETAP_LOCK_MONITOR || ETAP_EVENT_MONITOR */
#define ETAP_MONITOR		0
#endif	/* ETAP_LOCK_MONITOR || ETAP_EVENT_MONITOR */

#endif	/* _KERN_ETAP_OPTIONS_H_ */
