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
#
#  x-kernel build rules
#
VPATH			= ..:../..

MIGFLAGS		= -MD ${IDENT}
MIGKSFLAGS		= -DKERNEL_SERVER
MIGKUFLAGS		= -DKERNEL_USER

GRAPH_COMP_FILES	= protTbl.c protocols.c protocols.h traceLevels.c

PROTTBL_FILES		= ptblData.c

ROM_FILES		= initRom.c

OTHERS			= ${GRAPH_COMP_FILES} ${PROTTBL_FILES} ${ROM_FILES}

PROXY_BASE      	= ../xkern/protocols/proxy
LPROXY_FILES		= xk_lproxyU.c xk_lproxy.h
LPROXY_DEFS		:= ${PROXY_BASE}/xk_lproxy.defs
LPROXY_AWK		:= ${PROXY_BASE}/gen/lproxy.awk
LPROXY_DEMUX_SED	:= ${PROXY_BASE}/gen/lproxy.demux.sed
LPROXY_MATERIALS	:= ${LPROXY_AWK} ${LPROXY_DEMUX_SED}

UPROXY_FILES		= xk_uproxyS.c xk_uproxy.h
UPROXY_DEFS		= ${PROXY_BASE}/xk_uproxy.defs

PROXY_FILES		= xk_uproxyS.c xk_uproxy.h xk_lproxyU.c xk_lproxy.h

INCFLAGS		= -I.. -I../.. -I../../..

.if defined(XK_PROXY)
OTHERS			+= ${PROXY_FILES}
.endif


DEPENDENCIES =


.ORDER: ${GRAPH_COMP_FILES} ${ROM_FILES}

${GRAPH_COMP_FILES} ${ROM_FILES}: uk_xkern/gen/${TARGET_MACHINE}/graph.machine uk_xkern/gen/graph.comp uk_xkern/gen/genrom.awk uk_xkern/gen/rom
	cat ${uk_xkern/gen/${TARGET_MACHINE}/graph.machine:P} ${uk_xkern/gen/graph.comp:P} | ${_COMPOSE_} -f
	cat compose_rom ${uk_xkern/gen/rom:P} | awk -f ${uk_xkern/gen/genrom.awk:P} >initRom.c

.ORDER: ${PROTTBL_FILES}

${PROTTBL_FILES}: uk_xkern/gen/prottbl
	${_PTBLDUMP_} ${uk_xkern/gen/prottbl:P} > ptblData.c


.ORDER: ${LPROXY_FILES}

${LPROXY_FILES}: ${LPROXY_DEFS} ${LPROXY_MATERIALS}
	${_MIG_} ${_MIGFLAGS_} ${MIGKUFLAGS}	\
		-server /dev/null		\
		-user   xk_lproxyU.c.tmp  	\
		${${LPROXY_DEFS}:P}
	awk -f ${${LPROXY_AWK}:P} xk_lproxyU.c.tmp
	mv                         lproxy.rest.tmp     xk_lproxyU.c
	rm -f	lproxy.tmp
	sed '/^#/d' ${${LPROXY_DEMUX_SED}:P} >lproxy.tmp
	sed -f lproxy.tmp lproxy.demux.tmp >> xk_lproxyU.c
	rm	lproxy.demux.tmp xk_lproxyU.c.tmp lproxy.tmp


.ORDER: ${UPROXY_FILES}

${UPROXY_FILES}: ${UPROXY_DEFS}
	${_MIG_} ${_MIGFLAGS_} ${MIGKSFLAGS}	\
		-server xk_uproxyS.c    	\
		-user   /dev/null		\
		-sheader xk_uproxy_server.h	\
		${${UPROXY_DEFS}:P}


.if exists(depend.mk)
.include "depend.mk"
.endif

.include <${RULES_MK}>
