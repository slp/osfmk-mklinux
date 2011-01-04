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
# Mach Operating System
# Copyright (c) 1994 Carnegie Mellon University
# All Rights Reserved.
# 
# Permission to use, copy, modify and distribute this software and its
# documentation is hereby granted, provided that both the copyright
# notice and this permission notice appear in all copies of the
# software, derivative works or modified versions, and any portions
# thereof, and that both notices appear in supporting documentation.
# 
# CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS" 
# CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
# ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
# 
# Carnegie Mellon requests users of this software to return to
# 
#  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
#  School of Computer Science
#  Carnegie Mellon University
#  Pittsburgh PA 15213-3890
# 
# any improvements or extensions that they make and grant Carnegie Mellon
# the rights to redistribute these changes.
#
#
# MkLinux
#

VPATH 			= ..:net:protocols

INCFLAGS		= -I../include -I../

NO_STARTFILES		=
USE_STATIC_LIBRARIES 	=
OBJECT_FORMAT 		= ELF

PROGRAMS 		= versiondate.h mach_boot mach_boot.lilo

ILIST 			= mach_boot mach_boot.lilo
IDIR 			= /stand/

LDFLAGS			= -nostdlib -Ttext 0 -e start
LIBFLAGS		= -L../lib
LIBS			= -lboot -lsa_fs -lsa_mach

CFLAGS +=  -DDEVICE_NS8390 -DDEVICE_3C503 -m386
# The following flags are used for debugging precise pieces of software:
#	-DNETDEBUG
#	-DPROTODEBUG
#	-DTFTPDEBUG
#	-DBOOTPDEBUG
#	-DUDPIPDEBUG
#	-DETHERDEBUG
#	-DNS8390DEBUG
#	-DSMCDEBUG
#	-DARPDEBUG
#	-DDEBUG
#	-DEXCTRAP	/* future release, with control over IDT */

print_env:	
	@echo PATH=  ${PATH}
	@echo VPATH= ${VPATH}

boot.S_CFLAGS 		= ${CFLAGS} -DREMOTE

# start.o should be first
NOBJS = net_debug.o 3c503.o ns8390.o smc.o endian.o dlink.o \
	i386at.o more_asm.o \
	arp.o bootp.o tftp.o udpip.o icmp.o \
	net.o
OBJS  = start.o table.o boot.o mach_boot.o malloc.o \
	${NOBJS}
LOBJS = lilo.o table.o boot.o mach_boot.o malloc.o lilo_env.o\
	${NOBJS}

mach_boot_OFILES 	= ${OBJS}
mach_boot.lilo_OFILES	= ${LOBJS}
SOBJS = start.o lilo.o more_asm.o
COBJS = boot.o mach_boot.o malloc.o lilo_env.o\
	net_debug.o 3c503.o ns8390.o smc.o endian.o dlink.o \
	i386at.o \
	arp.o bootp.o tftp.o udpip.o icmp.o \
	net.o 
COBJS_ALIGN8 = table.o

NO_STRICT_ANSI =

.include <${RULES_MK}>

${PROGRAMS}: ../lib/libboot.a

${OTHERS}: $${.TARGET}.x
	${TARGET_EXEC_PREFIX}rmelfhdr ${.TARGET}.x ${.TARGET}
	@ls -l $@.x $@

${SOBJS:.o=.S}: $${.TARGET:.S=.s}
	${RM} ${_RMFLAGS_} ${.TARGET}
	${CP} ${${.TARGET:.S=.s}:P} ${.TARGET}

${COBJS:.o=.S}: $${.TARGET:.S=.c}
	${RM} ${_RMFLAGS_} ${.TARGET}
	${_CC_} ${_CCFLAGS_} -S ${${.TARGET:.S=.c}:P} 
	sed -e "s/\\.data/.text/"  ${.TARGET:.S=.s} > ${.TARGET}

${COBJS_ALIGN8:.o=.S}: $${.TARGET:.S=.c}
	${RM} ${_RMFLAGS_} ${.TARGET}
	${_CC_} ${_CCFLAGS_} -S ${${.TARGET:.S=.c}:P} 
	sed -e "s/\\.data/.text/" -e "s/\\.align.*2/.align 8/" \
	  ${.TARGET:.S=.s} > ${.TARGET}

${OBJS} ${ROBJS} ${LOBJS}: $${.TARGET:.o=.S}
	${_CC_} ${_CCFLAGS_} -traditional -E ${.TARGET:.o=.S} > ${.TARGET:.o=.i}
	sed '/^\#/d' ${.TARGET:.o=.i} > X${.TARGET:.o=.s}
	${_CC_} -c X${.TARGET:.o=.s} -o ${.TARGET}

versiondate.h: GETDATE

GETDATE:
	@echo "#define VERSIONDATE	"\"`date +"%D %T"`\" > versiondate.h

.if exists(depend.mk)
.include "depend.mk"
.endif
