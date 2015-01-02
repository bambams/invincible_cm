
#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>


#include <stdio.h>
#include <string.h>

#include "m_config.h"

#include "g_header.h"
#include "m_globvars.h"

#include "g_misc.h"

#include "c_header.h"
#include "e_slider.h"

#include "e_header.h"
#include "e_inter.h"
#include "e_tools.h"
#include "e_help.h"
#include "c_init.h"
#include "i_header.h"
#include "i_input.h"
#include "i_view.h"
#include "m_input.h"
#include "e_log.h"
#include "e_clip.h"
#include "e_files.h"
#include "e_build.h"
#include "e_complete.h"
#include "e_editor.h"

// remove c_prepr.h when finished testing source loading
#include "c_prepr.h"

extern struct fontstruct font [FONTS];

// these queues are declared in g_game.c. They're externed here so that they can be flushed when the editor does something slow.
extern ALLEGRO_EVENT_QUEUE* event_queue; // these queues are initialised in main.c
extern ALLEGRO_EVENT_QUEUE* fps_queue;
extern ALLEGRO_EVENT_QUEUE* control_queue; // in m_input.c

extern struct viewstruct view;
extern struct logstruct mlog;

/*

This file contains functions for displaying source code and editing text.

Keep in mind that it may be good to let the player read and maybe also edit source code while watching a game,
so try to make the display code work alongside the game code if needed.


How is this going to work??

I think there will be a main row of tabs across the very top of the screen:

 main | world | editor

 - main contains options for starting games, quitting etc
 - world is the game currently being run
  - it will also be how players start off games (by filling in boxes with filenames etc)
  - and will also be used for e.g. setting up PBEM games
 - editor displays source and allows editing (I think)
- MAYBE NOT - maybe don't separate world and editor, which means main could just be an icon somewhere.

- It should also be possible to split the screen between world and editor.
 - so write the editor in a way that allows it to be arbitrarily resized and run alongside world execution.

- should put display functions in a separate file
- should probably set up a special bitmap for the editor as it will only need to be updated every few frames at most.
 - draw cursor separately.

- input should be handled by catching clicks and presses

Need to do:

int source_to_editor(struct sourcestruct* src)
 - takes a sourcestruct and loads it into a source_editstruct

int editor_input(
 - call this function at the start of normal input functions if the editor is visible
 - returns 1 if all input captured by editor, 0 if some input may remain
 - captures:
  - all mouse clicks within editor panel
  - holding mouse IF the mouse started being held when pointer within editor
   - doesn't capture holding mouse if the mouse starting being held outside. In this case, game gets holding value and pointer treated as at edge of game panel.
  - if editor is focussed, all key presses.
 - editor becomes focussed if mouse is clicked on it.
 - editor loses focus if mouse is clicked elsewhere. Click is not ignored if within game window.

void display_editor_panel() (in a separate file)
 - draws editor panel bitmap to screen

- void update_editor_panel_bitmap()
 - refreshes or initialises editor panel bitmap if something has been changed.


editor panel looks like:
 - scrollbars at right and bottom
 - File and Edit menus at top
  - or maybe just buttons for various functions - shouldn't need too many.
 - tabs below menus.

*/

struct editorstruct editor;

int source_line_highlight_syntax(struct source_editstruct* se, int src_line, int in_a_comment);
void add_src_highlight(struct source_editstruct* se, int src_line, int stoken_pos, int stoken_end, int stoken_type);
void init_source_editstruct(struct source_editstruct* se);
int source_to_editor(struct sourcestruct* src, int esource);
int mark_as_long_comment(struct source_editstruct* se, int src_line, int comment_start);
void get_cursor_position_from_mouse(struct source_editstruct* se, int x, int y);

void editor_input(void);
void click_in_edit_window(int x, int y);
void	click_in_completion_box(struct source_editstruct* se, int x, int y);
void complete_code(struct source_editstruct* se, int select_line);
void editor_input_keys(void);
void editor_keypress(int key_press);
void editor_special_keypress(int key_press);
void update_source_lines(struct source_editstruct* se, int sline, int lines);
void cursor_etc_key(int key_press);
void window_find_cursor(struct source_editstruct* se);
int insert_empty_lines(struct source_editstruct* se, int before_line, int lines);
void delete_lines(struct source_editstruct* se, int start_line, int lines);
void movement_keys(struct source_editstruct* se);
void select_text(int x, int y);
int is_something_selected(struct source_editstruct* se);
int delete_selection(void);
int just_pressed_ctrl_key(int key);
void open_submenu(int sm);
void close_submenu(void);
void submenu_operation(int sm, int line);
void open_tab(int tab);
void change_tab(int new_tab);
int get_current_source_edit_index(void);

void overwindow_input(void);
void open_overwindow(int ow_type);

void close_editor(void);
struct source_editstruct* get_current_source_edit(void);

#define KEY_DELAY1 7
#define KEY_DELAY2 0

s16b source_edit_bcode [ESOURCES] [BCODE_MAX];



struct submenustruct submenu [SUBMENUS] =
{

 { // submenu 0
  SUBMENU_FILE_END,
  { // lines
   {"New", "", HELP_SUBMENU_NEW},
   {"Open", "", HELP_SUBMENU_OPEN},
   {"Save", "", HELP_SUBMENU_SAVE},
   {"Save as", "", HELP_SUBMENU_SAVE_AS},
   {"Close", "", HELP_SUBMENU_CLOSE_FILE},
//   {"Quit", ""},
  }
 },
 {
  SUBMENU_EDIT_END,
  {
   {"Undo", "Ctrl-Z", HELP_SUBMENU_UNDO},
   {"Redo", "Ctrl-Y", HELP_SUBMENU_REDO},
   {"Cut", "Ctrl-X", HELP_SUBMENU_CUT},
   {"Copy", "Ctrl-C", HELP_SUBMENU_COPY},
   {"Paste", "Ctrl-V", HELP_SUBMENU_PASTE},
   {"Clear", "", HELP_SUBMENU_CLEAR},
  }
 },
 {
  SUBMENU_SEARCH_END,
  {
   {"Find", "Ctrl-F", HELP_SUBMENU_FIND},
   {"Find next", "F3", HELP_SUBMENU_FIND_NEXT}
  }
 },
 {
  SUBMENU_COMPILE_END,
  {
   {"Test build", "F9", HELP_SUBMENU_TEST_BUILD},
   {"Build bcode", "", HELP_SUBMENU_BUILD_BCODE},
   {"Build asm", "", HELP_SUBMENU_BUILD_ASM},
   {"Crunched asm", "", HELP_SUBMENU_CRUNCH_ASM},
   {"Convert bcode", "", HELP_SUBMENU_CONVERT_BCODE},
   {"Import bcode", "", HELP_SUBMENU_IMPORT_BCODE}
  }
 }

};

// call at startup
// should be called before init_templates as some properties of the template window are derived from the editor window
void init_editor(void)
{

 editor.panel_x = settings.editor_x_split;
 editor.panel_y = 0;
 editor.panel_w = settings.option [OPTION_WINDOW_W] - settings.editor_x_split;
 editor.panel_h = settings.option [OPTION_WINDOW_H];
 editor.key_delay = 0;
 editor.selecting = 0;
 editor.cursor_flash = CURSOR_FLASH_MAX;
// editor.first_press = 1;
 editor.submenu_open = -1;

 int i;

 for (i = 0; i < ESOURCES; i ++)
 {
  editor.source_edit[i].active = 0;
  editor.source_edit[i].bcode.op = source_edit_bcode [i];
  editor.source_edit[i].bcode.bcode_size = BCODE_MAX;
  editor.source_edit[i].bcode.from_a_file = 0;
  editor.tab_index [i] = -1;
  editor.tab_type [i] = TAB_TYPE_NONE;
  editor.tab_name [i] [0] = '\0';
  editor.tab_name_unsaved [i] [0] = '\0';
 }

 editor.tab_highlight = -1;
 editor.current_tab = -1;
 editor.overwindow_type = OVERWINDOW_TYPE_NONE;

 editor.search_string [0] = '\0';

 editor.clipboard [0] = '\0';

 init_editor_display(editor.panel_w, editor.panel_h);
 init_log(editor.edit_window_w, LOG_WINDOW_H);

 init_undo();
 init_editor_files();

 completion.list_size = 0; // initialise code completion box

 init_code_completion(); // in e_complete.c

}

void init_source_editstruct(struct source_editstruct* se)
{

 strcpy(se->src_file_name, "init name"); // should never see these; they should always be overwritten by the actual name/path (or a provisional one) beforehand
 strcpy(se->src_file_path, "init path");

 se->cursor_line = 0;
 se->cursor_pos = 0;
 se->cursor_base = 0;
 se->window_line = 0;
 se->window_pos = 0;
 se->selected = 0;
 se->type = SOURCE_EDIT_TYPE_SOURCE;
 se->saved = 0;
 se->from_a_file = 0;

 int i;

 for (i = 0; i < SOURCE_TEXT_LINES; i ++)
 {
  strcpy(se->text [i], "");
  se->line_index [i] = i; // may change through editing
  se->comment_line [i] = 0;
 }


}

// currently this function cannot fail
int source_to_editor(struct sourcestruct* src, int esource)
{
 int i;

 struct source_editstruct* se = &editor.source_edit[esource];

 init_source_editstruct(se);

 se->active = 1;
 strcpy(se->src_file_name, src->src_file_name);
 strcpy(se->src_file_path, src->src_file_path);
// se->from_a_file = 1;
// se->saved = 1; // indicates that the source_edit matches the file on disk

 int in_a_comment = 0;

 for (i = 0; i < SOURCE_TEXT_LINES; i ++)
 {
  strcpy(se->text [i], src->text [i]);
  se->line_index [i] = i; // may change through editing
//  se->comment_line [i] = 0;
  in_a_comment = source_line_highlight_syntax(se, i, in_a_comment);
// if this code is changed, see corresponding code in convert_bcode_to_source_tab() in e_build.c
 }

// wait_for_space();

 return 1;

}


// converts se to sourcestruct format
// carries over name and path of file (or "untitled" if no name/file)
// returns 1 on success, 0 on failure
int source_edit_to_source(struct sourcestruct* src, struct source_editstruct* se)
{

 int i;

 src->from_a_file = se->from_a_file;

 if (se->from_a_file)
 {
  strcpy(src->src_file_name, se->src_file_name);
  strcpy(src->src_file_path, se->src_file_path);

 }
  else
  {
   strcpy(src->src_file_name, "(unnamed)");
   strcpy(src->src_file_path, "(unnamed)");
  }

 for (i = 0; i < SOURCE_TEXT_LINES; i ++)
 {
  strcpy(src->text [i], se->text [se->line_index [i]]);
 }

 return 1;

}


// currently this function cannot fail
int binary_to_editor(struct bcodestruct* bcode, int esource)
{
 int i;

 struct source_editstruct* se = &editor.source_edit[esource];

 init_source_editstruct(se);

 se->active = 1;
 se->type = SOURCE_EDIT_TYPE_BINARY; // init_source_editstruct sets type by default to _SOURCE
 strcpy(se->src_file_name, bcode->src_file_name);
 strcpy(se->src_file_path, bcode->src_file_path);
 se->from_a_file = 1;
 se->saved = 1; // indicates that the source_edit matches the file on disk

 fprintf(stdout, "bcode size %i (static %i) se bcode size %i (static %i) name [%s] path [%s]", bcode->bcode_size, bcode->static_length,
									se->bcode.bcode_size, se->bcode.static_length, se->src_file_name, se->src_file_path);

 for (i = 0; i < bcode->bcode_size; i ++)
 {
  se->bcode.op[i] = bcode->op[i];
 }

 se->bcode.static_length = bcode->static_length;

 return 1;

}


