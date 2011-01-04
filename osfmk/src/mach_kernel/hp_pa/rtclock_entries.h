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

#include <types.h>

extern int		rtc_config(void);
extern int		rtc_init(void);
extern kern_return_t	rtc_gettime(
				tvalspec_t		* curtime);
extern void		rtc_gettime_interrupts_disabled(
				tvalspec_t		* curtime);
extern kern_return_t	rtc_settime(
				tvalspec_t		* curtime);
extern kern_return_t	rtc_getattr(
				clock_flavor_t		flavor,
				clock_attr_t		ttr,
				mach_msg_type_number_t	* count);
extern kern_return_t	rtc_setattr(
				clock_flavor_t		flavor,
				clock_attr_t		ttr,
				mach_msg_type_number_t	count);
extern kern_return_t	rtc_maptime(
				ipc_port_t		* pager);
extern void		rtc_setalrm(
				tvalspec_t		* alarmtime);
extern void		rtclock_reset(void);
extern vm_offset_t	rtclock_map(
				dev_t			dev,
				vm_offset_t		off,
				int			prot);





