

/*

This is the system program for the second mission

*/

// It's best to put an #ifndef around this #include, even though the included file has one too,
//  to reduce the number of files the preprocessor needs to load.
#ifndef STANDARD_COMMANDS
#include "stand_coms.c"
#endif

interface
{
 PROGRAM_TYPE_SYSTEM,
// the following numbers are the basic parameters for the game.
// remember, the following are: option (1 if this parameter can be changed by user in the setup menu), default, min, max
//   if default is -1, the game's basic default will be used.
 {0, 2}, // players (up to 4 is possible)
 {0, 0}, // turns (0 means unlimited turns)
 {0, 0}, // minutes per turn (0 means turns of indefinite length)
 {0, 100}, // procs per team
 {0, -1}, // packets per team (-1 means use the game's basic default)
 {0, 80}, // w_block (size of game area in 128-pixel blocks)
 {0, 80}, // h_block
 1, // allow_player_clients
#ifndef ADVANCED
 0, // index of player who is operator (is -1 if no operator)
 0, // allow_user_observer (1 if a user observer is allowed; if there is an operator, this is probably unnecessary)
#endif
#ifdef ADVANCED
 -1, // index of player who is operator (is -1 if no operator)
 1, // allow_user_observer (1 if a user observer is allowed; if there is an operator, this is probably unnecessary)
#endif
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
  METH_COMMAND: {MT_CL_COMMAND_GIVE},
 }
}



// These are declarations for processes that will be defined later.

process enemy_delegate;
process spider_process;
process spiderm_process;


// Now some variables that the system will use to keep track of the game's state:
int game_state;
int world_size_x;
int world_size_y;
int counter;

enum
{
// stages:
GSTATE_PREGAME, // in pregame mode (16 ticks during which there is no user interface), or waiting for user to press start
GSTATE_SPAWN_ENEMY,
GSTATE_BUILD,
GSTATE_SCRAMBLE,

GSTATE_WON, // user won
GSTATE_LOST, // user lost
GSTATE_END // finished
};

static int spawn_enemy(int source_template, int x, int y, int angle);
static int try_spawn(int source_template, int x, int y, int angle);

// This is how to initialise strings.
// Note that unlike in C you can't use [] as a dimension and let the compiler work it out (it's not that smart).
// And you can't declare an array of pointers to strings.
// Also, each dimension's definition must be surrounded by braces.
int process_name [4] [16] =
{
 {"Delegate"},
 {"Spider"},
 {"Spider-M"},
};

int console_title [8] = {"System"};


