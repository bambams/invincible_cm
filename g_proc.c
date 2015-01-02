#include <allegro5/allegro.h>

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#include "m_config.h"
#include "m_globvars.h"

#include "g_misc.h"
#include "g_header.h"
#include "g_proc.h"

#include "g_group.h"
#include "g_motion.h"
#include "g_method.h"
#include "g_cloud.h"
#include "g_method_misc.h"
#include "g_method_clob.h"
#include "g_shape.h"
#include "g_world.h"
#include "i_error.h"
#include "x_sound.h"

#include "m_maths.h"

extern struct shapestruct shape_dat [SHAPES] [SHAPES_SIZES]; // in g_shape.c
extern const struct mtypestruct mtype [MTYPES]; // in g_method.c
extern const struct method_costsstruct method_cost; // in g_method.c

static int get_alloc_efficiency_from_fixed_dist(al_fixed closest_dist);
static void recalculate_proc_allocate_efficiency(struct procstruct* pr, int ignore_proc);
void add_new_process_with_allocate(struct procstruct* pr);

//int new_proc(int p);
//void initialise_proc(int p);

//void proc_explodes(struct procstruct* pr);
//int destroy_proc(struct procstruct* pr);


// Finds an empty entry in w's proc array at an appropriate place for this team
// Doesn't allocate or otherwise alter the proc, so it can
// Returns index of empty proc on success, -1 on failure
int find_empty_proc(int player_index)
{

 int p;

 for (p = w.player[player_index].proc_index_start; p < w.player[player_index].proc_index_end; p ++)
 {
  if (w.proc[p].exists == 0) // note: is 1 if exists, or -1 if undergoing deallocation
   return p;
 }

 return -1; // team is full

}

/*

Plan for procs:


*/

// creates new proc p in world w. p must have been found by find_empty_proc()
// doesn't give it x/y values, or put it into blocklists
// assumes pr->player_index has been set
// assumes shape and size have been set (from the shape and size values in the proc's bcode struct)
// assumes initialise_proc has been called to set various things to zero
// returns 1 on success, 0 on failure
int new_proc(int p)
{

 struct procstruct* pr = &w.proc[p];

// First we initialise the physics of the proc object
 pr->exists = 1;
 pr->lifetime = 0;

 pr->index = p; // may not be strictly necessary as this is set in initialise_world and should not have been changed since

 pr->x = al_itofix(0);
 pr->y = al_itofix(0);
 pr->x_block = 0;
 pr->y_block = 0;
 pr->x_speed = al_itofix(0);
 pr->y_speed = al_itofix(0);
 pr->angle = al_itofix(0);
 pr->test_angle = al_itofix(0);
 pr->spin = al_itofix(0);
 pr->spin_change = al_itofix(0);
// pr->instant_spin = 0;

 pr->shape_str = &shape_dat [pr->shape] [pr->size];
 pr->max_length = shape_dat [pr->shape] [pr->size].max_length;

 pr->drag = DRAG_BASE_FIXED; // current 1013
// pr->mass = 10 + 2 * pr->size; // think about this
 pr->moment = pr->mass;// * 24; // TO DO: calculate this properly (maybe could use vertices of pr's shape somehow?)

 pr->execution_count = EXECUTION_COUNT + 2; // remember to leave enough time for the creation graphic to be displayed.
// execution_count is likely to be overwritten after this


/*

The following code is no longer used.


// Now, execution_count.
// If we just set this to EXECUTION_COUNT, the player's irpt use will tend to spike as all procs created by a particular builder will execute in the same tick
// So we look for the timing that's currently used least and set it to that.
// The time method (not implemented yet) will let the player change execution_count, but that doesn't matter.

 int i;
 struct playerstruct* pl = &w.player[pr->player_index];
 int ex_count [EXECUTION_COUNT];

 pl->processes ++;

 for (i = 0; i < EXECUTION_COUNT; i ++)
 {
  ex_count [i] = 0;
 }

 pr->execution_count = -1; // temp value so it won't be counted yet

 for (i = pl->proc_index_start; i < pl->proc_index_end; i ++)
 {
  if (w.proc[i].exists > 0)
  {
   if (w.proc[i].execution_count >= 0
    && w.proc[i].execution_count < EXECUTION_COUNT) // procs with execution_count >= EXECUTION_COUNT because of a wait (using std method) won't be counted
     ex_count [w.proc[i].execution_count] ++;
  }
 }

 int lowest_ex_count = -1;
 int lowest_ex_count_number = 100000;

 for (i = 0; i < EXECUTION_COUNT; i ++)
 {
  if (ex_count [i] < lowest_ex_count_number)
  {
   lowest_ex_count = i;
   lowest_ex_count_number = ex_count [i];
//   fprintf(stdout, "\nFound ex_count %i", i);
  }
 }

 pr->execution_count = lowest_ex_count;
// fprintf(stdout, "\nset to %i", pr->execution_count);
*/


 return 1;

}

/*
This function sets various values related to a proc to their init values. Should be called anytime a new proc is being produced.
Must be called before derive_proc_properties_from_bcode() and new_proc()

Doesn't assume that the proc will go on to actually be created (and doesn't set anything that would be a problem if it didn't)
*/
void initialise_proc(int p)
{

 int i, j;

 struct procstruct* pr = &w.proc[p];

// First we initialise the physics of the proc object
// pr->view_focus = 0;

 pr->hit = 0;

 pr->deallocating = 0;

 pr->hit_edge_this_cycle = 0;
 pr->stuck = 0;

// these pointers are used in collision detection, which builds a linked list of procs in each occupied block:
 pr->blocklist_up = NULL;
 pr->blocklist_down = NULL;

 pr->group = -1;
 pr->number_of_group_connections = 0;

 for (i = 0; i < GROUP_CONNECTIONS; i ++)
 {
  pr->group_connection [i] = NULL;
  pr->connection_vertex [i] = 0;
 }

 for (i = 0; i < METHODS + 1; i ++)
 {
  pr->method [i].type = MTYPE_END;
  for (j = 0; j < METHOD_DATA_VALUES; j ++)
  {
   pr->method[i].data[j] = 0;
  }
  for (j = 0; j < METHOD_EXTENSIONS; j ++)
  {
   pr->method[i].extension[j] = 0;
  }
 }

 for (i = 0; i < PROC_BCODE_SIZE; i ++)
 {
  pr->bcode.op [i] = 0;
 }

 for (i = 0; i < METHOD_BANK; i ++)
 {
  pr->regs.mbank[i] = 0;
 }

 for (i = 0; i < REGISTERS; i ++)
 {
  pr->regs.reg [i] = 0;
 }

 for (i = 0; i < METHOD_VERTICES; i ++)
 {
   pr->vertex_method [i] = -1;
 }

// pr->central_method = -1;

 for (i = 0; i < COMMANDS; i ++)
 {
   pr->command [i] = 0;
 }

 pr->index_for_client = p; // not currently used

 pr->selected = 0;
// pr->select_time = 0;
 pr->deselected = 0;
 pr->map_selected = 0;


}



/*

Tries to create a proc in world w at x/y/angle.
p must have been found by find_empty_proc()
proc's shape and size must have been set
Use for single procs that don't become group members on creation.
Performs collision checks.
Can fail if no space at intended position, or if new_proc fails (e.g. proc array is full)

Returns 1 on success, 0 on failure

*/
int create_single_proc(int p, al_fixed x, al_fixed y, al_fixed angle)
{

 struct procstruct* pr = &w.proc[p];

#ifdef SANITY_CHECK
 if (x < BLOCK_SIZE_FIXED + al_itofix(10)
  || y < BLOCK_SIZE_FIXED + al_itofix(10)
  || x > w.w_fixed - (BLOCK_SIZE_FIXED + al_itofix(10))
  || y > w.w_fixed - (BLOCK_SIZE_FIXED + al_itofix(10)))
 {
   fprintf(stdout, "\nError: g_proc.c: create_single_proc(): attempted to create proc outside of world at (%i, %i) (world size %i, %i)", al_fixtoi(x), al_fixtoi(y), w.w_pixels, w.h_pixels);
   error_call();
 }
#endif


// collision test
 if (check_notional_block_collision_multi(pr->shape, pr->size, x, y, angle)
  || check_notional_solid_block_collision(pr->shape, pr->size, x, y, angle))
 {
  return 0;
 }

 if (!new_proc(p))
  return 0;

 pr->x = x;
 pr->y = y;
 pr->angle = angle;
 pr->float_angle = fixed_to_radians(pr->angle);
 pr->provisional_x = x;
 pr->provisional_y = y;
 pr->provisional_angle = angle;
 pr->old_x = x;
 pr->old_y = y;
 pr->old_angle = angle;
 pr->execution_count = EXECUTION_COUNT + (w.world_time % (EXECUTION_COUNT - 2)); // this should result in a reasonable spread of execution counts

 add_proc_to_blocklist(pr);

// remember to also call finish_adding_new_proc() after calling this function

 return 1;

}

