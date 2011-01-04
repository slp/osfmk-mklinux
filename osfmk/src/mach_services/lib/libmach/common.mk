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

PURE_MACH		= 1

USE_STATIC_LIBRARIES	=

VPATH                   = ${MAKETOP}mach_services/lib/libmach:${${TARGET_MACHINE}_VPATH}:${MAKETOP}mach_kernel/mach:${MAKETOP}mach_kernel/mach_debug:${MAKETOP}mach_kernel/device:${MAKETOP}default_pager/mach

LIBRARIES		= ${LOCAL_LIBMACH}

INCFLAGS                = -I${MAKETOP}mach_services/lib/libmach \
			  -I${MAKETOP}mach_services/lib/libmach/${TARGET_MACHINE}
mach_user.c_MIGFLAGS	= -DMIG_USER ${LOCAL_MIGFLAGS}
mach_debug.c_MIGFLAGS	= -DMIG_USER ${LOCAL_MIGFLAGS}
bootstrapServer.c_MIGFLAGS = -DTypeCheck=0 -DMIG_SERVER ${LOCAL_MIGFLAGS}
mach_port_user.c_MIGFLAGS = -DMIG_USER -DTypeCheck=0 ${LOCAL_MIGFLAGS}
clock_user.c_MIGFLAGS	= -DMIG_USER ${LOCAL_MIGFLAGS}
device_request_user.c_MIGFLAGS = -DMIG_USER ${LOCAL_MIGFLAGS}
device_user.c_MIGFLAGS	= -DMIG_USER ${LOCAL_MIGFLAGS}
mach_host_user.c_MIGFLAGS = -DMIG_USER ${LOCAL_MIGFLAGS}
mach_user.o_CFLAGS	= -DTypeCheck=0 ${CFLAGS}
ms_device_read_overwrite_request.o_CFLAGS = -D_BSD ${CFLAGS}
ms_device_read_overwrite.o_CFLAGS = -D_BSD ${CFLAGS}
ms_vm_read_overwrite.o_CFLAGS = -D_BSD ${CFLAGS}
DEFINES			= -DTypeCheck=0 
CFLAGS			= ${DEFINES} ${LOCAL_CFLAGS} \
			  ${${OBJECT_FORMAT}_EXTRA_WARNINGS} ${WERROR}
MIGFLAGS		= ${DEFINES} ${LOCAL_MIGFLAGS}
MIG_DEFS		= bootstrap.defs \
			  clock.defs \
			  clock_reply.defs \
			  default_pager_object.defs \
			  device.defs \
			  device_reply.defs \
			  device_request.defs \
			  exc.defs \
			  ledger.defs \
			  mach.defs \
			  mach_debug.defs \
			  mach_host.defs \
			  mach_norma.defs \
			  mach_port.defs \
			  memory_object.defs \
			  notify.defs \
			  sync.defs	

TRAPS			= clock_get_time device_read device_read_overwrite \
			  device_read_overwrite_request device_read_request \
			  device_write device_write_request \
			  host_statistics \
			  kernel_task_create mach_port_allocate \
			  mach_port_allocate_qos \
			  mach_port_allocate_name \
			  mach_port_allocate_full \
			  mach_port_allocate_subsystem mach_port_deallocate \
			  mach_port_destroy mach_port_insert_right \
			  mach_port_mod_refs \
			  mach_port_move_member mach_port_rename \
			  mach_port_request_notification \
			  memory_object_change_attributes \
			  memory_object_data_error \
			  memory_object_data_supply \
			  memory_object_data_unavailable \
			  memory_object_discard_reply \
			  memory_object_lock_request \
			  memory_object_synchronize_completed \
			  task_create task_info task_set_exception_ports \
			  task_set_special_port task_suspend task_terminate \
			  task_set_port_space \
			  thread_abort_safely \
			  thread_create thread_create_running \
			  thread_depress_abort thread_info \
			  thread_set_exception_ports thread_set_state \
			  thread_suspend thread_switch \
			  thread_terminate \
			  vm_allocate vm_behavior_set vm_deallocate \
			  vm_machine_attribute vm_map vm_msync \
			  vm_protect vm_read_overwrite vm_region vm_remap \
			  vm_wire vm_write

