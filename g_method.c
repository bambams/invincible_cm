#include <allegro5/allegro.h>

#include <stdio.h>
#include <math.h>

#include "m_config.h"

#include "g_header.h"

#include "g_world.h"
#include "g_misc.h"
#include "g_proc.h"
#include "g_packet.h"
#include "g_group.h"
#include "g_motion.h"
#include "g_method.h"
#include "g_method_ex.h"
#include "g_method_sy.h"
#include "g_method_pr.h"
#include "g_method_clob.h"
#include "g_method_misc.h"
#include "g_cloud.h"
#include "g_shape.h"
#include "m_globvars.h"
#include "m_maths.h"
#include "t_template.h"
#include "x_sound.h"

#include "i_console.h"
#include "i_disp_in.h"

/*

This file contains code for the active method pass that checks the active methods of each proc each tick.
It doesn't deal with called methods - they're in v_interp.c
It may deal with passive methods. not sure.


active_method_pass_each_tick() needs to:
 - go through each proc
 - go through each method, activating if active

in future, need to use a linked list approach


g_method_ex.c contains special code for some complex methods

*/

extern struct viewstruct view; // TO DO: think about putting a pointer to this in the worldstruct instead of externing it
extern struct controlstruct control; // defined in i_input.c. Used here for client process methods.
extern struct gamestruct game;
extern struct shapestruct shape_dat [SHAPES] [SHAPES_SIZES]; // defined in g_shape.c

extern struct templstruct templ [TEMPLATES]; // defined in t_template.c
/*
#define DATA_COST_NONE 0
#define DATA_COST_MIN 1
#define DATA_COST_LOW 2
#define DATA_COST_MED 3
#define DATA_COST_HIGH 8

#define EXTENSION_COST_NONE 0
#define EXTENSION_COST_MIN 1
#define EXTENSION_COST_LOW 2
#define EXTENSION_COST_MED 3
#define EXTENSION_COST_HIGH 4

#define UPKEEP_COST_NONE 0
#define UPKEEP_COST_MIN 2
#define UPKEEP_COST_LOW 4
#define UPKEEP_COST_MED 8
#define UPKEEP_COST_HIGH 12

#define EXT_UPKEEP_COST_NONE 0
#define EXT_UPKEEP_COST_MIN 1
#define EXT_UPKEEP_COST_LOW 2
#define EXT_UPKEEP_COST_MED 3
#define EXT_UPKEEP_COST_HIGH 4
*/

const struct method_costsstruct method_cost =
{
 {0, 2, 4, 8, 16, 32}, // base_cost
 {0, 2, 4, 8, 16, 32}, // upkeep_cost
 {0, 1, 2, 4, 8, 16}, // extension_cost
 {0, 1, 2, 4, 8, 16}, // extension_upkeep_cost
};

const struct mtypestruct mtype [MTYPES] =
{

// {"name", mclass, mbank_size, external, base_data_cost, base_upkeep_cost, extension_types, max_extensions, {"extension names"}, extension_data_cost, extension_upkeep_cost}
 {"(none)", MCLASS_ALL, 1, METHOD_COST_CAT_NONE, MEXT_INTERNAL, 0, 0, {}}, // MTYPE_NONE,

 {"move", MCLASS_PR, 1, METHOD_COST_CAT_HIGH, MEXT_EXTERNAL_ANGLED, 1, 4, {"power"}}, // MTYPE_PR_MOVE,
 {"new process", MCLASS_PR, 2, METHOD_COST_CAT_ULTRA, MEXT_INTERNAL, 0, 0, {}}, // MTYPE_PR_NEW, // creates a new proc
 {"new subpr", MCLASS_PR, 2, METHOD_COST_CAT_MED, MEXT_INTERNAL, 0, 0, {}}, // MTYPE_PR_NEW_SUB, // creates a new proc (limited)
 {"packet", MCLASS_PR, 1, METHOD_COST_CAT_HIGH, MEXT_EXTERNAL_ANGLED, 3, 4, {"power", "speed", "time"}}, // MTYPE_PR_PACKET, // creates a packet
 {"d-packet", MCLASS_PR, 1, METHOD_COST_CAT_HIGH, MEXT_EXTERNAL_ANGLED, 3, 4, {"power", "speed", "time"}}, // MTYPE_PR_DPACKET, // creates a packet; can rotate
 {"stream", MCLASS_PR, 1, METHOD_COST_CAT_ULTRA, MEXT_EXTERNAL_ANGLED, 3, 4, {"time", "range", "cycle"}}, // MTYPE_PR_STREAM
 {"d-stream", MCLASS_PR, 1, METHOD_COST_CAT_ULTRA, MEXT_EXTERNAL_ANGLED, 3, 4, {"time", "range", "cycle"}}, // MTYPE_PR_DSTREAM
 {"scan", MCLASS_PR, 3, METHOD_COST_CAT_MED, MEXT_INTERNAL, 1, 4, {"range"}}, // MTYPE_PR_SCAN, // scans for nearby procs
 {"gen irpt", MCLASS_PR, 1, METHOD_COST_CAT_ULTRA, MEXT_INTERNAL, 1, 4, {"capacity"}}, // MTYPE_PR_IRPT,
 {"allocate", MCLASS_PR, 1, METHOD_COST_CAT_ULTRA, MEXT_EXTERNAL_NO_ANGLE, 0, 0, {}}, // MTYPE_PR_ALLOCATE,
// {1, 0, DATA_COST_MIN}, // MTYPE_GET_XY, // provides x, y (in pixels) and x_speed, y_speed information
 {"std", MCLASS_PR, 1, METHOD_COST_CAT_MIN, MEXT_INTERNAL, 0, 0, {}}, // MTYPE_PR_STD // standard process method - contains all kinds of things
 {"maths", MCLASS_PR, 1, METHOD_COST_CAT_MIN, MEXT_INTERNAL, 0, 0, {}}, // MTYPE_PR_MATHS, // performs various trig functions
 {"designate", MCLASS_PR, 1, METHOD_COST_CAT_MIN, MEXT_INTERNAL, 1, 4, {"range"}}, // MTYPE_PR_DESIGNATE
 {"link", MCLASS_PR, 1, METHOD_COST_CAT_MIN, MEXT_EXTERNAL_NO_ANGLE, 0, 0, {}}, // MTYPE_PR_LINK
// {"time", MCLASS_PR, 1, METHOD_COST_CAT_MIN, MEXT_INTERNAL, 0, 0, {}}, // MTYPE_PR_TIME
 {"restore", MCLASS_PR, 1, METHOD_COST_CAT_HIGH, MEXT_INTERNAL, 1, 4, {"rate"}}, // MTYPE_PR_RESTORE
 {"redundancy", MCLASS_PR, 1, METHOD_COST_CAT_MIN, MEXT_INTERNAL, 1, 4, {"extra"}}, // MTYPE_PR_REDUNDANCY
    // Note: redundancy's costs are determined specially in derive_proc_properties_from_bcode
 {"broadcast", MCLASS_PR, 1, METHOD_COST_CAT_MIN, MEXT_EXTERNAL_NO_ANGLE, 1, 4, {"range"}}, // MTYPE_PR_BROADCAST
 {"listen", MCLASS_PR, 1, METHOD_COST_CAT_MIN, MEXT_INTERNAL, 0, 0, {}}, // MTYPE_PR_LISTEN
 {"yield", MCLASS_PR, 1, METHOD_COST_CAT_MED, MEXT_EXTERNAL_NO_ANGLE, 2, 4, {"range", "rate"}}, // MTYPE_PR_YIELD
 {"storage", MCLASS_PR, 1, METHOD_COST_CAT_MED, MEXT_INTERNAL, 1, 4, {"capacity"}}, // MTYPE_PR_STORAGE
 {"static", MCLASS_PR, 1, METHOD_COST_CAT_MIN, MEXT_EXTERNAL_NO_ANGLE, 0, 0, {}}, // MTYPE_PR_STATIC

// pr_std will combine info, time, action, output, command set (not receive)
// clob_std? maybe, maybe not.

// some special methods allowing interaction with client (may be made optional):
 {"command_pr", MCLASS_PR, 1, METHOD_COST_CAT_MIN, MEXT_INTERNAL, 0, 0, {}}, // MTYPE_PR_COMMAND, // fields: mode (read/write), command index, value (if write) (returns value, or success/fail)
// {"action", MCLASS_PR, 1, METHOD_COST_CAT_MIN, MEXT_INTERNAL, 0, 0, {}}, // MTYPE_PR_ACTION

 {"virtual", MCLASS_PR, 1, METHOD_COST_CAT_ULTRA, MEXT_INTERNAL, 2, 4, {"capacity", "charge"}}, // MTYPE_PR_VIRTUAL
// {"output", MCLASS_PR, 1, METHOD_COST_CAT_MIN, MEXT_INTERNAL, 0, 0, {}}, // MTYPE_PR_OUTPUT

#define NON_PROCESS_METHOD_DETAILS METHOD_COST_CAT_NONE,MEXT_INTERNAL,0,0,{}

// client methods
 {"com_give", MCLASS_CL, 1, NON_PROCESS_METHOD_DETAILS}, // MTYPE_CL_COMMAND_GIVE,
 {"template_cl", MCLASS_CL, 1, NON_PROCESS_METHOD_DETAILS}, // MTYPE_CL_TEMPLATE,
 {"output_cl", MCLASS_CL, 1, NON_PROCESS_METHOD_DETAILS}, // MTYPE_CL_OUTPUT,

// client/observer methods
 {"check point", MCLASS_CLOB, 1, NON_PROCESS_METHOD_DETAILS}, // MTYPE_CLOB_POINT
 {"query", MCLASS_CLOB, 1, NON_PROCESS_METHOD_DETAILS}, // MTYPE_CLOB_QUERY
 {"scan_clob", MCLASS_CLOB, 3, NON_PROCESS_METHOD_DETAILS}, // MTYPE_CLOB_SCAN
 {"maths_clob", MCLASS_CLOB, 1, NON_PROCESS_METHOD_DETAILS}, // MTYPE_CLOB_MATHS
 {"std_clob", MCLASS_CLOB, 1, NON_PROCESS_METHOD_DETAILS}, // MTYPE_CLOB_STD
 {"com_rec", MCLASS_CLOB, 1, NON_PROCESS_METHOD_DETAILS}, // MTYPE_CLOB_COMMAND_REC

// observer methods
 {"input", MCLASS_OB, 1, NON_PROCESS_METHOD_DETAILS}, // MTYPE_OB_INPUT
 {"view", MCLASS_OB, 2, NON_PROCESS_METHOD_DETAILS}, // MTYPE_OB_VIEW
 {"console", MCLASS_OB, 1, NON_PROCESS_METHOD_DETAILS}, // MTYPE_OB_CONSOLE
 {"select", MCLASS_OB, 1, NON_PROCESS_METHOD_DETAILS}, // MTYPE_OB_SELECT
 {"control", MCLASS_OB, 1, NON_PROCESS_METHOD_DETAILS}, // MTYPE_OB_CONTROL

// system methods
 {"place process", MCLASS_SY, 2, NON_PROCESS_METHOD_DETAILS}, // MTYPE_SY_PLACE_PROC
 {"template_sy", MCLASS_SY, 1, NON_PROCESS_METHOD_DETAILS}, // MTYPE_SY_TEMPLATE
 {"change process", MCLASS_SY, 1, NON_PROCESS_METHOD_DETAILS}, // MTYPE_SY_CHANGE_PROC
// {"connection", MCLASS_SY, 1, NON_PROCESS_METHOD_DETAILS}, // MTYPE_SY_CONNECTION
// {"destroy process", MCLASS_SY, 1, NON_PROCESS_METHOD_DETAILS}, // MTYPE_SY_DESTROY_PROC
 {"manage", MCLASS_SY, 2, NON_PROCESS_METHOD_DETAILS}, // MTYPE_SY_MANAGE

// general (MTYPE_END terminates a proc/program's list of methods)
 {"(end)", MCLASS_END, 1, NON_PROCESS_METHOD_DETAILS}, // MTYPE_END
 {"(sub-method)", MCLASS_ALL, 1, NON_PROCESS_METHOD_DETAILS}, // MTYPE_SUB
 {"(invalid)", MCLASS_ERROR, 1, NON_PROCESS_METHOD_DETAILS}, // MTYPE_ERROR_INVALID
 {"(vertex error)", MCLASS_ERROR, 1, NON_PROCESS_METHOD_DETAILS}, // MTYPE_ERROR_VERTEX
 {"(mass error)", MCLASS_ERROR, 1, NON_PROCESS_METHOD_DETAILS}, // MTYPE_ERROR_MASS
 {"(duplicate)", MCLASS_ERROR, 1, NON_PROCESS_METHOD_DETAILS}, // MTYPE_ERROR_DUPLICATE
 {"(forbidden)", MCLASS_ERROR, 1, NON_PROCESS_METHOD_DETAILS}, // MTYPE_ERROR_SUB

};





static void rotate_directional_method(int* data_angle, al_fixed* ex_angle, s16b mbank_angle, int turn_speed, int shape, int vertex);


