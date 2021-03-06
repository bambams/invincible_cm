/*

This file contains code for the setup menu (which lets the user setup a world with whatever settings are wanted).

Basically it contains setup functions that are called from s_menu.c when various things are done in the setup menu.

Plan:
Missions will work like this:
 - at start, a game program will be loaded in.
 - this program will contain setup routines that will load other programs into hidden game templates (will need to revise template code a bit to deal with this)
  - the hidden templates may just be player zero's templates, which the game program will have access to (along with other templates).
 - it will then be able to make use of these templates as needed.
 - it will also use a special game method to specify what templates the player may use.
  - it will need a set of game methods, only available to game programs, that will allow it to do things like this.
 - after it has run once, it will modify itself so as not to run these setup routines again.
Then, the player will:
 - see a text description of the mission and a "start" button. The player will be able to use the template and editor windows. Templates may come pre-loaded but may be changeable.
 - when clicks "start", proc zero will be placed in world and game will start to run.

How will setup work?
 - A bit like missions, except that the user will be able to choose:
  - number of players
  - what game program to load, if any.
 - if a game program is loaded, some or all of the other settings may be bypassed.
  - should give notice of this using console.
  - if not bypassed, game program will need access to these settings.
 - other things to choose:
  - number of procs
  - number of packets per player
  - size of world




So how is the actual game going to work? This is something I haven't really put enough thought into.
I think I've resolved that the default will be placing a single proc of user's choice (from the template window) into the world.
Most likely there will then be x control points around the map.
 - vacant control points are captured by a proc moving onto one.
 - once claimed, a control point may be captured by another player's proc if no proc of owner's team is on it.
 - control points increase score. May give other benefits as well.
Yeah, I think this works.
Procs will need some way of sensing control points; client programs will probably need some way of finding out where they are.



So what do I do next?
 - Add data structures for players. Probably have 5; user 0 can only be computer defender.
 - each player struct should have:
  - colours
  - proc index min and max
  - bcodestruct for client program
  - flag whether player is controlling the interface
   - client program will only have access to interface methods if it is the controlling user
   - if no user controller, then there will be an observer program as well
  - template structs
  - score


Two additional bcodestructs:
 - observer (handles user interface; whether one is needed will depend on whether the client programs are allowed to do this)
 - game




*/




#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>

#include <stdio.h>
#include <string.h>

#include "m_config.h"

#include "g_header.h"
#include "m_globvars.h"

#include "g_misc.h"
#include "g_method_misc.h"

#include "c_header.h"
#include "e_slider.h"
#include "e_header.h"
#include "e_log.h"

#include "c_init.h"
#include "c_prepr.h"
#include "i_input.h"
#include "i_view.h"
#include "m_input.h"
#include "t_template.h"
#include "t_files.h"

#include "s_setup.h"




extern struct world_preinitstruct w_pre_init; // whenever a world setup menu is entered (from above), this is set up with appropriate values (from gamefile or default values, as needed)
extern struct world_initstruct w_init; // this is the world_init generated by world setup menus
extern struct templstruct templ [TEMPLATES]; // in t_template.c

int get_init_array_from_bcode(int* pre_init_array, int elements, struct programstruct* cl, int bcode_pos);
int get_init_value_from_bcode(int* pre_init_value, struct programstruct* cl, int bcode_pos);

void fix_world_pre_init(struct world_preinitstruct* pre_init);
void fix_pre_init_array(int* value, int default_value, int min, int max);
void fix_pre_init_value_on_or_off(int* val);

int find_player_client_template(int p, int allow_operator);

