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

#VPATH			= ${MAKETOP}mach_svcs/include/servers

LIBRARIES		= libxmm.a

USE_STATIC_LIBRARIES	=

EXPLIB_TARGETS		= export_libxmm.a

ILIST			= libxmm.a
IDIR			= ${MACH3_LIBRARY_IDIR}
IMODE			= 644

MIG_DEFS		= proxy.defs xmm_net.defs memory_object.defs

OFILES			= proxyServer.o \
			  xmm_netServer.o xmm_netUser.o \
			  memory_objectServer.o memory_objectUser.o \
			  proxy.o atrium.o hash.o kalloc.o \
			  mig_loop.o net_bsdtcp.o net_bsdudp.o \
			  vm_dirty.o xmm.o xmm_buffer.o xmm_copy.o \
			  xmm_debug.o xmm_flush.o xmm_interpose.o \
			  xmm_invalid.o xmm_luser.o xmm_maycache.o \
			  xmm_multiplex.o xmm_net.o xmm_pagingfile.o \
			  xmm_split.o xmm_svm.o xmm_vm.o xmm_write.o \
			  xmm_zero.o zalloc.o
GARBAGE			= proxy.h		proxyServer.c \
						proxyUser.c \
			  xmm_net.h		xmm_netServer.c \
						xmm_netUser.c \
			  memory_object.h	memory_objectServer.c \
						memory_objectUser.c

.include <${RULES_MK}>

.ORDER: proxyServer.c proxyUser.c proxy.h

proxyServer.c proxyUser.c proxy.h: proxy.defs
	${_MIG_} ${_MIGFLAGS_} ${proxy.defs:P}  -server proxyServer.c \
					 -user proxyUser.c \
					 -header proxy.h

.ORDER: xmm_netServer.c xmm_netUser.c xmm_net.h

xmm_netServer.c xmm_netUser.c xmm_net.h: xmm_net.defs
	${_MIG_} ${_MIGFLAGS_} ${xmm_net.defs:P}  -server xmm_netServer.c \
					 -user xmm_netUser.c \
					 -header xmm_net.h

.ORDER: memory_objectServer.c memory_objectUser.c memory_object.h

memory_objectServer.c memory_objectUser.c memory_object.h: memory_object.defs
	${_MIG_} ${_MIGFLAGS_} ${memory_object.defs:P} \
					 -server memory_objectServer.c \
					 -user memory_objectUser.c \
					 -header memory_object.h
