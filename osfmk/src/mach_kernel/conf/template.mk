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
# Copyright (c) 1991,1990,1989,1988,1987,1986 Carnegie Mellon University
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

PURE_MACH=1

#
#  This must be here before any rules are possibly defined by the
#  machine dependent makefile fragment so that a plain "make" command
#  always works.  The config program will emit an appropriate rule to
#  cause "all" to depend on every kernel configuration it generates.
#

VPATH=..:../..

SUBDIRS=${XKMACHKERNEL:Duk_xkern} device mach mach_debug kern xmm

OTHERS=${LOAD} ${DYNAMIC}

KCC=${${.TARGET}_DYNAMIC:D${DCC}:U${_CC_}}
DEPENDENCIES=
USE_STATIC_LIBRARIES=
.if !empty(MACHINE:M*alpha*)
NO_STRICT_ANSI=
.endif
CCTYPE=ansi
GLINE=
#
# microkernel should be compiled using ANSI rules (doens't work properly
# with traditional rules) but don't enforce strict checking for now

DYNAMIC_MODULE=_kmod
LOAD_IDIR=/
DYNAMIC_IDIR=/sbin/subsys/
ILIST=${LOAD} ${DYNAMIC:S;$;${DYNAMIC_MODULE};g}

.if ${TARGET_MACHINE} == "ALPHA"
NORMAL_GCC_WARNINGS=
SH?=/bin/ksh
.else
NORMAL_GCC_WARNINGS=-Wtrigraphs -Wcomment ${${.TARGET}_NO_STRICT_PROTOTYPES:D:U${NO_STRICT_PROTOTYPES:D:U-Wstrict-prototypes -Wmissing-prototypes -Wimplicit -Wreturn-type}} -Werror
SH?=/bin/sh
.endif
EXTRA_GCC_WARNINGS=

MD_PGFLAG?=-pg
PGFLAG=${PROFILING:D${MD_PGFLAG}:U}
NCFLAGS=${IDENT} -D_ANSI_C_SOURCE ${NORMAL_${CC_COMPILER}_WARNINGS} ${WARNINGS:D${EXTRA_GCC_WARNINGS}}
CFLAGS=${PGFLAG} ${NCFLAGS} ${MACHINE_CFLAGS}
DRIVER_CFLAGS=${CFLAGS}
PROFILING_CFLAGS=${NCFLAGS}

# The kernel doesn't need, and shouldn't have, a startfile (ie. crt0)
NO_STARTFILES=

INCFLAGS=-I.. -I../..
MDINCFLAGS=-I.. -I../.. -I${EXPORTBASE}/mach_kernel -I${EXPORTBASE}/include

MDFLAGS=-K${EXPORTBASE}/mach_kernel -K${EXPORTBASE}/include

LDOBJS=${LDOBJS_PREFIX} ${OBJS} ${LDOBJS_SUFFIX}

LDDEPS=${LDDEPS_PREFIX} ${SYSDEPS} conf/newvers.sh \
	conf/version.major conf/version.minor conf/version.variant \
	conf/version.edit conf/version.patch \
	conf/copyright.osf conf/copyright.cmu

LDLIBS=${${TARGET_MACHINE}_LDLIBS}

#
# Make internal symbols in the kernel servers visible for profiling
# and debugging. Don't enable paranoid type checking in the MiG stubs.
#
MIG_CFLAGS=-Dmig_internal= -DTypeCheck=0
bootstrap_server.o_CFLAGS+=${CFLAGS} ${MIG_CFLAGS}
clock_server.o_CFLAGS+=${CFLAGS} ${MIG_CFLAGS}
dev_pager.o_CFLAGS+=${CFLAGS} ${MIG_CFLAGS}
device_server.o_CFLAGS+=${CFLAGS} ${MIG_CFLAGS}
exc_server.o_CFLAGS+=${CFLAGS} ${MIG_CFLAGS}
mach_debug_server.o_CFLAGS+=${CFLAGS} ${MIG_CFLAGS}
mach_host_server.o_CFLAGS+=${CFLAGS} ${MIG_CFLAGS}
mach_port_server.o_CFLAGS+=${CFLAGS} ${MIG_CFLAGS}
mach_server.o_CFLAGS+=${CFLAGS} ${MIG_CFLAGS}
mach_user_internal.o_CFLAGS+=${CFLAGS} ${MIG_CFLAGS}
ledger_server.o_CFLAGS+=${CFLAGS} ${MIG_CFLAGS}
monitor_server.o_CFLAGS+=${CFLAGS} ${MIG_CFLAGS}

