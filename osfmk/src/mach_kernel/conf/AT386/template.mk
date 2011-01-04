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
#
# OSF_IK_HISTORY
# Revision 1.14.6.5  1992/11/02  21:53:37  gm
# 	Added scsidisk.o_NO_STRICT_ANSI support.
# 	[1992/11/02  21:29:56  gm]
#
# Revision 1.14.6.4  1992/10/22  16:29:53  rod
# 	Move start.s to model dependent section.
# 	[1992/10/14  15:19:10  rod]
# 
# 	Move locore.s to model dependent section.
# 	[1992/10/14  12:53:19  rod]
# 
# Revision 1.14.6.3  1992/09/22  19:41:35  meissner
# 	7780: Set -fno-builtin if using GCC 2.00.
# 	[1992/09/22  19:40:59  meissner]
# 
# Revision 1.14.6.2  1992/08/27  14:06:17  garyf
# 	allow additional options to be passed to ld
# 	[1992/08/27  14:03:22  garyf]
# 
# Revision 1.14.4.2  1992/06/11  15:55:51  jeffc
# 	Don't recompile/relink dynamic subsystems unnecessarily.
# 	[1992/06/10  20:50:18  jeffc]
# 
# Revision 1.14.2.7  1992/04/08  20:50:59  jeffc
# 	Use ${SHLDSTRIP} on kmods
# 	[1992/04/06  22:22:33  jeffc]
# 
# Revision 1.14.2.6  1992/03/23  15:50:23  dwm
# 	Bug 1307,  name space confusion, give locore/assembler __s
# 	[1992/03/23  01:40:46  dwm]
# 
# Revision 1.14.2.5  1992/03/21  02:39:02  jeffc
# 	Fix error in previous submission: _host_INCDIRS must be defined
# 	after the include of the common makerules file.
# 	[1992/03/21  02:26:35  jeffc]
# 
# Revision 1.14.2.4  1992/03/20  22:07:13  jeffc
# 	Moved genassym/locore to MD makefile.
# 	[1992/03/20  18:51:29  jeffc]

# Revision 1.14.2.3  1991/12/07  21:59:31  jeffc
# 	Strip local symbols from executable
# 	[1991/12/06  15:30:14  jeffc]
# 
# Revision 1.14.2.2  1991/10/25  09:24:39  duthie
# 	Add dynamic cflags.  Don't use pic-lib specific cflags for
# 	dynamic subsystems.  This removes the pic-none from being used
# 	in the compiles.
# 	[91/10/25  09:15:47  duthie]
# 
# Revision 1.14  91/10/08  15:51:12  devrcs
# 	If object format is MACHO (aka Rose), make sure that -export_default
# 	kernel: gets passed to linker appropriately. Somehow we missed this
# 	when we did the cutover...
# 	[91/09/16  15:25:29  boot]
# 
# Revision 1.13  91/08/15  19:14:50  devrcs
# 	Add export_default option to linker options to put symbols in kernel package.
# 	Remove hack to build fake rose vmunix if we are already building a ROSE kernel.
# 	[91/08/07  11:59:26  duthie]
# 
# Revision 1.12  91/07/08  17:12:11  devrcs
# 	Make CPP usage for assembly files always be -traditional.
# 	Make CCTYPE=ansi the default
# 	[91/06/28  14:05:59  jeffc]
# 
# Revision 1.11  91/06/10  16:15:24  devrcs
# 	Change -export option to -export_default for the linker.  New syntax.
# 	[91/05/29  14:11:40  duthie]
# 
# 	Define OBJECT_FORMAT before it's usage.
# 	[91/05/22  14:45:20  kathyg]
# 
# 	Move branch up tree
# 	[91/05/22  19:59:54  devrcs]
# 
# Revision 1.10  91/05/22  15:29:31  devrcs
# 	Remove OBJECT_FORMAT setting for Rose format support
# 	[91/05/20  16:40:22  watkins]
# 
# 	Removed SYSDEPS
# 	[91/05/16  12:49:28  kathyg]
# 
# 	More Reno make changes.
# 	[91/04/23  13:55:07  gm]
# 
# Revision 1.8  91/03/04  16:59:50  devrcs
# 	Moved definition of LD and NM to ${MACHINE}/template.mk.
# 	Dynamic subsys integration.
# 	[90/12/15  10:57:55  kwallace]
# 
# Revision 1.7  90/10/31  13:50:53  devrcs
# 	Remove default for SWAPSYS.
# 	[90/10/09  16:53:07  brezak]
# 
# Revision 1.6  90/10/07  13:22:02  devrcs
# 	Necessary additions for decoding floating point emulator objects.
# 	[90/10/03  15:51:14  jd]
# 
# 	Added EndLog Marker.
# 	[90/09/28  09:05:53  gm]
# 
# Revision 1.5  90/09/23  15:44:24  devrcs
# 	Added A_OUT_GCC_EXEC_PREFIX to KCC
# 	[90/09/17  15:38:27  jd]
# 
# Revision 1.4  90/08/09  13:16:19  devrcs
# 	Merged '386 code
# 	[90/08/06  18:49:44  kevins]
# 
# 	General cleanup and put -D${TARGET_MACHINE} in S_RULE_1.
# 	[90/07/31  10:43:27  jd]
# 
# Revision 1.3  90/07/27  08:45:50  devrcs
# 	Updated for new configuration.
# 	[90/07/16  21:04:04  gm]
# 
# 	Changes for i386 builds.
# 	[90/07/13  16:05:30  gm]
# 
# 	Changed default S rules.
# 	[90/07/11  13:25:06  kevins]
# 
# Revision 1.2  90/04/27  18:56:33  devrcs
# 	General cleanup.
# 	[90/04/17  14:04:38  jd]
# 
# 	Created from Makefile.i386.
# 	[90/04/20  18:15:48  gm]
# 
# Revision 1.3  90/02/27  21:10:35  devrcs
# 	Added OSF Copyright marker.
# 	[90/02/27  19:50:21  gm]
# 
# Revision 1.2  90/02/23  00:22:21  devrcs
# 	Latest version for osc.5
# 	[90/02/20  11:07:50  kevins]
# 
# 	Indicate coff vs a.out
# 	[89/10/21            rvb]
# 
# Revision 2.6  89/09/25  12:20:10  rvb
# 	uprt.s -> start.s
# 	[89/09/23            rvb]
# 
# Revision 2.5  89/04/07  14:58:25  rvb
# 	We don't need INCLUDES any more.
# 	[89/04/07            rvb]
# 
# Revision 2.4  89/04/05  12:57:12  rvb
# 	Some changes for gcc, and locore is a .s
# 	[89/03/21            rvb]
# 
# Revision 2.3  89/02/25  17:39:18  gm0w
# 	Changes for cleanup.
# 
# Revision 2.2  89/01/23  22:15:23  af
# 	Created.
# 	[89/01/16  17:11:51  af]

