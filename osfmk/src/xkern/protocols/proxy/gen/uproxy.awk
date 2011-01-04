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
# uproxy.awk
#
# x-kernel v3.2
#
# Copyright (c) 1993,1991,1990  Arizona Board of Regents
#
#
# Revision: 1.2
# Date: 1993/04/05 00:58:44
#

# Pick off certain MIG-generate routines and write them to separate
# files, writing the rest of the source to a different file.  

call != 0 && /\/\* Routine/	{ call = 0 }
push != 0 && /\/\* Routine/	{ push = 0 }
/Routine xCall/		{ call = 1 }
/Routine xPush/		{ push = 1 }
call != 0 { print $0 >"uproxy.call.tmp"; next }
push != 0 { print $0 >"uproxy.push.tmp"; next }

{ print $0 >"uproxy.rest.tmp" }