static void main(void)
{

 int initialised = 0;
 int success;
 int test = 2;
 int player_factory_x;
 int player_factory_y;
 counter ++;

 if (initialised == 0)
 {
// Set up some colours:
// MS_OB_VIEW_COLOUR_BACK is the background colour. Other values are r, g, b
  call(METH_VIEW, MS_OB_VIEW_COLOUR_BACK, 21, 21, 21); // background colours are updated below as well
// MS_OB_VIEW_COLOUR_BACK2 is the colour of the little hexagonal cells in the background
  call(METH_VIEW, MS_OB_VIEW_COLOUR_BACK2, 90, 80, 90);
// recolour enemy player
  call(METH_VIEW, MS_OB_VIEW_COLOUR_PROC, 1, // player 1
							180, 190, // min and max red intensity
							20, 20, // min and max g intensity
							150, 40); // min and max b intensity

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
// Now make sure error messages for the enemy are discarded (these can be e.g. too many processes produced):
  call(METH_CONSOLE, MS_OB_CONSOLE_ERR_PLAYER, -1, 1); // remove this line if you need to debug the enemy processes

// load the observer (the observer program is automatically loaded from TEMPLATE_OBSERVER). Does nothing if no observer
  call(METH_MANAGE, MS_SY_MANAGE_LOAD_OBSERVER);

// Now load player 0's client (the client program is automatically loaded from TEMPLATE_P0_CLIENT)
  call(METH_MANAGE, MS_SY_MANAGE_LOAD_CLIENT, 0);
// Set player 0's allocate effect to "align" (allocator methods align the background hexes rather than disrupting them; default is disrupt)
  call(METH_MANAGE, MS_SY_MANAGE_ALLOCATE_EFFECT, 0, ALLOCATE_EFFECT_ALIGN);

// Now set up templates for player 1 (the attacker)
  call(METH_TEMPLATE, TEMPLATE_P1_CLIENT, process_start(enemy_delegate), process_end(enemy_delegate), &process_name [0] [0]);
  call(METH_MANAGE, MS_SY_MANAGE_LOAD_CLIENT, 1);

  call(METH_TEMPLATE, TEMPLATE_P1_PROCESS_0, process_start(spider_process), process_end(spider_process), &process_name [1] [0]);
  call(METH_TEMPLATE, TEMPLATE_P1_PROCESS_1, process_start(spiderm_process), process_end(spiderm_process), &process_name [2] [0]);

// We could check the return values of these calls, but here it is safe to assume they'll be successful (if not, nothing is going to work anyway)

  player_factory_x = 1000;
  player_factory_y = world_size_y / 2;

  initialised = 1; // make sure this initialisation code doesn't run again
  game_state = GSTATE_PREGAME;

  call(METH_VIEW, MS_OB_VIEW_FOCUS_XY, player_factory_x + 300, player_factory_y); // Sets camera location (the coordinates are the centre of the display)

 }


 if (counter % 40 == 0)
	{
 	int number_of_enemies;
 	number_of_enemies = call(METH_STD, MS_CLOB_STD_WORLD_TEAM_SIZE, 1);
  call(METH_VIEW, MS_OB_VIEW_COLOUR_BACK, 21 + (number_of_enemies / 5), 21, 21 + (number_of_enemies / 8));
  call(METH_VIEW, MS_OB_VIEW_COLOUR_BACK2, 90 + number_of_enemies, 80, 90 + number_of_enemies);
	}

// Now we check whether the game state has changed. Each game state has specific conditions that cause the next state to begin:
 switch(game_state)
 {
  case GSTATE_PREGAME:
// This status waits for the end of the pregame phase (16 ticks at the start during which the system can set up)
   if (call(METH_MANAGE, MS_SY_MANAGE_PHASE) == PHASE_PREGAME)
    break; // nothing to do.
// Okay, pregame is over:
   call(METH_CONSOLE, MS_OB_CONSOLE_COLOUR, COL_DRED);
   print("\n *** WARNING ***");
   call(METH_CONSOLE, MS_OB_CONSOLE_COLOUR, COL_LBLUE);
   print("\n\nA highly virulent self-reproducing infection has been detected!");
   print("\n\nRepel all hostile processes!!");
   call(METH_CONSOLE, MS_OB_CONSOLE_COLOUR, COL_LPURPLE);
   print("\n\n >> Click here to begin (processes will spawn from template 0) <<");
   call(METH_CONSOLE, MS_OB_CONSOLE_ACTION_SET, SYSTEM_CONSOLE, 1); // associates action 1 with last printed line
   game_state = GSTATE_SPAWN_ENEMY;
   break;


  case GSTATE_SPAWN_ENEMY:
// This status waits for the user to click the action line in the console
   if (call(METH_CONSOLE, MS_OB_CONSOLE_ACTION_CHECK) != CONSOLE_ACTION_SYSTEM)
    break;
   call(METH_CONSOLE, MS_OB_CONSOLE_COLOUR, COL_LBLUE);
   print("\n\nStarting...");
   call(METH_MANAGE, MS_SY_MANAGE_START_TURN);
   call(METH_VIEW, MS_OB_VIEW_SOUND, 0, 18, 100);
   game_state = GSTATE_BUILD;
// Now, place a process for the player:
   call(METH_PLACE,
        MS_SY_PLACE_T_CALL,  // MS_SY_PLACE_T_CALL - place from template
        1000, // x
        world_size_y / 2, // y
        0, // angle
        0, // start address
        BCODE_SIZE_PROCESS - 1, // end address (this means use the whole template)
        0, // player
        TEMPLATE_P0_PROCESS_0); // template access index - this is the template's actual number
/*   call(METH_PLACE,
        MS_SY_PLACE_T_CALL,
        1000,
        (world_size_y * 2) / 3); // don't need to reset the other registers*/
   int spider_index;
   spider_index = spawn_enemy(TEMPLATE_P1_PROCESS_0, world_size_x - 2000, world_size_y / 2, ANGLE_2);
   call(METH_MODIFY, MS_SY_MODIFY_DATA, spider_index, 1000);
   break;

  case GSTATE_BUILD:
   call(METH_MODIFY, MS_SY_MODIFY_DATA, spider_index, 1000);
  	int build_counter = 0;
  	build_counter ++;
  	if (build_counter >= 100)
				game_state = GSTATE_SCRAMBLE;
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
    print("\n\nMISSION COMPLETE.");
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
    print("\n\nYour system has been compromised.");
    print("\n\nMISSION FAILED.");
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


// returns index of newly created process
static int spawn_enemy(int source_template, int x, int y, int angle)
{

// For this one, we need to allow for the possibility that the player has moved processes to the spawn positions.
// Although the player can't have too many processes, so don't try too hard.

 int spawned;
 int success;

 success = try_spawn(source_template, x, y, angle);

 if (success != MR_NEW_SUCCESS) // SY_PLACE uses the same result values as PR_NEW
 {
  success = try_spawn(source_template, x, y - 300, angle);
  if (success != MR_NEW_SUCCESS) // SY_PLACE uses the same result values as PR_NEW
  {
   success = try_spawn(source_template, x, x + 300, angle);
  }
 }

 if (success != MR_NEW_SUCCESS)
		return -1; // shouldn't happen

	int retval;
	retval = get(METH_PLACE, MB_SY_PLACE_STATUS);

	return get(METH_PLACE, MB_SY_PLACE_STATUS);

}

static int try_spawn(int source_template, int x, int y, int angle)
{

 return call(METH_PLACE,
        MS_SY_PLACE_T_CALL,  // MS_SY_PLACE_T_CALL - place from template
        x, // x
        y, // y
        angle, // angle
        0, // start address
        BCODE_SIZE_PROCESS - 1, // end address
        1, // player
        source_template); // template access index - this is the template's actual number

}



process enemy_delegate
{
#include "de_m6.c"
}

process spider_process
{
#include "pr_spider.c"
}

process spiderm_process
{
#include "pr_spiderm.c"
}
