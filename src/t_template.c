/*

This file contains code for the setup menu (which lets the user setup a world with whatever settings are wanted).

Basically it sets up interface elements that are then used by code in s_menu.c to display a menu and deal with input from it.
s_menu.c calls back here for various things.

*/

#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>

#include <stdio.h>
#include <string.h>

#include "m_config.h"

#include "g_header.h"
#include "m_globvars.h"
#include "i_header.h"

#include "g_misc.h"

#include "c_header.h"
#include "e_slider.h"
#include "e_header.h"
#include "e_editor.h"
#include "e_log.h"
#include "e_files.h"
#include "e_build.h"
#include "e_help.h"
#include "e_inter.h"
#include "g_game.h"
#include "g_client.h"

#include "c_init.h"
#include "c_prepr.h"
#include "c_comp.h"
#include "i_input.h"
#include "i_view.h"
#include "i_buttons.h"
#include "m_input.h"
#include "f_turn.h"

#include "t_template.h"
#include "t_files.h"


extern struct fontstruct font [FONTS];
extern ALLEGRO_DISPLAY* display;

extern struct viewstruct view;
extern struct editorstruct editor; // defined in e_editor.c
extern struct logstruct mlog; // in e_log.c
extern struct world_initstruct w_init;

char tstring [MENU_STRING_LENGTH]; // used for temporary strings being printed to menu buttons

// This is just the height of the word "Templates" up the top
#define TEMPLATE_PANEL_HEADER_H 25

#define TEMPLATE_CAT_HEADING_H 23

#define TEMPLATE_BOX_W 400
#define TEMPLATE_BOX_H 80
#define TEMPLATE_NAME_BOX_X 20
#define TEMPLATE_NAME_BOX_Y 30
#define TEMPLATE_NAME_BOX_W 160
#define TEMPLATE_NAME_BOX_H 15

#define TEMPLATE_LEFT_MARGIN 10

#define TEMPL_BUTTON_X_SPACE 10
#define TEMPL_BUTTON_Y_SPACE 8
#define TEMPL_BUTTON_W 140
#define TEMPL_BUTTON_H 16

#define IMPORT_LIST_X (settings.editor_x_split + 100)
#define IMPORT_LIST_Y 200
#define IMPORT_LIST_HEADER_SIZE 20
#define IMPORT_LIST_W 250
#define IMPORT_LIST_H 165
#define IMPORT_LIST_LINE_H 20

//#define TURNFILEBUTTON_X1 150
#define TURNFILEBUTTON_W 120
#define TURNFILEBUTTON_SEPARATION 5
#define TURNFILEBUTTON_H 16
#define TURNFILEBUTTON_UP 20


struct templstruct templ [TEMPLATES]; // externed in t_files.c, g_method_ex.c, s_setup.c, f_save.c
struct templstruct default_mission_template [TEMPL_MISSION_DEFAULTS];

struct tstatestruct tstate; // externed in t_files.c and other places

s16b system_template_bcode_op [SYSTEM_BCODE_SIZE];
s16b client_templates_bcode_op [PLAYERS + 1] [CLIENT_BCODE_SIZE]; // one for each player +1 for the observer template
s16b proc_templates_bcode_op [TEMPLATES] [PROC_BCODE_SIZE];
s16b default_mission_bcode_op [CLIENT_BCODE_SIZE]; // externed in t_files.c
s16b default_mission_bcode_ob [CLIENT_BCODE_SIZE]; // externed in t_files.c
s16b default_mission_bcode_de [CLIENT_BCODE_SIZE]; // externed in t_files.c
s16b default_mission_bcode_proc [TEMPLATES_PER_PLAYER] [PROC_BCODE_SIZE]; // same

void set_template_box_locations(void);
void update_template_display(void);
void templates_input(void);
void set_empty_template(int t, int type, int player, int fixed);
void press_templ_button(int t, int b);
void press_templ_button_with_rmb(int t, int b);
void open_template_import_list(int t);
void press_turnfile_button(int p, int tbutton);
void copy_from_default_template(int t, int tc_source);
void set_clear_button_if_template_loaded(int t);

void add_proc_templates_for_player(int templ_type, int p, int fixed);

// call at startup only
// this needs to closely match the editor init functions because the message log is used for both.
// must be called after init_editor
void init_templates(void)
{


 tstate.panel_x = settings.editor_x_split;
 tstate.panel_y = 0;
 tstate.panel_w = settings.option [OPTION_WINDOW_W] - settings.editor_x_split;
 tstate.panel_h = settings.option [OPTION_WINDOW_H];
 tstate.template_panel_h = settings.option [OPTION_WINDOW_H] - LOG_WINDOW_H - TEMPLATE_PANEL_HEADER_H - 10; // this is the part that's visible
 tstate.use_scrollbar = 0;
 tstate.window_pos = 0;
 tstate.templ_highlight = -1;
 tstate.button_highlight = -1;
 tstate.turnfile_button_highlight_player = -1;
 tstate.turnfile_button_highlight_button = -1;

 reset_template_x_values();

// tstate.bmp = editor.sub_bmp;
// tstate.log_sub_bmp = editor.log_sub_bmp;

/* tstate.templ_sub_bmp = al_create_sub_bitmap(tstate.bmp, 0, TEMPLATE_PANEL_HEADER_H, tstate.panel_w - SLIDER_BUTTON_SIZE, tstate.template_panel_h);

 if (tstate.templ_sub_bmp == NULL)
 {
  fprintf(stdout, "\nError: t_template.c: init_templates(): couldn't create templ_sub_bitmap");
  error_call();
 }*/

 set_template_box_locations();

 tstate.import_list_open = 0;

 initialise_default_mission_templates();
 reset_log();

 reset_templates();

}

// Use this when the panel changes width (or is initialised)
// Don't need to call when panel is opened/closed altogether
void reset_template_x_values(void)
{

 tstate.tx1 = settings.editor_x_split + TEMPLATE_LEFT_MARGIN;
 tstate.tx2 = tstate.tx1 + TEMPLATE_BOX_W;

 tstate.button_x1 = tstate.tx2 - TEMPL_BUTTON_W - TEMPL_BUTTON_X_SPACE;
 tstate.button_x2 = tstate.button_x1 + TEMPL_BUTTON_W;

 tstate.turnfile_button_x1 [1] = tstate.tx2 - TURNFILEBUTTON_W;
 tstate.turnfile_button_x2 [1] = tstate.turnfile_button_x1 [1] + TURNFILEBUTTON_W;
 tstate.turnfile_button_x1 [0] = tstate.turnfile_button_x1 [1] - TURNFILEBUTTON_W - TURNFILEBUTTON_SEPARATION;
 tstate.turnfile_button_x2 [0] = tstate.turnfile_button_x1 [0] + TURNFILEBUTTON_W;



}

// this function resets all templates, either to clear them (e.g. at startup) or just before they start being filled in
void reset_templates(void)
{

 int i;

 for (i = 0; i < TEMPLATES; i ++)

 {
  templ[i].type = TEMPL_TYPE_CLOSED;
//  templ[i].bcode.note = NULL; // need to free this memory if there's any possibility a template's bcodestruct might have an active note struct (currently not possible), to avoid a memory leak
  templ[i].contents.file_name [0] = '\0';
  templ[i].contents.file_path [0] = '\0';
  clear_template(i);

  templ[i].highlight = 0;
  templ[i].contents.access_index = -1;
  templ[i].buttons = 0;
  templ[i].contents.loaded = 0;
  templ[i].fixed = 0;
  templ[i].category_heading = 0;
 }

 tstate.window_pos = 0;
 tstate.use_scrollbar = 0;
 tstate.templ_setup_counter = 0;
 tstate.templ_setup_y = 20; // 20?

}