# Considerably complexify this, on behalf of brain-dead System V archiver
# (which imposes 14-character maximum on object filenames).  Basically,
# there should be a name in either FULLTRAPS or TRUNCTRAPS for each of
# names in TRAPS.  If a name is longer than 9 characters (14 chars
# - "MS_" - ".o"), it has to be truncated in TRUNCTRAPS.  Yum!
#
FULLTRAPS		= task_info \
			  vm_map \
			  vm_msync \
			  vm_region \
			  vm_remap \
			  vm_wire \
			  vm_write
TRUNCTRAPS		= CLK_get_t \
			  DV_r_o_rq \
			  DV_r_over \
			  DV_r_req \
			  DV_read \
			  DV_w_req \
			  DV_write \
			  H_stats \
			  K_t_creat \
			  M_p_al_nm \
			  M_p_al_qs \
			  M_p_al_sb \
			  M_p_alloc \
			  M_p_deall \
			  M_p_dest \
			  M_p_in_rt \
			  M_p_m_ref \
			  M_p_mv_m \
			  M_p_renam \
			  M_p_rqnot \
			  MO_c_attr \
			  MO_d_err \
			  MO_d_supp \
			  MO_d_unav \
			  MO_disc_r \
			  MO_lock_r \
			  MO_sync_c \
			  TH_ab_saf \
			  TH_cre \
			  TH_cre_r \
			  TH_dep_ab \
			  TH_info \
			  TH_s_ex_p \
			  TH_s_stat \
			  TH_susp \
			  TH_switch \
			  TH_term \
			  TK_create \
			  TK_s_ex_p \
			  TK_s_p_s \
			  TK_s_sp_p \
			  TK_susp \
			  TK_term \
			  VM_alloc \
			  VM_behav \
			  VM_deall \
			  VM_m_attr \
			  VM_prot \
			  VM_r_over

# Same deal here as for trap names, except that here the length limit
# is 12 characters (no "MS_" prefix).  Any object file with a longer
# name has to be truncated in TRUNCOFILES.
#
TRUNCOFILES		= BTServer.o \
			  BTUser.o \
			  CK_rp_server.o \
			  DV_r_rp_over.o \
			  DV_req_user.o \
			  DV_rp_server.o \
			  DV_S_rp_serv.o \
			  DP_obj_svr.o \
			  DP_obj_user.o \
			  F_prntf_stde.o \
			  MG_deall.o \
			  MG_rp_setup.o \
			  MO_server.o \
			  MO_user.o \
			  M_debug_svr.o \
			  M_debug_user.o \
			  M_error_str.o \
			  M_host_user.o \
			  M_init_ports.o \
			  M_msg_dstry.o \
			  M_msg_rcv.o \
			  M_msg_send.o \
			  M_msg_server.o \
			  M_sub_join.o \
			  M_norma_user.o \
			  M_port_user.o

MACHINEOFILES		= ${${TARGET_MACHINE}_OFILES:S/mach_host_priv/M_h_p/}

OFILES =		  clock_res.o \
			  clock_sleep.o \
			  clock_user.o \
			  device_user.o \
			  error_codes.o \
			  exc_server.o \
			  exc_user.o \
			  ledger_user.o \
			  mach_error.o \
			  mach_init.o \
			  mach_msg.o \
			  mach_user.o \
			  malloc.o \
			  mig_allocate.o \
			  mig_strncpy.o \
			  mig_support.o \
			  notify_svr.o \
			  notify_user.o \
			  Snotify_svr.o \
			  panic.o \
			  port_obj.o \
			  prof_server.o \
			  prof_user.o \
			  safe_gets.o \
			  sbrk.o \
			  slot_name.o \
			  sync_user.o \
			  Smem_svr.o \
			  Smem_def_svr.o \
			  ${TRUNCOFILES} \
			  ${TRUNCTRAPS:S/^/MS_/g:S/$/.o/g} \
			  ${FULLTRAPS:S/^/ms_/g:S/$/.o/g} \
			  ${MACHINEOFILES}

