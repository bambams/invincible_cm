#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>

#include <stdio.h>

#include "m_config.h"

#include "g_header.h"
#include "m_globvars.h"
#include "t_template.h"
#include "e_slider.h"
#include "e_header.h"
#include "c_header.h"
#include "e_editor.h"
#include "e_help.h"
#include "i_view.h"
#include "i_sysmenu.h"
#include "i_programs.h"
#include "i_header.h"
#include "i_display.h"
#include "i_buttons.h"
#include "g_misc.h"

#include "m_maths.h"

#include "m_input.h"

extern struct editorstruct editor;
extern struct viewstruct view;

struct key_typestruct key_type [ALLEGRO_KEY_MAX];

void check_mode_buttons_and_panel_drag(void);
void mode_button(int mode_pressed);
static void close_window_box(void);

ALLEGRO_DISPLAY* display;

ALLEGRO_EVENT_QUEUE* control_queue;
extern ALLEGRO_EVENT_QUEUE* event_queue;
extern struct fontstruct font [FONTS];


void init_ex_control(void)
{

// Set up the control event queue:
   control_queue = al_create_event_queue();
   if (!control_queue)
   {
      fprintf(stdout, "\nm_input.c: init_ex_control(): Error: failed to create control_queue.");
      safe_exit(-1);
   }

   ALLEGRO_EVENT_SOURCE* acquire_event_source = al_get_mouse_event_source();
   if (acquire_event_source == NULL)
   {
      fprintf(stdout, "\nm_input.c: init_ex_control(): Error: failed to get mouse event source.");
      safe_exit(-1);
   }
   al_register_event_source(control_queue, acquire_event_source);

   acquire_event_source = al_get_display_event_source(display);
   if (acquire_event_source == NULL)
   {
      fprintf(stdout, "\nm_input.c: init_ex_control(): Error: failed to get display event source.");
      safe_exit(-1);
   }
   al_register_event_source(control_queue, acquire_event_source);

 ex_control.mouse_x_pixels = 0;
 ex_control.mouse_y_pixels = 0;
 ex_control.mb_press [0] = BUTTON_NOT_PRESSED;
 ex_control.mb_press [1] = BUTTON_NOT_PRESSED;
 ex_control.key_being_pressed = -1;
 ex_control.mousewheel_change = 0;
 ex_control.using_slider = 0;
 ex_control.mouse_on_display = 1;
 ex_control.mouse_dragging_panel = 0;

 int i;

 for (i = 0; i < ALLEGRO_KEY_MAX; i ++)
 {
  ex_control.key_press [i] = BUTTON_NOT_PRESSED;
 }
 ex_control.keys_pressed = 0;

// need to know current mousewheel position to be able to initialise it:
  ALLEGRO_MOUSE_STATE mouse_state;

  al_get_mouse_state(&mouse_state);

  ex_control.mousewheel_pos = mouse_state.w;

}

