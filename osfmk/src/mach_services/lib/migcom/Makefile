# Copyright (c) 1995, 1994, 1993, 1992, 1991, 1990  
# Open Software Foundation, Inc. 
#  
# Permission to use, copy, modify, and distribute this software and 
# its documentation for any purpose and without fee is hereby granted, 
# provided that the above copyright notice appears in all copies and 
# that both the copyright notice and this permission notice appear in 
# supporting documentation, and that the name of ("OSF") or Open Software 
# Foundation not be used in advertising or publicity pertaining to 
# distribution of the software without specific, written prior permission. 
#  
# OSF DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
# INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
# FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL OSF BE LIABLE FOR ANY 
# SPECIAL, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES 
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN 
# ACTION OF CONTRACT, NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING 
# OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE 
# 
# OSF Research Institute MK6.1 (unencumbered) 1/31/1995

SETUP_PROGRAMS		= migcom

PROGRAMS		= migcom

INCFLAGS		= -I.
ILIST			= migcom
IDIR			= ${MACH3_LIBRARY_IDIR}

DEPENDENCIES		=
CFLAGS			= -D_ANSI_C_SOURCE
error.o_CFLAGS		= ${CFLAGS} ${NO_VFPRINTF:D-DNO_VFPRINTF}
ident.c_OFILES		= parser.o lexxer.o mig.o \
			  error.o string.o type.o routine.o \
			  statement.o global.o \
			  header.o user.o server.o utils.o
OFILES			= ${ident.c_OFILES} ident.o
LIBS			= -ll
GARBAGE			= parser.c parser.h

.include <${RULES_MK}>

# force generation of ident.c whenever a .o is modified
ident.c: ${ident.c_OFILES}
	@echo "char *MigGenerationDate = \""`date`"\";" >ident.c
	@echo "char *MigMoreData = \""`whoami`"@"`hostname`"\";" >>ident.c

# force generation of intermediate .c files
lexxer.o: lexxer.c parser.h
parser.o: parser.c

# cause the 2 yacc passes to be serialized.
.ORDER:		parser.c parser.h

.if exists(depend.mk)
.include "depend.mk"
.endif
