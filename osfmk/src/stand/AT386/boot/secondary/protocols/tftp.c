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
 * TODO
 *
 *	- dynamically allocate the net buffers
 *	- the cache should be made smarter (recognize how
 *	much memory there is and cache the whole file in
 *	high memory if possible)
 */
/*
 * File : tftp.c
 *
 * Author : Eric PAIRE (O.S.F. Research Institute)
 *
 * This file contains TFTP functions used for Network bootstrap.
 */


#include <secondary/net.h>
#include <secondary/net/ns8390.h>
#include <secondary/net/3c503.h>
#include <secondary/net/dlink.h>
#include <secondary/net/endian.h>
#include <secondary/protocols/udpip.h>
#include <secondary/protocols/arp.h>
#include <secondary/protocols/tftp.h>

u16bits tftp_port;			/* current TFTP client port */

static u16bits tftp_waitfor;		/* packet waited for */
static unsigned tftp_retransmit;	/* current TFTP retransmission timer */
static enum tftp_state tftp_state;	/* current TFTP state */
/* Ack and error buffer, malloc? */
static u8bits tftp_buffer[DLINK_MAXHLEN +
			  sizeof (struct frame_udp) +
			  sizeof (struct frame_ip) +
			  /* tftp_3, since the largest */
			  sizeof (struct frame_tftp_3)];

int
tftp_init()
{
	tftp_port = 0;
}

static int
tftp_output_req(char *file)
{
	union frame_tftp *tp;
	struct udpip_output udpip;
	char *p;
	char *q;

	PPRINT(("tftp_output REQ ('%s')\n", file));

	tp = (union frame_tftp *)&udpip_buffer[dlink.dl_hlen +
					       sizeof (struct frame_ip) +
					       sizeof (struct frame_udp)];

	tp->u1.tftp_opcode = htons(TFTP_OPCODE_READ);
	q = tp->u1.tftp_request;
	p = file;
	while (*q++ = *p++)
		continue;
	p = TFTP_MODE;
	while (*q++ = *p++)
		continue;

	udpip.udpip_sport = tftp_port;
	udpip.udpip_dport = IPPORT_TFTP;
	udpip.udpip_src = udpip_laddr;
	udpip.udpip_dst = udpip_raddr;
	udpip.udpip_len = (q - tp->u1.tftp_request) + sizeof (u16bits);
	udpip.udpip_buffer = udpip_buffer;

	return udpip_output_with_protocol(&udpip,IPPROTO_UDP);
}

static int
tftp_output_ack(u16bits block)
{
	union frame_tftp *tp;
	struct udpip_output udpip;
#ifdef	TFTPDEBUG
	PPRINT(("tftp_output ACK <b=%d>\n", block));
#endif
	tp = (union frame_tftp *)&tftp_buffer[dlink.dl_hlen +
					      sizeof (struct frame_ip) +
					      sizeof (struct frame_udp)];

	tp->u4.tftp_opcode = htons(TFTP_OPCODE_ACK);
	tp->u4.tftp_block = htons(block);

	udpip.udpip_sport = tftp_port;
	udpip.udpip_dport = udpip_tftp;
	udpip.udpip_src = udpip_laddr;
	udpip.udpip_dst = udpip_raddr;
	udpip.udpip_len = 2 * sizeof (u16bits);
	udpip.udpip_buffer = tftp_buffer;

	return udpip_output_with_protocol(&udpip,IPPROTO_UDP);
}

static int
tftp_output_error(u16bits error,
		  char *msg)
{
	union frame_tftp *tp;
	struct udpip_output udpip;
	unsigned i;

	PPRINT(("tftp_output ERR (0x%x,'%s')\n", error, msg));

	tp = (union frame_tftp *)&tftp_buffer[dlink.dl_hlen +
					      sizeof (struct frame_ip) +
					      sizeof (struct frame_udp)];

	tp->u5.tftp_opcode = htons(TFTP_OPCODE_ERROR);
	tp->u5.tftp_errorcode = htons(error);
	for (i = 0; i < 511 && msg[i] != '\0'; i++)
		tp->u5.tftp_errormsg[i] = msg[i];
	tp->u5.tftp_errormsg[i++] = '\0';

	udpip.udpip_sport = tftp_port;
	udpip.udpip_dport = udpip_tftp;
	udpip.udpip_src = udpip_laddr;
	udpip.udpip_dst = udpip_raddr;
	udpip.udpip_len = 2 * sizeof (u16bits) + i;
	udpip.udpip_buffer = tftp_buffer;

	return udpip_output_with_protocol(&udpip,IPPROTO_UDP);
}

