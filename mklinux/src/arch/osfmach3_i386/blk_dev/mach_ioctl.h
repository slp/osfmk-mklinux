/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */

/*
 * XXX hack
 * Redefine IOCTLs because the IOC* definitions are not exported by
 * the micro-kernel, and they are different from the Linux definitions...
 * Hard-code the value here as a quick work-around. This should be undone
 * when the kernel interface for getting disk parameters gets standardized
 * and correctly exported.
 */
#define	OSFMACH3_IOCPARM_MASK	0x1fff		/* parameter length, at most 13 bits */
#define	OSFMACH3_IOC_VOID	0x20000000	/* no parameters */
#define	OSFMACH3_IOC_OUT	0x40000000	/* copy out parameters */
#define	OSFMACH3_IOC_IN		0x80000000	/* copy in parameters */
#define	OSFMACH3_IOC_INOUT	(OSFMACH3_IOC_IN|OSFMACH3_IOC_OUT)

#define _OSFMACH3_IOC(inout,group,num,len) \
	(inout | ((len & OSFMACH3_IOCPARM_MASK) << 16) | ((group) << 8) | (num))
#define	_OSFMACH3_IO(g,n)	_OSFMACH3_IOC(OSFMACH3_IOC_VOID, (g), (n), 0)
#define	_OSFMACH3_IOR(g,n,t)	_OSFMACH3_IOC(OSFMACH3_IOC_OUT,	(g), (n), sizeof(t))
#define	_OSFMACH3_IOW(g,n,t)	_OSFMACH3_IOC(OSFMACH3_IOC_IN, (g), (n), sizeof(t))
#define	_OSFMACH3_IOWR(g,n,t)	_OSFMACH3_IOC(OSFMACH3_IOC_INOUT, (g), (n), sizeof(t))

#define OSFMACH3_V_CONFIG        _OSFMACH3_IOW('v',1,union io_arg)/* Configure Drive */
#define OSFMACH3_V_REMOUNT       _OSFMACH3_IO('v',2)    		/* Remount Drive */
#define OSFMACH3_V_ADDBAD        _OSFMACH3_IOW('v',3,union io_arg)/* Add Bad Sector */
#define OSFMACH3_V_GETPARMS      _OSFMACH3_IOR('v',4,struct disk_parms)   /* Get drive/partition parameters */
#define OSFMACH3_V_FORMAT        _OSFMACH3_IOW('v',5,union io_arg)/* Format track(s) */
#define OSFMACH3_V_PDLOC		_OSFMACH3_IOR('v',6,int)		/* Ask driver where pdinfo is on disk */

#define OSFMACH3_V_ABS		_OSFMACH3_IOW('v',9,int)		/* set a sector for an absolute addr */
#define OSFMACH3_V_RDABS		_OSFMACH3_IOW('v',10,struct absio)/* Read a sector at an absolute addr */
#define OSFMACH3_V_WRABS		_OSFMACH3_IOW('v',11,struct absio)/* Write a sector to absolute addr */
#define OSFMACH3_V_VERIFY	_OSFMACH3_IOWR('v',12,union vfy_io)/* Read verify sector(s) */
#define OSFMACH3_V_XFORMAT	_OSFMACH3_IO('v',13)		/* Selectively mark sectors as bad */
#define OSFMACH3_V_SETPARMS	_OSFMACH3_IOW('v',14,int)	/* Set drivers parameters */
