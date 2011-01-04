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
 * POSIX Pthread Library
 */

#include "pthread_internals.h"
#include <sys/timers.h>              /* For struct timespec and getclock(). */

/*
 * Destroy a condition variable.
 */
int       
pthread_cond_destroy(pthread_cond_t *cond)
{
	kern_return_t kern_res;
	if (cond->sig == _PTHREAD_COND_SIG)
	{
		LOCK(cond->lock);
		if (cond->busy != (pthread_mutex_t *)NULL)
		{
			UNLOCK(cond->lock);
			return (EBUSY);
		} else
		{
			cond->sig = _PTHREAD_NO_SIG;
			MACH_CALL(semaphore_destroy(mach_task_self(),
						    cond->sem), kern_res);
			UNLOCK(cond->lock);
			return (ESUCCESS);
		}
	} else
		return (EINVAL); /* Not an initialized condition variable structure */
}

/*
 * Initialize a condition variable.  Note: 'attr' is ignored.
 */
int       
pthread_cond_init(pthread_cond_t *cond,
		  const pthread_condattr_t *attr)
{
	kern_return_t kern_res;

	LOCK_INIT(cond->lock);
	cond->sig = _PTHREAD_COND_SIG;
	cond->next = (pthread_cond_t *)NULL;
	cond->prev = (pthread_cond_t *)NULL;
	cond->busy = (pthread_mutex_t *)NULL;
	cond->waiters = 0;
	MACH_CALL(semaphore_create(mach_task_self(), 
				   &cond->sem, 
				   SYNC_POLICY_FIFO, 
				   0), kern_res);
	if (kern_res != KERN_SUCCESS)
	{
		cond->sig = _PTHREAD_NO_SIG;  /* Not a valid condition variable */
		return (ENOMEM);
	} else
		return (ESUCCESS);
}

/*
 * Signal a condition variable, waking up all threads waiting for it.
 */
int       
pthread_cond_broadcast(pthread_cond_t *cond)
{
	kern_return_t kern_res;
	if (cond->sig == _PTHREAD_COND_SIG_init)
	{
		int res;
		if (res = pthread_cond_init(cond, NULL))
			return (res);
	}
	if (cond->sig == _PTHREAD_COND_SIG)
	{ 
		LOCK(cond->lock);
		if (cond->waiters == 0)
		{ /* Avoid kernel call since there are no waiters... */
			UNLOCK(cond->lock);	
			return (ESUCCESS);
		}
		UNLOCK(cond->lock);
		MACH_CALL(semaphore_signal(cond->sem), kern_res);
		MACH_CALL(semaphore_signal_all(cond->sem), kern_res);
		if (kern_res == KERN_SUCCESS)
		{
			return (ESUCCESS);
		} else
		{
			return (EINVAL);
		}
	} else
		return (EINVAL); /* Not a condition variable */
}

/*
 * Signal a condition variable, waking only one thread.
 */
int       
pthread_cond_signal(pthread_cond_t *cond)
{
	kern_return_t kern_res;
	if (cond->sig == _PTHREAD_COND_SIG_init)
	{
		int res;
		if (res = pthread_cond_init(cond, NULL))
			return (res);
	}
	if (cond->sig == _PTHREAD_COND_SIG)
	{ 
		LOCK(cond->lock);
		if (cond->waiters == 0)
		{ /* Avoid kernel call since there are no waiters... */
			UNLOCK(cond->lock);	
			return (ESUCCESS);
		}
		UNLOCK(cond->lock);
		MACH_CALL(semaphore_signal(cond->sem), kern_res);
		if (kern_res == KERN_SUCCESS)
		{
			return (ESUCCESS);
		} else
		{
			return (EINVAL);
		}
	} else
		return (EINVAL); /* Not a condition variable */
}
/*
 * Manage a list of condition variables associated with a mutex
 */

static void
_pthread_cond_add(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
	pthread_cond_t *c;
	LOCK(mutex->lock);
	if ((c = mutex->busy) != (pthread_cond_t *)NULL)
	{
		c->prev = cond;
	} 
	cond->next = c;
	cond->prev = (pthread_cond_t *)NULL;
	mutex->busy = cond;
	UNLOCK(mutex->lock);
}

