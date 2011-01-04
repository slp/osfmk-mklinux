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
#
# MkLinux

PURE_MACH=1
MAKEFLAGS += PURE_MACH=1

# Only embed default configs via DEFAULT_CONFIG.
# Don't embed specific configs with MACH_KERNEL_${TARGET_MACHINE}_CONFIG
# and MACH_KERNEL_CONFIG, as these should optionally be set in environment.

# {DEFAULT_}CONFIG defs also in mach_kernel/conf/Makefile; reflect 
# changes there as well  

###############################################################################
###############################################################################
###                                                                         ###
###       NOTE:  the default kernel configuration should also match         ###
### src/mach_services/cmds/mach_perf/standalone/{TARGET_MACHINE}/machdep.mk ###
###                                                                         ###
###############################################################################
###############################################################################
DEFAULT_CONFIG=PRODUCTION
.if ${TARGET_MACHINE} == "INTEL860"
DEFAULT_CONFIG=FAST+PARAGON+NIC_A
.endif

CONFIG=${MACH_KERNEL_${TARGET_MACHINE}_CONFIG:U${MACH_KERNEL_CONFIG:U${DEFAULT_CONFIG}}}

SETUP_SUBDIRS	= mach ddb

EXPINC_SUBDIRS	= mach mach_debug device ddb profiling ${MACHINE} include
COMP_SUBDIRS	= conf ${CONFIG}

SUBDIRS		= ${EXPINC_SUBDIRS} ${COMP_SUBDIRS}

.include <${RULES_MK}>
