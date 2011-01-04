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

O_TARGET := blk_dev.o
O_OBJS   := block_dev.o gen_disk.o \
	aztcd.o cdu31a.o floppy.o hd.o ide.o mcd.o sbpcd.o \
	sony535.o xd.o \
	sd.o sg.o sr.o scsi_procfs.o cdrom.o

include $(TOPDIR)/Rules.make
