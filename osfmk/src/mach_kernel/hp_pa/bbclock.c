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
#include <mach/message.h>
#include <kern/clock.h>
#include <kern/spl.h>
#include <kern/misc_protos.h>
#include <machine/pdc.h>
#include <machine/iomod.h>
#include <machine/psw.h>
#include <machine/regs.h>

/* Forward */

extern int		bbc_config(void);

extern int		bbc_init(void);

extern kern_return_t 	bbc_gettime(tvalspec_t *);

extern kern_return_t 	bbc_settime(tvalspec_t *);

extern kern_return_t	bbc_getattr(
				    clock_flavor_t		flavor,
				    clock_attr_t		attr,
				    mach_msg_type_number_t	* count);
extern kern_return_t	bbc_setattr(
				    clock_flavor_t		flavor,
				    clock_attr_t		attr,
				    mach_msg_type_number_t	count);

#define NO_MAPTIME (kern_return_t(*)(ipc_port_t * pager))0

#define NO_SETALRM (void (*) (tvalspec_t* alarm_time))0

struct clock_ops  bbc_ops = {
	bbc_config,	bbc_init,	bbc_gettime,	bbc_settime,
	bbc_getattr,	bbc_setattr,	NO_MAPTIME,	NO_SETALRM,
};

struct pdc_time bbc_time;
decl_simple_lock_data(, bbc_lock)

/*
 * Configure battery-backed clock.
 */
int
bbc_config(void)
{
	printf("battery clock configured\n");
	return (1);
}

/*
 * Initialize battery-backed clock.
 */
int
bbc_init(void)
{
	simple_lock_init(&bbc_lock, ETAP_MISC_CLOCK);
	return (1);
}

/*
 * Get the current clock time.
 */

kern_return_t
bbc_gettime(
	tvalspec_t	*cur_time)	/* OUT */
{
    /* The PDC cannot be given the address of a stack variable because it
       requires the underlying physical address to be identical.  We could
       probably dispense with the lock since in case of concurrency we
       would just get a more up-to-date value. But perhaps SMP systems could
       have inconsistent clocks?  */
    simple_lock(&bbc_lock);
    pdc_iodc(PAGE0->mem_pdc, 1, PDC_TOD, PDC_TOD_READ, &bbc_time);
    cur_time->tv_sec = bbc_time.time.seconds;
    cur_time->tv_nsec =  bbc_time.time.microseconds * 1000;
    simple_unlock(&bbc_lock);
    return KERN_SUCCESS;
}

/*
 * Set the current clock time.
 */

kern_return_t
bbc_settime(
	tvalspec_t	*new_time)
{
    pdc_iodc(PAGE0->mem_pdc, 1, PDC_TOD, PDC_TOD_WRITE, new_time->tv_sec,
	     new_time->tv_nsec / NSEC_PER_USEC);
    return KERN_SUCCESS;
}
/*
 * Get clock device attributes.
 */
kern_return_t
bbc_getattr(
	clock_flavor_t		flavor,
	clock_attr_t		attr,		/* OUT */
	mach_msg_type_number_t	*count)		/* IN/OUT */
{
	if (*count != 1)
		return (KERN_FAILURE);
	switch (flavor) {

	case CLOCK_GET_TIME_RES:	/* >0 res */
		*(clock_res_t *) attr = NSEC_PER_SEC/NSEC_PER_USEC;
		break;

	case CLOCK_MAP_TIME_RES:	/* =0 no mapping */
	case CLOCK_ALARM_CURRES:	/* =0 no alarm */
	case CLOCK_ALARM_MINRES:
	case CLOCK_ALARM_MAXRES:
		*(clock_res_t *) attr = 0;
		break;

	default:
		return (KERN_INVALID_VALUE);
	}
	return (KERN_SUCCESS);
}

/*
 * Set clock device attributes.
 */
kern_return_t
bbc_setattr(
	clock_flavor_t		flavor,
	clock_attr_t		attr,		/* IN */
	mach_msg_type_number_t	count)		/* IN */
{
	return (KERN_FAILURE);
}
