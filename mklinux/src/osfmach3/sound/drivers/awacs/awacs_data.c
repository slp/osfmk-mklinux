/*
 *
 *
 *
 */

#include "awacs_types.h"

struct audio_driver awacs_audio_driver =	
{
  awacs_audio_open,
  awacs_audio_close,
  awacs_output_block,
  awacs_input_block,
  awacs_audio_ioctl,
  awacs_audio_prepare_for_input,
  awacs_audio_prepare_for_output,
  awacs_audio_reset,            /* reset          */
  awacs_audio_reset,            /* halt xfer      */
  NULL,				/* local_qlen     */
  NULL,				/* copy_from_user */
  awacs_audio_halt_input,
  awacs_audio_halt_output,
  awacs_audio_trigger,
  awacs_audio_set_speed,
  awacs_audio_set_bits,
  awacs_audio_set_channels
};

struct mixer_operations awacs_mixer_driver =
{ 
  {0},
  {0},
  awacs_mixer_ioctl,
  0
};

int awacs_num_devs = 0;
awacs_dev_table_t awacs_devs[MAX_AWACS_DEVS];

int awacs_rates[8] =
{
  7350,
  8820,
  11025,
  14700,
  17640,
  22050,
  29400,
  44100
};

