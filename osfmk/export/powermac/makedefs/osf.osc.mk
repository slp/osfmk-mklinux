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

.if !defined(_OSF_OSC_MK_)
_OSF_OSC_MK_=

#
#  Shortened for macro definitions, not to be used within a Makefile.
#
_T_M_=${TARGET_MACHINE}

#
#  Definitions for object file format - A_OUT, COFF or MACHO
#
.if !defined(USE_STATIC_LIBRARIES)
${_T_M_}_OBJECT_FORMAT?=ELF
.else
${_T_M_}_OBJECT_FORMAT?=ELF
.endif
OBJECT_FORMAT?=${${_T_M_}_OBJECT_FORMAT}

#
#  Definitions for archive file format - ASCARCH or COFF
#
${_T_M_}_ARCHIVE_FORMAT?=COFF
ARCHIVE_FORMAT?=${${_T_M_}_ARCHIVE_FORMAT}

#
#  Shared libraries definitions
#
#  First, see if we have specified an object format that does not
#  support shared libraries.
#
.if !defined(${OBJECT_FORMAT}_SHARED_LIBRARIES)
NO_SHARED_LIBRARIES=
.endif

#
#  Now see if anyone has specified STATIC.
#
.if defined(NO_SHARED_LIBRARIES) || defined(USE_STATIC_LIBRARIES)
USE_SHARED_LIBRARIES=0
.else
USE_SHARED_LIBRARIES=1
.endif

#
#  Define ${_T_M_}_VA_ARGV to be either VA_ARGV_IS_RECAST
#  to recast to char **, otherwise define VA_ARGV_IS_ROUTINE
#  If not defined here, we become VA_ARGV_UNKNOWN which should invoke
#  a #error directive where needed.
#
AT386_VA_ARGV=VA_ARGV_IS_RECAST
MMAX_VA_ARGV=VA_ARGV_IS_RECAST
PMAX_VA_ARGV=VA_ARGV_IS_RECAST
${_T_M_}_VA_ARGV?=VA_ARGV_UNKNOWN

#
#  Defined whether characters are sign or zero extended
#
AT386_CHAR_EXTEND=CHARS_EXTEND_SIGN
MMAX_CHAR_EXTEND=CHARS_EXTEND_ZERO
PMAX_CHAR_EXTEND=CHARS_EXTEND_ZERO
${_T_M_}_CHAR_EXTEND?=CHARS_EXTEND_UNKNOWN

#
#  Define MAILSYSTEM (used by rcs, etc. to send mail)
#
MAILSYSTEM?=SENDMAIL

#
#  Security flags
#
_ACL_SWARE_DEFS=-DSEC_ACL_SWARE
_ACL_POSIX_DEFS=-DSEC_ACL_POSIX
_ACL_DEFS_=${_${ACL_POLICY}_DEFS}
_SEC_C2_DEFS_=-DSEC_BASE ${_ACL_DEFS_}
_SEC_B1_DEFS_=-DSEC_BASE ${_ACL_DEFS_} -DSEC_PRIV -DSEC_MAC_OB


.if !defined(SEC_DEFS) && defined(SEC_LEVEL)
SEC_DEFS=${_SEC_${SEC_LEVEL}_DEFS_}
.endif

.if defined(SEC_DEFS)
SEC_LIBS=-lsecurity
.else
SEC_LIBS=
.endif
PROGRAMS_C2=${PROGRAMS_BASE} ${PROGRAMS_ACL} ${PROGRAMS_${ACL_POLICY}}
PROGRAMS_B1=${PROGRAMS_C2} ${PROGRAMS_PRIV} ${PROGRAMS_MAC}

.if defined(SEC_LEVEL) && !defined(ACL_POLICY)
ACL_POLICY=ACL_POSIX
.endif

#
#  Default locale/language information
#
LOCALE?=C
LOCALEPATH=${MAKETOP}usr/lib/nls/loc
MSGLANG?=en_US.ISO8859-1
MSGLANGPATH=${MAKETOP}usr/lib/nls/msg/${MSGLANG}

