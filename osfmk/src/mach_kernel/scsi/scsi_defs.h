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

/* INTEL_HIST */
/*
 * Revision 1.7.4.1  1994/02/15  23:24:44  andyp
 * Updates from the mainline.
 *
 * Revision 1.8  1994/02/11  23:58:53  richardg
 *  Reviewer: Jerrie Coffman
 *  Risk: low
 *  Benefit or PTS #: add support for enabling/disabling buffering on
 * 		tape drives.
 *  Testing: Tape EAT's
 *  Module(s):  scsi_tape_info_t structure.
 *
 * Revision 1.7  1993/09/24  17:33:19  jerrie
 * Added TGT_COMPRESSION flag to support tape data compression.
 *
 * Revision 1.6  1993/09/16  18:12:20  richardg
 * modified the memory allocation to evenly divide available MIO memory between existing scsi devices
 *
 * Revision 1.5  1993/06/30  22:54:04  dleslie
 * Adding copyright notices required by legal folks
 *
 * Revision 1.4  1993/04/27  20:48:09  dleslie
 * Copy of R1.0 sources onto main trunk
 *
 * Revision 1.2.6.2  1993/04/22  18:53:39  dleslie
 * First R1_0 release
 */
/* INTEL_ENDHIST */

/* CMU_HIST */
/*
 * Revision 2.9.2.1  92/03/28  10:16:02  jeffreyh
 * 	Pick up changes from MK71
 * 	[92/03/20  13:33:03  jeffreyh]
 * 
 * Revision 2.10  92/02/23  22:44:55  elf
 * 	Changed unused field into masterno in target descriptor.
 * 	[92/02/22  19:31:54  af]
 * 
 * Revision 2.9  91/08/24  12:28:38  af
 * 	Added processor_type infos, definition of an opaque type,
 * 	multiP locking.
 * 	[91/08/02  03:55:05  af]
 * 
 * Revision 2.8  91/07/09  23:22:53  danner
 * 	Added include of <scsi/rz_labels.h>
 * 	[91/07/09  11:16:30  danner]
 * 
 * Revision 2.7  91/06/19  11:57:43  rvb
 * 	File moved here from mips/PMAX since it is now "MI" code, also
 * 	used by Vax3100 and soon -- the omron luna88k.
 * 	[91/06/04            rvb]
 * 
 * Revision 2.6  91/05/14  17:30:18  mrt
 * 	Correcting copyright
 * 
 * Revision 2.5  91/05/13  06:05:34  af
 * 	Made counters unsigned, added copy_count for use by HBAs
 * 	that do unlimited DMA via double buffering.  Made explicit
 * 	two padding bytes, and let them be HBA-specific (e.g. used
 * 	for odd-byte-boundary conditions on some).
 * 	Made max_dma_data unsigned, a value of -1 means unlimited.
 * 	Removed unsed residue field.
 * 
 * 	Defined tape-specific information fields to target structure.
 * 	Added tape-specific flags and flag for targets that require
 * 	the long form of various scsi commands.
 * 	Added disconnected-state information to target structure.
 * 	Added watchdog field to adapter structure.
 * 	[91/05/12  16:24:10  af]
 * 
 * Revision 2.4.1.2  91/04/05  13:13:29  af
 * 	Made counters unsigned, added copy_count for use by HBAs
 * 	that do unlimited DMA via double buffering.  Made explicit
 * 	two padding bytes, and let them be HBA-specific (e.g. used
 * 	for odd-byte-boundary conditions on some).
 * 	Made max_dma_data unsigned, a value of -1 means unlimited.
 * 	Removed unsed residue field.
 * 
 * Revision 2.4.1.1  91/03/29  17:06:09  af
 * 	Defined tape-specific information fields to target structure.
 * 	Added tape-specific flags and flag for targets that require
 * 	the long form of various scsi commands.
 * 	Added disconnected-state information to target structure.
 * 	Added watchdog field to adapter structure.
 * 
 * Revision 2.4  91/02/05  17:45:43  mrt
 * 	Added author notices
 * 	[91/02/04  11:19:29  mrt]
 * 
 * 	Changed to use new Mach copyright
 * 	[91/02/02  12:18:11  mrt]
 * 
 * Revision 2.3  90/12/05  23:35:12  af
 * 	Cleanups, use BSD labels internally.
 * 	[90/12/03  23:47:29  af]
 * 
 * Revision 2.1.1.1  90/11/01  03:39:55  af
 * 	Created.
 * 	[90/09/03            af]
 */
