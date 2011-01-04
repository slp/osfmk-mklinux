/*
 * Copyright 1991-1998 by Open Software Foundation, Inc. 
 *              All Rights Reserved 
 *  
 * Permission to use, copy, modify, and distribute this software and 
 * its documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appears in all copies and 
 * that both the copyright notice and this permission notice appear in 
 * supporting documentation. 
 *  
 * OSF DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE. 
 *  
 * IN NO EVENT SHALL OSF BE LIABLE FOR ANY SPECIAL, INDIRECT, OR 
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT, 
 * NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION 
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
 * 
 */
/*
 * MkLinux
 */


#include <mach.h>
#include <mach/message.h>
#include <mach/mach_types.h>
#include <mach/port.h>
#include <mach/mach_types.h>
#include <mach/mach_traps.h>
#include <i386/eflags.h>
#include <gdb_packet.h>
#include <tgdb.h>


extern unsigned int tgdb_debug_flags;

void print_thread_state(struct i386_thread_state *ts);


void
tgdb_get_registers(mach_port_t thread,
                   struct i386_thread_state *ss,
                   struct i386_float_state *fs)
{
	kern_return_t kr;
	mach_msg_type_number_t count;
	
	kr = thread_abort(thread);

	if (kr == KERN_NO_THREAD) {
		/* No thread attach to this activation */
		bzero(ss, i386_THREAD_STATE_COUNT);
		bzero(fs, i386_FLOAT_STATE_COUNT);
		return;
	}

	if (kr != KERN_SUCCESS && kr != KERN_NO_THREAD) {
		printf("tgdb: can't abort thread\n");
		return;
	}

	count = i386_THREAD_STATE_COUNT;
	kr = thread_get_state(thread, i386_THREAD_STATE, 
			      (thread_state_t) ss, &count);
	if (kr != KERN_SUCCESS) {
		printf("tgdb: can't get thread state\n");
		return;
	}

	if (0 && tgdb_debug_flags & 256) {
		printf("\ntgdb_get_registers:\n");
		print_thread_state(ss);
	}

        if (fs) {
		count = i386_FLOAT_STATE_COUNT;
               	kr = thread_get_state(thread, i386_FLOAT_STATE, 
				      (thread_state_t)fs, &count);

       		if (kr != KERN_SUCCESS) {
               		printf("tgdb: can't get float thread state\n");
                       	return;
		}
        }

}

/*
** tgdb_get_break_addr: Used to get the address of the instruction the
**                      breakpoint is on.
*/
int
tgdb_get_break_addr(mach_port_t thread)
{

	struct i386_thread_state ss;

	tgdb_get_registers(thread, &ss, 0);
	return (int)ss.eip;

}

void
tgdb_set_registers(mach_port_t thread, 
		   struct i386_thread_state *ss, 
		   struct i386_float_state  *fs)
{
	kern_return_t kr;


	kr = thread_abort(thread);

	if (kr == KERN_NO_THREAD) {
		return;
	}

	if (0 && tgdb_debug_flags & 256) {
		printf("\ntgdb_set_registers:\n");
		print_thread_state(ss);
	}

	kr = thread_set_state(thread, i386_THREAD_STATE, (thread_state_t)ss, 
			      i386_THREAD_STATE_COUNT);

        if (kr != KERN_SUCCESS) {
                printf("tgdb: can't set thread state\n");
                return;
        }

        if (fs) {
                kr = thread_set_state(thread, i386_FLOAT_STATE,
                                     (thread_state_t)fs, 
				     i386_FLOAT_STATE_COUNT);

                if (kr != KERN_SUCCESS) {
                        printf("tgdb: can't set float state\n");
                        return;
                }
        }

        return;

}

/*
 * Set the trace bit in the thread's PSW.  This will cause it to
 * execute a single user-mode instruction, then raise a breakpoint
 * exception.
 */
void
tgdb_set_trace(mach_port_t thread)
{
	struct i386_thread_state ss;

	tgdb_get_registers(thread, &ss, 0);
	ss.efl |= EFL_TF;
	tgdb_set_registers(thread, &ss, 0);

}

