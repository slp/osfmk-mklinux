#!/bin/csh -f
#
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
# MkLinux
#
# 	Remove Rose object header from file
#
#	Skip first 4Kb, and don't copy more than
#	28 512-byte blocks (14 Kb). Only blocks 
#	1 through 28 are available for boot code
#	on the hard disk (/dev/hd0c). You wouldn't
#	want to wipe out the VTOC, would you?
#
# 	To make sure text is small enough to fit in 
#	boot track, use:
#       	odump -vL boot | grep ^\.text | awk '{print $3}'
#	or
#		size -x boot | egrep = | awk '{print $1}'	
#
#
#	For debugging or large boot binaries, use the whole:
#		dd if=$1 of=$2 bs=4k skip=1 count=6
#
# Show size of text
#
set size = `odump -vL $1 | grep ^\.text | awk '{print $3}' `
echo text size is $size 
echo text size should not exceed 0x3800
#
# Remove header
#
dd if=$1 of=$2 bs=512 skip=8 count=28