/* CMU_ENDHIST */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
 */
/*
 *	File: scsi_defs.h
 * 	Author: Alessandro Forin, Carnegie Mellon University
 *	Date:	9/90
 *
 *	Controller-independent definitions for the SCSI driver
 */

#ifndef	_SCSI_SCSI_DEFS_H_
#define	_SCSI_SCSI_DEFS_H_

#include <scsiinfo.h>
#include <kern/queue.h>
#include <kern/lock.h>
#include <chips/busses.h>

#include <scsi/scsi.h>
#include <scsi/scsi2.h>
#include <scsi/compat_30.h>
#include <device/disk_status.h>

#define	await(event)	sleep((event_t)(event),FALSE)

/* calculates (byte_offset / dev_bsize) */ 
#define btodb(byte_offset) \
	((unsigned)(byte_offset) / tgt->block_size)

/* calculates (block_number * dev_bsize) */

#define dbtob(block_number) \
	((unsigned)(block_number) * tgt->block_size)

typedef	vm_offset_t	opaque_t;	/* should be elsewhere */

/*
 * HBA-independent Watchdog
 */
struct watchdog_t {

	unsigned short	reset_count;
	char		nactive;

	char		watchdog_state;

#define SCSI_WD_INACTIVE	0
#define	SCSI_WD_ACTIVE		1
#define SCSI_WD_EXPIRED		2

/*	Pointer to device-specific reset routine		*/
	void	( *reset )(
			struct watchdog_t	*wd);

};

#define SCSI_NLUNS 8

/*
 * Internal error codes, and return values
 * XXX use the mach/error.h scheme XXX
 */
typedef unsigned int		scsi_ret_t;

#define	SCSI_ERR_GRAVITY(x)	((unsigned)(x)&0xf0000000U)
#define	SCSI_ERR_GRAVE		0x80000000U
#define SCSI_ERR_BAD		0x40000000

#define	SCSI_ERR_CLASS(x)	((unsigned)(x)&0x0fffffffU)
#define	SCSI_ERR_STATUS		0x00000001
#define	SCSI_ERR_SENSE		0x00000002
#define SCSI_ERR_MSEL		0x00000004
#define SCSI_ERR_MSEN		0x00000008
#define SCSI_ERR_NO_LUN		0x00000025

#define	SCSI_RET_IN_PROGRESS	0x00
#define	SCSI_RET_SUCCESS	0x01
#define	SCSI_RET_RETRY		0x02
#define SCSI_RET_NEED_SENSE	0x04
#define SCSI_RET_ABORTED	0x08
#define	SCSI_RET_DEVICE_DOWN	0x10

/*
 * Device-specific information kept by driver
 */
typedef struct {
	struct disklabel	l;
	struct {
	    unsigned int	badblockno;
	    recnum_t		save_rec;
	    char		*save_addr;
	    io_buf_len_t	save_count;
	    io_buf_len_t	save_resid;
	    int			retry_count;
	} b;
} scsi_disk_info_t;

typedef struct {
	boolean_t	read_only;
	unsigned int	buffer_mode; /* added for 3480 support */
	unsigned int	speed;
	unsigned int	density;
	unsigned int	maxreclen;
	boolean_t	fixed_size;
	int		retry_count;
} scsi_tape_info_t;

typedef struct {
	char		req_pending;
	char		req_id;
	char		req_lun;
	char		req_cmd;
	unsigned int	req_len;
	struct target_info *req_target;
	/* more later */
} scsi_processor_info_t;

typedef struct {
	int	open_count;
	struct red_list	*violates_standards;
} scsi_cdrom_info_t;

/*
 * Device descriptor
 */

#define	SCSI_TARGET_NAME_LEN	8+16+4+8	/* our way to keep it */

#define MAX_SCSI_TARGETS	8
#define MAX_LUNS		8

