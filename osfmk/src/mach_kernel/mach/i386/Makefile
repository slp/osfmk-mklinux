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

MIG_DEFS		= mach_i386.defs

DATAFILES		= boolean.h exception.h fp_reg.h flipc_dep.h \
			  kern_return.h mach_i386_types.h syscall_sw.h \
			  thread_status.h thread_state.h vm_param.h \
			  vm_types.h rpc.h \
			  machine_types.defs ${MIG_DEFS}

INCLUDES		= ${DATAFILES} ${_MIG_HDRS_}
SETUP_INCLUDES		= boolean.h kern_return.h vm_types.h

ILIST			= ${INCLUDES}
IDIR			= ${MACH3_INCLUDE_IDIR}mach/i386/
IMODE			= 644

INCFLAGS		= -I../..

.include <${RULES_MK}>

${_MIG_HDRS_}: machine_types.defs

.ORDER:	export_machine_types.defs ${_MIG_HDRS_}
