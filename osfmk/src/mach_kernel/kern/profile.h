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
/* CMU_HIST */
/*
 * Revision 2.1.2.1  92/09/15  17:22:51  jeffreyh
 * 	Moved from mach/profil.h to kern/profile.h
 * 	Removed mach/profilparam.h
 * 	[92/07/24            bernadat]
 * 
 * Revision 2.1.6.1  92/02/18  19:13:54  jeffreyh
 * 	Fixed Profiling code. [emcmanus@gr.osf.org]
 * 	Data structures and macros used for the  profiling service.
 * 	(Bernard Tabib & Andrei Danes @ gr.osf.org)
 * 	[91/09/26  04:49:23  bernadat]
 * 
 */
/* CMU_ENDHIST */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */

/*
 */

#ifndef _KERN_PROFILE_H
#define _KERN_PROFILE_H

#include <mach/boolean.h>
#include <vm/vm_kern.h> 

#if DCI

#define NB_PROF_BUFFER          6       /* # of buffers in port que + 2 */
#define	SIZE_PROF_BUFFER	600	/* size of a profil buffer (in natural_t) */
				        /* 200*number of items to profile */
					/* -> at most 1 packet every 2 secs */
					/*	profiled thread */

struct	prof_data {
	struct ipc_port *prof_port;	/* where to send a full buffer */
	task_t task;		       /* self-ref to task */

	struct buffer {
	    queue_chain_t p_list;
	    natural_t	*p_zone;	/* points to the actual storage area */
	    int p_index[NCPUS];	        /* next slot to be filled by cpu # */
	    int p_dropped;		/* # dropped samples when full */
	    boolean_t p_full[NCPUS];    /* is current buffer/cpu full ? */
	    struct prof_data *p_prof;	/* base to get prof_port */
	    char p_wakeme;		/* do wakeup when sent */
	} prof_area[NB_PROF_BUFFER];

	int		prof_index;	/* index of the buffer structure */
					/*   currently in use */

};

#else

#define	NB_PROF_BUFFER		4	/* number of buffers servicing a */
#define	SIZE_PROF_BUFFER	200	/* size of a profil buffer (in natural_t) */
					/* -> at most 1 packet every 2 secs */
					/*	profiled thread */

struct	prof_data {
	struct ipc_port *prof_port;	/* where to send a full buffer */

	struct buffer {
	    queue_chain_t p_list;
	    natural_t	*p_zone;	/* points to the actual storage area */
	    int p_index;		/* next slot to be filled */
	    int p_dropped;		/* # dropped samples when full */
	    boolean_t p_full;		/* is the current buffer full ? */ 
	    struct prof_data *p_prof;	/* base to get prof_port */
	    char p_wakeme;		/* do wakeup when sent */
	} prof_area[NB_PROF_BUFFER];

	int		prof_index;	/* index of the buffer structure */
					/*   currently in use */

};
#endif /* DCI */

typedef struct prof_data	*prof_data_t;
#define NULLPROFDATA ((prof_data_t) 0)
typedef struct buffer		*buffer_t;
#define NULLPBUF ((buffer_t) 0)

/* Macros */

#define	set_pbuf_nb(pbuf, nb) \
         (((nb) >= 0 && (nb) < NB_PROF_BUFFER) \
	 ? (pbuf)->prof_index = (nb), 1 \
	 : 0)


#define	get_pbuf_nb(pbuf) \
	(pbuf)->prof_index


extern vm_map_t kernel_map; 

/* MACRO set_pbuf_value 
** 
** enters the value 'val' in the buffer 'pbuf' and returns the following
** indications:     0: means that a fatal error occured: the buffer was full
**                       (it hasn't been sent yet)
**                  1: means that a value has been inserted successfully
**		    2: means that we'v just entered the last value causing 
**			the current buffer to be full.(must switch to 
** 			another buffer and signal the sender to send it)
*/ 

