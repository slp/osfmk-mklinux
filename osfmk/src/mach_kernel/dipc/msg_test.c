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
 */
/*
 * MkLinux
 */

/*
 * In-kernel mach messaging test program for dipc using untyped ipc
 *                   Ademola Aderibigbe, July 14 1994. 
 *   
 * The various component messaging tests have been coalesced under a
 * framework designed by Alan Langerman.  The test generates a series
 * of messages with optional inline and OOL components of arbitrary
 * size. The messages can also convey arbitrary numbers of inline and
 * out-of-line send and receive rights. The messages are crafted from
 * an input specification table and are sent to the remote node where
 * they are received, deciphered and validated. If the message transmits
 * port rights, additional messages are sent to and/or received from the
 * pertinent ports to validate the authenticity of those rights. Upon
 * completion of the validation process a reply (if requested) is sent
 * from the remote node to the originating node conveying the results
 * of the validation process.
 *
 */

#if !USER_SPACE
#include <kernel_test.h>
#endif /* !USER_SPACE */

#if	KERNEL_TEST

#include <mach/kkt_request.h>
#include <mach/message.h>
#include <mach/norma_special_ports.h>
#include <mach/port.h>
#include <mach/mach_server.h>
#include <mach/mach_port_server.h>
#include <ipc/ipc_space.h>
#include <kern/assert.h>
#include <kern/task.h>
#include <kern/thread.h>
#include <kern/zalloc.h>
#include <kern/host.h>
#include <kern/misc_protos.h>
#include <vm/vm_kern.h>
#include <dipc/dipc_funcs.h>
#include <dipc/dipc_kmsg.h>
#include <dipc/dipc_port.h>
#include <dipc/dipc_error.h>
#include <dipc/dipc_msg_progress.h>
#include <dipc/dipc_internal.h>
#include <kern/lock.h>		/* Is this necessary? */
#include <dipc/special_ports.h>

#elif	USER_SPACE

#include <stdlib.h>
#include <stdio.h>
#include <mach.h>
#include <servers/netname.h>
#include <string.h>
#include <unistd.h> /* pid */
#include <sys/types.h>
#include <assert.h>
#include <mach/norma_special_ports.h>
#include <errno.h>

#endif	/* KERNEL_TEST */


/*
 *	The following preprocessor directives allow for the conditional
 *	compilation of msg_test.c as either an in-kernel or user-space
 *	program by inserting the appropriate argument in function calls.
 */
#if	USER_SPACE

#define TASK_OR_MAP	mach_task_self()
#define	TASK_OR_SPACE	mach_task_self()

#elif	KERNEL_TEST

#define	TASK_OR_MAP	current_task()->map
#define	TASK_OR_SPACE	current_space()

#endif	/* USER_SPACE */


/*
 *	The sender generates messages with this ID.
 */
#define	MSG_TEST_SEND_ID	123

/*
 *	The sender will use this special ID when
 *	sending messages that will be received using
 *	the overwrite mode
 */
#define MSG_TEST_OVERWRITE_ID	234

/*
 *	When the receiver responds, it does so with
 *	one of two message ids.  The response to the
 *	initial message from the sender is a message
 *	containing the results of the validation
 *	of the received message and an optional tally
 *	of the parameters in the message. The following
 *	ID will be used for such messages:
 */

#define	RECEIVER_MSG_ID		321


/*	
 *	If the sender included send rights in the
 *	original message, the receiver will respond
 *	with additional messages sent to those send
 *	rights. These messages will have the following
 *	message id.
 */

#define	AUXILIARY_MSG_ID	456

/*
 *	If the sender included receive rights in
 *	the original message, the sender will then
 *	place additional messages on each receive
 *	right. These messages will have the following
 *	message id.
 */

#define	AUXILIARY_MSG_ID	456

/*	Messages from sending nodes requesting the
 *	creation of a port on the receiving node 
 *	to be used solely for the reception of overwrite
 *	messages from that node should have the following ID
 */

#define CREATE_OVERWRITE_PORT_ID 567

/*
 *	(Precursor) Messages alerting the receiver
 *	that the subsequent message should be received
 *	in the overwrite mode use the following ID:
 */
#define	PRECURSOR_MSG_ID	789

/*
 *	Reply to precursor message from receiver
 *	to sender should use the following ID:
 */

#define REPLY_TO_PRECURSOR_ID	890

/*
 *	Reply to the overwrite port creation request
 *	from receiver to sender should use the following ID:
 */

#define	REPLY_TO_CREATE_OVERWRITE_PORT_ID 901

/*
 *	Message alerting receiver that a process has
 *	finished sending its prescribed sequence
 *	of messages.
 */

#define SENDER_PROC_MSG_COMPLETION_ID	012


#define	INLINE_MAX		(32*1024)
#define MAX_MSG_SIZE		(INLINE_MAX + sizeof(mach_msg_header_t))

#define ZERO_TIMEOUT    	(0)

#define XMISSION_OK		(1)
#define XMISSION_ERRORS 	(2)


/*
 *	The test generator.
 *
 *	The send-side repeatedly invokes a test generator to send
 *	a bunch of messages with known values and data types to
 *	another node.  The data types are:
 *		message header only
 *		uninterpreted inline data
 *		inline send and recv rights
 *		uninterpreted ool data
 *		ool send and recv rights
 *
 *	The receiver is a largely-passive reflector of all messages
 *	originated from the test generator.  
 *
 *	The receiver can be set up to deliberately leak resources.
 *	See below.
 */

/*
 *	The following structure stores the information in each entry of the
 *	message test table. Each entry specifies the parameter limits used
 *	to construct all the messages in a given pass of the message table.
 */
typedef struct msg_test_action {
	boolean_t	reply;		/* Are we requesting a reply? */
	unsigned int	nmsgs;		/* number of messages to send */
	int		ibytes;		/* max or -min of inline data bytes */
	int		isnds;		/* max or -min of inline send rights */
	int		ircvs;		/* max or -min of inline recv rights */
	int		oregs;		/* max or -min number of OOL regions */
	int		obytes;		/* max or -min size of OOL regions  */
	int		osnds;		/* .. size of OOL port array (snds) */
	int		orcvs;		/* .. size of OOL port array (rcvs) */
} msg_test_action;

/*
 *	The following structure is used to keep track of the parameters
 *	used to construct the current message (in a specific pass). Note
 *	that the last four parameters in the structure are used to keep
 *	track of the numbers of the various types of OOL regions present
 *	in the message and are not themselves incremented when generating
 *	the next message.
 */

typedef struct msg_params_t {
	int	ibytes;		/* number of inline data bytes */
	int	isnds;		/* number of inline send rights */
	int	ircvs;		/* number of inline recv rights */
	int	oregs;		/* total number of OOL regions */
	int	obytes;		/* size of OOL data regions  */
	int	osnds;		/* size of OOL port array (snds) */
	int	orcvs;		/* size of OOL port array (rcvs) */
	/* The following parameters furnish more info on "oregs" above */
	int	n_odata_reg;	/* number of OOL data regions */
	int	n_osndr_arr;	/* number of OOL send right arrays */
	int	n_osnd1_arr;	/* number of OOL send-once right arrays */
	int	n_orcvs_arr;	/* number of OOL receive right arrays */
} msg_params_t;

int nparams = sizeof(msg_params_t)/sizeof(int);

/*
 *	The following array is used when reporting parameter tally 
 *	mismatches between the sent and received messages.
 */
char *params_strings[] = {
	"number of inline data bytes", 
	"number of inline send rights",
	"number of inline recv rights",
	"total number of OOL regions",
	"size of OOL data regions",
	"size of OOL port array (snds)",
	"size of OOL port array (rcvs)",
	"number of OOL data regions",
	"number of OOL send right arrays",
	"number of OOL send-once right arrays",
	"number of OOL receive right arrays"
};

/*
 *	Define increments to be used to update the sizes of the inline
 *	and out of line data regions as we generate successive messages
 *	in each pass.
 */
#define IBYTE_INCR 4
#define OBYTE_INCR 4

/*
 *	The following array is used to keep track of the increments
 *	to be applied to the various message components as we generate
 *	succesive messages.
 */
int msg_incr[] = {IBYTE_INCR, 1, 1 ,1, OBYTE_INCR, 1, 1};

/* 
 *	The following specifies the number of message parameters
 *	we need to update to generate the next message in sequence
 *	in any given pass.
 */ 
int param_count = sizeof(msg_incr)/sizeof(int);

/*
 *	Message layout used in these tests.
 *
 *	A message header followed by a message body (merely
 *	a count of the following data descriptors), followed
 *	by the descriptor array.  Immediately following the
 *	descriptors is the inline contents of the message.
 *
 *	If the message is not complex, the data start at the
 *	beginning of the message body -- there are no data
 *	descriptors and hence no need for a descriptor count.
 */
typedef struct msg_layout {
	mach_msg_header_t	h;
	mach_msg_body_t		b;
	mach_msg_descriptor_t	d[1];
} msg_layout_t;

/*
 * Following are numbers of messages to send to the ports
 * that will be migrated between nodes before and after the
 * the actual migration takes place.
 */

#define NMSGS_PRE	5
#define NMSGS_POST	2

unsigned int	nmsgs_pre = NMSGS_PRE;
unsigned int	nmsgs_post = NMSGS_POST;
unsigned int	nmsgs_stk = NMSGS_PRE + NMSGS_POST;


/*
 *	When msg_test_leak is set to TRUE, the receiver will leak all
 *	of the resources it accumulates from receiving messages. These
 *	resources include all port rights and OOL data. However, the 
 *	receiver reuses its message buffers, so inline data effectively
 *	does not leak.
 *
 *	The counts indicate how many ports and ool regions have leaked
 *	so far; and in the case of ool regions, how many bytes have
 *	leaked.  (Note:  actual memory leakage will be greater than the
 *	byte count, given that the system will be leaking pages.)
 */
boolean_t	msg_test_leak = FALSE;
unsigned int	msg_test_leak_port_count = 0;
vm_size_t	msg_test_leak_ool_region_size = 0;
unsigned int	msg_test_leak_ool_region_count = 0;


mach_port_t	msg_test_rcv_name;	/* receiving threads watch this port */
mach_port_t	msg_test_reply_name;	/* sender awaits replies here */

/*
 *	The in-kernel version of the test is done using two nodes: the
 *	sending node and the target (or receiving) node. The user space
 *	version of the test is done using any (specified) number of sending
 *	nodes in conjunction with one receiving node.
 */
#if	KERNEL_TEST

task_t          msg_test_task_snd, msg_test_task_rcv;
node_name	msg_target_node = NODE_NAME_NULL;

#elif	USER_SPACE

unsigned int	numb_sender_procs;

#endif /* KERNEL_TEST */

/*
 *      Define boot_node for defining the node on which the
 *      local nameserver is running (for use by the user-space DIPC
 *      message test).
 */
unsigned int	boot_node;

unsigned int	msg_table_index = 1;/* default: standard message test table */
msg_test_action	*msg_table_address; /* pointer to first test table entry */
unsigned int	msg_test_initialized = 0;
unsigned int	simple_msg_str_len = 16;
char		*simple_msg_str = "test string XYZ";/* simple message string */
unsigned int	msg_test_receiver_sync = 1; /* Check that rec'd msg in sync */ 
unsigned int	msg_test_receiver_verbose = 0;
unsigned int	msg_test_sender_verbose = 0;
unsigned int	msg_port_processing_verbose = 0;
int		msg_test_generator_break = 0; /* valid choices: 0, 1, 2, 3 */
int		msg_test_receiver_break = 0;
int		msg_component_verbose = 1;
int		msg_validation_verbose = 1;
mach_port_t	msg_test_snd_name;

/*
 *	Following is maximum number of data errors that will be reported when
 *	verbose mode is turned on i.e., msg_validation_verbose = 1
 */
#define ERR_MAX 10

#if	KERNEL_TEST
/*
 *	The following variables are global only so they may
 *	be examined easily in the debugger.
 */
ipc_port_t	msg_test_rcv_port;	/* msg_test_rcv_name's port */
ipc_port_t	msg_test_snd_port;


/*
 *	Function prototypes.
 */

void		msg_test_init(
			boolean_t	startup);
void		msg_test_sender(void);
void		kern_msg_test( /* used to be dipc_msg_test */
			node_name	target_node);
void		kern_msg_test_raw(
			node_name	target_node,
			msg_test_action	*address,
			int		pass_start,
			int		pass_end);
void		kern_msg_test_full(
			node_name	target_node,
			int		table_index,
			int		pass_start,
			int		pass_end);

#endif /* KERNEL_TEST */

void		msg_test_receiver(void);
void		msg_test_generator(
			mach_port_t		send_name,
			msg_test_action		*actions,
			int			count);
void		generate_message_parameters(
			msg_test_action		*act,
			int			*msg_incr,
			msg_params_t 		*cur,
			boolean_t		init);
msg_layout_t	*generate_message(
			mach_port_t		send_name,
			msg_params_t		*cur,
			msg_test_action		*act,
			boolean_t		deallocate,
			boolean_t		overwrite,
			mach_msg_copy_options_t	copy);
boolean_t	validate_message_header(
			msg_layout_t		*msg);
boolean_t	validate_message_descriptors(
			msg_layout_t		*msg);
void		tally_message_parameters(
			msg_layout_t		*msg,
			msg_params_t		*msg_params);
boolean_t	validate_data(
			msg_layout_t		*msg,
			char			*bytes,
			vm_size_t		size);
boolean_t	validate_port_right(
			mach_msg_type_name_t	disposition,
			mach_port_t		port);
void		fillin_msg_data(
			int			msgsize,
			vm_address_t		data_ptr);
void		fillin_ool_data(
			int 				region_size,
			mach_msg_ool_descriptor_t	*ool_descriptor,
			boolean_t			deallocate,
			mach_msg_copy_options_t		copy);
void		fillin_ool_port_array(
			int				array_size,
			mach_msg_ool_ports_descriptor_t *ool_ports_descriptor,
			boolean_t			deallocate,
			mach_msg_copy_options_t		copy,
			mach_msg_type_name_t		disposition);
#if	KERNEL_TEST /* Do I really need this ??? */
/*
int		bcmp(
			char				*string1,
			char				*string2,
			int				length);
*/
#endif	/* KERNEL_TEST */

void		create_port_right(
			mach_msg_type_name_t		type,
			mach_msg_port_descriptor_t *	pd);
void		create_port_right_array(
			mach_msg_ool_ports_descriptor_t *	opd);
void		send_precursor_message(
			mach_port_t			send_name,
			msg_params_t			*cur);
void		send_node_completion_message(
			mach_port_t			send_name);
void		reply_to_precursor(			
			msg_layout_t			*msg);
msg_layout_t	*overwrite_buffer_alloc(
			msg_layout_t			*precursor_msg, 
			vm_size_t			*msg_size_ptr );
void		alloc_ool_data_buf(
			int 				region_size,
			mach_msg_ool_descriptor_t	*ool_descriptor);
void		alloc_ool_port_array_buf(
			int				array_size,
			mach_msg_ool_ports_descriptor_t *ool_ports_descriptor,
			mach_msg_type_name_t		disposition);
void		acquire_overwrite_right(
			mach_port_t			remote_right,
			mach_port_t			*overwrite_right_ptr);
void		send_overwrite_right(
			msg_layout_t		*msg,
			mach_port_t		*overwrite_port_ptr,
			mach_port_t		overwrite_port_set);
void		send_simple_message(
			mach_msg_type_name_t	disposition,
			mach_port_t		port_name);
boolean_t	receive_simple_message(
			mach_port_t		port_name);
void		destroy_message_contents(
			msg_layout_t		*msg,
			boolean_t		send_side);
void		respond_to_port_right(
			mach_msg_type_name_t	disposition,
			mach_port_t		port);
void		receiver_responses(
			msg_layout_t		*msg);
void		sent_message_processing(
			msg_layout_t		*msg);
void		received_message_processing(
			msg_layout_t		*msg);
int		next_value(
			int			current_value,
			int			limit,
			int			incr);
boolean_t	decouple_entry(
			int			coupled_oregs,
			unsigned int		*numb_oregs,
			char			*flags_oregs);
boolean_t	user_table_input_check(
			int			table_index,
			int			pass_start,
			int			pass_end);
#if	USER_SPACE

int		get_node_numb(
			char			*str);
#endif	/* USER_SPACE */

#define	K	* 1024
#define	M	* 1024 K

/*
 *	Following are used to set the bits in the number from the message
 *	table below that prescribes OOL region transmission modes.
 */

/*	Define the settable bits	*/

#define LOW_BIT		0x80000
#define DEALLOCATE_BIT	0x40000
#define PHYSICAL_BIT	0x20000
#define	OVERWRITE_BIT	0x10000

#define NFLAGS	4	/* Number of flags */

/*	Define the flags that set the bits	*/

#define L       LOW_BIT |	/* Specified number is low point of range */
#define D       DEALLOCATE_BIT |/* Deallocate OOL regions after sending*/
#define P       PHYSICAL_BIT |	/* Physical copy OOL transmission*/
#define O       OVERWRITE_BIT |	/* Overwrite OOL reception */


/*
 *	Mandated standard message tests are detailed in the tables 
 *	which follow. The first table is the default for in-kernel
 *	testing while the second is the default for user space testing.
 *	The tables indicate the starting or ending size or number for
 *	each of the primitives currently in use according to the notation
 *	explained below.
 *
 *	The limit for the size or number of a given primitive in the
 *	message test table can be specified as a positive number
 *	(i.e., > 0) implying that that message component will have
 *	the range (0 to limit] or it can be a negative number (i.e., < 0)
 *	implying that the message components will have the range
 *	[abs(limit) to infinity). If the limit is zero, we do not have
 *	any of that component.
 *
 *	The primitives (and the corresponding limits in the table) are:
 *
 *		inline data components (max or min size of abs(ibytes) )
 *		inline send rights (max or min number of abs(isnds) )
 *		inline receive rights (max or min number of abs(ircvs) )
 *		out-of-line regions (max or min number of abs(oregs) )
 *		   note: number of OOL regions includes OOL port arrays, if any
 *		out-of-line data regions (max or min size of abs(obytes) )
 *		out-of-line send rights (max or min number of abs(osnds) )
 *		out-of-line receive rights (max or min number of abs(orcvs) )
 *
 *
 *	Notes:
 *
 *
 *
 *
 *	(1) The out-of-line (OOL) regions are distributed among OOL data
 *	regions, OOL port arrays of send rights and OOL port arrays of receive
 *	rights in round robin fashion.
 *
 *	(2) When the message specification includes inline send rights,
 *	we will actually transmit send rights followed by send-once rights
 *	in alternating fashion. If "n" send rights are specified, the message
 *	will contain (n/2 + n % 2) send rights and (n/2) send-once rights. 
 *
 *	(3) When the message specification includes OOL regions which include
 *	send right arrays, we will transmit arrays with send rights on one
 *	round-robin cycle and arrays with send-once rights on the subsequent 
 *	round-robin cycle as we cycle through the various OOL region types.
 *	Thus if one pass is:
 *
 *		OOL data , OOL send right array, OOL receive right array.
 *
 *	the following will be (subject to OOL region availability):
 *
 *		OOL data , OOL send-once right array, OOL receive right array
 *
 *	(4) Message pass "0" is one-way, all other passes are "pings".
 *
 */
