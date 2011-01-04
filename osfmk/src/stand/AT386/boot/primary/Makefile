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
# Copyright (c) 1992, 1991 Carnegie Mellon University
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

NO_STARTFILES		=
USE_STATIC_LIBRARIES 	=
OBJECT_FORMAT 		= ELF

PROGRAMS 		= bootprog rbootprog
OTHERS 			= boot rboot
LIBFLAGS		= -L../lib
LIBS			= -lboot
INCFLAGS		= -I../include

ILIST 			= boot rboot
IDIR 			= /stand/

LOADER_SCRIPT		= ../../../../../../src/stand/${TARGET_MACHINE}/boot/primary/elf_i386.boot

# bootprog_LDFLAGS 	= -nostdlib -Ttext 0 -e boot1
bootprog_LDFLAGS	= -nostdlib -T${LOADER_SCRIPT} -e boot1

# rbootprog_LDFLAGS 	= -nostdlib -Ttext 0 -e boot1
rbootprog_LDFLAGS	= -nostdlib -T${LOADER_SCRIPT} -e boot1

SETUP_PROGRAMS 		= rmelfhdr

# ugly workaround for common makefile problem

.if ((${.TARGETS} == "setup_all") || (${.TARGETS} == "setup_rmelfhdr"))
PROGRAMS		=${SETUP_PROGRAMS}
.else
DEPENDENCIES 		=
CFLAGS 			= ${${OBJECT_FORMAT}_EXTRA_WARNINGS} -m386 -DPRIMARY=1
rboot.S_CFLAGS 		= ${CFLAGS} -DREMOTE=1
.endif

rmelfhdr_OFILES 	= rmelfhdr.o

# start.o should be first, table.o should be second

OBJS 			= start.o table.o sys.o boot.o
ROBJS 			= start.o table.o sys.o rboot.o
bootprog_OFILES 	= ${OBJS}
rbootprog_OFILES 	= ${ROBJS}
SOBJS 			= start.o
COBJS 			= table.o boot.o rboot.o sys.o

MDINCFLAGS 		= -I.

.include <${RULES_MK}>

${PROGRAMS}: ../lib/libboot.a

${OTHERS}: $${.TARGET}prog
	${TARGET_EXEC_PREFIX}rmelfhdr ${.TARGET}prog ${.TARGET}
	@ls -l $@prog $@

${SOBJS:.o=.S}: $${.TARGET:.S=.s}
	${RM} ${_RMFLAGS_} ${.TARGET}
	${CP} ${${.TARGET:.S=.s}:P} ${.TARGET}

${COBJS:.o=.S}: $${.TARGET:.S=.c}
	${RM} ${_RMFLAGS_} ${.TARGET}
	${_CC_} ${_CCFLAGS_} -S ${${.TARGET:.S=.c}:P}
	sed '/^	\.align	2$$/d' ${.TARGET:.S=.s} > ${.TARGET}

${COBJS} ${SOBJS}: $${.TARGET:.o=.S}
	${_CC_} ${_CCFLAGS_} -c ${.TARGET:.o=.S}

rboot.c: boot.c
	${RM} ${_RMFLAGS_} ${.TARGET}
	${CP} ${boot.c:P} ${.TARGET}

.if exists(depend.mk)
.include "depend.mk"
.endif
