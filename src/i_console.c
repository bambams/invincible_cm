#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>

#include <stdio.h>
#include <string.h>

#include "m_config.h"

#include "g_header.h"
#include "g_method.h"
#include "e_slider.h"
#include "m_globvars.h"
#include "m_input.h"
#include "m_maths.h"
#include "g_shape.h"
#include "i_console.h"
#include "i_header.h"

#include "g_misc.h"

extern struct controlstruct control;

extern const struct mtypestruct mtype [MTYPES]; // in g_method.c
extern const struct method_costsstruct method_cost;

extern struct fontstruct font [FONTS];
extern struct viewstruct view;

//ALLEGRO_COLOR print_col [PRINT_COLS];
//ALLEGRO_COLOR print_fade_col [PRINT_COLS] [CONSOLE_LINE_FADE];
//ALLEGRO_COLOR console_background_colour;



/*

This file contains functions for running the in-game console, which displays all text output by the "print" command.
Actual display of the console is in i_display.c
The console window is a small box up the top left of the screen, which when clicked on expands to full size.
Its height can be adjusted by dragging the bottom border.
Its width is probably fixed.

The only control client programs have over it is turning it on or off. Otherwise, all clicks on it are intercepted.
 - need a way to have focus sent to a proc if a line it sent to the console is clicked on.
 - probably do this with a special controlstruct field available to the client program.

*/

struct consolestruct console [CONSOLES];


struct proc_boxstruct proc_box;



void write_to_console(const char *str, int target_console, int source_program_type, int source_index, int source_player, int text_colour);

void start_console_line(int c, int source_program_type, int source_index, int source_player, int text_colour);
void finish_console_line(int c);

void console_tick_rollover(void);
void fix_console_slider(int c);
void clear_console(int c);

void set_console_open(int c, int new_status);
void set_console_lines(int c, int new_h);

void init_score_boxes(void);
int open_score_box(int p);
int min_max_score_box(int p, int open_status);
int close_score_box(int p);
int score_box_position(int p, int x, int y);
int score_box_detail(int p, int detail);
void set_score_box_size(int b);
void check_mouse_on_proc_box(int x, int y, int just_pressed);
void draw_console_box(float xa, float ya, float xb, float yb, ALLEGRO_COLOR col, float main_bend_size_x, float main_bend_size_y, float small_bend_size_x, float small_bend_size_y, int all_corners);

// This function is called in initialisation just before the main game loop starts.
// It's also called during game loading (to zero out all of the console data)
void init_consoles(void)
{

 int i;

 for (i = 0; i < CONSOLES; i ++)
 {


  console[i].cpos = 0;
//  console[i].w_letters = CLINE_LENGTH;
  //console[i].w_pixels = (console[i].w_letters * LETTER_WIDTH) + 25 + 10;
// details of console height are set in set_console_open(), as these may change
  console[i].button_highlight = -1;
  console[i].printed_this_tick = 0;

//  console[i].button_x [1] = console[i].w_pixels - 4 - CONSOLE_BUTTON_SEPARATION * 2;
//  console[i].button_x [2] = console[i].w_pixels - 4 - CONSOLE_BUTTON_SEPARATION;

  console[i].window_pos = CLINES;

  console[i].open = CONSOLE_CLOSED;
  console[i].x = 10;
  console[i].y = 10;
  console[i].style = CONSOLE_STYLE_BASIC;
  console[i].font_index = FONT_SQUARE;
  console[i].colour_index = COL_BLUE;
  sprintf(console[i].title, "Console %i", i);
  set_console_size_etc(i, CONSOLE_LETTERS_MAX, 10, console[i].font_index, 1); // This also clears the console

//  clear_console(i); // must come after set_console_size(), which sets up the sliders

 }

// fprintf(stdout, "\nInit console scrollbar: clines %i min %i h_lines %i", CLINES, CLINES - console[i].h_lines, console[i].h_lines);
//void init_slider(struct sliderstruct* sl, int* value_pointer, int dir, int value_min, int value_max, int total_length, int button_increment, int track_increment, int slider_represents_size, int x, int y, int thickness)

 place_proc_box(640, 50, NULL);
 proc_box.button_highlight = 0;
 proc_box.maximised = 1;

}

// call this at start of game
// It's different from init_consoles() because it opens console[0], so it shouldn't be called when loading a game from disk
void setup_consoles(void)
{

 return; // let's just leave this up to the user programs
// set_console_open(0, CONSOLE_OPEN_MAX);

}


void run_consoles(void)
{

 int i;

 for (i = 0; i < CONSOLES; i ++)
 {
  if (console[i].open != CONSOLE_CLOSED
   && (console[i].style == CONSOLE_STYLE_BASIC
    || console[i].style == CONSOLE_STYLE_BASIC_UP))
   run_slider(&console[i].scrollbar_v, 0, 0);
 }

 console_tick_rollover();

}

/*

How will program_write_to_console work out whether to start a new line?
- I think the current "source" system will probably work okay, with minor changes:
 - clients will need a way to have a source entry
  - maybe source_type and source_index
 - at the end of each tick, each console that was written to and that is not at the start of its current line will start a new line
 - sounds good.

*/



// str should be null terminated, although no more than console[i].w_letters will be used anyway
void program_write_to_console(const char* str, int source_program_type, int source_index, int source_player)
{

 if (w.current_output_console < 0
  || w.current_output_console >= CONSOLES)
  return;

 write_to_console(str, w.current_output_console, source_program_type, source_index, source_player, w.print_colour);

}

// like program_write_to_console but uses error console settings and text is always red. TO DO: should there be a special highlight for errors to differentiate them from ordinary red text?
void write_error_to_console(const char* str, int source_program_type, int source_index, int source_player)
{
 int target_console = -1;

 switch(source_program_type)
 {
  case PROGRAM_TYPE_PROCESS:
   target_console = w.player[source_player].error_console; // note that source_index may be -1
   break;
  case PROGRAM_TYPE_DELEGATE:
  case PROGRAM_TYPE_OPERATOR:
   target_console = w.player[source_player].error_console;
   break;
  case PROGRAM_TYPE_SYSTEM:
   target_console = w.system_error_console;
   break;
  case PROGRAM_TYPE_OBSERVER:
   target_console = w.observer_error_console;
   break;
  default:
#ifdef SANITY_CHECK
   fprintf(stdout, "\nError: i_console.c: write_error_to_console(): source_program_type is %i (source_player is %i).", source_program_type, source_player);
   error_call();
#endif
   break;
 }


 if (target_console < 0
  || target_console >= CONSOLES)
   return; // e.g. if error messages from a source are muted

 write_to_console(str, target_console, source_program_type, source_index, source_player, PRINT_COL_DRED);

}


// called by program_write_to_console and write_error_to_console
void write_to_console(const char *str, int target_console, int source_program_type, int source_index, int source_player, int text_colour)
{

 int c = target_console;
 int ignore_line_break = 0; // if a new line is being started because the message source has changed, ignore the first char of the message if it's a line break

 if (c < 0
  || c >= CONSOLES)
   return;

// if the new text comes from a different source to the current line, make a new line
 if (console[c].cline[console[c].cpos].source_program_type == -1)  // if source_program_type is -1, this is a fresh new line that anyone can write to without causing another newline
 {
    start_console_line(c, source_program_type, source_index, source_player, text_colour);
    ignore_line_break = 1;
//  console[c].cline[console[c].cpos].source_program_type = source_program_type;
//  console[c].cline[console[c].cpos].source_index = source_index;
 }
  else
  {
     if (console[c].cline[console[c].cpos].source_program_type != source_program_type // different program type must mean different source
     || ((console[c].cline[console[c].cpos].source_program_type == PROGRAM_TYPE_DELEGATE // if the source is a delegate or process, need to check index as well
      || console[c].cline[console[c].cpos].source_program_type == PROGRAM_TYPE_PROCESS)
     && console[c].cline[console[c].cpos].source_index != source_index))
   {
    finish_console_line(c);
    start_console_line(c, source_program_type, source_index, source_player, text_colour);
    ignore_line_break = 1;
   }
  }

 console[c].printed_this_tick = 1;

 int current_length = strlen(console[c].cline[console[c].cpos].text);

 int space_left = console[c].w_letters - current_length;

 if (space_left <= 1) // leave 1 for null character at end
 {
  finish_console_line(c);
  start_console_line(c, source_program_type, source_index, source_player, text_colour);
//  return; this is right, is it? shouldn't return
 }

 int i = current_length;
 int j = 0;
 int counter = 160; // maximum number of characters printed at once

 if (ignore_line_break
  && (str[j] == '\n'
   || str[j] == '\r'))
    j++;

 while(str [j] != '\0'
    && counter > 0)
 {
  if (str[j] == '\n' || str[j] == '\r' || i >= console[c].w_letters - 1)
  {
   console[c].cline[console[c].cpos].text [i] = '\0';
   finish_console_line(c);
   start_console_line(c, source_program_type, source_index, source_player, text_colour);
   i = 0;
   if (str[j] == '\n' || str[j] == '\r')
    j ++;
   continue;
//   space_left = console[i].w_letters;
  }
  console[c].cline[console[c].cpos].text [i] = str [j];
  j ++;
  i ++;
  counter --;
 };

 console[c].cline[console[c].cpos].text [i] = '\0';
 console[c].cline[console[c].cpos].text [console[c].w_letters - 1] = '\0';
// fprintf(stdout, "\nPRINT: %s (%s) i %i j %i", console[i].cline[console[i].cpos].text, str, i, j);

}

// c must be a valid console
// source_program_type may be -1, which means that any source can write to the new line without starting another new line
void start_console_line(int c, int source_program_type, int source_index, int source_player, int text_colour)
{

 console[c].cline[console[c].cpos].text [0] = '\0';

 console[c].cline[console[c].cpos].used = 1;
 console[c].cline[console[c].cpos].source_program_type = source_program_type;
 console[c].cline[console[c].cpos].source_index = source_index;
 console[c].cline[console[c].cpos].colour = text_colour;

 if (source_program_type == PROGRAM_TYPE_PROCESS
  && source_index >= 0 // can be -1 if printing an error message generated during creation of proc by system program (because in such a case the proc itself doesn't exist, and neither does a parent proc)
  && source_index < w.max_procs) // could potentially exceed this if e.g. loading a file with an error in it
 {
  console[c].cline[console[c].cpos].action_type = CONSOLE_ACTION_PROC;
  console[c].cline[console[c].cpos].action_val1 = w.proc[source_index].player_index;
  console[c].cline[console[c].cpos].action_val2 = source_index;
  console[c].cline[console[c].cpos].action_val3 = w.print_proc_action_value3;
 }
  else
  {
   console[c].cline[console[c].cpos].action_type = CONSOLE_ACTION_NONE;
  }
// in either case, the action values can be changed later by a call to the appropriate method


}