// close_button_status indicates what to do if the user clicks on the native close window button or presses escape
// 0 means: open a close_window_box (usual action)
// 1 means: exit immediately (start screen only)
// 2 means: do nothing (probably means this is being called from inside close_window_box())
void get_ex_control(int close_button_status)
{

  int i;

  ALLEGRO_EVENT control_event;

  while (al_get_next_event(control_queue, &control_event))
  {
   if (control_event.type == ALLEGRO_EVENT_MOUSE_LEAVE_DISPLAY)
    ex_control.mouse_on_display = 0;
     else
     {
      if (control_event.type != ALLEGRO_EVENT_DISPLAY_CLOSE)
      {
// general mouse actions:
       if (control_event.mouse.display != NULL)
        ex_control.mouse_on_display = 1;
      }
       else
       {
        if (close_button_status == 0)
         close_window_box(); // program may exit from this.
        if (close_button_status == 1)
         safe_exit(0);
//        if (close_button_status == 2) - do nothing
       }
     }
  }

//   fprintf(stdout, "%i", ex_control.mouse_on_display);


  //if (ex_control.mouse_on_display)
  {

   ALLEGRO_MOUSE_STATE mouse_state;

   al_get_mouse_state(&mouse_state);

   ex_control.mouse_x_pixels = mouse_state.x;
   ex_control.mouse_y_pixels = mouse_state.y;

   for (i = 0; i < 2; i ++)
   {
    switch(ex_control.mb_press [i])
    {
     case BUTTON_NOT_PRESSED:
      if (mouse_state.buttons & (1 << i))
       ex_control.mb_press [i] = BUTTON_JUST_PRESSED;
      break;
     case BUTTON_JUST_PRESSED:
      if (mouse_state.buttons & (1 << i))
       ex_control.mb_press [i] = BUTTON_HELD;
        else
          ex_control.mb_press [i] = BUTTON_JUST_RELEASED;
      break;
     case BUTTON_JUST_RELEASED:
      if (mouse_state.buttons & (1 << i))
       ex_control.mb_press [i] = BUTTON_JUST_PRESSED;
        else
         ex_control.mb_press [i] = BUTTON_NOT_PRESSED;
      break;
     case BUTTON_HELD:
      if (!(mouse_state.buttons & (1 << i)))
       ex_control.mb_press [i] = BUTTON_JUST_RELEASED;
      break;
    }
   }

// if the left mouse button isn't being pressed, stop using any slider currently being used:
   if (ex_control.mb_press [0] <= 0)
    ex_control.using_slider = 0;

   ex_control.mousewheel_change = 0;

   if (mouse_state.z < ex_control.mousewheel_pos)
    ex_control.mousewheel_change = 1;

   if (mouse_state.z > ex_control.mousewheel_pos)
    ex_control.mousewheel_change = -1;

   ex_control.mousewheel_pos = mouse_state.z;

  }


  ALLEGRO_KEYBOARD_STATE key_state;

  al_get_keyboard_state(&key_state);
  ex_control.keys_pressed = 0;

  if (al_key_down(&key_state, ALLEGRO_KEY_ESCAPE))
  {
   if (close_button_status == 0)
    close_window_box(); // program may exit from this.
   if (close_button_status == 1)
    safe_exit(0);
  }

  for (i = 0; i < ALLEGRO_KEY_MAX; i ++)
  {
   if (al_key_down(&key_state, i))
   {
    ex_control.keys_pressed ++;
    switch(ex_control.key_press [i])
    {
     case BUTTON_JUST_RELEASED:
     case BUTTON_NOT_PRESSED:
      ex_control.key_press [i] = BUTTON_JUST_PRESSED; break;
     case BUTTON_JUST_PRESSED:
//     case BUTTON_HELD:
      ex_control.key_press [i] = BUTTON_HELD; break;
    }
   }
    else
    {
// not being pressed
     switch(ex_control.key_press [i])
     {
      case BUTTON_JUST_PRESSED:
      case BUTTON_HELD:
       ex_control.key_press [i] = BUTTON_JUST_RELEASED; break;
      case BUTTON_JUST_RELEASED:
//     case BUTTON_HELD:
       ex_control.key_press [i] = BUTTON_NOT_PRESSED; break;
     }
    }
  }

 check_mode_buttons_and_panel_drag();

}