// This function sets default values for a world_preinitstruct. These values are used to set up the startup menu based on values in a game program's interface definition.
// they are also the default values used for the setup game menu.
void setup_default_world_pre_init(struct world_preinitstruct* pre_init)
{

 int i;

 pre_init->players [PI_OPTION] = 1;
 pre_init->players [PI_DEFAULT] = DEFAULT_PLAYERS;
 pre_init->players [PI_MIN] = MIN_PLAYERS;
 pre_init->players [PI_MAX] = MAX_PLAYERS;

 pre_init->game_turns [PI_OPTION] = 1;
 pre_init->game_turns [PI_DEFAULT] = 0; // indefinite
 pre_init->game_turns [PI_MIN] = 0;
 pre_init->game_turns [PI_MAX] = MAX_LIMITED_GAME_TURNS;

 pre_init->game_minutes_each_turn [PI_OPTION] = 1;
 pre_init->game_minutes_each_turn [PI_DEFAULT] = 5;
 pre_init->game_minutes_each_turn [PI_MIN] = 1;
 pre_init->game_minutes_each_turn [PI_MAX] = MAX_TURN_LENGTH_MINUTES;

 pre_init->procs_per_team [PI_OPTION] = 1;
 pre_init->procs_per_team [PI_DEFAULT] = DEFAULT_PROCS;
 pre_init->procs_per_team [PI_MIN] = MIN_PROCS_PER_TEAM; // hopefully this won't cause any problems
 pre_init->procs_per_team [PI_MAX] = MAX_PROCS_PER_TEAM;

// pre_init->gen_limit [PI_OPTION] = 1;
// pre_init->gen_limit [PI_DEFAULT] = DEFAULT_GEN_LIMIT;
// pre_init->gen_limit [PI_MIN] = MIN_GEN_LIMIT; // hopefully this won't cause any problems
// pre_init->gen_limit [PI_MAX] = MAX_GEN_LIMIT;

 pre_init->packets_per_team [PI_OPTION] = 1;
 pre_init->packets_per_team [PI_DEFAULT] = DEFAULT_PACKETS;
 pre_init->packets_per_team [PI_MIN] = MIN_PACKETS; // hopefully this won't cause any problems
 pre_init->packets_per_team [PI_MAX] = MAX_PACKETS;

// init->max_clouds = CLOUDS;

 pre_init->w_block [PI_OPTION] = 1;
 pre_init->w_block [PI_DEFAULT] = DEFAULT_BLOCKS;
 pre_init->w_block [PI_MIN] = MIN_BLOCKS;
 pre_init->w_block [PI_MAX] = MAX_BLOCKS;

 pre_init->h_block [PI_OPTION] = 1;
 pre_init->h_block [PI_DEFAULT] = DEFAULT_BLOCKS;
 pre_init->h_block [PI_MIN] = MIN_BLOCKS;
 pre_init->h_block [PI_MAX] = MAX_BLOCKS;
/*
 pre_init->choose_col = 1;

 pre_init->team_col [0] [0] = 20;
 pre_init->team_col [0] [1] = 220;
 pre_init->team_col [0] [2] = 50;

 pre_init->team_col [1] [0] = 200;
 pre_init->team_col [1] [1] = 5;
 pre_init->team_col [1] [2] = 20;

 pre_init->team_col [2] [0] = 50;
 pre_init->team_col [2] [1] = 50;
 pre_init->team_col [2] [2] = 220;

 pre_init->team_col [3] [0] = 180;
 pre_init->team_col [3] [1] = 150;
 pre_init->team_col [3] [2] = 20;*/

 pre_init->allow_player_clients = 1;
 pre_init->player_operator = -1;
 pre_init->allow_user_observer = 1;

 for (i = 0; i < PLAYERS; i ++)
 {
  pre_init->may_change_client_template [i] = 1;
  pre_init->may_change_proc_templates [i] = 1;
  sprintf(pre_init->player_name [i], "Player %i", i);
 }


}

