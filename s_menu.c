/*

This file contains code for the setup menu (which lets the user setup a world with whatever settings are wanted).

Basically it sets up interface elements that are then used by code in s_menu.c to display a menu and deal with input from it.
s_menu.c calls back here for various things.

*/

#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_native_dialog.h>

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
#include "e_help.h"
#include "g_game.h"

#include "c_init.h"
#include "i_input.h"
#include "i_view.h"
#include "i_display.h"
#include "i_buttons.h"
#include "t_template.h"
#include "m_input.h"
#include "f_load.h"
#include "f_game.h"
#include "x_sound.h"

#include "s_setup.h"
#include "s_mission.h"


//#include "s_setup.h"

extern struct fontstruct font [FONTS];
extern ALLEGRO_DISPLAY* display;

// these queues are declared in g_game.c. They're externed here so that they can be flushed when the editor does something slow.
extern ALLEGRO_EVENT_QUEUE* event_queue; // these queues are initialised in main.c
extern ALLEGRO_EVENT_QUEUE* fps_queue;

extern struct gamestruct game; // in g_game.c
extern struct viewstruct view;

char mstring [MENU_STRING_LENGTH];



#define ELEMENT_NAME_SIZE 30

enum
{
EL_TYPE_ACTION, // does something (e.g. move to another menu) when clicked.
EL_TYPE_SLIDER, // has a slider
EL_TYPE_COLOUR, // gives a set of colours to choose from
EL_TYPE_HEADING, // does nothing and can't be selected
EL_TYPES
};

enum // these are particular sliders used in particular elements.
{
EL_SLIDER_PLAYERS,
EL_SLIDER_PROCS,
EL_SLIDER_TURNS,
EL_SLIDER_MINUTES,
//EL_SLIDER_GEN_LIMIT,
EL_SLIDER_W_BLOCK,
EL_SLIDER_H_BLOCK,
EL_SLIDERS
};

enum
{
// start menu
EL_ACTION_MISSION,
EL_ACTION_ADVANCED_MISSION,
EL_ACTION_TUTORIAL,
EL_ACTION_USE_SYSFILE,
EL_ACTION_LOAD,
//EL_ACTION_SETUP,
//EL_ACTION_TUTORIAL,
EL_ACTION_OPTIONS,
EL_ACTION_QUIT,
EL_ACTION_NAME, // player name input
EL_ACTION_SAVE_GAMEFILE,
EL_ACTION_LOAD_GAMEFILE,
EL_ACTION_START_MISSION,

// setup menu
EL_ACTION_START_GAME_FROM_SETUP,

// misc
EL_ACTION_BACK_TO_START,
EL_ACTION_NONE

};

struct sliderstruct element_slider [EL_SLIDERS];

// menu_liststruct holds the lists for creating menu elements.
// lists should be constant (mutable values are in menu_elementstruct)
struct menu_liststruct
{
 int type; // basic type of the element (e.g. button that does something, slider, etc.)
 int action; // specific effect of element (unique to each element, although a single element can be present in multiple menu types)
 int start_value; // if the element has a value (e.g. a slider) this is the default
 char name [ELEMENT_NAME_SIZE];
 int slider_index; // is -1 if element doesn't have a slider
 int help_type; // HELP_* string to print when right-clicked (see e_help.c)

};

// each time a menu is opened, a list of menu_elementstructs is initialised with values based on menu_liststruct
struct menu_elementstruct
{
 int list_index;
 int x1, y1, x2, y2, w, h; // location/size

 int type;
 int value;
 int fixed; // if 1, the value cannot be changed (probably because it's a PI_VALUES things and its PI_OPTION is set to zero)
 int highlight;
 int slider_index;

};


enum
{
	// This list must be ordered in the same way as the big menu_list array below
EL_MAIN_HEADING,
EL_MAIN_MISSION,
EL_MAIN_ADVANCED_MISSION,
EL_MAIN_TUTORIAL,
EL_MAIN_USE_SYSFILE,
EL_MAIN_LOAD,
EL_MAIN_LOAD_GAMEFILE,
//EL_MAIN_SETUP,
//EL_MAIN_TUTORIAL,
EL_MAIN_OPTIONS,
EL_MAIN_QUIT,

EL_SETUP_HEADING,
EL_SETUP_START,
EL_SETUP_PLAYERS,
EL_SETUP_TURNS,
EL_SETUP_MINUTES,
EL_SETUP_PROCS,
//EL_SETUP_GEN_LIMIT,
// EL_SETUP_PACKETS,  should probably derive this from procs rather than allowing it to be set
EL_SETUP_W_BLOCK,
EL_SETUP_H_BLOCK,
EL_SETUP_BACK_TO_START,
/*EL_SETUP_PLAYER_COL_0,
EL_SETUP_PLAYER_COL_1,
EL_SETUP_PLAYER_COL_2,
EL_SETUP_PLAYER_COL_3,*/
EL_SETUP_PLAYER_NAME_0,
EL_SETUP_PLAYER_NAME_1,
EL_SETUP_PLAYER_NAME_2,
EL_SETUP_PLAYER_NAME_3,
EL_SETUP_SAVE_GAMEFILE,


EL_MISSIONS_HEADING_TUTORIAL,
EL_MISSIONS_T1,
EL_MISSIONS_T2,
EL_MISSIONS_T3,
EL_MISSIONS_HEADING_ADV_TUTORIAL,
EL_MISSIONS_T4,
EL_TUTORIAL_BACK,
EL_MISSIONS_HEADING_MISSIONS,
EL_MISSIONS_M1,
EL_MISSIONS_M2,
EL_MISSIONS_M3,
EL_MISSIONS_M4,
EL_MISSIONS_M5,
EL_MISSIONS_M6,
EL_MISSIONS_BACK,
EL_MISSIONS_HEADING_ADVANCED_MISSIONS,
EL_ADVANCED_MISSIONS_M1,
EL_ADVANCED_MISSIONS_M2,
EL_ADVANCED_MISSIONS_M3,
EL_ADVANCED_MISSIONS_M4,
EL_ADVANCED_MISSIONS_M5,
EL_ADVANCED_MISSIONS_M6,
EL_ADVANCED_MISSIONS_BACK,


// when adding a new member to this list, must also add to the menu_list list below
ELS
};



