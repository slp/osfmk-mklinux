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

/*
 * ADB Driver - used internally by ADB device drivers
 *
 * Currently this is heavily tied to the design  of
 * the CUDA hardware and MACH driver
 */

#include <adb.h>
#include <platforms.h>
#include <debug.h>

#include <mach_kdb.h>
#include <kern/spl.h>
#include <kern/assert.h>
#include <machine/machparam.h>          /* spl definitions */
#include <types.h>
#include <device/io_req.h>
#include <device/tty.h>
#include <device/conf.h>
#include <chips/busses.h>
#include <ppc/misc_protos.h>
#include <ppc/POWERMAC/adb.h>
#include <ppc/POWERMAC/adb_io.h>
#include <ppc/POWERMAC/pram.h>

boolean_t	adb_initted = FALSE;
boolean_t	adb_busy = FALSE;
boolean_t	adb_polling = FALSE;
int		adb_count = 0;

adb_request_t	*adb_queue_root = NULL, *adb_queue_end = NULL;

#define	ADB_POOL_COUNT	16

adb_device_t	adb_devices[ADB_DEVICE_COUNT];
adb_request_t	adb_async_requests[ADB_POOL_COUNT], *adb_async_pool;
struct adb_ops	*adb_hardware;

long	adb_pram_read(long offset, char *buffer, long size);
long	adb_pram_write(long offset, char *buffer, long size);

struct pram_ops adb_pram_ops = {
	adb_pram_read,
	adb_pram_write
};

void adbattach( struct bus_device *device);
int adbprobe(caddr_t addr, void *ui);

struct bus_device	*adb_info[NADB];

struct bus_driver	adb_driver = {
	adbprobe, 0, adbattach, 0, NULL, "adb", adb_info, 0, 0, 0 };

static adb_request_t	adb_reg_data;

decl_simple_lock_data(, adb_lock)

static int
adb_is_dev_present(unsigned short addr)
{
	struct adb_device	*devp = &adb_devices[addr];
	unsigned short value;
	int retval;


	retval = adb_readreg(addr, 3, &value);

#if DEBUG
	if (retval != ADB_RET_TIMEOUT)
		printf("adb_is_dev_present(%2d) returned %d, value %04x\n", addr, retval, value);
#endif /* DEBUG */
	if (retval != ADB_RET_TIMEOUT) {
		devp->a_dev_addr = (value >> 8) & 0xf;
		devp->a_dev_orighandler = devp->a_dev_handler = (value & 0xff);
		devp->a_flags |= ADB_FLAGS_PRESENT;
		return TRUE;
	}


	return FALSE;
}

void
adb_set_handler(struct adb_device *devp, int handler)
{
	unsigned long	retval;
	unsigned short value;


	retval = adb_readreg(devp->a_addr, 3, &value);

	if (retval != ADB_RET_OK) {
		return;
	}

	value = (value & 0xF000) | handler;

	retval = adb_writereg(devp->a_addr, 3, value);

	if (retval != ADB_RET_OK) {
		return;
	}

	retval = adb_readreg(devp->a_addr, 3, &value);

	if (retval != ADB_RET_OK) {
		return;
	}

	/* Update to new handler/id */
	devp->a_dev_handler = (value & 0xff);

}

static void
adb_move_dev(unsigned short from, unsigned short to)
{
	int	addr;


	adb_writereg(from, 3, ((to << 8) | 0xfe));

	addr = adb_devices[to].a_addr;
	adb_devices[to] = adb_devices[from];
	adb_devices[to].a_addr = addr;
	adb_devices[from].a_flags = 0;
	
	return;
}

static boolean_t 
adb_find_unresolved_dev(unsigned short *devnum)
{
	int i;
	struct adb_device	*devp;


	devp = &adb_devices[1];

	for (i = 1; i < ADB_DEVICE_COUNT; i++, devp++) {
		if (devp->a_flags & ADB_FLAGS_UNRESOLVED) {
			*devnum = i;
			return	TRUE;
		}
	}

	return FALSE;
}

