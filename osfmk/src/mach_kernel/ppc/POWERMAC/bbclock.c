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
  Copyright 1988, 1989 by Intel Corporation, Santa Clara, California.

		All Rights Reserved

Permission to use, copy, modify, and distribute this software and
its documentation for any purpose and without fee is hereby
granted, provided that the above copyright notice appears in all
copies and that both the copyright notice and this permission notice
appear in supporting documentation, and that the name of Intel
not be used in advertising or publicity pertaining to distribution
of the software without specific, written prior permission.

INTEL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
IN NO EVENT SHALL INTEL BE LIABLE FOR ANY SPECIAL, INDIRECT, OR
CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT,
NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include <types.h>
#include <mach/message.h>
#include <kern/clock.h>
#include <kern/spl.h>
#include <kern/misc_protos.h>
#include <ppc/proc_reg.h>
#include <ppc/misc_protos.h>
#include <ppc/POWERMAC/bbclock_entries.h>
#include <ppc/POWERMAC/adb.h>
#include <ppc/POWERMAC/pram.h>

/* Forward */

extern int		bbc_config(void);
extern int		bbc_init(void);
extern kern_return_t	bbc_getattr(
				clock_flavor_t		flavor,
				clock_attr_t		attr,
				mach_msg_type_number_t	* count);
extern kern_return_t	bbc_setattr(
				clock_flavor_t		flavor,
				clock_attr_t		attr,
				mach_msg_type_number_t	count);

struct clock_ops  bbc_ops = {
	bbc_config,	bbc_init,	bbc_gettime,	bbc_settime,
	bbc_getattr,	bbc_setattr,	NO_MAPTIME,	NO_SETALRM,
};

/* local data */
extern void	rtcput(
			long	seconds);
extern int	rtcget(
			long	*seconds);

#define	LOCK_BBC()	splclock()
#define	UNLOCK_BBC(s)	splx(s)

#define	SECS_BETWEEN_1904_1970	2082844800

static long	bbc_get_utcdelta(void);

/*
 * Configure battery-backed clock.
 */
int
bbc_config(void)
{
	int		BbcFlag;
	long		rtclk;

#if	NCPUS > 1
	if (cpu_number() != master_cpu)
		return(1);
#endif
	/*
	 * Setup device.
	 */
	/* TODO NMGS */

	/*
	 * Probe the device by trying to read it.
	 */
	BbcFlag = (rtcget(&rtclk) ? 0 : 1);

	if (!BbcFlag)
		printf("WARNING: Battery Clock Failure!\n");

	return (BbcFlag);
}

/*
 * Initialize battery-backed clock.
 */
int
bbc_init(void)
{
	/* nothing to do here unless we wanted to check the
	   sanity of the time value of the clock at boot */
	return (1);
}

/*
 * Get the current clock time.
 */
kern_return_t
bbc_gettime(
	tvalspec_t	*cur_time)	/* OUT */
{
	long		rtclk;
	spl_t		s;
	
	s = LOCK_BBC();
	rtcget(&rtclk);
	cur_time->tv_sec  = rtclk - SECS_BETWEEN_1904_1970;
	cur_time->tv_nsec = 0;
	UNLOCK_BBC(s);

	return (KERN_SUCCESS);
}

/*
 * Set the current clock time.
 */
kern_return_t
bbc_settime(
	tvalspec_t	*new_time)
{
	long		rtclk;
	spl_t		s;

	s = LOCK_BBC();
	rtclk = new_time->tv_sec + SECS_BETWEEN_1904_1970;
	rtcput(rtclk);
	UNLOCK_BBC(s);

	return (KERN_SUCCESS);
}

/*
 * Get clock device attributes.
 */
kern_return_t
bbc_getattr(
	clock_flavor_t		flavor,
	clock_attr_t		attr,		/* OUT */
	mach_msg_type_number_t	*count)		/* IN/OUT */
{
	if (*count != 1)
		return (KERN_FAILURE);
	switch (flavor) {

	case CLOCK_GET_TIME_RES:	/* >0 res */
		*(clock_res_t *) attr = NSEC_PER_SEC;
		break;

	case CLOCK_MAP_TIME_RES:	/* =0 no mapping */
	case CLOCK_ALARM_CURRES:	/* =0 no alarm */
	case CLOCK_ALARM_MINRES:
	case CLOCK_ALARM_MAXRES:
		*(clock_res_t *) attr = 0;
		break;

	default:
		return (KERN_INVALID_VALUE);
	}
	return (KERN_SUCCESS);
}

/*
 * Set clock device attributes.
 */
kern_return_t
bbc_setattr(
	clock_flavor_t		flavor,
	clock_attr_t		attr,		/* IN */
	mach_msg_type_number_t	count)		/* IN */
{
	return (KERN_FAILURE);
}


/* DEVICE SPECIFIC ROUTINES */

int
rtcget(
	long	*rtclk)
{
	spl_t		s;
	adb_request_t	cmd;

	adb_init_request(&cmd);
	ADB_BUILD_CMD2(&cmd, ADB_PACKET_PSEUDO, ADB_PSEUDOCMD_GET_REAL_TIME);
	s = splhigh();
	adb_polling = TRUE;

	adb_send(&cmd, TRUE);

	adb_polling = FALSE;
	splx(s);

	if (cmd.a_result != ADB_RET_OK) 
		return	1;

	*rtclk = (cmd.a_reply.a_buffer[0] << 24) |
		 (cmd.a_reply.a_buffer[1] << 16) |
		 (cmd.a_reply.a_buffer[2] << 8) |
		cmd.a_reply.a_buffer[3];

	*rtclk -= bbc_get_utcdelta();

	return 0;
}	

void
rtcput(
	long	rtclk)
{
	spl_t		s;
	adb_request_t	cmd;

	/* Convert from GMT to localtime for Mac O/S */
	rtclk += bbc_get_utcdelta();

	s = splhigh();
	adb_polling = TRUE;

	adb_init_request(&cmd);
	cmd.a_cmd.a_hcount = 6;
	cmd.a_cmd.a_header[0] = ADB_PACKET_PSEUDO;
	cmd.a_cmd.a_header[1] = ADB_PSEUDOCMD_SET_REAL_TIME;
	cmd.a_cmd.a_header[2] = (rtclk >> 24) & 0xff;
	cmd.a_cmd.a_header[3] = (rtclk >> 16) & 0xff;
	cmd.a_cmd.a_header[4] = (rtclk >> 8) & 0xff;
	cmd.a_cmd.a_header[5] = rtclk & 0xff;

	adb_send(&cmd, TRUE);

	adb_polling = FALSE;
	splx(s);

}

static long
bbc_get_utcdelta(void)
{
	static	boolean_t	have_delta = FALSE;
	static	long		utc_delta = 0;
	long			read_ml;
	struct	pram_machine_location	mloc;

	if (have_delta)
		return	utc_delta;

	read_ml = pram->p_read(PRAM_ML_OFFSET, (char *) &mloc, sizeof(mloc));

	if (read_ml < sizeof(mloc)) 
		return 0;

	utc_delta = mloc.m_utcdelta & 0x00ffffff;

	/* Check to see if the delta needs to be signed.. */
	if ((utc_delta & 0x00800000) == 0x00800000) 
		utc_delta |= 0xFF000000;

	have_delta = TRUE;

	return	utc_delta;
}