// This function fills the key_type struct array with information about keys. Called at startup.
void init_key_type(void)
{
 int i;

 for (i = 0; i < ALLEGRO_KEY_MAX; i ++)
 {
  key_type [i].type = KEY_TYPE_OTHER; // default

// letters
  if (i >= ALLEGRO_KEY_A
   && i <= ALLEGRO_KEY_Z)
  {
    key_type [i].type = KEY_TYPE_LETTER;
    key_type [i].unshifted = 'a' + i - ALLEGRO_KEY_A;
    key_type [i].shifted = 'A' + i - ALLEGRO_KEY_A;
    continue;
  }

// numbers
  if (i >= ALLEGRO_KEY_0 && i <= ALLEGRO_KEY_9)
  {
    key_type [i].type = KEY_TYPE_NUMBER;
    key_type [i].unshifted = '0' + i - ALLEGRO_KEY_0;
    switch(i)
    {
     case ALLEGRO_KEY_1: key_type [i].shifted = '!'; break;
     case ALLEGRO_KEY_2: key_type [i].shifted = '@'; break;
     case ALLEGRO_KEY_3: key_type [i].shifted = '#'; break;
     case ALLEGRO_KEY_4: key_type [i].shifted = '$'; break;
     case ALLEGRO_KEY_5: key_type [i].shifted = '%'; break;
     case ALLEGRO_KEY_6: key_type [i].shifted = '^'; break;
     case ALLEGRO_KEY_7: key_type [i].shifted = '&'; break;
     case ALLEGRO_KEY_8: key_type [i].shifted = '*'; break;
     case ALLEGRO_KEY_9: key_type [i].shifted = '('; break;
     case ALLEGRO_KEY_0: key_type [i].shifted = ')'; break;
    }
    continue;
  }

// numpad
  if (i >= ALLEGRO_KEY_PAD_0 && i <= ALLEGRO_KEY_PAD_9)
  {
    key_type [i].type = KEY_TYPE_CURSOR;
    continue;
  }

// other things
  switch(i)
  {
   case ALLEGRO_KEY_MINUS:
    key_type [i].type = KEY_TYPE_SYMBOL;
    key_type [i].unshifted = '-';
    key_type [i].shifted = '_';
    break;
   case ALLEGRO_KEY_EQUALS:
    key_type [i].type = KEY_TYPE_SYMBOL;
    key_type [i].unshifted = '=';
    key_type [i].shifted = '+';
    break;
   case ALLEGRO_KEY_OPENBRACE:
    key_type [i].type = KEY_TYPE_SYMBOL;
    key_type [i].unshifted = '[';
    key_type [i].shifted = '{';
    break;
   case ALLEGRO_KEY_CLOSEBRACE:
    key_type [i].type = KEY_TYPE_SYMBOL;
    key_type [i].unshifted = ']';
    key_type [i].shifted = '}';
    break;
   case ALLEGRO_KEY_SEMICOLON:
    key_type [i].type = KEY_TYPE_SYMBOL;
    key_type [i].unshifted = ';';
    key_type [i].shifted = ':';
    break;
   case ALLEGRO_KEY_QUOTE:
    key_type [i].type = KEY_TYPE_SYMBOL;
    key_type [i].unshifted = '\'';
    key_type [i].shifted = '"';
    break;
   case ALLEGRO_KEY_COMMA:
    key_type [i].type = KEY_TYPE_SYMBOL;
    key_type [i].unshifted = ',';
    key_type [i].shifted = '<';
    break;
   case ALLEGRO_KEY_FULLSTOP:
    key_type [i].type = KEY_TYPE_SYMBOL;
    key_type [i].unshifted = '.';
    key_type [i].shifted = '>';
    break;
   case ALLEGRO_KEY_SLASH:
    key_type [i].type = KEY_TYPE_SYMBOL;
    key_type [i].unshifted = '/';
    key_type [i].shifted = '?';
    break;
   case ALLEGRO_KEY_SPACE:
    key_type [i].type = KEY_TYPE_SYMBOL;
    key_type [i].unshifted = ' ';
    key_type [i].shifted = ' ';
    break;
   case ALLEGRO_KEY_TILDE: // don't think my keyboard has this key
    key_type [i].type = KEY_TYPE_SYMBOL;
    key_type [i].unshifted = '~';
    key_type [i].shifted = '~';
    break;
   case ALLEGRO_KEY_BACKQUOTE:
    key_type [i].type = KEY_TYPE_SYMBOL;
    key_type [i].unshifted = '`';
    key_type [i].shifted = '~';
    break;
   case ALLEGRO_KEY_PAD_SLASH:
    key_type [i].type = KEY_TYPE_SYMBOL;
    key_type [i].unshifted = '/';
    key_type [i].shifted = '/';
    break;
   case ALLEGRO_KEY_PAD_ASTERISK:
    key_type [i].type = KEY_TYPE_SYMBOL;
    key_type [i].unshifted = '*';
    key_type [i].shifted = '*';
    break;
   case ALLEGRO_KEY_PAD_MINUS:
    key_type [i].type = KEY_TYPE_SYMBOL;
    key_type [i].unshifted = '-';
    key_type [i].shifted = '-';
    break;
   case ALLEGRO_KEY_PAD_PLUS:
    key_type [i].type = KEY_TYPE_SYMBOL;
    key_type [i].unshifted = '+';
    key_type [i].shifted = '+';
    break;
   case ALLEGRO_KEY_PAD_EQUALS:
    key_type [i].type = KEY_TYPE_SYMBOL;
    key_type [i].unshifted = '=';
    key_type [i].shifted = '=';
    break;
   case ALLEGRO_KEY_BACKSLASH:
   case ALLEGRO_KEY_BACKSLASH2: // what is this?
    key_type [i].type = KEY_TYPE_SYMBOL;
    key_type [i].unshifted = '\\';
    key_type [i].shifted = '|';
    break;
   case ALLEGRO_KEY_AT: // what is this? I don't think it's on my keyboard
    key_type [i].type = KEY_TYPE_SYMBOL;
    key_type [i].unshifted = '@';
    key_type [i].shifted = '@';
    break;
   case ALLEGRO_KEY_CIRCUMFLEX: // not sure about this one either
    key_type [i].type = KEY_TYPE_SYMBOL;
    key_type [i].unshifted = '^';
    key_type [i].shifted = '^';
    break;
   case ALLEGRO_KEY_COLON2: // or this one
    key_type [i].type = KEY_TYPE_SYMBOL;
    key_type [i].unshifted = ':';
    key_type [i].shifted = ':';
    break;
   case ALLEGRO_KEY_SEMICOLON2: // or this one
    key_type [i].type = KEY_TYPE_SYMBOL;
    key_type [i].unshifted = ';';
    key_type [i].shifted = ';';
    break;
// cursor etc keys
   case ALLEGRO_KEY_BACKSPACE:
   case ALLEGRO_KEY_DELETE:
   case ALLEGRO_KEY_TAB:
   case ALLEGRO_KEY_ENTER:
//   case ALLEGRO_KEY_INSERT: // not sure how to treat this one
   case ALLEGRO_KEY_HOME:
   case ALLEGRO_KEY_END:
   case ALLEGRO_KEY_PGUP:
   case ALLEGRO_KEY_PGDN:
   case ALLEGRO_KEY_LEFT:
   case ALLEGRO_KEY_RIGHT:
   case ALLEGRO_KEY_UP:
   case ALLEGRO_KEY_DOWN:
   case ALLEGRO_KEY_PAD_DELETE:
   case ALLEGRO_KEY_PAD_ENTER:
    key_type[i].type = KEY_TYPE_CURSOR;
    break;
   case ALLEGRO_KEY_LSHIFT:
   case ALLEGRO_KEY_RSHIFT:
   case ALLEGRO_KEY_LCTRL:
   case ALLEGRO_KEY_RCTRL:
   case ALLEGRO_KEY_ALT:
   case ALLEGRO_KEY_ALTGR:
    key_type[i].type = KEY_TYPE_MOD;
    break;

// everything else is KEY_TYPE_OTHER (already set above)

  }


 } // end for loop

}


