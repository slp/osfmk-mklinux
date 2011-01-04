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

.if !defined(_OSF_DEPEND_MK_)
_OSF_DEPEND_MK_=

nodep_all:
	${RM} ${_RMFLAGS_} depend.mk

.if !empty(.TARGETS:Mnodep_*.o)
${TARGETS:Mnodep_*.o}:
	${RM} -f ${.TARGET:S/^nodep_//} ${.TARGET:S/^nodep_//:.o=.d}
	echo "${.TARGET:S/^nodep_//}: ${.TARGET:S/^nodep_//}" \
		> ${.TARGET:S/^nodep_//:.o=.d}
.endif

.EXIT:
	${MD} ${_MDFLAGS_} .

.endif