// returns 1 if the line ends within a multi-line comment, 0 otherwise
// src_line is position in text array, not line index array
int source_line_highlight_syntax(struct source_editstruct* se, int src_line, int in_a_comment)
{

// char stoken_string [SOURCE_TEXT_LINE_LENGTH];
 char read_char;
 int char_type;
 int stoken_type;

 int i = 0;
 int finished_stoken = 0;
 int stoken_start;
 char stoken_word [SOURCE_TEXT_LINE_LENGTH];
 int j;

 int preprocessor_line = 0; // if 1, this forces anything on the line (other than a comment) to be preprocessor colour

 se->comment_line [src_line] = 0; // may be set to 1 later

 if (in_a_comment) // if already in a multi-line comment, mark this line as a comment as well (until */ is found)
 {
  se->comment_line [src_line] = 1;
  i = mark_as_long_comment(se, src_line, 0); // mark_as_long_comment returns the position in the source line in which the comment ends, or -1 if it doesn't end on this line
  if (i == -1)
   return 1;
  if (se->text [src_line] [i] == '\0')
   return 0; // comment must have finished immediately before end of line
  in_a_comment = 0;
 }
  else // no need to check for preprocessor directives if line is in a comment
  {
   if (se->text [src_line] [0] == '#')
    preprocessor_line = 1;
  }


 while(TRUE)
 {
// now read in the next token.
// determine the token type from the first character:
  do
  {
   read_char = se->text [src_line] [i];
   char_type = get_source_char_type(read_char);
   i++;
  } while (char_type == SCHAR_SPACE);

  switch(char_type)
  {
   case SCHAR_NULL:
   case SCHAR_ERROR:
   default:
    return in_a_comment; // finished. Ignore errors.
   case SCHAR_LETTER:
    stoken_type = STOKEN_TYPE_WORD; break;
   case SCHAR_NUMBER:
    stoken_type = STOKEN_TYPE_NUMBER; break;
   case SCHAR_OPERATOR:
    if (read_char == '/')
    {
     if (se->text [src_line] [i] == '/') // should be able to assume that the next character is within bounds (may be null terminator)
     {
// this code matches the way the compiler ignores */ if within a // comment (which I don't think is standard. oh well)
      add_src_highlight(se, src_line, i - 1, strlen(se->text [src_line]), STOKEN_TYPE_COMMENT);
      return in_a_comment; // should this be zero?
     }
     if (se->text [src_line] [i] == '*') // now check for beginning of long comment
     {
      i = mark_as_long_comment(se, src_line, i - 1);
      if (i == -1)
       return 1; // comment continues past this line
      in_a_comment = 0;  // comment must have finished on this line; now read the next stoken (after the end of the comment)
      continue;
     }
    }
    stoken_type = STOKEN_TYPE_OPERATOR; break;
   case SCHAR_QUOTE:
    stoken_type = STOKEN_TYPE_STRING; break;
   case SCHAR_PUNCTUATION:
    stoken_type = STOKEN_TYPE_PUNCTUATION; break;
  }

  stoken_start = i - 1;

// now read from the source line until we find a character that can't form part of this stoken (or that is of a different type)
  while(TRUE)
  {
   read_char = se->text [src_line] [i];
   char_type = get_source_char_type(read_char);
   finished_stoken = 0;
   if (stoken_type != STOKEN_TYPE_STRING)
   {
    switch(char_type)
    {
     case SCHAR_NULL:
     case SCHAR_ERROR: // not sure about this - if an unrecognised character is found, syntax highlighting is broken for the rest of the line
     case SCHAR_SPACE:
      finished_stoken = 1;
      break;
     case SCHAR_LETTER:
      if (stoken_type != STOKEN_TYPE_WORD)
       finished_stoken = 1;
      break;
     case SCHAR_NUMBER:
      if (stoken_type != STOKEN_TYPE_WORD
       && stoken_type != STOKEN_TYPE_NUMBER)
        finished_stoken = 1;
      break;
     case SCHAR_OPERATOR:
      if (stoken_type != STOKEN_TYPE_OPERATOR)
       finished_stoken = 1;
      break;
     case SCHAR_PUNCTUATION:
      if (stoken_type != STOKEN_TYPE_PUNCTUATION)
       finished_stoken = 1;
      break;
     case SCHAR_QUOTE:
      finished_stoken = 1;
      break;
     }
   }
    else // in a string, so treat most characters as part of the string
    {
     switch(char_type)
     {
      case SCHAR_NULL:
      case SCHAR_ERROR: // not sure about this - if an unrecognised character is found, syntax highlighting is broken for the rest of the line
       finished_stoken = 1;
       break;
      case SCHAR_QUOTE:
       i++; // to read in closing quote
       finished_stoken = 1;
       break;
      }
    }

   if (finished_stoken)
   {
// now go back to the start of the current stoken and colour it appropriately:
    if (stoken_type == STOKEN_TYPE_WORD)
    {
// if stoken is a word, need to check whether it's a keyword:
     for (j = stoken_start; j < i; j ++)
     {
      stoken_word [j - stoken_start] = se->text [src_line] [j];
     }
     stoken_word [j - stoken_start] = '\0';
     stoken_type = check_word_type(stoken_word);
    }
    if (preprocessor_line)
     stoken_type = STOKEN_TYPE_PREPROCESSOR;
    add_src_highlight(se, src_line, stoken_start, i, stoken_type);
    break;
   }

   i++;
  };

 };

}

// returns the position in the source line immediately after the end of the comment (i.e. after the closing */)
// returns -1 if the comment doesn't end on this line
// src_line is position in text array, not line_index array
int mark_as_long_comment(struct source_editstruct* se, int src_line, int comment_start)
{

 int i = comment_start;

 while(TRUE)
 {
  if (se->text [src_line] [i] == '\0')
   return -1;
  if (se->text [src_line] [i] == '*'
   && se->text [src_line] [i + 1] == '/')
  {
   se->source_colour [src_line] [i] = STOKEN_TYPE_COMMENT;
   se->source_colour [src_line] [i + 1] = STOKEN_TYPE_COMMENT;
   return i + 2; // note that i + 2 may be the null terminator
  }
  se->source_colour [src_line] [i] = STOKEN_TYPE_COMMENT;
  i++;
 };

}


int get_source_char_type(char read_source)
{

 if (read_source == '\0')
  return SCHAR_NULL;

 if ((read_source >= 'a' && read_source <= 'z')
  || (read_source >= 'A' && read_source <= 'Z')
  || read_source == '_')
   return SCHAR_LETTER;

 if (read_source >= '0' && read_source <= '9')
   return SCHAR_NUMBER;

 switch(read_source)
 {
  case '(':
  case ')':
  case '{':
  case '}':
  case '[':
  case ']':
  case '\'':
   return SCHAR_PUNCTUATION;

  case ';':
  case ':':
  case ',':
  case '.':
  case '~':
  case '+':
  case '-':
  case '*':
  case '/':
  case '=':
  case '<':
  case '>':
  case '&':
  case '|':
  case '^':
  case '%':
  case '!':
// the following aren't operators, but for now we'll treat them as operators for this purpose:
  case '\\':
  case '@':
  case '#':
  case '$':
  case '?':
  case '`':
   return SCHAR_OPERATOR;

  case '"':
   return SCHAR_QUOTE;

  case ' ':
   return SCHAR_SPACE;

  default: return SCHAR_ERROR; // invalid char

 }

 return SCHAR_ERROR; // invalid char

}



// src_line is position in text array, not line_index array
void add_src_highlight(struct source_editstruct* se, int src_line, int stoken_pos, int stoken_end, int stoken_type)
{


 while(stoken_pos <= stoken_end)
 {
  se->source_colour [src_line] [stoken_pos] = stoken_type;
  stoken_pos ++;
 };


}


// this function is called from game_loop in game.c when the editor is up
void run_editor(void)
{

//  if (game.keyboard_capture == INPUT_EDITOR)
  editor_input();

  editor.cursor_flash --;
  if (editor.cursor_flash <= 0)
   editor.cursor_flash = CURSOR_FLASH_MAX;

  draw_edit_bmp();

/*
 al_flush_event_queue(event_queue);
 ALLEGRO_EVENT ev;

 while(TRUE)
 {

  editor_input();
  draw_edit_bmp();

  al_wait_for_event(event_queue, &ev);
  al_flush_event_queue(fps_queue);

 }*/

}

/*

I think I've been taking the wrong approach with the editor.
It would be better to have the editor available from the game anytime (including during game setup)
Put a little icon up at the top right of the screen (allow client program to turn it off) - clicking on it pulls the editor over the right half of the screen (and re-minimises the editor when finished).
 - this should probably resize the game screen, and use a flag of some kind to tell the client program that it's been resized.
With 120 character limit, the editor will take up 120*6 = 720, which leaves some space for the game
 - maybe allow the client to force the editor to use the full screen and suspend the game?
There will be a variable in gamestruct which determines which side captures input.
 - switch between them by clicking on one side or the other.



Also, pausing. Should be two types:
 1. pause - activated by the client program, which will keep running (to allow selection etc)
 2. suspend - activated by the system; client program stops running as well.

*/


