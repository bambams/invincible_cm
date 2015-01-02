#include <allegro5/allegro.h>

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
//#include <limits.h>

#include "m_config.h"
#include "m_globvars.h"

#define G_WORLD
#include "g_header.h"
#undef G_WORLD

#include "g_motion.h"
#include "g_proc.h"
#include "g_packet.h"
#include "g_cloud.h"
#include "g_client.h"
#include "g_world.h"

#include "g_misc.h"

#include "c_header.h"
#include "c_comp.h"
#include "t_template.h"

#include "m_maths.h"
#include "i_disp_in.h"


extern int current_check; // see g_group.c

//void initialise_world(void);
//void new_world(int max_procs, int max_packets, int block_w, int block_h);
//void new_world_from_world_init(void);

void place_proc_randomly(struct procstruct* pr);
void init_block_nodes(void);

//void change_block_node(struct blockstruct* bl, int i, int new_x, int new_y, int new_size);

//void init_team_colour(struct playerstruct* pl, int base_cols [3]);

extern struct world_initstruct w_init;


// This function starts the game, deriving world parameters from a world_init struct.
void start_world(void)
{

 new_world_from_world_init();

}



// This function takes a world_initstruct and sets up a worldstruct based on the world_initstruct.
// Does not set up program stuff. That's done in setup_player_programs_from_templates() in s_setup.c
// Currently, any memory allocation failure causes a fatal error. Could change this (but it would require deallocation of a partially allocated world, which isn't implemented yet)
void new_world_from_world_init(void)
{

 int i, j;

 w.players = w_init.players;

 w.max_procs = w_init.procs_per_team * w_init.players;
 w.procs_per_team = w_init.procs_per_team;
// w.gen_limit = w_init.gen_limit;
 w.max_packets = w_init.packets_per_team * w_init.players;
 w.max_clouds = w_init.max_clouds;

 w.w_block = w_init.w_block; // if this is too low bad things may happen. 20 is probably a lower limit.
 w.h_block = w_init.h_block;

 w.w_fixed = block_to_fixed(w.w_block);
 w.h_fixed = block_to_fixed(w.h_block);

 w.w_pixels = al_fixtoi(w.w_fixed);
 w.h_pixels = al_fixtoi(w.h_fixed);

// fprintf(stdout, "\nWorld size %i x %i", w.w_block, w.h_block);
// error_call();

// when adding any dynamic memory allocation to this function, remember to free the memory in deallocate_world() below
 w.block = calloc(w.w_block, sizeof(struct blockstruct*));
 if (w.block == NULL)
 {
      fprintf(stdout, "g_world.c: Out of memory in allocating w.block");
      error_call();
 }
 for (i = 0; i < w.w_block; i ++)
 {
   w.block [i] = calloc(w.h_block, sizeof(struct blockstruct));
   if (w.block [i] == NULL)
   {
      fprintf(stdout, "g_world.c: Out of memory in allocating w.block");
      error_call();
   }
 }

 for (i = 0; i < w.w_block; i ++)
 {
  for (j = 0; j < w.h_block; j ++)
  {
   w.block [i][j].block_type = BLOCK_NORMAL;
  }
 }

 for (i = 0; i < w.w_block; i ++)
 {
/*  w.block [i] [0].block_type = BLOCK_FAR_TOP;
  w.block [i] [1].block_type = BLOCK_FAR_TOP;
  w.block [i] [2].block_type = BLOCK_TOP;
  w.block [i] [w.h_block - 1].block_type = BLOCK_FAR_BOTTOM;
  w.block [i] [w.h_block - 2].block_type = BLOCK_FAR_BOTTOM;
  w.block [i] [w.h_block - 3].block_type = BLOCK_BOTTOM;*/

  w.block [i] [0].block_type = BLOCK_SOLID;
  w.block [i] [1].block_type = BLOCK_SOLID;
  w.block [i] [2].block_type = BLOCK_EDGE_UP;
  w.block [i] [w.h_block - 1].block_type = BLOCK_SOLID;
  w.block [i] [w.h_block - 2].block_type = BLOCK_SOLID;
  w.block [i] [w.h_block - 3].block_type = BLOCK_EDGE_DOWN;
 }
 for (i = 0; i < w.h_block; i ++)
 {
  w.block [0] [i].block_type = BLOCK_SOLID;
  w.block [1] [i].block_type = BLOCK_SOLID;
//  if (i>0&&i<(w.h_block-1)) w.block [1] [i].block_type = BLOCK_SOLID;
  if (i>1&&i<(w.h_block-2)) w.block [2] [i].block_type = BLOCK_EDGE_LEFT;
//  if (i>0&&i<(w.h_block-1)) w.block [1] [i].block_type = BLOCK_SOLID;
//  if (i>1&&i<(w.h_block-2)) w.block [2] [i].block_type = BLOCK_SOLID;
  w.block [w.w_block - 1] [i].block_type = BLOCK_SOLID;
  w.block [w.w_block - 2] [i].block_type = BLOCK_SOLID;
//  if (i>0&&i<(w.h_block-1)) w.block [w.h_block - 2] [i].block_type = BLOCK_SOLID;
  if (i>1&&i<(w.h_block-2)) w.block [w.w_block - 3] [i].block_type = BLOCK_EDGE_RIGHT;
//  if (i>0&&i<(w.h_block-1)) w.block [w.h_block - 2] [i].block_type = BLOCK_SOLID;
//  if (i>1&&i<(w.h_block-2)) w.block [w.h_block - 3] [i].block_type = BLOCK_SOLID;
/*  w.block [0] [i].block_type = BLOCK_FAR_LEFT;
  if (i>0&&i<(w.h_block-1)) w.block [1] [i].block_type = BLOCK_FAR_LEFT;
  if (i>1&&i<(w.h_block-2)) w.block [2] [i].block_type = BLOCK_LEFT;
  w.block [w.h_block - 1] [i].block_type = BLOCK_FAR_RIGHT;
  if (i>0&&i<(w.h_block-1)) w.block [w.h_block - 2] [i].block_type = BLOCK_FAR_RIGHT;
  if (i>1&&i<(w.h_block-2)) w.block [w.h_block - 3] [i].block_type = BLOCK_RIGHT;*/
 }

 w.block [2] [2].block_type = BLOCK_EDGE_UP_LEFT;
 w.block [2] [w.h_block - 3].block_type = BLOCK_EDGE_DOWN_LEFT;
 w.block [w.w_block - 3] [2].block_type = BLOCK_EDGE_UP_RIGHT;
 w.block [w.w_block - 3] [w.h_block - 3].block_type = BLOCK_EDGE_DOWN_RIGHT;

// allocate memory for the proc array.
// use calloc because this isn't remotely time-critical.
 w.proc = calloc(w.max_procs, sizeof(struct procstruct));
 if (w.proc == NULL)
 {
      fprintf(stdout, "g_world.c: Out of memory in allocating w.proc");
      error_call();
 }
// when adding any dynamic memory allocation to this function, remember to free the memory in deallocate_world() below

// allocate memory for each proc's bcode:
 for (i = 0; i < w.max_procs; i ++)
 {
  w.proc[i].bcode.op = calloc(PROC_BCODE_SIZE, sizeof(s16b));
  if (w.proc[i].bcode.op == NULL)
  {
      fprintf(stdout, "g_world.c: Out of memory in allocating w.proc[%i].bcode.op", i);
      error_call();
  }
  w.proc[i].bcode.bcode_size = PROC_BCODE_SIZE;
 }
// when adding any dynamic memory allocation to this function, remember to free the memory in deallocate_world() below

 w.max_groups = (w.max_procs / 2) + 1;
// to accommodate the possibility that the world will consist entirely of groups of 2 procs, there must be
//  at least half as many available groups as there are procs.
// I don't think there's any need for more than this.
 w.group = calloc(w.max_groups, sizeof(struct groupstruct));
 if (w.group == NULL)
 {
      fprintf(stdout, "g_world.c: Out of memory in allocating w.group");
      error_call();
 }
// when adding any dynamic memory allocation to this function, remember to free the memory in deallocate_world() below

// now allocate the packet array:
 w.packet = calloc(w.max_packets, sizeof(struct packetstruct));
 if (w.packet == NULL)
 {
      fprintf(stdout, "g_world.c: Out of memory in allocating w.packet");
      error_call();
 }
// when adding any dynamic memory allocation to this function, remember to free the memory in deallocate_world() below

// now allocate the cloud array:
 w.cloud = calloc(w.max_clouds, sizeof(struct cloudstruct));
 if (w.cloud == NULL)
 {
      fprintf(stdout, "g_world.c: Out of memory in allocating w.cloud");
      error_call();
 }
// when adding any dynamic memory allocation to this function, remember to free the memory in deallocate_world() below

// these values are kept in the worldstruct so they can be saved into saved games and used to re-setup a loaded world:
  w.player_clients = w_init.player_clients;
  w.allow_observer = w_init.allow_observer;
  w.player_operator = w_init.player_operator;

// setup players
 for (i = 0; i < w.players; i ++)
 {
  w.player[i].active = 1;
  w.player[i].proc_index_start = i * w_init.procs_per_team;
  w.player[i].proc_index_end = (i+1) * w_init.procs_per_team;
  strcpy(w.player[i].name, w_init.player_name [i]); // if loading from disk, the name copied here will be updated later

  w.player[i].processes = 0;

  w.player[i].output_console = 0;
//  w.player[i].default_print_colour = PRINT_COL_LGREY;
  w.player[i].error_console = 0;

  w.may_change_client_template [i] = w_init.may_change_client_template [i];
  w.may_change_proc_templates [i] = w_init.may_change_proc_templates [i];

  if (w.player_clients == 1)
  {
   w.player[i].program_type_allowed = PLAYER_PROGRAM_ALLOWED_DELEGATE;
   if (w.player_operator == i)
    w.player[i].program_type_allowed  = PLAYER_PROGRAM_ALLOWED_OPERATOR;
  }
   else
    w.player[i].program_type_allowed = PLAYER_PROGRAM_ALLOWED_NONE;

  w.player[i].allocate_effect_type = ALLOCATE_EFFECT_DISRUPT;

 }

 w.system_output_console = 0;
 w.system_error_console = 0;
 w.observer_output_console = 0;
 w.observer_error_console = 0;

 for (i = 0; i < SYSTEM_COMS; i ++)
 {
  w.system_com [i] = 0;
 }

 initialise_world();

 w.allocated = 1;

}