// c must be a valid console
void finish_console_line(int c)
{
//fprintf(stdout, "\nFinish line %i action_type %i", console[c].cpos, console[c].cline[console[c].cpos].action_type);
 console[c].cline[console[c].cpos].time_stamp = w.total_time + CONSOLE_LINE_FADE;// * 2;

 console[c].cpos ++;
 if (console[c].cpos >= CLINES)
  console[c].cpos = 0;

}


// adds a new line to any console that:
//  - is open, and
//  - has been printed to this tick, and
//  - has text in its current line
// call it once each game tick
void console_tick_rollover(void)
{
 int i;

 for (i = 0; i < CONSOLES; i ++)
 {
  if (console[i].printed_this_tick == 0)
   continue;

  console[i].printed_this_tick = 0;

  if (console[i].open == CONSOLE_CLOSED)
   continue;

  if (console[i].cline[console[i].cpos].text [0] == '\0')
   continue;

  finish_console_line(i);
  start_console_line(i, -1, 0, 0, PRINT_COL_LGREY); // using -1 as program_type means that the new line won't have an assigned source and any source can write to it

 }

}


void display_consoles(void)
{

 int x1;
 int y1;
 int x2;
 int y2;
 int x, y;
 int has_header;
 int header_position;
 int header_h;
 int has_scrollbar;
 int text_alignment;
 int has_background;
 int highlight_recent_lines;
 int text_x_offset;
// remember: these values are reset each time through the loop,
//  so we can't initialise them to a default setting.

 int lines_printed;

 int i;

 for (i = 0; i < CONSOLES; i ++)
// if order of display changed, see also the loop in check_mouse_on_console below
 {
  if (console[i].open == CONSOLE_CLOSED)
   continue;

  x1 = console[i].x;
  y1 = console[i].y;
  x2 = x1 + console[i].w_pixels;
  y2 = y1 + console[i].h_pixels;

  if (console[i].style == CONSOLE_STYLE_BASIC_UP
   || console[i].style == CONSOLE_STYLE_BOX_UP)
  {
   y1 = console[i].y - console[i].h_pixels;
   y2 = console[i].y;
  }

  if (x1 > view.window_x
   || y1 > view.window_y
   || x2 < 0
   || y2 < 0)
    continue; // not drawn if moved entirely offscreen

  switch(console[i].style)
  {
   case CONSOLE_STYLE_BASIC:
    has_header = 1;
    header_position = 0;
    header_h = CONSOLE_BASIC_HEADER_H;
    y1 += header_h;
    y2 += header_h;
    has_scrollbar = 1;
    text_alignment = ALLEGRO_ALIGN_LEFT;
    highlight_recent_lines = 1;
    has_background = 1;
    text_x_offset = 0;
    break;
   case CONSOLE_STYLE_BASIC_UP:
    has_header = 1;
    header_position = 1;
    header_h = CONSOLE_BASIC_HEADER_H;
    y1 -= header_h;
    y2 -= header_h;
    has_scrollbar = 1;
    text_alignment = ALLEGRO_ALIGN_LEFT;
    highlight_recent_lines = 1;
    has_background = 1;
    text_x_offset = 0;
    break;
   case CONSOLE_STYLE_CLEAR_CENTRED:
    has_header = 0;
    header_position = 0;
    header_h = 0;
    has_scrollbar = 0;
    text_alignment = ALLEGRO_ALIGN_CENTRE;
    highlight_recent_lines = 0;
    has_background = 0;
    text_x_offset = console[i].w_pixels / 2;
    break;
   case CONSOLE_STYLE_BOX:
    has_header = 1;
    header_position = 0;
    header_h = CONSOLE_BOX_HEADER_H;
    y1 += header_h;
    y2 += header_h;
    has_scrollbar = 0;
    text_alignment = ALLEGRO_ALIGN_LEFT;
    highlight_recent_lines = 0;
    has_background = 1;
    text_x_offset = 0;
    break;
   case CONSOLE_STYLE_BOX_UP:
    has_header = 1;
    header_position = 1;
    header_h = CONSOLE_BOX_HEADER_H;
    y1 -= header_h;
    y2 -= header_h;
    has_scrollbar = 0;
    text_alignment = ALLEGRO_ALIGN_LEFT;
    highlight_recent_lines = 0;
    has_background = 1;
    text_x_offset = 0;
    break;
   default:
    fprintf(stdout, "\nError: i_console.c: display_consoles(): console %i invalid style %i", i, console[i].style);
    error_call();
    return;

  }


  lines_printed = console[i].h_lines;

#define CONSOLE_BUTTON_Y 2

#define MAIN_BEND_SIZE 6
#define SMALL_BEND_SIZE 3

// draw the header above the main part of the console:
  if (has_header)
  {
   if (header_position == 0)
   {
//    al_draw_filled_rectangle(x1, y1 - header_h, x2, y1, colours.base_fade [console[i].colour_index] [4]);
  if (console[i].open == CONSOLE_OPEN_MIN
			|| console[i].h_lines == 0)
    draw_console_box(x2, y1 - header_h, x1, y1 - SMALL_BEND_SIZE, colours.base_fade [console[i].colour_index] [4],
																					-MAIN_BEND_SIZE, MAIN_BEND_SIZE, -SMALL_BEND_SIZE, SMALL_BEND_SIZE,
																					1);
					else
      draw_console_box(x2, y1 - header_h, x1, y1, colours.base_fade [console[i].colour_index] [4],
																					-MAIN_BEND_SIZE, MAIN_BEND_SIZE, -SMALL_BEND_SIZE, SMALL_BEND_SIZE,
																					0);
//    if (draw_border)
//     al_draw_rectangle(x1, y1 - header_h, x2, y1, colours.base [console[i].colour_index] [SHADE_MED], 1);
    al_draw_textf(font[FONT_SQUARE_BOLD].fnt, colours.base [console[i].colour_index] [SHADE_MAX], x1 + 5, y1 - header_h + 3, 0, "%s", console[i].title);
   }
    else
    {
//     al_draw_filled_rectangle(x1, y2, x2, y2 + header_h, colours.base_fade [console[i].colour_index] [4]);
  if (console[i].open == CONSOLE_OPEN_MIN
			|| console[i].h_lines == 0)
    draw_console_box(x2, y2 + header_h, x1, y2 + SMALL_BEND_SIZE, colours.base_fade [console[i].colour_index] [4],
																					-MAIN_BEND_SIZE, -MAIN_BEND_SIZE, -SMALL_BEND_SIZE, -SMALL_BEND_SIZE,
																					1);
					else
      draw_console_box(x2, y2 + header_h, x1, y2, colours.base_fade [console[i].colour_index] [4],
																					-MAIN_BEND_SIZE, -MAIN_BEND_SIZE, -SMALL_BEND_SIZE, -SMALL_BEND_SIZE,
																					0);
     //if (draw_border)
//      al_draw_rectangle(x1, y2, x2, y2 + header_h, colours.base [console[i].colour_index] [SHADE_MED], 1);
     al_draw_textf(font[FONT_SQUARE_BOLD].fnt, colours.base [console[i].colour_index] [SHADE_MAX], x1 + 5, y2 + 3, 0, "%s", console[i].title);
    }

   int button_shade = SHADE_LOW;
   int bx = x1 + console[i].button_x [0];
   int by = y1 - header_h + CONSOLE_BUTTON_Y;
   if (header_position == 1)
    by = y2 + CONSOLE_BUTTON_Y;

   if (console[i].button_highlight == 0)
    al_draw_rectangle(bx - 1, by - 1, bx + CONSOLE_BUTTON_SIZE + 2, by + CONSOLE_BUTTON_SIZE + 2, colours.base [console[i].colour_index] [SHADE_MAX], 1);
   al_draw_filled_rectangle(bx, by, bx + CONSOLE_BUTTON_SIZE, by + CONSOLE_BUTTON_SIZE, colours.base [console[i].colour_index] [button_shade]);
   if (console[i].open == CONSOLE_OPEN_MAX)
   {
    if (header_position == 0)
     al_draw_filled_rectangle(bx + 1, by + 1, bx + CONSOLE_BUTTON_SIZE - 1, by + 3, colours.base [console[i].colour_index] [SHADE_MED]);
      else
       al_draw_filled_rectangle(bx + 1, by + CONSOLE_BUTTON_SIZE - 3, bx + CONSOLE_BUTTON_SIZE - 1, by + CONSOLE_BUTTON_SIZE - 1, colours.base [console[i].colour_index] [SHADE_MED]);
   }
     else
     {
      al_draw_filled_rectangle(bx + 1, by + 1, bx + CONSOLE_BUTTON_SIZE - 1, by + CONSOLE_BUTTON_SIZE - 1, colours.base [console[i].colour_index] [SHADE_MED]);
     }

  }

  if (console[i].open == CONSOLE_OPEN_MIN // already checked for CONSOLE_CLOSED
			|| console[i].h_lines == 0)
   continue;

  if (has_background)
  {
/*   if (header_position == 1)
    draw_console_box(x1, y1, x2, y2, colours.base_fade [console[i].colour_index] [2],
																					MAIN_BEND_SIZE, MAIN_BEND_SIZE, SMALL_BEND_SIZE, SMALL_BEND_SIZE);
					else
      draw_console_box(x1, y2, x2, y1, colours.base_fade [console[i].colour_index] [2],
																					  MAIN_BEND_SIZE, -MAIN_BEND_SIZE, SMALL_BEND_SIZE, -SMALL_BEND_SIZE);*/

// Don't worry about giving the box a bend (this gets a bit complicated with scrollbars, line highlighting etc)
   al_draw_filled_rectangle(x1, y1, x2, y2, colours.base_fade [console[i].colour_index] [2]); //colours.console_background);
//   if (draw_border)
//    al_draw_rectangle(x1, y1, x2, y2, colours.base [console[i].colour_index] [SHADE_MED], 1);
  }

  int cline_pos = console[i].window_pos + console[i].cpos - 1;
  if (cline_pos < 0)
   cline_pos += CLINES;
  cline_pos %= CLINES;
  int display_line_pos = lines_printed;

// if the way console lines are displayed is changed, may also need to change the way mouse clicks are dealt with (see check_mouse_on_consoles() below)


   if (console[i].line_highlight != -1)
   {
    int highlight_y = y1 + ((console[i].line_highlight) * font[console[i].font_index].height) + CONSOLE_LINE_OFFSET;
    al_draw_filled_rectangle(x1 + 1, highlight_y - 4, x2 - 1, highlight_y + font[console[i].font_index].height - 3, colours.base_fade [console[i].colour_index] [3]);
    //al_draw_filled_rectangle(x1 + 1, highlight_y - 4, x2 - 1 - SLIDER_BUTTON_SIZE, highlight_y + font[console[i].font_index].height - 3, colours.base_fade [console[i].colour_index] [3]);
   }

  while (display_line_pos > 0)
  {
   x = x1;
   y = y1 + ((display_line_pos - 1) * font[console[i].font_index].height) + CONSOLE_LINE_OFFSET;

   if (highlight_recent_lines
    && console[i].cline[cline_pos].time_stamp > w.total_time)
   {
    unsigned int background_shade = (console[i].cline[cline_pos].time_stamp - w.total_time) / 2;
//    if (background_shade >= CONSOLE_LINE_FADE) // don't need to check negatives as background_shade is unsigned
//     background_shade = CONSOLE_LINE_FADE - 1;
    if (background_shade >= CLOUD_SHADES) // don't need to check negatives as background_shade is unsigned
     background_shade = CLOUD_SHADES - 1;
    background_shade = 4;
				float line_height = (float) (font[console[i].font_index].height * (console[i].cline[cline_pos].time_stamp - w.total_time)) / (CONSOLE_LINE_FADE);
				if (line_height > font[console[i].font_index].height / 2)
					 line_height = font[console[i].font_index].height / 2;
//    al_draw_filled_rectangle(x1 + 1, y - 4 + (font[console[i].font_index].height/2) - line_height, x2 - 1 - SLIDER_BUTTON_SIZE, y - 4 + (font[console[i].font_index].height/2) + line_height,
    al_draw_filled_rectangle(x1 + 1, y - 4 + (font[console[i].font_index].height/2) - line_height, x2 - 1, y - 4 + (font[console[i].font_index].height/2) + line_height,
																													colours.base_fade [console[i].colour_index] [background_shade]); //colours.print_fade [console[i].cline[cline_pos].colour] [background_shade]);

//    al_draw_filled_rectangle(x1 + 1, y - 4, x2 - 1 - SLIDER_BUTTON_SIZE, y + font[console[i].font_index].height - 4, colours.base_fade [console[i].colour_index] [background_shade]); //colours.print_fade [console[i].cline[cline_pos].colour] [background_shade]);
//    al_draw_filled_rectangle(x1 + 1, y - 4, x2 - 1 - SLIDER_BUTTON_SIZE, y + font[console[i].font_index].height - 4, colours.base_fade [console[i].colour_index] [background_shade]); //colours.print_fade [console[i].cline[cline_pos].colour] [background_shade]);
   }


   if (console[i].cline[cline_pos].action_type != CONSOLE_ACTION_NONE)
    al_draw_textf(font[FONT_SQUARE].fnt, colours.base [console[i].colour_index] [SHADE_MED], x + 1, y, ALLEGRO_ALIGN_LEFT, ">");
//   if (console[i].cline[cline_pos].used != 0)
//    al_draw_textf(font[FONT_SQUARE].fnt, colours.print [console[i].cline[cline_pos].colour], x + 8, y, ALLEGRO_ALIGN_RIGHT, "-");

   al_draw_textf(font[FONT_SQUARE].fnt, colours.print [console[i].cline[cline_pos].colour], x + 10 + text_x_offset, y, text_alignment, "%s", console[i].cline[cline_pos].text);
   display_line_pos --;
   cline_pos --;
   if (cline_pos == -1)
    cline_pos = CLINES - 1;
  };
/*
   if (console[i].line_highlight != -1)
   {
    y = y1 + ((console[i].line_highlight) * font[console[i].font_index].height) + CONSOLE_LINE_OFFSET;
//    al_draw_filled_rectangle(x1 + 1, y - 4, x2 - 1 - SLIDER_BUTTON_SIZE, y + font[console[i].font_index].height - 3, colours.base_fade [console[i].colour_index] [6]);
    al_draw_rectangle(x1 + 1, y - 4, x2 - 1 - SLIDER_BUTTON_SIZE, y + font[console[i].font_index].height - 3, colours.base [console[i].colour_index] [SHADE_LOW], 1);
   }*/

  if (has_scrollbar)
   draw_scrollbar(&console[i].scrollbar_v, 0, 0);

 } // end of for i loop


}