// call this function while assembling a template list based on a world_init struct (see code in s_setup.c)
// type is e.g. TEMPL_TYPE_PROC
// player is player who owns this proc (-1 if no player)
// fixed is 1 or 0 (whether user can change it)
// this function will update tstate.templ_setup_counter and tstate.templ_setup_y
int add_template(int type, int player, int fixed, int access_index, int category_heading)
{

 int i = tstate.templ_setup_counter;

 if (i >= TEMPLATES)
 {
  fprintf(stdout, "\nError: t_template.c: add_template(): too many templates: %i (new template params %i, %i, %i).", i, type, player, fixed);
  error_call();
 }

 set_empty_template(i, type, player, fixed);

 templ[i].player = player;
 templ[i].contents.access_index = access_index;

// fprintf(stdout, "\nTemplate %i, type %i, pl %i, ai %i", i, type, player, access_index);

// now work out where the template will be in the template window:
// int x = settings.editor_x_split + 5;
 int j;

 templ[i].category_heading = category_heading;

 if (category_heading)
 {
  tstate.templ_setup_y += TEMPLATE_CAT_HEADING_H;
 }

// templ[i].x1 = x;
 templ[i].y1 = tstate.templ_setup_y;
// templ[i].x2 = x + TEMPLATE_BOX_W;
 templ[i].y2 = templ[i].y1 + TEMPLATE_BOX_H;

 for (j = 0; j < TEMPL_BUTTONS; j ++)
 {
//  templ[i].button_x1 [j] = templ[i].x2 - TEMPL_BUTTON_W - TEMPL_BUTTON_X_SPACE;
  templ[i].button_y1 [j] = templ[i].y1 + TEMPL_BUTTON_Y_SPACE + (TEMPL_BUTTON_Y_SPACE + TEMPL_BUTTON_H) * j;
//  templ[i].button_x2 [j] = templ[i].button_x1 [j] + TEMPL_BUTTON_W;
  templ[i].button_y2 [j] = templ[i].button_y1 [j] + TEMPL_BUTTON_H;
 }

 tstate.templ_setup_y += TEMPLATE_BOX_H + 5;
 tstate.templ_setup_counter ++;

 return i;

}

// call this function when finished calling add_template.
// It is called both when starting from a menu (from s_menu.c), and also when starting after loading a game from disk (from f_load.c)
void finish_template_setup(void)
{

 tstate.templates_available = tstate.templ_setup_counter;

 tstate.h = tstate.templ_setup_y;

 if (tstate.h > tstate.template_panel_h)
 {
   tstate.use_scrollbar = 1;
   init_slider(&tstate.tscrollbar, // *sl
               &tstate.window_pos, // *value_pointer
               SLIDEDIR_VERTICAL, //dir
               0, // value_min
               tstate.h - tstate.template_panel_h, // value_max
               tstate.template_panel_h, // total_length
               9, // button_increment
               80, // track_increment
               tstate.template_panel_h, // slider_represents_size
               settings.option [OPTION_WINDOW_W] - SLIDER_BUTTON_SIZE, // x
               TEMPLATE_PANEL_HEADER_H, // y
               SLIDER_BUTTON_SIZE, // thickness
               COL_BLUE, // colour
               0); // hidden_if_unused

 }

// now set up some overall bounding box values
// we assume that all templates and template buttons are in a vertical column, so the x values for template 0 can be used for all:
// tstate.button_x1 = templ[0].button_x1 [0];
// tstate.button_x2 = templ[0].button_x2 [0];
 tstate.box_y1 = templ[0].y1;

// now set up turnfile button location values:
 int t, p;

 for (p = 0; p < PLAYERS; p ++)
 {
  tstate.turnfile_buttons [p] = 0;
 }

 for (t = 0; t < tstate.templates_available; t ++)
 {
  if (templ[t].category_heading
   && (templ[t].type == TEMPL_TYPE_OPERATOR
				|| templ[t].type == TEMPL_TYPE_DEFAULT_OPERATOR
    || templ[t].type == TEMPL_TYPE_DELEGATE
    || templ[t].type == TEMPL_TYPE_PROC
    || templ[t].type == TEMPL_TYPE_DEFAULT_PROC))
  {
   p = templ[t].player;
#ifdef SANITY_CHECK
 if (p < 0 || p >= PLAYERS)
 {
  fprintf(stdout, "\nError: t_template.c: finish_template_setup(): invalid player index %i.", p);
  error_call();
 }
#endif
   tstate.turnfile_buttons [p] = 1;
   tstate.turnfile_button_y1 [p] = templ[t].y1 - TURNFILEBUTTON_UP;
   tstate.turnfile_button_y2 [p] = tstate.turnfile_button_y1 [p] + TURNFILEBUTTON_H;

  }


 }

 assign_template_references();

}

// template references allow in-game programs to find a template of a particular type
void assign_template_references(void)
{


// finally, need to set up the w.template_reference array, which relates various types of template to the relevant templ array index:
 int i, t;

 for (i = 0; i < FIND_TEMPLATES; i ++)
 {
  w.template_reference [i] = -1; // indicates no template of this type
 }

 for (t = 0; t < TEMPLATES; t ++)
 {
  switch(templ[t].type)
  {
   case TEMPL_TYPE_OBSERVER:
    w.template_reference [FIND_TEMPLATE_OBSERVER] = t;
    break;
   case TEMPL_TYPE_OPERATOR:
   case TEMPL_TYPE_DEFAULT_OPERATOR:
   case TEMPL_TYPE_DELEGATE:
    w.template_reference [FIND_TEMPLATE_P0_CLIENT + templ[t].player] = t;
    break;
   case TEMPL_TYPE_PROC:
   case TEMPL_TYPE_DEFAULT_PROC:
    switch(templ[t].player)
    {
     case 0:
      w.template_reference [FIND_TEMPLATE_P0_PROC_0 + templ[t].contents.access_index] = t;
      break;
     case 1:
      w.template_reference [FIND_TEMPLATE_P1_PROC_0 + templ[t].contents.access_index] = t;
      break;
     case 2:
      w.template_reference [FIND_TEMPLATE_P2_PROC_0 + templ[t].contents.access_index] = t;
      break;
     case 3:
      w.template_reference [FIND_TEMPLATE_P3_PROC_0 + templ[t].contents.access_index] = t;
      break;
    }
    break;
  } // end templ[].type switch
 } // end for t loop
/*
 fprintf(stdout, "\nTemplate reference: ");
 for (i = 0; i < FIND_TEMPLATES; i ++)
 {
  fprintf(stdout, "\n%i: %i", i, w.template_reference [i]);
 }
*/

}

/*

How will template types be arranged?

There does not need to be a template for the gamefile. It is dealt with separately (necessary because the gamefile determines the general template structure).
Each player will need:
 - a client program
  - however there will be an option to turn this off (so processes have to survive on their own)
 - 4(?) process templates
  - the first of which will be placed in the world at the start.

 - players 2-4 can only have client programs
 - there are options for player 1:
  - operator program - combines client and observer; can communicate with procs
  - client only
  - no client at all

 - if player 1 doesn't have an operator program, there is also an observer template


The first element of each program's interface will need to be one of the following:
PROGRAM_TYPE_PROCESS,
PROGRAM_TYPE_CLIENT,
PROGRAM_TYPE_OPERATOR,
PROGRAM_TYPE_OBSERVER,
PROGRAM_TYPE_SYSTEM

Only the correct type will be usable for any particular template (exception: an observer or client may substitute for an operator)
 - as standard, each type will have a prefix (not enforced): pr_, cl_, op_, ob_, sy_

How will the system be able to use templates?
 - must make use of one of the player sets
 - a system file will be able to claim one (or more) of the players for itself, and load its own subprocesses into the templates

How will templates be displayed once a system program has set them up?



What will be the default, in the absence of a system program?
 - there will be settings for:
  - player clients (if 0, no player has a client)
  - user-player operator (if 0, user-player has a client unless player clients is 0)
 - there will always be an observer template unless there's an operator
  - if there's an operator template but no operator (e.g. not loaded in, or a client loaded instead), load a default basic observer
   - also load default basic observer if no observer set.

So will have:
 players x 4: proc templates (first of which is first proc)
 players x 1: client templates (player 1's is operator if set)
 1: observer (if no operator)
Need to put headings in between template groups, so:

Observer
 - obs template box
Player 1
 - operator template box
 - first proc template box
 - proc template box
 - proc template box
 - proc template box
Player 2
 - client template box
 - first proc template box
 - proc template box
 - proc template box
 - proc template box

if something not present, just have a grey line: not enabled (or something)

Default (if no system program)
 Observer
 client + 4 procs for each player present


*/

// sets up single system file template, for the start menu
void setup_system_template(void)
{

// first clear all templates and prepare to start a new set
 reset_templates();

// now add gamefile template:
 add_template(TEMPL_TYPE_SYSTEM, -1, 0, -1, 1); // parameters are type, player, fixed, access_index, category_heading

 finish_template_setup();

}