// This function clears an existing world so it can be re-used (or initialises one just created in new_world_from_world_init()).
// The world's basic parameters (e.g. size, number of procs etc) should have been established, so new_world_from_world_init() must have been called first.
void initialise_world(void)
{

 int p, g, i, j;
 struct procstruct* pr;
 struct blockstruct* bl;
 struct groupstruct* gr;

 w.world_time = 0;
 w.total_time = 0;

 for (p = 0; p < w.max_procs; p ++)
 {
  pr = &w.proc [p];
  pr->exists = 0;
  pr->index = p;
 }

 for (g = 0; g < w.max_groups; g ++)
 {
  gr = &w.group [g];
  gr->exists = 0;
 }

 w.current_check = 0;

 for (i = 0; i < w.w_block; i ++)
 {
  for (j = 0; j < w.h_block; j ++)
  {
    bl = &w.block [i] [j];
    bl->tag = 0;
    bl->blocklist_down = NULL;

  }
 }


 w.blocktag = 1; // should probably be 1 more than the value that all of the blocks in the world are set to.
// w.players_active = 0;
// w.client_programs = 0; // is 1 if there are client programs, 0 otherwise

 init_packets();
 init_clouds();

// init markers:
 for (i = 0; i < MARKERS; i ++)
 {
  w.marker[i].active = 0;
 }

 init_block_nodes();

// now we zero out all programs except the system program
 init_program(&w.observer_program, PROGRAM_TYPE_OBSERVER, -1);
 for (i = 0; i < PLAYERS; i ++)
 {
  init_program(&w.player[i].client_program, PROGRAM_TYPE_DELEGATE, i);
 }
 w.actual_operator_player = -1; // will be updated to player index by copy_template_to_program() if an operator program is loaded in

 set_default_modifiable_colours(); // this sets to default the colours that can be modified by user programs

}





