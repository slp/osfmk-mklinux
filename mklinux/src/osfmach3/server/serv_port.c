/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#include <linux/autoconf.h>

#include <port_obj.h>
#include <mach/mach_port.h>

#include <osfmach3/serv_port.h>
#include <osfmach3/mach_init.h>
#include <osfmach3/mach3_debug.h>
#include <osfmach3/assert.h>

#include <linux/kernel.h>
#include <linux/malloc.h>
#include <linux/mm.h>

#define PORTHACK_LITTLE_MAGIC 1685

#define PORTHACK_HASH_SIZE 4096
#define PORTHACK_HASH_POOL 16
struct porthash {
	mach_port_t pr_port;
	void *pr_value;
	struct porthash *pr_next;
} *porthash[PORTHACK_HASH_SIZE],
  porthash_pool[PORTHACK_HASH_POOL],
  *porthash_poolptr;

#if	CONFIG_OSFMACH3_DEBUG
int porthack_allocs;
int porthack_registers;
int debug_port_reuse = 1;
#endif	/* CONFIG_OSFMACH3_DEBUG */


#if 1	/* Check mach_port_deallocate. */

kern_return_t
serv_port_deallocate_once(
	mach_port_t	task,
	mach_port_t	port)
{
	kern_return_t kr;
#if	CONFIG_OSFMACH3_DEBUG
	mach_port_urefs_t refs;
	mach_port_type_t ptype;
	mach_port_right_t right;

	kr = mach_port_type(task, port, &ptype);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(0, kr,
			    ("serv_port_deallocate_once(0x%x,0x%x): "
			     "mach_port_type", task, port));
		panic("serv_port_deallocate_once: mach_port_type failed");
	}
	switch (ptype) {
	    case MACH_PORT_TYPE_DEAD_NAME:
		right = MACH_PORT_RIGHT_DEAD_NAME;
		break;
	    case MACH_PORT_TYPE_SEND:
		right = MACH_PORT_RIGHT_SEND;
		break;
	    default:
		printk("type = 0x%x\n", ptype);
		panic("serv_port_deallocate_once: bad type");
		return mach_port_destroy(task, port);
	}
	kr = mach_port_get_refs(task, port, right, &refs);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(0, kr,
			    ("serv_port_deallocate_once(0x%x,0x%x): "
			     "mach_port_get_refs", task, port));
		panic("serv_port_deallocate_once: can't get refs");
	} else if (refs != 1) {
		printk("serv_port_dellocate_once(0x%x,0x%x): refs=%d\n",
		       task, port, refs);
		panic("serv_port_deallocate_once: refs != 1");
	}
#endif	/* CONFIG_OSFMACH3_DEBUG */
	kr = mach_port_deallocate(task, port);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(0, kr,
			    ("serv_port_deallocate_once(0x%x,0x%x): "
			     "mach_port_deallocate", task, port));
		panic("serv_port_deallocate_once: can't deallocate");
	}
#if	CONFIG_OSFMACH3_DEBUG
	/* Note race condition: someone else might reallocate the port. */
	kr = mach_port_type(task, port, &ptype);
	if (kr == KERN_SUCCESS) {
		printk("serv_port_deallocate_once(0x%x,0x%x): "
		       "port type = 0x%x\n", task, port, ptype);
		panic("serv_port_deallocate_once: "
		      "port not destroyed on deallocate");
	}
#endif	/* CONFIG_OSFMACH3_DEBUG */
	return KERN_SUCCESS;
}

kern_return_t
serv_port_destroy_receive(
	mach_port_t	task,
	mach_port_t	port)
{
	kern_return_t kr;
#if	CONFIG_OSFMACH3_DEBUG
	mach_port_type_t ptype;

	kr = mach_port_type(task, port, &ptype);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(0, kr,
			    ("serv_port_destroy_receive(0x%x,0x%x): "
			     "mach_port_type", task, port));
		panic("serv_port_destroy_receive: can't get type");
	}
	if (ptype != MACH_PORT_TYPE_RECEIVE) {
		printk("serv_port_destroy_receive(0x%x,0x%x): ptype=0x%x\n",
		       task, port, ptype);
		panic("serv_port_destroy_receive: not (pure) receive right");
	}
#endif	/* CONFIG_OSFMACH3_DEBUG */
	kr = mach_port_mod_refs(task, port, MACH_PORT_RIGHT_RECEIVE, -1);
	if (kr != KERN_SUCCESS) {
		MACH3_DEBUG(0, kr,
			    ("serv_port_destroy_receive(0x%x,0x%x): "
			     "mach_port_mod_refs(RECEIVE,-1)",
			     task, port));
		panic("serv_port_destroy_receive: can't destroy");
	}
#if	CONFIG_OSFMACH3_DEBUG
	kr = mach_port_type(task, port, &ptype);
	if (kr == KERN_SUCCESS) {
		printk("serv_port_destroy_receive(0x%x,0x%x): ptype=0x%x\n",
		       task, port, ptype);
		panic("serv_port_destroy_receive: port not destroyed");
	}
#endif	/* CONFIG_OSFMACH3_DEBUG */
	return KERN_SUCCESS;
}
#endif	/* 1 (Checking mach_port_deallocate) */


/*
 * The following functions are for managing receive rights.  We used to
 * rename these to corresponding data structures in the server.  Now we
 * provide these functions which allow a (void *) to be associated with
 * a newly-allocated receive right in a way that makes it efficient to
 * retrieve.
 */


kern_return_t
serv_port_rename(
	mach_port_t	port,
	void		*server_name)
{
	port_set_obj_value_type(port, server_name, PORTHACK_LITTLE_MAGIC);
	return KERN_SUCCESS;
}

