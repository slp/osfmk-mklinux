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
 * 
 */
/*
 * MkLinux
 */

/*
 * POSIX Pthread Library
 * -- Mutex variable support
 */

#include "pthread_internals.h"

/*
 * Destroy a mutex variable.
 */
int       
pthread_mutex_destroy(pthread_mutex_t *mutex)
{
	kern_return_t kern_res;
	if (mutex->sig != _PTHREAD_MUTEX_SIG)
		return (EINVAL);
	if ((mutex->owner != (pthread_t)NULL) || 
	    (mutex->busy != (pthread_cond_t *)NULL))
		return (EBUSY);
	mutex->sig = _PTHREAD_NO_SIG;
	MACH_CALL(semaphore_destroy(mach_task_self(),
				    mutex->sem), kern_res);
	return (ESUCCESS);
}

/*
 * Initialize a mutex variable, possibly with additional attributes.
 */
int       
pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr)
{
	kern_return_t kern_res;
	LOCK_INIT(mutex->lock);
	mutex->sig = _PTHREAD_MUTEX_SIG;
	if (attr)
	{
		if (attr->sig != _PTHREAD_MUTEX_ATTR_SIG)
			return (EINVAL);
		mutex->prioceiling = attr->prioceiling;
		mutex->protocol = attr->protocol;
	} else
	{
		mutex->prioceiling = _PTHREAD_DEFAULT_PRIOCEILING;
		mutex->protocol = _PTHREAD_DEFAULT_PROTOCOL;
	}
	mutex->owner = (pthread_t)NULL;
	mutex->next = (pthread_mutex_t *)NULL;
	mutex->prev = (pthread_mutex_t *)NULL;
	mutex->busy = (pthread_cond_t *)NULL;
	mutex->waiters = 0;
	MACH_CALL(semaphore_create(mach_task_self(), 
				   &mutex->sem, 
				   SYNC_POLICY_FIFO, 
				   0), kern_res);
	if (kern_res != KERN_SUCCESS)
	{
		mutex->sig = _PTHREAD_NO_SIG;  /* Not a valid mutex variable */
		return (ENOMEM);
	} else
	{
		return (ESUCCESS);
	}
}

/*
 * Manage a list of mutex variables owned by a thread
 */

static void
_pthread_mutex_add(pthread_mutex_t *mutex)
{
	pthread_mutex_t *m;
	pthread_t self = pthread_self();
	mutex->owner = self;
	if ((m = self->mutexes) != (pthread_mutex_t *)NULL)
	{ /* Add to list */
		m->prev = mutex;
	}
	mutex->next = m;
	mutex->prev = (pthread_mutex_t *)NULL;
	self->mutexes = mutex;
}

static void
_pthread_mutex_remove(pthread_mutex_t *mutex)
{
	pthread_mutex_t *n, *p;
	pthread_t self = pthread_self();
	if ((n = mutex->next) != (pthread_mutex_t *)NULL)
	{
		n->prev = mutex->prev;
	}
	if ((p = mutex->prev) != (pthread_mutex_t *)NULL)
	{
		p->next = mutex->next;
	} else
	{ /* This is the first in the list */
		self->mutexes = n;
	}
}

/*
 * Lock a mutex.
 * TODO: Priority inheritance stuff
 */
int       
pthread_mutex_lock(pthread_mutex_t *mutex)
{
	kern_return_t kern_res;
	if (mutex->sig == _PTHREAD_MUTEX_SIG_init)
	{
		int res;
		if (res = pthread_mutex_init(mutex, NULL))
			return (res);
	}
	if (mutex->sig != _PTHREAD_MUTEX_SIG)
		return (EINVAL);	/* Not a mutex variable */
	LOCK(mutex->lock);
	while (mutex->owner != (pthread_t)NULL)
	{
		mutex->waiters++;
		UNLOCK(mutex->lock);
		MACH_CALL(semaphore_wait(mutex->sem), kern_res);
		LOCK(mutex->lock);
		mutex->waiters--;
	}
	_pthread_mutex_add(mutex);
	UNLOCK(mutex->lock);
	return (ESUCCESS);
}

/*
 * Attempt to lock a mutex, but don't block if this isn't possible.
 */
int       
pthread_mutex_trylock(pthread_mutex_t *mutex)
{
	if (mutex->sig == _PTHREAD_MUTEX_SIG_init)
	{
		int res;
		if (res = pthread_mutex_init(mutex, NULL))
			return (res);
	}
	if (mutex->sig != _PTHREAD_MUTEX_SIG)
		return (EINVAL);	/* Not a mutex variable */
	LOCK(mutex->lock);
	if (mutex->owner != (pthread_t)NULL)
	{
		UNLOCK(mutex->lock);
		return (EBUSY);
	} else
	{
		_pthread_mutex_add(mutex);
		UNLOCK(mutex->lock);
		return (ESUCCESS);
	}
}

/*
 * Unlock a mutex.
 * TODO: Priority inheritance stuff
 */