#define	BOOT_COPY_REPORT	4
#define	BOOT_COPY_ACTIVITY(block)					\
	if (((block) & ((1 << BOOT_COPY_REPORT) - 1)) == 0) {		\
		switch ((block) % (1 << (BOOT_COPY_REPORT + 2))) {	\
		case 0:							\
			putchar('-');					\
			break;						\
		case (1 << BOOT_COPY_REPORT):				\
			putchar('\\');					\
			break;						\
		case (1 << BOOT_COPY_REPORT) * 2:			\
			putchar('|');					\
			break;						\
		case (1 << BOOT_COPY_REPORT) * 3:			\
			putchar('/');					\
			break;						\
		}							\
		putchar('\b');						\
	}


#define	FIRST_BLOCKS_CACHE
#define	MOVING_CACHE
#ifdef	FIRST_BLOCKS_CACHE
extern 	char *io_buf;
char	*tftp_cache_header;
char	*tftp_cache_tmp;
int	tftp_blocks_cached_header;

#define	TMP_CACHE_SIZE	8	/* (0x1000/512) */

int	tftp_cache_index[TMP_CACHE_SIZE];
int	tftp_first_index;
int	tftp_last_index;
int	tftp_cache_full;	 /* because last==first is empty cache also */
char 	*tftp_save_buffer;
#else
static char tftp_save_buffer[512];
#endif

static int tftp_save_count = 0;	/* count of valid data left in buffer */
static int tftp_save_offset;	/* offset of valid data in buffer */
static char tftp_file[128];
static long tftp_filesize;
static long tftp_fileoffset;
int	port_unreachable;

