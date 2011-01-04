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

.if !defined(_OSF_DOC_MK_)
_OSF_DOC_MK_=

#
# default nroff program to run
#
NROFF?=nroff
COL?=col
EPS?=eps
TBL?=tbl
TROFF?=troff

#
# default flags to roff
#
DEF_NROFFFLAGS?=${MANSECTION:D-man -h:U-mm}
DEF_TROFFFLAGS?=-mm -Tps
DEF_COLFLAGS?=-b

#
# all flags for roff
#
_NROFFFLAGS_=${${.TARGET}_DEF_NROFFFLAGS:U${DEF_NROFFFLAGS}} ${${.TARGET}_NROFFENV:U${NROFFENV}} ${${.TARGET}_NROFFFLAGS:U${NROFFFLAGS}} ${${.TARGET}_NROFFARGS:U${NROFFARGS}}
_TROFFFLAGS_=${${.TARGET}_DEF_TROFFFLAGS:U${DEF_TROFFFLAGS}} ${${.TARGET}_TROFFENV:U${TROFFENV}} ${${.TARGET}_TROFFFLAGS:U${TROFFFLAGS}} ${${.TARGET}_TROFFARGS:U${TROFFARGS}}
_COLFLAGS_=${${.TARGET}_DEF_COLFLAGS:U${DEF_COLFLAGS}} ${${.TARGET}_COLENV:U${COLENV}} ${${.TARGET}_COLFLAGS:U${COLFLAGS}} ${${.TARGET}_COLARGS:U${COLARGS}}
_TBLFLAGS_=${${.TARGET}_DEF_TBLFLAGS:U${DEF_TBLFLAGS}} ${${.TARGET}_TBLENV:U${TBLENV}} ${${.TARGET}_TBLFLAGS:U${TBLFLAGS}} ${${.TARGET}_TBLARGS:U${TBLARGS}}
_EPSFLAGS_=${${.TARGET}_DEF_EPSFLAGS:U${DEF_EPSFLAGS}} ${${.TARGET}_EPSENV:U${EPSENV}} ${${.TARGET}_EPSFLAGS:U${EPSFLAGS}} ${${.TARGET}_EPSARGS:U${EPSARGS}}

#
#  Default single suffix compilation rules
#
.if defined(MANSECTION)
.SUFFIXES: .out .0 .ps .doc .${MANSECTION}
.else
.SUFFIXES: .out .ps .doc
.endif

#
#  Default double suffix compilation rules
#
.doc.ps:
	${TBL} ${_TBLFLAGS_} ${.IMPSRC} | ${TROFF} ${_TROFFFLAGS_} - | ${EPS} ${_EPSFLAGS_} > ${.TARGET}.X
	${MV} ${.TARGET}.X ${.TARGET}

.doc.out:
	${TBL} ${_TBLFLAGS_} ${.IMPSRC} | ${NROFF} ${_NROFFFLAGS_} - | ${COL} ${_COLFLAGS_} > ${.TARGET}.X
	${MV} ${.TARGET}.X ${.TARGET}

.if defined(MANSECTION)
.${MANSECTION}.0:
	${NROFF} ${_NROFFFLAGS_} ${.IMPSRC} > ${.TARGET}.X
	${MV} -f ${.TARGET}.X ${.TARGET}
.endif

#
# Build Rules
#
.if defined(DOCUMENTS)
${DOCUMENTS}: $${$${.TARGET}_DOCFILES:U$${.TARGET}.doc}
.endif

.endif
