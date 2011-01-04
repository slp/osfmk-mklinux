#ifndef _LINUX_VC_IOCTL_H
#define _LINUX_VC_IOCTL_H

struct vc_mode {
	int	height;
	int	width;
	int	depth;
	int	pitch;
	int	mode;
	char	name[32];
	unsigned long fb_address;
	unsigned long cmap_adr_address;
	unsigned long cmap_data_address;
	unsigned long disp_reg_address;
};

#define VC_GETMODE	0x7667
#define VC_SETMODE	0x7668
#define VC_INQMODE	0x7669

#define VC_SETCMAP	0x766a
#define VC_GETCMAP	0x766b

#endif /* _LINUX_VC_IOCTL_H */
