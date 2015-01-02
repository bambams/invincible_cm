/*

This file contains the music-playing functions.

Every function with a name starting with "sthread" should run in the separate music thread.

*/


#include <allegro5/allegro.h>
#include "allegro5/allegro_audio.h"
#include "allegro5/allegro_acodec.h"

#include <stdio.h>
//#include <math.h>

#include "m_config.h"
#include "m_globvars.h"

//#include "g_misc.h"
//#include "g_header.h"

#include "x_sound.h"
#include "x_init.h"
#include "x_synth.h"
#include "x_music.h"



extern ALLEGRO_SAMPLE *sample [SAMPLES];
ALLEGRO_SAMPLE *msample [MSAMPLES];
extern ALLEGRO_EVENT_SOURCE sound_event_source;
extern ALLEGRO_EVENT_QUEUE *sound_queue;
extern ALLEGRO_EVENT sound_event;

float tone [TONES];

extern struct synthstruct synth;



static void sthread_play_msample(int msample_index, int play_tone, int vol);

enum
{
NOTE_0,
NOTE_1,
NOTE_2,
NOTE_3,
NOTE_4,
NOTE_5,
NOTE_6,
NOTE_7,
NOTES
};

//#define MBOX_W 11
//#define MBOX_H 11

// TRACKS is the number of simultaneous tracks:
#define TRACKS 4


struct mustatestruct
{
 int piece; // which piece is being played
 int piece_pos; // position in the piece command list
 int beat; // time position in bar
 int track_playing [TRACKS]; // which bar the track is playing
 int track_pos [TRACKS]; // data position in bar

// scales not currently used
 int track_scale [TRACKS]; // this is which scale the track is using
 int base_track_scale [TRACKS]; // this is which scale the track will return to after track is finished

 int track_note_offset [TRACKS];

 int note_list [TRACKS] [NOTES]; // this relates a note value to an index in the msample array.

 int track_length; // length of each track

};

#define PIECE_LENGTH 64

enum
{
PIECE_BASIC,
PIECES
};

struct piece_elementstruct
{
 int piece_command;
 int value1;
 int value2;
 int value3;
};

// A piece is a list of commands:
enum
{
PC_END,
PC_PLAY_TRACK, // values: track index, bar index
PC_MUTE, // values: track index
PC_WAIT, // values: bars to wait (not yet implemented)
PC_SCALE, // values: track index, new scale
PC_BASE_SCALE, // values: track index, new base scale
PC_BAR_LENGTH, // values: new bar length
PC_SET_NOTE, // values: track index, NOTE index, msample index
PC_SET_TRACK_NOTE_OFFSET, // values: track_index, offset
};

#define BAR_VALUES 4
#define BAR_LENGTH 16

//A bar is also a list of commands
enum
{
BC_END,
BC_PLAY, // plays sample value [0], tone value [1], time value [2], volume offset value [3]
};

struct barstruct
{
 int bar_command;
 int value [BAR_VALUES];
};

enum
{
BAR_EMPTY,
BAR_BASIC_BACK,
BAR_BASIC_UP,
BAR_BASIC_UP2,
BAR_BASIC_UP3,
BAR_BASIC_UP4,
BAR_BASIC_UP5,
BAR_BASIC_DOWN,

BARS
};

