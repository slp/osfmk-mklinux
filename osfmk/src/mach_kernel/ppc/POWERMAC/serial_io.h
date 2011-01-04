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

/* START_PPC_HISTORY
 * Revision 1.1.9.1  1996/06/14  08:41:14  emcmanus
 * 	Define CONSOLE_PORT and KGDB_PORT here instead of in ppc/serial_io.c.
 * 	kgdb_putc() and related functions have new prototypes.
 * 	New error return codes for kgdb_getc().  switch_to_*_console()
 * 	functions now use an (int) cookie, not a (void *).
 * 	[1996/06/10  10:49:33  emcmanus]
 *
 * 	kgdb_putc() and kgdb_getc() are now wrappers that call either the
 * 	new functions kgdb_scc_putc() and kgdb_scc_getc(), if running with
 * 	a serial console, or the corresponding vc* functions if running with
 * 	a video console.
 * 	[1996/05/07  09:37:10  emcmanus]
 *
 * Revision 1.1.7.3  1996/05/03  17:27:15  stephen
 * 	Added APPLE_FREE_COPYRIGHT
 * 	[1996/05/03  17:22:36  stephen]
 * 
 * Revision 1.1.7.2  1996/04/27  15:24:21  emcmanus
 * 	Added prototypes for the exported functions from ppc/serial_io.c.
 * 	[1996/04/27  15:04:27  emcmanus]
 * 
 * Revision 1.1.7.1  1996/04/11  09:12:39  emcmanus
 * 	Copied from mainline.ppc.
 * 	[1996/04/11  08:06:20  emcmanus]
 * 
 * Revision 1.1.5.1  1995/11/23  17:39:58  stephen
 * 	first powerpc checkin to mainline.ppc
 * 	[1995/11/23  16:54:19  stephen]
 * 
 * Revision 1.1.2.3  1995/11/02  15:19:22  stephen
 * 	first implementation of serial console from MB oct-1024
 * 	[1995/10/30  08:25:52  stephen]
 * 
 * Revision 1.1.3.2  95/09/05  17:56:29  stephen
 * 	no change
 * 
 * Revision 1.1.2.1  1995/08/25  06:36:32  stephen
 * 	Initial checkin of files for PowerPC port
 * 	[1995/08/23  15:16:07  stephen]
 * 
 * END_PPC_HISTORY
 */

#include <device/device_types.h>
#include <device/io_req.h>
#include <mach_kgdb.h>

/*
 *	Console is on the Printer Port (chip channel 0)
 *	Debugger is on the Modem Port (chip channel 1)
 */

#define	CONSOLE_PORT	0
#define	KGDB_PORT	1

/*
 * function declarations for performing serial i/o
 * other functions below are declared in kern/misc_protos.h
 *    cnputc, cngetc, cnmaygetc
 */

#if MACH_KGDB
void kgdb_putc(char c), no_spl_scc_putc(int chan, char c);
int kgdb_getc(boolean_t timeout), no_spl_scc_getc(int chan, boolean_t timeout);

/* kgdb_getc() special return values. */
#define KGDB_GETC_BAD_CHAR -1
#define KGDB_GETC_TIMEOUT  -2
#endif

void initialize_serial(void);

extern int		scc_probe(
				caddr_t xxx,
				void *bus_device);

extern io_return_t	scc_open(
				dev_t		dev,
				dev_mode_t	flag,
				io_req_t	ior);

extern void		scc_close(
				dev_t		dev);

extern io_return_t	scc_read(
				dev_t		dev,
				io_req_t	ior);

extern io_return_t	scc_write(
				dev_t		dev,
				io_req_t	ior);

extern io_return_t	scc_get_status(
				dev_t			dev,
				dev_flavor_t		flavor,
				dev_status_t		data,
				mach_msg_type_number_t	*status_count);

extern io_return_t	scc_set_status(
				dev_t			dev,
				dev_flavor_t		flavor,
				dev_status_t		data,
				mach_msg_type_number_t	status_count);

extern boolean_t	scc_portdeath(
				dev_t		dev,
				ipc_port_t	port);

extern int	 	scc_putc(
				int			unit,
				int			line,
				int			c);

extern int		scc_getc(
				int			unit,
				int			line,
				boolean_t		wait,
				boolean_t		raw);

/* Functions in serial_console.c for switching between serial and video
   consoles.  */
extern boolean_t	console_is_serial(void);
extern int		switch_to_serial_console(
				void);

extern int		switch_to_video_console(
				void);

extern void		switch_to_old_console(
				int			old_console);

#if MACH_KGDB
extern void		no_spl_putc(char c);
extern int		no_spl_getc(void);
#endif
