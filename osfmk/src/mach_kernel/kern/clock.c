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
 *	File:		kern/clock.c
 *	Purpose:	Routines for the creation and use of kernel
 *			alarm clock services. This file and the ipc
 *			routines in kern/ipc_clock.c constitute the
 *			machine-independent clock service layer.
 */

#include <cpus.h>
#include <mach_host.h>

#include <mach/boolean.h>
#include <mach/policy.h>
#include <mach/processor_info.h>
#include <mach/vm_param.h>
#include <machine/mach_param.h>
#include <kern/cpu_number.h>
#include <kern/misc_protos.h>
#include <kern/lock.h>
#include <kern/host.h>
#include <kern/processor.h>
#include <kern/sched.h>
#include <kern/spl.h>
#include <kern/thread.h>
#include <kern/thread_swap.h>
#include <kern/ipc_host.h>
#include <kern/clock.h>
#include <kern/zalloc.h>
#include <ipc/ipc_port.h>
#include <vm/vm_kern.h>		/* for kernel_map */
#include <device/dev_hdr.h>	/* for device_t */
#include <mach/mach_syscalls.h>

#include <mach/clock_reply.h>

/*
 * Exported interface
 */

#include <mach/clock_server.h>
#include <mach/mach_host_server.h>


/* local data declarations */
decl_simple_lock_data(,ClockLock)	/* clock system synchronization */
struct	zone		*alarm_zone;	/* zone for user alarms */
struct	alarm		*alrmfree;	/* alarm free list pointer */
struct	alarm		*alrmdone;	/* alarm done list pointer */

/* backwards compatibility */
int             hz = HZ;                /* GET RID OF THIS !!! */
int             tick = (1000000 / HZ);  /* GET RID OF THIS !!! */

/* external declarations */
extern	clock_t		port_name_to_clock(mach_port_t clock_name);
extern	struct clock	clock_list[];
extern	int		clock_count;

/* local clock subroutines */
void	FlushAlarms(
		clock_t		clock);

void	PostAlarm(
		clock_t		clock,
		alarm_t		alarm);

int	CheckTime(
		alarm_type_t	alarm_type,
		tvalspec_t	*alarm_time,
		tvalspec_t	*clock_time);

void	alarm_thread(void);
void	alarm_thread_continue(void);

/*
 *	Macros to lock/unlock clock system.
 */
#define LOCK_CLOCK(s)			\
	s = splclock();			\
	simple_lock(&ClockLock);

#define UNLOCK_CLOCK(s)			\
	simple_unlock(&ClockLock);	\
	splx(s);

/*
 * Configure the clock system. (Not sure if we need this,
 * as separate from clock_init()).
 */
void
clock_config(
	void)
{
	clock_t		clock;
	register int 	i;

	/*
	 * Configure clock devices.
	 */
	for (i = 0; i < clock_count; i++) {
		clock = &clock_list[i];
		if (clock->cl_ops) {
			if ((*clock->cl_ops->c_config)() == 0)
				clock->cl_ops = 0;
		}
	}
}

/*
 * Initialize the clock system.
 */
void
clock_init(
	void)
{
	clock_t		clock;
	register int	i;

	/*
	 * Initialize basic clock structures.
	 */
	simple_lock_init(&ClockLock, ETAP_MISC_CLOCK);
	for (i = 0; i < clock_count; i++) {
		clock = &clock_list[i];
		if (clock->cl_ops)
			(*clock->cl_ops->c_init)();
	}
}

/*
 * Initialize the clock ipc service facility.
 */
void
clock_service_create(
	void)
{
	clock_t		clock;
	register int	i;

	/*
	 * Initialize ipc clock services.
	 */
	for (i = 0; i < clock_count; i++) {
		clock = &clock_list[i];
		if (clock->cl_ops) {
			ipc_clock_init(clock);
			ipc_clock_enable(clock);
		}
	}

	/*
	 * Initialize the clock service alarm thread.
	 */
	(void) kernel_thread(kernel_task, alarm_thread, (char *) 0);

        /*
         * Initialize clock service alarms.
         */
	i = sizeof(struct alarm);
	alarm_zone = zinit(i, (4096/i)*i, 10*i, "alarms");
}

/*
 * Get the service port on a clock.
 */
