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
# MkLinux

VPATH 			= ../clock:../device:../exc:../host:../hw:../ipc:../pager:../port:../sched:../task:../thread:../vm:${TARGET_MACHINE}

PROGRAMS		= mach_perf

${PROGRAMS}: ../lib/libperf.a

ILIST			= mach_perf
IDIR			= /mach_servers/
CFLAGS			+= -DSTANDALONE ${${TARGET_MACHINE}_CFLAGS}
USE_STATIC_LIBRARIES	=
DEPENDENCIES		= YES

INCFLAGS                =  -I../include -I../clock -I../device -I../exc -I../host -I../hw -I../ipc -I../pager -I../port -I../sched -I../task -I../thread -I../vm ${INCDIRS} -I${EXPORTBASE}/mach_perf
LIBDIRS			+= -L../lib
LIBS			=  -lperf -lsa_mach -lmach -lsa_mach -lm ${${TARGET_MACHINE}_LIBS}
LDFLAGS			+= ${${TARGET_MACHINE}_LDFLAGS}

MIG_DEFS                =       bg.defs notify.defs

OFILES			= sa.o \
                          clock_perf.o \
                          cons.o \
			  device_perf.o \
		          exc_perf.o \
			  ../exc/exc_server_main.o \
		          host_perf.o \
		          hw_perf.o \
			  ipc_perf.o \
			  ../ipc/ipc_server_main.o \
			  ../ipc/ipc_server.o \
			  ../ipc/ipc_user.o \
		          misc.o \
			  norma.o \
		          notify.o \
			  pager_perf.o \
			  ../pager/pager_server_main.o \
			  ../pager/pager_server.o \
			  ../pager/pager_user.o \
		          port_perf.o \
		          ../port/port_notify.o \
			  random.o \
			  sched_perf.o \
		          task_perf.o \
		          thread_perf.o \
			  vm_perf.o \
                          bg_server.o \
                          bg_user.o \
                          cli.o \
                          cli_init.o \
                          synthetic.o \
			  ${${TARGET_MACHINE}_OFILES}

notify.c: notify.defs
	${_MIG_} ${_MIGFLAGS_} ${.ALLSRC} -server notify.c \
	-user /dev/null -header /dev/null

bg_server.c bg_user.c: bg.defs
	${_MIG_} ${_MIGFLAGS_} ${.ALLSRC} -server bg_server.c \
	-user bg_user.c -header /dev/null

# cli_init.h is constructed from a user file specifying synthetic proc
# definitions to be included as default. We cheat here, the order
# of cli_init.awk and cli_init.conf is important, the first on the
# line becomes the argument to the -f parameter of awk.

cli_init.c : cli_init.awk cli_init.conf
	awk -f $> > $@

.if exists(${TARGET_MACHINE}/machdep.mk)
.include "${TARGET_MACHINE}/machdep.mk"
.endif

.include <${RULES_MK}>

.if exists(depend.mk)
.include "depend.mk"
.endif