/*

This function reads the interface definition from cl's bcode (cl must be a system program) and uses it to set up w_pre_init with world initialisation values

All values are bounds-checked by a call to fix_world_pre_init() (below)
Assumes that bcode_pos doesn't need to be bounds_checked (as it should be right at the start of cl's bcode)

** remember - any changes in this function may need to be reflected in intercode_process_header() in c_inter.c and parse_system_program_interface() in c_comp.c

Returns new bcode_pos on success, -1 on failure
*/
int derive_pre_init_from_system_program(struct programstruct* cl, int bcode_pos)
{

// these must be in the same order as in intercode_process_header() in c_inter.c
//  and also parse_system_program_interface() in c_comp.c
//  and bounds-checking may need to be added in fix_world_pre_init()

// PI_VALUES arrays:
// some game parameters
 bcode_pos = get_init_array_from_bcode(w_pre_init.players, PI_VALUES, cl, bcode_pos);
 bcode_pos = get_init_array_from_bcode(w_pre_init.game_turns, PI_VALUES, cl, bcode_pos);
 bcode_pos = get_init_array_from_bcode(w_pre_init.game_minutes_each_turn, PI_VALUES, cl, bcode_pos);

// some world values
 bcode_pos = get_init_array_from_bcode(w_pre_init.procs_per_team, PI_VALUES, cl, bcode_pos);
// bcode_pos = get_init_array_from_bcode(w_pre_init.gen_limit, PI_VALUES, cl, bcode_pos);
 bcode_pos = get_init_array_from_bcode(w_pre_init.packets_per_team, PI_VALUES, cl, bcode_pos);
 bcode_pos = get_init_array_from_bcode(w_pre_init.w_block, PI_VALUES, cl, bcode_pos);
 bcode_pos = get_init_array_from_bcode(w_pre_init.h_block, PI_VALUES, cl, bcode_pos);

// some individual values
 bcode_pos = get_init_value_from_bcode(&w_pre_init.allow_player_clients, cl, bcode_pos);
 bcode_pos = get_init_value_from_bcode(&w_pre_init.player_operator, cl, bcode_pos);
 bcode_pos = get_init_value_from_bcode(&w_pre_init.allow_user_observer, cl, bcode_pos);

// PLAYERS arrays
 bcode_pos = get_init_array_from_bcode(w_pre_init.may_change_client_template, PLAYERS, cl, bcode_pos);
 bcode_pos = get_init_array_from_bcode(w_pre_init.may_change_proc_templates, PLAYERS, cl, bcode_pos);

 int i;

 for (i = 0; i < PLAYERS; i ++)
 {
  sprintf(w_pre_init.player_name [i], "Player %i", i);
 }

 fix_world_pre_init(&w_pre_init);

 return bcode_pos;

}



// returns new value of bcode_pos
int get_init_array_from_bcode(int* pre_init_array, int elements, struct programstruct* cl, int bcode_pos)
{
 int i;

 for (i = 0; i < elements; i ++)
 {
  pre_init_array [i] = cl->bcode.op [bcode_pos];
  bcode_pos ++;
 }

 return bcode_pos;

}

// returns new value of bcode_pos
int get_init_value_from_bcode(int* pre_init_value, struct programstruct* cl, int bcode_pos)
{
 *pre_init_value = cl->bcode.op [bcode_pos];
 bcode_pos ++;
 return bcode_pos;
}
// this confirms that pre_init values are within acceptable limits.
// if they aren't, it sets them appropriately
// TO DO: think about some kind of notification if a value reset
void fix_world_pre_init(struct world_preinitstruct* pre_init)
{

 fix_pre_init_array(pre_init->players, DEFAULT_PLAYERS, MIN_PLAYERS, MAX_PLAYERS);
 fix_pre_init_array(pre_init->game_turns, 0, 0, MAX_LIMITED_GAME_TURNS);
 fix_pre_init_array(pre_init->game_minutes_each_turn, 5, 0, MAX_TURN_LENGTH_MINUTES);

 fix_pre_init_array(pre_init->procs_per_team, DEFAULT_PROCS, MIN_PROCS_PER_TEAM, MAX_PROCS_PER_TEAM);
// fix_pre_init_array(pre_init->gen_limit, DEFAULT_GEN_LIMIT, MIN_GEN_LIMIT, MAX_GEN_LIMIT);
 fix_pre_init_array(pre_init->packets_per_team, DEFAULT_PACKETS, MIN_PACKETS, MAX_PACKETS);
 fix_pre_init_array(pre_init->w_block, DEFAULT_BLOCKS, MIN_BLOCKS, MAX_BLOCKS);
 fix_pre_init_array(pre_init->h_block, DEFAULT_BLOCKS, MIN_BLOCKS, MAX_BLOCKS);

 fix_pre_init_value_on_or_off(&pre_init->allow_player_clients);
// fix_pre_init_value_on_or_off(&pre_init->player_1_operator); // don't need to fix this as it can be -1 to PLAYERS and doesn't matter if it's out of bounds anyway
 fix_pre_init_value_on_or_off(&pre_init->allow_user_observer);

 int i;

 for (i = 0; i < PLAYERS; i ++)
 {
  fix_pre_init_value_on_or_off(&pre_init->may_change_client_template [i]);
  fix_pre_init_value_on_or_off(&pre_init->may_change_proc_templates [i]);
 }

/*
 int i, j;

 for (i = 0; i < PLAYERS; i ++)
 {
  for (j = 0; j < 3; j ++)
  {
   if (pre_init->team_col [i] [j] < 0)
    pre_init->team_col [i] [j] = 0;
   if (pre_init->team_col [i] [j] > 255)
    pre_init->team_col [i] [j] = 255;
  }
 }*/

}

