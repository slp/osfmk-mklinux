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

VPATH		= ..:../..

MIGFLAGS	= -MD ${IDENT}
MIGKSFLAGS	= -DKERNEL_SERVER
MIGKUFLAGS	= -DKERNEL_USER -maxonstack 1024

MACH_FILES =					\
	mach_server.h				\
	mach_server.c

MACH_PORT_FILES =				\
	mach_port_server.h			\
	mach_port_server.c

EXC_FILES = 					\
	exc_user.h				\
	exc_user.c

MEMORY_OBJECT_FILES = 				\
	memory_object_user.h			\
	memory_object_user.c

MEMORY_OBJECT_DEFAULT_FILES = 			\
	memory_object_default.h			\
	memory_object_default_user.c

PROF_FILES =					\
	prof_user.c				\
	prof.h

MACH_HOST_FILES =				\
	mach_host_server.h			\
	mach_host_server.c

CLOCK_FILES =					\
	clock_server.h				\
	clock_server.c

CLOCK_REPLY_FILES =				\
	clock_reply.h				\
	clock_reply_user.c

BOOTSTRAP_FILES = 				\
	bootstrap_server.h			\
	bootstrap_server.c

LEDGER_FILES = 					\
	ledger_user.c 				\
	ledger_server.h 			\
	ledger_server.c

SYNC_FILES =					\
	sync_server.h				\
	sync_server.c

MACH_NORMA_FILES =				\
	mach_norma_server.h			\
	mach_norma_server.c		

MACH_NORMA_USER_FILES =				\
	mach_norma.h				\
	mach_norma_user.c

MACH_USER_FILES =				\
	mach_user.h				\
	mach_user.c

OTHERS		= ${MACH_FILES} ${MACH_PORT_FILES} \
		  ${EXC_FILES} \
		  ${MEMORY_OBJECT_FILES} ${MEMORY_OBJECT_DEFAULT_FILES} \
		  ${PROF_FILES} ${MACH_HOST_FILES} ${LEDGER_FILES} \
		  ${CLOCK_FILES} ${CLOCK_REPLY_FILES} ${BOOTSTRAP_FILES} \
		  ${BOOTSTRAP_FILES} ${SYNC_FILES} \
		  ${MACH_NORMA_FILES} ${MACH_NORMA_USER_FILES} \
		  ${MACH_USER_FILES} ${T_M_FILES}

INCFLAGS	= -I.. -I../..
MDINCFLAGS	= -I.. -I../..

DEPENDENCIES	=

.if exists(../../mach/${TARGET_MACHINE}/machdep.mk)
.include "../../mach/${TARGET_MACHINE}/machdep.mk"
.endif
.include <${RULES_MK}>

.ORDER: ${MACH_FILES}

${MACH_FILES}: mach/mach.defs
	${_MIG_} ${_MIGFLAGS_} ${MIGKSFLAGS}		\
		-header /dev/null			\
		-user /dev/null				\
		-sheader mach_server.h			\
		-server mach_server.c			\
		${mach/mach.defs:P}

.ORDER: ${MACH_PORT_FILES}

${MACH_PORT_FILES}: mach/mach_port.defs
	${_MIG_} ${_MIGFLAGS_} ${MIGKSFLAGS}		\
		-header /dev/null			\
		-user /dev/null				\
		-sheader mach_port_server.h		\
		-server mach_port_server.c		\
		${mach/mach_port.defs:P}

.ORDER: ${EXC_FILES}

${EXC_FILES}: mach/exc.defs
	${_MIG_} ${_MIGFLAGS_} ${MIGKUFLAGS}		\
		-header exc_user.h			\
		-user exc_user.c			\
		-server /dev/null			\
		${mach/exc.defs:P}

.ORDER: ${MEMORY_OBJECT_FILES}

${MEMORY_OBJECT_FILES}: mach/memory_object.defs
	${_MIG_} ${_MIGFLAGS_} ${MIGKUFLAGS} -DSEQNOS	\
		-header memory_object_user.h		\
		-user memory_object_user.c		\
		-server /dev/null			\
		${mach/memory_object.defs:P}

.ORDER: ${MEMORY_OBJECT_DEFAULT_FILES}