static int
adb_find_freedev(void)
{
	struct adb_device	*devp;
	int	i;


	for (i = ADB_DEVICE_COUNT-1; i >= 1; i--) {
		devp = &adb_devices[i];

		if ((devp->a_flags & ADB_FLAGS_PRESENT) == 0) {
			return i;
		}
	}

	panic("ADB: Cannot find a free ADB slot for reassignment!");
	return -1;
}


int
adbprobe(caddr_t addr, void *ui)
{
	adb_request_t	cmd;
	unsigned short	value, devnum, freenum, devlist;
	struct adb_device	*devp;
	int	i;
 	spl_t	s;


	if (adb_initted) {
		simple_unlock(&adb_lock);
		return 1;
	}


	simple_lock_init(&adb_lock, ETAP_IO_DEV);

	for (i = 0; i < ADB_POOL_COUNT; i++) {
		adb_async_requests[i].a_next = adb_async_pool;
		adb_async_pool = &adb_async_requests[i];
	}

	s = spltty();
	adb_polling = TRUE;

	/* Kill the auto poll until a new dev id's have been setup */

	adb_init_request(&cmd);
	ADB_BUILD_CMD3(&cmd, ADB_PACKET_PSEUDO, ADB_PSEUDOCMD_START_STOP_AUTO_POLL, FALSE);
	adb_send(&cmd, TRUE);

	/*
	 * Send a ADB bus reset - reply is sent after bus has reset,
	 * so there is no need to wait for the reset to complete.
	 */

	adb_init_request(&cmd);
	ADB_BUILD_CMD2(&cmd, ADB_PACKET_ADB,
		       ADB_ADBCMD_RESET_BUS | (i << 4));
	adb_send(&cmd, TRUE);

	/*
	 * Okay, now attempt reassign the
	 * bus 
	 */

	/* Skip 0 -- its special! */
	for (i = 1; i < ADB_DEVICE_COUNT; i++) {
		adb_devices[i].a_addr = i;

		if (adb_is_dev_present(i)) {
			adb_devices[i].a_dev_type = i;
			adb_devices[i].a_flags |= ADB_FLAGS_UNRESOLVED;
		}
	}

	/* Now attempt to reassign the addresses */
	while (adb_find_unresolved_dev(&devnum)) {
		freenum = adb_find_freedev();
		adb_move_dev(devnum, freenum);

		if (!adb_is_dev_present(freenum)) {
			/* It didn't move.. damn! */
			adb_devices[devnum].a_flags &= ~ADB_FLAGS_UNRESOLVED;
			printf("WARNING : ADB DEVICE %d having problems "
			       "probing!\n", devnum);
			continue;
		}

		if (!adb_is_dev_present(devnum)) {
			/* no more at this address, good !*/
			/* Move it back.. */

			adb_move_dev(freenum, devnum);

			/* Check the device to talk again.. */
			(void) adb_is_dev_present(devnum);
			adb_devices[devnum].a_flags &= ~ADB_FLAGS_UNRESOLVED;
		} else {
			/* Found another device at the address, leave
			 * the first device moved to one side and set up
			 * newly found device for probing
			 */
			/* device_present already called on freenum above */
			adb_devices[freenum].a_flags &= ~ADB_FLAGS_UNRESOLVED;

			/* device_present already called on devnum above */
			adb_devices[devnum].a_dev_type = devnum;
			adb_devices[devnum].a_flags |= ADB_FLAGS_UNRESOLVED;
#if DEBUG
			printf("Found hidden device at %d, previous device "
			       "moved to %d\n", devnum, freenum);
#endif /* DEBUG */
		}
	}

	/*
	 * Now build up a dev list bitmap and total device count
	 */

	devp = adb_devices;
	devlist = 0;

	for (i = 0; i < ADB_DEVICE_COUNT; i++, devp++) {
		if ((devp->a_flags & ADB_FLAGS_PRESENT) == 0)
			continue;

		devlist |= (1<<i);
		adb_count++;
	}

	adb_init_request(&cmd)
	ADB_BUILD_CMD4(&cmd, ADB_PACKET_PSEUDO, ADB_PSEUDOCMD_SET_DEVICE_LIST, (devlist>>8), devlist & 0xff);
	adb_send(&cmd, TRUE);

	adb_init_request(&cmd);
	ADB_BUILD_CMD3(&cmd, ADB_PACKET_PSEUDO, ADB_PSEUDOCMD_SET_AUTO_RATE, 11);
	adb_send(&cmd, TRUE);

	adb_init_request(&cmd);
	ADB_BUILD_CMD3(&cmd, ADB_PACKET_PSEUDO, ADB_PSEUDOCMD_START_STOP_AUTO_POLL, TRUE);
	adb_send(&cmd, TRUE);

	adb_polling = FALSE;
	splx(s);

	return 1;
}

