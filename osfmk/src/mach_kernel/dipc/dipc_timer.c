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
 *	File:	dipc/dipc_timer.c
 *	Author:	Steve Sears
 *	Date:	1994
 *
 *	General purpose timer functions
 *	Includes:
 *		Ability to set up a timer with an arbitrary name and
 *		associate an ASCII string name with it.
 *		Ability to start and stop a timer multiple times.
 *		Ability to continuously bump a timer and obtain
 *		the aggregate time.
 *		Ability to accumulate arbitrary values with counter.
 */

#include <mach_kdb.h>

#include <kern/assert.h>
#include <kern/lock.h>
#include <mach/kkt_request.h>
#include <mach/kern_return.h>
#include <kern/zalloc.h>
#include <kern/lock.h>
#include <dipc/dipc_timer.h>
#include <kern/misc_protos.h>


extern void fc_get(unsigned int *);

/*
 *	States a timer can be in.
 *
 *	Note that the started and stopped states
 *	imply initialized.
 */
#define	TIMER_INVALID	0
#define	TIMER_INITED	1
#define	TIMER_STARTED	2
#define	TIMER_STOPPED	3


typedef struct timer_tree_node	*timer_tree_t;

struct timer_tree_node {
	timer_tree_t	left;
	timer_tree_t	right;
	timer_tree_t	next;
	unsigned int	key;
	char		*name;
	unsigned int	state;
	unsigned int	start[2];
	unsigned int	end[2];
	unsigned int	total[2];
	unsigned int	count;
	unsigned int	add;		/* running accumulator */
};

#define	TIMER_NULL	((timer_tree_t) 0)

#define	MAX_TIMERS	200
timer_tree_t		timer_tree;
timer_tree_t		timer_list;
zone_t			dipc_timer_zone;
decl_simple_lock_data(,timer_tree_lock)


timer_tree_t timer_search(unsigned int	key);
#if	PARAGON860
unsigned int timer_subtime(unsigned int *a, unsigned int *b);
#endif	/* PARAGON860 */

void		timer_show(
			timer_tree_t	tm,
			boolean_t	titled);

/*
 * Set up the global timer structures
 */
void
dipc_timer_init(void)
{
	timer_tree = TIMER_NULL;
	timer_list = TIMER_NULL;
	dipc_timer_zone = zinit(sizeof(struct timer_tree_node), MAX_TIMERS,
		sizeof(struct timer_tree_node), "dipc_timer_zone");
	zfill(dipc_timer_zone, 20);
	simple_lock_init(&timer_tree_lock, ETAP_DIPC_TIMER);
}

/*
 * Associate a timer with an arbitrary key.  Once the timer is set up,
 * any timer operation can occur on it.  Installing a key that is already
 * in the tree result in the timer value being reset.
 */
kern_return_t
timer_set(
	unsigned int	key,
	char		*name)
{
	timer_tree_t	tm, rtm;
	static int	 left_depth = 0, right_depth = 0;
	spl_t		s;

	s = splsched();
	simple_lock(&timer_tree_lock);
	tm = timer_search(key);
	if (tm) {
		tm->state = TIMER_INITED;
		tm->start[0] = 0;
		tm->start[1] = 0;
		tm->end[0] = 0;
		tm->end[1] = 0;
		tm->total[0] = 0;
		tm->total[1] = 0;
		tm->add = 0;
		tm->count = 0;
		simple_unlock(&timer_tree_lock);
		splx(s);
		return (KERN_SUCCESS);
	}

	tm = (timer_tree_t) zget(dipc_timer_zone);

	if (!tm) {
		/*
		 * Need to block for memory; do it the hard way
		 */
		simple_unlock(&timer_tree_lock);
		splx(s);
		tm = (timer_tree_t)zalloc(dipc_timer_zone);
		s = splsched();
		simple_lock(&timer_tree_lock);

		/*
		 * Recheck to make sure someone didn't sneak in and
		 * insert this key.
		 */
		if (timer_search(key)) {
			simple_unlock(&timer_tree_lock);
			splx(s);
			zfree(dipc_timer_zone, (vm_offset_t)tm);
			return (KERN_SUCCESS);
		}
	}

	bzero((char *)tm, sizeof(struct timer_tree_node));
	tm->name = name;
	tm->key = key;
	tm->state = TIMER_INITED;
	tm->next = timer_list;
	timer_list = tm;

	if (timer_tree) {
		/*
		 * Insert the node into its place in the tree.
		 * Insertions always occur at a leaf node.
		 */
		rtm = timer_tree;
		if (key > rtm->key)
			right_depth++;
		else
			left_depth++;
		while (1) {
			if (key > rtm->key) {
				if (rtm->right) {
					rtm = rtm->right;
					continue;
				} else {
					rtm->right = tm;
					break;
				}
			} else {
				if (rtm->left) {
					rtm = rtm->left;
					continue;
				} else {
					rtm->left = tm;
					break;
				}
			} 
		}
		/*
		 * make some effort to keep the tree balanced; rotate
		 * the tree if it is out of whack.
		 */
#define FUZZY 2		/* fuzzy balance constant */
		tm = timer_tree;
		if (left_depth - right_depth > FUZZY) {
			timer_tree = tm->left;
			rtm = timer_tree->right;
			timer_tree->right = tm;
			tm->left = rtm;
			left_depth--;
		} else if (right_depth - left_depth > FUZZY) {
			timer_tree = tm->right;
			rtm = timer_tree->left;
			timer_tree->left = tm;
			tm->right = rtm;
			right_depth--;
		}
	} else {
		timer_tree = tm;
	}
	simple_unlock(&timer_tree_lock);
	splx(s);

	return (KERN_SUCCESS);
}

