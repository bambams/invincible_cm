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
#include "g_method.h"
#include "g_client.h"
#include "g_shape.h"
#include "m_globvars.h"
#include "m_maths.h"
#include "t_template.h"
#include "g_method_clob.h"
#include "g_method_misc.h"
#include "i_error.h"

/*

This file contains code for running complex methods for client and observer programs.

Ideally it should contain only functions called from itself or another g_method file.

*/

extern const struct mtypestruct mtype [MTYPES]; // defined in g_method.c

// see run_world() in g_world.c for marker management
int set_marker(int type, int timeout, int colour)
{

 int i;

// fprintf(stdout, "\nSet marker type %i timeout %i", type, timeout);

 if (timeout < -1
  || type < 0
  || type >= MARKER_TYPES
  || colour < 0
  || colour >= BASIC_COLS)
  return -1;

 for (i = 0; i < MARKERS; i ++)
 {
  if (w.marker[i].active == 0)
  {
   w.marker[i].active = 1;
   w.marker[i].type = type;
   w.marker[i].colour = colour;
   w.marker[i].timeout = timeout;
   w.marker[i].size = MARKER_SIZE_MAX / 2;
   w.marker[i].phase = MARKER_PHASE_APPEAR;
   w.marker[i].phase_counter = MARKER_PHASE_TIME;
   switch(type)
   {
   	default:
			 case MARKER_BOX:
   	case MARKER_MAP_AROUND_1:
   	case MARKER_MAP_AROUND_2:
   	case MARKER_MAP_CROSS:
				case MARKER_MAP_POINT:
				case MARKER_MAP_BOX:
					 w.marker[i].phase = MARKER_PHASE_CONSTANT;
					 break;
				case MARKER_BASIC:
				case MARKER_BASIC_6:
				case MARKER_BASIC_8:
				case MARKER_CORNERS:
				case MARKER_SHARP:
				case MARKER_SHARP_5:
				case MARKER_SHARP_7:
					 w.marker[i].angle = ANGLE_8;
					 w.marker[i].spin = 48;
					 break;
				case MARKER_PROC_BOX:
					 break;
   }
   w.marker[i].x = 0;
   w.marker[i].y = 0;
   w.marker[i].bind_to_proc = -1;
   w.marker[i].bind_to_proc2 = -1;
//   w.last_marker = i; // so it can be used by subsequent call to place_marker
   return i;
  }
 }

 return -1;

}
/*
// for placing the bottom right-hand corner of a box marker (although I think it can be the top left as well)
int place_marker2(al_fixed x2, al_fixed y2)
{

 if (w.last_marker == -1)
  return 0;

 if (w.marker[w.last_marker].type != MARKER_BOX)
  return 0;

 w.marker[w.last_marker].x2 = x2;
 w.marker[w.last_marker].y2 = y2;

 return 1;
}*/

// can assume m is valid
int bind_marker(int m, int proc_index)
{

 if (proc_index < 0
  || proc_index >= w.max_procs
  || w.proc[proc_index].exists <= 0)
  return 0;

 w.marker[m].bind_to_proc = proc_index;
 w.marker[m].x = w.proc[proc_index].x;
 w.marker[m].y = w.proc[proc_index].y;

 return 1;
}

// for binding other end of line marker
int bind_marker2(int m, int proc_index)
{

 if (proc_index < 0
  || proc_index >= w.max_procs
  || w.proc[proc_index].exists <= 0)
  return 0;

 w.marker[m].bind_to_proc2 = proc_index;
 w.marker[m].x2 = w.proc[proc_index].x;
 w.marker[m].y2 = w.proc[proc_index].y;

 return 1;
}



int unbind_process(int proc_index)
{

 if (proc_index < 0
  || proc_index >= w.max_procs
  || w.proc[proc_index].exists <= 0)
  return 0;

 int i;

 for (i = 0; i < MARKERS; i ++)
	{
		if (w.marker[i].active != 0
			&&	(w.marker[i].bind_to_proc == proc_index
			 ||	w.marker[i].bind_to_proc2 == proc_index))
			expire_a_marker(i);
	}

 return 1;
}