// main bent corner is at xa, ya (which doesn't have to be top left - bend size x or y should be negative if bend on right or bottom)
// small bent corner is at xb, ya
void draw_console_box(float xa, float ya, float xb, float yb, ALLEGRO_COLOR col, float main_bend_size_x, float main_bend_size_y, float small_bend_size_x, float small_bend_size_y, int all_corners)
{
	ALLEGRO_VERTEX cbuffer [18];
 int b = 0;

// triangle 1
	 cbuffer[b].x = xa;
	 cbuffer[b].y = ya + main_bend_size_y;
 	cbuffer[b].z = 0;
 	cbuffer[b].color = col;
 	b++;
 	cbuffer[b].x = xa + main_bend_size_x;
 	cbuffer[b].y = ya;
 	cbuffer[b].z = 0;
 	cbuffer[b].color = col;
 	b++;
 	cbuffer[b].x = xb - small_bend_size_x;
 	cbuffer[b].y = ya;
 	cbuffer[b].z = 0;
 	cbuffer[b].color = col;
 	b++;

// triangle 2
 	cbuffer[b].x = xa;
 	cbuffer[b].y = ya + main_bend_size_y;
 	cbuffer[b].z = 0;
 	cbuffer[b].color = col;
 	b++;
 	cbuffer[b].x = xb - small_bend_size_x;
 	cbuffer[b].y = ya;
 	cbuffer[b].z = 0;
 	cbuffer[b].color = col;
 	b++;
 	cbuffer[b].x = xb;
 	cbuffer[b].y = ya + small_bend_size_y;
 	cbuffer[b].z = 0;
	 cbuffer[b].color = col;
	 b++;

// triangle 3
	 cbuffer[b].x = xa;
	 cbuffer[b].y = ya + main_bend_size_y;
	 cbuffer[b].z = 0;
	 cbuffer[b].color = col;
	 b++;
	 cbuffer[b].x = xb;
	 cbuffer[b].y = ya + small_bend_size_y;
	 cbuffer[b].z = 0;
	 cbuffer[b].color = col;
	 b++;
	 cbuffer[b].x = xb;
	 cbuffer[b].y = yb;
	 cbuffer[b].z = 0;
	 cbuffer[b].color = col;
	 b++;

// triangle 4
	 cbuffer[b].x = xa;
	 cbuffer[b].y = ya + main_bend_size_y;
	 cbuffer[b].z = 0;
	 cbuffer[b].color = col;
	 b++;
	 cbuffer[b].x = xa;
	 cbuffer[b].y = yb;
	 cbuffer[b].z = 0;
	 cbuffer[b].color = col;
	 b++;
	 cbuffer[b].x = xb;
	 cbuffer[b].y = yb;
	 cbuffer[b].z = 0;
	 cbuffer[b].color = col;
	 b++;

  if (all_corners == 1) // these triangles extend past yb, so need to adjust yb in the call
	 {

// triangle 5
	  cbuffer[b].x = xa;
	  cbuffer[b].y = yb;
	  cbuffer[b].z = 0;
	  cbuffer[b].color = col;
	  b++;
	  cbuffer[b].x = xb;
	  cbuffer[b].y = yb;
	  cbuffer[b].z = 0;
	  cbuffer[b].color = col;
	  b++;
	  cbuffer[b].x = xb - small_bend_size_x;
	  cbuffer[b].y = yb + small_bend_size_y;
	  cbuffer[b].z = 0;
	  cbuffer[b].color = col;
	  b++;

// triangle 6
	  cbuffer[b].x = xa;
	  cbuffer[b].y = yb;
	  cbuffer[b].z = 0;
	  cbuffer[b].color = col;
	  b++;
	  cbuffer[b].x = xa + small_bend_size_x;
	  cbuffer[b].y = yb + small_bend_size_y;
	  cbuffer[b].z = 0;
	  cbuffer[b].color = col;
	  b++;
	  cbuffer[b].x = xb - small_bend_size_x;
	  cbuffer[b].y = yb + small_bend_size_y;
	  cbuffer[b].z = 0;
	  cbuffer[b].color = col;
	  b++;


	 }

	al_draw_prim(cbuffer, NULL, NULL, 0, b, ALLEGRO_PRIM_TRIANGLE_LIST);


}


