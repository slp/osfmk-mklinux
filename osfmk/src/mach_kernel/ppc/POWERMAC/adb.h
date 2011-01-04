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
 * Copyright 1991-1998 by Apple Computer, Inc. 
 *              All Rights Reserved 
 *  
 * Permission to use, copy, modify, and distribute this software and 
 * its documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appears in all copies and 
 * that both the copyright notice and this permission notice appear in 
 * supporting documentation. 
 *  
 * APPLE COMPUTER DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE. 
 *  
 * IN NO EVENT SHALL APPLE COMPUTER BE LIABLE FOR ANY SPECIAL, INDIRECT, OR 
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM 
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT, 
 * NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION 
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. 
 */
/*
 * MkLinux
 */

#include	<device/device_types.h>
#include	<ppc/POWERMAC/adb_io.h>

#define	ADB_FLAGS_PRESENT	0x00000001	/* Device is present */
#define	ADB_FLAGS_REGISTERED	0x00000002	/* Device has a handler */
#define	ADB_FLAGS_UNRESOLVED	0x00000004	/* Device has not been fully probed */

struct adb_packet {
	unsigned char	a_header[8];
	int		a_hcount;
	unsigned char	a_buffer[32];
	int		a_bcount;
	int		a_bsize;
};

typedef struct adb_packet adb_packet_t;

/*
 * a_flags - used internal by ADB
 */

#define	ADB_IS_ASYNC	0x00001
#define	ADB_DONE	0x00002

struct adb_request {
	decl_simple_lock_data(,a_lock)
	int			a_result;	/* Result of ADB command */
	int			a_flags;	/* used internally */
	adb_packet_t		a_cmd;		/* Command packet */
	adb_packet_t		a_reply;	/* Reply packet */
	void			(*a_done)(struct adb_request *);
	struct adb_request	*a_next;
};

typedef struct adb_request adb_request_t;

struct adb_ops {
	void	(*ao_send)(adb_request_t *);
	void	(*ao_poll)(void);
};

typedef struct adb_ops adb_ops_t;

extern	struct adb_ops	*adb_hardware;

struct adb_device {
	unsigned short	a_dev_type;	/* Device Type (default id) */
	unsigned short	a_dev_handler;	/* Device handler ID */
	unsigned short	a_dev_orighandler;/* Original handler ID */
	unsigned short	a_dev_addr;	/* Device Reg 3 Address */
	int	a_addr;			/* Device Address */
	int	a_flags;
	int	a_type;
	void	(*a_handler)(int number, unsigned char *buffer, int bytes);
};

typedef struct adb_device adb_device_t;

extern struct adb_device	adb_devices[ADB_DEVICE_COUNT];

/*
 * ADB Commands
 */

#define	ADB_DEVCMD_SELF_TEST		0xff
#define	ADB_DEVCMD_CHANGE_ID		0xfe
#define	ADB_DEVCMD_CHANGE_ID_AND_ACT	0xfd
#define	ADB_DEVCMD_CHANGE_ID_AND_ENABLE	0x00

/*
 * Results for a_result
 */

#define	ADB_RET_OK			0	/* Successful */
#define	ADB_RET_INUSE			1	/* ADB Device in use */
#define	ADB_RET_NOTPRESENT		2	/* ADB Device not present */
#define ADB_RET_TIMEOUT			3	/* ADB Timeout  */
#define	ADB_RET_UNEXPECTED_RESULT	4	/* Unknown result */
#define	ADB_RET_REQUEST_ERROR		5	/* Packet Request Error */
#define	ADB_RET_BUS_ERROR		6	/* ADB Bus Error */


/*
 * ADB Packet Types
 */

#define	ADB_PACKET_ADB		0
#define	ADB_PACKET_PSEUDO	1
#define	ADB_PACKET_ERROR	2
#define	ADB_PACKET_TIMER	3
#define	ADB_PACKET_POWER	4
#define	ADB_PACKET_MACIIC	5

/*
 * ADB Device Commands 
 */

#define	ADB_ADBCMD_RESET_BUS	0x00
#define	ADB_ADBCMD_FLUSH_ADB	0x01
#define	ADB_ADBCMD_WRITE_ADB	0x08
#define ADB_ADBCMD_READ_ADB	0x0c

/*
 * ADB Pseudo Commands
 */

