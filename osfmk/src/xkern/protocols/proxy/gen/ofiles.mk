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
#
# Makefile
#
# x-kernel v3.2
#
# Copyright (c) 1993,1991,1990  Arizona Board of Regents
#
# Revision: 1.8
# Date: 1993/11/11 21:43:45
#

MIG_OFILES  		= xk_lproxyS.o xk_lproxyU.o xk_uproxyS.o xk_uproxyU.o 

MIG_CFILES  		= xk_lproxyS.c xk_lproxyU.c xk_uproxyS.c xk_uproxyU.c 
MIG_HFILES  		= xk_lproxy.h xk_uproxy.h

LPROXY_MATERIALS	= xk_lproxy.defs lproxy.awk   	\
			  lproxyServer.sed  lproxy.demux.sed
UPROXY_MATERIALS	= xk_uproxy.defs uproxy.awk   	\
			  uproxy.rest.sed uproxy.call.sed uproxy.push.sed


#
# Both of the mig-generated User files require some postprocessing by
# sed scripts
#

.ORDER : xk_lproxyS.c xk_lproxyU.c xk_lproxy.h

xk_lproxyS.c xk_lproxyU.c xk_lproxy.h : ${LPROXY_MATERIALS}
	${_MIG_} ${_MIGFLAGS_}  		\
		-sheader xk_lproxy_server.h	\
		-server xk_lproxyS.c.tmp	\
		-user   xk_lproxyU.c.tmp  	\
		${xk_lproxy.defs:P}
	awk -f ${lproxy.awk:P} xk_lproxyU.c.tmp
	mv                           lproxy.rest.tmp     xk_lproxyU.c
	sed -f ${lproxy.demux.sed:P} lproxy.demux.tmp >> xk_lproxyU.c
	sed -f ${lproxyServer.sed:P} xk_lproxyS.c.tmp >  xk_lproxyS.c
	rm	lproxy.demux.tmp


.ORDER : xk_uproxyS.c xk_uproxyU.c xk_uproxy.h

xk_uproxyS.c xk_uproxyU.c xk_uproxy.h : ${UPROXY_MATERIALS}
	${_MIG_} ${_MIGFLAGS_}  		\
		-sheader xk_uproxy_server.h	\
		-server xk_uproxyS.c    	\
		-user   xk_uproxyU.c.tmp	\
		${xk_uproxy.defs:P}
	awk -f ${uproxy.awk:P} xk_uproxyU.c.tmp
	sed -f ${uproxy.rest.sed:P} uproxy.rest.tmp >  xk_uproxyU.c
	sed -f ${uproxy.call.sed:P} uproxy.call.tmp >> xk_uproxyU.c
	sed -f ${uproxy.push.sed:P} uproxy.push.tmp >> xk_uproxyU.c
	rm	uproxy.call.tmp uproxy.push.tmp uproxy.rest.tmp