const struct menu_liststruct menu_list [ELS] =
{
// main menu
 {
  EL_TYPE_HEADING,
  EL_ACTION_NONE, // action
  0, // start_value (used as mission index)
  "Start menu", // name
  -1, // slider_index
  HELP_NONE, // help_type
 }, // EL_MAIN_HEADING
 {
  EL_TYPE_ACTION, // type
  EL_ACTION_MISSION, // action
  0, // start_value
  "Missions", // name
  -1, // slider_index
  HELP_MISSION_MENU, // help_type
 }, // EL_MAIN_MISSION
 {
  EL_TYPE_ACTION, // type
  EL_ACTION_ADVANCED_MISSION, // action
  0, // start_value
  "Advanced missions", // name
  -1, // slider_index
  HELP_ADVANCED_MISSION_MENU, // help_type
 }, // EL_MAIN_ADVANCED_MISSION
 {
  EL_TYPE_ACTION, // type
  EL_ACTION_TUTORIAL, // action
  0, // start_value
  "Tutorial", // name
  -1, // slider_index
  HELP_TUTORIAL_MENU, // help_type
 }, // EL_MAIN_TUTORIAL
 {
  EL_TYPE_ACTION, // type
  EL_ACTION_USE_SYSFILE, // action
  0, // start_value
  "Use System File", // name
  -1, // slider_index
  HELP_USE_SYSFILE, // help_type
 }, // EL_MAIN_USE_SYSFILE
 {
  EL_TYPE_ACTION, // type
  EL_ACTION_LOAD, // action
  0, // start_value
  "Load Saved Game", // name
  -1, // slider_index
  HELP_LOAD, // help_type
 }, // EL_MAIN_LOAD
 {
  EL_TYPE_ACTION, // type
  EL_ACTION_LOAD_GAMEFILE, // action
  0, // start_value
  "Load Gamefile", // name
  -1, // slider_index
  HELP_LOAD_GAMEFILE, // help_type
 }, // EL_MAIN_LOAD_GAMEFILE
/* {
  EL_TYPE_ACTION, // type
  EL_ACTION_SETUP, // action
  0, // start_value
  "Setup Game", // name
  -1, // slider_index
 }, // EL_MAIN_SETUP
 {
  EL_TYPE_ACTION, // type
  EL_ACTION_TUTORIAL, // tutorial
  0, // start_value
  "Tutorial", // name
  -1, // slider_index
 }, // EL_MAIN_TUTORIAL*/
 {
  EL_TYPE_ACTION, // type
  EL_ACTION_OPTIONS, // action
  0, // start_value
  "Options", // name
  -1, // slider_index
  HELP_OPTIONS, // help_type
 }, // EL_MAIN_OPTIONS
 {
  EL_TYPE_ACTION, // type
  EL_ACTION_QUIT, // action
  0, // start_value
  "Exit", // name
  -1, // slider_index
  HELP_MAIN_EXIT, // help_type
 }, // EL_MAIN_EXIT

// setup menu
 {
  EL_TYPE_HEADING,
  EL_ACTION_NONE, // action
  0, // start_value (used as mission index)
  "Set up system parameters", // name
  -1, // slider_index
  HELP_NONE, // help_type
 }, // EL_SETUP_HEADING
 {
  EL_TYPE_ACTION, // type
  EL_ACTION_START_GAME_FROM_SETUP, // action
  0, // start_value
  "Start", // name
  -1, // slider_index
  HELP_SETUP_START, // help_type
 }, // EL_SETUP_START
 {
  EL_TYPE_SLIDER, // type
  0, // action
  0, // start_value - derived from pre_init
  "Players", // name
  EL_SLIDER_PLAYERS, // slider_index
  HELP_SETUP_PLAYERS, // help_type
 }, // EL_SETUP_PLAYERS
 {
  EL_TYPE_SLIDER, // type
  0, // action
  0, // start_value - derived from pre_init
  "Turns", // name
  EL_SLIDER_TURNS, // slider_index
  HELP_SETUP_TURNS, // help_type
 }, // EL_SETUP_TURNS
 {
  EL_TYPE_SLIDER, // type
  0, // action
  0, // start_value - derived from pre_init
  "Minutes per turn", // name
  EL_SLIDER_MINUTES, // slider_index
  HELP_SETUP_MINUTES, // help_type
 }, // EL_SETUP_MINUTES
 {
  EL_TYPE_SLIDER, // type
  0, // action
  0, // start_value - derived from pre_init
  "Processes", // name
  EL_SLIDER_PROCS, // slider_index
  HELP_SETUP_PROCS, // help_type
 }, // EL_SETUP_PROCS
/* {
  EL_TYPE_SLIDER, // type
  0, // action
  0, // start_value - derived from pre_init
  "Gen limit", // name
  EL_SLIDER_GEN_LIMIT, // slider_index
 }, // EL_SETUP_GEN_LIMIT*/
 {
  EL_TYPE_SLIDER, // type
  0, // action
  0, // start_value - derived from pre_init
  "World size (x)", // name
  EL_SLIDER_W_BLOCK, // slider_index
  HELP_SETUP_W_BLOCK, // help_type
 }, // EL_SETUP_W_BLOCK
 {
  EL_TYPE_SLIDER, // type
  0, // action
  0, // start_value - derived from pre_init
  "World size (y)", // name
  EL_SLIDER_H_BLOCK, // slider_index
  HELP_SETUP_H_BLOCK, // help_type
 }, // EL_SETUP_H_BLOCK
 {
  EL_TYPE_ACTION, // type
  EL_ACTION_BACK_TO_START, // action
  0, // start_value
  "Back", // name
  -1, // slider_index
  HELP_SETUP_BACK_TO_START, // help_type
 }, // EL_SETUP_BACK_TO_START
/* {
  EL_TYPE_COLOUR,
  0, // action (player index)
  TEAM_COLOUR_GREEN, // start_value
  "Player 1 colour", // name
  -1, // slider_index
 }, // EL_SETUP_PLAYER_COL_0
 {
  EL_TYPE_COLOUR,
  1, // action (player index)
  TEAM_COLOUR_RED, // start_value
  "Player 2 colour", // name
  -1, // slider_index
 }, // EL_SETUP_PLAYER_COL_1
 {
  EL_TYPE_COLOUR,
  2, // action (player index)
  TEAM_COLOUR_BLUE, // start_value
  "Player 3 colour", // name
  -1, // slider_index
 }, // EL_SETUP_PLAYER_COL_2
 {
  EL_TYPE_COLOUR,
  3, // action (player index)
  TEAM_COLOUR_PURPLE, // start_value
  "Player 4 colour", // name
  -1, // slider_index
 }, // EL_SETUP_PLAYER_COL_3*/
 {
  EL_TYPE_ACTION,
  EL_ACTION_NAME, // action
  0, // start_value (used as player index)
  "Player 1 name", // name
  -1, // slider_index
  HELP_SETUP_PLAYER_NAME, // help_type
 }, // EL_SETUP_PLAYER_NAME_0
 {
  EL_TYPE_ACTION,
  EL_ACTION_NAME, // action (player index)
  1, // start_value (used as player index)
  "Player 2 name", // name
  -1, // slider_index
  HELP_SETUP_PLAYER_NAME, // help_type
 }, // EL_SETUP_PLAYER_NAME_1
 {
  EL_TYPE_ACTION,
  EL_ACTION_NAME, // action (player index)
  2, // start_value (used as player index)
  "Player 3 name", // name
  -1, // slider_index
  HELP_SETUP_PLAYER_NAME, // help_type
 }, // EL_SETUP_PLAYER_NAME_2
 {
  EL_TYPE_ACTION,
  EL_ACTION_NAME, // action (player index)
  3, // start_value (used as player index)
  "Player 4 name", // name
  -1, // slider_index
  HELP_SETUP_PLAYER_NAME, // help_type
 }, // EL_SETUP_PLAYER_NAME_3
 {
  EL_TYPE_ACTION,
  EL_ACTION_SAVE_GAMEFILE, // action
  0, // start_value (used as player index)
  "Save gamefile", // name
  -1, // slider_index
  HELP_SETUP_SAVE_GAMEFILE, // help_type
 }, // EL_SETUP_SAVE_GAMEFILE

// Missions
 {
  EL_TYPE_HEADING,
  EL_ACTION_NONE, // action
  0, // start_value (used as mission index)
  "Tutorials", // name
  -1, // slider_index
  HELP_NONE, // help_type
 }, // EL_MISSIONS_HEADING_TUTORIAL
 {
  EL_TYPE_ACTION,
  EL_ACTION_START_MISSION, // action
  MISSION_TUTORIAL1, // start_value (used as mission index)
  "Tutorial: basics", // name
  -1, // slider_index
  HELP_TUTORIAL_1, // help_type
 }, // EL_MISSIONS_T1
 {
  EL_TYPE_ACTION,
  EL_ACTION_START_MISSION, // action
  MISSION_TUTORIAL2, // start_value (used as mission index)
  "Tutorial: building + attacking", // name
  -1, // slider_index
  HELP_TUTORIAL_2, // help_type
 }, // EL_MISSIONS_T2
 {
  EL_TYPE_ACTION,
  EL_ACTION_START_MISSION, // action
  MISSION_TUTORIAL3, // start_value (used as mission index)
  "Tutorial: templates", // name
  -1, // slider_index
  HELP_TUTORIAL_3, // help_type
 }, // EL_MISSIONS_T3
 {
  EL_TYPE_HEADING,
  EL_ACTION_NONE, // action
  0, // start_value (used as mission index)
  "Advanced tutorial", // name
  -1, // slider_index
  HELP_NONE, // help_type
 }, // EL_MISSIONS_HEADING_ADV_TUTORIAL
 {
  EL_TYPE_ACTION,
  EL_ACTION_START_MISSION, // action
  MISSION_TUTORIAL4, // start_value (used as mission index)
  "Tutorial: delegation", // name
  -1, // slider_index
  HELP_TUTORIAL_4, // help_type
 }, // EL_MISSIONS_T4
 {
  EL_TYPE_ACTION, // type
  EL_ACTION_BACK_TO_START, // action
  0, // start_value
  "Back", // name
  -1, // slider_index
  HELP_TUTORIAL_BACK, // help_type
 }, // EL_TUTORIAL_BACK
 {
  EL_TYPE_HEADING,
  EL_ACTION_NONE, // action
  0, // start_value (used as mission index)
  "Missions", // name
  -1, // slider_index
  HELP_NONE, // help_type
 }, // EL_MISSIONS_HEADING_MISSIONS
 {
  EL_TYPE_ACTION,
  EL_ACTION_START_MISSION, // action
  MISSION_MISSION1, // start_value (used as mission index)
  "Mission 1", // name
  -1, // slider_index
  HELP_MISSION, // help_type
 }, // EL_MISSIONS_M1
 {
  EL_TYPE_ACTION,
  EL_ACTION_START_MISSION, // action
  MISSION_MISSION2, // start_value (used as mission index)
  "Mission 2", // name
  -1, // slider_index
  HELP_MISSION, // help_type
 }, // EL_MISSIONS_M2
 {
  EL_TYPE_ACTION,
  EL_ACTION_START_MISSION, // action
  MISSION_MISSION3, // start_value (used as mission index)
  "Mission 3", // name
  -1, // slider_index
  HELP_MISSION, // help_type
 }, // EL_MISSIONS_M3
 {
  EL_TYPE_ACTION,
  EL_ACTION_START_MISSION, // action
  MISSION_MISSION4, // start_value (used as mission index)
  "Mission 4", // name
  -1, // slider_index
  HELP_MISSION, // help_type
 }, // EL_MISSIONS_M4
 {
  EL_TYPE_ACTION,
  EL_ACTION_START_MISSION, // action
  MISSION_MISSION5, // start_value (used as mission index)
  "Mission 5", // name
  -1, // slider_index
  HELP_MISSION, // help_type
 }, // EL_MISSIONS_M5
 {
  EL_TYPE_ACTION,
  EL_ACTION_START_MISSION, // action
  MISSION_MISSION6, // start_value (used as mission index)
  "Mission 6", // name
  -1, // slider_index
  HELP_MISSION, // help_type
 }, // EL_MISSIONS_M6
 {
  EL_TYPE_ACTION, // type
  EL_ACTION_BACK_TO_START, // action
  0, // start_value
  "Back", // name
  -1, // slider_index
  HELP_MISSIONS_BACK, // help_type
 }, // EL_MISSIONS_BACK
 {
  EL_TYPE_HEADING,
  EL_ACTION_NONE, // action
  0, // start_value (used as mission index)
  "Advanced Missions", // name
  -1, // slider_index
  HELP_NONE, // help_type
 }, // EL_MISSIONS_HEADING_MISSIONS
 {
  EL_TYPE_ACTION,
  EL_ACTION_START_MISSION, // action
  MISSION_ADVANCED1, // start_value (used as mission index)
  "Advanced Mission 1", // name
  -1, // slider_index
  HELP_ADVANCED_MISSION, // help_type
 }, // EL_ADVANCED_MISSIONS_M1
 {
  EL_TYPE_ACTION,
  EL_ACTION_START_MISSION, // action
  MISSION_ADVANCED2, // start_value (used as mission index)
  "Advanced Mission 2", // name
  -1, // slider_index
  HELP_ADVANCED_MISSION, // help_type
 }, // EL_ADVANCED_MISSIONS_M2
 {
  EL_TYPE_ACTION,
  EL_ACTION_START_MISSION, // action
  MISSION_ADVANCED3, // start_value (used as mission index)
  "Advanced Mission 3", // name
  -1, // slider_index
  HELP_ADVANCED_MISSION, // help_type
 }, // EL_ADVANCED_MISSIONS_M3
 {
  EL_TYPE_ACTION,
  EL_ACTION_START_MISSION, // action
  MISSION_ADVANCED4, // start_value (used as mission index)
  "Advanced Mission 4", // name
  -1, // slider_index
  HELP_ADVANCED_MISSION, // help_type
 }, // EL_ADVANCED_MISSIONS_M4
 {
  EL_TYPE_ACTION,
  EL_ACTION_START_MISSION, // action
  MISSION_ADVANCED5, // start_value (used as mission index)
  "Advanced Mission 5", // name
  -1, // slider_index
  HELP_ADVANCED_MISSION, // help_type
 }, // EL_ADVANCED_MISSIONS_M5
 {
  EL_TYPE_ACTION,
  EL_ACTION_START_MISSION, // action
  MISSION_ADVANCED6, // start_value (used as mission index)
  "Advanced Mission 6", // name
  -1, // slider_index
  HELP_ADVANCED_MISSION, // help_type
 }, // EL_ADVANCED_MISSIONS_M6
 {
  EL_TYPE_ACTION, // type
  EL_ACTION_BACK_TO_START, // action
  0, // start_value
  "Back", // name
  -1, // slider_index
  HELP_MISSIONS_BACK, // help_type
 }, // EL_ADVANCED_MISSIONS_BACK




/*
 {
  EL_TYPE_COL, // type
  0, // action
  0, // start_value - derived from pre_init
  "Colour", // name
  -1, // slider_index
 }, // EL_SETUP_COL*/



};

enum
{
MENU_MAIN,
MENU_SETUP,
MENU_MISSIONS,
MENU_ADVANCED_MISSIONS,
MENU_TUTORIAL,

MENUS
};

int menu_list_main [] =
{
EL_MAIN_HEADING,
EL_MAIN_MISSION,
EL_MAIN_ADVANCED_MISSION,
EL_MAIN_TUTORIAL,
EL_MAIN_USE_SYSFILE,
EL_MAIN_LOAD,
EL_MAIN_LOAD_GAMEFILE,
//EL_MAIN_SETUP,
//EL_MAIN_TUTORIAL,
//EL_MAIN_OPTIONS,
EL_MAIN_QUIT,
-1 // terminates list
};