// This function sets up templates based on w_init settings
// it should be called just before entering the pregame phase, as it needs to set up templates for the system program to load things into (and then for the user to interact with during first interturn phase)
void setup_templates_for_game_start(void)
{

// first clear all templates and prepare to start a new set
 reset_templates();

 int templ_fixed = 0;

 if (!w_init.allow_observer)
  templ_fixed = 1;

 add_template(TEMPL_TYPE_OBSERVER, -1, templ_fixed, -1, 1); // parameters are type, player, fixed, access_index, category_heading

// now, for each player add client + 4 proc templates:
 int i;

 for (i = 0; i < w_init.players; i ++)
 {
  if (w_init.player_clients)
  {
   templ_fixed = 1;
   if (w_init.may_change_client_template [i])
    templ_fixed = 0;
   if (i == w_init.player_operator) // player_operator is -1 if no operator
    add_template(TEMPL_TYPE_OPERATOR, i, templ_fixed, -1, 1);
     else
      add_template(TEMPL_TYPE_DELEGATE, i, templ_fixed, -1, 1);
  }
  templ_fixed = 1;
  if (w_init.may_change_proc_templates [i])
   templ_fixed = 0;
  add_proc_templates_for_player(TEMPL_TYPE_PROC, i, templ_fixed);
 }

 finish_template_setup();


}

// Just makes empty templates
void setup_templates_for_default_template_init(void)
{

 reset_templates();
 add_template(TEMPL_TYPE_DEFAULT_OPERATOR, 0, 0, -1, 1);
 add_template(TEMPL_TYPE_DEFAULT_OBSERVER, -1, 0, -1, 0);
 add_template(TEMPL_TYPE_DEFAULT_DELEGATE, 0, 0, -1, 0);
 add_proc_templates_for_player(TEMPL_TYPE_DEFAULT_PROC, 0, 0);

}


// Sets up templates for player 0 to load default programs into
void setup_templates_for_mission_menu(void)
{

// first clear all templates and prepare to start a new set
 reset_templates();

// add_template(TEMPL_TYPE_OBSERVER, -1, templ_fixed, -1, 1); // parameters are type, player, fixed, access_index, category_heading
 add_template(TEMPL_TYPE_DEFAULT_OPERATOR, 0, 0, -1, 1);
 copy_from_default_template(0, TEMPL_MISSION_DEFAULT_OPERATOR);
 add_proc_templates_for_player(TEMPL_TYPE_DEFAULT_PROC, 0, 0);
 copy_from_default_template(1, TEMPL_MISSION_DEFAULT_PROC0);
 templ[1].contents.access_index = 0;
 copy_from_default_template(2, TEMPL_MISSION_DEFAULT_PROC1);
 templ[2].contents.access_index = 1;
 copy_from_default_template(3, TEMPL_MISSION_DEFAULT_PROC2);
 templ[3].contents.access_index = 2;
 copy_from_default_template(4, TEMPL_MISSION_DEFAULT_PROC3);
 templ[4].contents.access_index = 3;

 finish_template_setup();

}



// Sets up templates for player 0 to load default programs into
void setup_templates_for_advanced_mission_menu(void)
{

// first clear all templates and prepare to start a new set
 reset_templates();

// add_template(TEMPL_TYPE_OBSERVER, -1, templ_fixed, -1, 1); // parameters are type, player, fixed, access_index, category_heading
 add_template(TEMPL_TYPE_DEFAULT_OBSERVER, 0, 0, -1, 1);
 copy_from_default_template(0, TEMPL_MISSION_DEFAULT_OBSERVER);
 add_template(TEMPL_TYPE_DEFAULT_DELEGATE, 0, 0, -1, 1);
 copy_from_default_template(1, TEMPL_MISSION_DEFAULT_DELEGATE);
 add_proc_templates_for_player(TEMPL_TYPE_DEFAULT_PROC, 0, 0);
 copy_from_default_template(2, TEMPL_MISSION_DEFAULT_PROC0);
 templ[2].contents.access_index = 0;
 copy_from_default_template(3, TEMPL_MISSION_DEFAULT_PROC1);
 templ[3].contents.access_index = 1;
 copy_from_default_template(4, TEMPL_MISSION_DEFAULT_PROC2);
 templ[4].contents.access_index = 2;
 copy_from_default_template(5, TEMPL_MISSION_DEFAULT_PROC3);
 templ[5].contents.access_index = 3;

 finish_template_setup();

}


/*
// finishes adding player 1's templates
void finish_templates_for_mission(void)
{

// Change the existing process templates from TEMPL_TYPE_DEFAULT_PROC to TEMPL_TYPE_PROC:
	templ[1].type = TEMPL_TYPE_PROC;
	templ[2].type = TEMPL_TYPE_PROC;
	templ[3].type = TEMPL_TYPE_PROC;
	templ[4].type = TEMPL_TYPE_PROC;

 add_template(TEMPL_TYPE_DELEGATE, 1, 1, -1, 1);
 add_proc_templates_for_player(TEMPL_TYPE_PROC, 1, 1);

 finish_template_setup();

}*/

// Call this when leaving the mission screen - it copies the contents of the mission screen templates to the default templates
// Also called at initialisation (in which case both parameters are 1)
void copy_mission_templates_to_default(int copy_standard, int copy_advanced)
{

 int i;
 i = 0;

 if (copy_standard)
	{
  copy_template_contents(&default_mission_template [TEMPL_MISSION_DEFAULT_OPERATOR].contents, &templ[i++].contents);
	}

 if (copy_advanced)
 {
  copy_template_contents(&default_mission_template [TEMPL_MISSION_DEFAULT_OBSERVER].contents, &templ[i++].contents);
  copy_template_contents(&default_mission_template [TEMPL_MISSION_DEFAULT_DELEGATE].contents, &templ[i++].contents);
 }

 copy_template_contents(&default_mission_template [TEMPL_MISSION_DEFAULT_PROC0].contents, &templ[i++].contents);
 copy_template_contents(&default_mission_template [TEMPL_MISSION_DEFAULT_PROC1].contents, &templ[i++].contents);
 copy_template_contents(&default_mission_template [TEMPL_MISSION_DEFAULT_PROC2].contents, &templ[i++].contents);
 copy_template_contents(&default_mission_template [TEMPL_MISSION_DEFAULT_PROC3].contents, &templ[i++].contents);

}


// Call this when leaving the mission screen - it copies the contents of the mission screen templates to the default templates
void copy_default_mission_templates_to_actual_templates(int starting_from_template, int advanced)
{

 if (!advanced)
	{
		copy_from_default_template(starting_from_template++, TEMPL_MISSION_DEFAULT_OPERATOR);
	}
	 else
		{
 		copy_from_default_template(starting_from_template++, TEMPL_MISSION_DEFAULT_OBSERVER);
 		copy_from_default_template(starting_from_template++, TEMPL_MISSION_DEFAULT_DELEGATE);
		}
	copy_from_default_template(starting_from_template++, TEMPL_MISSION_DEFAULT_PROC0);
	copy_from_default_template(starting_from_template++, TEMPL_MISSION_DEFAULT_PROC1);
	copy_from_default_template(starting_from_template++, TEMPL_MISSION_DEFAULT_PROC2);
	copy_from_default_template(starting_from_template++, TEMPL_MISSION_DEFAULT_PROC3);

 assign_template_references();

}

void copy_from_default_template(int t, int tc_source)
{

 copy_template_contents(&templ[t].contents, &default_mission_template[tc_source].contents);
 set_clear_button_if_template_loaded(t);

}

// Copies the content structure of one template to another
void copy_template_contents(struct templ_contentstruct* tc_target, struct templ_contentstruct* tc_source)
{

// First save a copy of tc_target's bcode op pointer:
 s16b* tc_target_op = tc_target->bcode.op;

// Copy the contents:
 *tc_target = *tc_source;

// reassign tc_target's op pointer:
 tc_target->bcode.op = tc_target_op;

// copy the contents of tc_source's bcode op array:
#ifdef SANITY_CHECK
 if (tc_source->bcode.bcode_size != tc_target->bcode.bcode_size)
	{
		fprintf(stdout, "\nError: t_template.c: copy_template_contents(): mismatched bcode size (%i vs %i).", tc_source->bcode.bcode_size, tc_target->bcode.bcode_size);
		error_call();
	}
#endif

 int i;

 for (i = 0; i < tc_source->bcode.bcode_size; i ++)
	{
		tc_target->bcode.op [i] = tc_source->bcode.op [i];
	}

}