// This function frees the memory used by the worldstruct
// It doesn't otherwise reinitialise the worldstruct
// Call it only when certain that memory has been allocated for the worldstruct (as otherwise it will try to free already free memory)
void deallocate_world(void)
{

#ifdef SANITY_CHECK
 if (w.allocated == 0)
 {
  fprintf(stdout, "\nError: g_world.c:deallocate_world() called when w.allocated already 0.");
  error_call();
 }
#endif

 int i;

// first, free each column of blocks:
 for (i = 0; i < w.w_block; i ++)
 {
  free(w.block [i]);
 }

// now free the row of pointers to the columns:
 free(w.block);

// free procs' bcode:
 for (i = 0; i < w.max_procs; i ++)
 {
  free(w.proc[i].bcode.op);
 }

// free the rest of the arrays:
 free(w.proc);
 free(w.group);
 free(w.packet);
 free(w.cloud);

 w.allocated = 0;

}





//void colour_fraction(int base_cols [3], int out_cols [3], int fraction, int subtract);





void run_world(void)
{

// currently does nothing

}

// manages marker fade, proc selections and similar
// currently this is run during soft pause but not during hard pause. Is that correct? (not sure)
// Most other marker-related code is in g_method_clob.c (as markers are dealt with by observer methods)
void run_markers(void)
{

 int i;

 for (i = 0; i < MARKERS; i ++)
 {
  if (w.marker[i].active != 0)
  {
   switch(w.marker[i].phase)
   {
   	case MARKER_PHASE_CONSTANT:
   		if (w.marker[i].timeout > 0) // -1 means indefinite
					{
      w.marker[i].timeout --;
      if (w.marker[i].timeout <= 0)
	 		  {
       w.marker[i].active = 0;
       continue;
			   }
					}
			  break;
    case MARKER_PHASE_APPEAR:
     w.marker[i].phase_counter --;
     if (w.marker[i].phase_counter <= 0)
					{
      w.marker[i].phase = MARKER_PHASE_EXISTS;
					}
   		if (w.marker[i].timeout > 0) // -1 means indefinite
					{
      w.marker[i].timeout --;
      if (w.marker[i].timeout <= 0)
	 		  {
       w.marker[i].phase = MARKER_PHASE_FADE;
       w.marker[i].phase_counter = MARKER_PHASE_TIME;
	 		  }
					}
	 		 break;
			 case MARKER_PHASE_EXISTS:
   		if (w.marker[i].timeout > 0) // -1 means indefinite
					{
      w.marker[i].timeout --;
      if (w.marker[i].timeout <= 0)
	 		  {
       w.marker[i].phase = MARKER_PHASE_FADE;
       w.marker[i].phase_counter = MARKER_PHASE_TIME;
	 		  }
					}
					w.marker[i].angle += 32;
     break;
    case MARKER_PHASE_FADE: // doesn't process timeout
     w.marker[i].phase_counter --;
     if (w.marker[i].phase_counter <= 0)
					{
      w.marker[i].active = 0;
					}
					break;
				}

   if (w.marker[i].bind_to_proc != -1)
   {
    if (w.proc[w.marker[i].bind_to_proc].exists <= 0)
     w.marker[i].active = 0; // should really make it fade out if appropriate
      else
      {
       w.marker[i].x = w.proc[w.marker[i].bind_to_proc].x;
       w.marker[i].y = w.proc[w.marker[i].bind_to_proc].y;
      }
   }
   if (w.marker[i].bind_to_proc2 != -1)
   {
    if (w.proc[w.marker[i].bind_to_proc2].exists <= 0)
     w.marker[i].active = 0; // should really make it fade out if appropriate
      else
      {
       w.marker[i].x2 = w.proc[w.marker[i].bind_to_proc2].x;
       w.marker[i].y2 = w.proc[w.marker[i].bind_to_proc2].y;
      }
   }
  }
 }

// now deal with proc selection
//  (this used to be in the proc processing functions, but had to be moved here because those functions aren't called during a soft pause while this function is)
 struct procstruct* pr;

 for (i = 0; i < w.max_procs; i ++)
 {

  if (w.proc[i].exists <= 0)
   continue;

  pr = &w.proc[i];

  if (pr->selected != 0)
  {
// pr->selected must be non-zero for two passes through this function, in order for it to be picked up by the display functions
    if (pr->selected == 2)
     pr->selected = 1;
      else
       pr->selected = 0;
  }

  if (pr->map_selected != 0)
  {
// pr->map_selected must be non-zero for two passes through this function, in order for it to be picked up by the display functions
    if (pr->map_selected == 2)
     pr->map_selected = 1;
      else
       pr->map_selected = 0;
  }

 }

}