msg_test_action	message_tests[] = {/* Default table for in-kernel testing */
/*  
                                       L = low (i.e., min, else number is max)
         where Flags = [LDVO?]:        D = deallocate (equiv. to vm_deallocate)
                                       P = physical copy (else virtual copy)
				       O = overwrite (else dynamically alloc')
		     INLINE	 		    OUT_OF_LINE   
Reply  Nmsgs   Ibytes    Isnds  Ircvs     Oregs       Obytes   Osnds Orcvs
       XXXXX, SXXXXXXXX, SXXXX, SXXXX, L D P O XXXXX  SXXXXXXXXX, SXXXX, SXXXX,
FALSE, 12345, -12345678, -1234, -1234, L D P O 12345, -123456789, -1234, -1234,
*/
FALSE,     1,         0,     0,	    0,	           0,	       0,     0,     0,
TRUE,      1,         0,     0,	    0,	           0,	       0,     0,     0,
TRUE,	   1,         1,     0,	    0,	           0,	       0,     0,     0,
TRUE,	  15,	  -8180,     0,	    0,	           0,	       0,     0,     0,
TRUE,	   1,	      0,     1,	    0,	           0,	       0,     0,     0,
TRUE,	  13,         0,    13,	    0,	           0,	       0,     0,     0,
TRUE,	   1,	      0,     0,	    1,	           0,	       0,     0,     0,
TRUE,	  13,	      0,     0,	   -4,	           0,	       0,     0,     0,
TRUE,	   3,	 -16360,    30,	   10,	           0,	       0,     0,     0,
TRUE,	  10,	      0,     0,	    0, L           1,	  -128 K,     0,     0,
TRUE,	   5,	      0,     0,	    0, L           2,	   -8192,     4,    -2,
TRUE,	   2,	      0,     0,	    0,             1,	    -2 M,     0,     0,
TRUE,	  10,	   -8 K,    -3,	    4, L D         2,	   -16 K,    -1,    -1,
TRUE,	   3,	      0,     0,	    0,             3,	       0,     0,     0,
#if USER_SPACE
TRUE,	  20,	      0,     0,	    0,	   P       1,	   -8192,     0,     0,
TRUE,	   5,	      0,     0,	    0, L D P O     2,	       0,     4,    -2,
TRUE,	  10,	      0,     0,	    0, L D P O    10,	    -128,     0,     0,
#endif /* USER_SPACE */
};
int	message_test_count = sizeof(message_tests) / sizeof(msg_test_action);

/*
 *	Any user can specify that an alternate message test table be used.
 *	This is a convenient holding place for tests that may not work at
 *	the moment allowing the standard table to be used as a regression
 *	test table.
 */

msg_test_action	message_tests_alt[] = {/* Default table for user space test*/
/*  
                                       L = low (i.e., min, else number is max)
         where Flags = [LDVO?]:        D = deallocate (equiv. to vm_deallocate)
                                       P = physical copy (else virtual copy)
				       O = overwrite (else dynamically alloc')
		     INLINE	 		    OUT_OF_LINE   
Reply  Nmsgs   Ibytes    Isnds  Ircvs     Oregs       Obytes   Osnds Orcvs
       XXXXX, SXXXXXXXX, SXXXX, SXXXX, L D P O XXXXX  SXXXXXXXXX, SXXXX, SXXXX,
FALSE, 12345, -12345678, -1234, -1234, L D P O 12345, -123456789, -1234, -1234,
*/
FALSE,     1,         0,     0,	    0,	           0,	       0,     0,     0,
TRUE,	   1,	      0,     0,	    0, L           2,	       0,     4,    -2,
TRUE,	   5,	      0,     0,	    0, L D         2,	       0,     4,     2,
#if USER_SPACE
TRUE,	   1,	      0,     0,	    0, L D P O     3,	   -8192,     4,     2,
TRUE,	   1,	      0,     0,	    0,       O     1,	       0,     1,     0,
TRUE,	   1,	      0,     0,	    0,       O     1,	       0,     5,     0,
TRUE,	   1,	      0,     0,	    0,       O     1,	       0,     0,     5,
TRUE,	   2,	      0,     0,	    0, L     O    10,	  -32768,     0,     1,
TRUE,	   5,	      0,     0,	    0, L D P       2,    -4*8192,     4,     2,
#endif /* USER_SPACE */
TRUE,	   5,	      0,     0,	    0, L D         2,    -4*8192,     4,     2,
#if USER_SPACE
TRUE,	   2,	      0,     0,	    0, L D   O    10,    -4*8192,     5,     3,
TRUE,	   5,	      0,     0,	    0, L D P O     2,      -8192,     4,     2,
TRUE,	   1,	  -16 K,    -5,	   -6, L D   O     3,	       0,     0,     0,
#endif /* USER_SPACE */
TRUE,	  20,	      0,     0,	    0, L           1,	   -8192,     0,     0,
TRUE,	  20,	   -8 K,    -3,	    4, L D         2,	   -16 K,     0,     0,
TRUE,	   1,	  -16 K,     0,	    0, L D         1,	    -1 M,     0,     0,
TRUE,	   2,	  -16 K,    -2,	   -1, L D         5,	    -1 M,    -1,    -2,
TRUE,	   2,	  -16 K,    -2,	   -1, L           5,	    -1 M,    -1,    -2,
TRUE,	   2,	  -16 K,    -5,	   -6, L D         2,	    -1 M,    -5,    -4,
TRUE,	   1,	  -16 K,     0,	    0, L D         1,	    -2 M,     0,     0,
#if 0
TRUE,	   2,	  -16 K,    -2,	   -1, L           5,	    -2 M,    -1,    -2,
TRUE,	   2,	  -16 K,    -5,	   -6, L D         2,	    -2 M,    -5,    -4,
#if USER_SPACE
TRUE,	   2,	  -16 K,    -5,	   -6, L D   O     2,	    -2 M,    -5,    -4,
TRUE,	   2,	  -16 K,     0,	    0, L     O	   1,	    -5 M,     0,     0,
#endif /* USER_SPACE */
TRUE,	   1,	  -16 K,     0,	    0,	           1,	   -10 M,     0,     0,
TRUE,	   1,	  -16 K,    -5,	   -6, L D         2,	   -10 M,    -5,    -4,
#if USER_SPACE
TRUE,	   1,	  -16 K,    -5,	   -6, L D   O     2,	   -10 M,    -5,    -4,
TRUE,	   1,	  -16 K,    -5,	   -6, L     O     2,	   -10 M,    -5,    -4,
#endif /* USER_SPACE */
TRUE,	   1,	  -16 K,     0,	    0,	           1,	   -20 M,     0,     0,
TRUE,	   1,	  -16 K,     0,	    0,	           1,	   -50 M,     0,     0,
#endif	/* 0 */
};

#define NUMBER_OF_TABLES (2)
#define STANDARD_TABLE	(1)
#define ALTERNATE_TABLE	(2) /* Not used currently */
#define OTHER_TABLE (3) /* Not used currently */
#define ADDRESS_SPEC (NUMBER_OF_TABLES + 1)

/*	
 *	The following provides a handy infrastructure for installing new
 *	messsage test tables in a manner that makes it easy for the user
 *	to request message table information e.g., table index numbers,
 *	table names, table addresses and number of passes. 
 */

#define test_table_entry(x) { (#x" "), (x), ((int) (sizeof(x)/sizeof( msg_test_action)))}

typedef struct test_table_record_t{
		char *table_str;
		msg_test_action	*table_ptr;
		int		table_size;
}test_table_record_t;

test_table_record_t	test_table_record[] = {
				test_table_entry(message_tests),
				test_table_entry(message_tests_alt)
			};

/*
 *	Table installation involves three simple steps:
 *	
 *	Increment the NUMBER_OF_TABLES by one for each new table added.
 *
 *	declare and initialize the table e.g.,
 *		msg_test_action	message_tests_three[] = {....};
 *
 *	add an entry at the end of test_table_record_t
 *		test_table_record[] = {,test_table_entry(message_tests_three)};
 */

/*	To add flexibility to the program, the entire set of message tests
 *	from the message tables specified earlier can be run or segments of
 *	the table can be run by specifying the start and end passes using
 *	the variables msg_test_start and msg_test_end by way of the various
 *	user interfaces.
 */
 
int	msg_test_start = -1;
int	msg_test_end = -1;


#if	KERNEL_TEST

/*
 *	The following are used to serialize and synchronize
 *	the kernel tests. 
 */

extern	boolean_t	unit_test_thread_sync_done;
extern	boolean_t	kernel_test_thread_blocked;
extern	boolean_t	unit_test_thread_blocked;
extern	boolean_t	unit_test_done;
decl_simple_lock_data(extern, kernel_test_lock)

/* 
 *	The following initialization function creates two new tasks
 *	each with a kernel thread. 
 */
void
msg_test_init(boolean_t	startup)
{
	spl_t		s;
	ipc_space_t	msg_test_space_snd, msg_test_space_rcv;
	ipc_port_t	port;
	kern_return_t	kr;
	assert(startup == startup);

	if (msg_test_initialized++ > 0)
		return;

	/*
	 *	Allocate two tasks: one for sending and the other for
	 *	receiving messages.
	 */
	kr = kernel_task_create(kernel_task, 
				(vm_offset_t) 0,
				(vm_size_t) 0,
				&msg_test_task_snd);
	assert(kr == KERN_SUCCESS);
	/* msg_test_space_snd = current_space(); Not yet! */
	msg_test_space_snd = msg_test_task_snd->itk_space;

	kr = kernel_task_create(kernel_task, 
				(vm_offset_t) 0,
				(vm_size_t) 0,
				&msg_test_task_rcv);
	assert(kr == KERN_SUCCESS);
	/* msg_test_space_rcv = current_space(); Not yet! */
	msg_test_space_rcv = msg_test_task_rcv->itk_space;

	/*
	 *	Stock the "receiving" task with a well-known port.
	 *	It is the port that the message receiving thread
	 *	hangs out on waiting for incoming messages.  It is
	 *	registered in the special ports table, so that it
	 *	may be easily found by remote nodes.
	 */
	kr = mach_port_allocate(msg_test_space_rcv,
				MACH_PORT_RIGHT_RECEIVE,
				&msg_test_rcv_name);
	assert(kr == KERN_SUCCESS);
	kr = ipc_object_translate(msg_test_space_rcv, msg_test_rcv_name,
				  MACH_PORT_RIGHT_RECEIVE,
				  (ipc_object_t *)&port);
	assert(kr == KERN_SUCCESS);

	/*
	 *	ipc_object_translate returns a locked but unreferenced port.
	 */
	assert(port->ip_references == 1);
	assert(port->ip_srights == 0);
	ip_unlock(port);
	msg_test_rcv_port = port; /* for debugging and ipc testing */
	kr = norma_set_special_port((host_t) 1, NORMA_TEST_PORT, port);
	assert(kr == KERN_SUCCESS);


	/*
	 *	Stock the "sending" task with a port to be used.
	 *	by the message sending thread for picking up
	 *	replies (to the messages it sends to the receiving
	 *	task).
	 */
	kr = mach_port_allocate(msg_test_space_snd,
				MACH_PORT_RIGHT_RECEIVE,
				&msg_test_reply_name);
	assert(kr == KERN_SUCCESS);

	/*
	 *	Initialize the lock that will be used to guard
	 *	variables that will be used in the test synchronization
	 *	scheme.
	 */ 

	(void) kernel_thread(msg_test_task_rcv, msg_test_receiver, (char *) 0);
	(void) kernel_thread(msg_test_task_snd, msg_test_sender, (char *) 0);
	/*
	 *	Suspend this thread until msg_test_sender() is ready to
	 *	pick up its wake-up call. 
	 */
	s = splsched();
	simple_lock(&kernel_test_lock);
	while(!unit_test_thread_sync_done){
		assert_wait((event_t) &msg_test_init, FALSE);
		kernel_test_thread_blocked = TRUE;
		simple_unlock(&kernel_test_lock);
		splx(s);
		thread_block((void (*)(void)) 0);
		s = splsched();
		simple_lock(&kernel_test_lock);
		kernel_test_thread_blocked = FALSE;
	}
	unit_test_thread_sync_done = FALSE; /* Reset for next use */
	simple_unlock(&kernel_test_lock);
	splx(s);

	assert(msg_test_initialized > 0);
	printf("msg_test: Initialized\n");
}


/*
 *	Running the In-kernel, IPC-level and DIPC-level tests:
 *
 *	To run the in-kernel IPC-level tests, the target node specified is
 *	the same as the number of the node from which the test is initiated.
 *
 *	To run the in-kernel DIPC-level tests, the target node specified is
 *	different from the number of the node from which the test is initiated.
 *	 
 * 	The in-kernel tests are currently instantiated from the debugger.
 *	(When originally started "in-kernel", msg_test_sender blocks waiting
 *	for a user request.) The user, from the debugger,  calls
 *
 *                                either:
 *
 *	    kern_msg_test(target_node)
 *
 *	    --to run all tests in the standard message test table
 *                              
 *                                 or:
 *
 *	    kern_msg_test_full(target_node, table_index, start_pass, end_pass)
 *
 *	   --for more control over which tests are to be executed
 *
 *                                 or:
 *
 *	    kern_msg_test_raw(target_node, table_address, start_pass, end_pass)
 *
 *	   --if the message test table address is known.
 *
 *	The user "continues" in the debugger so that the pertinent thread has a
 *	chance to run.
 */

void
msg_test_sender()
{
	kern_return_t	kr;
	msg_test_action	*actions;
	spl_t		s;
	int		count; /* number of passes in test table */
	int		passes; /* number of passes in test */
	node_name	this_node;
	kernel_boot_info_t	mkbootinfo;		/* 4K array */
	char		*func = "msg_test_sender";

	assert(current_task() == msg_test_task_snd);
	assert(current_space() == msg_test_task_snd->itk_space);


	for (;;) {

		/*
	 	 *	First time through we resume the thread suspended
		 *	in msg_test_init() because msg_test_sender is set
		 *	to respond to its wake-up call. On subsequent trips
		 *	through this point the wakeup is discarded... 
		 */

		s = splsched();
		simple_lock(&kernel_test_lock);
		unit_test_thread_sync_done = TRUE;
		assert_wait((event_t) msg_test_sender, FALSE);
		if(kernel_test_thread_blocked)
			thread_wakeup((event_t)&msg_test_init);
		simple_unlock(&kernel_test_lock);
		splx(s);
		thread_block((void (*)(void)) 0);

		if (!dipc_node_is_valid(msg_target_node)) {
			printf("%s:  invalid node %d\n",func,msg_target_node);
			continue;
		}
		/*
		 *	Compute number of passes in test from number of passes
		 *	in table (and where applicable, user specified starting
		 *	and ending passes).
		 */
		count = test_table_record[msg_table_index - 1 ].table_size;
		passes = ((msg_test_end == -1) ? count :  msg_test_end + 1)
			 - ((msg_test_start == -1) ? 0 : msg_test_start);

		/*
		 *	Determine if we are running IPC or DIPC
		 *	level in-kernel tests.
		 */

		this_node = dipc_node_self();
		if(this_node == msg_target_node){
			printf("This node (%d) will run %d passes of the in-kernel IPC-level message test.\n\n", this_node, passes); 
			/*
			 *	Bypass the use of the special nodes table.
			 *	The call which follows gives us a reference
			 *	and a naked send right.
			 */
			msg_test_snd_port = 
					ipc_port_make_send(msg_test_rcv_port);

		}else{
			printf("Node %d will run %d passes of the in-kernel message test. Target node: %d\n\n", this_node, passes, msg_target_node);

			/*
			 *	Obtain a send right to the target node.
			 */
			kr = norma_get_special_port((host_t) 1,
				msg_target_node, NORMA_TEST_PORT,
				&msg_test_snd_port);
			if (kr != KERN_SUCCESS) {
				printf("%s:  norma_get_special_port returns %d\n", func, kr);
				continue;
			}
		}

		/*
		 *	Contrary to what one might surmise after a cursory
		 *	examination of the comments in one of the functions
		 *	eventually called by ipc_port_copyout_send(),
		 *	a reference will not be consumed by this call.
		 *	If we follow the code path, one finds that a comment
		 *	in ipc_right_copyout() is misleading and it should 
		 *	be fixed to read "Copyout a capability to a space.
		 *	If successful, consumes a ref for the object under
		 *	certain conditions and under other conditions, it
		 *	transfers the ref to the appropriate entry."
		 */
 
		msg_test_snd_name = ipc_port_copyout_send( 
			msg_test_snd_port, current_space());

		assert(msg_test_snd_name != MACH_PORT_DEAD &&
		       msg_test_snd_name != MACH_PORT_NULL &&
		       msg_test_snd_name != (mach_port_t) msg_test_snd_port);

		ip_lock(msg_test_snd_port);
		if(this_node != msg_target_node){
			/*
			 *	When the sender and receiver are on 
			 *	different nodes, the only reference on
			 *	the proxy is the reference we took when
			 *	when we requested a send right to the
			 *	principal (on the remote node).
			 */

			assert(msg_test_snd_port->ip_references == 1);
		}else{
			/*
			 *	When the sender and receiver are on the same
			 *	node, the reference count on the port is the
			 *	sum of the references we started out with
			 *	(i.e., one, see	ipc_test_init()), plus the
			 *	reference we took when the receiver blocked
			 *	while trying to receive its first message, plus
			 *	the reference we took when the sender
			 *	requested a send right (to the receiver) via
			 *	norma_get_special_port().
			 */
			assert(msg_test_snd_port->ip_references == 3);
			assert(msg_test_rcv_port == msg_test_snd_port);
		}
		assert(msg_test_snd_port->ip_srights == 1);
		ip_unlock(msg_test_snd_port);

		/*
		 *	Send a bunch of messages as stipulated by either the
		 *	standard message test table or one of the other message
		 *	test tables.
		 */

		assert(msg_table_index > 0 && msg_table_index <= ADDRESS_SPEC);

		if(msg_table_index == ADDRESS_SPEC) {/* Use table at address input*/
			actions = msg_table_address;
			count = message_test_count;
		}else{ /* Use address of standard or alternate tables */
			actions = test_table_record[msg_table_index - 1 ].table_ptr;
			count = test_table_record[msg_table_index - 1 ].table_size;	
		}

		msg_test_generator(msg_test_snd_name, actions, count);
		/* printf("We have exited from msg_test_generator\n"); */

		mach_port_destroy(current_space(), msg_test_snd_name);

		/*
		 *	If this invocation of the message test is from the
		 *	automatic execution at boot time of the dipc loopback
		 *	tests or the dipc multinode tests , we need to wake up
		 *	the thread waiting for the test to complete.
		 *	If no thread is waiting, the wake up will be ignored.
		 */

		s = splsched();
		simple_lock(&kernel_test_lock);
		unit_test_done = TRUE;
		if(unit_test_thread_blocked)
			thread_wakeup((event_t)&kern_msg_test);
		simple_unlock(&kernel_test_lock);
		splx(s);
	}
}


/*
 * kern_msg_test is one test entry point. We will go into a loop with a
 * series of messages with optional inline and  OOL data regions, inline
 * and OOL send and receive right arrays as specified in the standard
 * message table. Will soon change to kern_msg_test.
 */


void
kern_msg_test(
	node_name	target_node)
{
	if (!dipc_node_is_valid(target_node)) {
		printf("kern_msg_test:  invalid node %d\n", target_node);
		return;
	}

	msg_target_node = target_node;
	msg_table_index = STANDARD_TABLE;
	msg_test_start = -1; /* Reset; we might have set on previous test. */ 
	msg_test_end = -1; 

	thread_wakeup_one((event_t) msg_test_sender);
}