// pass a pointer to a 4-value int array to this, and it will verify that they are acceptable pre_init values and fix if not
void fix_pre_init_array(int* value, int default_value, int min, int max)
{

 if (value [PI_OPTION] == 0)
 {
  if (value [PI_DEFAULT] == -1)
   value [PI_DEFAULT] = default_value;
  if (value [PI_DEFAULT] < min)
   value [PI_DEFAULT] = min;
  if (value [PI_DEFAULT] > max)
   value [PI_DEFAULT] = max;

  return; // if value can't be changed, just use the PI_DEFAULT or, if PI_DEFAULT is -1, use the built-in default. don't need to set PI_MIN or MAX
 }

 if (value [PI_MIN] == 0)
  value [PI_MIN] = min;
 if (value [PI_MAX] == 0)
  value [PI_MAX] = max;

 if (value [PI_MIN] < min)
  value [PI_MIN] = min;
 if (value [PI_MIN] > max)
  value [PI_MIN] = max;

 if (value [PI_MAX] > max
  || value [PI_MAX] == -1)
  value [PI_MAX] = max;
 if (value [PI_MAX] < value [PI_MIN])
  value [PI_MAX] = value [PI_MIN];

 if (value [PI_DEFAULT] == -1)
  value [PI_DEFAULT] = default_value;
 if (value [PI_DEFAULT] < value [PI_MIN])
  value [PI_DEFAULT] = value [PI_MIN];
 if (value [PI_DEFAULT] > value [PI_MAX])
  value [PI_DEFAULT] = value [PI_MAX];

}


// fixes a pre_init value to 0 or 1
void fix_pre_init_value_on_or_off(int* val)
{

 if (*val < 0)
  *val = 0;
 if (*val > 1)
  *val = 1;

}


/*
sets up a game using a gamefile in a template (finds first template containing a gamefile)
fills in w_pre_init with values from gamefile, and sets up w.system_program from the template

currently gamefile must be a system program

returns 1 success, 0 fail (on failure, writes to mlog)
*/
int use_sysfile_from_template(void)
{

 int t = find_loaded_gamefile_template();

 if (t == -1) // couldn't find a gamefile template with gamefile loaded
 {
  write_line_to_log("A system file must be loaded into the system file template.", MLOG_COL_ERROR);
// TO DO: this error message needs to be a bit more useful
  return 0;
 }

 if (!copy_template_to_program(t, &w.system_program, PROGRAM_TYPE_SYSTEM, -1)) // -1 is player (no player)
 {
// copy_template_to_program should also have written an error message to mlog on failure
  return 0;
 }

// at this point copy_template_to_program() has called derive_program_properties_from_bcode(), which will have set up w_pre_init with values from the system program.

 return 1;

}


