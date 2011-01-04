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

VPATH	= ${MAKETOP}/mach_kernel/hp_pa:${MAKETOP}/mach_kernel/hp_pa/${TARGET_MACHINE}

DATAFILES		= arch_types.h		\
			  asm.h			\
			  asm_em.h		\
			  asp.h			\
			  ast.h			\
			  break.h		\
			  clock.h		\
			  cpu.h			\
			  cpu_data.h		\
			  db_machdep.h		\
			  debug.h		\
			  endian.h		\
			  iodc.h		\
			  iomod.h		\
			  iplhdr.h		\
			  kkt.h			\
			  kkt_map.h		\
			  lock.h		\
			  locore.h		\
			  mach_param.h		\
			  machine_routines.h	\
			  machine_rpc.h		\
			  machparam.h		\
			  opcode_is_something.h	\
			  pdc.h			\
			  pim.h			\
			  pmap.h		\
			  psw.h			\
			  regs.h		\
			  rpb.h			\
			  rtclock_entries.h	\
			  setjmp.h		\
			  spl.h			\
			  syscall_subr.h	\
			  task.h		\
			  thread.h		\
			  thread_act.h		\
			  trap.h		\
			  viper.h		\
			  vm_tuning.h		\
			  xpr.h			\
			  ${${TARGET_MACHINE}_DATAFILES}


INCLUDES		= ${DATAFILES}

EXPDIR			= /mach_kernel/hp_pa/

.if	exists(${TARGET_MACHINE}/machdep.mk)
.include "${TARGET_MACHINE}/machdep.mk"
.endif
.include <${RULES_MK}>
