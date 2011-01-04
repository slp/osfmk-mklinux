# Copyright (c) 1991-1998 Open Software Foundation, Inc. 
#  
# 
# 
# MkLinux
#

#
# Makefile for the linux server on top of the OSF/Mach3 micro-kernel.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
# Note 2! The CFLAGS definitions are now in the main makefile...

O_TARGET := mm.o
O_OBJS   := init.o memory.o mlock.o mmap.o mprotect.o mremap.o \
	    swapfile.o vmalloc.o

include $(TOPDIR)/Rules.make
