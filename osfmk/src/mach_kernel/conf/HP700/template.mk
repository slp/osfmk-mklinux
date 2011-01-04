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

.if defined(KGDB)
SUBDIRS+=kgdb
.endif

.if defined(KERNEL_OBJECT_FORMAT)
OBJECT_FORMAT=${KERNEL_OBJECT_FORMAT}
.endif

.if ${OBJECT_FORMAT} == "SOM"
genassym.o_CCTYPE=host
genassym_CCTYPE=host
.endif

genassym.o: ${MACHINE}/genassym.c
	${_CC_} -c ${_GENINC_} ${INCDIRS} ${IDENT} ${${MACHINE}/genassym.c:P}

genassym_CCTYPE=host
genassym: genassym.o
	${_CC_} genassym.o -o ${.TARGET}

assym.s: genassym
	./genassym > ${.TARGET}

${SOBJS}: assym.s

TARGET_MACHINE_CFLAGS+= -mdisable-fpregs

.if ${OBJECT_FORMAT} == "SOM"
LDFLAGS=-N -H -R11000 -e start
.elif ${OBJECT_FORMAT} == "ELF"
LDFLAGS=-Ttext 11000 -e start
.endif

${SOBJS:.o=.S}: $${$${.TARGET:.S=.o}_SOURCE}
	${RM} ${_RMFLAGS_} ${.TARGET}
	${CP} ${${${.TARGET:.S=.o}_SOURCE}:P} ${.TARGET}

${SOBJS}: $${.TARGET:.o=.S}
	${_traditional_CC_} -E ${_CCFLAGS_} -DASSEMBLER ${${.TARGET:.o=.S}:P} |\
		${SED} -e '/^#/d' > ${.TARGET:.o=.s}
	${AS} ${.TARGET:.o=.s} -o ${.TARGET}
	${RM} ${_RMFLAGS_} ${.TARGET:.o=.pp} ${.TARGET:.o=.s}
# We remove "#" lines because gas does not deal with them correctly
# and will give incorrect line numbers in diagnostics.

#
# Temporarily disable strict ANSI checking for the following files
#
# hp_pa/
decode_exc.o_NO_STRICT_PROTOTYPES=
denormal.o_NO_STRICT_PROTOTYPES=
denormal.o_NO_STRICT_ANSI=
fpudispatch.o_NO_STRICT_PROTOTYPES=
timex_fix.o_NO_STRICT_PROTOTYPES=
# spmath/
dfadd.o_NO_STRICT_PROTOTYPES=
dfcmp.o_NO_STRICT_PROTOTYPES=
dfdiv.o_NO_STRICT_PROTOTYPES=
dfmpy.o_NO_STRICT_PROTOTYPES=
dfrem.o_NO_STRICT_PROTOTYPES=
dfsqrt.o_NO_STRICT_PROTOTYPES=
dfsub.o_NO_STRICT_PROTOTYPES=
divsfm.o_NO_STRICT_PROTOTYPES=
divsfr.o_NO_STRICT_PROTOTYPES=
divsim.o_NO_STRICT_PROTOTYPES=
divsir.o_NO_STRICT_PROTOTYPES=
divu.o_NO_STRICT_PROTOTYPES=
divufr.o_NO_STRICT_PROTOTYPES=
divuir.o_NO_STRICT_PROTOTYPES=
fcnvff.o_NO_STRICT_PROTOTYPES=
fcnvfx.o_NO_STRICT_PROTOTYPES=
fcnvfxt.o_NO_STRICT_PROTOTYPES=
fcnvxf.o_NO_STRICT_PROTOTYPES=
frnd.o_NO_STRICT_PROTOTYPES=
impys.o_NO_STRICT_PROTOTYPES=
impyu.o_NO_STRICT_PROTOTYPES=
mdrr.o_NO_STRICT_PROTOTYPES=
mpyaccs.o_NO_STRICT_PROTOTYPES=
mpyaccu.o_NO_STRICT_PROTOTYPES=
mpys.o_NO_STRICT_PROTOTYPES=
mpyscv.o_NO_STRICT_PROTOTYPES=
mpyu.o_NO_STRICT_PROTOTYPES=
mpyucv.o_NO_STRICT_PROTOTYPES=
setovfl.o_NO_STRICT_PROTOTYPES=
sfadd.o_NO_STRICT_PROTOTYPES=
sfcmp.o_NO_STRICT_PROTOTYPES=
sfdiv.o_NO_STRICT_PROTOTYPES=
sfmpy.o_NO_STRICT_PROTOTYPES=
sfrem.o_NO_STRICT_PROTOTYPES=
sfsqrt.o_NO_STRICT_PROTOTYPES=
sfsub.o_NO_STRICT_PROTOTYPES=

hpux_label.o_NO_STRICT_ANSI=
hpux_label.o_CFLAGS+=-fno-builtin 
rz.o_NO_STRICT_ANSI=
rz.o_CFLAGS+=-fno-builtin 
rz_audio.o_NO_STRICT_ANSI=
rz_audio.o_CFLAGS+=-fno-builtin 
rz_cpu.o_NO_STRICT_ANSI=
rz_cpu.o_CFLAGS+=-fno-builtin 
rz_disk.o_NO_STRICT_ANSI=
rz_disk.o_CFLAGS+=-fno-builtin 
rz_disk_bbr.o_NO_STRICT_ANSI=
rz_disk_bbr.o_CFLAGS+=-fno-builtin 
rz_host.o_NO_STRICT_ANSI=
rz_host.o_CFLAGS+=-fno-builtin 
rz_labels.o_NO_STRICT_ANSI=
rz_labels.o_CFLAGS+=-fno-builtin 
rz_tape.o_NO_STRICT_ANSI=
rz_tape.o_CFLAGS+=-fno-builtin 
scsi.o_NO_STRICT_ANSI=
scsi.o_CFLAGS+=-fno-builtin 
scsi_alldevs.o_NO_STRICT_ANSI=
scsi_alldevs.o_CFLAGS+=-fno-builtin 
scsi_cpu.o_NO_STRICT_ANSI=
scsi_cpu.o_CFLAGS+=-fno-builtin 
scsi_disk.o_NO_STRICT_ANSI=
scsi_disk.o_CFLAGS+=-fno-builtin 
scsi_printer.o_NO_STRICT_ANSI=
scsi_printer.o_CFLAGS+=-fno-builtin 
scsi_rom.o_NO_STRICT_ANSI=
scsi_rom.o_CFLAGS+=-fno-builtin 
scsi_tape.o_NO_STRICT_ANSI=
scsi_tape.o_CFLAGS+=-fno-builtin 
scsi_worm.o_NO_STRICT_ANSI=
scsi_worm.o_CFLAGS+=-fno-builtin 
scsit.o_NO_STRICT_ANSI=
scsit.o_CFLAGS+=-fno-builtin 
scsi_info.o_NO_STRICT_ANSI=
scsi_info.o_NO_STRICT_PROTOTYPES=
scsi_53C700_hdw.o_NO_STRICT_ANSI=
scsi_53C700_hdw.o_CFLAGS+=-fno-builtin 
bios_label.o_NO_STRICT_ANSI=
bios_label.o_CFLAGS+=-fno-builtin 
conf.o_NO_STRICT_ANSI=
conf.o_CFLAGS+=-DHP700 -DMACH -DMACH_KERNEL -DMACH_LOAD -DTIMEX -DTIMEX_BUG -DPC596XMT_BUG -DUSELEDS -DSTACK_GROWTH_UP -fno-builtin 
