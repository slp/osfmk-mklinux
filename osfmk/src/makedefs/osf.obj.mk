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

.if !defined(_OSF_OBJ_MK_)
_OSF_OBJ_MK_=

#
# definitions for compilation
#
_ALL_OFILES_=${OFILES:U${PROGRAMS:@.PROG.@${${.PROG.}_OFILES:U${.PROG.}.o}@}}
_OFILES_=${OFILES:U${${.TARGET}_OFILES:U${.TARGET}.o}}

#
#  Default double suffix compilation rules
#

.c.o:
	${_CC_} -c ${_CCFLAGS_} ${.IMPSRC}

.C.o:
	${_CPLUSPLUS_} -c ${_CPLUSPLUSFLAGS_} ${.IMPSRC}

.cc.o:
	${_CPLUSPLUS_} -c ${_CPLUSPLUSFLAGS_} ${.IMPSRC}

# rename the yacc map file if using vista
.y.o:
	${_YACC_} ${_YFLAGS_} ${.IMPSRC}
	${_CC_} -c ${_CCFLAGS_} y.tab.c
	-${RM} -f y.tab.c
.if defined(VISTA)
	${MV} -f y.tab.M ${.TARGET:.o=.M}
.endif
	${MV} -f y.tab.o ${.TARGET}

.y.c:
	${_YACC_} ${_YFLAGS_} ${.IMPSRC}
	${MV} -f y.tab.c ${.TARGET}
	${RM} -f y.tab.h

.if	!defined(NO_Y_H_RULE)
.y.h:
	${_YACC_} -d ${_YFLAGS_} ${.IMPSRC}
	${MV} -f y.tab.h ${.TARGET}
	${RM} -f y.tab.c
.endif

# rename the lex map file if using vista
.l.o:
	${_LEX_} ${_LFLAGS_} ${.IMPSRC}
	${_CC_} -c ${_CCFLAGS_} lex.yy.c
	-${RM} -f lex.yy.c
.if defined(VISTA)
	${MV} -f lex.yy.M ${.TARGET:.o=.M}
.endif
	${MV} -f lex.yy.o ${.TARGET}

.l.c:
	${_LEX_} ${_LFLAGS_} ${.IMPSRC}
	${MV} -f lex.yy.c ${.TARGET}

.c.pp:
	${_CC_} -E ${_CCFLAGS_} ${.IMPSRC} > ${.TARGET}

.if defined(OFILES) || defined(PROGRAMS)
${_ALL_OFILES_}: ${HFILES}
.endif

.endif

