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

#
# Installation rules for security tests
#

CHGRP	?=	/usr/bin/chgrp
CHPRIV	?=	/tcb/bin/chpriv
CHOWN	?=	/usr/bin/chown
CHMOD	?=	/usr/bin/chmod

.if defined(RELDIR) \
&& !empty(RELDIR)
TDTRDIR=${TESTDIR}/${RELDIR}
.else
TDTRDIR=${TESTDIR}
.endif


_TDEDIT_P_=${${.TDEDIT_P.}:P}

tdinst_all: $${_all_targets_};@

tdinst_all: _TDCHECKS_ _TDCHPRIV_  _TDSUBSUITES_
	@true

_TDCHECKS_: ${.ALWAYS}
.if !defined(SEC_LEVEL) || ((${SEC_LEVEL} != "B1") && (${SEC_LEVEL} != "C2"))
	@echo "SEC_LEVEL must be B1 or C2 for tdinst_all"
	@false
.endif
	@true

.if defined(TDPOSTINSTALL)
&& !empty(TDPOSTINSTALL)
${TDPOSTINSTALL}: _HELPERS_ ${TDINSTFILES}
.endif

# copy scripts and other files, if any, to Files dir.
# Use this rule to set rx on all scripts, and set owner/group - tdmod
# doesn't do it. No work to do if there's no Files subdirectory
# (e.g. when processing TDSUBSUITES)
_TDAUXFILES_: _HELPERS_ ${COPYSCRIPTS} ${OTHERFILES}
	@if [ -d ${TDTRDIR}/Files ] ; then \
	  ( if [ "$(COPYSCRIPTS)" != "" ] ; then \
	      ( ${CP} -h ${COPYSCRIPTS:@.TDEDIT_P.@${_TDEDIT_P_}@} \
			${TDTRDIR}/Files ; \
	      ) \
	    fi; \
	    if [ "$(OTHERFILES)" != "" ] ; then \
	      ( ${CP} -h ${OTHERFILES:@.TDEDIT_P.@${_TDEDIT_P_}@} \
			${TDTRDIR}/Files ; \
		cd ${TDTRDIR}/Files; \
		${CHMOD} ugo+r ${OTHERFILES} ;\
	      ) \
	    fi; \
	    cd ${TDTRDIR}/Files; \
	    if [ "`echo *.sh`" != "*.sh" ] ; then \
		( ${CHOWN} root *.sh; \
		  ${CHGRP} system *.sh; \
		  ${CHMOD} 755 *.sh; \
	        ) \
	    fi; \
	  ) \
	fi;

foo:

_TDCHPRIV_: _HELPERS_ ${TDINSTFILES} _TDAUXFILES_ ${TDPOSTINSTALL}
.if defined(TDINSTFILES)\
&& !empty(TDINSTFILES)	\
&& defined(SEC_LEVEL)
.if (${SEC_LEVEL} == "C2")
	${CHOWN} root ${TDINSTFILES:S/^/${TESTDIR}\/${RELDIR}\/Files\//g}
	${CHMOD} 4111 ${TDINSTFILES:S/^/${TESTDIR}\/${RELDIR}\/Files\//g}
.elif (${SEC_LEVEL} == "B1")
.if defined(PRIVILEGES) \
&& !empty(PRIVILEGES)
	${CHPRIV} -p -a ${PRIVILEGES} \
		${TDINSTFILES:S/^/${TESTDIR}\/${RELDIR}\/Files\//g}
.endif
.endif
.endif

_HELPERS_: _TDINSTALL_ ${HELPERS}
.if defined(HELPERS) \
&& !empty(HELPERS)
	${CP} ${.ALLSRC:N_TDINSTALL_} ${TDTRDIR}/Files
.endif

_TDSUBSUITES_: _TDINSTALL_ ${TDSUBSUITES}
.if defined(TDSUBSUITES) \
&& !empty(TDSUBSUITES)
	@${MAKEPATH} ${TDTRDIR}/Subsuites
	${CP} -h ${.ALLSRC:N_TDINSTALL_} ${TDTRDIR}/Subsuites
.endif

.if !defined(TESTDIR) || !defined(RELDIR) || !defined(RULE) \
|| empty(TESTDIR) || empty(RELDIR) || empty(RULE)

_TDINSTALL_:

.else

.if (${RULE} == "cmd" || ${RULE} == "start")

_TDINSTALL_: ${TCIN} ${TDOUTFILES}
	${RM} -rf ${TDTRDIR}
	@${MAKEPATH} ${TDTRDIR}
	@echo "tdinit, mkrules, edit rules, tdmod"
	@${TESTBIN}/tdinit ${TDTRDIR}
	@${TESTBIN}/mkrules -r ${RULE} ${TDTRDIR}
	@${SED} \
		-e "s,ACLCMD,/tcb/bin/lsacl," \
		-e "s,ILEVELCMD,/tcb/bin/lsilevel," \
		-e "s,SLEVELCMD,/tcb/bin/lslevel," \
		-e "s,PRIVCMD,/tcb/bin/lspriv," \
		-e "s,LSCMD,/bin/ls," \
		-e "s,EPA,/tcb/bin/epa," \
		-e "s,TTYSIM,${TESTBIN}/ttysim," \
		-e "s,EPA,/tcb/bin/epa," \
		-e "s,ACLOPTS,$(ACLOPTS)," \
		-e "s,SLEVELOPTS,$(SLEVELOPTS)," \
		-e "s,PRIVOPTS,$(PRIVOPTS)," \
		-e "s,LSOPTS,$(LSOPTS)," \
		-e "s,OBJENV,$(OBJENV)," \
		-e "s,ILEVELOPTS,$(ILEVELOPTS)," \
		-e "s/TSARGS/$(TSARGS)/" \
		-e "s,CMD,${MODULELOC}," \
		-e "s,VER,${VER}," \
		${TDTRDIR}/Rules > /tmp/sedrule$$$$ ; \
	mv /tmp/sedrule$$$$ ${TDTRDIR}/Rules ;
	@for tcase in ${TCIN:@.TDEDIT_P.@${_TDEDIT_P_}@} ; do \
		(sed -e "s@SECPRODUCT@${SECPRODUCT}@" $$tcase | \
		tdmod `${GENPATH} -I.` ${TDTRDIR} ); \
	done; 

.elif (${RULE} == "stm")

_TDINSTALL_: ${TDOUTFILES}
	${RM} -rf ${TDTRDIR}
	@${MAKEPATH} ${TDTRDIR}
	@echo "tdinit, mkrules, edit rules, tdmod"
	@${TESTBIN}/tdinit ${TDTRDIR}
	@${TESTBIN}/mkrules -r ${RULE} ${TDTRDIR}
	@${SED} \
		-e "s^SOURCE_LOCATION^${.ALLSRC:H}^" \
		-e "s^CASENAME^${TESTCASE}^" \
		-e "s^MODNAME^${MODULE}^" \
		-e "s^HELPERS^${HELPERS}^" \
		-e "s^PURPOSE^standard functionality^" \
        ${STMDEST}/tdmod.in | ${TESTBIN}/tdmod ${TDTRDIR}
.endif

.endif

#
# end of osf.sectest.mk
