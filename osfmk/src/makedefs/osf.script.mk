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

.if !defined(_OSF_SCRIPT_MK_)
_OSF_SCRIPT_MK_=

#
#  Definitions for rules using sed
#
_N_A_S_F_=THIS IS NOT A SOURCE FILE - DO NOT EDIT

#
#  Default single suffix compilation rules
#
.csh:
	${SED} -e '1s;^#!;&;' -e t\
	       -e 's;^#\(.*\)\@SOURCEWARNING\@;\1${_N_A_S_F_};' -e t\
	       ${${.TARGET}_SED_OPTIONS:U${SED_OPTIONS}}\
	 ${.IMPSRC} > ${.TARGET}.X
	${CHMOD} +x ${.TARGET}.X
	${MV} -f ${.TARGET}.X ${.TARGET}

.sh:
	${SED} -e '1s;^#!;&;' -e t\
	       -e 's;^#\(.*\)\@SOURCEWARNING\@;\1${_N_A_S_F_};' -e t\
	       ${${.TARGET}_SED_OPTIONS:U${SED_OPTIONS}}\
	 ${.IMPSRC} > ${.TARGET}.X
	${CHMOD} +x ${.TARGET}.X
	${MV} -f ${.TARGET}.X ${.TARGET}

.endif