/*
 * kern_msg_test_raw is the test entry point that permits the kernel 
 * "aficionado" to specify the address of the message test table as well as
 * the starting and ending passes for the test.  
 */

void
kern_msg_test_raw(
	node_name	target_node,
	msg_test_action	*address,
	int		pass_start,
	int		pass_end)
{
	boolean_t		faulty_data = FALSE;
	int			i, msg_test_count;
	test_table_record_t	*ttrt;

	/* Check for bogus input parameters */
	if (!dipc_node_is_valid(target_node)) {
		printf("kern_msg_test:  invalid node %d\n", target_node);
		return;
	}
	
	if(pass_end < pass_start){
		printf("Ending pass number must be greater than or the same as the starting pass number for any message tests to be executed\n");
		return;
	}

	/*
	 *	Compute number of passes in that part of the message table we
	 *	care about.
	 */
	msg_test_count = pass_end + 1;
	
	msg_target_node = target_node;
	msg_table_index = ADDRESS_SPEC; /* For raw table address */
	msg_test_start = pass_start; 
	msg_test_end = pass_end; 
	msg_table_address = address; 
	thread_wakeup_one((event_t) msg_test_sender);
}

/*
 * kern_msg_test_full is the newer test entry point allowing for more
 * flexibility in that the user can select either the standard (default)
 * message test table or some other installed message table and also
 * allows the user to specify the starting and ending passes for the test.
 */

void
kern_msg_test_full(
	node_name	target_node,
	int		table_index,
	int		pass_start,
	int		pass_end)
{
	boolean_t		data_ok;

	/*
	 *	Check that the user input message table selection as well as
	 *	the starting and ending passes are valid.
	 */

	data_ok = user_table_input_check(table_index, pass_start, pass_end);
	if(!data_ok)
		return;

	/* Check for bogus input node parameter */
	if (!dipc_node_is_valid(target_node)) {
		printf("kern_msg_test:  invalid node %d\n", target_node);
		return;
	}

	/*
	 *	Set a global variable for use later on.
	 */

	msg_target_node = target_node;

	thread_wakeup_one((event_t) msg_test_sender);
}

#elif	USER_SPACE


int	recv_node; /*node where receiver is running */

#define MAX_NODES 56 /* maximum number of nodes for dipc-level tests */

/* 
 *	This is the entry point for the user space dipc and ipc level tests.
 *
 *	To run the dipc tests, the user specifies a positive number for n (see
 *	below) and the program forks off n sender processes which end up on
 *	various nodes as per the round-robin process allocation mechanism.
 *	The receiver runs on the node where the program is started.
 *
 *	To run the ipc tests, the user sets n to 0 so we don't fork off any
 *	sender processes and instead create a sender process (task) on the
 *	local node on which the receiver also runs. (This code path does not
 *      work for the 386/486 at this point since rfork does not seem to do
 *      the right thing on that machine. On the other hand it seems to work
 *      just fine for the PARAGON 860. If uniformity is desired, we can run
 *      user space ipc-level tests by turning off ACCEPT_PROCS or by 
 *      bringing up only one node. )
 */

main(int argc, char *argv[])
{
	kern_return_t		kr;
	netname_name_t  	name;
	int			i, node;
	int			parent = 1;
	boolean_t		data_ok;
	int			msg_test_count;
	mach_port_t		name_server_p;
	mach_port_t		msg_node_name_port; /* for port location */
	test_table_record_t	*ttrt;
	msg_test_action		*actions;
	int			count;
	int			temp, error;
	char			*func = "main";
	char			*usage = "Usage is:\n msg_test  [-n <number of nodes>] [-i <table index>] [-s <start pass>] [-e <end pass>]\n";
	char			*string_msg = "number of forked sender nodes";



	/*
	 *	Set some default input parameters. 
 	 */

	int	nodes = 2;	/* Number of (forked) sender nodes */
	int	table_index = 2;	/* Message Table Index */
	int	pass_start = -1;	/* Starting Pass, defaults to 0 */
	int	pass_end = -1;	/* Ending pass, defaults to last pass */

	/*
	 *	 Process user input. If problem occurs, specify usage info.
	 */
	error = 0;
	while(argc >= 2){
	    if(argc == 2){ /* Missing value or need space between opt & value*/
		printf("Error found while parsing command input\n");
		printf("Check: White space between all options and values?\n");
		printf(usage);
		exit(1);
	    }
	    if(argv[1][0] == '-'){
		temp = atoi(&argv[2][0]); /* save the number for this option */
		error = errno; /* error checking does not work on paragon? */
		/* printf("argv[1][1] is %c temp is %d\n", argv[1][1], temp);*/
	        switch(argv[1][1]){
		    case 'n':
			if(error != 0){
				printf("Invalid -n argument. Exiting...\n");
				exit(1);
			}
			if((nodes = temp) < 0){
				printf("%s cannot be negative. Exiting...\n",
						 string_msg);
				exit(1);
			}else if(nodes > MAX_NODES){
				printf("%s exceeds %d. Exiting...\n",
						 string_msg, MAX_NODES);
				exit(1);
			}
			break;
		    case 'i':
			if(error != 0){
				printf("Invalid -i argument. Exiting...\n");
				exit(1);
			}
			table_index = temp;
			break;
		    case 's':
			if(error != 0){
				printf("Invalid -s argument. Exiting...\n");
				exit(1);
			}
			pass_start = temp;
			break;
		    case 'e':
			if(error != 0){
				printf("Invalid -e argument. Exiting...\n");
				exit(1);
			}
			pass_end = temp;
			break;
		    default:
			printf(usage);
			exit(1);
		}
		argc-=2;
		argv+=2;
	    }else{
		printf("Error found while parsing command input\n");
		printf("Check: Hyphen ""-"" must precede option specifiers\n"); 
		printf(usage);
		exit(1);
	    }
	}
	printf("\n");

	/*
	 *	Make the number of sending processes global so receiver can
	 *	pick it up and use it to determine when all sending nodes have
         *	finished sending their prescribed message sequence. Also check
	 *	the validity of the user table input.
	 */

	numb_sender_procs = (nodes > 0 ? nodes : 1);
	data_ok = user_table_input_check(table_index, pass_start, pass_end);
	if(!data_ok)
		exit(1);

	/*
	 *	Print out the effective user input or default input.
	 */

	if(!nodes){
		printf("Doing user-space ipc-level tests\n");
	}else{
		printf("Doing user-space dipc-level tests with");
		printf(" %d forked senders\n", nodes);
	}
	printf(" Using message table %d\n", table_index);
	printf(" Starting at pass: %d\n",
		 (pass_start == -1 ? 0 : pass_start));
	printf(" Ending with pass: %d\n", (pass_end == -1 ? 
			test_table_record[table_index -1 ].table_size :
				pass_end));

	/*
	 *	Sending nodes may need to know which node we are running on...
 	 *	Note that norma_port_location hint expects a send right. If
	 *	it is not given one, it responds with an arcane and confusing
	 *	message. 
	 */
	kr = mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE,
				&msg_node_name_port);
	if(kr != KERN_SUCCESS) {
		printf("%s: mach_port_allocate problem kr is 0x%x\n",func, kr);
		exit(1);
	}

	kr = mach_port_insert_right(mach_task_self(), msg_node_name_port,
		msg_node_name_port, MACH_MSG_TYPE_MAKE_SEND);
	if(kr != KERN_SUCCESS) {
		printf("%s: mach_port_insert_right problem kr is 0x%x\n", func, kr);
		exit(1);
	}

	kr = norma_port_location_hint(mach_task_self(), msg_node_name_port, &node);
	if(kr != KERN_SUCCESS) {
		printf("%s: norma_port_location_hint problem kr is 0x%x\n", func, kr);

		exit(1);
	}
	printf("%s is running on node %d\n",func, node);

	recv_node = node; /* Not used in this version of the test */

	/*
	 *	Stock the task with a well-known port. It is the port
	 *	that the message receiving thread hangs out on waiting for
	 *	incoming messages.  It is registered with the name server,
	 *	so that it may be easily found by remote nodes.
	 */

	kr = mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE,
				&msg_test_rcv_name);
	if(kr != KERN_SUCCESS) {
		printf("%s: mach_port_allocate problem kr is 0x%x\n",func, kr);
		exit(1);
	}

	/*
	 *	Determine the node number of the boot node. 
	 */
	/* Try mach_host_self() not mach_host_priv_self */

	boot_node = get_node_numb("ROOT_FS_NODE");
	if(boot_node == -1){
		printf("%s: Can't get boot node number:", func);
		printf(" Using node 0 for name service!\n");
		boot_node = 0;
	}

	/*	Next, we get send rights to the lnsvr name server on the boot
	 *	boot node.
	 *	XXXWe need to build and run mach_init to use the snames name
	 *	server solution used on the 486.XXX
	 */

	kr = norma_get_nameserver_port(mach_host_priv_self(),  boot_node, 
		&name_server_p);
	if (kr != NETNAME_SUCCESS) {
		printf("%s: norma_get_nameserver_port problem: kr = %d or 0x%x\n", func, kr, kr);
		exit(1);
	}

/*
        XXX The following fragment did not work until I replaced
        mach_task_self() with MACH_PORT_NULL XXX

	kr = netname_check_in(name_server_p, "message_test_port",
                               mach_task_self(), msg_test_rcv_name);
	if (kr != NETNAME_SUCCESS) {
		printf("%s: netname_check_in problem kr = 0x%x\n",func, kr);
		exit(1);
        }
*/
	kr = netname_check_in(name_server_p, "message_test_port",
                               MACH_PORT_NULL, msg_test_rcv_name);
	if (kr != NETNAME_SUCCESS) {
		printf("%s: netname_check_in problem kr = 0x%x\n",func, kr);
		exit(1);
        }

	/* Use address of standard or alternate or other tables */
	actions = test_table_record[table_index - 1 ].table_ptr;
	count = test_table_record[table_index - 1 ].table_size;	

	/*
	 *	For ipc-level testing, use rfork to create a sender process
	 *	In what follows, we use the syscall number (276) for rfork
	 *	from syscalls.master 276. At this point in time, (rfork)
	 *	returns the child pid in the parent (as expected) but returns
	 *	the parent pid in the child (as opposed to the zero, which is
	 *	what it should return in the child). We really should fix
	 *	that but in the short run, we detect the child by subtracting
         *	the pid of the parent from what rfork returns. XXX
	 */

	
	if(!nodes){
		int	my_pid = getpid();
		int	rnode = node;
		
		printf("my_pid is %d and my ppid is %d\n", my_pid,
			 getppid());
		if ((parent = syscall(281, rnode) /* rfork(node) */) < 0) {
			printf("Can't rfork\n");
			exit(1);
		}else if (!parent /* (parent - my_pid) == 0 */) {/* process is a child */
			printf("parent in child is %d\n", parent);
			msg_test_generator(msg_test_snd_name, actions, count);
			exit(1);
		}else if (parent != my_pid){
			printf("parent in parent is %d\n", parent); 
			/*
		 	 *	The parent will call msg_test_receiver 
		 	 *	after rfork'ing its child.
		 	 */
			msg_test_receiver();
			printf("\n\tUser space message test completed\n");
			exit(1);
		}
	}

	/* 
	 *	For dipc-level testing, use fork to create as many
	 *	sender processes as are requested.
	 */

	for(i = 0; ((parent) && (i < nodes)); i++){
		if ((parent = fork()) < 0) {
			printf("Can't fork\n");
			exit(1);
		}
		else if (!parent) {/* process is a child */
			msg_test_generator(msg_test_snd_name, actions, count);
			exit(1);
		}
		else if ((i == (nodes - 1)) && parent){ 
			/*
			 *	The parent will call msg_test_receiver 
			 *	after fork'ing all its children.
			 */
			msg_test_receiver();
			printf("\n\tUser space message test completed\n");
			exit(1);

		}
	}
}

#endif	/* KERNEL_TEST */

/*
 *	The following defines are used when calling the function
 *	destroy_message_contents.  The function needs to know if it is
 *	being called from the node sending a message or the receiving node.
 *	This is because the sending node has the option of using the
 *	automagic feature of mach_msg_overwite to deallocate the pages
 *	in OOL regions. If the deallocation feature is used, there is no
 *	need to try to deallocate those pages in destroy_message_contents.
 */

#define RECEIVE_SIDE FALSE
#define SEND_SIDE TRUE

/*
 *	Hang out listening for incoming messages.
 *	Respond to every one that has a reply port.
 */
void
msg_test_receiver(void)
{
	msg_layout_t		*msg, *receive_msg;
	kern_return_t		kr;
	mach_msg_return_t	mr;
	mach_port_t		overwrite_port, overwrite_port_set;
	boolean_t		overwrite;
	vm_size_t		receive_size, receive_limit;
	char			*func = "msg_test_receiver";
	int			sender_procs_completed = 0;

#if	KERNEL_TEST

	assert(current_task() == msg_test_task_rcv);
	assert(current_space() == msg_test_task_rcv->itk_space);

#endif	/* KERNEL_TEST */

	/*
	 *	Allocate a buffer large enough to hold the
	 *	biggest inline message we'll ever construct.
	 */
	kr = vm_allocate(TASK_OR_MAP, (vm_offset_t *) &msg,
			 (vm_size_t)MAX_MSG_SIZE, TRUE);
	if (kr != KERN_SUCCESS) {
		printf("%s:  vm_allocate failed, kr = 0x%x \n", func, kr);
		assert(FALSE);
	}

	/*
	 *	Create a port set to use when receiving overwrite
	 *	messages.
	 */

	kr = mach_port_allocate(TASK_OR_SPACE, MACH_PORT_RIGHT_PORT_SET,
			&overwrite_port_set);
 	if (kr != KERN_SUCCESS) {
		printf("%s:  mach_port_allocate fails, kr = %d\n", func, kr);
		assert(FALSE);
	}

	for (;;) {

		msg->h.msgh_local_port = msg_test_rcv_name;
		msg->h.msgh_size = MAX_MSG_SIZE;
#if	KERNEL_TEST

		mr = mach_msg_overwrite(&msg->h, MACH_RCV_MSG, 0,
					msg->h.msgh_size,
					msg->h.msgh_local_port,
					MACH_MSG_TIMEOUT_NONE,
					MACH_PORT_NULL, 0, 0);
		if (mr != MACH_MSG_SUCCESS) {
			printf("%s:  mach_msg_overwrite(1) returns 0x%x\n", func, mr);

			assert(FALSE);
			continue;
		}

#else	/* KERNEL_TEST */
		mr = mach_msg(&msg->h, MACH_RCV_MSG, 0, MAX_MSG_SIZE,
				msg_test_rcv_name, MACH_MSG_TIMEOUT_NONE,
				MACH_PORT_NULL);
		if (mr != MACH_MSG_SUCCESS) {
			printf("%s:  mach_msg(1) returns 0x%x\n", func, mr);
			assert(FALSE);
			continue;
		}
#endif	/* KERNEL_TEST */
		/*
		 *	This can be set up (msg_test_receiver_break = 1) so
		 *	that after each test message is received, the test
		 *	thread drops into the debugger so the message can be 
		 *	inspected.  msg_test_receiver_break can be unset so
		 *	that the  message test suite can be run without
		 *	the need to intervene manually on the receiving node.
 		 */

		if (msg_test_receiver_break) {
			printf("\nBREAKING FOR RECEIVE SIDE INSPECTION, msg=0x%x\n", msg);
			assert(FALSE);
		}

		/*
		 *	We check to see if we have a  message requesting the
		 *	creation of an overwrite port for use in conjunction
		 *	with receiving messages that use the overwrite option
		 */
		overwrite = FALSE; /* Initialize for the default case */
		if(msg->h.msgh_id == CREATE_OVERWRITE_PORT_ID){
		
			/*
			 *	Create a special "overwrite" port and
			 *	convey send rights to it to the node waiting
			 *	to dispatch the overwrite message. Add it
			 *	to the port set to be used for receiving
			 *	overwrite messages.
			 */
			send_overwrite_right(msg, &overwrite_port,
				overwrite_port_set);
			continue;
		} 
		/*
		 *	We now check to see if we have a precursor message,
		 *	i.e., one which indicates that there is a pending
		 *	message that is to be received with the overwrite
		 *	option. If we do have a precursor message, the
		 *	overwrite message has been marshalled on the sending
		 *	node and is waiting to be sent.
		 */
		else if(msg->h.msgh_id == PRECURSOR_MSG_ID){
			/*
			 *	Use message parameters to allocate buffers
			 *	for OOL regions and insert OOL message
			 *	descriptors in the receive buffer.
			 */

			receive_msg = overwrite_buffer_alloc(msg,
				&receive_size);

			/*
			 *	Acknowledge the precursor message with an
			 *	"I-am-ready-to-receive" the overwrite message.
			 */

			reply_to_precursor(msg);
			/*
			 *	 We must add the size of the trailer
			 *	to get the receive message buffer size.
			 *	XXX Need to clarify the story on trailers. XXX
			 */
			/*
			 *	Note:
			 *	sizeof(mach_msg_format_0_trailer_t) = 20
			 */

			/* printf("%s: return from overwrite_buffer_alloc with
receive_size of %d\n", func, receive_size); */
			receive_limit = receive_size + 4 +/* Alignment fudge */
					sizeof(mach_msg_format_0_trailer_t);
			receive_msg->h.msgh_local_port = overwrite_port_set;

			/*
			 *	Pick up the message using the overwrite
			 *	facility.
			 */
			mr = mach_msg_overwrite(0,
					MACH_RCV_MSG|MACH_RCV_OVERWRITE, 0,
					receive_limit,
					receive_msg->h.msgh_local_port,
					MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL,
					&receive_msg->h, receive_size );
			if (mr != MACH_MSG_SUCCESS) {
				printf("%s:  mach_msg_overwrite(2) returns 0x%x\n", func, mr);
				if(mr == MACH_RCV_TOO_LARGE)
					printf("%s: receive_limit is %d while message size is %d\n", func, receive_limit, receive_msg->h.msgh_size);				
				assert(FALSE);
				continue;
			}

			if(receive_msg->h.msgh_id != MSG_TEST_OVERWRITE_ID){
				printf("%s: problem, expecting an overwrite message with msg_id %d, got one with msg_id %d\n", func, MSG_TEST_OVERWRITE_ID, receive_msg->h.msgh_id);
#if	USER_SPACE
			exit(1);
#else	/* USER_SPACE */
			assert(FALSE);
#endif	/* USER_SPACE */

			}
			overwrite = TRUE;
#if	USER_SPACE
		}else if (msg->h.msgh_id == SENDER_PROC_MSG_COMPLETION_ID){
			if(++sender_procs_completed == numb_sender_procs)			
				return; /* Senders are done! */
			continue;
#endif	/* USER_SPACE */

		}else if (msg->h.msgh_id != MSG_TEST_SEND_ID){
			printf("%s: Unrecognized message with msg_id %d\n",
				func,  msg->h.msgh_id);
			continue;
		}

		/*
	 	 *	After receiving a message, the receiver validates
		 *	the message, optionally checks that the parameters of
		 *	the sent message line up with those of the received
		 *	message, optionally sends back a reply and optionally
		 *	destroys the message contents.
	 	 */ 

		received_message_processing(overwrite ? receive_msg : msg);

		/*
 		 *	Deallocate the buffer used to receive the entire 
		 *	overwrite message.
 		 */
		if(overwrite){
			kr = vm_deallocate(TASK_OR_MAP, (vm_offset_t) 
				receive_msg, receive_size);
			if (kr != KERN_SUCCESS) {
				printf("%s:  vm_deallocate failed, kr = 0x%x \n", func, kr);
#if	USER_SPACE
			exit(1);
#else	/* USER_SPACE */
			assert(FALSE);
#endif	/* USER_SPACE */
			}
		}
	}

}