void clear_markers(void)
{
	int i;

	for (i = 0; i < MARKERS; i ++)
	{
		w.marker[i].active = 0;
	}

}

void expire_markers(void)
{

	int i;

	for (i = 0; i < MARKERS; i ++)
	{
		if (w.marker[i].active == 0)
		 continue;
		expire_a_marker(i);
	}

}

void expire_a_marker(int m)
{
		switch(w.marker[m].phase)
		{
		 case MARKER_PHASE_EXISTS:
		 	w.marker[m].phase = MARKER_PHASE_FADE;
		 	w.marker[m].phase_counter = MARKER_PHASE_TIME;
		 	break;
		 case MARKER_PHASE_APPEAR:
		 	w.marker[m].phase = MARKER_PHASE_FADE;
		 	w.marker[m].phase_counter = MARKER_PHASE_TIME - w.marker[m].phase_counter;
		 	break;
		 case MARKER_PHASE_CONSTANT:
				w.marker[m].active = 0;
				break;
// MARKER_PHASE_FADE does nothing as the marker is already fading
		}

}


/*

Need to fix how markers work.

- remove proc selection
 - replace it with a link from the proc to a marker that is set to it.
- add a "clear all markers" call
- add an "all markers expire" call that makes markers timeout (so that their disappearance animation plays)

types:
MARKER_BASIC, // 4 triangles pointing inwards; rotate; animations: inwards + fade in, outwards + fade out
MARKER_BASIC_6, // 6 triangles pointing inwards; rotate; animations: inwards + fade in, outwards + fade out
MARKER_BASIC_8, // 8 triangles pointing inwards; rotate; animations: inwards + fade in, outwards + fade out
MARKER_PROC_BOX, // box around proc (size depends on size of proc it's bound to); animations: outwards/inwards
MARKER_CORNERS, // lines at corners
MARKER_MAP_POINT, // single point on map
MARKER_MAP_AROUND_1, // up, down, left, right
MARKER_MAP_AROUND_2, // points
MARKER_MAP_LINES, // hor/ver lines


*/