void check_mode_buttons_and_panel_drag(void)
{

//     ex_control.mode_button_pressed = 0;

 int i;

 for (i = 0; i < MODE_BUTTONS; i ++)
 {
    settings.mode_button_highlight [i] = 0;
 }


  if (ex_control.mouse_x_pixels >= settings.mode_button_x [0]
   && ex_control.mouse_x_pixels <= settings.mode_button_x [MODE_BUTTONS - 1] + MODE_BUTTON_SIZE
   && ex_control.mouse_y_pixels >= MODE_BUTTON_Y
   && ex_control.mouse_y_pixels <= MODE_BUTTON_Y + MODE_BUTTON_SIZE)
  {
   for (i = 0; i < MODE_BUTTONS; i ++)
   {
    if (settings.mode_button_available [i] == 1
     && ex_control.mouse_x_pixels >= settings.mode_button_x [i]
     && ex_control.mouse_x_pixels <= settings.mode_button_x [i] + MODE_BUTTON_SIZE)
    {
     settings.mode_button_highlight [i] = 1;
     if (ex_control.mb_press [0] == BUTTON_JUST_PRESSED)
      mode_button(i);
     if (ex_control.mb_press [1] == BUTTON_JUST_PRESSED)
					{
						switch(i)
						{
						 case MODE_BUTTON_EDITOR:
						 	print_help(HELP_MODE_BUTTON_EDITOR); break;
						 case MODE_BUTTON_TEMPLATES:
						 	print_help(HELP_MODE_BUTTON_TEMPLATES); break;
						 case MODE_BUTTON_PROGRAMS:
						 	print_help(HELP_MODE_BUTTON_PROGRAMS); break;
						 case MODE_BUTTON_SYSMENU:
						 	print_help(HELP_MODE_BUTTON_SYSMENU); break;
						 case MODE_BUTTON_CLOSE:
						 	print_help(HELP_MODE_BUTTON_CLOSE); break;
						}
					}
     return; // can return because we know the mouse isn't over any other mode button, or over the edge of the panel (for the code below)
    }
   }
  }

/*			fprintf(stdout, "\nZ(%i,%i,%i,%i)", ex_control.mb_press [0] == BUTTON_JUST_PRESSED,
												settings.edit_window != EDIT_WINDOW_CLOSED,
												ex_control.mouse_x_pixels,settings.editor_x_split);*/

		ex_control.panel_drag_ready = 0;

  if (settings.edit_window != EDIT_WINDOW_CLOSED
			&& ex_control.mouse_x_pixels > settings.editor_x_split
			&& ex_control.mouse_x_pixels < settings.editor_x_split + 12)
		{
			ex_control.panel_drag_ready = 1;
			if (ex_control.mb_press [0] == BUTTON_JUST_PRESSED)
			 ex_control.mouse_dragging_panel = 1;
		}

		if (ex_control.mouse_dragging_panel != 0)
		{
			int new_window_columns = (settings.option [OPTION_WINDOW_W] - ex_control.mouse_x_pixels - 30) / editor.text_width + 1;
//			int new_main_window_size = settings.option [OPTION_WINDOW_W] - (settings.edit_window_columns * editor.text_width) - 30;
			if (ex_control.mb_press [0] >= BUTTON_JUST_PRESSED)
			{
	   if (new_window_columns < 68)
					new_window_columns = 68; // 70 is just enough to give space for the template menu
	   if (new_window_columns > 160)
					new_window_columns = 160;
				if (settings.option [OPTION_WINDOW_W] - (new_window_columns * editor.text_width)	< 540)
					new_window_columns = (settings.option [OPTION_WINDOW_W] - 540) / editor.text_width;
				if (settings.edit_window_columns != new_window_columns)
					view.just_resized = 1;
	   settings.edit_window_columns = new_window_columns;
    settings.editor_x_split = settings.option [OPTION_WINDOW_W] - (settings.edit_window_columns * editor.text_width) - 30;
    editor.panel_x = settings.editor_x_split;
    editor.panel_w = settings.option [OPTION_WINDOW_W] - settings.editor_x_split;
    resize_display_window(settings.editor_x_split, settings.option [OPTION_WINDOW_H]);
    ex_control.mouse_on_display = 0;
//    change_edit_panel_width();
			}
			 else
				{
					ex_control.mouse_dragging_panel = 0;
				}
		}

}