/*
 *	Message test generator output banner. 
 */
#define	BANNER_LINES	7
#if	 KERNEL_TEST
char	*banner[BANNER_LINES] = {
	"\t Table following shows ranges of message components by pass\n\n",
	"\t\t\t\t\tL = low (else number is high)\n",
	"\tKey for Flags:\t\t\tD = deallocate (else don't auto dealloc)\n",
	"\t\t\t\t\tP = physical (else virtual copy)\n",
	"\t\t\t\t\tO = overwrite (else dynamically alloc')\n",
	"\t\t\t  INLINE\t\t\tOUT_OF_LINE\n",
" Pass  Nmsgs    Ibytes   Isnds  Ircvs Flags-Oregs     Obytes   Osnds  Orcvs \n"
};
#else	/* KERNEL_TEST */
char	*banner[BANNER_LINES] = {
	"\t Table following shows ranges of message components by pass\n\n",
	"\t\t\t\t\tL = low (else number is high)\n",
	"\tKey for Flags:\t\t\tD = deallocate (else don't auto dealloc)\n",
	"\t\t\t\t\tP = physical (else virtual copy)\n",
	"\t\t\t\t\tO = overwrite (else dynamically alloc')\n",
	"\t\t\t  INLINE\t\t\tOUT_OF_LINE\n",
"Node Pass  Nmsgs    Ibytes   Isnds  Ircvs Flags-Oregs     Obytes   Osnds  Orcvs \n"
};
#endif	/* KERNEL_TEST */


char	*reply_str[2] = {
	"xmission okay  ",
	"xmission errors"
};

/*
 *	Send a range of messages with inline and/or out-of-line data with
 *	varying lengths as well as optional inline and OOL port rights
 *	as specified by the message table. The actual lengths of inline data
 *	regions sent must meet the constraints of the kernel, viz., they must
 *	be aligned on natural word boundaries.
 *
 *	The test can be set up so that we break (i.e., drop into the debugger)
 *	after commencing each iteration (i.e., pass) and/or after each test
 *	message is generated but before it is sent to the remote node.
 *	The complete message test suite can also be run without the need for
 *	manual intervention. The table below shows the implication of setting
 *	msg_test_generator_break at various values.
 *
 *	msg_test_generator_break			action
 *      		0			don't break on send-side
 *			1			break at start of iteration
 *			2			break after each message
 *			3			break for iteration and message
 */

#define BREAK_FOR_EACH_ITERATION 1
#define BREAK_FOR_EACH_MESSAGE 2

void
msg_test_generator(
	mach_port_t	send_name,
	msg_test_action	*actions,
	int		count)
{
	kern_return_t	kr;
	msg_layout_t	*msg;
	msg_layout_t	*reply;
	msg_test_action	*act;
	int		i, node;
	int		act_oregs_init; /* table entry value for act->oregs */
	unsigned int	numb_oregs; /* the numeric part of "act->oregs" */
	int	start;
	int 	*cur_ptr, *reply_ptr; /* Used to walk reply; compare tallies */
	unsigned int	pass, nmsgs;
	unsigned int	param_count; /*Number of message parameters .....*/
	unsigned int	loop_cnt;	/*Temporary variable */
	unsigned int	inline_msg_size; /* Inline message size */
	char		flags_oregs[5];
	char		*func = "msg_test_generator";
	boolean_t	deallocate;
	boolean_t	overwrite_right = FALSE; /* will be true when we get 
				send rights to overwrite port */
	boolean_t	overwrite; /* true if pass sends overwrite messages */
	mach_port_t	overwrite_send_r; /* send right to overwrite port */
	mach_port_t	name_server_p; /* send right to name server port */
	msg_params_t	current, *cur; /* current message parameters */
	mach_msg_copy_options_t	copy;

	/* Allocate a msg buffer */
	kr = vm_allocate(TASK_OR_MAP, (vm_offset_t *) &reply,
			 (vm_size_t)MAX_MSG_SIZE, TRUE);
	if (kr != KERN_SUCCESS) {
		printf("%s: vm_allocate problem kr is 0x%x\n", func, kr);
#if	USER_SPACE
		exit(1);
#else	/* USER_SPACE */
		assert(FALSE);
#endif	/* USER_SPACE */
        }


#if	USER_SPACE

	/*
	 *	Create a port to pick up replies (to the original message) on. 
	 */
	kr = mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE,
				&msg_test_reply_name);
	if(kr != KERN_SUCCESS) {
		printf("%s: mach_port_allocate problem kr is 0x%x\n",func, kr);
		exit(1);
	}

	/*
	 *	It is useful to know which node we are running on.
 	 *	Note that norma_port_location hint expects a send right.
	 */

	kr = mach_port_insert_right(mach_task_self(), msg_test_reply_name,
			msg_test_reply_name, MACH_MSG_TYPE_MAKE_SEND);
	if(kr != KERN_SUCCESS) {
		printf("%s: mach_port_insert_right problem kr is 0x%x\n", func, kr);
		exit(1);
	}

	kr = norma_port_location_hint(mach_task_self(), msg_test_reply_name,
			&node);
	if(kr != KERN_SUCCESS) {
		printf("%s: norma_port_location_hint problem, kr is 0x%x\n", func, kr);
		exit(1);
	}
	printf("%s is running on node %d\n",func, node);

        /*
         *	Get a send right to the name server running on the boot
	 *	node by first looking up the pertinent port using the string
	 *	"message_test_port". If the lookup fails, the name server is
	 *	not running on the boot node and we will exit, otherwise
	 *	we will end up with a send right to the name server on the
	 *	boot node.
         */

	/* printf("%s: recv_node = %d\n", func, recv_node); */

	kr = norma_get_nameserver_port(mach_host_priv_self(), boot_node,
		&name_server_p);
	if (kr != NETNAME_SUCCESS) {
		printf("%s: norma_get_nameserver_port problem: kr = %d or 0x%x\n", func, kr, kr);
		exit(1);
	}

	kr = netname_look_up(name_server_p,  "", "message_test_port",
		&send_name);
	if (kr != NETNAME_SUCCESS) {
		printf("%s: netname_look_up problem kr is %d\n", func, kr);
		exit(1);
	}

#endif	/* USER_SPACE */

	/* Adjust the start and end passes if appropriate */

	if (msg_test_start == -1)
	    start = 0;
	else
	    start = msg_test_start;

	if (msg_test_end != -1)
	    count = msg_test_end + 1;

	assert((msg_test_start >= -1) && (msg_test_end >= -1));

	if(msg_test_sender_verbose)
		printf("%s:  invoked for pass %d thru pass %d\n\n", func,
			((msg_test_start == -1) ? 0 : msg_test_start),
			((msg_test_end == -1) ? count - 1 : msg_test_end));

	if(msg_component_verbose){

		/* Pretty print the message component header information */

		for (i = 0; i < BANNER_LINES; i++)
			printf("%s\n", banner[i]);
	}
	
	/* Perform the stipulated tests for each pass */
	loop_cnt = 0; /*Temporary counter for debugging purposes */
	for (pass = start; pass < count; ++pass) {

		act = &actions[pass];
 
		/*
		 *	Decouple "oregs" into the flags component
		 *	and the number component.
		 */  
		strcpy(flags_oregs, "    "); /* clear flags_oregs */
		if(!decouple_entry(act->oregs, &numb_oregs, flags_oregs)){
			printf("Problem deciphering ""oregs"" table entry\n");
			assert(FALSE);
		}
		if(msg_component_verbose){
			/* Pretty print the message component information */
#if	USER_SPACE
			printf("%3d ", node);
#endif	/* USER_SPACE */
			printf("%5d %6d %10d %6d ",
				pass, /* (act->reply ? "<->":"->"), */
				act->nmsgs, act->ibytes, act->isnds);
			printf("%6d  %4s%6d", act->ircvs, flags_oregs, numb_oregs);
			printf("%11d  %6d %6d\n", act->obytes, act->osnds, act->orcvs);
		}

		if (msg_test_generator_break & BREAK_FOR_EACH_ITERATION) {
			printf("\nBREAKING FOR ITERATION\n");
			assert(FALSE);
		}

		/*
		 *	Use the [DPO] flags that are set in act->oregs to
		 *	select the appropriate options....(The overwrite
		 *	option will be tested in user space because
		 *	mach_msg_overwrite in the kernel does not have
		 *	the overwrite option.The physical copy option will
		 *	also be tested in user space.)
		 */

		deallocate = (act->oregs & DEALLOCATE_BIT);
		copy = ((act->oregs & PHYSICAL_BIT)? MACH_MSG_PHYSICAL_COPY:
					MACH_MSG_VIRTUAL_COPY);
		overwrite = (act->oregs & OVERWRITE_BIT);

		/*
		 *	At this point, we know if we will be sending
		 *	overwrite messages during this pass. If we will, and
		 *	if we haven't sent any overwrite messages during
		 *	previous passes, we send a message to the receiver
		 *	to get it to create an overwrite port and convey to us
		 *	send rights to that port in the reply message.
		 */

		if(overwrite && !overwrite_right){
			acquire_overwrite_right(send_name, &overwrite_send_r);
			overwrite_right = TRUE;
		}

		/*
		 *	Store the actual table value "act->oregs" so we can
		 *	restore the message test table to its original state
		 *	at the end of this pass.
		 */
		act_oregs_init = act->oregs;

		/* 
		 *	Determine the oregs numerical limit, giving it the
		 *	pertinent sign depending on whether the "L" flag 
		 *	is set or not. 
		 */
		act->oregs = ((act->oregs & LOW_BIT) ? -numb_oregs :
		 			numb_oregs);

		/*	Get the parameters for first message in this pass. */

		cur = &current;

		generate_message_parameters(act, msg_incr, cur, TRUE);


		/* 
		 *	Within each pass we generate the requisite number
		 *	of messages by varying the size of the inline
		 *	or OOL regions; the number of inline send and 
		 *	receive rights and the size of the OOL port arrays.
		 */

		for (nmsgs = 0; nmsgs < act->nmsgs; ++nmsgs) {

			/*
			 *	The following message will be sent to the
			 *	overwrite port if it is to be received with
			 *	the overwrite option otherwise it will go
			 *	to the regular message port we acquired from
			 *	the nameserver.
			 */

			msg = generate_message((overwrite ? overwrite_send_r: 
				send_name), cur, act, deallocate, overwrite,
				copy);

			inline_msg_size = msg->h.msgh_size;

			if(msg_test_generator_break & BREAK_FOR_EACH_MESSAGE){
				printf("\nBREAKING FOR SEND SIDE MESSAGE INSPECTION, msg=0x%x\n", msg);
				assert(FALSE);
			}
			/*
			 *	If the overwrite option is on, send a
			 *	precursor message with proper id and the
			 *	OOL message parameters and pick up the
			 *	reply to that message.
			 */

			if(overwrite)
				send_precursor_message(send_name, cur);
				
			/*
			 * At last, send the actual message.
			 */

			kr = mach_msg_overwrite(&msg->h, MACH_SEND_MSG,
						msg->h.msgh_size,
						0, MACH_PORT_NULL,
						MACH_MSG_TIMEOUT_NONE,
						MACH_PORT_NULL, 0, 0);
			if (kr != MACH_MSG_SUCCESS) {
				printf("%s:  mach_msg_overwrite:  msg 0x%x kr 0x%x\n",
				       func, msg, kr);
				assert(FALSE);
				goto loop;
			}

			/*
			 *	We're not done yet....  if the message had
			 *	any port rights in it, we've got to walk
			 *	along the message doing some other cute things.
			 */
			if (msg->h.msgh_bits & MACH_MSGH_BITS_COMPLEX)
				sent_message_processing(msg);

			/*
			 *	If no reply is needed, bail.
			 */
			if (!act->reply)
				goto loop;

			/*
			 *	Await reply.
			 */

			reply->h.msgh_size = MAX_MSG_SIZE;
			reply->h.msgh_local_port = msg->h.msgh_local_port;

			kr = mach_msg_overwrite(&reply->h, MACH_RCV_MSG, 0,
						reply->h.msgh_size,
						reply->h.msgh_local_port,
						MACH_MSG_TIMEOUT_NONE,
						MACH_PORT_NULL,
						0, 0);
			if (kr != MACH_MSG_SUCCESS) {
				printf("ack failed, kr = 0x%x\n", kr);
				assert(FALSE);
				goto loop;
			}

			if (reply->h.msgh_id != RECEIVER_MSG_ID) {
				printf("strange reply 0x%x\n", reply);
				assert(FALSE);
			}

			/*	Check that receiver returns XMISSION_OK 
			 *	indicating that all is well with the dipc
			 *	"microcosm". If we get back the value:
			 *	XMISSION_ERRORS, we know that there 
			 *	were validation errors in the message
			 *	received on the remote node. If we get
			 *	anything else in that field, it's time to
			 *	turn all the debugging options on! Next
			 *	iteration will enhance "granularity" of
			 *	response in returned value to indicate:
			 *	"inline data errors", "ool data errors",
			 *	"inline send and/or receive right errors". 
			 */
	
			reply_ptr = (int *)(&(reply->b));
			if(*reply_ptr == (int) XMISSION_ERRORS){
				printf("\t\t\tremote verification error!\n");
				if (msg_test_generator_break) {
					assert(FALSE);
				}
			} else if (*reply_ptr != (int) XMISSION_OK){
				printf("\t\t\tunknown reply!\n");
				assert(FALSE);
			}

			/* Check the message tally counts if requested */
			if(msg_test_receiver_sync){
				reply_ptr++;
				cur_ptr = &(cur->ibytes); 
				for(i = 0; i < nparams; i++){
					if(*reply_ptr++ != *cur_ptr++){
						printf("%s: mismatch in message parameter tally.\n%s:  sent msg has %d, received msg has %d\n", func, params_strings[i], *(cur_ptr - 1), *(reply_ptr - 1));
						assert(FALSE); 
					}
				}
			}

		    loop:
			destroy_message_contents(msg, SEND_SIDE);
			kr = vm_deallocate(TASK_OR_MAP,
					   (vm_offset_t) msg, inline_msg_size);
			assert(kr == KERN_SUCCESS);

			/* 
			 *	We now get the parameters of the next message
			 */

			generate_message_parameters(act, msg_incr, cur,FALSE);

		}
		/*	Before going to the next pass, we set act->oregs
		 *	in our message table back to its original value.
		 */

		act->oregs = act_oregs_init;	 
	}

	kr = vm_deallocate(TASK_OR_MAP, (vm_offset_t) reply, MAX_MSG_SIZE);
	assert(kr == KERN_SUCCESS);
	/*
	 *	XXX After all tests are completed, send a message to the
	 *	receiver to destroy the overwrite port. To be done....XXX
	 */
#if	USER_SPACE
	printf("\nCompleted %d mandated passes originating on node %d\n", count - start, node);
	send_node_completion_message(send_name);
	return;
#else	/* USER_SPACE */
	printf("\nIn-kernel message test: completed %d mandated passes \n", count - start);
	return;
#endif	/* USER_SPACE */
}

/*
 *	Use the following macro to determine the inital values of the
 *	message components.
 */

#define initial(x) (x > 0 ? 1 : -x)

/*
 *	Within each pass we compute the message parameters for each individual  
 *	message from the information given in the "message action table". 
 */

void
generate_message_parameters(
	msg_test_action	*act,
	int		*msg_incr,
	msg_params_t	*cur,
	boolean_t	init)
{
	int	i;
	int	*param;
	int	*table;
	char	*func = "generate_message_parameters";

	/*
	 *	Within each pass, we generate the first message by
	 *	initializing the size and or number of the various
	 *	message primitives based on information from the "message
	 *	action table". Succeeding messages are generated by
	 *	adding specified increments to the various parameters.
	 */

	param = &cur->ibytes;
	table = &act->ibytes;
	if(msg_test_sender_verbose)
		printf("%s: cur->ibytes = %d, act->ibytes = %d\n",func, *param, *table);
	if(init){
		for(i = 0; i < param_count; i++, param++, table++)
			*param = initial(*table);
	}else{

		for(i = 0; i < param_count; i++, param++, table++, msg_incr++)
			*param = next_value(*param, *table, *msg_incr);
	}

	/*
 	 *	Untyped IPC restricts inline data to be word-aligned.
	 *	Note that the check below permits a message to contain
	 *	slightly more data than specified by the action table
	 *	when the high_data parameter isn't aligned.
	 */

	if (cur->ibytes & 3)
		cur->ibytes = (cur->ibytes + 3) & ~3;

	if(msg_test_sender_verbose)
		printf("%s: \t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",func,
			cur->ibytes, cur->isnds, cur->ircvs, cur->oregs,
			cur->obytes, cur->osnds, cur->orcvs);

}

/* Alternate between the transmittal of sends and send-once's */
#define toggle(x) x = ! x 

/*
 *	The message structure is stocked with pertinent information.
 */ 
