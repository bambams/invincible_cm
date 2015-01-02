#include <allegro5/allegro.h>

#include <stdio.h>

#include "m_config.h"

#include "g_header.h"

#include "m_maths.h"
#include "g_misc.h"
#include "g_motion.h"
#include "m_globvars.h"
#include "m_input.h"
#include "e_slider.h"
#include "e_header.h"
#include "c_header.h"
#include "e_editor.h"
#include "i_console.h"


void initialise_control(void);

extern struct gamestruct game;
extern struct viewstruct view;

struct controlstruct control;

// this array indicates which allegro_key enum corresponds to each KEY_? enum. Used for allowing system/clob programs to access keyboard
int corresponding_allegro_key [KEYS] =
{

ALLEGRO_KEY_0, // KEY_0,
ALLEGRO_KEY_1, // KEY_1,
ALLEGRO_KEY_2, // KEY_2,
ALLEGRO_KEY_3, // KEY_3,
ALLEGRO_KEY_4, // KEY_4,
ALLEGRO_KEY_5, // KEY_5,
ALLEGRO_KEY_6, // KEY_6,
ALLEGRO_KEY_7, // KEY_7,
ALLEGRO_KEY_8, // KEY_8,
ALLEGRO_KEY_9, // KEY_9,

ALLEGRO_KEY_A, // KEY_A,
ALLEGRO_KEY_B, // KEY_B,
ALLEGRO_KEY_C, // KEY_C,
ALLEGRO_KEY_D, // KEY_D,
ALLEGRO_KEY_E, // KEY_E,
ALLEGRO_KEY_F, // KEY_F,
ALLEGRO_KEY_G, // KEY_G,
ALLEGRO_KEY_H, // KEY_H,
ALLEGRO_KEY_I, // KEY_I,
ALLEGRO_KEY_J, // KEY_J,
ALLEGRO_KEY_K, // KEY_K,
ALLEGRO_KEY_L, // KEY_L,
ALLEGRO_KEY_M, // KEY_M,
ALLEGRO_KEY_N, // KEY_N,
ALLEGRO_KEY_O, // KEY_O,
ALLEGRO_KEY_P, // KEY_P,
ALLEGRO_KEY_Q, // KEY_Q,
ALLEGRO_KEY_R, // KEY_R,
ALLEGRO_KEY_S, // KEY_S,
ALLEGRO_KEY_T, // KEY_T,
ALLEGRO_KEY_U, // KEY_U,
ALLEGRO_KEY_V, // KEY_V,
ALLEGRO_KEY_W, // KEY_W,
ALLEGRO_KEY_X, // KEY_X,
ALLEGRO_KEY_Y, // KEY_Y,
ALLEGRO_KEY_Z, // KEY_Z,

ALLEGRO_KEY_MINUS, // KEY_MINUS,
ALLEGRO_KEY_EQUALS, // KEY_EQUALS,
ALLEGRO_KEY_OPENBRACE, // KEY_SBRACKET_OPEN,
ALLEGRO_KEY_CLOSEBRACE, // KEY_SBRACKET_CLOSE,
ALLEGRO_KEY_BACKSLASH, // KEY_BACKSLASH,
ALLEGRO_KEY_SEMICOLON, // KEY_SEMICOLON,
ALLEGRO_KEY_QUOTE, // KEY_APOSTROPHE,
ALLEGRO_KEY_COMMA, // KEY_COMMA,
ALLEGRO_KEY_FULLSTOP, // KEY_PERIOD,
ALLEGRO_KEY_SLASH, // KEY_SLASH,

ALLEGRO_KEY_UP, // KEY_UP,
ALLEGRO_KEY_DOWN, // KEY_DOWN,
ALLEGRO_KEY_LEFT, // KEY_LEFT,
ALLEGRO_KEY_RIGHT, // KEY_RIGHT,

ALLEGRO_KEY_ENTER, // KEY_ENTER,
ALLEGRO_KEY_BACKSPACE, // KEY_BACKSPACE,
ALLEGRO_KEY_INSERT, // KEY_INSERT,
ALLEGRO_KEY_HOME, // KEY_HOME,
ALLEGRO_KEY_PGUP, // KEY_PGUP,
ALLEGRO_KEY_PGDN, // KEY_PGDN,
ALLEGRO_KEY_DELETE, // KEY_DELETE,
ALLEGRO_KEY_END, // KEY_END,



ALLEGRO_KEY_TAB, // KEY_TAB,
// KEY_ESCAPE is not available to user programs

ALLEGRO_KEY_PAD_0, // KEY_PAD_0,
ALLEGRO_KEY_PAD_1, // KEY_PAD_1,
ALLEGRO_KEY_PAD_2, // KEY_PAD_2,
ALLEGRO_KEY_PAD_3, // KEY_PAD_3,
ALLEGRO_KEY_PAD_4, // KEY_PAD_4,
ALLEGRO_KEY_PAD_5, // KEY_PAD_5,
ALLEGRO_KEY_PAD_6, // KEY_PAD_6,
ALLEGRO_KEY_PAD_7, // KEY_PAD_7,
ALLEGRO_KEY_PAD_8, // KEY_PAD_8,
ALLEGRO_KEY_PAD_9, // KEY_PAD_9,
ALLEGRO_KEY_PAD_MINUS, // KEY_PAD_MINUS,
ALLEGRO_KEY_PAD_PLUS, // KEY_PAD_PLUS,
ALLEGRO_KEY_PAD_ENTER, // KEY_PAD_ENTER,
ALLEGRO_KEY_PAD_DELETE, // KEY_PAD_DELETE,

ALLEGRO_KEY_LSHIFT, // KEY_LSHIFT,
ALLEGRO_KEY_RSHIFT, // KEY_RSHIFT,
ALLEGRO_KEY_LCTRL, // KEY_LCTRL,
ALLEGRO_KEY_RCTRL, // KEY_RCTRL,


//KEYS

};