#define GROUP_SEPARATION2 5000
#define GROUP_SEPARATION2_FIXED al_itofix(5)

/*

Returns 1 on success, 0 on failure
*/
int create_new_group_member(struct procstruct* pr, int new_p, int source_vertex, int pr2_vertex, al_fixed pivot_angle)
{

 al_fixed new_x, new_y, new_angle;
 int shape = w.proc[new_p].shape;
 int size = w.proc[new_p].size;

 new_angle = pivot_angle; //pr->angle + pivot_angle;

// fprintf(stdout, "prxy (%i, %i)\n", pr->x / GRAIN_MULTIPLY, pr->y / GRAIN_MULTIPLY);

// First we find out where source_vertex is:
 new_x = pr->x + fixed_xpart(pr->angle + pr->shape_str->vertex_angle [source_vertex], pr->shape_str->vertex_dist [source_vertex]);

// fprintf(stdout, "pr->angle %i pivot_angle %i new_angle %i xpart(%i, %i)\n", pr->angle, pivot_angle, new_angle, pr->angle + pr->shape_str->vertex_angle [source_vertex], pr->shape_str->vertex_dist_grain [source_vertex]);
// fprintf(stdout, "new_x1 %i\n", new_x / GRAIN_MULTIPLY);

// now we need to find out where the centre point of the new proc needs to go. Need to work backwards from pr2_vertex
 new_x -= fixed_xpart(new_angle + shape_dat [shape] [size].vertex_angle [pr2_vertex], shape_dat [shape] [size].vertex_dist [pr2_vertex] + GROUP_SEPARATION2_FIXED);

// fprintf(stdout, "new_x2 %i\n", new_x / GRAIN_MULTIPLY);

// now do the same for y
 new_y = pr->y + fixed_ypart(pr->angle + pr->shape_str->vertex_angle [source_vertex], pr->shape_str->vertex_dist [source_vertex]);
// fprintf(stdout, "new_y1 %i\n", new_y / GRAIN_MULTIPLY);
 new_y -= fixed_ypart(new_angle + shape_dat [shape] [size].vertex_angle [pr2_vertex], shape_dat [shape] [size].vertex_dist [pr2_vertex] + GROUP_SEPARATION2_FIXED);
// fprintf(stdout, "new_y2 %i\n", new_y / GRAIN_MULTIPLY);

// now we find out whether the new proc, if placed, would collide with anything:
/*
 if (pr->index != 100)
 {
  new_proc_fail_cloud(new_x, new_y, new_angle, shape, size);
  return 0;
 }*/

 if (check_notional_block_collision_multi(shape, size, new_x, new_y, new_angle)
  || check_notional_solid_block_collision(shape, size, new_x, new_y, new_angle))
 {
  start_error(PROGRAM_TYPE_PROCESS, pr->index, pr->player_index);
  error_string("\nProcess ");
  error_number(pr->index);
  error_string(" failed to create new process (collision).");
  new_proc_fail_cloud(new_x, new_y, new_angle, shape, size, pr->player_index);
  return 0;
 }

 if (!new_proc(new_p))
 {
  start_error(PROGRAM_TYPE_PROCESS, pr->index, pr->player_index);
  error_string("\nProcess ");
  error_number(pr->index);
  error_string(" failed to create new process (too many processes).");
  return 0;
 }

 struct procstruct* new_pr = &w.proc[new_p];

 new_pr->x = new_x;
 new_pr->y = new_y;
 new_pr->angle = new_angle; // I think new_pr->angle may be changed by call to connect_procs below
 new_pr->float_angle = fixed_to_radians(new_pr->angle);
 new_pr->provisional_x = new_x;
 new_pr->provisional_y = new_y;
 new_pr->provisional_angle = new_angle;
 new_pr->old_x = new_x;
 new_pr->old_y = new_y;
 new_pr->old_angle = new_angle;
 new_pr->execution_count = EXECUTION_COUNT + 2; // new proc will always execute 1 or 2 ticks after parent

 int connect_success = connect_procs(pr, new_pr, source_vertex, pr2_vertex);

 if (connect_success == -1)
 {
  new_pr->exists = 0; // don't worry about deallocation as the proc never really existed
  start_error(PROGRAM_TYPE_PROCESS, pr->index, pr->player_index);
  error_string("\nProcess ");
  error_number(pr->index);
  error_string(" failed to create new process (could not connect?).");
  return 0;
 }


 new_pr->old_angle = new_pr->angle;
 new_pr->float_angle = fixed_to_radians(new_pr->angle);

/*

This doesn't work

#ifdef SANITY_CHECK
 if (connect_success == 0)
 {
  fprintf(stdout, "\ng_proc.c:create_new_group_member(): Error: call to connect_procs failed\n");
  error_call();
 }
#endif
// TO DO: think about how to deal with failure of connect_procs. May not be necessary if we can guarantee at this point that the call will be successful.

*/

 add_proc_to_blocklist(new_pr);

// fprintf(stdout, "\nnew proc index %i at %i, %i", new_pr->index, new_pr->x, new_pr->y);

 return 1;

}