msg_layout_t
*generate_message(
	mach_port_t	send_name,
	msg_params_t	*cur,
	msg_test_action	*act,
	boolean_t	deallocate,
	boolean_t	overwrite,
	mach_msg_copy_options_t	copy)
{
	kern_return_t	kr;
	unsigned int	numb_msg_desc;	/* Number of message descriptors */
	unsigned int	inline_msg_size; /* Inline message size */
	msg_layout_t	*msg;
	char		*inline_data;
	int		desc_count;
	int		loop_oregs;
	int		i;
	boolean_t	null_regions; /* Are there zero-length OOL regions? */
	boolean_t	send_once;
	mach_msg_port_descriptor_t	*pd;
	mach_msg_ool_descriptor_t	*od;
	mach_msg_ool_ports_descriptor_t	*opd;
	char				*func = "generate_message";

	numb_msg_desc = cur->isnds + cur->ircvs + cur->oregs; 
	if(msg_test_sender_verbose){                
		printf("%s: \t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",
			func, cur->ibytes, cur->isnds,
			cur->ircvs, cur->oregs, cur->obytes, 
			cur->osnds, cur->orcvs, numb_msg_desc);
	}


	/*
	 *	Compute the buffer size for the entire inline
	 *	msg consisting of the header, the inline data
	 *	bytes and the inline message descriptors.
	 */

	inline_msg_size = cur->ibytes + (numb_msg_desc == 0 ?
			sizeof(mach_msg_header_t) :
			sizeof(msg_layout_t) + (numb_msg_desc - 1) * 
			sizeof(mach_msg_descriptor_t));

	/*
	 *	Allocate a buffer for the entire inline msg.
	 */
	kr = vm_allocate(TASK_OR_MAP, (vm_offset_t *)&msg,
			inline_msg_size, TRUE);
	if (kr != KERN_SUCCESS) {
		printf("%s:  vm_allocate failed, kr = 0x%x \n", func, kr);
		assert(FALSE);
	}

	/*
	 *	Set up destination and (optional) reply port.
	 */
			
	msg->h.msgh_remote_port = send_name;
	if (act->reply) {
		msg->h.msgh_local_port = msg_test_reply_name;
		msg->h.msgh_bits = MACH_MSGH_BITS( MACH_MSG_TYPE_COPY_SEND, 
						MACH_MSG_TYPE_MAKE_SEND);
	}else{
		msg->h.msgh_bits = MACH_MSGH_BITS( MACH_MSG_TYPE_COPY_SEND, 0);
	}
	if (numb_msg_desc > 0)
		msg->h.msgh_bits |= MACH_MSGH_BITS_COMPLEX;

	if(overwrite)
		msg->h.msgh_id = MSG_TEST_OVERWRITE_ID;
	else
		msg->h.msgh_id = MSG_TEST_SEND_ID;

	msg->h.msgh_size = inline_msg_size;
	if (numb_msg_desc > 0)
		msg->b.msgh_descriptor_count = numb_msg_desc;

	/*
	 *	Primitive:  for each message component that has a non zero
	 *	count and is conveyed using a message descriptor, a loop is
	 *	used to fill in the descriptor structure.
	 */
	
	/*
	 *	Fill in (optional) inline data bytes, using a verifiable
	 *	pattern.
	 */
	if (cur->ibytes > 0) {
		if (numb_msg_desc > 0)
			inline_data = (char *) &msg->d[numb_msg_desc];
		else
			inline_data = (char *) &msg->b;
		fillin_msg_data(cur->ibytes, (vm_address_t)inline_data);
	}
	/* 
	 *	Next, we handle inline send rights if any.
	 */ 
	desc_count = 0; /*...count numb of descriptors */
	for(i = 0; i < (cur->isnds + 1)/2; i++){

		/* first insert a send right.... */
		pd = (mach_msg_port_descriptor_t *) &msg->d[desc_count++];
		create_port_right(MACH_MSG_TYPE_MAKE_SEND, pd);

		if(desc_count == cur->isnds)
			 break;

		/* then insert a send-once right.... */
		pd = (mach_msg_port_descriptor_t *) &msg->d[desc_count++];
		create_port_right(MACH_MSG_TYPE_MAKE_SEND_ONCE,pd);

	}
	/* 
	 *	Next, we handle inline receive rights if any.
	 */ 
	for(i = 0; i < cur->ircvs; i++, desc_count++){
		pd = (mach_msg_port_descriptor_t *) &msg->d[desc_count];
		create_port_right(MACH_MSG_TYPE_MOVE_RECEIVE, pd);
	}


	/* 
	 *	Now we handle OOL regions if any. The OOL regions are
	 *	distributed in round-robin fashion among OOL data regions,
	 *	port arrays of send rights and  port arrays of receive rights.
	 *	
	 *	Notes:
	 *
	 *	(1)We will transmit zero length OOL regions if cur->oregs > 0
	 *	and cur->obytes, cur->osnds and cur->orcvs equal zero
	 *	simultaneously.
	 * 
	 *	(2) When the message specification includes send right arrays,
 	 *	we will transmit arrays with send rights on one round-robin
 	 *	cycle and arrays with send-once rights on the following 
 	 *	round-robin cycle as we rotate through the various OOL region
	 *	types.
 	 *	Thus if one pass is:
 	 *
	 *	  OOL data , OOL send right array, OOL receive right array.
 	 *
	 *	the following will be (subject to OOL region availability):
	 *
	 *	  OOL data , OOL send-once right array, OOL receive right array
	 */
	loop_oregs = cur->oregs; /* ...count-down variable */
	cur->n_odata_reg = 0; /* count of OOL data regions */
	cur->n_osndr_arr = 0; /* count OOL send right arrays */
	cur->n_osnd1_arr = 0; /* count OOL send-once arrays */
	cur->n_orcvs_arr = 0; /* count OOL rcv right arrays */

	null_regions = ((cur->obytes == 0) && (cur->osnds == 0)
					&& (cur->orcvs == 0));

	send_once = FALSE; /* for toggling between send and send-once rights */
	while(loop_oregs > 0){
		if((cur->obytes > 0) || null_regions){
			/* OOL data regions */
			od = (mach_msg_ool_descriptor_t *) &msg->d[desc_count];
			fillin_ool_data(cur->obytes, od, deallocate, copy);
			cur->n_odata_reg++;
			desc_count++;
			if(--loop_oregs == 0)
				break;
		}
		if((cur->osnds > 0) || null_regions){
			/* OOL port array for send rights */
			opd = (mach_msg_ool_ports_descriptor_t *)
				&msg->d[desc_count];
			if(!send_once){
				fillin_ool_port_array(cur->osnds, opd,
					deallocate, copy,
					MACH_MSG_TYPE_MAKE_SEND);
				cur->n_osndr_arr++;
			}else{
				fillin_ool_port_array( cur->osnds, opd,
					deallocate, copy, 
					MACH_MSG_TYPE_MAKE_SEND_ONCE);
				cur->n_osnd1_arr++;
			}
			toggle(send_once);
			desc_count++;
			if(--loop_oregs == 0)
				break;
		}
		if((cur->orcvs > 0) || null_regions){
			/* OOL port array for receive rights */
			opd = (mach_msg_ool_ports_descriptor_t *)
				&msg->d[desc_count];
			fillin_ool_port_array(cur->orcvs, opd, deallocate,
				copy, MACH_MSG_TYPE_MOVE_RECEIVE);
			cur->n_orcvs_arr++;
			desc_count++;
			if(--loop_oregs == 0)
				break;
		}
	}
	assert(desc_count == numb_msg_desc);
	assert(desc_count == (cur->isnds + cur->ircvs + cur->n_odata_reg +
		cur->n_osndr_arr + cur->n_osnd1_arr+ cur->n_orcvs_arr));

	/*
	 *	At this point we adjust for those cases where we would
	 *	have allocated certain types OOL regions because one or
	 *	more of "cur->obytes", "cur->osnds" or "cur->orcvs" is
	 *	postitive but there are not enough OOL regions to go
	 *	around.
	 */
	if(cur->n_odata_reg == 0 && cur->obytes != 0){
		cur->obytes = 0;
		if(msg_test_sender_verbose)
			printf("%s: made adjustment to cur->obytes.\n", func);
	}
	if(cur->n_osndr_arr == 0 && cur->osnds != 0){
		cur->osnds = 0;
		if(msg_test_sender_verbose)
			printf("%s: made adjustment to cur->osnds.\n", func);
	}
	if(cur->n_orcvs_arr == 0 && cur->orcvs != 0){
		cur->orcvs = 0;
		if(msg_test_sender_verbose)
			printf("%s: made adjustment to cur->orcvs.\n", func);
	}

	if (msg_test_sender_verbose) {
		printf("\t...msg 0x%x %s",msg, (act->reply ? "Ping" : "One-way" ));
		printf(" inline(bytes=%d,s=%d,r=%d)\n", cur->ibytes,
			cur->isnds, cur->ircvs);
		printf(" \t.....OOL[%d data * %dbytes, ", cur->n_odata_reg,
			cur->obytes);
		printf("%d send right arrays(sr=%d), %d send-once arrays(s1=%d), %d receive right arrays(r=%d)]\n",cur->n_osndr_arr, cur->osnds,
			cur->n_osnd1_arr, cur->osnds, cur->n_orcvs_arr,
			cur->orcvs);					
	} 
	return msg;

}

boolean_t
validate_message_header(
	msg_layout_t	*msg)
{
	boolean_t	complex, complex_ool;
	char		*func = "validate_message_header";

	assert((msg->h.msgh_bits&MACH_MSGH_BITS_DIPC_FORMAT)==0);
	assert((msg->h.msgh_bits&MACH_MSGH_BITS_META_KMSG)==0);
	assert((msg->h.msgh_bits&MACH_MSGH_BITS_MIGRATING)==0);
	assert((msg->h.msgh_bits&MACH_MSGH_BITS_PLACEHOLDER)==0);

	if ((msg->h.msgh_id != MSG_TEST_SEND_ID) &&
		(msg->h.msgh_id != MSG_TEST_OVERWRITE_ID)) {
		printf("%s:  strange message 0x%x with id %d\n",
		       func, msg, msg->h.msgh_id);
		assert(FALSE);
	}

	complex = msg->h.msgh_bits & MACH_MSGH_BITS_COMPLEX;

#if	KERNEL_TEST

	complex_ool = msg->h.msgh_bits & MACH_MSGH_BITS_COMPLEX_OOL;
	assert(complex_ool ? complex : TRUE);

#endif /* KERNEL_TEST */

	return TRUE;
}


boolean_t
validate_port_right(
	mach_msg_type_name_t	disposition,
	mach_port_t		port)
{
	char		*func = "validate_port_right";

	switch (disposition) {
	    case MACH_MSG_TYPE_PORT_NAME:
		printf("\t%s:  disposition MACH_MSG_TYPE_PORT_NAME is bogus!\n", func);
		assert(FALSE);
		return FALSE;

	    case MACH_MSG_TYPE_PORT_SEND_ONCE:
	    case MACH_MSG_TYPE_PORT_SEND:
	    case MACH_MSG_TYPE_PORT_RECEIVE:
		/*
		 *	For now, just pretend that all incoming ports
		 *	are OK.  I suppose we could do additional work,
		 *	such as checking the reference counts, etc.
		 *	Later, if ever.
		 */
		break;

	    default:
		printf("\t%s:  bogus disposition %d\n", func, disposition);
		assert(FALSE);
		return FALSE;
	}
	return TRUE;
}


boolean_t
validate_message_descriptors(
	msg_layout_t	*msg)
{
	mach_msg_descriptor_t		*mmd;
	mach_msg_ool_descriptor_t	*od;
	mach_msg_ool_ports_descriptor_t	*opd;
	vm_size_t			ool_size;
	char				*ool_data;
	int				cnt, i, k, nports;
	mach_port_t			*mpp;
	char				*func = "validate_message_descriptors";

	if (!(msg->h.msgh_bits & MACH_MSGH_BITS_COMPLEX))
		return TRUE;

	mmd = (mach_msg_descriptor_t *) &msg->d[0];
	cnt = (int) msg->b.msgh_descriptor_count;

	for (i = 0; i < cnt; i++, mmd++){
		switch (mmd->type.type) {

		    case MACH_MSG_PORT_DESCRIPTOR:
			if (validate_port_right(mmd->port.disposition,
						mmd->port.name) == FALSE)
				return FALSE;
			break;

		    case MACH_MSG_OOL_VOLATILE_DESCRIPTOR:
		    case MACH_MSG_OOL_DESCRIPTOR:
			od = (mach_msg_ool_descriptor_t *) &msg->d[i];
			ool_data = (char *) od->address;
			ool_size = (vm_size_t) od->size;
			if (validate_data(msg, ool_data, ool_size) == FALSE) {
				printf("\t%s, desc %d:  ool region contents bogus\n",
				       func, i);
				assert(FALSE);
				return FALSE;
			}
			break;

		    case MACH_MSG_OOL_PORTS_DESCRIPTOR:
			opd = (mach_msg_ool_ports_descriptor_t *) &msg->d[i];
			nports = (int) opd->count;
			mpp = (mach_port_t *) mmd->ool_ports.address;
			assert(mmd->ool_ports.address == opd->address);
			for(k = 0; k < nports; k++, mpp++){
				if (validate_port_right(
					mmd->ool_ports.disposition, *mpp) ==
					FALSE)

					return FALSE;
			}
			break;

		    default:
			printf("\t%s, desc %d:  unrecognized descriptor type %d\n",
			       func, i, mmd->type.type);
			assert(FALSE);
			return FALSE;
		}
	}
	return TRUE;
}

void
tally_message_parameters(
	msg_layout_t	*msg,
	msg_params_t	*msg_params)
{
	mach_msg_descriptor_t	*mmd;
	int			cnt, i;
	mach_port_t		*mpp;
	char			*func = "tally_message_parameters";
	int			isndr, isnd1; /* tally various send r types*/
	int			*params_field_ptr;

	/*
	 *	Initialize the message parameter structure
	 */


	params_field_ptr = &msg_params->ibytes;
	for (i = 0; i < nparams; i++, params_field_ptr++)
		*params_field_ptr = 0;	

	if (!(msg->h.msgh_bits & MACH_MSGH_BITS_COMPLEX)){
	
		msg_params->ibytes = (int) (msg->h.msgh_size -
					sizeof(mach_msg_header_t));
		if (msg_test_sender_verbose)
			printf("%s: simple msg:  msg_params->ibytes is %d\n",
				func, msg_params->ibytes);
		return;
	}else{
		msg_params->ibytes = (int) (msg->h.msgh_size -
				((char *) &msg->d[msg->b.msgh_descriptor_count] 
				- (char *) msg));
		if (msg_test_sender_verbose)
			printf("%s: cmplx msg: msg_params->ibytes is %d\n",
				func, msg_params->ibytes);
	}
	mmd = (mach_msg_descriptor_t *) &msg->d[0];
	cnt = (int) msg->b.msgh_descriptor_count;

	if (msg_test_sender_verbose)
		printf("%s: descriptor count is %d\n",func, cnt);
	/*
	 *	Walk the message tallying up the descriptor field entries.
	 */
	isnd1 = 0; /* initialize send-once right count */
	isndr = 0; /* initialize send right count */
	for (i = 0; i < cnt; i++, mmd++){
		switch (mmd->type.type) {
			case MACH_MSG_PORT_DESCRIPTOR:
				switch(mmd->port.disposition){

					case MACH_MSG_TYPE_PORT_SEND_ONCE:
						isnd1++;
						msg_params->isnds++;
						break;
	    				case MACH_MSG_TYPE_PORT_SEND:
						isndr++;
						msg_params->isnds++;
						break;
	    				case MACH_MSG_TYPE_PORT_RECEIVE:
						msg_params->ircvs++;
						break;
					default:
						printf("\t%s:  bogus disposition %d\n", func, mmd->port.disposition);
						assert(FALSE);
				}
				break;

			case MACH_MSG_OOL_VOLATILE_DESCRIPTOR:
			case MACH_MSG_OOL_DESCRIPTOR:
				if((msg_params->n_odata_reg) > 0){
					if((int)(mmd->out_of_line.size) != 
						msg_params->obytes){
						printf("\t%s, desc %d:  ool region sizes should be same for each region in a given message\n",func, i);
printf("mmd->out_of_line.size = %d, msg_params->obytes = %d \n", mmd->out_of_line.size, msg_params->obytes);
						assert(FALSE);
					}
				}else if(msg_params->n_odata_reg == 0)
					/* Set OOL region size */
					msg_params->obytes = (int)
						mmd->out_of_line.size;
				else{
					printf("%s: msg_params->n_odata_reg < 0\n", func);
					assert(FALSE);
				}
				(msg_params->n_odata_reg)++;	
				break;

			case MACH_MSG_OOL_PORTS_DESCRIPTOR:
				switch(mmd->ool_ports.disposition){
					case MACH_MSG_TYPE_PORT_SEND_ONCE:
					    if((msg_params->n_osnd1_arr) > 0){
					        if((int)mmd->ool_ports.count !=
						    msg_params->osnds){
					            printf("\t%s, desc %d:  ool port array count should be same for each type of OOL right in a given message\n",func, i);
					            assert(FALSE);
					         }
 					    }else if(msg_params->n_osnd1_arr ==
							0){
					        /* Set OOL port array size for sends*/
					        msg_params->osnds = (int)
					        mmd->ool_ports.count;
					    }
					    (msg_params->n_osnd1_arr)++;

					    break;
	    				case MACH_MSG_TYPE_PORT_SEND:
					    if((msg_params->n_osndr_arr) > 0){
					        if((int)mmd->ool_ports.count !=
						    msg_params->osnds){
					            printf("\t%s, desc %d:  ool port array count should be same for each type of OOL right in a given message\n",func, i);
					            assert(FALSE);
					         }
 					    }else if(msg_params->n_osndr_arr ==
							0){
					        /* Set OOL port array size for sends*/
					        msg_params->osnds = (int)
					        mmd->ool_ports.count;
					    }
					    (msg_params->n_osndr_arr)++;
					    break;

	    				case MACH_MSG_TYPE_PORT_RECEIVE:
					    if((msg_params->n_orcvs_arr) > 0){
					        if((int)mmd->ool_ports.count != 
					             msg_params->orcvs){
					             printf("\t%s, desc %d:  ool port array count should be same for each type of OOL right in a given message\n",func, i);
					             assert(FALSE);
					        }
					    }else if(msg_params->n_orcvs_arr ==
								0){
					     /* Set OOL port array size for receives*/
					        msg_params->orcvs = (int)
					        mmd->ool_ports.count;
					    }
					    (msg_params->n_orcvs_arr)++;
					    break;

	    				case MACH_MSG_TYPE_MAKE_SEND:
					    if((int)mmd->ool_ports.count != 0){
						/* We have a problem */
						printf("\t%s: Oops!: should never get OOL port arrays of disposition mach_msg_type_make_send with count= %d\n", func, (int)mmd->ool_ports.count );
						assert(FALSE);
					    }else{
						/* XXX Fudging: Check for null arrays. We do this because when a null OOL send right array is sent via  MACH_MSG_TYPE_MAKE_SEND, its disposition field does not get translated to MACH_MSG_TYPE_PORT_SEND on reception XXX*/
						(msg_params-> n_osndr_arr )++;
					    }
					    break;
	    				case MACH_MSG_TYPE_MAKE_SEND_ONCE:
					    if((int)mmd->ool_ports.count != 0){
						/* We have a problem */
						printf("\t%s: Oops!: should never get OOL port arrays of disposition mach_msg_type_make_send_once with count= %d\n", func, (int)mmd->ool_ports.count);
						assert(FALSE);
					    }else{
						/* XXX Fudging! When a null OOL send-once array is sent via MACH_MSG_TYPE_MAKE_SEND_ONCE, its disposition field does not get translated to MACH_MSG_TYPE_PORT_SEND_ONCE on reception XXX*/

						(msg_params-> n_osnd1_arr )++;
					    }
					    break;
					default: 
					    	printf("\t%s:  bogus disposition %d\n", func, mmd->ool_ports.disposition);
					    	assert(FALSE);
			}
			break;
		   	default:
				printf("\t%s, desc %d:  unrecognized descriptor type %d\n", func, i, mmd->type.type);			
				assert(FALSE);
		}
	}
	if (msg_test_sender_verbose)
	/* if(msg_params->oregs > 0 || msg_params->n_odata_reg > 0) */
		printf(" before leaving tally.... msg_params->n_odata_reg = %d, msg_params->n_osndr_arr  = %d msg_params->n_osnd1_arr = %d msg_params->n_orcvs_arr\n", msg_params->n_odata_reg, msg_params->n_osndr_arr, msg_params->n_osnd1_arr, msg_params->n_orcvs_arr);

	msg_params->oregs = msg_params->n_odata_reg + msg_params->n_osndr_arr
		+ msg_params->n_osnd1_arr + msg_params->n_orcvs_arr;

}


boolean_t
validate_data(
	msg_layout_t	*msg,
	char		*bytes,
	vm_size_t	size)
{
	register unsigned char	*str;
	register int	i;
	int		errors, nulls;
	char		*func = "validate_data";

	/*
	 *	Validate the contents of a message buffer.
	 *	Message buffers are filled with a repeating
	 *	ASCII sequence.
	 */

	errors = 0;
	nulls = 0;
	str = (unsigned char *) bytes;
	for (i = 0; i < size; i++, str++) {
		if (*str != ('0' + (i & 63))){
			errors++;
			if(*str == '\0')
				nulls++;
			if(msg_validation_verbose)
				if(errors <  ERR_MAX )
					printf("%s: data error found at byte %d, sent = %c recv'd = %c \n",func, i, ('0' + (i & 63)), *str);
				else if(errors == ERR_MAX)
					printf("%s: error count exceeds max (%d),\n\t\t verbose error reporting now turned off\n\n",func, ERR_MAX);
		}
	}
	if(msg_validation_verbose && errors > 0)
		printf("%s: total error count is %d, of which %d are nulls\n", func, errors, nulls);
	return errors == 0;
}


