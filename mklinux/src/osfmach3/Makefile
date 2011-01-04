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

L_TARGET := osfmach3.a
L_OBJS    = $(join $(SUB_DIRS),$(SUB_DIRS:%=/%.o))
O_TARGET := 
O_OBJS    =

MOD_LIST_NAME :=
ALL_SUB_DIRS = blk_dev chr_dev device kernel mm net_dev server
SUB_DIRS = $(ALL_SUB_DIRS)

ifeq ($(CONFIG_SOUND),y)
SUB_DIRS += sound
else
  ifeq ($(CONFIG_SOUND),m)
  MOD_SUB_DIRS += sound
  endif
endif

include $(TOPDIR)/Rules.make