void set_clear_button_if_template_loaded(int t)
{

 if (templ[t].contents.loaded == 1)
	{
  templ[t].buttons = 1;
  templ[t].button_type [0] = TEMPL_BUTTON_CLEAR;
	}

}

void add_proc_templates_for_player(int templ_type, int p, int fixed)
{

	int i;

 for (i = 0; i < TEMPLATES_PER_PLAYER; i ++)
 {
  add_template(templ_type, p, fixed, i, 0);
 }

}


// this sets up a template as an empty template that can be loaded into. Different to clear_template
// can be client or proc template.
void set_empty_template(int t, int type, int player,  int fixed)
{

 templ[t].type = type;

 switch(type)
 {
  case TEMPL_TYPE_SYSTEM:
   templ[t].contents.bcode.op = system_template_bcode_op;
   templ[t].contents.bcode.bcode_size = SYSTEM_BCODE_SIZE;
   break;
  case TEMPL_TYPE_PROC:
		case TEMPL_TYPE_DEFAULT_PROC:
   templ[t].contents.bcode.op = proc_templates_bcode_op [t];
   templ[t].contents.bcode.bcode_size = PROC_BCODE_SIZE;
   break;
  case TEMPL_TYPE_OBSERVER:
  case TEMPL_TYPE_DEFAULT_OBSERVER:
   templ[t].contents.bcode.op = client_templates_bcode_op [PLAYERS]; // index PLAYERS is reserved for observer
   templ[t].contents.bcode.bcode_size = CLIENT_BCODE_SIZE;
   break;
  case TEMPL_TYPE_DELEGATE:
  case TEMPL_TYPE_DEFAULT_DELEGATE:
  case TEMPL_TYPE_OPERATOR:
		case TEMPL_TYPE_DEFAULT_OPERATOR:
   templ[t].contents.bcode.op = client_templates_bcode_op [player];
   templ[t].contents.bcode.bcode_size = CLIENT_BCODE_SIZE;
   break;
 }

// fprintf(stdout, "\nset_empty_template: type %i bcode_size %i", type, templ[t].bcode.bcode_size);

 templ[t].contents.origin = TEMPL_ORIGIN_NONE;
 templ[t].contents.loaded = 0;
 templ[t].fixed = 0;

 strcpy(templ[t].contents.file_name, "Empty");

 templ[t].buttons = 2;
 templ[t].button_type [0] = TEMPL_BUTTON_OPEN_FILE;
 templ[t].button_type [1] = TEMPL_BUTTON_IMPORT;

 if (fixed)
 {
  templ[t].buttons = 0;
  templ[t].fixed = 1;
 }

}

void set_opened_template(int t, int type, int fixed)
{

 templ[t].type = type;
 templ[t].contents.loaded = 1;
 templ[t].fixed = fixed;

 templ[t].buttons = 1;
 templ[t].button_type [0] = TEMPL_BUTTON_CLEAR;
// templ[t].button_type [1] = TEMPL_BUTTON_REFRESH ????

 if (fixed)
  templ[t].buttons = 0;

}

// just sets a template to empty without deleting it or configuring it otherwise.
// use to clear a template, or when open or import fails
void clear_template(int t)
{

// templ[t].type = templ_type;
 templ[t].contents.origin = TEMPL_ORIGIN_NONE;
 templ[t].contents.loaded = 0;

// default to the appropriate proc bcode (NO - don't do this. Should do it when setting up the template in the first place)
// templ[t].bcode.op = proc_templates_bcode_op [t];
// templ[t].bcode.bcode_size = PROC_BCODE_SIZE;

 strcpy(templ[t].contents.file_name, "Error?"); // should not be displayed
// strcpy(templ[t].templ_name, "Error?");

}


// this function is called from various loops whenever the template window is up.
void run_templates(void)
{

//  if (game.input_capture == INPUT_EDITOR)

  templates_input();

  draw_template_bmp();

}

void templates_input(void)
{

 int i, j;
 int printed_help = 0; // set to 1 if a help string printed because of a right mouse button press (to avoid printing multiple strings when a button on a template is clicked)

 tstate.templ_highlight = -1;
 tstate.button_highlight = -1;
 tstate.list_highlight = -1;
 tstate.turnfile_button_highlight_button = -1;
 tstate.turnfile_button_highlight_player = -1;

 // check for the mouse pointer being in the game window:
 if (ex_control.mouse_x_pixels < settings.editor_x_split)
 {
  if (settings.keyboard_capture == INPUT_EDITOR
   && ex_control.mb_press [0] == BUTTON_JUST_PRESSED)
  {
   initialise_control();
   settings.keyboard_capture = INPUT_WORLD;
  }
// needs work - see also equivalent in e_editor.c and i_sysmenu.c
  return;
 }

 run_slider(&mlog.scrollbar_v, 0, 0);
 if (tstate.use_scrollbar)
  run_slider(&tstate.tscrollbar, 0, 0);

 int mouse_x = ex_control.mouse_x_pixels;// - settings.editor_x_split;
 int mouse_y = ex_control.mouse_y_pixels; // this is offset by tstate.window_pos and TEMPLATE_PANEL_HEADER_H after the import list has been checked
 int just_pressed = (ex_control.mb_press [0] == BUTTON_JUST_PRESSED);
 int just_pressed_rmb = (ex_control.mb_press [1] == BUTTON_JUST_PRESSED);



 if (tstate.import_list_open) // note that import list is not affected by tstate.window_pos
 {
  if (mouse_x < IMPORT_LIST_X
   || mouse_x > IMPORT_LIST_X + IMPORT_LIST_W
   || mouse_y < IMPORT_LIST_Y - IMPORT_LIST_HEADER_SIZE
   || mouse_y > IMPORT_LIST_Y + IMPORT_LIST_H)
  {
   if (just_pressed)
    tstate.import_list_open = 0;
   return;
  }

  int list_item = (mouse_y - IMPORT_LIST_Y) / IMPORT_LIST_LINE_H;

  if (list_item < 0
   || list_item >= ESOURCES)
    return;

  if (tstate.import_list[list_item].open == 0)
   return;

  tstate.list_highlight = list_item;

  if (just_pressed)
  {
   import_template_from_list(list_item, tstate.list_t);
   tstate.import_list_open = 0;
  }

  return;

 }


 int mouse_in_template = -1;

 mouse_y += tstate.window_pos;// - TEMPLATE_PANEL_HEADER_H;

 if (mouse_x >= tstate.tx1
  && mouse_x <= tstate.tx2
  && mouse_y >= tstate.box_y1)
 {
  for (i = 0; i < TEMPLATES; i ++)
  {
   if (templ[i].type != TEMPL_TYPE_CLOSED
    && mouse_y >= templ[i].y1
    && mouse_y <= templ[i].y2)
   {
    mouse_in_template = i;
    if (templ[i].fixed == 0)
     tstate.templ_highlight = mouse_in_template;
    break;
   }
  }
// check for the mouse being in the button column:
  if (mouse_in_template != -1
   && mouse_x >= tstate.button_x1
   && mouse_x <= tstate.button_x2)
  {
   for (j = 0; j < templ[mouse_in_template].buttons; j ++)
   {
    if (mouse_y >= templ[mouse_in_template].button_y1 [j]
     && mouse_y <= templ[mouse_in_template].button_y2 [j])
    {
     tstate.button_highlight = j;
     if (just_pressed_rmb)
					{
						press_templ_button_with_rmb(mouse_in_template, j);
						printed_help = 1;
						break;
					}
     if (just_pressed)
     {
      press_templ_button(mouse_in_template, j);
      break;
     }
    }
   }
  }
 }

 if (mouse_x >= tstate.turnfile_button_x1 [0]
  && mouse_x <= tstate.turnfile_button_x2 [1])
 {
  for (i = 0; i < PLAYERS; i ++)
  {
   if (tstate.turnfile_buttons [i] == 0)
    continue;
   if (mouse_y >= tstate.turnfile_button_y1 [i]
    && mouse_y <= tstate.turnfile_button_y2 [i])
   {
    int tbutton = -1;
    if (mouse_x <= tstate.turnfile_button_x2 [0])
     tbutton = 0;
    if (mouse_x >= tstate.turnfile_button_x1 [1])
     tbutton = 1;
    if (tbutton == -1)
     break;
    tstate.turnfile_button_highlight_button = tbutton;
    tstate.turnfile_button_highlight_player = i;
    if (just_pressed_rmb)
				{
					print_help(HELP_TURNFILE_BUTTON);
					printed_help = 1;
				}
    if (just_pressed)
    {
     press_turnfile_button(i, tbutton);
    }
    break;
   }
  }
 }

// number of lines scrolled by moving the mousewheel:
#define TEMPLATE_MOUSEWHEEL_SPEED 48

// check for mousewheel movement:
 if (tstate.use_scrollbar)
 {
  if (ex_control.mousewheel_change == 1)
  {
   tstate.window_pos += TEMPLATE_MOUSEWHEEL_SPEED;
   if (tstate.window_pos > tstate.h - tstate.template_panel_h)
    tstate.window_pos = tstate.h - tstate.template_panel_h;
   slider_moved_to_value(&tstate.tscrollbar, tstate.window_pos);
  }
  if (ex_control.mousewheel_change == -1)
  {
   tstate.window_pos -= TEMPLATE_MOUSEWHEEL_SPEED;
   if (tstate.window_pos < 0)
    tstate.window_pos = 0;
   slider_moved_to_value(&tstate.tscrollbar, tstate.window_pos);
  }
 }

 if (just_pressed_rmb == 1
		&&	printed_help == 0
		&& mouse_in_template != -1)
	{
		switch(templ[mouse_in_template].type)
		{
		 case TEMPL_TYPE_PROC:
			 print_help(HELP_TEMPL_PROC); break;
 		case TEMPL_TYPE_DEFAULT_PROC:
			 print_help(HELP_TEMPL_DEFAULT_PROC); break;
		 case TEMPL_TYPE_SYSTEM:
			 print_help(HELP_TEMPL_SYSTEM); break;
		 case TEMPL_TYPE_OPERATOR:
			 print_help(HELP_TEMPL_OPERATOR); break;
		 case TEMPL_TYPE_DEFAULT_OPERATOR:
			 print_help(HELP_TEMPL_DEFAULT_OPERATOR); break;
		 case TEMPL_TYPE_DELEGATE:
			 print_help(HELP_TEMPL_DELEGATE); break;
		 case TEMPL_TYPE_OBSERVER:
			 print_help(HELP_TEMPL_OBSERVER); break;
		 case TEMPL_TYPE_DEFAULT_OBSERVER:
			 print_help(HELP_TEMPL_DEFAULT_OBSERVER); break;
		 case TEMPL_TYPE_DEFAULT_DELEGATE:
			 print_help(HELP_TEMPL_DEFAULT_DELEGATE); break;

		}
	}


// may have returned by this point

}