int menu_list_setup [] =
{
EL_SETUP_HEADING,
EL_SETUP_START,
EL_SETUP_PLAYERS,
EL_SETUP_TURNS,
EL_SETUP_MINUTES,
EL_SETUP_PROCS,
//EL_SETUP_GEN_LIMIT,
// EL_SETUP_PACKETS,
EL_SETUP_W_BLOCK,
EL_SETUP_H_BLOCK,
/*EL_SETUP_PLAYER_COL_0, // must come after EL_SETUP_PLAYERS (as some colour menu options aren't shown if the game has fewer players)
EL_SETUP_PLAYER_COL_1,
EL_SETUP_PLAYER_COL_2,
EL_SETUP_PLAYER_COL_3,*/
EL_SETUP_PLAYER_NAME_0,
EL_SETUP_PLAYER_NAME_1,
EL_SETUP_PLAYER_NAME_2,
EL_SETUP_PLAYER_NAME_3,
EL_SETUP_SAVE_GAMEFILE,
EL_SETUP_BACK_TO_START,

-1 // terminates list
};

int menu_list_missions [] =
{
EL_MISSIONS_HEADING_MISSIONS,
EL_MISSIONS_M1,
EL_MISSIONS_M2,
EL_MISSIONS_M3,
EL_MISSIONS_M4,
EL_MISSIONS_M5,
EL_MISSIONS_M6,
EL_MISSIONS_BACK,
-1 // terminates list
};

int menu_list_advanced_missions [] =
{
EL_MISSIONS_HEADING_ADVANCED_MISSIONS,
EL_ADVANCED_MISSIONS_M1,
EL_ADVANCED_MISSIONS_M2,
EL_ADVANCED_MISSIONS_M3,
EL_ADVANCED_MISSIONS_M4,
EL_ADVANCED_MISSIONS_M5,
EL_ADVANCED_MISSIONS_M6,
EL_ADVANCED_MISSIONS_BACK,
-1 // terminates list
};


int menu_list_tutorial [] =
{
EL_MISSIONS_HEADING_TUTORIAL,
EL_MISSIONS_T1,
EL_MISSIONS_T2,
EL_MISSIONS_T3,
EL_MISSIONS_HEADING_ADV_TUTORIAL,
EL_MISSIONS_T4,
EL_TUTORIAL_BACK,
-1 // terminates list
};



// MAX_ELEMENTS is the max number of elements in one menu
#define MAX_ELEMENTS 30
#define MENU_H 50
#define MENU_W 300

struct menu_elementstruct menu_element [MAX_ELEMENTS];

struct world_preinitstruct w_pre_init; // whenever a world setup menu is entered (from above), this is set up with appropriate values (from gamefile or default values, as needed)
struct world_initstruct w_init; // this is the world_init generated by world setup menus

enum
{
MENU_TYPE_NORMAL, // vertical list of options
//MENU_TYPE_PREGAME, // just shows text

};

/*enum
{
PREGAME_BUTTON_GO,
PREGAME_BUTTON_BACK,
PREGAME_BUTTONS

};*/

enum
{
MENU_TEXT_NONE,
MENU_TEXT_PLAYER_0_NAME,
MENU_TEXT_PLAYER_1_NAME,
MENU_TEXT_PLAYER_2_NAME,
MENU_TEXT_PLAYER_3_NAME,
};

#define MENU_STRIPES 24
#define STRIPE_COLS 12

struct menu_statestruct
{
 int menu_type; // MENU_TYPE_NORMAL or MENU_TYPE_PREGAME

 int elements; // this is the number of elements the current menu has. Set by open_menu()
 int h; // total height (in pixels) of menu display
 int window_pos; // if menu is too long to display, this is the pos of the top of screen
// int edit_window;
 struct sliderstruct mscrollbar; // vertical scrollbar if menu is too long to display
 int use_scrollbar; // 0 if no scrollbar, 1 if scrollbar
 int menu_templ_state; // e.g. MENU_TEMPL_STATE_MAIN
 int menu_text_box; // MENU_TEXT_xxx (is MENU_TEXT_NONE if no text box open)

/* int pregame_button_x [PREGAME_BUTTONS];
 int pregame_button_y [PREGAME_BUTTONS];
 int pregame_button_w [PREGAME_BUTTONS];
 int pregame_button_h [PREGAME_BUTTONS];
 int pregame_button_highlight [PREGAME_BUTTONS];*/

 int stripe_group_col;
 int stripe_group_shade;
 int stripe_group_time;
 int stripe_next_group_count;
 int stripe_next_stripe;

 int stripe_exists [MENU_STRIPES];
 float stripe_x [MENU_STRIPES];
 float stripe_size [MENU_STRIPES];
 int stripe_col [MENU_STRIPES];
 int stripe_shade [MENU_STRIPES];

 ALLEGRO_COLOR stripe_al_col [STRIPE_COLS];

};

struct menu_statestruct mstate;


void run_menu_input(void);
void open_menu(int menu_index);
void reset_menu_templates(void);
void display_menu_1(void);
void display_menu_2(void);
void menu_loop(void);
void run_menu(void);
void derive_value_from_preinit_array(int* values, int* pre_values, struct menu_elementstruct* me);
//void open_pregame_menu(void);
void run_game_from_menu(int need_to_initialise, int playing_mission);

void derive_world_init_from_menu(void);
void derive_world_init_value(int* target, int* pre_init_source, int m_element);
int get_menu_element_value(int t_index);
void run_intro_screen(void);
void run_menu_stripes(int draw);
void draw_menu_button(float xa, float ya, float xb, float yb, ALLEGRO_COLOR button_colour);

#define MENU_X 100

// this function initialises the menu state when a menu is opened
// it populates the menu_element list using values from the relevant menu_list.
// it may do other things as well.
void open_menu(int menu_index)
{


// we don't want the new menu to receive the mouse button press (which I think is particularly a problem for sliders):
 if (ex_control.mb_press [0] == BUTTON_JUST_PRESSED)
  ex_control.mb_press [0] = BUTTON_HELD;

 int i = 0;
 int using_list_index;
 int x = MENU_X, y = 100;
 int values_from_pre [3] = {0,0,0};
 int *mlist; // this is a pointer to an array of elements, which must be -1 terminated.
 int max_players = 1;

 switch(menu_index)
 {
  case MENU_MAIN:
   mlist = menu_list_main;
// for now, the only template that should be open is a single gamefile template:
   reset_menu_templates();
   break;
  case MENU_SETUP:
   mlist = menu_list_setup;
// don't need to reset the system template here (although it won't be used again)
   break;
  case MENU_MISSIONS:
   mlist = menu_list_missions;
   break;
  case MENU_ADVANCED_MISSIONS:
			mlist = menu_list_advanced_missions;
			break;
  case MENU_TUTORIAL:
			mlist = menu_list_tutorial;
			break;
  default:
   fprintf(stdout, "\ns_menu.c: open_menu(): unrecognised menu_index %i", menu_index);
   error_call();
   mlist = menu_list_main; // avoids compiler warning
   break;
 }


 if (settings.edit_window == EDIT_WINDOW_CLOSED)
  open_templates();

 settings.mode_button_available [MODE_BUTTON_PROGRAMS] = 0;
 settings.mode_button_available [MODE_BUTTON_TEMPLATES] = 1;
 settings.mode_button_available [MODE_BUTTON_EDITOR] = 1;
 settings.mode_button_available [MODE_BUTTON_SYSMENU] = 0;
 settings.mode_button_available [MODE_BUTTON_CLOSE] = 0;

 int ml = 0;

 while (mlist [ml] != -1)
 {
#ifdef SANITY_CHECK
  if (i >= MAX_ELEMENTS)
  {
   fprintf(stdout, "\nError: s_menu.c: open_menu(): too many elements (%i) in menu %i (max %i).", i, menu_index, MAX_ELEMENTS);
   error_call();
  }
#endif
  using_list_index = mlist [ml];
// some mlist entries may not be displayed, depending on initialisation factors (currently, this is only done to remove player colour options for players that can't exist)
  switch (using_list_index)
  {
//   case EL_SETUP_PLAYER_COL_1:
   case EL_SETUP_PLAYER_NAME_1:
    if (max_players < 2)
    {
     ml ++;
     continue;
    }
    break;
//   case EL_SETUP_PLAYER_COL_2:
   case EL_SETUP_PLAYER_NAME_2:
    if (max_players < 3)
    {
     ml ++;
     continue;
    }
    break;
//   case EL_SETUP_PLAYER_COL_3:
   case EL_SETUP_PLAYER_NAME_3:
    if (max_players < 4)
    {
     ml ++;
     continue;
    }
    break;
  }
  menu_element [i].list_index = using_list_index;
  menu_element [i].x1 = x;
  menu_element [i].y1 = y;
  menu_element [i].w = MENU_W;
  menu_element [i].h = MENU_H;
  menu_element [i].x2 = x + menu_element [i].w;
  menu_element [i].y2 = y + menu_element [i].h;
  menu_element [i].highlight = 0;
  menu_element [i].value = menu_list[using_list_index].start_value;
  menu_element [i].type = menu_list[using_list_index].type;
  menu_element [i].fixed = 0; // can be changed in derive_value_from_preinit_array
// some values are derived from pre_init:
  switch(menu_element [i].list_index)
  {
   case EL_SETUP_PLAYERS: // derive_value_from_preinit_array() takes values from w_pre_init and puts them in values_from_pre and the value field of menu_element [i]
    derive_value_from_preinit_array(values_from_pre, w_pre_init.players, &menu_element [i]);
    max_players = values_from_pre [0]; // default value (which is used if value fixed)
    if (menu_element[i].fixed == 0) // if not fixed, use the maximum value that players can be set to
     max_players = values_from_pre [2];
    break;
   case EL_SETUP_TURNS:
    derive_value_from_preinit_array(values_from_pre, w_pre_init.game_turns, &menu_element [i]);
    break;
   case EL_SETUP_MINUTES:
    derive_value_from_preinit_array(values_from_pre, w_pre_init.game_minutes_each_turn, &menu_element [i]);
    break;
   case EL_SETUP_PROCS:
    derive_value_from_preinit_array(values_from_pre, w_pre_init.procs_per_team, &menu_element [i]);
    break;
//   case EL_SETUP_GEN_LIMIT:
//    derive_value_from_preinit_array(values_from_pre, w_pre_init.gen_limit, &menu_element [i]);
//    break;
   case EL_SETUP_W_BLOCK:
    derive_value_from_preinit_array(values_from_pre, w_pre_init.w_block, &menu_element [i]);
    break;
   case EL_SETUP_H_BLOCK:
    derive_value_from_preinit_array(values_from_pre, w_pre_init.h_block, &menu_element [i]);
    break;


  }

  menu_element[i].slider_index = menu_list[using_list_index].slider_index;

  if (menu_element [i].slider_index != -1)
  {
   init_slider(&element_slider [menu_element [i].slider_index],
               &menu_element [i].value, // value_pointer
               SLIDEDIR_HORIZONTAL,  // dir
               values_from_pre [1], // value_min
               values_from_pre [2], // value_max
               100, // total_length
               1, // button_increment
               1, // track_increment
               1, // slider_represents_size
               x + 160, // x
               y + 20, // y
               SLIDER_BUTTON_SIZE, // thickness
               COL_GREY, // colour
               0); // hidden_if_unused
  }
  y += MENU_H; // the final value of y is used to determine mstate.h
  i ++;
  ml ++;
 };

 y += 30; // a bit of space at the end

 mstate.elements = i;
 mstate.h = y;
 mstate.window_pos = 0;

 if (mstate.h > settings.option [OPTION_WINDOW_H])
 {
   mstate.use_scrollbar = 1;
   init_slider(&mstate.mscrollbar, // *sl
               &mstate.window_pos, // *value_pointer
               SLIDEDIR_VERTICAL, //dir
               0, // value_min
               mstate.h - settings.option [OPTION_WINDOW_H], // value_max
               settings.option [OPTION_WINDOW_H], // total_length
               9, // button_increment
               80, // track_increment
               settings.option [OPTION_WINDOW_H], // slider_represents_size
               settings.editor_x_split - SLIDER_BUTTON_SIZE, // x
               0, // y
               SLIDER_BUTTON_SIZE, // thickness
               COL_GREEN, // colour
               0); // hidden_if_unused

 }
  else
  {
   mstate.use_scrollbar = 0;
  }

 mstate.menu_text_box = MENU_TEXT_NONE;


}