void
adbattach( struct bus_device *device)
{
	int	i;
	adb_device_t	*devp = &adb_devices[0];

	printf("\nadb0: %d devices on bus.", adb_count);
	for (i = 0; i < ADB_DEVICE_COUNT; i++, devp++) {
		if (devp->a_flags & ADB_FLAGS_PRESENT) {
			printf("\n    %d: ", i);
			switch (devp->a_dev_type) {
			case	ADB_DEV_PROTECT:
				printf("security device");
				break;
			case	ADB_DEV_KEYBOARD:
				printf("keyboard");
				break;
			case	ADB_DEV_MOUSE:
				printf("mouse");
				break;
			case	ADB_DEV_TABLET:
				printf("tablet");
				break;
			case	ADB_DEV_MODEM:
				printf("modem");
				break;
			case	ADB_DEV_APPL:
				printf("application device");
				break;
			default:
				printf("unknown device id=%d", i);
			}
		}
	}
}

/*
 * adbopen
 *
 */

io_return_t
adbopen(dev_t dev, dev_mode_t flag, io_req_t ior)
{
	return	D_SUCCESS;
}

void
adbclose(dev_t dev)
{
	return;
}

io_return_t
adbread(dev_t dev, io_req_t ior)
{
	return	D_INVALID_OPERATION;
}

io_return_t
adbwrite(dev_t dev, io_req_t ior)
{
	return	D_INVALID_OPERATION;
}

boolean_t
adbportdeath(dev_t dev, ipc_port_t port)
{
	return	FALSE;
}


io_return_t
adbgetstatus(dev_t dev, dev_flavor_t flavor, dev_status_t data,
	mach_msg_type_number_t *status_count)
{
	struct	adb_info *info = (struct adb_info *) data;
	struct adb_device *devp;
	int	i;

	switch (flavor) {
	case	ADB_GET_INFO:
		devp = adb_devices; 
       		for (i = 0; i < ADB_DEVICE_COUNT; i++, devp++) {
			if ((devp->a_flags & ADB_FLAGS_PRESENT) == 0)
				continue;

			info->a_addr = devp->a_addr;
			info->a_type = devp->a_dev_type;
			info->a_handler = devp->a_dev_handler;
			info->a_orighandler = devp->a_dev_orighandler;
			info++;
		}
		*status_count = (sizeof(struct adb_info)*adb_count)/sizeof(int);
		break;

	case	ADB_GET_COUNT:
		*((unsigned int *) data) = adb_count;
		*status_count = 1;
		break;

	case	ADB_READ_DATA: {
		struct adb_regdata *reg = (struct adb_regdata *) data;

		reg->a_count = adb_reg_data.a_reply.a_bcount;
		bcopy((char *) adb_reg_data.a_reply.a_buffer,
			(char *) reg->a_buffer, adb_reg_data.a_reply.a_bcount);
		*status_count = (sizeof(struct adb_regdata))/sizeof(int);
	}
		break;
			
	default:
		return	D_INVALID_OPERATION;
	}

	return	D_SUCCESS;
}