int
tftp_input(struct udpip_input *udpip_input)
{
	union frame_tftp *tp;
	u16bits block;
	unsigned datalen;
	u16bits error;

	if (udpip_input->udpip_len < sizeof (u16bits))
		return (0);

	tp = (union frame_tftp *)udpip_input->udpip_addr;

#ifdef	 TFTPDEBUG
	PPRINT(("tftp_input(0x%x, 0x%x)\n",
		       tftp_state, tp->u1.tftp_opcode));
#endif
	if ( !(tp->u1.tftp_opcode == TFTP_OPCODE_ERROR 
	       && tftp_state == TFTP_STATE_ABORTING)
	    && tftp_state != TFTP_STATE_RUNNING && tftp_state != TFTP_STATE_START)
		return (0);

	switch (ntohs(tp->u1.tftp_opcode)) {
	    case TFTP_OPCODE_DATA:
		if (udpip_input->udpip_len < 2 * sizeof (u16bits))
			return (0);
		block = ntohs(tp->u3.tftp_block);
#ifdef	TFTPDEBUG
		PPRINT(("received DATA <b=%d, %d b, read: %d>\n",
			       block, 
			       udpip_input->udpip_len - 2 * sizeof (u16bits),
			       tftp_filesize));
#endif
		if (block != tftp_waitfor)
			return (0);

		tftp_state = TFTP_STATE_RUNNING;

#if 0
		if (tftp_save_count != 0) {
			printf("tftp error: overwriting not done\n");
			/* XXX report more errors? */
		} else
#endif		       
		{
			(void)tftp_output_ack(block);
			tftp_retransmit = ((tftp_retransmit * 9) + getelapsed()) / 10;
			if (tftp_retransmit == 0)
				tftp_retransmit = 1;
			settimeout(tftp_retransmit);
			datalen = udpip_input->udpip_len - 2 * sizeof (u16bits);
			tftp_save_count = datalen;
			tftp_save_offset = 0;
#ifdef	FIRST_BLOCKS_CACHE
			if (tftp_filesize <= 0x1000-512) {
				/* cache first blocks of the file 
				 * since they contain the header...
				 */
				if(!io_buf) {
					/* keep the value in accordance
					 * with secondary.c
					 */
				    io_buf = (char *)malloc(0x2000);
				    tftp_cache_header = io_buf;
				    tftp_cache_tmp = &io_buf[0x1000];
			        }
				tftp_blocks_cached_header |= (1 << (tftp_waitfor-1));
				memcpy((char *)tftp_cache_header+tftp_filesize,
				       tp->u3.tftp_data,
				       tftp_save_count);
				tftp_save_buffer = tftp_cache_header+tftp_filesize;
			}
#ifdef	MOVING_CACHE
			else {
				int nextindex;
				/* we are not in the first page of the file
				 * hence save the data in a rollover buffer
				 * XXX should take away modulo, since
				 * we have a power of 2 for cache
				 * make it with 'and' mask.
				 */
/*
 * firstindex points to the first empty buffer
 * lastindex points to the oldest buffer containing valid data
 */
				nextindex = (tftp_first_index + 1) % TMP_CACHE_SIZE;
				tftp_cache_index[tftp_first_index] = tftp_waitfor-1;
				if (tftp_first_index == tftp_last_index && tftp_cache_full) {
					/* the new position is the last block of the cache
					 * -> adjust the last position of the cache
					 */
					tftp_last_index = (tftp_last_index + 1) % TMP_CACHE_SIZE;
				}
#ifdef	TFTPDEBUG
				printf("caching b=%d %x (%d - %d)\n",
				       tftp_waitfor-1, tftp_filesize,
				       tftp_first_index, tftp_last_index);
#endif
				memcpy((char *)tftp_cache_tmp+tftp_first_index*512,
				       tp->u3.tftp_data,
				       tftp_save_count);
				tftp_save_buffer = tftp_cache_tmp+tftp_first_index*512;
				tftp_first_index = nextindex;
				if (tftp_first_index == tftp_last_index)
					tftp_cache_full = 1;
			}
#else	/* MOVING CACHE */
			tftp_save_buffer = &io_buf[0x1000];
			bcopy(tp->u3.tftp_data, tftp_save_buffer, datalen);
#endif
#else	/* first block cache */
			bcopy(tp->u3.tftp_data, tftp_save_buffer, datalen);
#endif
			tftp_waitfor++;
			tftp_filesize += datalen;

			if (datalen < TFTP_DATA_MAX) {
#ifdef	TFTPDEBUG
				printf("\ntftp: end of '%s'\n%d b read\n",
				       tftp_file,tftp_filesize);
#endif
				tftp_port = 0;
				udpip_abort(udpip_tftp);
				tftp_state = TFTP_STATE_FINISHED;
			}
		}
		break;

	case TFTP_OPCODE_ERROR:
		if (udpip_input->udpip_len < 2 * sizeof (u16bits) + 1)
			return (0);
		error = ntohs(tp->u5.tftp_errorcode);
		PPRINT(("received ERROR %d\n",error));
		switch (tftp_state) {
		case TFTP_STATE_RUNNING:
			printf("\ntftp block error: '%s' #%d (error %d: '%s')\n",
			       tftp_file,
			       tftp_waitfor, error, tp->u5.tftp_errormsg);
			break;
		default:
			printf("\ntftp error: '%s' (error %d: '%s')\n",
			       tftp_file,
			       error, tp->u5.tftp_errormsg);
			break;
		}
		tftp_state = TFTP_STATE_ERROR;
		break;

	default:
		return (0);
	}
	return (1);
}


