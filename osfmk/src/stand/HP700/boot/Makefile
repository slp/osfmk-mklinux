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

USE_STATIC_LIBRARIES 	=
OBJECT_FORMAT 		= SOM

RELOC=	500000

CFLAGS += -mdisable-fpregs -DHPOSFCOMPAT -D_BSD
SFLAGS	= -DLOCORE -DASSEMBLER ${CFLAGS} -DRELOC=0x500000

PROGRAMS 		= boot

LDFLAGS 		= -nostdlib -N -H -R500000 -e begin

INCFLAGS		= -I../include 


ILIST 			= boot 
IDIR 			= /stand/

# ugly workaround for common makefile problem

# start.o should be first, table.o should be second

OBJS 			= srt0.o cache_s.o cache_c.o pdc.o iodc.o bcopy.o printf.o prf.o exit.o close.o boot.o conf.o \
			  disklabel.o bzero.o dev.o alloc.o stat.o open.o ufs.o devopen.o gets.o \
			  loadunix.o read.o lseek.o

OFILES 			= ${OBJS}
SFILES 			= srt0.o cache_s.o
COBJS 			= 

boot: ${OBJS}
	${_LD_} ${SHLDSTRIP} ${_LDFLAGS_} ${LDOBJS} ${_LIBS_} ${OBJS} -o boot

${SFILES:.o=.S}: $${.TARGET:.S=.s}
	${RM} ${_RMFLAGS_} ${.TARGET} ${.TARGET:.S=.pp}
	${CP} ${${.TARGET:.S=.s}:P} ${.TARGET}
	${_traditional_CC_} -E ${_CCFLAGS_} ${SFLAGS}  ${.TARGET} \
		| ${APP} | ${M4} | ${SED} '/^#/d' > ${.TARGET:.S=.pp}
	${RM} ${_RMFLAGS_} ${.TARGET}
	${MV} ${.TARGET:.S=.pp} ${.TARGET}

${SFILES}: $${.TARGET:.o=.S}
	${AS} ${.TARGET:.o=.S} -o ${.TARGET}

.include <${RULES_MK}>
