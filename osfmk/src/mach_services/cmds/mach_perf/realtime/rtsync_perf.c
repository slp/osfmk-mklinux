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

#define TEST_NAME rtsync

#include <mach_perf.h>
#include <mach/sync_policy.h>

char *private_options = "\n\
";

int sema_create_destroy_test();
int sema_create_test();
int sema_destroy_test();
int signal_wait_test();
int lockset_create_destroy_test();
int lockset_create_test();
int lockset_destroy_test();
int lock_acquire_test();
int lock_release_test();
int lock_handoff_test();

struct test tests[] = {
"sema_create",			0, sema_create_test, 0, 0, 0, 0,
"sema_destroy",			0, sema_destroy_test, 0, 0, 0, 0,
"sema_create/destroy",		0, sema_create_destroy_test, 0, 0, 0, 0,
"signal/wait (FIFO)",		0, signal_wait_test, SYNC_POLICY_FIFO, 0, 0, 0,
"lockset_create",			0, lockset_create_test, 0, 0, 0, 0,
"lockset_destroy",			0, lockset_destroy_test, 0, 0, 0, 0,
"lockset_create/destroy",		0, lockset_create_destroy_test, 0, 0, 0, 0,
"lock_acquire (FIFO)",			0, lock_acquire_test, SYNC_POLICY_FIFO, 0, 0, 0,
"lock_release (FIFO)",			0, lock_release_test, SYNC_POLICY_FIFO, 0, 0, 0,
"lock_handoff (FIFO)",			0, lock_handoff_test, SYNC_POLICY_FIFO, 0, 0, 0,
"lock_acquire (FIXED)",			0, lock_acquire_test, SYNC_POLICY_FIXED_PRIORITY, 0, 0, 0,
"lock_release (FIXED)",			0, lock_release_test, SYNC_POLICY_FIXED_PRIORITY, 0, 0, 0,
"lock_handoff (FIXED)",			0, lock_handoff_test, SYNC_POLICY_FIXED_PRIORITY, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0
};

int
max_sems = 600;
int
max_locksets = 600;

main(argc, argv)
	int argc;
	char *argv[];
{
	int i;

	test_init();
	for (i = 1; i < argc; i++)
		if (!is_gen_opt(argc, argv, &i, tests,
				private_options))
			usage();
	run_tests(tests);
	test_end();
}

sema_create_destroy_test()
{
	register i;
	mach_port_t task = mach_task_self();
	semaphore_port_t *sems, *one_sem;

	set_max_loops(max_sems);
	MACH_CALL( vm_allocate_temporary, (mach_task_self(),
				  (vm_offset_t *)&sems,
				  loops*sizeof(semaphore_port_t),
				  TRUE));
	start_time();
	for (i=loops, one_sem = sems; i--; one_sem++)  {
                MACH_CALL( semaphore_create, (task,
					      one_sem, SYNC_POLICY_FIFO, 1));
		MACH_CALL( semaphore_destroy, (task, (*one_sem)));
	}
	stop_time();
      
	for (i=loops, one_sem = sems; i--; one_sem++)  {
		MACH_CALL( mach_port_destroy, (task, (*one_sem)));
	}
	MACH_CALL( vm_deallocate, (mach_task_self(),
				  (vm_offset_t )sems,
				  loops*sizeof(semaphore_port_t)));

	test_cache(sema_create_destroy_test, ());

}

sema_create_test()
{
	register i;
	mach_port_t task = mach_task_self();
	semaphore_port_t *sems, *one_sem;

	set_max_loops(max_sems);
	MACH_CALL( vm_allocate_temporary, (mach_task_self(),
				  (vm_offset_t *)&sems,
				  loops*sizeof(semaphore_port_t),
				  TRUE));
	start_time();
	for (i=loops, one_sem = sems; i--; one_sem++)  {
                MACH_CALL( semaphore_create, (task,
					      one_sem, SYNC_POLICY_FIFO, 1));
	}
	stop_time();
      
	for (i=loops, one_sem = sems; i--; one_sem++)  {
		MACH_CALL( semaphore_destroy, (task, (*one_sem)));
		MACH_CALL( mach_port_destroy, (task, (*one_sem)));
	}
	MACH_CALL( vm_deallocate, (mach_task_self(),
				  (vm_offset_t )sems,
				  loops*sizeof(semaphore_port_t)));
}

