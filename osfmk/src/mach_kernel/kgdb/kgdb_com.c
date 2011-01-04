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
 * MkLinux
 */

#include <kern/misc_protos.h>
#include <kgdb/kgdb_defs.h>

char kgdb_pkt[PBUFSIZ];		/* In/Out packet buffer */
char kgdb_data[PBUFSIZ/2];	/* In/Out data area */

/*
 * Forward prototypes
 */
void kgdb_decode_array(
        char    *src,
        char    *dst,
        int     *len,
        char    **next);

vm_offset_t kgdb_decode_word(
	char	*src,
	char	**next);

int _kgdb_fromhex(
	int	val);

/*
 * kgdb_putpkt - format and send a command packet to remote gdb.
 *
 */
void
kgdb_putpkt(
	kgdb_cmd_pkt_t	*reply)
{
	int		i,j;
	unsigned char	checksum = 0;
	char		*pkt = kgdb_pkt;
	int		ch;
	char		*buf;	
	int		len;
	
	KGDB_DEBUG(("kgdb_putpkt: cmd 0x%x data addr 0x%x count 0x%x\n", 
		     reply->k_cmd, reply->k_data, reply->k_size));

	/*
	 * Check the size
	 */
	if ((reply->k_size * 2 + 1) > (PBUFSIZ - 5)) {
		kgdb_panic("kgdb_putpkt: packet too large");
		/* NOTREACHED */
	}

	/*
	 * Encapsulate the data into the packet buffer
	 */

	if (reply->k_cmd != 0) {
		*pkt++ = reply->k_cmd;
	}

	if (reply->k_size) {
		buf = (char *) reply->k_data;
		for (i = 0; i < reply->k_size; i++) {
			char	c;
	
			c = kgdb_tohex((buf[i] >> 4) & 0xF);
			*pkt++ = c;
			c = kgdb_tohex(buf[i] & 0xF);
			*pkt++ = c;
		}
	}

	/*
	 * Send the packet and wait for an acknowledgement
	 */
	len = pkt - kgdb_pkt;

	kgdb_putc(STX);		/* Send the packet header */
   
	do {
		KGDB_DEBUG(("kgdb_putpkt:  send packet "));
		for (i = 0; i < len; i++) {
			KGDB_DEBUG(("%c", kgdb_pkt[i]));
		}
		KGDB_DEBUG(("\n"));
		for (i = 0; i < len; i++) {
#if KGDB_RUN_LENGTH_ENCODING
			for (j=i; j<len;j++) {
				if (kgdb_pkt[i] != kgdb_pkt[j])
					break;
			}
			if (j > (i+3)) {
				/* Don't send _too_ much RLE */
				if (j > i+98) /* j-i+28 < 127 */
					j = i+98;
				/* KGDB_DEBUG(("RLE : writing '%c' %d times\n",
				       kgdb_pkt[i],j-i)); */
				kgdb_putc(kgdb_pkt[i]);
				checksum += (int)kgdb_pkt[i];

				kgdb_putc('*');
				checksum += '*';

				kgdb_putc(j-i+28);
				checksum += (j-i+28);

				i=j-1;
				continue;
				/* Doesn't drop into code below */
			}
#endif
			kgdb_putc(kgdb_pkt[i]);
			checksum += kgdb_pkt[i];
		}
		/* Send end of packet and checksum */
		kgdb_putc(ETX);
		kgdb_putc(kgdb_tohex((checksum >> 4) & 0xF));
		kgdb_putc(kgdb_tohex(checksum & 0xF));

		/* Receive acknowledgement */
		if ((ch = kgdb_getc(TRUE)) != ACK)
			delay(1000000);
	} while (ch != ACK);
}

/*
 * kgdb_getpkt - receive and parse a remote gdb command packet
 */
