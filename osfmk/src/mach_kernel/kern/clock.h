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
 *	File:		kern/clock.h
 *	Purpose:	Data structures for the kernel alarm clock
 *			facility. This file is used only by kernel
 *			level clock facility routines.
 */

#ifndef	_KERN_CLOCK_H_
#define	_KERN_CLOCK_H_

#include <ipc/ipc_port.h>
#include <mach/message.h>
#include <mach/clock_types.h>
#include <kern/host.h>


/*
 * Actual clock alarm structure. Used for user clock_sleep() and
 * clock_alarm() calls. Alarms are allocated from the alarm free
 * list and entered in time priority order into the active alarm
 * chain of the target clock.
 */
struct	alarm {
	struct	alarm	*al_next;		/* next alarm in chain */
	struct	alarm	*al_prev;		/* previous alarm in chain */
	int		al_status;		/* alarm status */
	tvalspec_t	al_time;		/* alarm time */
	struct {				/* message alarm data */
		int			type;		/* alarm type */
		ipc_port_t		port;		/* alarm port */
		mach_msg_type_name_t	port_type;	/* alarm port type */
		struct	clock		*clock;		/* alarm clock */
	} al_alrm;
#define al_type		al_alrm.type
#define al_port		al_alrm.port
#define al_port_type	al_alrm.port_type
#define al_clock	al_alrm.clock
};
typedef	struct alarm	*alarm_t;
typedef struct alarm	alarm_data_t;

/* alarm status */
#define ALARM_FREE	0		/* alarm is on free list */
#define	ALARM_SLEEP	1		/* active clock_sleep() */
#define ALARM_CLOCK	2		/* active clock_alarm() */
#define ALARM_DONE	4		/* alarm has expired */

/*
 * Clock operations list structure. Contains vectors to machine
 * dependent clock routines. The routines c_config, c_init, and
 * c_gettime must be implemented for every clock device.
 */
struct	clock_ops {
	int		(*c_config)(	/* configuration */
				void);

	int		(*c_init)(	/* initialize */
				void);

	kern_return_t	(*c_gettime)(	/* get time */
				tvalspec_t	*cur_time);

	kern_return_t	(*c_settime)(	/* set time */
				tvalspec_t	*clock_time);

	kern_return_t	(*c_getattr)(	/* get attributes */
				clock_flavor_t	flavor,
				clock_attr_t	attr,
				mach_msg_type_number_t	*count);

	kern_return_t	(*c_setattr)(	/* set attributes */
				clock_flavor_t	flavor,
				clock_attr_t	attr,
				mach_msg_type_number_t	count);

	kern_return_t	(*c_maptime)(	/* map time */
				ipc_port_t	*pager);

	void		(*c_setalrm)(	/* set next alarm */
				tvalspec_t	*alarm_time);
};
typedef struct clock_ops *clock_ops_t;
typedef struct clock_ops clock_ops_data_t;

/*
 * Actual clock object data structure. Contains the machine
 * dependent operations list, clock operations ports, and a
 * chain of pending alarms.
 */
struct	clock {
	clock_ops_t	cl_ops;		/* operations list */
	struct ipc_port	*cl_service;	/* service port */
	struct ipc_port	*cl_control;	/* control port */
	struct	{			/* alarm chain head */
		struct alarm *al_next;
	} cl_alarm;
};
typedef	struct clock	*clock_t;
typedef struct clock	clock_data_t;
#define CLOCK_NULL	(clock_t)0

/*
 * Configure the clock system.
 */
extern void		clock_config(void);
/*
 * Initialize the clock system.
 */
extern void		clock_init(void);

/*
 * Initialize the clock ipc service facility.
 */
extern void		clock_service_create(void);

/*
 * Service clock alarm interrupts. Called from machine dependent
 * layer at splclock(). The clock_id argument specifies the clock,
 * and the clock_time argument gives that clock's current time.
 */
extern void		clock_alarm_intr(
				clock_id_t	clock_id,
				tvalspec_t	*clock_time);


/*
 * The calling thread is suspended until the requested time.
 */
extern kern_return_t	clock_sleep(
				clock_t		clock,
				sleep_type_t	sleep_type,
				tvalspec_t	*sleep_time,
				simple_lock_t	lock);

#endif	/* _KERN_CLOCK_H_ */