typedef struct target_info {
	queue_chain_t	links;			/* to queue for bus */
	io_req_t	ior;			/* what we are doing */

	int		open_count;		/* number of opens */

	unsigned int	flags;
#define	TGT_DID_SYNCH		0x00000001	/* finished the synch neg */
#define	TGT_TRY_SYNCH		0x00000002	/* do the synch negotiation */
#define	TGT_FULLY_PROBED	0x00000004	/* can sleep to wait */
#define	TGT_ONLINE		0x00000008	/* did the once-only stuff */
#define	TGT_ALIVE		0x00000010
#define	TGT_BBR_ACTIVE		0x00000020	/* bad block replc in progress */
#define	TGT_DISCONNECTED	0x00000040	/* waiting for reconnect */
#define	TGT_WRITTEN_TO		0x00000080	/* tapes: needs a filemark on close */
#define	TGT_REWIND_ON_CLOSE	0x00000100	/* tapes: rewind */
#define	TGT_BIG			0x00000200	/* disks: > 1Gb, use long R/W */
#define	TGT_REMOVABLE_MEDIA	0x00000400	/* e.g. floppy, cd-rom,.. */
#define	TGT_READONLY		0x00000800	/* cd-rom, scanner, .. */
#define	TGT_OPTIONAL_CMD	0x00001000	/* optional cmd, ignore errors */
#define TGT_WRITE_LABEL		0x00002000	/* disks: enable overwriting of label */
#define	TGT_US			0x00004000	/* our desc, when target role */
#define	TGT_DATA_WAS_READ	0x00008000	/* tapes: skip eof on close ? */
#define	TGT_FOUND_EOF		0x00010000	/* tapes: eof found */
#define TGT_COMPRESSION		0x00020000	/* tapes: data compression */
#define TGT_PROBED_LUNS		0x00040000
#define TGT_CHAINED_IO_SUPPORT	0x00080000	/* supports IO_SCATTER lists */

#define	TGT_HW_SPECIFIC_BITS	0xfff00000U	/* see specific HBA */
	char			*hw_state;	/* opaque */
	char			*dma_ptr;
	char			*cmd_ptr;
	struct scsi_devsw_struct	*dev_ops;	/* circularity */
	char			target_id;
	char			unit_no;
	unsigned char		sync_period;
	unsigned char		sync_offset;
	decl_simple_lock_data(,target_lock)
	/*
	 * State info kept while waiting to seize bus, either for first
	 * selection or while in disconnected state
	 */
	struct {
	    struct script	*script;
	    int			( *handler )( void );
	    unsigned int	out_count;
	    unsigned int	in_count;
	    unsigned int	copy_count;	/* optional */
	    unsigned int	dma_offset;
	    natural_t	 	dma_id;
	    vm_offset_t		dma_physaddr;	/* phys address for IO */
	    natural_t	 	dma_window;	/* size of current map */
	    unsigned char	identify;
	    unsigned char	cmd_count;
	    unsigned char	hba_dep[4];
	} transient_state;
	unsigned int	block_size;
	volatile char	done;
	unsigned char	cur_cmd;
#ifdef	PARAGON860	/* performance mod */
	char		true_lun; /* the real LUN */
#endif	/* PARAGON860 */
	char		lun;
	char		masterno;
	char		tgt_name[SCSI_TARGET_NAME_LEN];
	union {
		scsi_disk_info_t	disk;
		scsi_tape_info_t	tape;
		scsi_processor_info_t	cpu;
	} dev_info;
	scsi_cdrom_info_t cdrom_info; /* not in union as can be viewed as a disk */
#ifdef	PARAGON860	/* performance mod */
        unsigned int    max_dma_data;
#endif	/* PARAGON860 */
#ifdef 	POWERMAC
	unsigned int	retry_count;	/* Retry count for SCSI_RET_RETRY */
#endif
#if  NSCSIINFO > 0
      scsi2_inquiry_data_t    target_inquiry;
#endif
} target_info_t;

#define dma_id        transient_state.dma_id
#define dma_physaddr  transient_state.dma_physaddr
#define dma_window    transient_state.dma_window

/*
 * Device-specific operations
 */
typedef struct scsi_devsw_struct {
	char		*( *driver_name )(		/* my driver's name */
				boolean_t	internal );
	void		( *optimize )(			/* tune up params */
				target_info_t	*target );
	scsi_ret_t	( *open )(			/* open time ops */
				target_info_t	*target,
				io_req_t	ior );
	scsi_ret_t	( *close )(			/* close time ops */
				dev_t		dev_id,
				target_info_t	*target );
	void		( *strategy )(			/* sort/start routine */
				io_req_t	ior );
	void		( *restart )(			/* completion routine */
				target_info_t	*target,
				boolean_t	done );
	io_return_t	( *get_status )(		/* specialization */
				dev_t			dev_id,
				target_info_t		*target,
				dev_flavor_t		flavor,
				dev_status_t		status,
				mach_msg_type_number_t	*status_count );
	io_return_t	( *set_status )(		/* specialization */
				dev_t			dev_id,
				target_info_t		*target,
				dev_flavor_t		flavor,
				dev_status_t		status,
				mach_msg_type_number_t	status_count );
} scsi_devsw_t;

