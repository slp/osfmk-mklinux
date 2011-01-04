/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

#ifndef _OSFMACH3_SERV_PORT_H_
#define _OSFMACH3_SERV_PORT_H_ 1

#include <linux/autoconf.h>

#include <mach/port.h>
#include <mach/kern_return.h>
#include <mach/rpc.h>

#if 	CONFIG_OSFMACH3_DEBUG
#define PORT_OBJ_ASSERT
#endif	/* CONFIG_OSFMACH3_DEBUG */
#include <port_obj.h>

/* Functions for receive rights. */
kern_return_t serv_port_rename(mach_port_t port, void *server_name);
kern_return_t serv_port_allocate_name(mach_port_t *mach_name,
				      void *server_name);
kern_return_t serv_port_allocate_subsystem(mach_port_t subsystem,
					   mach_port_t *mach_name,
					   void *server_name);
kern_return_t serv_port_rename(mach_port_t port, void *server_name);
void *serv_port_name(mach_port_t name);
#define serv_port_name_macro(name) port_get_obj_value(name)
#if	CONFIG_OSFMACH3_DEBUG
#else	/* CONFIG_OSFMACH3_DEBUG */
#define serv_port_name(name) serv_port_name_macro(name)
#endif	/* CONFIG_OSFMACH3_DEBUG */
kern_return_t serv_port_destroy(mach_port_t name);

/* Functions for send rights. */
void serv_port_register(mach_port_t name, void *value);
void serv_port_unregister(mach_port_t name);
void *serv_port_lookup(mach_port_t name);

#endif	/* _OSFMACH3_SERV_PORT_H_ */
