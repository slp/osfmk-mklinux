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

#ifndef AWACS_PDM_EXTERNS
#include "awacs_pdm_hw.h"

#define AWACS_DMA_BUFSIZE (64*4*1024)  /* Space for expanded buffers */
#define AWACS_DMA_HARDWARE_BUFSIZE (8*1024)
#define AWACS_NUM_ACTIVE 16

struct awacs_devc_t 
{
  decl_simple_lock_data(,lock)
  int                    dev;
  boolean_t           is_present;
  awacs_regmap_t        *awacs_regs;  
  unsigned char		*DMA_out;
  unsigned long		*DMA_out_buf0, *DMA_out_buf1;
  int			DMA_out_put;
  int			DMA_out_len;
  int			DMA_out_get;
  int			DMA_out_busy;
  int			DMA_out_buf0_busy;
  int			DMA_out_buf1_busy;
  int			DMA_out_ior_put;
  int			DMA_out_ior_get;
  io_req_t		DMA_out_ior_list[AWACS_NUM_ACTIVE];
  int			DMA_out_ior_length[AWACS_NUM_ACTIVE];

  int                   rate;
  int			desired_rate;
  int                   recsrc;
  int			output_volume;
  int			input_volume;
  unsigned long		status;
  
  int                    SoundControl;
  int                    CodecControl[5];
};

typedef struct awacs_devc_t awacs_devc_t;

struct awacs_dev_table_t
{
  int            dev;
  awacs_devc_t   *devc;
};

typedef struct awacs_dev_table_t awacs_dev_table_t;


enum awacs_mach_cmds 
{
  AWACS_DEVCMD_RATE = 1,
  AWACS_DEVCMD_OUTPUTVOL,
  AWACS_DEVCMD_INPUTVOL,
  AWACS_DEVCMD_RECSRC,
  AWACS_DEVCMD_HALT
};

enum sound_device_masks
{
  SOUND_MASK_VOLUME = (1 << 0 ),
  SOUND_MASK_CD     = (1 << 8 ),
  SOUND_MASK_MIC    = (1 << 7 )
};
#endif

/*
 *
 *
 */

int awacs_pdm_probe(caddr_t port, void *ui );
void awacs_pdm_attach( struct bus_device *dev );

io_return_t awacs_pdm_open( int dev, dev_mode_t mode, io_req_t ior );
void        awacs_pdm_close( int dev );
io_return_t awacs_pdm_write( int dev, io_req_t ior );
io_return_t awacs_pdm_read( int dev, io_req_t ior );
io_return_t awacs_pdm_setstat( dev_t dev, dev_flavor_t flavor, dev_status_t data,
			   mach_msg_type_number_t count );
io_return_t awacs_pdm_getstat( dev_t dev, dev_flavor_t flavor, dev_status_t data,
			   mach_msg_type_number_t *count );