struct barstruct bar [BARS] [BAR_LENGTH] =
{
 {
  {BC_END},
 }, // BAR_EMPTY
 {
  {BC_PLAY, {NOTE_1, 1, 0}},
  {BC_END},
 }, // BAR_BASIC_BACK
 {
  {BC_PLAY, {NOTE_0, TONE_2C, 0, 3}},
  {BC_PLAY, {NOTE_0, TONE_1G, 2}},
  {BC_PLAY, {NOTE_0, TONE_1F, 3, 3}},
  {BC_PLAY, {NOTE_0, TONE_2C, 5}},
  {BC_PLAY, {NOTE_0, TONE_1E, 7, 3}},
  {BC_PLAY, {NOTE_0, TONE_2E, 9}},
  {BC_PLAY, {NOTE_0, TONE_1DS, 11, 3}},
  {BC_PLAY, {NOTE_0, TONE_1A, 13}},
  {BC_END},
 }, // BAR_BASIC_UP
 {
  {BC_PLAY, {NOTE_0, TONE_2C, 0, 3}},
  {BC_PLAY, {NOTE_0, TONE_1G, 2}},
  {BC_PLAY, {NOTE_0, TONE_1F, 3, 3}},
  {BC_PLAY, {NOTE_0, TONE_2C, 5}},
  {BC_PLAY, {NOTE_0, TONE_1E, 7, 3}},
  {BC_PLAY, {NOTE_0, TONE_2E, 9}},
  {BC_PLAY, {NOTE_0, TONE_2DS, 11, 3}},
  {BC_PLAY, {NOTE_0, TONE_2D, 13}},
  {BC_END},
 }, // BAR_BASIC_UP2
 {
  {BC_PLAY, {NOTE_0, TONE_2C, 0, 3}},
  {BC_PLAY, {NOTE_0, TONE_1G, 2}},
  {BC_PLAY, {NOTE_0, TONE_1DS, 3, 3}},
  {BC_PLAY, {NOTE_0, TONE_2C, 5}},
  {BC_PLAY, {NOTE_0, TONE_1E, 7, 3}},
  {BC_PLAY, {NOTE_0, TONE_2E, 9}},
  {BC_PLAY, {NOTE_0, TONE_1DS, 11, 3}},
  {BC_PLAY, {NOTE_0, TONE_1AS, 13}},
  {BC_END},
 }, // BAR_BASIC_UP3
 {
  {BC_PLAY, {NOTE_0, TONE_3C, 0}},
  {BC_PLAY, {NOTE_0, TONE_3DS, 2}},
  {BC_PLAY, {NOTE_0, TONE_3F, 3}},
  {BC_PLAY, {NOTE_0, TONE_3G, 5}},
  {BC_END},
 }, // BAR_BASIC_UP4
 {
  {BC_PLAY, {NOTE_0, TONE_3C, 0}},
  {BC_PLAY, {NOTE_0, TONE_2A, 2}},
  {BC_PLAY, {NOTE_0, TONE_2G, 3}},
  {BC_PLAY, {NOTE_0, TONE_3G, 5}},
  {BC_END},
 }, // BAR_BASIC_UP5
 {
  {BC_PLAY, {NOTE_1, TONE_4C, 0}},
  {BC_PLAY, {NOTE_1, TONE_3C, 2}},
  {BC_PLAY, {NOTE_1, TONE_3A, 3}},
  {BC_PLAY, {NOTE_1, TONE_3F, 5}},
  {BC_END},
 }, // BAR_BASIC_DOWN


};