// This function checks whether the mouse is on top of a console
// If the button has just been pressed, it also checks whether the mouse has clicked on one of the console size buttons (and changes the size if so)
// will also in future do the following:
//  - if used clicked on a line in console, returns:
//   - if line printed by proc: number of proc (could be zero)
//   - if line printed by client: (number of client * -1) - 1
//   - otherwise, returns -100
// returns after it finds the first console the mouse is on
void check_mouse_on_consoles_etc(int x, int y, int just_pressed)
{

 int i;

 int y1, y2;
 int has_header = 0;
 int header_y = 0;
 int header_size = CONSOLE_BASIC_HEADER_H;
// int accepts_mouse_clicks = 1; // this means accepts mouse clicks in the text

 for (i = 0; i < CONSOLES; i ++)
 {
  console[i].line_highlight = -1;
 }

 ex_control.console_action_type = CONSOLE_ACTION_NONE;

 for (i = CONSOLES - 1; i >= 0; i --)
// Note reverse direction of i count. This is because the higher numbered consoles are displayed on top
//  TO DO: think about - should there be some kind of ordering? Maybe click on the top bar to bring to front? Hm
 {
  if (console[i].open == CONSOLE_CLOSED)
   continue;

  switch(console[i].style)
  {
   case CONSOLE_STYLE_BASIC:
    has_header = 1;
    header_size = CONSOLE_BASIC_HEADER_H;
    y1 = console[i].y + header_size;
    y2 = console[i].y + header_size + console[i].h_pixels;
    header_y = console[i].y;
    break;
   case CONSOLE_STYLE_BASIC_UP:
    has_header = 1;
    header_size = CONSOLE_BASIC_HEADER_H;
    y1 = console[i].y - header_size - console[i].h_pixels;
    y2 = console[i].y - header_size;
    header_y = y2;
    break;
   case CONSOLE_STYLE_CLEAR_CENTRED:
    has_header = 0;
    header_size = 0;
    y1 = console[i].y;
    y2 = console[i].y + console[i].h_pixels;
    break;
   case CONSOLE_STYLE_BOX:
    has_header = 1;
    header_size = CONSOLE_BOX_HEADER_H;
    y1 = console[i].y + header_size;
    y2 = console[i].y + header_size + console[i].h_pixels;
    break;
   case CONSOLE_STYLE_BOX_UP:
    has_header = 1;
    header_size = CONSOLE_BOX_HEADER_H;
    y1 = console[i].y - header_size - console[i].h_pixels;
    y2 = console[i].y - header_size;
    header_y = y2;
    break;
   default:
    fprintf(stdout, "\nError in i_console.c: check_mouse_on_consoles_etc(): console %i is invalid style %i", i, console[i].style);
    error_call();
    return;

  }

  console[i].button_highlight = -1;

  if (x >= console[i].x
   && x <= console[i].x + console[i].w_pixels)
  {
   if (has_header == 1
    && y >= header_y
    && y < header_y + header_size)
   {
// mouse in console header
     control.mouse_status = MOUSE_STATUS_CONSOLE;

     if (x >= console[i].x + console[i].button_x [0] - 2
      && x <= console[i].x + console[i].button_x [0] + CONSOLE_BUTTON_SIZE + 2)
     {
       console[i].button_highlight = 0;
       if (just_pressed)
       {
        if (console[i].open == CONSOLE_OPEN_MIN)
         set_console_open(i, CONSOLE_OPEN_MAX);
          else
           set_console_open(i, CONSOLE_OPEN_MIN);
       }
     }
    return;
   }
   if (console[i].open == CONSOLE_OPEN_MAX // have already checked for mouse in console header
    && x < console[i].x + console[i].w_pixels
    && y >= y1
    && y <= y2)
   {
    control.mouse_status = MOUSE_STATUS_CONSOLE;

// If mouse on scrollbar, just set mouse_status to MOUSE_STATUS_CONSOLE then return (no line highlighting):
    if (x > console[i].x + console[i].w_pixels - SLIDER_BUTTON_SIZE)
					return;

    int cline_pos = console[i].window_pos + console[i].cpos - console[i].h_lines;// - 1;

//    int mouse_cline = ((y - console[i].y - header_top_offset) / LINE_HEIGHT); // this is the line on the screen, not accounting for scrolling of the console
//    int mouse_cline = ((y - y1 - header_top_offset) / LINE_HEIGHT); // this is the line on the screen, not accounting for scrolling of the console
    int mouse_cline = ((y - y1) / font[console[i].font_index].height); // this is the line on the screen, not accounting for scrolling of the console

    if (mouse_cline >= console[i].h_lines)
     mouse_cline = console[i].h_lines - 1;

    console[i].line_highlight = mouse_cline;

    if (just_pressed)
    {

     mouse_cline += cline_pos; // now adjust for scrolling

     if (mouse_cline < 0) // cline_pos can be -ve
      mouse_cline += CLINES;

     mouse_cline %= CLINES;

     if (console[i].cline[mouse_cline].action_type != CONSOLE_ACTION_NONE)
     {
      ex_control.console_action_type = console[i].cline[mouse_cline].action_type;
      ex_control.console_action_console = i;
      ex_control.console_action_val1 = console[i].cline[mouse_cline].action_val1;
      ex_control.console_action_val2 = console[i].cline[mouse_cline].action_val2;
      ex_control.console_action_val3 = console[i].cline[mouse_cline].action_val3;
     }

    }
    return;
   }
  }

 } // end of for loop

 control.mouse_status = MOUSE_STATUS_AVAILABLE;

 check_mouse_on_proc_box(x, y, just_pressed);

 if (view.map_visible == 1
  && x >= view.map_x
  && x <= view.map_x + view.map_w
  && y >= view.map_y
  && y <= view.map_y + view.map_h)
  {
   control.mouse_status = MOUSE_STATUS_MAP;
  }


 return;


}


void set_console_open(int c, int new_status)
{

 if (console[c].open == new_status)
  return;

 console[c].open = new_status;

 if (new_status == 0)
 {
  return; // don't need to reset console size as it's not displayed at all
 }


/*
 switch(new_status)
 {

  case CONSOLE_OPEN_MIN:
//   console[c].h_lines = 0;
   break;
//  case CONSOLE_OPEN_MED:
//   console[c].h_lines = 10;
//   break;
  case CONSOLE_OPEN_MAX:
//   console[c].h_lines = 10;
   break;

 }*/

// console[c].h_pixels = (console[c].h_lines - 1) * LINE_HEIGHT + 20;

 fix_console_slider(c);

// init_slider(&console[c].scrollbar_v, &console[c].window_pos, SLIDEDIR_VERTICAL, console[c].h_lines, CLINES, console[c].h_pixels - 2, 1, console[c].h_lines - 1, console[c].h_lines, console[c].w_pixels - (SLIDER_BUTTON_SIZE / 2), console[c].y + CONSOLE_HEADER_H + 1, SLIDER_BUTTON_SIZE);

}

// new_w and new_h are in lines (not pixels)
// new_h can be from 1 to 40
// doesn't check against size of screen
// force_update is used while loading saved games. It updates console size properties even if new_w and new_h
//  are the same as the console's existing properties.
void set_console_size_etc(int c, int new_w, int new_h, int font_index, int force_update)
{

 if (new_h < CONSOLE_LINES_MIN) // currently 1
  new_h = CONSOLE_LINES_MIN;
 if (new_h > CONSOLE_LINES_MAX) // currently 40
  new_h = CONSOLE_LINES_MAX;

 if (new_w < CONSOLE_LETTERS_MIN)
  new_w = CONSOLE_LETTERS_MIN;
 if (new_w > CONSOLE_LETTERS_MAX)
  new_w = CONSOLE_LETTERS_MAX;

// don't bother updating if the values aren't being changed (unless force_update is set)
 if (console[c].h_lines == new_h
  && console[c].w_letters == new_w
  && console[c].font_index == font_index
  && force_update == 0)
  return;

 console[c].w_letters = new_w;
 console[c].h_lines = new_h;

 console[c].w_pixels = (console[c].w_letters * font[console[c].font_index].width) + 25 + 10;
 console[c].h_pixels = (console[c].h_lines - 1) * font[console[c].font_index].height + 20;

 console[c].button_x [0] = console[c].w_pixels - 4 - CONSOLE_BUTTON_SEPARATION;

 fix_console_slider(c);
 clear_console(c);

// fprintf (stdout, "\nConsole %i size %i pixels %i", c, new_size, console[c].h_pixels);

}

// Changes number of lines in console.
// Doesn't clear console.
void set_console_lines(int c, int new_h)
{

 if (new_h < CONSOLE_LINES_MIN) // currently 1
  new_h = CONSOLE_LINES_MIN;
 if (new_h > CONSOLE_LINES_MAX) // currently 40
  new_h = CONSOLE_LINES_MAX;

 if (console[c].h_lines == new_h)
  return;

 console[c].h_lines = new_h;
 console[c].h_pixels = (console[c].h_lines - 1) * font[console[c].font_index].height + 20;

 fix_console_slider(c);

}

void fix_console_slider(int c)
{
 switch(console[c].style)
 {
  case CONSOLE_STYLE_BASIC:
   init_slider(&console[c].scrollbar_v, &console[c].window_pos, SLIDEDIR_VERTICAL, console[c].h_lines, CLINES, console[c].h_pixels - 2, 1, console[c].h_lines - 1, console[c].h_lines, console[c].x + console[c].w_pixels - SLIDER_BUTTON_SIZE, console[c].y + CONSOLE_BASIC_HEADER_H + 1, SLIDER_BUTTON_SIZE, console[c].colour_index, 1);
   break;
  case CONSOLE_STYLE_BASIC_UP:
//   init_slider(&console[c].scrollbar_v, &console[c].window_pos, SLIDEDIR_VERTICAL, console[c].h_lines, CLINES, console[c].h_pixels - 2, 1, console[c].h_lines - 1, console[c].h_lines, console[c].x + console[c].w_pixels - SLIDER_BUTTON_SIZE, console[c].y + 1, SLIDER_BUTTON_SIZE, COL_BLUE);
   init_slider(&console[c].scrollbar_v, &console[c].window_pos, SLIDEDIR_VERTICAL, console[c].h_lines, CLINES, console[c].h_pixels - 2, 1, console[c].h_lines - 1, console[c].h_lines, console[c].x + console[c].w_pixels - SLIDER_BUTTON_SIZE, console[c].y - console[c].h_pixels - CONSOLE_BASIC_HEADER_H + 1, SLIDER_BUTTON_SIZE, console[c].colour_index, 1);
   break;
 }

}




// deletes all messages from a console
// assumes c is a valid console index
void clear_console(int c)
{
 int j;

  for (j = 0; j < CLINES; j ++)
  {
   console[c].cline[j].used = 0;
   console[c].cline[j].text [0] = 0;
   console[c].cline[j].source_program_type = -1;
   console[c].cline[j].source_index = -1;
   console[c].cline[j].colour = PRINT_COL_LGREY;
   console[c].cline[j].action_type = CONSOLE_ACTION_NONE;
   console[c].cline[j].action_val1 = 0;
   console[c].cline[j].action_val2 = 0;
   console[c].cline[j].action_val3 = 0;
   console[c].cline[j].time_stamp = 0;
  }

  console[c].window_pos = CLINES;
  console[c].cpos = 0;
  slider_moved_to_value(&console[c].scrollbar_v, console[c].window_pos);

}