#if DCI
#define DCIset_pbuf_value(pbuf, ncpu, val1, val2, val3) \
          { \
	  register buffer_t a = &((pbuf)->prof_area[(pbuf)->prof_index]); \
	  register int i ;\
	  register boolean_t f = a->p_full[ncpu]; \
	  if (f == TRUE ) {\
	     a->p_dropped++; \
             *(val1) = 0; \
	  } else { \
	    i = a->p_index[ncpu]; \
	    a->p_zone[i++] = *(val1); \
	    a->p_zone[i++] = *(val2); \
	    a->p_zone[i] = *(val3); \
	    a->p_index[ncpu] += 3; \
	    if (i == SIZE_PROF_BUFFER-1) { \
               a->p_full[ncpu] = TRUE; \
               *(val1) = 2; \
            } \
            else \
		*(val1) = 1; \
          } \
	}

#define	DCIreset_pbuf_area(pbuf, ncpu) \
	{ \
	 int i; \
	 (pbuf)->prof_index = ((pbuf)->prof_index + 1) % NB_PROF_BUFFER; \
	 i = (pbuf)->prof_index; \
	 (pbuf)->prof_area[i].p_index[ncpu] = 0; \
	 (pbuf)->prof_area[i].p_dropped = 0; \
	}

#define DCIreset_pbuf_areas(pbuf)	\
	{ \
	  int j; \
	  for(j=0;j<NCPUS;j++) \
		DCIreset_pbuf_area(pbuf,j); \
	} 
#else	/* !DCI */
#if	MACH_PROF

#define set_pbuf_value(pbuf, val) \
	 { \
	  register buffer_t a = &((pbuf)->prof_area[(pbuf)->prof_index]); \
	  register int i ;\
	  register boolean_t f = a->p_full; \
			  \
	  if (f == TRUE ) {\
	     a->p_dropped++; \
             *(val) = 0L; \
	  } else { \
	    i = a->p_index++; \
	    a->p_zone[i] = *(val); \
	    if (i == SIZE_PROF_BUFFER-1) { \
               a->p_full = TRUE; \
               *(val) = 2; \
            } \
            else \
		*(val) = 1; \
          } \
	}
         
#define	reset_pbuf_area(pbuf) \
	{ \
	 int i; \
	 (pbuf)->prof_index = ((pbuf)->prof_index + 1) % NB_PROF_BUFFER; \
	 i = (pbuf)->prof_index; \
	 (pbuf)->prof_area[i].p_index = 0; \
	 (pbuf)->prof_area[i].p_dropped = 0; \
	}

#endif	/* MACH_PROF */
#endif /* !DCI */

/*
** Global variable: the head of the queue of buffers to send 
** It is a queue with locks (uses macros from queue.h) and it
** is shared by hardclock() and the sender_thread() 
*/

mpqueue_head_t prof_queue; 

#if DCI
extern void	DCIprofile(
			int     cpu,             /* current cpu thread is on */
			natural_t	thread,     /* current thread */
			natural_t	sp,         /* stack pointer */
			natural_t	pc,         /* program counter */
			prof_data_t	pbuf);      /* trace/prof data area */
#else
extern void	profile(
			natural_t	pc,         /* program counter */
			prof_data_t	pbuf);      /* trace/prof data area */
#endif /* DCI */

#if MACH_PROF

#define task_prof_init(task) \
	task->task_profiled = FALSE; \
     	task->profil_buffer = NULLPROFDATA;

#define act_prof_init(thr_act, task) \
	thr_act->act_profiled = task->task_profiled;	\
	thr_act->profil_buffer = task->profil_buffer;

#define task_prof_deallocate(task) \
	if (task->profil_buffer) \
		task_sample(task, MACH_PORT_NULL); \

#define act_prof_deallocate(thr_act) \
	if (thr_act->act_profiled_own && thr_act->profil_buffer)  \
		thread_sample(thr_act, MACH_PORT_NULL); \

extern kern_return_t thread_sample(thread_act_t, ipc_port_t);
extern kern_return_t task_sample(task_t, ipc_port_t);

#else /* !MACH_PROT */

#define task_prof_init(task)
#define act_prof_init(thr_act, task)
#define task_prof_deallocate(task)
#define act_prof_deallocate(thr_act)

		
#endif	/* !MACH_PROF */

#endif	/* _KERN_PROFILE_H */
