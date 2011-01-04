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
# Revision 1.1.37.2  1996/04/01  15:11:38  yp
# 	Added SA_LIBMACH and SA_LDFLAGS.
# 	[96/04/01            yp]
# 
# Revision 1.1.37.1  1996/03/27  13:23:26  yp
# 	Moved misc.c, random.c, prof.c to lib.
# 	[96/03/20            yp]
# 
# 	Factorized machine-dependent machdep.mk.
# 	[96/03/11            yp]
# 
# 	Added semaphore and lock set files.
# 	Added latency test.
# 	[96/02/19            yp]
# 
# Revision 1.1.17.8  1996/01/25  11:32:59  bernadat
# 	Added -I${EXPORTBASE}/mach_perf
# 	Added norma.o as standard.
# 	[95/12/26            bernadat]
# 
# 	Added port/port_notify.o
# 	[95/12/22            bernadat]
# 
# Revision 1.1.17.7  1995/04/15  19:12:24  barbou
# 	Added clock and host into INCFLAGS list for dependencies' sake.
# 	[95/04/13            barbou]
# 
# Revision 1.1.17.6  1995/04/12  12:27:12  madhu
# 	Revision 1.1.24.2  1995/04/12  07:58:10  madhu
# 		Revision 1.1.6.9  1994/12/09  14:03:02  madhu
# 			changes by Nick for synthetic tests.
# 		       [1994/12/09  13:46:15  madhu]
# 		       Unix version
# 		 Revision 1.1.21.4  94/11/24  16:37:53  stephen
# 		       added simple cli interface for procs used for synthetic
# 			benchmarking, and a configuration file allowing defaults
# 			to be inbuild
# 		 Revision 1.1.21.3  1994/11/18  10:16:04  stephen
# 		       intermediate initial version with synthetic benchmarking
# 			but no CLI
# 		 Revision 1.1.6.8  1994/09/21  08:23:01  bernadat
# 		       Remode device.o Added notify.o
# 		       Added clock_perf.o, cons.o, host_perf.o
# 		       [94/09/14            bernadat]
# 	[1995/04/12  11:37:42  madhu]
# 
# Revision 1.1.17.5  1995/04/07  19:13:27  barbou
# 	Moved CFLAGS and LDFLAGS to machine-dependent machdep.mk.
# 	[95/03/28            barbou]
# 
# Revision 1.1.17.4  1995/01/10  08:08:16  devrcs
# 	mk6 CR801 - copyright marker not _FREE
# 	[1994/12/01  21:35:17  dwm]
# 
# Revision 1.1.17.2  1994/09/10  21:51:29  bolinger
# 	Merge up to NMK17.3
# 	[1994/09/08  19:44:50  bolinger]
# 
# Revision 1.1.17.1  1994/06/18  18:45:09  bolinger
# 	Import NMK17.2 sources into MK6.
# 	[1994/06/18  18:35:49  bolinger]
# 
# Revision 1.1.6.7  1994/07/06  07:55:00  bernadat
# 	Added device.o
# 	[94/07/04            bernadat]
# 
# Revision 1.1.6.6  1994/06/21  17:01:50  barbou
# 	Added dependencies.
# 	Fixed INCFLAGS.
# 	Added device_perf.o.
# 	[94/06/09            barbou]
# 
# Revision 1.1.6.5  1994/06/01  13:43:44  barbou
# 	Added dependency on libperf.a.
# 	[94/06/01            barbou]
# 
# Revision 1.1.6.4  1994/04/11  08:06:54  bernadat
# 	Moved exc/trap.o hw/lock.o hw/locore.o and vm/page.o
# 	to machinde dependent include.
# 	[94/04/08            bernadat]
# 
# Revision 1.1.6.3  1994/03/09  14:35:57  bernadat
# 	Added pager test.
# 	[94/03/09            bernadat]
# 
# Revision 1.1.6.2  1994/03/01  13:27:03  bernadat
# 	Replaced ipc_Server.o with ipc_server.o
# 	Added exc_server_main.o
# 	[94/03/01            bernadat]
# 
# Revision 1.1.6.1  1994/02/21  09:23:54  bernadat
# 	Removed mach_port_allocate.o
# 	Removed unused -e options.
# 	[94/02/21            bernadat]
# 
# 	Removed cthreads.
# 	Renamed mach_sa to sa_mach.
# 	[94/02/18            bernadat]
# 
# 	Checked in NMK16_1 revision
# 	[94/02/11            bernadat]
# 
# Revision 1.1.2.4  1993/09/29  10:23:37  bernadat
# 	Moved doprnt.o to lib/
# 	[93/09/29            bernadat]
# 
# 	Put -lcthreads before -lmach_sa. Otherwise we get
# 	wrong versions of calls such as mig_get_reply_port()
# 	[93/09/29            bernadat]
# 
# Revision 1.1.2.3  1993/09/28  10:13:37  bernadat
# 	Replace ipc_server.c with ipc_Server.c
# 	[93/09/28            bernadat]
# 
# Revision 1.1.2.2  1993/09/21  06:19:45  bernadat
# 	Initial Revision
# 	[93/09/20            bernadat]
# 

