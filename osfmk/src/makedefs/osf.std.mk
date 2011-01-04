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

.if !defined(_OSF_STD_MK_)
_OSF_STD_MK_=

#
#  Default rule - All other rules appear after variable definitions
#
.MAIN: build_all

#
#  Debugging entry for checking environment
#
print_env:
	@printenv

#
#  Use this as a dependency for any rule which should always be triggered
#  (e.g. recursive makes).
#
ALWAYS=

#
#  Set defaults for input variables which are not already defined
#
DEF_RMFLAGS?=-f
DEF_ARFLAGS?=crl
DEF_MDFLAGS?=-rm

ROOT_OWNER?=root
KMEM_GROUP?=kmem
TTY_GROUP?=tty
SETUID_MODE?=4711
SETGID_MODE?=2711

OWNER?=bin
IOWNER?=${OWNER}
GROUP?=bin
IGROUP?=${GROUP}
IMODE?=755

#
#  Program macros
#
AR?=whatar
AS?=as
AWK?=awk
CC?=whatcc
CHMOD?=chmod
CP?=cp
ECHO?=echo
GENCAT?=gencat
GENPATH?=genpath
LD?=whatld
LEX?=lex
LINT?=lint
LN?=ln
LORDER?=lorder
MAKEPATH?=makepath
MD?=md
MKCATDEFS?=mkcatdefs
MV?=mv
RANLIB?=whatranlib
RELEASE?=release
RM?=rm
SED?=sed
SORT?=sort
TAGS?=ctags
TAR?=tar
TOUCH?=touch
TR?=tr
TSORT?=tsort
UUDECODE?=uudecode
XSTR?=xstr
YACC?=yacc

#
#  Include project specific information
#
.include <osf.${project_name}.mk>

#
# Include compiler specific information
#
compiler_name?=gcc
 
.include <osf.${compiler_name}.mk>

#
#  C compiler variations
#
#
# Certain portions of the build need to lex/compile/run
# programs. These use the CCTYPE=host flag in order to provide the
# "right" lexer/compiler/linker/archiver
#
CCTYPE?=ansi
_CCTYPE_=${${.TARGET}_CCTYPE:U${CCTYPE}}

.if defined(HOST_CC)
_host_CC_=${HOST_CC}
.else
_host_CC_=cc
.endif
.if defined(ANSI_CC)
_ansi_CC_=${ANSI_CC}
.else
_ansi_CC_=cc
.endif
.if defined(TRADITIONAL_CC)
_traditional_CC_=${TRADITIONAL_CC}
.else
_traditional_CC_=cc
.endif
.if defined(WRITABLE_STRINGS_CC)
_writable_strings_CC_=${WRITABLE_STRINGS_CC}
.else
_writable_strings_CC_=cc
.endif
_CC_=${_${_CCTYPE_}_CC_}

.if defined(HOST_CPLUSPLUS)
_host_CPLUSPLUS_=${HOST_CPLUSPLUS}
.else
_host_CPLUSPLUS_=cc
.endif
.if defined(ANSI_CPLUSPLUS)
_ansi_CPLUSPLUS_=${ANSI_CPLUSPLUS}
.else
_ansi_CPLUSPLUS_=${_ansi_CC_}
.endif
.if defined(TRADITIONAL_CPLUSPLUS)
_traditional_CPLUSPLUS_=${TRADITIONAL_CPLUSPLUS}
.else
_traditional_CPLUSPLUS_=${_traditional_CC_}
.endif
.if defined(WRITABLE_STRINGS_CPLUSPLUS)
_writable_strings_CPLUSPLUS_=${WRITABLE_STRINGS_CPLUSPLUS}
.else
_writable_strings_CPLUSPLUS_=${_traditional_CC_}
.endif
_CPLUSPLUS_=${_${_CCTYPE_}_CPLUSPLUS_}

