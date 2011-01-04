#
# Makefile for the linux HFS filesystem routines
#
# Copyright 1997 by Paul H. Hargrove
# This file may be distributed under the terms of the GPL.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
# Note 2! The CFLAGS definitions are now in the main makefile...

# B-tree sources and headers:
BSRCS   = balloc.c    bdelete.c   bfind.c     bins_del.c  \
          binsert.c   bnode.c     brec.c      btree.c

# HFS sources and headers:
HSRCS   = bitmap.c    bitops.c    catalog.c   extent.c    \
          mdb.c       part_tbl.c  string.c

# Linux sources and headers:
LSRCS   = dir.c       dir_cap.c   dir_dbl.c   dir_nat.c   \
          file.c      file_cap.c  file_hdr.c  inode.c     \
          super.c     sysdep.c    trans.c     version.c

SRCS    = $(BSRCS) $(HSRCS) $(LSRCS)

O_TARGET := hfs.o
O_OBJS   := $(SRCS:.c=.o)
M_OBJS   := $(O_TARGET)

include $(TOPDIR)/Rules.make