void press_templ_button(int t, int b)
{

 switch(templ[t].button_type [b])
 {
  case TEMPL_BUTTON_OPEN_FILE:
   if (open_file_into_template(t))
   {
    set_opened_template(t, templ[t].type, 0);
   }
   break;
  case TEMPL_BUTTON_IMPORT:
   open_template_import_list(t);
   break;
  case TEMPL_BUTTON_CLEAR:
//   fprintf(stdout, "\nCleared %i %i", t, templ[t].type);
   set_empty_template(t, templ[t].type, templ[t].player, 0); // 0 is fixed value - assume that this template can't be fixed if it's being cleared
   break;
 }


}

void press_templ_button_with_rmb(int t, int b)
{

 switch(templ[t].button_type [b])
 {
  case TEMPL_BUTTON_OPEN_FILE:
  	print_help(HELP_TEMPLATE_OPEN_FILE);
   break;
  case TEMPL_BUTTON_IMPORT:
  	print_help(HELP_TEMPLATE_IMPORT);
   break;
  case TEMPL_BUTTON_CLEAR:
  	print_help(HELP_TEMPLATE_CLEAR);
   break;
 }


}


void press_turnfile_button(int p, int tbutton)
{

 switch(tbutton)
 {
  case 0:
   load_turnfile(p);
   break;
  case 1:
   save_turnfile(p);
   break;
 }


}


void open_template_import_list(int t)
{

 tstate.import_list_open = 1;
 tstate.list_t = t;
 tstate.list_highlight = -1;

 int i;

 for (i = 0; i < ESOURCES; i ++)
 {
  switch(editor.tab_type [i])
  {
   case TAB_TYPE_NONE:
    tstate.import_list[i].open = 0;
    break;
   case TAB_TYPE_SOURCE:
   case TAB_TYPE_BINARY:
    tstate.import_list[i].open = 1;
    strcpy(tstate.import_list[i].name, editor.tab_name [i]);
    tstate.import_list[i].se = &editor.source_edit [editor.tab_index [i]];
    break;
  }
 }

}



// this sets up the screen locations for the template boxes.
// call at startup.
// values are used in both drawing and input.
// values are offsets within the editor window (not absolute to the screen)
void set_template_box_locations(void)
{


 int i, j;
// int x = 5;
 int y = 20;

// tstate.tx1 = x;
// tstate.tx2 = x + TEMPLATE_BOX_W;
// tstate.button_x1 = tstate.tx2 - TEMPL_BUTTON_W - TEMPL_BUTTON_X_SPACE;
// tstate.button_x2 = tstate.tx1 + TEMPL_BUTTON_W;

 for (i = 0; i < TEMPLATES; i ++)
 {
//  templ[i].x1 = x;
  templ[i].y1 = y;
//  templ[i].x2 = x + TEMPLATE_BOX_W;
  templ[i].y2 = y + TEMPLATE_BOX_H;
  for (j = 0; j < TEMPL_BUTTONS; j ++)
  {
//   templ[i].button_x1 [j] = templ[i].x2 - TEMPL_BUTTON_W - TEMPL_BUTTON_X_SPACE;
   templ[i].button_y1 [j] = templ[i].y1 + TEMPL_BUTTON_Y_SPACE + (TEMPL_BUTTON_Y_SPACE + TEMPL_BUTTON_H) * j;
//   templ[i].button_x2 [j] = templ[i].button_x1 [j] + TEMPL_BUTTON_W;
   templ[i].button_y2 [j] = templ[i].button_y1 [j] + TEMPL_BUTTON_H;
  }
  y += TEMPLATE_BOX_H;

 }

// we assume that all buttons are in a vertical column:
// tstate.button_x1 = templ[0].button_x1 [0];
// tstate.button_x2 = templ[0].button_x2 [0];
 tstate.box_y1 = templ[0].y1;

}

void draw_template_bmp(void)
{

 al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA);

 update_template_display();

 if (ex_control.panel_drag_ready == 1
		|| ex_control.mouse_dragging_panel == 1)
		draw_panel_drag_ready_line();



// al_set_target_bitmap(al_get_backbuffer(display));

// al_draw_bitmap(tstate.bmp, tstate.panel_x, tstate.panel_y, 0); // TO DO!!!!: need to treat the editor bitmap as a subbitmap of the display backbuffer, which would let us save this drawing operation

// al_flip_display(); - rely on calling loop to do this

}