#define SCSI_OPTIMIZE_NULL ((void (*)(target_info_t *)) 0)
#define SCSI_OPEN_NULL ((scsi_ret_t (*)(target_info_t *,io_req_t)) 0)
#define SCSI_CLOSE_NULL ((scsi_ret_t (*)(dev_t,target_info_t *)) 0)
#define SCSI_STRATEGY_NULL ((void (*)(io_req_t)) 0)
#define SCSI_START_NULL ((void (*)(target_info_t *, boolean_t)) 0)
#define SCSI_GET_STATUS_NULL	\
	((io_return_t (*)(dev_t,target_info_t *,	\
		dev_flavor_t,dev_status_t,mach_msg_type_number_t *)) 0)
#define SCSI_SET_STATUS_NULL	\
	((io_return_t (*)(dev_t,target_info_t *,	\
		dev_flavor_t,dev_status_t,mach_msg_type_number_t)) 0)

extern scsi_devsw_t	scsi_devsw[];

/*
 * HBA descriptor
 */

typedef struct {
	/* initiator (us) state */
	unsigned char	initiator_id;
	unsigned char	masterno;
	unsigned int	max_dma_data;
	unsigned int	max_dma_segs;
	char		*hw_state;		/* opaque */
	void		( *go )(
				target_info_t		*tgt,
				unsigned int		cmd_count,
				unsigned int		in_count,
				boolean_t		cmd_only);
	void		( *watchdog )(
				struct watchdog_t	*hw);
	boolean_t	( *probe )(
				target_info_t		*tgt,
				io_req_t		ior);
	void		( *tgt_setup )(
				target_info_t		*tgt);
	int		( *remote_op)(
				      target_info_t	*tgt,
				      int		op,
				      unsigned int	remote_pa,
				      char		*buffer,
				      unsigned int	count);
	/* per-target state */
#ifdef	notdef /* PARAGON860 */	/* performance mod */
	target_info_t	*target[MAX_SCSI_TARGETS][MAX_LUNS];
#else	/* PARAGON860 */
	target_info_t		*target[8];
#endif	/* PARAGON860 */
} scsi_softc_t;

extern	scsi_softc_t	*scsi_softc[];

#define	BGET(d,mid,id)	(d[mid] & (1 << id))		/* bitmap ops */
#define BSET(d,mid,id)	d[mid] |= (1 << id)
#define BCLR(d,mid,id)	d[mid] &= ~(1 << id)

extern unsigned char	scsi_no_synchronous_xfer[];	/* one bitmap per ctlr */
extern unsigned char	scsi_use_long_form[];		/* one bitmap per ctlr */
extern unsigned char	scsi_might_disconnect[];	/* one bitmap per ctlr */
extern unsigned char	scsi_should_disconnect[];	/* one bitmap per ctlr */
extern unsigned char	scsi_initiator_id[];		/* one id per ctlr */

extern boolean_t	scsi_exabyte_filemarks;
extern boolean_t	scsi_no_automatic_bbr;
extern int		scsi_bbr_retries;
extern int		scsi_watchdog_period;
extern int		scsi_delay_after_reset;
extern unsigned int	scsi_per_target_virtual;	/* 2.5 only */

extern int		scsi_debug;

/*
 * Function Prototypes for device-specific disk/tape routines.
 */

/*	Close device					*/
extern void		rz_close(
				dev_t		dev);

/*	Return device information			*/
extern io_return_t	rz_devinfo(
				dev_t		dev,
				dev_flavor_t	flavor,
				char		*info);

/*	Get device status				*/
extern io_return_t	rz_get_status(
				dev_t			dev,
				dev_flavor_t		flavor,
				dev_status_t		status,
				mach_msg_type_number_t	*status_count);

/*	Open device, subject to timeout			*/
extern io_return_t	rz_open(
				dev_t		dev,
				dev_mode_t	mode,
				io_req_t	ior);

/*	SCSI read					*/
extern io_return_t	rz_read(
				dev_t		dev,
				io_req_t	ior);

