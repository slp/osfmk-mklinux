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

EXPLIB_SUBDIRS          = lib

.if defined(PURE_MACH)

SETUP_SUBDIRS           = lib
EXPINC_SUBDIRS          = include servers
EXPLIB_SUBDIRS		= lib
COMP_SUBDIRS		= servers
SUBDIRS                 = include lib servers

.else

SETUP_SUBDIRS           = cmds lib
EXPINC_SUBDIRS          = include servers cmds
EXPLIB_SUBDIRS		= lib
COMP_SUBDIRS		= servers cmds
SUBDIRS                 = include lib servers cmds

.endif

.include <${RULES_MK}>
