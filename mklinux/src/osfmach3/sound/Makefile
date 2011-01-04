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

O_TARGET := 	sound.o
O_OBJS   := 	audio.o         dmabuf.o        midi_synth.o    patmgr.o        sound_switch.o  soundcard.o	 \
		dev_table.o     midibuf.o       sequencer.o     sound_timer.o   sys_timer.o			 \
		drivers/awacs/awacs_entry.o     drivers/awacs/awacs_ops.o       drivers/awacs/awacs_data.o

include $(TOPDIR)/Rules.make