int run_console_method(struct programstruct* cl, int m)
{

 int mbase = m * METHOD_SIZE;
 s16b *mb = &cl->regs.mbank [mbase];

 int c = mb [1]; // note: must always bounds-check c (can't do it here as some modes require c==-1 to be valid or use mb[1] for a different purpose)
 int p;
 int i;

 switch(mb [0])
 {
  case MSTATUS_OB_CONSOLE_OPEN: //  - field 1: console index (0-3)
   if (c < 0 || c >= CONSOLES)
    return 0;
   if (console[c].open == CONSOLE_OPEN_MAX)
    return 0;
   set_console_open(c, CONSOLE_OPEN_MAX);
   return 1;
  case MSTATUS_OB_CONSOLE_CLOSE: //  - field 1: console index (0-3)
   if (c < 0 || c >= CONSOLES)
    return 0;
   if (console[c].open == CONSOLE_CLOSED)
    return 0;
   finish_console_line(c);
   console[c].open = CONSOLE_CLOSED;
   return 1;
  case MSTATUS_OB_CONSOLE_MIN: //  - field 1: console index (0-3)
   if (c < 0 || c >= CONSOLES)
    return 0;
   if (console[c].open == CONSOLE_OPEN_MIN)
    return 0;
   set_console_open(c, CONSOLE_OPEN_MIN);
   return 1;
  case MSTATUS_OB_CONSOLE_STATE: //  - field 1: index //  - field 2: closed = 0, minimised = 1, med = 2, large = 3
   if (c < 0 || c >= CONSOLES)
    return 0;
   mb [2] = console[c].open;
   return mb [2];
  case MSTATUS_OB_CONSOLE_GET_XY: //  - field 1: index //  - fields 2, 3: x/y
   if (c < 0 || c >= CONSOLES)
    return 0;
//   if (console[c].open == CONSOLE_CLOSED)
//    return 0; should be able to relocate even when console is currently closed
   mb [2] = console[c].x;
   mb [3] = console[c].y;
//   fix_console_slider(c);
   return 1;
  case MSTATUS_OB_CONSOLE_MOVE: //  - field 1: index //  - fields 2, 3: x/y
//fprintf(stdout, "\n c %i", c);
   if (c < 0 || c >= CONSOLES)
    return 0;
//fprintf(stdout, "\n B");
//   if (console[c].open == 0)
//    return 0; // should this be an error? not sure - is there a reason to allow closed consoles to be moved?
   console[c].x = mb [2];
   console[c].y = mb [3]; // no bounds checking. if console off screen it just won't be drawn and can't be clicked on
   fix_console_slider(c);
//   fprintf(stdout, "\n relocated %i, %i", console[c].x, console[c].y);
   return 1;
  case MSTATUS_OB_CONSOLE_SIZE: //field 1: index // field 2: width (1-40) (is bounds_checked in set_console_size) field 3: height
   if (c < 0 || c >= CONSOLES)
    return 0;
// This can only be done while the console is closed
//   if (console[c].open != CONSOLE_CLOSED)
//    return 0;
   set_console_size_etc(c, mb [2], mb [3], console[c].font_index, 0); // 0 means console won't be updated if the values aren't actually being changed
   return 1;
  case MSTATUS_OB_CONSOLE_LINES:
   if (c < 0 || c >= CONSOLES)
    return 0;
   set_console_lines(c, mb[2]);
   return 1;
  case MSTATUS_OB_CONSOLE_ACTION_SET: //field 1: c, field 2: val1, field 3: val2 (action type is set automatically)
   if (c < 0 || c >= CONSOLES)
    return 0;
   if (!cl->bcode.printing)
    return 0; // program isn't printing anything, so there's nothing to attach an action to
   switch(cl->type)
   {
    case PROGRAM_TYPE_OPERATOR:
     console[c].cline[console[c].cpos].action_type = CONSOLE_ACTION_OPERATOR; break;
    case PROGRAM_TYPE_SYSTEM:
     console[c].cline[console[c].cpos].action_type = CONSOLE_ACTION_SYSTEM; break;
    case PROGRAM_TYPE_OBSERVER:
     console[c].cline[console[c].cpos].action_type = CONSOLE_ACTION_OBSERVER; break;
    default: // probably should never happen?
     console[c].cline[console[c].cpos].action_type = CONSOLE_ACTION_NONE; break;
// The other type of console action is a process action, but that is automatically applied when a process sends a message and isn't relevant here.
   }
//   fprintf(stdout, "\nSetting cons %i line %i to action type %i ", c, console[c].cpos, console[c].cline[console[c].cpos].action_type);

   console[c].cline[console[c].cpos].action_val1 = mb [2];
   console[c].cline[console[c].cpos].action_val2 = mb [3];
   return 1;
  case MSTATUS_OB_CONSOLE_ACTION_CHECK: // field 1: action_type, field 2, 3, 4: action_val1, 2, 3. The method returns 0 if no action set this tick
// Note that:
//  - Only one action should ever occur each tick (as the user can click on one line at most)
//  - This method can be called multiple times in the same tick, by different programs, and will return the same result each time.
//   - This means that different programs (e.g. system and operator) can both be waiting for different
//     actions without interfering with each other.
//   if (c < 0 || c >= CONSOLES)
//    return 0; - not relevant
   if (ex_control.console_action_type == CONSOLE_ACTION_NONE)
    return CONSOLE_ACTION_NONE; // nothing was clicked this tick
   mb [0] = ex_control.console_action_console; // remember - mb [1] is usually console number, but can use it here as this is intended for reading
   mb [1] = ex_control.console_action_val1;
   mb [2] = ex_control.console_action_val2;
   mb [3] = ex_control.console_action_val3;
   return ex_control.console_action_type;
  case MSTATUS_OB_CONSOLE_CLEAR: // field 1: console
   if (c < 0 || c >= CONSOLES)
    return 0;
   clear_console(c);
   return 1;
  case MSTATUS_OB_CONSOLE_STYLE: // field 1: console; field 2: style setting
   if (c < 0 || c >= CONSOLES)
    return 0;
// style can't be changed while a console is open
   if (console[c].open != CONSOLE_CLOSED
    || mb [2] < 0
    || mb [2] >= CONSOLE_STYLES)
    return 0;
   console[c].style = mb [2];
   return 1;

  case MSTATUS_OB_CONSOLE_OUT: // sets which console current program's output is sent to (field 1: console)
   if (c < 0
    || c >= CONSOLES)
     w.current_output_console = -1;
      else
       w.current_output_console = c;
   break;
  case MSTATUS_OB_CONSOLE_ERR: // sets which console current program's error output is sent to (field 1: console)
   if (c < 0
    || c >= CONSOLES)
     w.current_error_console = -1;
      else
       w.current_error_console = c;
   break;
  case MSTATUS_OB_CONSOLE_COLOUR:
// affects the next line (lines?) of text printed to any console
// doesn't use c as it is not specific to any console
   if (mb[1] >= 0
    && mb[1] < PRINT_COLS)
     w.print_colour = mb[1];
   break;
  case MSTATUS_OB_CONSOLE_FONT:
   if (c < 0 || c >= CONSOLES)
    return 0;
   switch(mb[2])
   {
   	case FONT_BASIC: set_console_size_etc(c, console[c].w_letters, console[c].h_lines, FONT_BASIC, 0); break;
   	case FONT_BASIC_BOLD: set_console_size_etc(c, console[c].w_letters, console[c].h_lines, FONT_BASIC_BOLD, 0); break;
   	case FONT_SQUARE: set_console_size_etc(c, console[c].w_letters, console[c].h_lines, FONT_SQUARE, 0); break;
   	case FONT_SQUARE_BOLD: set_console_size_etc(c, console[c].w_letters, console[c].h_lines, FONT_SQUARE_BOLD, 0); break;
   	case FONT_SQUARE_LARGE: set_console_size_etc(c, console[c].w_letters, console[c].h_lines, FONT_SQUARE_LARGE, 0); break;
// default does nothing
   }
   break;
  case MSTATUS_OB_CONSOLE_BACKGROUND:
   if (c < 0 || c >= CONSOLES)
    return 0;
   if (mb[2] >= 0
    && mb[2] < BASIC_COLS)
    console[c].colour_index = mb[2];
			break;

  case MSTATUS_OB_CONSOLE_TITLE:
   if (c < 0 || c >= CONSOLES)
    return 0;
   if (mb[2] >= 0
    && mb[2] < cl->bcode.bcode_size - (CONSOLE_TITLE_LENGTH + 1))
   {
   	for (i = 0; i < CONSOLE_TITLE_LENGTH; i ++)
				{
					console[c].title [i] = cl->bcode.op [mb[2] + i];
					if (cl->bcode.op [mb[2] + i] == 0)
						 break;
				}
				console[c].title [CONSOLE_TITLE_LENGTH - 1] = '0';
   }
			break;

		case MSTATUS_OB_CONSOLE_LINES_USED:
   if (c < 0 || c >= CONSOLES)
    return 0;
   int lines_used = 0;
   for (i = 0; i < CLINES; i ++)
			{
				if (console[c].cline [i].text [0] != '\0')
					 lines_used ++;
			}
		 return lines_used;

  case MSTATUS_OB_CONSOLE_OUT_SYSTEM: //  - field 1: console index (-1 to set ignore)
   if (c < 0
    || c >= CONSOLES)
     w.system_output_console = -1;
      else
       w.system_output_console = c;
   return 1;
  case MSTATUS_OB_CONSOLE_OUT_OBSERVER: //  - field 1: console index (-1 to set ignore)
   if (c < 0
    || c >= CONSOLES)
     w.observer_output_console = -1;
      else
       w.observer_output_console = c;
   return 1;
  case MSTATUS_OB_CONSOLE_OUT_PLAYER:
   p = mb [2];
   if (p < 0
    || p >= w.players)
     return 0;
   if (c < 0
    || c >= CONSOLES)
     w.player[p].output_console = -1;
      else
       w.player[p].output_console = c;
   return 1;
  case MSTATUS_OB_CONSOLE_OUT2_PLAYER:
   p = mb [2];
   if (p < 0
    || p >= w.players)
     return 0;
   if (c < 0
    || c >= CONSOLES)
     w.player[p].output2_console = -1;
      else
       w.player[p].output2_console = c;
   return 1;

  case MSTATUS_OB_CONSOLE_ERR_SYSTEM: //  - field 1: console index (-1 to set ignore)
   if (c < 0
    || c >= CONSOLES)
     w.system_error_console = -1;
      else
       w.system_error_console = c;
   return 1;
  case MSTATUS_OB_CONSOLE_ERR_OBSERVER: //  - field 1: console index (-1 to set ignore)
   if (c < 0
    || c >= CONSOLES)
     w.observer_error_console = -1;
      else
       w.observer_error_console = c;
   return 1;
  case MSTATUS_OB_CONSOLE_ERR_PLAYER:
   p = mb [2];
   if (p < 0
    || p >= w.players)
     return 0;
   if (c < 0
    || c >= CONSOLES)
     w.player[p].error_console = -1;
      else
       w.player[p].error_console = c;
   return 1;

 }

 return 0; // invalid status

}



void check_mouse_on_proc_box(int x, int y, int just_pressed)
{

  proc_box.button_highlight = 0;

  if (view.focus_proc != NULL
   && x >= proc_box.x1
   && x <= proc_box.x2
   && y >= proc_box.y1
   && ((proc_box.maximised == 1
     && y <= proc_box.y2)
    || (proc_box.maximised == 0
     && y <= proc_box.y1 + PBOX_LINE + 3)))
  {
   control.mouse_status = MOUSE_STATUS_PROCESS;
   if (y >= proc_box.y1
    && y < proc_box.y1 + PBOX_HEADER_HEIGHT)
   {
// mouse in proc box header
     if (x >= proc_box.button_x1 - 2
      && x <= proc_box.button_x2 + 2)
     {
       proc_box.button_highlight = 1;
       if (just_pressed)
       {
        if (proc_box.maximised == 0)
         proc_box.maximised = 1;
          else
           proc_box.maximised = 0;

        reset_proc_box_height(view.focus_proc);
       }
     }
    return;
   }

  }

 return;

}



#ifdef SCORE_BOXES

/*

****************************************************************************************

score box code


****************************************************************************************

*/