void editor_input(void)
{

 struct source_editstruct* se = get_current_source_edit();
// NOTE: se may be NULL!


 if (settings.keyboard_capture == INPUT_EDITOR)
  editor_input_keys();

// overwindow intercepts all mouse input:
 if (editor.overwindow_type != OVERWINDOW_TYPE_NONE)
 {
  overwindow_input();
  return;
// TO DO: presently this prevents keyboard_capture being shifted back to the world. Is this correct?
 }

 int lm_button_status = ex_control.mb_press [0]; // this value can be changed if the button status is captured (e.g. because user clicked on a submenu)

 editor.submenu_name_highlight = -1; // is updated below if needed

// check for the mouse pointer being in the game window:
 if (ex_control.mouse_x_pixels < settings.editor_x_split)
 {
  if (settings.keyboard_capture == INPUT_EDITOR
   && lm_button_status == BUTTON_JUST_PRESSED)
  {
   initialise_control();
   settings.keyboard_capture = INPUT_WORLD;
   editor.selecting = 0;
   completion.list_size = 0;
  }
// TO DO: needs work - see also equivalent in t_template.c and i_sysmenu.c
  return;
 }

// number of lines scrolled by moving the mousewheel:
#define EDITOR_MOUSEWHEEL_SPEED 8

// check for mousewheel movement:
 if (se != NULL)
 {
  if (completion.list_size > 0
			&& ex_control.mouse_x_pixels >= completion.box_x
			&& ex_control.mouse_x_pixels <= completion.box_x2
			&& ex_control.mouse_y_pixels >= completion.box_y
			&& ex_control.mouse_y_pixels <= completion.box_y2)
		{
    if (ex_control.mousewheel_change == 1)
					scroll_completion_box_down(4);
    if (ex_control.mousewheel_change == -1)
					scroll_completion_box_up(4);
		}
		 else
			{
    if (ex_control.mousewheel_change == 1)
    {
     se->window_line += EDITOR_MOUSEWHEEL_SPEED;
     if (se->window_line >= SOURCE_TEXT_LINES)
      se->window_line = SOURCE_TEXT_LINES - 1;
     slider_moved_to_value(&editor.scrollbar_v, se->window_line);
    }
    if (ex_control.mousewheel_change == -1)
    {
     se->window_line -= EDITOR_MOUSEWHEEL_SPEED;
     if (se->window_line < 0)
      se->window_line = 0;
     slider_moved_to_value(&editor.scrollbar_v, se->window_line);
    }
			}
 }



// now check for the user interacting with the scrollbars
 if (editor.current_tab != -1
  && editor.tab_type [editor.current_tab] == TAB_TYPE_SOURCE) // no scrollbars if file is binary
 {
// editor scrollbar
  run_slider(&editor.scrollbar_v, 0, 0);
  run_slider(&editor.scrollbar_h, 0, 0);
//  editor.source_edit[editor.current_source_edit].window_line = editor.scrollbar_v.value;
 }
// message log scrollbar:
 run_slider(&mlog.scrollbar_v, 0, 0);

/*
// check for the user clicking the button to close the editor
 if (lm_button_status == BUTTON_JUST_PRESSED
  && ex_control.mouse_x >= game.editor_mode_button_x
  && ex_control.mouse_x <= game.editor_mode_button_x + EDITOR_MODE_BUTTON_SIZE
  && ex_control.mouse_y >= game.editor_mode_button_y
  && ex_control.mouse_y <= game.editor_mode_button_y + EDITOR_MODE_BUTTON_SIZE)
 {
   close_editor();
   lm_button_status = BUTTON_HELD; // to avoid the editor being closed again before get_ex_control can be called again
   return;
 }*/

 int mouse_x = ex_control.mouse_x_pixels - editor.panel_x;
 int mouse_y = ex_control.mouse_y_pixels - editor.panel_y;

// check for user clicking on submenu:
 if (editor.submenu_open != -1)
 {
// check for mouse being inside submenu box:
  if (mouse_x >= editor.submenu_x
   && mouse_x <= editor.submenu_x + editor.submenu_w
   && mouse_y >= editor.submenu_y
   && mouse_y <= editor.submenu_y + editor.submenu_h)
  {
   editor.submenu_highlight = (mouse_y - editor.submenu_y) / SUBMENU_LINE_HEIGHT;
   if (editor.submenu_highlight < 0
    || editor.submenu_highlight >= submenu[editor.submenu_open].lines)
     editor.submenu_highlight = -1;
// choose a submenu item by releasing the mouse button:
   if (lm_button_status == BUTTON_JUST_RELEASED)
   {
    submenu_operation(editor.submenu_open, editor.submenu_highlight);
    close_submenu();
    completion.list_size = 0;
   }
   if (ex_control.mb_press [1] == BUTTON_JUST_PRESSED
				&& editor.submenu_highlight != -1)
			{
				print_help(submenu[editor.submenu_open].line[editor.submenu_highlight].help_type);
			 completion.list_size = 0;
			}

/*   if (editor.submenu_highlight < 0)
    editor.submenu_highlight = 0;
     else
     {
      if (editor.submenu_highlight >= submenu[editor.submenu_open].lines)
       editor.submenu_highlight = submenu[editor.submenu_open].lines;
     }*/

   lm_button_status = BUTTON_NOT_PRESSED; // captures mouse button press if mouse over submenu
  }
   else
   {
    editor.submenu_highlight = -1;
   }
// remember to add code to close submenu if user clicks elsewhere
 }

 int mouse_in_completion_box = 0;

  if (completion.list_size > 0
			&& ex_control.mouse_x_pixels >= completion.box_x
			&& ex_control.mouse_x_pixels <= completion.box_x2
			&& ex_control.mouse_y_pixels >= completion.box_y
			&& ex_control.mouse_y_pixels <= completion.box_y2)
		{
			mouse_in_completion_box = 1;
  	completion.select_line = completion.window_pos + ((ex_control.mouse_y_pixels - completion.box_y - COMPLETION_BOX_LINE_Y_OFFSET) / COMPLETION_BOX_LINE_H);
  	if (completion.select_line < 0)
				completion.select_line = 0;
  	if (completion.select_line >= completion.list_size)
				completion.select_line = completion.list_size - 1;
		}

// now check for the user clicking in the editing window:
 if (lm_button_status == BUTTON_JUST_PRESSED)
 {
  if (completion.list_size > 0
			&& mouse_in_completion_box == 1)
		{
   close_submenu();
			click_in_completion_box(se, ex_control.mouse_x_pixels - completion.box_x, ex_control.mouse_y_pixels - completion.box_y);
		}
		 else
			{
				completion.list_size = 0;
    if (mouse_x >= EDIT_WINDOW_X
     && mouse_y >= EDIT_WINDOW_Y
     && mouse_x <= EDIT_WINDOW_X + editor.edit_window_w
     && mouse_y <= EDIT_WINDOW_Y + editor.edit_window_h)
    {
     close_submenu();
     click_in_edit_window(mouse_x - EDIT_WINDOW_X, mouse_y - EDIT_WINDOW_Y);
    }
			}
 }

// menu bar (File Edit Compile etc.)
    if (mouse_y >= EMENU_BAR_Y
     && mouse_y <= EMENU_BAR_Y + EMENU_BAR_H)
    {
     int submenu_chosen = (mouse_x - EMENU_BAR_X) / EMENU_BAR_NAME_WIDTH;
     if (lm_button_status == BUTTON_JUST_PRESSED)
     {
      if (submenu_chosen >= 0 && submenu_chosen < SUBMENUS)
      {
       open_submenu(submenu_chosen);
      }
     }
      else
      {
       if (submenu_chosen >= 0 && submenu_chosen < SUBMENUS)
       {
        editor.submenu_name_highlight = submenu_chosen;
        if (editor.submenu_open != -1
         && editor.submenu_open != submenu_chosen)
          open_submenu(submenu_chosen);
       }
      }
    }


   editor.tab_highlight = -1;

// menu bar (File Edit Compile etc.)
    if (mouse_y >= SOURCE_TAB_Y
     && mouse_y <= SOURCE_TAB_Y + SOURCE_TAB_H)
    {
     int tab_chosen = (mouse_x - SOURCE_TAB_X) / SOURCE_TAB_W;
     if (lm_button_status == BUTTON_JUST_PRESSED)
     {
      if (tab_chosen >= 0
       && tab_chosen < ESOURCES
       && editor.tab_index [tab_chosen] != -1)
      {
       open_tab(tab_chosen);
      }
     }
      else
      {
       if (tab_chosen >= 0 && tab_chosen < ESOURCES)
       {
        if (editor.tab_index [tab_chosen] != -1)
         editor.tab_highlight = tab_chosen;
       }
      }
    }


 if (editor.selecting == 1
  && lm_button_status == BUTTON_HELD
  && mouse_x >= EDIT_WINDOW_X
  && mouse_y >= EDIT_WINDOW_Y
  && mouse_x <= EDIT_WINDOW_X + editor.edit_window_w
  && mouse_y <= EDIT_WINDOW_Y + editor.edit_window_h)
 {
  select_text(mouse_x - EDIT_WINDOW_X, mouse_y - EDIT_WINDOW_Y);
 }

 if (lm_button_status <= 0)
 {
  // if mouse button not being pressed, stop selecting:
   editor.selecting = 0; // but doesn't remove the selection values from the source_editstruct
 }



}


void overwindow_input(void)
{

 int lm_button_status = ex_control.mb_press [0];

 int mouse_x = ex_control.mouse_x_pixels - editor.panel_x;
 int mouse_y = ex_control.mouse_y_pixels - editor.panel_y;
 int i;

 struct source_editstruct* se;

 if (lm_button_status == BUTTON_JUST_PRESSED
  && mouse_x >= editor.overwindow_x
  && mouse_x <= editor.overwindow_x + editor.overwindow_w
  && mouse_y >= editor.overwindow_y
  && mouse_y <= editor.overwindow_y + editor.overwindow_h)
 {
  for (i = 0; i < editor.overwindow_buttons; i ++)
  {
   if (mouse_x >= editor.overwindow_button_x [i]
    && mouse_x <= editor.overwindow_button_x [i] + OVERWINDOW_BUTTON_W
    && mouse_y >= editor.overwindow_button_y [i]
    && mouse_y <= editor.overwindow_button_y [i] + OVERWINDOW_BUTTON_H)
   {
    switch(editor.overwindow_button_type [i])
    {
     case OVERWINDOW_BUTTON_TYPE_NO:
      editor.overwindow_type = OVERWINDOW_TYPE_NONE;
      break;
     case OVERWINDOW_BUTTON_TYPE_CLOSE_TAB:
      close_source_tab(editor.current_tab, 1);
      editor.overwindow_type = OVERWINDOW_TYPE_NONE;
      break;
     case OVERWINDOW_BUTTON_TYPE_FIND:
      editor.overwindow_type = OVERWINDOW_TYPE_NONE;
      se = get_current_source_edit();
      if (se != NULL
       && se->type == SOURCE_EDIT_TYPE_SOURCE)
      {
       find_next();
       window_find_cursor(se);
      }
// should match code for pressing enter just below
      return;
     case OVERWINDOW_BUTTON_TYPE_CANCEL_FIND:
      editor.overwindow_type = OVERWINDOW_TYPE_NONE;
      break;

    }
   }
  }

 }

 int input_result;

 switch(editor.overwindow_type)
 {
  case OVERWINDOW_TYPE_FIND:
   input_result = accept_text_box_input(TEXT_BOX_EDITOR_FIND);
   if (input_result == 1) // enter pressed
   {
    editor.overwindow_type = OVERWINDOW_TYPE_NONE;
    se = get_current_source_edit();
    if (se != NULL
     && se->type == SOURCE_EDIT_TYPE_SOURCE)
    {
     find_next();
     window_find_cursor(se);
    }
    if (ex_control.key_press [ALLEGRO_KEY_ENTER] == BUTTON_JUST_PRESSED)
    {
     ex_control.key_press [ALLEGRO_KEY_ENTER] = BUTTON_HELD;
     ex_control.key_being_pressed = ALLEGRO_KEY_ENTER;
    }
    if (ex_control.key_press [ALLEGRO_KEY_PAD_ENTER] == BUTTON_JUST_PRESSED)
    {
     ex_control.key_press [ALLEGRO_KEY_PAD_ENTER] = BUTTON_HELD;
     ex_control.key_being_pressed = ALLEGRO_KEY_PAD_ENTER;
    }
    editor.key_delay = 30;
// should match code for button press just above
   }
   break;
 }



}


void open_overwindow(int ow_type)
{

 editor.overwindow_type = ow_type;
 editor.overwindow_w = 200;
 editor.overwindow_h = 100;
 editor.overwindow_x = (editor.panel_w / 2) - (editor.overwindow_w / 2);
 editor.overwindow_y = (editor.panel_h / 2) - (editor.overwindow_h / 2);

 switch(ow_type)
 {
  case OVERWINDOW_TYPE_CLOSE:
   editor.overwindow_buttons = 2;
   editor.overwindow_button_type [0] = OVERWINDOW_BUTTON_TYPE_CLOSE_TAB;
   editor.overwindow_button_type [1] = OVERWINDOW_BUTTON_TYPE_NO;
   editor.overwindow_button_x [0] = editor.overwindow_x + 20;
   editor.overwindow_button_y [0] = editor.overwindow_y + editor.overwindow_h - 20 - OVERWINDOW_BUTTON_H;
   editor.overwindow_button_x [1] = editor.overwindow_x + editor.overwindow_w - 30 - OVERWINDOW_BUTTON_W;
   editor.overwindow_button_y [1] = editor.overwindow_y + editor.overwindow_h - 20 - OVERWINDOW_BUTTON_H;
   break;
  case OVERWINDOW_TYPE_FIND:
   editor.overwindow_buttons = 2;
   editor.overwindow_button_type [0] = OVERWINDOW_BUTTON_TYPE_FIND;
   editor.overwindow_button_type [1] = OVERWINDOW_BUTTON_TYPE_CANCEL_FIND;
   editor.overwindow_button_x [0] = editor.overwindow_x + 20;
   editor.overwindow_button_y [0] = editor.overwindow_y + editor.overwindow_h - 20 - OVERWINDOW_BUTTON_H;
   editor.overwindow_button_x [1] = editor.overwindow_x + editor.overwindow_w - 30 - OVERWINDOW_BUTTON_W;
   editor.overwindow_button_y [1] = editor.overwindow_y + editor.overwindow_h - 20 - OVERWINDOW_BUTTON_H;
   start_text_input_box(TEXT_BOX_EDITOR_FIND, editor.search_string, SEARCH_STRING_LENGTH);
   break;


 }

}

void close_overwindow(void)
{
 editor.overwindow_type = OVERWINDOW_TYPE_NONE;
}