.if defined(HOST_LD)
_host_LD_=${HOST_LD}
.else
_host_LD_=ld
.endif
.if defined(ANSI_LD)
_ansi_LD_=${ANSI_LD}
.else
_ansi_LD_=ld
.endif
.if defined(TRADITIONAL_LD)
_traditional_LD_=${TRADITIONAL_LD}
.else
_traditional_LD_=ld
.endif
.if defined(WRITABLE_STRINGS_LD)
_writable_strings_LD_=${WRITABLE_STRINGS_LD}
.else
_writable_strings_LD_=ld
.endif
_LD_=${_${_CCTYPE_}_LD_}

.if defined(HOST_AR)
_host_AR_=${HOST_AR}
.else
_host_AR_=ar
.endif
.if defined(ANSI_AR)
_ansi_AR_=${ANSI_AR}
.else
_ansi_AR_=ar
.endif
.if defined(TRADITIONAL_AR)
_traditional_AR_=${TRADITIONAL_AR}
.else
_traditional_AR_=ar
.endif
.if defined(WRITABLE_STRINGS_AR)
_writable_strings_AR_=${WRITABLE_STRINGS_AR}
.else
_writable_strings_AR_=ar
.endif
_AR_=${_${_CCTYPE_}_AR_}

.if defined(HOST_RANLIB)
_host_RANLIB_=${HOST_RANLIB}
.else
_host_RANLIB_=ranlib
.endif
.if defined(ANSI_RANLIB)
_ansi_RANLIB_=${ANSI_RANLIB}
.else
_ansi_RANLIB_=ranlib
.endif
.if defined(TRADITIONAL_RANLIB)
_traditional_RANLIB_=${TRADITIONAL_RANLIB}
.else
_traditional_RANLIB_=ranlib
.endif
.if defined(WRITABLE_STRINGS_RANLIB)
_writable_strings_RANLIB_=${WRITABLE_STRINGS_RANLIB}
.else
_writable_strings_RANLIB_=ranlib
.endif
_RANLIB_=${_${_CCTYPE_}_RANLIB_}

_host_CFLAGS_=
_ansi_CFLAGS_=${_O_F_CFLAGS_} ${_CCDEFS_} ${_SHCCDEFS_}
_traditional_CFLAGS_=${_ansi_CFLAGS_}
_writable_strings_CFLAGS_=${_ansi_CFLAGS_}
_CC_CFLAGS_=${_${_CCTYPE_}_CFLAGS_} 

INCDIRS += ${EXTRA_INCDIRS}

_host_INCDIRS_=
_ansi_INCDIRS_=${INCDIRS}
_traditional_INCDIRS_=${INCDIRS}
_writable_strings_INCDIRS_=${INCDIRS}
_CC_INCDIRS_=${_${_CCTYPE_}_INCDIRS_}

# C++ specific include directory that must come before
# the standard include files.  Typically the include
# files here are just a extern "C" {} shell around the
# real include files.
CPLUSPLUS_INCDIRS=-I${_CC_EXEC_PREFIX_}g++-include

LIBDIRS += ${EXTRA_LIBDIRS}

_host_LIBDIRS_=
.if defined(USE_LOADER_ONLY)
_ansi_LIBDIRS_=${SHLIBDIRS:S;/usr/shlib$;/usr/shlib/loader;g} ${LIBDIRS}
.elif ${USE_SHARED_LIBRARIES}
_ansi_LIBDIRS_=${SHLIBDIRS} ${LIBDIRS}
.else
_ansi_LIBDIRS_=${LIBDIRS}
.endif
_traditional_LIBDIRS_=${_ansi_LIBDIRS_}
_writable_strings_LIBDIRS_=${_ansi_LIBDIRS_}
_CC_LIBDIRS_=${_${_CCTYPE_}_LIBDIRS_}

_YACC_=${YACC}
_LEX_=${LEX}