io_return_t
adbsetstatus(dev_t dev, dev_flavor_t flavor, dev_status_t data,
	mach_msg_type_number_t status_count)
{
	struct adb_info *info = (struct adb_info *) data;
	struct adb_device *devp;


	switch (flavor) {
	case	ADB_SET_HANDLER:
		if (info->a_addr < 1 || info->a_addr >= ADB_DEVICE_COUNT) 
			return	D_NO_SUCH_DEVICE;

		devp = &adb_devices[info->a_addr];
		adb_set_handler(devp, info->a_handler);
		if (devp->a_dev_handler != info->a_handler) 
			return	D_READ_ONLY;
		break;

	case	ADB_READ_REG: {
		struct adb_regdata *reg = (struct adb_regdata *) data;

		adb_init_request(&adb_reg_data);
		ADB_BUILD_CMD2(&adb_reg_data, ADB_PACKET_ADB,
			(ADB_ADBCMD_READ_ADB|(reg->a_addr<<4)|reg->a_reg));
		
		adb_send(&adb_reg_data, TRUE);

		if (adb_reg_data.a_result != ADB_RET_OK) {
			return D_IO_ERROR;
		}
	}
		break;

	case	ADB_WRITE_REG: {
		struct adb_regdata *reg = (struct adb_regdata *) data;

		adb_init_request(&adb_reg_data);
		ADB_BUILD_CMD2_BUFFER(&adb_reg_data, ADB_PACKET_ADB,
			(ADB_ADBCMD_WRITE_ADB|(reg->a_addr<<4)|reg->a_reg),
			reg->a_count, &reg->a_buffer);

		adb_send(&adb_reg_data, TRUE);

		if (adb_reg_data.a_result != ADB_RET_OK) {
			return D_IO_ERROR;
		}
	}
		break;

	default:
		return	D_INVALID_OPERATION;
	}

	return	D_SUCCESS;
}

/*
 * Register a routine which is to send events from all
 * devices matching a given type.
 */

void
adb_register_handler(int type,
		void (*handler)(int number, unsigned char *packet, int count))
{
	int	dev;
	adb_device_t	*devp;


	for (dev = 0; dev < ADB_DEVICE_COUNT; dev++) {
		devp = &adb_devices[dev];

		if (devp->a_dev_type != type)
			continue;

		devp->a_flags |= ADB_FLAGS_REGISTERED;
		devp->a_handler = handler;
	}

}

/* 
 * Register a routine to handle a dev at a specific address
 */

void
adb_register_dev(int devnum, void (*handler)(int number, unsigned char *packet, int count))
{
	adb_device_t	*devp;


	if (devnum < 0 || devnum > ADB_DEVICE_COUNT)
		panic("adb_register: addr is out of range.");

	devp = &adb_devices[devnum];

	devp->a_flags |= ADB_FLAGS_REGISTERED;
	devp->a_handler = handler;

}

void
adb_done(adb_request_t *req)
{
	adb_packet_t	*pack = &req->a_reply;


	/* Note, adb_busy is not reset because the CUDA chip
	 * needs time to settle back into an idle state.
	 * adb_busy will be reset in adb_next_request() when
	 * the CUDA driver is ready for the next one.
	 */

	if (req->a_flags & ADB_IS_ASYNC) {
		simple_lock(&adb_lock);
		req->a_next = adb_async_pool;
		adb_async_pool = req;
		simple_unlock(&adb_lock);
		return;
	}

	req->a_flags |= ADB_DONE;

	if (!adb_polling) {
		if (req->a_done)
			req->a_done(req);
		else
			thread_wakeup((event_t) req);
	}

}

void
adb_unsolicited_done(adb_packet_t *pack)
{
	adb_device_t	*devp;
	int	devnum;


	/* adb_next_request() will reset this when CUDA
	 * asks for the next request.. until then the bus
	 * has to become idle
	 */

	adb_busy = TRUE;

	devnum = pack->a_header[2] >> 4;
	devp = &adb_devices[devnum];

	if (devp->a_flags & ADB_FLAGS_REGISTERED)
		devp->a_handler(devnum, pack->a_buffer, pack->a_bcount);

}

