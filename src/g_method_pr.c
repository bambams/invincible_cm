#include <allegro5/allegro.h>

#include <stdio.h>
#include <math.h>

#include "m_config.h"


#include "g_header.h"

#include "c_header.h"
#include "e_slider.h"
#include "e_header.h"
#include "e_editor.h"
#include "e_log.h"

#include "g_world.h"
#include "g_misc.h"
#include "g_proc.h"
#include "g_packet.h"
#include "g_motion.h"
#include "g_client.h"
#include "m_globvars.h"
#include "m_maths.h"
#include "g_method_misc.h"
#include "g_shape.h"
#include "i_error.h"
#include "g_cloud.h"
#include "x_sound.h"

// NEW_DISTANCE is some extra distance between parent and child vertices
#define NEW_DISTANCE 4

/*

This file contains code for running complex methods.

Ideally it should contain only functions called from itself or from g_method.c

*/

extern struct viewstruct view; // TO DO: think about putting a pointer to this in the worldstruct instead of externing it

extern struct shapestruct shape_dat [SHAPES] [SHAPES_SIZES]; // set up in g_shape.c

extern const struct mtypestruct mtype [MTYPES]; // defined in g_method.c

//int generate_irpt(struct procstruct* pr, int m, int amount);




/*
This function runs METHOD_NEW, which creates a new proc
It also runs test calls:
 - test to see whether proc creation possible (runs most of the function, except actual proc creation)
 - check data cost
 - check irpt cost

It doesn't assume that the new proc can be created or that the new method's settings are correct (it checks everything that was not checked before this function was called)

pr is the parent
m is the method of the parent that is creating the new proc
source_bcode is the bcode that is to be copied into the new proc's bcode, and from which the new proc's interface is to be derived
 - it can be pr's bcode or a template bcode.

return value is an MRES_NEW_* enum.

If the mode is a cost checking mode, the cost will be in MBANK_PR_NEW_STATUS

*/
int run_pr_new_method(struct procstruct* pr, int m, struct bcodestruct* source_bcode)
{

 int mbase = m * METHOD_SIZE;
 s16b* mb = &pr->regs.mbank [mbase];

 int start_address = mb [MBANK_PR_NEW_START_ADDRESS];
 int end_address = mb [MBANK_PR_NEW_END_ADDRESS];

// check that the specified start and end addresses are correct:

 int retval = check_start_end_addresses(start_address, end_address, source_bcode->bcode_size);

 switch(retval)
 {
  case 0: break; // success
  case 1:
   start_error(PROGRAM_TYPE_PROCESS, pr->index, pr->player_index);
   error_string("\nProcess ");
   error_number(pr->index);
   error_string(" failed to create new process (invalid start address: ");
   error_number(start_address);
   error_string(").");
   return MRES_NEW_FAIL_START_BOUNDS;
  case 2:
   start_error(PROGRAM_TYPE_PROCESS, pr->index, pr->player_index);
   error_string("\nProcess ");
   error_number(pr->index);
   error_string(" failed to create new process (invalid end address: ");
   error_number(end_address);
   error_string(").");
   return MRES_NEW_FAIL_END_BOUNDS;
 }

 int limited = 0;
 if (pr->method[m].type == MTYPE_PR_NEW_SUB)
  limited = 1;

 struct basic_proc_propertiesstruct prop;

 int basic_properties = get_basic_proc_properties_from_bcode(&prop, &source_bcode->op [start_address], limited);

 switch(basic_properties)
 {
  case PROC_PROPERTIES_SUCCESS:
   break;
  case PROC_PROPERTIES_FAILURE_PROGRAM_TYPE:
   start_error(PROGRAM_TYPE_PROCESS, pr->index, pr->player_index);
   error_string("\nProcess ");
   error_number(pr->index);
   error_string(" failed to create new process (invalid program type: ");
   error_number(prop.type);
   error_string(").");
   return MRES_NEW_FAIL_TYPE;
  case PROC_PROPERTIES_FAILURE_SHAPE:
   start_error(PROGRAM_TYPE_PROCESS, pr->index, pr->player_index);
   error_string("\nProcess ");
   error_number(pr->index);
   error_string(" failed to create new process (invalid shape: ");
   error_number(prop.shape);
   error_string(").");
   return MRES_NEW_FAIL_SHAPE;
  case PROC_PROPERTIES_FAILURE_SIZE:
   start_error(PROGRAM_TYPE_PROCESS, pr->index, pr->player_index);
   error_string("\nProcess ");
   error_number(pr->index);
   error_string(" failed to create new process (invalid size: ");
   error_number(prop.size);
   error_string(").");
   return MRES_NEW_FAIL_SIZE;
 }

// we may just be checking the proc's data/irpt costs:
 switch(mb [MBANK_PR_NEW_STATUS])
 {
  case MSTATUS_PR_NEW_BUILD:
  case MSTATUS_PR_NEW_TEST:
  case MSTATUS_PR_NEW_T_BUILD:
  case MSTATUS_PR_NEW_T_TEST:
   break; // carry on

  case MSTATUS_PR_NEW_COST_DATA:
  case MSTATUS_PR_NEW_T_COST_DATA:
   mb [MBANK_PR_NEW_STATUS] = prop.data_cost;
   return MRES_NEW_TEST_SUCCESS;

  case MSTATUS_PR_NEW_COST_IRPT:
  case MSTATUS_PR_NEW_T_COST_IRPT:
   mb [MBANK_PR_NEW_STATUS] = prop.irpt_cost;
   return MRES_NEW_TEST_SUCCESS;

  default:
   return MRES_NEW_FAIL_STATUS;

 }

// now make sure we can make more processes:
 if (w.player[pr->player_index].processes >= w.procs_per_team)
 {
   start_error(PROGRAM_TYPE_PROCESS, pr->index, pr->player_index);
   error_string("\nProcess ");
   error_number(pr->index);
   error_string(" failed to create new process (too many processes).");
   return MRES_NEW_FAIL_TOO_MANY_PROCS;
 }

// check that the parent has enough stored data:

 if (prop.data_cost > *(pr->data))
 {
/*  start_error(PROGRAM_TYPE_PROCESS, pr->index, pr->player_index);
  error_string("\nProcess ");
  error_number(pr->index);
  error_string(" failed to create new process (have ");
  error_number(pr->data);
  error_string(" data, need ");
  error_number(prop.data_cost);
  error_string(").");*/
  return MRES_NEW_FAIL_DATA;
 } // note that this test can also be performed by a call to the new method with MSTATUS_PR_NEW_GET_DATA_COST

 if (prop.irpt_cost > *(pr->irpt))
 {

/*  start_error(PROGRAM_TYPE_PROCESS, pr->index, pr->player_index);
  error_string("\nProcess ");
  error_number(pr->index);
  error_string(" failed to create new process (have ");
  error_number(pr->irpt);
  error_string(" irpt, need ");
  error_number(prop.irpt_cost);
  error_string(").");*/
  return MRES_NEW_FAIL_IRPT;
 }

 int p = find_empty_proc(pr->player_index);
// find_empty proc just confirms that there's an empty procstruct that can be placed; it doesn't do anything to create the proc, so we can call it when we're just testing and will not create the proc found
// the value of p found here will be used later to create the proc (unless this is a test)
 if (p == -1)
 {
  start_error(PROGRAM_TYPE_PROCESS, pr->index, pr->player_index);
  error_string("\nProcess ");
  error_number(pr->index);
  error_string(" failed to create new process (too many processes).");
  return MRES_NEW_FAIL_TOO_MANY_PROCS;
 }

// now check vertices and shape things:

 int parent_vertex = mb [MBANK_PR_NEW_VERTEX1];
 int child_vertex = mb [MBANK_PR_NEW_VERTEX2];
 if (child_vertex == -1)
  child_vertex = prop.base_vertex;

 struct shapestruct* parent_shape = pr->shape_str;
 if (parent_vertex < 0 || parent_vertex >= parent_shape->vertices)
 {
  start_error(PROGRAM_TYPE_PROCESS, pr->index, pr->player_index);
  error_string("\nProcess ");
  error_number(pr->index);
  error_string(" failed to create new process (invalid parent vertex ");
  error_number(parent_vertex);
  error_string("; range 0 to ");
  error_number(parent_shape->vertices);
  error_string(").");
  return MRES_NEW_FAIL_PARENT_VERTEX;
 }

 if (pr->vertex_method [parent_vertex] == -1
		|| pr->method[pr->vertex_method [parent_vertex]].type != MTYPE_PR_LINK)
 {
  start_error(PROGRAM_TYPE_PROCESS, pr->index, pr->player_index);
  error_string("\nProcess ");
  error_number(pr->index);
  error_string(" failed to create new process (no link method on vertex ");
  error_number(parent_vertex);
  error_string(").");
  return MRES_NEW_FAIL_PARENT_VERTEX;
 }

// new_proc_shape and new_proc_size have been verified above
 struct shapestruct* child_shape = &shape_dat [prop.shape] [prop.size];
 if (child_vertex < 0 || child_vertex >= child_shape->vertices)
 {
  start_error(PROGRAM_TYPE_PROCESS, pr->index, pr->player_index);
  error_string("\nProcess ");
  error_number(pr->index);
  error_string(" failed to create new process (invalid child vertex ");
  error_number(child_vertex);
  error_string("; range 0 to ");
  error_number(child_shape->vertices);
  error_string(").");
  return MRES_NEW_FAIL_CHILD_VERTEX;
 }

 al_fixed child_angle = short_angle_to_fixed(mb [MBANK_PR_NEW_ANGLE]);

 al_fixed parent_vertex_x = pr->x + fixed_xpart(pr->angle + parent_shape->vertex_angle [parent_vertex], (parent_shape->vertex_dist [parent_vertex] + al_itofix(NEW_DISTANCE)));
 al_fixed parent_vertex_y = pr->y + fixed_ypart(pr->angle + parent_shape->vertex_angle [parent_vertex], (parent_shape->vertex_dist [parent_vertex] + al_itofix(NEW_DISTANCE)));

 al_fixed new_angle = pr->angle + child_angle + parent_shape->vertex_angle [parent_vertex] + (AFX_ANGLE_2 - child_shape->vertex_angle [child_vertex]);
 fix_fixed_angle(&new_angle);

 al_fixed new_x = parent_vertex_x - fixed_xpart(new_angle + child_shape->vertex_angle [child_vertex], child_shape->vertex_dist [child_vertex]);
 al_fixed new_y = parent_vertex_y - fixed_ypart(new_angle + child_shape->vertex_angle [child_vertex], child_shape->vertex_dist [child_vertex]);

// TO DO: I'm not sure that these check functions are 100% reliable if a group member
//  is being created, as the group member's actual x/y values may be very slightly different
//  to new_x/new_y. Need to fix this (although it should almost never matter)
//   * shouldn't be a problem now that floating point is no longer being used. I think.

 if (check_notional_block_collision_multi(prop.shape, prop.size, new_x, new_y, new_angle)
  || check_notional_solid_block_collision(prop.shape, prop.size, new_x, new_y, new_angle))
 {
/*  start_error(PROGRAM_TYPE_PROCESS, pr->index, pr->player_index);
  error_string("\nProcess ");
  error_number(pr->index);
  error_string(" failed to create new process (collision).");*/
  new_proc_fail_cloud(new_x, new_y, new_angle, prop.shape, prop.size, pr->player_index);
  return MRES_NEW_FAIL_OBSTACLE;
 }

// if it was a test, stop here:
 if (mb [MBANK_PR_NEW_STATUS] == MSTATUS_PR_NEW_TEST
  || mb [MBANK_PR_NEW_STATUS] == MSTATUS_PR_NEW_T_TEST)
 {
  return MRES_NEW_TEST_SUCCESS;
 }

// not a test - so initialise the proc.
// Note that the proc is not actually created until create_single_proc or create_new_group_member is called successfully.
//  until then, can safely return.

 struct procstruct* new_pr = &w.proc[p];

 initialise_proc(p);

 new_pr->player_index = pr->player_index;

 int i, j;

// copy the bcode:
 j = 0;
 for (i = start_address; i < (end_address + 1); i ++)
 {
  new_pr->bcode.op [j] = source_bcode->op [i];
  j++;
 }
// pad new_pr's bcode with zeroes
 while (j < PROC_BCODE_SIZE)
 {
  new_pr->bcode.op [j] = 0;
  j++;
 };

 if (!derive_proc_properties_from_bcode(new_pr, &prop, pr->index))
 {
  start_error(PROGRAM_TYPE_PROCESS, pr->index, pr->player_index);
  error_string("\nProcess ");
  error_number(pr->index);
  error_string(" failed to create new process. ");
  return MRES_NEW_FAIL_INTERFACE;
 }

// now we work out whether the procs will be joined together as part of a group.
// this happens if both procs have a LINK method at the connecting vertex (this will have been set up for new_pr in derive_proc_properties_from_bcode) and the LINK mbank register is set to 1:
 if (mb [MBANK_PR_NEW_LINK] == 1
		&& pr->vertex_method [parent_vertex] != -1
  && new_pr->vertex_method [child_vertex] != -1
  && pr->method[pr->vertex_method [parent_vertex]].type == MTYPE_PR_LINK
  && new_pr->method[new_pr->vertex_method [child_vertex]].type == MTYPE_PR_LINK)
 {
// LINK methods match up. Try to create new proc as a group member:
   if (!create_new_group_member(pr, p, parent_vertex, child_vertex, new_angle))
   {
    start_error(PROGRAM_TYPE_PROCESS, pr->index, pr->player_index);
    error_string("\nProcess ");
    error_number(pr->index);
    error_string(" failed to create new process (collision).");
    return MRES_NEW_FAIL_OBSTACLE;
   }
 }
  else
  {
// no matching LINK methods. Try to create new proc as a single, separate proc:
   if (!create_single_proc(p, new_x, new_y, new_angle))
   {
    new_proc_fail_cloud(new_x, new_y, new_angle, prop.shape, prop.size, pr->player_index);
    return MRES_NEW_FAIL_OBSTACLE;
   }
  }

 if (prop.method_error > 0)
 {
  for (i = 0; i < METHODS; i ++)
  {
   if (mtype[w.proc[p].method[i].type].mclass == MCLASS_ERROR)
   {
    start_error(PROGRAM_TYPE_PROCESS, p, pr->player_index);
    error_string("\nNew process ");
    error_number(p);
    error_string(": method ");
    error_number(i);
    switch(w.proc[p].method[i].type)
    {
     case MTYPE_ERROR_INVALID:
      error_string(" invalid type."); break;
     case MTYPE_ERROR_VERTEX:
      error_string(" invalid vertex."); break;
     case MTYPE_ERROR_MASS:
      error_string(" mass error."); break;
     case MTYPE_ERROR_DUPLICATE:
      error_string(" duplicate error."); break;
     case MTYPE_ERROR_SUB:
      error_string(" forbidden for new subprocess method."); break;
    }
   }
  }
 }

 finish_adding_new_proc(p);

// at this point the proc has been created and this function should not be able to fail.

 *(pr->data) -= prop.data_cost;
 *(pr->irpt) -= prop.irpt_cost;

 play_game_sound(SAMPLE_NEW, TONE_1G, 60, new_x, new_y);

 return MRES_NEW_SUCCESS;

}