/*
Call this function after:
 - a new proc's basic_prop_properties struct has been filled in
 - the new proc's bcode has been written.
It copies basic properties from the bpp struct and finishes setting up method information.
Assumes that BCODE_MAX is large enough to contain an entire interface definition (if it isn't there is a larger problem).
Does not check for sufficient irpt or data. Assumes that this is done before calling, if necessary.

Can be called both when inserting a new proc into the world, and when running a new proc creation method.

parent_proc_index is pr->index of parent. Is -1 if no parent (e.g. if system program creating it)

returns 1 on success, 0 on failure
 *** Note that when any new failure mode is added to this function, it may also need to be added to the new proc test functions in g_method_ex.c (which check whether a proc can be created from a bcodestruct)
*/
int derive_proc_properties_from_bcode(struct procstruct* pr, struct basic_proc_propertiesstruct* bprop, int parent_proc_index)
{

 int bcode_pos;

 bcode_pos = BCODE_HEADER_ALL_TYPE; // start of interface definition in bcode, ignoring the jump instruction
// note that bcode_pos isn't bounds-checked in this function; we assume that bcode is at least large enough for the interface definition (otherwise nothing would work)

 bcode_pos++; // skip type
 pr->shape = bprop->shape;
 bcode_pos++; // skip shape
 pr->size= bprop->size;
 bcode_pos++; // skip size
 pr->base_vertex= bprop->base_vertex;
 bcode_pos++; // skip base_vertex

 pr->shape_str = &shape_dat [pr->shape] [pr->size];

 pr->hp_max = pr->shape_str->base_hp_max; // can be modified below by a redundancy method
 pr->single_proc_irpt_max = pr->shape_str->base_irpt_max;
 pr->single_proc_data_max = pr->shape_str->base_data_max;

// now initialise basic values from the bprop struct
 pr->mass = bprop->mass;
 pr->method_mass = bprop->method_mass;
 pr->base_cost = bprop->base_cost;

 pr->instructions_each_execution = INSTRUCTIONS_PROC; // can be increased by method
 pr->instructions_left_after_last_exec = pr->instructions_each_execution;
// pr->irpt_use = 0;
// pr->irpt_base = pr->single_proc_irpt_max; // not sure about this, but only used to display irpt use immediately after creation so should be okay.
// pr->special_method_penalty = 1;

 int i, j;
 int type;

 pr->mobile = 1; // this will be set to zero below if the proc has a method that requires immobility
// pr->irpt_gen_number = 0; // number that will be added to player.gen_number (and tested against w.gen_limit)
 pr->redundancy = 0;
 pr->listen_method = -1; // this will be set to index of proc's listen method (if any)
 pr->allocate_method = -1; // this will be set to index of proc's allocate method
 pr->virtual_method = -1; // index of proc's virtual method
 pr->give_irpt_link_method = -1;
 pr->give_data_link_method = -1;
 pr->irpt_gain = 0;
 pr->irpt_gain_max = 0;

 for (i = 0; i < METHOD_VERTICES; i ++)
 {
  pr->vertex_method[i] = bprop->vertex_method[i];
 }

 pr->method[METHODS].type = MTYPE_END; // method array has METHODS+1 elements

 for (i = 0; i < METHODS; i ++)
 {

  type = bprop->method_type[i];
  pr->method[i].type = type;

// now go through the interface method data.
// this does different things depending on the type of method it is
//  bcode_pos++; // don't need this - INTERFACE_METHOD_DATA includes the type, and M_IDATA_ACCEL_RATE etc. start at 1 to allow for it

// first, check whether the method is external and if so set up the external vertex:
// this code assumes that the first M_IDATA entry for an external method is always the vertex.
// also assumes that the method's vertex is correct (this will have been checked when bprop was set up; if incorrect method type is MTYPE_ERROR_VERTEX)
   if (mtype[type].external != MEXT_INTERNAL)
   {
    pr->method[i].ex_vertex = pr->bcode.op [bcode_pos + INTERFACE_METHOD_DATA_VERTEX];
    if (mtype[type].external == MEXT_EXTERNAL_ANGLED)
    {
     int exv = pr->method[i].ex_vertex;
// make sure the angle is pointing out away from the body of the proc
//  - if it isn't, correct it (this doesn't cause method failure)
     pr->method[i].ex_angle = short_angle_to_fixed(pr->bcode.op [bcode_pos + INTERFACE_METHOD_DATA_ANGLE]); // vertex_angle is added later

     if ((pr->shape_str->external_angle_wrapped [exv] == 0 // means that minimum permissible angle < max permissible angle, so method's angle must be between them
        && (pr->method[i].ex_angle < pr->shape_str->external_angle [EXANGLE_LOW] [exv]
         || pr->method[i].ex_angle > pr->shape_str->external_angle [EXANGLE_HIGH] [exv]))
// or external angle values are wrapped, in which case minimum permissible angle > max permissible angle (so method's angle must be outside of them)
     || (pr->shape_str->external_angle_wrapped [exv] == 1
        && pr->method[i].ex_angle > pr->shape_str->external_angle [EXANGLE_LOW] [exv]
        && pr->method[i].ex_angle < pr->shape_str->external_angle [EXANGLE_HIGH] [exv]))
     {
// method is pointing inward, which isn't allowed. Instead, set it to the closest valid angle (i.e. the closest of the min or max values):
      if (angle_difference(pr->method[i].ex_angle, pr->shape_str->external_angle [EXANGLE_LOW] [exv])
        < angle_difference(pr->method[i].ex_angle, pr->shape_str->external_angle [EXANGLE_HIGH] [exv]))
      {
       start_error(PROGRAM_TYPE_PROCESS, parent_proc_index, pr->player_index);
       error_string("\nNew process ");
       error_number(pr->index);
       error_string(" method ");
       error_number(i);
       error_string(" angle invalid; corrected from ");
       error_number(fixed_angle_to_int(pr->method[i].ex_angle));
       error_string(" to ");
       error_number(fixed_angle_to_int(pr->shape_str->external_angle [EXANGLE_LOW] [exv]));
       error_string(".");
//       fprintf(stdout, "\npr %i method %i angle corrected from %f to %f (max is %f; wrapped %i)", pr->index, i, al_fixtof(pr->method[i].ex_angle), al_fixtof(pr->shape_str->external_angle [EXANGLE_LOW] [exv]), al_fixtof(pr->shape_str->external_angle [EXANGLE_HIGH] [exv]), pr->shape_str->external_angle_wrapped [exv]);
       pr->method[i].ex_angle = pr->shape_str->external_angle [EXANGLE_LOW] [exv];
//       pr->method[i].ex_angle = (pr->shape_str->external_angle [EXANGLE_LOW] [exv] + pr->shape_str->vertex_angle [exv]) & AFX_MASK;;
      }
       else
       {
        start_error(PROGRAM_TYPE_PROCESS, parent_proc_index, pr->player_index);
        error_string("\nNew process ");
        error_number(pr->index);
        error_string(" method ");
        error_number(i);
        error_string(" angle invalid; corrected from ");
        error_number(fixed_angle_to_int(pr->method[i].ex_angle));
        error_string(" to ");
        error_number(fixed_angle_to_int(pr->shape_str->external_angle [EXANGLE_HIGH] [exv]));
        error_string(".");
//        fprintf(stdout, "\npr %i method %i angle corrected from %f to %f (min is %f; wrapped %i)", pr->index, i, al_fixtof(pr->method[i].ex_angle), al_fixtof(pr->shape_str->external_angle [EXANGLE_HIGH] [exv]), al_fixtof(pr->shape_str->external_angle [EXANGLE_LOW] [exv]), pr->shape_str->external_angle_wrapped [exv]);
//        pr->method[i].ex_angle = (pr->shape_str->external_angle [EXANGLE_HIGH] [exv] + pr->shape_str->vertex_angle [exv]) & AFX_MASK;;
        pr->method[i].ex_angle = pr->shape_str->external_angle [EXANGLE_HIGH] [exv];
       }
     }

// now add in vertex_angle to get actual angle
     pr->method[i].ex_angle = (pr->method[i].ex_angle + pr->shape_str->vertex_angle [exv]) & AFX_MASK;

    }
   }

   j = 0;
//   int ext = 0;
//   int total_ext = 0;

   for (j = 0; j < METHOD_EXTENSIONS; j ++)
   {
    pr->method[i].extension[j] = bprop->method_extension[i][j];
   }

// now fill in the remaining data (data values were initiliased to zero by initialise_proc):
  switch(type)
  {
   case MTYPE_PR_MOVE:
// data values are verified in verify_method_data() in f_load.c (any changes here may need to be reflected there)
    pr->method[i].data [MDATA_PR_MOVE_SETTING_MAX] = 5 + (pr->method[i].extension [0] * 3); // power
    pr->method[i].data [MDATA_PR_MOVE_IRPT_COST] = 2; // irpt used per point of accel per tick
    break;
   case MTYPE_PR_PACKET:
// data values are verified in verify_method_data() in f_load.c (any changes here may need to be reflected there)
    pr->method[i].data [MDATA_PR_PACKET_IRPT_COST] = (2 + pr->method[i].extension [0] + pr->method[i].extension [1] + pr->method[i].extension [2]) * 32; // irpt used to fire
    pr->method[i].data [MDATA_PR_PACKET_RECYCLE] = 0; // reload time
    break;

   case MTYPE_PR_DPACKET:
// data values are verified in verify_method_data() in f_load.c (any changes here may need to be reflected there)
    pr->method[i].data [MDATA_PR_DPACKET_IRPT_COST] = (2 + pr->method[i].extension [0] + pr->method[i].extension [1] + pr->method[i].extension [2]) * 36; // irpt used to fire
    pr->method[i].data [MDATA_PR_DPACKET_RECYCLE] = 0; // reload time
// Now need to work out min and max angle values.
// When the method's angle data value (MDATA_PR_DPACKET_ANGLE) is 0, it is pointing directly away from the proc's centre (i.e. its angle is the vertex's angle).
// Negative values are anticlockwise, positive values are clockwise.
    pr->method[i].data [MDATA_PR_DPACKET_ANGLE] = 0; // angle data value
//    pr->method[i].data [MDATA_PR_DPACKET_ANGLE_MIN] = fixed_angle_to_int(pr->shape_str->external_angle [EXANGLE_PREVIOUS] [pr->method[i].ex_vertex]) - ANGLE_1; // Note no & ANGLE_MASK
//    pr->method[i].data [MDATA_PR_DPACKET_ANGLE_MAX] = fixed_angle_to_int(pr->shape_str->external_angle [EXANGLE_NEXT] [pr->method[i].ex_vertex]);
/*
// The following calculation is also used in c_data.c
    if (pr->shape_str->external_angle_wrapped [pr->method[i].ex_vertex] == 0)
    {
      pr->method[i].data [MDATA_PR_DPACKET_ANGLE_MIN] = fixed_angle_to_int(pr->shape_str->external_angle [EXANGLE_LOW] [pr->method[i].ex_vertex]) - ANGLE_1;
      pr->method[i].data [MDATA_PR_DPACKET_ANGLE_MAX] = fixed_angle_to_int(pr->shape_str->external_angle [EXANGLE_HIGH] [pr->method[i].ex_vertex]);
    }
     else
     {
//      pr->method[i].data [MDATA_PR_DPACKET_ANGLE_MIN] = fixed_angle_to_int(pr->shape_str->external_angle [EXANGLE_HIGH] [pr->method[i].ex_vertex] - pr->shape_str->vertex_angle [pr->method[i].ex_vertex]); // Note no & ANGLE_MASK
//      pr->method[i].data [MDATA_PR_DPACKET_ANGLE_MAX] = fixed_angle_to_int(pr->shape_str->external_angle [EXANGLE_LOW] [pr->method[i].ex_vertex] - pr->shape_str->vertex_angle [pr->method[i].ex_vertex]);
//      pr->method[i].data [MDATA_PR_DPACKET_ANGLE_MIN] = angle_difference_signed(fixed_angle_to_int(pr->shape_str->vertex_angle [pr->method[i].ex_vertex]), fixed_angle_to_int(pr->shape_str->external_angle [EXANGLE_LOW] [pr->method[i].ex_vertex]));
//      pr->method[i].data [MDATA_PR_DPACKET_ANGLE_MAX] = angle_difference_signed(fixed_angle_to_int(pr->shape_str->vertex_angle [pr->method[i].ex_vertex]), fixed_angle_to_int(pr->shape_str->external_angle [EXANGLE_HIGH] [pr->method[i].ex_vertex]));

//      fixed_angle_to_int(pr->shape_str->external_angle [EXANGLE_HIGH] [pr->method[i].ex_vertex] - pr->shape_str->vertex_angle [pr->method[i].ex_vertex]); // Note no & ANGLE_MASK
//      pr->method[i].data [MDATA_PR_DPACKET_ANGLE_MAX] = fixed_angle_to_int(pr->shape_str->external_angle [EXANGLE_LOW] [pr->method[i].ex_vertex] - pr->shape_str->vertex_angle [pr->method[i].ex_vertex]);
      pr->method[i].data [MDATA_PR_DPACKET_ANGLE_MIN] = fixed_angle_to_int(pr->shape_str->external_angle [EXANGLE_HIGH] [pr->method[i].ex_vertex]) - ANGLE_1;
      pr->method[i].data [MDATA_PR_DPACKET_ANGLE_MAX] = fixed_angle_to_int(pr->shape_str->external_angle [EXANGLE_LOW] [pr->method[i].ex_vertex]);


     }
     */

/*

     if ((pr->shape_str->external_angle_wrapped [exv] == 0 // means that minimum permissible angle < max permissible angle, so method's angle must be between them
        && (pr->method[i].ex_angle < pr->shape_str->external_angle [EXANGLE_LOW] [exv]
         || pr->method[i].ex_angle > pr->shape_str->external_angle [EXANGLE_HIGH] [exv]))
// or external angle values are wrapped, in which case minimum permissible angle > max permissible angle (so method's angle must be outside of them)
     || (pr->shape_str->external_angle_wrapped [exv] == 1
        && pr->method[i].ex_angle > pr->shape_str->external_angle [EXANGLE_LOW] [exv]
        && pr->method[i].ex_angle < pr->shape_str->external_angle [EXANGLE_HIGH] [exv]))

       pr->method[m].data [MDATA_PR_DPACKET_ANGLE] -= DPACKET_TURN_SPEED;
// Find out if the turret has turned past its target:
       if (pr->method[m].data [MDATA_PR_DPACKET_ANGLE] < pr->method[m].data [MDATA_PR_DPACKET_ANGLE_MIN])
        pr->method[m].data [MDATA_PR_DPACKET_ANGLE] = pr->method[m].data [MDATA_PR_DPACKET_ANGLE_MIN];
       pr->method[m].ex_angle = (short_angle_to_fixed(pr->method[m].data [MDATA_PR_DPACKET_ANGLE]) + pr->shape_str->vertex_angle [pr->method[m].ex_vertex]) & AFX_MASK;


*/
    break;
   case MTYPE_PR_IRPT:
// data values are verified in verify_method_data() in f_load.c (any changes here may need to be reflected there)
//    pr->irpt_gen_number += 2 + pr->method[i].extension [0];
    pr->method[i].data [MDATA_PR_IRPT_MAX] = (8 + (pr->method[i].extension [0] * 4)) * 6;
    pr->method[i].data [MDATA_PR_IRPT_GEN] = pr->method[i].data [MDATA_PR_IRPT_MAX];
    pr->irpt_gain = pr->method[i].data [MDATA_PR_IRPT_MAX];
    pr->irpt_gain_max = pr->method[i].data [MDATA_PR_IRPT_MAX]; // this is used only to calculate a group's max irpt gain
    break;
   case MTYPE_PR_ALLOCATE:
    pr->mobile = 0;
    pr->allocate_method = i;
    pr->method[i].data [MDATA_PR_ALLOCATE_EFFICIENCY] = 100;
    break;
   case MTYPE_PR_STATIC:
    pr->mobile = 0;
    break;
   case MTYPE_PR_LINK:
// data values are verified in verify_method_data() in f_load.c (any changes here may need to be reflected there)
    pr->method[i].data [MDATA_PR_LINK_CONNECTION] = -1; // nothing connected (may be changed later in creation)
    pr->method[i].data [MDATA_PR_LINK_INDEX] = -1; // nothing connected (may be changed later in creation)
    pr->method[i].data [MDATA_PR_LINK_RECEIVED] = 0; // message received from other proc (-1 indicates no connection)
    pr->method[i].data [MDATA_PR_LINK_OTHER_METHOD] = -1; // index of other proc's LINK method connected to this one
    break;
   case MTYPE_PR_DESIGNATE:
// data values are verified in verify_method_data() in f_load.c (any changes here may need to be reflected there)
    pr->method[i].data [MDATA_PR_DESIGNATE_INDEX] = -1; // nothing designated
    pr->method[i].data [MDATA_PR_DESIGNATE_RANGE] = (9 + (pr->method[i].extension [0] * 3)) * 100;
    break;
   case MTYPE_PR_SCAN:
// data values are verified in verify_method_data() in f_load.c (any changes here may need to be reflected there)
    pr->method[i].data [MDATA_PR_SCAN_RANGE] = (4 + (pr->method[i].extension [0] * 2)) * 100;
    break;
   case MTYPE_PR_RESTORE:
// data values are verified in verify_method_data() in f_load.c (any changes here may need to be reflected there)
    pr->method[i].data [MDATA_PR_RESTORE_RATE] = 1 + pr->method[i].extension [0];
    break;
   case MTYPE_PR_REDUNDANCY:
    pr->hp_max = (pr->hp_max * (140 + (pr->method[i].extension [0] * 20))) / 100;
    pr->redundancy = 2 + pr->method[i].extension [0];
    break;
   case MTYPE_PR_BROADCAST:
    pr->method[i].data [MDATA_PR_BROADCAST_RANGE_MAX] = (6 + (pr->method[i].extension [0] * 3)) * 100;
    break;
   case MTYPE_PR_LISTEN:
    if (pr->listen_method == -1)
     pr->listen_method = i;
    break;
   case MTYPE_PR_YIELD:
    pr->method[i].data [MDATA_PR_YIELD_ACTIVATED] = 0; // not yet activated
    pr->method[i].data [MDATA_PR_YIELD_RATE_MAX_IRPT] = 255 + (pr->method[i].extension [1] * 127);
    pr->method[i].data [MDATA_PR_YIELD_RATE_MAX_DATA] = 4 + (pr->method[i].extension [1] * 4);
    pr->method[i].data [MDATA_PR_YIELD_RANGE] = 500 + (pr->method[i].extension [0] * 300);
    break;
   case MTYPE_PR_STORAGE:
    pr->single_proc_data_max *= 140 + (pr->method[i].extension [0] * 20);
    pr->single_proc_data_max /= 100;
    break;
   case MTYPE_PR_VIRTUAL:
    pr->virtual_method = i;
    pr->method[i].data [MDATA_PR_VIRTUAL_MAX] = 96 + (pr->method[i].extension [0] * 32);//(pr->shape_str->base_hp_max * (2 + pr->method[i].extension [0])) / 2;
    pr->regs.mbank [i * METHOD_SIZE + MBANK_PR_VIRTUAL_MAX] = pr->method[i].data [MDATA_PR_VIRTUAL_MAX];
    break;

// Errors:
   case MTYPE_ERROR_INVALID: // data is attempted type
   case MTYPE_ERROR_VERTEX: // data is attempted vertex
   case MTYPE_ERROR_SUB: // data is attempted type
   case MTYPE_ERROR_MASS: // data is attempted mass
// data values are verified in verify_method_data() in f_load.c (any changes here may need to be reflected there)
    pr->method[i].data [0] = bprop->method_data[i];
    break;

// Remember: may need to add verification code to f_load.c to verify method data values!
  }

// the following code must be run once per loop
  bcode_pos += INTERFACE_METHOD_DATA;

 }

// check irpt gen number against player total and w.gen_limit:
/* if (w.player[pr->player_index].gen_number + pr->irpt_gen_number > w.gen_limit)
 {
  start_error(PROGRAM_TYPE_PROCESS, parent_proc_index, pr->player_index);
  error_string("\nNew process creation failed (irpt gen too high).");
  return 0;
 }*/

 pr->scan_bitfield = 0;
 pr->scan_bitfield |= (1 << pr->player_index);
 pr->scan_bitfield |= (1 << (pr->shape + MSCAN_BF1_SHAPE0));
 pr->scan_bitfield |= (1 << (pr->size + MSCAN_BF1_SIZE0));

 pr->hp = pr->hp_max;

 pr->single_proc_irpt = 0;//pr->irpt_max; // not sure about this - probably shouldn't start with max irpt
 pr->irpt = &pr->single_proc_irpt;
 pr->irpt_max = &pr->single_proc_irpt_max;

 pr->single_proc_data = 0;
 pr->data = &pr->single_proc_data;
 pr->data_max = &pr->single_proc_data_max;

 return 1;

}


