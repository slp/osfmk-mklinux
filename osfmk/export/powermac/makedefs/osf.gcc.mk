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


.if !defined(_OSF_GCC_MK_)
_OSF_GCC_MK_=

#
# Variables used externally to identify this compiler
#
CC_COMPILER=GCC
CC_NAME=gcc

# Flag to just get rid of the startfile

_CC_NOSTARTFILE_=-nostartfiles

#
#  Standalone Mach server linker flags
#
_MACHO_SA_MACH_LDFLAGS_=${_CC_NOSTARTFILE_} -nostdlib -e ___start_mach -u ___start_mach
_ELF_SA_MACH_LDFLAGS_=${_CC_NOSTARTFILE_} -nostdlib -e __start_mach -u __start_mach
_SOM_SA_MACH_LDFLAGS_=${_CC_NOSTARTFILE_} -nostdlib -e __start_mach -u __start_mach
SA_MACH_LDFLAGS=${_${OBJECT_FORMAT}_SA_MACH_LDFLAGS_}

#
#  Flags to get the loader to strip symbols.  Do not strip local
#  functions if profiling.
#
_PROF_XSTRIP ?= ${PROFILING:D-X:U-x}
_MACHO_LDSTRIP_	= -%ld," ${_PROF_XSTRIP}" -%ld," -S"
.if defined(GCC_LATEST)
_ELF_LDSTRIP_	= -Xlinker ${_PROF_XSTRIP} -Xlinker -S
.else
_ELF_LDSTRIP_	= -%ld," ${_PROF_XSTRIP}" -%ld," -S"
.endif

#
#  Strip options when invoking 'cc'
#
_MACHO_SHLDSTRIP_ = ${_PROF_XSTRIP} -S
_ELF_SHLDSTRIP_	  = ${_PROF_XSTRIP} -S

#
#  Linker variations
#
ANSI_LD?=${_CC_EXEC_PREFIX_}ld
TRADITIONAL_LD?=${ANSI_LD}
WRITABLE_STRINGS_LD?=${ANSI_LD}

#
#  Strip options when invoking 'ld' directly
#
LDSTRIP	   = ${_${OBJECT_FORMAT}_LDSTRIP_}
SHLDSTRIP  = ${_${OBJECT_FORMAT}_SHLDSTRIP_}

#
#  Linker Flags to create a shared library.
#
_MACHO_CREATE_SHLIB_LDFLAGS_ = -R ${EXPORTS}
.if defined(GCC_LATEST)
_ELF_CREATE_SHLIB_LDFLAGS_ = -shared -soname ${.TARGET}
.else
_ELF_CREATE_SHLIB_LDFLAGS_ = -G -dy -Ttext 0 -h ${.TARGET}
.endif

#
#  C compiler variations
#
.if ${_CC_EXEC_PREFIX_} == ""
BASE_CC?=${CC_NAME} ${GLINE} -fno-builtin
.else
BASE_CC?=${_CC_EXEC_PREFIX_}${CC_NAME} -B${_CC_EXEC_PREFIX_} ${GLINE} -fno-builtin
.endif
ANSI_CC?=${BASE_CC} ${${.TARGET}_NO_STRICT_ANSI:D:U${NO_STRICT_ANSI:D:U-pedantic -Wpointer-arith}}
TRADITIONAL_CC?=${BASE_CC} -traditional
WRITABLE_STRINGS_CC?=${BASE_CC} -fwritable-strings

.if !defined(CC_OPT_LEVEL)
CC_OPT_LEVEL = -O2
.endif

#
# Macros to enable GCC specific warnings
#
_CC_PTR_WARNING_=-Wpointer_arith
ANSI_CC?=${BASE_CC} ${${.TARGET}_NO_STRICT_ANSI:D:U${NO_STRICT_ANSI:D:U-pedantic -Wpointer-arith}}

MACHO_EXTRA_WARNINGS	= -Wall\
			  -Waggregate-return\
			  -Wcast-align\
			  -Wimplicit\
			  ${${.TARGET}_NO_STRICT_PROTOTYPES:D:U${NO_STRICT_PROTOTYPES:D:U-Wmissing-prototypes -Wstrict-prototypes}}\
			  -Wnested-externs\
			  -Wno-parentheses\
			  -Wshadow\
			  -Wtraditional\
			  -Wwrite-strings

