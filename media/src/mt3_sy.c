/*

This is the system program for the third tutorial mission.

*/

interface
{
 PROGRAM_TYPE_SYSTEM,
// the following numbers are the basic parameters for the game.
// remember, the following are: option (1 if this parameter can be changed by user in the setup menu), default, min, max
 {0, 2}, // players (up to 4 is possible)
 {0, 0}, // turns (0 means unlimited turns)
 {0, 0}, // minutes per turn (0 means indefinite turns)
 {0, 100}, // procs per team
 {0, -1}, // packets per team (-1 means use the game's basic default)
 {0, 60}, // w_block (size of game area in 128-pixel blocks)
 {0, 60}, // h_block
 1, // allow_player_clients
 0, // index of player who is operator (is -1 if no operator)
 0, // allow_user_observer (1 if a user observer is allowed - if there is an operator, this is probably unnecessary)
 {1, 0}, // may_change_client_template (if 0, player's client template is locked)
 {1, 0}, // may_change_proc_templates (if 0, player's process templates are locked)
// The following are the methods available to the system.
 {
  METH_PLACE: {MT_SY_PLACE_PROC},
  {MT_NONE}, // space for PLACE_PROC
  METH_TEMPLATE: {MT_SY_TEMPLATE},
  METH_MODIFY: {MT_SY_MODIFY},
  METH_MANAGE: {MT_SY_MANAGE},
  {MT_NONE}, // space for MANAGE
  METH_INPUT: {MT_OB_INPUT},
  METH_VIEW: {MT_OB_VIEW},
  {MT_NONE}, // space for VIEW
  METH_CONSOLE: {MT_OB_CONSOLE},
  METH_QUERY: {MT_CLOB_QUERY},
  METH_STD: {MT_CLOB_STD},
 }
}



// These are declarations for processes that will be defined later.
process operator;
process cfactory;
process basic_defence_process;
process attacker;


// Now some variables that the system will use to keep track of the game's state:
int game_state;
int world_size_x;
int world_size_y;

enum
{
// stages:
GSTATE_PREGAME, // in pregame mode (16 ticks during which there is no user interface), or waiting for user to press start
GSTATE_INTRO1, // display intro and wait for user to click
GSTATE_INTRO2,
GSTATE_INTRO3,
GSTATE_INTRO4,
GSTATE_SPAWN_PLAYER,
GSTATE_TEMPLATE_TRY_AGAIN,
GSTATE_SPAWN_ENEMY,
GSTATE_SCRAMBLE,

GSTATE_WON, // user won
GSTATE_LOST, // user lost
GSTATE_END // finished
};

static void spawn_enemy(int source_template, int x, int y);
static int try_spawn(int source_template, int x, int y);

// This is how to initialise strings.
// Note that unlike in C you can't use [] as a dimension and let the compiler work it out (it's not that smart).
// And you can't declare an array of pointers to strings.
// Also, each dimension's definition must be surrounded by braces.
int process_name [5] [16] =
{
 {"Operator"},
 {"Factory"},
 {"Defender"},
 {"Attacker"},
};

int console_title [8] = {"System"};