/*
this function runs METH_PACKET, which creates a packet

returns the MBANK_PACKET_STATUS value
*/
int run_packet_method(struct procstruct* pr, int m)
{

// al_fixed packet_x = pr->x + fixed_xpart(pr->angle + pr->shape_str->vertex_angle [pr->method[m].ex_vertex], pr->shape_str->vertex_dist [pr->method[m].ex_vertex] + al_itofix(4));
// al_fixed packet_y = pr->y + fixed_ypart(pr->angle + pr->shape_str->vertex_angle [pr->method[m].ex_vertex], pr->shape_str->vertex_dist [pr->method[m].ex_vertex] + al_itofix(4));
 al_fixed packet_x = pr->x + fixed_xpart(pr->angle + pr->shape_str->vertex_angle [pr->method[m].ex_vertex], pr->shape_str->vertex_dist [pr->method[m].ex_vertex])
                           + fixed_xpart(pr->angle + pr->method[m].ex_angle, 5);
 al_fixed packet_y = pr->y + fixed_ypart(pr->angle + pr->shape_str->vertex_angle [pr->method[m].ex_vertex], pr->shape_str->vertex_dist [pr->method[m].ex_vertex])
                           + fixed_ypart(pr->angle + pr->method[m].ex_angle, 5);

 int pk = new_packet(PACKET_TYPE_BASIC, pr->player_index, packet_x, packet_y);

 if (pk == -1)
  return MSTATUS_PR_PACKET_FAIL_TOO_MANY;
// another failure status is MSTATUS_PACKET_FAIL_IRPT

 struct packetstruct* pack = &w.packet[pk];

// the old_x etc values are used to work out the velocity of the vertex (which is added to the packet's velocity).
// the calculation isn't perfect, because of the +1000 added to the vertex distance. But it's close enough.
 al_fixed vertex_x  = pr->x + fixed_xpart(pr->angle + pr->shape_str->vertex_angle [pr->method[m].ex_vertex], pr->shape_str->vertex_dist [pr->method[m].ex_vertex] + al_itofix(1));
 al_fixed vertex_y  = pr->y + fixed_ypart(pr->angle + pr->shape_str->vertex_angle [pr->method[m].ex_vertex], pr->shape_str->vertex_dist [pr->method[m].ex_vertex] + al_itofix(1));
 al_fixed vertex_old_x = pr->old_x + fixed_xpart(pr->old_angle + pr->shape_str->vertex_angle [pr->method[m].ex_vertex], pr->shape_str->vertex_dist [pr->method[m].ex_vertex] + al_itofix(1));
 al_fixed vertex_old_y = pr->old_y + fixed_ypart(pr->old_angle + pr->shape_str->vertex_angle [pr->method[m].ex_vertex], pr->shape_str->vertex_dist [pr->method[m].ex_vertex] + al_itofix(1));

#define PACKET_SPEED_BASE 6
#define PACKET_SPEED_EXT 2

 int speed = PACKET_SPEED_BASE + pr->method[m].extension [MEX_PR_PACKET_SPEED] * PACKET_SPEED_EXT;

 pack->x_speed = (vertex_x - vertex_old_x) + fixed_xpart(pr->angle + pr->method[m].ex_angle, al_itofix(speed));
 pack->y_speed = (vertex_y - vertex_old_y) + fixed_ypart(pr->angle + pr->method[m].ex_angle, al_itofix(speed));

 pack->timeout = 72;
 pack->timeout += pr->method[m].extension [MEX_PR_PACKET_RANGE] * 21;
// the speed extension reduces the timeout proportionally
 pack->timeout *= PACKET_SPEED_BASE;
 pack->timeout /= speed;

 pack->angle = get_angle(pack->y_speed, pack->x_speed);//pr->angle;
 if (pr->regs.mbank [(m*METHOD_SIZE) + MBANK_PR_PACKET_FRIENDLY] == 1)
  pack->team_safe = -1;
   else
    pack->team_safe = pr->player_index;
 pack->damage = 12 + pr->method[m].extension [MEX_PR_PACKET_POWER] * 3;//16 + pr->method[m].extension [MEX_PR_PACKET_POWER] * 4;
 pack->colour = pr->player_index;

 pack->method_extensions [MEX_PR_PACKET_POWER] = pr->method[m].extension [MEX_PR_PACKET_POWER]; // used for display
 pack->method_extensions [MEX_PR_PACKET_SPEED] = pr->method[m].extension [MEX_PR_PACKET_SPEED]; // used for display
 pack->method_extensions [MEX_PR_PACKET_RANGE] = pr->method[m].extension [MEX_PR_PACKET_RANGE]; // used for display
 pack->collision_size = 0; //pr->method[m].extension [MEX_PR_PACKET_POWER]; // used for actual collision tests

 pack->source_proc = pr->index;
 pack->source_method = m;
 pack->source_vertex = pr->method[m].ex_vertex;

 pr->method[m].data [MDATA_PR_PACKET_RECYCLE] = PACKET_RECYCLE_TIME;

 play_game_sound(SAMPLE_CHIRP, TONE_2C - (pr->method[m].extension [MEX_PR_PACKET_POWER] * 3), 100, pr->x, pr->y);

 return MSTATUS_PR_PACKET_SUCCESS;


}