kern_return_t
host_get_clock_service(
	host_t		host,
	clock_id_t	clock_id,
	clock_t		*clock)		/* OUT */
{
	if (host == HOST_NULL || clock_id < 0 || clock_id >= clock_count) {
		*clock = CLOCK_NULL;
		return (KERN_INVALID_ARGUMENT);
	}

	*clock = &clock_list[clock_id];
	if ((*clock)->cl_ops == 0)
		return (KERN_FAILURE);
	return (KERN_SUCCESS);
}

/*
 * Get the control port on a clock.
 */
kern_return_t
host_get_clock_control(
	host_t		host,
	clock_id_t	clock_id,
	clock_t		*clock)		/* OUT */
{
	if (host == HOST_NULL || clock_id < 0 || clock_id >= clock_count) {
		*clock = CLOCK_NULL;
		return (KERN_INVALID_ARGUMENT);
	}

	*clock = &clock_list[clock_id];
	if ((*clock)->cl_ops == 0)
		return (KERN_FAILURE);
	return (KERN_SUCCESS);
}

/*
 * Get the current clock time (syscall trap version).
 */
kern_return_t
syscall_clock_get_time(
	mach_port_t	clock_name,
	tvalspec_t	*cur_time)	/* OUT */
{
	clock_t		clock;
	tvalspec_t	swtime;
	kern_return_t	rvalue;

	/*
	 * Convert the trap parameters.
	 */
	clock = port_name_to_clock(clock_name);

	/*
	 * Call the actual clock_get_time routine.
	 */
	rvalue = clock_get_time(clock, &swtime);

	/*
	 * Return the current time.
	 */
	if (rvalue == KERN_SUCCESS) {
		copyout((char *)&swtime, (char *)cur_time,
			sizeof(tvalspec_t));
	}
	return (rvalue);
}

/*
 * Get the current clock time.
 */
kern_return_t
clock_get_time(
	clock_t		clock,
	tvalspec_t	*cur_time)	/* OUT */
{
	if (clock == CLOCK_NULL)
		return (KERN_INVALID_ARGUMENT);
	return ((*clock->cl_ops->c_gettime)(cur_time));
}

/*
 * Get clock attributes.
 */
kern_return_t
clock_get_attributes(
	clock_t			clock,
	clock_flavor_t		flavor,
	clock_attr_t		attr,		/* OUT */
	mach_msg_type_number_t	*count)		/* IN/OUT */
{
	kern_return_t	(*getattr)(
				clock_flavor_t	flavor,
				clock_attr_t	attr,
				mach_msg_type_number_t	*count);

	if (clock == CLOCK_NULL)
		return (KERN_INVALID_ARGUMENT);
	if (getattr = clock->cl_ops->c_getattr)
		return((*getattr)(flavor, attr, count));
	else
		return (KERN_FAILURE);
}

/*
 * Set the current clock time.
 */
kern_return_t
clock_set_time(
	clock_t		clock,
	tvalspec_t	new_time)
{
	tvalspec_t	*clock_time;
	kern_return_t	(*settime)(tvalspec_t *clock_time);

	if (clock == CLOCK_NULL)
		return (KERN_INVALID_ARGUMENT);
	if ((settime = clock->cl_ops->c_settime) == 0)
		return (KERN_FAILURE);
	clock_time = &new_time;
	if (BAD_TVALSPEC(clock_time))
		return (KERN_INVALID_VALUE);

	/*
	 * Flush all outstanding alarms.
	 */
	FlushAlarms(clock);

	/*
	 * Set the new time.
	 */
	return ((*settime)(clock_time));
}

/*
 * Set the clock alarm resolution.
 */
kern_return_t
clock_set_attributes(
	clock_t			clock,
	clock_flavor_t		flavor,
	clock_attr_t		attr,
	mach_msg_type_number_t	count)
{
	kern_return_t	(*setattr)(
				clock_flavor_t	flavor,
				clock_attr_t	attr,
				mach_msg_type_number_t	count);

	if (clock == CLOCK_NULL)
		return (KERN_INVALID_ARGUMENT);
	if (setattr = clock->cl_ops->c_setattr)
		return ((*setattr)(flavor, attr, count));
	else
		return (KERN_FAILURE);
}

