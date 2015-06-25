#include "math.h"
#include <stdio.h>

#define ALSA_PCM_NEW_HW_PARAMS_API
#include <alsa/asoundlib.h>

#define RONAN

#ifndef WIN32 // Fake windows stuff
#define __stdcall
#endif

#include "ronan.cpp"


int main() {
  syWRonan ronan;

  memset(&ronan, 0, sizeof(syWRonan));

  ronan.texts[0] = "!AY l4AHv yUW nAOr3AH_";
  ronanCBInit(&ronan);
  ronanCBSetSR(&ronan, 44100);
  ronanCBSetCtl(&ronan, 4, 70);
  ronanCBSetCtl(&ronan, 5, 100);

  long loops;
  int rc;
  int size;
  snd_pcm_t *handle;
  snd_pcm_hw_params_t *params;
  unsigned int val;
  int dir;
  snd_pcm_uframes_t frames;
  char *buffer;


  /* Open PCM device for playback. */
  rc = snd_pcm_open(&handle, "default",
                    SND_PCM_STREAM_PLAYBACK, 0);
  if (rc < 0) {
    fprintf(stderr,
            "unable to open pcm device: %s\n",
            snd_strerror(rc));
    exit(1);
  }

  /* Allocate a hardware parameters object. */
  snd_pcm_hw_params_alloca(&params);

  /* Fill it in with default values. */
  snd_pcm_hw_params_any(handle, params);

  /* Set the desired hardware parameters. */

  /* Interleaved mode */
  snd_pcm_hw_params_set_access(handle, params,
                      SND_PCM_ACCESS_RW_INTERLEAVED);

  /* Signed 16-bit little-endian format */
  snd_pcm_hw_params_set_format(handle, params,
                              SND_PCM_FORMAT_S16_LE);

  /* Two channels (stereo) */
  snd_pcm_hw_params_set_channels(handle, params, 2);

  /* 44100 bits/second sampling rate (CD quality) */
  val = 44100;
  snd_pcm_hw_params_set_rate_near(handle, params,
                                  &val, &dir);

  /* Set period size to 128 frames. */
  frames = 128;
  snd_pcm_hw_params_set_period_size_near(handle,
                              params, &frames, &dir);

  /* Write the parameters to the driver */
  rc = snd_pcm_hw_params(handle, params);
  if (rc < 0) {
    fprintf(stderr,
            "unable to set hw parameters: %s\n",
            snd_strerror(rc));
    exit(1);
  }

  /* Use a buffer large enough to hold one period */
  snd_pcm_hw_params_get_period_size(params, &frames,
                                    &dir);
  size = frames * 4; /* 2 bytes/sample, 2 channels */
  buffer = (char *) malloc(size);

  snd_pcm_hw_params_get_period_time(params,
                                    &val, &dir);

  loops = 4 *1000*1000 / val;

  unsigned long counter = 0;
  short *bufferShort = (short *) buffer;

  float floatBuffer[frames*2];

  long noteSpeed = 300*128/frames;
  long tickCounter = 0;

  while (loops > 0) {
    loops--;

	ronanCBTick(&ronan);

	if (tickCounter%noteSpeed == 0) {
		printf("Note on\n");
		ronanCBNoteOn(&ronan);
	}
	if (tickCounter%noteSpeed == (noteSpeed*99/100)) {
		printf("Note off\n");
		ronanCBNoteOff(&ronan);
	}

    tickCounter ++;

    for(unsigned short frame=0; frame<frames ;frame++) {
    	short sigShort = ((counter*80)%20000)-10000;
        float sig = ((float)sigShort)/10000;
		//sig = sigShort>0 ? 1.0 : -1.0;
        floatBuffer[frame*2] = sig;
        floatBuffer[frame*2+1] = sig;
        counter += 1;
    }

    ronanCBProcess(&ronan, floatBuffer, frames);

    for(unsigned short frame=0; frame<frames ;frame++) {
            bufferShort[frame*2] = floatBuffer[frame*2]*10000;
            bufferShort[frame*2+1] = floatBuffer[frame*2+1]*10000;
    }

    rc = snd_pcm_writei(handle, buffer, frames);
    if (rc == -EPIPE) {
      /* EPIPE means underrun */
      fprintf(stderr, "underrun occurred\n");
      snd_pcm_prepare(handle);
    } else if (rc < 0) {
      fprintf(stderr,
              "error from writei: %s\n",
              snd_strerror(rc));
    }  else if (rc != (int)frames) {
      fprintf(stderr,
              "short write, write %d frames\n", rc);
    }
  }

  snd_pcm_drain(handle);
  snd_pcm_close(handle);
  free(buffer);

  return 0;
}
