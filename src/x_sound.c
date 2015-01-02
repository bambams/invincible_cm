#include <allegro5/allegro.h>
#include "allegro5/allegro_audio.h"
#include "allegro5/allegro_acodec.h"

#include <stdio.h>
//#include <math.h>

#include "m_config.h"
#include "m_globvars.h"

#include "g_misc.h"
#include "g_header.h"

#include "x_sound.h"
#include "x_init.h"
#include "x_synth.h"
#include "x_music.h"

extern struct gamestruct game;
extern struct viewstruct view;

ALLEGRO_SAMPLE *sample [SAMPLES];
extern ALLEGRO_SAMPLE *msample [MSAMPLES];
extern ALLEGRO_EVENT_SOURCE sound_event_source;
extern ALLEGRO_EVENT_QUEUE *sound_queue;
extern ALLEGRO_EVENT sound_event;

float tone [TONES];

extern struct synthstruct synth;


void play_interface_sound(int s, int pitch)
{


 if (settings.sound_on == 0
  || game.fast_forward != FAST_FORWARD_OFF)
   return;

// al_play_sample(sample [s], 1.0, 0.5, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);

//bool al_play_sample(ALLEGRO_SAMPLE *spl, float gain, float pan, float speed,
//   ALLEGRO_PLAYMODE loop, ALLEGRO_SAMPLE_ID *ret_id)

 sound_event.user.data1 = s;
 sound_event.user.data2 = pitch;
 sound_event.user.data3 = 0; // pan (-1 to 1)
 sound_event.user.data4 = 80; // vol (0 to 100)

 al_emit_user_event(&sound_event_source, &sound_event, NULL);

}

void play_game_sound(int s, int pitch, int vol, al_fixed x, al_fixed y)
{

 if (settings.sound_on == 0
  || game.fast_forward != FAST_FORWARD_OFF
  || x < view.camera_x - view.centre_x - al_itofix(100)
  || x > view.camera_x + view.centre_x + al_itofix(100)
  || y < view.camera_y - view.centre_y - al_itofix(100)
  || y > view.camera_y + view.centre_y + al_itofix(100))
   return;

 sound_event.user.data1 = s;
 sound_event.user.data2 = pitch;
 sound_event.user.data3 = (al_fixtoi(x - view.camera_x) * 100) / al_fixtoi(view.centre_x); // pan (-1 to 1)
 sound_event.user.data4 = vol; // volume (0-100)

 al_emit_user_event(&sound_event_source, &sound_event, NULL);


}

void	play_view_sound(void)
{

	game.play_sound = 0;

	if (game.fast_forward != 0)
		return;

	if (game.play_sound_note < 0
		|| game.play_sound_note >= TONES)
		return;

	game.play_sound_counter = 5;

 sound_event.user.data1 = SAMPLE_BLIP1;
 sound_event.user.data2 = game.play_sound_note;
 sound_event.user.data3 = 0; // pan (-1 to 1)
 sound_event.user.data4 = game.play_sound_vol; // volume (-1 to 1)

 al_emit_user_event(&sound_event_source, &sound_event, NULL);

}


struct sound_configstruct sound_config;


void *thread_check_sound_queue(ALLEGRO_THREAD *thread, void *arg)
{

// return NULL;

 ALLEGRO_EVENT received_sound_event;
 int event_received;

 sthread_init_mustate(0);

// return NULL;

 while(TRUE)
 {

  event_received = al_wait_for_event_timed(sound_queue, &received_sound_event, 0.1);

// check for a signal that the user has exited:
  if (al_get_thread_should_stop(thread))
   return NULL;

  if (event_received == 0)
   continue;

  if (received_sound_event.type == ALLEGRO_EVENT_TIMER)
  {
//   if (sthread_play_current_piece() == 0)
//    sthread_init_mustate(0);
   continue;
  }


   float pan = (float) received_sound_event.user.data3 / 100;

   if (pan < -1)
    pan = -1;
   if (pan > 1)
    pan = 1;

   al_play_sample(sample [received_sound_event.user.data1], sound_config.effect_volume * (float) received_sound_event.user.data4 * 0.01, pan, tone [received_sound_event.user.data2], ALLEGRO_PLAYMODE_ONCE, NULL);

 };

}