/*
 * Fill in a buffer with a recognizable pattern; in this
 * case, a subset of the printable ASCII collating sequence.
 * The sequence is repeated as many times as is required to
 * fill the buffer.
 */

void
fillin_msg_data(
	int		msgsize,
	vm_address_t	data_ptr)
{
	register char	*str;
	register int	i;

	str = (char *)data_ptr;
	for (i = 0; i < msgsize; i++, str++) {
		*str = '0' + (i & 63);
	}

}

/*
 * Thus function allocates the buffer for the OOL data region and
 * calls the appropriate function to fill it with data.
 * The message descriptor is updated with the pertinent information.
 */
void
fillin_ool_data(
	int 				region_size,
	mach_msg_ool_descriptor_t	*ool_descriptor,
	boolean_t			deallocate,
	mach_msg_copy_options_t		copy)
{
	kern_return_t	kr;
	vm_offset_t	obuf;
	char		*func = "fillin_ool_data";
	/*
	 *	Fill in ool data.  This requires allocating another
	 *	buffer. It will be deallocated automagically if the
	 *	deallocate option is set, otherwise it will be
	 *	vm_deallocated in routine: destroy_message_contents.
	 */

	assert(region_size >= 0);
	kr = vm_allocate(TASK_OR_MAP, (vm_offset_t *) &obuf,
			(vm_size_t) region_size, TRUE);
	if (kr != KERN_SUCCESS) {
		printf("%s:  vm_allocate, kr=0x%x\n", func, kr);
		assert(FALSE);
	}
	fillin_msg_data(region_size, (vm_address_t) obuf);
	ool_descriptor->address = (void *) obuf;
	ool_descriptor->size = region_size;
	ool_descriptor->deallocate = deallocate;
	ool_descriptor->copy = copy;
	ool_descriptor->type = MACH_MSG_OOL_DESCRIPTOR;
}

/*
 * Thus function allocates the buffer for the OOL port arrays and
 * calls the appropriate function to fill it with port rights.
 * The message descriptor is updated with the pertinent information.
 */

void				
fillin_ool_port_array(
	int				array_size,
	mach_msg_ool_ports_descriptor_t *ool_ports_descriptor,
	boolean_t			deallocate,
	mach_msg_copy_options_t		copy,
	mach_msg_type_name_t		disposition)
{
	kern_return_t	kr;
	vm_offset_t	obuf;
	vm_size_t	buf_size;
	char		*func = "fillin_ool_port_array";
	/*
	 *	Fill in ool port right array. This requires allocating
	 *	another buffer. It will be deallocated automagically if
	 *	the deallocate option is set, otherwise it will be
	 *	vm_deallocated in destroy_message_contents.
	 */
	assert(array_size >= 0);

	buf_size = array_size * sizeof(mach_port_t);
	kr = vm_allocate(TASK_OR_MAP, (vm_offset_t *) &obuf,
				(vm_size_t) buf_size, TRUE);
	if (kr != KERN_SUCCESS) {
		printf("%s:  vm_allocate, kr=0x%x\n", func, kr);
		assert(FALSE);
	}
	ool_ports_descriptor->address = (void *) obuf;
	ool_ports_descriptor->count = array_size;
	ool_ports_descriptor->deallocate = deallocate;
	ool_ports_descriptor->copy = copy;
	ool_ports_descriptor->disposition = disposition;
	ool_ports_descriptor->type = MACH_MSG_OOL_PORTS_DESCRIPTOR;
	create_port_right_array(ool_ports_descriptor);
}

void
respond_to_port_right(
	mach_msg_type_name_t	disposition,
	mach_port_t		port)
{
	unsigned int	j;
	char		*func = "respond_to_port_right";

	switch (disposition) {
	    case MACH_MSG_TYPE_PORT_SEND_ONCE:
	    case MACH_MSG_TYPE_PORT_SEND:

		send_simple_message(disposition, port);
		if(msg_port_processing_verbose)
			printf("%s: (SRM) Sending a simple message to port 0x%x\n",func, port);
		break;
	    case MACH_MSG_TYPE_PORT_RECEIVE:
		for (j = 0; j < nmsgs_stk; j++){
			if(msg_port_processing_verbose)
				printf("%s: (RRM) Receiving a simple message from port 0x%x\n",func, port);
			(void) receive_simple_message(port);
		}
		break;

	    default:
		printf("\t%s:  bogus disposition %d\n", func, disposition);
		assert(FALSE);
	}
}


void
receiver_responses(
	msg_layout_t	*msg)
{
	mach_msg_descriptor_t		*mmd;
	mach_msg_ool_descriptor_t	*od;
	mach_msg_ool_ports_descriptor_t	*opd;
	vm_size_t			ool_size;
	char				*ool_data;
	int				cnt, i, k, nports;
	mach_port_t			*mpp;
	char				*func = "receiver responses";

	if (!(msg->h.msgh_bits & MACH_MSGH_BITS_COMPLEX))
		return;

	mmd = (mach_msg_descriptor_t *) &msg->d[0];
	cnt = (int) msg->b.msgh_descriptor_count;

	for (i = 0; i < cnt; i++, mmd++){
		switch (mmd->type.type) {

		    case MACH_MSG_PORT_DESCRIPTOR:
			respond_to_port_right(mmd->port.disposition, mmd->port.name);
			break;

		    case MACH_MSG_OOL_VOLATILE_DESCRIPTOR:
		    case MACH_MSG_OOL_DESCRIPTOR:
			/* do nothing for the moment */
			break;

		    case MACH_MSG_OOL_PORTS_DESCRIPTOR:
			opd = (mach_msg_ool_ports_descriptor_t *) &msg->d[i];
			nports = (int) opd->count;
			mpp = (mach_port_t *) mmd->ool_ports.address;
			assert(mmd->ool_ports.address == opd->address);
			for(k = 0; k < nports; k++, mpp++){
				respond_to_port_right(
					mmd->ool_ports.disposition, *mpp);

			}

			break;

		    default:
			printf("\t%s, desc %d:  unrecognized descriptor type %d\n",
			       func, i, mmd->type.type);
			assert(FALSE);
		}
	}
}


/*
 *	Free up resources acquired on message reception.
 *
 *	By setting msg_test_leak, we can blithely leak all
 *	acquired OOL regions and port rights, causing the
 *	system to degrade in an interesting and repeatable way.
 */
void
destroy_message_contents(
	msg_layout_t	*msg,
	boolean_t	send_side)
{
	kern_return_t			kr;
	mach_msg_descriptor_t		*mmd;
	mach_msg_ool_descriptor_t	*od;
	mach_msg_ool_ports_descriptor_t	*opd;
	vm_size_t			ool_size;
	vm_offset_t			*ool_data;
	int				cnt, i;
	char				*func = "destroy_message_contents";

	if (!(msg->h.msgh_bits & MACH_MSGH_BITS_COMPLEX))
		return;

	mmd = (mach_msg_descriptor_t *) &msg->d[0];
	cnt = (int) msg->b.msgh_descriptor_count;

	for (i = 0; i < cnt; i++, mmd++){
		switch (mmd->type.type) {
		    case MACH_MSG_PORT_DESCRIPTOR:
			if (msg_test_leak == TRUE) {
				++msg_test_leak_port_count;
				break;
			}
			(void) mach_port_destroy(TASK_OR_SPACE, mmd->port.name);
			break;

		    case MACH_MSG_OOL_VOLATILE_DESCRIPTOR:
		    case MACH_MSG_OOL_DESCRIPTOR:
			od = (mach_msg_ool_descriptor_t *) &msg->d[i];
			ool_data = (vm_offset_t *) od->address;
			ool_size = (vm_size_t) od->size;

			if (msg_test_leak == TRUE) {
				++msg_test_leak_ool_region_count;
				msg_test_leak_ool_region_size += ool_size;
				break;
			}
			/*
			 *	The OOL descriptor has a "deallocate"
			 *	option which can be set so that the
			 *	OOL pages are deallocated (as if by
			 *	vm_deallocate) after the message is sent.
			 *	If that option is turned on, there is
			 *	no need to "vm_deallocate" the region (on 
			 *	the sending node).
			 *
			 */
			
			if(send_side && od->deallocate){
				if(msg_test_sender_verbose)
					printf("OOL data region automagically deallocated\n");
				break;
			}
			kr = vm_deallocate(TASK_OR_MAP,
					   (vm_offset_t)ool_data,
					   ool_size);
			assert(kr == KERN_SUCCESS);
			break;

		    case MACH_MSG_OOL_PORTS_DESCRIPTOR:
			opd = (mach_msg_ool_ports_descriptor_t *) &msg->d[i];
			ool_data = (vm_offset_t *) opd->address;
			ool_size = (vm_size_t) opd->count *
					sizeof(mach_port_t);

			if (msg_test_leak == TRUE) {
				++msg_test_leak_ool_region_count;
				msg_test_leak_ool_region_size += ool_size;
				break;
			}
			/*
			 *	The OOL PORTS descriptor also has a 
			 *	"deallocate" option which can be set so
			 *	that the pages containing the OOL port arrays
			 *	are delloacted after the message is sent.
			 *	If that option is turned on, there is
			 *	no need to "vm_deallocate" the region (on 
			 *	the sending node).
			 *
			 */
			
			if(send_side && opd->deallocate){
				if(msg_test_sender_verbose)
					printf("OOL ARRAY automagically deallocated\n");
				break;
			}

			kr = vm_deallocate(TASK_OR_MAP,
					   (vm_offset_t) ool_data,
					   ool_size);
			assert(kr == KERN_SUCCESS);
			break;

		    default:
			printf("\t%s, desc %d:  unrecognized descriptor type %d\n",
			       func, i, mmd->type.type);
			assert(FALSE);
			break;
		}
	}
}


/*
 *	This routine is used to send the precursor messages, to indicate
 *	that a message that is to be received with the overwrite option
 *	has been marshalled on the sending node and is waiting to
 *	be sent. We also pick up the acknowledgement (reply) from the
 *	receiving node.
 */

void
send_precursor_message(
	mach_port_t		send_name,
	msg_params_t		*cur)
{
	kern_return_t		kr;
	mach_msg_return_t	mr;
	msg_layout_t		*msg;
	vm_size_t		msg_size;
	int			i, *cur_ptr, *msg_ptr;
	char			*func = "send_precursor_message";
   
	/*
	 *	We allocate enough space in the message for the header and
	 *	the entire message parameter structure (msg_params_t).
	 */
	msg_size = sizeof(mach_msg_header_t) + sizeof(msg_params_t);
				 
	kr = vm_allocate(TASK_OR_MAP, (vm_offset_t *) &msg, msg_size, TRUE);
	if (kr != KERN_SUCCESS) {
		printf("%s:  vm_allocate failed, kr = 0x%x \n", func, kr);
		assert(FALSE);
	}

	/*
	 *	Set up message header.
	 */
	msg->h.msgh_remote_port = send_name;
	msg->h.msgh_local_port = msg_test_reply_name;
	msg->h.msgh_bits = MACH_MSGH_BITS( MACH_MSG_TYPE_COPY_SEND, 
						MACH_MSG_TYPE_MAKE_SEND);
	msg->h.msgh_id = PRECURSOR_MSG_ID;
	msg->h.msgh_size = msg_size;

	/*
	 *	Insert message parameters into message and send it off.
	 */
    	msg_ptr = (int *)(&(msg->b));
	cur_ptr = (int *)(&(cur->ibytes));
	for(i = 0; i < nparams; i++)
		*msg_ptr++ = *cur_ptr++; 
	
	mr = mach_msg_overwrite(&msg->h, MACH_SEND_MSG, msg->h.msgh_size, 0,
				MACH_PORT_NULL, MACH_MSG_TIMEOUT_NONE,
				MACH_PORT_NULL,0, 0);
	if (mr != MACH_MSG_SUCCESS) {
		printf("\t%s:  failed  on message send, msg 0x%x mr 0x%x\n",
		       func, &msg->h, mr);
		assert(FALSE);
        }

	/*	Pick up the reply to the message from the receiver. */

	msg->h.msgh_size = sizeof(mach_msg_header_t) +
			sizeof(mach_msg_trailer_t);
   	mr = mach_msg_overwrite(&msg->h, MACH_RCV_MSG, 0, msg->h.msgh_size,
				msg_test_reply_name , MACH_MSG_TIMEOUT_NONE,
				MACH_PORT_NULL, 0, 0);
	if (mr != MACH_MSG_SUCCESS) {
		printf("\t%s:  message receive failed, msg 0x%x mr 0x%x\n",
		       func, &msg->h, mr);
		assert(FALSE);
	}

	if(msg->h.msgh_id != REPLY_TO_PRECURSOR_ID){
		printf("%s: problem, received msg_id %d != expected msg_id %d\n", func,  msg->h.msgh_id, REPLY_TO_PRECURSOR_ID);
#if	USER_SPACE
		exit(1);
#endif	/* USER_SPACE */
	}

	kr = vm_deallocate(TASK_OR_MAP, (vm_offset_t) msg, msg_size);
	if (kr != KERN_SUCCESS) {
		printf("%s:  vm_allocate failed, kr = 0x%x \n", func, kr);
		assert(FALSE);
	}
}

/*
 *	This routine is used to send the node completion messages, to
 *	alert the receiver to the fact that a node has finished sending
 *	its prescribed sequence of messages. 
 */

void
send_node_completion_message(
	mach_port_t		send_name)
{
	kern_return_t		kr;
	mach_msg_return_t	mr;
	msg_layout_t		*msg;
	vm_size_t		msg_size;
	char			*func = "send_node_completion_message";
   
	/*
	 *	We allocate enough space for a message header. 
	 */
	msg_size = sizeof(mach_msg_header_t);
				 
	kr = vm_allocate(TASK_OR_MAP, (vm_offset_t *) &msg, msg_size, TRUE);
	if (kr != KERN_SUCCESS) {
		printf("%s:  vm_allocate failed, kr = 0x%x \n", func, kr);
		assert(FALSE);
	}

	/*
	 *	Set up message header.
	 */
	msg->h.msgh_remote_port = send_name;
	msg->h.msgh_local_port = MACH_PORT_NULL;
	msg->h.msgh_bits = MACH_MSGH_BITS( MACH_MSG_TYPE_COPY_SEND, 0);
	msg->h.msgh_id = SENDER_PROC_MSG_COMPLETION_ID;
	msg->h.msgh_size = msg_size;
	mr = mach_msg_overwrite(&msg->h, MACH_SEND_MSG, msg->h.msgh_size, 0,
				MACH_PORT_NULL, MACH_MSG_TIMEOUT_NONE,
				MACH_PORT_NULL,0, 0);
	if (mr != MACH_MSG_SUCCESS) {
		printf("\t%s:  failed  on message send, msg 0x%x mr 0x%x\n",
		       func, &msg->h, mr);
		assert(FALSE);
        }

	kr = vm_deallocate(TASK_OR_MAP, (vm_offset_t) msg, msg_size);
	if (kr != KERN_SUCCESS) {
		printf("%s:  vm_allocate failed, kr = 0x%x \n", func, kr);
		assert(FALSE);
	}
}

/*	This routine sends the reply to the precursor messsage to the
 *	sender waiting to send the overwrite message. The reply is the 
 *	go ahead for the overwrite message to be sent.
 */

void
reply_to_precursor(			
	msg_layout_t		*precursor_msg)
{
	mach_msg_return_t	mr;
	kern_return_t		kr;
	msg_layout_t		*msg;
	vm_size_t		msg_size;
	char			*func = "reply_to_precursor";

	/*
	 *	We allocate enough space in the message for the header.
	 */
	msg_size = sizeof(mach_msg_header_t);
				 
	kr = vm_allocate(TASK_OR_MAP, (vm_offset_t *) &msg, msg_size, TRUE);
	if (kr != KERN_SUCCESS) {
		printf("%s:  vm_allocate failed, kr = 0x%x \n", func, kr);
		assert(FALSE);
	}

	/*
	 *	Set up message header. Do I really need to give it send
	 *	rights to this port? No! It has a permanent copy of the
	 *	right for the overwrite port. We reuse the buffer used
	 *	to receive the precursor message.
	 *
	 */

	msg->h.msgh_local_port = MACH_PORT_NULL;
	msg->h.msgh_remote_port = precursor_msg->h.msgh_remote_port;
	msg->h.msgh_bits = MACH_MSGH_BITS( MACH_MSG_TYPE_MOVE_SEND, 0);
	msg->h.msgh_id = REPLY_TO_PRECURSOR_ID;
	msg->h.msgh_size = sizeof(mach_msg_header_t);

	mr = mach_msg_overwrite(&msg->h, MACH_SEND_MSG, msg->h.msgh_size, 0,
				MACH_PORT_NULL, MACH_MSG_TIMEOUT_NONE,
				MACH_PORT_NULL,0, 0);
	if (mr != MACH_MSG_SUCCESS) {
		printf("\t%s:  failed  on message send, msg 0x%x mr 0x%x\n",
		       func, &msg->h, mr);
		assert(FALSE);
        }

	kr = vm_deallocate(TASK_OR_MAP, (vm_offset_t) msg, msg_size);
	if (kr != KERN_SUCCESS) {
		printf("%s:  vm_allocate failed, kr = 0x%x \n", func, kr);
		assert(FALSE);
	}

}

/*
 *	This routine uses the OOL message parameters conveyed in the
 *	precursor message to allocate buffers for the OOL regions and
 *	generates the receive buffer with the necessary OOL descriptors.
 *	A pointer to the receive buffer is returned
 */

