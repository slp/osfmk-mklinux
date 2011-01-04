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

${TARGET_MACHINE}_VPATH		+= AT386

${TARGET_MACHINE}_CFLAGS	= -pic-none -DTHREAD_STATE=${MACHINE}_THREAD_STATE -DTHREAD_STATE_COUNT=${MACHINE}_THREAD_STATE_COUNT
${TARGET_MACHINE}_LDFLAGS	= 

#######################################################
#######################################################
###                                                 ###
### NOTE:  this default kernel configuration should ###
###        match src/mach_kernel/conf/Makefile      ###
###                                                 ###
#######################################################
#######################################################
MACH_KERNEL_CONFIG?=PRODUCTION

SFILES		= fstcw.o fixdfsi.o fabs.o setjmp.o

${TARGET_MACHINE}_OFILES	= fpu.o \
				  machine.o \
				  prof.o \
				  sym.o \
				  ../exc/trap.o \
				  ../hw/lock.o \
				  ../hw/locore.o \
				  ../vm/page.o \
				  ${SFILES} \
				  ${PROF_FILES}


# make sure the kernel exists before using 
.if exists(../../../../mach_kernel/${MACH_KERNEL_CONFIG}/mach_kernel.${MACH_KERNEL_CONFIG})
SYMBOL_FILE = ../../../../mach_kernel/${MACH_KERNEL_CONFIG}/mach_kernel.${MACH_KERNEL_CONFIG}
sym.c:	${SYMBOL_FILE}
	echo "#include <mach_perf.h>" > sym.c
	echo "struct symbol kernel_symbols[] = {" >> sym.c
	${ELF_CC_EXEC_PREFIX}/nm -x ${SYMBOL_FILE}| grep " T " | awk -F\| ' { print  "(unsigned)"$$2", \""$$1}' | sed -e 's/ *$$/\", 0,/' >> sym.c
	echo "(unsigned) 0, 0, 0};" >> sym.c
	echo "unsigned int kernel_symbols_count = (sizeof(kernel_symbols)/sizeof(struct symbol)) - 1;" >> sym.c
	echo "char *kernel_symbols_version = \"`strings -a ${SYMBOL_FILE} | grep mach_kernel/${MACH_KERNEL_CONFIG}`\\\\n\";" >>sym.c
.else
sym.c:	
	echo "#include <mach_perf.h>" > sym.c
	echo "struct symbol kernel_symbols[] = {(unsigned) 0, 0, 0};" >> sym.c
	echo "unsigned int kernel_symbols_count = 0;" >> sym.c
	echo char *kernel_symbols_version = \"\"\; >>sym.c
.endif

.SUFFIXES: .S .s

${SFILES:.o=.S}: $${@:.S=.s}
	${RM} ${_RMFLAGS_} ${.TARGET}
	${CP} ${.ALLSRC} ${.TARGET}


${SFILES}: $${@:.o=.S}
	${_CC_} -E ${_CCFLAGS_} -DASSEMBLER ${${.TARGET:.o=.S}:P} \
		> ${.TARGET:.o=.pp}
	${SED} '/^#/d' ${.TARGET:.o=.pp} > ${.TARGET:.o=.s}
	${_CC_} ${_CCFLAGS_} -c ${.TARGET:.o=.s}
	${RM} ${_RMFLAGS_} ${.TARGET:.o=.pp} ${.TARGET:.o=.s}
