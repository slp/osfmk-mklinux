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

DATAFILES	= boolean.h exception.h kern_return.h rpc.h \
		  thread_state.h thread_status.h \
		  vm_param.h vm_types.h machine_types.defs \
		  syscall_sw.h

INCLUDES		= ${DATAFILES}
SETUP_INCLUDES		= boolean.h kern_return.h vm_types.h

ILIST			= ${INCLUDES}
IDIR			= ${MACH3_INCLUDE_IDIR}mach/${MACHINE}/
IMODE			= 644

INCFLAGS		= -I../..

.include <${RULES_MK}>
