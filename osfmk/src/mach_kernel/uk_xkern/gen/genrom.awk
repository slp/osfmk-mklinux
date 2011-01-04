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
# x-kernel v3.2
#
# Copyright (c) 1993,1991,1990  Arizona Board of Regents
#
# $Revision: 1.1.8.1 $
# $Date: 1995/02/23 17:58:27 $
#

#
# generates an 'initRom' routine from an existing ROM file which
# initializes the 'rom' array as if the ROM file had been read at
# runtime.  This is mainly for in-kernel use.


BEGIN {
	fmtString = "\txkr = addRomOpt(\"%s\");\n"			\
		    "\tif (xkr != XK_SUCCESS) return XK_FAILURE;\n"	

	printf("\n#include <xkern/include/romopt.h>\n");
	printf("\nxkern_return_t\ninitRom()\n{\n");
	printf("\txkern_return_t\txkr;\n\n");
}

{ 
    if ( substr( $0, 1, 1 ) == "#" ) {
	next;
    }
    printf(fmtString, $0);
}

END {
	printf("\treturn XK_SUCCESS;\n}\n");
}
