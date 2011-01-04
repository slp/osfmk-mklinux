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

VPATH		+= :${TARGET_MACHINE}:${EXT2FS_VPATH}:${BEXT2FS_VPATH}:${MINIXFS_VPATH}:${HFS_VPATH}
INCFLAGS	+= -I${TARGET_MACHINE} ${EXT2FS_INCFLAGS} ${BEXT2FS_INCFLAGS} ${MINIXFS_INCFLAGS} ${HFS_INCFLAGS}

${TARGET_MACHINE}_OFILES = ${EXT2FS_OFILES} ${BEXT2FS_OFILES} ${MINIXFS_OFILES} ${HFS_OFILES} fs_switch.o

.include "ext2fs/machdep.mk"
.include "bext2fs/machdep.mk"
.include "minixfs/machdep.mk"
.include "POWERMAC/COPYLEFT/hfs/machdep.mk"