/*
This is the pass through each proc's methods that occurs each tick.
In some places it assumes that the pass that occurs after each proc's code execution (in active_method_pass_after_execution()) does bounds-checking.


*/
void active_method_pass_each_tick(void)
{

 int p;
 int m;
 int mbase;
 struct procstruct* pr;
 int irpt_cost;

 for (p = 0; p < w.max_procs; p ++)
 {
  if (w.proc[p].exists <= 0)
   continue;
  pr = &w.proc[p];
  for (m = 0; m < METHODS; m ++)
  {
   switch(pr->method [m].type)
   {
    case MTYPE_END:
     m = METHODS;
     break;
    case MTYPE_PR_MOVE:
// this code runs the accel method each tick. Code in active_method_pass_after_execution() runs one after execution to set things up.
     mbase = m * METHOD_SIZE;
//     fprintf(stdout, "\npr %i delay %i counter %i rate %i}", pr->index, pr->regs.mbank [mbase + MBANK_PR_ACCEL_DELAY], pr->regs.mbank [mbase + MBANK_PR_ACCEL_COUNTER], pr->regs.mbank [mbase + MBANK_PR_ACCEL_RATE]);
     if (pr->method [m].data [MDATA_PR_MOVE_FIRING] > 0) // used by display to fade out flame after firing stops
      pr->method [m].data [MDATA_PR_MOVE_FIRING] --;
     if (pr->regs.mbank [mbase + MBANK_PR_MOVE_DELAY] > 0)
     {
      pr->regs.mbank [mbase + MBANK_PR_MOVE_DELAY] --;
      break;
     }
     int accel_rate = pr->regs.mbank [mbase + MBANK_PR_MOVE_RATE];
     if (pr->regs.mbank [mbase + MBANK_PR_MOVE_COUNTER] > 0
      && accel_rate > 0)
     {
      if (accel_rate > pr->method [m].data [MDATA_PR_MOVE_SETTING_MAX])
       accel_rate = pr->method [m].data [MDATA_PR_MOVE_SETTING_MAX];
      irpt_cost = accel_rate * pr->method [m].data [MDATA_PR_MOVE_IRPT_COST];// * pr->special_method_penalty;
//      fprintf(stdout, "\nirpt cost %i (ar %i, cost %i)", irpt_cost, accel_rate, pr->method [m].data [MDATA_PR_ACCEL_IRPT_COST]);
      if (irpt_cost > *(pr->irpt))
       break;
      *(pr->irpt) -= irpt_cost;
      pr->regs.mbank [mbase + MBANK_PR_MOVE_COUNTER] --;
// MDATA_PR_ACCEL_FIRING is verified in verify_method_data in f_load.c. Check there if changed here.
//      pr->method [m].data [MDATA_PR_MOVE_FIRING] = 8;
      pr->method [m].data [MDATA_PR_MOVE_FIRING] += 2;
      if (pr->method [m].data [MDATA_PR_MOVE_FIRING] > 8)
							pr->method [m].data [MDATA_PR_MOVE_FIRING] = 8;
      pr->method [m].data [MDATA_PR_MOVE_ACTUAL_RATE] = accel_rate;
      if (pr->group == -1)
       apply_impulse_to_proc_at_vertex(pr, pr->method[m].ex_vertex, al_itofix(accel_rate) / 2, pr->angle + pr->method[m].ex_angle + AFX_ANGLE_2);
        else
         apply_impulse_to_group_at_member_vertex(&w.group [pr->group], pr, pr->method[m].ex_vertex, al_itofix(accel_rate) / 2, pr->angle + pr->method[m].ex_angle + AFX_ANGLE_2);

      break;
     }
     break; // end MTYPE_PR_MOVE
    case MTYPE_PR_PACKET:
     mbase = m * METHOD_SIZE;
     if (pr->method[m].data [MDATA_PR_PACKET_RECYCLE] > 0)
      pr->method[m].data [MDATA_PR_PACKET_RECYCLE] --;
     if (pr->regs.mbank [mbase + MBANK_PR_PACKET_COUNTER] > 0)
     {
      pr->regs.mbank [mbase + MBANK_PR_PACKET_COUNTER] --;
      if (pr->regs.mbank [mbase + MBANK_PR_PACKET_COUNTER] == 0)
      {
       if (pr->method[m].data [MDATA_PR_PACKET_RECYCLE] == 0)
       {
        irpt_cost = pr->method[m].data [MDATA_PR_PACKET_IRPT_COST];// * pr->special_method_penalty;
        if (*(pr->irpt) >= irpt_cost)
        {
         *(pr->irpt) -= irpt_cost;
         run_packet_method(pr, m);
//         pr->regs.mbank [mbase + MBANK_PR_PACKET_COUNTER] = run_packet_method(pr, m);
        }
       }
        else
         pr->regs.mbank [mbase + MBANK_PR_PACKET_COUNTER] = 1; // if COUNTER is zero but RECYCLE isn't yet, try again next tick.
      }
     }
     break; // end MTYPE_PR_DPACKET
    case MTYPE_PR_DPACKET:
     mbase = m * METHOD_SIZE;
//     if (pr->index == 101)
//      fprintf(stdout, "\nm %i data angle %i angle_min %i angle_max %i mbank %i", m, pr->method[m].data [MDATA_PR_DPACKET_ANGLE], pr->method[m].data [MDATA_PR_DPACKET_ANGLE_MIN],
//            pr->method[m].data [MDATA_PR_DPACKET_ANGLE_MAX], pr->regs.mbank [mbase + MBANK_PR_DPACKET_ANGLE]);
//#define DPACKET_TURN_SPEED 48
/*
     if (pr->method[m].data [MDATA_PR_DPACKET_ANGLE] != pr->regs.mbank [mbase + MBANK_PR_DPACKET_ANGLE])
     {
//      if (pr->regs.mbank [mbase + MBANK_PR_DPACKET_ANGLE] < pr->method[m].data [MDATA_PR_DPACKET_ANGLE])
      int angle_turn = delta_turn_towards_angle(pr->method[m].data [MDATA_PR_DPACKET_ANGLE] & ANGLE_MASK, pr->regs.mbank [mbase + MBANK_PR_DPACKET_ANGLE] & ANGLE_MASK, 1);
      if (angle_turn == 1)
      {
       pr->method[m].data [MDATA_PR_DPACKET_ANGLE] += DPACKET_TURN_SPEED;
// Find out if the turret has turned past its target:
//       if (pr->regs.mbank [mbase + MBANK_PR_DPACKET_ANGLE] > pr->method[m].data [MDATA_PR_DPACKET_ANGLE])
//        pr->method[m].data [MDATA_PR_DPACKET_ANGLE] = pr->regs.mbank [mbase + MBANK_PR_DPACKET_ANGLE];
       if (pr->method[m].data [MDATA_PR_DPACKET_ANGLE] > pr->method[m].data [MDATA_PR_DPACKET_ANGLE_MAX])
        pr->method[m].data [MDATA_PR_DPACKET_ANGLE] = pr->method[m].data [MDATA_PR_DPACKET_ANGLE_MAX];
       pr->method[m].ex_angle = (int_angle_to_fixed(pr->method[m].data [MDATA_PR_DPACKET_ANGLE]) + pr->shape_str->vertex_angle [pr->method[m].ex_vertex]) & AFX_MASK;
      }
       else
       {
//        if (pr->regs.mbank [mbase + MBANK_PR_DPACKET_ANGLE] > pr->method[m].data [MDATA_PR_DPACKET_ANGLE])
        if (angle_turn == -1)
        {
         pr->method[m].data [MDATA_PR_DPACKET_ANGLE] -= DPACKET_TURN_SPEED;
// Find out if the turret has turned past its target:
//         if (pr->regs.mbank [mbase + MBANK_PR_DPACKET_ANGLE] < pr->method[m].data [MDATA_PR_DPACKET_ANGLE])
//          pr->method[m].data [MDATA_PR_DPACKET_ANGLE] = pr->regs.mbank [mbase + MBANK_PR_DPACKET_ANGLE];
//         if (pr->method[m].data [MDATA_PR_DPACKET_ANGLE] > pr->method[m].data [MDATA_PR_DPACKET_ANGLE_MAX])
//          pr->method[m].data [MDATA_PR_DPACKET_ANGLE] = pr->method[m].data [MDATA_PR_DPACKET_ANGLE_MAX];
         if (pr->method[m].data [MDATA_PR_DPACKET_ANGLE] < pr->method[m].data [MDATA_PR_DPACKET_ANGLE_MIN])
          pr->method[m].data [MDATA_PR_DPACKET_ANGLE] = pr->method[m].data [MDATA_PR_DPACKET_ANGLE_MIN];
         pr->method[m].ex_angle = (int_angle_to_fixed(pr->method[m].data [MDATA_PR_DPACKET_ANGLE]) + pr->shape_str->vertex_angle [pr->method[m].ex_vertex]) & AFX_MASK;
        }
       }
     }*/


/*
     if (pr->method[m].data [MDATA_PR_DPACKET_ANGLE] != pr->regs.mbank [mbase + MBANK_PR_DPACKET_ANGLE])
     {
      if (pr->regs.mbank [mbase + MBANK_PR_DPACKET_ANGLE] < pr->method[m].data [MDATA_PR_DPACKET_ANGLE])
//      int angle_turn = delta_turn_towards_angle(pr->method[m].data [MDATA_PR_DPACKET_ANGLE], pr->regs.mbank [mbase + MBANK_PR_DPACKET_ANGLE], 1);
//      if (angle_turn == 1)
      {
       pr->method[m].data [MDATA_PR_DPACKET_ANGLE] -= DPACKET_TURN_SPEED;
// Find out if the turret has turned past its target:
       if (pr->regs.mbank [mbase + MBANK_PR_DPACKET_ANGLE] > pr->method[m].data [MDATA_PR_DPACKET_ANGLE])
        pr->method[m].data [MDATA_PR_DPACKET_ANGLE] = pr->regs.mbank [mbase + MBANK_PR_DPACKET_ANGLE];
//       if (pr->method[m].data [MDATA_PR_DPACKET_ANGLE] < pr->method[m].data [MDATA_PR_DPACKET_ANGLE_MIN])
//        pr->method[m].data [MDATA_PR_DPACKET_ANGLE] = pr->method[m].data [MDATA_PR_DPACKET_ANGLE_MIN];
       if (pr->method[m].data [MDATA_PR_DPACKET_ANGLE] < pr->shape_str->vertex_method_angle_min [pr->method[m].ex_vertex])
        pr->method[m].data [MDATA_PR_DPACKET_ANGLE] = pr->shape_str->vertex_method_angle_min [pr->method[m].ex_vertex];
       pr->method[m].ex_angle = (short_angle_to_fixed(pr->method[m].data [MDATA_PR_DPACKET_ANGLE]) + pr->shape_str->vertex_angle [pr->method[m].ex_vertex]) & AFX_MASK;
      }
       else
       {
        if (pr->regs.mbank [mbase + MBANK_PR_DPACKET_ANGLE] > pr->method[m].data [MDATA_PR_DPACKET_ANGLE])
//        if (angle_turn == -1)
        {
         pr->method[m].data [MDATA_PR_DPACKET_ANGLE] += DPACKET_TURN_SPEED;
// Find out if the turret has turned past its target:
         if (pr->regs.mbank [mbase + MBANK_PR_DPACKET_ANGLE] < pr->method[m].data [MDATA_PR_DPACKET_ANGLE])
          pr->method[m].data [MDATA_PR_DPACKET_ANGLE] = pr->regs.mbank [mbase + MBANK_PR_DPACKET_ANGLE];
         if (pr->method[m].data [MDATA_PR_DPACKET_ANGLE] > pr->shape_str->vertex_method_angle_max [pr->method[m].ex_vertex])
          pr->method[m].data [MDATA_PR_DPACKET_ANGLE] = pr->shape_str->vertex_method_angle_max [pr->method[m].ex_vertex];
//         if (pr->method[m].data [MDATA_PR_DPACKET_ANGLE] > pr->method[m].data [MDATA_PR_DPACKET_ANGLE_MAX])
//          pr->method[m].data [MDATA_PR_DPACKET_ANGLE] = pr->method[m].data [MDATA_PR_DPACKET_ANGLE_MAX];
         pr->method[m].ex_angle = (short_angle_to_fixed(pr->method[m].data [MDATA_PR_DPACKET_ANGLE]) + pr->shape_str->vertex_angle [pr->method[m].ex_vertex]) & AFX_MASK;
        }
       }
     }*/

     rotate_directional_method(&pr->method[m].data [MDATA_PR_DPACKET_ANGLE],
                               &pr->method[m].ex_angle,
                               pr->regs.mbank [mbase + MBANK_PR_DPACKET_ANGLE],
                               48, // turn speed
                               pr->shape,
                               pr->method[m].ex_vertex);

     if (pr->method[m].data [MDATA_PR_DPACKET_RECYCLE] > 0)
      pr->method[m].data [MDATA_PR_DPACKET_RECYCLE] --;
     if (pr->regs.mbank [mbase + MBANK_PR_DPACKET_COUNTER] > 0)
     {
      pr->regs.mbank [mbase + MBANK_PR_DPACKET_COUNTER] --;
      if (pr->regs.mbank [mbase + MBANK_PR_DPACKET_COUNTER] == 0)
      {
       if (pr->method[m].data [MDATA_PR_DPACKET_RECYCLE] == 0)
       {
        irpt_cost = pr->method[m].data [MDATA_PR_DPACKET_IRPT_COST];// * pr->special_method_penalty;
        if (*(pr->irpt) >= irpt_cost)
        {
         *(pr->irpt) -= irpt_cost;
         run_dpacket_method(pr, m);
//         pr->regs.mbank [mbase + MBANK_PR_PACKET_COUNTER] = run_packet_method(pr, m);
        }
       }
        else
         pr->regs.mbank [mbase + MBANK_PR_DPACKET_COUNTER] = 1; // if COUNTER is zero but RECYCLE isn't yet, try again next tick.
      }
     }

     break; // end MTYPE_PR_PACKET

    case MTYPE_PR_STREAM:
     if (pr->method[m].data [MDATA_PR_STREAM_RECYCLE] > 0)
      pr->method[m].data [MDATA_PR_STREAM_RECYCLE] --;
     if (pr->method[m].data [MDATA_PR_STREAM_STATUS] != 0)
      run_stream_method(pr, m, MTYPE_PR_STREAM);
     break;

    case MTYPE_PR_DSTREAM:
     mbase = m * METHOD_SIZE;
     if (pr->method[m].data [MDATA_PR_STREAM_STATUS] == STREAM_STATUS_INACTIVE)
      rotate_directional_method(&pr->method[m].data [MDATA_PR_DSTREAM_ANGLE],
                               &pr->method[m].ex_angle,
                               pr->regs.mbank [mbase + MBANK_PR_DSTREAM_ANGLE],
                               32, // turn speed
                               pr->shape,
                               pr->method[m].ex_vertex);
     if (pr->method[m].data [MDATA_PR_STREAM_RECYCLE] > 0)
      pr->method[m].data [MDATA_PR_STREAM_RECYCLE] --;
     if (pr->method[m].data [MDATA_PR_STREAM_STATUS] != 0)
      run_stream_method(pr, m, MTYPE_PR_DSTREAM);
     break;


    case MTYPE_PR_VIRTUAL:
     if (pr->method[m].data [MDATA_PR_VIRTUAL_STATE] < 0)
      pr->method[m].data [MDATA_PR_VIRTUAL_STATE] ++;
     if (pr->method[m].data [MDATA_PR_VIRTUAL_PULSE] > 0)
      pr->method[m].data [MDATA_PR_VIRTUAL_PULSE] --;
     if (pr->method[m].data [MDATA_PR_VIRTUAL_CHARGE_PULSE] > 0)
      pr->method[m].data [MDATA_PR_VIRTUAL_CHARGE_PULSE] --;
     break;

   }

// note: at this point m may be out of bounds, so don't use it to index pr->methods

  }



 }



}

// returns method's angle as a result of change
static void rotate_directional_method(int* data_angle, al_fixed* ex_angle, s16b mbank_angle, int turn_speed, int shape, int vertex)
{
      if (mbank_angle < *data_angle)
      {
       *data_angle -= turn_speed;
// Find out if the turret has turned past its target:
       if (mbank_angle > *data_angle)
        *data_angle = mbank_angle;
       if (*data_angle < shape_dat[shape][0].vertex_method_angle_min [vertex])
        *data_angle = shape_dat[shape][0].vertex_method_angle_min [vertex];
       *ex_angle = (short_angle_to_fixed(*data_angle) + shape_dat[shape][0].vertex_angle [vertex]) & AFX_MASK;
      }
       else
       {
        if (mbank_angle > *data_angle)
        {
         *data_angle += turn_speed;
// Find out if the turret has turned past its target:
         if (mbank_angle < *data_angle)
          *data_angle = mbank_angle;
         if (*data_angle > shape_dat[shape][0].vertex_method_angle_max [vertex])
          *data_angle = shape_dat[shape][0].vertex_method_angle_max [vertex];
         *ex_angle = (short_angle_to_fixed(*data_angle) + shape_dat[shape][0].vertex_angle [vertex]) & AFX_MASK;
        }
       }

}


