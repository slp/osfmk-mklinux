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
# osfmach3_i386/Makefile
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
#

#
# Set these to indicate how to link it..
#
# -zmagic:   (aout, old GCC-2.5.2)
#
# ZLINKFLAGS   = -Ttext 0x1000
# BZLINKFLAGS  = -Ttext 0x100000     # (for big high loaded kernels)
# LINKFLAGS    = -Ttext 0x100000
#
# -qmagic:   (aout)
#
# ZLINKFLAGS   = -Ttext 0x0xfe0
# BZLINKFLAGS  = -Ttext 0xfffe0      # (for big high loaded kernels)
# LINKFLAGS    = -Ttext 0xfffe0
#

AS86    =$(CROSS_COMPILE)as86 -0 -a
AS386   =$(CROSS_COMPILE)as86 -3
LD86    =$(CROSS_COMPILE)ld86 -0

ifdef CONFIG_KERNEL_ELF

LD=$(CROSS_COMPILE)ld -m elf_i386
CPP=$(CC) -E -D__ELF__
OBJDUMP=$(CROSS_COMPILE)objdump
OBJDUMP_FLAGS=-k -q 
ENCAPS=$(CROSS_COMPILE)encaps
OBJCOPY=$(CROSS_COMPILE)objcopy -O binary -R .note -R .comment -R .stab -R .stabstr
ZLDFLAGS=-e startup_32 
#LDFLAGS=-e stext
LDFLAGS=-e __start_mach -static -nostdlib
ZIMAGE_OFFSET=0x1000
IMAGE_OFFSET=0x100000
ZLINKFLAGS =-Ttext $(ZIMAGE_OFFSET) $(ZLDFLAGS)
BZLINKFLAGS =-Ttext $(IMAGE_OFFSET) $(ZLDFLAGS)
#LINKFLAGS =-Ttext $(IMAGE_OFFSET) $(LDFLAGS)
LINKFLAGS =-Ttext 0x0c000000 $(LDFLAGS)
#VMLINUX_INKERNEL = vmlinux.in_kernel
#INKERNEL_LINKFLAGS = -Ttext 0xcc000000 $(LDFLAGS)
else

#
# -qmagic (we need to remove the 32 byte header for bootup purposes)
#
ZLINKFLAGS =-qmagic -Ttext 0xfe0
BZLINKFLAGS =-qmagic -Ttext 0xfffe0
LINKFLAGS =-qmagic -Ttext 0xfffe0
endif
MIG = mig
MIGCOM = migcom
LIBS := $(LIBS) $(TOPDIR)/export-osfmach3/$(ARCH)/lib/libcthreads.a $(TOPDIR)/export-osfmach3/$(ARCH)/lib/libmach_maxonstack.a $(TOPDIR)/export-osfmach3/$(ARCH)/lib/libsa_mach.a


CFLAGS := $(CFLAGS) -Werror -fno-builtin -nostdinc
ifdef CONFIG_OSFMACH3_DEBUG
CFLAGS := $(CFLAGS) -fno-omit-frame-pointer
endif

CFLAGS := $(CFLAGS) -pipe

ifdef CONFIG_M386
CFLAGS := $(CFLAGS) -m386 -DCPU=386
endif

ifdef CONFIG_M486
CFLAGS := $(CFLAGS) -m486 -DCPU=486
endif

ifdef CONFIG_M586
CFLAGS := $(CFLAGS) -m486 -malign-loops=2 -malign-jumps=2 -malign-functions=2 -DCPU=586
endif

ifdef CONFIG_M686
CFLAGS := $(CFLAGS) -m486 -malign-loops=2 -malign-jumps=2 -malign-functions=2 -DCPU=686
endif

ifdef SMP
CFLAGS := $(CFLAGS) -D__SMP__
endif

HEAD := arch/$(ARCH)/kernel/head.o

SUBDIRS := $(SUBDIRS) arch/$(ARCH)/blk_dev arch/$(ARCH)/chr_dev arch/$(ARCH)/kernel arch/$(ARCH)/mm arch/$(ARCH)/server arch/$(ARCH)/lib
ARCHIVES := arch/$(ARCH)/blk_dev/blk_dev.o arch/$(ARCH)/chr_dev/chr_dev.o arch/$(ARCH)/kernel/kernel.o arch/$(ARCH)/mm/mm.o arch/$(ARCH)/server/server.o $(ARCHIVES)
LIBS := $(TOPDIR)/arch/$(ARCH)/lib/lib.a $(LIBS) $(TOPDIR)/arch/$(ARCH)/lib/lib.a

ifdef CONFIG_MATH_EMULATION
SUBDIRS := $(SUBDIRS) arch/$(ARCH)/math-emu
DRIVERS := $(DRIVERS) arch/$(ARCH)/math-emu/math.a
endif

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

MAKEBOOT = $(MAKE) -C arch/$(ARCH)/boot

zImage: vmlinux
	@$(MAKEBOOT) zImage

bzImage: vmlinux
	@$(MAKEBOOT) bzImage

compressed: zImage

zlilo: vmlinux
	@$(MAKEBOOT) BOOTIMAGE=zImage zlilo

bzlilo: vmlinux
	@$(MAKEBOOT) BOOTIMAGE=bzImage zlilo

zdisk: vmlinux
	@$(MAKEBOOT) BOOTIMAGE=zImage zdisk

bzdisk: vmlinux
	@$(MAKEBOOT) BOOTIMAGE=bzImage zdisk

install: vmlinux
	@$(MAKEBOOT) install

archclean:
	rm -f vmlinux.in_kernel
	rm -f osfmach3/server/serv_callback_user.c
	rm -f osfmach3/server/serv_callback_user.h
	rm -f osfmach3/server/serv_callback_server.c
	rm -f osfmach3/server/serv_callback_server.h
	rm -f osfmach3/server/memory_object_server.c
	rm -f osfmach3/server/memory_object_server.h
	@$(MAKEBOOT) clean
	$(MAKE) -C arch/$(ARCH)/kernel clean

archdep:
	@$(MAKEBOOT) dep