${MEMORY_OBJECT_DEFAULT_FILES}: mach/memory_object_default.defs
	${_MIG_} ${_MIGFLAGS_} ${MIGKUFLAGS} -DSEQNOS	\
		-header memory_object_default.h		\
		-user memory_object_default_user.c	\
		-server /dev/null			\
		${mach/memory_object_default.defs:P}

.ORDER: ${PROF_FILES}

${PROF_FILES}: mach/prof.defs
	${_MIG_} ${_MIGFLAGS_} ${MIGKUFLAGS}		\
		-header prof.h				\
		-iheader prof_internal.h		\
		-user prof_user.c			\
		-server /dev/null			\
		${mach/prof.defs:P}

.ORDER: ${MACH_HOST_FILES}

${MACH_HOST_FILES}: mach/mach_host.defs
	${_MIG_} ${_MIGFLAGS_} ${MIGKSFLAGS}		\
		-header /dev/null			\
		-user /dev/null				\
		-sheader mach_host_server.h		\
		-server mach_host_server.c		\
		${mach/mach_host.defs:P}

.ORDER: ${CLOCK_FILES}

${CLOCK_FILES}: mach/clock.defs
	${_MIG_} ${_MIGFLAGS_} ${MIGKSFLAGS}		\
		-header /dev/null			\
		-user /dev/null				\
		-sheader clock_server.h			\
		-server clock_server.c			\
		${mach/clock.defs:P}

.ORDER: ${CLOCK_REPLY_FILES}

${CLOCK_REPLY_FILES}: mach/clock_reply.defs
	${_MIG_} ${_MIGFLAGS_} ${MIGKUFLAGS}		\
		-header clock_reply.h			\
		-user clock_reply_user.c		\
		-server /dev/null			\
		${mach/clock_reply.defs:P}

.ORDER: ${BOOTSTRAP_FILES}

${BOOTSTRAP_FILES}: mach/bootstrap.defs
	${_MIG_} ${_MIGFLAGS_} ${MIGKSFLAGS}		\
		-header /dev/null			\
		-user /dev/null				\
		-sheader bootstrap_server.h		\
		-server bootstrap_server.c		\
                ${mach/bootstrap.defs:P}

.ORDER: ${LEDGER_FILES}

${LEDGER_FILES}: mach/ledger.defs ${MACH_TYPES_DEFS}
	-${MAKE_MACH}
	${_MIG_} ${_MIGFLAGS_} ${MIGKSFLAGS} ${MIGKUFLAGS}	\
		-header /dev/null				\
		-user ledger_user.c				\
		-sheader ledger_server.h			\
		-server ledger_server.c				\
		${mach/ledger.defs:P}

.ORDER: ${SYNC_FILES}

${SYNC_FILES}: mach/sync.defs
	${_MIG_} ${_MIGFLAGS_} ${MIGKSFLAGS}		\
		-header /dev/null			\
		-user /dev/null				\
		-sheader sync_server.h			\
		-server sync_server.c			\
		${mach/sync.defs:P}

.ORDER: ${MACH_NORMA_FILES}

$(MACH_NORMA_FILES): mach/mach_norma.defs
	${_MIG_} ${_MIGFLAGS_} ${MIGKSFLAGS}		\
		-header /dev/null			\
		-user /dev/null				\
		-sheader mach_norma_server.h		\
		-server mach_norma_server.c		\
                ${mach/mach_norma.defs:P}

.ORDER: ${MACH_NORMA_USER_FILES}

${MACH_NORMA_USER_FILES}:  mach/mach_norma.defs
	${_MIG_} ${_MIGFLAGS_} ${MIGKUFLAGS}		\
		-header mach_norma.h			\
		-user mach_norma_user.c			\
		-server /dev/null			\
		${mach/mach_norma.defs:P}

.ORDER: ${NORMA_INTERNAL_FILES}

${MACH_USER_FILES}:  mach/mach.defs
	${_MIG_} -X ${_MIGFLAGS_} ${MIGKUFLAGS}		\
		-header mach_user.h			\
		-user mach_user.c			\
		-server /dev/null			\
		${mach/mach.defs:P}

.if exists(depend.mk)
.include "depend.mk"
.endif