/*	Set device state				*/
extern io_return_t	rz_set_status(
				dev_t			dev,
				dev_flavor_t		flavor,
				dev_status_t		status,
				mach_msg_type_number_t	status_count);

/*	SCSI write					*/
extern	io_return_t	rz_write(
				dev_t		dev,
				io_req_t	ior);

/*	Misc.						*/

/*	Test if device is alive				*/
extern	boolean_t	rz_check(
				dev_t		dev,
				scsi_softc_t	**p_sc,
				target_info_t	**p_tgt);

extern	void		rz_simpleq_strategy(
				io_req_t	ior,
				void		(*start)(target_info_t *, boolean_t));

/*	Open SCSI tape device				*/
extern	io_return_t	tz_open(
				dev_t		dev,
				dev_mode_t	mode,
				io_req_t	ior);

/*	SCSI tape read					*/
extern	io_return_t	tz_read(
				dev_t		dev,
				io_req_t	ior);

/*	SCSI tape write					*/
extern	io_return_t	tz_write(
				dev_t		dev,
				io_req_t	ior);

/*	SCSI CD-ROM Functions.				*/

extern	io_return_t	cd_open(
				dev_t		dev,
				dev_mode_t	mode,
				io_req_t	req);

extern	void		cd_close(
				dev_t		dev);

extern	io_return_t	cd_write(
				dev_t		dev,
				io_req_t	ior);

extern	io_return_t	cd_read(
				dev_t		dev,
				io_req_t	ior);

extern io_return_t	cd_get_status(
				dev_t			dev,
				dev_flavor_t		flavor,
				dev_status_t		status,
				mach_msg_type_number_t	*status_count);

extern io_return_t	cd_set_status(
				dev_t			dev,
				dev_flavor_t		flavor,
				dev_status_t		status,
				mach_msg_type_number_t	status_count);

extern io_return_t	cd_devinfo(
				dev_t           dev,
				dev_flavor_t    flavor,
				char            *info);

/*
 *	Function prototypes for SCSI device-specific routines
 */

/*	Translate boolean to "rz" or "CD-ROM"		*/
extern	char * sccdrom_name(
			boolean_t	internal);
	
/*	Translate boolean to "cz" or "comm"		*/
extern	char * sccomm_name(
			boolean_t	internal);

/*	Translate boolean to "sc" or "cpu"		*/
extern	char * sccpu_name(
			boolean_t	internal);

/*	Initialize new SCSI CPU target structure	*/
extern	void	sccpu_new_initiator(
			target_info_t	*self,
			target_info_t	*initiator);

/*	SCSI CPU start I/O routine			*/
extern	void	sccpu_start(
			target_info_t	*tgt,
			boolean_t	done);

/*	SCSI CPU strategy routine			*/
extern	void	sccpu_strategy(
				io_req_t	ior);

/*	SCSI disk Bad Block Replacement after error	*/
extern	boolean_t	scdisk_bad_block_repl(
				target_info_t	*tgt,
				unsigned int	blockno);

/*	SCSI disk Bad Block Replacement strategy	*/
extern	void	scdisk_bbr_start(
			target_info_t	*tgt,
			boolean_t	done);

/*	SCSI disk get status				*/
extern	io_return_t	scdisk_get_status(
				dev_t			dev,
				target_info_t		*tgt,
				dev_flavor_t		flavor,
				dev_status_t		status,
				mach_msg_type_number_t	*status_count);

/*	SCSI disk mode selection routine		*/
extern	char	scdisk_mode_select(
			target_info_t	*tgt,
			int		lbn,
			io_req_t	ior,
			char		*mdata,
			int		mlen,
			boolean_t	save);

/*	Translate boolean to "rz" or "disk"		*/
extern	char * scdisk_name(
			boolean_t	internal);

/*	SCSI disk open routine				*/
extern	scsi_ret_t	scdisk_open(
				target_info_t		*tgt,
				io_req_t	        req);

/*	SCSI disk close routine				*/
extern	scsi_ret_t	scdisk_close(
				dev_t			dev,
				target_info_t		*tgt);

/*	SCSI disk read routine				*/
extern	void	scdisk_read(
			target_info_t	*tgt,
			unsigned int	secno,
			io_req_t	ior);