void update_template_display(void)
{


 al_set_clipping_rectangle(editor.panel_x, editor.panel_y, editor.panel_w, editor.panel_h);

 al_clear_to_color(colours.base [COL_BLUE] [SHADE_MIN]);
 reset_i_buttons();

 int x, y, i, j, p, shade;
 int x2, y2, x3, y3;

 al_draw_textf(font[FONT_SQUARE_BOLD].fnt, colours.base [COL_GREY] [SHADE_MAX], 5, 2, ALLEGRO_ALIGN_LEFT, "Templates");

// draw scrollbars, if needed (scrollbar must be draw on tstate.bmp, not tstate.templ_sub_bmp):
 if (tstate.use_scrollbar)
 {
  draw_scrollbar(&tstate.tscrollbar, 0, 0);
 }

// al_set_target_bitmap(tstate.templ_sub_bmp);
 al_set_clipping_rectangle(editor.panel_x, editor.panel_y + TEMPLATE_PANEL_HEADER_H, tstate.panel_w - SLIDER_BUTTON_SIZE, tstate.template_panel_h);
// tstate.templ_sub_bmp = al_create_sub_bitmap(tstate.bmp, 0, TEMPLATE_PANEL_HEADER_H, tstate.panel_w - SLIDER_BUTTON_SIZE, tstate.template_panel_h);

 for (i = 0; i < TEMPLATES; i ++)
 {
  if (templ[i].type == TEMPL_TYPE_CLOSED)
   break; // A closed template indicates the end of the list
// may need to draw a category heading above the box:
  if (templ[i].category_heading)
  {
   y = templ[i].y1 - tstate.window_pos - TEMPLATE_CAT_HEADING_H;
   if (y + TEMPLATE_CAT_HEADING_H > 0)
   {
    switch(templ[i].type)
    {
     case TEMPL_TYPE_OBSERVER:
      al_draw_textf(font[FONT_SQUARE_BOLD].fnt, colours.base [COL_GREY] [SHADE_HIGH], tstate.tx1, y + 8, ALLEGRO_ALIGN_LEFT, "Observer"); break;
     case TEMPL_TYPE_OPERATOR:
     case TEMPL_TYPE_DELEGATE:
//     case TEMPL_TYPE_FIRST_PROC:
     case TEMPL_TYPE_PROC:
     case TEMPL_TYPE_DEFAULT_PROC:
      p = templ[i].player;
//      al_draw_textf(bold_font, base_col [COL_GREY] [SHADE_HIGH], templ[i].x1, y + 8, ALLEGRO_ALIGN_LEFT, "Player %i", p + 1);
      al_draw_textf(font[FONT_SQUARE_BOLD].fnt, colours.base [COL_GREY] [SHADE_HIGH], tstate.tx1, y + 8, ALLEGRO_ALIGN_LEFT, "%i. %s", p+1, w.player[p].name);
      if (tstate.turnfile_buttons [p] == 1)
      {
       shade = SHADE_LOW;
       if (tstate.turnfile_button_highlight_button == 0
        && tstate.turnfile_button_highlight_player == p)
         shade = SHADE_MED;

    	add_menu_button(tstate.turnfile_button_x1 [0],
                                templ[i].y1 - tstate.window_pos - TURNFILEBUTTON_UP,
                                tstate.turnfile_button_x2 [0],
                                templ[i].y1 - tstate.window_pos - TURNFILEBUTTON_UP + TURNFILEBUTTON_H,
                                colours.base [COL_BLUE] [shade], MBUTTON_TYPE_SMALL);

/*
       al_draw_filled_rectangle(tstate.turnfile_button_x1 [0],
                                templ[i].y1 - tstate.window_pos - TURNFILEBUTTON_UP,
                                tstate.turnfile_button_x2 [0],
                                templ[i].y1 - tstate.window_pos - TURNFILEBUTTON_UP + TURNFILEBUTTON_H,
                                colours.base [COL_BLUE] [shade]);*/

/*
	      add_menu_string(tstate.turnfile_button_x1 [0] + (TURNFILEBUTTON_W/2), templ[i].y1 - tstate.window_pos - TURNFILEBUTTON_UP + 3,
																							&colours.base [COL_GREY] [SHADE_HIGH],
																							ALLEGRO_ALIGN_LEFT,
																							FONT_SQUARE,
																							"Load turnfile");*/


//       al_draw_textf(font[FONT_SQUARE].fnt, colours.base [COL_GREY] [SHADE_HIGH], tstate.turnfile_button_x1 [0] + (TURNFILEBUTTON_W/2), templ[i].y1 - tstate.window_pos - TURNFILEBUTTON_UP + 3, ALLEGRO_ALIGN_CENTRE, "Load turnfile");
       add_menu_string(tstate.turnfile_button_x1 [0] + (TURNFILEBUTTON_W/2), templ[i].y1 - tstate.window_pos - TURNFILEBUTTON_UP + 4, &colours.base [COL_GREY] [SHADE_HIGH],
																							ALLEGRO_ALIGN_CENTRE, FONT_SQUARE, "Load turnfile");


       shade = SHADE_LOW;
       if (tstate.turnfile_button_highlight_button == 1
        && tstate.turnfile_button_highlight_player == p)
         shade = SHADE_MED;

       add_menu_button(tstate.turnfile_button_x1 [1],
                                templ[i].y1 - tstate.window_pos - TURNFILEBUTTON_UP,
                                tstate.turnfile_button_x2 [1],
                                templ[i].y1 - tstate.window_pos - TURNFILEBUTTON_UP + TURNFILEBUTTON_H,
                                colours.base [COL_BLUE] [shade], MBUTTON_TYPE_SMALL);

//       al_draw_textf(font[FONT_SQUARE].fnt, colours.base [COL_GREY] [SHADE_HIGH], tstate.turnfile_button_x1 [1] + (TURNFILEBUTTON_W/2), templ[i].y1 - tstate.window_pos - TURNFILEBUTTON_UP + 3, ALLEGRO_ALIGN_CENTRE, "Save turnfile");
       add_menu_string(tstate.turnfile_button_x1 [1] + (TURNFILEBUTTON_W/2), templ[i].y1 - tstate.window_pos - TURNFILEBUTTON_UP + 4, &colours.base [COL_GREY] [SHADE_HIGH],
																							ALLEGRO_ALIGN_CENTRE, FONT_SQUARE, "Save turnfile");

      }
// see also turnfile button location setup in finish_template_setup()
      break;
     case TEMPL_TYPE_SYSTEM:
      al_draw_textf(font[FONT_SQUARE_BOLD].fnt, colours.base [COL_GREY] [SHADE_HIGH], tstate.tx1, y + 8, ALLEGRO_ALIGN_LEFT, "System file"); break;
    }
   }
  }
// draw box around template entry
// note: x/y may be invalid here (if used for turnfile buttons) so need to make sure to set them from the templ[] values
  x = tstate.tx1;
  y = templ[i].y1 - tstate.window_pos;
  x2 = tstate.tx2;
  y2 = templ[i].y2 - tstate.window_pos;
  if (y > tstate.template_panel_h)
   break; // not visible, due to scrolling
  if (y2 < 0)
   continue; // not visible, due to scrolling
  int templ_col = COL_BLUE;
  switch (templ[i].type)
  {
   case TEMPL_TYPE_DEFAULT_OBSERVER:
   case TEMPL_TYPE_OBSERVER: templ_col = COL_AQUA; break;
   case TEMPL_TYPE_DEFAULT_OPERATOR:
   case TEMPL_TYPE_OPERATOR: templ_col = COL_TURQUOISE; break;
   case TEMPL_TYPE_DEFAULT_DELEGATE:
   case TEMPL_TYPE_DELEGATE: templ_col = COL_TURQUOISE; break;
   case TEMPL_TYPE_SYSTEM: templ_col = COL_TURQUOISE; break;
  }
  if (tstate.templ_highlight == i)
   add_menu_button(x, y, x2, y2, colours.base [templ_col] [SHADE_MED], MBUTTON_TYPE_TEMPLATE);
    else
				{
					if (templ[i].fixed)
      add_menu_button(x, y, x2, y2, colours.base [COL_DULL] [SHADE_MIN], MBUTTON_TYPE_TEMPLATE);
       else
        add_menu_button(x, y, x2, y2, colours.base [templ_col] [SHADE_LOW], MBUTTON_TYPE_TEMPLATE);
				}

//fprintf(stdout, "\n%i,%i %i,%i %i", x, y, x2, y2, tstate.tx1);

//   al_draw_filled_rectangle(x, y, x2, y2, base_col [COL_BLUE] [SHADE_LOW]);
//  al_draw_rectangle(x, y, x2, y2, base_col [COL_BLUE] [SHADE_MED], 1);

  switch(templ[i].type)
  {
   case TEMPL_TYPE_DEFAULT_OBSERVER:
    sprintf(tstring, "Default mission observer template");
    add_menu_string(x + 3, y + 3, &colours.base [COL_GREY] [SHADE_MED],	ALLEGRO_ALIGN_LEFT, FONT_SQUARE, tstring);
    break;
   case TEMPL_TYPE_OBSERVER:
//    al_draw_textf(font[FONT_SQUARE].fnt, colours.base [COL_GREY] [SHADE_MED], x + 3, y + 3, ALLEGRO_ALIGN_LEFT, "Observer template");
    add_menu_string(x + 3, y + 3, &colours.base [COL_GREY] [SHADE_MED],	ALLEGRO_ALIGN_LEFT, FONT_SQUARE, "Observer template");
    break;
   case TEMPL_TYPE_DEFAULT_OPERATOR:
    sprintf(tstring, "Default mission operator template");
    add_menu_string(x + 3, y + 3, &colours.base [COL_GREY] [SHADE_MED],	ALLEGRO_ALIGN_LEFT, FONT_SQUARE, tstring);
    break;
   case TEMPL_TYPE_OPERATOR:
//    al_draw_textf(font[FONT_SQUARE].fnt, colours.base [COL_GREY] [SHADE_MED], x + 3, y + 3, ALLEGRO_ALIGN_LEFT, "Player %i Operator template", templ[i].player); break;
    sprintf(tstring, "Player %i Operator template", templ[i].player);
    add_menu_string(x + 3, y + 3, &colours.base [COL_GREY] [SHADE_MED],	ALLEGRO_ALIGN_LEFT, FONT_SQUARE, tstring);
    break;
   case TEMPL_TYPE_DEFAULT_DELEGATE:
    sprintf(tstring, "Default mission delegate template");
    add_menu_string(x + 3, y + 3, &colours.base [COL_GREY] [SHADE_MED],	ALLEGRO_ALIGN_LEFT, FONT_SQUARE, tstring);
    break;
   case TEMPL_TYPE_DELEGATE:
//    al_draw_textf(font[FONT_SQUARE].fnt, colours.base [COL_GREY] [SHADE_MED], x + 3, y + 3, ALLEGRO_ALIGN_LEFT, "Player %i Delegate template", templ[i].player); break;
    sprintf(tstring, "Player %i Delegate template", templ[i].player);
    add_menu_string(x + 3, y + 3, &colours.base [COL_GREY] [SHADE_MED],	ALLEGRO_ALIGN_LEFT, FONT_SQUARE, tstring);
    break;
   case TEMPL_TYPE_DEFAULT_PROC:
    sprintf(tstring, "Default mission process template %i", templ[i].contents.access_index);
    add_menu_string(x + 3, y + 3, &colours.base [COL_GREY] [SHADE_MED],	ALLEGRO_ALIGN_LEFT, FONT_SQUARE, tstring);
    break;
   case TEMPL_TYPE_PROC:
//   case TEMPL_TYPE_FIRST_PROC:
//    if (templ[i].access_index == -1)
//     al_draw_textf(font, base_col [COL_GREY] [SHADE_MED], x + 3, y + 3, ALLEGRO_ALIGN_LEFT, "Player %i Process template", templ[i].player); // not sure what to call this - may not actually be needed as procs should probably always be accessible
//      else
//       al_draw_textf(font[FONT_SQUARE].fnt, colours.base [COL_GREY] [SHADE_MED], x + 3, y + 3, ALLEGRO_ALIGN_LEFT, "Player %i Process template %i", templ[i].player, templ[i].access_index);
/*    sprintf(tstring, "Pt %i [%i %i %i %i %i %i %i]", templ[i].player, templ[i].contents.access_index, templ[i].contents.bcode.op [2],
												templ[i].contents.bcode.op [3],
												templ[i].contents.bcode.op [4],
												templ[i].contents.bcode.op [5],
												templ[i].contents.bcode.op [6],
												templ[i].contents.bcode.op [7]);*/
    sprintf(tstring, "Player %i Process template %i", templ[i].player, templ[i].contents.access_index);
    add_menu_string(x + 3, y + 3, &colours.base [COL_GREY] [SHADE_MED],	ALLEGRO_ALIGN_LEFT, FONT_SQUARE, tstring);
    break;
   case TEMPL_TYPE_SYSTEM:
    add_menu_string(x + 3, y + 3, &colours.base [COL_GREY] [SHADE_MED],	ALLEGRO_ALIGN_LEFT, FONT_SQUARE, "System file template");
    break;

//    al_draw_textf(font[FONT_SQUARE].fnt, colours.base [COL_GREY] [SHADE_MED], x + 3, y + 3, ALLEGRO_ALIGN_LEFT, "System file template"); break;
//   case TEMPL_TYPE_FIRST_PROC:
//    al_draw_textf(font, base_col [COL_GREY] [SHADE_MED], x + 3, y + 3, ALLEGRO_ALIGN_LEFT, "Process template 0", templ[i].access_index); break;
  }
// draw template name
  x3 = x + TEMPLATE_NAME_BOX_X;
  y3 = y + TEMPLATE_NAME_BOX_Y;
  if (templ[i].contents.loaded)
  {
   add_menu_button(x3, y3, x3 + TEMPLATE_NAME_BOX_W, y3 + TEMPLATE_NAME_BOX_H, colours.base [COL_GREY] [SHADE_MIN], MBUTTON_TYPE_SMALL);
//   al_draw_rectangle(x3, y3, x3 + TEMPLATE_NAME_BOX_W, y3 + TEMPLATE_NAME_BOX_H, colours.base [COL_GREY] [SHADE_MED], 1);
//   al_draw_textf(font[FONT_SQUARE].fnt, colours.base [COL_GREY] [SHADE_MED], x3 + 3, y3 + 3, ALLEGRO_ALIGN_LEFT, "%s", templ[i].file_name);
   add_menu_string(x3 + 3, y3 + 3, &colours.base [COL_GREY] [SHADE_MED],	ALLEGRO_ALIGN_LEFT, FONT_SQUARE, templ[i].contents.file_name);

  }
   else
   {
    if (!templ[i].fixed)
    {
     add_menu_button(x3, y3, x3 + TEMPLATE_NAME_BOX_W, y3 + TEMPLATE_NAME_BOX_H, colours.base [COL_GREY] [SHADE_MIN], MBUTTON_TYPE_SMALL);
//     al_draw_rectangle(x3, y3, x3 + TEMPLATE_NAME_BOX_W, y3 + TEMPLATE_NAME_BOX_H, colours.base [COL_GREY] [SHADE_LOW], 1);
//     al_draw_textf(font[FONT_SQUARE].fnt, colours.base [COL_GREY] [SHADE_LOW], x3 + 3, y3 + 3, ALLEGRO_ALIGN_LEFT, "Empty");
     add_menu_string(x3 + 3, y3 + 3, &colours.base [COL_GREY] [SHADE_LOW],	ALLEGRO_ALIGN_LEFT, FONT_SQUARE, "Empty");
    }
   }
  y3 += TEMPLATE_NAME_BOX_H + 5;
  if (templ[i].fixed)
  {
//   al_draw_textf(font[FONT_SQUARE].fnt, colours.base [COL_GREY] [SHADE_MED], templ[i].x2 - TEMPL_BUTTON_W - TEMPL_BUTTON_X_SPACE + (TEMPL_BUTTON_W / 2), y + TEMPL_BUTTON_Y_SPACE, ALLEGRO_ALIGN_CENTRE, "Locked");
   add_menu_string(tstate.tx2 - TEMPL_BUTTON_W - TEMPL_BUTTON_X_SPACE + (TEMPL_BUTTON_W / 2), y + TEMPL_BUTTON_Y_SPACE, &colours.base [COL_GREY] [SHADE_MED],	ALLEGRO_ALIGN_CENTRE, FONT_SQUARE, "Locked");

//   al_draw_textf(font[FONT_SQUARE].fnt, colours.base [COL_GREY] [SHADE_MED], x3 + 250, y3 - (TEMPLATE_NAME_BOX_H + 5) + 18, ALLEGRO_ALIGN_LEFT, "Locked");
  }
   else
   {
    switch(templ[i].contents.origin)
    {
     case TEMPL_ORIGIN_NONE:
      break;
//   case TEMPL_ORIGIN_GAME:
//    al_draw_textf(font, base_col [COL_GREY] [SHADE_MED], x3, y3, ALLEGRO_ALIGN_LEFT, "Mandatory");
//    break;
     case TEMPL_ORIGIN_FILE:
     	snprintf(tstring, MENU_STRING_LENGTH-1, "Path: %s", templ[i].contents.file_path);
//      al_draw_textf(font[FONT_SQUARE].fnt, colours.base [COL_GREY] [SHADE_MED], x3, y3, ALLEGRO_ALIGN_LEFT, "Path: %s", templ[i].file_path);
      add_menu_string(x3 + 3, y3, &colours.base [COL_GREY] [SHADE_MED],	ALLEGRO_ALIGN_LEFT, FONT_BASIC, tstring);
      break;
     case TEMPL_ORIGIN_EDITOR:
//      al_draw_textf(font[FONT_SQUARE].fnt, colours.base [COL_GREY] [SHADE_MED], x3, y3, ALLEGRO_ALIGN_LEFT, "Imported from editor");
      add_menu_string(x3 + 3, y3, &colours.base [COL_GREY] [SHADE_MED],	ALLEGRO_ALIGN_LEFT, FONT_BASIC, "Imported from editor");
      break;
     case TEMPL_ORIGIN_SYSTEM:
      add_menu_string(x3 + 3, y3, &colours.base [COL_GREY] [SHADE_MED],	ALLEGRO_ALIGN_LEFT, FONT_BASIC, "Copied from system program");
//      al_draw_textf(font[FONT_SQUARE].fnt, colours.base [COL_GREY] [SHADE_MED], x3, y3, ALLEGRO_ALIGN_LEFT, "Copied from system program");
      break;
     case TEMPL_ORIGIN_CLIENT:
//      al_draw_textf(font[FONT_SQUARE].fnt, colours.base [COL_GREY] [SHADE_MED], x3, y3, ALLEGRO_ALIGN_LEFT, "Copied from client program");
      add_menu_string(x3 + 3, y3, &colours.base [COL_GREY] [SHADE_MED],	ALLEGRO_ALIGN_LEFT, FONT_BASIC, "Copied from client program");
      break;
     case TEMPL_ORIGIN_TURNFILE:
//      al_draw_textf(font[FONT_SQUARE].fnt, colours.base [COL_GREY] [SHADE_MED], x3, y3, ALLEGRO_ALIGN_LEFT, "Copied from client program");
      add_menu_string(x3 + 3, y3, &colours.base [COL_GREY] [SHADE_MED],	ALLEGRO_ALIGN_LEFT, FONT_BASIC, "From turnfile");
      break;
    }
   }

// draw buttons, but only if the template is highlighted (to reduce clutter)

  if (tstate.templ_highlight == i)
	 {
   for (j = 0; j < templ[i].buttons; j ++)
   {
    x = tstate.button_x1;
    y = templ[i].button_y1 [j] - tstate.window_pos;
    x2 = tstate.button_x2;
    y2 = templ[i].button_y2 [j] - tstate.window_pos;
    if (tstate.templ_highlight == i
     && tstate.button_highlight == j)
     add_menu_button(x, y, x2, y2, colours.base [templ_col] [SHADE_MAX], MBUTTON_TYPE_SMALL);
      else
       add_menu_button(x, y, x2, y2, colours.base [templ_col] [SHADE_HIGH], MBUTTON_TYPE_SMALL);
//    al_draw_rectangle(x, y, x2, y2, base_col [templ_col] [SHADE_HIGH], 1);
    switch(templ[i].button_type [j])
    {
     case TEMPL_BUTTON_CLEAR:
//      al_draw_textf(font[FONT_SQUARE].fnt, colours.base [COL_GREY] [SHADE_HIGH], x + (TEMPL_BUTTON_W / 2), y + 3, ALLEGRO_ALIGN_CENTRE, "Clear");
      add_menu_string(x + (TEMPL_BUTTON_W / 2), y + 3, &colours.base [COL_GREY] [SHADE_HIGH],	ALLEGRO_ALIGN_CENTRE, FONT_SQUARE, "Clear");
      break;
     case TEMPL_BUTTON_OPEN_FILE:
//      al_draw_textf(font[FONT_SQUARE].fnt, colours.base [COL_GREY] [SHADE_HIGH], x + (TEMPL_BUTTON_W / 2), y + 3, ALLEGRO_ALIGN_CENTRE, "Load file");
      add_menu_string(x + (TEMPL_BUTTON_W / 2), y + 3, &colours.base [COL_GREY] [SHADE_HIGH],	ALLEGRO_ALIGN_CENTRE, FONT_SQUARE, "Load file");
      break;
     case TEMPL_BUTTON_IMPORT:
//      al_draw_textf(font[FONT_SQUARE].fnt, colours.base [COL_GREY] [SHADE_HIGH], x + (TEMPL_BUTTON_W / 2), y + 3, ALLEGRO_ALIGN_CENTRE, "Import from editor");
      add_menu_string(x + (TEMPL_BUTTON_W / 2), y + 3, &colours.base [COL_GREY] [SHADE_HIGH],	ALLEGRO_ALIGN_CENTRE, FONT_SQUARE, "Import from editor");
      break;
    }
   } // end for j template button loop
	 } // end if templ_highlight
 } // end for i template loop

 draw_menu_buttons();

 display_log(editor.panel_x + EDIT_WINDOW_X, editor.panel_y + editor.mlog_window_y);

// draw message log scrollbar
 al_set_clipping_rectangle(editor.panel_x, editor.panel_y, editor.panel_w, editor.panel_h);
// al_set_target_bitmap(tstate.bmp);
 draw_scrollbar(&mlog.scrollbar_v, 0, 0);

// finally, if the import list is open, draw it: (target bitmap should be tstate.bmp at this point)
 if (tstate.import_list_open)
 {
   x = IMPORT_LIST_X;
   y = IMPORT_LIST_Y; // not affected by window_pos
   x2 = IMPORT_LIST_X + IMPORT_LIST_W;
   y2 = IMPORT_LIST_Y + IMPORT_LIST_H;
   al_draw_filled_rectangle(x, y - IMPORT_LIST_HEADER_SIZE, x2, y2, colours.base [COL_BLUE] [SHADE_LOW]);
   al_draw_filled_rectangle(x, y - IMPORT_LIST_HEADER_SIZE, x2, y, colours.base [COL_BLUE] [SHADE_HIGH]);
   al_draw_rectangle(x, y - IMPORT_LIST_HEADER_SIZE, x2, y2, colours.base [COL_BLUE] [SHADE_MED], 1);
   al_draw_textf(font[FONT_SQUARE].fnt, colours.base [COL_BLUE] [SHADE_LOW], x + 10, y - IMPORT_LIST_HEADER_SIZE + 6, ALLEGRO_ALIGN_LEFT, "Choose file to import from editor");

   for (i = 0; i < ESOURCES; i ++)
   {
    y = IMPORT_LIST_Y + (IMPORT_LIST_LINE_H * i);
    if (tstate.list_highlight == i)
     al_draw_filled_rectangle(x + 1, y, x2 - 1, y + IMPORT_LIST_LINE_H, colours.base [COL_BLUE] [SHADE_MED]);
    if (tstate.import_list[i].open)
     al_draw_textf(font[FONT_SQUARE].fnt, colours.base [COL_BLUE] [SHADE_MAX], x + 5, y + 6, ALLEGRO_ALIGN_LEFT, "%s", tstate.import_list[i].name);
      else
       al_draw_textf(font[FONT_SQUARE].fnt, colours.base [COL_BLUE] [SHADE_MED], x + 5, y + 6, ALLEGRO_ALIGN_LEFT, "Empty");
   }

 }

}