// This function adds the finishing touches to a new proc
// It should be called only when it is too late for proc creation to fail
void finish_adding_new_proc(int p)
{

// w.player[w.proc[p].player_index].gen_number += w.proc[p].irpt_gen_number;

 w.player[w.proc[p].player_index].processes ++;

 if (w.proc[p].allocate_method != -1)
  add_new_process_with_allocate(&w.proc[p]);

}


// doesn't assume method_type is a valid MTYPE
// if loading, accepts any method that a proc could have (including sub and error methods)
// returns 1 valid, 0 invalid
int check_valid_proc_method(int method_type, int loading)
{

 if (loading
  && (method_type >= 0 && method_type < MTYPES)
  && (mtype[method_type].mclass == MCLASS_ERROR
   || mtype[method_type].mclass == MCLASS_ALL
   || mtype[method_type].mclass == MCLASS_END))
   return 1;

 if (method_type < 0
  || method_type >= MTYPE_END)
   return 0;

 if (mtype[method_type].mclass != MCLASS_PR
  && mtype[method_type].mclass != MCLASS_ALL)
   return 0;

 return 1;

}

// pass this function a basic_proc_properties struct and a pointer to the start of a proc interface definition in bcode
// it will fill in the basic_proc_properties struct with values suitable for calculating cost or creating a proc
// all values in basic_proc_properties will be bounds-checked and suitable for use
//  - doesn't confirm that proc can be created (e.g. doesn't check that gen_number won't exceed maximum)
//  - doesn't set up method values like external angle and similar things that can't cause a method not to be created
// assumes that there is space in the bcode array for a full interface (must check this before calling!)
// returns a PROC_PROPERTIES_xxx value (0 is success)
int get_basic_proc_properties_from_bcode(struct basic_proc_propertiesstruct* prop, s16b* bcode_op, int limited_methods)
{

 int bp = 2; // to skip the jump instruction

// bcode_pos = BCODE_HEADER_ALL_TYPE; // start of interface definition in bcode, ignoring the jump instruction
// note that bcode_pos isn't bounds-checked in this function; we assume that bcode is at least large enough for the interface definition (otherwise nothing would work)

 if (bcode_op [bp] != PROGRAM_TYPE_PROCESS)
	{
		prop->type = bcode_op [bp];
  fprintf(stdout, "\ninvalid type %i at %i", bcode_op [bp], bp);
  return PROC_PROPERTIES_FAILURE_PROGRAM_TYPE;
	}
 bp ++;

 prop->shape = bcode_op [bp];
 if (prop->shape < 0 || prop->shape >= SHAPES)
  return PROC_PROPERTIES_FAILURE_SHAPE;
 bp ++;

 prop->size = bcode_op [bp];
 if (prop->size < 0 || prop->size >= SHAPES_SIZES)
  return PROC_PROPERTIES_FAILURE_SIZE;
 bp ++;

 prop->base_vertex = bcode_op [bp];
// if (prop->base_vertex < 0 || prop->size >= SHAPES_SIZES)
//  return PROC_PROPERTIES_FAILURE_SIZE;
// invalid base_vertex values will get picked up in new process creation (if relevant)
 bp ++;

 struct shapestruct* shape_str = &shape_dat [prop->shape] [prop->size];

// now initialise basic values from the shape_type struct (see g_shape.c)
// some of these values can be changed later in generation
 prop->mass = shape_str->shape_mass;
 prop->base_cost = 16 + prop->size * 8;
 prop->method_mass = 0;
 prop->method_error = 0;

 int mass_max = shape_str->mass_max; // this value is just used here
 int method_mass = 0;

 int i, j;
 int type;
 int large_method_counter = 0;
 int ext;
 int total_ext;
 int proc_has_redundancy_method = 0; // can only have one redundancy method
 int proc_has_storage_method = 0; // only 1 storage method
 int proc_has_allocate_method = 0;
 int proc_has_virtual_method = 0;

// initialise some things:
 for (i = 0; i < METHOD_VERTICES; i ++)
 {
  prop->vertex_method [i] = -1;
 }
 for (i = 0; i < METHODS; i ++)
 {
  prop->method_type[i] = MTYPE_NONE;
  prop->method_data[i] = 0;
  prop->method_vertex[i] = -1;
  prop->method_extensions[i] = 0;
  for (j = 0; j < METHOD_EXTENSIONS; j++)
  {
   prop->method_extension[i][j] = 0;
  }
 }
 for (i = 0; i < METHOD_VERTICES; i ++)
 {
  prop->vertex_method[i] = -1;
 }


 for (i = 0; i < METHODS; i ++)
 {
  large_method_counter --;
// check if we're making space for a method that takes up multiple method slots:
  if (large_method_counter > 0)
  {
   prop->method_type[i] = MTYPE_SUB;
   bp += INTERFACE_METHOD_DATA;
   continue;
  }

  type = bcode_op [bp];

// first make sure the method type is valid and there's enough space for it in the mbank:
  if (type < 0 // should accept MTYPE_NONE (for methods that are the later parts of a large method)
   || type >= MTYPE_END
   || (mtype[type].mclass != MCLASS_PR
    && mtype[type].mclass != MCLASS_ALL)
   || ((i + mtype[type].mbank_size) * METHOD_SIZE > METHOD_BANK)) // mbank_size needs to be multiplied by METHOD_SIZE because it's in units of methods (i.e. 4 mbank entries), and not mbank entries.
  {
   prop->method_type[i] = MTYPE_ERROR_INVALID;
   prop->method_data[i] = type;
   bp += INTERFACE_METHOD_DATA;
   large_method_counter = mtype[MTYPE_ERROR_INVALID].mbank_size; // unlike other errors, just use one method slot (since we don't know what this method is supposed to be)
   prop->method_error = 1;
   continue;
  }

  prop->method_type[i] = type;

// now go through the interface method data.
// this does different things depending on the type of method it is
//  bp++; // don't need this - INTERFACE_METHOD_DATA includes the type, and M_IDATA_ACCEL_RATE etc. start at 1 to allow for it

// first, check whether the method is external and if so set up the external vertex:
// this code assumes that the first M_IDATA entry for an external method is always the vertex.
   if (mtype[type].external != MEXT_INTERNAL)
   {
    prop->method_vertex[i] = bcode_op [bp + INTERFACE_METHOD_DATA_VERTEX];
    int exv = prop->method_vertex[i];
    if (exv < 0
     || exv >= shape_str->vertices
     || prop->vertex_method [exv] != -1)
    {
//     fprintf(stdout, "\nvertex failure: method %i exv %i shape_str->vertices %i prop->vertex_method[exv] %i", i, exv, shape_str->vertices, prop->vertex_method[exv]);
     bp += INTERFACE_METHOD_DATA;
     large_method_counter = mtype[type].mbank_size; // make as much space as would have been made if the error didn't happen (to avoid messing up order of remaining methods)
     prop->method_type[i] = MTYPE_ERROR_VERTEX;
     prop->method_data[i] = exv;
     prop->method_error = 1;
     continue;
    }

    prop->vertex_method [exv] = i;
   }

   if (limited_methods
    && (type == MTYPE_PR_NEW
     || type == MTYPE_PR_IRPT))
   {
     bp += INTERFACE_METHOD_DATA;
     large_method_counter = mtype[type].mbank_size; // make as much space as would have been made if the error didn't happen (to avoid messing up order of remaining methods)
     prop->method_type[i] = MTYPE_ERROR_SUB;
     prop->method_data[i] = type;
     prop->method_error = 1;
     continue;
   }


   j = 0;
   ext = 0;
   total_ext = 0;

// now go through and add up the extensions/cost/mass of each method
   while (j < mtype[type].extension_types)
   {
    ext = bcode_op [bp + INTERFACE_METHOD_DATA_EX1 + j];
    if (ext <= 0)
    {
     j ++;
     continue;
    }
    if (ext + total_ext > mtype[type].max_extensions)
    {
     ext = mtype[type].max_extensions - total_ext; // number of extensions is capped at mtype[type].max_extensions
    }
    total_ext += ext;
    prop->method_extension[i][j] = ext;
    j ++;
   };

   prop->method_extensions[i] = total_ext;

   method_mass = method_cost.base_cost[mtype[type].cost_category] + (method_cost.extension_cost[mtype[type].cost_category] * total_ext);

   switch(type)
   {
// All duplicate method tests must go here, so that the proc's cost can be calculated correctly (with the error method costs rather than the duplicated method costs)
    case MTYPE_PR_REDUNDANCY:
     if (proc_has_redundancy_method != 0)
     {
// can only have one redundancy method
      bp += INTERFACE_METHOD_DATA;
      large_method_counter = mtype[type].mbank_size; // make as much space as would have been made if the error didn't happen (to avoid messing up order of remaining methods)
      prop->method_type[i] = MTYPE_ERROR_DUPLICATE;
      prop->method_error = 1;
      continue;
     }
     method_mass = (shape_str->shape_mass * (40 + (total_ext * 20))) / 100;
     if (prop->mass + method_mass > mass_max)
     {
// need to test for this now so that proc_has_redundancy_method isn't set
      bp += INTERFACE_METHOD_DATA;
      large_method_counter = mtype[type].mbank_size; // make as much space as would have been made if the error didn't happen (to avoid messing up order of remaining methods)
      prop->method_type[i] = MTYPE_ERROR_MASS; // must come after large_method_counter is set
      prop->method_data[i] = method_mass;
      prop->method_error = 1;
      continue;
     }
     proc_has_redundancy_method = 1;
     break;
    // first, screen out any duplicates of single methods:
   case MTYPE_PR_ALLOCATE:
    if (proc_has_allocate_method == 1)
    {
// can only have one allocate method
     bp += INTERFACE_METHOD_DATA;
     large_method_counter = mtype[type].mbank_size; // make as much space as would have been made if the error didn't happen (to avoid messing up order of remaining methods)
     prop->method_type[i] = MTYPE_ERROR_DUPLICATE;
     prop->method_error = 1;
     continue;
    }
    proc_has_allocate_method = 1;
    break;
   case MTYPE_PR_STORAGE:
    if (proc_has_storage_method == 1)
    {
// can only have one storage method
       bp += INTERFACE_METHOD_DATA;
       large_method_counter = mtype[type].mbank_size; // make as much space as would have been made if the error didn't happen (to avoid messing up order of remaining methods)
       prop->method_type[i] = MTYPE_ERROR_DUPLICATE;
       prop->method_error = 1;
       continue;
    }
    proc_has_storage_method = 1;
    break;
   case MTYPE_PR_VIRTUAL:
    if (proc_has_virtual_method == 1)
    {
// can only have one storage method
       bp += INTERFACE_METHOD_DATA;
       large_method_counter = mtype[type].mbank_size; // make as much space as would have been made if the error didn't happen (to avoid messing up order of remaining methods)
       prop->method_type[i] = MTYPE_ERROR_DUPLICATE;
       prop->method_error = 1;
       continue;
    }
    proc_has_virtual_method = 1;
    break;

   }

   if (prop->mass + method_mass > mass_max)
   {
    bp += INTERFACE_METHOD_DATA;
    large_method_counter = mtype[type].mbank_size; // make as much space as would have been made if the error didn't happen (to avoid messing up order of remaining methods)
    prop->method_type[i] = MTYPE_ERROR_MASS; // must come after large_method_counter is set
    prop->method_data[i] = method_mass;
    prop->method_error = 1;
    continue;
   }

   prop->mass += method_mass;
   prop->method_mass += method_mass;
   prop->base_cost += method_cost.upkeep_cost[mtype[type].cost_category] + (method_cost.extension_upkeep_cost[mtype[type].cost_category] * total_ext);

// the following code must be run once per loop
   bp += INTERFACE_METHOD_DATA;
   large_method_counter = mtype[type].mbank_size;
 }

// now add MTYPE_END at the end of the list of methods:
 i = METHODS; // this is METHODS, and not METHODS-1, because the method array has a spare element at the end for the final MTYPE_END if all other methods are full.
 prop->method_type[METHODS] = MTYPE_END;
 while(TRUE)
 {
  if (i == 0)
  {
   prop->method_type[i] = MTYPE_END;
   break;
  }
  if ((prop->method_type[i] == MTYPE_NONE
				|| prop->method_type[i] == MTYPE_END)
   && (prop->method_type[i - 1] != MTYPE_NONE
				&& prop->method_type[i - 1] != MTYPE_END))
  {
   prop->method_type[i] = MTYPE_END;
   break;
  }
  i --;
 };

// prop->mass += shape_str->vertices * 2;
// prop->mass += prop->size * (4 + prop->size);;

 prop->data_cost = prop->mass;
// prop->data_cost += shape_str->vertices * 2;
// prop->data_cost += prop->size * (4 + prop->size);;
 prop->irpt_cost = prop->data_cost;

// fprintf(stdout, "\nData cost: %i", prop->data_cost);

 return PROC_PROPERTIES_SUCCESS;

} // end of get_basic_prop_properties_from_bcode