int
adb_readreg(int number, int reg, unsigned short *value)
{
	adb_request_t	readreg;


	adb_init_request(&readreg);
	ADB_BUILD_CMD2(&readreg, ADB_PACKET_ADB, (ADB_ADBCMD_READ_ADB | number << 4 | reg));

	adb_send(&readreg, TRUE);

	*value = (readreg.a_reply.a_buffer[0] << 8) | readreg.a_reply.a_buffer[1];


	return readreg.a_result;
}

int
adb_writereg(int number, int reg, unsigned short value)
{
	adb_request_t	writereg;


	adb_init_request(&writereg);
	ADB_BUILD_CMD4(&writereg, ADB_PACKET_ADB, (ADB_ADBCMD_WRITE_ADB | number << 4 | reg),
			value >> 8, value & 0xff);

	adb_send(&writereg, TRUE);


	return writereg.a_result;
}

void
adb_writereg_async(int number, int reg, unsigned short value)
{
	adb_request_t	*adb;


	if (adb_async_pool == NULL) 
		panic("ADB: Async Pool empty");

	adb = adb_async_pool;
	adb_async_pool = adb->a_next;

	adb_init_request(adb);
	ADB_BUILD_CMD4(adb, ADB_PACKET_ADB, (ADB_ADBCMD_WRITE_ADB | number << 4 | reg),
			value >> 8, value & 0xff);

	adb->a_flags |= ADB_IS_ASYNC;

	adb_send(adb, FALSE);

}

void
adb_send(adb_request_t *adb, boolean_t wait)
{
	spl_t	s;

	if (!adb_polling) 
		s  = spltty();

	simple_lock(&adb_lock);

	adb->a_reply.a_bsize = sizeof(adb->a_reply.a_buffer);

	if (wait)
		adb->a_done = NULL;

	adb->a_flags &= ~ADB_DONE;
	adb->a_next = NULL;

	if (adb_busy) {
		if (adb_queue_root == NULL)
			adb_queue_root = adb_queue_end = adb;
		else {
			adb_queue_end->a_next = adb;
			adb_queue_end = adb;
		}
		if (adb_polling) {
			adb_hardware->ao_poll();
			if ((adb->a_flags & ADB_DONE) == 0) 
				panic("adb_send: request did not complete?!?");
		}
	} else {
		adb_busy = TRUE;
		adb_hardware->ao_send(adb);
	}

	if (wait && !adb_polling) {
		int count;

		simple_lock(&adb->a_lock);
		simple_unlock(&adb_lock);
		thread_sleep_simple_lock((event_t) adb, simple_lock_addr(adb->a_lock), TRUE);
		simple_lock(&adb_lock);

	}

	simple_unlock(&adb_lock);

	if (!adb_polling)
		splx(s);

}

adb_request_t *
adb_next_request()
{
	adb_request_t	*adb;

	if (!adb_polling)
		simple_lock(&adb_lock);

	adb = adb_queue_root;

	if (adb) {
		adb_queue_root = adb_queue_root->a_next;
		if (adb_queue_root == NULL)
			adb_queue_end = NULL;
		adb_busy = TRUE;
	} else 	adb_busy = FALSE;

	if (!adb_polling)
		simple_unlock(&adb_lock);

	return adb;
}

void
adb_poll(void)
{
	boolean_t	save_poll = adb_polling;


	adb_polling = TRUE;

	adb_hardware->ao_poll();

	adb_polling = save_poll;

}

/* 
 * Pram support
 */

long
adb_pram_read(long offset, char *buffer, long size)
{
	adb_request_t	cmd;
	spl_t		s;


	adb_init_request(&cmd);
	ADB_BUILD_CMD4(&cmd, ADB_PACKET_PSEUDO, ADB_PSEUDOCMD_GET_PRAM,
		(offset>>8), (offset & 0xff));

	s = splhigh();
	adb_polling = TRUE;

	adb_send(&cmd, TRUE);

	adb_polling = FALSE;
	splx(s);

	if (cmd.a_result != ADB_RET_OK) 
		return	0;

	bcopy((char *) &cmd.a_reply.a_buffer, buffer, size);

	return size;
}

long
adb_pram_write(long offset, char *buffer, long size)
{
	printf("adb_pram_write not supported yet\n");
	return 0;
}