/*
 * Map the clock time.
 */
kern_return_t
clock_map_time(
	clock_t		clock,
	ipc_port_t	*pager)		/* OUT */
{
	kern_return_t	(*maptime)(
				ipc_port_t *pager);

	if (clock == CLOCK_NULL)
		return (KERN_INVALID_ARGUMENT);
	if (maptime = clock->cl_ops->c_maptime)
		return ((*maptime)(pager));
	else
		return (KERN_FAILURE);
}

/*
 * Setup a clock alarm.
 */
kern_return_t
clock_alarm(
	clock_t			clock,
	alarm_type_t		alarm_type,
	tvalspec_t		alarm_time,
	ipc_port_t		alarm_port,
	mach_msg_type_name_t	alarm_port_type)
{
	int			chkstat;
	spl_t			s;
	alarm_t			alarm;
	tvalspec_t		clock_time;
	kern_return_t		reply_code;

	if (clock == CLOCK_NULL)
		return (KERN_INVALID_ARGUMENT);
	if (clock->cl_ops->c_setalrm == 0)
		return (KERN_FAILURE);
	if (IP_VALID(alarm_port) == 0)
		return (KERN_INVALID_CAPABILITY);

	/*
	 * Check alarm parameters. If parameters are invalid,
	 * send alarm message immediately.
	 */
	(*clock->cl_ops->c_gettime)(&clock_time);
	chkstat = CheckTime(alarm_type, &alarm_time, &clock_time);
	if (chkstat <= 0) {
		reply_code = (chkstat < 0 ? KERN_INVALID_VALUE : KERN_SUCCESS);
		clock_alarm_reply(alarm_port, alarm_port_type,
				  reply_code, alarm_type, clock_time);
		return (KERN_SUCCESS);
	}

	/*
	 * Get alarm and add to clock alarm list.
	 */

	LOCK_CLOCK(s);
	if ((alarm = alrmfree) == 0) {
		UNLOCK_CLOCK(s);
		alarm = (alarm_t) zalloc(alarm_zone);
		if (alarm == 0)
			return (KERN_RESOURCE_SHORTAGE);
		LOCK_CLOCK(s);
	}
	else
		alrmfree = alarm->al_next;

	alarm->al_status = ALARM_CLOCK;
	alarm->al_time = alarm_time;
	alarm->al_type = alarm_type;
	alarm->al_port = alarm_port;
	alarm->al_port_type = alarm_port_type;
	alarm->al_clock = clock;
	PostAlarm(clock, alarm);
	UNLOCK_CLOCK(s);

	return (KERN_SUCCESS);
}

boolean_t	delayed_rt_crash;
long		cs_stamp[2];

#if 0
extern void	cyctm05_stamp(long *);
#endif
extern void	log_thread_action(thread_t, char *);

/*
 * Sleep on a clock. System trap. User-level libmach clock_sleep
 * interface call takes a tvalspec_t sleep_time argument which it
 * converts to sleep_sec and sleep_nsec arguments which are then
 * passed to clock_sleep_trap.
 */
kern_return_t
clock_sleep_trap(
	mach_port_t	clock_name,
	sleep_type_t	sleep_type,
	int		sleep_sec,
	int		sleep_nsec,
	tvalspec_t	*wakeup_time)
{
	clock_t		clock;
	tvalspec_t	swtime;
	kern_return_t	rvalue;
	tvalspec_t	curtime, temp;

	/*
	 * Convert the trap parameters.
	 */
	clock = port_name_to_clock(clock_name);
	swtime.tv_sec  = sleep_sec;
	swtime.tv_nsec = sleep_nsec;

	temp = swtime;

	/*
	 * Call the actual clock_sleep routine.
	 */
	rvalue = clock_sleep(clock, sleep_type, &swtime, 0);

	/*
	 * Return current time as wakeup time.
	 */
	if (rvalue != KERN_INVALID_ARGUMENT && rvalue != KERN_FAILURE) {
		(*clock->cl_ops->c_gettime)(&curtime);
		SUB_TVALSPEC (&curtime, &temp);
#if 0
		log_thread_action (current_thread(),
				"clock_sleep_trap returned");

		if (delayed_rt_crash && curtime.tv_sec >= 0 &&
				curtime.tv_nsec >= 2000000) {
			panic ("clock_sleep_trap: delayed wakeup %d",
				curtime.tv_nsec);
		}
#endif
		copyout((char *)&swtime, (char *)wakeup_time,
			sizeof(tvalspec_t));
	}
	return (rvalue);
}	