// This function is called after a proc's code is executed.
// It runs some methods, and sets up other methods based on values put into the mbank during execution
// The methods may then do things during active_method_pass_each_tick
void active_method_pass_after_execution(struct procstruct* pr)
{

 int m;
 int mbase; // if this gives a "may be used uninitialised" warning it's probably because I've left out the mbase = m * METHOD_SIZE line from a case below.

// pr->special_method_penalty = 1;

 for (m = 0; m < METHODS; m ++)
 {
   switch(pr->method [m].type)
   {
    case MTYPE_END:
     m = METHODS;
     break;
    case MTYPE_PR_MOVE:
     mbase = m * METHOD_SIZE;
     if (pr->regs.mbank [mbase + MBANK_PR_MOVE_RATE] > pr->method [m].data [MDATA_PR_MOVE_SETTING_MAX])
     {
      pr->regs.mbank [mbase + MBANK_PR_MOVE_RATE] = pr->method [m].data [MDATA_PR_MOVE_SETTING_MAX];
     }
     if (pr->regs.mbank [mbase + MBANK_PR_MOVE_RATE] < 0)
     {
      pr->regs.mbank [mbase + MBANK_PR_MOVE_RATE] = 0;//pr->method [m].data [(m * METHOD_HIDDEN_VALUES) + MHIDE_MOVE_RATE_MAX];
     }
     break;
//    case MTYPE_PR_IRPT:
//     mbase = m * METHOD_SIZE;
//     pr->regs.mbank [mbase + MBANK_PR_IRPT_LEFT] = pr->method[m].data [MDATA_PR_IRPT_LEFT];
//     pr->method[m].data [MDATA_PR_IRPT_LEFT] = pr->method[m].data [MDATA_PR_IRPT_MAX];
//     break;
    case MTYPE_PR_ALLOCATE:
     mbase = m * METHOD_SIZE;
     pr->method[m].data [MDATA_PR_ALLOCATE_COUNTER] += pr->method[m].data [MDATA_PR_ALLOCATE_EFFICIENCY];
     if (pr->method[m].data [MDATA_PR_ALLOCATE_COUNTER] > 100)
      pr->method[m].data [MDATA_PR_ALLOCATE_COUNTER] = 100;
     break;
    case MTYPE_PR_LINK:
     pr->method[m].data [MDATA_PR_LINK_RECEIVED] = 0; // no message received
     break;
    case MTYPE_PR_DESIGNATE:
// if designated proc has been destroyed, remove designation (pr is not informed of this)
     if (pr->method [m].data [MDATA_PR_DESIGNATE_INDEX] != -1
      && w.proc[pr->method [m].data [MDATA_PR_DESIGNATE_INDEX]].deallocating != 0) // deallocation period should be guaranteed to be long enough (I think it's 32) so all designate methods reset
       pr->method [m].data [MDATA_PR_DESIGNATE_INDEX] = -1;
     break;
    case MTYPE_PR_RESTORE:
     pr->method [m].data [MDATA_PR_RESTORE_USED] = 0; // so it can be used again next tick
     break;
    case MTYPE_PR_LISTEN:
     pr->method[m].data [MDATA_PR_LISTEN_RECEIVED] = 0; // reset so that method can start listening again in the new cycle.
     break;
    case MTYPE_PR_YIELD:
     pr->method[m].data [MDATA_PR_YIELD_ACTIVATED] = 0; // reset so method can activate again next tick
     break;
    case MTYPE_PR_STREAM:
    case MTYPE_PR_DSTREAM:
     mbase = m * METHOD_SIZE;
     if (pr->method[m].data [MDATA_PR_STREAM_RECYCLE] > 0)
      break;
     if (pr->regs.mbank [mbase + MBANK_PR_STREAM_FIRE] > 0)
//      && pr->regs.mbank [mbase + MBANK_PR_STREAM_TIME] > 0)
      run_stream_method(pr, m, pr->method [m].type);
     break;
    case MTYPE_PR_VIRTUAL:
     if (pr->method[m].data [MDATA_PR_VIRTUAL_STATE] > 0)
     {
      int virtual_active_cost = 64 + ((pr->method[m].extension [0] + pr->method[m].extension [1]) * 32);
      if (*(pr->irpt) < virtual_active_cost)
      {
       virtual_method_break(pr);
      }
       else
       {
        *(pr->irpt) -= virtual_active_cost;
//        pr->special_method_penalty ++; // Remember that this is reset each cycle, so this increment just sets it up for the current cycle
       }
     }
     break;



// when adding a new entry, remember to calculate mbase! (this is done inside the cases because for some method types it is not needed)



// remember that these are active methods!! called methods go elsewhere

/*
So, how is the new method going to work?
We'll probably need 8 address in the mbank.
How about:
0 - status
1 - parent vertex
2 - child vertex
3 - angle
4 - start address
5 - end address

statuses:
0 - do nothing
1 - active (i.e. run immediately after execution)
2 - test
3 - successfully created new proc
4 - successful test
5 - failed: obstacle
6 - failed: insufficient irpt
7 - failed: insufficient data
8 - failed: start address out of bounds
9 - failed: end address out of bounds

The method should derive the child's shape and size from the start_address plus 4 and 5 or whatever.


*/


   }

// note: at this point m may be out of bounds, so don't use it to index pr->methods

 }

}