int run_dpacket_method(struct procstruct* pr, int m)
{

// al_fixed packet_x = pr->x + fixed_xpart(pr->angle + pr->shape_str->vertex_angle [pr->method[m].ex_vertex], pr->shape_str->vertex_dist [pr->method[m].ex_vertex] + al_itofix(4)) + fixed_xpart(int_angle_to_fixed(pr->method[m].data [MDATA_PR_DPACKET_ANGLE]), al_itofix(4));
// al_fixed packet_y = pr->y + fixed_ypart(pr->angle + pr->shape_str->vertex_angle [pr->method[m].ex_vertex], pr->shape_str->vertex_dist [pr->method[m].ex_vertex] + al_itofix(4)) + fixed_ypart(int_angle_to_fixed(pr->method[m].data [MDATA_PR_DPACKET_ANGLE]), al_itofix(4));
 al_fixed packet_x = pr->x + fixed_xpart(pr->angle + pr->shape_str->vertex_angle [pr->method[m].ex_vertex], pr->shape_str->vertex_dist [pr->method[m].ex_vertex])
                           + fixed_xpart(pr->angle + pr->method[m].ex_angle, 5);
 al_fixed packet_y = pr->y + fixed_ypart(pr->angle + pr->shape_str->vertex_angle [pr->method[m].ex_vertex], pr->shape_str->vertex_dist [pr->method[m].ex_vertex])
                           + fixed_ypart(pr->angle + pr->method[m].ex_angle, 5);

// al_fixed packet_x = pr->x + fixed_xpart(pr->angle + pr->shape_str->vertex_angle [pr->method[m].ex_vertex], pr->shape_str->vertex_dist [pr->method[m].ex_vertex] + al_itofix(4)) + al_itofix(xpart(pr->method[m].data [MDATA_PR_DPACKET_ANGLE], 4));
// al_fixed packet_y = pr->y + fixed_ypart(pr->angle + pr->shape_str->vertex_angle [pr->method[m].ex_vertex], pr->shape_str->vertex_dist [pr->method[m].ex_vertex] + al_itofix(4)) + al_itofix(ypart(pr->method[m].data [MDATA_PR_DPACKET_ANGLE], 4));

 int pk = new_packet(PACKET_TYPE_DPACKET, pr->player_index, packet_x, packet_y);

 if (pk == -1)
  return MSTATUS_PR_PACKET_FAIL_TOO_MANY;
// another failure status is MSTATUS_PACKET_FAIL_IRPT

 struct packetstruct* pack = &w.packet[pk];

// the old_x etc values are used to work out the velocity of the vertex (which is added to the packet's velocity).
// the calculation isn't perfect, because of the +1000 added to the vertex distance. But it's close enough.
 al_fixed vertex_x  = pr->x + fixed_xpart(pr->angle + pr->shape_str->vertex_angle [pr->method[m].ex_vertex], pr->shape_str->vertex_dist [pr->method[m].ex_vertex] + al_itofix(1));
 al_fixed vertex_y  = pr->y + fixed_ypart(pr->angle + pr->shape_str->vertex_angle [pr->method[m].ex_vertex], pr->shape_str->vertex_dist [pr->method[m].ex_vertex] + al_itofix(1));
 al_fixed vertex_old_x = pr->old_x + fixed_xpart(pr->old_angle + pr->shape_str->vertex_angle [pr->method[m].ex_vertex], pr->shape_str->vertex_dist [pr->method[m].ex_vertex] + al_itofix(1));
 al_fixed vertex_old_y = pr->old_y + fixed_ypart(pr->old_angle + pr->shape_str->vertex_angle [pr->method[m].ex_vertex], pr->shape_str->vertex_dist [pr->method[m].ex_vertex] + al_itofix(1));

#define DPACKET_SPEED_BASE 5
#define DPACKET_SPEED_EXT 2

 int speed = DPACKET_SPEED_BASE + pr->method[m].extension [MEX_PR_PACKET_SPEED] * DPACKET_SPEED_EXT;

// al_fixed firing_angle = pr->angle + pr->method[m].ex_angle + short_angle_to_fixed(pr->method[m].data [MDATA_PR_DPACKET_ANGLE]);
 al_fixed firing_angle = pr->angle + pr->method[m].ex_angle;// + pr->shape_str->vertex_angle [pr->method[m].ex_vertex];

 pack->x_speed = (vertex_x - vertex_old_x) + fixed_xpart(firing_angle, al_itofix(speed));
 pack->y_speed = (vertex_y - vertex_old_y) + fixed_ypart(firing_angle, al_itofix(speed));

 pack->timeout = 64;
 pack->timeout += pr->method[m].extension [MEX_PR_PACKET_RANGE] * 17;
// the speed extension reduces the timeout proportionally
 pack->timeout *= DPACKET_SPEED_BASE;
 pack->timeout /= speed;

 pack->angle = get_angle(pack->y_speed, pack->x_speed);//pr->angle;
// pack->team_safe = pr->player_index;
 if (pr->regs.mbank [(m*METHOD_SIZE) + MBANK_PR_DPACKET_FRIENDLY] == 1)
  pack->team_safe = -1;
   else
    pack->team_safe = pr->player_index;
 pack->damage = 9 + pr->method[m].extension [MEX_PR_PACKET_POWER] * 2; //12 + pr->method[m].extension [MEX_PR_PACKET_POWER] * 3;
 pack->colour = pr->player_index;

 pack->method_extensions [MEX_PR_PACKET_POWER] = pr->method[m].extension [MEX_PR_PACKET_POWER]; // used for display
 pack->method_extensions [MEX_PR_PACKET_SPEED] = pr->method[m].extension [MEX_PR_PACKET_SPEED]; // used for display
 pack->method_extensions [MEX_PR_PACKET_RANGE] = pr->method[m].extension [MEX_PR_PACKET_RANGE]; // used for display
 pack->collision_size = 0;//pr->method[m].extension [MEX_PR_PACKET_POWER]; // used for actual collision tests

 pack->source_proc = pr->index;
 pack->source_method = m;
 pack->source_vertex = pr->method[m].ex_vertex;

 pr->method[m].data [MDATA_PR_PACKET_RECYCLE] = PACKET_RECYCLE_TIME;

 play_game_sound(SAMPLE_CHIRP, TONE_2C - (pr->method[m].extension [MEX_PR_PACKET_POWER] * 3), 100, pr->x, pr->y);

 return MSTATUS_PR_PACKET_SUCCESS;


}


