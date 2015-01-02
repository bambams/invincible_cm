#include <allegro5/allegro.h>
#include "allegro5/allegro_audio.h"
#include "allegro5/allegro_acodec.h"

#include <stdio.h>
#include <math.h>

#include "m_config.h"
#include "m_globvars.h"

#include "g_header.h"

#include "g_misc.h"

#include "x_sound.h"
#include "x_init.h"
#include "x_synth.h"

#define RESERVED_SAMPLES 16

extern struct sound_configstruct sound_config; // in x_sound.c


void load_in_sample(int s, const char* file_name);
void load_in_msample(int s, const char* file_name);
void build_tone_array(void);

extern struct gamestruct game;

extern ALLEGRO_SAMPLE *sample [SAMPLES];
extern ALLEGRO_SAMPLE *msample [MSAMPLES];

ALLEGRO_EVENT_SOURCE sound_event_source;
ALLEGRO_EVENT_QUEUE *sound_queue;
ALLEGRO_EVENT sound_event;

ALLEGRO_TIMER* sound_timer;

ALLEGRO_THREAD *sound_thread;
int started_sound_thread = 0;

extern float tone [TONES];


void init_sound(void)
{

 settings.sound_on = 1;

//    al_init_acodec_addon(); - shouldn't need this


 if (!al_install_audio()
  || !al_init_acodec_addon())
 {
  fprintf(stdout, "\nAllegro audio installation failed. Starting without sound.");
  settings.sound_on = 0;
  return;
 }


 if (!al_reserve_samples(RESERVED_SAMPLES))
 {
  fprintf(stdout, "\nCould not set up Allegro audio voice/mixer. Starting without sound.");
  settings.sound_on = 0;
  return;
 }

 set_synth(SYNTH_BASE_SINE, // base_type
											7500, // length
											200, // linear attack
											200, // linear decay
											250, // base_freq
											0); // phase
	msample [MSAMPLE_NOTE] = synthesise(0); // synthesise exits on failure to create sample

 set_synth(SYNTH_BASE_SINE, // base_type
											15000, // length
											1000, // linear attack
											1000, // linear decay
											250, // base_freq
											0); // phase
	add_synth_filter(FILTER_SINE_ADD, // type
																		(synth.base_freq / 2), // base freqency
																		0); // phase
	add_synth_filter(FILTER_SINE_ADD, // type
																		(synth.base_freq / 3), // base freqency
																		0); // phase
	add_synth_filter(FILTER_SINE_ADD, // type
																		(synth.base_freq / 4), // base freqency
																		0); // phase
	msample [MSAMPLE_WARBLE] = synthesise(1); // synthesise exits on failure to create sample

 set_synth(SYNTH_BASE_SINE, // base_type
											5000, // length
											200, // linear attack
											500, // linear decay
											250, // base_freq
											0); // phase
	add_synth_filter(FILTER_SINE_ADD, // type
																		(synth.base_freq / 2), // base freqency
																		0); // phase
	add_synth_filter(FILTER_SINE_ADD, // type
																		(synth.base_freq / 3), // base freqency
																		0); // phase
	add_synth_filter(FILTER_SINE_ADD, // type
																		(synth.base_freq / 4), // base freqency
																		0); // phase
	msample [MSAMPLE_NOTE2] = synthesise(2); // synthesise exits on failure to create sample



 //load_in_msample(MSAMPLE_NOTE, "data/sound/amb/soft.wav");
// load_in_msample(MSAMPLE_NOTE2, "data/sound/amb/bell.wav");

/*
 synth.base_type = SYNTH_BASE_SINE;
 synth.length = 15000;
 synth.linear_attack = 1000;
 synth.linear_decay = 1000;
 synth.base_freq = 250;
 synth.phase = 0;
 synth.filters = 1;
 synth.filter[0].type = FILTER_FLANGER;
 synth.filter[0].start_time = 100;
	synthesise(1);

 msample [MSAMPLE_WARBLE] = al_create_sample(synth_sample [1], 15000, SAMPLE_FREQUENCY, ALLEGRO_AUDIO_DEPTH_FLOAT32, ALLEGRO_CHANNEL_CONF_1, 0);

 if (msample [MSAMPLE_WARBLE] == NULL)
	{
		  fprintf(stdout, "\nError: failed to create sample.");
    safe_exit(-1);
	}*/
/*
 synth.base_type = SYNTH_BASE_SINE;
 synth.length = 15000;
 synth.linear_attack = 1000;
 synth.linear_decay = 1000;
 synth.base_freq = 2200;
 synth.phase = 0;
 synth.filters = 4;
 synth.filter[0].type = FILTER_SINE_ADD;
 synth.filter[0].base_freq = (synth.base_freq / 2);
 synth.filter[1].type = FILTER_SINE_ADD;
 synth.filter[1].base_freq = (synth.base_freq / 3);
 synth.filter[2].type = FILTER_SINE_ADD;
 synth.filter[2].base_freq = (synth.base_freq / 4);
 synth.filter[2].phase = 0.1;
// synth.filter[3].type = FILTER_SINE_ADD;
// synth.filter[3].base_freq = (synth.base_freq / 5);

	synthesise(2);

 sample [SAMPLE_CHIRP] = al_create_sample(synth_sample [2], 15000, SAMPLE_FREQUENCY, ALLEGRO_AUDIO_DEPTH_FLOAT32, ALLEGRO_CHANNEL_CONF_1, 0);

 if (sample [SAMPLE_CHIRP] == NULL)
	{
		  fprintf(stdout, "\nError: failed to create sample.");
    safe_exit(-1);
	}
*/


 load_in_sample(SAMPLE_BLIP1, "data/sound/blip.wav");
 load_in_sample(SAMPLE_KILL, "data/sound/kill.wav");
 load_in_sample(SAMPLE_CHIRP, "data/sound/chirp.wav");
 load_in_sample(SAMPLE_OVER, "data/sound/over.wav");
 load_in_sample(SAMPLE_ALLOC, "data/sound/alloc.wav");
 load_in_sample(SAMPLE_NEW, "data/sound/new.wav");

// load_in_amb_sample(AMB_WARBLE, "sound/amb/warble.wav");
// load_in_amb_sample(AMB_NOTE, "sound/amb/note.wav");



 //al_stop_samples();

 build_tone_array();

 sound_config.music_volume = (float) settings.option [OPTION_VOL_MUSIC] / 100;
 sound_config.effect_volume = (float) settings.option [OPTION_VOL_EFFECT] / 100;

// remember to suppress sound when fast-forwarding!

 sound_timer = al_create_timer(0.20); // 0.20
 if (!sound_timer)
 {
    fprintf(stdout, "\nError: failed to create sound timer.");
    safe_exit(-1);
 }
 al_start_timer(sound_timer);
 al_init_user_event_source(&sound_event_source);

 sound_queue = al_create_event_queue();
 al_register_event_source(sound_queue, &sound_event_source);
 al_register_event_source(sound_queue, al_get_timer_event_source(sound_timer));

 sound_event.user.type = ALLEGRO_GET_EVENT_TYPE(1, 0, 4, 0);

 sound_thread = al_create_thread(thread_check_sound_queue, NULL);
 al_start_thread(sound_thread);

 started_sound_thread = 1;

}

void stop_sound_thread(void)
{

 if (started_sound_thread)
 {

  al_join_thread(sound_thread, NULL);

 }

}

void load_in_sample(int s, const char* file_name)
{

 if (settings.sound_on == 0)
  return;

 sample [s] = al_load_sample(file_name);

 if (!sample [s])
 {
  fprintf(stdout, "\nCould not load sample file (%s). Sound disabled.", file_name);
  settings.sound_on = 0;
 }

}


void load_in_msample(int s, const char* file_name)
{

 if (settings.sound_on == 0)
  return;

 msample [s] = al_load_sample(file_name);

 if (!msample [s])
 {
  fprintf(stdout, "\nCould not load sample file (%s). Sound disabled.", file_name);
  settings.sound_on = 0;
 }

}




void build_tone_array(void)
{

 int i;

 for (i = 0; i < TONES; i ++)
 {
  tone [i] = 0.5 * pow(1.05946309, i + 1);
 }


}