#define	ADB_PSEUDOCMD_WARM_START		0x00
#define	ADB_PSEUDOCMD_START_STOP_AUTO_POLL	0x01
#define	ADB_PSEUDOCMD_GET_6805_ADDRESS		0x02
#define	ADB_PSEUDOCMD_GET_REAL_TIME		0x03
#define	ADB_PSEUDOCMD_GET_PRAM			0x07
#define	ADB_PSEUDOCMD_SET_6805_ADDRESS		0x08
#define	ADB_PSEUDOCMD_SET_REAL_TIME		0x09
#define	ADB_PSEUDOCMD_POWER_DOWN		0x0a
#define	ADB_PSEUDOCMD_SET_POWER_UPTIME		0x0b
#define	ADB_PSEUDOCMD_SET_PRAM			0x0c
#define	ADB_PSEUDOCMD_MONO_STABLE_RESET		0x0d
#define	ADB_PSEUDOCMD_SEND_DFAC			0x0e
#define	ADB_PSEUDOCMD_BATTERY_SWAP_SENSE	0x10
#define	ADB_PSEUDOCMD_RESTART_SYSTEM		0x11
#define	ADB_PSEUDOCMD_SET_IPL_LEVEL		0x12
#define	ADB_PSEUDOCMD_FILE_SERVER_FLAG		0x13
#define	ADB_PSEUDOCMD_SET_AUTO_RATE		0x14
#define	ADB_PSEUDOCMD_GET_AUTO_RATE		0x16
#define	ADB_PSEUDOCMD_SET_DEVICE_LIST		0x19
#define	ADB_PSEUDOCMD_GET_DEVICE_LIST		0x1a
#define	ADB_PSEUDOCMD_SET_ONE_SECOND_MODE	0x1b
#define	ADB_PSEUDOCMD_SET_POWER_MESSAGES	0x21
#define	ADB_PSEUDOCMD_GET_SET_IIC		0x22
#define	ADB_PSEUDOCMD_ENABLE_DISABLE_WAKEUP	0x23
#define	ADB_PSEUDOCMD_TIMER_TICKLE		0x24
#define	ADB_PSEUDOCMD_COMBINED_FORMAT_IIC	0X25

/*
 * Macros to help build commands up
 */

#define	ADB_BUILD_CMD1(c, p1) {(c)->a_cmd.a_header[0] = p1; (c)->a_cmd.a_hcount = 1; }
#define	ADB_BUILD_CMD2(c, p1, p2) {(c)->a_cmd.a_header[0] = p1; (c)->a_cmd.a_header[1] = p2; (c)->a_cmd.a_hcount = 2; }
#define	ADB_BUILD_CMD3(c, p1, p2, p3) {(c)->a_cmd.a_header[0] = p1; (c)->a_cmd.a_header[1] = p2; (c)->a_cmd.a_header[2] = p3; (c)->a_cmd.a_hcount = 3; }
#define	ADB_BUILD_CMD4(c, p1, p2, p3, p4) {(c)->a_cmd.a_header[0] = p1; (c)->a_cmd.a_header[1] = p2; \
					 (c)->a_cmd.a_header[2] = p3; (c)->a_cmd.a_header[3] = p4; (c)->a_cmd.a_hcount = 4; }
#define	ADB_BUILD_CMD2_BUFFER(c, p1, p2, len, buf) {(c)->a_cmd.a_header[0] = p1; (c)->a_cmd.a_header[1] = p2; (c)->a_cmd.a_hcount = 2;\
		(c)->a_cmd.a_bcount = len;\
		memcpy(&(c)->a_cmd.a_buffer, buf, len); }

#define	adb_init_request(a)	{ bzero((char *) a, sizeof(*a)); if (!adb_polling) simple_lock_init(&(a)->a_lock, ETAP_IO_TTY); }

/*
 * Prototypes
 */

void		adb_register_handler(int id,
			void (*handler)(int number, unsigned char *packet, int count));
void		adb_register_dev(int id,
			void (*handler)(int number, unsigned char *packet, int count));
int		adb_readreg(int devnum, int regnum, unsigned short *value);
int		adb_writereg(int devnum, int regnum, unsigned short value);
void		adb_writereg_async(int devnum, int regnum, unsigned short value);
void		adb_send(adb_request_t *req, boolean_t wait);
void		adb_done(adb_request_t *req);
void		adb_unsolicited_done(adb_packet_t *response);
adb_request_t	*adb_next_request(void);
void		adb_poll(void);

extern boolean_t	adb_polling;
void		adb_set_handler(struct adb_device *devp, int handler);

/*
 * For configuration
 */

io_return_t	adbopen(dev_t dev, dev_mode_t flag, io_req_t ior);
void		adbclose(dev_t dev);
io_return_t	adbread(dev_t dev, io_req_t ior);
io_return_t	adbwrite(dev_t dev, io_req_t ior);
boolean_t	adbportdeath(dev_t dev, ipc_port_t port);
io_return_t	adbgetstatus(dev_t dev, dev_flavor_t flavor,
		dev_status_t data, mach_msg_type_number_t *status_count);
io_return_t     adbsetstatus(dev_t dev, dev_flavor_t flavor, dev_status_t data,
        		mach_msg_type_number_t status_count);