sema_destroy_test()
{
	register i;
	mach_port_t task = mach_task_self();
	semaphore_port_t *sems, *one_sem;

	set_max_loops(max_sems);
	MACH_CALL( vm_allocate_temporary, (mach_task_self(),
				  (vm_offset_t *)&sems,
				  loops*sizeof(semaphore_port_t),
				  TRUE));
	for (i=loops, one_sem = sems; i--; one_sem++)  {
                MACH_CALL( semaphore_create, (task,
					      one_sem, SYNC_POLICY_FIFO, 1));
	}
      
	start_time();
	for (i=loops, one_sem = sems; i--; one_sem++)  {
		MACH_CALL( semaphore_destroy, (task, (*one_sem)));
	}
	stop_time();

	for (i=loops, one_sem = sems; i--; one_sem++)  {
		MACH_CALL( mach_port_destroy, (task, (*one_sem)));
	}

	MACH_CALL( vm_deallocate, (mach_task_self(),
				  (vm_offset_t )sems,
				  loops*sizeof(semaphore_port_t)));
}

volatile mach_port_t slave_thread;
volatile semaphore_port_t sem1, sem2;

signal_wait_slave()
{
        slave_thread = mach_thread_self();

        while (1) {
                MACH_CALL( semaphore_signal, (sem1));
                MACH_CALL( semaphore_wait, (sem2));
        }
}

signal_wait_test(int policy)
{
        register i;
	mach_port_t task = mach_task_self();
        mach_thread_t thread;

	MACH_CALL( semaphore_create, (task, &sem1, policy, 1));
	MACH_CALL( semaphore_create, (task, &sem2, policy, 1));

        slave_thread = 0;
        new_thread(&thread, (vm_offset_t)signal_wait_slave, (vm_offset_t)0);

        while (!slave_thread);

        start_time();
        for (i=loops; i--;)  {
		MACH_CALL( semaphore_wait, (sem1));
		MACH_CALL( semaphore_signal, (sem2));
	}
	stop_time();

        kill_thread(thread);

	MACH_CALL( semaphore_destroy, (task, (sem1)));
	MACH_CALL( mach_port_destroy, (task, (sem1)));
	MACH_CALL( semaphore_destroy, (task, (sem2)));
	MACH_CALL( mach_port_destroy, (task, (sem2)));
}


lockset_create_destroy_test()
{
	register i;
	mach_port_t task = mach_task_self();
	lock_set_port_t *locks, *one_lock;

	set_max_loops(max_locksets);
	MACH_CALL( vm_allocate_temporary, (mach_task_self(),
				  (vm_offset_t *)&locks,
				  loops*sizeof(lock_set_port_t),
				  TRUE));
	start_time();
	for (i=loops, one_lock = locks; i--; one_lock++)  {
                MACH_CALL( lock_set_create, (task,
					     one_lock, 1, SYNC_POLICY_FIFO));
		MACH_CALL( lock_set_destroy, (task, (*one_lock)));
	}
	stop_time();
      
	for (i=loops, one_lock = locks; i--; one_lock++)  {
		MACH_CALL( mach_port_destroy, (task, (*one_lock)));
	}
	MACH_CALL( vm_deallocate, (mach_task_self(),
				  (vm_offset_t )locks,
				  loops*sizeof(lock_set_port_t)));

	test_cache(lockset_create_destroy_test, ());

}

lockset_create_test()
{
	register i;
	mach_port_t task = mach_task_self();
	lock_set_port_t *locks, *one_lock;

	set_max_loops(max_locksets);
	MACH_CALL( vm_allocate_temporary, (mach_task_self(),
				  (vm_offset_t *)&locks,
				  loops*sizeof(lock_set_port_t),
				  TRUE));
	start_time();
	for (i=loops, one_lock = locks; i--; one_lock++)  {
                MACH_CALL( lock_set_create, (task,
					     one_lock, 1, SYNC_POLICY_FIFO));
	}
	stop_time();
      
	for (i=loops, one_lock = locks; i--; one_lock++)  {
		MACH_CALL( lock_set_destroy, (task, (*one_lock)));
		MACH_CALL( mach_port_destroy, (task, (*one_lock)));
	}
	MACH_CALL( vm_deallocate, (mach_task_self(),
				  (vm_offset_t )locks,
				  loops*sizeof(lock_set_port_t)));
}