.if exists(${LIBMACH_INCLUDE_PREFIX}${TARGET_MACHINE}/machdep.mk)
.include "${LIBMACH_INCLUDE_PREFIX}${TARGET_MACHINE}/machdep.mk"
.endif
.include <${RULES_MK}>

# Sources for raw traps are taken from kernel include files.
# Routines that shadow libc versions are ignored.
# .h files are used for mig generated interfaces instead of the .c files

migsubst.sed: common.mk
	for i in ${TRAPS}; do \
		echo "s/kern_return_t $$i"'$$'"/kern_return_t mig_$$i/"; \
	done > migsubst.sed

prof.h prof_user.c prof_server.c :	prof.defs
	${_MIG_} ${_MIGFLAGS_} ${.ALLSRC:M*.defs}  -server prof_server.c \
				-user prof_user.c

memory_object_server.c memory_object_user.c:	memory_object.defs
	${_MIG_} ${_MIGFLAGS_} ${.ALLSRC:M*.defs}  -server memory_object_server.c \
				 -user   memory_object_user.c

Smem_svr.c: memory_object.defs
	${_MIG_} ${_MIGFLAGS_} -DSEQNOS ${.ALLSRC:M*.defs} -server Smem_svr.c 

Smem_def_svr.c: memory_object_default.defs
	${_MIG_} ${_MIGFLAGS_} -DSEQNOS ${.ALLSRC:M*.defs} -server Smem_def_svr.c

default_pager_object_server.c default_pager_object_user.c: \
						default_pager_object.defs
	${_MIG_} ${_MIGFLAGS_} ${.ALLSRC:M*.defs}  -server default_pager_object_server.c \
				 -user   default_pager_object_user.c

mach_user.c: mach.defs migsubst.sed
	${_MIG_} ${_MIGFLAGS_} ${.ALLSRC:M*.defs}  -user ${.TARGET}.X -server /dev/null -header mach_interface.h
	sed -f migsubst.sed < mach_interface.h > mig_mach_interface.h
	sed -f migsubst.sed -e 's/include "mach_interface.h"/include "mig_mach_interface.h"/' < ${.TARGET}.X > ${.TARGET}
	${RM} ${_RMFLAGS_} ${.TARGET}.X

mach_port_user.c: mach_port.defs migsubst.sed
	${_MIG_} ${_MIGFLAGS_} ${.ALLSRC:M*.defs}  -user ${.TARGET}.X -server /dev/null -header mach_port.h
	sed -f migsubst.sed < mach_port.h > mig_mach_port.h
	sed -f migsubst.sed -e 's/include "mach_port.h"/include "mig_mach_port.h"/' < ${.TARGET}.X > ${.TARGET}
	${RM} ${_RMFLAGS_} ${.TARGET}.X

exc_server.c exc_user.c: exc.defs
	${_MIG_} ${_MIGFLAGS_} ${.ALLSRC:M*.defs}  -server exc_server.c -user exc_user.c

clock_user.c: clock.defs migsubst.sed
	${_MIG_} ${_MIGFLAGS_} ${.ALLSRC:M*.defs}  -user ${.TARGET}.X -header clock.h -server /dev/null
	sed -f migsubst.sed < clock.h > mig_clock.h
	sed -f migsubst.sed -e 's/include "clock.h"/include "mig_clock.h"/' < ${.TARGET}.X > ${.TARGET}
	${RM} ${_RMFLAGS_} ${.TARGET}.X

clock_reply_server.c: clock_reply.defs
	${_MIG_} ${_MIGFLAGS_} ${.ALLSRC:M*.defs}  -server ${.TARGET} -user /dev/null