int run_designate_point(struct procstruct* pr, int m, s16b* mb)
{

   al_fixed des_x_offset = al_itofix((int) mb [MBANK_PR_DESIGNATE_X]);
   al_fixed des_y_offset = al_itofix((int) mb [MBANK_PR_DESIGNATE_Y]);

   al_fixed range = al_itofix(pr->method[m].data [MDATA_PR_DESIGNATE_RANGE]);
   al_fixed negative_range = al_fixsub(0, range);

   if (des_x_offset > range)
    des_x_offset = range;
     else
     {
      if (des_x_offset < negative_range)
       des_x_offset = negative_range;
     }
   if (des_y_offset > range)
    des_y_offset = range;
     else
     {
      if (des_y_offset < negative_range)
       des_y_offset = negative_range;
     }

 int find_proc = check_point_collision(pr->x + des_x_offset, pr->y + des_y_offset);

 if (find_proc == -1)
 {
  pr->method [m].data [MDATA_PR_DESIGNATE_INDEX] = -1;
  return 0;
 }

// note that these are hidden data variables, not mbank entries. The target proc's index is not directly available to the proc.
 pr->method [m].data [MDATA_PR_DESIGNATE_INDEX] = find_proc;
 return 1;

}

// looks for designated proc; if in range, finds it and fills in x/y mbank entries
// returns 0 if not found
//  - doesn't distinguish between different reasons for not having found it
//  - assumes that MDATA_PR_DESIGNATE_INDEX will have been set to -1 if designated proc destroyed
int run_designate_find_proc(struct procstruct* pr, int m, s16b* mb, int method_status)
{

 int target_index = pr->method [m].data [MDATA_PR_DESIGNATE_INDEX];

 if (target_index == -1)
  return 0; // not found

 al_fixed range = al_itofix(pr->method[m].data [MDATA_PR_DESIGNATE_RANGE]);

// for now range is a square:
 if (w.proc[target_index].x < pr->x - range
  || w.proc[target_index].x > pr->x + range
  || w.proc[target_index].y < pr->y - range
  || w.proc[target_index].y > pr->y + range)
 {
  return 0; // not found
 }

 if (method_status == MSTATUS_PR_DESIGNATE_LOCATE)
	{
  mb [MBANK_PR_DESIGNATE_X] = al_fixtoi(w.proc[target_index].x - pr->x);
  mb [MBANK_PR_DESIGNATE_Y] = al_fixtoi(w.proc[target_index].y - pr->y);
	}
	 else // must be MSTATUS_PR_DESIGNATE_SPEED
	  {
    mb [MBANK_PR_DESIGNATE_X] = al_fixtoi(w.proc[target_index].x_speed * 16);
    mb [MBANK_PR_DESIGNATE_Y] = al_fixtoi(w.proc[target_index].y_speed * 16);
	  }

 return 1;

}

