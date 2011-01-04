/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#ifndef	_OSFMACH3_CHAR_DEV_H
#define _OSFMACH3_CHAR_DEV_H
#include <linux/fs.h>
#include <linux/tty.h>

struct osfmach3_chrdev_struct {
	const char 	*name;
	int 		max_minors;
	int		(*dev_to_name)(kdev_t, char *);
};
extern struct osfmach3_chrdev_struct osfmach3_chrdevs[];
extern int *osfmach3_chrdev_refs[];

extern int osfmach3_register_chrdev(unsigned int major,
				    const char *name,
				    int max_minors,
				    int (*dev_to_name)(kdev_t, char *),
				    int *refs_array);

extern int osfmach3_tty_dev_to_name(kdev_t dev, char *name);
extern int osfmach3_tty_open(struct tty_struct *tty);
extern void osfmach3_tty_start(struct tty_struct *tty);

#endif	/* _OSFMACH3_CHAR_DEV_H */
