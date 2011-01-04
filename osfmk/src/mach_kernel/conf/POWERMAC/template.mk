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

#
# Temporarily disable strict ANSI checking on the SCSI subsystem.
#
bios_label.o_NO_STRICT_ANSI=
bios_label.o_NO_STRICT_PROTOTYPES=
scsi_aha15_hdw.o_NO_STRICT_ANSI=
scsi_aha15_hdw.o_NO_STRICT_PROTOTYPES=
scsi_aha17_hdw.o_NO_STRICT_ANSI=
scsi_aha17_hdw.o_NO_STRICT_PROTOTYPES=
scsi_53C825_hdw.o_NO_STRICT_ANSI=
scsi_53C825_hdw.o_NO_STRICT_PROTOTYPES=
scsi_53C94_hdw.o_NO_STRICT_ANSI=
scsi_53C94_hdw.o_NO_STRICT_PROTOTYPES=
mesh.o_NO_STRICT_ANSI=
mesh.o_NO_STRICT_PROTOTYPES=
macsd.o_NO_STRICT_ANSI=
macsd.o_NO_STRICT_PROTOTYPES=
rz.o_NO_STRICT_ANSI=
rz.o_NO_STRICT_PROTOTYPES=
rz_audio.o_NO_STRICT_ANSI=
rz_audio.o_NO_STRICT_PROTOTYPES=
rz_cpu.o_NO_STRICT_ANSI=
rz_cpu.o_NO_STRICT_PROTOTYPES=
rz_disk.o_NO_STRICT_ANSI=
rz_disk.o_NO_STRICT_PROTOTYPES=
rz_disk_bbr.o_NO_STRICT_ANSI=
rz_disk_bbr.o_NO_STRICT_PROTOTYPES=
rz_host.o_NO_STRICT_ANSI=
rz_host.o_NO_STRICT_PROTOTYPES=
rz_labels.o_NO_STRICT_ANSI=
rz_labels.o_NO_STRICT_PROTOTYPES=
rz_tape.o_NO_STRICT_ANSI=
rz_tape.o_NO_STRICT_PROTOTYPES=
scsi.o_NO_STRICT_ANSI=
scsi.o_NO_STRICT_PROTOTYPES=
scsi_alldevs.o_NO_STRICT_ANSI=
scsi_alldevs.o_NO_STRICT_PROTOTYPES=
scsi_cpu.o_NO_STRICT_ANSI=
scsi_cpu.o_NO_STRICT_PROTOTYPES=
scsi_disk.o_NO_STRICT_ANSI=
scsi_disk.o_NO_STRICT_PROTOTYPES=
scsi_printer.o_NO_STRICT_ANSI=
scsi_printer.o_NO_STRICT_PROTOTYPES=
scsi_rom.o_NO_STRICT_ANSI=
scsi_rom.o_NO_STRICT_PROTOTYPES=
scsi_tape.o_NO_STRICT_ANSI=
scsi_tape.o_NO_STRICT_PROTOTYPES=
scsi_worm.o_NO_STRICT_ANSI=
scsi_worm.o_NO_STRICT_PROTOTYPES=
scsit.o_NO_STRICT_ANSI=
scsit.o_NO_STRICT_PROTOTYPES=
scsi_info.o_NO_STRICT_ANSI=
scsi_info.o_NO_STRICT_PROTOTYPES=
kkt.o_NO_STRICT_ANSI=
kkt.o_NO_STRICT_PROTOTYPES=
conf.o_NO_STRICT_ANSI=
conf.o_NO_STRICT_PROTOTYPES=
autoconf.o_NO_STRICT_ANSI=
autoconf.o_NO_STRICT_PROTOTYPES=
mac_label.o_NO_STRICT_ANSI=
mac_label.o_NO_STRICT_PROTOTYPES=
powermac_scsi.o_NO_STRICT_ANSI=
powermac_scsi.o_NO_STRICT_PROTOTYPES=

# the scsi dma stuff uses non-ansi header files
amic_dma.o_NO_STRICT_ANSI=
amic_dma.o_NO_STRICT_PROTOTYPES=

# the ethernet driver is non-ansi
if_mace.o_NO_STRICT_ANSI=
if_mace.o_NO_STRICT_PROTOTYPES=

# rtclock.c uses long long for the processor clock
rtclock.o_NO_STRICT_ANSI=

# ata.c IDE driver uses long long to calculate mb's
wd.o_NO_STRICT_ANSI=
wcd.o_NO_STRICT_ANSI=
atapi.o_NO_STRICT_ANSI=

# tulip drive uses non-ansi headers
if_tulip.o_NO_STRICT_ANSI=

# genassym.o actually is an assembly file,
# we name it genassym.o to help with the automatic
# dependency generation
genassym.o: ${MACHINE}/genassym.c
	${_CC_} -MD ${PROFILING_CFLAGS} -S -o ${.TARGET} -c ${_GENINC_} ${INCDIRS} ${IDENT} ${${MACHINE}/genassym.c:P}

assym.s: genassym.o
	sed -e '/#DEFINITION#/!d' -e 's/^.*#DEFINITION#//' -e 's/\$$//' genassym.o > ${.TARGET}

${SOBJS}: assym.s
lowmem_vectors.o: assym.s

.if defined(KERNEL_OBJECT_FORMAT)
OBJECT_FORMAT=${KERNEL_OBJECT_FORMAT}
.else
OBJECT_FORMAT?=ELF
.endif

# lowmem_vectors.s must be at head of link line.

LDOBJS_PREFIX=${ORDERED} lowmem_vectors.o
LDLIBS=`${_CC_} --print-libgcc-file-name`

# -msoft-float needed to avoid the compiler using floats to optimise
# moves in the kernel
TARGET_MACHINE_CFLAGS+= -msoft-float -fsigned-bitfields -Dvolatile=__volatile -mcpu=604

# avoid gcc-2.7.2 known bug
go.o_CC_OPT_LEVEL = ${CC_OPT_LEVEL} -fno-strength-reduce

#LDFLAGS = -Ttext 800000 -Tdata 200000
LDFLAGS = -Ttext 200000 -Tdata 400000

lowmem_vectors.o_SOURCE=ppc/lowmem_vectors.s
${SOBJS:.o=.S} lowmem_vectors.S : $${$${.TARGET:.S=.o}_SOURCE}
	${RM} ${_RMFLAGS_} ${.TARGET}
	${CP} ${${${.TARGET:.S=.o}_SOURCE}:P} ${.TARGET}

${SOBJS} lowmem_vectors.o: $${.TARGET:.o=.S} lowmem_vectors.S
	${_CC_} -E ${_CCFLAGS_} -DASSEMBLER ${${.TARGET:.o=.S}:P} \
		> ${.TARGET:.o=.pp}
	${SED} '/^#/d' ${.TARGET:.o=.pp} > ${.TARGET:.o=.s}
	${_CC_} ${_CCFLAGS_} -mppc -Wa,-Z -c ${.TARGET:.o=.s}
	${RM} ${_RMFLAGS_} ${.TARGET:.o=.pp} ${.TARGET:.o=.s}

makedis_CCTYPE=host

makedis: ddb/makedis.c
	${_CC_} -o ${.TARGET} ${ddb/makedis.c:P}

ppc_disasm.c: ppc/ppc_disasm.i makedis
	./makedis -w -h ./ppc_disasm.h ${ppc/ppc_disasm.i:P} > ./ppc_disasm.c

ppc_disasm.o_CFLAGS += -Dperror=db_printf -Dexit=db_error -Dmalloc=db_disasm_malloc

.ORDER: ppc_disasm.c db_disasm.o
