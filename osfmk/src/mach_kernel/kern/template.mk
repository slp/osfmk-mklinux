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

MIGFLAGS	= -MD ${IDENT} -X
MIGKSFLAGS	= -DKERNEL_SERVER
MIGKUFLAGS	= -DKERNEL_USER

NORMA_TASK_FILES =				\
	norma_task_server.h			\
	norma_task_server.c 

NORMA_TASK_USER_FILES =				\
	norma_task.h				\
	norma_task_user.c

OTHERS		= ${NORMA_TASK_FILES} ${NORMA_TASK_USER_FILES}

INCFLAGS	= -I.. -I../..
MDINCFLAGS	= -I.. -I../..

DEPENDENCIES	=

.include <${RULES_MK}>

.ORDER: ${NORMA_TASK_FILES}

${NORMA_TASK_FILES}:  kern/norma_task.defs
	${_MIG_} ${_MIGFLAGS_} ${MIGKSFLAGS}		\
		-header /dev/null			\
		-user /dev/null				\
		-sheader norma_task_server.h		\
                -server norma_task_server.c		\
                ${kern/norma_task.defs:P}

.ORDER: ${NORMA_TASK_USER_FILES}

${NORMA_TASK_USER_FILES}:  kern/norma_task.defs
	${_MIG_} ${_MIGFLAGS_} ${MIGKUFLAGS}		\
		-header norma_task.h			\
		-user norma_task_user.c			\
		-server /dev/null			\
                ${kern/norma_task.defs:P}

.if exists(depend.mk)
.include "depend.mk"
.endif
