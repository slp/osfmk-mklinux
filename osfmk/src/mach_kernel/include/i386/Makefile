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

VPATH	= ../../i386:../../i386/${TARGET_MACHINE}

EXPINC_SUBDIRS	= mp

DATAFILES		= arch_types.h ast.h ast_types.h flipc_page.h \
			  gdb_defs.h kgdb_defs.h kgdb_setjmp.h cpu_data.h \
			  cpu_number.h db_machdep.h lock.h mach_param.h \
			  machine_routines.h machine_rpc.h machparam.h ntoh.h \
			  pmap.h rtclock_entries.h setjmp.h endian.h \
			  syscall_subr.h task.h thread.h trap.h \
			  vm_tuning.h xpr.h spl.h kkt.h thread_act.h kkt_map.h\
			  ${${TARGET_MACHINE}_DATAFILES}

INCLUDES		= ${DATAFILES}

EXPDIR			= /mach_kernel/i386/

.if	exists(${TARGET_MACHINE}/machdep.mk)
.include "${TARGET_MACHINE}/machdep.mk"
.endif
.include <${RULES_MK}>
