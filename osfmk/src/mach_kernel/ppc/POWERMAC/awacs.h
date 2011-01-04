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

#include "awacs_hw.h"

struct audio_dma_cmd_t
{
  dbdma_command_t            desc_data;
  dbdma_command_t            desc_seq;
  dbdma_command_t            desc_stop;
};
typedef struct audio_dma_cmd_t audio_dma_cmd_t;

struct seq_num_t
{
  volatile ulong_t      *seq_num_virt;
  ulong_t                seq_num_phys;
  volatile ulong_t       seq_num_exp;
  ulong_t                seq_num_last;
  volatile ulong_t       num_active;
  ulong_t                max_buffers;
};
typedef struct seq_num_t seq_num_t;

struct awacs_devc_t 
{
  decl_simple_lock_data(,lock)
  int                    dev;
  boolean_t           is_present;
  awacs_regmap_t        *awacs_regs;

  dbdma_regmap_t      *input_dbdma;
  audio_dma_cmd_t       *input_cmds;
  seq_num_t              input_seq;

  dbdma_regmap_t      *output_dbdma;
  audio_dma_cmd_t       *output_cmds;
  seq_num_t              output_seq;   

  int                    rate;
  int                    recsrc;
  int			output_volume;
  int			input_volume;
  unsigned long		status;
  
  int                   SoundControl;
  int                   CodecControl[5];
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


/*
 *
 *
 */

int awacs_probe(caddr_t port, void *ui );
void awacs_attach( struct bus_device *dev );

io_return_t awacs_open( int dev, dev_mode_t mode, io_req_t ior );
void        awacs_close( int dev );
io_return_t awacs_write( int dev, io_req_t ior );
io_return_t awacs_read( int dev, io_req_t ior );
io_return_t awacs_setstat( dev_t dev, dev_flavor_t flavor, dev_status_t data,
			   mach_msg_type_number_t count );
io_return_t awacs_getstat( dev_t dev, dev_flavor_t flavor, dev_status_t data,
			   mach_msg_type_number_t *count );
void awacs_input_int(int device, struct ppc_saved_state *ssp);
void awacs_output_int(int device, struct ppc_saved_state *ssp);

io_return_t awacs_sg_space_avail( awacs_devc_t *devc, io_req_t ior, int mode );

io_return_t awacs_sg_io( awacs_devc_t *devc, io_req_t ior, int mode );

int awacs_add_audio_buffer( awacs_devc_t *devc,  char *buf,  int count, 
			    void * ior, int mode );

awacs_devc_t *awacs_dev_to_devc( int dev );

