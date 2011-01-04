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

VPATH			= ${TARGET_MACHINE}

DATAFILES		= asm.h eflags.h exec.h ndr_def.h kkt.h machlimits.h \
			  endian.h trap.h soundcard.h \
			  ${${TARGET_MACHINE}_DATAFILES}

#DATAFILES		= asm.h ast.h ast_types.h cpu_number.h db_machdep.h \
#			  eflags.h exec.h iopb.h ipl.h lock.h mach_param.h \
#			  machine_routines.h machparam.h mp_desc.h \
#			  ndr_def.h pic.h pio.h pit.h pmap.h proc_reg.h \
#			  sched_param.h seg.h setjmp.h stab.h thread.h \
#			  time_stamp.h timer.h trap.h tss.h vm_tuning.h \
#			  xpr.h \
#			  ${${TARGET_MACHINE}_DATAFILES}

INCLUDES		= ${DATAFILES}

ILIST			= ${INCLUDES}
IDIR			= ${MACH3_INCLUDE_IDIR}i386/
IMODE			= 644

.if exists(${TARGET_MACHINE}/machdep.mk)
.include "${TARGET_MACHINE}/machdep.mk"
.endif
.include <${RULES_MK}>