VPATH 			= ../clock:../device:../exc:../host:../hw:../ipc:../pager:../port:../sched:../realtime:../rpc:../task:../thread:../vm:${TARGET_MACHINE}

PROGRAMS		= mach_perf

ILIST			= mach_perf
IDIR			= ${MACH3_ROOT_SERVERS_IDIR}
CFLAGS			+= -DMONITOR ${${TARGET_MACHINE}_CFLAGS}
USE_STATIC_LIBRARIES	=
DEPENDENCIES		= YES

INCFLAGS                = -I../include -I../clock -I../device -I../exc \
			  -I../host -I../hw -I../ipc -I../pager -I../port \
			  -I../sched -I../realtime -I../rpc -I../task \
			  -I../thread -I../vm ${INCDIRS} \
			  -I${EXPORTBASE}/mach_services/cmds/mach_perf
EXTRA_LIBDIRS		= -L../lib
LIBS			=  -lperf ${SA_LIBMACH} ${${TARGET_MACHINE}_LIBS}

LDFLAGS			+= ${${TARGET_MACHINE}_LDFLAGS} ${SA_LDFLAGS}

MIG_DEFS                = bg.defs notify.defs

OFILES			= mach_perf.o \
                          clock_perf.o \
                          cons.o \
			  device_perf.o \
		          exc_perf.o \
			  exc_server_main.o \
		          host_perf.o \
		          hw_perf.o \
			  ipc_perf.o \
			  ipc_server_main.o \
			  ../ipc/ipc_server.o \
			  ../ipc/ipc_user.o \
			  latency_perf.o \
			  norma.o \
		          notify.o \
			  pager_perf.o \
			  pager_server_main.o \
			  ../pager/pager_server.o \
			  ../pager/pager_user.o \
		          port_perf.o \
		          ../port/port_notify.o \
			  rtsync_perf.o \
			  sched_perf.o \
		          task_perf.o \
		          thread_perf.o \
			  vm_perf.o \
                          bg_server.o \
                          bg_user.o \
                          cli.o \
                          cli_init.o \
                          synthetic.o \
			  ${${TARGET_MACHINE}_OFILES}

${PROGRAMS}: ../lib/libperf.a

notify.c: notify.defs
	${_MIG_} ${_MIGFLAGS_} ${.ALLSRC} -server notify.c \
	-user /dev/null -header /dev/null

bg_server.c bg_user.c: bg.defs
	${_MIG_} ${_MIGFLAGS_} ${.ALLSRC} -server bg_server.c \
	-user bg_user.c -header /dev/null

# cli_init.h is constructed from a user file specifying synthetic proc
# definitions to be included as default. We cheat here, the order
# of cli_init.awk and cli_init.conf is important, the first on the
# line becomes the argument to the -f parameter of awk.

cli_init.c : cli_init.awk cli_init.conf
	awk -f $> > $@

.if exists(${TARGET_MACHINE}/machdep.mk)
.include "${TARGET_MACHINE}/machdep.mk"
.endif

.include "../${TARGET_MACHINE}/machdep.mk"

.include <${RULES_MK}>

.if exists(depend.mk)
.include "depend.mk"
.endif