#
#  Flags to keep ${MAKEPATH} program quiet on success
#
MAKEPATHFLAGS=-quiet

#
#  Format-dependent lorder rules 
#
.if defined(ORDER_LIBRARIES)
COFF_LORDER=`lorder *.o | tsort`
ELF_LORDER=${_OFILES_}
A_OUT_LORDER=${_OFILES_}
MACHO_LORDER=`${LORDER} ${_OFILES_} | ${TSORT}`
.endif

#
#  Linker Flags to create a shared library.
#
_CREATE_SHLIB_LDFLAGS_ = ${_${OBJECT_FORMAT}_CREATE_SHLIB_LDFLAGS_} 

#
# Shared library directories
#
SHLIBDIRS = ${${OBJECT_FORMAT}_SHLIBDIRS} 

#
# Where compiler files reside
#
_CC_EXEC_PREFIX_=${${OBJECT_FORMAT}_CC_EXEC_PREFIX}

#
# Standard utilities (usually standard regardles of compiler)
#
ANSI_AR?=${_CC_EXEC_PREFIX_}ar
TRADITIONAL_AR?=${ANSI_AR}
WRITABLE_STRINGS_AR?=${ANSI_AR}

ANSI_RANLIB?=${_CC_EXEC_PREFIX_}ranlib
TRADITIONAL_RANLIB?=${ANSI_RANLIB}
WRITABLE_STRINGS_RANLIB?=${ANSI_RANLIB}

ANSI_CONFIG?=${TARGET_EXEC_PREFIX}config
TRADITIONAL_CONFIG?=${ANSI_CONFIG}
WRITABLE_STRINGS_CONFIG?=${ANSI_CONFIG}

ANSI_MIG?=${TARGET_EXEC_PREFIX}mig
TRADITIONAL_MIG?=${ANSI_MIG}
WRITABLE_STRINGS_MIG?=${ANSI_MIG}

_CCDEFS_= -D${MACHINE} -D__${MACHINE}__

.if ${USE_SHARED_LIBRARIES}
_SHCCDEFS_=-D_SHARED_LIBRARIES
.else
_SHCCDEFS_=
.endif

_PMAX_MACHO_CFLAGS_=-G 0
_MMAX_MACHO_CFLAGS_=-mnosb
_O_F_CFLAGS_=${_${_T_M_}_${OBJECT_FORMAT}_CFLAGS_}


#
# Compiler specific switch to disable standard include path search
# for header files
#
_CC_NOSTDINC_=${_${_CCTYPE_}_NOSTDINC_}

_host_GENINC_=
_ansi_GENINC_=${x:L:!${GENPATH} -I.!} -I-
_traditional_GENINC_=${x:L:!${GENPATH} -I.!} -I-
_writable_strings_GENINC_=${x:L:!${GENPATH} -I.!} -I-
_CC_GENINC_=${_${_CCTYPE_}_GENINC_}

#
#  Any special linker options dealing with non-pic code
#  and shared libraries.
#
_host_GLUE_=
_ansi_GLUE_=${_${OBJECT_FORMAT}_GLUE_}
_traditional_GLUE_=${_${OBJECT_FORMAT}_GLUE_}
_writable_strings_GLUE_=${_${OBJECT_FORMAT}_GLUE_}
_CC_GLUE_=${_${_CCTYPE_}_GLUE_}

_host_GENLIB_=
_ansi_GENLIB_=
_traditional_GENLIB_=
_writable_strings_GENLIB_=
_CC_GENLIB_=${_${_CCTYPE_}_GENLIB_}

#
# Lex specific flags
# 
_host_LFLAGS_=
_ansi_LFLAGS_=${FLEXSKEL:D-S${FLEXSKEL}}
_traditional_LFLAGS_=${FLEXSKEL:D-S${FLEXSKEL}}
_writable_strings_LFLAGS_=${FLEXSKEL:D-S${FLEXSKEL}}
_LEX_LFLAGS_=${_${_CCTYPE_}_LFLAGS_}