/*
// this function is used when:
//  - an irpt gen method is automatically called at start of execution
//  - an irpt gen method is called by proc during execution
// returns amount generated.
int generate_irpt(struct procstruct* pr, int m, int amount)
{

// fprintf(stdout, "\ngenerate amount %i for %i", amount, pr->index);

 if (amount <= 0)
  return 0;

 if (amount > pr->method[m].data [MDATA_PR_IRPT_LEFT])
  amount = pr->method[m].data [MDATA_PR_IRPT_LEFT];

// fprintf(stdout, "\ngenerate amount %i for %i (%i) before %i ", amount, pr->index, amount, *instr_left);

 if (*(pr->irpt) + amount > *(pr->irpt_max))
  amount = *(pr->irpt_max) - *(pr->irpt);

 if (amount == 0)
  return 0;

 *(pr->irpt) += amount;
 pr->method[m].data [MDATA_PR_IRPT_LEFT] -= amount;
 pr->irpt_base += amount; // so that generated irpt isn't taken into account in working out irpt_use

// mb [MBANK_PR_IRPT_LEFT] = pr->method[m].data [MDATA_PR_IRPT_LEFT];

 return amount;

// fprintf(stdout, "\nGenerated %i for %i", amount, pr->index);

}
*/

// Call this function if a process is broadcasting.
// Don't call if power mbank register <= zero.
// returns 1 on success, 0 on failure.
s16b run_broadcast(struct procstruct* source_proc, int m)
{

 s16b* mb = &source_proc->regs.mbank [m*METHOD_SIZE];

 s16b message_id = mb [MBANK_PR_BROADCAST_ID];
 s16b message_value1 = mb [MBANK_PR_BROADCAST_VALUE1];
 s16b message_value2 = mb [MBANK_PR_BROADCAST_VALUE2];
 int range = mb [MBANK_PR_BROADCAST_POWER];
 al_fixed fix_range;
 al_fixed x = source_proc->x;
 al_fixed y = source_proc->y;

 if (range > source_proc->method[m].data [MDATA_PR_BROADCAST_RANGE_MAX])
  range = source_proc->method[m].data [MDATA_PR_BROADCAST_RANGE_MAX];

 fix_range = al_itofix(range);

// it costs irpt to broadcast:
 int irpt_cost = 32 + (range / 32);

 if (*(source_proc->irpt) < irpt_cost)
  goto failed_broadcast;
 *(source_proc->irpt) -= irpt_cost;

 int block_x1 = fixed_to_block(x - fix_range);
 if (block_x1 < 2)
  block_x1 = 2;
 int block_y1 = fixed_to_block(y - fix_range);
 if (block_y1 < 2)
  block_y1 = 2;
 int block_x2 = fixed_to_block(x + fix_range);
 if (block_x2 >= w.w_block - 2)
  block_x2 = w.w_block - 2;
 int block_y2 = fixed_to_block(y + fix_range);
 if (block_y2 >= w.h_block - 2)
  block_y2 = w.h_block - 2;

 int bx, by;
 int write_address;
 int x_offset, y_offset;
 struct procstruct* check_proc;

 for (bx = block_x1; bx < block_x2; bx ++)
 {
  for (by = block_y1; by < block_y2; by ++)
  {

#ifdef SANITY_CHECK
   if (bx < 2
    || bx >= w.w_block - 2
    || by < 2
    || by >= w.h_block - 2)
   {
    fprintf(stdout, "\nError: g_method_pr.c: run_broadcast(): block (%i, %i) out of bounds (max %i, %i)", bx, by, w.w_block - 2, w.h_block - 2);
    error_call();
   }
#endif


   if (w.block[bx][by].tag != w.blocktag) // nothing is in this block this cycle
    continue;

   check_proc = w.block[bx][by].blocklist_down;

   while(check_proc != NULL)
   {

    if (check_proc->player_index != source_proc->player_index
     || check_proc->listen_method == -1
     || check_proc == source_proc)
    {
     check_proc = check_proc->blocklist_down;
     continue;
    }
    if ((message_id | check_proc->regs.mbank [(check_proc->listen_method * METHOD_SIZE) + MBANK_PR_LISTEN_ID_WANT]) == 0
     || (message_id & check_proc->regs.mbank [(check_proc->listen_method * METHOD_SIZE) + MBANK_PR_LISTEN_ID_NEED]) != check_proc->regs.mbank [(check_proc->listen_method * METHOD_SIZE) + MBANK_PR_LISTEN_ID_NEED]
     || (message_id & check_proc->regs.mbank [(check_proc->listen_method * METHOD_SIZE) + MBANK_PR_LISTEN_ID_REJECT]) != 0
     || check_proc->method[check_proc->listen_method].data[MDATA_PR_LISTEN_RECEIVED] >= LISTEN_MESSAGES_MAX)
    {
     check_proc = check_proc->blocklist_down;
     continue;
    }
    x_offset = al_fixtoi(al_fixsub(x, check_proc->x));
    y_offset = al_fixtoi(al_fixsub(y, check_proc->y));
// check_proc may be out of range even if part of its block is in range:
    if (abs(x_offset) > range
     || abs(y_offset) > range)
    {
     check_proc = check_proc->blocklist_down;
     continue;
    }
    write_address = check_proc->regs.mbank [(check_proc->listen_method * METHOD_SIZE) + MBANK_PR_LISTEN_ADDRESS];
    write_address += check_proc->method[check_proc->listen_method].data[MDATA_PR_LISTEN_RECEIVED] * LISTEN_MESSAGE_SIZE;
// make sure write_address is within bounds, with enough space
    if (write_address <= 0 // reject 0 (as this probably means the listen method hasn't been set up properly)
     || write_address >= PROC_BCODE_SIZE - (LISTEN_MESSAGE_SIZE * 2))
    {
     check_proc = check_proc->blocklist_down;
     continue;
    }
    check_proc->bcode.op [write_address + LISTEN_RECORD_ID] = message_id;
    check_proc->bcode.op [write_address + LISTEN_RECORD_VALUE1] = message_value1;
    check_proc->bcode.op [write_address + LISTEN_RECORD_VALUE2] = message_value2;
    check_proc->bcode.op [write_address + LISTEN_RECORD_OFFSET_X] = x_offset;
    check_proc->bcode.op [write_address + LISTEN_RECORD_OFFSET_Y] = y_offset;
// TO DO: it should probably cost irpt or instr to receive a message.
    check_proc->method[check_proc->listen_method].data[MDATA_PR_LISTEN_RECEIVED] ++;
    check_proc = check_proc->blocklist_down;
   }; // end while (check_proc != NULL) loop
  } // end for by
 } // end for bx



 return 1; // 1 indicates successful broadcast (although it doesn't guarantee that the message was received)

 failed_broadcast:
  return 0;

}


