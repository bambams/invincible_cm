#include <allegro5/allegro.h>
#include "allegro5/allegro_audio.h"
#include "allegro5/allegro_acodec.h"

#include <stdio.h>
#include <math.h>

#include "m_config.h"
#include "m_globvars.h"

//#include "g_header.h"

#include "g_misc.h"

#include "x_sound.h"
#include "x_init.h"
#include "x_synth.h"



/*

How will the synth work?

- This file contains functions that accept a pointer to a float buffer (with a specified size) and fill the buffer with samples based on the current state of the synth struct.
- It's then up to the sound program to use the results.
- *hopefully* this shouldn't affect game performance as it's all run in a different thread.

- The synth struct contains:
 - base frequency
 - phase
 - length
 - an array of filters
  - each filter does something to the sample it's called for

*/

struct synthstruct synth;
float synth_sample [SYNTH_SAMPLES] [SYNTH_SAMPLE_MAX_SIZE];


void set_synth(int base_type, int length, int linear_attack, int linear_decay, int base_freq, int phase)
{

 synth.base_type = base_type;
 synth.length = length;
 synth.linear_attack = linear_attack;
 synth.linear_decay = linear_decay;
 synth.base_freq = base_freq;
 synth.phase = phase;
 synth.filters = 0;	// can be added later

}

int add_synth_filter(int type, float base_freq, float phase)
{

	if (synth.filters >= FILTERS)
		return 0;

	synth.filter[synth.filters].type = type;
//	synth.filter[synth.filters].start_time = start_time; - not currently used
//	synth.filter[synth.filters].end_time = end_time;
	synth.filter[synth.filters].base_freq = base_freq;
	synth.filter[synth.filters].phase = phase;

	synth.filters++;

	return 1;

}



ALLEGRO_SAMPLE* synthesise(int ssi)
{

#ifdef SANITY_CHECK
 if (synth.length >= SYNTH_SAMPLE_MAX_SIZE)
	{
		fprintf(stdout, "\nError: x_synth.c: synthesise(): synth.length too high (%i; max is %i)", synth.length, SYNTH_SAMPLE_MAX_SIZE);
		error_call();
	}
	if (ssi >= SYNTH_SAMPLES)
	{
		fprintf(stdout, "\nError: x_synth.c: synthesise(): synth_sample index too high (ssi %i; max is %i)", ssi, SYNTH_SAMPLES);
		error_call();
	}
#endif

 const float dt = 1.0 / SAMPLE_FREQUENCY;
// float t = 0;
 float ti;//, tj;
 int i, j;
 float total_amplitude = 1;

 for (i = 0; i < synth.length; i ++)
 {
 	ti = synth.phase + i * dt;
 	synth_sample [ssi] [i] = sin(TWOPI * synth.base_freq * ti + 0);
 }

		for (j = 0; j < synth.filters; j ++)
		{
			switch(synth.filter[j].type)
			{
			 case FILTER_SINE_ADD:
			 	total_amplitude += 0.5;
			 	for (i = 0; i < synth.length; i ++)
					{
   	  ti = synth.filter[j].phase + i * dt;
    	 synth_sample [ssi] [i] += sin(TWOPI * synth.filter[j].base_freq * ti + 0);
					}
			 	break;
			 case FILTER_FLANGER: // doesn't work at the moment
			 	total_amplitude += 0.1;
			 	int delay_time;
			 	for (i = 0; i < synth.length; i ++)
					{
						delay_time = ((sin((float) i * dt * TWOPI * 50)) * (float) synth.filter[j].start_time) + synth.filter[j].start_time;
 			 	if (i > delay_time)
	 					synth_sample [ssi] [i] += synth_sample [ssi] [i - delay_time];
					}
				 break;
			}

		}

 	if (synth.linear_attack > 0)
		{
			for (i = 0; i < synth.linear_attack; i ++)
			{
			 synth_sample [ssi] [i] *= i / synth.linear_attack;
		 }
		}

		if (synth.linear_decay > 0)
		{
			for (i = synth.length - synth.linear_decay; i < synth.length; i ++)
			{
			 synth_sample [ssi] [i] *= (synth.length - i) / synth.linear_decay;
			}
		}

		for (i = 0; i < synth.length; i ++)
		{
		 synth_sample [ssi] [i] /= (total_amplitude * 5);
		}

// finally, create the sample

 ALLEGRO_SAMPLE* temp_sample = al_create_sample(synth_sample [ssi], synth.length, SAMPLE_FREQUENCY, ALLEGRO_AUDIO_DEPTH_FLOAT32, ALLEGRO_CHANNEL_CONF_1, 0);

/* fprintf(stdout, "\nCreate sample: ssi %i length %i (%f,%f,%f,%f,%f,%f,%f)", ssi, synth.length,
									synth_sample [ssi] [0],
									synth_sample [ssi] [1],
									synth_sample [ssi] [2],
									synth_sample [ssi] [3],
									synth_sample [ssi] [4],
									synth_sample [ssi] [5],
									synth_sample [ssi] [6]
									);*/

 if (temp_sample == NULL)
	{
		  fprintf(stdout, "\nError: x_synth.c: synthesise(): failed to create sample.");
    error_call();
	}

 return temp_sample;

}