/*
 * timer_start()
 *
 * Start the timer associated with key.
 */
kern_return_t
timer_start(
	unsigned int	key)
{
	timer_tree_t	tm;
	
	tm = timer_search(key);
	if (!tm)
		return (KERN_INVALID_NAME);
	assert(tm->state != TIMER_STARTED);
	tm->state = TIMER_STARTED;
	fc_get(tm->start);

	return (KERN_SUCCESS);
}

/*
 * timer_stop()
 *
 * Stop the timer, total the time
 */
kern_return_t
timer_stop(
	unsigned int	key,
	unsigned int	add)
{
	timer_tree_t	tm;
	register unsigned int	total, pre;
	unsigned int	stop_time[2];

	tm = timer_search(key);
	if (!tm)
		return (KERN_INVALID_NAME);
	fc_get(stop_time);

	if (tm->state == TIMER_STOPPED)
		return( KERN_SUCCESS);

	assert(tm->state == TIMER_STARTED);
	tm->state = TIMER_STOPPED;

	tm->end[0] = stop_time[0];
	tm->end[1] = stop_time[1];

#if	PARAGON860
	total = pre = tm->total[1];
	total += timer_subtime(tm->end, tm->start);
	tm->total[1] = total;
	if (total <= pre)
		tm->total[0]++;
#else	/*PARAGON860*/
	ADD_TVALSPEC((tvalspec_t *)tm->total, (tvalspec_t *)tm->end);
	SUB_TVALSPEC((tvalspec_t *)tm->total, (tvalspec_t *)tm->start);
#endif	/*PARAGON860*/
	tm->count++;
	tm->add += add;

	return (KERN_SUCCESS);
}

/*
 * timer_add()
 *
 * Add two timer keys together, putting the results in the first
 * one.  The second timer is left untouched.
 */
kern_return_t
timer_add(
	unsigned int	prin,
	unsigned int	adden)
{
	timer_tree_t	tp, ta;
	unsigned int	total, pre;
	spl_t		s;

	tp = timer_search(prin);
	ta = timer_search(adden);
	if (!tp || !ta)
		return (KERN_INVALID_NAME);

	s = splsched();
	simple_lock(&timer_tree_lock);
#if	PARAGON860
	pre = tp->total[1];
	total = tp->total[1] + ta->total[1];
	tp->total[1] = total;
	if (total <= pre)
		tp->total[0]++;
	tp->total[0] += ta->total[0];
#else	/*PARAGON860*/
	ADD_TVALSPEC((tvalspec_t *)tp->total, (tvalspec_t *)ta->total);
#endif	/*PARAGON860*/
	tp->count    += ta->count;
	tp->add      += ta->add;
	simple_unlock(&timer_tree_lock);
	splx(s);

	return (KERN_SUCCESS);
}

/*
 * timer_abort()
 *
 * Abort a timer that has been started that is not going to complete.
 * This is simply a state change.
 */
kern_return_t
timer_abort(
	unsigned int	key)
{
	timer_tree_t	tm;

	tm = timer_search(key);
	if (!tm)
		return (KERN_INVALID_NAME);
	tm->state = TIMER_STOPPED;

	return (KERN_SUCCESS);
}
	


/*
 * timer_check()
 *
 * Do a checkpoint on the time, accumulating the time into the total
 * and copying the end time to the start time.
 */
kern_return_t
timer_check(
	unsigned int	key)
{
	timer_tree_t	tm;

	if (timer_stop(key, 0) != KERN_SUCCESS)
		return (KERN_INVALID_NAME);
	tm = timer_search(key);
	tm->start[0] = tm->end[0];
	tm->start[1] = tm->end[1];

	return (KERN_SUCCESS);
}