/*
This function is called whenever a proc or client program calls one of its methods during execution.
Only certain methods work this way.
Return value is the return value of the particular method called.
 - or is zero if there is some kind of error
 - although -1 is the error code for run_scan, at least so far. Hm. TO DO: think about how to handle errors.

Does not assume that m is a valid method
For large methods, assumes that there is enough space in the mbank for any references to be valid (this should have been checked when the method type was derived from the interface definition)

If this function is called by a client program, pr will be NULL. pr should only be dereferenced for proc methods and never for client methods.
Similarly, if this function is called by a proc program, cl will be NULL.
*/
//s16b call_method(struct procstruct* pr, struct programstruct* cl, int m, struct methodstruct methods [METHODS + 1], int* instr_left)
s16b call_method(struct procstruct* pr, struct programstruct* cl, int m, struct methodstruct* methods, int* instr_left)
{

 int* irpt_left;
 int t, i, p; // template (used for new process methods)

 if (cl == NULL)
  irpt_left = pr->irpt; // both are pointers
   else
    irpt_left = &cl->irpt;

// note that the cost value is used twice in this macro
#define MCOST(cost,fail_result) if (*irpt_left < cost) return fail_result; *irpt_left -= cost;

 if (m < 0 || m >= METHODS)
  return 0;

 int type = methods[m].type;
// int mbase = m * METHOD_SIZE;
 s16b* mb;
 int proc_index, command_index;

 if (cl == NULL)
  mb = &pr->regs.mbank [m*METHOD_SIZE];
   else
    mb = &cl->regs.mbank [m*METHOD_SIZE];

 switch(type)
 {
  case MTYPE_PR_SCAN:
// see also MTYPE_CLOB_SCAN below
   ;

// Before calling the scanning functions, the method does some bounds checking.
// Note that nothing is bounds-checked against the edge of the map. That is done for each block (because a scan that partly covers an out-of-bounds area is still valid for the in-bounds area)

   int scan_size = mb [MBANK_PR_SCAN_SIZE];

// Check the size of the scan (which is a square around the x/y point):
   if (scan_size > methods[m].data [MDATA_PR_SCAN_RANGE])
    scan_size = methods[m].data [MDATA_PR_SCAN_RANGE];

// Can't have a scan of negative size.
// Might as well check this for examine calls even though scan size isn't relevant for examine.
   if (scan_size < 0)
    return MRES_SCAN_FAIL_SIZE;

// Make sure the centre of the scan is not out of range:
   if (mb [MBANK_PR_SCAN_X1] >= methods[m].data [MDATA_PR_SCAN_RANGE]
    || mb [MBANK_PR_SCAN_X1] <= (0 - methods[m].data [MDATA_PR_SCAN_RANGE])
    || mb [MBANK_PR_SCAN_Y1] >= methods[m].data [MDATA_PR_SCAN_RANGE]
    || mb [MBANK_PR_SCAN_Y1] <= (0 - methods[m].data [MDATA_PR_SCAN_RANGE]))
     return MRES_SCAN_FAIL_RANGE;

// If the given size would result in the scan being partly out of range, it shrinks so that it
//  is still a square covering an area around the centre point that is entirely in range.
// First check x:
   if (mb [MBANK_PR_SCAN_X1] + scan_size > methods[m].data [MDATA_PR_SCAN_RANGE])
    scan_size = methods[m].data [MDATA_PR_SCAN_RANGE] - mb [MBANK_PR_SCAN_X1];
     else
     {
      if (mb [MBANK_PR_SCAN_X1] - scan_size < (0 - methods[m].data [MDATA_PR_SCAN_RANGE]))
       scan_size = methods[m].data [MDATA_PR_SCAN_RANGE] + mb [MBANK_PR_SCAN_X1];
     }
// then check y. If the scan centre is near a corner, both x and y limits may be applied (resulting in the smallest valid square)
   if (mb [MBANK_PR_SCAN_Y1] + scan_size > methods[m].data [MDATA_PR_SCAN_RANGE])
    scan_size = methods[m].data [MDATA_PR_SCAN_RANGE] - mb [MBANK_PR_SCAN_X1];
     else
     {
      if (mb [MBANK_PR_SCAN_Y1] - scan_size < (0 - methods[m].data [MDATA_PR_SCAN_RANGE]))
       scan_size = methods[m].data [MDATA_PR_SCAN_RANGE] + mb [MBANK_PR_SCAN_X1];
     }

   al_fixed fix_scan_centre_x = pr->x + al_itofix(mb [MBANK_PR_SCAN_X1]);
   al_fixed fix_scan_centre_y = pr->y + al_itofix(mb [MBANK_PR_SCAN_Y1]);

   int scan_cost, max_number;

   if (mb [0] == MSTATUS_PR_SCAN_SCAN)
   {

    max_number = mb [MBANK_PR_SCAN_NUMBER];
    if (max_number <= 0)
     return MRES_SCAN_FAIL_NUMBER;
    if (max_number > SCANLIST_MAX_SIZE - 1)
     max_number = SCANLIST_MAX_SIZE - 1;

    scan_cost = 64 + (max_number * 2) + (scan_size / 8);

    MCOST(scan_cost, MRES_SCAN_FAIL_IRPT);

    return run_scan_square_spiral(&pr->bcode,
                           pr,
                           al_fixtoi(fix_scan_centre_x), al_fixtoi(fix_scan_centre_y), scan_size,
                           fix_scan_centre_x, fix_scan_centre_y, al_itofix(scan_size),
                           max_number,
                           mb,
                           SCAN_RESULTS_XY);

// return value of run_scan is the number of procs found (or a negative MRES_SCAN_* result if error)
   }
   if (mb [0] == MSTATUS_PR_SCAN_EXAMINE)
   {
//   fprintf(stdout, "\nscan centre offsets %i,%i", mb [MBANK_PR_SCAN_CENTRE_X], mb [MBANK_PR_SCAN_CENTRE_Y]);

    MCOST(48, MRES_SCAN_FAIL_IRPT);

//    fprintf(stdout, "\nMethod: pr->x = %i pr->y = %i scan_centre_x_offset = %i scan_centre_y_offset = %i, to (%i, %i)", pr->x, pr->y, scan_centre_x_offset, scan_centre_y_offset, (pr->x + scan_centre_x_offset) / GRAIN_MULTIPLY, (pr->y + scan_centre_y_offset) / GRAIN_MULTIPLY);
    return run_examine(&pr->bcode,
                   fix_scan_centre_x, // centre of scan
                   fix_scan_centre_y,
                   pr->x,
                   pr->y,
                   mb);
// return value of run_examine is 1 if proc found at point, 0 otherwise
   }
// note that procs can't use the modes that give indices (they can only get x/y values)
   return MRES_SCAN_FAIL_STATUS; // probably an invalid status

  case MTYPE_PR_DPACKET: // calling dpacket just returns its current angle. Calling normal PACKET method does nothing.
   return methods[m].data [MDATA_PR_DPACKET_ANGLE];

  case MTYPE_PR_STREAM:
   if (methods[m].data [MDATA_PR_STREAM_STATUS] == STREAM_STATUS_INACTIVE)
    return methods[m].data [MDATA_PR_STREAM_RECYCLE];
   return -1;

  case MTYPE_PR_DSTREAM: // not sure about this - how to allow access to status?
   return methods[m].data [MDATA_PR_DSTREAM_ANGLE];

  case MTYPE_PR_STD:
   MCOST(1,0);
   switch(mb [0])
   {
    case MSTATUS_PR_STD_GET_X:
     return al_fixtoi(pr->x);
    case MSTATUS_PR_STD_GET_Y:
     return al_fixtoi(pr->y);
    case MSTATUS_PR_STD_GET_SPEED_X:
     return al_fixtoi(pr->x_speed * SPEED_INT_ADJUSTMENT);
    case MSTATUS_PR_STD_GET_SPEED_Y:
     return al_fixtoi(pr->y_speed * SPEED_INT_ADJUSTMENT);
    case MSTATUS_PR_STD_GET_ANGLE:
     return fixed_angle_to_short(pr->angle);
    case MSTATUS_PR_STD_GET_HP:
     return pr->hp;
    case MSTATUS_PR_STD_GET_HP_MAX:
     return pr->hp_max;
    case MSTATUS_PR_STD_GET_INSTR:
     return *instr_left;
    case MSTATUS_PR_STD_GET_INSTR_MAX:
     return pr->instructions_each_execution;
    case MSTATUS_PR_STD_GET_IRPT:
     return *irpt_left;
    case MSTATUS_PR_STD_GET_IRPT_MAX:
     return *(pr->irpt_max);
    case MSTATUS_PR_STD_GET_OWN_IRPT_MAX:
					return pr->single_proc_irpt_max;
    case MSTATUS_PR_STD_GET_DATA:
     return *(pr->data);
    case MSTATUS_PR_STD_GET_DATA_MAX:
     return *(pr->data_max);
    case MSTATUS_PR_STD_GET_OWN_DATA_MAX:
					return pr->single_proc_data_max;
    case MSTATUS_PR_STD_GET_TEAM:
     return pr->player_index;
    case MSTATUS_PR_STD_GET_SPIN:
     if (pr->group == -1)
      return fixed_angle_to_short(pr->spin);
       else
        return fixed_angle_to_short(w.group[pr->group].spin);
    case MSTATUS_PR_STD_GET_GR_X:
     if (pr->group == -1)
      return al_fixtoi(pr->x);
       else
        return al_fixtoi(w.group[pr->group].x); // this should be the centre of mass of the group
    case MSTATUS_PR_STD_GET_GR_Y:
     if (pr->group == -1)
      return al_fixtoi(pr->y);
       else
        return al_fixtoi(w.group[pr->group].y);
/*    case MSTATUS_PR_STD_GET_GR_ANGLE:
     if (pr->group == -1)
      return fixed_angle_to_short(pr->angle);
       else
        return fixed_angle_to_short(w.group[pr->group].angle);*/
    case MSTATUS_PR_STD_GET_GR_SPEED_X:
     if (pr->group == -1)
      return al_fixtoi(pr->x_speed * SPEED_INT_ADJUSTMENT);
       else
        return al_fixtoi(w.group[pr->group].x_speed * SPEED_INT_ADJUSTMENT);
    case MSTATUS_PR_STD_GET_GR_SPEED_Y:
     if (pr->group == -1)
      return al_fixtoi(pr->y_speed * SPEED_INT_ADJUSTMENT);
       else
        return al_fixtoi(w.group[pr->group].y_speed * SPEED_INT_ADJUSTMENT);
    case MSTATUS_PR_STD_GET_GR_MEMBERS:
     if (pr->group == -1)
      return 1;
       else
        return w.group[pr->group].total_members;
    case MSTATUS_PR_STD_GET_EX_COUNT:
     return pr->execution_count;
// world-related information:
    case MSTATUS_PR_STD_GET_WORLD_W:
     return w.w_pixels;
    case MSTATUS_PR_STD_GET_WORLD_H:
     return w.h_pixels;
//    case MSTATUS_PR_STD_GET_GEN_LIMIT:
//     return w.gen_limit;
//    case MSTATUS_PR_STD_GET_GEN_NUMBER:
//     return w.player[pr->player_index].gen_number;
    case MSTATUS_PR_STD_GET_EFFICIENCY:
     if (pr->allocate_method != -1)
      return pr->method[pr->allocate_method].data [MDATA_PR_ALLOCATE_EFFICIENCY]; // no extra cost in this case
     MCOST(31,0); // total cost 32 - get_point_alloc_efficiency is relatively expensive
     return get_point_alloc_efficiency(pr->x, pr->y);
// when adding a get mode to PR_STD, may also need to add to CLOB_QUERY
    case MSTATUS_PR_STD_GET_TIME: // fills mb [1] and mb [2] with world time (mb [1] is time / 32767; mb [2] is time % 32767); returns value that will be in mb [2]
// this function is also available from the time method
     mb [1] = w.world_time / 32767;
     mb [2] = w.world_time % 32767;
     return mb [2];
// Note that the query version of the vertex-related info modes uses mb [2] instead of [1]
    case MSTATUS_PR_STD_GET_VERTICES:
					 return pr->shape_str->vertices;
				case MSTATUS_PR_STD_GET_VERTEX_ANGLE:
					 if (mb [1] < 0
							||	mb [1] >= pr->shape_str->vertices)
								return -1;
						return fixed_angle_to_short(pr->shape_str->vertex_angle [mb [1]]);
				case MSTATUS_PR_STD_GET_VERTEX_DIST:
					 if (mb [1] < 0
							||	mb [1] >= pr->shape_str->vertices)
								return -1;
						return pr->shape_str->vertex_dist_pixel [mb [1]];
				case MSTATUS_PR_STD_GET_VERTEX_ANGLE_PREV:
					 if (mb [1] < 0
							||	mb [1] >= pr->shape_str->vertices)
								return -1;
						return fixed_angle_to_short(pr->shape_str->external_angle [EXANGLE_PREVIOUS] [mb [1]]);
				case MSTATUS_PR_STD_GET_VERTEX_ANGLE_NEXT:
					 if (mb [1] < 0
							||	mb [1] >= pr->shape_str->vertices)
								return -1;
						return fixed_angle_to_short(pr->shape_str->external_angle [EXANGLE_NEXT] [mb [1]]);
				case MSTATUS_PR_STD_GET_VERTEX_ANGLE_MIN:
					 if (mb [1] < 0
							||	mb [1] >= pr->shape_str->vertices)
								return -1;
						return pr->shape_str->vertex_method_angle_min [mb [1]];
				case MSTATUS_PR_STD_GET_VERTEX_ANGLE_MAX:
					 if (mb [1] < 0
							||	mb [1] >= pr->shape_str->vertices)
								return -1;
						return pr->shape_str->vertex_method_angle_max [mb [1]];
				case MSTATUS_PR_STD_GET_METHOD:
					 if (mb [1] < 0
							||	mb [1] >= METHODS)
								return -1;
						return pr->method[mb[1]].type;
				case MSTATUS_PR_STD_GET_METHOD_FIND: // searches proc's methods for mtype of mb[1], starting from method slot mb[2]
// Could check that mb[1]	is a valid method type, but let's not bother
// But we do need to check that mb [2] is valid:
					 if (mb [2] < 0
							||	mb [2] >= METHODS)
								return -1;
						i = mb[2];
						while(i < METHODS)
						{
							if (pr->method[i].type == mb[1])
								return i;
							i++;
						};
						return -1;
				case MSTATUS_PR_STD_GET_MBANK: // really just for CLOB as for processes this is just a more expensive get_index() call
					return 0; // Probably not even worth implementing this

// non-get functions:
    case MSTATUS_PR_STD_ACTION:
// This sets w.print_proc_action_value3, which is attached as value3 to all following console lines printed by the proc (until called again or end of execution).
// w.print_proc_action_value3 is reset to 0 before each proc execution
     w.print_proc_action_value3 = mb [1];
     break;
    case MSTATUS_PR_STD_WAIT: // delays next execution of this proc by mb [1] (up to 2 * EXECUTION_COUNT; EC is currently 16)
//     fprintf(stdout, "\nproc %i method %i mb {%i,%i,%i,%i} time was %i ", pr->index, m, mb [0], mb [1], mb [2], mb [3], pr->execution_count);
     if (mb [1] > 0)
       pr->execution_count += mb [1];
     if (pr->execution_count > EXECUTION_COUNT * 2)
      pr->execution_count = EXECUTION_COUNT * 2;
//     fprintf(stdout, " now %i", pr->execution_count);
     return 1;
    case MSTATUS_PR_STD_COLOUR:
     if (mb[1] >= 0
      && mb[1] < PRINT_COLS)
      {
       w.print_colour = mb[1];
       return 1;
      }
     return 0;
    case MSTATUS_PR_STD_SET_COMMAND:
     if (mb [1] < 0 || mb [1] >= COMMANDS)
      return 0;
     MCOST(2,0);
     pr->command [mb [1]] = mb [2];
     return 1;
    case MSTATUS_PR_STD_COMMAND_BIT_0:
     if (mb [1] < 0 || mb [1] >= COMMANDS
						|| mb [2] < 0 || mb [2] >= 16)
      return 0;
     MCOST(1,0);
     pr->command [mb [1]] &= (0xffff ^ (1 << mb [2]));
     return 1;
    case MSTATUS_PR_STD_COMMAND_BIT_1:
     if (mb [1] < 0 || mb [1] >= COMMANDS
						|| mb [2] < 0 || mb [2] >= 16)
      return 0;
     MCOST(1,0);
     pr->command [mb [1]] |= (1 << mb [2]);
     return 1;
    case MSTATUS_PR_STD_PRINT_OUT:
					w.current_output_console = w.player[pr->player_index].output_console;
     return 1;
    case MSTATUS_PR_STD_PRINT_OUT2:
					w.current_output_console = w.player[pr->player_index].output2_console;
     return 1;
    case MSTATUS_PR_STD_PRINT_ERR:
					w.current_output_console = w.player[pr->player_index].error_console;
// Note that STD_PRINT_ERR results in normal print commands being treated as errors and displayed in the error console.
// It doesn't affect where errors generated by this process (e.g. out of bounds access) go.
     return 1;
    case MSTATUS_PR_STD_TEMPLATE_NAME:
// prints name of a template (belonging to this player)
     print_player_template_name(PROGRAM_TYPE_PROCESS, pr->index, pr->player_index, mb [1]);
					return 1;

    default: return 0;


   }
   return 0;

  case MTYPE_PR_MATHS:
  case MTYPE_CLOB_MATHS: // client/observer maths method works in exactly the same way
   switch(mb [MBANK_PR_MATHS_STATUS])
   {
    case MSTATUS_PR_MATHS_ATAN2:
     MCOST(12,0);
//     fprintf(stdout, "\natan2(%i, %i)", mb [MBANK_PR_MATHS_TERM1], mb [MBANK_PR_MATHS_TERM2]);
     return get_angle_int(mb [MBANK_PR_MATHS_TERM1], mb [MBANK_PR_MATHS_TERM2]);
    case MSTATUS_PR_MATHS_SIN:
     MCOST(4,0);
//     fprintf(stdout, "\nsin(%i) * %i", mb [MBANK_PR_MATHS_TERM1], mb [MBANK_PR_MATHS_TERM2]);
     return al_fixtoi(al_fixmul(al_fixsin(short_angle_to_fixed(mb [MBANK_PR_MATHS_TERM1])), al_itofix(mb [MBANK_PR_MATHS_TERM2])));
//     return fixed_angle_to_short(al_fixsin(short_angle_to_fixed(mb [MBANK_PR_MATHS_TERM1]))) * mb [MBANK_PR_MATHS_TERM2];
//     return ypart(mb [MBANK_PR_MATHS_TERM1], mb [MBANK_PR_MATHS_TERM2]);
    case MSTATUS_PR_MATHS_COS:
     MCOST(4,0);
//     fprintf(stdout, "\ncos(%i) * %i", mb [MBANK_PR_MATHS_TERM1], mb [MBANK_PR_MATHS_TERM2]);
     return al_fixtoi(al_fixmul(al_fixcos(short_angle_to_fixed(mb [MBANK_PR_MATHS_TERM1])), al_itofix(mb [MBANK_PR_MATHS_TERM2])));
//     return fixed_angle_to_short(al_fixsin(short_angle_to_fixed(mb [MBANK_PR_MATHS_TERM1]))) * mb [MBANK_PR_MATHS_TERM2];
//     return fixed_angle_to_short(al_fixcos(short_angle_to_fixed(mb [MBANK_PR_MATHS_TERM1]), short_angle_to_fixed(mb [MBANK_PR_MATHS_TERM2]));
//     return xpart(mb [MBANK_PR_MATHS_TERM1], mb [MBANK_PR_MATHS_TERM2]);
    case MSTATUS_PR_MATHS_HYPOT:
     MCOST(8,0);
//     return hypot(mb [MBANK_PR_MATHS_TERM2], mb [MBANK_PR_MATHS_TERM1]);
 //fprintf(stdout, "\nHypot: %i, %i result %i", mb [MBANK_PR_MATHS_TERM2], mb [MBANK_PR_MATHS_TERM1], al_fixhypot(al_itofix(mb [MBANK_PR_MATHS_TERM2]), al_itofix(mb [MBANK_PR_MATHS_TERM1])));
     return al_fixtoi(al_fixhypot(al_itofix(mb [MBANK_PR_MATHS_TERM2]), al_itofix(mb [MBANK_PR_MATHS_TERM1])));
    case MSTATUS_PR_MATHS_TURN_DIR: // TERM1 is angle, TERM2 is target angle
     MCOST(2,0);
// this code is based on delta_turn_towards_angle() in maths.c
     int angle1 = mb [MBANK_PR_MATHS_TERM1] & ANGLE_MASK;
     int angle2 = mb [MBANK_PR_MATHS_TERM2] & ANGLE_MASK;
     if ((angle1 < angle2 && angle2 > angle1 + ANGLE_2)
      || (angle1 > angle2 && angle2 > angle1 - ANGLE_2))
     {
      return -1;
     }
     if (angle1 != angle2)
     {
      return 1;
     }
     return 0;
    case MSTATUS_PR_MATHS_ANGLE_DIFF:
     MCOST(2,0);
     return angle_difference_int(mb [MBANK_PR_MATHS_TERM1], mb [MBANK_PR_MATHS_TERM2]);
    case MSTATUS_PR_MATHS_ANGLE_DIFF_S:
     MCOST(2,0);
     return angle_difference_signed(mb [MBANK_PR_MATHS_TERM1], mb [MBANK_PR_MATHS_TERM2]);
    case MSTATUS_PR_MATHS_ABS:
     MCOST(1,0);
     return abs(mb [MBANK_PR_MATHS_TERM1]);
    case MSTATUS_PR_MATHS_SQRT:
     MCOST(8,0);
     if (mb [MBANK_PR_MATHS_TERM1] <= 0)
      return 0;
     return al_fixtoi(al_fixsqrt(al_itofix(mb [MBANK_PR_MATHS_TERM1])));
//     return sqrt(mb [MBANK_PR_MATHS_TERM1]);
    case MSTATUS_PR_MATHS_POW:
     MCOST(4,0);
     return pow(mb [MBANK_PR_MATHS_TERM1], mb [MBANK_PR_MATHS_TERM1]);
/*    case MSTATUS_PR_MATHS_RANDOM:
     MCOST(4);
     if (mb [MBANK_PR_MATHS_TERM1] == 0)
      return 0;
     return 0; // rand() % mb [MBANK_PR_MATHS_TERM1]; not currently implemented. Note - don't use rand() as this will create consistency problems*/

// remember that client/observer programs use this code as well as procs - don't put any proc-specific stuff here

   default: return 0;
   }
   break; // end of case MTYPE_PR_MATHS and MTYPE_CLOB_MATHS

   case MTYPE_PR_IRPT:
    switch(mb [MBANK_PR_IRPT_STATUS])
    {
     case MSTATUS_PR_IRPT_SET:
     	if (mb [1] >= 0)
     	{
							 pr->method[m].data [MDATA_PR_IRPT_GEN] = mb [1];
							 if (pr->method[m].data [MDATA_PR_IRPT_GEN] > pr->method[m].data [MDATA_PR_IRPT_MAX])
									 pr->method[m].data [MDATA_PR_IRPT_GEN] = pr->method[m].data [MDATA_PR_IRPT_MAX];
							pr->irpt_gain = pr->method[m].data [MDATA_PR_IRPT_GEN];
     	}
      return pr->method[m].data [MDATA_PR_IRPT_GEN];
     case MSTATUS_PR_IRPT_MAX: // returns total capacity
      return pr->method[m].data [MDATA_PR_IRPT_MAX];
    }
    return 0;

   case MTYPE_PR_ALLOCATE:
// the status value is 0 if the method hasn't been used this execution, and 1 if it has. It's reset to 0 immediately after execution by active_method_pass_after_execution()
// TO DO: should I put something in the mbank to indicate status?
    if (pr->method[m].data [MDATA_PR_ALLOCATE_COUNTER] < 100 // not ready
     || *(pr->data) == *(pr->data_max))
     return 0; // fail/don't need to run
    int data_allocated = 8;// + pr->method[m].extension [0];
    if (*(pr->data) + data_allocated > *(pr->data_max))
    {
     data_allocated = *(pr->data_max) - *(pr->data);
// Can assume data_allocated > 0 because of test just above
    }
    int allocation_cost = data_allocated * 32;// * pr->special_method_penalty; // special_method_penalty will be 1, or 2 if the proc has a virtual method
    MCOST(allocation_cost,0);

    *(pr->data) += data_allocated;

    pr->method[m].data [MDATA_PR_ALLOCATE_COUNTER] = 0; // used
    play_game_sound(SAMPLE_ALLOC, TONE_1C, 100, pr->x, pr->y);

// now set up the data gen line:
//    pr->method[m].data [MDATA_PR_ALLOCATE_LINE_TIME] = w.world_time + 8; // TO DO: make sure variable types match (may be signed/unsigned?)
    int block_line_centre_x = fixed_to_block(pr->x);
    int block_line_centre_y = fixed_to_block(pr->y);
    int block_line_angle = grand(ANGLE_1);
    int block_line_dist = grand(5);
    int block_line_end_x = block_line_centre_x + xpart(block_line_angle, block_line_dist); // - grand(3) + grand(3);
    int block_line_end_y = block_line_centre_y + ypart(block_line_angle, block_line_dist); // - grand(3) + grand(3);
    if (block_line_end_x < 2)
     block_line_end_x = 2;
    if (block_line_end_y < 2)
     block_line_end_y = 2;
    if (block_line_end_x > w.w_block - 3)
     block_line_end_x = w.w_block - 3;
    if (block_line_end_y > w.h_block - 3)
     block_line_end_y = w.h_block - 3;
    int block_line_node = grand(9);
    struct blockstruct* bl = &w.block [block_line_end_x] [block_line_end_y];

    struct cloudstruct* cld;

    cld = new_cloud(CLOUD_DATA_LINE, pr->x + fixed_xpart(pr->angle + pr->shape_str->vertex_angle [pr->method[m].ex_vertex], pr->shape_str->vertex_dist [pr->method[m].ex_vertex] + al_itofix(6)),
                                     pr->y + fixed_ypart(pr->angle + pr->shape_str->vertex_angle [pr->method[m].ex_vertex], pr->shape_str->vertex_dist [pr->method[m].ex_vertex] + al_itofix(6)));

    if (cld != NULL)
    {
     cld->timeout = 7 + data_allocated;
     cld->colour = pr->player_index;
//     cld->data [0] = ((block_line_end_x << BLOCK_SIZE_BITSHIFT) + (bl->node_x [block_line_node] * GRAIN_MULTIPLY) - pr->x) / GRAIN_MULTIPLY;
//     cld->data [1] = ((block_line_end_y << BLOCK_SIZE_BITSHIFT) + (bl->node_y [block_line_node] * GRAIN_MULTIPLY) - pr->y) / GRAIN_MULTIPLY;

     cld->data [0] = al_fixtoi(block_to_fixed(block_line_end_x) + al_itofix(bl->node_x [block_line_node]) - cld->x);
     cld->data [1] = al_fixtoi(block_to_fixed(block_line_end_y) + al_itofix(bl->node_y [block_line_node]) - cld->y);
     cld->data [2] = grand(30000);
/*     cld->data [2] = grand(2); // first line vertical or horizontal
     cld->data [3] = 10 + grand(30); // first line length
     if (cld->data [2] == 0) // vertical
     {
      cld->data [3] = 10 + grand(abs(cld->data [0]) + 1);
      if (cld->data [0] < 0)
       cld->data [3] *= -1;
     }
      else

      {
       cld->data [3] = 10 + grand(abs(cld->data [1]) + 1);
       if (cld->data [1] < 0)
        cld->data [3] *= -1;
      }*/
    }


  //if (pr->index == 0)
//  {
//   fprintf(stdout, "\nbl(%i,%i) node %i", block_line_end_x, block_line_end_y, block_line_node);
  //}

  {
  /*bl->node_x [block_line_node] += grand(5) - grand(5);
  bl->node_y [block_line_node] += grand(5) - grand(5);
  bl->node_size [block_line_node] += grand(5) - grand(5);
  if (bl->node_size [block_line_node] < 5)
   bl->node_size [block_line_node] = 5;
  if (bl->node_size [block_line_node] > 40)
   bl->node_size [block_line_node] = 40;
*/
  if (w.player[pr->player_index].allocate_effect_type == ALLOCATE_EFFECT_ALIGN)
		{
			align_block_node(bl, block_line_node);
		}
		 else
    change_block_node(bl, block_line_node, bl->node_x [block_line_node] + grand(5) - grand(5), bl->node_y [block_line_node] + grand(5) - grand(5), bl->node_size [block_line_node] + grand(5) - grand(5));
  bl->node_disrupt_timestamp [block_line_node] = w.world_time + NODE_DISRUPT_TIME_CHANGE;
  change_block_node_colour(bl, block_line_node, pr->player_index);
  /*
  if (bl->node_team_col [block_line_node] == w.player[pr->player_index].colour)
  {
   if (bl->node_col_saturation [block_line_node] < BACK_COL_SATURATIONS - 1)
    bl->node_col_saturation [block_line_node] ++;
  }
   else
   {
    bl->node_team_col [block_line_node] = w.player[pr->player_index].colour;
    bl->node_col_saturation [block_line_node] = 0;
   }*/


 }

    return 1;

// new method can be called for three purposes: testing whether it will run, checking data cost and checking irpt cost

   case MTYPE_PR_NEW:
   case MTYPE_PR_NEW_SUB:
    switch(mb [MBANK_PR_NEW_STATUS])
    {
     case MSTATUS_PR_NEW_BUILD:
      MCOST(64, MRES_NEW_FAIL_IRPT);
      return run_pr_new_method(pr, m, &pr->bcode);

     case MSTATUS_PR_NEW_T_BUILD:
      t = get_player_template_index(pr->player_index, mb [MBANK_PR_NEW_TEMPLATE]);
      if (t == -1)
       return MRES_NEW_FAIL_TEMPLATE;
      MCOST(64, MRES_NEW_FAIL_IRPT);
      return run_pr_new_method(pr, m, &templ[t].contents.bcode);

     case MSTATUS_PR_NEW_TEST:
     case MSTATUS_PR_NEW_COST_DATA:
     case MSTATUS_PR_NEW_COST_IRPT:
      MCOST(32, MRES_NEW_FAIL_IRPT);
      return run_pr_new_method(pr, m, &pr->bcode);

     case MSTATUS_PR_NEW_T_TEST:
     case MSTATUS_PR_NEW_T_COST_DATA:
     case MSTATUS_PR_NEW_T_COST_IRPT:
      MCOST(32, MRES_NEW_FAIL_IRPT);
      t = get_player_template_index(pr->player_index, mb [MBANK_PR_NEW_TEMPLATE]);
      if (t == -1)
       return MRES_NEW_FAIL_TEMPLATE;
      return run_pr_new_method(pr, m, &templ[t].contents.bcode);

     default:
      return -1; // fail
    }
    break;

   case MTYPE_PR_DESIGNATE:
//    fprintf(stdout, "\ndesignate method called: %i", mb [MBANK_PR_DESIGNATE_STATUS]);
    switch(mb [MBANK_PR_DESIGNATE_STATUS])
    {
     case MSTATUS_PR_DESIGNATE_LOCATE: // find current designated proc
      MCOST(4,0);
      return run_designate_find_proc(pr, m, &mb [0], MSTATUS_PR_DESIGNATE_LOCATE);

     case MSTATUS_PR_DESIGNATE_SPEED: // get speed of currently designated proc
      MCOST(4,0);
      return run_designate_find_proc(pr, m, &mb [0], MSTATUS_PR_DESIGNATE_SPEED);

     case MSTATUS_PR_DESIGNATE_ACQUIRE: // find new proc and designate it
      MCOST(16,0);
      return run_designate_point(pr, m, &mb [0]);

     default:
      return 0;

    }

   case MTYPE_PR_LINK:
// most of this method's functions are active. calling it just returns the message sent by other side of connection:
//  (the data value will be -1 if no connection, no message sent etc, although this can be misleading as -1 is also a valid message)
   switch(mb [0])
   {
    case MSTATUS_PR_LINK_EXISTS: // is anything connected?
// verify_method_data in f_load.c verifies this as -1 to GROUP_CONNECTIONS when loading a save file
//fprintf(stdout, "\nLink exists: %i m %i data %i", pr->index, m, pr->method[m].data [MDATA_PR_LINK_CONNECTION]);
     if (pr->method[m].data [MDATA_PR_LINK_CONNECTION] == -1)
      return 0;
     return 1;
    case MSTATUS_PR_LINK_NEXT_EXECUTION: // returns counter until connected proc's next execution (used with the time method for synchonising executions)
     if (pr->method[m].data [MDATA_PR_LINK_INDEX] == -1)
      return -1;
// reduce the returned count by 1 if the other proc will execute later than this one (so that if they're going to execute in the same tick, but the other hasn't executed yet, return 0)
     if (pr->method[m].data [MDATA_PR_LINK_INDEX] > pr->index)
      return w.proc[pr->method[m].data [MDATA_PR_LINK_INDEX]].execution_count - 1;
     return w.proc[pr->method[m].data [MDATA_PR_LINK_INDEX]].execution_count;
    case MSTATUS_PR_LINK_MESSAGE:
     if (pr->method[m].data [MDATA_PR_LINK_INDEX] == -1)
      return 0;
     int link_messages_received = w.proc[pr->method[m].data [MDATA_PR_LINK_INDEX]].method[pr->method[m].data [MDATA_PR_LINK_OTHER_METHOD]].data [MDATA_PR_LINK_RECEIVED];
     if (link_messages_received >= LINK_MESSAGES_MAX)
      return 0;
     MCOST(4,0);
     int link_send_write_address = w.proc[pr->method[m].data [MDATA_PR_LINK_INDEX]].regs.mbank [(pr->method[m].data [MDATA_PR_LINK_OTHER_METHOD] * METHOD_SIZE) + MBANK_PR_LINK_MESSAGE_ADDRESS]
                                 + link_messages_received * LINK_MESSAGE_SIZE;
     if (link_send_write_address <= 0 // fails on 0 because this almost certainly indicates a failure to initialise properly
      || link_send_write_address >= w.proc[pr->method[m].data [MDATA_PR_LINK_INDEX]].bcode.bcode_size - 1)
      return 0;
     w.proc[pr->method[m].data [MDATA_PR_LINK_INDEX]].bcode.op [link_send_write_address] = mb [MBANK_PR_LINK_VALUE1];
     w.proc[pr->method[m].data [MDATA_PR_LINK_INDEX]].bcode.op [link_send_write_address + 1] = mb [MBANK_PR_LINK_VALUE2];
     w.proc[pr->method[m].data [MDATA_PR_LINK_INDEX]].method[pr->method[m].data [MDATA_PR_LINK_OTHER_METHOD]].data [MDATA_PR_LINK_RECEIVED] ++;
     return 1;
    case MSTATUS_PR_LINK_RECEIVED:
     return pr->method[m].data [MDATA_PR_LINK_RECEIVED]; // is number of messages received since last cycle
    case MSTATUS_PR_LINK_GIVE_IRPT:
    	if (mb [1] == 0)
					 pr->give_irpt_link_method = -1;
					  else
					   pr->give_irpt_link_method = m;
					break;
    case MSTATUS_PR_LINK_GIVE_DATA:
    	if (mb [1] == 0)
					 pr->give_data_link_method = -1;
					  else
					   pr->give_data_link_method = m;
					break;
    case MSTATUS_PR_LINK_TAKE_IRPT:
    	if (pr->method[m].data [MDATA_PR_LINK_CONNECTION] == -1)
						return 0;
// calling take with mb [1] == 1 sets the other process to give its irpt to this one.
// calling take with mb [0] == 0 means that if the other process is set to give to this one (and not otherwise), its give setting is removed
   	 if (mb [1] == 0)
					{
						if (pr->group_connection[pr->method[m].data [MDATA_PR_LINK_CONNECTION]]->give_irpt_link_method == pr->method[m].data [MDATA_PR_LINK_OTHER_METHOD])
							pr->group_connection[pr->method[m].data [MDATA_PR_LINK_CONNECTION]]->give_irpt_link_method = -1;
					}
				   else
					   pr->group_connection[pr->method[m].data [MDATA_PR_LINK_CONNECTION]]->give_irpt_link_method = pr->method[m].data [MDATA_PR_LINK_OTHER_METHOD];
					break;
    case MSTATUS_PR_LINK_TAKE_DATA:
//    	fprintf(stdout, "\nproc %i take data %i", pr->index, mb [1]);
    	if (pr->method[m].data [MDATA_PR_LINK_CONNECTION] == -1)
						return 0;
// calling take with mb [1] == 1 sets the other process to give its data to this one.
// calling take with mb [0] == 0 means that if the other process is set to give to this one (and not otherwise), its give setting is removed
   	 if (mb [1] == 0)
					{
						if (pr->group_connection[pr->method[m].data [MDATA_PR_LINK_CONNECTION]]->give_data_link_method == pr->method[m].data [MDATA_PR_LINK_OTHER_METHOD])
							pr->group_connection[pr->method[m].data [MDATA_PR_LINK_CONNECTION]]->give_data_link_method = -1;
					}
				   else
					   pr->group_connection[pr->method[m].data [MDATA_PR_LINK_CONNECTION]]->give_data_link_method = pr->method[m].data [MDATA_PR_LINK_OTHER_METHOD];
					break;

/*
    case MSTATUS_PR_LINK_SEND_IRPT:
//     fprintf(stdout, "\npr %i send to method %i mb (%i,%i,%i,%i)", pr->index, m, mb[0], mb[1], mb[2], mb[3]);
     if (pr->method[m].data [MDATA_PR_LINK_CONNECTION] == -1)
      return 0; // nothing connected
// first take some overhead:
     MCOST(24,0);
     other_proc = &w.proc[pr->method[m].data [MDATA_PR_LINK_INDEX]];
// now try to transfer irpt:
     if (mb [MBANK_PR_LINK_VALUE1] > 0)
     {
      int send_irpt = mb [MBANK_PR_LINK_VALUE1];
// make sure pr has irpt to send, and other_proc has space in its irpt buffer:
      if (send_irpt > *(pr->irpt))
       send_irpt = *(pr->irpt);
      if (send_irpt > (*(other_proc->irpt_max) - *(other_proc->irpt)))
       send_irpt = *(other_proc->irpt_max) - *(other_proc->irpt);
// now send it
      *(pr->irpt) -= send_irpt;
      *(other_proc->irpt) += send_irpt;
      pr->irpt_base -= send_irpt; // these adjustments of irpt_base values mean that transferred irpt is counted in other_proc's irpt_use rather than pr's
      other_proc->irpt_base += send_irpt;
      fprintf(stdout, "\nProcess %i (%i) gave %i to proc %i (%i)", pr->index, *(pr->irpt) - send_irpt, send_irpt, other_proc->index, *(other_proc->irpt));
      return send_irpt;
//      fprintf(stdout, "\ngave %i to proc %i (now has %i)", irpt, other_proc->index, other_proc->irpt);
     }
     return 0;
    case MSTATUS_PR_LINK_SEND_DATA:
     if (pr->method[m].data [MDATA_PR_LINK_CONNECTION] == -1)
      return 0; // nothing connected
     other_proc = &w.proc[pr->method[m].data [MDATA_PR_LINK_INDEX]];
// first take some overhead:
     MCOST(24,0);
// now try to transfer data:
     if (mb [MBANK_PR_LINK_VALUE1] > 0)
     {
      int send_data = mb [MBANK_PR_LINK_VALUE1];
// make sure pr has data to send, and other_proc has space in its data buffer:
      if (send_data > *(pr->data))
       send_data = *(pr->data);
      if (send_data > (*(other_proc->data_max) - *(other_proc->data)))
       send_data = *(other_proc->data_max) - *(other_proc->data);
// now send it
      *(pr->data) -= send_data;
      *(other_proc->data) += send_data;
      return send_data;
     }
     return 0;
*/
    case MSTATUS_PR_LINK_DISCONNECT:
      if (pr->method[m].data [MDATA_PR_LINK_CONNECTION] == -1)
       return 0; // nothing connected
/*      fprintf(stdout, "\nDisconnecting: pr %i method %i MDATA_PR_LINK_CONNECTION %i MDATA_PR_LINK_INDEX %i w.proc[pr->method [m].data [MDATA_PR_LINK_INDEX]].group %i pr->group %i",
              pr->index, m, pr->method[m].data [MDATA_PR_LINK_CONNECTION],
              pr->method [m].data [MDATA_PR_LINK_INDEX],
              w.proc[pr->method [m].data [MDATA_PR_LINK_INDEX]].group,
              pr->group);*/
      prepare_shared_resource_distribution(pr, 0, pr->method[m].data [MDATA_PR_LINK_CONNECTION]); // must be called before disconnect_procs
      disconnect_procs(pr, &w.proc[pr->method [m].data [MDATA_PR_LINK_INDEX]]);
      fix_resource_pools(); // must be called after disconnect_procs
      return 1;

   }
   return 0;

  case MTYPE_PR_RESTORE:
     if (pr->hp == pr->hp_max
      || mb [MBANK_PR_RESTORE_NUMBER] <= 0
      || pr->method[m].data [MDATA_PR_RESTORE_USED] != 0)
      return 0;
     pr->method[m].data [MDATA_PR_RESTORE_USED] = 1; // can't be used again until next tick
     int amount_restored = mb [MBANK_PR_RESTORE_NUMBER];
     if (amount_restored > pr->method[m].data [MDATA_PR_RESTORE_RATE])
      amount_restored = pr->method[m].data [MDATA_PR_RESTORE_RATE];
     if (pr->hp + amount_restored > pr->hp_max)
      amount_restored = pr->hp_max - pr->hp;
#define RESTORE_IRPT_COST 128
     if ((amount_restored * RESTORE_IRPT_COST) > *(pr->irpt))
      amount_restored = *(pr->irpt) / RESTORE_IRPT_COST;
     pr->hp += amount_restored;
     *(pr->irpt) -= amount_restored * RESTORE_IRPT_COST; // is 128 a good value for cost? not sure. Seems reasonable.
     return amount_restored;
// TO DO: need some kind of visual effect here!

  case MTYPE_PR_LISTEN:
// LISTEN returns the number of messages received (from broadcasts) since the last execution. Shouldn't be greater than LISTEN_MESSAGES_MAX.
   return pr->method[m].data [MDATA_PR_LISTEN_RECEIVED];

  case MTYPE_PR_BROADCAST:
   if (mb [MBANK_PR_BROADCAST_POWER] > 0
    && mb [MBANK_PR_BROADCAST_ID] != 0)
    return run_broadcast(pr, m);
   return 0;

  case MTYPE_PR_YIELD:
   if (pr->method[m].data [MDATA_PR_YIELD_ACTIVATED] == 1)
    return MRES_YIELD_FAIL_ALREADY;
   if (mb [MBANK_PR_YIELD_IRPT] <= 0
    && mb [MBANK_PR_YIELD_DATA] <= 0)
    return MRES_YIELD_FAIL_SETTINGS;
   return run_yield(pr, m); // returns MRES_YIELD_* values

  case MTYPE_PR_COMMAND: // fields: mode (r/w), command index, value (value ignored if reading). returns value (read) or success/fail (write)
   if (mb [1] < 0 || mb [1] >= COMMANDS)
    return 0;
   switch(mb[0])
   {
   	case MSTATUS_PR_COMMAND_VALUE:
     MCOST(2,0);
     return pr->command [mb [1]];
    case MSTATUS_PR_COMMAND_BIT:
     MCOST(1,0);
					if (mb [2] < 0 || mb [2] >= 16)
						return 0;
					if ((pr->command [mb [1]] & (1 << mb [2])))
						return 1;
					return 0;
   }
   return pr->command [mb [0]];

/*
//     fprintf(stdout, "\npr %i command(%i, %i) = %i", pr->index, mb [0], mb [1], pr->command [mb [1]]);
     command_index = mb [1];
     if (command_index < 0 || command_index >= COMMANDS)
      return 0;
     switch(mb [0])
     {
      case MSTATUS_PR_COMMAND_READ:
       MCOST(1,0);
       return pr->command [command_index];
      case MSTATUS_PR_COMMAND_WRITE:
       MCOST(2,0);
       pr->command [command_index] = mb [2];
       return 1;
      default: return 0;
     }*/
//     break; // end MTYPE_PR_COMMAND - interaction with proc command arrays

    case MTYPE_PR_VIRTUAL:
     switch(mb [0])
     {
      case MSTATUS_PR_VIRTUAL_CHARGE:
       if (pr->method[m].data [MDATA_PR_VIRTUAL_STATE] >= pr->method[m].data [MDATA_PR_VIRTUAL_MAX] // full
        || pr->method[m].data [MDATA_PR_VIRTUAL_STATE] < 0 // in recycle state
        || pr->method[m].data [MDATA_PR_VIRTUAL_PULSE] >= 10) // has already been charged this tick
         return 0;
       int virtual_charge_amount = mb [1];
       if (virtual_charge_amount > 4 + (pr->method[m].extension [1] * 3))
        virtual_charge_amount = 4 + (pr->method[m].extension [1] * 3);
       if (pr->method[m].data [MDATA_PR_VIRTUAL_STATE] + virtual_charge_amount > pr->method[m].data [MDATA_PR_VIRTUAL_MAX])
        virtual_charge_amount = pr->method[m].data [MDATA_PR_VIRTUAL_MAX] - pr->method[m].data [MDATA_PR_VIRTUAL_STATE];
#define VIRTUAL_CHARGE_COST_PER_UNIT 32
       int virtual_charge_cost = virtual_charge_amount * VIRTUAL_CHARGE_COST_PER_UNIT;
       if (virtual_charge_cost > *(pr->irpt))
       {
        virtual_charge_amount = *(pr->irpt) / VIRTUAL_CHARGE_COST_PER_UNIT;
       }
       if (virtual_charge_amount <= 0)
        return 0;
// now charge:
       pr->method[m].data [MDATA_PR_VIRTUAL_STATE] += virtual_charge_amount;
       mb [MBANK_PR_VIRTUAL_CURRENT] = pr->method[m].data [MDATA_PR_VIRTUAL_STATE];
       *(pr->irpt) -= virtual_charge_cost;
       pr->method[m].data [MDATA_PR_VIRTUAL_CHARGE_PULSE] = 16;
/*       if (pr->method[m].data [MDATA_PR_VIRTUAL_CHARGE_PULSE] > VIRTUAL_METHOD_PULSE_MAX)
        pr->method[m].data [MDATA_PR_VIRTUAL_PULSE] = VIRTUAL_METHOD_PULSE_MAX;
       if (pr->method[m].data [MDATA_PR_VIRTUAL_STATE] == virtual_charge_amount)
        pr->method[m].data [MDATA_PR_VIRTUAL_PULSE] = VIRTUAL_METHOD_PULSE_MAX;*/
       break;
      case MSTATUS_PR_VIRTUAL_GET_MAX:
       return pr->method[m].data [MDATA_PR_VIRTUAL_MAX];
      case MSTATUS_PR_VIRTUAL_GET_STATE:
       if (pr->method[m].data [MDATA_PR_VIRTUAL_STATE] < 0)
        return pr->method[m].data [MDATA_PR_VIRTUAL_STATE] * -1; // returns number of ticks left until can charge
       if (pr->method[m].data [MDATA_PR_VIRTUAL_STATE] == 0)
        return 0; // off, but can charge
       return -1; // active
      case MSTATUS_PR_VIRTUAL_DISABLE:
//       if (pr->method[m].data [MDATA_PR_VIRTUAL_STATE] > 0)
//        pr->special_method_penalty--;
       pr->method[m].data [MDATA_PR_VIRTUAL_STATE] = 0;
       return 0;
     }
     break;



// client methods:

  case MTYPE_CL_COMMAND_GIVE: // fields are: status, proc index, command index, value. Return value is success/fail
// mb [1] is always the process index
   proc_index = mb [1];
   if (proc_index < 0 || proc_index >= w.max_procs)
    return 0;
   if (w.proc[proc_index].exists <= 0
    || (cl->player != -1
     && w.proc[proc_index].player_index != cl->player))
    return 0;
   command_index = mb [2];
   if (command_index < 0 || command_index >= COMMANDS)
    return 0;
  	switch(mb[0])
  	{
  		case MSTATUS_CL_COMMAND_SET_VALUE:
     MCOST(1,0);
     w.proc[proc_index].command [command_index] = mb [3];
     break;
  	 case MSTATUS_CL_COMMAND_BIT_0:
     MCOST(1,0);
     if (mb [3] < 0 || mb [3] >= 16)
      return 0;
     w.proc[proc_index].command [command_index] &= (0xffff ^ (1 << mb [3]));
  	 	break;
  	 case MSTATUS_CL_COMMAND_BIT_1:
     MCOST(1,0);
     if (mb [3] < 0 || mb [3] >= 16)
      return 0;
     w.proc[proc_index].command [command_index] |= (1 << mb [3]);
  	 	break;
  	 default:
					return 0;
  	}
   return 1;

   case MTYPE_CL_TEMPLATE:
    return cl_copy_to_template(cl, &mb [0]);

    case MTYPE_CL_OUTPUT:
     switch(mb[0])
     {
      case MSTATUS_CL_OUTPUT_COLOUR:
       if (mb[1] >= 0
        && mb[1] < PRINT_COLS)
        {
         w.print_colour = mb[1];
         return 1;
        }
       break;
     }
     return 0;

// client/observer methods:

  case MTYPE_CLOB_POINT: // fields: mode, x, y
   switch(mb [0])
   {
     case MSTATUS_CLOB_POINT_EXACT: // check for proc at location. Fields: x, y (in world pixels). Returns index of proc, or -1 if no proc found.
      MCOST(8,0);
      return check_point_collision(al_itofix((int) mb [1]), al_itofix((int) mb [2]));
     case MSTATUS_CLOB_POINT_FUZZY: // check for proc at location, with fuzziness (see check_fuzzy_point_collision). Fields: x, y (in world pixels). Returns index of proc, or -1 if no proc found.
      MCOST(8,0);
      return check_fuzzy_point_collision(al_itofix((int) mb [1]), al_itofix((int) mb [2]));
     case MSTATUS_CLOB_POINT_ALLOC_EFFIC: // returns allocator efficiency of a proc located at x,y
      MCOST(32,0); // expensive because it involves quite a bit of checking
      return get_point_alloc_efficiency(al_itofix((int) mb [1]), al_itofix((int) mb [2]));

// should maybe add a way to check whether the console is over a particular screen location
   }
   break; // end MTYPE_CLOB_POINT

  case MTYPE_CLOB_QUERY:
   MCOST(1,0);
   return run_query(&mb [0]);

  case MTYPE_CLOB_SCAN:
// see also MTYPE_PR_SCAN above
   ;
   int clob_scan_range = 1200; // size of the scan circle
   int clob_scan_x1, clob_scan_y1, clob_scan_size, results_type;

   switch(mb[0])
   {
    case MSTATUS_CLOB_SCAN_SCAN: // this runs a scan just like a proc's scan, except that the coordinates can be anywhere in the world.
    case MSTATUS_CLOB_SCAN_SCAN_INDEX: // the same, except that the results are proc indices rather than coordinates
    clob_scan_x1 = mb [MBANK_PR_SCAN_X1];
    clob_scan_y1 = mb [MBANK_PR_SCAN_Y1];
    clob_scan_size = mb [MBANK_PR_SCAN_SIZE];
    if (clob_scan_x1 <= 0
     || clob_scan_x1 >= w.w_pixels
     || clob_scan_y1 <= 0
     || clob_scan_y1 >= w.h_pixels)
     return MRES_SCAN_FAIL_RANGE;
    if (clob_scan_size < 0)
     return MRES_SCAN_FAIL_SIZE;
    if (clob_scan_size > clob_scan_range)
     clob_scan_size = clob_scan_range;
    results_type = SCAN_RESULTS_XY;
    if (mb[0] == MSTATUS_CLOB_SCAN_SCAN_INDEX)
     results_type = SCAN_RESULTS_INDEX;
    int clob_max_number = mb [MBANK_PR_SCAN_NUMBER];
    if (clob_max_number <= 0)
     return MRES_SCAN_FAIL_NUMBER;
    if (clob_max_number > SCANLIST_MAX_SIZE)
     clob_max_number = SCANLIST_MAX_SIZE;

    int clob_scan_cost = 64 + (clob_max_number * 2) + (clob_scan_size / 8);

    MCOST(clob_scan_cost, MRES_SCAN_FAIL_IRPT);

    return run_scan_square_spiral(
                           &cl->bcode,
                           NULL, // ignore_proc (NULL means no procs excluded from the scan)
                           clob_scan_x1,
                           clob_scan_x1,
                           clob_scan_size,
                           al_itofix(clob_scan_x1),
                           al_itofix(clob_scan_y1),
                           al_itofix(clob_scan_size),
                           clob_max_number,
                           mb,
                           results_type);
// return value of run_scan is the number of procs found (or MRES_SCAN_FAIL_* if error)

    case MSTATUS_CLOB_SCAN_EXAMINE:
// This does some of the same bounds-checking as the previous cases, but not all of it is needed.
// Actually this mode is probably unnecessary as a CLOB can just use its query method. Oh well.
     clob_scan_x1 = mb [MBANK_PR_SCAN_X1];
     clob_scan_y1 = mb [MBANK_PR_SCAN_Y1];
     if (clob_scan_x1 <= 0
      || clob_scan_x1 >= w.w_pixels
      || clob_scan_y1 <= 0
      || clob_scan_y1 >= w.h_pixels)
      return MRES_SCAN_FAIL_RANGE;

     MCOST(48, MRES_SCAN_FAIL_IRPT);

     return run_examine(&cl->bcode,
                   al_itofix(clob_scan_x1), // centre of scan
                   al_itofix(clob_scan_y1),
                   al_itofix(0), // base to calculate x/y of target from
                   al_itofix(0),
                   mb);
// return value of run_examine is 1 if proc found at point, 0 otherwise

   case MSTATUS_CLOB_SCAN_RECTANGLE:
// don't need to do bounds-checking here, because run_scan_rectangle handles it
    MCOST(64, MRES_SCAN_FAIL_IRPT);

    return run_scan_rectangle(&cl->bcode,
                   al_itofix((int) mb [MBANK_PR_SCAN_X1]), // scan corner (note that coordinates from a rectangle scan are absolute, not relative to scan centre)
                   al_itofix((int) mb [MBANK_PR_SCAN_Y1]), // corner
                   al_itofix((int) mb [MBANK_PR_SCAN_SIZE]), // corner (this is in the position you'd expect X2)
                   al_itofix((int) mb [MBANK_PR_SCAN_Y2]), // corner
                   mb,
                   SCAN_RESULTS_XY);
// return value of run_scan_rectangle is the number of procs found (or MRES_SCAN_FAIL_* if error)

   case MSTATUS_CLOB_SCAN_RECTANGLE_INDEX:
    MCOST(64, MRES_SCAN_FAIL_IRPT);

    return run_scan_rectangle(&cl->bcode,
                   al_itofix((int) mb [MBANK_PR_SCAN_X1]), // scan corner
                   al_itofix((int) mb [MBANK_PR_SCAN_Y1]), // corner
                   al_itofix((int) mb [MBANK_PR_SCAN_SIZE]), // corner
                   al_itofix((int) mb [MBANK_PR_SCAN_Y2]), // corner
                   mb,
                   SCAN_RESULTS_INDEX);
// return value of run_scan_rectangle is the number of procs found (or MRES_SCAN_FAIL_* if error)
    default: return MRES_SCAN_FAIL_STATUS;
   } // end MTYPE_CLOB_SCAN

//  case MTYPE_CLOB_MATHS:   this method is a fall through case above with MTYPE_PR_MATHS
//   break;

  case MTYPE_CLOB_STD:
   MCOST(1,0);
   switch(mb [0])
   {
    case MSTATUS_CLOB_STD_WORLD_SIZE_X:
     return w.w_pixels;
    case MSTATUS_CLOB_STD_WORLD_SIZE_Y:
     return w.h_pixels;
/*    case MSTATUS_CLOB_WORLD_BLOCKS_X:   doesn't seem necessary
     return w.w_block;
    case MSTATUS_CLOB_WORLD_BLOCKS_Y:
     return w.h_block;*/
    case MSTATUS_CLOB_STD_WORLD_PROCS:
     return w.max_procs;
    case MSTATUS_CLOB_STD_WORLD_TEAMS:
     return w.players;
    case MSTATUS_CLOB_STD_WORLD_TEAM_SIZE:
     p = cl->player;
     if (p == -1) // indicates that this is an observer or system program
     {
      if (mb[1] < 0 || mb[1] >= w.players)
       return -1;
      p = mb[1];
     }
     int procs_on_team = 0;
     for (i = w.player[p].proc_index_start; i < w.player[p].proc_index_end; i ++)
     {
      if (w.proc[i].exists > 0)
       procs_on_team++;
     }
     return procs_on_team;
    case MSTATUS_CLOB_STD_WORLD_MAX_PROCS_EACH:
     return w.max_procs / w.players;
    case MSTATUS_CLOB_STD_WORLD_FIRST_PROC:
     if (cl->player == -1) // indicates that this is an observer or system program
     {
      if (mb[1] < 0 || mb[1] >= w.players)
       return -1;
      return w.player[mb[1]].proc_index_start;
     }
     return w.player[cl->player].proc_index_start;
    case MSTATUS_CLOB_STD_WORLD_LAST_PROC:
     if (cl->player == -1) // indicates that this is an observer or system program
     {
      if (mb[1] < 0 || mb[1] >= w.players)
       return -1;
      return w.player[mb[1]].proc_index_end;
     }
     return w.player[cl->player].proc_index_end;
    case MSTATUS_CLOB_STD_WORLD_TEAM:
//     if (cl->player == -1) // indicates that this is an observer or system program
//      return -1;
     return cl->player; // returns -1 if no team
/*    case MSTATUS_CLOB_WORLD_GEN_LIMIT:
     return w.gen_limit;
    case MSTATUS_CLOB_WORLD_GEN_NUMBER:
     if (cl->player == -1) // indicates that this is an observer or system program
     {
      if (mb[1] < 0 || mb[1] >= w.players)
       return -1;
      return w.player[mb[1]].gen_number;
     }
     return w.player[cl->player].gen_number;*/
    case MSTATUS_CLOB_STD_WORLD_INSTR_LEFT:
     return *instr_left;
    case MSTATUS_CLOB_STD_WORLD_TIME: // fills mb [1] and mb [2] with world time (mb [1] is time / 32767; mb [2] is time % 32767); returns value that will be in mb [2]
     mb [1] = w.world_time / 32767;
     mb [2] = w.world_time % 32767;
     return mb [2];
    case MSTATUS_CLOB_STD_TEMPLATE_NAME:
// prints name of a template (belonging to this player or, if OB or SY program, a specified player)
     if (cl->player == -1) // OB or SY program
					{
						if (mb[1] < 0 || mb[1]>=w.players)
							return 0;
      print_player_template_name(cl->type, -1, mb[1], mb [2]);
					}
					 else
       print_player_template_name(cl->type, -1, cl->player, mb [1]);
					return 1;


   }
   return 0;

  case MTYPE_CLOB_COMMAND_REC: // fields are: proc index, command index. Return value is value
   proc_index = mb [1];
   if (proc_index < 0 || proc_index >= w.max_procs)
    return 0;
   if (w.proc[proc_index].exists <= 0
    || (cl->player != -1
     && w.proc[proc_index].player_index != cl->player))
    return 0;
   command_index = mb [2];
   if (command_index < 0 || command_index >= COMMANDS)
    return 0;
   switch(mb [0])
   {
			 case MSTATUS_CLOB_COMMAND_VALUE:
     MCOST(1,0);
     return w.proc[proc_index].command [command_index];
 				break;
			 case MSTATUS_CLOB_COMMAND_BIT:
			 	if (mb [3] < 0 || mb [3] >= 16)
						return 0;
			 	if (w.proc[proc_index].command [command_index] & (1 << mb [3]))
						return 1;
 				return 0;
   }
   return 0;

// observer methods:

  case MTYPE_OB_INPUT:
   switch(mb [MBANK_OB_INPUT_MODE])
   {
    case MSTATUS_OB_INPUT_MODE_MOUSE_BUTTON:
     if (control.mouse_status == MOUSE_STATUS_OUTSIDE)
      return MOUSE_STATUS_OUTSIDE; // mouse not available (e.g. is in editor window)
     mb [1] = control.mbutton_press [0];
     mb [2] = control.mbutton_press [1];
//     fprintf(stdout, "\nbuttons: %i %i", control.mbutton_press [0], control.mbutton_press [1]);
     return control.mouse_status;
    case MSTATUS_OB_INPUT_MODE_MOUSE_XY: // fields are: x (on map), y (on map)
     if (control.mouse_status == MOUSE_STATUS_OUTSIDE)
      return MOUSE_STATUS_OUTSIDE; // mouse not available (e.g. is in editor window)
     mb [1] = control.mouse_x_world_pixels;
     mb [2] = control.mouse_y_world_pixels;
     return control.mouse_status;
    case MSTATUS_OB_INPUT_MODE_MOUSE_SCREEN_XY: // fields are: x (on screen), y (on screen)
     if (control.mouse_status == MOUSE_STATUS_OUTSIDE)
      return MOUSE_STATUS_OUTSIDE; // mouse not available (e.g. is in editor window)
     mb [1] = control.mouse_x_screen_pixels; // need to make sure this can't exceed 32767
     mb [2] = control.mouse_y_screen_pixels;
//     fprintf(stdout, "\nmouse screen_pixels %i, %i", control.mouse_x_screen_pixels, control.mouse_y_screen_pixels);
     return control.mouse_status;
    case MSTATUS_OB_INPUT_MODE_MOUSE_MAP_XY: // fields are: world x,y coordinates corresponding to position of mouse pointer on map (not updated if pointer not on map)
//      fprintf(stdout, "\nMouse map called: status %i", control.mouse_status);
      if (control.mouse_status != MOUSE_STATUS_MAP)
       return control.mouse_status;
//      MCOST(1);
/*      if (control.mouse_x_screen_pixels >= view.map_x
       && control.mouse_x_screen_pixels <= view.map_x + view.map_w
       && control.mouse_y_screen_pixels >= view.map_y
       && control.mouse_y_screen_pixels <= view.map_y + view.map_h)*/
//      {
       int map_x_pos = control.mouse_x_screen_pixels - view.map_x;// / view.map_proportion_x; // this is how far across the map the mouse cursor is
//       map_x_pos = map_x_pos / al_fixtof(view.map_proportion_x);
       map_x_pos = al_fixtoi(al_fixdiv(al_itofix(map_x_pos), view.map_proportion_x));
//       map_x_pos = map_x_pos / view.map_proportion_x;
       mb [1] = map_x_pos;
       int map_y_pos = control.mouse_y_screen_pixels - view.map_y;// / view.map_proportion_y; // this is how far across the map the mouse cursor is
//       map_y_pos = map_y_pos / al_fixtof(view.map_proportion_y);
       map_y_pos = al_fixtoi(al_fixdiv(al_itofix(map_y_pos), view.map_proportion_y));
//       map_y_pos = map_y_pos / view.map_proportion_y;
       mb [2] = map_y_pos;
//       fprintf(stdout, "\nmap pos B: %i, %i", map_x_pos, map_y_pos);
       return MOUSE_STATUS_MAP;
//      }
    case MSTATUS_OB_INPUT_KEY: // fields: status, key (in KEYS_?)
     if (mb[1] < 0 || mb[1] >= KEYS)
      return 0;
     return control.key_press [mb[1]]; // returns -1 (just released), 0 (not pressed), 1 (just pressed) or 2 (held).
    case MSTATUS_OB_INPUT_ANY_KEY:
					if (control.any_key == -1)
					{
						mb[0] = BUTTON_NOT_PRESSED;
						return -1;
					}
					mb[0] = control.key_press [control.any_key];
					return control.any_key;
   }
   break; // end MTYPE_CL_INPUT, // mouse and keyboard input

  case MTYPE_OB_VIEW: // interaction with user view
// modes:
//  0 - camera focus x/y - fields: x, y (in pixels) (does nothing if out of bounds)
//  1 - camera follow proc - fields: proc index (does nothing if invalid). Must be updated each tick.
//  2 - show data for proc - fields: proc index. Stays until changed. If invalid (e.g. -1) removes data display until set again. TO DO: deal with proc destruction.
// TO DO: think about making view part of the worldstruct (or at least including a pointer to the current view in worldstruct)
   switch(mb [0])
   {


    case MSTATUS_OB_VIEW_FOCUS_XY: // moves camera focus to x/y position in world (x/y in pixels)
     if (mb [1] > 0 && mb [1] < view.w_x_pixel
      && mb [2] > 0 && mb [2] < view.w_y_pixel)
     {
      view.camera_x = al_itofix(mb [1]);
      view.camera_y = al_itofix(mb [2]);
     }
     return 1;

    case MSTATUS_OB_VIEW_FOCUS_PROC: // set camera focus to a particular proc (just once; doesn't follow it)
//     fprintf(stdout, "\nCamera focus: %i", mb [mbase + 1]);
     if (mb [1] >= 0
      && mb [1] < w.max_procs
      && w.proc[mb [1]].exists == 1)
     {
      view.camera_x = w.proc[mb [1]].x;
      view.camera_y = w.proc[mb [1]].y;
      return 1;
     }
     return 0;

    case MSTATUS_OB_VIEW_PROC_DATA: // sets proc data display to a new proc
     if (mb [1] >= 0
      && mb [1] < w.max_procs
      && w.proc[mb [1]].exists == 1)
     {
      view.focus_proc = &w.proc[mb [1]];
     }
      else
       view.focus_proc = NULL;
     reset_proc_box_height(view.focus_proc); // focus_proc == NULL is okay
     return 1;

    case MSTATUS_OB_VIEW_PROC_DATA_XY: // proc data box location
     place_proc_box(mb [1], mb [2], view.focus_proc); // focus_proc == NULL is okay
// some bounds checking is done in the drawing function in i_display.c
     return 1;

    case MSTATUS_OB_VIEW_SCROLL_XY: // scroll view x/y
     view.camera_x += al_itofix((int) mb [1]);
     if (view.camera_x < view.camera_x_min)
      view.camera_x = view.camera_x_min;
     if (view.camera_x > view.camera_x_max)
      view.camera_x = view.camera_x_max;
     view.camera_y += al_itofix((int) mb [2]);
     if (view.camera_y < view.camera_y_min)
      view.camera_y = view.camera_y_min;
     if (view.camera_y > view.camera_y_max)
      view.camera_y = view.camera_y_max;
//     fprintf(stdout, "\nScroll to (%i, %i) itofix (%f, %f) (camera at %f, %f)", mb [1], mb[2], al_fixtof(al_itofix((int) mb [1])), al_fixtof(al_itofix((int) mb [2])), al_fixtof(view.camera_x), al_fixtof(view.camera_y));
     break;

    case MSTATUS_OB_VIEW_MAP_VISIBLE: // turns map on or off
     if (mb [1] == 0
      || mb [1] == 1)
      view.map_visible = mb [1];
     break;

    case MSTATUS_OB_VIEW_MAP_XY: // set map x/y location on screen
     view.map_x = (int) mb [1];
     if (view.map_x < 0)
      view.map_x = 0;
     if (view.map_x > view.window_x)
      view.map_x = view.window_x;
     view.map_y = (int) mb [2];
     if (view.map_y < 0)
      view.map_y = 0;
     if (view.map_y > view.window_y)
      view.map_y = view.window_y;
     break;

    case MSTATUS_OB_VIEW_MAP_SIZE: // set map w/h
     view.map_w = (int) mb [1];
     if (view.map_w < MAP_MINIMUM_SIZE) // minimum map size.
      view.map_w = MAP_MINIMUM_SIZE;
     if (view.map_w > view.window_x)
      view.map_w = view.window_x;
     view.map_h = (int) mb [2];
     if (view.map_h < MAP_MINIMUM_SIZE)
      view.map_h = MAP_MINIMUM_SIZE;
     if (view.map_h > view.window_y)
      view.map_h = view.window_y;
     view.map_proportion_x = al_fixdiv(al_itofix(view.map_w), w.w_fixed);
     view.map_proportion_y = al_fixdiv(al_itofix(view.map_h), w.h_fixed);
//     view.map_proportion_x = view.map_w / w.w_fixed;
//     view.map_proportion_y = view.map_h / w.h_fixed;
     break;

    case MSTATUS_OB_VIEW_GET_DISPLAY_SIZE: // get window w/h (fields: 1 width 2 height) returns resize flag (is 1 when execution begins from load or if screen resized e.g. by editor being opened (holds value of 1 for a single tick); otherwise is 0)
     mb [1] = view.window_x;
     mb [2] = view.window_y;
     return view.just_resized;

    case MSTATUS_OB_VIEW_COLOUR_PROC:
// 1 is player index
// 2-7 are min/max colours (r min, r max, g min etc.)
     if (mb [1] < 0
      || mb [1] >= w.players)
       return 0;
// colour values aren't bounds-checked here, because the colour-setting functions do bounds checking
     w.player[mb[1]].proc_colour_min [0] = mb [2];
     w.player[mb[1]].proc_colour_max [0] = mb [3];
     w.player[mb[1]].proc_colour_min [1] = mb [4];
     w.player[mb[1]].proc_colour_max [1] = mb [5];
     w.player[mb[1]].proc_colour_min [2] = mb [6];
     w.player[mb[1]].proc_colour_max [2] = mb [7];
     map_player_base_colours(mb [1]);
     return 1;

    case MSTATUS_OB_VIEW_COLOUR_PACKET:
// 1 is player index
// 2-7 are min/max colours (r min, r max, g min etc.)
     if (mb [1] < 0
      || mb [1] >= w.players)
       return 0;
// colour values aren't bounds-checked here, because the colour-setting functions do bounds checking
     w.player[mb[1]].packet_colour_min [0] = mb [2];
     w.player[mb[1]].packet_colour_max [0] = mb [3];
     w.player[mb[1]].packet_colour_min [1] = mb [4];
     w.player[mb[1]].packet_colour_max [1] = mb [5];
     w.player[mb[1]].packet_colour_min [2] = mb [6];
     w.player[mb[1]].packet_colour_max [2] = mb [7];
     map_player_packet_colours(mb [1]);
     map_player_virtual_colours(mb [1]);
     return 1;

    case MSTATUS_OB_VIEW_COLOUR_DRIVE:
// 1 is player index
// 2-7 are min/max colours (r min, r max, g min etc.)
     if (mb [1] < 0
      || mb [1] >= w.players)
       return 0;
// colour values aren't bounds-checked here, because the colour-setting functions do bounds checking
     w.player[mb[1]].drive_colour_min [0] = mb [2];
     w.player[mb[1]].drive_colour_max [0] = mb [3];
     w.player[mb[1]].drive_colour_min [1] = mb [4];
     w.player[mb[1]].drive_colour_max [1] = mb [5];
     w.player[mb[1]].drive_colour_min [2] = mb [6];
     w.player[mb[1]].drive_colour_max [2] = mb [7];
     map_player_drive_colours(mb [1]);
     return 1;

    case MSTATUS_OB_VIEW_COLOUR_BACK:
// 1-3 are r,g,b
// colour values aren't bounds-checked here, because the colour-setting functions do bounds checking
     w.background_colour [0] = mb [1];
     w.background_colour [1] = mb [2];
     w.background_colour [2] = mb [3];
     map_background_colour();
     return 1;

    case MSTATUS_OB_VIEW_COLOUR_BACK2:
// 1-3 are r,g,b
// colour values aren't bounds-checked here, because the colour-setting functions do bounds checking
     w.hex_base_colour [0] = mb [1];
     w.hex_base_colour [1] = mb [2];
     w.hex_base_colour [2] = mb [3];
     for (i = 0; i < w.players; i ++)
					{
						map_hex_colours(i);
					}
     return 1;

    case MSTATUS_OB_VIEW_SOUND: // Note that sounds won't play while fast forwarding.
     if (game.play_sound_counter == 0)
						game.play_sound_counter = 1; // means it will play later this tick
    	game.play_sound = 1;
    	game.play_sound_sample = mb [1]; // these values are all bounds-checked when the sound is played. Invalid values just result in no sound.
    	game.play_sound_note = mb [2];
    	game.play_sound_vol = mb [3];
    	if (game.play_sound_vol > 100)
						game.play_sound_vol = 100;
					if (game.play_sound_vol <= 0)
						game.play_sound_vol = 1;
					break;

   }
   return 0; // end MTYPE_OB_VIEW

  case MTYPE_OB_CONSOLE:
   return run_console_method(cl, m); // this is in i_console.c

  case MTYPE_OB_SELECT: // allows visible selection indicators etc
   switch(mb [0])
   {

    case MSTATUS_OB_SELECT_SET_MARKER: // set marker
     return set_marker((int) mb [1], (int) mb [2], (int) mb [3]); // in g_method_clob.c
// fields: 1 type, 2 timeout (the + 1 is there because the timeout will be decremented once before the marker can be shown)
// returns marker index on success, -1 on failure

    case MSTATUS_OB_SELECT_PLACE_MARKER: // locate marker
    	if (mb [1] < 0
						|| mb [1] >= MARKERS
						|| w.marker[mb [1]].active == 0)
							return 0;
     w.marker[mb[1]].x = al_itofix((int) mb [2]);
     w.marker[mb[1]].y = al_itofix((int) mb [3]);
     return 1;
// fields: 1 x, 2 y
// returns 1/0 success/fail

    case MSTATUS_OB_SELECT_PLACE_MARKER2: // locate marker 2 (affects the most recently set marker) (is meant for boxes and lines)
    	if (mb [1] < 0
						|| mb [1] >= MARKERS
						|| w.marker[mb [1]].active == 0)
							return 0;
     w.marker[mb[1]].x2 = al_itofix((int) mb [2]);
     w.marker[mb[1]].y2 = al_itofix((int) mb [3]);
// will just do nothing if the marker doesn't have two points
     return 1;

    case MSTATUS_OB_SELECT_BIND_MARKER: // bind marker to proc (affects the most recently set marker)
    	if (mb [1] < 0
						|| mb [1] >= MARKERS
						|| w.marker[mb[1]].active == 0
						|| w.marker[mb[1]].type == MARKER_BOX
						|| w.marker[mb[1]].type == MARKER_MAP_BOX) // can't bind box marker
							return 0;
     return bind_marker(mb [1], mb [2]); // in g_method_clob.c

    case MSTATUS_OB_SELECT_BIND_MARKER2: // bind marker to proc (affects the most recently set marker)
    	if (mb [1] < 0
						|| mb [1] >= MARKERS
						|| w.marker[mb[1]].active == 0
						|| w.marker[mb[1]].type == MARKER_BOX
						|| w.marker[mb[1]].type == MARKER_MAP_BOX) // can't bind box marker
							return 0;
     return bind_marker2(mb [1], mb [2]); // in g_method_clob.c

    case MSTATUS_OB_SELECT_UNBIND_PROCESS:
    	return unbind_process(mb [1]); // in g_method_clob.c

    case MSTATUS_OB_SELECT_CLEAR_MARKERS: // clear all markers
    	clear_markers();
     break;

    case MSTATUS_OB_SELECT_EXPIRE_MARKERS: // clear all markers
    	expire_markers();
     break;

    case MSTATUS_OB_SELECT_CLEAR_A_MARKER:
    	if (mb [1] < 0
						|| mb [1] >= MARKERS)
						return 0;
					w.marker[mb[1]].active = 0;
     break;

    case MSTATUS_OB_SELECT_EXPIRE_A_MARKER:
    	if (mb [1] < 0
						|| mb [1] >= MARKERS
						|| w.marker[mb[1]].active == 0)
						return 0;
    	expire_a_marker(mb[1]);
     break;

    case MSTATUS_OB_SELECT_MARKER_SPIN:
    	if (mb [1] < 0
						|| mb [1] >= MARKERS
						|| w.marker[mb[1]].active == 0)
						return 0;
					if (mb [2] >= -MARKER_SPIN_MAX
						&& mb [2] < MARKER_SPIN_MAX)
							w.marker[mb[1]].spin = mb [2];
     break;

    case MSTATUS_OB_SELECT_MARKER_ANGLE:
    	if (mb [1] < 0
						|| mb [1] >= MARKERS
						|| w.marker[mb[1]].active == 0)
						return 0;
					w.marker[mb[1]].angle = mb [2] & ANGLE_MASK;
     break;

    case MSTATUS_OB_SELECT_MARKER_SIZE:
    	if (mb [1] < 0
						|| mb [1] >= MARKERS
						|| w.marker[mb[1]].active == 0)
						return 0;
					if (mb [2] >= 0
						&& mb [2] < MARKER_SIZE_MAX)
							w.marker[mb[1]].size = mb [2];
     break;

   }
   // fields: type, duration, x, y
   // type determines on screen or map or both etc
   break; // end MTYPE_OB_SELECT
  case MTYPE_OB_CONTROL:
   switch(mb[0])
   {
    case MSTATUS_OB_CONTROL_PAUSED: // - returns 0 if not soft-paused, 1 if soft-paused
     if (game.pause_soft == 1)
      return 1;
     return 0; // observer can't access hard pause because hard pause means observer program doesn't run
    case MSTATUS_OB_CONTROL_FF: // - returns 0 if not FF, 1 if FF. Sets mb[1] to FF type
     if (game.fast_forward != FAST_FORWARD_OFF)
     {
      mb [1] = game.fast_forward_type;
      return 1;
     }
     return 0;

      case MSTATUS_OB_CONTROL_PAUSE_SET: // soft pause
       if (game.phase != GAME_PHASE_WORLD)
        return 0; // can only manage pause during world phase (different from system)
       if (mb [1] < 0 || mb [1] > 1)
        return 0;
       game.pause_soft = mb [1];
       return 1;
      case MSTATUS_OB_CONTROL_FF_SET: // sets fast-forward to 0 or 1
       if (game.phase != GAME_PHASE_WORLD)
        return 0; // can't FF during interturn phase
       if (mb [1] < FAST_FORWARD_OFF || mb [1] > FAST_FORWARD_JUST_STARTED)
        return 0;
       game.fast_forward = mb [1];
       game.fast_forward_type = mb [2];
       if (game.fast_forward_type < 0
        || game.fast_forward_type >= FAST_FORWARD_TYPES)
         game.fast_forward_type = FAST_FORWARD_TYPE_SMOOTH;
       return 1;

    case MSTATUS_OB_CONTROL_PHASE: // - returns 0 if in world phase, 1 if in interturn phase, 2 if in pregame phase, 3 if game over phase
     return game.phase;
    case MSTATUS_OB_CONTROL_SYSTEM_SET: // - allows communication with system program. 64 registers. fields: 1-register 2-value. returns 1 on success, 0 on failure.
     if (mb [1] < 0 || mb [1] >= SYSTEM_COMS)
      return 0;
     w.system_com [mb [1]] = mb [2];
     return 1;
    case MSTATUS_OB_CONTROL_SYSTEM_READ: // - inverse of SYSTEM_SET. returns value.
     if (mb [1] < 0 || mb [1] >= SYSTEM_COMS)
      return 0;
     return w.system_com [mb [1]];
    case MSTATUS_OB_CONTROL_TURN: // - returns which turn we're in
     return game.current_turn;
    case MSTATUS_OB_CONTROL_TURNS: // - returns how many turns in game
     return game.turns;
    case MSTATUS_OB_CONTROL_SECONDS_LEFT: // - returns time left in turn (in seconds)
     return game.current_turn_time_left / TICKS_TO_SECONDS_DIVISOR; // this is approximate. can't use *0.03 because we're avoiding floating point stuff in non-display game code
    case MSTATUS_OB_CONTROL_TICKS_LEFT: // - returns time left in turn in ticks (returns 32767 if overflow)
     if (game.current_turn_time_left >= 32767)
      return 32767;
       else
        return game.current_turn_time_left;
    case MSTATUS_OB_CONTROL_TURN_SECONDS: // - returns time for each turn (in seconds)
     return game.minutes_each_turn * 60;
   }
   break; // end MTYPE_OB_CONTROL

// system methods:
    case MTYPE_SY_PLACE_PROC: // this is similar to the proc new process method (and uses roughly the same mbank address system), but with some differences (e.g. proc can go anywhere)
     return run_sy_place_method(cl, m);
//     return run_sy_place_process_method(cl, m);
    case MTYPE_SY_TEMPLATE:
//     sy_copy_to_template(&mb [mbase]);
     return sy_copy_to_template(&mb [0]);
    case MTYPE_SY_MODIFY_PROC:
     return run_sy_modify_process_method(&mb [0]);
    case MTYPE_SY_MANAGE:
     return run_sy_manage_method(&mb [0]);


  default: return 0;

 } // end MTYPE switch

// may have returned earlier

 return 0;

}



