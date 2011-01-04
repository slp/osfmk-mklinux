# Copyright (c) 1991-1998 Open Software Foundation, Inc. 
#  
# 
# 
# MkLinux
#

#
# Makefile for the linux i386-specific parts of the OSF/Mach3 server.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
# Note 2! The CFLAGS definition is now in the main makefile...

ifdef SMP

.S.o:
	$(CC) -D__ASSEMBLY__ $(AFLAGS) -traditional -c $< -o $*.o
else
.S.o:
	$(CC) -D__ASSEMBLY__ -c $< -o $*.o
endif

O_TARGET := server.o
O_OBJS	 := osfmach3_setjmp.o parent_osf1_syscalls.o trap.o

include $(TOPDIR)/Rules.make