struct piece_elementstruct piece [PIECES] [PIECE_LENGTH] =
{

// PIECE_BASIC
{
// {PC_PLAY_TRACK, 0, BAR_BASIC_BACK},
 {PC_BAR_LENGTH, 1},
 {PC_SET_NOTE, 0, NOTE_0, MSAMPLE_NOTE},
 {PC_SET_NOTE, 0, NOTE_1, MSAMPLE_NOTE},
 {PC_SET_NOTE, 1, NOTE_0, MSAMPLE_NOTE},
 {PC_SET_NOTE, 1, NOTE_1, MSAMPLE_NOTE},
 {PC_WAIT}, // always need to start with wait
 {PC_BAR_LENGTH, 15},
 {PC_SET_TRACK_NOTE_OFFSET, 0, 15},
 {PC_PLAY_TRACK, 0, BAR_BASIC_UP},
 {PC_WAIT},
 {PC_PLAY_TRACK, 0, BAR_BASIC_UP},
 {PC_WAIT},
 {PC_PLAY_TRACK, 0, BAR_BASIC_UP},
 {PC_WAIT},
 {PC_PLAY_TRACK, 0, BAR_BASIC_UP2},
 {PC_WAIT},
 {PC_PLAY_TRACK, 0, BAR_BASIC_UP},
 {PC_WAIT},
 {PC_PLAY_TRACK, 0, BAR_BASIC_UP3},
 {PC_WAIT},
 {PC_PLAY_TRACK, 0, BAR_BASIC_UP2},
 {PC_WAIT},
/* {PC_PLAY_TRACK, 0, BAR_BASIC_UP2},
 {PC_WAIT},
 {PC_PLAY_TRACK, 0, BAR_BASIC_UP},
 {PC_WAIT},
 {PC_PLAY_TRACK, 0, BAR_BASIC_UP3},
 {PC_WAIT},
 {PC_PLAY_TRACK, 0, BAR_BASIC_UP4},
 {PC_WAIT},
 {PC_PLAY_TRACK, 0, BAR_BASIC_UP},
 {PC_WAIT},
 {PC_PLAY_TRACK, 0, BAR_BASIC_UP2},
 {PC_WAIT},
 {PC_PLAY_TRACK, 0, BAR_BASIC_UP5},
 {PC_WAIT},
 {PC_PLAY_TRACK, 0, BAR_BASIC_UP},
 {PC_PLAY_TRACK, 1, BAR_BASIC_DOWN},
 {PC_WAIT},
 {PC_PLAY_TRACK, 0, BAR_BASIC_UP},
 {PC_PLAY_TRACK, 1, BAR_BASIC_DOWN},
 {PC_WAIT},*/
 {PC_END},
}, // end PIECE_BASIC

};

struct mustatestruct mustate;

enum
{
SCALE_BASIC,
SCALE_MAJOR,
SCALE_TYPES
};

#define SCALE_SIZE 11

int scale [SCALE_TYPES] [SCALE_SIZE] =
{
 { // SCALE_BASIC (pentatonic)
  TONE_2C,
  TONE_2D,
  TONE_2F,
  TONE_2G,
  TONE_2A,
  TONE_2C,
  TONE_2D,
  TONE_2F,
  TONE_2G,
  TONE_2A,
  TONE_3C
 },
  { // SCALE_MAJOR
  TONE_3C,
  TONE_3D,
  TONE_3E,
  TONE_3F,
  TONE_3G,
  TONE_3A,
  TONE_3B,
  TONE_4C,
  TONE_4D,
  TONE_4E,
  TONE_4F
 }
};




// This function is called whenever a new piece is started.
void sthread_init_mustate(int play_piece)
{
 int i, j;

 mustate.piece = play_piece;
 mustate.piece_pos = 0;
 mustate.beat = 0;
 mustate.track_length = BAR_LENGTH; // should be reset by piece
 for (i = 0; i < TRACKS; i ++)
 {
  mustate.track_playing [i] = BAR_EMPTY;
  mustate.track_playing [i] = 0;
  mustate.track_pos [i] = 0;
  mustate.track_scale [i] = SCALE_BASIC;
  mustate.base_track_scale [i] = SCALE_BASIC;
  mustate.track_note_offset [i] = 0;
  for (j = 0; j < NOTES; j ++)
		{
			mustate.note_list [i] [j] = MSAMPLE_NOTE;
		}
 }


}