// call this when one of the mode buttons is pressed
// see also close_any_edit_window() in g_game.c
void mode_button(int mode_pressed)
{

 if (settings.mode_button_available == 0)
  return;

 if (mode_pressed == settings.edit_window)
  return; // nothing to do

 switch(settings.edit_window)
 {
  case EDIT_WINDOW_CLOSED:
// since the case of (mode_pressed == settings.edit_window) has already been excluded, we know that the display window needs to be resized when this button is pressed
   resize_display_window(settings.editor_x_split, settings.option [OPTION_WINDOW_H]);
   break;
  case EDIT_WINDOW_EDITOR:
   close_editor();
   break;
  case EDIT_WINDOW_TEMPLATES:
   close_templates();
   break;
  case EDIT_WINDOW_SYSMENU:
   close_sysmenu();
   break;
  case EDIT_WINDOW_PROGRAMS:
   close_programs_window();
   break;
 }

 switch(mode_pressed)
 {
  case MODE_BUTTON_CLOSE:
   resize_display_window(settings.option [OPTION_WINDOW_W], settings.option [OPTION_WINDOW_H]);
   break;
  case MODE_BUTTON_EDITOR:
   settings.edit_window = EDIT_WINDOW_EDITOR;
   open_editor();
   break;
  case MODE_BUTTON_TEMPLATES:
   settings.edit_window = EDIT_WINDOW_TEMPLATES;
   open_templates();
   break;
  case MODE_BUTTON_SYSMENU:
   settings.edit_window = EDIT_WINDOW_SYSMENU;
   open_sysmenu();
   break;
  case MODE_BUTTON_PROGRAMS:
   settings.edit_window = EDIT_WINDOW_PROGRAMS;
   open_programs_window();
   break;

 }

}