#
#  Project rules
#
MIG?=mig

.if defined(HOST_MIG)
_host_MIG_=${HOST_MIG}
.else
_host_MIG_=mig
.endif
.if defined(ANSI_MIG)
_ansi_MIG_=${ANSI_MIG}
.else
_ansi_MIG_=mig
.endif
.if defined(TRADITIONAL_MIG)
_traditional_MIG_=${TRADITIONAL_MIG}
.else
_traditional_MIG_=mig
.endif
.if defined(WRITABLE_STRINGS_MIG)
_writable_strings_MIG_=${WRITABLE_STRINGS_MIG}
.else
_writable_strings_MIG_=mig
.endif
_MIG_=${_${_CCTYPE_}_MIG_}

_MIGFLAGS_=\
	${${.TARGET}_MIGENV:U${MIGENV}}\
	${${.TARGET}_MIGFLAGS:U${MIGFLAGS}}\
	${${.TARGET}_MIGARGS:U${MIGARGS}}\
	${_CC_MIG_NOSTDINC_} ${_GENINC_} ${_CC_INCDIRS_}

.if defined(MIG_DEFS)
_MIG_HDRS_=${MIG_DEFS:.defs=.h}
_MIG_USRS_=${MIG_DEFS:.defs=User.c}
_MIG_SRVS_=${MIG_DEFS:.defs=Server.c}

${_MIG_HDRS_}: $${.TARGET:.h=.defs}
	${_MIG_} ${_MIGFLAGS_} ${${.TARGET:.h=.defs}:P} \
		-server /dev/null -user /dev/null

${_MIG_USRS_}: $${.TARGET:S/User.c$$/.defs/}
	${_MIG_} ${_MIGFLAGS_} ${${.TARGET:S/User.c$/.defs/}:P} \
		 -server /dev/null

${_MIG_SRVS_}: $${.TARGET:S/Server.c$$/.defs/}
	${_MIG_} ${_MIGFLAGS_} ${${.TARGET:S/Server.c$/.defs/}:P} \
		 -user /dev/null
.endif

.if defined(MSGHDRS)
${MSGHDRS}: $${.TARGET:_msg.h=.msg}
	${MKCATDEFS} ${.TARGET:_msg.h=} ${.ALLSRC} > ${.TARGET:_msg.h=.cat.in}
	-${GENCAT} ${.TARGET:_msg.h=.cat} ${.TARGET:_msg.h=.cat.in}
	-${RM} ${_RMFLAGS_} ${.TARGET:_msg.h=.cat.in}
.if empty(.TARGETS:Msetup_*)
	-${RM} ${_RMFLAGS_} ${MSGLANGPATH}/${.TARGET:_msg.h=.cat}
	${MAKEPATH} ${MAKEPATHFLAGS} ${MSGLANGPATH}/${.TARGET:_msg.h=.cat}
	${CP} ${.TARGET:_msg.h=.cat} ${MSGLANGPATH}/${.TARGET:_msg.h=.cat}
.endif
.endif

.if defined(CATFILES)
${CATFILES}: $${.TARGET:.cat=.msg}
	${MKCATDEFS} ${.TARGET:.cat=} ${.ALLSRC} > ${.TARGET}.in
	-${GENCAT} ${.TARGET} ${.TARGET}.in
	-${RM} ${_RMFLAGS_} ${.TARGET}.in
.if empty(.TARGETS:Msetup_*)
	-${RM} ${_RMFLAGS_} ${MSGLANGPATH}/${.TARGET}
	${MAKEPATH} ${MAKEPATHFLAGS} ${MSGLANGPATH}/${.TARGET}
	${CP} ${.TARGET} ${MSGLANGPATH}/${.TARGET}
.endif
.endif

.endif

COMPOSE?=compose
PTBLDUMP?=ptbldump
_COMPOSE_=${COMPOSE}
_PTBLDUMP_=${PTBLDUMP}