/*

Allocator efficiency:
This is determined by the closest other proc with an allocator method.
Distance is the greater of x and y distance (so it's a square, effectively)

Each proc with an allocate method stores the closest other proc with an allocate method
and also an efficiency rating

When a new allocator proc is added:
 - we check all other allocator procs to find:
  - which one is closest to the new proc
   - that one determines the proc's efficiency
  - whether for each other proc, the new proc is the closest
   - if so, the new proc determines the other proc's efficiency
 - remember that they could be like this:  A     B C (B is A's closest, but C is B's closest)

When an allocator proc is destroyed:
 - we check all other allocator procs to find any for which the destroyed proc is the closest
  - for those ones, we recalculate closest proc

Data values:
MDATA_PR_ALLOCATE_STATUS, // has method been used yet this cycle?
MDATA_PR_ALLOCATE_COUNTER, // Efficiency is added to this
MDATA_PR_ALLOCATE_EFFICIENCY,
MDATA_PR_ALLOCATE_CLOSEST_ALLOC, // index of closest other proc with allocate method
MDATA_PR_ALLOCATE_ALLOC_DIST, // distance of closest other proc with allocate method

Also need, in proc:
 int allocate_method; // -1 if no allocate method

*/

static int get_alloc_efficiency_from_fixed_dist(al_fixed closest_dist)
{

 int effic = al_fixtoi(closest_dist) / ALLOCATE_INTERFERENCE_RANGE_DIVIDE;

 if (effic > 100)
  return 100;

 return effic;

}