// this is called at startup, and when input capture is set to the editor, and also when loading from disk
void initialise_control(void)
{
 control.mouse_status = MOUSE_STATUS_AVAILABLE; // TO DO: make sure this is correct (could be wrong if it's possible for game to start with editor open and mouse in editor window)
 control.mouse_x_world_pixels = 0;
 control.mouse_y_world_pixels = 0;
 control.mouse_x_screen_pixels = 0;
 control.mouse_y_screen_pixels = 0;
 control.mbutton_press [0] = BUTTON_NOT_PRESSED; // not sure about this - should we try to read the actual mouse status?
 control.mbutton_press [1] = BUTTON_NOT_PRESSED;

 int i;

 for (i = 0; i < KEYS; i ++)
 {
  control.key_press [i] = BUTTON_NOT_PRESSED;
 }
 control.any_key = -1;

// control.mouse_hold_x_pixels [0] = -1;
// control.mouse_hold_x_pixels [1] = -1;
/*
 int i;

 for (i = 0; i < MBB_BITS; i ++)
 {
  control.mb_bits [i] = 0;
 }*/


}

/*

Plan: how to handle input

Option 1:
- Have a client process.
- client process has the following methods (all of which are probably costless; no point in imposing cost on client process):
 - mouse: call to poll mouse.
  - return value is status: 0 for mouse not on world screen (e.g. is on another panel), 1 for success
  - first mbank entry is mode. Usual mode is query:
   - two mbank entries give x/y in world (not on screen), given in pixel units.
   - final mbank entry contains bitfield for buttons and maybe wheel
  - another mode allows settings, maybe? (this is to allow box drawing etc to be set)
   - not sure if needed
   - maybe could merge mouse and keyboard methods and use the status mbank entry to determined what's being checked.
  - another mode allows checking of whether a particular proc is under the cursor
   - one field will be team of proc under mouse cursor (not necess clicked on).
    - maybe that's the only information that'll be available about procs on other teams.
    - or maybe the client process will have access to all of this information? Probably.
 - keyboard: call to query keyboard
  - first mbank sets the key being queried
  - second mbank is the result: -1 just released; 0 not pressed; 1 just pressed; 2 held
  - third mbank is a bitfield for shift etc (although these can be queried individually) - maybe not needed.

- to support these input methods, there'll be a special input structure kept up to date each frame
 - contains mouse information
 - contains information for each key (although maybe not all keys)

- So how can the client process select particular procs?
 - each proc will be assigned a number on creation. These numbers are not re-used. They are visible only to the client process.
 - each proc will also have a special field for client interactions.

 - the client process will have a method for interacting with these:
  - first mbank entry is mode
   - some different basic mode types:
    - proc-based allows read access to information about the proc (location, hp etc)
    - client/team based allows r/w access to the special client interactions field for all procs on client's team
     - procs will also have a method giving r/w access to this field
    - client based but all teams: similar to client/team, but can be used for all teams.
    - bcode allows read access to proc's bcode
    - method allows read access to methods
    - mbank allows read access to mbank
  - second mbank entry is proc number
  - third is field number
  - fourth is field value - read or write depending on mode
  - return value is 1 if proc exists, 0 if not

 - the client process will have another method for creating interface elements
  - first mbank entry is mode. modes are:
   - mode: selection indicators
    - second is field number
    - third is result attached to field
     - none (default)
     - select (maybe various types of select)
   - mode: proc data display
    - when called, sets which proc is being inspected
    - there'll need to be a way of setting the proc under the cursor, to allow other team procs to be inspected.
    - probably make this proc the focus as well
    - call with -1 proc to cancel.
   - mode: proc focus
    - sets which proc is in focus (-1 for no focus) - may not be needed

 - a point-marking method:
  - has a certain number (32?) of points
  - each point has a type:
   - appearance
   - displayed on screen?
   - displayed on map?
  - other information:
   - active?
   - times out? (set to -1 if no timeout)

 - a viewpoint method:
  - can be used to find out information about the game window (x/y location, size etc)
  - maybe could be used to set focus on a particular proc? This would only work if other teams' procs can be selected



- keep in mind that there's no reason to withhold from the client any information visible to the player.


A few other things to do:
 - large-scale initialisation in asm: [20] will initialise 20 memory addresses (to zero)

*/