/*

How should procs be able to identify each other?

I think the client program should probably be able to use proc indexes - the observer certainly should be able to.
However, procs probably shouldn't.
They will need some other way to track other procs.

How about this:
- new method:
MTYPE_PR_DESIGNATE

fields:
MBANK_PR_DESIGNATE_STATUS, // 0 = read, 1 = write
MBANK_PR_DESIGNATE_X, // x offset
MBANK_PR_DESIGNATE_Y, // y offset

when called with status 0, the method:
 - checks whether designated proc exists and is within range
  - returns 0 if either fails
 - fills X/Y mbank entries with offset of designated proc
 - returns 1

when called with status 1:
 - checks whether a proc is at x/y offset from proc
 - if not, returns 0
 - if yes, stores designated proc's index in a secret variable and returns 1.

procs can have multiple designate methods, each with a single target

after each proc execution, method checks that designated proc still exists - cancels designation if it doesn't
 - but to the proc itself this will just look like the target proc is out of range.
  - could have some way of letting the proc know if a designated proc was destroyed within range? maybe.

alternatively, a deallocating proc (i.e. just destroyed) retains its x/y values, so the designater will be able to check whether
 the place where it was destroyed is, at the time of calling, within range
  - would still need to check designated procs each execution, though


This means that client programs will not be able to usefully communicate proc indexes to procs.
How will clients handle user selection and proc behaviour management?
 - will probably need to store details of procs in a huge array and run through it each tick (or every few ticks)

Something like:

enum
{
PROC_CONTROL_MODE,
PROC_CONTROL_TARGET,
PROC_CONTROL_DATA
};

int proc_control [PROCS] [PROC_CONTROL_DATA];

enum
{
CONTROL_MODE_IDLE,
CONTROL_MODE_MOVE,
CONTROL_MODE_ATTACK
};

If set to attack:
 - the client will check whether the target still exists.
  - if it doesn't, client will set proc's behaviour appropriately
 - if it does, client will set proc's command x/y target values to target's new location
  - if in range, the proc will designate the target and chase it that way, ignoring command values.

If set to idle:
 - the client will do nothing

If set to move:
 - the client will do nothing

*/










/*


Things to do next:
 - fix data and irpt costs of everything
  - currently only new proc creation costs anything. Need to set costs for all actions
   - including running code.
  - this will involve adding an irpt generator to most procs

*/

/*

MTYPE_PR_HOLD

Sticks two procs together
Both procs must have this method at their connecting vertices
Mbank:
MBANK_PR_HOLD_SEND,
MBANK_PR_HOLD_IRPT,
MBANK_PR_HOLD_DATA,
MBANK_PR_HOLD_CONNECT

Things that can be done through the connection:
nothing
send message
give irpt
give data
disconnect

How about this:
mb [0] is message
 - other side can call its own hold method to get message
mb [1] is irpt transfer
 - in active method execution, try to take this much irpt and give it to connected proc.
mb [2] is data transfer
mb [3] is connection
 - default 1 if connected
 - if set to 0, procs will disconnect during active method execution.
 - if procs are disconnected some other way (e.g. other proc is destroyed or lets go itself) is also set to 0

calling the method gives message (0 if no proc connected)


*/