int get_point_alloc_efficiency(al_fixed x, al_fixed y)
{

 int i;
 al_fixed closest_dist = al_itofix(ALLOCATE_INTERFERENCE_BOX_SIZE);
 al_fixed x_dist, y_dist;

 for (i = 0; i < w.max_procs - 1; i ++)
 {
  if (w.proc[i].exists <= 0
   || w.proc[i].allocate_method == -1)
   continue;

  x_dist = abs(x - w.proc[i].x);
  y_dist = abs(y - w.proc[i].y);

// the distance value used is the greater of the x and y distances
  if (x_dist > y_dist)
  {
   if (x_dist < closest_dist)
   {
    closest_dist = x_dist;
   }
  }
   else
   {
    if (y_dist < closest_dist)
    {
     closest_dist = y_dist;
    }
   }
 }

 return get_alloc_efficiency_from_fixed_dist(closest_dist);

}




// Call this just after creating a new proc with an allocate method.
// It will set the proc's allocator efficiency,
//  and also set the efficiency for other nearby procs if the new proc is their closest allocator proc
void add_new_process_with_allocate(struct procstruct* pr)
{

 int i;
 al_fixed closest_dist = al_itofix(ALLOCATE_INTERFERENCE_BOX_SIZE);
 int closest_proc = -1;
 al_fixed x_dist, y_dist;

 pr->method [pr->allocate_method].data [MDATA_PR_ALLOCATE_ALLOC_DIST] = ALLOCATE_INTERFERENCE_BOX_SIZE;
 pr->method [pr->allocate_method].data [MDATA_PR_ALLOCATE_CLOSEST_ALLOC] = -1;

 for (i = 0; i < w.max_procs - 1; i ++)
 {
  if (w.proc[i].allocate_method == -1
   || w.proc[i].exists <= 0
   || i == pr->index)
   continue;

  x_dist = abs(pr->x - w.proc[i].x);
  y_dist = abs(pr->y - w.proc[i].y);

// the distance value used is the greater of the x and y distances
  if (x_dist > y_dist)
  {
   if (x_dist < closest_dist)
   {
    closest_dist = x_dist;
    closest_proc = i;
   }
   if (al_fixtoi(x_dist) < w.proc[i].method [w.proc[i].allocate_method].data [MDATA_PR_ALLOCATE_ALLOC_DIST])
   {
    w.proc[i].method [w.proc[i].allocate_method].data [MDATA_PR_ALLOCATE_ALLOC_DIST] = al_fixtoi(x_dist);
    w.proc[i].method [w.proc[i].allocate_method].data [MDATA_PR_ALLOCATE_CLOSEST_ALLOC] = pr->index;
    w.proc[i].method [w.proc[i].allocate_method].data [MDATA_PR_ALLOCATE_EFFICIENCY] = get_alloc_efficiency_from_fixed_dist(x_dist);
   }
  }
   else
   {
    if (y_dist < closest_dist)
    {
     closest_dist = y_dist;
     closest_proc = i;
    }
    if (al_fixtoi(y_dist) < w.proc[i].method [w.proc[i].allocate_method].data [MDATA_PR_ALLOCATE_ALLOC_DIST])
    {
     w.proc[i].method [w.proc[i].allocate_method].data [MDATA_PR_ALLOCATE_ALLOC_DIST] = al_fixtoi(y_dist);
     w.proc[i].method [w.proc[i].allocate_method].data [MDATA_PR_ALLOCATE_CLOSEST_ALLOC] = pr->index;
     w.proc[i].method [w.proc[i].allocate_method].data [MDATA_PR_ALLOCATE_EFFICIENCY] = get_alloc_efficiency_from_fixed_dist(y_dist);
    }
   }
 }

 if (closest_proc != -1)
 {
  pr->method [pr->allocate_method].data [MDATA_PR_ALLOCATE_ALLOC_DIST] = al_fixtoi(closest_dist);
  pr->method [pr->allocate_method].data [MDATA_PR_ALLOCATE_CLOSEST_ALLOC] = closest_proc;
  pr->method [pr->allocate_method].data [MDATA_PR_ALLOCATE_EFFICIENCY] = get_alloc_efficiency_from_fixed_dist(closest_dist);
 }
  else
  {
   pr->method [pr->allocate_method].data [MDATA_PR_ALLOCATE_ALLOC_DIST] = ALLOCATE_INTERFERENCE_BOX_SIZE;
   pr->method [pr->allocate_method].data [MDATA_PR_ALLOCATE_CLOSEST_ALLOC] = -1;
   pr->method [pr->allocate_method].data [MDATA_PR_ALLOCATE_EFFICIENCY] = 100;
  }


}