device_user.c: device.defs migsubst.sed
	${_MIG_} ${_MIGFLAGS_} ${.ALLSRC:M*.defs}  -user ${.TARGET}.X -header device.h -server /dev/null
	sed -f migsubst.sed < device.h > mig_device.h
	sed -f migsubst.sed -e 's/include "device.h"/include "mig_device.h"/' < ${.TARGET}.X > ${.TARGET}
	${RM} ${_RMFLAGS_} ${.TARGET}.X

device_request_user.c: device_request.defs migsubst.sed
	${_MIG_} ${_MIGFLAGS_} ${.ALLSRC:M*.defs}  -user ${.TARGET}.X -server /dev/null
	sed -f migsubst.sed < device_request.h > mig_device_request.h
	sed -f migsubst.sed -e 's/include "device_request.h"/include "mig_device_request.h"/' < ${.TARGET}.X > ${.TARGET}
	${RM} ${_RMFLAGS_} ${.TARGET}.X

device_reply_server.c: device_reply.defs
	${_MIG_} ${_MIGFLAGS_} ${.ALLSRC:M*.defs}  -server ${.TARGET} -user /dev/null

Sdevice_reply_server.c: device_reply.defs
	${_MIG_} ${_MIGFLAGS_} -DSEQNOS ${.ALLSRC:M*.defs}  -server ${.TARGET} -user /dev/null

device_read_reply_overwrite.c: device_reply.defs migsubst.sed
	${_MIG_} ${_MIGFLAGS_} ${.ALLSRC:M*.defs}  -user ${.TARGET}.X -server /dev/null
	sed -f migsubst.sed < device_reply.h > mig_device_reply.h
	sed -f migsubst.sed -e 's/include "device_reply.h"/include "mig_device_reply.h"/' < ${.TARGET}.X > ${.TARGET}
	${RM} ${_RMFLAGS_} ${.TARGET}.X

mach_host_user.c: mach_host.defs migsubst.sed
	${_MIG_} ${_MIGFLAGS_} ${.ALLSRC:M*.defs}  -user ${.TARGET}.X -server /dev/null
	sed -f migsubst.sed < mach_host.h > mig_mach_host.h
	sed -f migsubst.sed  -e 's/include "mach_host.h"/include "mig_mach_host.h"/' < ${.TARGET}.X > ${.TARGET}
	${RM} ${_RMFLAGS_} ${.TARGET}.X

mach_debug_server.c mach_debug_user.c: mach_debug.defs
	${_MIG_} ${_MIGFLAGS_} ${.ALLSRC:M*.defs}  -server mach_debug_server.c \
				 -user  mach_debug_user.c

mach_norma_user.c: mach_norma.defs
	${_MIG_} ${_MIGFLAGS_} ${.ALLSRC:M*.defs}  -user ${.TARGET} -server /dev/null

notify_svr.c notify_user.c: notify.defs
	${_MIG_} ${_MIGFLAGS_} ${.ALLSRC:M*.defs}  -server notify_svr.c -user notify_user.c

Snotify_svr.c: notify.defs
	${_MIG_} ${_MIGFLAGS_} -DSEQNOS ${.ALLSRC:M*.defs}  -server Snotify_svr.c -user /dev/null

sync_user.c: sync.defs
	${_MIG_} ${_MIGFLAGS_} ${.ALLSRC}  -user  ${.TARGET} -server /dev/null

ledger_user.c: ledger.defs
	${_MIG_} ${_MIGFLAGS_} ${.ALLSRC}  -user ${.TARGET} -server /dev/null
#
# Rules to generate truncated object filenames
#
BTServer.o: bootstrapServer.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

BTUser.o: bootstrapUser.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

CK_rp_server.o: clock_reply_server.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

DV_r_rp_over.o: device_read_reply_overwrite.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

DV_req_user.o: device_request_user.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

DV_rp_server.o: device_reply_server.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

DV_S_rp_serv.o: Sdevice_reply_server.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

DP_obj_svr.o: default_pager_object_server.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

DP_obj_user.o: default_pager_object_user.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

F_prntf_stde.o: fprintf_stderr.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

MG_deall.o: mig_deallocate.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

MG_rp_setup.o: mig_reply_setup.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

MO_server.o: memory_object_server.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