###############################################################################
#BEGIN	Machine dependent Makefile fragment for the i386
###############################################################################

VOLATILE=

FORCE_VOLATILE=-fvolatile

LDOBJS_PREFIX= ${ORDERED} locore.o

GLINE=

MD_PGFLAG=-pg -mno-mcount

# define volatile to be __volatile__ for gcc with -traditional
#
.if ${CCTYPE} == "traditional"
TARGET_MACHINE_CFLAGS=\
	-Dvolatile\=__volatile__ -Dsigned\=__signed__
.endif

LOCORE_DEPS=	assym.s i386/start.s			\
		i386/locore.s i386/cswitch.s

locore.S: ${LOCORE_DEPS}
	cat ${.ALLSRC} >locore.S

locore.o_SOURCE=locore.S

# genassym.o actually is an assembly file, we call it this
# to help with the dependancy rules
genassym.o: ${MACHINE}/genassym.c
	${_CC_} -MD ${PROFILING_CFLAGS} -S -o ${.TARGET} -c ${_GENINC_} ${INCDIRS} ${IDENT} ${${MACHINE}/genassym.c:P}

assym.s: genassym.o
	sed -e '/#DEFINITION#/!d' -e 's/^.*#DEFINITION#//' -e 's/\$$//' genassym.o > ${.TARGET}

${SOBJS}: assym.s

locore.o: assym.s

exec.o: mach/mach_interface.h
exec.o: mach/mach_user_internal.h

CC_OPT_LEVEL=-O2

.if defined(KERNEL_OBJECT_FORMAT)
OBJECT_FORMAT=${KERNEL_OBJECT_FORMAT}
.else
OBJECT_FORMAT?=ELF
.endif

# For GCC 2.+ add the -fno-builtin switch, so that exit is not considered
# a builtin function.  Also, -fomit-frame-pointer is automatically set
# when doing optimizations
.if exists(${${OBJECT_FORMAT}_GCC_EXEC_PREFIX}/specs)
TARGET_MACHINE_CFLAGS+= -fno-builtin
.endif

.if defined(SYMBOLIC_DEBUG_INFO)
TARGET_MACHINE_CFLAGS+= -g
.endif