// Call this just before a proc with an allocate method is destroyed.
// It will reset the efficiency values of all other procs with allocate methods for which pr was the closest allocator proc
void remove_process_with_allocate(struct procstruct* pr)
{

 int i;

 for (i = 0; i < w.max_procs - 1; i ++)
 {
  if (w.proc[i].allocate_method == -1
   || w.proc[i].exists <= 0
   || w.proc[i].method [w.proc[i].allocate_method].data [MDATA_PR_ALLOCATE_CLOSEST_ALLOC] != pr->index
   || i == pr->index) // probably not needed because of the immediately preceding test
   continue;

  recalculate_proc_allocate_efficiency(&w.proc[i], pr->index);

 }

}

// This is like add_new_process_with_allocate(), but only pr's values will change. It won't change any others.
// Use it when removing a proc, to recalculate the efficiencies of any other allocators for which this is the closest.
// ignore_proc should be the proc that's being removed (or can be -1 if no ignore_proc)
static void recalculate_proc_allocate_efficiency(struct procstruct* pr, int ignore_proc)
{

 int i;
 al_fixed closest_dist = al_itofix(ALLOCATE_INTERFERENCE_BOX_SIZE);
 int closest_proc = -1;
 al_fixed x_dist, y_dist;

 pr->method [pr->allocate_method].data [MDATA_PR_ALLOCATE_ALLOC_DIST] = ALLOCATE_INTERFERENCE_BOX_SIZE;
 pr->method [pr->allocate_method].data [MDATA_PR_ALLOCATE_CLOSEST_ALLOC] = -1;

 for (i = 0; i < w.max_procs - 1; i ++)
 {
  if (w.proc[i].allocate_method == -1
   || w.proc[i].exists <= 0
   || i == pr->index
   || i == ignore_proc)
   continue;

  x_dist = abs(pr->x - w.proc[i].x);
  y_dist = abs(pr->y - w.proc[i].y);

// the distance value used is the greater of the x and y distances
  if (x_dist > y_dist)
  {
   if (x_dist < closest_dist)
   {
    closest_dist = x_dist;
    closest_proc = i;
   }
  }
   else
   {
    if (y_dist < closest_dist)
    {
     closest_dist = y_dist;
     closest_proc = i;
    }
   }
 }

 if (closest_proc != -1)
 {
  pr->method [pr->allocate_method].data [MDATA_PR_ALLOCATE_ALLOC_DIST] = al_fixtoi(closest_dist);
  pr->method [pr->allocate_method].data [MDATA_PR_ALLOCATE_CLOSEST_ALLOC] = closest_proc;
  pr->method [pr->allocate_method].data [MDATA_PR_ALLOCATE_EFFICIENCY] = get_alloc_efficiency_from_fixed_dist(closest_dist);
 }
  else
  {
   pr->method [pr->allocate_method].data [MDATA_PR_ALLOCATE_ALLOC_DIST] = ALLOCATE_INTERFERENCE_BOX_SIZE;
   pr->method [pr->allocate_method].data [MDATA_PR_ALLOCATE_CLOSEST_ALLOC] = -1;
   pr->method [pr->allocate_method].data [MDATA_PR_ALLOCATE_EFFICIENCY] = 100;
  }

}




