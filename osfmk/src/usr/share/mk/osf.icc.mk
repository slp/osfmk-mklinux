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

.if !defined(_OSF_ICC_MK_)
_OSF_ICC_MK_=

#
# Variables used externally to identify this compiler
#
CC_COMPILER=ICC
CC_NAME=icc

#
# The PGI compiler does not handle optimization well,
# so clear CC_OPT_LEVEL
#
CC_OPT_LEVEL ?=
CFLAGS+=	-DPGI

#
#  Standalone Mach server linker flags
#
_MACHO_SA_MACH_LDFLAGS_=-Mnostdlib -Mnostartup -Wl,-e___start_mach,-u___start_mach
_ELF_SA_MACH_LDFLAGS_=-Mnostdlib -Mnostartup -Wl,-e___start_mach,-u___start_mach
_COFF_SA_MACH_LDFLAGS_=-Mnostdlib -Mnostartup -Wl,-e___start_mach,-u___start_mach
SA_MACH_LDFLAGS=${_${OBJECT_FORMAT}_SA_MACH_LDFLAGS_}
LIBS += -lpgc


#
#  Flags to get the loader to strip symbols.  
#  ld860.exe does not have a switch to protect local functions.
#
_MACHO_SHLDSTRIP_ = ${_PROF_XSTRIP} -s
_ELF_SHLDSTRIP_	  = ${_PROF_XSTRIP} -s
_COFF_SHLDSTRIP_  = ${_PROF_XSTRIP} -s

#
#  Linker variations
#
ANSI_LD?=${_CC_EXEC_PREFIX_}ld860.exe
TRADITIONAL_LD?=${ANSI_LD}
WRITABLE_STRINGS_LD?=${ANSI_LD}

#
#  Strip options when invoking 'ld' directly
#
LDSTRIP	   = ${_${OBJECT_FORMAT}_LDSTRIP_}
SHLDSTRIP  = ${_${OBJECT_FORMAT}_SHLDSTRIP_}

#
#  Linker Flags to create a shared library.
#  ld860.exe does not do shared libraries.
#
_MACHO_CREATE_SHLIB_LDFLAGS_ = 
_ELF_CREATE_SHLIB_LDFLAGS_ = 
_COFF_CREATE_SHLIB_LDFLAGS_ = 

#
# Various macros
#
_CC_PTR_WARNING_=

#
#  C compiler variations
#
BASE_CC?=${_CC_EXEC_PREFIX_}${CC_NAME} ${GLINE}
ANSI_CC?=${BASE_CC}
TRADITIONAL_CC?=${BASE_CC}
WRITABLE_STRINGS_CC?=${BASE_CC}

#
# ICC does not seem to have specific warning parameters
#
MACHO_EXTRA_WARNINGS	= 
ELF_EXTRA_WARNINGS	=
COFF_EXTRA_WARNINGS	=

#
# Compiler specific switch to disable standard include path search
# for header files
#
_host_NOSTDINC_=-nostdinc
_ansi_NOSTDINC_=-Mnostdinc
_traditional_NOSTDINC_=-Mnostdinc
_writable_strings_NOSTDINC_=-Mnostdinc

#
# MIG invokes GCC's preprocessor directly, so this is really
# GCC's nostdinc flag
#
_CC_MIG_NOSTDINC_=-nostdinc

#
# C flags for turning on/off position independent code.
# ICC does not support PIC
#
_MACHO_PIC_=
_ELF_PIC_=
_COFF_PIC_=
_host_PIC_=
_ansi_PIC_=${_${OBJECT_FORMAT}_PIC_}
_traditional_PIC_=${_${OBJECT_FORMAT}_PIC_}
_writable_strings_PIC_=${_${OBJECT_FORMAT}_PIC_}

#
#  Variables to send to linker when not generating
#  shared libraries.
#
_MACHO_NOSHRLIB_=
_ELF_NOSHRLIB_=
_COFF_NOSHRLIB_=

#
#  Any special linker options dealing with non-pic code
#  and shared libraries.
#
.if ${USE_SHARED_LIBRARIES}
_MACHO_GLUE_=
_ELF_GLUE_=
_COFF_GLUE_=
.else
_${OBJECT_FORMAT}_GLUE_=${_${OBJECT_FORMAT}_NOSHRLIB_}
.endif

#
# icc doesn't grok .S suffix -- nor will it preprocess a .s file
# with -E.  So copy the .S file to a .c file (sic), removing assembler
# comments so that ice won't barf on unmatched single quotes
# (apostrophes).  Once the file is preprocessed, we have to undo the
# tokenization helpfully provided by ice -E, as well.
#

${SOBJS:.o=.c}: $${.TARGET:.c=.S}
	${SED} \
		-e 's;//.*;;' ${.TARGET:.c=.S} > ${.TARGET}

${SOBJS}: $${.TARGET:.o=.c}
	${_traditional_CC_} -E ${_CCFLAGS_} -DASSEMBLER ${${.TARGET:.o=.c}:P} \
		> ${.TARGET:.o=.pp}
	${SED} \
		-e 's;: :;::;' \
		-e 's;/ /;//;' \
		-e 's;\([a-z0-9]\) \. \([a-z]\)[ 	];\1.\2 ;' \
		-e 's;\(\.[a-z]*\) _ \([_a-z0-9]\);\1 _\2;g' \
		-e 's;_ \([_a-z0-9]\);_\1;g' \
		-e 's;\. ;.;g' \
		${.TARGET:.o=.pp} > ${.TARGET:.o=.s}
	${KCC} -c ${_CCFLAGS_} -DASSEMBLER ${.TARGET:.o=.s}
	${RM} ${_RMFLAGS_} ${.TARGET:.o=.pp} ${.TARGET:.o=.s}

.endif