void
kgdb_getpkt(
	kgdb_cmd_pkt_t	*req)
{
	int		c;
	char		*pkt;
	unsigned char	checksum;
	unsigned char	csum1;
	unsigned char	csum2;
	boolean_t	gotany, gotbad;

	KGDB_DEBUG(("kgdb_getpkt: entered\n"));
	while(TRUE) {
		/*
		 * Wait for start of packet character
		 */
		gotany = FALSE;
		KGDB_DEBUG(("kgdb_getpkt: waiting for STX (%c), got: ", STX));
		do {
			c = kgdb_getc(gotany);
			gotany = TRUE;
			if (c == KGDB_GETC_TIMEOUT) {
				KGDB_DEBUG(("<timeout>\n"));
				goto nack;
			}
			if (c == KGDB_GETC_BAD_CHAR)
				KGDB_DEBUG(("<bad char>"));
			else
				KGDB_DEBUG(("%2x", c));
	    	} while (c != STX);
		KGDB_DEBUG(("\n"));

restart_packet:
		/*
		 * Start of packet detected
		 */
		checksum = 0;
		pkt = kgdb_pkt;

		gotbad = FALSE;
		while (TRUE) {
			c = kgdb_getc(TRUE);
			switch (c) {
			case KGDB_GETC_TIMEOUT:
				KGDB_DEBUG(("kgdb_getpkt: timeout\n"));
				goto nack;
			case KGDB_GETC_BAD_CHAR:
				gotbad = TRUE;
				continue;
			case STX:
				KGDB_DEBUG(("kgdb_getpkt: STX again, restart\n"));
				goto restart_packet;
			}
			KGDB_DEBUG(("%c", c));
			if (c == ETX) {
				break;
			}
			if (pkt >= (kgdb_pkt + PBUFSIZ - 1)) {
				kgdb_panic("kgdb_getpkt:  SIZE protocol violation");
				/* NOTREACHED */
			}
			*pkt++ = c;
			checksum += c;
		}

		/*
		 * End of packet detected, terminate string and
		 * verify the checksum
		 */
		*pkt = 0;
		KGDB_DEBUG(("kgdb_getpkt: got \"%s\"\n", kgdb_pkt));
		csum1 = kgdb_fromhex(kgdb_getc(TRUE));
		csum2 = kgdb_fromhex(kgdb_getc(TRUE));
		if (!gotbad && (checksum & 0xFF) == ((csum1 << 4) + csum2)) {
			break;
		}

		/*
		 * Bad checksum, acknowledge and try again.
		 */
		KGDB_DEBUG(("kgdb_getpkt:  bad checksum got 0x%x wanted 0x%x\n",
			   (csum1 << 4) + csum2, checksum &0xFF));	
		KGDB_DEBUG(("%s\n", kgdb_pkt));
nack:
		kgdb_putc(NAK);
	}

	/*
	 * Got a good packet, send an acknowledgement
	 */
	KGDB_DEBUG(("kgdb_getpkt:  sending ACK\n"));
	KGDB_DEBUG(("%s\n", kgdb_pkt));
	kgdb_putc(ACK);

	/*
	 * Parse the packet
	 */
	pkt = kgdb_pkt;
	req->k_cmd = *pkt++;
	req->k_data = (vm_offset_t) kgdb_data;
	switch (req->k_cmd) {
	    case 'c':
		if (*pkt != 0)
		    req->k_addr = kgdb_decode_word(pkt, &pkt);
		else	
		    req->k_addr = KGDB_CMD_NO_ADDR;
		break;
	    case 'G':
		kgdb_decode_array(pkt,
				  (char *) req->k_data,
				  &req->k_size,
				  &pkt);
		if (req->k_size != REGISTER_BYTES) {
		    KGDB_DEBUG(("kgdb_getpkt: reg count got 0x%s wanted 0x%x\n",
			        req->k_size, REGISTER_BYTES));
		    kgdb_panic("kgdb_getpkt: reg count");
		    /* NOTREACHED */
		}
		break;
	    case 'm':
		req->k_addr = kgdb_decode_word(pkt, &pkt);
		req->k_size = (int)kgdb_decode_word(++pkt, &pkt);
		break;
	    case 'M':
		req->k_addr = kgdb_decode_word(pkt, &pkt);
		req->k_size = (int)kgdb_decode_word(++pkt, &pkt);
		kgdb_decode_array(++pkt,
				  (char *) req->k_data,
				  &req->k_size,
				  &pkt);
		break;
	    case 'P':
		req->k_addr = kgdb_decode_word(pkt, &pkt);
		kgdb_decode_array(++pkt,
				  (char *) req->k_data,
				  &req->k_size,
				  &pkt);
		break;
	    case 's':
		if (*pkt != 0)
		    req->k_addr = kgdb_decode_word(pkt, &pkt);
		else	
		    req->k_addr = KGDB_CMD_NO_ADDR;
		break;
	    default:
		/* Any illegal commands dealt with in kgdb_enter */
		break;
	}
	KGDB_DEBUG(("kgdb_getpkt: cmd %c addr 0x%x size 0x%x data 0x%x *data 0x%x\n", 
	      req->k_cmd, req->k_addr, req->k_size, req->k_data, *((int *) req->k_data)));
}

/*
 * kgdb_decode_aray
 *     - convert a stream of ascii hex digits into an array of integers,
 *	 halting when the first non-hex digit is encountered.
 */
void
kgdb_decode_array(
	char	*src,
	char	*dst,
	int	*len,
	char	**next)
{
	char 		*p = src;
	int		nib1;
	int		nib2;

	while (TRUE) {
		nib1 = _kgdb_fromhex(*p++);
		if (nib1 == -1) {
			--p;
			break;
		}
		nib2 = _kgdb_fromhex(*p++);
		if (nib2 == -1) {	
		    KGDB_DEBUG(("kgdb_decode:  bad nibble 0x%x\n", *--p));
		    kgdb_panic("kgdb_decode: bad nibble\n");
		    /* NOTREACHED */
		}
		*dst++ = (nib1 * 16) + nib2;
	}
	if (len)
		*len = (p - src)/2;
	*next = p;	
}

/*
 * kgdb_decode_word
 *     - convert a stream of ascii hex digits into a word of data.
 *	 The stream may have an odd number of characters. Terminate
 *       when first non-hex digit is encountered
 */ 
vm_offset_t
kgdb_decode_word(
	char	*src,
	char	**next)
{
	char 		*p = src;
	vm_offset_t	result = 0;
	unsigned int	nib;
	int		i;

	for (i=0; i < (2*sizeof(vm_offset_t)); i++) {
		nib = _kgdb_fromhex(*p++);
		if (nib == -1) {
			--p;
			break;
		}
		result = (result << 4) + nib;
	}
	*next = p;
	return result;
}

int
kgdb_fromhex(
	int	val)
{
	int	v;

	if ((v = _kgdb_fromhex(val)) == -1) {
		KGDB_DEBUG(("kgdb_fromhex:  bad digit 0x%x\n", val));
		kgdb_panic("kgdb_fromhex:  invalid hex digit");
		/* NOTREACHED */
		return(-1);	/* Pacify ANSI C */
	}
	else {
		return(v);
	}
}

int
_kgdb_fromhex(
	int	val)
{
	if (val >= '0' && val <= '9') {
		return (val - '0');
	}
	else if (val >= 'a' && val <= 'f') {
		return(val - 'a' + 10);
	}
	return(-1);
}

int
kgdb_tohex(
	int	val)
{
	if (val < 10) {
		return(val + '0');
	}
	else {
		return(val + 'a' - 10);
	}
}