MO_user.o: memory_object_user.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

MS_CLK_get_t.o: ms_clock_get_time.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

MS_DV_r_o_rq.o: ms_device_read_overwrite_request.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

MS_DV_r_over.o: ms_device_read_overwrite.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

MS_DV_r_req.o: ms_device_read_request.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

MS_DV_read.o: ms_device_read.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

MS_DV_w_req.o: ms_device_write_request.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

MS_DV_write.o: ms_device_write.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

MS_H_stats.o: ms_host_statistics.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

MS_K_t_creat.o: ms_kernel_task_create.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

MS_M_p_al_nm.o: ms_mach_port_allocate_name.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

MS_M_p_al_qs.o: ms_mach_port_allocate_qos.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

MS_M_p_al_fl.o: ms_mach_port_allocate_full.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

MS_M_p_al_sb.o: ms_mach_port_allocate_subsystem.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

MS_M_p_alloc.o: ms_mach_port_allocate.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

MS_M_p_deall.o: ms_mach_port_deallocate.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

MS_M_p_dest.o: ms_mach_port_destroy.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

MS_M_p_in_rt.o: ms_mach_port_insert_right.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

MS_M_p_m_ref.o: ms_mach_port_mod_refs.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

MS_M_p_mv_m.o: ms_mach_port_move_member.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

MS_M_p_renam.o: ms_mach_port_rename.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

MS_M_p_rqnot.o: ms_mach_port_request_notification.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

MS_MO_c_attr.o: ms_memory_object_change_attributes.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

MS_MO_d_err.o: ms_memory_object_data_error.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

MS_MO_d_supp.o: ms_memory_object_data_supply.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

MS_MO_d_unav.o: ms_memory_object_data_unavailable.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

MS_MO_disc_r.o: ms_memory_object_discard_reply.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

MS_MO_lock_r.o: ms_memory_object_lock_request.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

MS_MO_sync_c.o: ms_memory_object_synchronize_completed.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

MS_TH_ab_saf.o: ms_thread_abort_safely.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

MS_TH_cre.o: ms_thread_create.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

MS_TH_cre_r.o: ms_thread_create_running.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

MS_TH_dep_ab.o: ms_thread_depress_abort.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

MS_TH_info.o: ms_thread_info.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

MS_TH_s_ex_p.o: ms_thread_set_exception_ports.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

MS_TH_s_stat.o: ms_thread_set_state.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

MS_TH_susp.o: ms_thread_suspend.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

MS_TH_switch.o: ms_thread_switch.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

MS_TH_term.o: ms_thread_terminate.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

MS_TK_create.o: ms_task_create.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

MS_TK_s_ex_p.o: ms_task_set_exception_ports.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

MS_TK_s_p_s.o: ms_task_set_port_space.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

MS_TK_s_sp_p.o: ms_task_set_special_port.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

MS_TK_susp.o: ms_task_suspend.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

MS_TK_term.o: ms_task_terminate.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

MS_VM_alloc.o: ms_vm_allocate.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

MS_VM_behav.o: ms_vm_behavior_set.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

MS_VM_deall.o: ms_vm_deallocate.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

MS_VM_m_attr.o: ms_vm_machine_attribute.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

MS_VM_prot.o: ms_vm_protect.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

MS_VM_r_over.o: ms_vm_read_overwrite.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

M_debug_svr.o: mach_debug_server.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

M_debug_user.o: mach_debug_user.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

M_error_str.o: mach_error_string.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

M_host_user.o: mach_host_user.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

M_init_ports.o: mach_init_ports.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

M_msg_dstry.o: mach_msg_destroy.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

M_msg_rcv.o: mach_msg_receive.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

M_msg_send.o: mach_msg_send.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

M_msg_server.o: mach_msg_server.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

M_sub_join.o: mach_subsystem_join.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

M_norma_user.o: mach_norma_user.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

M_port_user.o: mach_port_user.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@

M_h_p_self.o: mach_host_priv_self.o
	${RM} ${_RMFLAGS_} $@
	${CP} $> $@