// The following code deals with text entry into the menu interface (e.g. player names) and also editor text boxes (currently just the text search box)
// Currently, any open text box will accept keyboard input (so if there's more than one, all will accept it). Could fix but not a serious issue.

struct text_input_box_struct
{
 char* str;
 int cursor_pos;
 int max_length;
};

struct text_input_box_struct text_input_box [TEXT_BOXES];

void add_char_to_text_box(int b, char input_char);
int text_box_cursor_etc(int b, int key_pressed);


// Must call this before accepting input in input box
// b is index in text_input_box array
void start_text_input_box(int b, char* input_str, int max_length)
{

 text_input_box[b].str = input_str;
 text_input_box[b].str [0] = '\0';
 text_input_box[b].cursor_pos = 0;
 text_input_box[b].max_length = max_length;

}


// assumes start_text_input_box has been called
// returns 1 if enter pressed, 0 otherwise
int accept_text_box_input(int b)
{

  if (ex_control.key_press [ALLEGRO_KEY_LCTRL]
   || ex_control.key_press [ALLEGRO_KEY_RCTRL])
    return 0; // ignore this because otherwise it will pick up an f when ctrl-f pressed

 int i;

// so check whether another key is being pressed instead:
   for (i = 0; i < ALLEGRO_KEY_MAX; i ++)
   {
    if (ex_control.key_press [i] != BUTTON_JUST_PRESSED
     || key_type [i].type == KEY_TYPE_OTHER
     || key_type [i].type == KEY_TYPE_MOD)
      continue;
    editor.cursor_flash = CURSOR_FLASH_MAX;
    if (key_type [i].type == KEY_TYPE_CURSOR)
    {
     return text_box_cursor_etc(b, i);
    }
// must be a character that can be added to the string
     if (ex_control.key_press [ALLEGRO_KEY_LSHIFT]
      || ex_control.key_press [ALLEGRO_KEY_RSHIFT])
       add_char_to_text_box(b, key_type [i].shifted);
        else
         add_char_to_text_box(b, key_type [i].unshifted);
   }


 return 0;

}