ELF_EXTRA_WARNINGS	= -Wall\
			  -Waggregate-return\
			  -Wcast-align\
			  -Wimplicit\
			  ${${.TARGET}_NO_STRICT_PROTOTYPES:D:U${NO_STRICT_PROTOTYPES:D:U-Wmissing-prototypes -Wstrict-prototypes}}\
			  -Wnested-externs\
			  -Wno-parentheses\
			  -Wshadow\
			  -Wtraditional\
			  -Wwrite-strings

SOM_EXTRA_WARNINGS	= -Wall\
			  -Waggregate-return\
			  -Wcast-align\
			  -Wimplicit\
			  ${${.TARGET}_NO_STRICT_PROTOTYPES:D:U${NO_STRICT_PROTOTYPES:D:U-Wmissing-prototypes -Wstrict-prototypes}}\
			  -Wnested-externs\
			  -Wno-parentheses\
			  -Wshadow\
			  -Wtraditional\
			  -Wwrite-strings

.if ${GCC_LATEST} == "2_7_1"
ELF_EXTRA_WARNINGS+=-Wno-unused-parm
MACHO_EXTRA_WARNINGS+=-Wno-unused-parm
.endif

#
# Compiler specific switch to disable standard include path search
# for header files
#
_host_NOSTDINC_=
_ansi_NOSTDINC_=-nostdinc
_traditional_NOSTDINC_=-nostdinc
_writable_strings_NOSTDINC_=-nostdinc

#
# MIG invokes GCC's preprocessor directly, so this is really
# GCC's nostdinc flag
#
_CC_MIG_NOSTDINC_=-nostdinc

#
#  C flags for turning on/off position independent code.
#
.if ${USE_SHARED_LIBRARIES}
_MACHO_PIC_=-pic-extern
.if defined(GCC_LATEST)
# Use -fpic only for objects of a shared lib
.if defined(SHARED_LIBRARIES)
_ELF_PIC_=-fpic
.else
_ELF_PIC_=
.endif
.else
_ELF_PIC_=-fpic -dy 
.endif
.else
_MACHO_PIC_=-pic-none
_ELF_PIC_=
.endif

_host_PIC_=
_ansi_PIC_=${_${OBJECT_FORMAT}_PIC_}
_traditional_PIC_=${_${OBJECT_FORMAT}_PIC_}
_writable_strings_PIC_=${_${OBJECT_FORMAT}_PIC_}
_CC_PICLIB_=${_${_CCTYPE_}_PIC_}

#
#  Variables to send to linker when not generating
#  shared libraries.
#
_MACHO_NOSHRLIB_=-noshrlib
.if defined(GCC_LATEST)
_ELF_NOSHRLIB_=-static
.else
_ELF_NOSHRLIB_=-dn
.endif

#
#  Any special linker options dealing with non-pic code
#  and shared libraries.
#
.if ${USE_SHARED_LIBRARIES}
_MACHO_GLUE_=-%ld," -warn_nopic -glue"
_ELF_GLUE_=
_COFF_GLUE_=
.else
_${OBJECT_FORMAT}_GLUE_=${_${OBJECT_FORMAT}_NOSHRLIB_}
.endif

#
# Start/End files dynamic discovery
#
.if !defined(NO_STARTFILES)

#_host_LDFLAGS_=
_ansi_LDFLAGS_=-nostartfiles
_traditional_LDFLAGS_=-nostartfiles
_writable_strings_LDFLAGS_=-nostartfiles
_CC_LDFLAGS_=${_${_CCTYPE_}_LDFLAGS_}

_START_FILE_0_=${_${_CCTYPE_}_LDFLAGS_:D${:!for i in `echo $$LIBDIRS | sed 's/-L//g'` ; do if [ -n "$$i" -a -r "$$i/crt0.o" ]; then echo "$$i/crt0.o"; break; elif [ -n "$$i" -a -r "$$i/crt1.o" ]; then echo "$$i/crt1.o"; break; fi done!}}
_START_FILE_I_=${_${_CCTYPE_}_LDFLAGS_:D${:!for i in `echo $$LIBDIRS | sed 's/-L//g'` ; do if [ -n "$$i" -a -r "$$i/crti.o" ]; then echo "$$i/crti.o"; break; fi done!}}
_START_FILE_N_=${_${_CCTYPE_}_LDFLAGS_:D${:!for i in `echo $$LIBDIRS | sed 's/-L//g'` ; do if [ -n "$$i" -a -r "$$i/crtn.o" ]; then echo "$$i/crtn.o"; break; fi done!}}

.endif	# !NO_STARTFILES

.endif	# _OSF_GCC_MK_