int
tftp_engine(int  tftp_op,
	    char *file,
	    void *addr,
	    int  count)
{
	extern u16bits udpip_tftp;
	u16bits waitfor;
	unsigned i;

	switch (tftp_op) {
	    case TFTP_ENGINE_START:
		if (tftp_state != TFTP_STATE_START &&
		    tftp_state != TFTP_STATE_FINISHED) {
			int error;
			/*
			 * abort previous transfer
			 */
			error = tftp_engine(TFTP_ENGINE_ABORT,
						(char *)0, (char *)0, 0);
			if(error)
				return error;
		}

		strcpy(tftp_file, file);

		tftp_waitfor = 1;
		tftp_state = TFTP_STATE_FINISHED;
		tftp_port = udpip_port++;
		tftp_save_count = 0;
		tftp_save_offset = 0;
		tftp_filesize = 0;
		tftp_fileoffset = 0;
#ifdef	FIRST_BLOCKS_CACHE
		tftp_blocks_cached_header = 0;
		tftp_last_index = tftp_first_index = 0;
		tftp_cache_full = 0;
#endif
#if 0
		PPRINT(("tftp(START, '%s', port %d, 0x%x, %d)\n",
			       tftp_file, tftp_port, addr, count));
#endif
#ifdef	TFTPDEBUG
		printf("\nRequesting %s (port %d)\n",
			       tftp_file, tftp_port);
#endif
		i = 0;
		port_unreachable = 0;
		tftp_retransmit = 4000;
		settimeout(tftp_retransmit);

		while (i < TFTP_RETRANSMIT_MAX) {
			if (tftp_state == TFTP_STATE_FINISHED || isatimeout()) {
				tftp_state = TFTP_STATE_START;
				PPRINT(("TFTP initial request %s\n",
					       isatimeout()?"(timed out)":""));
				(void)tftp_output_req(file);

				settimeout(tftp_retransmit);
				tftp_retransmit <<= 1;
				if (tftp_retransmit > TFTP_RETRANSMIT)
					tftp_retransmit = TFTP_RETRANSMIT;
				i++;
#ifdef	TFTPDEBUG
				putchar('.');
#else
				BOOT_COPY_ACTIVITY(i);
#endif
				udpip_tftp = 0;		/* tftp server port */
			}
			if (tftp_state != TFTP_STATE_START)
				break;
			if (is_intr_char() || (port_unreachable == 1)) {
				tftp_port = 0;
				udpip_abort(udpip_tftp);
				tftp_state = TFTP_STATE_FINISHED;
				return (NETBOOT_ABORT);
			}
			(*dlink.dl_input)(udpip_buffer, IP_MSS);
		}
#ifdef	TFTP_DEBUG
		putchar('\n');
#endif
		if (i >= TFTP_RETRANSMIT_MAX) {
			printf("\ntftp time-out\n");
			tftp_port = 0;
			udpip_abort(udpip_tftp);
			tftp_state = TFTP_STATE_FINISHED;
			return (NETBOOT_ERROR);
		}
		/*
		 * From this point, we assume that:
		 * tftp's first request arrived in server
		 * server has sent first block which is saved in tftp_save_buffer
		 * From here now, don't procrastinate with future reads
		 * ASSERT(tftp_state == RUNNING) or FINISHED, if file < 512
		 */
		return (NETBOOT_SUCCESS);

	    case TFTP_ENGINE_READ:
	    {
		int size;
		extern int poff;
		int offsetadjust; /* amount of offset adjustment */
		int readblindly;  /* read file forward, not keeping data */
		int savecount;    /* when readblindly, save requested count */

		if (tftp_state != TFTP_STATE_RUNNING &&
		    tftp_state != TFTP_STATE_FINISHED) {
			printf("tftp read bad state: %d\n", tftp_state);
			tftp_state = TFTP_STATE_FINISHED;
			return (NETBOOT_ERROR);
		}

		/*
		 * Take into account data already received before
		 * requesting more data
		 */
/*
 * warning (passed as global variables):
 *	poff	offset where to begin to read in the file
 */
		readblindly = 0;
start_read:
		if(readblindly) {
			count = savecount;	/* restore requested count */
			readblindly = 0;	/* stop the mechanism */
#ifdef	TFTPDEBUG
			printf("REAL READ begins: poff= x%x foff= x%x count= %d\n",
			       poff, tftp_fileoffset, count);
#endif
		}

		offsetadjust = poff - tftp_fileoffset;
		if (offsetadjust != 0) {

#ifdef	TFTPDEBUG
			printf("need to adjust file offset by %d\n",offsetadjust);
#endif
			if (offsetadjust < 0) {
			    int error;
#ifdef	FIRST_BLOCKS_CACHE
			    int firstblock, lastblock, blockoffset;
			
			    /* here we have read the file at most once
			     * hence the cache can be better used...
			     */
			    firstblock = poff / 512;
			    blockoffset = poff % 512;
			    lastblock = (poff+count) / 512;

			    if (poff <= 0x1000) {
					/* data may be in the header cache buffer */
				while ((firstblock <= lastblock) &&
				       (tftp_blocks_cached_header & (1 << firstblock))) {
#ifdef	TFTPDEBUG
					printf("block poff %d cached (%x)\n",
					       firstblock, tftp_blocks_cached_header);
#endif
					firstblock++;
				}
				if (firstblock > lastblock &&
				    0x1000 - poff >= count ) {
					/* the whole region is cached,
					 * we can successfully copy it!
					 */
					memcpy(addr,(char *)io_buf+poff,count);
					return NETBOOT_SUCCESS;
				}
			    } 
#ifdef MOVING_CACHE
                            else if (tftp_cache_full ||
				tftp_first_index != tftp_last_index) {
				    int i, incache = 0;
				    char *saveaddr = addr;
				    int savecount = count;
				    int firstloop;

				    /* check if data is in tmp buffer */
				    i = tftp_last_index;

				    firstloop = tftp_cache_full;
				    while ((i != tftp_first_index) ||
					   (tftp_first_index == i && firstloop)) {
					    /* check data is in cache */
#ifdef	TFTPDEBUG
					    printf("Check cache[%d,%d]\n",
						   i, tftp_cache_index[i]);
#endif
					    if (tftp_cache_index[i] == firstblock) {
						    incache = 1;
						    break;
					    }
					    i = (i+1) % TMP_CACHE_SIZE;
					    firstloop = 0;
				    }
				    if (incache) {
					while ((i != tftp_first_index ||
						(tftp_first_index == i && firstloop))
						&& count > 0) {
						size = 512;
						if(blockoffset)
							size=512-blockoffset;
						if (size > count)
							size = count;
						if (size != 0) {
							char *this_addr=tftp_cache_tmp+i*512;
#ifdef	TFTPDEBUG
							printf("cache cpy(x%x %d[b= %d bof= %d sz= %d])\n",
							       addr, i, tftp_cache_index[i],
							       blockoffset, size);
#endif
							if (this_addr == tftp_save_buffer) {
								/* we copy from the current buffer,
								 * use the normal execution path
								 */
								tftp_fileoffset -= (tftp_save_offset +blockoffset);
								tftp_save_offset = blockoffset;
								tftp_save_count = 512; /* XXX? last block<512 */
								goto full_local_buffer;
							}
							memcpy(addr,
							       (char *)this_addr+blockoffset,
							       size);
							count -= size;
							addr = (char *)addr + size;
							blockoffset = 0;
						} /* size != 0 */
						i = (i+1) % TMP_CACHE_SIZE;
					} /* while */
					if (count == 0) {
						return NETBOOT_SUCCESS;
					}
					printf("?cache adj=%d\n",poff-tftp_fileoffset);
					return NETBOOT_ERROR;
				    } /* incache */
			    } /* cache_full */
#endif	/* second cache */
#endif	/* the two caches */
			    error = tftp_engine(TFTP_ENGINE_START,
						tftp_file, (char *)0, 0);
			    if(error)
				    return error;
			    error = tftp_engine(TFTP_ENGINE_READ,
						(char *)0, addr, count);
			    return error;
		    }
			/* read forward */
			readblindly = 1;
			savecount = count;
			count = offsetadjust;	/* read 'offsetadjust' bytes */
		}

full_local_buffer:
		size = tftp_save_count;
		if (size > count)
				size = count;
		if (size != 0) {
			if (readblindly == 0)
				memcpy(addr,tftp_save_buffer+tftp_save_offset,size);
			count -= size;
			tftp_save_count -= size;
			tftp_save_offset += size;
			tftp_fileoffset += size;
#ifdef	TFTPDEBUG
			if(readblindly == 0) {
				PPRINT(("tftp read('%s', 0x%x, read= %d leftread= %d, leftbuf= %d, foff x%x, poff x%x)\n",
				       tftp_file, addr, size, count,tftp_save_count,tftp_fileoffset,poff));
		        }
#endif
			if(readblindly == 0)
				addr = (char *)addr + size;
		}
		/*
		 * here, count == 0 or >0
		 * if count == 0 tftp_save_count ==0 or >0
		 * else tftp_save_count == 0
		 */
		if (tftp_state != TFTP_STATE_RUNNING && count != 0) {
			/* error as well to read past end of file */
			int error = (tftp_state == TFTP_STATE_FINISHED)?0:1; 
			printf("tftp not running: state=%d\n",
			       tftp_state);
			tftp_state = TFTP_STATE_FINISHED;
			return (NETBOOT_ERROR);
		}
read_more:
#ifdef	TFTPDEBUG
		PPRINT(("tftp(READ, '%s', 0x%x, read= %d leftbuf= %d foff=x%x poff=x%x)\n",
			tftp_file, addr, count, tftp_save_count,
			tftp_fileoffset, poff));
#endif
		while (count) {
			/*
			 * invariant: count > 0, and tftp_save_count == 0
			 *	we must request a new buffer
			 */
			if (!(count > 0 && tftp_save_count == 0))
				printf("tftp read: X?! c=%d, tc=%d\n",
				       count, tftp_save_count);
			waitfor = tftp_waitfor;
			i = 0;
			port_unreachable = 0;
			while (i < TFTP_RETRANSMIT_MAX) {
				if (tftp_state != TFTP_STATE_RUNNING)
					break;	/* error or finished */
				if (tftp_save_count > 0)
					break;	/* we've received data */
				if (isatimeout()) {
					(void)tftp_output_ack(tftp_waitfor - 1);
					tftp_retransmit <<= 1;
					if (tftp_retransmit > TFTP_RETRANSMIT) {
						tftp_retransmit = TFTP_RETRANSMIT;
						i++;
					}
					settimeout(tftp_retransmit);
				}
				if (waitfor != tftp_waitfor) {
					i = 0;
					waitfor = tftp_waitfor;
				}
				if (is_intr_char() || (port_unreachable == 1)) {
					(void)tftp_engine(TFTP_ENGINE_ABORT,
							  (char *)0,(char *)0,0);
					return (NETBOOT_ABORT);
				}
				(*dlink.dl_input)(udpip_buffer, IP_MSS);
			}
			if (i >= TFTP_RETRANSMIT_MAX) {
				printf("\ntftp time-out.\n");
				(void)tftp_engine(TFTP_ENGINE_ABORT,
						  (char *)0,(char *)0,0);
				return (NETBOOT_ERROR);
			}
			switch (tftp_state) {
			    case TFTP_STATE_RUNNING:
				break;
			    case TFTP_STATE_FINISHED:
				if (count <= tftp_save_count)
					/*
					 * if finished but enough data: success
					 */
					break;
				/* FALLTHROUGH */
			    case TFTP_STATE_ERROR:
			    default:
				printf("\ntftp engine error.\n");
				(void)tftp_engine(TFTP_ENGINE_ABORT,
						  (char *)0,(char *)0,0);
				return (NETBOOT_ERROR);
			}
#if	1 /* forfun */
			BOOT_COPY_ACTIVITY(tftp_waitfor);
#endif	/* forfun */
			size = tftp_save_count;
			if (size > count)
				size = count;
			if (readblindly == 0)
				memcpy(addr, tftp_save_buffer+tftp_save_offset,size);
			count -= size;
			tftp_save_count -= size;
			tftp_save_offset += size;
			tftp_fileoffset += size;
#ifdef	TFTPDEBUG
			if (readblindly == 0) {
				PPRINT(("tftp read('%s', 0x%x, read= %d leftread= %d, leftbuf= %d, foff x%x, poff x%x)\n",
				       tftp_file, addr, size, count,tftp_save_count,tftp_fileoffset,poff));
		        }
#endif
			if(readblindly == 0)
				addr = (char *)addr + size;
		} /* while count */

		if(readblindly)
			/* we've read the 'count' bytes of offset adjustment
			   so let's get real data now... */
			goto start_read;
		return (NETBOOT_SUCCESS);
	    }

	    case TFTP_ENGINE_ABORT:
		PPRINT(("tftp(ABORT, '%s', 0x%x, %d)\n",
			       tftp_file, addr, count));
		if (tftp_state == TFTP_STATE_RUNNING) {
			int i;
			tftp_state = TFTP_STATE_ABORTING;
			i = 0;
			tftp_retransmit = 1;
			port_unreachable = 0;	/* set by ICMP */
			settimeout(tftp_retransmit);
			while (i < TFTP_RETRANSMIT_MAX) {
				if (tftp_state == TFTP_STATE_ERROR)
					break;	/* error */
				if (port_unreachable)
					break;
				if (isatimeout()) {
					(void)tftp_output_error(0,"abort file transfer");
					tftp_retransmit <<= 1;
					if (tftp_retransmit > TFTP_RETRANSMIT) {
						tftp_retransmit = TFTP_RETRANSMIT;
						i++;
					}
					settimeout(tftp_retransmit);
				}
				(*dlink.dl_input)(udpip_buffer, IP_MSS);
			}
			tftp_port = 0;
			udpip_abort(udpip_tftp);
		}
		tftp_state = TFTP_STATE_FINISHED;
		return(0);

	    default:
		printf("Unknown tftp engine operation: %d\n",tftp_op);
PDWAIT;
		return(NETBOOT_ERROR);
	}
	/*NOTREACHED*/
	PPRINT(("tftp: unreachable end point\n"));
	return (NETBOOT_ERROR);
}

