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

XMM_INTERNAL_FILES =				\
	xmm_internal_server.h			\
	xmm_internal_server.c 

XMM_INTERNAL_USER_FILES =			\
	xmm_internal.h				\
	xmm_internal_user.c

PROXY_FILES = 					\
	proxy_server.h				\
	proxy_server.c

PROXY_USER_FILES = 				\
	proxy_user.h				\
	proxy_user.c

OTHERS		= ${XMM_INTERNAL_FILES} ${XMM_INTERNAL_USER_FILES} \
		  ${PROXY_FILES} ${PROXY_USER_FILES} ${T_M_FILES}

INCFLAGS	= -I.. -I../..
MDINCFLAGS	= -I.. -I../..

DEPENDENCIES	=

.if exists(../../xmm/${TARGET_MACHINE}/machdep.mk)
.include "../../xmm/${TARGET_MACHINE}/machdep.mk"
.endif
.include <${RULES_MK}>

.ORDER: ${XMM_INTERNAL_FILES}

${XMM_INTERNAL_FILES}:  xmm/xmm_internal.defs
	${_MIG_} ${_MIGFLAGS_} ${MIGKSFLAGS}		\
		-header /dev/null			\
		-user /dev/null				\
		-sheader xmm_internal_server.h	\
                -server xmm_internal_server.c		\
                ${xmm/xmm_internal.defs:P}

.ORDER: ${XMM_INTERNAL_USER_FILES}

${XMM_INTERNAL_USER_FILES}:  xmm/xmm_internal.defs
	${_MIG_} ${_MIGFLAGS_} ${MIGKUFLAGS}		\
		-header xmm_internal.h		\
		-user xmm_internal_user.c		\
		-server /dev/null			\
                ${xmm/xmm_internal.defs:P}

.ORDER: ${PROXY_FILES}

${PROXY_FILES}: xmm/xmm_proxy.defs
	${_MIG_} ${_MIGFLAGS_} ${MIGKSFLAGS}		\
		-header /dev/null			\
		-user /dev/null				\
		-sheader proxy_server.h			\
		-server proxy_server.c			\
                ${xmm/xmm_proxy.defs:P}

.ORDER: ${PROXY_USER_FILES}

${PROXY_USER_FILES}: xmm/xmm_proxy.defs
	${_MIG_} ${_MIGFLAGS_} ${MIGKUFLAGS}		\
		-header proxy_user.h			\
		-user proxy_user.c			\
		-server /dev/null			\
		${xmm/xmm_proxy.defs:P}

.if exists(depend.mk)
.include "depend.mk"
.endif