// assumes sm is valid
void open_submenu(int sm)
{

  editor.submenu_open = sm;
  editor.submenu_highlight = -1;
  editor.submenu_x = EMENU_BAR_X + (EMENU_BAR_NAME_WIDTH * sm);
  editor.submenu_y = EMENU_BAR_Y + EMENU_BAR_H;
  editor.submenu_w = SUBMENU_WIDTH;
  editor.submenu_h = submenu[sm].lines * SUBMENU_LINE_HEIGHT;
}

// can't assume this will be called any time a submenu is closed (e.g. submenu_open alone is used to switch between submenus)
void close_submenu(void)
{

 editor.submenu_open = -1;

}

void submenu_operation(int sm, int line)
{

 struct source_editstruct* se;

 switch(sm)
 {
  case SUBMENU_FILE:
   switch(line)
   {
    case SUBMENU_FILE_NEW:
     new_empty_source_tab(); // may fail, but just ignore failure (an error will have been written to the log)
     break;
    case SUBMENU_FILE_OPEN:
     open_file_into_free_tab(); // may fail, but just ignore failure
     break;
    case SUBMENU_FILE_SAVE:
     save_current_file();
     break;
    case SUBMENU_FILE_SAVE_AS:
     save_as();
     break;
    case SUBMENU_FILE_CLOSE:
     close_source_tab(editor.current_tab, 0);
     break;
   }
   break;
  case SUBMENU_EDIT:
   switch(line)
   {
// if any edit operations are changed, may need to update the code for keyboard shortcuts too (in editor_input_keys())
    case SUBMENU_EDIT_UNDO:
     call_undo();
     break;
    case SUBMENU_EDIT_REDO:
     call_redo();
     break;
    case SUBMENU_EDIT_COPY:
     copy_selection();
     break;

    case SUBMENU_EDIT_CUT:
     copy_selection();
     delete_selection(); // ignore return value
     break;
    case SUBMENU_EDIT_PASTE:
     paste_clipboard();
     break;
   }
   break;
  case SUBMENU_SEARCH:
   switch(line)
   {
    case SUBMENU_SEARCH_FIND:
     open_overwindow(OVERWINDOW_TYPE_FIND);
     break;
    case SUBMENU_SEARCH_FIND_NEXT:
     se = get_current_source_edit();
     if (se != NULL
      && se->type == SOURCE_EDIT_TYPE_SOURCE)
     {
      find_next();
      window_find_cursor(se);
     }
     break;
   }
   break;
  case SUBMENU_COMPILE:
   switch(line)
   {
    case SUBMENU_COMPILE_TEST:
     build_source_tab(COMPILE_OUTPUT_TEST);
     break;
    case SUBMENU_COMPILE_TO_ASM:
     build_source_tab(COMPILE_OUTPUT_ASM_TAB);
     break;
    case SUBMENU_COMPILE_TO_ASM_CRUNCH:
     build_source_tab(COMPILE_OUTPUT_ASM_TAB_CRUNCH);
     break;
    case SUBMENU_COMPILE_TO_BCODE:
     build_source_tab(COMPILE_OUTPUT_BCODE_TAB);
     break;
    case SUBMENU_COMPILE_CONVERT_BCODE:
     convert_bcode_to_source_tab();
     break;
    case SUBMENU_COMPILE_IMPORT_BCODE:
     import_proc_bcode_to_bcode_tab();
     break;
   }
   break;
 }

}

// assumes tab is valid
void open_tab(int tab)
{

 if (editor.current_tab == tab)
  return;

 editor.current_tab = tab;

 change_tab(tab);


}


void click_in_edit_window(int x, int y)
{


 struct source_editstruct* se = get_current_source_edit();

 if (se == NULL
  || se->type != SOURCE_EDIT_TYPE_SOURCE)
  return;

// check for click in code completion box here:

 completion.list_size = 0; // remove code completion box


 get_cursor_position_from_mouse(se, x, y);
 editor.cursor_flash = CURSOR_FLASH_MAX;

/* se->cursor_line = se->window_line + ((y + 3) / EDIT_LINE_H); // 3 is fine-tuning

 if (se->cursor_line >= SOURCE_TEXT_LINES)
  se->cursor_line = SOURCE_TEXT_LINES - 1;
 if (se->cursor_line < 0)
  se->cursor_line = 0;

 se->cursor_pos = se->window_pos - 1 + ((x + SOURCE_WINDOW_MARGIN) / editor.text_width);

 if (se->cursor_pos > strlen(se->text [se->line_index [se->cursor_line]]))
  se->cursor_pos = strlen(se->text [se->line_index [se->cursor_line]]);

 se->cursor_base = se->cursor_pos;*/

 editor.selecting = 1;
 se->select_fix_line = se->cursor_line;
 se->select_fix_pos = se->cursor_pos;
 se->select_free_line = se->cursor_line;
 se->select_free_pos = se->cursor_pos;

//fprintf(stdout, "\nClicked: w_line %i w_pos %i (%i, %i) line %i pos %i ", se->window_line, se->window_pos, x, y, se->cursor_line, se->cursor_pos );


// now put the cursor at the end of the line by searching for null terminator from the start
/* int i = 0;
 while(i < se->cursor_pos
    && se->text [se->line_index [se->cursor_line]] [i] != '\0')
 {
  i++;
 };*/

// se->cursor_pos = i;
// se->cursor_base = i;

//fprintf(stdout, "new_pos %i line %i %i %i", se->cursor_pos, (int) se->text [se->cursor_line] [0], (int) se->text [se->cursor_line] [1], (int) se->text [se->cursor_line] [2]);


}




void select_text(int x, int y)
{

 struct source_editstruct* se = get_current_source_edit();

 if (se == NULL
  || se->type != SOURCE_EDIT_TYPE_SOURCE)
  return;

 get_cursor_position_from_mouse(se, x, y);

// check whether the mouse has been moved since the selecting started)
 if (se->select_fix_line != se->cursor_line
  || se->select_fix_pos != se->cursor_pos
  || se->select_free_line != se->cursor_line
  || se->select_free_pos != se->cursor_pos)
 {
  se->selected = 1;
  se->select_free_line = se->cursor_line;
  se->select_free_pos = se->cursor_pos;
 }
  else
   se->selected = 0;


}

void get_cursor_position_from_mouse(struct source_editstruct* se, int x, int y)
{

 se->cursor_line = se->window_line + ((y + 3 + EDIT_LINE_OFFSET) / EDIT_LINE_H); // 3 is fine-tuning

 if (se->cursor_line >= SOURCE_TEXT_LINES)
  se->cursor_line = SOURCE_TEXT_LINES - 1;
 if (se->cursor_line < 0)
  se->cursor_line = 0;

 se->cursor_pos = se->window_pos - 1 + ((x + SOURCE_WINDOW_MARGIN) / editor.text_width);

 if (se->cursor_pos > strlen(se->text [se->line_index [se->cursor_line]]))
  se->cursor_pos = strlen(se->text [se->line_index [se->cursor_line]]);

 se->cursor_base = se->cursor_pos;

}

// user clicked in the code completion box
// x and y are offsets from the top left of the box
void	click_in_completion_box(struct source_editstruct* se, int x, int y)
{

	int line_clicked;

	line_clicked = completion.window_pos + ((y - COMPLETION_BOX_LINE_Y_OFFSET) / COMPLETION_BOX_LINE_H);

	complete_code(se, line_clicked); // complete_code will bounds-check line_clicked and check se for NULL

}


// can call this anytime the editor window should be open (even if it's already open)
void open_editor(void)
{

 settings.edit_window = EDIT_WINDOW_EDITOR;
 settings.keyboard_capture = INPUT_EDITOR;

}

void close_editor(void)
{

 editor.submenu_open = -1;

 settings.edit_window = EDIT_WINDOW_CLOSED;
 settings.keyboard_capture = INPUT_WORLD;

}