static void
_pthread_cond_remove(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
	pthread_cond_t *n, *p;
	LOCK(mutex->lock);
	if ((n = cond->next) != (pthread_cond_t *)NULL)
	{
		n->prev = cond->prev;
	}
	if ((p = cond->prev) != (pthread_cond_t *)NULL)
	{
		p->next = cond->next;
	} else
	{ /* This is the first in the list */
		mutex->busy = n;
	}
	UNLOCK(mutex->lock);
}

/*
 * Suspend waiting for a condition variable.
 * Note: we have to keep a list of condition variables which are using
 * this same mutex variable so we can detect invalid 'destroy' sequences.
 */
static int       
_pthread_cond_wait(pthread_cond_t *cond, 
		   pthread_mutex_t *mutex,
		   const struct timespec *abstime)
{
	int res;
	kern_return_t kern_res;
	pthread_mutex_t *busy;
	tvalspec_t then;
	if (cond->sig == _PTHREAD_COND_SIG_init)
	{
		if (res = pthread_cond_init(cond, NULL))
			return (res);
	}
	if (cond->sig != _PTHREAD_COND_SIG)
		return (EINVAL); /* Not a condition variable */
	LOCK(cond->lock);
	busy = cond->busy;
	if ((busy != (pthread_mutex_t *)NULL) && (busy != mutex))
	{ /* Must always specify the same mutex! */
		UNLOCK(cond->lock);
		return (EINVAL);
	}
	cond->waiters++;
	if (cond->waiters == 1)
	{
		_pthread_cond_add(cond, mutex);
		cond->busy = mutex;
	}
	if ((res = pthread_mutex_unlock(mutex)) != ESUCCESS)
	{
		cond->waiters--;
		if (cond->waiters == 0)
		{
			_pthread_cond_remove(cond, mutex);
			cond->busy = (pthread_mutex_t *)NULL;
		}
		UNLOCK(cond->lock);
		return (res);
	}	
	UNLOCK(cond->lock);
	if (abstime)
	{
		struct timespec now;
		getclock(TIMEOFDAY, &now);
		/* Compute relative time to sleep */
		then.tv_nsec = abstime->tv_nsec - now.tv_nsec;
	        then.tv_sec = abstime->tv_sec - now.tv_sec;
		if (then.tv_nsec < 0)
		{
			then.tv_nsec += 1000000000;  /* nsec/sec */
			then.tv_sec--;
		}
		if (((int)then.tv_sec < 0) ||
		    ((then.tv_sec == 0) && (then.tv_nsec == 0)))
		{
			kern_res = KERN_OPERATION_TIMED_OUT;
		} else
		{
			MACH_CALL(semaphore_timedwait(cond->sem, then),
				  kern_res);
		}
	} else
	{
		MACH_CALL(semaphore_wait(cond->sem), kern_res);
	}
	LOCK(cond->lock);
	cond->waiters--;
	if (cond->waiters == 0)
	{
		_pthread_cond_remove(cond, mutex);
		cond->busy = (pthread_mutex_t *)NULL;
	}
	UNLOCK(cond->lock);
	if ((res = pthread_mutex_lock(mutex)) != ESUCCESS)
	{
		return (res);
	}
	if (kern_res == KERN_SUCCESS)
	{
		return (ESUCCESS);
	} else
	if (kern_res == KERN_OPERATION_TIMED_OUT)
	{
		return (ETIMEDOUT);
	} else
	{
		return (EINVAL);
	}
}

int       
pthread_cond_wait(pthread_cond_t *cond, 
		  pthread_mutex_t *mutex)
{
	return (_pthread_cond_wait(cond, mutex, (struct timespec *)NULL));
}

int       
pthread_cond_timedwait(pthread_cond_t *cond, 
		       pthread_mutex_t *mutex,
		       const struct timespec *abstime)
{
	return (_pthread_cond_wait(cond, mutex, abstime));
}