/*

The time has come to plan how the basic game start and management things will work.

Basically there needs to be some way to get the user into a world and allow procs to be placed there.

First, there will be a start menu:
 - Missions
 - Setup game
 - options
 - help
 - exit

Missions:
 - this will be a series of levels where the user is given a goal and enemies, and also a default setup (procs, client etc) that they can change if they want.
 - will be direct and indirect control options
 - player can save and load games

Setup game:
 - will allow player to set any aspects of game, including multiplayer.
 - once setup complete, player can:
  - play (start game)
  - generate gamefile (to allow other players to play)
   - probably give the option of providing default client programs (that can be overriden if needed) (or also could forbid client programs altogether)
    - possible client programs will not ever be forbidden, but may be prevented from using any necessary methods (making them useless for anything other than observation)
 - can also load a gamefile

Maybe a game starts by selecting a client program and a series of templates
 - Client program (and *possibly* also procs) can use the new proc method to create a proc directly from the template (and perhaps also sub-procs from template? not sure if needed)
  - actually it might make more sense to only allow procs to do this. Client program can direct them to do so but cannot otherwise place procs in world.
  - at start of a game, one of the templates (say, the first one?) is placed at a set position in the world.
   - maybe more than one, depending on the game mode.
 - Depending on game mode, it may be possible to change templates at any time.
  - need a new menu window to allow this to happen.
  - also need to make sure editor can coexist with this menu and with other game start menus.

So will need to do the following:
* new m_menu file to contain basic startup menu
* new s_* files to contain game start menus:
 - missions (s_mission)
 - setup game (s_setup)
 - game loop will be called from these files, as appropriate.
 - these menus should be able to co-exist with the source editor and the template menu (possibly just one of each at any time?)
* new t_template file to deal with templates
 - will have a list:
  - client program
  - templates
  - not game program (this will be chosen by setup)
 - can load from disk or import from editor
 - will automatically compile any sources when moved to template (i.e. templates will only be in binary format)
  - this means will probably need a "refresh" option to recompile or reacquire files open in editor.


Possibly each proc (in binary or source form) will contain a description that's displayed in the template selection screen?





*/