static void main(void)
{

 int initialised = 0;
 int success;
 int test = 2;

 if (initialised == 0)
 {
// Set up some colours:
// MS_OB_VIEW_COLOUR_BACK is the background colour. Other values are r, g, b
  call(METH_VIEW, MS_OB_VIEW_COLOUR_BACK, 21, 21, 51);
// MS_OB_VIEW_COLOUR_BACK2 is the colour of the little hexagonal cells in the background
  call(METH_VIEW, MS_OB_VIEW_COLOUR_BACK2, 80, 80, 250);
// Now recolour enemy player:
  call(METH_VIEW, MS_OB_VIEW_COLOUR_PROC, 1, // player 1
							210, 240, // min and max red intensity
							30, 160, // min and max g intensity
							30, 40); // min and max b intensity

// Set up some details of basic game parameters:
  world_size_x = world_x(); // world_x() is a built-in function that calls the WORLD method to get the world dimensions in pixels
  world_size_y = world_y();

// The system program uses console 7 (and leaves the other consoles to the operator):
#define SYSTEM_CONSOLE 7
  call(METH_CONSOLE, MS_OB_CONSOLE_BACKGROUND, SYSTEM_CONSOLE, BCOL_PURPLE);
  call(METH_CONSOLE, MS_OB_CONSOLE_TITLE, SYSTEM_CONSOLE, &console_title [0]);
  call(METH_CONSOLE, MS_OB_CONSOLE_OPEN, SYSTEM_CONSOLE);
  call(METH_CONSOLE, MS_OB_CONSOLE_LINES, SYSTEM_CONSOLE, 14);
  call(METH_CONSOLE, MS_OB_CONSOLE_MOVE, SYSTEM_CONSOLE, 10, 10); // Places console on the screen (last two parameters are x/y)
// Now, by default everything any program prints using the print command will go to console 0.
// We need to set the system program to print to console 7:
  call(METH_CONSOLE, MS_OB_CONSOLE_OUT_SYSTEM, SYSTEM_CONSOLE);
// But that call doesn't take effect until the next time the system program executes, so we need to set the console now:
  call(METH_CONSOLE, MS_OB_CONSOLE_OUT, SYSTEM_CONSOLE);

// Now set up templates for player 0 (the user)
// Copy operator into player 0's client template:
  call(METH_TEMPLATE, TEMPLATE_P0_CLIENT, process_start(operator), process_end(operator), &process_name [0] [0]);
// Now load player 0's client (the client program is automatically loaded from TEMPLATE_P0_CLIENT)
  call(METH_MANAGE, MS_SY_MANAGE_LOAD_CLIENT, 0);
// Set player 0's allocate effect to "align" (allocator methods align the background hexes rather than disrupting them; default is disrupt)
  call(METH_MANAGE, MS_SY_MANAGE_ALLOCATE_EFFECT, 0, ALLOCATE_EFFECT_ALIGN);

// Now set up templates for player 1 (the attacker)
// Copy target into player 1's first (0) process template:
  call(METH_TEMPLATE, TEMPLATE_P1_PROCESS_0, process_start(attacker), process_end(attacker), &process_name [3] [0]);

// We could check the return values of these calls, but here it is safe to assume they'll be successful (if not, nothing is going to work anyway)

  initialised = 1; // make sure this initialisation code doesn't run again
  game_state = GSTATE_PREGAME;

  call(METH_VIEW, MS_OB_VIEW_FOCUS_XY, 1000, world_size_y / 2); // Sets camera to near process 0's location.

 }

// Now we check whether the game state has changed. Each game state has specific conditions that cause the next state to begin:
 switch(game_state)
 {
  case GSTATE_PREGAME:
// This status waits for the end of the pregame phase (16 ticks at the start during which the system can set up)
   if (call(METH_MANAGE, MS_SY_MANAGE_PHASE) == PHASE_PREGAME)
    break; // nothing to do.
// Okay, pregame is over:
   call(METH_CONSOLE, MS_OB_CONSOLE_COLOUR, COL_LGREY);
   print("\nSYSTEM INITIALISED...\n");
   print("\nProgram: training schedule A.3\n");
// print("\n                                                                           .");
   call(METH_CONSOLE, MS_OB_CONSOLE_COLOUR, COL_LBLUE);
   print("\nToday we will learn how to use templates.");
   call(METH_CONSOLE, MS_OB_CONSOLE_COLOUR, COL_LPURPLE);
   print("\n\n >> Click here to begin <<");
   call(METH_CONSOLE, MS_OB_CONSOLE_ACTION_SET, SYSTEM_CONSOLE, 1); // associates action 1 with last printed line
   game_state = GSTATE_INTRO1;
   break;

  case GSTATE_INTRO1:
// This status waits for the user to click the action line in the console
   if (call(METH_CONSOLE, MS_OB_CONSOLE_ACTION_CHECK) != CONSOLE_ACTION_SYSTEM)
    break;
// and clear the system console
   call(METH_CONSOLE, MS_OB_CONSOLE_CLEAR, SYSTEM_CONSOLE);
   call(METH_CONSOLE, MS_OB_CONSOLE_OPEN, SYSTEM_CONSOLE);
   call(METH_CONSOLE, MS_OB_CONSOLE_COLOUR, COL_LBLUE);
   call(METH_VIEW, MS_OB_VIEW_SOUND, 0, 18, 100);
// Still haven't started game
// print("\n                                                                           .");
   print("\nRemember that each of the processes that you control (and each enemy");
   print("\nprocess as well) is a small computer program.");
   print("\n\nTemplates store these programs in a form that can be used to");
   print("\nmake new processes.");
   call(METH_CONSOLE, MS_OB_CONSOLE_COLOUR, COL_LPURPLE);
   print("\n\n >> Click here to continue <<");
   call(METH_CONSOLE, MS_OB_CONSOLE_ACTION_SET, SYSTEM_CONSOLE, 1); // associates action 1 with last printed line
   game_state = GSTATE_INTRO2;
   break;

  case GSTATE_INTRO2:
// This status waits for the user to click the action line in the console
   if (call(METH_CONSOLE, MS_OB_CONSOLE_ACTION_CHECK) != CONSOLE_ACTION_SYSTEM)
    break;
// and clear the system console
   call(METH_CONSOLE, MS_OB_CONSOLE_CLEAR, SYSTEM_CONSOLE);
   call(METH_CONSOLE, MS_OB_CONSOLE_OPEN, SYSTEM_CONSOLE);
   call(METH_CONSOLE, MS_OB_CONSOLE_COLOUR, COL_LBLUE);
   call(METH_VIEW, MS_OB_VIEW_SOUND, 0, 18, 100);
// Still haven't started game
// print("\n                                                                           .");
   print("\nNow, open the template panel by clicking the Te button in the top");
   print("\nright of the display.");
   print("\n\nYou should see a list: an observer template, an operator");
   print("\ntemplate, and your four process templates (you are Player 0).");
   call(METH_CONSOLE, MS_OB_CONSOLE_COLOUR, COL_LPURPLE);
   print("\n\n >> Click here to continue <<");
   call(METH_CONSOLE, MS_OB_CONSOLE_ACTION_SET, SYSTEM_CONSOLE, 1); // associates action 1 with last printed line
   game_state = GSTATE_INTRO3;
   break;

  case GSTATE_INTRO3:
// This status waits for the user to click the action line in the console
   if (call(METH_CONSOLE, MS_OB_CONSOLE_ACTION_CHECK) != CONSOLE_ACTION_SYSTEM)
    break;
// and clear the system console
   call(METH_CONSOLE, MS_OB_CONSOLE_CLEAR, SYSTEM_CONSOLE);
   call(METH_CONSOLE, MS_OB_CONSOLE_OPEN, SYSTEM_CONSOLE);
   call(METH_CONSOLE, MS_OB_CONSOLE_COLOUR, COL_LBLUE);
   call(METH_VIEW, MS_OB_VIEW_SOUND, 0, 18, 100);
// Still haven't started game
// print("\n                                                                           .");
   print("\nThe observer template isn't relevant to this mode, so you");
   print("\ncan ignore it for now.");
   print("\n\nThe operator template loads an operator program, which gives you a");
   print("\nuser interface. The system has automatically loaded a standard");
   print("\noperator program for you.");
   print("\n\nProcess templates contain code for new processes. For example,");
   print("\na factory process can build a new process by copying code directly");
   print("\nfrom a process template.");
   call(METH_CONSOLE, MS_OB_CONSOLE_COLOUR, COL_LPURPLE);
   print("\n\n >> Click here to continue <<");
   call(METH_CONSOLE, MS_OB_CONSOLE_ACTION_SET, SYSTEM_CONSOLE, 1); // associates action 1 with last printed line
   game_state = GSTATE_INTRO4;
   break;

  case GSTATE_INTRO4:
// This status waits for the user to click the action line in the console
   if (call(METH_CONSOLE, MS_OB_CONSOLE_ACTION_CHECK) != CONSOLE_ACTION_SYSTEM)
    break;
// and clear the system console
   call(METH_CONSOLE, MS_OB_CONSOLE_CLEAR, SYSTEM_CONSOLE);
   call(METH_CONSOLE, MS_OB_CONSOLE_OPEN, SYSTEM_CONSOLE);
   call(METH_CONSOLE, MS_OB_CONSOLE_COLOUR, COL_LBLUE);
   call(METH_VIEW, MS_OB_VIEW_SOUND, 0, 18, 100);
// Still haven't started game
// print("\n                                                                           .");
   print("\nNow, your process templates are currently empty. You'll need to load");
   print("\nsomething into them!");
   print("\n\nUsing the 'Load file' button in each template:");
   print("\n- load prsc_cfactory.c, which is in the /src directory, into process");
   print("\n  template 0. Your first process will be created from this.");
   print("\n- load prsc_defend.c into process template 1. This is the basic");
   print("\n  defensive process from the previous training missions.");
   print("\n- use process templates 2 and 3 to experiment. Any file starting");
   print("\n  with 'pr' should be a process, and if it starts with 'prsc' it");
   print("\n  should accept commands ('pr' with no 'sc' should mean an");
   print("\n  autonomous process that you can't control).");
   call(METH_CONSOLE, MS_OB_CONSOLE_COLOUR, COL_LPURPLE);
   print("\n\n >> Click here when you've finished loading your templates <<");
   call(METH_CONSOLE, MS_OB_CONSOLE_ACTION_SET, SYSTEM_CONSOLE, 1); // associates action 1 with last printed line
   game_state = GSTATE_SPAWN_PLAYER;
   break;

  case GSTATE_SPAWN_PLAYER:
// This status waits for the user to click the action line in the console
   if (call(METH_CONSOLE, MS_OB_CONSOLE_ACTION_CHECK) != CONSOLE_ACTION_SYSTEM)
    break;
// So now we start the game:
   call(METH_MANAGE, MS_SY_MANAGE_START_TURN);
// and clear the system console
try_building:
   call(METH_CONSOLE, MS_OB_CONSOLE_CLEAR, SYSTEM_CONSOLE);
   call(METH_CONSOLE, MS_OB_CONSOLE_OPEN, SYSTEM_CONSOLE);
   call(METH_CONSOLE, MS_OB_CONSOLE_COLOUR, COL_LBLUE);
   call(METH_VIEW, MS_OB_VIEW_SOUND, 0, 18, 100);
   success = call(METH_PLACE,
       MS_SY_PLACE_T_CALL,  // MS_SY_PLACE_T_CALL - place from template
       1000, // x
       world_size_y / 2, // y
       0, // angle
       0, // start address
       BCODE_SIZE_PROCESS - 1, // end address
       0, // player
       TEMPLATE_P0_PROCESS_0); // template access index
   if (success == MR_NEW_SUCCESS)
   {
    game_state = GSTATE_SPAWN_ENEMY;
    break;
   }
   call(METH_CONSOLE, MS_OB_CONSOLE_COLOUR, COL_LRED);
   print("\n\nFailure: Tried to build from template 0, but couldn't.");
   call(METH_CONSOLE, MS_OB_CONSOLE_COLOUR, COL_LBLUE);
   print("\n\nI'm afraid something went wrong.");
   print("\nTry loading src/prsc_cfactory.c into Player 0 process template 0.");
   call(METH_CONSOLE, MS_OB_CONSOLE_COLOUR, COL_LPURPLE);
   print("\n\n >> Click here when template 0 is loaded <<");
   call(METH_CONSOLE, MS_OB_CONSOLE_ACTION_SET, SYSTEM_CONSOLE, 1);
   game_state = GSTATE_TEMPLATE_TRY_AGAIN;
   break;
  case GSTATE_TEMPLATE_TRY_AGAIN:
   if (call(METH_CONSOLE, MS_OB_CONSOLE_ACTION_CHECK) == CONSOLE_ACTION_SYSTEM)
   {
    goto try_building;
   }
   break;

  case GSTATE_SPAWN_ENEMY:
// Check for the size of player 0's team.
   call(METH_CONSOLE, MS_OB_CONSOLE_CLEAR, SYSTEM_CONSOLE);
   call(METH_CONSOLE, MS_OB_CONSOLE_OPEN, SYSTEM_CONSOLE);
   call(METH_VIEW, MS_OB_VIEW_SOUND, 0, 18, 100);
   call(METH_CONSOLE, MS_OB_CONSOLE_COLOUR, COL_LRED);
   print("\nWARNING - HOSTILE PROCESSES DETECTED");
   call(METH_CONSOLE, MS_OB_CONSOLE_COLOUR, COL_LBLUE);
   print("\n\nSomeone is trying to gain unauthorised access to the system.");
   print("\n\nUse the templates you have loaded to repel this intrusion!");
   game_state = GSTATE_SCRAMBLE;
   spawn_enemy(TEMPLATE_P1_PROCESS_0, world_size_x - 1200, world_size_y / 6);
   spawn_enemy(TEMPLATE_P1_PROCESS_0, world_size_x - 1100, (world_size_y / 6) * 2);
   spawn_enemy(TEMPLATE_P1_PROCESS_0, world_size_x - 1000, (world_size_y / 6) * 3);
   spawn_enemy(TEMPLATE_P1_PROCESS_0, world_size_x - 1100, (world_size_y / 6) * 4);
   spawn_enemy(TEMPLATE_P1_PROCESS_0, world_size_x - 1200, (world_size_y / 6) * 5);
   break;

  case GSTATE_SCRAMBLE:
   if (call(METH_STD, MS_CLOB_STD_WORLD_TEAM_SIZE, 1) == 0)
   {
    call(METH_CONSOLE, MS_OB_CONSOLE_CLEAR, SYSTEM_CONSOLE);
    call(METH_CONSOLE, MS_OB_CONSOLE_OPEN, SYSTEM_CONSOLE);
    call(METH_VIEW, MS_OB_VIEW_SOUND, 0, 18, 100);
    call(METH_CONSOLE, MS_OB_CONSOLE_COLOUR, COL_LGREY);
    print("\nAll unauthorised processes eliminated!");
    print("\n\nSystem status returning to normal.");
    call(METH_CONSOLE, MS_OB_CONSOLE_COLOUR, COL_LBLUE);
    print("\n\nWell done - training schedule A.3 complete.");
    call(METH_CONSOLE, MS_OB_CONSOLE_COLOUR, COL_DBLUE);
    print("\n\nYou can return to the main menu through the System menu.");
    print("\n(Click on the Sy button in the top right of the display.)");
    game_state = GSTATE_WON;
   }
   if (call(METH_STD, MS_CLOB_STD_WORLD_TEAM_SIZE, 0) == 0)
   {
    call(METH_CONSOLE, MS_OB_CONSOLE_CLEAR, SYSTEM_CONSOLE);
    call(METH_CONSOLE, MS_OB_CONSOLE_OPEN, SYSTEM_CONSOLE);
    call(METH_VIEW, MS_OB_VIEW_SOUND, 0, 11, 100);
    call(METH_CONSOLE, MS_OB_CONSOLE_COLOUR, COL_LRED);
    print("\nAll defensive processes eliminated!");
    call(METH_CONSOLE, MS_OB_CONSOLE_COLOUR, COL_LBLUE);
    print("\n\nSorry - you will need to try again.");
    call(METH_CONSOLE, MS_OB_CONSOLE_COLOUR, COL_DBLUE);
    print("\n\nYou can return to the main menu through the System menu.");
    print("\n(Click on the Sy button in the top right of the display.)");
    game_state = GSTATE_LOST;
   }
   break;

  case GSTATE_WON:
   call(METH_MANAGE, MS_SY_MANAGE_GAMEOVER, GAME_END_YOU_WON);
// When playing a mission started through the missions menu, status GAME_END_YOU_WON saves mission as complete
   break;
  case GSTATE_LOST:
   call(METH_MANAGE, MS_SY_MANAGE_GAMEOVER, GAME_END_YOU_LOST);
   break;

 }

}



static void spawn_enemy(int source_template, int x, int y)
{

// For this one, we need to allow for the possibility that the player has moved processes to the spawn positions.
// Although the player can't have too many processes, so don't try too hard.

 int spawned;
 int success;

 success = try_spawn(source_template, x, y);

 if (success != MR_NEW_SUCCESS) // SY_PLACE uses the same result values as PR_NEW
 {
  success = try_spawn(source_template, x, y - 300);
  if (success != MR_NEW_SUCCESS) // SY_PLACE uses the same result values as PR_NEW
  {
   success = try_spawn(source_template, x, x + 300);
  }
 }

}

static int try_spawn(int source_template, int x, int y)
{

 return call(METH_PLACE,
        MS_SY_PLACE_T_CALL,  // MS_SY_PLACE_T_CALL - place from template
        x, // x
        y, // y
        ANGLE_2 - ANGLE_16, // angle
        0, // start address
        BCODE_SIZE_PROCESS - 1, // end address
        1, // player
        source_template); // template access index - this is the template's actual number

}



process operator
{
#include "op_basic.c"
}

process cfactory

{
#include "prsc_cfactory.c"
}

process basic_defence_process
{
#include "prsc_defend.c"
}

process attacker
{
#include "pr_attack.c"
}