/*	SCSI disk set status				*/
extern	io_return_t	scdisk_set_status(
				dev_t			dev,
				target_info_t		*tgt,
				dev_flavor_t		flavor,
				dev_status_t		status,
				mach_msg_type_number_t	status_count);

/*	SCSI disk I/O start routine			*/
extern	void	scdisk_start(
			target_info_t	*tgt,
			boolean_t	done);

/*	SCSI disk R/W start routine (for rz_disk_bbr)	*/
extern	void	scdisk_start_rw(
			target_info_t	*tgt,
			io_req_t	ior);

/*	SCSI disk strategy routine			*/
extern	void	scdisk_strategy(
				register io_req_t	ior);

/*	SCSI disk verify routine			*/
extern	char	scdisk_verify(
			target_info_t	*tgt,
			unsigned int	secno,
			unsigned int	nsectrs,
			io_req_t	ior);

/*	SCSS disk write routine				*/
extern	void	scdisk_write(
			target_info_t	*tgt,
			unsigned int	secno,
			io_req_t	ior);
	
/*	Translate boolean to "jz" or "jukebox"		*/
extern	char * scjb_name(
			boolean_t	internal);

/*	Translate boolean to "mz" or "opti"		*/
extern	char * scmem_name(
			boolean_t	internal);

/*	Translate boolean to "lz" or "printer"		*/
extern	char * scprt_name(
			boolean_t	internal);

/*	SCSI printer optimization routine (nugatory)	*/
extern	void	scprt_optimize(
				target_info_t	*tgt);
	
/*	Translate boolean to "oz" or "scanner"		*/
extern	char * scscn_name(
			boolean_t	internal);

/*	SCSI tape close routine				*/
extern	scsi_ret_t	sctape_close(
				dev_t		dev,
				target_info_t	*tgt);

/*	SCSI tape get status				*/
extern	io_return_t	sctape_get_status(
				dev_t			dev,
				target_info_t		*tgt,
				dev_flavor_t		flavor,
				dev_status_t		status,
				mach_msg_type_number_t	*status_count);

/*	SCSI tape mode selection routine		*/
extern	char	sctape_mode_select(
			target_info_t	*tgt,
			unsigned char	*vuque_data,
			int		vuque_data_len,
			unsigned int	newspeed,
			io_req_t	ior);

/*	Translate boolean to "tz" or "tape"		*/
extern	char * sctape_name(
			boolean_t	internal);

/*	SCSI tape open routine				*/
extern	scsi_ret_t	sctape_open(
				target_info_t	*tgt,
				io_req_t	req);

/*	SCSI tape optimization routine			*/
extern	void		sctape_optimize(
				target_info_t	*tgt);

/*	SCSI tape read routine				*/
extern	void	sctape_read(
			target_info_t	*tgt,
			io_req_t	ior);

/*	SCSI tape set status				*/
extern	io_return_t	sctape_set_status(
				dev_t			dev,
				target_info_t		*tgt,
				dev_flavor_t		flavor,
				dev_status_t		status,
				mach_msg_type_number_t	status_count);

/*	SCSI tape I/O start routine			*/
extern	void	sctape_start(
			target_info_t	*tgt,
			boolean_t	done);

/*	SCSI tape strategy routine			*/
extern	void	sctape_strategy(
				io_req_t	ior);

/*	SCSI tape write routine				*/
extern	void	sctape_write(
		target_info_t	*tgt,
		io_req_t	ior);

/*	Translate boolean to "rz" or "WORM-disk"	*/
extern	char * scworm_name(
			boolean_t	internal);

#ifdef i386
/*	SCSI disk get_status routine (i386 version)	*/
extern	io_return_t	scsi_i386_get_status(
				dev_t			dev,
				target_info_t		*tgt,
				dev_flavor_t		flavor,
				dev_status_t		status,
				mach_msg_type_number_t	*status_count);

/*	SCSI disk set_status routine (i386 version)	*/
extern	io_return_t	scsi_i386_set_status(
				dev_t			dev,
				target_info_t		*tgt,
				dev_flavor_t		flavor,
				dev_status_t		status,
				mach_msg_type_number_t	status_count);
#endif /* i386 */

#ifdef PPC
/*	SCSI disk get_status routine (PPC version)	*/
extern	io_return_t	scsi_ppc_get_status(
				dev_t			dev,
				target_info_t		*tgt,
				dev_flavor_t		flavor,
				dev_status_t		status,
				mach_msg_type_number_t	*status_count);

