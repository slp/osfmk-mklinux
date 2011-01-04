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

# Only embed default configs via DEFAULT_CONFIG.
# Don't embed specific configs with MACH_KERNEL_${TARGET_MACHINE}_CONFIG
# and MACH_KERNEL_CONFIG, as these should optionally be set in environment.

# {DEFAULT_}CONFIG defs also in ../Makefile; reflect changes there as well

###############################################################################
###############################################################################
###                                                                         ###
###       NOTE:  the default kernel configuration should also match         ###
### src/mach_services/cmds/mach_perf/standalone/{TARGET_MACHINE}/machdep.mk ###
###                                                                         ###
###############################################################################
###############################################################################
DEFAULT_CONFIG=PRODUCTION
.if ${TARGET_MACHINE} == "INTEL860"
DEFAULT_CONFIG=FAST+PARAGON+NIC_A
.endif

CONFIG=${MACH_KERNEL_${TARGET_MACHINE}_CONFIG:U${MACH_KERNEL_CONFIG:U${DEFAULT_CONFIG}}}

CONFIG_OTHERS	= ../${CONF}/Makefile \
		  ../${CONF}/platforms.h \
		  ../${CONF}/vers.config \
		  ../${CONF}/device/Makefile \
		  ../${CONF}/mach/Makefile \
		  ../${CONF}/mach_debug/Makefile \
		  ../${CONFIG}/kern/Makefile \
		  ../${CONFIG}/uk_xkern/Makefile \
		  ../${CONFIG}/xmm/Makefile 
OTHERS		= ${CONFIG:@CONF@${CONFIG_OTHERS}@}

DEPENDENCIES	=

.include <${RULES_MK}>

.LINKS: ${CONFIG:@CONF@../${CONF}/platforms.h@}

${CONFIG:@CONF@../${CONF}/Makefile@}: ${TARGET_MACHINE}/$${.TARGET:H:T}
	@echo "[ configuring ${.TARGET:H:T} ]"
	@-if [ -d ../${.TARGET:H:T} ]; \
	  then true; \
	  else mkdir ../${.TARGET:H:T}; \
	  fi
	@-rm -f ${.TARGET:H:T}
	@cp ${${TARGET_MACHINE}/${.TARGET:H:T}:P} ${.TARGET:H:T}
	${_CONFIG_} -MD `genpath -I.` -c . ${CONFIG_FLAGS} ${.TARGET:H:T}

${CONFIG:@CONF@../${CONF}/platforms.h@}: ../$${.TARGET:H:T}/Makefile
	-${RM} ${_RMFLAGS_} ${.TARGET}
	-${LN} -s cputypes.h ${.TARGET}

${CONFIG:@CONF@../${CONF}/vers.config@}: ../$${.TARGET:H:T}/Makefile
	echo ${.TARGET:H:T} > ${.TARGET}

${CONFIG:@CONF@../${CONF}/device/Makefile@}: \
		../$${.TARGET:H:H:T}/Makefile ../device/template.mk
	${MAKEPATH} ../${.TARGET:H:H:T}/device/.
	sed -n -e '/^IDENT=/p' ../${.TARGET:H:H:T}/Makefile > ${.TARGET}.X
	cat ${../device/template.mk:P} >> ${.TARGET}.X
	${MV} ${.TARGET}.X ${.TARGET}

${CONFIG:@CONF@../${CONF}/mach/Makefile@}: \
		../$${.TARGET:H:H:T}/Makefile ../mach/template.mk
	${MAKEPATH} ../${.TARGET:H:H:T}/mach/.
	sed -n -e '/^IDENT=/p' ../${.TARGET:H:H:T}/Makefile > ${.TARGET}.X
	cat ${../mach/template.mk:P} >> ${.TARGET}.X
	${MV} ${.TARGET}.X ${.TARGET}

${CONFIG:@CONF@../${CONF}/mach_debug/Makefile@}: \
		../$${.TARGET:H:H:T}/Makefile ../mach_debug/template.mk
	${MAKEPATH} ../${.TARGET:H:H:T}/mach_debug/.
	sed -n -e '/^IDENT=/p' ../${.TARGET:H:H:T}/Makefile > ${.TARGET}.X
	cat ${../mach_debug/template.mk:P} >> ${.TARGET}.X
	${MV} ${.TARGET}.X ${.TARGET}

../${CONFIG}/uk_xkern/Makefile: ../${CONFIG}/Makefile \
				  ../uk_xkern/gen/template.mk
	${MAKEPATH} ../${CONFIG}/uk_xkern/.
	sed -n -e '/^IDENT=/p' ../${CONFIG}/Makefile > ${.TARGET}.X
	sed -n -e '/^XK_PROXY=/p' ../${CONFIG}/Makefile >> ${.TARGET}.X
	cat ${../uk_xkern/gen/template.mk:P} >> ${.TARGET}.X
	${MV} ${.TARGET}.X ${.TARGET}

../${CONFIG}/kern/Makefile: ../${CONFIG}/Makefile \
			     ../kern/template.mk
	${MAKEPATH} ../${CONFIG}/kern/.
	sed -n -e '/^IDENT=/p' ../${CONFIG}/Makefile > ${.TARGET}.X
	cat ${../kern/template.mk:P} >> ${.TARGET}.X
	${MV} ${.TARGET}.X ${.TARGET}

../${CONFIG}/xmm/Makefile: ../${CONFIG}/Makefile \
			     ../xmm/template.mk
	${MAKEPATH} ../${CONFIG}/xmm/.
	sed -n -e '/^IDENT=/p' ../${CONFIG}/Makefile > ${.TARGET}.X
	cat ${../xmm/template.mk:P} >> ${.TARGET}.X
	${MV} ${.TARGET}.X ${.TARGET}

.if exists(config.local)
${CONFIG:@CONF@../${CONF}/Makefile@}: config.local
.else
config.local:
	@true
.endif

.if exists(${TARGET_MACHINE}/config.local)
${CONFIG:@CONF@../${CONF}/Makefile@}: ${TARGET_MACHINE}/config.local
.else
${TARGET_MACHINE}/config.local:
	@true
.endif

.if exists(depend.mk)
.include "depend.mk"
.endif





