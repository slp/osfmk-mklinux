/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#ifndef	_OSFMACH3_INODE_PAGER_H_
#define _OSFMACH3_INODE_PAGER_H_

#include <mach/mach_port.h>
#include <mach/memory_object.h>

#include <linux/fs.h>
#include <linux/wait.h>

#define INODE_PAGER_MAX_CLUSTER_SIZE	8	/* 8 pages */
#define INODE_PAGER_MESSAGE_SIZE	8192

/* Inode memory object */
struct i_mem_object {
	memory_object_t	imo_mem_obj;	  /* memory object manager port */
	memory_object_control_t	imo_mem_obj_control;  /* mem obj control port */
	int		imo_refcnt;	  /* reference count */
	boolean_t	imo_cacheable;	  /* cacheable after termination ? */
	boolean_t	imo_attrchange;	  /* m_o_change_attr in progress */
	struct wait_queue *imo_attrchange_wait;	/* wait queue for above */
	memory_object_copy_strategy_t imo_copy_strategy;
	int		imo_errors;	  /* pageout error count */
	struct inode	*imo_inode;	  /* inode for this object */
	mach_port_urefs_t imo_urefs;	  /* user refs on request port */
};

extern memory_object_t	inode_pager_setup(struct inode *inode);
extern void		inode_pager_uncache(struct inode *inode);
extern void		inode_pager_uncache_now(struct inode *inode);
extern void		inode_pager_release(struct inode *inode);

extern struct i_mem_object *imo_create(struct inode *inode,
				       boolean_t allocate_port);
extern void		imo_ref(struct i_mem_object *imo);

#endif	/* _OSFMACH3_INODE_PAGER_H_ */