// call this whenever we want to go back to just having a single gamefile template
void reset_menu_templates(void)
{

  setup_system_template();

}

// this function sets default, min and max values for a menu element based on values in the pre_initstruct
// if the value can't be changed by the user, changes the menu element to reflect this
void derive_value_from_preinit_array(int* values, int* pre_values, struct menu_elementstruct* me)
{

 values [0] = pre_values [PI_DEFAULT];
 me->value = values [0];

 values [1] = pre_values [PI_MIN];
 values [2] = pre_values [PI_MAX];

 if (pre_values [PI_OPTION] == 0)
  me->fixed = 1;

}


void display_menu_1(void)
{

 al_set_target_bitmap(al_get_backbuffer(display));
 al_set_clipping_rectangle(0, 0, settings.option [OPTION_WINDOW_W], settings.option [OPTION_WINDOW_H]);

 run_menu_stripes(1);

}

// when this is called, the editor or template windows may have already been drawn on the right side
void display_menu_2(void)
{


 al_set_target_bitmap(al_get_backbuffer(display));
 al_set_clipping_rectangle(0, 0, settings.option [OPTION_WINDOW_W], settings.option [OPTION_WINDOW_H]);
 reset_i_buttons();

 int i, y1, y2;

 switch(mstate.menu_type)
 {
  case MENU_TYPE_NORMAL:
   for (i = 0; i < mstate.elements; i ++)
   {
    y1 = menu_element[i].y1 - mstate.window_pos;
    y2 = menu_element[i].y2 - mstate.window_pos;
    if (menu_element[i].fixed)
    {
    	add_menu_button(menu_element[i].x1 + 1, y1 + 1, menu_element[i].x2 - 1, y2 - 1, colours.base_trans [COL_BLUE] [SHADE_LOW] [TRANS_MED], MBUTTON_TYPE_MENU);
//    	draw_menu_button(menu_element[i].x1 + 1, y1 + 1, menu_element[i].x2 - 1, y2 - 1, colours.base_trans [COL_BLUE] [SHADE_LOW] [TRANS_MED]);
     add_menu_string(menu_element[i].x1 + 15, y1 + 22, &colours.base [COL_GREY] [SHADE_HIGH], ALLEGRO_ALIGN_LEFT, FONT_SQUARE_BOLD, menu_list[menu_element[i].list_index].name);
     sprintf(mstring, "%i", menu_element[i].value);
     add_menu_string(menu_element[i].x1 + 195, y1 + 22, &colours.base [COL_GREY] [SHADE_HIGH], ALLEGRO_ALIGN_RIGHT, FONT_SQUARE_BOLD, mstring);
//     al_draw_textf(font[FONT_SQUARE_BOLD].fnt, colours.base [COL_GREY] [SHADE_HIGH], menu_element[i].x1 + 15, y1 + 22, 0, "%s", menu_list[menu_element[i].list_index].name);
//     al_draw_textf(font[FONT_SQUARE_BOLD].fnt, colours.base [COL_GREY] [SHADE_HIGH], menu_element[i].x1 + 195, y1 + 22, ALLEGRO_ALIGN_RIGHT, "%i", menu_element[i].value);
    }
     else
     {
     	if (menu_element[i].type == EL_TYPE_HEADING)
						{
//       draw_menu_button(menu_element[i].x1 + 1, y1 + 1, menu_element[i].x2 - 1, y2 - 1, colours.base_trans [COL_BLUE] [SHADE_LOW] [TRANS_MED]);
       add_menu_button(menu_element[i].x1 + 1, y1 + 1, menu_element[i].x2 - 1, y2 - 1, colours.base_trans [COL_BLUE] [SHADE_LOW] [TRANS_MED], MBUTTON_TYPE_MENU);
//       al_draw_textf(font[FONT_SQUARE_BOLD].fnt, colours.base [COL_GREY] [SHADE_HIGH], menu_element[i].x1 + 15, y1 + 22, 0, "%s", menu_list[menu_element[i].list_index].name);
       add_menu_string(menu_element[i].x1 + 15, y1 + 22, &colours.base [COL_GREY] [SHADE_HIGH], ALLEGRO_ALIGN_LEFT, FONT_SQUARE_BOLD, menu_list[menu_element[i].list_index].name);
						}
						 else
							{
								if (menu_list[menu_element[i].list_index].action == EL_ACTION_START_MISSION)
								{
									if (missions.locked [menu_list[menu_element[i].list_index].start_value] == 1)
									{
//          draw_menu_button(menu_element[i].x1 + 1, y1 + 1, menu_element[i].x2 - 1, y2 - 1, colours.base_trans [COL_TURQUOISE] [SHADE_LOW] [TRANS_FAINT]);
          add_menu_button(menu_element[i].x1 + 1, y1 + 1, menu_element[i].x2 - 1, y2 - 1, colours.base_trans [COL_TURQUOISE] [SHADE_LOW] [TRANS_FAINT], MBUTTON_TYPE_MENU);
									}
           else
											{
            if (menu_element[i].highlight)
             add_menu_button(menu_element[i].x1 + 1, y1 + 1, menu_element[i].x2 - 1, y2 - 1, colours.base_trans [COL_BLUE] [SHADE_HIGH] [TRANS_THICK], MBUTTON_TYPE_MENU);
//             draw_menu_button(menu_element[i].x1 + 1, y1 + 1, menu_element[i].x2 - 1, y2 - 1, colours.base_trans [COL_BLUE] [SHADE_HIGH] [TRANS_THICK]);
              else
               add_menu_button(menu_element[i].x1 + 1, y1 + 1, menu_element[i].x2 - 1, y2 - 1, colours.base_trans [COL_BLUE] [SHADE_MED] [TRANS_THICK], MBUTTON_TYPE_MENU);
//               draw_menu_button(menu_element[i].x1 + 1, y1 + 1, menu_element[i].x2 - 1, y2 - 1, colours.base_trans [COL_BLUE] [SHADE_MED] [TRANS_THICK]);
//										  al_draw_rectangle(menu_element[i].x2 - 47.5, y1 + 18.5, menu_element[i].x2 - 38.5, y1 + 31.5, colours.base [COL_BLUE] [SHADE_MAX], 1);
										  add_menu_button(menu_element[i].x2 - 29.5, y1 + 16.5, menu_element[i].x2 - 16.5, y1 + 33.5, colours.base_trans [COL_BLUE] [SHADE_MAX] [TRANS_MED], MBUTTON_TYPE_MISSION_STATUS);
										  add_menu_button(menu_element[i].x2 - 27.5, y1 + 18.5, menu_element[i].x2 - 18.5, y1 + 31.5, colours.base_trans [COL_BLUE] [SHADE_LOW] [TRANS_FAINT], MBUTTON_TYPE_MISSION_STATUS);
												if (missions.status [menu_list[menu_element[i].list_index].start_value] != MISSION_STATUS_UNFINISHED)
												{
//											  al_draw_rectangle(menu_element[i].x2 - 46 + (j * 12) - 1.5, y1 + 18.5, menu_element[i].x2 - 40 + (j * 12) + 1.5, y1 + 31.5, colours.base [COL_BLUE] [SHADE_MAX], 1);
//													if (j < mission_rank)
										   add_menu_button(menu_element[i].x2 - 26, y1 + 20, menu_element[i].x2 - 20, y1 + 30, colours.base_trans [COL_GREY] [SHADE_MAX] [TRANS_MED], MBUTTON_TYPE_MISSION_STATUS);
//														al_draw_filled_rectangle(menu_element[i].x2 - 46, y1 + 20, menu_element[i].x2 - 40, y1 + 30, colours.base [COL_BLUE] [SHADE_MAX]);
//													add_menu_button(menu_element[i].x2 - 46, y1 + 20, menu_element[i].x2 - 40, y1 + 30, colours.base [COL_BLUE] [SHADE_MAX], MBUTTON_TYPE_MENU);
												}
//													draw_menu_button(menu_element[i].x2 - 46, y1 + 20, menu_element[i].x2 - 40, y1 + 30, colours.base [COL_BLUE] [SHADE_MAX]);
/*
            int mission_rank = missions.status [menu_list[menu_element[i].list_index].start_value] - MISSION_STATUS_UNFINISHED;
            for (j = 0; j < 3; j ++)
												{
											  al_draw_rectangle(menu_element[i].x2 - 46 + (j * 12) - 1.5, y1 + 18.5, menu_element[i].x2 - 40 + (j * 12) + 1.5, y1 + 31.5, colours.base [COL_BLUE] [SHADE_MAX], 1);
													if (j < mission_rank)
														al_draw_filled_rectangle(menu_element[i].x2 - 46 + (j * 12), y1 + 20, menu_element[i].x2 - 40 + (j * 12), y1 + 30, colours.base [COL_BLUE] [SHADE_MAX]);
												}*/
											}
								}
								 else
									{
          if (menu_element[i].highlight)
           add_menu_button(menu_element[i].x1 + 1, y1 + 1, menu_element[i].x2 - 1, y2 - 1, colours.base_trans [COL_BLUE] [SHADE_HIGH] [TRANS_THICK], MBUTTON_TYPE_MENU);
//           draw_menu_button(menu_element[i].x1 + 1, y1 + 1, menu_element[i].x2 - 1, y2 - 1, colours.base_trans [COL_BLUE] [SHADE_HIGH] [TRANS_THICK]);
            else
             add_menu_button(menu_element[i].x1 + 1, y1 + 1, menu_element[i].x2 - 1, y2 - 1, colours.base_trans [COL_BLUE] [SHADE_MED] [TRANS_THICK], MBUTTON_TYPE_MENU);
//             draw_menu_button(menu_element[i].x1 + 1, y1 + 1, menu_element[i].x2 - 1, y2 - 1, colours.base_trans [COL_BLUE] [SHADE_MED] [TRANS_THICK]);
									}

//        al_draw_textf(font[FONT_SQUARE_BOLD].fnt, colours.base [COL_GREY] [SHADE_HIGH], menu_element[i].x1 + 15, y1 + 22, 0, "%s", menu_list[menu_element[i].list_index].name);
        add_menu_string(menu_element[i].x1 + 15, y1 + 22, &colours.base [COL_GREY] [SHADE_HIGH], ALLEGRO_ALIGN_LEFT, FONT_SQUARE_BOLD, menu_list[menu_element[i].list_index].name);

        switch(menu_element[i].list_index)
        {
         case EL_SETUP_PLAYER_NAME_0:
         case EL_SETUP_PLAYER_NAME_1:
         case EL_SETUP_PLAYER_NAME_2:
         case EL_SETUP_PLAYER_NAME_3:
         	sprintf(mstring, ": %s", w_pre_init.player_name [menu_element[i].list_index - EL_SETUP_PLAYER_NAME_0]);
    	     add_menu_string(menu_element[i].x1 + 105, y1 + 22, &colours.base [COL_GREY] [SHADE_HIGH], ALLEGRO_ALIGN_LEFT, FONT_SQUARE, mstring);
//          al_draw_textf(font[FONT_SQUARE].fnt, colours.base [COL_GREY] [SHADE_HIGH], menu_element[i].x1 + 105, y1 + 22, 0, ": %s", w_pre_init.player_name [menu_element[i].list_index - EL_SETUP_PLAYER_NAME_0]);
          break;
         case EL_SETUP_TURNS:
          if (menu_element[i].value == 0)
    	      add_menu_string(menu_element[i].x1 + 25, y1 + 34, &colours.base [COL_GREY] [SHADE_HIGH], ALLEGRO_ALIGN_LEFT, FONT_SQUARE, " (0=unlimited)");
//           al_draw_textf(font[FONT_SQUARE].fnt, colours.base [COL_GREY] [SHADE_HIGH], menu_element[i].x1 + 25, y1 + 34, 0, " (unlimited)");
//         al_draw_textf(font, colours.base [COL_GREY] [SHADE_HIGH], menu_element[i].x1 + 50, y1 + 22, 0, " (unlimited)");
          break;
         case EL_SETUP_MINUTES:
          if (menu_element[i].value == 0)
    	      add_menu_string(menu_element[i].x1 + 25, y1 + 34, &colours.base [COL_GREY] [SHADE_HIGH], ALLEGRO_ALIGN_LEFT, FONT_SQUARE, " (0=indefinite)");
//           al_draw_textf(font[FONT_SQUARE].fnt, colours.base [COL_GREY] [SHADE_HIGH], menu_element[i].x1 + 25, y1 + 34, 0, " (indefinite)");
//        if (w_pre_init.game_turns == 0)
//         al_draw_textf(font, colours.base [COL_GREY] [SHADE_HIGH], menu_element[i].x1 + 105, y1 + 22, 0, ": unlimited");
          break;

        }

        if (menu_element[i].slider_index != -1)
         draw_choosebar(&element_slider [menu_element[i].slider_index], 0, mstate.window_pos);
       }
     }
   }
   draw_button_buffer();
   draw_menu_strings();
// Now need to draw any choosebars on top of the menu buttons:
   for (i = 0; i < mstate.elements; i ++)
   {
    if (!menu_element[i].fixed
					&&	menu_element[i].slider_index != -1)
     draw_choosebar(&element_slider [menu_element[i].slider_index], 0, mstate.window_pos);
   }
   if (mstate.use_scrollbar)
    draw_scrollbar(&mstate.mscrollbar, 0, 0);
   break;
/*
  case MENU_TYPE_PREGAME:
   al_draw_textf(font, colours.base [COL_GREY] [SHADE_HIGH], 50, 200, ALLEGRO_ALIGN_LEFT, "Pregame menu");

   for (i = 0; i < PREGAME_BUTTONS; i ++)
   {
    if (mstate.pregame_button_highlight [i])
     al_draw_filled_rectangle( mstate.pregame_button_x [i], mstate.pregame_button_y [i], mstate.pregame_button_x [i] + mstate.pregame_button_w [i], mstate.pregame_button_y [i] + mstate.pregame_button_h [i], base_col [COL_BLUE] [SHADE_HIGH]);
      else
       al_draw_filled_rectangle(mstate.pregame_button_x [i], mstate.pregame_button_y [i], mstate.pregame_button_x [i] + mstate.pregame_button_w [i], mstate.pregame_button_y [i] + mstate.pregame_button_h [i], base_col [COL_BLUE] [SHADE_MED]);
    switch(i)
    {
     case PREGAME_BUTTON_GO:
      al_draw_textf(bold_font, colours.base [COL_GREY] [SHADE_HIGH], mstate.pregame_button_x [i] + 10, mstate.pregame_button_y [i] + 10, ALLEGRO_ALIGN_LEFT, "Go"); break;
     case PREGAME_BUTTON_BACK:
      al_draw_textf(bold_font, colours.base [COL_GREY] [SHADE_HIGH], mstate.pregame_button_x [i] + 10, mstate.pregame_button_y [i] + 10, ALLEGRO_ALIGN_LEFT, "Back"); break;
    }
   }
// assume that there is no need for a scrollbar in the pregame menu
   break;*/
 }


// code to draw mode buttons is also in i_display.c
// this will overwrite the editor, if it's open

 draw_mode_buttons();
/*
 for (i = 0; i < MODE_BUTTONS; i ++)
 {
  if (settings.mode_button_available [i] == 0)
   continue;

  if (settings.mode_button_highlight [i] == 0)
  {
   al_draw_filled_rectangle(settings.mode_button_x [i], settings.mode_button_y [i],
    settings.mode_button_x [i] + MODE_BUTTON_SIZE, settings.mode_button_y [i] + MODE_BUTTON_SIZE, base_col [COL_BLUE] [SHADE_MED]);
   al_draw_rectangle(settings.mode_button_x [i], settings.mode_button_y [i],
    settings.mode_button_x [i] + MODE_BUTTON_SIZE, settings.mode_button_y [i] + MODE_BUTTON_SIZE, base_col [COL_BLUE] [SHADE_MAX], 1);
  }
   else
   {
    al_draw_filled_rectangle(settings.mode_button_x [i], settings.mode_button_y [i],
     settings.mode_button_x [i] + MODE_BUTTON_SIZE, settings.mode_button_y [i] + MODE_BUTTON_SIZE, base_col [COL_BLUE] [SHADE_HIGH]);
    al_draw_rectangle(settings.mode_button_x [i], settings.mode_button_y [i],
     settings.mode_button_x [i] + MODE_BUTTON_SIZE, settings.mode_button_y [i] + MODE_BUTTON_SIZE, base_col [COL_GREY] [SHADE_MAX], 1);
   }


  switch(i)
  {
   case MODE_BUTTON_PROGRAMS:
    al_draw_textf(font, base_col [COL_GREY] [SHADE_MAX], settings.mode_button_x [i] + (MODE_BUTTON_SIZE / 2), settings.mode_button_y [i] + 3, ALLEGRO_ALIGN_CENTRE, "Pr"); break;
   case MODE_BUTTON_TEMPLATES:
    al_draw_textf(font, base_col [COL_GREY] [SHADE_MAX], settings.mode_button_x [i] + (MODE_BUTTON_SIZE / 2), settings.mode_button_y [i] + 3, ALLEGRO_ALIGN_CENTRE, "Te"); break;
   case MODE_BUTTON_EDITOR:
    al_draw_textf(font, base_col [COL_GREY] [SHADE_MAX], settings.mode_button_x [i] + (MODE_BUTTON_SIZE / 2), settings.mode_button_y [i] + 3, ALLEGRO_ALIGN_CENTRE, "Ed"); break;
   case MODE_BUTTON_SYSMENU:
    al_draw_textf(font, base_col [COL_GREY] [SHADE_MAX], settings.mode_button_x [i] + (MODE_BUTTON_SIZE / 2), settings.mode_button_y [i] + 3, ALLEGRO_ALIGN_CENTRE, "Sy"); break;
   case MODE_BUTTON_CLOSE:
    al_draw_textf(font, base_col [COL_GREY] [SHADE_MAX], settings.mode_button_x [i] + (MODE_BUTTON_SIZE / 2), settings.mode_button_y [i] + 3, ALLEGRO_ALIGN_CENTRE, "X"); break;

  }

 }*/

// finally, if a text box is open need to draw it over the top:
 if (mstate.menu_text_box != MENU_TEXT_NONE)
 {
#define NAME_BOX_X (MENU_X - 20)
#define NAME_BOX_W 250
#define NAME_BOX_Y 245
#define NAME_BOX_H 55
#define NAME_TEXT_BOX_X 50
#define NAME_TEXT_BOX_W 90
#define NAME_TEXT_BOX_Y 25
#define NAME_TEXT_BOX_H 15
    int naming_player = mstate.menu_text_box - MENU_TEXT_PLAYER_0_NAME;
#ifdef SANITY_CHECK
    if (naming_player < 0
     || naming_player >= PLAYERS)
    {
     fprintf(stdout, "\nError: s_menu.c:display_menu_2(): naming_player out of bounds (%i)", naming_player);
     error_call();
    }
#endif
    char* naming_player_name = w_pre_init.player_name[naming_player];
    al_draw_filled_rectangle(NAME_BOX_X, NAME_BOX_Y, NAME_BOX_X + NAME_BOX_W, NAME_BOX_Y + NAME_BOX_H, colours.base [COL_BLUE] [SHADE_LOW]);
    al_draw_rectangle(NAME_BOX_X, NAME_BOX_Y, NAME_BOX_X + NAME_BOX_W, NAME_BOX_Y + NAME_BOX_H, colours.base [COL_BLUE] [SHADE_MAX], 1);
    al_draw_textf(font[FONT_SQUARE_BOLD].fnt, colours.base [COL_GREY] [SHADE_HIGH], NAME_BOX_X + 10, NAME_BOX_Y + 6, ALLEGRO_ALIGN_LEFT, "Player %i name", naming_player);
// draw box for text to appear in
    al_draw_filled_rectangle(NAME_BOX_X + NAME_TEXT_BOX_X, NAME_BOX_Y + NAME_TEXT_BOX_Y, NAME_BOX_X + NAME_TEXT_BOX_X + NAME_TEXT_BOX_W, NAME_BOX_Y + NAME_TEXT_BOX_Y + NAME_TEXT_BOX_H, colours.base [COL_BLUE] [SHADE_MIN]);
    al_draw_rectangle(NAME_BOX_X + NAME_TEXT_BOX_X, NAME_BOX_Y + NAME_TEXT_BOX_Y, NAME_BOX_X + NAME_TEXT_BOX_X + NAME_TEXT_BOX_W, NAME_BOX_Y + NAME_TEXT_BOX_Y + NAME_TEXT_BOX_H, colours.base [COL_BLUE] [SHADE_HIGH], 1);

    al_draw_textf(font[FONT_SQUARE].fnt, colours.base [COL_GREY] [SHADE_HIGH], NAME_BOX_X + NAME_TEXT_BOX_X + 3, NAME_BOX_Y + NAME_TEXT_BOX_Y + 3, ALLEGRO_ALIGN_LEFT, "%s", naming_player_name);
// TO DO: cursor flash
    int cursor_x = NAME_BOX_X + NAME_TEXT_BOX_X + 3 + (strlen(naming_player_name) * font[FONT_SQUARE].width);
    int cursor_y = NAME_BOX_Y + NAME_TEXT_BOX_Y + 1;
    al_draw_filled_rectangle(cursor_x, cursor_y, cursor_x + 2, cursor_y + 12, colours.base [COL_GREY] [SHADE_MAX]);

 }

 if (settings.option [OPTION_SPECIAL_CURSOR])
  draw_mouse_cursor();
 al_flip_display();
 al_set_target_bitmap(al_get_backbuffer(display));

}