// returns 1 if piece still playing, or 0 if it's finished
int sthread_play_current_piece(void)
{

// Not currently used because I haven't written any music for it yet. Will be returned to service at some point.
 // should be static

 int i;

// First we go through the tracks:
 for (i = 0; i < TRACKS; i ++)
 {
  while(TRUE)
  {

   switch(bar [mustate.track_playing [i]] [mustate.track_pos [i]].bar_command)
   {
    case BC_PLAY:
     if (mustate.beat >= bar [mustate.track_playing [i]] [mustate.track_pos [i]].value [2])
     {
      sthread_play_msample(mustate.note_list [i] [bar [mustate.track_playing [i]] [mustate.track_pos [i]].value [0]], // Note type
																							    bar [mustate.track_playing [i]] [mustate.track_pos [i]].value [1] + mustate.track_note_offset [i], // pitch
																							    bar [mustate.track_playing [i]] [mustate.track_pos [i]].value [3]);
//                        scale [mustate.track_scale [i]] [bar [mustate.track_playing [i]] [mustate.track_pos [i]].value [1]],
//                        1);
      break; // will increment mustate.track_pos [i] (after this loop) then continue the loop
     }
     goto finished_track; // not ready to play yet
    case BC_END:
     goto finished_track;
// other BC types might just break instead of exiting the loop.
   }
   mustate.track_pos [i]++;
  };
  finished_track:
   continue;
 }


 mustate.beat++;

// now we check the piece commands:
// int value1, value2;

 while(TRUE)
 {
// these continue until they reach PC_WAIT or PC_END
  switch(piece [mustate.piece] [mustate.piece_pos].piece_command)
  {
   case PC_WAIT:
    if (mustate.beat >= mustate.track_length)
    {
     mustate.beat = 0;
     mustate.piece_pos++;
     continue;
    }
    return 1;
   case PC_PLAY_TRACK: // values: track index, bar index
    mustate.track_playing [piece [mustate.piece] [mustate.piece_pos].value1] = piece [mustate.piece] [mustate.piece_pos].value2;
    mustate.track_pos [piece [mustate.piece] [mustate.piece_pos].value1] = 0;
    mustate.track_scale [piece [mustate.piece] [mustate.piece_pos].value1] = mustate.base_track_scale [piece [mustate.piece] [mustate.piece_pos].value1];
    mustate.piece_pos++;
    break;
   case PC_MUTE:  // values: track index
    mustate.track_playing [piece [mustate.piece] [mustate.piece_pos].value1] = BAR_EMPTY;
    mustate.piece_pos++;
    break;
   case PC_BAR_LENGTH: // values: bar length
    mustate.track_length = piece [mustate.piece] [mustate.piece_pos].value1;
    mustate.piece_pos++;
    break;
   case PC_SCALE: // values: track index, scale index
    mustate.track_scale [piece [mustate.piece] [mustate.piece_pos].value1] = piece [mustate.piece] [mustate.piece_pos].value2;
    mustate.piece_pos++;
    break;
   case PC_BASE_SCALE: // values: track index, scale index
    mustate.base_track_scale [piece [mustate.piece] [mustate.piece_pos].value1] = piece [mustate.piece] [mustate.piece_pos].value2;
    mustate.piece_pos++;
    break;
   case PC_SET_NOTE: // values: track index, NOTE index, msample index
				mustate.note_list [piece [mustate.piece] [mustate.piece_pos].value1] [piece [mustate.piece] [mustate.piece_pos].value2] = piece [mustate.piece] [mustate.piece_pos].value3;
    mustate.piece_pos++;
				break;
			case PC_SET_TRACK_NOTE_OFFSET:
    mustate.track_note_offset [piece [mustate.piece] [mustate.piece_pos].value1] = piece [mustate.piece] [mustate.piece_pos].value2;
    mustate.piece_pos++;
				break;
   case PC_END:
    return 0;
// remember mustate.piece_pos++; if needed!
  }
 };

  return 0; // should never reach here

}


static void sthread_play_msample(int msample_index, int play_tone, int vol)
{

 al_play_sample(msample [msample_index], 0.5 + ((float) vol * 0.1), 0, tone [play_tone], ALLEGRO_PLAYMODE_ONCE, NULL);

// sthread_add_mbox_echo(mb, note_type, play_tone);
// could pan to left/right depending on x position?

}

