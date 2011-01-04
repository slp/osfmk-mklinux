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

VPATH		+= :${TARGET_MACHINE}:${UFS_VPATH}:${EXT2FS_VPATH}:${BEXT2FS_VPATH}
INCFLAGS	+= -I${TARGET_MACHINE} ${UFS_INCFLAGS} ${EXT2FS_INCFLAGS} ${BEXT2FS_INCFLAGS}

HP700_OFILES	= ${EXT2FS_OFILES} ${BEXT2FS_OFILES} ${UFS_OFILES} fs_switch.o 

.if ${OBJECT_FORMAT} == "ELF"
CFLAGS		+= -mportable-runtime
.else
CFLAGS		+= -DHPOSFCOMPAT -DOSF
HP700_OFILES	+= hpux_fs.o
.endif

.include "ext2fs/machdep.mk"
.include "bext2fs/machdep.mk"
.include "ufs/machdep.mk"