void draw_menu_button(float xa, float ya, float xb, float yb, ALLEGRO_COLOR button_colour)
{

#define BUTTON_NOTCH_Y 6
//#define BUTTON_NOTCH_X (BUTTON_NOTCH_Y / 3.0)
#define BUTTON_NOTCH_X (BUTTON_NOTCH_Y)
     static ALLEGRO_VERTEX button_fan [8];
     int b = 0;

     button_fan[b].x = xa + BUTTON_NOTCH_X;
     button_fan[b].y = ya;
     button_fan[b].z = 0;
     button_fan[b].color = button_colour;
     b++;
     button_fan[b].x = xb - BUTTON_NOTCH_X*2;
     button_fan[b].y = ya;
     button_fan[b].z = 0;
     button_fan[b].color = button_colour;
     b++;
     button_fan[b].x = xb;
     button_fan[b].y = ya + BUTTON_NOTCH_Y*2;
     button_fan[b].z = 0;
     button_fan[b].color = button_colour;
     b++;
     button_fan[b].x = xb;
     button_fan[b].y = yb - BUTTON_NOTCH_Y;
     button_fan[b].z = 0;
     button_fan[b].color = button_colour;
     b++;
     button_fan[b].x = xb - BUTTON_NOTCH_X;
     button_fan[b].y = yb;
     button_fan[b].z = 0;
     button_fan[b].color = button_colour;
     b++;
     button_fan[b].x = xa + BUTTON_NOTCH_X*2;
     button_fan[b].y = yb;
     button_fan[b].z = 0;
     button_fan[b].color = button_colour;
     b++;
     button_fan[b].x = xa;
     button_fan[b].y = yb - BUTTON_NOTCH_Y*2;
     button_fan[b].z = 0;
     button_fan[b].color = button_colour;
     b++;
     button_fan[b].x = xa;
     button_fan[b].y = ya + BUTTON_NOTCH_Y;
     button_fan[b].z = 0;
     button_fan[b].color = button_colour;
     b++;


/*
     button_fan[b].x = xa;
     button_fan[b].y = ya;
     button_fan[b].z = 0;
     button_fan[b].color = button_colour;
     b++;
     button_fan[b].x = xb - BUTTON_NOTCH_X;
     button_fan[b].y = ya;
     button_fan[b].z = 0;
     button_fan[b].color = button_colour;
     b++;
     button_fan[b].x = xb;
     button_fan[b].y = ya + BUTTON_NOTCH_Y;
     button_fan[b].z = 0;
     button_fan[b].color = button_colour;
     b++;
     button_fan[b].x = xb;
     button_fan[b].y = yb;
     button_fan[b].z = 0;
     button_fan[b].color = button_colour;
     b++;
     button_fan[b].x = xa + BUTTON_NOTCH_X;
     button_fan[b].y = yb;
     button_fan[b].z = 0;
     button_fan[b].color = button_colour;
     b++;
     button_fan[b].x = xa;
     button_fan[b].y = yb - BUTTON_NOTCH_Y;
     button_fan[b].z = 0;
     button_fan[b].color = button_colour;
     b++;
*/
/*
     button_fan[b].x = xa + BUTTON_NOTCH_X;
     button_fan[b].y = ya;
     button_fan[b].z = 0;
     button_fan[b].color = button_colour;
     b++;
     button_fan[b].x = xb;
     button_fan[b].y = ya;
     button_fan[b].z = 0;
     button_fan[b].color = button_colour;
     b++;
     button_fan[b].x = xb;
     button_fan[b].y = yb - BUTTON_NOTCH_Y;
     button_fan[b].z = 0;
     button_fan[b].color = button_colour;
     b++;
     button_fan[b].x = xb - BUTTON_NOTCH_X;
     button_fan[b].y = yb;
     button_fan[b].z = 0;
     button_fan[b].color = button_colour;
     b++;
     button_fan[b].x = xa;
     button_fan[b].y = yb;
     button_fan[b].z = 0;
     button_fan[b].color = button_colour;
     b++;
     button_fan[b].x = xa;
     button_fan[b].y = ya + BUTTON_NOTCH_Y;
     button_fan[b].z = 0;
     button_fan[b].color = button_colour;
     b++;*/


//  al_draw_prim(fan_buffer, NULL, NULL, fan_vertex_list, fan_index[i].vertices, ALLEGRO_PRIM_TRIANGLE_FAN);
  al_draw_prim(button_fan, NULL, NULL, 0, b, ALLEGRO_PRIM_TRIANGLE_FAN);





}