#define NODE_SPACING ((BLOCK_SIZE_PIXELS + 3) / 3)

void init_block_nodes(void)
{

 int i, j, k, l, nd;
 struct blockstruct* bl;
// int spacing = (BLOCK_SIZE_PIXELS + 3) / 3;
 int space_size = (NODE_SPACING / 6) * 5;
// int space_min = spacing / 12;

 for (i = 0; i < w.w_block; i ++)
 {
  for (j = 0; j < w.h_block; j ++)
  {
    bl = &w.block [i] [j];
    nd = 0;

    for (k = 0; k < 3; k ++)
    {
     for (l = 0; l < 3; l ++)
     {
//      bl->node_size [nd] = (space_size - grand(24)) / 2;
//      bl->node_x [nd] = (k * spacing) + spacing / 2;
//      bl->node_y [nd] = (l * spacing) + spacing / 2;

      bl->node_size [nd] = ((space_size) / 2) - 2 + 2;
      if (grand(4) == 0)
       bl->node_size [nd] += 2;
      if (grand(7) == 0)
       bl->node_size [nd] += 2;
      if (grand(7) == 0)
       bl->node_size [nd] -= 2;
      if (grand(7) == 0)
       bl->node_size [nd] -= 4;
      bl->node_x [nd] = (k * NODE_SPACING) + NODE_SPACING / 2;
      bl->node_y [nd] = (l * NODE_SPACING) + NODE_SPACING / 2;

      if ((j + l) % 2 == 0)
       bl->node_x [nd] += NODE_SPACING / 2;

      bl->node_x_base [nd] = bl->node_x [nd];
      bl->node_y_base [nd] = bl->node_y [nd];

      bl->node_team_col [nd] = 0;
      bl->node_col_saturation [nd] = 0;

      bl->node_disrupt_timestamp [nd] = 0;//UINT_MAX - 300;

      nd ++;
     }
    }


/*
    bl->node_x [0] = (BLOCK_SIZE_PIXELS / 6) + grand((BLOCK_SIZE_PIXELS * 4) / 6);
    bl->node_y [0] = (BLOCK_SIZE_PIXELS / 6) + grand((BLOCK_SIZE_PIXELS * 4) / 6);

    bl->node_x [1] = grand(BLOCK_SIZE_PIXELS - bl->node_x [0] - (BLOCK_SIZE_PIXELS / 8)) + (BLOCK_SIZE_PIXELS / 8) + bl->node_x [0];
    bl->node_y [1] = bl->node_y [0];

    bl->node_x [2] = bl->node_x [0];
    bl->node_y [2] = grand(BLOCK_SIZE_PIXELS - bl->node_y [0] - (BLOCK_SIZE_PIXELS / 8)) + (BLOCK_SIZE_PIXELS / 8) + bl->node_y [0];*/

  }
 }


}