msg_layout_t
*overwrite_buffer_alloc(msg_layout_t	*precursor_msg, /* precursor message */
			 vm_size_t	*msg_size_ptr /* buffer size ptr*/)
{
	kern_return_t	kr;
	msg_layout_t	*msg;
	int		i, *msg_ptr, *cur_ptr, *t_cur_ptr;
	int		desc_count; /* running decscriptor count */
	int		numb_msg_desc; /* number of message descriptors */
	int		loop_oregs;
	boolean_t	null_regions, send_once;
	msg_params_t	current, *cur;
	msg_params_t	tallied_current, *t_cur;
	mach_msg_ool_descriptor_t	*od;
	mach_msg_ool_ports_descriptor_t	*opd;
	char				*func = "*overwrite_buffer_alloc";

/*
 *	Put message parameters into msg_params_t structure.
 */	
    	msg_ptr = (int *)(&(precursor_msg->b));
	cur_ptr = (int *)(&current);
	for(i = 0; i < nparams; i++)
		*cur_ptr++ = *msg_ptr++ ; 

	cur = &current; 
/*	
 *	cur->oregs       is total number of OOL regions
 *	cur->n_odata_reg is count of OOL data regions 
 *	cur->n_osndr_arr is count of OOL send right arrays 
 *	cur->n_osnd1_arr is count of OOL send-once arrays 
 *	cur->n_orcvs_arr is count of OOL rcv right arrays
 */

	if(cur->oregs != cur->n_odata_reg + cur->n_osndr_arr +
		cur->n_osnd1_arr + cur->n_orcvs_arr){
		printf("%s: number of OOL regions does not add up\n", func);
		printf("\t # of OOL regions is %d, sum of regions is %d\n",
			cur->oregs, (cur->n_odata_reg + cur->n_osndr_arr +
		cur->n_osnd1_arr + cur->n_orcvs_arr));
#if	USER_SPACE
		exit(1);
#else	/* USER_SPACE */
		assert(FALSE);
#endif	/* USER_SPACE */
	}
/*
 *	Compute the buffer size for the entire overwrite "receive"
 *	msg consisting of the header, descriptor count, the
 *	OOL region and inline descriptors and the inline data
 */

	numb_msg_desc = cur->isnds + cur->ircvs + cur->oregs; 

	*msg_size_ptr =  cur->ibytes + (numb_msg_desc == 0 ?
			sizeof(mach_msg_header_t) :
			sizeof(msg_layout_t) + (numb_msg_desc - 1) * 
			sizeof(mach_msg_descriptor_t));

	if(msg_test_receiver_verbose){
		printf("%s: number of message descriptors = %d\n", func,
			numb_msg_desc);
		printf("%s: allocated buffer size is %d\n", func,
			*msg_size_ptr);
	} 

/*
 *	Allocate a buffer for the entire overwrite receive msg.
 */
	kr = vm_allocate(TASK_OR_MAP, (vm_offset_t *)&msg, *msg_size_ptr,
				TRUE);
	if (kr != KERN_SUCCESS) {
		printf("%s:  vm_allocate failed, kr = 0x%x \n", func, kr);
		assert(FALSE);
	}

/*
 *	Fill in the entire overwrite receive msg.
 */
	
	msg->b.msgh_descriptor_count = (mach_msg_size_t) numb_msg_desc;
	if(msg_test_receiver_verbose){
		printf("%s: precursor message indicates %d OOL regions\n",
			func, cur->oregs);
		printf(" \t.....OOL[%d data * %dbytes, ", cur->n_odata_reg,
			cur->obytes);
		printf("%d send right arrays(sr=%d), %d send-once arrays(s1=%d), %d receive right arrays(r=%d)]\n",cur->n_osndr_arr, cur->osnds,
			cur->n_osnd1_arr, cur->osnds, cur->n_orcvs_arr,
			cur->orcvs);					
	}

	/*
	 *	 Fill in the descriptors and allocate buffers for:
	 *		OOL data regions
	 *		OOL port arrays for send rights
	 *		OOL port array for send-once rights
	 *		OOL port array for receive rights
	 *	Since the overwrite option needs to find the descriptors
	 *	in the scatter buffer in the same order as in the message
	 *	sent, we will use the same algorithm as that used to stock
	 *	the message:
	 */

	desc_count = 0;
	loop_oregs = cur->oregs; /* ...count-down variable */
	null_regions = ((cur->obytes == 0) && (cur->osnds == 0)
					&& (cur->orcvs == 0));
	send_once = FALSE; /* for toggling between send and send-once rights */

	t_cur = &tallied_current; /* Keep track of counts */
	t_cur->n_odata_reg = 0; /* count of OOL data regions */
	t_cur->n_osndr_arr = 0; /* count OOL send right arrays */
	t_cur->n_osnd1_arr = 0; /* count OOL send-once arrays */
	t_cur->n_orcvs_arr = 0; /* count OOL rcv right arrays */

	while(loop_oregs > 0){
		if((cur->obytes > 0) || null_regions){
			/* OOL data regions */
			od = (mach_msg_ool_descriptor_t *) &msg->d[desc_count];
			alloc_ool_data_buf(cur->obytes, od);
			t_cur->n_odata_reg++;
			desc_count++;
			if(--loop_oregs == 0)
				break;
		}
		if((cur->osnds > 0) || null_regions){
			/* OOL port array for send rights */
			opd = (mach_msg_ool_ports_descriptor_t *)
				&msg->d[desc_count];
			if(!send_once){
			alloc_ool_port_array_buf(cur->osnds, opd,
				MACH_MSG_TYPE_MAKE_SEND);
				t_cur->n_osndr_arr++;
			}else{
			alloc_ool_port_array_buf(cur->osnds, opd,
				MACH_MSG_TYPE_MAKE_SEND_ONCE);
				t_cur->n_osnd1_arr++;
			}
			toggle(send_once);
			desc_count++;
			if(--loop_oregs == 0)
				break;
		}
		if((cur->orcvs > 0) || null_regions){
			/* OOL port array for receive rights */
			opd = (mach_msg_ool_ports_descriptor_t *)
				&msg->d[desc_count];
			alloc_ool_port_array_buf(cur->orcvs, opd,
				MACH_MSG_TYPE_MOVE_RECEIVE);
			t_cur->n_orcvs_arr++;
			desc_count++;
			if(--loop_oregs == 0)
				break;
		}
	}
	/* Check that the tallied counts match the parameter counts */
	
	cur_ptr = &(cur->n_odata_reg);
	t_cur_ptr = &(t_cur->n_odata_reg); 
	for(i = 0; i < 4; i++){
		if(*t_cur_ptr++ != *cur_ptr++){
			printf("%s: mismatch in message parameter.\n%s:  sent msg has %d, tallied msg has %d\n", func, params_strings[i + 7], *(cur_ptr - 1), *(t_cur_ptr - 1));
#if	USER_SPACE
			exit(1);
#endif	/* USER_SPACE */
		}
	}

	return msg;
}

void
alloc_ool_data_buf(
	int 				region_size,
	mach_msg_ool_descriptor_t	*ool_descriptor)
{
	kern_return_t	kr;
	vm_offset_t	obuf;
	char		*func = "alloc_ool_data_buf";
	/*
	 *	Allocate a buffer for an ool data region.  Need to
	 *	work on deallocation later.
	 */

	kr = vm_allocate(TASK_OR_MAP, (vm_offset_t *) &obuf,
			(vm_size_t) region_size, TRUE);
	if (kr != KERN_SUCCESS) {
		printf("%s:  vm_allocate, kr=0x%x\n", func, kr);
		assert(FALSE);
	}
	ool_descriptor->address = (void *) obuf;
	ool_descriptor->size = region_size; /* later on get delivered size */
	ool_descriptor->copy = MACH_MSG_OVERWRITE;
	/* ool_descriptor->copy = MACH_MSG_ALLOCATE;  XXX temporary XXX*/
	ool_descriptor->type = MACH_MSG_OOL_DESCRIPTOR;
	/*	
	 *	Note: After reception, we can check that copy strategy
	 *	used for delivery matches what we asked for on the send-side
	 *	by comparing send-side "copy" value with the post-reception
	 *	value of ool_descriptor->copy.
	 */

}

void				
alloc_ool_port_array_buf(
	int				array_size,
	mach_msg_ool_ports_descriptor_t *ool_ports_descriptor,
	mach_msg_type_name_t		disposition)
{
	kern_return_t	kr;
	vm_offset_t	obuf;
	vm_size_t	buf_size;
	char		*func = "alloc_ool_port_array_buf";
	/*
	 *	Allocate a buffer for an ool port right array. Need to
	 *	work on deallocation later.
	 */

	buf_size = array_size * sizeof(mach_port_t);
	kr = vm_allocate(TASK_OR_MAP, (vm_offset_t *) &obuf,
				(vm_size_t) buf_size, TRUE);
	if (kr != KERN_SUCCESS) {
		printf("%s:  vm_allocate, kr=0x%x\n", func, kr);
		assert(FALSE);
	}
	ool_ports_descriptor->address = (void *) obuf;
	ool_ports_descriptor->count = array_size;
	ool_ports_descriptor->disposition = disposition;
	ool_ports_descriptor->copy = MACH_MSG_OVERWRITE;
	/* ool_ports_descriptor->copy = MACH_MSG_ALLOCATE; XXX temporary XXX*/
	ool_ports_descriptor->type = MACH_MSG_OOL_PORTS_DESCRIPTOR;
	/*	
	 *	Note: After reception we can check that copy strategy
	 *	used for delivery matches what we asked for on the send-side
	 *	by comparing send-side "copy" value with the post-reception
	 *	value of ool_ports_descriptor->copy.
	 */

}

/*
 *	This function sends a message to the receiver requesting the
 *	creation of a special overwrite port and send rights to that
 *	port. The send rights are (hopefully) picked up here.
 */

void
acquire_overwrite_right(
	mach_port_t	remote_right,
	mach_port_t	*overwrite_right)
{
	kern_return_t		kr;
	mach_msg_return_t	mr;
	msg_layout_t		*msg;
	vm_size_t		msg_size;
	char			*func = "acquire_overwrite_right";
   
	msg_size = sizeof(mach_msg_header_t);
				 
	kr = vm_allocate(TASK_OR_MAP, (vm_offset_t *) &msg, msg_size, TRUE);
	if (kr != KERN_SUCCESS) {
		printf("%s:  vm_allocate failed, kr = 0x%x \n", func, kr);
		assert(FALSE);
	}
	/* set up and send the request message */

	msg->h.msgh_remote_port = remote_right;
	msg->h.msgh_local_port = msg_test_reply_name;
	msg->h.msgh_bits = MACH_MSGH_BITS( MACH_MSG_TYPE_COPY_SEND, 
						MACH_MSG_TYPE_MAKE_SEND);
	msg->h.msgh_id = CREATE_OVERWRITE_PORT_ID;
	msg->h.msgh_size = msg_size;
    
	mr = mach_msg_overwrite(&msg->h, MACH_SEND_MSG, msg->h.msgh_size, 0,
				MACH_PORT_NULL, MACH_MSG_TIMEOUT_NONE,
				MACH_PORT_NULL, 0, 0);
	if (mr != MACH_MSG_SUCCESS) {
		printf("\t%s:  failed  on message send, msg 0x%x mr 0x%x\n",
		       func, &msg->h, mr);
#if	USER_SPACE
		exit(1);
#else	/* USER_SPACE */
		assert(FALSE);
#endif	/* USER_SPACE */

        }
	/* pick up the reply with the send rights to the "overwrite" port */

	msg->h.msgh_size = msg_size + sizeof(mach_msg_trailer_t);
   	mr = mach_msg_overwrite(&msg->h, MACH_RCV_MSG, 0, msg->h.msgh_size,
				msg_test_reply_name, MACH_MSG_TIMEOUT_NONE,
				MACH_PORT_NULL, 0, 0);
	if (mr != MACH_MSG_SUCCESS) {
		printf("\t%s:  message receive failed, msg 0x%x mr 0x%x\n",
		       func, &msg->h, mr);
#if	USER_SPACE
		exit(1);
#else	/* USER_SPACE */
		assert(FALSE);
#endif	/* USER_SPACE */

	}
	if(msg->h.msgh_id == REPLY_TO_CREATE_OVERWRITE_PORT_ID)
		*overwrite_right = msg->h.msgh_remote_port;
	else{
		printf("%s: problem, received msg_id %d != expected msg_id %d\n", func,  msg->h.msgh_id, REPLY_TO_CREATE_OVERWRITE_PORT_ID);
#if	USER_SPACE
		exit(1);
#else	/* USER_SPACE */
		assert(FALSE);
#endif /* USER_SPACE */

	}

	kr = vm_deallocate(TASK_OR_MAP, (vm_offset_t) msg, msg_size);
	if (kr != KERN_SUCCESS) {
		printf("%s:  vm_deallocate failed, kr = 0x%x \n", func, kr);
#if	USER_SPACE
		exit(1);
#else	/* USER_SPACE */
		assert(FALSE);
#endif	/* USER_SPACE */

	}
}


/*
 *	This function create a special "overwrite" port and conveys
 *	send rights to it back to the requesting node (with the pending
 *	overwrite message). The port is added to the port set that will
 *	be used to receive overwrite messages.
 */

void
send_overwrite_right(
		msg_layout_t	*msg,
		mach_port_t	*overwrite_port_ptr,
		mach_port_t	overwrite_port_set)
{
	kern_return_t		kr;
	mach_msg_return_t	mr;
	char			*func = "send_overwrite_right";

	kr = mach_port_allocate(TASK_OR_SPACE, MACH_PORT_RIGHT_RECEIVE,
			overwrite_port_ptr);
 	if (kr != KERN_SUCCESS) {
		printf("%s:  mach_port_allocate fails, kr = %d\n", func, kr);
		assert(FALSE);
	}

	/*	Add the port to the "overwrite" port set. */

	kr = mach_port_move_member(TASK_OR_SPACE, *overwrite_port_ptr,
					overwrite_port_set);

	/*
	 *	Set up message header. Use reply port in incoming message. 
	 */

	msg->h.msgh_local_port = *overwrite_port_ptr;
	msg->h.msgh_bits = MACH_MSGH_BITS( MACH_MSG_TYPE_COPY_SEND, 
						MACH_MSG_TYPE_MAKE_SEND);
	msg->h.msgh_id = REPLY_TO_CREATE_OVERWRITE_PORT_ID;
	msg->h.msgh_size = sizeof(mach_msg_header_t);

	mr = mach_msg_overwrite(&msg->h, MACH_SEND_MSG, msg->h.msgh_size, 0,
				MACH_PORT_NULL, MACH_MSG_TIMEOUT_NONE,
				MACH_PORT_NULL,0, 0);
	if (mr != MACH_MSG_SUCCESS) {
		printf("\t%s:  failed  on message send, msg 0x%x mr 0x%x\n",
		       func, &msg->h, mr);
		assert(FALSE);
        }
}


void
send_simple_message(
	mach_msg_type_name_t	disposition,
	mach_port_t		port_name)
{
	kern_return_t		kr;
	mach_msg_return_t	mr;
	msg_layout_t		*msg;
	vm_size_t		msg_size;
	char			*func = "send_simple_message";
   
	msg_size = sizeof(mach_msg_header_t) + simple_msg_str_len;
				 
	kr = vm_allocate(TASK_OR_MAP, (vm_offset_t *) &msg, msg_size, TRUE);
	if (kr != KERN_SUCCESS) {
		printf("%s:  vm_allocate failed, kr = 0x%x \n", func, kr);
		assert(FALSE);
	}

	msg->h.msgh_bits = MACH_MSGH_BITS(disposition, 0);
	msg->h.msgh_local_port = MACH_PORT_NULL;
	msg->h.msgh_id = AUXILIARY_MSG_ID;
	msg->h.msgh_size = msg_size;
	msg->h.msgh_remote_port = port_name;
    
	bcopy(simple_msg_str, (char *)&msg->b, simple_msg_str_len);
	mr = mach_msg_overwrite(&msg->h, MACH_SEND_MSG,
				msg->h.msgh_size, 0, MACH_PORT_NULL,
				MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL,
				0, 0);
	if (mr != MACH_MSG_SUCCESS) {
		printf("\t%s:  failed  on message send, msg 0x%x mr 0x%x\n",
		       func, &msg->h, mr);
		assert(FALSE);
        }

	kr = vm_deallocate(TASK_OR_MAP, (vm_offset_t) msg, msg_size);
	if (kr != KERN_SUCCESS) {
		printf("%s:  vm_deallocate failed, kr = 0x%x \n", func, kr);
		assert(FALSE);
	}
}

boolean_t
receive_simple_message(
	mach_port_t	port_name)
{
	kern_return_t	kr;
	mach_msg_return_t	mr;
	msg_layout_t	*msg;
	vm_size_t	msg_size;
	unsigned int	err_cnt;
	char		*func = "receive_simple_message";

	msg_size = sizeof(mach_msg_header_t) + simple_msg_str_len
		+sizeof(mach_msg_trailer_t);
				 
	kr = vm_allocate(TASK_OR_MAP, (vm_offset_t *) &msg, msg_size, TRUE);
	if (kr != KERN_SUCCESS) {
		printf("%s:  vm_allocate failed, kr = 0x%x \n", func, kr);
		assert(FALSE);
	}

	msg->h.msgh_size = msg_size;
	msg->h.msgh_local_port = port_name;

   	mr = mach_msg_overwrite(&msg->h, MACH_RCV_MSG, 0,
				msg->h.msgh_size, port_name,
				MACH_MSG_TIMEOUT_NONE,
				MACH_PORT_NULL, 0, 0);
	if (mr != MACH_MSG_SUCCESS) {
		printf("\t%s:  message receive failed, msg 0x%x mr 0x%x\n",
		       func, &msg->h, mr);
		assert(FALSE);
	}

	/*
	 *	Validate message ID and expected contents of the inline
	 *	message.
	 */
	if(msg->h.msgh_id != AUXILIARY_MSG_ID){
		printf("\t%s: msg->h.msgh_id != %d\n", AUXILIARY_MSG_ID);
		assert(FALSE);
	}
	err_cnt = bcmp((const char *) &msg->b, 
		       (const char *) simple_msg_str,
		       simple_msg_str_len);
	if (err_cnt != 0) {
		printf("\t%s:  msg 0x%x id %d failed validation\n", func,
		       msg->h.msgh_id, msg);
		assert(FALSE);
	}

	kr = vm_deallocate(TASK_OR_MAP, (vm_offset_t) msg, msg_size);
	if (kr != KERN_SUCCESS) {
		printf("%s:  vm_deallocate failed, kr = 0x%x \n", func, kr);
		assert(FALSE);
	}

	return err_cnt == 0;
}


