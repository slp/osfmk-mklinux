/*
 *		NET_ALIAS network device aliasing definitions.
 *
 *
 * Version:	@(#)net_alias.h	0.50   4/20/97
 *
 * Author:	Juan Jose Ciarlante, <jjciarla@raiz.uncu.edu.ar>
 *
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 *	
 * Fixes:
 *	Juan Jose Ciarlante	:	Added tx/rx stats for aliases.
 *	Juan Jose Ciarlante 	:	hash_tab size now in net_alias.c
 *	Juan Jose Ciarlante	:	added sysctl interface
 */

#ifndef _NET_ALIAS_H
#define _NET_ALIAS_H

#include <linux/types.h>
#include <linux/if.h>
#include <linux/netdevice.h>

struct net_alias;
struct net_alias_info;
struct net_alias_type;


/*
 * main alias structure
 * note that *defines* dev & devname
 */

struct net_alias
{
  struct device dev;		/* alias device defn*/
  char name[IFNAMSIZ];		/* device name defn */
  unsigned hash;		/* my hash value: for quick rehash */
  unsigned slot;		/* slot number */
  void *data;			/* private data */
  struct device *main_dev;	/* pointer to main device */
  struct net_alias_type *nat;	/* alias type object bound */
  struct net_alias *next;	/* next alias (hashed linked list) */
  unsigned long rx_lookups;     /* 'fake' rx pkts */
  unsigned long tx_lookups;     /* 'fake' tx pkts */
};


/*
 *  alias structure pointed by main device
 *  it holds main device's alias hash table
 */

struct net_alias_info
{
  int n_aliases;		/* num aliases */
  int truesize;                 /* actual malloc size for struct + hashtab */
  struct device *taildev;	/* my last (alias) device */
  int max_aliases;              /* max aliases allowed for main device */
  unsigned hash_tab_size;       /* hash_tab size in elements */
  struct net_alias *hash_tab[0]; /* hashed alias table */
};

/*
 * net_alias_type class
 * declares a generic (AF_ independent) structure that will
 * manage generic to family-specific behavior.
 */

struct net_alias_type
{
  int type;			/* aliasing type: address family */
  int n_attach;	 		/* number of aliases attached */
  char name[16];		/* af_name */
  __u32 (*get_addr32)		/* get __u32 addr 'representation'*/
    (struct net_alias_type *this, struct sockaddr*);	
  int (*dev_addr_chk)		/* address checking func: */
    (struct net_alias_type *this, struct device *, struct sockaddr *);
  struct device * (*dev_select)	/* closest alias selector*/
    (struct net_alias_type *this, struct device *, struct sockaddr *sa);
  int (*alias_init_1)		/* called after alias creation: */
    (struct net_alias_type *this,struct net_alias *alias, struct sockaddr *sa);
  int (*alias_done_1)		/* called before alias deletion */
    (struct net_alias_type *this, struct net_alias *alias);
  int (*alias_print_1)	
    (struct net_alias_type *this, struct net_alias *alias, char *buf, int len);
  struct net_alias_type *next;	/* link */
};


/*
 * is dev an alias?
 */

static __inline__ int
net_alias_is(struct device *dev)
{
  return (dev->my_alias != NULL);
}


/*
 * does dev have aliases?
 */

static __inline__ int
net_alias_has(struct device *dev)
{
  return (dev->alias_info != NULL);
}

/*
 *	Initialise net_alias module
 */
extern void net_alias_init(void);

/*
 *	dev_get() with added aliasing magic
 */
extern struct device * net_alias_dev_get(char *dev_name, int aliasing_ok, int *err, struct sockaddr *sa, void *data);
extern int net_alias_dev_rehash(struct device *dev, struct sockaddr *sa);

/*
 *	PROC_FS entries
 */
extern int net_alias_getinfo(char *buf, char **, off_t , int , int );
extern int net_alias_types_getinfo(char *buf, char **, off_t , int , int );

/*
 *	net_alias_type (address family) registration 
 */
extern int register_net_alias_type(struct net_alias_type *nat, int type);
extern int unregister_net_alias_type(struct net_alias_type *nat);

/*
 *	get alias device _with_ specified address
 */
extern struct device * net_alias_dev_chk(struct device *main_dev, struct sockaddr *sa, int flags_on, int flags_off);
extern struct device * net_alias_dev_chk32(struct device *main_dev, int family, __u32 addr32, int flags_on, int flags_off);

/*
 *	get 'closest' device to specified address (returns main_dev if
 *	nothing better)
 *	if succesfull, also increment rx stats.
 */
extern struct device * net_alias_dev_rx(struct device *main_dev, struct sockaddr *sa_src, struct sockaddr *sa_dst);
extern struct device * net_alias_dev_rx32(struct device *main_dev, int family, __u32 src, __u32 dst);


/*
 * returns MY 'true' main device
 * intended for alias devices
 */

static __inline__ struct device *net_alias_main_dev(struct device *dev)
{
  return (net_alias_is(dev))? dev->my_alias->main_dev : dev;
}


/*
 * returns NEXT 'true' device
 * intended for true devices
 */

static __inline__ struct device *
net_alias_nextdev(struct device *dev)
{
  return (dev->alias_info)? dev->alias_info->taildev->next : dev->next;
}


/*
 * sets NEXT 'true' device
 * intended for main devices (treat main device as block: dev+aliases).
 */

static __inline__ struct device *
net_alias_nextdev_set(struct device *dev, struct device *nextdev)
{
  struct device *pdev = dev;
  if (net_alias_has(dev))
  {
    pdev = dev->alias_info->taildev; /* point to last dev alias */
  }
  pdev->next = nextdev;
  return nextdev;
}

/*
 *	lookup counters (used for alias devices stats)
 */
static __inline__ void net_alias_inc_rx(struct net_alias *alias)
{
        if (alias != NULL) alias->rx_lookups++;
}
static __inline__ void net_alias_inc_tx(struct net_alias *alias)
{
        if (alias != NULL) alias->tx_lookups++;
}

/*
 *	To be called when passing down a pkt, to _switch_ from alias device
 *	to actual device, also incr. alias tx counter.
 */
static __inline__ struct device *net_alias_dev_tx(struct device *dev)
{
        struct net_alias *alias = dev->my_alias;
        if (alias) {
                net_alias_inc_tx(alias);
                return alias->main_dev;
        } 
        return dev;
}

#endif  /* _NET_ALIAS_H */