void disrupt_block_nodes(al_fixed x, al_fixed y, int player_cause, int size)
{

 unsigned int bx = fixed_to_block(x);
 unsigned int by = fixed_to_block(y);

 int i;

 if (bx >= w.w_block
  || by >= w.h_block) // don't need to check negatives as bx/by are unsigned
  return; // just fail if out of bounds

 struct blockstruct* bl = &w.block [bx] [by];

 for (i = 0; i < 9; i ++)
 {
  change_block_node(bl, i, bl->node_x [i] + grand(size) - grand(size), bl->node_y [i] + grand(size) - grand(size), bl->node_size [i] + grand(size) - grand(size));
  change_block_node_colour(bl, i, player_cause);
  bl->node_disrupt_timestamp [i] = w.world_time + NODE_DISRUPT_TIME_CHANGE;
 }

}


void disrupt_single_block_node(al_fixed x, al_fixed y, int player_cause, int size)
{

// we need a slight fine-tuning offset here:
 x -= al_itofix(BLOCK_SIZE_PIXELS / 6);
 y -= al_itofix(BLOCK_SIZE_PIXELS / 6);

 unsigned int bx = fixed_to_block(x);
 unsigned int by = fixed_to_block(y);

//  fprintf(stdout, "\ndisrupt_single_block_node() called: block (%i, %i) (%i, %i) at (%f, %f)", bx, by, bx * BLOCK_SIZE_PIXELS, by * BLOCK_SIZE_PIXELS, al_fixtof(x), al_fixtof(y));


 unsigned int i; // if changed to signed, need to make sure bounds checks below check for negative

 if (bx >= w.w_block
  || by >= w.h_block) // don't check for negatives as bx/by are unsigned
  return; // just fail if out of bounds

 struct blockstruct* bl = &w.block [bx] [by];

 int int_x = al_fixtoi(x);
 int int_y = al_fixtoi(y);

//  fprintf(stdout, "\nxy (%f, %f) int_xy (%i, %i)", al_fixtof(x), al_fixtof(y), int_x, int_y);

 int_x -= BLOCK_SIZE_PIXELS * bx;
 int_x *= 3;
 int_x /= BLOCK_SIZE_PIXELS;

 int_y -= BLOCK_SIZE_PIXELS * by;
 int_y *= 3;
 int_y /= BLOCK_SIZE_PIXELS;

 i = (int_x * 3) + int_y;
//  fprintf(stdout, "\nnode (%i) from source (%i, %i)", i, int_x, int_y);

#ifdef SANITY_CHECK
 if (i >= 9) // i is unsigned so don't need to check for negative
 {
  fprintf(stdout, "\ng_world.c: disrupt_single_block_node(): invalid node (%i) from source (%f, %f)", i, al_fixtof(x), al_fixtof(y));
  error_call();
 }
#endif

// for (i = 0; i < 9; i ++)
 {
  change_block_node(bl, i, bl->node_x [i] + grand(size) - grand(size), bl->node_y [i] + grand(size) - grand(size), bl->node_size [i] + grand(size) - grand(size));
  change_block_node_colour(bl, i, player_cause);
//  bl->node_x [i] += grand(5) - grand(5);
//  bl->node_y [i] += grand(5) - grand(5);
/*
  bl->node_size [i] += grand(size) - grand(size);
  if (bl->node_size [i] < 5)
   bl->node_size [i] = 5;
  if (bl->node_size [i] > 40)
   bl->node_size [i] = 40;*/
  bl->node_disrupt_timestamp [i] = w.world_time + NODE_DISRUPT_TIME_CHANGE;


 }

}

