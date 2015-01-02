/*

This is a system program set up for basic multiplayer.

*/

interface
{
 PROGRAM_TYPE_SYSTEM,
 // The following are: option, default, min, max
 // If min or max is left as 0, the game's actual min/max values will be used
 {1, 2, 2}, // players
 {1, 3, 0}, // turns
 {1, 5, 1}, // minutes per turn
 {1, 100}, // procs per team
 {1, 400}, // packets per team
 {1, 60}, // w_block
 {1, 60}, // h_block
 1, // allow_player_clients
 -1, // index of player who is operator (is -1 if no operator)
 1, // allow_user_observer
 {1, 1, 1, 1}, // may_change_client_template
 {1, 1, 1, 1}, // may_change_proc_templates
 {
  METH_PLACE: {MT_SY_PLACE_PROC},
  {MT_NONE}, // space for PLACE_PROC
  METH_TEMPLATE: {MT_SY_TEMPLATE},
  METH_MODIFY: {MT_SY_MODIFY},
  METH_MANAGE: {MT_SY_MANAGE},
  {MT_NONE}, // space for MANAGE
  METH_VIEW: {MT_OB_VIEW},
  {MT_NONE}, // space for VIEW
  METH_CONSOLE: {MT_OB_CONSOLE},
  METH_QUERY: {MT_CLOB_QUERY},
  METH_STD: {MT_CLOB_STD},
  METH_MATHS: {MT_CLOB_MATHS}
 }
}


// These are declarations for processes that will be defined later.
process observer;

// Now some variables that the system will use to keep track of the game's state:
int game_state;
int players;

enum
{
GSTATE_PREGAME, // in pregame mode, or waiting for user to press start button in system menu
GSTATE_START, // display intro and waiting for user to click on factory
GSTATE_PLAYING, // enemies appear! Waiting for user to destroy them. Can move to won or lost
GSTATE_TURN, // between turns
//GSTATE_FINISHED, // someone won
//GSTATE_END // finished
};

static void spawn_player_process(int p, int x, int y, int angle, int source_template);
static void process_creation_error(int p, int error_code);

// This is how to initialise strings.
// Note that unlike in C you can't use [] as a dimension and let the compiler work it out (it's not that smart).
// Also, each dimension must be surrounded by braces.
int observer_name [10] = {"Observer"};
int console_title [8] = {"System"};