/*

Need to revise this as it doesn't work very well.

basically:
 - if multiple keys are pressed at the same time, all should be registered.
 - then the last one to be registered counts as the key being held down
 - as long as that key is being held and no new key is pressed, that key keeps being added subject to key delay
  - however, if another key is pressed, it is immediately set as the key being held down and replaces the previous key


*/
void editor_input_keys(void)
{

 if (editor.selecting)
  return; // don't accept keypresses while the mouse is being dragged to select text.
 if (editor.overwindow_type != OVERWINDOW_TYPE_NONE)
  return; // don't accept keypresses while overwindow open

 int i, j, found_word, char_type;
 struct source_editstruct* se;

 if (ex_control.keys_pressed == 0)
 {
  editor.key_delay = 0;
  ex_control.key_being_pressed = -1;
  return;
 }

  editor.already_pressed_cursor_key_etc = 0;

  close_submenu();

// test for modified keys here:
  if (ex_control.key_press [ALLEGRO_KEY_LCTRL] > 0
   || ex_control.key_press [ALLEGRO_KEY_RCTRL] > 0)
  {
   if (editor.key_delay > 0)
   {
    if (ex_control.keys_pressed == 1) // just the control key
     editor.key_delay = 0;
      else
       editor.key_delay --;
    return;
   }

   if (ex_control.key_press [ALLEGRO_KEY_F] != 0)
   {
// if changed, check code for Edit submenu in submenu_operation
     open_overwindow(OVERWINDOW_TYPE_FIND);
//     editor.key_delay = 15;
     return;
   }

   if (ex_control.key_press [ALLEGRO_KEY_C] != 0
    || ex_control.key_press [ALLEGRO_KEY_INSERT] != 0
    || ex_control.key_press [ALLEGRO_KEY_PAD_0] != 0)
   {
    if (just_pressed_ctrl_key(ALLEGRO_KEY_C)
     || just_pressed_ctrl_key(ALLEGRO_KEY_INSERT)
     || just_pressed_ctrl_key(ALLEGRO_KEY_PAD_0))
    {
// if changed, check code for Edit submenu in submenu_operation
     copy_selection();
     editor.key_delay = KEY_DELAY1;
     editor.cursor_flash = CURSOR_FLASH_MAX;
    }
    return;
   }
   if (ex_control.key_press [ALLEGRO_KEY_V] != 0)
   {
    if (just_pressed_ctrl_key(ALLEGRO_KEY_V))
    {
// if changed, check code for Edit submenu in submenu_operation
     paste_clipboard();
     editor.key_delay = KEY_DELAY1;
     editor.cursor_flash = CURSOR_FLASH_MAX;
    }
    return;
   }
   if (ex_control.key_press [ALLEGRO_KEY_X] != 0)
   {
    if (just_pressed_ctrl_key(ALLEGRO_KEY_X))
    {
// if changed, check code for Edit submenu in submenu_operation
     copy_selection();
     delete_selection(); // ignore return value
     editor.key_delay = KEY_DELAY1;
     editor.cursor_flash = CURSOR_FLASH_MAX;
    }
    return;
   }
   if (ex_control.key_press [ALLEGRO_KEY_Z] != 0)
   {
    if (just_pressed_ctrl_key(ALLEGRO_KEY_Z))
    {
// if changed, check code for Edit submenu in submenu_operation
     call_undo();
     editor.key_delay = KEY_DELAY1;
     editor.cursor_flash = CURSOR_FLASH_MAX;
    }
    return;
   }
   if (ex_control.key_press [ALLEGRO_KEY_Y] != 0)
   {
    if (just_pressed_ctrl_key(ALLEGRO_KEY_Y))
    {
// if changed, check code for Edit submenu in submenu_operation
     call_redo();
     editor.key_delay = KEY_DELAY1;
     editor.cursor_flash = CURSOR_FLASH_MAX;
    }
    return;
   }

   if ((ex_control.key_press [ALLEGRO_KEY_PAD_4] > 0
    || ex_control.key_press [ALLEGRO_KEY_LEFT] > 0)
     && ex_control.key_press [ALLEGRO_KEY_PAD_6] <= 0
     && ex_control.key_press [ALLEGRO_KEY_RIGHT] <= 0)
   {
    se = get_current_source_edit();
    if (se == NULL
     || se->type != SOURCE_EDIT_TYPE_SOURCE)
     return;
    i = se->cursor_pos;
    j = se->cursor_line;
    found_word = 0;
    while(TRUE)
    {
     i --;
     if (i == 0 && found_word == 1) // this deals with a word that starts at the start of the line
      break;
     while (i <= 0) // reached start of line
     {
      if (j <= 0) // reached start of file
      {
       i = 0;
       j = 0;
       break;
      }
      j --;
      i = strlen(se->text [se->line_index [j]]) - 1; // this may result in i being -1, in which case the while (i <= 0) loop will reiterate
     } // end while (i <= 0)
     if (i == 0 && j == 0)
      break; // reached start of file
     char_type = get_source_char_type(se->text [se->line_index [j]] [i]);
     if (char_type == SCHAR_LETTER
      || char_type == SCHAR_NUMBER)
     {
      found_word = 1;
     }
      else
      {
       if (found_word == 1)
        break;
      }
    } // end while(TRUE)
    se->cursor_pos = i;
    se->cursor_line = j;
    if (ex_control.key_press [ALLEGRO_KEY_PAD_4] == BUTTON_JUST_PRESSED
     || ex_control.key_press [ALLEGRO_KEY_LEFT] == BUTTON_JUST_PRESSED)
      editor.key_delay = KEY_DELAY1;
       else
        editor.key_delay = KEY_DELAY2;
    editor.cursor_flash = CURSOR_FLASH_MAX;
    window_find_cursor(se);
    return;
   } // end if left cursor key pressed


   if ((ex_control.key_press [ALLEGRO_KEY_PAD_6] > 0
    || ex_control.key_press [ALLEGRO_KEY_RIGHT] > 0)
     && ex_control.key_press [ALLEGRO_KEY_PAD_4] <= 0
     && ex_control.key_press [ALLEGRO_KEY_LEFT] <= 0)
   {
    se = get_current_source_edit();
    if (se == NULL
     || se->type != SOURCE_EDIT_TYPE_SOURCE)
     return;
    i = se->cursor_pos;
    j = se->cursor_line;
    found_word = 0;
    while(TRUE)
    {
     i ++;
     while (i >= strlen(se->text [se->line_index [j]])) // reached end of line
     {
      if (j >= SOURCE_TEXT_LINES - 1) // reached end of file
      {
       i = 0;
       j = SOURCE_TEXT_LINES - 1;
       break;
      }
      j ++;
      i = 0;
      found_word = 1; // end of line counts as gap between words
     } // end while (i >= strlen(etc))
     if (i == 0 && j == SOURCE_TEXT_LINES - 1)
      break; // reached end of file
     char_type = get_source_char_type(se->text [se->line_index [j]] [i]);
     if (char_type != SCHAR_LETTER
      && char_type != SCHAR_NUMBER)
     {
      found_word = 1; // here found_word actually means found non-word
     }
      else
      {
       if (found_word == 1)
        break;
      }
    } // end while(TRUE)
    se->cursor_pos = i;
// the following block just makes sure that if the cursor goes below the window, it only scrolls a few lines rather than resetting the window. Still need to call window_find_cursor() below
    if (j > se->cursor_line)
    {
     se->cursor_line = j;
     while (se->cursor_line >= se->window_line + editor.edit_window_lines)
     {
      se->window_line ++;
     };
//     if (se->cursor_line >= se->window_line + editor.edit_window_lines)
//      se->window_line = se->cursor_line - editor.edit_window_lines;
    }
     if (ex_control.key_press [ALLEGRO_KEY_PAD_6] == BUTTON_JUST_PRESSED
      || ex_control.key_press [ALLEGRO_KEY_RIGHT] == BUTTON_JUST_PRESSED)
       editor.key_delay = KEY_DELAY1;
        else
         editor.key_delay = KEY_DELAY2;
     editor.cursor_flash = CURSOR_FLASH_MAX;
     window_find_cursor(se);
     return;
    }

   editor.key_delay = 0;
   return;
  } // end of if CTRL key being pressed



// first: if a key was being held down, check if it still is:
 if (ex_control.key_being_pressed != -1)
 {
  if (ex_control.key_press [ex_control.key_being_pressed] <= 0) // no longer being pressed
  {
   ex_control.key_being_pressed = -1;
   editor.key_delay = 0;
// so check whether another key is being pressed instead:
   for (i = 0; i < ALLEGRO_KEY_MAX; i ++)
   {
    if (ex_control.key_press [i] > 0)
    {
     if (key_type [i].type != KEY_TYPE_MOD)
     {
      ex_control.key_being_pressed = i;
      if (key_type [i].type == KEY_TYPE_OTHER)
       editor_special_keypress(i);
        else
         editor_keypress(i);
      editor.key_delay = KEY_DELAY1;
      editor.cursor_flash = CURSOR_FLASH_MAX;
      break;
 //    return;
     }
    }
   }
  }
   else // key_being_pressed is still being pressed.
   {
    if (editor.key_delay > 0)
    {
     editor.key_delay --;
    }
     else
     {
      editor.key_delay = KEY_DELAY2;
      editor_keypress(ex_control.key_being_pressed);
//   fprintf(stdout, "\nPressed: %i type %i max %i", ex_control.key_being_pressed, key_type [ex_control.key_being_pressed], ALLEGRO_KEY_MAX);
//     return;
     }
// now we check for another key just having been pressed:
    for (i = 0; i < ALLEGRO_KEY_MAX; i ++)
    {
     if (ex_control.key_press [i] == BUTTON_JUST_PRESSED
      && i != ex_control.key_being_pressed)
     {
      if (key_type [i].type != KEY_TYPE_MOD)
      {
       ex_control.key_being_pressed = i;
       if (key_type [i].type == KEY_TYPE_OTHER)
        editor_special_keypress(i);
         else
          editor_keypress(i);
       editor.key_delay = KEY_DELAY1;
       editor.cursor_flash = CURSOR_FLASH_MAX;
 //    return;
// lack of return or break means that all currently pressed keys will be processed - the last one processed will end up being the new key_being_pressed
      }
     }
    }

   }

 } // end if ex_control.key_being_pressed != -1
  else
  {
     editor.key_delay = 0; // not sure this is strictly necessary
// no key previously being pressed. Check for a new key to be pressed:
    for (i = 0; i < ALLEGRO_KEY_MAX; i ++)
    {
     if (ex_control.key_press [i] > 0)
     {
      if (key_type [i].type != KEY_TYPE_MOD)
      {
       ex_control.key_being_pressed = i;
       if (key_type [i].type == KEY_TYPE_OTHER)
        editor_special_keypress(i);
         else
          editor_keypress(i);
       editor.key_delay = KEY_DELAY1;
       editor.cursor_flash = CURSOR_FLASH_MAX;
 //    return;
// lack of return or break means that all currently pressed keys will be processed - the last one processed will end up being the new key_being_pressed
      }
     }
    }
  }


}


/*

void editor_input_keys(void)
{

 if (editor.selecting)
  return; // don't accept keypresses while the mouse is being dragged to select text.

 int i;

// first: if a key was being held down, check if it still is:
 if (ex_control.key_being_pressed != -1)
 {
  if (ex_control.key_press [ex_control.key_being_pressed] > 0)
  {
// still being pressed
   if (editor.key_delay > 0)
   {
    editor.key_delay --;
    return;
   }
    else
    {
     editor.key_delay = KEY_DELAY2;
     editor_keypress(ex_control.key_being_pressed);
//   fprintf(stdout, "\nPressed: %i type %i max %i", ex_control.key_being_pressed, key_type [ex_control.key_being_pressed], ALLEGRO_KEY_MAX);
//     return;
    }
  }
   else
   {
    ex_control.key_being_pressed = -1;
    editor.key_delay = 0;
   }
 }

// editor.key_delay = 0;


 if (ex_control.keys_pressed > 0)
 {
  close_submenu();

// test for modified keys here:
  if (ex_control.key_press [ALLEGRO_KEY_LCTRL] > 0
   || ex_control.key_press [ALLEGRO_KEY_RCTRL] > 0)
  {
   if (ex_control.key_press [ALLEGRO_KEY_C] != 0
    || ex_control.key_press [ALLEGRO_KEY_INSERT] != 0
    || ex_control.key_press [ALLEGRO_KEY_PAD_0] != 0)
   {
    if (just_pressed_ctrl_key(ALLEGRO_KEY_C)
     || just_pressed_ctrl_key(ALLEGRO_KEY_INSERT)
     || just_pressed_ctrl_key(ALLEGRO_KEY_PAD_0))
    {
// if changed, check code for Edit submenu in submenu_operation
     copy_selection();
     editor.key_delay = KEY_DELAY1;
     editor.cursor_flash = CURSOR_FLASH_MAX;
    }
    return;
   }
   if (ex_control.key_press [ALLEGRO_KEY_V] != 0)
   {
    if (just_pressed_ctrl_key(ALLEGRO_KEY_V))
    {
// if changed, check code for Edit submenu in submenu_operation
     paste_clipboard();
     editor.key_delay = KEY_DELAY1;
     editor.cursor_flash = CURSOR_FLASH_MAX;
    }
    return;
   }
   if (ex_control.key_press [ALLEGRO_KEY_X] != 0)
   {
    if (just_pressed_ctrl_key(ALLEGRO_KEY_X))
    {
// if changed, check code for Edit submenu in submenu_operation
     copy_selection();
     delete_selection(); // ignore return value
     editor.key_delay = KEY_DELAY1;
     editor.cursor_flash = CURSOR_FLASH_MAX;
    }
    return;
   }
   if (ex_control.key_press [ALLEGRO_KEY_Z] != 0)
   {
    if (just_pressed_ctrl_key(ALLEGRO_KEY_Z))
    {
// if changed, check code for Edit submenu in submenu_operation
     call_undo();
     editor.key_delay = KEY_DELAY1;
     editor.cursor_flash = CURSOR_FLASH_MAX;
    }
    return;
   }
   if (ex_control.key_press [ALLEGRO_KEY_Y] != 0)
   {
    if (just_pressed_ctrl_key(ALLEGRO_KEY_Y))
    {
// if changed, check code for Edit submenu in submenu_operation
     call_redo();
     editor.key_delay = KEY_DELAY1;
     editor.cursor_flash = CURSOR_FLASH_MAX;
    }
    return;
   }

  }

  for (i = 0; i < ALLEGRO_KEY_MAX; i ++)
  {
   if (ex_control.key_press [i] > 0
    && i != ex_control.key_being_pressed
    && key_type [i].type != KEY_TYPE_OTHER
    && key_type [i].type != KEY_TYPE_MOD)
   {
    if (ex_control.key_being_pressed == -1)
     ex_control.key_being_pressed = i;
    editor_keypress(i);
    editor.key_delay = KEY_DELAY1;
    editor.cursor_flash = CURSOR_FLASH_MAX;
    return;
   }
  }
 }




}

*/

// This function assumes that either lctrl or rctrl is being pressed.
// it checks whether key is being pressed
// then also checks that any of lctrl, rctrl or key has just been pressed
int just_pressed_ctrl_key(int key)
{

 if (ex_control.key_press [key] <= 0)
  return 0;

 if (ex_control.key_press [key] == BUTTON_JUST_PRESSED
  || ex_control.key_press [ALLEGRO_KEY_LCTRL] == BUTTON_JUST_PRESSED
  || ex_control.key_press [ALLEGRO_KEY_RCTRL] == BUTTON_JUST_PRESSED)
   return 1;

 return 0;

}