/*
 * Kernel internally callable clock sleep routine. The calling
 * thread is suspended until the requested sleep time is reached.
 * If a lock is passed, it is released just prior to blocking.
 */
kern_return_t
clock_sleep(
	clock_t		clock,
	sleep_type_t	sleep_type,
	tvalspec_t	*sleep_time,
	simple_lock_t	lock)
{
	int		chkstat;
	spl_t		s;
	alarm_t		alarm;
	tvalspec_t	clock_time;
	kern_return_t	rvalue;

	if (clock == CLOCK_NULL)
		return (KERN_INVALID_ARGUMENT);
	if (clock->cl_ops->c_setalrm == 0)
		return (KERN_FAILURE);

	/*
	 * Check sleep parameters. If parameters are invalid
	 * return an error, otherwise post alarm request.
	 */
	(*clock->cl_ops->c_gettime)(&clock_time);
#if 0
	cyctm05_stamp(&cs_stamp[0]);
	cs_stamp[1] += 100000;		/* HACK - 100ms */
#endif
	chkstat = CheckTime(sleep_type, sleep_time, &clock_time);
	if (chkstat < 0)
		return (KERN_INVALID_VALUE);
	rvalue = KERN_SUCCESS;
	if (chkstat > 0) {
		/*
		 * Get alarm and add to clock alarm list.
		 */

		LOCK_CLOCK(s);
		if ((alarm = alrmfree) == 0) {
			UNLOCK_CLOCK(s);
			alarm = (alarm_t) zalloc(alarm_zone);
			if (alarm == 0)
				return (KERN_RESOURCE_SHORTAGE);
			LOCK_CLOCK(s);
		}
		else
			alrmfree = alarm->al_next;

		alarm->al_time = *sleep_time;
		alarm->al_status = ALARM_SLEEP;
		PostAlarm(clock, alarm);

		/*
		 * Wait for alarm to occur.
		 */
		assert_wait((event_t)alarm, TRUE);
		UNLOCK_CLOCK(s);
		if (lock)
			simple_unlock(lock);
		/* should we force spl(0) at this point? */
		thread_block((void (*)(void)) 0);
		/* we should return here at ipl0 */

		/*
		 * Note if alarm expired normally or whether it
		 * was aborted. If aborted, delete alarm from
		 * clock alarm list. Return alarm to free list.
		 */
		LOCK_CLOCK(s);
		if (alarm->al_status != ALARM_DONE) {
			/* This means we were interrupted and that
			   thread->wait_result != THREAD_AWAKENED. */
			if ((alarm->al_prev)->al_next = alarm->al_next)
				(alarm->al_next)->al_prev  = alarm->al_prev;
			rvalue = KERN_ABORTED;
		}
		*sleep_time = alarm->al_time;
		alarm->al_status = ALARM_FREE;
		alarm->al_next = alrmfree;
		alrmfree = alarm;
		UNLOCK_CLOCK(s);
	}
	return (rvalue);
}


/*
 * CLOCK INTERRUPT SERVICE ROUTINES.
 */

/*
 * Service clock alarm interrupts. Called from machine dependent
 * layer at splclock(). The clock_id argument specifies the clock,
 * and the clock_time argument gives that clock's current time.
 */