void start_menus(void)
{

 game.phase = GAME_PHASE_MENU;
 settings.edit_window = EDIT_WINDOW_CLOSED;
 mstate.menu_type = MENU_TYPE_NORMAL;

 mstate.stripe_group_col = 0;
 mstate.stripe_group_shade = SHADE_HIGH;
 mstate.stripe_group_time = 30;
 mstate.stripe_next_group_count = 200;
 mstate.stripe_next_stripe = 40;
 int i;

 for (i = 0; i < MENU_STRIPES; i ++)
	{
  mstate.stripe_exists [i] = 0;
  mstate.stripe_x [i] = 0;
  mstate.stripe_size [i] = 5;
  mstate.stripe_col [i] = 3;
  mstate.stripe_shade [i] = SHADE_MAX;
	}

	mstate.stripe_al_col [0] = al_map_rgb(10, 60, 110);
	mstate.stripe_al_col [1] = al_map_rgb(10, 40, 120);
	mstate.stripe_al_col [2] = al_map_rgb(30, 30, 90);
	mstate.stripe_al_col [3] = al_map_rgb(30, 15, 100);
	mstate.stripe_al_col [4] = al_map_rgb(50, 10, 90);
	mstate.stripe_al_col [5] = al_map_rgb(50, 5, 140);
	mstate.stripe_al_col [6] = al_map_rgb(10, 70, 100);
	mstate.stripe_al_col [7] = al_map_rgb(10, 30, 120);
	mstate.stripe_al_col [8] = al_map_rgb(40, 30, 110);
	mstate.stripe_al_col [9] = al_map_rgb(30, 23, 70);
	mstate.stripe_al_col [10] = al_map_rgb(30, 80, 120);
	mstate.stripe_al_col [11] = al_map_rgb(120, 10, 80); // rare colour

 for (i = 0; i < settings.option [OPTION_WINDOW_W]; i ++)
 {
 	run_menu_stripes(0);
 }

 run_intro_screen();

 open_menu(MENU_MAIN);

 menu_loop();

}



// this function handles timing etc for the menu interface.
// it also calls game functions etc if the game is started from a menu
// it can co-exist with the template and editor windows
void menu_loop(void)
{


 al_flush_event_queue(event_queue);
 al_flush_event_queue(fps_queue);

 ALLEGRO_EVENT ev;

 do
 {

  display_menu_1(); // prepares screen for menu and possible editor/templates to be written

  run_menu();

  run_menu_input(); // note that this function can result in a change in menu

  switch(settings.edit_window)
  {
   case EDIT_WINDOW_EDITOR:
    run_editor(); break; // if editor is open, this overwrites the right-hand side of the display bitmap
   case EDIT_WINDOW_TEMPLATES:
    run_templates(); break; // this does too
  }

  display_menu_2(); // finishes drawing menu stuff

//  check_sound_queue();

  al_wait_for_event(event_queue, &ev);

 } while (TRUE);


}

// this function does basic maintenance stuff for the current menu
void run_menu(void)
{

 int i;

 for (i = 0; i < mstate.elements; i ++)
 {
  menu_element[i].highlight = 0;
 }

}

