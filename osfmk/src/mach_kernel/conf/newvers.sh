#!/bin/sh -
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
#
# Mach Operating System
# Copyright (c) 1991,1990,1989 Carnegie Mellon University
# All Rights Reserved.
# 
# Permission to use, copy, modify and distribute this software and its
# documentation is hereby granted, provided that both the copyright
# notice and this permission notice appear in all copies of the
# software, derivative works or modified versions, and any portions
# thereof, and that both notices appear in supporting documentation.
# 
# CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
# CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
# ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
# 
# Carnegie Mellon requests users of this software to return to
# 
#  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
#  School of Computer Science
#  Carnegie Mellon University
#  Pittsburgh PA 15213-3890
# 
# any improvements or extensions that they make and grant Carnegie Mellon
# the rights to redistribute these changes.
#

#
# newvers.sh	major minor variant edit (patch-ignored) osf_cr cmu_cr
#
# copyright (formerly arg $1) was split into last two args (now uses two files)
#

major="$1"; minor="$2"; variant="$3"; edit="$4"; patch="$5"; osf_cr="$6"; cmu_cr="$7"

# use the BUILD env variable rather than $6 for build id
build=${BUILD:+$BUILD}
build=${build:-$patch}
vers=${variant}${edit}${build}

v="VERSION(${vers})" d=`pwd` h=`hostname` t=`date`
u=${USER-"nobody"} s=${SANDBOX-""}
if [ -z "$d" -o -z "$h" -o -z "$t" ]; then
    exit 1
fi

CONFIG=`expr "$d" : '.*/\([^/]*\)$'`
d=`expr "$d" : '.*/\([^/]*/[^/]*\)$'`
(
  /bin/echo "int  version_major      = ${major};" ;
  /bin/echo "int  version_minor      = ${minor};" ;
  /bin/echo "const char version_variant[]  = \"${variant}\";" ;
  /bin/echo "const char version_patch[]    = \"${build}\";" ;
  /bin/echo "int  version_edit       = ${edit};" ;
  /bin/echo "const char version[] = \"Mach 3.0 ${v}: ${u} <${s}>; ${t}; $d ($h)" | sed -e 's?$?\\n\";?' ;
  /bin/echo "const char osf_copyright[] = \"\\" ;
  sed <$osf_cr -e '/^#/d' -e 's/"/\\"/g' -e 's;[ 	]*$;;' -e '/^$/d' -e 's;$;\\n\\;' ;
  /bin/echo "\";" ;
  /bin/echo "const char cmu_copyright[] = \"\\" ;
  sed <$cmu_cr -e '/^#/d' -e 's/"/\\"/g' -e 's;[ 	]*$;;' -e '/^$/d' -e 's;$;\\n\\;' ;
  /bin/echo "\";" ;
) > vers.c
#
exit 0