void
clock_alarm_intr(
	clock_id_t	clock_id,
	tvalspec_t	*clock_time)
{
	clock_t			clock;
	register alarm_t	alrm1;
	register alarm_t	alrm2;
	tvalspec_t		*alarm_time;
	tvalspec_t		temp1, temp2;
	spl_t			s;

	clock = &clock_list[clock_id];

	/*
	 * Update clock alarm list. All alarms that are due are moved
	 * to the alarmdone list to be serviced by the alarm_thread.
	 */

	LOCK_CLOCK(s);
	alrm1 = (alarm_t) &clock->cl_alarm;
	while (alrm2 = alrm1->al_next) {
		alarm_time = &alrm2->al_time;
		if (CMP_TVALSPEC(alarm_time, clock_time) > 0)
			break;

		/*
		 * Alarm has expired, so remove it from the
		 * clock alarm list.
		 */  
		if (alrm1->al_next = alrm2->al_next)
			(alrm1->al_next)->al_prev = alrm1;

		/*
		 * If a clock_sleep() alarm, wakeup the thread
		 * which issued the clock_sleep() call.
		 */
		if (alrm2->al_status == ALARM_SLEEP) {
			alrm2->al_next = 0;
			alrm2->al_status = ALARM_DONE;
			temp2 = alrm2->al_time;
			(*clock->cl_ops->c_gettime)(&alrm2->al_time);
			temp1 = alrm2->al_time;

#if 0
			SUB_TVALSPEC (&temp1, &temp2);
			if (delayed_rt_crash && temp1.tv_sec >= 0 &&
					temp1.tv_nsec >= 1500000) {
				panic ("clock_sleep_intr: delayed wakeup");
			}
#endif
			thread_wakeup((event_t)alrm2);
		}

 		/*
		 * If a clock_alarm() alarm, place the alarm on
		 * the alarm done list and wakeup the dedicated
		 * kernel alarm_thread to service the alarm.
		 */
		else {
			assert(alrm2->al_status == ALARM_CLOCK);
			if (alrm2->al_next = alrmdone)
				alrmdone->al_prev = alrm2;
			else
				thread_wakeup((event_t)&alrmdone);
			alrm2->al_prev = (alarm_t) &alrmdone;
			alrmdone = alrm2;
			alrm2->al_status = ALARM_DONE;
		}
	}

	/*
	 * Setup the clock dependent layer to deliver another
	 * interrupt for the next pending alarm.
	 */
	if (alrm2)
		(*clock->cl_ops->c_setalrm)(alarm_time);
	UNLOCK_CLOCK(s);
}


/*
 * ALARM THREAD ROUTINES.
 */

/*
 * Setup routine for alarm servicing thread. The alarm thread
 * is used to service the alarm done list.
 */
void
alarm_thread(void)
{
    {
	thread_t			thread;
	processor_set_t			pset;
        kern_return_t                   ret;
        policy_base_t                   base;
        policy_limit_t                  limit;
        policy_fifo_base_data_t	        fifo_base;
        policy_fifo_limit_data_t        fifo_limit;
	extern void vm_page_free_reserve(int pages);

        /*
         * Set thread privileges.
         */
	thread = current_thread();
        thread->vm_privilege = TRUE;
	vm_page_free_reserve(5);	/* XXX */
        stack_privilege(thread);
	thread_swappable(current_act(), FALSE);

	/*
	 * Set thread scheduling priority and policy.
	 */
	pset = thread->processor_set;
        base = (policy_base_t) &fifo_base;
        limit = (policy_limit_t) &fifo_limit;
        fifo_base.base_priority = 0;
        fifo_limit.max_priority = 0;
        ret = thread_set_policy(thread->top_act, pset, POLICY_FIFO, 
				base, POLICY_FIFO_BASE_COUNT,
				limit, POLICY_FIFO_LIMIT_COUNT);
        if (ret != KERN_SUCCESS)
                printf("WARNING: alarm_thread is being TIMESHARED!\n");
    }

	/*
	 * Run actual alarm_thread() code
	 *
	 * Alarm servicing thread continuation routine. Used to service
	 * the alarm done list and deliver messages to ports for expired
	 * clock_alarm() calls.
	 */
    {
	register alarm_t	alrm;
	tvalspec_t		wakeup_time;
	kern_return_t		code;
	spl_t			s;

#if	MACH_ASSERT
	if (watchacts & WA_BOOT)
	    printf("alarm_thread RUNNING\n");
#endif	/* MACH_ASSERT */

	for (;;) {
	    LOCK_CLOCK(s);
	    while (alrm = alrmdone) {
		if (alrmdone = alrm->al_next)
			alrmdone->al_prev = (alarm_t) &alrmdone;
		UNLOCK_CLOCK(s);
		(*(alrm->al_clock)->cl_ops->c_gettime)(&wakeup_time);
		code = (alrm->al_status == ALARM_DONE ? KERN_SUCCESS :
			KERN_ABORTED);
		if (IP_VALID(alrm->al_port)) {
			clock_alarm_reply(alrm->al_port, alrm->al_port_type,
					  code, alrm->al_type, wakeup_time);
		}
		LOCK_CLOCK(s);
		alrm->al_status = ALARM_FREE;
		alrm->al_next = alrmfree;
		alrmfree = alrm;
	    }
	    assert_wait((event_t)&alrmdone, FALSE);
	    UNLOCK_CLOCK(s);
	    thread_block((void (*)(void)) 0);
	}
    }
}