.if defined(HOST_CONFIG)
_host_CONFIG_=${HOST_CONFIG}
.else
_host_CONFIG_=config
.endif
.if defined(ANSI_CONFIG)
_ansi_CONFIG_=${ANSI_CONFIG}
.else
_ansi_CONFIG_=config
.endif
.if defined(TRADITIONAL_CONFIG)
_traditional_CONFIG_=${TRADITIONAL_CONFIG}
.else
_traditional_CONFIG_=config
.endif
.if defined(WRITABLE_STRINGS_CONFIG)
_writable_strings_CONFIG_=${WRITABLE_STRINGS_CONFIG}
.else
_writable_strings_CONFIG_=config
.endif
_CONFIG_=${_${_CCTYPE_}_CONFIG_}

#
#
#  Compilation optimization level.  This should be set to whatever
#  combination of -O and -g flags you desire.  Defaults to -O.
#
#  Allow these flags to be overridden per target
#
.if defined(OPT_LEVEL)
_CC_OL_=${${.TARGET}_OPT_LEVEL:U${OPT_LEVEL}}
_LD_OL_=${${.TARGET}_OPT_LEVEL:U${OPT_LEVEL}}
.else
CC_OPT_LEVEL?=-O
_CC_OL_=${${.TARGET}_OPT_LEVEL:U${${.TARGET}_CC_OPT_LEVEL:U${CC_OPT_LEVEL}}}
LD_OPT_LEVEL?=
_LD_OL_=${${.TARGET}_OPT_LEVEL:U${${.TARGET}_LD_OPT_LEVEL:U${LD_OPT_LEVEL}}}
.endif

#
#  Program flags for makefile, environment and command line args
#
_INCFLAGS_=\
	${${.TARGET}_INCARGS:U${INCARGS}}\
	${${.TARGET}_INCFLAGS:U${INCFLAGS}}\
	${${.TARGET}_INCENV:U${INCENV}}
_GENINC_=\
	${_CC_GENINC_} ${_INCFLAGS_:!${GENPATH} ${_INCFLAGS_}!}
_CPLUSPLUS_GENINC_=${_GENINC_} ${CPLUSPLUS_INCDIRS}
_LIBFLAGS_=\
	${${.TARGET}_LIBARGS:U${LIBARGS}}\
	${${.TARGET}_LIBFLAGS:U${LIBFLAGS}}\
	${${.TARGET}_LIBENV:U${LIBENV}}
_GENLIB_=\
	${_CC_GENLIB_} ${_LIBFLAGS_:!${GENPATH} ${_LIBFLAGS_}!}
_LIBS_=	${${.TARGET}_LIBSENV:U${LIBSENV}}\
	${${.TARGET}_LIBS:U${LIBS}}\
	${${.TARGET}_LIBSARGS:U${LIBSARGS}} ${TARGET_LIBS} ${_CC_LIBS_}
_CCFLAGS_NOINC_=\
	${_CC_CFLAGS_}\
	${_CC_OL_}\
	${DEPENDENCIES:D-MD:U}\
	${${.TARGET}_CENV:U${CENV}}\
	${${.TARGET}_CFLAGS:U${CFLAGS}} ${TARGET_FLAGS}\
	${${.TARGET}_CARGS:U${CARGS}}
_CCFLAGS_NOPIC_=\
	${_CCFLAGS_NOINC_}\
	${_CC_NOSTDINC_} ${_GENINC_} ${_CC_INCDIRS_}
_CCFLAGS_=${_CCFLAGS_NOPIC_} ${_CC_PICLIB_}
_CPLUSPLUSFLAGS_=\
	${_CCFLAGS_NOINC_}\
	${_CC_NOSTDINC_} ${_CPLUSPLUS_GENINC_} ${_CC_INCDIRS_} ${_CC_PICLIB_}
_LD_LDFLAGS_=\
	${${.TARGET}_LDENV:U${LDENV}}\
	${${.TARGET}_LDFLAGS:U${LDFLAGS}}\
	${${.TARGET}_LDARGS:U${LDARGS}}
