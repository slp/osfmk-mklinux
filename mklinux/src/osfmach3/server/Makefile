# Copyright (c) 1991-1998 Open Software Foundation, Inc. 
#  
# 
# 
# MkLinux
#

#
# Makefile for the linux server on top of the OSF/Mach3 micro-kernel.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
# Note 2! The CFLAGS definitions are now in the main makefile...

O_TARGET := server.o
O_OBJS   := cmu_compat.o console_feed.o fake_interrupt.o \
	free_area_pager.o gen_trap.o inode_pager.o \
	jiffies_thread.o mach3_debug.o \
	mig_user.o parent_linux.o parent_osf1.o parent_server.o \
	serv_callback_user.o serv_callback_server.o \
	serv_init.o serv_misc.o serv_notify.o \
	serv_port.o server_thread.o uniproc.o \
	user_copy.o user_memory.o ux_server_loop.o

include $(TOPDIR)/Rules.make

serv_callback_user.c serv_callback_server.c \
serv_callback_user.h serv_callback_server.h: serv_callback.defs
	$(MIG) -maxonstack 700 -I$(TOPDIR)/export-osfmach3/$(ARCH)/include \
	-user serv_callback_user.c -server serv_callback_server.c \
	-header serv_callback_user.h -sheader serv_callback_server.h \
	serv_callback.defs

fake_interrupt.o: fake_interrupt.c serv_callback_user.h serv_callback_server.h

memory_object_server.c memory_object_server.h: \
  $(TOPDIR)/export-osfmach3/$(ARCH)/include/mach/memory_object.defs
	$(MIG) -maxonstack 700 -I$(TOPDIR)/export-osfmach3/$(ARCH)/include \
	-user /dev/null -server memory_object_server.c -header /dev/null \
	-sheader memory_object_server.h \
	$(TOPDIR)/export-osfmach3/$(ARCH)/include/mach/memory_object.defs

free_area_pager.o: free_area_pager.c \
  memory_object_server.c memory_object_server.h

inode_pager.o: inode_pager.c memory_object_server.c memory_object_server.h
