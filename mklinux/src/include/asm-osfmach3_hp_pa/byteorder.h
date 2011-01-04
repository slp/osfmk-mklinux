/*
 * Copyright (c) 1991-1998 Open Software Foundation, Inc. 
 *  
 * 
 */
/*
 * MkLinux
 */
#ifndef _MACHINE_BYTEORDER_H
#define _MACHINE_BYTEORDER_H

#ifndef __BIG_ENDIAN
#define __BIG_ENDIAN 4321
#endif

#ifndef __BIG_ENDIAN_BITFIELD
#define __BIG_ENDIAN_BITFIELD
#endif

#define ntohl(x) ((unsigned long int) (x))
#define ntohs(x) ((unsigned short int) (x))
#define htonl(x) ((unsigned long int) (x))
#define htons(x) ((unsigned short int) (x))

/*
 * In-kernel byte order macros to handle stuff like
 * byte-order-dependent filesystems etc.
 */
#define cpu_to_le32(x) le32_to_cpu((x))
extern __inline__ unsigned long le32_to_cpu(unsigned long x)
{
     	return (((x & 0x000000ffU) << 24) |
		((x & 0x0000ff00U) <<  8) |
		((x & 0x00ff0000U) >>  8) |
		((x & 0xff000000U) >> 24));
}


#define cpu_to_le16(x) le16_to_cpu((x))
extern __inline__ unsigned short le16_to_cpu(unsigned short x)
{
	return (((x & 0x00ff) << 8) |
		((x & 0xff00) >> 8));
}

#define cpu_to_be32(x) (x)
#define be32_to_cpu(x) (x)
#define cpu_to_be16(x) (x)
#define be16_to_cpu(x) (x)


#endif /* !(_MACHINE_BYTEORDER_H) */