void add_char_to_text_box(int b, char input_char)
{

 int str_length = strlen(text_input_box[b].str);

 if (str_length == text_input_box[b].max_length - 1)
  return;

// For now, just add characters at the end. Deal with allowing cursor movement within the string later.

// simple case: char is to be added at end
// if (text_input_box.cursor_pos == str_length)
 {
  text_input_box[b].str [text_input_box[b].cursor_pos] = input_char;
  text_input_box[b].cursor_pos ++;
  text_input_box[b].str [text_input_box[b].cursor_pos] = '\0';
  return;
 }

/*
// more complicated case: push text to right
 int i = text_input_box.max_length - 1;
 while(i > str_length)
 {
  text_input_box.str [i]
 };*/

}

// returns 1 if enter pressed, 0 otherwise
int text_box_cursor_etc(int b, int key_pressed)
{

 switch(key_pressed)
 {
  case ALLEGRO_KEY_BACKSPACE:
   if (text_input_box[b].cursor_pos == 0)
    return 0;
   text_input_box[b].cursor_pos --;
   text_input_box[b].str [text_input_box[b].cursor_pos] = '\0';
   break;
  case ALLEGRO_KEY_ENTER:
  case ALLEGRO_KEY_PAD_ENTER:
   return 1;
 }

 return 0;

}

#define CLOSEWINDOW_W 140
#define CLOSEWINDOW_H 60

