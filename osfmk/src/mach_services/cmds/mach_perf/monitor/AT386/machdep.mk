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
# OLD HISTORY
# Revision 1.1.33.1  1996/03/27  13:23:11  yp
# 	Added RPC performance tests.
# 	Removed profiling files (now in lib).
# 	[96/03/27            yp]
# 
# Revision 1.1.10.11  1996/01/25  11:32:31  bernadat
# 	moved norma.o to standalone/Makefile.
# 	[95/12/26            bernadat]
# 
# 	Homogenized copyright headers
# 	[95/11/30            bernadat]
# 
# 	added mach_kernel prefix to search string to avoid multiple line
# 	result from strings | grep.
# 	[95/12/11            bernadat]
# 
# Revision 1.1.10.10  1995/11/03  17:09:17  bruel
# 	Fix and re-enable sym.c generation.
# 	[95/10/26            bernadat]
# 	[95/11/03            bruel]
# 
# Revision 1.1.10.9  1995/05/14  19:57:58  dwm
# 	ri-osc CR1304 - merge (nmk19_latest - nmk19b1) diffs into mainline.
# 	disable 386 elf nm stuff by default; still doesn't seem to work.
# 	[1995/05/14  19:47:56  dwm]
# 
# Revision 1.1.10.8  1995/04/24  08:23:35  madhu
# 	Fix and re-enable sym.c generation.
# 	[1995/04/24  08:22:54  madhu]
# 
# Revision 1.1.10.7  1995/04/07  19:13:04  barbou
# 	Added machine-dependent CFLAGS and LDFLAGS.
# 	[95/03/28            barbou]
# 
# Revision 1.1.10.6  1995/02/28  02:20:12  dwm
# 	Merged with changes from 1.1.10.5
# 	[1995/02/28  02:19:49  dwm]
# 
# 	mk6 CR1120 - Merge mk6pro_shared into cnmk_shared
# 	# Rev 1.1.20.1  1995/02/06  14:34:59  dwm
# 	# mk6 CR1025 - comment out broken/unused namelist hogwash
# 	[1995/02/28  02:15:53  dwm]
# 
# Revision 1.1.14.3  1995/01/10  07:22:27  devrcs
# 	mk6 CR801 - copyright marker not _FREE
# 	[1994/12/01  21:35:40  dwm]
# 
# 	mk6 CR668 - 1.3b26 merge
# 	Use format-independent NM macro to name nm utility.
# 	[1994/11/08  22:06:16  bolinger]
# 
# Revision 1.1.10.2  1994/09/10  21:51:25  bolinger
# 	Merge up to NMK17.3
# 	[1994/09/08  19:44:47  bolinger]
# 
# Revision 1.1.10.1  1994/06/18  18:44:49  bolinger
# 	Import NMK17.2 sources into MK6.
# 	[1994/06/18  18:35:40  bolinger]
# 
# Revision 1.1.5.6  1994/07/06  07:54:56  bernadat
# 	Surround kernel_symbols_version echo command with double quotes
# 	to prevent echo from eating white spaces.
# 	[94/07/06            bernadat]
# 
# Revision 1.1.5.5  1994/04/28  09:53:07  bernadat
# 	Removed unused prof.o_CENV.
# 	[94/04/25            bernadat]
# 
# 	Added standalone profiling.
# 	[94/04/18            bernadat]
# 
# Revision 1.1.5.4  1994/04/14  13:54:42  bernadat
# 	Added norma.o machine.o
# 	[94/04/14            bernadat]
# 
# Revision 1.1.5.3  1994/04/11  08:06:47  bernadat
# 	Moved machine dependent module from ../Makefile
# 	to here.
# 	[94/03/30            bernadat]
# 
# Revision 1.1.5.2  1994/02/22  12:41:45  bernadat
# 	Added setjmp.o
# 	[94/02/21            bernadat]
# 
# Revision 1.1.5.1  1994/02/21  09:23:50  bernadat
# 	Checked in NMK16_1 revision
# 	[94/02/11            bernadat]
# 
# Revision 1.1.2.3  1993/11/02  12:33:16  bernadat
# 	Added fixdfsi.s fabs.s
# 	[93/10/22            bernadat]
# 
# Revision 1.1.2.2  1993/09/21  06:19:36  bernadat
# 	Initial Revision
# 	[93/09/20            bernadat]
# 

${TARGET_MACHINE}_VPATH		+= AT386

SFILES		= setjmp.o

${TARGET_MACHINE}_OFILES	= rpc_perf.o \
				  rpc_server_main.o \
				  ../rpc/rpc_server.o \
				  ../rpc/rpc_user.o \
				  machine.o \
				  ../exc/trap.o \
				  ../hw/lock.o \
				  ../hw/locore.o \
				  ../vm/page.o \
				  ${SFILES} \
				  ${PROF_FILES}