// Call this when a yield method is called.
// Only call if either of first two method registers is > 0 (as otherwise no transfer is being attempted)
s16b run_yield(struct procstruct* pr, int m)
{


 s16b* mb = &pr->regs.mbank [m*METHOD_SIZE];

 s16b irpt_transfer = mb [MBANK_PR_YIELD_IRPT];
 if (irpt_transfer < 0)
  irpt_transfer = 0;
 if (irpt_transfer > pr->method[m].data [MDATA_PR_YIELD_RATE_MAX_IRPT])
  irpt_transfer = pr->method[m].data [MDATA_PR_YIELD_RATE_MAX_IRPT];

#define YIELD_OVERHEAD (32)
 //* pr->special_method_penalty)

 int total_irpt_cost = irpt_transfer + YIELD_OVERHEAD; // add some overhead (this is also done below if irpt_transfer has to be recalculated)

 if (*(pr->irpt) < total_irpt_cost)
  return MRES_YIELD_FAIL_IRPT; // failed (we could just try to transfer what we can, but let's not do that yet)

 s16b data_transfer = mb [MBANK_PR_YIELD_DATA];
 if (data_transfer < 0)
  data_transfer = 0;
 if (data_transfer > pr->method[m].data [MDATA_PR_YIELD_RATE_MAX_DATA])
  data_transfer = pr->method[m].data [MDATA_PR_YIELD_RATE_MAX_DATA];
 if (data_transfer > *(pr->data))
  data_transfer = *(pr->data);

 if (irpt_transfer == 0
  && data_transfer == 0)
   return MRES_YIELD_FAIL_DATA; // nothing to do.

 int range = pr->method[m].data [MDATA_PR_YIELD_RANGE];
 if (mb [MBANK_PR_YIELD_X] > range
  || mb [MBANK_PR_YIELD_X] < 0-range
  || mb [MBANK_PR_YIELD_Y] > range
  || mb [MBANK_PR_YIELD_Y] < 0-range)
  return MRES_YIELD_FAIL_RANGE;

 int find_proc = check_point_collision(pr->x + al_itofix(mb [MBANK_PR_YIELD_X]), pr->y + al_itofix(mb [MBANK_PR_YIELD_Y]));

 if (find_proc == -1
  || find_proc == pr->index // don't bother transferring to the proc itself
  || (pr->group != -1
			&&	w.proc[find_proc].group == pr->group)) // don't bother transferring to other members of the same group
  return MRES_YIELD_FAIL_TARGET;

 struct procstruct* target_proc = &w.proc[find_proc];

 if (data_transfer > *(target_proc->data_max) - *(target_proc->data))
  data_transfer = *(target_proc->data_max) - *(target_proc->data);

 if (irpt_transfer > *(target_proc->irpt_max) - *(target_proc->irpt))
 {
  irpt_transfer = *(target_proc->irpt_max) - *(target_proc->irpt);
  total_irpt_cost = irpt_transfer + YIELD_OVERHEAD; // this is also done above
 }

 if (data_transfer == 0
  && irpt_transfer == 0)
   return MRES_YIELD_FAIL_FULL; // nothing to do as target is full

 *(target_proc->data) += data_transfer;
 *(pr->data) -= data_transfer;
 *(target_proc->irpt) += irpt_transfer;
 *(pr->irpt) -= total_irpt_cost;

// set the first two registers to zero to indicate success:
// pr->regs.mbank[(m*METHOD_SIZE) + MBANK_PR_YIELD_IRPT] = 0;
// pr->regs.mbank[(m*METHOD_SIZE) + MBANK_PR_YIELD_DATA] = 0;
//  (I could set the registers to the amount actually transferred, but that would mean the process would always need to re-initialise them after use)


 pr->method[m].data [MDATA_PR_YIELD_ACTIVATED] = 1;

    struct cloudstruct* cld;

    cld = new_cloud(CLOUD_YIELD_LINE, pr->x + fixed_xpart(pr->angle + pr->shape_str->vertex_angle [pr->method[m].ex_vertex], pr->shape_str->vertex_dist [pr->method[m].ex_vertex] + al_itofix(9)),
                                      pr->y + fixed_ypart(pr->angle + pr->shape_str->vertex_angle [pr->method[m].ex_vertex], pr->shape_str->vertex_dist [pr->method[m].ex_vertex] + al_itofix(9)));

    if (cld != NULL)
    {
     cld->timeout = 8;// + data_allocated;
     cld->colour = pr->player_index;

     cld->data [0] = target_proc->index;
     cld->data [1] = grand(30000); // this is just used as a pseudorandom number seed, so it can be any value
    }

 return MRES_YIELD_SUCCESS;

}