// Displays a close window box and blocks everything else until the user either exits the game or continues
static void close_window_box(void)
{

 int x = settings.option [OPTION_WINDOW_W] / 2;
 int y = settings.option [OPTION_WINDOW_H] / 2;
 int mouse_x, mouse_y, just_pressed;
 ALLEGRO_EVENT ev;

 int xa, ya, xb, yb;

 al_set_target_bitmap(al_get_backbuffer(display));
 al_set_clipping_rectangle(0, 0, settings.option [OPTION_WINDOW_W], settings.option [OPTION_WINDOW_W]);
 reset_i_buttons();

 while(TRUE)
 {

//  al_clear_to_color(colours.base [COL_BLUE] [SHADE_LOW]);

  get_ex_control(2); // 2 means that clicking the native close window button will not do anything, as the exit game box is already being displayed
  mouse_x = ex_control.mouse_x_pixels;
  mouse_y = ex_control.mouse_y_pixels;
  just_pressed = (ex_control.mb_press [0] == BUTTON_JUST_PRESSED);

  add_menu_button(x - CLOSEWINDOW_W, y - CLOSEWINDOW_H, x + CLOSEWINDOW_W, y + CLOSEWINDOW_H, colours.base_trans [COL_RED] [SHADE_LOW] [TRANS_FAINT], MBUTTON_TYPE_MENU);

//  al_draw_filled_rectangle(x - CLOSEWINDOW_W, y - CLOSEWINDOW_H, x + CLOSEWINDOW_W, y + CLOSEWINDOW_H, colours.base [COL_RED] [SHADE_LOW]);
//  al_draw_rectangle(x - CLOSEWINDOW_W, y - CLOSEWINDOW_H, x + CLOSEWINDOW_W, y + CLOSEWINDOW_H, colours.base [COL_RED] [SHADE_MED], 1);

  xa = x - CLOSEWINDOW_W + 20;
  ya = y + 10;
  xb = x - CLOSEWINDOW_W + 90;
  yb = y + 30;

  if (mouse_x > xa
   && mouse_x < xb
   && mouse_y > ya
   && mouse_y < yb)
  {

//   al_draw_filled_rectangle(xa, ya, xb, yb, colours.base [COL_RED] [SHADE_HIGH]);
   add_menu_button(xa, ya, xb, yb, colours.base_trans [COL_RED] [SHADE_HIGH] [TRANS_FAINT], MBUTTON_TYPE_SMALL);
   add_menu_string(xa + 35, ya + 6, &colours.base [COL_GREY] [SHADE_MAX], ALLEGRO_ALIGN_CENTRE, FONT_SQUARE, "Exit");
//   al_draw_textf(font[FONT_SQUARE].fnt, colours.base [COL_GREY] [SHADE_MAX], xa + 35, ya + 6, ALLEGRO_ALIGN_CENTRE, "Exit");
   if (just_pressed)
    safe_exit(0);
  }
   else
   {
    add_menu_button(xa, ya, xb, yb, colours.base_trans [COL_RED] [SHADE_MED] [TRANS_FAINT], MBUTTON_TYPE_SMALL);
//    al_draw_filled_rectangle(xa, ya, xb, yb, colours.base [COL_RED] [SHADE_MED]);
    add_menu_string(xa + 35, ya + 6, &colours.base [COL_GREY] [SHADE_MAX], ALLEGRO_ALIGN_CENTRE, FONT_SQUARE, "Exit");
//    al_draw_textf(font[FONT_SQUARE].fnt, colours.base [COL_GREY] [SHADE_MAX], xa + 35, ya + 6, ALLEGRO_ALIGN_CENTRE, "Exit");
   }

  xa = x + CLOSEWINDOW_W - 90;
  ya = y + 10;
  xb = x + CLOSEWINDOW_W - 20;
  yb = y + 30;

  if (mouse_x > xa
   && mouse_x < xb
   && mouse_y > ya
   && mouse_y < yb)
  {
//   al_draw_filled_rectangle(xa, ya, xb, yb, colours.base [COL_RED] [SHADE_HIGH]);
   add_menu_button(xa, ya, xb, yb, colours.base_trans [COL_RED] [SHADE_HIGH] [TRANS_FAINT], MBUTTON_TYPE_SMALL);
   add_menu_string(xa + 35, ya + 6, &colours.base [COL_GREY] [SHADE_MAX], ALLEGRO_ALIGN_CENTRE, FONT_SQUARE, "Cancel");
//   al_draw_textf(font[FONT_SQUARE].fnt, colours.base [COL_GREY] [SHADE_MAX], xa + 35, ya + 6, ALLEGRO_ALIGN_CENTRE, "Cancel");
   if (just_pressed)
    break;
  }
   else
   {
//    al_draw_filled_rectangle(xa, ya, xb, yb, colours.base [COL_RED] [SHADE_MED]);
    add_menu_button(xa, ya, xb, yb, colours.base_trans [COL_RED] [SHADE_MED] [TRANS_FAINT], MBUTTON_TYPE_SMALL);
    add_menu_string(xa + 35, ya + 6, &colours.base [COL_GREY] [SHADE_MAX], ALLEGRO_ALIGN_CENTRE, FONT_SQUARE, "Cancel");
//    al_draw_textf(font[FONT_SQUARE].fnt, colours.base [COL_GREY] [SHADE_MAX], xa + 35, ya + 6, ALLEGRO_ALIGN_CENTRE, "Cancel");
   }

  draw_menu_buttons();
  al_draw_textf(font[FONT_SQUARE].fnt, colours.base [COL_GREY] [SHADE_MAX], x, y - 40, ALLEGRO_ALIGN_CENTRE, "Exit Invincible Countermeasure?");
  al_draw_textf(font[FONT_SQUARE].fnt, colours.base [COL_GREY] [SHADE_MAX], x, y - 25, ALLEGRO_ALIGN_CENTRE, "(any unsaved changes will be lost)");

  if (settings.option [OPTION_SPECIAL_CURSOR])
   draw_mouse_cursor();
  al_flip_display();
  al_wait_for_event(event_queue, &ev);
//  al_set_target_bitmap(al_get_backbuffer(display));

 };

 flush_game_event_queues();

}