/*
 * Suspend thread, retrieve registers, and clear the trace bit in the
 * thread's PSW.
 */
void
tgdb_clear_trace(mach_port_t thread)
{
	struct i386_thread_state ss;

	if (thread_suspend(thread) != KERN_SUCCESS) {
		printf("tgdb: can't suspend thread\n");
		return;
	}
	tgdb_get_registers(thread, &ss, 0);
	ss.efl &= ~EFL_TF;
	tgdb_set_registers(thread, &ss, 0);

}

/*
 * Retrieve basic information about each thread in the task.  This will
 * be simply formatted and printed for the remote gdb user.
 */
void
tgdb_basic_thread_info(
	mach_port_t thread,
	mach_port_t task,
	unsigned int *buffer)
{
	struct thread_basic_info tbi;
	unsigned int size = sizeof(tbi);
	struct i386_thread_state ss;
	kern_return_t kr;

	if (task != task) task = task;
	
	kr = thread_info(thread, THREAD_BASIC_INFO,
			 (thread_info_t)&tbi, &size);
	if (kr == KERN_NO_THREAD) {
		*buffer++ = 0;	/* run state */
		*buffer++ = 0;	/* flags */
		*buffer++ = 0;  /* susp count */
		*buffer++ = 0;  /* pc */
	} else {
		tgdb_get_registers(thread, &ss, 0);
	  	*buffer++ = tbi.run_state;
		*buffer++ = tbi.flags;
		*buffer++ = tbi.suspend_count;
#if 0
		if (ss.flags & 2) {
			if (tgdb_vm_read(task,
					 ss.r4 - 0x14, /* FM_CRP */
					 sizeof (*buffer),
					 (vm_offset_t) buffer) != KERN_SUCCESS)
				*buffer = ss.r31;
		} else 
#endif
	  		*buffer = ss.eip;
	}
}

void
print_thread_state(struct i386_thread_state *ts)
{

  printf("gs\t%x\tfs\t%x\tes\t%x\tds\t%x\n",
	ts->gs, ts->fs, ts->es, ts->ds);

  printf("edi\t%x\tesi\t%x\tebp\t%x\tesp\t%x\n",
	ts->edi, ts->esi, ts->ebp, ts->esp);

  printf("ebx\t%x\tedx\t%x\tecx\t%x\teax\t%x\n",
	ts->ebx, ts->edx, ts->ecx, ts->eax);

  printf("eip\t%x\tcs\t%x\tefl\t%x\tuesp\t%x\tss\t%x\n",
	ts->eip, ts->cs, ts->efl, ts->uesp, ts->ss);

  return;
}

void
i386ts_to_gdb(struct i386_thread_state *in, struct i386_float_state *fp,
              struct gdb_registers *gdb)
{

  gdb->eax		= in->eax;
  gdb->ecx		= in->ecx;
  gdb->edx		= in->edx;
  gdb->ebx		= in->ebx;
  gdb->esp		= in->uesp;
  gdb->ebp		= in->ebp;
  gdb->esi		= in->esi;
  gdb->edi		= in->edi;
  gdb->eip		= in->eip;
  gdb->efl		= in->efl;
  gdb->cs		= in->cs;
  gdb->ss		= in->ss;
  gdb->ds		= in->ds;
  gdb->es		= in->es;
  gdb->fs		= in->fs;
  gdb->gs		= in->gs;

  if (fp == fp) fp = fp;

}


void
gdb_to_i386ts(struct gdb_registers *gdb, struct i386_thread_state *in, 
              struct i386_float_state *fp)

{

  in->eax              = gdb->eax;
  in->ecx              = gdb->ecx;
  in->edx              = gdb->edx;
  in->ebx              = gdb->ebx;
  in->uesp             = gdb->esp;
  in->ebp              = gdb->ebp;
  in->esi              = gdb->esi;
  in->edi              = gdb->edi;
  in->eip              = gdb->eip;
  in->efl              = gdb->efl;
  in->cs               = gdb->cs;
  in->ss               = gdb->ss;
  in->ds               = gdb->ds;
  in->es               = gdb->es;
  in->fs               = gdb->fs;
  in->gs               = gdb->gs;

  if (fp == fp) fp = fp;
}