void run_menu_input(void)
{

 get_ex_control(0);

 int i;
 int mouse_x = ex_control.mouse_x_pixels;
// int mouse_y = ex_control.mouse_y_pixels; // ignores window_pos
 int abs_mouse_y = ex_control.mouse_y_pixels + mstate.window_pos; // absolute mouse_y
 int just_pressed = (ex_control.mb_press [0] == BUTTON_JUST_PRESSED);
 int rmb_just_pressed = (ex_control.mb_press [1] == BUTTON_JUST_PRESSED);
/*
 if (just_pressed == 1)
	{
		static int sample_play;
  play_interface_sound(sample_play, TONE_1C);
  sample_play++;
  sample_play %= 3;
	}*/


// fprintf(stdout, "\n(%i: %i, my %i amy %i)", mstate.h, mstate.window_pos, mouse_y, abs_mouse_y);

 if (mstate.menu_text_box != MENU_TEXT_NONE)
 {
  if (accept_text_box_input(TEXT_BOX_PLAYER_NAME) == 1
   || just_pressed == 1)
    mstate.menu_text_box = MENU_TEXT_NONE;
  return;
 }

 if (mstate.use_scrollbar)
//  && mstate.menu_type != MENU_TYPE_PREGAME) // pregame menu doesn't have a scrollbar (but use_scrollbar is retained from the previous menu in case we return there)
  run_slider(&mstate.mscrollbar, 0, 0);

// check for the mouse pointer being in the editor/template window:
  if (settings.edit_window != EDIT_WINDOW_CLOSED
   && mouse_x >= settings.editor_x_split)
  {
   if (just_pressed)
    settings.keyboard_capture = INPUT_EDITOR;
   return;
  }

/*
 if (mstate.menu_type == MENU_TYPE_PREGAME)
 {
  for (i = 0; i < PREGAME_BUTTONS; i ++)
  {
   if (mouse_x >= mstate.pregame_button_x [i]
    && mouse_x <= mstate.pregame_button_x [i] + mstate.pregame_button_w [i]
    && mouse_y >= mstate.pregame_button_y [i] // use mouse_y not abs_mouse_y because the menu that opened the pregame menu may have been scrolled down
    && mouse_y <= mstate.pregame_button_y [i] + mstate.pregame_button_h [i])
   {
    mstate.pregame_button_highlight [i] = 1;
    if (just_pressed)
    {
     switch(i)
     {
      case PREGAME_BUTTON_GO:
// assume that derive_world_init_from_menu() was called when the pregame menu was opened, so world_init will have been set up
       if (!setup_world_programs_from_templates()) // this function initialises programs as well as copying from templates. Must be called before run_game().
        return; // if setup_world_programs_from_templates() fails it writes a failure message to the mlog
       run_game_from_menu(1);
       return; // note return, not break!
      case PREGAME_BUTTON_BACK:
       mstate.menu_type = MENU_TYPE_NORMAL; // should just go back to the setup menu or similar
       game.phase = GAME_PHASE_MENU;
       reset_menu_templates(); // TO DO: when new menu types are added, this call may need to identify the menu we are returning to
       return; // note return, not break!
     }
    }
   }
    else
     mstate.pregame_button_highlight [i] = 0;
  } // end for i loop
  return;
 } // end MENU_TYPE_PREGAME
*/

 for (i = 0; i < mstate.elements; i ++)
 {

   if (menu_element[i].slider_index != -1
    && !menu_element[i].fixed)
    run_slider(&element_slider [menu_element[i].slider_index], 0, mstate.window_pos);

  if (mouse_x > menu_element[i].x1
   && mouse_x < menu_element[i].x2
   && abs_mouse_y > menu_element[i].y1
   && abs_mouse_y < menu_element[i].y2)
  {
   menu_element[i].highlight = 1;
   if (just_pressed)
   {
    if (menu_list[menu_element[i].list_index].type == EL_TYPE_ACTION)
    {
     switch(menu_list[menu_element[i].list_index].action)
     {
      case EL_ACTION_QUIT:
// TO DO: think about what happens here if the user has unsaved source tabs open in the editor.
       safe_exit(0);
       break;
      case EL_ACTION_MISSION:
       play_interface_sound(SAMPLE_BLIP1, TONE_2C);
       // first load gamefile (and setup system program for use if appropriate)
       setup_templates_for_mission_menu();
       open_menu(MENU_MISSIONS);
// remember to change settings.status before and after run_game
       break;
      case EL_ACTION_ADVANCED_MISSION:
       play_interface_sound(SAMPLE_BLIP1, TONE_2D);
       // first load gamefile (and setup system program for use if appropriate)
//       setup_templates_for_mission_menu();
       setup_templates_for_advanced_mission_menu();
       open_menu(MENU_ADVANCED_MISSIONS);
// remember to change settings.status before and after run_game
       break;
      case EL_ACTION_TUTORIAL:
       play_interface_sound(SAMPLE_BLIP1, TONE_2E);
       open_menu(MENU_TUTORIAL);
// remember to change settings.status before and after run_game
       break;
      case EL_ACTION_START_GAME_FROM_SETUP:
       play_interface_sound(SAMPLE_BLIP1, TONE_2C);
// here we need to derive a world_init from the pre_init combined with the menu element values then use that world_init to set up the world
       derive_world_init_from_menu();
// then we enter pregame phase so that the system program can do whatever it needs to set up the world.
       setup_templates_for_game_start();
//       if (!setup_player_programs_from_templates()) // this function initialises programs as well as copying from templates. Must be called before run_game().
//        return; // if setup_player_programs_from_templates() fails it writes a failure message to the mlog
       run_game_from_menu(1, -1); // 1 means needs to initialise (because not loading from saved game); -1 means not playing a mission

// start world here

//       mstate.menu_templ_state = MENU_TEMPL_STATE_PREGAME;
//       open_pregame_menu();
       break;
      case EL_ACTION_SAVE_GAMEFILE:
       play_interface_sound(SAMPLE_BLIP1, TONE_2C);
// this button appears in the setup menu
// it generates a gamefile containing all of the current menu and w_init settings, as well as the system program's bcode
// when loaded (from main menu) it lets the player choose which player they are, then goes straight to pregame
       derive_world_init_from_menu();
       if (save_gamefile()) // in f_game.c
       {
        play_interface_sound(SAMPLE_BLIP1, TONE_2G);
        open_menu(MENU_MAIN); // don't go back to main menu if save failed
       }
         else
          play_interface_sound(SAMPLE_BLIP1, TONE_1E);
       break;

      case EL_ACTION_LOAD_GAMEFILE:
       play_interface_sound(SAMPLE_BLIP1, TONE_3C);
       if (load_gamefile() // first loads a gamefile
        && use_sysfile_from_template()) // then tries to use the system file that load_gamefile() should have loaded into template 0
       {
        play_interface_sound(SAMPLE_BLIP1, TONE_3C);
        setup_templates_for_game_start();
//        mstate.menu_templ_state = MENU_TEMPL_STATE_PREGAME;
//        open_pregame_menu(); // this changes the menu type but leaves the elements unchanged - they will be used later by derive_world_init_from_menu
        run_game_from_menu(1, -1); // 1 means needs to initialise (because not loading from saved game); -1 means not playing a mission
       }
        else
        {
         play_interface_sound(SAMPLE_BLIP1, TONE_1E);
         reset_menu_templates();
        }
       break;

      case EL_ACTION_BACK_TO_START:
       play_interface_sound(SAMPLE_BLIP1, TONE_2E);
       open_menu(MENU_MAIN);
       break;

      case EL_ACTION_USE_SYSFILE:
       play_interface_sound(SAMPLE_BLIP1, TONE_2C);
       if (use_sysfile_from_template()) // in s_setup.c. If the gamefile results in a system program, this sets w.system_program.active to 1
       {
// use_sysfile_from_template() will have called derive_program_properties_from_bcode(), so w_pre_init will be usable.
// next step is to use the now filled-in w_pre_init as the basis of a setup menu:
        open_menu(MENU_SETUP);
       } // on failure, use_sysfile_from_template() writes error message to mlog. We don't need to otherwise deal with it failing.
       break;

      case EL_ACTION_LOAD:
       play_interface_sound(SAMPLE_BLIP1, TONE_2C);
       if (load_game())
       {
        play_interface_sound(SAMPLE_BLIP1, TONE_2G);
        run_game_from_menu(0, w.playing_mission); // 0 means don't initialise world as load_game() has already done so. load_game() has also set w.playing_mission.
       }
        else
        {
         play_interface_sound(SAMPLE_BLIP1, TONE_1E);
         reset_menu_templates(); // load_game may have partially loaded templates before exiting
        }
       break;

      case EL_ACTION_NAME:
       play_interface_sound(SAMPLE_BLIP1, TONE_2A);
       start_text_input_box(TEXT_BOX_PLAYER_NAME, w_pre_init.player_name[menu_list[menu_element[i].list_index].start_value], PLAYER_NAME_LENGTH);
       mstate.menu_text_box = MENU_TEXT_PLAYER_0_NAME + menu_list[menu_element[i].list_index].start_value;
       break;

      case EL_ACTION_START_MISSION:
       play_interface_sound(SAMPLE_BLIP1, TONE_2C);
       int mission_index = menu_list[menu_element[i].list_index].start_value;
       if (mission_index >= MISSION_MISSION1) // i.e. is not a tutorial
							{
// The default templates are about to be removed, so we save their contents so that they can be available for the next time a mission is started:
        if (mission_index < MISSION_ADVANCED1)
         copy_mission_templates_to_default(1, 0);
          else
           copy_mission_templates_to_default(0, 1);
							}
       setup_system_template();
       if (use_mission_system_file(mission_index))
       {
        if (use_sysfile_from_template())
        {
         open_menu(MENU_SETUP); // terrible hack that uses the setup menu open function to set up the setup menu, so that derive_world_init_from_menu() will work
         derive_world_init_from_menu();
//         finish_templates_for_mission();
         setup_templates_for_game_start();
         if (mission_index >= MISSION_MISSION1) // i.e. is not a tutorial
							  {
          if (mission_index < MISSION_ADVANCED1)
           copy_default_mission_templates_to_actual_templates(1, 0); // start from operator (ignoring observer, which is template 0). 0 means not advanced.
            else
             copy_default_mission_templates_to_actual_templates(0, 1); // start from observer in template 0. 1 means advanced.
							  }
         run_game_from_menu(1, mission_index);
        }
       }
       break;

     } // end of switch action

    } // end of if action

   } // end of if just_pressed
   if (rmb_just_pressed)
			{
				print_help(menu_list[menu_element[i].list_index].help_type);
			}
   break;
  } // end of if mouse in menu

 } // end of for i loop for menu elements

// number of lines scrolled by moving the mousewheel:
#define EDITOR_MOUSEWHEEL_SPEED 48

// check for mousewheel movement:
 if (mstate.use_scrollbar)
 {
  if (ex_control.mousewheel_change == 1)
  {
   mstate.window_pos += EDITOR_MOUSEWHEEL_SPEED;
   if (mstate.window_pos > mstate.h - settings.option [OPTION_WINDOW_H])
    mstate.window_pos = mstate.h - settings.option [OPTION_WINDOW_H];
   slider_moved_to_value(&mstate.mscrollbar, mstate.window_pos);
  }
  if (ex_control.mousewheel_change == -1)
  {
   mstate.window_pos -= EDITOR_MOUSEWHEEL_SPEED;
   if (mstate.window_pos < 0)
    mstate.window_pos = 0;
   slider_moved_to_value(&mstate.mscrollbar, mstate.window_pos);
  }
 }


// may not reach here

}



void run_game_from_menu(int need_to_initialise, int playing_mission)
{

// at this point we need to wait for the user to let go of the left mouse button
//  (otherwise the game will regard it as having just been clicked when the game starts)

//  ALLEGRO_EVENT ev;
/*
  while (TRUE)
  {
   al_wait_for_event(event_queue, &ev);
   get_ex_control();
   if (ex_control.mb_press [0] != BUTTON_HELD)
    break;
  }*/


 al_flush_event_queue(event_queue);
 al_flush_event_queue(fps_queue);


//       game.phase = GAME_PHASE_PREGAME;
       run_game(need_to_initialise, playing_mission);
// finished, so we return to menu
       game.phase = GAME_PHASE_MENU;
       settings.edit_window = EDIT_WINDOW_CLOSED;
       mstate.menu_type = MENU_TYPE_NORMAL;
       open_menu(MENU_MAIN);
//       reset_menu_templates(); - not needed - open_menu(MENU_MAIN) calls this

}

/*
This function derives w_init (a global world_initstruct) from pre_init combined with menu element values.
It should not be able to fail (although it may need to be able to if it ever involves compilation or similar)
*/
void derive_world_init_from_menu(void)
{

 derive_world_init_value(&w_init.players, w_pre_init.players, EL_SETUP_PLAYERS);
 derive_world_init_value(&w_init.game_turns, w_pre_init.game_turns, EL_SETUP_TURNS);
 derive_world_init_value(&w_init.game_minutes_each_turn, w_pre_init.game_minutes_each_turn, EL_SETUP_MINUTES);
 derive_world_init_value(&w_init.procs_per_team, w_pre_init.procs_per_team, EL_SETUP_PROCS);
// derive_world_init_value(&w_init.gen_limit, w_pre_init.gen_limit, EL_SETUP_GEN_LIMIT);

 w_init.packets_per_team = MAX_PACKETS; //w_init.procs_per_team * 3; // not sure about this
 w_init.max_clouds = CLOUDS;

 derive_world_init_value(&w_init.w_block, w_pre_init.w_block, EL_SETUP_W_BLOCK);
 derive_world_init_value(&w_init.h_block, w_pre_init.h_block, EL_SETUP_H_BLOCK);

// the following should all be menu elements as well:
 w_init.player_clients = w_pre_init.allow_player_clients;
 w_init.player_operator = w_pre_init.player_operator;
 w_init.allow_observer = w_pre_init.allow_user_observer;

// fprintf(stdout, "\nderive_world_init_from_menu: %i x %i", w_init.w_block, w_init.h_block);
// error_call();

 int i;

 for (i = 0; i < w_init.players; i ++)
 {
//  w_init.team_colour [i] = get_menu_element_value(EL_SETUP_PLAYER_COL_0 + i);
  strcpy(w_init.player_name[i], w_pre_init.player_name [i]);
 }

 for (i = 0; i < w_init.players; i ++)
 {
  w_init.may_change_client_template [i] = w_pre_init.may_change_client_template [i];
  w_init.may_change_proc_templates [i] = w_pre_init.may_change_proc_templates [i];
 } // should probably make these a menu option as well


}