/*	SCSI disk set_status routine (PPC version)	*/
extern	io_return_t	scsi_ppc_set_status(
				dev_t			dev,
				target_info_t		*tgt,
				dev_flavor_t		flavor,
				dev_status_t		status,
				mach_msg_type_number_t	status_count);
#endif /* PPC */

/*
 *	Function prototypes for generic SCSI routines
 */

/*	Set up SCSI driver after probe			*/
extern	void	scsi_attach(
			struct bus_device *ui);

/*	Called when HBA sees BusReset interrupt		*/
extern	void	scsi_bus_was_reset(
			scsi_softc_t	*sc);

/*	Check SCSI sense data				*/
extern	boolean_t	scsi_check_sense_data(
				target_info_t		*tgt,
				scsi_sense_data_t	*sns);

/*	Warn user of some SCSI device error		*/
extern	void	scsi_error(
			target_info_t	*tgt,
			unsigned int	code,
			unsigned char	info,
			char		*addtl);

/*	Format SCSI disk unit				*/
extern	char	scsi_format_unit(
			target_info_t	*tgt,
			unsigned char	mode,
			unsigned char	vuqe,
			unsigned int	intlv,
			io_req_t	ior);

/*	Initiate SCSI I/O without wait for completion	*/
extern	void	scsi_go(
			target_info_t	*tgt,
			unsigned int	insize,
			unsigned int	outsize,
			boolean_t	cmd_only);

/*	Initiate SCSI I/O and wait for completion	*/
extern	void	scsi_go_and_wait(
			target_info_t	*tgt,
			unsigned int	insize,
			unsigned int	outsize,
			io_req_t	ior);

/*	SCSI inquiry routine				*/
extern	char	scsi_inquiry(
			target_info_t	*tgt,
			int		pagecode);

/*	Group 1 Long Read				*/
extern	void	scsi_long_read(
			target_info_t	*tgt,
			unsigned int	secno,
			io_req_t	ior);

/*	Group 1 Long Write				*/
extern	void	scsi_long_write(
			target_info_t	*tgt,
			unsigned int	secno,
			io_req_t	ior);

/*	Allocate SCSI master state structure.		*/
extern	scsi_softc_t  *	scsi_master_alloc(
			       	unsigned	unit,
			       	char		*hw);

/*	Allow/Prevent SCSI medium removal		*/
extern	void	scsi_medium_removal(
			target_info_t	*tgt,
			boolean_t	allow,
			io_req_t	ior);

/*	SCSI mode sense routine				*/
extern	char	scsi_mode_sense(
			target_info_t	*tgt,
			unsigned char	pagecode,
			int		len,
			io_req_t	ior);

extern void scsi_verify_state(
		        target_info_t	*tgt);
extern void scsi_initialize(
			target_info_t 	*tgt);

/*	SCSI inquiry print routine			*/
extern	void	scsi_print_inquiry(
			scsi2_inquiry_data_t	*inq,
			int			pagecode,
			char			*result);

/*	Print SCSI sense data				*/
extern	void	scsi_print_sense_data(
			scsi_sense_data_t	*sns);

/*	See if a SCSI device answers after autoconfig.	*/
extern	boolean_t	scsi_probe(
				scsi_softc_t	*sc,
				target_info_t	**tgt_ptr,
				int		target_id,
				int		lun,
				io_req_t	ior);

extern void		scsi_probe_luns(
				target_info_t		*tgt);


/*	Read block limits of SCSI tape			*/
extern	char	scsi_read_block_limits(
			target_info_t	*tgt,
			io_req_t	ior);

/*	Read capacity of SCSI disk			*/
extern	char	scsi_read_capacity(
			target_info_t	*tgt,
			int		lbn,
			io_req_t	ior);

/*	Read defect data from SCSI disk			*/
extern	char	scsi_read_defect(
			target_info_t	*tgt,
			unsigned char	mode,
			io_req_t	ior);

/*	Reassign bad blocks on SCSI disk		*/
extern	void	scsi_reassign_blocks(
			target_info_t	*tgt,
			unsigned int	*defect_list,
			int		n_defects,
			io_req_t	ior);

/*	Issue SCSI Receive command			*/
extern	void	scsi_receive(
			target_info_t	*tgt,
			io_req_t	ior);

/*	Receive SCSI diagnostic results			*/
extern	char	scsi_receive_diag(
			target_info_t	*tgt,
			char		*result,
			unsigned short	result_len,
			io_req_t	ior);

