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

/* #include <i386/ipl.h> */

/* #include <i386/pit.h> */

#define PITCTL_PORT	0x43		/* PIT control port */
#define PIT_C0          0x00            /* select counter 0 */
#define PITCTR0_PORT	0x40		/* counter 0 port */	

/* #include <i386/pio.h> */

typedef unsigned short i386_ioport_t;

extern __inline__ unsigned char inb(
				i386_ioport_t port)
{
	unsigned char datum;
	__asm__ volatile("inb %1, %0" : "=a" (datum) : "d" (port));
	return(datum);
}

extern __inline__ void outb(
				i386_ioport_t port,
				unsigned char datum)
{
	__asm__ volatile("outb %0, %1" : : "a" (datum), "d" (port));
}

mach_port_t 		iopl_port;
mach_port_t 		at386_timer_port;
memory_object_t 	timer_mem;
mapped_tvalspec_t  	*timer_mapped_time;
tvalspec_t 		inter_time = {0, 0};

unsigned int	i8254_count_per_intr;
unsigned int	i8254_micro_per_intr;

#define 	CLKNUM		1193167

#define 	i8254C(f)	((CLKNUM + ((f) / 2)) / (f))

void 		at386_read_timer();

void
at386_timer_init()
{

	tvalspec_t time;
	unsigned count = 1;
	unsigned resolution;

	if (debug)
		printf("at386_timer_init()\n");


	/*
 	 * Check if it is not already initialized
	 */

	if (clock_get_attributes(at386_timer_port,
				  CLOCK_ALARM_CURRES,
				  &resolution,
				  &count) == KERN_SUCCESS)
		return;

	/*
	 * Get permissions to access timer I/O register.
	 */

	if (iopl_port == MACH_PORT_NULL) {
		MACH_CALL(device_open, (master_device_port,
					MACH_PORT_NULL,
					0,
					mach_perf_token,
					"iopl",
					&iopl_port));
	}
	
	if (at386_timer_port == MACH_PORT_NULL) {
		/*
		 * Open timer 
		 */

		  MACH_CALL(host_get_clock_service , (mach_host_self(),
						      REALTIME_CLOCK,
						      &at386_timer_port));

		/*
		 * Map timer 
		 */
		  
		  MACH_CALL(clock_map_time, (at386_timer_port, 
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
			     VM_INHERIT_SHARE		     /* inheritance */
			     ));
	}

	MACH_CALL( clock_get_attributes, (at386_timer_port,
					  CLOCK_ALARM_CURRES,
					  &resolution,
					  &count));

	i8254_micro_per_intr = (resolution / NSEC_PER_USEC);
	i8254_count_per_intr = i8254C((NSEC_PER_SEC / resolution));

	at386_read_timer (&time);
}

void
at386_read_timer(time)
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
	 * time since the last interrupt. (This could be
	 * done in assembler for a bit more speed).
	 */

	lsb = inb(PITCTR0_PORT);			/* least */
	val = (inb(PITCTR0_PORT) << 8) | lsb; 		/* most */
	val = i8254_count_per_intr - val;
	if (inter_time.tv_nsec = (val * i8254_micro_per_intr)) {
		inter_time.tv_nsec /= i8254_count_per_intr;
		inter_time.tv_nsec *= NSEC_PER_USEC;
	}
	/*
	 * If the time has changed, use the
	 * new time, otherwise utilize the incremental time.
	 */
	MTS_TO_TS(timer_mapped_time, time);
	if (CMP_TVALSPEC(&orig_time, time) == 0)
		ADD_TVALSPEC(time, &inter_time);
}
			   
void	(*user_timer)() = at386_read_timer;
void	(*user_timer_init)() = at386_timer_init;

machine_init() {
}

	
			   
			   