// only used by derive_world_init_from_menu
// target is the target address; pre_init_source is an array with PI_xxxx elements; m_element is an EL_xxxx template
void derive_world_init_value(int* target, int* pre_init_source, int m_element)
{

 if (pre_init_source [PI_OPTION])
  *target = get_menu_element_value(m_element);
   else
    *target = pre_init_source [PI_DEFAULT];

}


// finds a menu_element based on list[t_index] and returns its value
// assumes element exists (exits with error on failure)
int get_menu_element_value(int t_index)
{

 int i = 0;

 while(TRUE)
 {
  if (i >= mstate.elements)
  {
   fprintf(stdout, "\nError: could not find menu list member %i.", t_index);
   error_call();
  }
  if (menu_element[i].list_index == t_index)
   return menu_element[i].value;
  i ++;
 };

}



ALLEGRO_BITMAP* title_bitmap;

// assumes settings already setup
void run_intro_screen(void)
{


 al_set_target_bitmap(al_get_backbuffer(display));

 al_flush_event_queue(event_queue);
 al_flush_event_queue(fps_queue);

 ALLEGRO_EVENT ev;
 int mouse_x;
 int mouse_y;
 int just_pressed;
 int y_line;
 int screen_centre_x = settings.option [OPTION_WINDOW_W] / 2;

#define START_BOX_Y (settings.option [OPTION_WINDOW_H] / 2)
#define START_BOX_W 150
#define START_BOX_H 15

 do
 {

  get_ex_control(1); // 1 means that clicking the native close window button will exit immediately
  mouse_x = ex_control.mouse_x_pixels;
  mouse_y = ex_control.mouse_y_pixels;
  just_pressed = (ex_control.mb_press [0] == BUTTON_JUST_PRESSED);

//  if (ex_control.key_press [ALLEGRO_KEY_ESCAPE] > 0)
//    return;

//  al_clear_to_color(colours.base [COL_BLUE] [SHADE_LOW]);
  run_menu_stripes(1);

//  al_draw_textf(font[FONT_SQUARE_LARGE].fnt, colours.base [COL_GREY] [SHADE_MAX], settings.option [OPTION_WINDOW_W] / 2, 200, ALLEGRO_ALIGN_CENTRE, "INVINCIBLE COUNTERMEASURE");
  al_draw_bitmap(title_bitmap, (settings.option [OPTION_WINDOW_W] / 2) - (al_get_bitmap_width(title_bitmap) / 2), 150, 0);
//  al_draw_textf(font[FONT_SQUARE].fnt, colours.base [COL_GREY] [SHADE_MAX], settings.option [OPTION_WINDOW_W] / 2, 240, ALLEGRO_ALIGN_CENTRE, "version: beta 1");

  reset_i_buttons();

  if (mouse_x >= screen_centre_x - START_BOX_W
   && mouse_x <= screen_centre_x + START_BOX_W
   && mouse_y >= START_BOX_Y - START_BOX_H
   && mouse_y <= START_BOX_Y + START_BOX_H)
  {
   add_menu_button(screen_centre_x - START_BOX_W, START_BOX_Y - START_BOX_H,
                    screen_centre_x + START_BOX_W, START_BOX_Y + START_BOX_H,
                    colours.base_trans [COL_BLUE] [SHADE_HIGH] [TRANS_THICK], MBUTTON_TYPE_MENU);
/*
   al_draw_rectangle(screen_centre_x - START_BOX_W, START_BOX_Y - START_BOX_H,
                    screen_centre_x + START_BOX_W, START_BOX_Y + START_BOX_H,
                    colours.base [COL_BLUE] [SHADE_HIGH], 1);*/

   if (just_pressed)
   {
    play_interface_sound(SAMPLE_BLIP1, TONE_2C);
    return;
   }

  }
   else
    add_menu_button(screen_centre_x - START_BOX_W, START_BOX_Y - START_BOX_H,
                    screen_centre_x + START_BOX_W, START_BOX_Y + START_BOX_H,
                    colours.base_trans [COL_BLUE] [SHADE_MED] [TRANS_THICK], MBUTTON_TYPE_MENU);

  draw_menu_buttons();

  al_draw_textf(font[FONT_SQUARE_BOLD].fnt, colours.base [COL_GREY] [SHADE_MAX], settings.option [OPTION_WINDOW_W] / 2, START_BOX_Y - 4, ALLEGRO_ALIGN_CENTRE, ">> start <<");

  y_line = 100;
  al_draw_textf(font[FONT_SQUARE].fnt, colours.base [COL_GREY] [SHADE_HIGH], settings.option [OPTION_WINDOW_W] - 50, settings.option [OPTION_WINDOW_H] - y_line, ALLEGRO_ALIGN_RIGHT, "Version beta 2");
  y_line -= 25;
  al_draw_textf(font[FONT_SQUARE].fnt, colours.base [COL_GREY] [SHADE_HIGH], settings.option [OPTION_WINDOW_W] - 50, settings.option [OPTION_WINDOW_H] - y_line, ALLEGRO_ALIGN_RIGHT, "Copyright 2014 Linley Henzell");
  y_line -= 15;
  al_draw_textf(font[FONT_SQUARE].fnt, colours.base [COL_GREY] [SHADE_HIGH], settings.option [OPTION_WINDOW_W] - 50, settings.option [OPTION_WINDOW_H] - y_line, ALLEGRO_ALIGN_RIGHT, "Free software (GPL v3 or later)");
  y_line -= 15;
  al_draw_textf(font[FONT_SQUARE].fnt, colours.base [COL_GREY] [SHADE_HIGH], settings.option [OPTION_WINDOW_W] - 50, settings.option [OPTION_WINDOW_H] - y_line, ALLEGRO_ALIGN_RIGHT, "Built with Allegro 5");

  if (settings.option [OPTION_SPECIAL_CURSOR])
   draw_mouse_cursor();
  al_flip_display();
  al_set_target_bitmap(al_get_backbuffer(display));
  al_wait_for_event(event_queue, &ev);


 } while (TRUE);


}

#define STRIPE_POLY_BUFFER 1000
#define STRIPE_SPEED 1.31

ALLEGRO_VERTEX stripe_poly_buffer [STRIPE_POLY_BUFFER];
int stripe_vertex_pos;
void add_stripe_vertex(float x, float y, int col, int shade);

void run_menu_stripes(int draw)
{

 int i;
 int top_displace = settings.option [OPTION_WINDOW_H] / 3;

// first move all stripes and count down:
 mstate.stripe_next_group_count --;
 mstate.stripe_group_time ++;

 if (mstate.stripe_next_group_count <= 0)
	{
		mstate.stripe_next_group_count = 300 + grand(300);
  mstate.stripe_group_time = 0;
  mstate.stripe_next_stripe = 1;
					  int new_col = grand(STRIPE_COLS - 1);
					  if (new_col == mstate.stripe_group_col)
								new_col ++; // this is the only way to get col 11
					  mstate.stripe_group_col = new_col;
//					  mstate.stripe_group_col ++;
					  mstate.stripe_group_col %= STRIPE_COLS;

//					  fprintf(stdout, "\nnew_col %i", mstate.stripe_group_col );
	}

 if (mstate.stripe_next_stripe > 0)
 {
		mstate.stripe_next_stripe --;
		if (mstate.stripe_next_stripe == 0)
		{
			for (i = 0; i < MENU_STRIPES; i ++)
			{
				if (mstate.stripe_exists [i] == 0)
				{
					mstate.stripe_exists [i] = 1;
					mstate.stripe_x [i] = settings.option [OPTION_WINDOW_W];
				 mstate.stripe_col [i] = mstate.stripe_group_col;
				 mstate.stripe_shade [i] = mstate.stripe_group_shade;
					if (mstate.stripe_group_time < 100)
					{
					 mstate.stripe_size [i] = 10 + grand(40);
					 mstate.stripe_next_stripe = mstate.stripe_size [i] + 5 + grand(50);
					}
					 else
					 {
						 mstate.stripe_next_stripe = -1; // no more stripes until end
					  mstate.stripe_size [i] = (mstate.stripe_next_group_count + 200) * STRIPE_SPEED;
					 }
					break;
				}
			}
		}
 }


 for (i = 0; i < MENU_STRIPES; i ++)
	{
		if (mstate.stripe_exists [i] == 0)
			continue;
		mstate.stripe_x [i] -= STRIPE_SPEED;
		if (mstate.stripe_x [i] + mstate.stripe_size [i] < top_displace * -1)
			mstate.stripe_exists [i] = 0;
	}

 if (!draw)
		return;

// now draw:

 al_clear_to_color(colours.base [COL_BLUE] [SHADE_LOW]);
/*
 for (i = 0; i < MENU_STRIPES; i++)
	{
		if (mstate.stripe_exists [i] != 0)
		{
	  al_draw_filled_rectangle(mstate.stripe_x [i], 0, mstate.stripe_x [i] + mstate.stripe_size [i], settings.option [OPTION_WINDOW_H] + 1, colours.base [mstate.stripe_col [i]] [mstate.stripe_shade [i]]);
		}
	}*/

 stripe_vertex_pos = 0;

 for (i = 0; i < MENU_STRIPES; i++)
	{
		if (mstate.stripe_exists [i] != 0)
		{
			add_stripe_vertex(mstate.stripe_x [i], settings.option [OPTION_WINDOW_H] + 1, mstate.stripe_col [i], mstate.stripe_shade [i]);
			add_stripe_vertex(mstate.stripe_x [i] + top_displace, -1, mstate.stripe_col [i], mstate.stripe_shade [i]);
			add_stripe_vertex(mstate.stripe_x [i] + mstate.stripe_size [i], settings.option [OPTION_WINDOW_H] + 1, mstate.stripe_col [i], mstate.stripe_shade [i]);

			add_stripe_vertex(mstate.stripe_x [i] + mstate.stripe_size [i], settings.option [OPTION_WINDOW_H] + 1, mstate.stripe_col [i], mstate.stripe_shade [i]);
			add_stripe_vertex(mstate.stripe_x [i] + top_displace, -1, mstate.stripe_col [i], mstate.stripe_shade [i]);
			add_stripe_vertex(mstate.stripe_x [i] + top_displace + mstate.stripe_size [i], -1, mstate.stripe_col [i], mstate.stripe_shade [i]);

			if (stripe_vertex_pos >= STRIPE_POLY_BUFFER - 6)
				break;
		}
	}



 al_draw_prim(stripe_poly_buffer, NULL, NULL, 0, stripe_vertex_pos, ALLEGRO_PRIM_TRIANGLE_LIST);


}

void add_stripe_vertex(float x, float y, int col, int shade)
{

			stripe_poly_buffer [stripe_vertex_pos].x = x;
			stripe_poly_buffer [stripe_vertex_pos].y = y;
			stripe_poly_buffer [stripe_vertex_pos].z = 0;
			stripe_poly_buffer [stripe_vertex_pos].color = mstate.stripe_al_col [col];
			stripe_vertex_pos ++;

}