static void main(void)
{

 int initialised = 0;
 int success;
 int i;

 if (initialised == 0)
 {

// The system program uses console 7 (and leaves the other consoles to the observer):
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

// Now load the observer process into the observer template. Note that the user can replace it if they want:
// The template will be activated at the end of pregame (see below)
  call(METH_TEMPLATE, TEMPLATE_OBSERVER, process_start(observer), process_end(observer), &observer_name [0]);
// Set the camera:
  call(METH_VIEW, MS_OB_VIEW_FOCUS_XY, world_x() / 2, world_y() / 2);

// Find out how many players there are:
  players = world_teams();

  initialised = 1;
  game_state = GSTATE_PREGAME;

 }


// Now we check whether the game state has changed. Each game state has specific conditions that cause the next state to begin:
 switch(game_state)
 {
  case GSTATE_PREGAME:
// This status waits for the end of the pregame phase
   if (call(METH_MANAGE, MS_SY_MANAGE_PHASE) == PHASE_PREGAME)
    break; // nothing to do.
// Okay, pregame is over:
   call(METH_CONSOLE, MS_OB_CONSOLE_COLOUR, COL_LBLUE);
   print("\nWelcome to a multiplayer game for ", players, " players!");
   print("\n\nIf you're setting up your turn, press the Te button to open the");
   print("\ntemplate menu and load programs into your templates.");
   print("\nWhen you've finished, click the Save turnfile button.");
   print("\n\nWhen you have all players' turnfiles, open the Te menu and");
   print("\nload them in using Load turnfile, then click on the line below:");
   call(METH_CONSOLE, MS_OB_CONSOLE_COLOUR, COL_LPURPLE);
   print("\n\n >> Click here to begin <<");
   call(METH_CONSOLE, MS_OB_CONSOLE_ACTION_SET, SYSTEM_CONSOLE, 1);
   game_state = GSTATE_START;
   break;

  case GSTATE_START:
// This status waits for the user to click the action line in the console
   if (call(METH_CONSOLE, MS_OB_CONSOLE_ACTION_CHECK) != CONSOLE_ACTION_SYSTEM)
    break;
   call(METH_VIEW, MS_OB_VIEW_SOUND, 0, 18, 100);
// First activate the observer:
   call(METH_MANAGE, MS_SY_MANAGE_LOAD_OBSERVER);
// Now activate each player's client, if any:
   for (i = 0; i < players; i ++)
   {
    call(METH_MANAGE, MS_SY_MANAGE_LOAD_CLIENT, i); // Does nothing if no client loaded
   }
// Now try to place a process from each player's process template 0:
   int world_width;
   world_width = (world_x() - 1600) / 2;
   int world_height;
   world_height = (world_y() - 1600) / 2;
   int centre_x;
   centre_x = world_x() / 2;
   int centre_y;
   centre_y = world_y() / 2;
   int source_template;
   for (i = 0; i < players; i ++)
   {
    if (i == 0)
     source_template = TEMPLATE_P0_PROCESS_0;
    if (i == 1)
     source_template = TEMPLATE_P1_PROCESS_0;
    if (i == 2)
     source_template = TEMPLATE_P2_PROCESS_0;
    if (i == 3)
     source_template = TEMPLATE_P3_PROCESS_0;

    spawn_player_process(i,
                         centre_x + cos((ANGLE_1 / players) * i, world_width),
                         centre_y + sin((ANGLE_1 / players) * i, world_height),
                         ((ANGLE_1 / players) * i) + ANGLE_2,
                         source_template);
   }
// So now we start the game:
   call(METH_MANAGE, MS_SY_MANAGE_START_TURN);
   print("\n\nStarting...");
   game_state = GSTATE_PLAYING;
   break;
  case GSTATE_PLAYING:
// Now check whether either player has been eliminated:
   int players_left;
   int surviving_player;
   players_left = 0;

   for (i = 0; i < players; i ++)
   {
    if (call(METH_STD, MS_CLOB_STD_WORLD_TEAM_SIZE, i) > 0)
    {
     players_left ++;
     surviving_player = i;
    }
   }

   if (players_left == 0)
   {
// all of players' last procs eliminated at the same time
    call(METH_MANAGE, MS_SY_MANAGE_GAMEOVER, GAME_END_DRAW);
   }

   if (players_left == 1)
   {
    call(METH_MANAGE, MS_SY_MANAGE_GAMEOVER, GAME_END_PLAYER_WON, surviving_player);
   }

   if (call(METH_MANAGE, MS_SY_MANAGE_PHASE) == PHASE_TURN)
   {
// Now print the end-of-turn text and change to GSTATE_TURN, which waits for the player to click on the action line:
    call(METH_CONSOLE, MS_OB_CONSOLE_COLOUR, COL_LBLUE);
    print("\n\nTurn complete!");
    print("\n\nNow you can load new programs into your templates, if you want to.");
    print("\n(These will be used when your processes create new processes.)");
    print("\n\nWhen you have finished, use Save turnfile and share turnfiles with");
    print("\nother players. You can also save the game at this point.");
    print("\nThen, when you have all turnfiles loaded:");
    call(METH_CONSOLE, MS_OB_CONSOLE_COLOUR, COL_LPURPLE);
    print("\n\n >> Click here to begin next turn <<");
    call(METH_CONSOLE, MS_OB_CONSOLE_ACTION_SET, SYSTEM_CONSOLE, 1);
    game_state = GSTATE_TURN;
   }
   break;
  case GSTATE_TURN:
// This status waits for the user to click the action line in the console
   if (call(METH_CONSOLE, MS_OB_CONSOLE_ACTION_CHECK) != CONSOLE_ACTION_SYSTEM)
    break;
   call(METH_VIEW, MS_OB_VIEW_SOUND, 0, 18, 100);
// So now we start the turn:
   call(METH_MANAGE, MS_SY_MANAGE_START_TURN);
// and tell the user:
   print("\n\nStarting turn...");
   game_state = GSTATE_PLAYING;
// Now activate each player's client, if any:
   for (i = 0; i < players; i ++)
   {
    call(METH_MANAGE, MS_SY_MANAGE_LOAD_CLIENT, i);
// This won't do anything (including replacing an existing client) if no client is loaded into the template
   }
// Might as well reload the observer in case the user has updated it:
   call(METH_MANAGE, MS_SY_MANAGE_LOAD_OBSERVER);
// Also won't do anything if no observer loaded
   break;

 }


 int step_forward;

 if (step_forward == 1)
 {
  call(METH_MANAGE, MS_SY_MANAGE_PAUSE_SET, 1); // pause
  step_forward = 0;
 }

}