void
create_port_right(
	mach_msg_type_name_t		type,
	mach_msg_port_descriptor_t	*pd)
{
	kern_return_t	kr;
	mach_port_t	mport, tmp_name;

#if	KERNEL_TEST

	ipc_port_t	port;

#endif	/* KERNEL_TEST */

	unsigned int	j;
	char		*func = "create_port_right";

	/*
	 *	No matter what type is requested, start by
	 *	allocating a new receive right.  We will then
	 *	create the requested type from this right.
	 */

	kr = mach_port_allocate(TASK_OR_SPACE,MACH_PORT_RIGHT_RECEIVE, &mport);
 	if (kr != KERN_SUCCESS) {
		printf("%s:  mach_port_allocate fails, kr = %d\n", func, kr);
		assert(FALSE);
	}
	/*
	 *	Add a send right to the receive right.
	 */
#if	USER_SPACE

 	kr = mach_port_insert_right(mach_task_self(), mport,
 		mport, MACH_MSG_TYPE_MAKE_SEND);
 	if (kr != KERN_SUCCESS) {
		printf("%s:  mach_port_insert_right fails, kr = %d\n",
				       func, kr);
		assert(FALSE);
	}

#else	/* USER_SPACE */
	kr = ipc_object_translate(current_space(), mport,
				MACH_PORT_RIGHT_RECEIVE,
				(ipc_object_t *)&port);
	assert(kr == KERN_SUCCESS);
	
	/*
	 *	ipc_object_translate returns a locked but
	 *	unreferenced port.  Acquire a reference for
	 *	use by this routine...  we will have to
	 *	remember to drop this reference later.
	 */
	ip_reference(port);
	assert(port->ip_references == 2);
	assert(port->ip_srights == 0);
	ip_unlock(port);

	/*
	 *	We've already got a receive right but make
	 *	a send right as well.  If the caller requested
	 *	a receive right, we are going to use this send
	 *	right to stock the receive right with messages,
	 *	just so that the system has to work harder to
	 *	migrate messages or, later, receive messages
	 *	across the wire.
	 *
	 *	If the caller requested some form of send right,
	 *	we'll use the one we're making now.
	 */

	(void) ipc_port_make_send(port);
	tmp_name = ipc_port_copyout_send(port, current_space());
	assert(tmp_name == mport);
#endif	/* USER_SPACE */

	pd->name = mport;
	pd->disposition = type;
	pd->type = MACH_MSG_PORT_DESCRIPTOR;

	switch (type) {
	    case MACH_MSG_TYPE_MOVE_RECEIVE:
		for (j = 0; j < nmsgs_pre; j++){
			if(msg_port_processing_verbose)
				printf("%s: (RRM) Sending a simple message to port 0x%x\n", func, mport);
			send_simple_message(MACH_MSG_TYPE_COPY_SEND, mport);
		}
		break;

	    case MACH_MSG_TYPE_MAKE_SEND:
	    case MACH_MSG_TYPE_MAKE_SEND_ONCE:

		break;

	    case MACH_MSG_TYPE_MOVE_SEND:
	    case MACH_MSG_TYPE_MOVE_SEND_ONCE:
	    case MACH_MSG_TYPE_COPY_SEND:
	    default:
		printf("%s:  can't handle requested type %d\n", type);
		assert(FALSE);
		break;
	}
}
void
create_port_right_array(
	mach_msg_ool_ports_descriptor_t	*opd)
{
	kern_return_t	kr;
	mach_port_t	mport, tmp_name;
	mach_port_t	*mpp; /* mach_port_t pointer */

#if	KERNEL_TEST

	ipc_port_t	port;

#endif	/* KERNEL_TEST */

	unsigned int	i, j;
	char		*func = "create_port_right_array";

	/*
	 *	We create as many ports as requested and place each one in
	 *	the buffer provided starting at the adress passed down to us.
	 */
	mpp = (mach_port_t *) opd->address;
	for(i = 0; i < (int) opd->count; i++, mpp++){

		/*
		 *	No matter what type is requested, start by
	 	 *	allocating a new receive right.  We will then
	 	 *	create the requested type from this right.
	 	 */
		kr = mach_port_allocate(TASK_OR_SPACE, MACH_PORT_RIGHT_RECEIVE,
					&mport);
 		if (kr != KERN_SUCCESS) {
			printf("%s:  mach_port_allocate fails, kr = %d\n",
				func, kr);
			assert(FALSE);
		}
	
		/*
	 	 *	Add a send right to the receive right.
	 	 */
#if	USER_SPACE

 		kr = mach_port_insert_right(mach_task_self(), mport,
 			mport, MACH_MSG_TYPE_MAKE_SEND);
 		if (kr != KERN_SUCCESS) {
			printf("%s:  mach_port_insert_right fails, kr = %d\n",
				       func, kr);
			assert(FALSE);
		}

#else	/* USER_SPACE*/

		kr = ipc_object_translate(current_space(), mport,
					MACH_PORT_RIGHT_RECEIVE,
					(ipc_object_t *) &port);
		assert(kr == KERN_SUCCESS);
	
		/*
	 	 *	ipc_object_translate returns a locked but
	 	 *	unreferenced port.  Acquire a reference for
	 	 *	use by this routine...  we will have to
	 	 *	remember to drop this reference later.
	 	 */
		ip_reference(port);
		assert(port->ip_references == 2);
		assert(port->ip_srights == 0);
		ip_unlock(port);

		/*
	 	 *	We've already got a receive right but make
		 *	a send right as well.  If the caller requested
	 	 *	a receive right, we are going to use this send
	 	 *	right to stock the receive right with messages,
	 	 *	just so that the system has to work harder to
	 	 *	migrate messages or, later, receive messages
	 	 *	across the wire.
	 	 *
	 	 *	If the caller requested some form of send right,
	 	 *	we'll use the one we're making now.
	 	 */

		(void) ipc_port_make_send(port);
		tmp_name = ipc_port_copyout_send(port, current_space());
		assert(tmp_name == mport);
#endif	/* USER_SPACE */

		/*
		 *	Insert the port in the pre-allocated buffer
		 */

		*mpp = mport;
		switch (opd->disposition) {
	    		case MACH_MSG_TYPE_MOVE_RECEIVE:
				for (j = 0; j < nmsgs_pre; j++){
					if(msg_port_processing_verbose)
						printf("%s: (RRM) Sending a simple message to port 0x%x\n", func, mport);
					send_simple_message(
						MACH_MSG_TYPE_COPY_SEND,mport);
				}
				break;

	    		case MACH_MSG_TYPE_MAKE_SEND:
	    		case MACH_MSG_TYPE_MAKE_SEND_ONCE:

				break;

	    		case MACH_MSG_TYPE_MOVE_SEND:
	    		case MACH_MSG_TYPE_MOVE_SEND_ONCE:
	    		case MACH_MSG_TYPE_COPY_SEND:
	    		default:
				printf("%s:  can't handle requested type %d\n", opd->disposition);
				assert(FALSE);
				break;
		}
		if(msg_port_processing_verbose){
			printf("Here is our mach_port_t array:\n");
			printf("port id #%d: 0x%x\n", i, *mpp);
		}
	}
	if(msg_port_processing_verbose){
		printf("Break to compare the above with ipc_port_t space\n" );
		assert(FALSE);
	}
}



/*
 *	After sending a message, the sender may have to do more
 *	to it, just to make life more difficult.  Post additional
 *	messages on migrating receive rights, receive expected
 *	messages on migrating send rights.
 */
void
sent_message_processing(
	msg_layout_t	*msg)
{				
	mach_msg_descriptor_t		*mmd;
	mach_port_t			*mpp;
	unsigned int			cnt, i, j, k, nports;
	char				*func = "sent_message_processing";

	if (!(msg->h.msgh_bits & MACH_MSGH_BITS_COMPLEX))
		return;

	mmd = (mach_msg_descriptor_t *) &msg->d[0];
	cnt = (int) msg->b.msgh_descriptor_count;

	for (i = 0; i < cnt; i++, mmd++){
		if(mmd->type.type == MACH_MSG_PORT_DESCRIPTOR){
			switch (mmd->port.disposition) {

		    		case MACH_MSG_TYPE_MOVE_RECEIVE:
					for (j = 0; j < nmsgs_post; j++){
					if(msg_port_processing_verbose)
						printf("%s (RRM) Sending a simple message to 0x%x\n", func, mmd->port.name);
					send_simple_message(
						MACH_MSG_TYPE_COPY_SEND,
						mmd->port.name);
					}
					break;

		    		case MACH_MSG_TYPE_MAKE_SEND:
	    			case MACH_MSG_TYPE_MAKE_SEND_ONCE:

					/*
					 *	To validate the send rights 
					 *	conveyed by the previously
					 *	sent message, we will try
			 		 *	to receive messages from the
					 *	those ports. We will end up
					 *	blocking here until the
					 *	receiver actually sends those
					 *	messages.
			 		 */
					if(msg_port_processing_verbose)
						printf("%s (SRM) Receiving a simple message from 0x%x\n", func, mmd->port.name);
					(void) receive_simple_message(mmd->port.name);
					break;

		    		default:
					break;
			}
		}

		else if(mmd->type.type == MACH_MSG_OOL_PORTS_DESCRIPTOR){
			nports = (int) mmd->ool_ports.count;
			mpp = (mach_port_t *) mmd->ool_ports.address;
			if (!mpp) break;
			switch (mmd->ool_ports.disposition) {

		    		case MACH_MSG_TYPE_MOVE_RECEIVE:
					for(k = 0; k < nports; k++, mpp++){
				            for (j = 0; j < nmsgs_post; j++){
					       if( msg_port_processing_verbose)
					        printf("%s (RRM) Sending a simple message to 0x%x\n", func, *mpp);
					    send_simple_message( 
					       MACH_MSG_TYPE_COPY_SEND, *mpp);
			                    }
					}
					break;

		    		case MACH_MSG_TYPE_MAKE_SEND:
	    			case MACH_MSG_TYPE_MAKE_SEND_ONCE:

					/*
					 *	To validate the send rights 
					 *	conveyed by the previously
					 *	sent message, we will try
			 		 *	to receive messages from the
					 *	those ports. We will end up
					 *	blocking here until the
					 *	receiver actually sends those
					 *	messages.
			 		 */
					for(k = 0; k < nports; k++, mpp++){
						if(msg_port_processing_verbose)
							printf("%s (SRM) Receiving a simple message from 0x%x\n", func, *mpp);
						(void)
						receive_simple_message(*mpp);
					}
					break;

		    		default:
					break;
			}
		}
	}
}

/*
 *	After receiving a message, the receiver validates the message,
 *	optionally checks that the parameters of the sent message line
 *	up with those of the received message, optionally sends back a reply
 *	and optionally destroys the message contents. 
 */
void
received_message_processing(
	msg_layout_t	*msg)
{	
	mach_msg_return_t	mr;			
	boolean_t		good_message;
	boolean_t		complex, complex_ool;
	vm_size_t		inline_size;
	int			i;
	char			*inline_data;
	int 			*cur_ptr, *msg_ptr; /* insert tally counts */
	msg_params_t		current, *cur;
	char			*func = "received_message_processing";

	good_message = FALSE;
	complex = msg->h.msgh_bits & MACH_MSGH_BITS_COMPLEX;

	/*
	 *	XXX Got to fix the following as MACH_MSGH_BITS_COMPLEX_OOL	
	 *	is for internal use only. XXX
	 */
#if	KERNEL_TEST

	complex_ool = msg->h.msgh_bits & MACH_MSGH_BITS_COMPLEX_OOL;

	if (msg_test_receiver_verbose)
		printf("%s:  received %s %s message 0x%x, id %d, size %d\n",
			func, complex ? "complex" : "",
			 complex_ool ? "(COMPLEX_OOL)" : "", msg,
			 msg->h.msgh_id, msg->h.msgh_size);
#endif	/* KERNEL_TEST */

	/*
 	 *	We've got a message in hand.  First job is to validate its
	 *	contents: header, any passed ports or ool regions, and the
	 *	message body itself.	
	 */

	if (complex) {
		inline_data = (char *) &msg->d[msg->b.msgh_descriptor_count];
		inline_size = (vm_size_t) msg->h.msgh_size -
				(inline_data - (char *) msg);
	} else {
		inline_data = (char *) &msg->b;
		inline_size = (vm_size_t) (msg->h.msgh_size -
				 sizeof(mach_msg_header_t));
	}
	/*
	 *	If we have been asked to check that the parameters of
	 *	the sent message line up with those of the received
	 *	message, we tally the parameters and send them back
	 *	to the test generator in the reply message below.
	 */

	if(msg_test_receiver_sync){
		cur = &current;
		tally_message_parameters(msg, cur);
		if(msg_test_receiver_verbose){
			printf("%s: output of tally_message_parameters\n", func);
                        printf("\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n", cur->ibytes,
				cur->isnds, cur->ircvs, cur->oregs,
				cur->obytes, cur->osnds, cur->orcvs);

			printf("\t\t%d\t%d\t%d\t%d\t%d\n", cur->n_odata_reg,
				cur->n_osndr_arr, cur->n_osnd1_arr,
				cur->n_orcvs_arr,
				(cur->isnds + cur->ircvs +cur->oregs));
		}
	}
	if (validate_message_header(msg) == TRUE &&
	    (!complex || validate_message_descriptors(msg) == TRUE)){
		if(validate_data(msg, inline_data, inline_size) == TRUE) {
			/*
		 	 *	Message passes all validation checks.
		 	 */
			good_message = TRUE;
		}else{
			printf("\t%s: inline data bogus\n",func);
			good_message = FALSE;
		}

		/*	For complex messages only, do interesting
		 *	things with all inline port rights.
		 */

		if (complex) 
			receiver_responses(msg);
	}


	/*
	 *	We may reuse the inline message buffer forever but
	 *	we need to be able to selectively free up any inline port
	 *	rights or ool regions. The destroy routine is as careful
	 *	as it can be, although it can be confused by a bogus message.
	 */

	if (complex)
		destroy_message_contents(msg, RECEIVE_SIDE);

	/*
	 *	If the sender requested a reply, send back a simple response.
	 *	This action demonstrates that the send right carried along
	 *	along as the reply port really does work.  We will send the 
	 *	reply even if the message arrived in a damaged form:  this
	 *	allows the possibility that the sender might be able to
	 *	continue. The response will convey to the sending node, the
	 *	results of the validation done on this node and, if requested,
	 *	the tally of the number and/or size of the various message
	 *	component primitives found in the message. 
	 */

	if (msg->h.msgh_remote_port != MACH_PORT_NULL) {
		msg->h.msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_MOVE_SEND, 0 );
		msg->h.msgh_local_port = MACH_PORT_NULL;
		msg->h.msgh_id = RECEIVER_MSG_ID;
		msg->h.msgh_size = sizeof(mach_msg_header_t) + 
			msg_test_receiver_sync ? 8*(nparams + 1): 8;

		assert(MAX_MSG_SIZE > msg->h.msgh_size);

		/* Insert validation result in the reply message */
		msg_ptr = (int *)(&(msg->b));
		if (good_message == TRUE)
			*msg_ptr++ = (int) XMISSION_OK;
		else {
			*msg_ptr++ = (int) XMISSION_ERRORS;
			printf("\t%s: xmission errors msg=0x%x\n", func, msg);
			assert(FALSE);
		}
		/* Insert the message tally counts, if requested */
		if(msg_test_receiver_sync)
			cur_ptr = &(cur->ibytes);
			for(i = 0; i < nparams; i++)
				*msg_ptr++ = *cur_ptr++; 

		mr = mach_msg_overwrite(&msg->h, MACH_SEND_MSG,
					msg->h.msgh_size, 0, MACH_PORT_NULL,
					MACH_MSG_TIMEOUT_NONE,
					MACH_PORT_NULL, 0, 0);
		if (mr != MACH_MSG_SUCCESS) {
			printf("%s:  reply failed mr=0x%x, msg=0x%x\n",
			       func, mr, msg);
			assert(FALSE);
		}
	}
}

/*
 *	This simple function is used to generate the next values for
 *	the sizes or numbers of each of the the various message
 *	component primitives in a given pass of the message test table.
 *	The limit for the size or number of a given primitive can be
 *	specified in the message test table as a positive number
 *	(i.e., > 0) in which case our message components will have
 *	the range (0..limit] or it can be a negative number (i.e., < 0)
 *	implying that our message components will have the range
 *	[abs(limit)..infinity). If the limit is zero, we do not have
 *	any of that component.
 */
int
next_value(
	int	current_value,
	int	limit,
	int	incr)
{
	int	next;
	if (limit > 0) {
		next = current_value + incr;
		if (next > limit)
			next = 1;
	} else if (limit < 0)
		next = current_value + incr;
	else if (limit == 0)
		next = 0;
	return next;
}

/*
 *	This function is used to decouple the message table entry "oregs"
 *	into the flags component and the number component.
 */  
boolean_t
decouple_entry(
	int		coupled_oregs,
	unsigned int	*numb_oregs,
	char		*flags_oregs)
{
	int i, numb;
	int flag_numb = 0; /* number of flags found */
	char *flag_string = "LDPO"; /* flag/bit correspondence */

	*numb_oregs = coupled_oregs; 
	numb = 0x80000; /*bit corresponding to first flag */

	for(i = 0; i < NFLAGS; i++, numb >>= 1){
		if(numb & *numb_oregs){/* flag is set */
			flags_oregs[i] = flag_string[i];
			flag_numb++;
			*numb_oregs &= ~numb; /* clear the bit for that flag */
		}else /* insert a blank in the flags string */
			flags_oregs[i] = ' ';
	}
	flags_oregs[i] = '\0';
	if(msg_test_sender_verbose){
		printf("in decouple entry, act->oregs = 0x%x ", coupled_oregs);
		printf("Found %d flags: %s number is %u\n", flag_numb,
			flags_oregs, *numb_oregs);
	}

	/* numb_oregs cannot exceed 16**4 = 65536 */
	if(*numb_oregs > 65536)return FALSE;
	return TRUE; 
}

/*
 *	This routine checks that the user input message table selection
 *	as well as the starting and ending passes are valid. If not, we
 *	return with a value of FALSE.
 */
boolean_t
user_table_input_check(
	int		table_index,
	int		pass_start,
	int		pass_end)
{
	int			i, msg_test_count;
	test_table_record_t	*ttrt;
	boolean_t		input_ok = TRUE;
	/*
	 *	If requested, print out all valid table numbers,
	 *	names, sizes and address.
	 */
	if(table_index == 0){
		printf("\n ""0"" is not a valid test table selection...printing out table info\n\n");
		printf("Index.		Table name		Address		Passes \n\n");
		ttrt = test_table_record;
		for( i = 0; i < NUMBER_OF_TABLES; i++, ttrt++)
			printf("%d\t\t%-20s\t0x%x\t%d\n", (i+1), ttrt->table_str, ttrt->table_ptr, ttrt->table_size);
		return FALSE;
	}
	if(table_index > 0 && table_index <= NUMBER_OF_TABLES)
		msg_test_count = test_table_record[table_index -1 ].table_size;

	else{
		printf("The second input parameter (table index) must be between %d and %d\n", 1, NUMBER_OF_TABLES);
		return FALSE;
	}

	if(pass_start < -1 || (pass_start > msg_test_count - 1)){
		printf("Starting pass number is out of range, should be between -1 and %d inclusive\n",  msg_test_count - 1);
		input_ok = FALSE;
	}
	if(pass_end < -1 || (pass_end > msg_test_count - 1)){
		printf("Ending pass number is out of range, should be between -1 and %d inclusive\n",  msg_test_count - 1);
		input_ok = FALSE;
	}
	if(pass_end != -1 && pass_end < pass_start){
		printf("Ending pass number must be greater than or the same as the starting pass number for any message tests to be executed\n");
		input_ok = FALSE;
	}
	if(!input_ok)
		return FALSE;

	/*
	 *	Set some global variables for use later on.
	 */
	msg_table_index = table_index; /* 1 for standard, 2 for alternate */
	msg_test_start = pass_start; /* -1 for default starting pass */
	msg_test_end = pass_end; /* -1 for default ending pass */

	if(msg_test_sender_verbose)
	      printf("This run will use the %s table [which has %d passes.]\n",
		   test_table_record[table_index -1 ].table_str,
		   test_table_record[table_index -1 ].table_size); 
	return TRUE;
}

#if	USER_SPACE

/*
 *	This routine is used to determine the node number of the boot node.
 */

int
get_node_numb(
	char		*str)
{
	kern_return_t		kr;
	int			node_numb;
	kernel_boot_info_t	mkbootinfo;		/* 4K array */
	char			*where_string;
	char			*where_number; /* where number is found */
	char			*where_nl; /* new-line termination */
	char			*func = "get_node_numb";

	/*
	 *	First get the boot config info (as a string from the kernel).
	 */

	kr = host_get_boot_info(mach_host_priv_self(), mkbootinfo);
	if (kr != KERN_SUCCESS) {
		printf("%s: host_get_boot_info problem kr is 0x%x\n",func, kr);
		return(-1);
	}
	/* printf("Boot config info from the kernel: %s\n", mkbootinfo); */

	where_string = strstr((const char *)mkbootinfo, (const char *)str);
	if (where_string == NULL) {
		printf("Error in %s:strstr can't find %s in mkbootinfo\n",
			 func, str);
		return(-1);
	}
	where_number = where_string +  strlen(str) + 1;

	/* Boot config parameters are supposed to be NL terminated! */
	where_nl = strchr((const char *) where_number, (int) '\n');
	if (where_nl == NULL) {
		printf("Error in %s:strchr can't find new-line after config param\n", func);
		return(-1);
	}

	*where_nl = '\0'; /* Mark the end of the config parameter value */
	node_numb = atoi(where_number);
	printf("boot node is %d\n", node_numb);
	return (node_numb);
}

#endif	/* USER_SPACE */