// wrapper around hurt_proc for cases where the damage is caused by something that is caught by virtual interface.
// damage sources that are not caught by virtual interface should call hurt_proc directly
void apply_packet_damage_to_proc(struct procstruct* pr, int damage, int cause_team)
{


 if (pr->virtual_method != -1
  && pr->method[pr->virtual_method].data [MDATA_PR_VIRTUAL_STATE] > 0)
 {
  if (damage >= pr->method[pr->virtual_method].data [MDATA_PR_VIRTUAL_STATE])
  {
   damage -= pr->method[pr->virtual_method].data [MDATA_PR_VIRTUAL_STATE];

   virtual_method_break(pr);

  }
   else
   {
    pr->method[pr->virtual_method].data [MDATA_PR_VIRTUAL_STATE] -= damage;
    pr->method[pr->virtual_method].data [MDATA_PR_VIRTUAL_PULSE] += (damage / 2);
    if (pr->method[pr->virtual_method].data [MDATA_PR_VIRTUAL_PULSE] > VIRTUAL_METHOD_PULSE_MAX)
     pr->method[pr->virtual_method].data [MDATA_PR_VIRTUAL_PULSE] = VIRTUAL_METHOD_PULSE_MAX;
    return;
   }
 }

 if (damage <= 0)
  return;

 hurt_proc(pr->index, damage, cause_team);

}





// cause_team is the player index of the player that caused the damage (e.g. owner of packet). Can be proc's own team if self-inflicted.
void hurt_proc(int p, int damage, int cause_team)
{
//return;
 w.proc[p].hp -= damage;
 if (w.proc[p].hp <= 0)
 {
  proc_explodes(&w.proc[p], cause_team);
  destroy_proc(&w.proc[p]);
 }

}

// Sets a proc's virtual method to broken and creates the appropriate cloud.
void virtual_method_break(struct procstruct* pr)
{

	  pr->method[pr->virtual_method].data [MDATA_PR_VIRTUAL_STATE] = (0 - VIRTUAL_METHOD_RECYCLE);
//   pr->special_method_penalty --;
   struct cloudstruct* cl = new_cloud(CLOUD_VIRTUAL_BREAK, pr->x, pr->y);

   if (cl != NULL)
			{
    cl->timeout = 16;
    cl->angle = pr->angle;
    cl->colour = pr->player_index;
    cl->data [0] = pr->shape;
    cl->data [1] = pr->size;
    cl->x_speed = 0;//pr->x_speed;
    cl->y_speed = 0;//pr->y_speed;
			}

}

// This function creates an explosion for proc pr. It doesn't destroy the proc.
// Team is the team that destroyed the proc (or proc's own team if self-destructed or similar)
void proc_explodes(struct procstruct* pr, int destroyer_team)
{

 disrupt_block_nodes(pr->x, pr->y, destroyer_team, 5);
 play_game_sound(SAMPLE_KILL, TONE_2E - (pr->size * 4), 50, pr->x, pr->y);

 struct cloudstruct* cl = new_cloud(CLOUD_PROC_EXPLODE_LARGE, pr->x, pr->y);
 if (cl == NULL)
  return; // too many clouds

 cl->timeout = 22;
 cl->angle = pr->angle;
 cl->colour = destroyer_team;
 cl->data [0] = pr->shape;
 cl->data [1] = pr->size;
 cl->x_speed = 0;
 cl->y_speed = 0;

 int i;
 int angle;
 al_fixed speed;

 for (i = 2; i < CLOUD_DATA; i ++)
 {
  cl->data [i] = grand(ANGLE_1);
 }

 int fragments = 8 + (pr->size * 4);
 int fragment_angle = grand(ANGLE_1);


  for (i = 0; i < fragments; i ++)
  {

   cl = new_cloud(CLOUD_PROC_EXPLODE_SMALL, pr->x, pr->y);
   if (cl == NULL)
    return; // too many clouds

   angle = fragment_angle;
   fragment_angle += grand(ANGLE_4);
   speed = al_itofix(100 + grand(1500));

   cl->timeout = 300; // cloud turns into proc_explode_small2 when it stops moving
   cl->angle = int_angle_to_fixed(angle);
   cl->colour = pr->player_index;

   cl->x_speed = fixed_xpart(cl->angle, speed / 100);
   cl->y_speed = fixed_ypart(cl->angle, speed / 100);
   cl->x += fixed_xpart(cl->angle, speed / 25);
   cl->y += fixed_ypart(cl->angle, speed / 25);

   cl->data [0] = 3 + grand(10 + pr->size * 4); // size 1
   cl->data [1] = 3 + grand(10 + pr->size * 4); // size 2
   cl->data [2] = -ANGLE_32 + (grand(ANGLE_16));
//   cl->data [3] = w.player [pr->player_index].colour;
   cl->data [4] = destroyer_team;

  }

}

/*
 struct cloudstruct* cl = new_cloud(CLOUD_PROC_EXPLODE, pr->x, pr->y);

 if (cl != NULL)
 {
  cl->timeout = 22;
  cl->angle = pr->angle;
  //cl->x_speed = pr->x_speed;
  //cl->y_speed = pr->y_speed;

  int i;

  cl->data [0] = pr->shape;
  cl->data [1] = pr->size;

  for (i = 2; i < 16; i ++)
  {
   cl->data [i] = grand(ANGLE_1); // TO DO: think about whether cloud-related stuff should affect the game's RNG
  }

 }
*/



/*
This function partially removes a proc from the world.
It:
 - sets proc's exists value to -1 (deallocating) (will later be changed to 0 (non-existent))
 - sets proc's deallocating counter to 100 (it's decremented each tick, and when it reaches zero the proc is completely removed)
 - removes proc from any group it's in
 - if proc's bcodestruct has a bnotestruct (which would have been malloc'd), free it (bcodenote not currently implemented)

Does not remove proc from blocklist. Relies on any blocklist check to also check exists value.

Does not subtract 1 from player.processes. This is done when deallocation is complete

*/
int destroy_proc(struct procstruct* pr)
{

// w.player[pr->player_index].gen_number -= pr->irpt_gen_number;

 unbind_process(pr->index); // removes all markers bound to this process

 if (pr->allocate_method != -1)
  remove_process_with_allocate(pr);

// if the proc's bcode has a bcodenotestruct (generated by the compiler to hold debugging information) it will have been malloc'd, so we need to free it.
/* if (pr->bcode.note != NULL)
 {
  free(pr->bcode.note);
  pr->bcode.note = NULL;
 }

not yet implemented
 */

 if (pr->group != -1)
 {
// severs all connections
  extract_destroyed_proc_from_group(pr);
 }

 pr->exists = -1;
 pr->hp = 0;
 pr->deallocating = DEALLOCATE_COUNTER;

 return 1;

}