// runs the MTYPE_CLOB_QUERY method, which gets information about a proc
//  fields: mode, proc index
// returns value on success
// returns -1 if status invalid or if proc doesn't exist and is not deallocating
// returns -2 if process doesn't exist and is still deallocating
//  - note that -1 or -2 may be valid values for some modes, so it doesn't necessarily mean failure. Should test with one of the ones that does first.
s16b run_query(s16b* mb)
{

 int proc_index = mb [MBANK_CLOB_QUERY_INDEX];

 if (proc_index < 0
  || proc_index >= w.max_procs
  || w.proc[proc_index].exists == 0)
   return -1; // invalid index, or process does not exist and is not deallocating

 if (w.proc[proc_index].exists == -1)
  return -2; // process has just been destroyed and is still deallocating

 struct procstruct* pr = &w.proc[proc_index];
 int i;

 switch(mb [MBANK_CLOB_QUERY_STATUS])
 {
// status uses same enums as MTYPE_PR_STD_GET functions
    case MSTATUS_PR_STD_GET_X:
     return al_fixtoi(pr->x);
    case MSTATUS_PR_STD_GET_Y:
     return al_fixtoi(pr->y);
    case MSTATUS_PR_STD_GET_SPEED_X:
     return al_fixtoi(pr->x_speed * SPEED_INT_ADJUSTMENT);
    case MSTATUS_PR_STD_GET_SPEED_Y:
     return al_fixtoi(pr->y_speed * SPEED_INT_ADJUSTMENT);
    case MSTATUS_PR_STD_GET_ANGLE:
     return al_fixtoi(pr->angle);
    case MSTATUS_PR_STD_GET_HP:
     return pr->hp;
    case MSTATUS_PR_STD_GET_HP_MAX:
     return pr->hp_max;
    case MSTATUS_PR_STD_GET_INSTR:
     return pr->instructions_left_after_last_exec; // this value is stored after proc finishes executing
    case MSTATUS_PR_STD_GET_INSTR_MAX:
     return pr->instructions_each_execution;
    case MSTATUS_PR_STD_GET_IRPT:
     return *(pr->irpt);
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
      return al_fixtoi(pr->spin);
       else
        return al_fixtoi(w.group[pr->group].spin);
    case MSTATUS_PR_STD_GET_GR_X:
     if (pr->group == -1)
      return al_fixtoi(pr->x); // or should this return 0 or something? not sure
       else
        return al_fixtoi(w.group[pr->group].x); // this should be the centre of mass of the group
    case MSTATUS_PR_STD_GET_GR_Y:
     if (pr->group == -1)
      return al_fixtoi(pr->y);
       else
        return al_fixtoi(w.group[pr->group].y);
/*    case MSTATUS_PR_STD_GET_GR_ANGLE:
     if (pr->group == -1)
      return al_fixtoi(pr->angle);
       else
        return al_fixtoi(w.group[pr->group].angle);*/
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
// world information (not really useful for queries, but since queries match info might as well include it here).
    case MSTATUS_PR_STD_GET_WORLD_W:
     return w.w_pixels;
    case MSTATUS_PR_STD_GET_WORLD_H:
     return w.h_pixels;
    case MSTATUS_PR_STD_GET_TIME: // fills mb [1] and mb [2] with world time (mb [1] is time / 32767; mb [2] is time % 32767); returns value that will be in mb [2]
     mb [1] = w.world_time / 32767;
     mb [2] = w.world_time % 32767;
     return mb [2];
    case MSTATUS_PR_STD_GET_EFFICIENCY:
     if (pr->allocate_method != -1)
      return pr->method[pr->allocate_method].data [MDATA_PR_ALLOCATE_EFFICIENCY]; // no extra cost in this case
     return get_point_alloc_efficiency(pr->x, pr->y);
// Note that the get version of the vertex-related info modes uses mb [1] instead of [2]
    case MSTATUS_PR_STD_GET_VERTICES:
					 return pr->shape_str->vertices;
				case MSTATUS_PR_STD_GET_VERTEX_ANGLE:
					 if (mb [2] < 0
							||	mb [2] >= pr->shape_str->vertices)
								return -1;
						return fixed_angle_to_short(pr->shape_str->vertex_angle [mb [2]]);
				case MSTATUS_PR_STD_GET_VERTEX_DIST:
					 if (mb [2] < 0
							||	mb [2] >= pr->shape_str->vertices)
								return -1;
						return pr->shape_str->vertex_dist_pixel [mb [2]];
				case MSTATUS_PR_STD_GET_VERTEX_ANGLE_PREV:
					 if (mb [2] < 0
							||	mb [2] >= pr->shape_str->vertices)
								return -1;
						return fixed_angle_to_short(pr->shape_str->external_angle [EXANGLE_PREVIOUS] [mb [2]]);
				case MSTATUS_PR_STD_GET_VERTEX_ANGLE_NEXT:
					 if (mb [2] < 0
							||	mb [2] >= pr->shape_str->vertices)
								return -1;
						return fixed_angle_to_short(pr->shape_str->external_angle [EXANGLE_NEXT] [mb [2]]);
				case MSTATUS_PR_STD_GET_VERTEX_ANGLE_MIN:
					 if (mb [2] < 0
							||	mb [2] >= pr->shape_str->vertices)
								return -1;
						return pr->shape_str->vertex_method_angle_min [mb [2]];
				case MSTATUS_PR_STD_GET_VERTEX_ANGLE_MAX:
					 if (mb [2] < 0
							||	mb [2] >= pr->shape_str->vertices)
								return -1;
						return pr->shape_str->vertex_method_angle_max [mb [2]];
				case MSTATUS_PR_STD_GET_METHOD:
					 if (mb [2] < 0
							||	mb [2] >= METHODS)
								return -1;
						return pr->method[mb[2]].type;
				case MSTATUS_PR_STD_GET_METHOD_FIND: // searches proc's methods for mtype of mb[2], starting from method slot mb[3]
// Could check that mb[2]	is a valid method type, but let's not bother
// But we do need to check that mb [3] is valid:
					 if (mb [3] < 0
							||	mb [3] >= METHODS)
								return -1;
						i = mb[3];
						while(i < METHODS)
						{
							if (pr->method[i].type == mb[2])
								return i;
							i++;
						};
						return -1;
				case MSTATUS_PR_STD_GET_MBANK:
					 if (mb [2] < 0
							||	mb [2] >= METHOD_BANK)
								return 0;
						return pr->regs.mbank [mb[2]];


//    case MSTATUS_PR_STD_GET_GEN_LIMIT:
//     return w.gen_limit;
//    case MSTATUS_PR_STD_GET_GEN_NUMBER:
//     return w.player[pr->player_index].gen_number;
   }

 return -1; // error (but not reliable indication of error - see notes above function)

}



