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

#ifndef	_AUDIO_
#define	_AUDIO_

#include <kern/lock.h>
#include <kern/queue.h>
#include <himem.h>

/*
 * Queue for audio output.
 */
struct auqueue {
	queue_head_t	auq_head;	/* queue of io_req_t */
	unsigned	auq_len;	/* length of queue */
	unsigned	auq_maxlen;	/* maximum length of queue */
	queue_entry_t	auq_wait;	/* first entry for which
					   a thread is waiting */
	decl_simple_lock_data(,auq_lock) /* lock for queue and counters */
};

struct au_req {
	struct au_req *	au_next;	/* next, ... */
	struct au_req *	au_prev;	/* prev pointers: link in done,
					   defered, or in-progress list */
	int		dev;		/* audio device */
	io_req_t	ior;		/* ior corresponding to this dma request */
	vm_offset_t	virt_addr;	/* virtual start add. of dma request */
	vm_offset_t	phys_addr;	/* physical start add. of dma request */
	int		length;		/* length of the dma request */
	unsigned long	flags;		/* see defines below */
#if HIMEM 
	hil_t		hil;		/* Used for himem convert/revert */
#endif /* HIMEM */

};

/* values used for 'flags' field of struct 'au_req' */
#define AUR_NONE	0x0
#define AUR_IS_LAST 	0x1	/* last dma request of the ior */

#if HIMEM
#define AUR_IS_LOW_MEM	0x2	/* Physical address of DMA is low memory */

extern int free_low_page;		/* number of free low memory pages */
decl_simple_lock_data(,auq_free_low)	/* reserved by himem_reserve() */
#endif /* HIMEM */

typedef struct au_req *au_req_t;

extern struct auqueue *auq_out;

/*
 * Output audio queue (auq_out)
 * Entries are added to and deleted from these structures by these macros,
 *  which should be called with ipl raised to splimp().
 * XXX locking XXX
 */

#define	AU_QFULL(auq)		((auq)->auq_maxlen > 0 && \
				 (auq)->auq_len >= (auq)->auq_maxlen)
#define	AU_ENQUEUE(auq, aur) {					\
	simple_lock(&(auq)->auq_lock);			\
	enqueue_tail(&(auq)->auq_head, (queue_entry_t)aur);	\
	(auq)->auq_len++;					\
	simple_unlock(&(auq)->auq_lock);			\
}

#define	AU_ENQUEUE_WAIT(auq, aur) { 				\
	simple_lock(&(auq)->auq_lock); 			\
	enqueue_tail(&(auq)->auq_head, (queue_entry_t)aur); 	\
	(auq)->auq_len++; 					\
	if ((aur->ior->io_op & IO_SYNC) == 0) 			\
	    while (AU_QFULL(auq)) { 				\
		if ((auq)->auq_wait == (queue_entry_t)0) 	\
		    (auq)->auq_wait = (queue_entry_t)(aur); 	\
		thread_sleep_simple_lock((event_t)((auq)->auq_wait),	\
			simple_lock_addr((auq)->auq_lock), FALSE); \
		simple_lock(&(auq)->auq_lock); 		\
	    } 							\
	simple_unlock(&(auq)->auq_lock); 			\
}

#define	AU_PREPEND(auq, aur) {					\
	simple_lock(&(auq)->auq_lock);			\
	enqueue_head(&(auq)->auq_head, (queue_entry_t)aur);	\
	(auq)->auq_len++;					\
	simple_unlock(&(auq)->auq_lock);			\
}

#define	AU_DEQUEUE(auq, aur) { 					\
	simple_lock(&(auq)->auq_lock); 			\
	if (((aur) = (au_req_t)dequeue_head(&(auq)->auq_head)) != 0) { \
	    (auq)->auq_len--;					\
	    if ((auq)->auq_wait != (queue_entry_t)0) { 		\
		assert((auq)->auq_maxlen > 0); 			\
		if ((auq)->auq_len < (auq)->auq_maxlen) { 	\
		    thread_wakeup((event_t)(auq)->auq_wait); 	\
		    (auq)->auq_wait = queue_next((auq)->auq_wait); \
		    if (queue_end(&(auq)->auq_head, (auq)->auq_wait)) \
			(auq)->auq_wait = (queue_entry_t)0; 	\
		} 						\
	    } 							\
	}							\
	simple_unlock(&(auq)->auq_lock); 			\
}

#define	AUQ_MAXLEN	50

#define	AUQ_INIT(auq) {						\
	queue_init(&(auq)->auq_head);				\
	simple_lock_init(&(auq)->auq_lock, ETAP_IO_REQ);	\
	(auq)->auq_len = 0;					\
	(auq)->auq_maxlen = AUQ_MAXLEN;				\
	(auq)->auq_wait = (queue_entry_t)0; 			\
}
#endif	/* _AUDIO_ */