/*

What are score boxes, and how will they work?
 - they will be under the control of the observer/operator (or system)
 - the operator/observer will be able to set them to display any text or numbers
 - will also be able to set them to display special elements
  - to start with, will be able to display an irpt use graph

What will they look like:
 - probably vertical rows

But maybe implement all of this later.
For now, just have:
 - a score (set by observer, which gets it from the system program)
 - number of procs / max
 - irpt_use / lowest threshold
 - irpt choke
 - graph of irpt use

Scores:
 - the system will have control over the game score. There'll probably be several variables to record score, with the system program controlling what they are and what they do.
 - the system program will be able to set a winner etc.

Will also need a way for client programs to ask the system program about game rules and score
 - maybe just the settings method?
 - can use something similar to the proc command array.
 - won't need any extra score values in the worldstruct. Just leave it up to the system program.
 - observer/operator will be able to query values for other players as well.

*/

#define IRPT_HISTORY_LENGTH 128
#define SCORE_BOXES PLAYERS
#define SCORE_BOX_W 150
#define SCORE_BOX_LINE_H 15
#define SCORE_BOX_HEADER_H 15

enum
{
SCORE_BOX_CLOSED,
SCORE_BOX_OPEN_MIN,
SCORE_BOX_OPEN_MAX
};

struct score_boxstruct
{
 int open; // closed, min or max (although possibly always max for now)
 int x, y;
 int w, h;
 int irpt_history [IRPT_HISTORY_LENGTH];
 ALLEGRO_COLOR* irpt_history_colour [IRPT_HISTORY_LENGTH];
 int irpt_history_pos;
 int detailed; // if 0, just has score and procs. If 1, has many things
 int button_x;
 int button_y;
 int button_highlight;

};


struct score_boxstruct score_box [SCORE_BOXES];

// these two things (which are used in displaying score boxes) can be set by init_score_boxes as they remain valid unless the world settings change.
float score_box_gen_threshold [4];
float graph_scale;

// This function is called (from the console init function) in initialisation just before the main game loop starts.
void init_score_boxes(void)
{
 int i;

 for (i = 0; i < SCORE_BOXES; i ++)
 {
  score_box[i].open = SCORE_BOX_CLOSED;
  score_box[i].x = 0;
  score_box[i].y = 0;
  score_box[i].detailed = 0;
  score_box[i].button_highlight = 0;
 }

}


void run_score_boxes(void)
{

// not currently used


}


void display_score_boxes(void)
{

 return;

 float x, y, x2, y2;

 int i, tcol;

 for (i = 0; i < SCORE_BOXES; i ++)
 {
  if (score_box[i].open == SCORE_BOX_CLOSED)
   continue;

  tcol = i;

  x = score_box[i].x;
  y = score_box[i].y;
  x2 = x + score_box[i].w;
  y2 = y + score_box[i].h;

// box
  if (score_box[i].open == SCORE_BOX_OPEN_MAX)
  {
   al_draw_filled_rectangle(x, y + SCORE_BOX_HEADER_H, x2, y2, colours.team [tcol] [TCOL_BOX_FILL]);
   al_draw_rectangle(x, y, x2, y2, colours.team [tcol] [TCOL_BOX_OUTLINE], 1);
// header
   al_draw_filled_rectangle(x + 0.5, y + 0.5, x2 - 0.5, y + SCORE_BOX_HEADER_H - 0.5, colours.team [tcol] [TCOL_BOX_HEADER_FILL]);
   al_draw_line(x + 0.5, y + SCORE_BOX_HEADER_H, x2 - 0.5, y + SCORE_BOX_HEADER_H, colours.team [tcol] [TCOL_BOX_OUTLINE], 1);


/*   al_draw_filled_rectangle(x, y + SCORE_BOX_HEADER_H + 0.5, x2, y2, team_colours [tcol] [TCOL_BOX_FILL]);
   al_draw_rectangle(x, y, x2, y2, team_colours [tcol] [TCOL_BOX_OUTLINE], 1);
// header
   al_draw_filled_rectangle(x + 0.5, y + 0.5, x2 - 0.5, y + SCORE_BOX_HEADER_H - 0.5, team_colours [tcol] [TCOL_BOX_HEADER_FILL]);
   al_draw_line(x + 0.5, y + SCORE_BOX_HEADER_H, x2 - 0.5, y + SCORE_BOX_HEADER_H, team_colours [tcol] [TCOL_BOX_OUTLINE], 1);*/
  }
   else
   {
   al_draw_filled_rectangle(x, y, x2 - 0.5, y + SCORE_BOX_HEADER_H - 0.5, colours.team [tcol] [TCOL_BOX_HEADER_FILL]);
   al_draw_rectangle(x, y, x2 - 0.5, y + SCORE_BOX_HEADER_H - 0.5, colours.team [tcol] [TCOL_BOX_OUTLINE], 1);
//   al_draw_filled_rectangle(x + 0.5, y + 0.5, x2 - 0.5, y + SCORE_BOX_HEADER_H - 0.5, team_colours [tcol] [TCOL_BOX_HEADER_FILL]);
//   al_draw_rectangle(x + 0.5, y + 0.5, x2 - 0.5, y + SCORE_BOX_HEADER_H - 0.5, team_colours [tcol] [TCOL_BOX_OUTLINE], 1);
   }
// header
//  al_draw_filled_rectangle(x + 0.5, y + 0.5, x2 - 0.5, y + SCORE_BOX_HEADER_H - 0.5, team_colours [tcol] [TCOL_BOX_HEADER_FILL]);
//  al_draw_filled_rectangle(x + 0.5, y + 0.5, x2 - 0.5, y + SCORE_BOX_HEADER_H - 0.5, team_colours [tcol] [TCOL_BOX_HEADER_FILL]);
//  al_draw_line(x + 0.5, y + SCORE_BOX_HEADER_H, x2 - 0.5, y + SCORE_BOX_HEADER_H, team_colours [tcol] [TCOL_BOX_OUTLINE], 1);

//  al_draw_textf(bold_font, team_colours [tcol] [TCOL_BOX_TEXT_BOLD], x + 5, y + 3, 0, "Player %i", i);
  al_draw_textf(bold_font, colours.team [tcol] [TCOL_BOX_TEXT_BOLD], x + 5, y + 3, 0, "%s", w.player[i].name);
// button
  al_draw_filled_rectangle(score_box[i].button_x, score_box[i].button_y, score_box[i].button_x + CONSOLE_BUTTON_SIZE, score_box[i].button_y + CONSOLE_BUTTON_SIZE, colours.team [tcol] [TCOL_BOX_BUTTON]);
  if (score_box[i].button_highlight)
   al_draw_rectangle(score_box[i].button_x - 1, score_box[i].button_y - 1, score_box[i].button_x + CONSOLE_BUTTON_SIZE + 1.5, score_box[i].button_y + CONSOLE_BUTTON_SIZE + 1.5, colours.team [tcol] [TCOL_BOX_OUTLINE], 1);
// draw box in button and exit if closed
  if (score_box[i].open == SCORE_BOX_OPEN_MIN)
  {
   al_draw_filled_rectangle(score_box[i].button_x + 1, score_box[i].button_y + 1, score_box[i].button_x + CONSOLE_BUTTON_SIZE - 1, score_box[i].button_y + CONSOLE_BUTTON_SIZE - 1, colours.team [tcol] [TCOL_BOX_OUTLINE]);
   continue;
  }
   else
    al_draw_filled_rectangle(score_box[i].button_x + 1, score_box[i].button_y + 1, score_box[i].button_x + CONSOLE_BUTTON_SIZE - 1, score_box[i].button_y + 3, colours.team [tcol] [TCOL_BOX_OUTLINE]);
  y += SCORE_BOX_HEADER_H;
// score
  al_draw_textf(font, colours.team [tcol] [TCOL_BOX_TEXT], x + 5, y + 3, 0, "Score %i", w.player[i].score);
  y += SCORE_BOX_LINE_H;
// procs
  al_draw_textf(font, colours.team [tcol] [TCOL_BOX_TEXT], x + 5, y + 3, 0, "Processes");
  al_draw_textf(font, colours.team [tcol] [TCOL_BOX_TEXT], x2 - 5, y + 3, ALLEGRO_ALIGN_RIGHT, "%i(%i)", w.player[i].processes, w.procs_per_team);
  y += SCORE_BOX_LINE_H;
  if (score_box[i].detailed == 0)
   continue;
// irpt gen limit
//  al_draw_textf(font, colours.team [tcol] [TCOL_BOX_TEXT], x + 5, y + 3, 0, "Irpt gen");
//  al_draw_textf(font, colours.team [tcol] [TCOL_BOX_TEXT], x2 - 5, y + 3, ALLEGRO_ALIGN_RIGHT, "%i(%i)", w.player[i].gen_number, w.gen_limit);
//  y += SCORE_BOX_LINE_H;
//  if (score_box[i].detailed == 0)
//   continue;

 }

}

// opens a score box for player p
// doesn't assume p is valid
int open_score_box(int p)
{

 if (p < 0
  || p >= w.players
  || w.player[p].active == 0)
  return 0;

 score_box[p].open = SCORE_BOX_OPEN_MAX;
 set_score_box_size(p);
 return 1;

}

int close_score_box(int p)
{

 if (p < 0
  || p >= w.players
  || w.player[p].active == 0)
  return 0;

 score_box[p].open = SCORE_BOX_CLOSED;
 return 1;

}

// opens a score box for player p
// doesn't assume p is valid
int min_max_score_box(int p, int open_status)
{

 if (p < 0
  || p >= w.players
  || w.player[p].active == 0)
  return 0;

 score_box[p].open = open_status;
// set_score_box_size(p);
 return 1;

}


int score_box_position(int p, int x, int y)
{

 if (p < 0
  || p >= w.players
  || w.player[p].active == 0
  || x < -10
  || x > 2000 // not great - but can't use viewstruct values because screen may be resized (and if so it's up to observer program to relocate UI elements)
  || y < -10
  || y > 2000)
  return 0;

 score_box[p].x = x;
 score_box[p].y = y;
 score_box[p].button_x = x + score_box[p].w - 4 - CONSOLE_BUTTON_SEPARATION;
 score_box[p].button_y = y + 2;
 return 1;

}

int score_box_detail(int p, int detail)
{

 if (p < 0
  || p >= w.players
  || w.player[p].active == 0
  || detail < 0
  || detail > 1)
  return 0;

 score_box[p].detailed = detail;
 set_score_box_size(p);
 return 1;
}


void set_score_box_size(int b)
{

 score_box[b].w = SCORE_BOX_W;

 if (score_box[b].detailed == 0)
 {
  score_box[b].h = SCORE_BOX_HEADER_H + SCORE_BOX_LINE_H*2;
  return;
 }

 score_box[b].h = SCORE_BOX_HEADER_H + (SCORE_BOX_LINE_H * 3) + 3;

}