lockset_destroy_test()
{
	register i;
	mach_port_t task = mach_task_self();
	lock_set_port_t *locks, *one_lock;

	set_max_loops(max_locksets);
	MACH_CALL( vm_allocate_temporary, (mach_task_self(),
				  (vm_offset_t *)&locks,
				  loops*sizeof(lock_set_port_t),
				  TRUE));
	for (i=loops, one_lock = locks; i--; one_lock++)  {
                MACH_CALL( lock_set_create, (task,
					     one_lock, 1, SYNC_POLICY_FIFO));
	}
      
	start_time();
	for (i=loops, one_lock = locks; i--; one_lock++)  {
		MACH_CALL( lock_set_destroy, (task, (*one_lock)));
	}
	stop_time();

	for (i=loops, one_lock = locks; i--; one_lock++)  {
		MACH_CALL( mach_port_destroy, (task, (*one_lock)));
	}

	MACH_CALL( vm_deallocate, (mach_task_self(),
				  (vm_offset_t )locks,
				  loops*sizeof(lock_set_port_t)));
}

lock_acquire_test(int policy)
{
	register i;
	mach_port_t task = mach_task_self();
	lock_set_port_t *locks, *one_lock;

	set_max_loops(max_locksets);
	MACH_CALL( vm_allocate_temporary, (mach_task_self(),
				  (vm_offset_t *)&locks,
				  loops*sizeof(lock_set_port_t),
				  TRUE));
	for (i=loops, one_lock = locks; i--; one_lock++)  {
                MACH_CALL( lock_set_create, (task,
					     one_lock, 1, policy));
	}
	start_time();
	for (i=loops, one_lock = locks; i--; one_lock++)  {
		MACH_CALL( lock_acquire, ((*one_lock), 0));
	}
	stop_time();
      
	for (i=loops, one_lock = locks; i--; one_lock++)  {
		MACH_CALL( lock_set_destroy, (task, (*one_lock)));
		MACH_CALL( mach_port_destroy, (task, (*one_lock)));
	}
	MACH_CALL( vm_deallocate, (mach_task_self(),
				  (vm_offset_t )locks,
				  loops*sizeof(lock_set_port_t)));
}


lock_release_test(int policy)
{
	register i;
	mach_port_t task = mach_task_self();
	lock_set_port_t *locks, *one_lock;

	set_max_loops(max_locksets);
	MACH_CALL( vm_allocate_temporary, (mach_task_self(),
				  (vm_offset_t *)&locks,
				  loops*sizeof(lock_set_port_t),
				  TRUE));
	for (i=loops, one_lock = locks; i--; one_lock++)  {
                MACH_CALL( lock_set_create, (task,
					     one_lock, 1, policy));
		MACH_CALL( lock_acquire, ((*one_lock), 0));
	}
	
	start_time();
	for (i=loops, one_lock = locks; i--; one_lock++)  {
		MACH_CALL( lock_release, ((*one_lock), 0));
	}
	stop_time();
      
	for (i=loops, one_lock = locks; i--; one_lock++)  {
		MACH_CALL( lock_set_destroy, (task, (*one_lock)));
		MACH_CALL( mach_port_destroy, (task, (*one_lock)));
	}
	MACH_CALL( vm_deallocate, (mach_task_self(),
				  (vm_offset_t )locks,
				  loops*sizeof(lock_set_port_t)));
}


volatile mach_port_t slave_thread;
volatile lock_set_port_t one_lock;

lock_handoff_slave()
{
        slave_thread = mach_thread_self();

        while (1) {
                MACH_CALL( lock_handoff_accept, (one_lock, 0));
                MACH_CALL( lock_handoff, (one_lock, 0));
        }
}

/*
 * Allocate more locks in a lock set and test on handoff those (w/o accept)
 * Allocate lock sets and handoff in the first of each (w/o accept)
 */
lock_handoff_test(int policy)
{
        register i;
	mach_port_t task = mach_task_self();
        mach_thread_t thread;

	MACH_CALL( lock_set_create, (task, &one_lock, 1, policy));
	MACH_CALL( lock_acquire, (one_lock, 0));

        slave_thread = 0;
        new_thread(&thread, (vm_offset_t)lock_handoff_slave, (vm_offset_t)0);

        while (!slave_thread);

        start_time();
        for (i=loops; i--;)  {
                MACH_CALL( lock_handoff, (one_lock, 0));
                MACH_CALL( lock_handoff_accept, (one_lock, 0));
	}
	stop_time();

        kill_thread(thread);

	MACH_CALL( lock_set_destroy, (task, (one_lock)));
	MACH_CALL( mach_port_destroy, (task, (one_lock)));
}