/*
 * CLOCK PRIVATE SERVICING SUBROUTINES.
 */

/*
 * Flush all pending alarms on a clock. All alarms
 * are activated and timestamped correctly, so any
 * programs waiting on alarms/threads will proceed
 * with accurate information.
 */
void
FlushAlarms(
	clock_t		clock)
{
	spl_t		s;
	alarm_t		alrm1;
	alarm_t		alrm2;

	/*
	 * Flush all outstanding alarms.
	 */

	LOCK_CLOCK(s);
	alrm1 = (alarm_t) &clock->cl_alarm;
	while (alrm2 = alrm1->al_next) {
		/*
		 * Remove alarm from the clock alarm list.
		 */  
		if (alrm1->al_next = alrm2->al_next)
			(alrm1->al_next)->al_prev = alrm1;

		/*
		 * If a clock_sleep() alarm, wakeup the thread
		 * which issued the clock_sleep() call.
		 */
		if (alrm2->al_status == ALARM_SLEEP) {
			alrm2->al_next = 0;
			thread_wakeup((event_t)alrm2);
		}
 		/*
		 * If a clock_alarm() alarm, place the alarm on
		 * the alarm done list and wakeup the dedicated
		 * kernel alarm_thread to service the alarm.
		 */
		else {
			assert(alrm2->al_status == ALARM_CLOCK);
			if (alrm2->al_next = alrmdone)
				alrmdone->al_prev = alrm2;
			else
				thread_wakeup((event_t)&alrmdone);
			alrm2->al_prev = (alarm_t) &alrmdone;
			alrmdone = alrm2;
		}
	}
	UNLOCK_CLOCK(s);
}

/*
 * Post an alarm on a clock's active alarm list. The alarm is
 * inserted in time-order into the clock's active alarm list.
 * Always called from within a LOCK_CLOCK() code section.
 */
void
PostAlarm(
	clock_t		clock,
	alarm_t		alarm)
{
	register alarm_t	alrm1;
	register alarm_t	alrm2;
	tvalspec_t		*alarm_time;
	tvalspec_t		*queue_time;

	/*
	 * Traverse alarm list until queue time is greater
	 * than alarm time, then insert alarm.
	 */
	alarm_time = &alarm->al_time;
	alrm1 = (alarm_t) &clock->cl_alarm;
	while (alrm2 = alrm1->al_next) {
		queue_time = &alrm2->al_time;
		if (CMP_TVALSPEC(queue_time, alarm_time) > 0)
			break;
		alrm1 = alrm2;
	}
	alrm1->al_next = alarm;
	alarm->al_next = alrm2;
	alarm->al_prev = alrm1;
	if (alrm2)
		alrm2->al_prev  = alarm;

	/*
	 * If the inserted alarm is the 'earliest' alarm,
	 * reset the device layer alarm time accordingly.
	 */
	if (clock->cl_alarm.al_next == alarm)
		(*clock->cl_ops->c_setalrm)(alarm_time);
}

/*
 * Check the validity of 'alarm_time' and 'alarm_type'. If either
 * argument is invalid, return a negative value. If the 'alarm_time'
 * is now, return a 0 value. If the 'alarm_time' is in the future,
 * return a positive value.
 */
int
CheckTime(
	alarm_type_t	alarm_type,
	tvalspec_t	*alarm_time,
	tvalspec_t	*clock_time)
{
	if (BAD_ALRMTYPE(alarm_type))
		return (-1);
	if (BAD_TVALSPEC(alarm_time))
		return (-1);
	if ((alarm_type & ALRMTYPE) == TIME_RELATIVE)
		ADD_TVALSPEC(alarm_time, clock_time);
	return (CMP_TVALSPEC(alarm_time, clock_time));
}