// Call this for both PR_STREAM and PR_DSTREAM methods
void run_stream_method(struct procstruct* pr, int m, int method_type)
{

// This is structured as a series of if statements rather than a switch because many of the ifs fall through to each other under certain circumstances
// that might be confusing in a switch.

// This method has a huge upfront cost (which requires a multi-part process to even have a buffer large enough to pay it at once)
//  then doesn't cost any more while it runs.

 int step_limit = 64 + (pr->method[m].extension [MEX_PR_STREAM_RANGE] * 24);
 int steps = step_limit;
// int method_cost = STREAM_IRPT_COST + ((STREAM_IRPT_COST / 2) * (pr->method[m].extension [MEX_PR_STREAM_CYCLE] + pr->method[m].extension [MEX_PR_STREAM_RANGE] + pr->method[m].extension [MEX_PR_STREAM_TIME]));
 int damage = 36;
 if (method_type == MTYPE_PR_DSTREAM)
		 damage = 24;

 if (pr->method[m].data [MDATA_PR_STREAM_STATUS] == STREAM_STATUS_INACTIVE)
 {
   int method_cost = 2048 + (512 * (pr->method[m].extension [MEX_PR_STREAM_CYCLE] + pr->method[m].extension [MEX_PR_STREAM_RANGE] + pr->method[m].extension [MEX_PR_STREAM_TIME]));
//   method_cost *= pr->special_method_penalty;

   if (*(pr->irpt) < method_cost)
	  {
// set firing register to 0
    pr->regs.mbank [(m*METHOD_SIZE) + MBANK_PR_STREAM_FIRE] = 0;
    return; // failed
	  }
	  *(pr->irpt) -= method_cost;

   pr->method[m].data [MDATA_PR_STREAM_STATUS] = STREAM_STATUS_WARMUP;
   pr->method[m].data [MDATA_PR_STREAM_COUNTER] = STREAM_WARMUP_LENGTH; // warmup length
// set the firing register to 0:
   pr->regs.mbank [(m*METHOD_SIZE) + MBANK_PR_STREAM_FIRE] = 0;
 }

 if (pr->method[m].data [MDATA_PR_STREAM_STATUS] == STREAM_STATUS_WARMUP)
 {
    pr->method[m].data [MDATA_PR_STREAM_COUNTER] --;
    if (pr->method[m].data [MDATA_PR_STREAM_COUNTER] == 0)
    {
     pr->method[m].data [MDATA_PR_STREAM_STATUS] = STREAM_STATUS_FIRING; // in this case the irpt cost will be paid below.
//     pr->method[m].data [MDATA_PR_STREAM_COUNTER] = ;// pr->regs.mbank [(m*METHOD_SIZE) + MBANK_PR_STREAM_TIME];
     pr->method[m].data [MDATA_PR_STREAM_RECYCLE] = 96 - (pr->method[m].extension [MEX_PR_STREAM_CYCLE] * 16);
//     if (pr->method[m].data [MDATA_PR_STREAM_COUNTER] > 6 + (pr->method[m].extension [MEX_PR_STREAM_TIME] * 3))
     pr->method[m].data [MDATA_PR_STREAM_COUNTER] = 6 + (pr->method[m].extension [MEX_PR_STREAM_TIME] * 3);
     pr->method[m].data [MDATA_PR_STREAM_MAX_COUNTER] = pr->method[m].data [MDATA_PR_STREAM_COUNTER];
     play_game_sound(SAMPLE_OVER, TONE_2C, 100, pr->x, pr->y);
    }
      else
      {
       steps = ((STREAM_WARMUP_LENGTH - pr->method[m].data [MDATA_PR_STREAM_COUNTER]) * step_limit) / STREAM_WARMUP_LENGTH;
      }
 }

 if (pr->method[m].data [MDATA_PR_STREAM_STATUS] == STREAM_STATUS_FIRING)
 {
    pr->method[m].data [MDATA_PR_STREAM_COUNTER] --;
    if (pr->method[m].data [MDATA_PR_STREAM_COUNTER] == 0)
    {
     pr->method[m].data [MDATA_PR_STREAM_STATUS] = STREAM_STATUS_COOLDOWN;
     pr->method[m].data [MDATA_PR_STREAM_COUNTER] = STREAM_COOLDOWN_LENGTH;
    }
 }

 if (pr->method[m].data [MDATA_PR_STREAM_STATUS] == STREAM_STATUS_COOLDOWN)
 {
  pr->method[m].data [MDATA_PR_STREAM_COUNTER] --;
  if (pr->method[m].data [MDATA_PR_STREAM_COUNTER] <= 0)
  {
   pr->method[m].data [MDATA_PR_STREAM_STATUS] = STREAM_STATUS_INACTIVE;
   pr->method[m].data [MDATA_PR_STREAM_COUNTER] = 0;
   return;
  }
//  steps = (pr->method[m].data [MDATA_PR_STREAM_COUNTER] * step_limit) / STREAM_COOLDOWN_LENGTH;
  steps += pr->method[m].data [MDATA_PR_STREAM_COUNTER] - STREAM_COOLDOWN_LENGTH;
 }

// Now is firing, warmup up or cooling down. Need to do collision detection for the beam:

 al_fixed start_x, start_y;
 al_fixed x, y;
 al_fixed x_inc, y_inc;
 al_fixed firing_angle_fixed = pr->angle + pr->method[m].ex_angle;// + pr->shape_str->vertex_angle [pr->method[m].ex_vertex];

// int steps;
 int i;
 int hit_proc = -1;

// start_x = pr->x + fixed_xpart(pr->angle + pr->shape_str->vertex_angle [pr->method[m].ex_vertex], pr->shape_str->vertex_dist [pr->method[m].ex_vertex] + al_itofix(4));
// start_y = pr->y + fixed_ypart(firing_angle_fixed, pr->shape_str->vertex_dist [pr->method[m].ex_vertex] + al_itofix(4));
 start_x = pr->x + fixed_xpart(pr->angle + pr->shape_str->vertex_angle [pr->method[m].ex_vertex], pr->shape_str->vertex_dist [pr->method[m].ex_vertex]);
 start_y = pr->y + fixed_ypart(pr->angle + pr->shape_str->vertex_angle [pr->method[m].ex_vertex], pr->shape_str->vertex_dist [pr->method[m].ex_vertex]);
 start_x += fixed_xpart(pr->angle + pr->method[m].ex_angle, al_itofix(4));
 start_y += fixed_ypart(pr->angle + pr->method[m].ex_angle, al_itofix(4));

 x = start_x;
 y = start_y;
 x_inc = fixed_xpart(firing_angle_fixed, STREAM_FIX_STEP_PIXELS);
 y_inc = fixed_ypart(firing_angle_fixed, STREAM_FIX_STEP_PIXELS);

 for (i = 0; i < steps; i ++)
 {
// collision check
 hit_proc = check_point_collision_ignore_team(x, y, pr->player_index);

 if (hit_proc != -1)
 {
  w.proc[hit_proc].hit = 2;
  if (pr->method[m].data [MDATA_PR_STREAM_STATUS] == STREAM_STATUS_FIRING)
  {
   apply_packet_damage_to_proc(&w.proc[hit_proc], damage, pr->player_index);
  }
  break;
 }


// edge check
  if (x < (al_itofix(2) * BLOCK_SIZE_PIXELS)
   || x > w.w_fixed - (al_itofix(2 * BLOCK_SIZE_PIXELS))
   || y < (al_itofix(2) * BLOCK_SIZE_PIXELS)
   || y > w.h_fixed - (al_itofix(2 * BLOCK_SIZE_PIXELS)))
   break; // hit_proc will be -1

  x += x_inc;
  y += y_inc;

 }

// now create the beam cloud thing:

  struct cloudstruct* cl;

  cl = new_cloud(CLOUD_STREAM, start_x, start_y);

  if (cl != NULL)
  {
    cl->timeout = 1;
    cl->colour = pr->player_index;
    cl->x2 = x;
    cl->y2 = y;
    cl->data [0] = pr->method[m].data [MDATA_PR_STREAM_STATUS];
    cl->data [1] = pr->method[m].data [MDATA_PR_STREAM_COUNTER];
    cl->data [2] = 0;
    if (hit_proc != -1)
     cl->data [2] = 1;
  }


}





