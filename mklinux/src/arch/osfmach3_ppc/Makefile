# Copyright (c) 1991-1998 Open Software Foundation, Inc. 
#  
# 
# 
# MkLinux

#
# osfmach3_ppc/Makefile
#
# This file is included by the global makefile so that you can add your own
# architecture-specific flags and dependencies. Remember to do have actions
# for "archclean" and "archdep" for cleaning up and making dependencies for
# this architecture
#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
#
# Copyright (C) 1994 by Linus Torvalds
# Changes for PPC by Gary Thomas
#

# PowerPC (cross) tools
#AS		= as.ppc
#ASFLAGS		=
#LD		= ld.ppc
#LINKFLAGS	= -T arch/ppc/ld.script -Ttext 0x90000000 -Map vmlinux.map
#LINKFLAGS	= -T arch/ppc/ld.script -Ttext 0x90000000
HOSTCC		= /usr/bin/gcc
#CC		= cc.ppc
#CFLAGS		= -D__KERNEL__ -I$(TOPDIR)/include \
#		-Wall -Wstrict-prototypes \
#		-msoft-float \
#		-fomit-frame-pointer \
#		-fno-builtin \
#		-finhibit-size-directive \
#		-fsigned-char \
#		-O2
#CPP		= $(CC) -E $(CFLAGS)
#AR		= ar.ppc
#RANLIB		= ranlib.ppc
#STRIP		= strip.ppc
#NM		= nm.ppc

CROSS_COMPILE	=
CFLAGS		+= -nostdinc \
		-fno-builtin \
		-fsigned-char \
		-D__powerpc__ \
		-mcpu=604
LINKFLAGS	:= -T arch/$(ARCH)/ld.script -Ttext 0x10000000 -Tdata 0x10200000 -u __start_mach -e __start_mach
MIG		= mig
MIGCOM		= migcom
LIBS		:= $(LIBS) $(TOPDIR)/export-osfmach3/$(ARCH)/lib/libcthreads.a $(TOPDIR)/export-osfmach3/$(ARCH)/lib/libmach_maxonstack.a $(TOPDIR)/export-osfmach3/$(ARCH)/lib/libsa_mach.a
CFLAGS		:= $(CFLAGS) -Werror
ifdef	CONFIG_OSFMACH3_DEBUG
CFLAGS		:= $(CFLAGS) -fno-omit-frame-pointer
endif

#
# Set these to indicate how to link it..
#
# -zmagic:   
#
# ZLINKFLAGS   = -Ttext 0x1000
# LINKFLAGS    = -Ttext 0x100000
#
# -qmagic (we need to remove the 32 byte header for bootup purposes)
#
## ZLINKFLAGS =-qmagic -Ttext 0xfe0
## LINKFLAGS =-qmagic -Ttext 0xfffe0
## CFLAGS := $(CFLAGS) -pipe

## ifdef CONFIG_M486
## CFLAGS := $(CFLAGS) -m486
## else
## ifdef CONFIG_M586
## CFLAGS := $(CFLAGS) -mpentium
## else
## CFLAGS := $(CFLAGS) -m386
## endif
## endif

HEAD := arch/$(ARCH)/kernel/head.o

ARCH_SUBDIRS = arch/$(ARCH)/blk_dev arch/$(ARCH)/chr_dev arch/$(ARCH)/kernel arch/$(ARCH)/mm arch/$(ARCH)/server arch/$(ARCH)/lib
SUBDIRS := $(SUBDIRS) $(ARCH_SUBDIRS)
ARCHIVES := arch/$(ARCH)/blk_dev/blk_dev.o arch/$(ARCH)/chr_dev/chr_dev.o arch/$(ARCH)/kernel/kernel.o arch/$(ARCH)/mm/mm.o arch/$(ARCH)/server/server.o arch/$(ARCH)/lib/lib.o $(ARCHIVES)
#NO_RD_ARCHIVES := arch/$(ARCH)/kernel/no_ramdisk.o $(ARCHIVES)
#ARCHIVES := arch/$(ARCH)/kernel/ramdisk.o arch/$(ARCH)/kernel/root_sd.o $(ARCHIVES)

arch/$(ARCH)/blk_dev: dummy
	$(MAKE) linuxsubdirs SUBDIRS=arch/$(ARCH)/blk_dev

arch/$(ARCH)/chr_dev: dummy
	$(MAKE) linuxsubdirs SUBDIRS=arch/$(ARCH)/chr_dev

arch/$(ARCH)/kernel: dummy
	$(MAKE) linuxsubdirs SUBDIRS=arch/$(ARCH)/kernel

arch/$(ARCH)/mm: dummy
	$(MAKE) linuxsubdirs SUBDIRS=arch/$(ARCH)/mm

arch/$(ARCH)/server: dummy
	$(MAKE) linuxsubdirs SUBDIRS=arch/$(ARCH)/server

arch/$(ARCH)/lib: dummy
	$(MAKE) linuxsubdirs SUBDIRS=arch/$(ARCH)/lib

MAKEBOOT = $(MAKE) -C arch/$(ARCH)/boot

vmlinux.no_ramdisk: $(CONFIGURATION) init/main.o init/version.o linuxsubdirs arch/ppc/kernel/root_sd.o
	$(LD) $(LINKFLAGS) $(HEAD) init/main.o init/version.o \
		arch/ppc/kernel/root_sd.o \
		$(NO_RD_ARCHIVES) \
		$(FILESYSTEMS) \
		$(DRIVERS) \
		$(LIBS) -o vmlinux.no_ramdisk

vmlinux-hd.no_ramdisk: $(CONFIGURATION) init/main.o init/version.o linuxsubdirs arch/ppc/kernel/root_hd.o
	$(LD) $(LINKFLAGS) $(HEAD) init/main.o init/version.o \
		arch/ppc/kernel/root_hd.o \
		$(NO_RD_ARCHIVES) \
		$(FILESYSTEMS) \
		$(DRIVERS) \
		$(LIBS) -o vmlinux-hd.no_ramdisk

## 
#zImage: vmlinux
#	@$(MAKEBOOT) zImage

#tImage: vmlinux
#	@$(MAKEBOOT) tImage

#bImage: vmlinux.no_ramdisk
#	@$(MAKEBOOT) bImage

#bdisk: vmlinux.no_ramdisk
#	@$(MAKEBOOT) bdisk

#bdisk-hd: vmlinux-hd.no_ramdisk
#	@$(MAKEBOOT) bdisk-hd

## compressed: zImage
## 
## zlilo: vmlinux
## 	@$(MAKEBOOT) zlilo
## 
#zdisk: vmlinux
#	@$(MAKEBOOT) zdisk
 
## install: vmlinux
## 	@$(MAKEBOOT) install

archclean:
	rm -f vmlinux.in_kernel
	rm -f osfmach3/server/serv_callback_user.c
	rm -f osfmach3/server/serv_callback_user.h
	rm -f osfmach3/server/serv_callback_server.c
	rm -f osfmach3/server/serv_callback_server.h
	rm -f osfmach3/server/memory_object_server.c
	rm -f osfmach3/server/memory_object_server.h
#	@$(MAKEBOOT) clean

archdep:
#	@$(MAKEBOOT) dep