// runs the method that copies bcode from the client program to a template
// returns 1 success/0 failure
s16b cl_copy_to_template(struct programstruct* cl, s16b* mb)
{

// fprintf(stdout, "\n sy_copy_to_template: mb {%i, %i, %i, %i}", mb [0], mb [1], mb [2], mb [3]);

 int p = cl->player;

 if (p == -1 // possible if for some reason the system program has this method.
  || w.may_change_proc_templates [p] == 0) // system program has prohibited this player from changing templates
  return 0;

 int program_type = cl->type; // should be PROGRAM_TYPE_DELEGATE or PROGRAM_TYPE_OPERATOR

 int t;

  if (mb [MBANK_CL_TEMPLATE_START] < 0
   || mb [MBANK_CL_TEMPLATE_START] >= CLIENT_BCODE_SIZE - BCODE_HEADER_ALL_END)
  {
   start_error(program_type, -1, p);
   error_string("\nFailed: bcode to template (invalid start address ");
   error_number(mb [MBANK_CL_TEMPLATE_START]);
   error_string(").");
   return 0;
  }

// first we work out which template we need to target:

 if (mb [MBANK_CL_TEMPLATE_INDEX] < 0
  || mb [MBANK_CL_TEMPLATE_INDEX] >= TEMPLATES_PER_PLAYER)
 {
   start_error(program_type, -1, p);
   error_string("\nFailed: bcode to template (invalid template ");
   error_number(mb [MBANK_CL_TEMPLATE_INDEX]);
   error_string(").");
   return 0;
 }

 int find_template_type;

 switch(p)
 {
  case 0: find_template_type = FIND_TEMPLATE_P0_PROC_0 + mb [MBANK_CL_TEMPLATE_INDEX]; break;
  case 1: find_template_type = FIND_TEMPLATE_P1_PROC_0 + mb [MBANK_CL_TEMPLATE_INDEX]; break;
  case 2: find_template_type = FIND_TEMPLATE_P2_PROC_0 + mb [MBANK_CL_TEMPLATE_INDEX]; break;
  case 3: find_template_type = FIND_TEMPLATE_P3_PROC_0 + mb [MBANK_CL_TEMPLATE_INDEX]; break;
  default:
   fprintf(stdout, "\nmethod_clob.c: cl_copy_to_template(): unknown p value %i", p);
   error_call();
   find_template_type = 0; // avoids compiler warning
   break;
 }

 t = w.template_reference [find_template_type];

 if (t == -1)
 {
   start_error(program_type, -1, p);
   error_string("\nFailed: bcode to template (invalid template ");
   error_number(mb [MBANK_CL_TEMPLATE_INDEX]);
   error_string(").");
   return 0;
 }

// at this stage we should be able to assume t is valid

 clear_template(t);

 if (!check_program_type(cl->bcode.op [mb [MBANK_CL_TEMPLATE_START] + BCODE_HEADER_ALL_TYPE], PROGRAM_TYPE_PROCESS))
 {
   start_error(program_type, -1, p);
   error_string("\nFailed: bcode to template (invalid type ");
   error_number(cl->bcode.op [mb [MBANK_CL_TEMPLATE_START] + BCODE_HEADER_ALL_TYPE]);
   error_string(").");
   return 0;
 }

// fprintf(stdout, "\n copying bcode to template %i", t);
 int retval = copy_bcode_to_template(t, &cl->bcode, mb [MBANK_CL_TEMPLATE_START], program_type, p, TEMPL_ORIGIN_CLIENT, mb [MBANK_CL_TEMPLATE_END], mb [MBANK_CL_TEMPLATE_NAME]);
// copy_bcode_to_template does bounds-checking on the start and end addresses

 return retval;

}


