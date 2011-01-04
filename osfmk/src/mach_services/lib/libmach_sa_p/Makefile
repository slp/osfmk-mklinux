#
# Copyright 1991-1998 by Open Software Foundation, Inc. 
#              All Rights Reserved 
#  
# Permission to use, copy, modify, and distribute this software and 
# its documentation for any purpose and without fee is hereby granted, 
# provided that the above copyright notice appears in all copies and 
# that both the copyright notice and this permission notice appear in 
# supporting documentation. 
#  
# OSF DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
# INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
# FOR A PARTICULAR PURPOSE. 
#  
# IN NO EVENT SHALL OSF BE LIABLE FOR ANY SPECIAL, INDIRECT, OR 
# CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
# LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT, 
# NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION 
# WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
#
#
# MkLinux

PURE_MACH		= 1

VPATH                   = ../libmach:../libmach/${TARGET_MACHINE}:${MAKETOP}mach_kernel/mach:${MAKETOP}mach_kernel/mach_debug:${MAKETOP}mach_kernel/device

LIBRARIES		= libmach_sa_p.a
EXPLIB_TARGETS		= export_libmach_sa_p.a

INCFLAGS                = -I../libmach -I../libmach/${TARGET_MACHINE}
ILIST			= libmach_sa_p.a
IDIR			= ${MACH3_LIBRARY_IDIR}
IMODE			= 644

DEFINES			= -DMACH_IPC_COMPAT=0 -DSTANDALONE -DTypeCheck=0
CFLAGS			= ${DEFINES} -DGPROF -pg
MIGFLAGS		= ${DEFINES}
MIG3_DEFS		= exc.defs memory_object.defs mach.defs \
			  mach_host.defs mach_debug.defs mach_norma.defs \
			  device.defs device_request.defs device_reply.defs \
			  mach_port.defs default_pager_object.defs notify.defs
OFILES			= _setjmp.o bcopy.o bzero.o \
			  default_pager_object_server.o \
			  default_pager_object_user.o device_reply_server.o \
			  device_request_user.o device_server.o \
			  device_user.o doprnt.o error_codes.o \
			  exc_server.o exc_user.o exit.o \
			  gcc.o get_privileged_ports.o \
			  htg_vm_map.o mach_debug_server.o \
			  mach_debug_user.o mach_error_string.o \
			  mach_host_server.o mach_host_user.o \
			  mach_init.o mach_init_sa.o mach_msg.o \
			  mach_msg_destroy.o mach_msg_receive.o \
			  mach_msg_send.o mach_msg_server.o \
			  mach_norma_server.o mach_norma_user.o \
			  mach_port_server.o mach_port_user.o \
			  mach_server.o mach_user.o memcpy.o \
			  memory_object_server.o \
			  memory_object_user.o mig_allocate.o \
			  mig_deallocate.o mig_reply_setup.o \
			  mig_strncpy.o mig_support.o \
			  ms_mach_port_allocate.o ms_mach_port_allocate_name.o \
			  ms_mach_port_deallocate.o \
			  ms_mach_port_insert_right.o \
			  ms_task_create.o ms_task_set_special_port.o \
			  ms_task_suspend.o ms_task_terminate.o \
			  ms_thread_depress_abort.o ms_vm_allocate.o \
			  ms_vm_deallocate.o ms_vm_map.o \
			  notify_server.o notify_user.o \
			  panic.o printf.o slot_name.o \
			  sprintf.o strcat.o strcmp.o strcpy.o \
			  strlen.o strncpy.o \
			  ${${TARGET_MACHINE}COBJS} \
			  ${${TARGET_MACHINE}SOBJS}

.if exists(${TARGET_MACHINE}/machdep.mk)
.include "${TARGET_MACHINE}/machdep.mk"
.endif
.include <${RULES_MK}>

memory_object_server.c \
memory_object_user.c:		memory_object.defs
	${MIG3} ${_MIGFLAGS_} $> -server memory_object_server.c \
				 -user   memory_object_user.c