static void spawn_player_process(int p, int x, int y, int angle, int source_template)
{

// This function places a player's first process.
// Players have 500 data to use - any that isn't used will be given to the process (if it fits)

  int success;
  int process_index;
  int data_cost;

// First work out the cost of the player's process:
  success = call(METH_PLACE,
       MS_SY_PLACE_T_COST_DATA, // gets the cost of building a template
       0, // MB_PR_NEW_VERTEX1: Vertex of this process that new process will be created at.
       0, // MB_PR_NEW_VERTEX2: Vertex of child process (-1 means use child process' base_vertex interface value)
       0, // MB_PR_NEW_ANGLE: Angle of child process (0 means pointing directly away)
       0, // MB_PR_NEW_START: start address of new process' code
       BCODE_SIZE_PROCESS - 1, // MB_PR_NEW_END: end address
       0, // MB_PR_NEW_LINK: 1 means try to connect (does nothing if fails)
       source_template); // MB_PR_NEW_TEMPLATE: Source template index (not relevant here; only used when building from a template)
// This call leaves the cost in register 0 of the PLACE method:
  if (success != MR_NEW_TEST_SUCCESS)
  {
   process_creation_error(p, success);
   return;
  }

  data_cost = get(METH_PLACE, 0);

// Now, place a process for the player:
  success = call(METH_PLACE,
       MS_SY_PLACE_T_CALL,  // MS_SY_PLACE_T_CALL - place from template
       x, // x
       y, // y
       angle, // angle
       0, // start address
       BCODE_SIZE_PROCESS - 1, // end address
       p, // player
       source_template); // template access index - this is the template's actual number

  if (success != MR_NEW_SUCCESS)
  {
   process_creation_error(p, success);
   return;
  }

  process_index = get(METH_PLACE, 0);

  call(METH_MODIFY, MS_SY_MODIFY_DATA, process_index, 500 - data_cost);

}

static void process_creation_error(int p, int error_code)
{

 call(METH_CONSOLE, MS_OB_CONSOLE_COLOUR, COL_LRED);

 print("\nWarning: Player ",p,"'s process could not be placed.");

 switch(error_code)
 {

     case MR_NEW_FAIL_TYPE: print("\nFailure: invalid type."); break;
     case MR_NEW_FAIL_INTERFACE: print("\nFailure: invalid interface."); break;
     case MR_NEW_FAIL_SHAPE: print("\nFailure: invalid shape."); break;
     case MR_NEW_FAIL_SIZE: print("\nFailure: invalid size."); break;
     case MR_NEW_FAIL_TEMPLATE: print("\nFailure: invalid template."); break;
     case MR_NEW_FAIL_TEMPLATE_EMPTY: print("\nFailure: template not loaded."); break;
     default: print("\nFailure: unknown reason (", error_code, ")"); break;

 }

 call(METH_CONSOLE, MS_OB_CONSOLE_COLOUR, COL_LBLUE);


}


process observer
{
#include "ob_basic.c"
}


