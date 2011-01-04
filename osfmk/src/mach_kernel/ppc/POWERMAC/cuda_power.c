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

#include <types.h>
#include <mach/message.h>
#include <kern/clock.h>
#include <kern/spl.h>
#include <kern/misc_protos.h>
#include <ppc/POWERMAC/adb.h>
#include <ppc/POWERMAC/powermac.h>
#include <ppc/POWERMAC/powermac_gestalt.h>

/*
 * Power down - and never return. If the hardware doesn't support this,
 * then drop into the debugger if it's there, or loop forever.
 */
void powermac_powerdown(void)
{
	spl_t		s;
	adb_request_t	cmd;

	s = splhigh();
	/* power down */
	if ((powermac_info.machine != gestaltPowerMac6100_60) &&
	    (powermac_info.machine != gestaltPowerMac6100_66) &&
	    (powermac_info.machine != gestaltPowerMac6100_80)) {

		adb_polling = TRUE;
		adb_init_request(&cmd);
		ADB_BUILD_CMD2(&cmd, ADB_PACKET_PSEUDO, ADB_PSEUDOCMD_POWER_DOWN);
		adb_send(&cmd, TRUE);
		adb_polling = FALSE;
		/* If we come back, it's an error */
		printf("PLEASE EMAIL bugs@mklinux.apple.com saying that the shutdown\n");
		printf("isn't correctly configured, SPECIFYING YOUR MACHINE TYPE\n\n");
	}
		/* If we come back, just loop around */
	printf("Is is now safe to switch off your machine\n");
	while(1);
}

/*
 * reboot - and never return.
 */
void powermac_reboot(void)
{
	spl_t		s;
	adb_request_t	cmd;

	s = splhigh();
	adb_polling = TRUE;
	
	adb_init_request(&cmd);
	ADB_BUILD_CMD2(&cmd, ADB_PACKET_PSEUDO, ADB_PSEUDOCMD_RESTART_SYSTEM);
	adb_send(&cmd, TRUE);

	panic("system should have restarted");


}