// if there is a gamefile template with a game program loaded in, returns templ index of first one found
// if none found (or if an empty gamefile template found), returns -1
int find_loaded_gamefile_template(void)
{

 int i;

 for (i = 0; i < TEMPLATES; i ++)
 {
  if (templ[i].type == TEMPL_TYPE_SYSTEM
   && templ[i].contents.loaded)
   return i;
 }

 return -1;
}

/*
// returns index of template - e.g. find_template_access_index(1, 2) finds template index of player 1's second proc
// returns -1 if not found
// doesn't test whether the template has anything in it
int find_template_access_index(int player_index, int templ_access_index)
{

 int t;

 for (t = 0; t < TEMPLATES; t ++)
 {
  if (templ[t].player == player_index
   && templ[t].type == TEMPL_TYPE_PROC
   && templ[t].access_index == templ_access_index)
    return t;
 }

 return -1;

}
*/

// can call this anytime the template window should be open (even if it's already open)
void open_templates(void)
{

 settings.edit_window = EDIT_WINDOW_TEMPLATES;
 settings.keyboard_capture = INPUT_EDITOR;

}

void close_templates(void)
{

// editor.submenu_open = -1;

 tstate.import_list_open = 0;

 settings.edit_window = EDIT_WINDOW_CLOSED;
 settings.keyboard_capture = INPUT_WORLD;

}




