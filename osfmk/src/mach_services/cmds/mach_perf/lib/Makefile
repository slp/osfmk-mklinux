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


VPATH			= ${TARGET_MACHINE}

LIBRARIES		= libperf.a
USE_STATIC_LIBRARIES	= YES
DEPENDENCIES		= YES

INCDIRS			:= ${INC3DIRS} ${INCDIRS} 
INCFLAGS                = -I../include \
			  -I${EXPORTBASE}/mach_services/cmds/mach_perf

DEFINES			= ${${TARGET_MACHINE}_DEFINES}
CFLAGS			= ${${TARGET_MACHINE}_CFLAGS} ${DEFINES}
MIGFLAGS		= ${DEFINES} ${INCFLAGS}
LDFLAGS			= ${${TARGET_MACHINE}_LDFLAGS}

MIG_DEFS		= printf.defs \
			  prof_ctrl.defs \
			  exception.defs

PURE_MACH_OFILES	= qsort.o \
			  floor.o \
			  ux_stubs.o \
			  ${${TARGET_MACHINE}_PURE_MACH_OFILES}

UNIX_OFILES		= ux_cons.o \
			  ux_prof.o \
			  ${${TARGET_MACHINE}_UNIX_OFILES}
LINUX_OFILES		=

.if	!IMPURE_MACH
SYSTEM_OFILES		= ${PURE_MACH_OFILES}
test.o_CFLAGS		+= -DPURE_MACH
.else
SYSTEM_OFILES		= ${UNIX_OFILES}
.endif

OFILES			= cons.o \
			  debug.o \
                          doprnt.o \
			  exc_serv.o \
                          exception.o \
			  fork.o \
                          inherit_serv.o \
                          inherit_user.o \
                          mach.o \
			  mach_prof.o \
			  misc.o \
                          printf_serv.o \
                          printf_user.o \
                          printf.o \
                          prof.o \
                          prof_serv.o \
                          prof_user.o \
			  random.o \
                          resources.o \
			  sched.o \
                          server.o \
                          server_serv.o \
                          server_user.o \
                          stat.o \
                          test.o \
                          thread.o \
                          time.o \
			  sym.o \
			  ${SYSTEM_OFILES} \
                          ${${TARGET_MACHINE}_OFILES}

exception.o:	exception.c exception.h

ux_prof.o:	prof_ctrl.h

printf_serv.c printf_user.c printf.h: printf.defs
	${_MIG_} ${_MIGFLAGS_} ${.ALLSRC} -server printf_serv.c \
	-user printf_user.c -header printf.h

inherit_serv.c inherit_user.c: ipc_inherit.defs
	${_MIG_} ${_MIGFLAGS_} ${.ALLSRC} -server inherit_serv.c \
	-user inherit_user.c -header /dev/null

prof_serv.c prof_user.c prof_ctrl.h: prof_ctrl.defs
	${_MIG_} ${_MIGFLAGS_} ${.ALLSRC} -server prof_serv.c \
	-user prof_user.c -header prof_ctrl.h

server_serv.c server_user.c: server.defs
	${_MIG_} ${_MIGFLAGS_} ${.ALLSRC} -server server_serv.c \
	-user server_user.c -header /dev/null

exc_serv.c exception.h: exception.defs
	${_MIG_} ${_MIGFLAGS_} ${.ALLSRC} -server exc_serv.c \
	-user /dev/null -header exception.h

.if exists(${TARGET_MACHINE}/machdep.mk)
.include "${TARGET_MACHINE}/machdep.mk"
.endif

.include <${RULES_MK}>

.if exists(depend.mk)
.include "depend.mk"
.endif





