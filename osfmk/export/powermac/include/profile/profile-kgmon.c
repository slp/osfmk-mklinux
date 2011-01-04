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

#include <profile/profile-internal.h>

#ifdef MACH_KERNEL
#include <profile/profile-md.h>
#endif

#ifndef PROFILE_VARS
#define PROFILE_VARS(cpu) (&_profile_vars)
#endif

extern int printf(const char *, ...);


/*
 * Kgmon interface.  This returns the count of bytes moved if everything was ok,
 * or -1 if there were errors.
 */

long
_profile_kgmon(int write,
	       size_t count,
	       long indx,
	       int max_cpus,
	       void **p_ptr,
	       void (*control_func)(kgmon_control_t))
{
	kgmon_control_t kgmon;
	int cpu;
	int error = 0;
	int i;
	struct profile_vars *pv;
	static struct callback dummy_callback;

	*p_ptr = (void *)0;

	/*
	 * If the number passed is not within bounds, just copy the data directly.
	 */

	if (!LEGAL_KGMON(indx)) {
		*p_ptr = (void *)indx;
		if (!write) {
			if (PROFILE_VARS(0)->debug) {
				printf("_profile_kgmon: copy %5ld bytes, from 0x%lx\n",
				       (long)count,
				       (long)indx);
			}

		} else {
			if (PROFILE_VARS(0)->debug) {
				printf("_profile_kgmon: copy %5ld bytes, to 0x%lx\n",
				       (long)count,
				       (long)indx);
			}
		}			

		return count;
	}

	/*
	 * Decode the record number into the component pieces.
	 */

	DECODE_KGMON(indx, kgmon, cpu);

	if (PROFILE_VARS(0)->debug) {
		printf("_profile_kgmon: start: kgmon control = %2d, cpu = %d, count = %ld\n",
		       kgmon, cpu, (long)count);
	}

	/* Validate the CPU number */
	if (cpu < 0 || cpu >= max_cpus) {
		if (PROFILE_VARS(0)->debug) {
			printf("KGMON, bad cpu %d\n", cpu);
		}

		return -1;

	} else {
		pv = PROFILE_VARS(cpu);

		if (!write) {
			switch (kgmon) {
			default:
				if (PROFILE_VARS(0)->debug) {
					printf("Unknown KGMON read command\n");
				}

				error = -1;
				break;

			case KGMON_GET_STATUS:		/* return whether or not profiling is active */
				if (cpu != 0) {
					if (PROFILE_VARS(0)->debug) {
						printf("KGMON_GET_STATUS: cpu = %d\n", cpu);
					}

					error = -1;
					break;
				}

				if (count != sizeof(pv->active)) {
					if (PROFILE_VARS(0)->debug) {
						printf("KGMON_GET_STATUS: count = %ld, should be %ld\n",
						       (long)count,
						       (long)sizeof(pv->active));
					}

					error = -1;
					break;
				}

				*p_ptr = (void *)&pv->active;
				break;

			case KGMON_GET_DEBUG:		/* return whether or not debugging is active */
				if (cpu != 0) {
					if (PROFILE_VARS(0)->debug) {
						printf("KGMON_GET_DEBUG: cpu = %d\n", cpu);
					}

					error = -1;
					break;
				}

				if (count != sizeof(pv->debug)) {
					if (PROFILE_VARS(0)->debug) {
						printf("KGMON_GET_DEBUG: count = %ld, should be %ld\n",
						       (long)count,
						       (long)sizeof(pv->active));
					}

					error = -1;
					break;
				}

				*p_ptr = (void *)&pv->debug;
				break;

			case KGMON_GET_PROFILE_VARS:	/* return the _profile_vars structure */
				if (count != sizeof(struct profile_vars)) {
					if (PROFILE_VARS(0)->debug) {
						printf("KGMON_GET_PROFILE_VARS: count = %ld, should be %ld\n",
						       (long)count,
						       (long)sizeof(struct profile_vars));
					}

					error = -1;
					break;
				}

				_profile_update_stats(pv);
				*p_ptr = (void *)pv;
				break;

			case KGMON_GET_PROFILE_STATS:	/* return the _profile_stats structure */
				if (count != sizeof(struct profile_stats)) {
					if (PROFILE_VARS(0)->debug) {
						printf("KGMON_GET_PROFILE_STATS: count = %ld, should be = %ld\n",
						       (long)count,
						       (long)sizeof(struct profile_stats));
					}

					error = -1;
					break;
				}

				_profile_update_stats(pv);
				*p_ptr = (void *)&pv->stats;
				break;
			}

		} else {
			switch (kgmon) {
			default:
				if (PROFILE_VARS(0)->debug) {
					printf("Unknown KGMON write command\n");
				}

				error = -1;
				break;

			case KGMON_SET_PROFILE_ON:	/* turn on profiling */
				if (cpu != 0) {
					if (PROFILE_VARS(0)->debug) {
						printf("KGMON_SET_PROFILE_ON, cpu = %d\n", cpu);
					}

					error = -1;
					break;
				}

				if (!PROFILE_VARS(0)->active) {
					for (i = 0; i < max_cpus; i++) {
						PROFILE_VARS(i)->active = 1;
					}

					if (control_func) {
						(*control_func)(kgmon);
					}

					_profile_md_start();
				}

				count = 0;
				break;

			case KGMON_SET_PROFILE_OFF:	/* turn off profiling */
				if (cpu != 0) {
					if (PROFILE_VARS(0)->debug) {
						printf("KGMON_SET_PROFILE_OFF, cpu = %d\n", cpu);
					}

					error = -1;
					break;
				}

				if (PROFILE_VARS(0)->active) {
					for (i = 0; i < max_cpus; i++) {
						PROFILE_VARS(i)->active = 0;
					}

					_profile_md_stop();

					if (control_func) {
						(*control_func)(kgmon);
					}
				}

				count = 0;
				break;

			case KGMON_SET_PROFILE_RESET:	/* reset profiling */
				if (cpu != 0) {
					if (PROFILE_VARS(0)->debug) {
						printf("KGMON_SET_PROFILE_RESET, cpu = %d\n", cpu);
					}

					error = -1;
					break;
				}

				for (i = 0; i < max_cpus; i++) {
					_profile_reset(PROFILE_VARS(i));
				}

				if (control_func) {
					(*control_func)(kgmon);
				}

				count = 0;
				break;

			case KGMON_SET_DEBUG_ON:	/* turn on profiling */
				if (cpu != 0) {
					if (PROFILE_VARS(0)->debug) {
						printf("KGMON_SET_DEBUG_ON, cpu = %d\n", cpu);
					}

					error = -1;
					break;
				}

				if (!PROFILE_VARS(0)->debug) {
					for (i = 0; i < max_cpus; i++) {
						PROFILE_VARS(i)->debug = 1;
					}

					if (control_func) {
						(*control_func)(kgmon);
					}
				}

				count = 0;
				break;

			case KGMON_SET_DEBUG_OFF:	/* turn off profiling */
				if (cpu != 0) {
					if (PROFILE_VARS(0)->debug) {
						printf("KGMON_SET_DEBUG_OFF, cpu = %d\n", cpu);
					}

					error = -1;
					break;
				}

				if (PROFILE_VARS(0)->debug) {
					for (i = 0; i < max_cpus; i++) {
						PROFILE_VARS(i)->debug = 0;
					}

					if (control_func) {
						(*control_func)(kgmon);
					}
				}

				count = 0;
				break;
			}
		}
	}

	if (error) {
		if (PROFILE_VARS(0)->debug) {
			printf("_profile_kgmon: done:  kgmon control = %2d, cpu = %d, error = %d\n",
			       kgmon, cpu, error);
		}

		return -1;
	}

	if (PROFILE_VARS(0)->debug) {
		printf("_profile_kgmon: done:  kgmon control = %2d, cpu = %d, count = %ld\n",
		       kgmon, cpu, (long)count);
	}

	return count;
}
