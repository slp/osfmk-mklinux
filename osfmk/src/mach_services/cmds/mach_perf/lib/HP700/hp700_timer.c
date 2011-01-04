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

#include <mach_perf.h>
#include <mach/clock_types.h>

mach_port_t 		hp700_timer_port = MACH_PORT_NULL;
memory_object_t 	timer_mem;
mapped_tvalspec_t  	*timer_mapped_time;
int			*last_timer_val;
tvalspec_t 		inter_time = {0, 0};
unsigned 		hp700_timer_resolution;

void 		hp700_read_timer();
boolean_t	hp700_timer_initialized = FALSE;

void
hp700_timer_init()
{

	tvalspec_t time;
	unsigned count = 1;

	if (debug)
		printf("hp700_timer_init()\n");


	/*
 	 * Check if it is not already initialized
	 */

	if (clock_get_attributes(hp700_timer_port,
				  CLOCK_GET_TIME_RES,
				  &hp700_timer_resolution,
				  &count) == KERN_SUCCESS)
		return;

	if (hp700_timer_port == MACH_PORT_NULL) {
		/*
		 * Open timer 
		 */

		MACH_CALL(host_get_clock_service , (mach_host_self(),
						    REALTIME_CLOCK,
						    &hp700_timer_port));

		/*
		 * Map timer 
		 */

		MACH_CALL(clock_map_time, (hp700_timer_port, 
				   &timer_mem));

		MACH_CALL(vm_map,
			  (mach_task_self(),			/* target */
			   (vm_offset_t *)&timer_mapped_time,	/* address */
			   vm_page_size,			/* size */
			   0,					/* mask */
			   TRUE,				/* anywhere */
			   timer_mem,				/* MOR */
			   0,					/* offset */
			   FALSE,				/* copy */
			   VM_PROT_READ,			/* cur_prot */
			   VM_PROT_READ,			/* max_prot */
			   VM_INHERIT_SHARE			/* inheritance */
			   ));

		last_timer_val = (int *)(timer_mapped_time+1);
	}

	MACH_CALL( clock_get_attributes, (hp700_timer_port,
					  CLOCK_GET_TIME_RES,
					  &hp700_timer_resolution,
					  &count));

	hp700_read_timer (&time);
	hp700_timer_initialized = TRUE;
}

void
hp700_read_timer(time)
tvalspec_t *time;
{
	tvalspec_t orig_time;
	unsigned int	lsb, val;

	/*
	 * get mapped time 
	 */

	MTS_TO_TS(timer_mapped_time, time);
	orig_time = *time;

	/*
	 * Determine the incremental
	 * time since the last interrupt.
	 */

	inter_time.tv_nsec = (read_timer_reg() - (*last_timer_val)) *
                        hp700_timer_resolution;
	/*
	 * If the time has changed, use the
	 * new time, otherwise utilize the incremental time.
	 */
	MTS_TO_TS(timer_mapped_time, time);
	if (CMP_TVALSPEC(&orig_time, time) == 0)
		ADD_TVALSPEC(time, &inter_time);
}
			   
void	(*user_timer)() = hp700_read_timer;
void	(*user_timer_init)() = hp700_timer_init;

machine_init() {
}
			   