_LDFLAGS_=\
	${_CC_GLUE_}\
	${_CC_LDFLAGS_}\
	${_LD_OL_}\
	${_LD_LDFLAGS_}\
	${_CC_NOSTDLIB_} ${_GENLIB_} ${_CC_LIBDIRS_}
_SHLDFLAGS_=\
	${_CREATE_SHLIB_LDFLAGS_} \
	${_CC_SHLDFLAGS_}\
	${${.TARGET}_SHLDENV:U${SHLDENV}}\
	${${.TARGET}_SHLDFLAGS:U${SHLDFLAGS}}\
	${${.TARGET}_SHLDARGS:U${SHLDARGS}}\
	${_LD_NOSTDLIB_} ${_GENLIB_} ${_CC_LIBDIRS_}
_LFLAGS_=\
	${_LEX_LFLAGS_}\
	${${.TARGET}_LENV:U${LENV}}\
	${${.TARGET}_LFLAGS:U${LFLAGS}}\
	${${.TARGET}_LARGS:U${LARGS}}
_YFLAGS_=\
	${${.TARGET}_YENV:U${YENV}}\
	${${.TARGET}_YFLAGS:U${YFLAGS}}\
	${${.TARGET}_YARGS:U${YARGS}}
_LINTFLAGS_=\
	${${.TARGET}_LINTENV:U${LINTENV}}\
	${${.TARGET}_LINTFLAGS:U${LINTFLAGS}}\
	${${.TARGET}_LINTARGS:U${LINTARGS}}\
	${_GENINC_} ${_CC_INCDIRS_}
_TAGSFLAGS_=\
	${${.TARGET}_TAGSENV:U${TAGSENV}}\
	${${.TARGET}_TAGSFLAGS:U${TAGSFLAGS}}\
	${${.TARGET}_TAGSARGS:U${TAGSARGS}}
_RMFLAGS_=\
	${${.TARGET}_DEF_RMFLAGS:U${DEF_RMFLAGS}}
_MDINCFLAGS_=-I. ${MDINCARGS} ${MDINCFLAGS} ${MDINCENV}
_MDFLAGS_=\
	${DEF_MDFLAGS}\
	${_MDINCFLAGS_:!${GENPATH} ${_MDINCFLAGS_}!} -I-\
	${_INCFLAGS_:!${GENPATH} ${_INCFLAGS_}!}\
	${_CC_INCDIRS_} ${MDENV} ${MDFLAGS} ${MDARGS}

#
#  Define these with default options added
#
_RELEASE_=${RELEASE_PREFIX}${RELEASE} ${RELEASE_OPTIONS}

#
#  Define binary targets
#
.if defined(PROGRAMS)
BINARIES+=${PROGRAMS}
.endif
.if defined(LIBRARIES)
BINARIES+=${LIBRARIES}
.endif
.if defined(SHARED_LIBRARIES)
BINARIES+=${SHARED_LIBRARIES}
.endif
.if defined(OBJECTS)
BINARIES+=${OBJECTS}
.endif