void editor_keypress(int key_press)
{

#ifdef SANITY_CHECK
 if (key_press < 0 || key_press >= ALLEGRO_KEY_MAX)
 {
  fprintf(stdout, "\nError: key_press out of bounds (%i)", key_press);
  error_call();
 }
#endif

 struct source_editstruct* se = get_current_source_edit();

 if (se == NULL
  || se->type != SOURCE_EDIT_TYPE_SOURCE)
  return;

 switch(key_type [key_press].type)
 {
  case KEY_TYPE_LETTER:
  case KEY_TYPE_NUMBER:
  case KEY_TYPE_SYMBOL:
  	completion.list_size = 0; // remove code completion box, if present
   if (!delete_selection())
   {
    window_find_cursor(se);
    break;
   }
   if (ex_control.key_press [ALLEGRO_KEY_LSHIFT]
    || ex_control.key_press [ALLEGRO_KEY_RSHIFT])
     add_char(key_type [key_press].shifted, 1);
      else
       add_char(key_type [key_press].unshifted, 1);
   window_find_cursor(se);
   se->cursor_base = se->cursor_pos;
   return; // end letters
  case KEY_TYPE_CURSOR:
   if (!editor.already_pressed_cursor_key_etc)
   {
    cursor_etc_key(key_press);
    editor.already_pressed_cursor_key_etc = 1; // set to 0 again at start of editor_key_input
   }
   break;


 }// end switch key_type

}


void editor_special_keypress(int key_press)
{

#ifdef SANITY_CHECK
 if (key_press < 0 || key_press >= ALLEGRO_KEY_MAX)
 {
  fprintf(stdout, "\nError: key_press out of bounds (%i)", key_press);
  error_call();
 }
#endif

 struct source_editstruct* se;

 switch(key_press)
 {
  case ALLEGRO_KEY_F3:
   se = get_current_source_edit();
   if (se != NULL
    && se->type == SOURCE_EDIT_TYPE_SOURCE)
   {
    find_next();
    window_find_cursor(se);
   }
   break;

 }// end switch key_press

}

/*
ALLEGRO_KEY_A ... ALLEGRO_KEY_Z,
ALLEGRO_KEY_0 ... ALLEGRO_KEY_9,
ALLEGRO_KEY_PAD_0 ... ALLEGRO_KEY_PAD_9,
ALLEGRO_KEY_F1 ... ALLEGRO_KEY_F12,
ALLEGRO_KEY_ESCAPE,
ALLEGRO_KEY_TILDE,
ALLEGRO_KEY_MINUS,
ALLEGRO_KEY_EQUALS,
ALLEGRO_KEY_BACKSPACE,
ALLEGRO_KEY_TAB,
ALLEGRO_KEY_OPENBRACE, ALLEGRO_KEY_CLOSEBRACE,
ALLEGRO_KEY_ENTER,
ALLEGRO_KEY_SEMICOLON,
ALLEGRO_KEY_QUOTE,
ALLEGRO_KEY_BACKSLASH, ALLEGRO_KEY_BACKSLASH2,
ALLEGRO_KEY_COMMA,
ALLEGRO_KEY_FULLSTOP,
ALLEGRO_KEY_SLASH,
ALLEGRO_KEY_SPACE,
ALLEGRO_KEY_INSERT, ALLEGRO_KEY_DELETE,
ALLEGRO_KEY_HOME, ALLEGRO_KEY_END,
ALLEGRO_KEY_PGUP, ALLEGRO_KEY_PGDN,
ALLEGRO_KEY_LEFT, ALLEGRO_KEY_RIGHT,
ALLEGRO_KEY_UP, ALLEGRO_KEY_DOWN,
ALLEGRO_KEY_PAD_SLASH, ALLEGRO_KEY_PAD_ASTERISK,
ALLEGRO_KEY_PAD_MINUS, ALLEGRO_KEY_PAD_PLUS,
ALLEGRO_KEY_PAD_DELETE, ALLEGRO_KEY_PAD_ENTER,
ALLEGRO_KEY_PRINTSCREEN, ALLEGRO_KEY_PAUSE,
ALLEGRO_KEY_ABNT_C1, ALLEGRO_KEY_YEN, ALLEGRO_KEY_KANA,
ALLEGRO_KEY_CONVERT, ALLEGRO_KEY_NOCONVERT,
ALLEGRO_KEY_AT, ALLEGRO_KEY_CIRCUMFLEX,
ALLEGRO_KEY_COLON2, ALLEGRO_KEY_KANJI,
ALLEGRO_KEY_LSHIFT, ALLEGRO_KEY_RSHIFT,
ALLEGRO_KEY_LCTRL, ALLEGRO_KEY_RCTRL,
ALLEGRO_KEY_ALT, ALLEGRO_KEY_ALTGR,
ALLEGRO_KEY_LWIN, ALLEGRO_KEY_RWIN,
ALLEGRO_KEY_MENU,
ALLEGRO_KEY_SCROLLLOCK,
ALLEGRO_KEY_NUMLOCK,
ALLEGRO_KEY_CAPSLOCK
ALLEGRO_KEY_PAD_EQUALS,
ALLEGRO_KEY_BACKQUOTE,
ALLEGRO_KEY_SEMICOLON2,
ALLEGRO_KEY_COMMAND
*/

// assumes that key is a valid character to put in source code (e.g. a letter)
// returns 1 on success, 0 on failure
// Note that this function is used to add letters from code completion
// set check_completion to 0 if doing so
int add_char(char added_char, int check_completion)
{

 struct source_editstruct* se = get_current_source_edit();

 if (se == NULL
  || se->type != SOURCE_EDIT_TYPE_SOURCE)
  return 0;

// fprintf(stdout, "\nAdding %i at line %i pos %i index line %i", added_char, se->cursor_line, se->cursor_pos, se->line_index [se->cursor_line]);
// return;

// make sure the new character will fit:
 if (se->cursor_pos >= SOURCE_TEXT_LINE_LENGTH - 2
  || strlen(se->text [se->line_index [se->cursor_line]]) >= SOURCE_TEXT_LINE_LENGTH - 2)
 {
  write_line_to_log("Source line too long.", MLOG_COL_ERROR);
  return 0; // fail
 }

// are the - 2 bits in the if statement correct? Not 100% sure.

// now shift the rest of the line to the right:
 int i = strlen(se->text [se->line_index [se->cursor_line]]);

 int at_line_end = 0;

 if (i == se->cursor_pos)
 {
  at_line_end = 1;
 }

 i++;

 while (i > se->cursor_pos)
 {
  if (i > 0)
   se->text [se->line_index [se->cursor_line]] [i] = se->text [se->line_index [se->cursor_line]] [i - 1];
  i --;
 };

 if (at_line_end == 1)
  se->text [se->line_index [se->cursor_line]] [se->cursor_pos + 1] = '\0';
 se->text [se->line_index [se->cursor_line]] [se->cursor_pos] = added_char;
 add_char_undo(added_char);

 if (check_completion) // don't want to check it when we're actually adding letters because of code completion
  check_code_completion(se, 0);

 se->cursor_pos ++;
 se->saved = 0; // indicates that source has been modified

 update_source_lines(se, se->cursor_line, 1);

 return 1;

}
/*
void update_source_lines(struct source_editstruct* se, int sline, int lines)
{

 int next_line_commented = 0;

 if (sline < SOURCE_TEXT_LINES - 1)
 {

  next_line_commented = se->comment_line [se->line_index [sline + 1]];
 }

 int new_comment = source_line_highlight_syntax(se, se->line_index [sline], se->comment_line [se->line_index [sline]]);
// source_line_highlight_syntax returns 1 if the line ends in a multi-line comment

// the update may have resulted in the line now ending in a multi-line comment when it didn't before:
 int i = sline;

 if (new_comment
  && !next_line_commented)
 {
  do
  {
   i++;
  }  while (i < SOURCE_TEXT_LINES - 1
         && source_line_highlight_syntax(se, se->line_index [i], 1));
 }

// the update may have resulted in the line now *not* ending in a multi-line comment when it did before:
 if (!new_comment
  && next_line_commented)
 {
  do
  {
   i++;
  }  while (i < SOURCE_TEXT_LINES - 1
         && !source_line_highlight_syntax(se, se->line_index [i], 0));
 }


}
*/



// updates syntax highlighting on one or more lines of source code
// if these lines now end in a comment when they didn't before, or vice-versa, it continues on and updates further lines.
// sline is position in line_index array (not text array)
void update_source_lines(struct source_editstruct* se, int sline, int lines)
{

// fprintf(stdout, "\nUpdating: sline %i lines %i", sline, lines);

// first update all lines we've been told to update:
 int i = 0;
 int in_a_comment = se->comment_line [se->line_index [sline]];

 if (sline + lines >= SOURCE_TEXT_LINES - 1)
  lines = SOURCE_TEXT_LINES - sline - 1;

 while(i < lines)
 {
  in_a_comment = source_line_highlight_syntax(se, se->line_index [sline + i], in_a_comment);
  i ++;

 };
// finished updating the specified lines.
// now see if the update has resulted in the immediately following line's comment status changing:


 sline += lines - 1;

 if (sline >= SOURCE_TEXT_LINES - 1)
  return;

 int next_line_commented = se->comment_line [se->line_index [sline + 1]];
// int comment_status = 0;

 i = sline;

// fprintf(stdout, "\nin_a_comment %i next_line_commented %i", in_a_comment, next_line_commented);



// the update may have resulted in the line now ending in a multi-line comment when it didn't before:
 if (in_a_comment
 && !next_line_commented)
 {
  do
  {
   i++;
  }  while (i < SOURCE_TEXT_LINES - 1
         && source_line_highlight_syntax(se, se->line_index [i], 1));
 }

// the update may have resulted in the line now *not* ending in a multi-line comment when it did before:
 if (!in_a_comment
  && next_line_commented)
 {
  do
  {
   i++;
// this doesn't work correctly - it uncomments many lines that it doesn't need to (although the ultimate result is correct). Not sure why.
  }  while (i < SOURCE_TEXT_LINES - 1
         && !source_line_highlight_syntax(se, se->line_index [i], 0));
 }



}




