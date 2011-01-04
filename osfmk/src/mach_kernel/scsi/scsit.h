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
#ifndef	_SCSI_SCSIT_H_
#define	_SCSI_SCSIT_H_

#include <mach/error.h>

#define SCSIT_ROUND_SIZE 256

typedef int scsit_return_t;

#define	SCSITSUB			8
#define	SCSITERR(errno)		(err_dipc | err_sub(SCSITSUB) | errno)

#define	SCSIT_SUCCESS			ERR_SUCCESS /* operation successful */
#define	SCSIT_FAILURE			SCSITERR(1)
#define SCSIT_UNINITIALIZED_HOST	SCSITERR(2)
#define SCSIT_RESOURCE_SHORTAGE		SCSITERR(3)

extern int _scsit_node_self;

#define scsit_node_self() _scsit_node_self

typedef void *scsit_handle_t;

typedef void (*scsit_complete_t)
    (scsit_handle_t, void *, char *, unsigned int, scsit_return_t);

typedef void (*scsit_remote_complete_t)
    (scsit_handle_t, node_name, char *, unsigned int, scsit_return_t);

typedef scsit_complete_t scsit_recv_complete_t;
typedef scsit_complete_t scsit_send_complete_t;


typedef void (*scsit_target_select_t)
    (node_name, int, unsigned int);

extern void scsit_init(
		       unsigned int		max_pending);

scsit_return_t scsit_lun_allocate(
				  scsit_recv_complete_t	recv_complete,
				  scsit_target_select_t	target_select,
				  scsit_send_complete_t	send_complete,
				  unsigned int 		*lun);

extern scsit_return_t scsit_node_setup(
				       node_name	node);

extern scsit_return_t scsit_handle_alloc(
					 scsit_handle_t *handle);
extern scsit_return_t scsit_handle_mismatch(
					 scsit_handle_t handle);
extern scsit_return_t scsit_handle_free(
					scsit_handle_t handle);

extern scsit_return_t scsit_send(
				 scsit_handle_t		handle,
				 node_name		node,
				 int			lun,
				 void			*opaque_handle,
				 char			*buffer,
				 unsigned int		size,
				 boolean_t		sg);

extern scsit_return_t scsit_receive(
				    scsit_handle_t	handle,
				    node_name		node,
				    int			lun,
				    void		*opaque_handle,
				    char		*buffer,
				    unsigned int	size,
				    boolean_t		sg);

extern scsit_return_t scsit_remote_op(
				      scsit_handle_t	handle,
				      node_name		node,
				      int		op,
				      unsigned int	remote_pa,
				      char		*buffer,
				      unsigned int	size,
				      scsit_remote_complete_t complete);

#define SCSIT_STATS MACH_ASSERT && MACH_KDB

#if	SCSIT_STATS
extern void scsit_stats(void);
#endif

#endif	/*_SCSI_SCSIT_H_*/
