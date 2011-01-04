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

.if !defined(_OSF_RULES_MK_)
_OSF_RULES_MK_=

.include <osf.std.mk>

.if !defined(TOSTAGE)
.if defined(PROGRAMS)
.include <osf.prog.mk>
.endif
.if defined(LIBRARIES) || defined(SHARED_LIBRARIES)
.include <osf.lib.mk>
.endif
.if defined(BINARIES)
.include <osf.obj.mk>
.endif
.if defined(SCRIPTS)
.include <osf.script.mk>
.endif
.if defined(MANPAGES)
.include <osf.man.mk>
.endif
.if defined(DOCUMENTS)
.include <osf.doc.mk>
.endif
.if defined(DEPENDENCIES)
.include <osf.depend.mk>
.endif
.endif

.if (${.TARGETS} == "tdinst_all")
.include <osf.sectest.mk>
.endif

.endif