default_pager_object_server.c \
default_pager_object_user.c:	default_pager_object.defs
	${MIG3} ${_MIGFLAGS_} $> -server default_pager_object_server.c \
				 -user   default_pager_object_user.c

mach_interface.h mach_server.c : mach.defs
	${MIG3} ${_MIGFLAGS_} $> -server mach_server.c -user /dev/null \
				 -header mach_interface.h

mach_user.c: mach.defs
	${MIG3} ${_MIGFLAGS_} $> -user $@.X -server /dev/null -header /dev/null
	sed -e 's/vm_allocate/mig_vm_allocate/' \
	    -e 's/vm_map/mig_vm_map/' \
	    -e 's/vm_deallocate/mig_vm_deallocate/' \
	    -e 's/task_create/mig_task_create/' \
	    -e 's/task_terminate/mig_task_terminate/' \
	    -e 's/task_suspend/mig_task_suspend/' \
	    -e 's/task_set_special_port/mig_task_set_special_port/' \
	    < $@.X > $@
	${RM} ${_RMFLAGS_} $@.X

# This is a funny one.  We do the mig into a temporary directory and
# then pick out the one file that we actually want.  We use a
# temporary directory to avoid interfering with the files normally
# generated from mach.defs.  We use htg_vm_map.defs instead of
# using mach.defs directly so that the dependencies show up in
# htg_vm_map.defs.d instead of overwriting mach.defs.d.

htg_vm_map.c : htg_vm_map.defs
	${RM} -rf tmp_htg_vm_map
	mkdir tmp_htg_vm_map
	${MIG3} ${_MIGFLAGS_} -DEMULATOR -i tmp_htg_vm_map/ \
		-user htg_vm_map.c \
		-header tmp_htg_vm_map/mach_interface.h \
		-server /dev/null htg_vm_map.defs
	mv tmp_htg_vm_map/htg_vm_map.c htg_vm_map.c

htg_vm_map.defs : mach.defs
	ln -s $? $@

htg_vm_map.o : mach_interface.h

mach_port_server.c : mach_port.defs
	${MIG3} ${_MIGFLAGS_} $> -server mach_port_server.c -user /dev/null

mach_port_user.c: mach_port.defs
	${MIG3} ${_MIGFLAGS_} $> -user $@.X -server /dev/null
	sed -e 's/mach_port_allocate/mig_mach_port_allocate/' \
	    -e 's/mach_port_deallocate/mig_mach_port_deallocate/' \
	    -e 's/mach_port_insert_right/mig_mach_port_insert_right/' \
	    < $@.X > $@
	${RM} ${_RMFLAGS_} $@.X

exc_server.c exc_user.c: exc.defs
	${MIG3} ${_MIGFLAGS_} $> -server exc_server.c -user exc_user.c

device_server.c device_user.c: device.defs
	${MIG3} ${_MIGFLAGS_} $> -server device_server.c -user device_user.c

device_request_user.c: device_request.defs
	${MIG3} ${_MIGFLAGS_} $> -user $@ -server /dev/null

device_reply_server.c: device_reply.defs
	${MIG3} ${_MIGFLAGS_} $> -server $@ -user /dev/null

mach_host_server.c: mach_host.defs
	${MIG3} ${_MIGFLAGS_} $> -server $@ -user /dev/null

mach_host_user.c: mach_host.defs
	${MIG3} ${_MIGFLAGS_} $> -user $@.X -server /dev/null
	sed -e 's/thread_depress_abort/mig_thread_depress_abort/' \
	    < $@.X > $@
	${RM} ${_RMFLAGS_} $@.X

mach_debug_server.c mach_debug_user.c: mach_debug.defs
	${MIG3} ${_MIGFLAGS_} $> -server mach_debug_server.c \
				 -user   mach_debug_user.c

mach_norma_server.c mach_norma_user.c: mach_norma.defs
	${MIG3} ${_MIGFLAGS_} $> -server mach_norma_server.c \
				 -user   mach_norma_user.c

notify_server.c notify_user.c: notify.defs
	${MIG3} ${_MIGFLAGS_} $> -server notify_server.c -user notify_user.c