void cursor_etc_key(int key_press)
{

 struct source_editstruct* se = get_current_source_edit();

 if (se == NULL
  || se->type != SOURCE_EDIT_TYPE_SOURCE)
  return;

 int i;

 switch(key_press)
 {
// just cursor movement:


  case ALLEGRO_KEY_PAD_4:
  case ALLEGRO_KEY_LEFT:
  case ALLEGRO_KEY_PAD_6:
  case ALLEGRO_KEY_RIGHT:
  case ALLEGRO_KEY_PAD_8:
  case ALLEGRO_KEY_UP:
  case ALLEGRO_KEY_PAD_2:
  case ALLEGRO_KEY_DOWN:
  case ALLEGRO_KEY_PAD_1:
  case ALLEGRO_KEY_END:
  case ALLEGRO_KEY_PAD_7:
  case ALLEGRO_KEY_HOME:
  case ALLEGRO_KEY_PAD_9:
  case ALLEGRO_KEY_PGUP:
  case ALLEGRO_KEY_PAD_3:
  case ALLEGRO_KEY_PGDN:
   movement_keys(se);
   break;

// text movement, deletion etc:
  case ALLEGRO_KEY_ENTER:
  case ALLEGRO_KEY_PAD_ENTER:
   se->saved = 0; // indicates that source has been modified
   if (completion.list_size != 0)
			{
				complete_code(se, completion.select_line);
				break;
			}
   if (!delete_selection())
   {
    window_find_cursor(se);
    break;
   }
// pressing enter inserts an empty line directly after the current line, then copies the rest of the current line to it, then deletes the rest of the current line.
// first insert an empty line (must check whether possible):
   if (insert_empty_lines(se, se->cursor_line + 1, 1))
   {
// current line is now followed by an empty line.
// now copy remaining text from current line onto next line:
    i = 0;
    while (se->text [se->line_index [se->cursor_line]] [se->cursor_pos + i] != '\0')
    {
     se->text [se->line_index [se->cursor_line + 1]] [i] = se->text [se->line_index [se->cursor_line]] [se->cursor_pos + i];
     i++;
    };
    se->text [se->line_index [se->cursor_line + 1]] [i] = '\0';
    se->text [se->line_index [se->cursor_line]] [se->cursor_pos] = '\0';
// update syntax highlighting for both lines:
    update_source_lines(se, se->cursor_line, 2);
//    update_source_lines(se, se->cursor_line + 1);
// put cursor on next line
    add_enter_undo();
    se->cursor_line ++;
    se->cursor_pos = 0;
    window_find_cursor(se);
   }
    else
    {
      write_line_to_log("Out of space in source file.", MLOG_COL_ERROR);
    }
   se->cursor_base = se->cursor_pos;
   break;

  case ALLEGRO_KEY_BACKSPACE:
   se->saved = 0; // indicates that source has been modified
   if (is_something_selected(se))
   {
    delete_selection(); // ignore return value
    return;
   }
   if (se->cursor_pos > 0)
   {
    se->cursor_pos --;
    backspace_char_undo(se->text [se->line_index [se->cursor_line]] [se->cursor_pos]);
    check_code_completion(se, 1);
    i = se->cursor_pos;
    while(i < SOURCE_TEXT_LINE_LENGTH - 1)
    {
// this loop runs to the end of the line, ignoring null terminator, but that doesn't really matter.
     se->text [se->line_index [se->cursor_line]] [i] = se->text [se->line_index [se->cursor_line]] [i + 1];
     i++;
    };
    update_source_lines(se, se->cursor_line, 1);
    window_find_cursor(se);
    se->cursor_base = se->cursor_pos;
    break;
   }
// cursor must be at start of line.
// first make sure it's not at the start of the file:
   if (se->cursor_line == 0)
   {
    window_find_cursor(se);
    se->cursor_base = se->cursor_pos;
    break;
   }
// now make sure the current line can fit on the end of the previous line:
   if (strlen(se->text [se->line_index [se->cursor_line]]) + strlen(se->text [se->line_index [se->cursor_line - 1]]) >= SOURCE_TEXT_LINE_LENGTH)
   {
    write_line_to_log("Lines too long to combine.", MLOG_COL_ERROR);
    se->cursor_base = se->cursor_pos;
    break;
   }
// now set cursor pos to the end of the previous line:
   se->cursor_pos = strlen(se->text [se->line_index [se->cursor_line - 1]]);
   add_undo_remove_enter();
// copy current line to end of previous line, and update it:
   strcat(se->text [se->line_index [se->cursor_line - 1]], se->text [se->line_index [se->cursor_line]]);
// now delete the current line:
   delete_lines(se, se->cursor_line, 1);
// finally reduce cursor_line and run syntax highlighting:
   se->cursor_line --;
   update_source_lines(se, se->cursor_line, 1);
   window_find_cursor(se);
   se->cursor_base = se->cursor_pos;
   break;

  case ALLEGRO_KEY_DELETE:
  case ALLEGRO_KEY_PAD_DELETE:
   se->saved = 0; // indicates that source has been modified
   if (is_something_selected(se))
   {
    delete_selection(); // ignore return value
    return;
   }
   if (se->cursor_pos < strlen(se->text [se->line_index [se->cursor_line]]))
   {
    delete_char_undo(se->text [se->line_index [se->cursor_line]] [se->cursor_pos]);
    i = se->cursor_pos;
    while(i < SOURCE_TEXT_LINE_LENGTH - 1)
    {
// this loop runs to the end of the line, ignoring null terminator, but that doesn't really matter.
     se->text [se->line_index [se->cursor_line]] [i] = se->text [se->line_index [se->cursor_line]] [i + 1];
     i++;
    };
    update_source_lines(se, se->cursor_line, 1);
    window_find_cursor(se);
    se->cursor_base = se->cursor_pos;
    break;
   }
// cursor must be at the end of the line.
// first make sure it's not at the end of the file:
   if (se->cursor_line >= SOURCE_TEXT_LINES - 1)
   {
    window_find_cursor(se);
    se->cursor_base = se->cursor_pos;
    break;
   }
// now make sure the next line can fit on the end of the current line:
   if (strlen(se->text [se->line_index [se->cursor_line]]) + strlen(se->text [se->line_index [se->cursor_line + 1]]) >= SOURCE_TEXT_LINE_LENGTH)
   {
    write_line_to_log("Lines too long to combine.", MLOG_COL_ERROR);
    se->cursor_base = se->cursor_pos;
    break;
   }
   add_undo_remove_enter();
// now copy next line to end of current line, and update it:
   strcat(se->text [se->line_index [se->cursor_line]], se->text [se->line_index [se->cursor_line + 1]]);
// now delete the next line:
   delete_lines(se, se->cursor_line + 1, 1);
// finally run syntax highlighting:
   update_source_lines(se, se->cursor_line, 1);
   window_find_cursor(se);
   se->cursor_base = se->cursor_pos;
   break;



// may need to have number pad keys here too, with fall-through if numlock off
/*
Check: have I been using the indirect line-index method for accessing source_edit->text? Because I should.

   case ALLEGRO_KEY_BACKSPACE:

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
*/
 }

}

// if the user presses a key while the window is away from the cursor, bring the window back
// also updates the scrollbar to deal with any changes in window position
void window_find_cursor(struct source_editstruct* se)
{

 if (se->window_line < se->cursor_line - editor.edit_window_lines
  || se->window_line > se->cursor_line)// + editor.edit_window_lines)
  se->window_line = se->cursor_line;

 if (se->window_pos < se->cursor_pos - editor.edit_window_chars
  || se->window_pos > se->cursor_pos)// + editor.edit_window_lines)
 {
// 	fprintf(stdout, "\nwindow_pos %i cursor_pos %i ewc %i", se->window_pos, se->cursor_pos, editor.edit_window_chars);
  se->window_pos = se->cursor_pos - editor.edit_window_chars;
  if (se->window_pos < 0)
   se->window_pos = 0;
 }


 slider_moved_to_value(&editor.scrollbar_v, se->window_line);
 slider_moved_to_value(&editor.scrollbar_h, se->window_pos);

}


// if any movement key is being pressed, all of them are accepted inputs:
void movement_keys(struct source_editstruct* se)
{

 int old_line = se->cursor_line;
 int old_pos = se->cursor_pos;
 editor.cursor_flash = CURSOR_FLASH_MAX;

 int reset_code_completion_box = 1; // completion.list_size will be reset to 0 if this is still 1 by the end of the function. It is set to zero by up and down

// the following is code for left arrow without control (left arrow with control is dealt with in editor_input_keys()):
 if ((ex_control.key_press [ALLEGRO_KEY_PAD_4] > 0
  || ex_control.key_press [ALLEGRO_KEY_LEFT] > 0)
   && ex_control.key_press [ALLEGRO_KEY_PAD_6] <= 0
   && ex_control.key_press [ALLEGRO_KEY_RIGHT] <= 0)
 {
    if (se->cursor_pos > 0)
    {
     se->cursor_pos --;
    }
     else
     {
      if (se->cursor_line > 0)
      {
       se->cursor_line --;
       se->cursor_pos = strlen(se->text [se->line_index [se->cursor_line]]);
       if (se->cursor_line < se->window_line)
        se->window_line --;
      }
     }
   window_find_cursor(se);
   se->cursor_base = se->cursor_pos;
 }

 if ((ex_control.key_press [ALLEGRO_KEY_PAD_6] > 0
  || ex_control.key_press [ALLEGRO_KEY_RIGHT] > 0)
   && ex_control.key_press [ALLEGRO_KEY_PAD_4] <= 0
   && ex_control.key_press [ALLEGRO_KEY_LEFT] <= 0)
 {

// the following is code for right arrow without control (right arrow with control is dealt with in editor_input_keys()):
   if (se->cursor_pos < strlen(se->text [se->line_index [se->cursor_line]]))
   {
    se->cursor_pos ++;
   }
    else
    {
     if (se->cursor_line < SOURCE_TEXT_LINES - 1)
     {
      se->cursor_line ++;
      se->cursor_pos = 0;
      if (se->cursor_line >= se->window_line + editor.edit_window_lines)
       se->window_line ++;
     }
    }
   window_find_cursor(se);
   se->cursor_base = se->cursor_pos;
 }


 if ((ex_control.key_press [ALLEGRO_KEY_PAD_8] > 0
  || ex_control.key_press [ALLEGRO_KEY_UP] > 0)
   && ex_control.key_press [ALLEGRO_KEY_PAD_2] <= 0
   && ex_control.key_press [ALLEGRO_KEY_DOWN] <= 0)
 {
		if (completion.list_size > 0)
		{
			completion_box_select_line_up();
	  reset_code_completion_box = 0;
		}
		 else
			{
    if (se->cursor_line > 0)
    {
     se->cursor_line --;
     se->cursor_pos = se->cursor_base;
     if (se->cursor_pos > strlen(se->text [se->line_index [se->cursor_line]]))
      se->cursor_pos = strlen(se->text [se->line_index [se->cursor_line]]);
     if (se->cursor_line < se->window_line)
      se->window_line --;
    }
    window_find_cursor(se);
			}
 }


 if ((ex_control.key_press [ALLEGRO_KEY_PAD_2] > 0
  || ex_control.key_press [ALLEGRO_KEY_DOWN] > 0)
   && ex_control.key_press [ALLEGRO_KEY_PAD_8] <= 0
   && ex_control.key_press [ALLEGRO_KEY_UP] <= 0)
 {
		if (completion.list_size > 0)
		{
   completion_box_select_line_down();
	  reset_code_completion_box = 0;
		}
		 else
			{
    if (se->cursor_line < SOURCE_TEXT_LINES - 1)
    {
     se->cursor_line ++;
     se->cursor_pos = se->cursor_base;
     if (se->cursor_pos > strlen(se->text [se->line_index [se->cursor_line]]))
      se->cursor_pos = strlen(se->text [se->line_index [se->cursor_line]]);
     if (se->cursor_line >= se->window_line + editor.edit_window_lines)
      se->window_line ++;
    }
    window_find_cursor(se);
			}
 }


 if ((ex_control.key_press [ALLEGRO_KEY_PAD_1] > 0
  || ex_control.key_press [ALLEGRO_KEY_END] > 0)
   && ex_control.key_press [ALLEGRO_KEY_PAD_7] <= 0
   && ex_control.key_press [ALLEGRO_KEY_HOME] <= 0)
  {
   se->cursor_pos = strlen(se->text [se->line_index [se->cursor_line]]);
   window_find_cursor(se);
   se->cursor_base = se->cursor_pos;
  }

 if ((ex_control.key_press [ALLEGRO_KEY_PAD_7] > 0
  || ex_control.key_press [ALLEGRO_KEY_HOME] > 0)
   && ex_control.key_press [ALLEGRO_KEY_PAD_1] <= 0
   && ex_control.key_press [ALLEGRO_KEY_END] <= 0)
 {
   se->cursor_pos = 0;//strlen(se->text [se->line_index [se->cursor_line]]);
   window_find_cursor(se);
   se->cursor_base = se->cursor_pos;
 }

 if ((ex_control.key_press [ALLEGRO_KEY_PAD_9] > 0
  || ex_control.key_press [ALLEGRO_KEY_PGUP] > 0)
   && ex_control.key_press [ALLEGRO_KEY_PAD_3] <= 0
   && ex_control.key_press [ALLEGRO_KEY_PGDN] <= 0)
  {
 		if (completion.list_size > 0)
		 {
 			completion_box_select_lines_up(11);
	   reset_code_completion_box = 0;
		 }
		  else
		  {
     se->cursor_line -= editor.edit_window_lines - 1;
     if (se->cursor_line < 0)
      se->cursor_line = 0;
     if (se->cursor_pos > strlen(se->text [se->line_index [se->cursor_line]]))
      se->cursor_pos = strlen(se->text [se->line_index [se->cursor_line]]);
     se->window_line -= editor.edit_window_lines - 1;
     if (se->window_line < 0)
      se->window_line = 0;
     window_find_cursor(se);
     se->cursor_base = se->cursor_pos;
		  }
  }

 if ((ex_control.key_press [ALLEGRO_KEY_PAD_3] > 0
  || ex_control.key_press [ALLEGRO_KEY_PGDN] > 0)
   && ex_control.key_press [ALLEGRO_KEY_PAD_9] <= 0
   && ex_control.key_press [ALLEGRO_KEY_PGUP] <= 0)
 {
		if (completion.list_size > 0)
	 {
			completion_box_select_lines_down(11);
   reset_code_completion_box = 0;
	 }
	  else
			{
    se->cursor_line += editor.edit_window_lines - 1;
    if (se->cursor_line >= SOURCE_TEXT_LINES)
     se->cursor_line = SOURCE_TEXT_LINES - 1;
    if (se->cursor_pos > strlen(se->text [se->line_index [se->cursor_line]]))
     se->cursor_pos = strlen(se->text [se->line_index [se->cursor_line]]);
    se->window_line += editor.edit_window_lines - 1;
    if (se->window_line >= SOURCE_TEXT_LINES)
     se->window_line = SOURCE_TEXT_LINES - 1;
    window_find_cursor(se);
    se->cursor_base = se->cursor_pos;
			}
  }

 if (ex_control.key_press [ALLEGRO_KEY_LSHIFT] > 0
  || ex_control.key_press [ALLEGRO_KEY_RSHIFT] > 0)
 {
  if (se->selected == 0)
  {
   se->select_fix_line = old_line;
   se->select_fix_pos = old_pos;
  }
  se->selected = 1;
  se->select_free_line = se->cursor_line;
  se->select_free_pos = se->cursor_pos;
 }
  else
   se->selected = 0;

 if (reset_code_completion_box == 1)
		completion.list_size = 0;

}