/*
 * Allocate a receive right, setting *mach_name.  name is the value we
 * will associate with the right, i.e., what used to be the name demanded
 * from Mach for the port.
 */
kern_return_t
serv_port_allocate_name(
	mach_port_t	*mach_name,
	void		*server_name)
{
	kern_return_t kr;

	kr = mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE,
				mach_name);
	if (kr != KERN_SUCCESS)
		return kr;
	serv_port_rename(*mach_name, server_name);
	return KERN_SUCCESS;
}

kern_return_t
serv_port_allocate_subsystem(
	mach_port_t	subsystem,
	mach_port_t	*mach_name,
	void		*server_name)
{
	kern_return_t kr;

	kr = mach_port_allocate_subsystem(mach_task_self(), subsystem,
					  mach_name);
	if (kr != KERN_SUCCESS)
		return kr;
	serv_port_rename(*mach_name, server_name);
	return KERN_SUCCESS;
}

/*
 * Retrieve the value associated with a receive right.
 */
#ifndef	serv_port_name	/* may be a macro in osfmach3/port.h */
void *
serv_port_name(
	mach_port_t name)
{
	ASSERT(port_get_obj_type(name) == PORTHACK_LITTLE_MAGIC);
	return serv_port_name_macro(name);
}
#endif	/* serv_port_name */

/*
 * Destroy a port right allocated with serv_port_allocate_name, and the
 * associated data structure.
 */
kern_return_t
serv_port_destroy(
	mach_port_t name)
{
	kern_return_t kr;

	ASSERT(port_get_obj_type(name) == PORTHACK_LITTLE_MAGIC);
	kr = serv_port_destroy_receive(mach_task_self(), name);
	return kr;
}


/*
 * The following functions are for managing send rights.  We use a hash
 * table to allow a value to be associated with any right.  This is less
 * efficient than the method used for receive rights, so it should be
 * avoided on critical paths such as system calls.
 */

/*
 * Allocate a porthash structure.  Since this function may be called before
 * the malloc subsystem is ready, we dole the first few allocations out of
 * a static array.  It might be better to use zalloc.
 */
struct porthash *
serv_porthash_alloc(void)
{
	struct porthash *ph;
	static boolean_t inited = FALSE;

	if (!inited) {
		int i;

		porthash_poolptr = porthash_pool;
		for (i = 0; i < PORTHACK_HASH_POOL - 1; i++)
			porthash_pool[i].pr_next = &porthash_pool[i + 1];
		inited = TRUE;
	}
	if (porthash_poolptr != NULL) {
		ph = porthash_poolptr;
		porthash_poolptr = ph->pr_next;
		return ph;
	}
	/*
	 * We don't want to block while holding the spinlock, so first try a
	 * non-blocking malloc, and if that fails, release the lock and block.
	 */
	ph = (struct porthash *) kmalloc(sizeof (struct porthash), GFP_KERNEL);
	return ph;
}

/*
 * Free a porthash structure.  Calling FREE might not work here, because
 * current_thread() might be undefined.  So we hold on to our structures
 * forever.
 */
void
serv_porthash_free(
	struct porthash *ph)
{
	ph->pr_next = porthash_poolptr;
	porthash_poolptr = ph;
}

/*
 * Look up a value in the hash table and return the place where the
 * corresponding structure either is or should be inserted.  The hash
 * function (stolen from device_reply_hdlr.c) is supposedly good for
 * port names allocated by Mach.
 */
struct porthash **
serv_port_hash(
	mach_port_t name)
{
	unsigned int hash;
	struct porthash **p;

	ASSERT(MACH_PORT_VALID(name));
	hash = (int) name;
	hash = ((hash & 0xff) + (hash >> 8)) % PORTHACK_HASH_SIZE;
	for (p = &porthash[hash]; *p; p = &(*p)->pr_next) {
		if ((*p)->pr_port == name)
			break;
	}
	return p;
}

/*
 * Associate a value with a port name.
 */
void
serv_port_register(
	mach_port_t	name,
	void		*value)
{
	struct porthash **p, *ph;

	ph = serv_porthash_alloc();
	if (ph == NULL) {
		panic("serv_port_register: can't allocate hash entry");
	}
	p = serv_port_hash(name);
#if	CONFIG_OSFMACH3_DEBUG
	if (debug_port_reuse && *p != NULL) {
		printk("reused port 0x%x, old 0x%x new 0x%x\n",
		       (int) name, (int) (*p)->pr_value, (int) value);
		Debugger("port reuse");
		serv_porthash_free(ph);
		return;
	}
	porthack_registers++;
#endif	/* CONFIG_OSFMACH3_DEBUG */
	ph->pr_port = name;
	ph->pr_value = value;
	ph->pr_next = NULL;
	*p = ph;
}

/*
 * Break the association between a port name and its value.  A value must
 * have been registered.
 */
void
serv_port_unregister(
	mach_port_t name)
{
	struct porthash **p, *ph;

	p = serv_port_hash(name);
	if (*p == NULL)
		panic("serv_port_unregister(0x%x): not registered", name);
	else {
		ph = *p;
		*p = ph->pr_next;
		serv_porthash_free(ph);
#if	CONFIG_OSFMACH3_DEBUG
		porthack_registers--;
#endif	/* CONFIG_OSFMACH3_DEBUG */
	}
}

/*
 * Return the value associated with a port name, or NULL if there is none.
 */
void *
serv_port_lookup(
	mach_port_t name)
{
	struct porthash *ph;

	ph = *serv_port_hash(name);
	if (ph == NULL)
		return NULL;
	else
		return ph->pr_value;
}