#endif

/*

****************************************************************************************

proc box code


****************************************************************************************

*/


// assumes view.focus_proc is valid
void draw_proc_box(void)
{

 int i, j;
 float x = proc_box.x1;// + 5;
 float y = proc_box.y1;// + 5;
 float text_x = x + 10;


 if (x >= view.window_x
  || y >= view.window_y
  || x < -PBOX_W
  || y < -300)
   return;

 struct procstruct* pr = view.focus_proc;

// y2 needs to be able to predict the size of the box at this point:
 int y2 = proc_box.y2;


 int tcol = pr->player_index;

 ALLEGRO_COLOR heading_col = colours.team [tcol] [TCOL_BOX_TEXT_BOLD];
 ALLEGRO_COLOR text_col = colours.team [tcol] [TCOL_BOX_TEXT];
 ALLEGRO_COLOR faint_col = colours.team [tcol] [TCOL_BOX_TEXT_FAINT];

  if (proc_box.maximised == 0)
    draw_console_box(x + PBOX_W, y, x, y + PBOX_LINE + 3 - SMALL_BEND_SIZE, colours.team [tcol] [TCOL_BOX_HEADER_FILL],
																					-MAIN_BEND_SIZE, MAIN_BEND_SIZE, -SMALL_BEND_SIZE, SMALL_BEND_SIZE,
																					1);
					else
      draw_console_box(x + PBOX_W, y, x, y + PBOX_LINE + 3, colours.team [tcol] [TCOL_BOX_HEADER_FILL],
																					-MAIN_BEND_SIZE, MAIN_BEND_SIZE, -SMALL_BEND_SIZE, SMALL_BEND_SIZE,
																					0);


// al_draw_filled_rectangle(x, y, x + PBOX_W, y + PBOX_LINE + 3, colours.team [tcol] [TCOL_BOX_HEADER_FILL]);

 al_draw_textf(font[FONT_SQUARE_BOLD].fnt, heading_col, x + 8, y + 3, ALLEGRO_ALIGN_LEFT, "Process %i", pr->index);

// button:
  al_draw_filled_rectangle(proc_box.button_x1, proc_box.button_y1, proc_box.button_x2, proc_box.button_y2, colours.team [tcol] [TCOL_BOX_BUTTON]);
  if (proc_box.button_highlight)
   al_draw_rectangle(proc_box.button_x1 - 1.5, proc_box.button_y1 - 1.5, proc_box.button_x2 + 1.5, proc_box.button_y2 + 1.5, colours.team [tcol] [TCOL_BOX_OUTLINE], 1);
// draw box in button and exit if closed
  if (proc_box.maximised == 0)
  {
   al_draw_textf(font[FONT_SQUARE].fnt, text_col, x + 95, y + 3, ALLEGRO_ALIGN_LEFT, "hp %i(%i) data %i(%i)", pr->hp, pr->hp_max, *(pr->data), *(pr->data_max));
   al_draw_filled_rectangle(proc_box.button_x1 + 1, proc_box.button_y1 + 1, proc_box.button_x2 - 1, proc_box.button_y2 - 1, colours.team [tcol] [TCOL_BOX_OUTLINE]);
//   al_draw_rectangle(x, y, x + PBOX_W, y + PBOX_LINE + 4, colours.team [tcol] [TCOL_BOX_OUTLINE], 1);
   return;
  }
// finish drawing button
  al_draw_filled_rectangle(proc_box.button_x1 + 1, proc_box.button_y1 + 1, proc_box.button_x2 - 1, proc_box.button_y1 + 3, colours.team [tcol] [TCOL_BOX_OUTLINE]);
// now draw rectangle for whole box:
  al_draw_filled_rectangle(x, y + PBOX_LINE + 3, x + PBOX_W, y2, colours.team [tcol] [TCOL_BOX_FILL]);
//  al_draw_filled_rectangle(x, y + PBOX_LINE + 4.5, x + PBOX_W, y2, colours.team [tcol] [TCOL_BOX_FILL]);
//  al_draw_line(x, y + PBOX_LINE + 4, x + PBOX_W - 1, y + PBOX_LINE + 4, colours.team [tcol] [TCOL_BOX_OUTLINE], 1);
//  al_draw_rectangle(x, y, x + PBOX_W, y2, colours.team [tcol] [TCOL_BOX_OUTLINE], 1);

// al_set_target_bitmap(al_get_backbuffer(display));
 al_set_clipping_rectangle(x, y, PBOX_W, y2 - y);

 y += PBOX_LINE + 6;


 al_draw_textf(font[FONT_SQUARE].fnt, text_col, text_x, y, ALLEGRO_ALIGN_LEFT, "hp %i(%i) ex %i", pr->hp, pr->hp_max, pr->execution_count);
 y += PBOX_LINE;
 al_draw_textf(font[FONT_SQUARE].fnt, text_col, text_x, y, ALLEGRO_ALIGN_LEFT, "at (%i, %i) speed %i (%i, %i)",
               al_fixtoi(pr->x),
               al_fixtoi(pr->y),
															al_fixtoi(al_fixhypot(pr->y_speed, pr->x_speed) * 16),
               al_fixtoi(pr->x_speed * 16),
               al_fixtoi(pr->y_speed * 16));
 y += PBOX_LINE;
 al_draw_textf(font[FONT_SQUARE].fnt, text_col, text_x, y, ALLEGRO_ALIGN_LEFT, "angle %i spin %i block (%i, %i)",
               fixed_angle_to_int(pr->angle),
               fixed_angle_to_int(pr->spin),
															pr->x_block,
															pr->y_block);
 y += PBOX_LINE;
 int print_base_cost = pr->base_cost;
/* if (pr->virtual_method != -1
  && pr->method[pr->virtual_method].data [MDATA_PR_VIRTUAL_STATE] > 0)
   print_base_cost = pr->base_cost * 2;*/
 al_draw_textf(font[FONT_SQUARE].fnt, text_col, text_x, y, ALLEGRO_ALIGN_LEFT, "irpt %i(%i) base %i",
               *(pr->irpt),
               *(pr->irpt_max),
//               pr->irpt_use,
               print_base_cost);
//               pr->special_method_penalty);
 y += PBOX_LINE;
 al_draw_textf(font[FONT_SQUARE].fnt, text_col, text_x, y, ALLEGRO_ALIGN_LEFT, "data %i(%i) instr %i(%i) used %i",
               *(pr->data),
               *(pr->data_max),
               pr->instructions_left_after_last_exec,
               pr->instructions_each_execution,
               pr->instructions_each_execution - pr->instructions_left_after_last_exec);
 y += PBOX_LINE;
 al_draw_textf(font[FONT_SQUARE].fnt, text_col, text_x, y, ALLEGRO_ALIGN_LEFT, "size %i mass %i(%i) moment %i",
               pr->size,
               pr->mass,
               pr->shape_str->mass_max,
               pr->moment);
 y += PBOX_LINE;
 al_draw_textf(font[FONT_SQUARE].fnt, text_col, text_x, y, ALLEGRO_ALIGN_LEFT, "base mass %i method mass %i(%i)",
               pr->shape_str->shape_mass,
               pr->mass - pr->shape_str->shape_mass,
               pr->shape_str->mass_max - pr->shape_str->shape_mass);
 y += PBOX_LINE;

 if (pr->group != -1)
 {
  al_draw_textf(font[FONT_SQUARE].fnt, text_col, text_x, y, ALLEGRO_ALIGN_LEFT, "group mass %i moment %i spin %f",
                w.group[pr->group].total_mass,
                w.group[pr->group].moment,
                al_fixtof(w.group[pr->group].spin));//fixed_angle_to_int(w.group[pr->group].spin));
  y += PBOX_LINE;
 }
/*pr->command [5] = -32765;
pr->command [6] = -32765;
pr->command [7] = -32765;
pr->command [8] = -32765;
pr->command [9] = -32765;
pr->command [10] = -32765;
pr->command [11] = -32765;

pr->command [12] = -32765;
pr->command [13] = -32765;
pr->command [14] = -32765;
pr->command [15] = -32765;*/
  al_draw_textf(font[FONT_SQUARE].fnt, text_col, text_x, y, ALLEGRO_ALIGN_LEFT, "commands");
  y += PBOX_LINE;
  al_draw_textf(font[FONT_SQUARE].fnt, faint_col, text_x + 4, y, ALLEGRO_ALIGN_LEFT, "(0.%i 1.%i 2.%i 3.%i 4.%i 5.%i)",
                pr->command [0],
                pr->command [1],
                pr->command [2],
                pr->command [3],
                pr->command [4],
                pr->command [5]);
  y += PBOX_LINE;
  al_draw_textf(font[FONT_SQUARE].fnt, faint_col, text_x + 4, y, ALLEGRO_ALIGN_LEFT, "(6.%i 7.%i 8.%i 9.%i 10.%i 11.%i)",
                pr->command [6],
                pr->command [7],
                pr->command [8],
                pr->command [9],
                pr->command [10],
                pr->command [11]);
  y += PBOX_LINE;
  al_draw_textf(font[FONT_SQUARE].fnt, faint_col, text_x + 4, y, ALLEGRO_ALIGN_LEFT, "(12.%i 13.%i 14.%i 15.%i)",
                pr->command [12],
                pr->command [13],
                pr->command [14],
                pr->command [15]);
/*  for (i = 0; i < 8; i ++)
  {
   al_draw_textf(font, text_col, text_x + 40 + i * 38, y, ALLEGRO_ALIGN_RIGHT, "%i", pr->command [i]);
  }
  y += PBOX_LINE;
  for (i = 0; i < 8; i ++)
  {
   al_draw_textf(font, text_col, text_x + 40 + i * 38, y, ALLEGRO_ALIGN_RIGHT, "%i", pr->command [i + 8]);
  }
  y += PBOX_LINE;*/


/* al_draw_textf(font, base_col [2] [4], x, y, ALLEGRO_ALIGN_LEFT, "Registers");
 y += PBOX_LINE;

 for (i = 0; i < REGISTERS; i ++)
 {
  al_draw_textf(font, text_col, x + i * 20, y, ALLEGRO_ALIGN_RIGHT, "%i", pr->regs.reg [i]);
 }

 y += PBOX_LINE*2;

 al_draw_textf(font, base_col [2] [4], x, y, ALLEGRO_ALIGN_LEFT, "Command");
 y += PBOX_LINE;

 for (i = 0; i < COMMANDS; i ++)
 {
  al_draw_textf(font, text_col, x + i * 30, y, ALLEGRO_ALIGN_RIGHT, "%i", pr->command [i]);
 }*/

 y += PBOX_LINE*2;
 int method_type;

// x += 26;

 al_draw_textf(font[FONT_SQUARE_BOLD].fnt, heading_col, text_x, y, ALLEGRO_ALIGN_LEFT, "Methods");
 text_x += 19;
 y += 3;
 for (i = 0; i < METHODS; i ++)
 {
   y += PBOX_LINE;
   method_type = pr->method [i].type;

   if (method_type == MTYPE_NONE
    || method_type >= MTYPE_END)
   {
    if (method_type == MTYPE_SUB)
    {
     al_draw_textf(font[FONT_SQUARE].fnt, faint_col, text_x + 8, y, ALLEGRO_ALIGN_RIGHT, "%i. ", i);
     al_draw_textf(font[FONT_SQUARE].fnt, faint_col, text_x + 8, y, ALLEGRO_ALIGN_LEFT, "%s", mtype[method_type].name);
//     al_draw_textf(font, faint_col, text_x + 110, y, ALLEGRO_ALIGN_LEFT, "(%i,%i,%i,%i) (%i,%i,%i,%i)", pr->regs.mbank[i * METHOD_SIZE], pr->regs.mbank[i * METHOD_SIZE + 1], pr->regs.mbank[i * METHOD_SIZE + 2], pr->regs.mbank[i * METHOD_SIZE + 3], pr->method[i].data [0], pr->method[i].data [1], pr->method[i].data [2], pr->method[i].data [3]);
     al_draw_textf(font[FONT_SQUARE].fnt, faint_col, text_x + 110, y, ALLEGRO_ALIGN_LEFT, "(%i,%i,%i,%i)", pr->regs.mbank[i * METHOD_SIZE], pr->regs.mbank[i * METHOD_SIZE + 1], pr->regs.mbank[i * METHOD_SIZE + 2], pr->regs.mbank[i * METHOD_SIZE + 3]);
//     al_draw_textf(font, faint_col, x + 180, y, ALLEGRO_ALIGN_LEFT, "(%i,%i,%i,%i)", pr->method[i].data [0], pr->method[i].data [1], pr->method[i].data [2], pr->method[i].data [3]);
     y += PBOX_LINE + 2;
     continue;
    }
    if (method_type == MTYPE_END)
     break;
    al_draw_textf(font[FONT_SQUARE].fnt, faint_col, text_x, y, ALLEGRO_ALIGN_RIGHT, "%i. ", i);
    al_draw_textf(font[FONT_SQUARE].fnt, faint_col, text_x, y, ALLEGRO_ALIGN_LEFT, "%s", mtype[method_type].name);
    switch(method_type)
    {
     case MTYPE_ERROR_INVALID:
     case MTYPE_ERROR_SUB:
      al_draw_textf(font[FONT_SQUARE].fnt, faint_col, text_x + 110, y, ALLEGRO_ALIGN_LEFT, "(type %i)", pr->method[i].data [0]);
      break;
     case MTYPE_ERROR_MASS:
      al_draw_textf(font[FONT_SQUARE].fnt, faint_col, text_x + 110, y, ALLEGRO_ALIGN_LEFT, "(mass %i)", pr->method[i].data [0]);
      break;
     case MTYPE_ERROR_VERTEX:
      al_draw_textf(font[FONT_SQUARE].fnt, faint_col, text_x + 110, y, ALLEGRO_ALIGN_LEFT, "(vertex %i)", pr->method[i].data [0]);
      break;
    }
    y += PBOX_LINE + 2;
    continue;
   }

   if (method_type < 0
    || method_type >= MTYPES)
   {
    al_draw_textf(font[FONT_SQUARE].fnt, faint_col, text_x, y, ALLEGRO_ALIGN_RIGHT, "%i. ", i);
    al_draw_textf(font[FONT_SQUARE].fnt, faint_col, text_x, y, ALLEGRO_ALIGN_LEFT, " (unknown method type: %i)", method_type);
    y += PBOX_LINE + 2;
    continue;
   }

   al_draw_textf(font[FONT_SQUARE].fnt, text_col, text_x, y, ALLEGRO_ALIGN_RIGHT, "%i. ", i);
   al_draw_textf(font[FONT_SQUARE].fnt, text_col, text_x, y, ALLEGRO_ALIGN_LEFT, "%s", mtype[method_type].name);
/*
   if (mtype[method_type].external != MEXT_INTERNAL)
   {
    if (mtype[method_type].external == MEXT_EXTERNAL_ANGLED)
     al_draw_textf(font, text_col, text_x + 50, y, ALLEGRO_ALIGN_LEFT, "vertex %i angle %i", pr->method[i].ex_vertex, fixed_angle_to_int(pr->method[i].ex_angle));
      else
       al_draw_textf(font, text_col, text_x + 50, y, ALLEGRO_ALIGN_LEFT, "vertex %i", pr->method[i].ex_vertex);
   }*/
//     al_draw_textf(font, faint_col, text_x + 110, y, ALLEGRO_ALIGN_LEFT, "(%i,%i,%i,%i)", pr->regs.mbank[i * METHOD_SIZE], pr->regs.mbank[i * METHOD_SIZE + 1], pr->regs.mbank[i * METHOD_SIZE + 2], pr->regs.mbank[i * METHOD_SIZE + 3]);

//     al_draw_textf(font, faint_col, text_x + 70, y, ALLEGRO_ALIGN_LEFT, "(%i,%i,%i,%i) (%i,%i,%i,%i,%i)", pr->regs.mbank[i * METHOD_SIZE], pr->regs.mbank[i * METHOD_SIZE + 1], pr->regs.mbank[i * METHOD_SIZE + 2], pr->regs.mbank[i * METHOD_SIZE + 3], pr->method[i].data [0], pr->method[i].data [1], pr->method[i].data [2], pr->method[i].data [3], pr->method[i].data [4]);
// 70 was 110

   al_draw_textf(font[FONT_SQUARE].fnt, faint_col, x + 110, y, ALLEGRO_ALIGN_LEFT, "(%i,%i,%i,%i)", pr->regs.mbank[i * METHOD_SIZE], pr->regs.mbank[i * METHOD_SIZE + 1], pr->regs.mbank[i * METHOD_SIZE + 2], pr->regs.mbank[i * METHOD_SIZE + 3]);
//   al_draw_textf(font, faint_col, x + 110, y, ALLEGRO_ALIGN_LEFT, "(%i,%i,%i,%i)", -32767, -32767, -32767, -32767);

   y += PBOX_LINE;
   int ext_line_x = text_x;
   char substring [20];

     sprintf(substring, "m %i",
             method_cost.base_cost [mtype[method_type].cost_category]
              + (method_cost.extension_cost [mtype[method_type].cost_category] * (pr->method[i].extension [0] + pr->method[i].extension [1] + pr->method[i].extension [2])));
     al_draw_textf(font[FONT_SQUARE].fnt, faint_col, ext_line_x, y, ALLEGRO_ALIGN_LEFT, "%s", substring);
     ext_line_x += (strlen(substring) * font[FONT_SQUARE].width) + 6;
// TO DO: fix for redundancy!

   switch(method_type)
   {
    case MTYPE_PR_IRPT:
     al_draw_textf(font[FONT_SQUARE].fnt, faint_col, ext_line_x, y, ALLEGRO_ALIGN_LEFT, "%s+%i (max %i)", mtype[method_type].extension_name [0], pr->method[i].extension [0], pr->method[i].data [MDATA_PR_IRPT_MAX]);
     break;
    case MTYPE_PR_ALLOCATE:
     al_draw_textf(font[FONT_SQUARE].fnt, faint_col, ext_line_x, y, ALLEGRO_ALIGN_LEFT, "eff %i vx %i", pr->method[i].data [MDATA_PR_ALLOCATE_EFFICIENCY], pr->method[i].ex_vertex);
     break;
// some method types (e.g. errors) don't get this far - see above

    default:
     for (j = 0; j < mtype[method_type].extension_types; j ++)
     {
      sprintf(substring, "%s+%i", mtype[method_type].extension_name [j], pr->method[i].extension [j]);
      al_draw_textf(font[FONT_SQUARE].fnt, faint_col, ext_line_x, y, ALLEGRO_ALIGN_LEFT, "%s", substring);
      ext_line_x += (strlen(substring) * font[FONT_SQUARE].width) + 6;
     }
     if (mtype[method_type].external == MEXT_EXTERNAL_ANGLED)
     {
      sprintf(substring, "vx %i ang %i", pr->method[i].ex_vertex, fixed_angle_to_int(pr->method[i].ex_angle));
      al_draw_textf(font[FONT_SQUARE].fnt, faint_col, ext_line_x, y, ALLEGRO_ALIGN_LEFT, "%s", substring);
      ext_line_x += (strlen(substring) * font[FONT_SQUARE].width) + 6;
     }
     if (mtype[method_type].external == MEXT_EXTERNAL_NO_ANGLE)
     {
      sprintf(substring, "vx %i", pr->method[i].ex_vertex);
      al_draw_textf(font[FONT_SQUARE].fnt, faint_col, ext_line_x, y, ALLEGRO_ALIGN_LEFT, "%s", substring);
      ext_line_x += (strlen(substring) * font[FONT_SQUARE].width) + 6;
     }
     break;
   }

   y += 2;


 }

// al_set_target_bitmap(al_get_backbuffer(display));
 al_set_clipping_rectangle(0, 0, settings.option [OPTION_WINDOW_W], settings.option [OPTION_WINDOW_H]);

}















