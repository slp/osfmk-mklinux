#!/sbin/sh
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
#

#
# 	Copy boot image to hard disk.
# 	We only have the first 28 blocks on the at386 hard disk.
#

#
# 	User can override default device
#
device=hd0c
if [ $# = 1 ]; then
	device=$1
fi

#
#       Make sure we're not on a Sequent
#
if [ -f "/sbin/model" ]; then
        if [ `/sbin/model` = "SQT" ]; then
                echo "install_boot: no need to install for SQT"
                exit 0
        fi
fi

#
# 	Make sure everything exists
#
if [ ! -f "/stand/boot.hd" ]; then
	echo "install_boot: /stand/boot.hd missing, exiting"
	exit 1
fi

if [ ! -b "/dev/$device" ]; then
	echo install_boot: /dev/$device missing, exiting
	exit 1
fi

echo Installing bootstrap binary onto $device

#
# 	Don't install if the current version is up to date
#	If cmp is not available, just install the new boot code.
#
if [ -x /usr/bin/cmp ]; then
	dd if=/dev/$device of=/boot.hd bs=512 count=28
	if [ ! "$?" = "0" ]; then
        	echo "install_boot: dd failed, exiting"
        	exit 1
	fi

	/usr/bin/cmp /boot.hd /stand/boot.hd >/dev/null 2>&1 
	if [ ! "$?" = "0" ]; then
		dd if=/stand/boot.hd of=/dev/$device bs=512 count=28
		echo "bootrack installed"
	else
		echo "bootrack up to date"
	fi
else
	dd if=/stand/boot.hd of=/dev/$device bs=512 count=28
	echo "bootrack installed"
fi


#
#	Clean up temporary files
#
if [ -f "/boot.hd" ]; then
	rm -f /boot.hd
fi