/*	Request SCSI sense (mandatory, all)		*/
extern	char	scsi_request_sense(
			target_info_t	*tgt,
			io_req_t	ior,
			char		**data);

/*	SCSI tape rewind				*/
extern	io_return_t	scsi_rewind(
				target_info_t	*tgt,
				io_req_t	ior,
				boolean_t	wait);

/*	Issue SCSI Send command				*/
extern	void	scsi_send(
			target_info_t	*tgt,
			io_req_t	ior);

/*	Inquire for slave device			*/
extern	int	scsi_slave(
			struct bus_device	*ui,
			caddr_t			reg);

/*	Allocate SCSI slave state structure.		*/
extern	target_info_t *	scsi_slave_alloc(
				unsigned	unit,
				unsigned	slave,
				char		*hw );

/*	SCSI tape forward/back space routine		*/
extern	char	scsi_space(
			target_info_t	*tgt,
			unsigned char	mode,
			int		count,
			io_req_t	ior);

/*	SCSI start/stop unit routine			*/
extern	char	scsi_start_unit(
			target_info_t	*tgt,
			unsigned char	ss,
			io_req_t	ior);

/*	SCSI test unit ready routine			*/
extern	char	scsi_test_unit_ready(
			target_info_t	*tgt,
			io_req_t	ior);

/*	SCSI Watchdog timer resets bus if held up too long	*/
extern	void	scsi_watchdog(
			struct watchdog_t	*hw);

/*	SCSI write (tape) filemarks routine		*/
extern	char	scsi_write_filemarks(
			target_info_t	*tgt,
			unsigned int	count,
			io_req_t	ior);

/*	SCSI2 inquiry with printed display		*/
extern	void	scsi2_print_inquiry(
			scsi2_inquiry_data_t	*inq,
			unsigned char		pagecode,
			char			*result);

/*	How big is the command?				*/
extern	int	sizeof_scsi_command(
			unsigned char		command);

/*	Scsi audio stuff				*/
extern	int	scsi_pause_resume(
			target_info_t		*tgt,
			boolean_t		stop_it,
			io_req_t		ior);

extern	int	scsi_play_audio(
			target_info_t		*tgt,
			unsigned int		start,
			unsigned int		len,
			boolean_t		relative_address,
			io_req_t		ior);

extern	int	scsi_play_audio_long(
			target_info_t		*tgt,
			unsigned int		start,
			unsigned int		len,
			boolean_t		relative_address,
			io_req_t		ior);

extern	int	scsi_play_audio_msf(
			target_info_t		*tgt,
			int			sm,
			int			ss,
			int			sf,
			int			em,
			int			es,
			int			ef,
			io_req_t		ior);

extern	int	scsi_play_audio_track_index(
			target_info_t		*tgt,
			int			st,
			int			si,
			int			et,
			int			ei,
			io_req_t		ior);

extern	int	scsi_play_audio_track_relative(
			target_info_t		*tgt,
			unsigned int		lba,
			int			st,
			unsigned int		len,
			io_req_t		ior);

extern	int	scsi_play_audio_track_relative_long(
			target_info_t		*tgt,
			unsigned int		lba,
			int			st,
			unsigned int		len,
			io_req_t		ior);

extern	int	scsi_read_header(
			target_info_t		*tgt,
			boolean_t		msf_format,
			unsigned int		lba,
			unsigned int		allocsize,
			io_req_t		ior);

extern	int	scsi_read_subchannel(
			target_info_t		*tgt,
			boolean_t		msf_format,
			unsigned int		data_format,
			unsigned int		trackno,
			io_req_t		ior);

extern	int	scsi_read_toc(
			target_info_t		*tgt,
			boolean_t		msf_format,
			int			trackno,
			int			allocsize,
			io_req_t		ior);

extern	int	scsi2_mode_select(
			target_info_t		*tgt,
			boolean_t		save,
			unsigned char		*page,
			int			pagesize,
			io_req_t		ior);

extern	int	cdrom_vendor_specific(
			target_info_t		*tgt,
			scsi_command_group_2	*cmd,
			unsigned char		*params,
			int			paramlen,
			int			retlen,
			io_req_t		ior);


extern	void	aha_intr(
			int			unit);

extern	void	eaha_intr(
			int			unit);

#endif	/* _SCSI_SCSI_DEFS_H_ */