int       
pthread_mutex_unlock(pthread_mutex_t *mutex)
{
	kern_return_t kern_res;
	int waiters;
	if (mutex->sig == _PTHREAD_MUTEX_SIG_init)
	{
		int res;
		if (res = pthread_mutex_init(mutex, NULL))
			return (res);
	}
	if (mutex->sig != _PTHREAD_MUTEX_SIG)
		return (EINVAL);	/* Not a mutex variable */
	LOCK(mutex->lock);
	if (mutex->owner != pthread_self())
	{
		UNLOCK(mutex->lock);
		return (EPERM);
	} else
	{
		_pthread_mutex_remove(mutex);
		mutex->owner = (pthread_t)NULL;
		waiters = mutex->waiters;
		UNLOCK(mutex->lock);
		if (waiters)
		{
			MACH_CALL(semaphore_signal(mutex->sem), kern_res);
		}
		return (ESUCCESS);
	}
}

/*
 * Fetch the priority ceiling value from a mutex variable.
 * Note: written as a 'helper' function to hide implementation details.
 */
int       
pthread_mutex_getprioceiling(const pthread_mutex_t *mutex, 
                             int *prioceiling)
{
	if (mutex->sig == _PTHREAD_MUTEX_SIG)
	{
		*prioceiling = mutex->prioceiling;
		return (ESUCCESS);
	} else
	{
		return (EINVAL); /* Not an initialized 'attribute' structure */
	}
}

/*
 * Set the priority ceiling for a mutex.
 * Note: written as a 'helper' function to hide implementation details.
 */
int       
pthread_mutex_setprioceiling(pthread_mutex_t *mutex, 
			     int prioceiling, 
			     int *old_prioceiling)
{
	if (mutex->sig == _PTHREAD_MUTEX_SIG)
	{
		if ((prioceiling >= -999) ||
		    (prioceiling <= 999))
		{
			*old_prioceiling = mutex->prioceiling;
			mutex->prioceiling = prioceiling;
			return (ESUCCESS);
		} else 
		{
			return (EINVAL); /* Invalid parameter */
		}
	} else
	{
		return (EINVAL); /* Not an initialized 'attribute' structure */
	}
}

/*
 * Destroy a mutex attribute structure.
 */
int       
pthread_mutexattr_destroy(pthread_mutexattr_t *attr)
{
	attr->sig = _PTHREAD_NO_SIG;  /* Uninitialized */
	return (ESUCCESS);
}

/*
 * Get the priority ceiling value from a mutex attribute structure.
 * Note: written as a 'helper' function to hide implementation details.
 */
int       
pthread_mutexattr_getprioceiling(const pthread_mutexattr_t *attr, 
				 int *prioceiling)
{
	if (attr->sig == _PTHREAD_MUTEX_ATTR_SIG)
	{
		*prioceiling = attr->prioceiling;
		return (ESUCCESS);
	} else
	{
		return (EINVAL); /* Not an initialized 'attribute' structure */
	}
}

/*
 * Get the mutex 'protocol' value from a mutex attribute structure.
 * Note: written as a 'helper' function to hide implementation details.
 */
int       
pthread_mutexattr_getprotocol(const pthread_mutexattr_t *attr, 
			      int *protocol)
{
	if (attr->sig == _PTHREAD_MUTEX_ATTR_SIG)
	{
		*protocol = attr->protocol;
		return (ESUCCESS);
	} else
	{
		return (EINVAL); /* Not an initialized 'attribute' structure */
	}
}

/*
 * Initialize a mutex attribute structure to system defaults.
 */
int       
pthread_mutexattr_init(pthread_mutexattr_t *attr)
{
	attr->prioceiling = _PTHREAD_DEFAULT_PRIOCEILING;
	attr->protocol = _PTHREAD_DEFAULT_PROTOCOL;
	attr->sig = _PTHREAD_MUTEX_ATTR_SIG;
	return (ESUCCESS);
}

/*
 * Set the priority ceiling value in a mutex attribute structure.
 * Note: written as a 'helper' function to hide implementation details.
 */
int       
pthread_mutexattr_setprioceiling(pthread_mutexattr_t *attr, 
				 int prioceiling)
{
	if (attr->sig == _PTHREAD_MUTEX_ATTR_SIG)
	{
		if ((prioceiling >= -999) ||
		    (prioceiling <= 999))
		{
			attr->prioceiling = prioceiling;
			return (ESUCCESS);
		} else 
		{
			return (EINVAL); /* Invalid parameter */
		}
	} else
	{
		return (EINVAL); /* Not an initialized 'attribute' structure */
	}
}

/*
 * Set the mutex 'protocol' value in a mutex attribute structure.
 * Note: written as a 'helper' function to hide implementation details.
 */
int       
pthread_mutexattr_setprotocol(pthread_mutexattr_t *attr, 
			      int protocol)
{
	if (attr->sig == _PTHREAD_MUTEX_ATTR_SIG)
	{
		if ((protocol == PTHREAD_PRIO_NONE) || 
		    (protocol == PTHREAD_PRIO_INHERIT) ||
		    (protocol == PTHREAD_PRIO_PROTECT))
		{
			attr->protocol = protocol;
			return (ESUCCESS);
		} else 
		{
			return (EINVAL); /* Invalid parameter */
		}
	} else
	{
		return (EINVAL); /* Not an initialized 'attribute' structure */
	}
}
