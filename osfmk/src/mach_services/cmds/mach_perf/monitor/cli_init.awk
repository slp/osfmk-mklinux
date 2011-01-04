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
# OLD HISTORY
# Revision 1.1.4.2  1996/01/25  11:33:04  bernadat
# 	Homogenized copyright headers
# 	[95/11/30            bernadat]
# 
# Revision 1.1.4.1  1995/04/12  12:27:35  madhu
# 	copyright marker not _FREE
# 	[1995/04/11  14:54:38  madhu]
# 
# Revision 1.1.2.3  1994/12/09  15:42:05  madhu
# 	header changes
# 	[1994/12/09  15:38:45  madhu]
# 
# 	Chamged the header
# 	[1994/12/09  15:05:59  madhu]
# 
# Revision 1.1.2.2  1994/12/09  15:16:54  madhu
# 	Chamged the header
# 
# Revision 1.1.2.1  1994/12/09  14:03:14  madhu
# 	changes by Nick for synthetic tests.
# 

# this awk script is responsible for translating cli_init.conf into
# cli_init.c, which defines an array of strings containing all
# non-comment lines (for space reasons)

BEGIN	{ QUOTE=34; printf("char *cli_init[] = {\n"); }
/^\#/	{ next; }
{ printf("%c%s%c,\n",QUOTE,$0,QUOTE); }
END	{ printf("(char*)0 };\nint cli_init_count=0;\n")}