#
#  Definitions for clean/rmtarget/clobber
#
_CLEAN_TARGET=${.TARGET:S/^clean_//}
.if !defined(CLEANFILES)
_CLEAN_DEFAULT_=\
	${_CLEAN_TARGET}.X\
	${OFILES:U${${_CLEAN_TARGET}_OFILES:U${_CLEAN_TARGET}.o}}\
	${${_CLEAN_TARGET}_GARBAGE:U${GARBAGE}}
_CLEANFILES_=\
	${CLEANFILES:U${${_CLEAN_TARGET}_CLEANFILES:U${_CLEAN_DEFAULT_}}}
.endif

_RMTARGET_TARGET=${.TARGET:S/^rmtarget_//}

_CLOBBER_TARGET=${.TARGET:S/^clobber_//}
_CLOBBER_DEFAULT_=\
	${_CLOBBER_TARGET}.X\
	${OFILES:U${${_CLOBBER_TARGET}_OFILES:U${_CLOBBER_TARGET}.o}}\
	${${_CLOBBER_TARGET}_GARBAGE:U${GARBAGE}} ${CATFILES} ${MSGHDRS}
_CLOBBERFILES_=${_CLOBBER_TARGET} \
	${CLEANFILES:U${${_CLOBBER_TARGET}_CLEANFILES:U${_CLOBBER_DEFAULT_}}}

#
#  Definitions for lint
#
_LINT_TARGET=${.TARGET:S/^lint_//}
.if !defined(LINTFILES)
_LINT_OFILES_=${OFILES:U${${_LINT_TARGET}_OFILES:U${_LINT_TARGET}.o}}
LINTFILES=${${_LINT_TARGET}_LINTFILES:U${_LINT_OFILES_:.o=.c}}
.endif

#
#  Definitions for tags
#
_TAGS_TARGET=${.TARGET:S/^tags_//}
.if !defined(TAGSFILES)
_TAGS_OFILES_=${OFILES:U${${_TAGS_TARGET}_OFILES:U${_TAGS_TARGET}.o}}
TAGSFILES?=${${_TAGS_TARGET}_TAGSFILES:U${_TAGS_OFILES_:.o=.c}}
.endif

#
#  Definitions for setup
#
_SETUP_TARGET=${.TARGET:S/^setup_//}
_SETUPFILES_=${TOOLS}/${_SETUP_TARGET}

#
#  Definitions for export
#
_EXPORT_TARGET=${.TARGET:S/^export_//}
_EXPDIR_=${${_EXPORT_TARGET}_EXPDIR:U${EXPDIR:U${${_EXPORT_TARGET}_IDIR:U${IDIR:U/_MISSING_EXPDIR_/}}}}
_EXPLINKS_=${${_EXPORT_TARGET}_EXPLINKS:U${EXPLINKS}}
_DO_EXPLINKS_=\
	(cd ${EXPORTBASE}${_EXPDIR_:H};\
	 ${RM} ${_RMFLAGS_} ${_EXPLINKS_}\
	 ${_EXPLINKS_:@.LINK.@; ${LN} ${_EXPORT_TARGET} ${.LINK.}@})
.if defined(EXPLINKS)
_MAKE_EXPLINKS_=${_DO_EXPLINKS_}
.else
_MAKE_EXPLINKS_=${${_EXPORT_TARGET}_EXPLINKS:U@true:D${_DO_EXPLINKS_}}
.endif
_EXPFILES_=${EXPORTBASE}${_EXPDIR_}${_EXPORT_TARGET}

#
#  Definitions for install/release
#
_INSTALL_TARGET=${.TARGET:S/^install_//}
.if defined(TOSTAGE)

.if defined(NO_STRIP)
_NO_STRIP_=-nostrip
.else
_NO_STRIP_=${${_INSTALL_TARGET}_NOSTRIP:D-nostrip}
.endif

_IDIR_=${${_INSTALL_TARGET}_IDIR:U${IDIR:U/_MISSING_IDIR_/}}

.endif

#
#  Default single suffix compilation rules
#
.SUFFIXES:
.if !defined(TOSTAGE)
.SUFFIXES: .o .s .pp .c .h .y .l .sh .csh .txt .uu .cc .C

.uu:
	${UUDECODE} ${.IMPSRC}
.endif

#
#  Include pass definitions for standard targets
#
.include <osf.${project_name}.passes.mk>

#
#  Special rules
#

#
#  Symlink rules
#
.if defined(SYMLINKS)
.LINKS: ${SYMLINKS}

.if defined(EXPORTBASE)
.LINKS: ${SYMLINKS:@_EXPORT_TARGET@${_EXPFILES_}@}
.endif

${SYMLINKS}:
	${RM} ${_RMFLAGS_} ${.TARGET}
	${LN} -s ${${.TARGET}_LINK} ${.TARGET}
.endif

#
#  Compilation rules
#
all: build_all;@

build_all: $${_all_targets_};@

comp_all: $${_all_targets_};@

#
#  Clean up rules
#
clean_all: $${_all_targets_}
	-${RM} ${_RMFLAGS_} core

.if !empty(_CLEAN_TARGETS_:Mclean_*)
${_CLEAN_TARGETS_:Mclean_*}:
	-${RM} ${_RMFLAGS_} ${_CLEANFILES_}
.endif

rmtarget_all: $${_all_targets_}
	-${RM} ${_RMFLAGS_} core

.if !empty(_RMTARGET_TARGETS_:Mrmtarget_*)
${_RMTARGET_TARGETS_:Mrmtarget_*}:
	-${RM} ${_RMFLAGS_} ${_RMTARGET_TARGET}
.endif

clobber_all: $${_all_targets_}
	-${RM} ${_RMFLAGS_} core depend.mk

.if !empty(_CLOBBER_TARGETS_:Mclobber_*)
${_CLOBBER_TARGETS_:Mclobber_*}:
	-${RM} ${_RMFLAGS_} ${_CLOBBERFILES_}
.endif

#
#  Lint rules
#
lint_all: $${_all_targets_};@

.if !empty(_LINT_TARGETS_:Mlint_*)
${_LINT_TARGETS_:Mlint_*}: $${LINTFILES}
	${LINT} ${_LINTFLAGS_} ${.ALLSRC}
.endif

#
#  Tags rules
#
tags_all: $${_all_targets_};@

.if !empty(_TAGS_TARGETS_:Mtags_*)
${_TAGS_TARGETS_:Mtags_*}: $${TAGSFILES}
	${TAGS} ${_TAGSFLAGS_} ${.ALLSRC}
.endif

#
# Setup rules
#
setup_all: $${_all_targets_};@
.if !empty(_SETUP_TARGETS_:Msetup_*)
${_SETUP_TARGETS_:Msetup_*}: $${_SETUPFILES_};@
${_SETUP_TARGETS_:Msetup_*:S/^setup_//g:@_SETUP_TARGET@${_SETUPFILES_}@}:\
		$${.TARGET:T}
	-${RM} ${_RMFLAGS_} ${.TARGET}
	${MAKEPATH} ${.TARGET}
.if defined(EXPORT_USING_TAR)
	@echo " Exporting ${.ALLSRC} to ${.TARGET}"
	@if [ -f ${.ALLSRC} ]; then \
		(cd ${.ALLSRC:H}; ${TAR} chf - ${.ALLSRC:T}) | \
		(cd ${.TARGET:H}; ${TAR} xf -); \
	else \
		(cd ${.ALLSRC:H}; ${TAR} cf - ${.ALLSRC:T}) | \
		(cd ${.TARGET:H}; ${TAR} xf -); \
	fi
.else
.if defined(EXPORT_DONT_FOLLOW_LINKS)
	${CP} -p -R ${.ALLSRC} ${.TARGET}
.else
	@echo " Exporting ${.ALLSRC} to ${.TARGET}"
	@if [ -f ${.ALLSRC} ]; then \
		${CP} -p -R -h ${.ALLSRC} ${.TARGET}; \
	else \
		${CP} -p -R ${.ALLSRC} ${.TARGET}; \
	fi
.endif
.endif
.if	defined(SETUP_LINKS)
	${RM} ${_RMFLAGS_} ${TOOLS}/${SETUP_LINKS} 
	${LN} ${.TARGET} ${TOOLS}/${SETUP_LINKS}
.endif
.endif

#
#  Export rules
#
.if !defined(EXPORTBASE)

export_all: ${ALWAYS}
	@echo "You must define EXPORTBASE to do an ${.TARGET}"

.if !empty(_EXPORT_TARGETS_:Mexport_*)
${_EXPORT_TARGETS_:Mexport_*}: ${ALWAYS}
	@echo "You must define EXPORTBASE to do an ${.TARGET}"
.endif

.else

export_all: $${_all_targets_}

.if !empty(_EXPORT_TARGETS_:Mexport_*)
${_EXPORT_TARGETS_:Mexport_*}: $${_EXPFILES_};@

${_EXPORT_TARGETS_:Mexport_*:S/^export_//g:@_EXPORT_TARGET@${_EXPFILES_}@}:\
		$${.TARGET:T}
	-${RM} ${_RMFLAGS_} ${.TARGET}
	${MAKEPATH} ${.TARGET}
.if	defined(EXPORT_USING_TAR)
	@echo " Exporting ${.ALLSRC} to ${.TARGET}"
	@if [ -f ${.ALLSRC} ]; then \
		(cd ${.ALLSRC:H}; ${TAR} chf - ${.ALLSRC:T}) | \
		(cd ${.TARGET:H}; ${TAR} xf -); \
	else \
		(cd ${.ALLSRC:H}; ${TAR} cf - ${.ALLSRC:T}) | \
		(cd ${.TARGET:H}; ${TAR} xf -); \
	fi
.else
.if defined(EXPORT_DONT_FOLLOW_LINKS)
	${CP} -p -R ${.ALLSRC} ${.TARGET}
.else
	@echo " Exporting ${.ALLSRC} to ${.TARGET}"
	@if [ -f ${.ALLSRC} ]; then \
		${CP} -p -R -h ${.ALLSRC} ${.TARGET}; \
	else \
		${CP} -p -R ${.ALLSRC} ${.TARGET}; \
	fi
.endif
.endif
	${.ALLSRC:T:@_EXPORT_TARGET@${_MAKE_EXPLINKS_}@}
.endif

.endif

#
#  Installation/release rules
#
.if !defined(TOSTAGE)

install_all: ${ALWAYS}
	@echo "You must define TOSTAGE to do an ${.TARGET}"

.if !empty(_INSTALL_TARGETS_:Minstall_*)
${_INSTALL_TARGETS_:Minstall_*}: ${ALWAYS}
	@echo "You must define TOSTAGE to do an ${.TARGET}"
.endif

.else

install_all: $${_all_targets_};@

.if !empty(_INSTALL_TARGETS_:Minstall_*)

.if defined(FROMSTAGE)

${_INSTALL_TARGETS_:Minstall_*}: ${ALWAYS}
	${_RELEASE_} ${_NOSTRIP_}\
		-o ${${_INSTALL_TARGET}_IOWNER:U${IOWNER}}\
		-g ${${_INSTALL_TARGET}_IGROUP:U${IGROUP}}\
		-m ${${_INSTALL_TARGET}_IMODE:U${IMODE}}\
		-tostage ${TOSTAGE}\
		-fromstage ${FROMSTAGE}\
		${_IDIR_}${_INSTALL_TARGET}\
		${${_INSTALL_TARGET}_ILINKS:U${ILINKS}}

.else

${_INSTALL_TARGETS_:Minstall_*}: $${_INSTALL_TARGET}
	${_RELEASE_} ${_NOSTRIP_}\
		-o ${${_INSTALL_TARGET}_IOWNER:U${IOWNER}}\
		-g ${${_INSTALL_TARGET}_IGROUP:U${IGROUP}}\
		-m ${${_INSTALL_TARGET}_IMODE:U${IMODE}}\
		-tostage ${TOSTAGE}\
		-fromfile ${${_INSTALL_TARGET}:P}\
		${_IDIR_}${_INSTALL_TARGET}\
		${${_INSTALL_TARGET}_ILINKS:U${ILINKS}}

.endif

.endif

.endif

.endif
