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

#include <kern/misc_protos.h>
#include <kgdb/kgdb_defs.h>

boolean_t kgdb_debug = FALSE;
boolean_t kgdb_initialized = FALSE;
char kgdb_data[PBUFSIZ/2];	/* In data area */
int kgdb_version = KGDB_PROTOCOL_VERSION;
int kgdb_state = STEP_NONE;

/*
 * kgdb_init - initialize kgdb
 */
void
kgdb_init(void)
{

	KGDB_DEBUG(("kgdb_init: entered\n"));

	/*
	 * Perform machine dependent initialization
	 */
	kgdb_machine_init();
}

/*
 * kgdb_enter - enter kgdb due to either a local trap or a remote command
 *
 * Input
 *	type - reason kgdb entered
 *	regs - machine state in gdb format
 */
void
kgdb_enter(
	int		generic_type,	
	int		type,	
	kgdb_regs_t	*regs)
{
	kgdb_cmd_pkt_t	snd;
	kgdb_cmd_pkt_t	rcv;
	char		cmd;
	boolean_t	done = FALSE;
	char		*s;
	char		*d;
	int		i;
	int		error = KGDB_MEM_ERROR;

	switch (generic_type) {
	    case KGDB_COMMAND:
		break;
	    case KGDB_BREAK_POINT:
	    case KGDB_SINGLE_STEP:
		regs->reason = KGDB_SIGTRAP;
		break;
	    default:
		regs->reason = (type << ERROR_SHIFT) | generic_type;
		break;
	}

	if (generic_type == KGDB_KERROR || generic_type == KGDB_KEXCEPTION) {
		snd.k_cmd = 'E';
		snd.k_size = sizeof(regs->reason);
		snd.k_data = (vm_offset_t) &regs->reason;
		kgdb_putpkt(&snd);
	}
	else {
		/* if (generic_type != KGDB_COMMAND) { */

		/*
		 * kgdb entered due to a local trap, tell remote gdb
		 */
		if (!kgdb_initialized) {
		    while(1) {
			int	c;

			KGDB_DEBUG(("kgdb_enter:  sending initial ACK\n"));
			kgdb_putc(ACK);
			if ((c = kgdb_getc(FALSE)) != ACK) {
			    KGDB_DEBUG(("kgdb_enter:  got 0x%x\n", c));
			    delay(1000000);
			}
			else
			    break;
		    }
		    KGDB_DEBUG(("kgdb_enter:  got initial ACK\n"));
		    kgdb_initialized = TRUE;
		} else {
			snd.k_cmd = 'S';
			snd.k_size = sizeof(regs->reason);
			snd.k_data = (vm_offset_t) &regs->reason;
			kgdb_putpkt(&snd);
		}
	}

	/*
	 * Read a command from remote gdb
	 */
	while (!done) {
	    kgdb_getpkt(&rcv);
	    switch (rcv.k_cmd) {
		case '?':		/* gdb initial inquiry */
			snd.k_cmd = 'S';
			snd.k_size = sizeof(regs->reason);
			snd.k_data = (vm_offset_t) &regs->reason;
			kgdb_putpkt(&snd);
			break;
		case 'c':		/* continue current thread */
			if (rcv.k_addr != KGDB_CMD_NO_ADDR)
				regs->eip = rcv.k_addr;
			kgdb_clear_single_step(regs);
			kgdb_state = STEP_NONE;
			done = TRUE;
			break;
		case 'd':		/* Toggle debug mode */
			kgdb_debug = !kgdb_debug;
			snd.k_cmd = 0;
			snd.k_size = 0;
			snd.k_data = (vm_offset_t) kgdb_data;
			kgdb_putpkt(&snd);
			break;
		case 'g':		/* send registers to gdb*/
			snd.k_cmd = 0;
			snd.k_size = REGISTER_BYTES;
			snd.k_data = (vm_offset_t) regs;
			kgdb_putpkt(&snd);
			break;
		case 'G':		/* update registers from gdb*/
			s = (char *) rcv.k_data;
			d = (char *) regs;
			for (i = 0; i < REGISTER_BYTES; i++)
				*d++ = *s++;
			snd.k_cmd = 'O';
			snd.k_size = 0;
			kgdb_putpkt(&snd);
			break;
		case 'H':		/* set thread - ignore for now */
			KGDB_DEBUG(("TODO NMGS - set thread ignored\n"));
			snd.k_cmd = 0;
			snd.k_size = 0;
			snd.k_data = (vm_offset_t) kgdb_data;
			kgdb_putpkt(&snd);
			break;
		case 'm':		/* send memory */
			if (!kgdb_read_bytes(rcv.k_addr,
					     rcv.k_size, 
					     kgdb_data)) {
				snd.k_cmd = 'E';
				snd.k_size = sizeof(error);
				snd.k_data = (vm_offset_t) &error;
				kgdb_putpkt(&snd);
			}
			else {
				snd.k_cmd = 0;
				snd.k_size = rcv.k_size;
				snd.k_data = (vm_offset_t) kgdb_data;
				kgdb_putpkt(&snd);
			}
			break;
		case 'M':		/* update  memory */
			if (!kgdb_write_bytes(rcv.k_addr, rcv.k_size, 
					    		(char *) rcv.k_data)) {
				snd.k_cmd = 'E';
				snd.k_size = sizeof(error);
				snd.k_data = (vm_offset_t) &error;
				kgdb_putpkt(&snd);
			}
			else {
				snd.k_cmd = 'O';
				snd.k_size = 0;
				kgdb_putpkt(&snd);
			}
			break;
		case 'P':		/* Write to single register */
			i = REGISTER_OFFSET(rcv.k_addr);
			if (i >= REGISTER_BYTES) {
				KGDB_DEBUG(("M: bad register %d\n",rcv.k_addr));
				error = KGDB_KERROR;
				snd.k_cmd = 'E';
				snd.k_size = sizeof(error);
				snd.k_data = (vm_offset_t) &error;
			} else { 	/* Update register */
				if (!kgdb_write_bytes((vm_offset_t)regs+i,
						      rcv.k_size,
						      (char *) rcv.k_data)) {
					snd.k_cmd = 'E';
					snd.k_size = sizeof(error);
					snd.k_data = (vm_offset_t) &error;
				} else {
					snd.k_cmd = 'O';
					snd.k_size = 0;
				}
			}
			kgdb_putpkt(&snd);
			break;
			
		case 's':		/* single step current thread */
			if (rcv.k_addr != KGDB_CMD_NO_ADDR)
				regs->eip = rcv.k_addr;
			kgdb_set_single_step(regs);
			kgdb_state = STEP_ONCE;
			done = TRUE;	
			break;
		case 'v':		/* send protocol version */
			snd.k_cmd = 0;
			snd.k_size = sizeof(kgdb_version);
			snd.k_data = (vm_offset_t) &kgdb_version;
			kgdb_putpkt(&snd);
			break;
		default:
			KGDB_DEBUG(("kgdb_enter:  bad command 0x%x\n", 
								rcv.k_cmd));
			snd.k_cmd = 0;
			snd.k_size = 0;
			snd.k_data = (vm_offset_t) kgdb_data;
			kgdb_putpkt(&snd);
			break;
	    }
	}
}

kgdb_jmp_buf_t	*kgdb_recover = JMP_BUF_NULL; 
const char	*kgdb_panic_msg = (const char *) 0;

void
kgdb_panic(
	const char	*message)
{
	KGDB_DEBUG(("kgdb_panic: %s\n", message));

	/*
	 * Remember the message and bomb out
	 */
	kgdb_panic_msg = message;
	kgdb_longjmp(kgdb_recover, 1);
	/* NOTREACHED */
}

boolean_t
kgdb_in_single_step(void)
{
	return(kgdb_state == STEP_ONCE);
}


