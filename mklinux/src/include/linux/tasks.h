#ifndef _LINUX_TASKS_H
#define _LINUX_TASKS_H

/*
 * This is the maximum nr of tasks - change it if you need to
 */
 
#include <linux/autoconf.h>

#ifdef __SMP__
#define NR_CPUS	32		/* Max processors that can be running in SMP */
#else
#define NR_CPUS 1
#endif

#ifdef	CONFIG_OSFMACH3
/*
 * A more reasonable value for NR_TASKS:
 * we want to run some heavily multi-threaded applications
 * and we need one task_struct per cloned "thread"...
 */
#define NR_TASKS	2048
#define REAL_NR_TASKS	512
#define MAX_TASKS_PER_USER (REAL_NR_TASKS/2)
#else	/* CONFIG_OSFMACH3 */
#define NR_TASKS	512
#define MAX_TASKS_PER_USER (NR_TASKS/2)
#endif	/* CONFIG_OSFMACH3 */

#define MIN_TASKS_LEFT_FOR_ROOT 4

#endif