.if ${OBJECT_FORMAT} == "MACHO"
LDFLAGS = -export_default kernel: -T ${TEXTORG} -e _pstart
.elif ${OBJECT_FORMAT} == "A_OUT"
LDFLAGS = -T ${TEXTORG} -e _pstart
.elif ${OBJECT_FORMAT} == "ELF"
LDFLAGS = -Ttext ${TEXTORG} -e pstart
.endif
LDFLAGS+=${LDOPTS}

ENCODED=fpe.o

${ENCODED}: i386/fp/${OBJECT_FORMAT}/$${.TARGET:.o=.b}
	${UUDECODE} ${i386/fp/${OBJECT_FORMAT}/${.TARGET:.o=.b}:P}

MACHINEDEP_RULES=

MACH_I386_FILES = mach/mach_i386_server.c
mach_i386_server.o_CFLAGS+=${CFLAGS} ${MIG_CFLAGS}

.ORDER:  ${MACH_I386_FILES}

${MACH_I386_FILES}: mach/i386/mach_i386.defs ${MACH_TYPES_DEFS}
	-${MAKE_MACH}
	${_MIG_} ${DEPENDS} ${_MIGKSFLAGS_}			\
		-header /dev/null				\
		-user /dev/null					\
		-sheader mach/mach_i386_server.h		\
		-server mach/mach_i386_server.c			\
		${mach/i386/mach_i386.defs:P}

CFLAGS+=${_CC_PICLIB_}
PROFILING_CFLAGS+=${_CC_PICLIB_}

${COBJS}: $${$${.TARGET}_SOURCE}
	${KCC} -c ${_CCFLAGS_NOPIC_} ${${${.TARGET}_SOURCE}:P}

${SOBJS:.o=.S}: $${$${.TARGET:.S=.o}_SOURCE}
	${RM} ${_RMFLAGS_} ${.TARGET}
	${CP} ${${${.TARGET:.S=.o}_SOURCE}:P} ${.TARGET}

${SOBJS} locore.o: $${.TARGET:.o=.S}
	${_traditional_CC_} -E ${_CCFLAGS_} -DASSEMBLER ${${.TARGET:.o=.S}:P} \
		> ${.TARGET:.o=.pp}
	${SED} -e '/^#/d' \
	       -e '/^\.stabs.*Ltext0$$/s/^\.stabs /&"${.TARGET:.o=.s}"/' \
		${.TARGET:.o=.pp} > ${.TARGET:.o=.s}
	${KCC} -c ${.TARGET:.o=.s}
	${RM} ${_RMFLAGS_} ${.TARGET:.o=.pp} ${.TARGET:.o=.s}

${SOBJS}: assym.s

#
# Temporarily disable strict ANSI checking on the SCSI subsystem.
#
at386_scsi.o_NO_STRICT_ANSI=
autoconf.o_NO_STRICT_ANSI=
hbaconf.o_NO_STRICT_ANSI=
bios_label.o_NO_STRICT_ANSI=
conf.o_NO_STRICT_ANSI=
adsl.o_NO_STRICT_ANSI=
scsi_hba_hdw.o_NO_STRICT_ANSI=
scsi_aha15_hdw.o_NO_STRICT_ANSI=
scsi_aha17_hdw.o_NO_STRICT_ANSI=
scsi_53C810_hdw.o_NO_STRICT_ANSI=
scsi_53C825_hdw.o_NO_STRICT_ANSI=
rz.o_NO_STRICT_ANSI=
rz_audio.o_NO_STRICT_ANSI=
rz_cpu.o_NO_STRICT_ANSI=
rz_disk.o_NO_STRICT_ANSI=
rz_disk_bbr.o_NO_STRICT_ANSI=
rz_host.o_NO_STRICT_ANSI=
rz_labels.o_NO_STRICT_ANSI=
rz_tape.o_NO_STRICT_ANSI=
scsi.o_NO_STRICT_ANSI=
scsi_alldevs.o_NO_STRICT_ANSI=
scsi_cpu.o_NO_STRICT_ANSI=
scsi_disk.o_NO_STRICT_ANSI=
scsi_printer.o_NO_STRICT_ANSI=
scsi_rom.o_NO_STRICT_ANSI=
scsi_tape.o_NO_STRICT_ANSI=
scsi_worm.o_NO_STRICT_ANSI=
scsit.o_NO_STRICT_ANSI=
kkt.o_NO_STRICT_ANSI=
scsi_info.o_NO_STRICT_ANSI=

xk_lproxyU.o_NO_STRICT_PROTOTYPES=

#
# Avoid complications associated with `asm' statements in C code
# by disallowing an optimization that can cause corruption of
# stack pointers
#
i386_rpc.o_CFLAGS+=${CFLAGS} -fno-defer-pop

###############################################################################
#END	Machine dependent Makefile fragment for the i386
###############################################################################