#define NODE_LOCATION_MIN -5
#define NODE_LOCATION_MAX (BLOCK_SIZE_PIXELS+5)
#define NODE_SIZE_MIN 5
#define NODE_SIZE_MAX 40

// Moves the block node to new_x,new_y
void change_block_node(struct blockstruct* bl, int i, int new_x, int new_y, int new_size)
{

 bl->node_x [i] = new_x;
 if (bl->node_x [i] < NODE_LOCATION_MIN)
  bl->node_x [i] = NODE_LOCATION_MIN;
   else
   {
    if (bl->node_x [i] > NODE_LOCATION_MAX)
     bl->node_x [i] = NODE_LOCATION_MAX;
   }

 bl->node_y [i] = new_y;
 if (bl->node_y [i] < NODE_LOCATION_MIN)
  bl->node_y [i] = NODE_LOCATION_MIN;
   else
   {
    if (bl->node_y [i] > NODE_LOCATION_MAX)
     bl->node_y [i] = NODE_LOCATION_MAX;
   }

 bl->node_size [i] = new_size;
 if (bl->node_size [i] < NODE_SIZE_MIN)
  bl->node_size [i] = NODE_SIZE_MIN;
   else
   {
    if (bl->node_size [i] > NODE_SIZE_MAX)
     bl->node_size [i] = NODE_SIZE_MAX;
   }

}


// Moves the block node back towards its original position
void align_block_node(struct blockstruct* bl, int i)
{

 if (bl->node_x [i] < bl->node_x_base [i])
	{
		bl->node_x [i] ++;
	}
	 else
		{
   if (bl->node_x [i] > bl->node_x_base [i])
		  bl->node_x [i] --;
		}

 if (bl->node_y [i] < bl->node_y_base [i])
	{
		bl->node_y [i] ++;
	}
	 else
		{
   if (bl->node_y [i] > bl->node_y_base [i])
		  bl->node_y [i] --;
		}

	bl->node_size [i] += grand(5) - grand(5);

	if (bl->node_size [i] < 10)
		bl->node_size [i] = 10;

	if (bl->node_size [i] > 22)
		bl->node_size [i] = 22;

}


void change_block_node_colour(struct blockstruct* bl, int i, int player_cause)
{

  if (bl->node_team_col [i] == player_cause)
  {
   if (bl->node_col_saturation [i] < BACK_COL_SATURATIONS - 1)
    bl->node_col_saturation [i] ++;
//   fprintf(stdout, "\nBlock node %i colour matches (%i) saturation now %i", i, player_cause, bl->node_col_saturation [i]);
  }
   else
   {
//    fprintf(stdout, "\nBlock node %i colour (%i) changed to (%i)", i, bl->node_team_col [i], player_cause);
    bl->node_team_col [i] = player_cause;
    bl->node_col_saturation [i] = 0;
   }


}