#
#  These macros are filled in by the config program depending on the
#  current configuration.  The MACHDEP macro is replaced by the
#  contents of the machine dependent makefile template and the others
#  are replaced by the corresponding symbol definitions for the
#  configuration.
#
%CFLAGS

%OBJS

%CFILES

%SFILES

%BFILES

%ORDERED

%LOAD

%DYNAMIC

%MACHDEP

.include <${RULES_MK}>

.if	defined(DYNAMIC) && !defined(TOSTAGE)

${DYNAMIC}: $${.TARGET:S/$$/${DYNAMIC_MODULE}/}

${DYNAMIC:S/$/${DYNAMIC_MODULE}/g}: $${$${.TARGET}_OBJECTS} ${VMUNIX_LIB}
	${DLD} ${DLD_FLAGS} -e ${${.TARGET}_ENTRY} \
		-o ${.TARGET} ${${.TARGET}_OBJECTS} ${VMUNIX_LIB}
.endif

INCDIRS:=-I${EXPORTBASE}/mach_kernel ${INCDIRS}

_CC_GENINC_=-I.
_CCDEFS_=${TARGET_MACHINE_CFLAGS}
_host_INCDIRS_=${INCDIRS}

.PRECIOUS: Makefile

${LOAD}: ${PRELDDEPS} ${LDOBJS} ${LDDEPS}
	${SH} ${conf/newvers.sh:P} \
		`cat ${conf/version.major:P} \
		     ${conf/version.minor:P} \
		     ${conf/version.variant:P} \
		     ${conf/version.edit:P} \
		     ${conf/version.patch:P}` \
		${conf/copyright.osf:P} \
		${conf/copyright.cmu:P}
	${KCC} -c ${_CCFLAGS_} vers.c
	rm -f ${.TARGET}
	@echo [ loading ${.TARGET} ]
	${_LD_} ${_LD_LDFLAGS_} ${LDOBJS} vers.o ${LDLIBS}
	chmod 755 a.out
	-mv a.out ${.TARGET}
.if defined(SYMBOLIC_DEBUG_INFO)
	rm -f ${.TARGET}.debug
	@echo [ loading ${.TARGET}.debug ]
	${_LD_} ${_LD_LDFLAGS_} ${LDOBJS} vers.o ${LDLIBS}
	chmod 755 a.out
	-mv a.out ${.TARGET}.debug
.endif

.if defined(SYS_RULE_1)
	${SYS_RULE_1}
.endif
.if defined(SYS_RULE_2)
	${SYS_RULE_2}
.endif

relink: ${LOAD:=.relink}

${LOAD:=.relink}: ${LDDEPS}
	${SH} ${conf/newvers.sh:P} \
		`cat ${conf/version.major:P} \
		     ${conf/version.minor:P} \
		     ${conf/version.variant:P} \
		     ${conf/version.edit:P} \
		     ${conf/version.patch:P}` \
		${conf/copyright.osf:P} \
		${conf/copyright.cmu:P}
	${KCC} -c ${_CCFLAGS_} vers.c
	rm -f ${.TARGET:.relink=}
	@echo [ loading ${.TARGET:.relink=} ]
	${_LD_} ${SHLDSTRIP} ${_LD_LDFLAGS_} ${LDOBJS} vers.o ${LDLIBS}
	chmod 755 a.out
	-mv a.out ${.TARGET:.relink=}
.if defined(SYMBOLIC_DEBUG_INFO)
	rm -f ${.TARGET:.relink=.debug}
	@echo [ loading ${.TARGET:.relink=.debug} ]
	${_LD_} ${_LD_LDFLAGS_} ${LDOBJS} vers.o ${LDLIBS}
	chmod 755 a.out
	-mv a.out ${.TARGET:.relink=.debug}
.endif

#
# Extra SYS rules for automatically running makeboot
#
.if defined(SYS_RULE_1)
	${SYS_RULE_1}
.endif
.if defined(SYS_RULE_2)
	${SYS_RULE_2}
.endif

.ORDER: ${LOAD}

${OBJS}: ${OBJSDEPS}

.if !defined(MACHINEDEP_RULES)
${COBJS}: $${$${.TARGET}_SOURCE}
	${KCC} -c ${_CCFLAGS_} ${${${.TARGET}_SOURCE}:P}
.endif

${DECODE_OFILES}: $${$${.TARGET}_SOURCE}
	${RM} ${_RMFLAGS_} ${.TARGET}
	${UUDECODE} ${${${.TARGET}_SOURCE}:P}

.if exists(depend.mk) && !defined(TOSTAGE)
.include "depend.mk"
.endif
