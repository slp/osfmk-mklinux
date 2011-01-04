/*
 *
 *
 *
 */
#include  "../../sound_config.h"
#include "../../../../include/osfmach3/device_utils.h"
#include "../../../../include/osfmach3/device_reply_hdlr.h"

/*------------------------------------------------------
 *
 * types
 *
 *-----------------------------------------------------*/
struct awacs_devc_t
{
  int                dev;
  int                mixer_dev;
  int                audio_speed;
  int                audio_bits;
  int                audio_channels;

  int                output_active;
  int                output_bufs_active;

  int                input_active;

  char               mach_dev_name[16];
  mach_port_t        device_port;
  mach_port_t        reply_port;
};
typedef struct awacs_devc_t awacs_devc_t;


struct awacs_dev_table_t
{
  int                dev;
  int                mixer;
  awacs_devc_t       *devc;
};
typedef struct awacs_dev_table_t awacs_dev_table_t;

#define MAX_AWACS_DEVS     2

/*------------------------------------------------------
 *
 * prototypes
 *
 *-----------------------------------------------------*/
/*
 * awacs_entry.c
 */
int probe_awacs( struct address_info *hw_config );
void attach_awacs( struct address_info *hw_config );
void unload_awacs( struct address_info *hw_config );
int probe_awacs_mpu( struct address_info *hw_config );
void attach_awacs_mpu( struct address_info *hw_config );
void unload_awacs_mpu( struct address_info *hw_config );
awacs_devc_t *awacs_dev_to_devc( int dev );
awacs_devc_t *awacs_mixer_to_devc( int dev );


/*
 * awacs_ops.c
 */ 
int   awacs_audio_open(int dev, int mode );
void  awacs_audio_close( int dev );
void  awacs_output_block( int dev, unsigned long buf, int count, int intrflag, int dma_restart );
void  awacs_input_block( int dev, unsigned long buf, int count, int intrflag, int dma_restart );
int   awacs_audio_ioctl(int dev, unsigned int cmd, caddr_t arg, int local );
int   awacs_audio_prepare_for_input(int dev, int bufsize, int nbufs );
int   awacs_audio_prepare_for_output(int dev, int bufsize, int nbufs );
void  awacs_audio_reset( int dev );
void  awacs_audio_halt_xfer( int dev );
void  awacs_audio_halt_input( int dev );
void  awacs_audio_halt_output( int dev );
void  awacs_audio_trigger( int dev, int bits ); 
int   awacs_audio_set_speed( int dev, int speed );
unsigned int awacs_audio_set_bits( int dev, unsigned int bits );
short awacs_audio_set_channels( int dev, short channels );
int   awacs_mixer_ioctl( int dev, unsigned int cmd, caddr_t arg);


/*-----------------------------------------------------
 *
 * externs
 *
 *----------------------------------------------------*/
extern struct audio_driver     awacs_audio_driver;
extern struct mixer_operations awacs_mixer_driver;
extern int                     awacs_num_devs;
extern awacs_dev_table_t       awacs_devs[MAX_AWACS_DEVS];

/*-----------------------------------------------------
 *
 * 
 *
 *----------------------------------------------------*/
enum awacs_mach_cmds 
{
  AWACS_DEVCMD_RATE = 1,
  AWACS_DEVCMD_OUTPUTVOL,
  AWACS_DEVCMD_INPUTVOL,
  AWACS_DEVCMD_RECSRC,
  AWACS_DEVCMD_HALT
};






