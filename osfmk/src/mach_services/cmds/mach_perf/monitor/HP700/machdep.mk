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
#
# OLD HISTORY
# Revision 1.1.17.1  1996/03/27  13:23:17  yp
# 	Removed profiling files (now in lib).
# 	[96/03/18            yp]
# 
# Revision 1.1.7.3  1996/02/06  14:23:33  bruel
# 	disabled broken kernel_symbols[] generation it works.
# 	[96/02/06            bruel]
# 
# Revision 1.1.7.2  1996/01/25  11:32:44  bernadat
# 	New compiler: Fixed rules for sym.c
# 	[96/01/02            bernadat]
# 
# 	Homogenized copyright headers
# 	[95/11/30            bernadat]
# 
# Revision 1.1.7.1  1995/04/07  19:13:14  barbou
# 	Merged into mainline.
# 	Added machine-dependent CFLAGS and LDFLAGS.
# 	Use PRODUCTION as default kernel configuration.
# 	[95/03/27            barbou]
# 
# Revision 1.1.5.1  1994/09/07  13:07:45  bruel
# 	Adapt  sym.c file generation for SOM.
# 	Use STD+WS as default.
# 	[94/08/19            bernadat]
# 	Use libc fpu support, since fpu.s seems broken.
# 	[94/08/19            bernadat]
# 	[94/09/01            bruel]
# 
# Revision 1.1.3.1  1994/04/28  17:53:16  bruel
# 	Adapted from i386.
# 	[94/04/28            bruel]
# 

${TARGET_MACHINE}_VPATH		+= HP700

#SFILES				= fpu.o
${TARGET_MACHINE}_OFILES	= ${SFILES} \
				  machine.o \
				  ../exc/trap.o \
				  ../hw/lock.o \
				  ../hw/locore.o \
				  ../vm/page.o 

.SUFFIXES: .S .s

${SFILES:.o=.S}: $${.TARGET:.S=.s}
	${RM} ${_RMFLAGS_} ${.TARGET} ${.TARGET:.S=.pp}
	${CP} ${${.TARGET:.S=.s}:P} ${.TARGET}
	${_CC_} -E ${_CCFLAGS_} -DASSEMBLER ${.TARGET} \
		> ${.TARGET:.S=.pp}
	${RM} ${_RMFLAGS_} ${.TARGET}
	${MV} ${.TARGET:.S=.pp} ${.TARGET}

${SFILES}: $${.TARGET:.o=.S}
	${AS} ${.TARGET:.o=.S} -o ${.TARGET}