void run_input(void)
{

  if (game.phase == GAME_PHASE_PREGAME)
		{
   control.mouse_status = MOUSE_STATUS_OUTSIDE;
   control.any_key = -1;
			return;
		}

  int i;

// this code removes focus from a proc that has been destroyed
//  - possibly it can be removed? (TO DO: check whether everything focus_proc does can be delegated to client program)
  if (view.focus_proc != NULL
   && view.focus_proc->exists != 1) // will be -1 if proc has just been deallocated
  {
   view.focus_proc = NULL;
   reset_proc_box_height(NULL);
  }

  if (ex_control.mouse_on_display == 0)
  {
   control.mouse_status = MOUSE_STATUS_OUTSIDE;
   goto mouse_unavailable;
  }

// check for the mouse pointer being in the editor/template window:
  if (settings.edit_window != EDIT_WINDOW_CLOSED
   && ex_control.mouse_x_pixels >= settings.editor_x_split)
  {
//   initialise_control();
   control.mouse_status = MOUSE_STATUS_OUTSIDE;
   if (ex_control.mb_press [0] == BUTTON_JUST_PRESSED)
    settings.keyboard_capture = INPUT_EDITOR;
// TO DO: should probably not do this if mouse began being held while in the game window
// also, should accept keyboard input for the game screen at this point
   return;
  }

 control.mouse_status = MOUSE_STATUS_AVAILABLE;

// check for the mouse pointer being in the console window, a score box etc:
//  because of the return just above, check_mouse_on_consoles_etc is not called if the mouse is in the editor window
  check_mouse_on_consoles_etc(ex_control.mouse_x_pixels, ex_control.mouse_y_pixels, (ex_control.mb_press [0] == BUTTON_JUST_PRESSED));

  control.mouse_x_world_pixels = ex_control.mouse_x_pixels + al_fixtoi(view.camera_x - view.centre_x);
  control.mouse_y_world_pixels = ex_control.mouse_y_pixels + al_fixtoi(view.camera_y - view.centre_y);

  control.mouse_x_screen_pixels = ex_control.mouse_x_pixels;
  control.mouse_y_screen_pixels = ex_control.mouse_y_pixels;

mouse_unavailable:

  for (i = 0; i < 2; i ++)
  {
   switch(control.mbutton_press [i])
   {
    case BUTTON_NOT_PRESSED:
     if (ex_control.mb_press [i] != BUTTON_NOT_PRESSED)
      control.mbutton_press [i] = BUTTON_JUST_PRESSED;
     break;
    case BUTTON_JUST_RELEASED:
     if (ex_control.mb_press [i] <= BUTTON_NOT_PRESSED)
      control.mbutton_press [i] = BUTTON_NOT_PRESSED;
       else control.mbutton_press [i] = BUTTON_JUST_PRESSED;
     break;
    case BUTTON_JUST_PRESSED:
     if (ex_control.mb_press [i] <= BUTTON_NOT_PRESSED)
      control.mbutton_press [i] = BUTTON_JUST_RELEASED;
       else
        control.mbutton_press [i] = BUTTON_HELD;
     break;
    case BUTTON_HELD:
     if (ex_control.mb_press [i] <= BUTTON_NOT_PRESSED)
      control.mbutton_press [i] = BUTTON_JUST_RELEASED;
     break;
   }
  }


// not sure the following bounds checks are needed (they're probably handled in the client program method code for moving the camera) but it can't hurt
 if (view.camera_x < view.camera_x_min)
  view.camera_x = view.camera_x_min;
 if (view.camera_y < view.camera_y_min)
  view.camera_y = view.camera_y_min;
 if (view.camera_x > view.camera_x_max)
  view.camera_x = view.camera_x_max;
 if (view.camera_y > view.camera_y_max)
  view.camera_y = view.camera_y_max;

 control.any_key = -1;

// we need to specially calculate whether the keys have just been pressed etc, rather than relying on ex_control values, because the controlstruct is saved to file and ex_control may not be reliable when loading/saving
 for (i = KEYS - 1; i >= 0; i --) // Counts down so that any_key detects letters in preference to shift, control etc
 {
  switch(control.key_press[i])
  {
   case BUTTON_JUST_RELEASED:
    if (ex_control.key_press [corresponding_allegro_key [i]] == BUTTON_NOT_PRESSED)
     control.key_press [i] = BUTTON_NOT_PRESSED;
      else
						{
       control.key_press [i] = BUTTON_JUST_PRESSED;
       control.any_key = i;
						}
    break;
   case BUTTON_NOT_PRESSED:
    if (ex_control.key_press [corresponding_allegro_key [i]] != BUTTON_NOT_PRESSED)
				{
     control.key_press [i] = BUTTON_JUST_PRESSED; // if ex_control value is BUTTON_JUST_RELEASED, control value will be set to BUTTON_JUST_PRESSED and then (probably) to BUTTON_JUST_RELEASED the next tick
     control.any_key = i;
				}
    break;
   case BUTTON_JUST_PRESSED:
    if (ex_control.key_press [corresponding_allegro_key [i]] <= BUTTON_NOT_PRESSED) // <= BUTTON_NOT_PRESSED means BUTTON_NOT_PRESSED (0) or BUTTON_JUST_RELEASED (-1)
     control.key_press [i] = BUTTON_JUST_RELEASED;
      else
						{
       control.key_press [i] = BUTTON_HELD;
//       control.any_key = i;
						}
    break;
   case BUTTON_HELD:
    if (ex_control.key_press [corresponding_allegro_key [i]] <= BUTTON_NOT_PRESSED) // <= BUTTON_NOT_PRESSED means BUTTON_NOT_PRESSED (0) or BUTTON_JUST_RELEASED (-1)
     control.key_press [i] = BUTTON_JUST_RELEASED;
    break;
  }
 }


}