/*
 * timer_free
 *
 * Release a timer from the tree: UNIMPLEMENTED
 */
kern_return_t
timer_free(
	unsigned int	key)
{
	timer_tree_t	tm;

	return (KERN_SUCCESS);
}

/*
 * Given a key, find the associate timer structure in the timer tree
 */
timer_tree_t
timer_search(
	unsigned int	key)
{
	timer_tree_t	tm;

	tm = timer_tree;
	while (tm) {
		if (key == tm->key)
			return (tm);
		if (key < tm->key)
			tm = tm->left;
		else
			tm = tm->right;
	}

	return (TIMER_NULL);
}

/*
 * timer_clear_tree
 *
 * This is the BIG HAMMER so must be used with caution; doing a
 * timer_clear_tree with running timers present will blow up in
 * your face.
 */
void
timer_clear_tree(void)
{
	timer_tree_t tm;

	for(tm = timer_list; tm != TIMER_NULL; tm = tm->next) {
		tm->state = TIMER_INITED;
		tm->start[0] = tm->start[1] = 0;
		tm->end[0]   = tm->end[1] = 0;
		tm->total[0] = tm->total[1] = 0;
		tm->count = 0;
		tm->add = 0;
	}
}

#if	PARAGON860
unsigned int
timer_subtime(
	unsigned int *a,
	unsigned int *b)
{
	int atime;

	if (*a > *b)
		atime = *a - *b;
	else
		atime = (unsigned int)0xffffffff - *b + *a;

	return (atime);
}
#endif	/*PARAGON860*/


#if	MACH_KDB

#include <ddb/db_output.h>

void	db_timer_print_list(void);

void
db_timer_print(void)
{
	timer_tree_t tm;

	db_printf("    Key    Name               Usecs    Count      ");
	db_printf("Added Rate\n");

	for(tm = timer_list; tm != TIMER_NULL; tm = tm->next)
	    if (tm->total[1] || tm->total[0])
		timer_show(tm, TRUE);
}



void
timer_show(
	timer_tree_t	tm,
	boolean_t	titled)
{
	unsigned int	rate1, rate2, print_sec;
#if	PARAGON860
	int		subt;
	double		usec, rate;
#else	/*PARAGON860*/
	int		usec, rate;
#endif	/*PARAGON860*/
	char		*sec_name = "usec";

#if	PARAGON860
	usec = tm->total[1];
	usec = (usec * 2) / 100;
	subt = usec;
	print_sec = subt;
	if (tm->add != 0) {
		rate = tm->add;
		rate = rate / usec;
		subt = rate;
		rate -= subt;
		rate1 = subt;
		rate *= 1000;
		rate2 = rate;
	} else if (tm->count != 0) {
		rate = usec / tm->count;
		rate1 = rate;
	}
#else	/* PARAGON860 */
	usec = tm->total[1]/1000 + tm->total[0]*1000000;
	if (usec) {
		if (tm->add) {
			if (tm->add < 4200000)
			    rate = (1000 * tm->add) / usec;
			else if (tm->add < 42000000)
			    rate = ((100 * tm->add) / usec) * 10;
			else if (tm->add < 420000000)
			    rate = ((10 * tm->add) / usec) * 100;
			else
			    rate = (tm->add / usec) * 1000;
			rate1 = rate / 1000;
			rate2 = rate - rate1*1000;
		} else if (tm->count != 0) {
			rate1 = usec / tm->count;
		}
	} else
	    rate1 = rate2 = 0;
	print_sec = usec;
#endif	/* PARAGON860 */

	db_printf(titled ? "0x%08x %-15s %8d " :
		  "key: %8x name: %15s %s: %8d ",
		  tm->key, tm->name, titled ? print_sec : (int)sec_name, 
		  titled ? 0 : print_sec);

	if (tm->add != 0) {
		db_printf(titled ? "%8d 0x%08x %d.%03d\n" :
			  "count: %8d added: 0x%x rate: %d.%03d\n",
			  tm->count, tm->add, rate1, rate2);
	} else if (tm->count != 0) {
		db_printf(titled ? "%8d 0x%08x %d %s/cnt\n" :
			  "count: %8d added: 0x%x rate: %d %s/cnt\n",
			  tm->count, tm->add, rate1, sec_name);
	} else
		db_printf(titled ? "%8d 0x%08x <no rate>\n" :
			  "count: %8d added: 0x%x <no rate>\n",
			  tm->count, tm->add);
}

void
db_show_one_timer(
	unsigned int	key)
{
	timer_tree_t	tm;

	tm = timer_search(key);
	if (tm == TIMER_NULL) {
		db_printf("Unknown\n");
		return;
	}

	timer_show(tm, FALSE);
}
#endif	/* MACH_KDB */
