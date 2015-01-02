/*

This is the system program for the first tutorial mission.

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
 {0, 40}, // w_block (size of game area in 128-pixel blocks)
 {0, 40}, // h_block
 1, // allow_player_clients
 0, // index of player who is operator (is -1 if no operator)
 0, // allow_user_observer (1 if a user observer is allowed - if there is an operator, this is probably unnecessary)
 {0, 0}, // may_change_client_template (if 0, player's client template is locked)
 {0, 0}, // may_change_proc_templates (if 0, player's process templates are locked)
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
process basic_defence_process;
process target_process;


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
GSTATE_INTRO5,
GSTATE_SCRAMBLE,
GSTATE_2LEFT,
GSTATE_1LEFT,

GSTATE_WON, // user won
GSTATE_LOST, // user lost
GSTATE_END // finished
};

static void spawn_enemies(void);
static int try_spawn(int x, int y);


// This is how to initialise strings.
// Note that unlike in C you can't use [] as a dimension and let the compiler work it out (it's not that smart).
// And you can't declare an array of pointers to strings.
// Also, each dimension's definition must be surrounded by braces.
int process_name [5] [16] =
{
 {"Operator"},
 {"Defender"},
 {"Bad process"},
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
// Copy basic_defence_process into player 0's first (0) process template:
  call(METH_TEMPLATE, TEMPLATE_P0_PROCESS_0, process_start(basic_defence_process), process_end(basic_defence_process), &process_name [1] [0]);

// Now set up templates for player 1 (the attacker)
// Copy target into player 1's first (0) process template:
  call(METH_TEMPLATE, TEMPLATE_P1_PROCESS_0, process_start(target_process), process_end(target_process), &process_name [2] [0]);
// Set player 0's allocate effect to "align" (allocator methods align the background hexes rather than disrupting them)
  call(METH_MANAGE, MS_SY_MANAGE_ALLOCATE_EFFECT, 0, ALLOCATE_EFFECT_ALIGN);

// We could check the return values of these calls, but here it is safe to assume they'll be successful (if not, nothing is going to work anyway)

// Now, place a process for the player:
  call(METH_PLACE,
       MS_SY_PLACE_T_CALL,  // MS_SY_PLACE_T_CALL - place from template
       1000, // x
       world_size_y / 2, // y
       ANGLE_2, // angle
       0, // start address
       BCODE_SIZE_PROCESS - 1, // end address
       0, // player
       TEMPLATE_P0_PROCESS_0); // template access index - this is the template's actual number
// There doesn't seem much point in checking the return values of any of these calls as they should never fail.
  call(METH_MODIFY, MS_SY_MODIFY_INSTANT, 0); // modifies the process so its spawning animation doesn't show.
// (we can assume that it was process 0 because it is the first process for player 0)

// do the same thing again
  call(METH_PLACE,
       MS_SY_PLACE_T_CALL,  // MS_SY_PLACE_T_CALL - place from template
       1000, // x
       (world_size_y / 2) - 200); // y
// don't need to update the remaining method registers - they should retain their value from the previous call.
  call(METH_MODIFY, MS_SY_MODIFY_INSTANT, 1); // can assume the second process placed is process 1
// and again:
  call(METH_PLACE,
       MS_SY_PLACE_T_CALL,  // MS_SY_PLACE_T_CALL - place from template
       1000, // x
       (world_size_y / 2) + 200); // y
  call(METH_MODIFY, MS_SY_MODIFY_INSTANT, 2);

// Now place some enemies:
  call(METH_PLACE,
       MS_SY_PLACE_T_CALL,  // MS_SY_PLACE_T_CALL - place from template
       3300, // x
       (world_size_y / 4), // y
       ANGLE_2, // angle - ANGLE_2 is 1/2 of a circle
       0, // start address
       BCODE_SIZE_PROCESS - 1, // end address
       1, // player
       TEMPLATE_P1_PROCESS_0); // template access index - this is the template's actual number
  call(METH_MODIFY, MS_SY_MODIFY_INSTANT, get(METH_PLACE, MB_SY_PLACE_STATUS)); // the status register holds the index of the process just created.
  call(METH_PLACE,
       MS_SY_PLACE_T_CALL,  // MS_SY_PLACE_T_CALL - place from template
       3700, // x
       (world_size_y / 2)); // y
  call(METH_MODIFY, MS_SY_MODIFY_INSTANT, get(METH_PLACE, MB_SY_PLACE_STATUS)); // the status register holds the index of the process just created.
  call(METH_PLACE,
       MS_SY_PLACE_T_CALL,  // MS_SY_PLACE_T_CALL - place from template
       3300, // x
       ((world_size_y * 3) / 4)); // y
  call(METH_MODIFY, MS_SY_MODIFY_INSTANT, get(METH_PLACE, MB_SY_PLACE_STATUS)); // the status register holds the index of the process just created.

  initialised = 1; // make sure this initialisation code doesn't run again
  game_state = GSTATE_PREGAME;

  call(METH_VIEW, MS_OB_VIEW_FOCUS_XY, query_x(0) + 300, query_y(0)); // Sets camera to process 0's location.

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
   print("\nProgram: training schedule A.1\n");
// print("\n                                                                           .");
   call(METH_CONSOLE, MS_OB_CONSOLE_COLOUR, COL_LBLUE);
   print("\nToday we will begin learning the basic tools of system defence.");
//   print("\nToday we will begin learning to use the basic tools of system defence.");
   call(METH_CONSOLE, MS_OB_CONSOLE_COLOUR, COL_LPURPLE);
   print("\n\n >> Click here to begin <<");
   call(METH_CONSOLE, MS_OB_CONSOLE_ACTION_SET, SYSTEM_CONSOLE, 1); // associates action 1 with last printed line
   game_state = GSTATE_INTRO1;
   break;

  case GSTATE_INTRO1:
// This status waits for the user to click the action line in the console
   if (call(METH_CONSOLE, MS_OB_CONSOLE_ACTION_CHECK) != CONSOLE_ACTION_SYSTEM)
    break;
// So now we start the game:
   call(METH_MANAGE, MS_SY_MANAGE_START_TURN);
// and clear the system console
   call(METH_CONSOLE, MS_OB_CONSOLE_CLEAR, SYSTEM_CONSOLE);
   call(METH_CONSOLE, MS_OB_CONSOLE_OPEN, SYSTEM_CONSOLE);
   call(METH_CONSOLE, MS_OB_CONSOLE_COLOUR, COL_LBLUE);
   call(METH_VIEW, MS_OB_VIEW_SOUND, 0, 18, 100);
// print("\n                                                                           .");
   print("\nYou should see three blue (friendly) processes on your monitor.");
   print("\nThese are simple agents designed to remove malicious or corrupted");
   print("\nprograms.");
   print("\n\nYou should also see a console at the bottom left of your display,");
   print("\nand an overall system map at the bottom right.");
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
// print("\n                                                                           .");
   print("\nLeft-click on a process to select it, or hold and drag the mouse");
   print("\nbutton to select multiple processes at once.");
   print("\n(The blue box on the right of your display provides diagnostic");
   print("\ninformation about the selected process. Use the button in the top");
   print("\nright to minimise it if you want it to go away.)");
   print("\n\nRight-click on the display or the map to issue a movement\ninstruction.");
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
// print("\n                                                                           .");
   print("\n(You can also press ? to get a list of general commands at any time.)");
   call(METH_CONSOLE, MS_OB_CONSOLE_COLOUR, COL_LPURPLE);
   print("\n >> Click here to continue <<");
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
// print("\n                                                                           .");
   print("\nNow, your task here is to find and remove three bad processes that");
   print("\nare misallocating data needed by the system.");
   print("\n\nUse the system map to locate the bad processes.");
   call(METH_CONSOLE, MS_OB_CONSOLE_COLOUR, COL_LPURPLE);
   print("\n\n >> Click here to continue <<");
   call(METH_CONSOLE, MS_OB_CONSOLE_ACTION_SET, SYSTEM_CONSOLE, 1); // associates action 1 with last printed line
   game_state = GSTATE_INTRO5;
   break;

  case GSTATE_INTRO5:
// This status waits for the user to click the action line in the console
   if (call(METH_CONSOLE, MS_OB_CONSOLE_ACTION_CHECK) != CONSOLE_ACTION_SYSTEM)
    break;
// and clear the system console
   call(METH_CONSOLE, MS_OB_CONSOLE_CLEAR, SYSTEM_CONSOLE);
   call(METH_CONSOLE, MS_OB_CONSOLE_OPEN, SYSTEM_CONSOLE);
   call(METH_CONSOLE, MS_OB_CONSOLE_COLOUR, COL_LBLUE);
   call(METH_VIEW, MS_OB_VIEW_SOUND, 0, 18, 100);
// print("\n                                                                           .");
   print("\nTell your processes to eliminate all of the bad processes.");
   print("\n\nSelect a friendly process and right-click on a hostile process to");
   print("\nissue an attack command.");
   print("\n\nYou can also set waypoints by holding shift when right-clicking,");
   print("\nor give an attack-move command by holding control.");
   print("\n(Attack-move makes the process attack anything it finds on the way.)");
   game_state = GSTATE_SCRAMBLE;
   break;

  case GSTATE_SCRAMBLE:
// Check for the size of player 1's team.
   if (call(METH_STD, MS_CLOB_STD_WORLD_TEAM_SIZE, 1) <= 2)
   {
    call(METH_CONSOLE, MS_OB_CONSOLE_OPEN, SYSTEM_CONSOLE);
    call(METH_CONSOLE, MS_OB_CONSOLE_COLOUR, COL_LBLUE);
    call(METH_VIEW, MS_OB_VIEW_SOUND, 0, 18, 100);
    print("\n\nTwo targets left.");
    game_state = GSTATE_2LEFT;
   }
   break;

  case GSTATE_2LEFT:
// Check for the size of player 1's team.
   if (call(METH_STD, MS_CLOB_STD_WORLD_TEAM_SIZE, 1) <= 1)
   {
    call(METH_CONSOLE, MS_OB_CONSOLE_OPEN, SYSTEM_CONSOLE);
    call(METH_CONSOLE, MS_OB_CONSOLE_COLOUR, COL_LBLUE);
    call(METH_VIEW, MS_OB_VIEW_SOUND, 0, 18, 100);
    print("\nOne target left!");
    game_state = GSTATE_1LEFT;
   }
   break;

  case GSTATE_1LEFT:
   if (call(METH_STD, MS_CLOB_STD_WORLD_TEAM_SIZE, 1) == 0)
   {
    call(METH_CONSOLE, MS_OB_CONSOLE_CLEAR, SYSTEM_CONSOLE);
    call(METH_CONSOLE, MS_OB_CONSOLE_OPEN, SYSTEM_CONSOLE);
    call(METH_VIEW, MS_OB_VIEW_SOUND, 0, 18, 100);
    print("\nAll bad processes eliminated!");
    call(METH_CONSOLE, MS_OB_CONSOLE_COLOUR, COL_LGREY);
    print("\n\nSystem status returning to normal.");
    call(METH_CONSOLE, MS_OB_CONSOLE_COLOUR, COL_LBLUE);
    print("\n\nWell done - training schedule A.1 complete.");
    call(METH_CONSOLE, MS_OB_CONSOLE_COLOUR, COL_DBLUE);
    print("\n\nYou can return to the main menu through the System menu.");
    print("\n(Click on the Sy button in the top right of the display.)");
    game_state = GSTATE_WON;
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

// Remove data from allocators so they keep allocating:
  call(METH_MODIFY, MS_SY_MODIFY_DATA, 100, 0);
  call(METH_MODIFY, MS_SY_MODIFY_DATA, 101, 0);
  call(METH_MODIFY, MS_SY_MODIFY_DATA, 102, 0);


}



process operator
{
#include "op_basic.c"
}

process basic_defence_process
{
#include "prsc_defend.c"
}


process target_process
{

// This is the process's interface definition. It determines the nature of the process and
// the methods it has available. It must come before any code.
interface
{
 PROGRAM_TYPE_PROCESS, SHAPE_8OCTAGON, 3, 0, // program type, shape, size (from 0-3), base vertex
 {
  METH_IRPT: {MT_PR_IRPT}, // generates irpt (which is needed to do anything, and to survive)
  METH_ALLOCATE: {MT_PR_ALLOCATE, 0}, // allocates data from the surroundings. This is an external method; it will go on vertex 0.
 }
}

static void main(void)
{
// All this process does is call the allocator.

	// Call the allocator to generate some data (unlike the IRPT method, it doesn't run automatically):
 call(METH_ALLOCATE); // can only be called once each cycle.


}

}

