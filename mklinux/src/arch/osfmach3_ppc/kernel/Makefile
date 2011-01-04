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

#
# Makefile for the linux kernel.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
# Note 2! The CFLAGS definitions are now in the main makefile...

all: kernel.o head.o

O_TARGET := kernel.o
O_OBJS   := misc.o port_io.o process.o ptrace.o setup.o stubs.o signal.o \
	    string.o support.o syscalls.o traps.o ksyms.o

head.o: head.S $(TOPDIR)/include/linux/tasks.h
	$(CC) -D__ASSEMBLY__ -traditional -c $*.S -o $*.o

ifeq ($(CONFIG_BLK_DEV_INITRD),y)
O_OBJS += ramdisk.o
endif

ramdisk.o: ramdisk.image mk_ramdisk
	./mk_ramdisk ramdisk.image $*.s
	$(AS) -o $@ $*.s
	rm $*.s

include $(TOPDIR)/Rules.make

mk_ramdisk: mk_ramdisk.c
	$(HOSTCC) -o mk_ramdisk mk_ramdisk.c
