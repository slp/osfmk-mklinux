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
 * Revision 2.2  93/03/18  10:37:04  mrt
 * 	Started, from Berkeley's driver.
 * 	[93/03/09            af]
 * 
 */
/* CMU_HIST */
/*
 * Copyright (c) 1991, 1992 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the Computer Systems
 * 	Engineering Group at Lawrence Berkeley Laboratory.
 * 4. The name of the Laboratory may not be used to endorse or promote 
 *    products derived from this software without specific prior written 
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	$Header: /u1/osc/rcs/mach_kernel/chips/audio_defs.h,v 1.1.6.1 1995/01/06 18:49:57 devrcs Exp $ (LBL)
 */

#define AUCB_SIZE 4096
#define AUCB_MOD(k)	((k) & (AUCB_SIZE - 1))

#define AUCB_INIT(cb)	((cb)->cb_head = (cb)->cb_tail = (cb)->cb_drops = \
			 (cb)->cb_pdrops = 0)

#define AUCB_EMPTY(cb)	((cb)->cb_head == (cb)->cb_tail)
#define AUCB_FULL(cb)	(AUCB_MOD((cb)->cb_tail + 1) == (cb)->cb_head)
#define AUCB_LEN(cb)	(AUCB_MOD((cb)->cb_tail - (cb)->cb_head))

#define MAXBLKSIZE (AUCB_SIZE / 2)
#define DEFBLKSIZE 128

#ifndef LOCORE

/*
 * Our own circular buffers, used if not doing DMA.
 * [af: with some work we could use the circbuf.c code instead]
 */
typedef struct au_cb {
	int	cb_head;		/* queue head */
	int	cb_tail;		/* queue tail */
	int	cb_thresh;		/* threshold for wakeup */
	unsigned int	cb_waking;	/* needs wakeup at softint level */
	unsigned int	cb_pause;	/* io paused */
	unsigned int	cb_drops;	/* missed samples from over/underrun */
	unsigned int	cb_pdrops;	/* sun compat -- paused samples */
	unsigned char	cb_data[AUCB_SIZE];	/* data buffer */
} au_cb_t;

/*
 * Handle on a bi-directional stream of samples
 */
typedef struct au_io {
	unsigned int	au_stamp;	/* time stamp */
	int	au_lowat;		/* xmit low water mark (for wakeup) */
	int	au_hiwat;		/* xmit high water mark (for wakeup) */
	int	au_blksize;		/* recv block (chunk) size */
	int	au_backlog;		/* # samples of xmit backlog to gen. */
	struct	au_cb au_rb;		/* read (recv) buffer */
	struct	au_cb au_wb;		/* write (xmit) buffer */
} au_io_t;

/*
 * Interface to specific chips
 */
typedef struct {
	void		(*init)();
	void		(*close)();
	void		(*setport)();
	int		(*getport)();
	void		(*setgains)();
	void		(*getgains)();
	io_return_t	(*setstate)();
	io_return_t	(*getstate)();
} audio_switch_t;

/*
 * Callbacks into audio module, and interface to kernel
 */
extern void		audio_attach(
				void *,
				audio_switch_t *,
				void **);

extern boolean_t	audio_hwintr(
				void *,
				unsigned int,
				unsigned int *);

extern io_return_t	audio_open(
				dev_t,
				dev_mode_t,
				io_req_t );

extern void		audio_close(
				dev_t );

extern io_return_t	audio_read(
				dev_t,
				io_req_t );

extern io_return_t	audio_write(
				dev_t, io_req_t );

extern io_return_t	audio_get_status(
				dev_t,
				dev_flavor_t,
				dev_status_t,
				mach_msg_type_number_t *);

extern io_return_t	audio_set_status(
				dev_t,
				dev_flavor_t,
				dev_status_t,
				mach_msg_type_number_t);

#endif /* !defined(LOCORE) */