void place_proc_box(int x, int y, struct procstruct* pr)
{

 proc_box.x1 = x;
 proc_box.y1 = y;
 proc_box.x2 = x + PBOX_W;
 reset_proc_box_height(pr);

 proc_box.button_x1 = proc_box.x1 + PBOX_W - 4 - CONSOLE_BUTTON_SEPARATION;
 proc_box.button_y1 = proc_box.y1 + 2;
 proc_box.button_x2 = proc_box.button_x1 + CONSOLE_BUTTON_SIZE;
 proc_box.button_y2 = proc_box.button_y1 + CONSOLE_BUTTON_SIZE;

}


// this function calculates size of proc_box based on proc's number of methods.
// pr can be NULL, in which case the size is just the header.
void reset_proc_box_height(struct procstruct* pr)
{

 proc_box.y2 = proc_box.y1 + (PBOX_LINE * 15);
 proc_box.y2 += 9;

 if (pr == NULL)
 {
  proc_box.y2 += proc_box.y1 + (PBOX_LINE*2) + 2;
  return;
 }

 if (pr->group != -1)
  proc_box.y2 += PBOX_LINE;

 int i = 15;

 while(i > 0 &&
       (pr->method[i].type == MTYPE_NONE
     || pr->method[i].type == MTYPE_END))
 {
  i--;
 };

 proc_box.y2 = proc_box.y2 + ((i+1) * ((PBOX_LINE*2) + 2));

}