// returns 1 if successful, 0 on failure
// before_line is position in line_index
int insert_empty_lines(struct source_editstruct* se, int before_line, int lines)
{

 if (before_line + lines >= SOURCE_TEXT_LINES - 1)
  return 0;

 if (lines <= 0)
  return 1;

 int in_a_comment = 0;

 if (before_line > 0
  && se->comment_line [se->line_index [before_line - 1]])
   in_a_comment = 1;

// first make sure there are enough empty lines at the end of the source
// also copy the indexes of those empty lines into a storage array so they can be used later
 int i;
 int line_index_save [SOURCE_TEXT_LINES];
 int j = 0;

 for (i = SOURCE_TEXT_LINES - lines; i < SOURCE_TEXT_LINES; i ++)
 {
  if (se->text [se->line_index [i]] [0] != '\0')
   return 0; // failed
  line_index_save [j] = se->line_index [i];
  j++;
 }

// now work back from the end of the end of the source, pushing each line back by lines:
 i = SOURCE_TEXT_LINES - 1;

 while (i >= before_line + lines)
 {
  se->line_index [i] = se->line_index [i - lines];
  i --;
 };

// now set the line_index values of the inserted lines to the lines that were removed from the end of the source:
 for (i = 0; i < lines; i ++)
 {
  se->line_index [before_line + i] = line_index_save [i];
  se->comment_line [se->line_index [before_line + i]] = in_a_comment;
 }

 return 1;

}


// start_line is position in line_index
// note that the content of the deleted line is lost.
void delete_lines(struct source_editstruct* se, int start_line, int lines)
{

 if (start_line + lines >= SOURCE_TEXT_LINES - 1)
  return; // hopefully this should never happen if this function is called correctly

// first store the indexes of the lines being deleted.
 int i;
 int line_index_save [SOURCE_TEXT_LINES];
 int j = 0;

 for (i = start_line; i < start_line + lines; i ++)
 {
  line_index_save [j] = se->line_index [i];
  j++;
 }

// now reassign all lines after start_line, up to (SOURCE_TEXT_LINES - lines):
 i = start_line;
 while (i + lines < SOURCE_TEXT_LINES)
 {
  se->line_index [i] = se->line_index [i + lines];
  i ++;
 };

 int source_ends_in_a_comment = 0;

 if (se->comment_line [se->line_index [SOURCE_TEXT_LINES - 1]])
  source_ends_in_a_comment = 1; // this may not work correctly if the last line in the file contains an opening comment. Oh well.

// now replace the remaining lines at the end of the text array with the lines that were saved earlier:
 for (i = 0; i < lines; i ++)
 {
  se->text [line_index_save [i]] [0] = '\0';
  se->line_index [SOURCE_TEXT_LINES - lines + i] = line_index_save [i];
  se->comment_line [se->line_index [SOURCE_TEXT_LINES - lines + i]] = source_ends_in_a_comment;
 }


}



// assumes select_line/pos values are in-bounds
// returns 1 on success, 0 on failure
// success means no error (may not actually delete anything)
int delete_selection(void)
{

 struct source_editstruct* se = get_current_source_edit();

 if (se == NULL
  || se->type != SOURCE_EDIT_TYPE_SOURCE)
  return 0;

 if (!is_something_selected(se))
  return 1;

 se->saved = 0; // indicates that source has been modified

 int start_line = 0, start_pos = 0, end_line = 0, end_pos = 0;

 int i;

// if the start and end are on the same line, it's a bit simpler:
 if (se->select_fix_line == se->select_free_line)
 {
  if (se->select_fix_pos == se->select_free_pos)
   return 1; // nothing to delete
  start_pos = se->select_fix_pos;
  end_pos = se->select_free_pos;
  if (se->select_free_pos < se->select_fix_pos)
  {
   start_pos = se->select_free_pos;
   end_pos = se->select_fix_pos;
  }
// add the selected text to the undo buffer:
  se->cursor_line = se->select_fix_line;
  se->cursor_pos = start_pos;
  for (i = start_pos; i < end_pos; i ++)
  {
//   se->cursor_pos ++;
   delete_char_undo(se->text [se->line_index [se->select_fix_line]] [i]);
  }
// now delete the text by copying over it with later text from the same line:
  i = 0;
  while (TRUE)
  {
   se->text [se->line_index [se->select_fix_line]] [start_pos + i] = se->text [se->line_index [se->select_fix_line]] [end_pos + i];
   if (se->text [se->line_index [se->select_fix_line]] [end_pos + i] == '\0')
    break;
   i ++;
  };
  se->selected = 0;
  update_source_lines(se, se->select_fix_line, 1);
  se->cursor_line = se->select_fix_line;
  se->cursor_pos = start_pos;
  window_find_cursor(se);
  return 1;
 }

 if (se->select_fix_line < se->select_free_line)
 {
  start_line = se->select_fix_line;
  start_pos = se->select_fix_pos;
  end_line = se->select_free_line;
  end_pos = se->select_free_pos;
 }
 if (se->select_fix_line > se->select_free_line)
 {
  start_line = se->select_free_line;
  start_pos = se->select_free_pos;
  end_line = se->select_fix_line;
  end_pos = se->select_fix_pos;
 }


 if (strlen(se->text [se->line_index [start_line]]) + strlen(se->text [se->line_index [end_line]]) >= SOURCE_TEXT_LINE_LENGTH - 1)
 {
   write_line_to_log("Couldn't delete selection; first and last lines too long to join.", MLOG_COL_ERROR);
   return 0;
 }


 add_block_to_undo(start_line, start_pos, end_line, end_pos, UNDO_TYPE_DELETE_BLOCK);

// at this point we know multiple lines are involved.
// first we remove all lines between the first and last (if any):
 if (end_line - start_line > 1)
 {
  delete_lines(se, start_line + 1, end_line - start_line - 1);
  end_line = start_line + 1;
 }

/*

TO DO:  put this code back in (with undo)

// Now we check that it will be possible to stitch together the first and last lines:
 if (strlen(se->text [se->line_index [start_line]]) + strlen(se->text [se->line_index [end_line]]) >= SOURCE_TEXT_LINE_LENGTH - 1)
 {
// lines are too long. So we just remove selected text from each:
  se->text [se->line_index [start_line]] [start_pos] = '\0';
  i = 0;
  while (TRUE)
  {
   se->text [se->line_index [end_line]] [i] = se->text [se->line_index [end_line]] [end_pos + i];
   if (se->text [se->line_index [end_line]] [end_pos + i] == '\0')
    break;
   i ++;
  };
  se->selected = 0;
  update_source_lines(se, start_line, 2);
//  update_source_lines(se, end_line, 1);
  se->cursor_line = start_line;
  se->cursor_pos = start_pos;
  window_find_cursor(se);
  return;
 }*/

// truncate start line:
 se->text [se->line_index [start_line]] [start_pos] = '\0';
// add end line to start line:
 char* end_line_end_pos = &se->text [se->line_index [end_line]] [end_pos];
 strcat(se->text [se->line_index [start_line]], end_line_end_pos);
// strcat(se->text [se->line_index [start_line]], se->text [se->line_index [end_line]]);
// delete end line:
 delete_lines(se, end_line, 1);
 update_source_lines(se, start_line, 1);
 se->cursor_line = start_line;
 se->cursor_pos = start_pos;
 window_find_cursor(se);
 se->selected = 0;

 return 1;

}


// checks whether at least one character is selected
int is_something_selected(struct source_editstruct* se)
{

 if (se->selected == 1
  && (se->select_fix_line != se->select_free_line
   || se->select_fix_pos != se->select_free_pos))
   return 1;

 return 0;

}

// returns pointer to currently open source_edit (based on which tab is open)
// returns NULL if none open (e.g. current tab is not a source file)
struct source_editstruct* get_current_source_edit(void)
{

 if (editor.current_tab == -1
 || (editor.tab_type [editor.current_tab] != TAB_TYPE_SOURCE
  && editor.tab_type [editor.current_tab] != TAB_TYPE_BINARY))
  return NULL;

 return &editor.source_edit [editor.tab_index [editor.current_tab]];

}

// Like get_current_source_edit() but returns index in editor.source_edit []
// Returns -1 if no current source_edit
int get_current_source_edit_index(void)
{

 if (editor.current_tab == -1
 || (editor.tab_type [editor.current_tab] != TAB_TYPE_SOURCE
  && editor.tab_type [editor.current_tab] != TAB_TYPE_BINARY))
  return -1;

 return editor.tab_index [editor.current_tab];

}


// changes the currently open tab
void change_tab(int new_tab)
{

 if (new_tab == -1)
 {
   editor.current_tab = -1;
   return;
 }

 int new_tab_source_edit;

 switch(editor.tab_type [new_tab])
 {
  case TAB_TYPE_SOURCE:
   editor.current_tab = new_tab;
   new_tab_source_edit = editor.tab_index [new_tab];
   init_slider(&editor.scrollbar_v, &editor.source_edit[new_tab_source_edit].window_line, SLIDEDIR_VERTICAL, 0, SOURCE_TEXT_LINES, editor.edit_window_h, 1, (editor.edit_window_h / EDIT_LINE_H) - 1, editor.edit_window_h / EDIT_LINE_H, EDIT_WINDOW_X + settings.editor_x_split + editor.edit_window_w, EDIT_WINDOW_Y - 1, SLIDER_BUTTON_SIZE, COL_BLUE, 0);
   init_slider(&editor.scrollbar_h, &editor.source_edit[new_tab_source_edit].window_pos, SLIDEDIR_HORIZONTAL, 0, SOURCE_TEXT_LINE_LENGTH, editor.edit_window_w, 1, (editor.edit_window_w / editor.text_width) - 1, editor.edit_window_w / editor.text_width, EDIT_WINDOW_X + settings.editor_x_split - 1, EDIT_WINDOW_Y + editor.edit_window_h, SLIDER_BUTTON_SIZE, COL_BLUE, 0);
   break;
  case TAB_TYPE_BINARY:
   editor.current_tab = new_tab;
   new_tab_source_edit = editor.tab_index [new_tab];
   break;

 }

// if (new_se == -1) - not currently possible but will need to do something in this case

//void init_slider(struct sliderstruct* sl, int* value_pointer, int dir, int value_min, int value_max, int total_length, int button_increment, int track_increment, int slider_represents_size, int x, int y, int thickness)


}






// this function is called after the editor does something (like opening a file or running the compiler) that may take a significant amount of time
// it's also called from other places (e.g. from game startup in g_game.c)
void flush_game_event_queues(void)
{

 al_flush_event_queue(event_queue);
 al_flush_event_queue(fps_queue);
 al_flush_event_queue(control_queue);

}
