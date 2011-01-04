/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#ifndef	_OSFMACH3_CONSOLE_H
#define _OSFMACH3_CONSOLE_H

#include <mach/mach_types.h>

extern mach_port_t	osfmach3_console_port;
extern mach_port_t	osfmach3_keyboard_port;
extern mach_port_t	osfmach3_video_port;
extern vm_address_t	osfmach3_video_physaddr;
extern vm_address_t	osfmach3_video_map_base;
extern vm_size_t	osfmach3_video_map_size;
extern unsigned long	osfmach3_video_offset;
extern memory_object_t	osfmach3_video_memory_object;
extern int		osfmach3_use_mach_console;

extern boolean_t	osfmach3_con_probe(void);
extern unsigned long	osfmach3_con_init(unsigned long kmem_start);
extern void		osfmach3_launch_console_read_thread(void *tty_handle);

#define GETS_MAX 128	/* Max # of chars gets() will put in its buffer */
extern int		cnputc(int c);
extern void		cnflush(void);
extern int		cngetc(void);
extern char		*gets(char buf[GETS_MAX]);

#endif	/* _OSFMACH3_CONSOLE_H */
