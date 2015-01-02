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
#include "i_error.h"
#include "m_globvars.h"
#include "m_maths.h"
#include "t_template.h"
#include "t_files.h"
#include "g_method_misc.h"
#include "s_turn.h"

extern struct tstatestruct tstate;
extern struct gamestruct game;

/*

This file contains code for running complex methods for system programs.

Ideally it should contain only functions called from itself or another g_method file.

*/

extern const struct mtypestruct mtype [MTYPES]; // defined in g_method.c

extern struct templstruct templ [TEMPLATES]; // defined in t_template.c

//int test_sy_place_from_bcode(struct programstruct* cl, s16b* mb);
//int test_sy_place_from_template(s16b* mb);
//int test_sy_place(struct bcodestruct* bcode, int start_address, int end_address, s16b* mb);

//int sy_place_from_bcode(struct programstruct* cl, s16b* mb);
//int sy_place_from_template(s16b* mb);
//int sy_place(struct bcodestruct* source_bcode, int start_address, int end_address, s16b* mb);

int sy_copy_to_template(s16b* mb);

int run_sy_place_method_2(struct programstruct* cl, int m, struct bcodestruct* source_bcode);


int run_sy_place_method(struct programstruct* cl, int m)
{

 int mbase = m * METHOD_SIZE;
 int t;
 s16b *mb = &cl->regs.mbank [mbase];

// fprintf(stdout, "\ncreating: %i", mb [mbase + MBANK_SY_PLACE_STATUS]);

 int template_reference_index = -1;

 switch(mb [mbase + MBANK_SY_PLACE_STATUS])
 {

  case MSTATUS_SY_PLACE_CALL:
  case MSTATUS_SY_PLACE_CALL_TEST:
  case MSTATUS_SY_PLACE_COST_DATA:
  case MSTATUS_SY_PLACE_COST_IRPT:
   return run_sy_place_method_2(cl, m, &cl->bcode);

  case MSTATUS_SY_PLACE_T_CALL:
  case MSTATUS_SY_PLACE_T_CALL_TEST:
  case MSTATUS_SY_PLACE_T_COST_DATA:
  case MSTATUS_SY_PLACE_T_COST_IRPT:
   template_reference_index = mb [MBANK_SY_PLACE_TEMPLATE];
   if (template_reference_index < 0
    || template_reference_index >= FIND_TEMPLATES)
    return MRES_NEW_FAIL_TEMPLATE;
   t = w.template_reference [template_reference_index];
   if (t == -1)
    return MRES_NEW_FAIL_TEMPLATE;
   if (templ[t].contents.loaded == 0)
    return MRES_NEW_FAIL_TEMPLATE_EMPTY;
//   fprintf(stdout, "\nplacing from template index %i (reference %i)", t, template_reference_index);
   return run_sy_place_method_2(cl, m, &templ[t].contents.bcode);

  default:
   return MRES_NEW_FAIL_STATUS;

/*
// following not used
  case MSTATUS_SY_PLACE_BCODE_TEST:
   return test_sy_place_from_bcode(cl, mb);

  case MSTATUS_SY_PLACE_BCODE:
   return sy_place_from_bcode(cl, mb);

  case MSTATUS_SY_PLACE_TEMPLATE_TEST:
   return test_sy_place_from_template(mb);

  case MSTATUS_SY_PLACE_TEMPLATE:
   return sy_place_from_template(mb);
*/
 }


}

// similar to run_pr_new_method()
int run_sy_place_method_2(struct programstruct* cl, int m, struct bcodestruct* source_bcode)
{

 int mbase = m * METHOD_SIZE;
 s16b* mb = &cl->regs.mbank [mbase];

 int start_address = mb [MBANK_SY_PLACE_ADDRESS1];
 int end_address = mb [MBANK_SY_PLACE_ADDRESS2];

// check that the specified start and end addresses are correct:

 int retval = check_start_end_addresses(start_address, end_address, source_bcode->bcode_size);

 switch(retval)
 {
  case 0: break; // success
  case 1:
   fprintf(stdout, "\nFail: start %i end %i bcode_size %i", start_address, end_address, source_bcode->bcode_size);
   start_error(PROGRAM_TYPE_SYSTEM, -1, -1);
   error_string("\nSystem program failed to create process (invalid start address: ");
   error_number(start_address);
   error_string(").");
   return MRES_NEW_FAIL_START_BOUNDS;
  case 2:
   start_error(PROGRAM_TYPE_SYSTEM, -1, -1);
   error_string("\nSystem program failed to create process (invalid end address: ");
   error_number(end_address);
   error_string(").");
   return MRES_NEW_FAIL_END_BOUNDS;
 }

 struct basic_proc_propertiesstruct prop;



 int basic_properties = get_basic_proc_properties_from_bcode(&prop, &source_bcode->op [start_address], 0);

 switch(basic_properties)
 {
  case PROC_PROPERTIES_SUCCESS:
   break;
  case PROC_PROPERTIES_FAILURE_PROGRAM_TYPE:
   start_error(PROGRAM_TYPE_SYSTEM, -1, -1);
   error_string("\nSystem program failed to create process (invalid program type: ");
   error_number(prop.type);
   error_string(").");
   return MRES_NEW_FAIL_TYPE;
  case PROC_PROPERTIES_FAILURE_SHAPE:
   start_error(PROGRAM_TYPE_SYSTEM, -1, -1);
   error_string("\nSystem program failed to create process (invalid shape: ");
   error_number(prop.shape);
   error_string(").");
   return MRES_NEW_FAIL_SHAPE;
  case PROC_PROPERTIES_FAILURE_SIZE:
   start_error(PROGRAM_TYPE_SYSTEM, -1, -1);
   error_string("\nSystem program failed to create process (invalid size: ");
   error_number(prop.size);
   error_string(").");
   return MRES_NEW_FAIL_SIZE;
 }

// we may just be checking the proc's data/irpt costs:
 switch(mb [MBANK_SY_PLACE_STATUS])
 {
  case MSTATUS_SY_PLACE_CALL:
  case MSTATUS_SY_PLACE_CALL_TEST:
  case MSTATUS_SY_PLACE_T_CALL:
  case MSTATUS_SY_PLACE_T_CALL_TEST:
   break; // carry on

  case MSTATUS_SY_PLACE_COST_DATA:
  case MSTATUS_SY_PLACE_T_COST_DATA:
   mb [MBANK_SY_PLACE_STATUS] = prop.data_cost;
   return MRES_NEW_TEST_SUCCESS;

  case MSTATUS_SY_PLACE_COST_IRPT:
  case MSTATUS_SY_PLACE_T_COST_IRPT:
   mb [MBANK_SY_PLACE_STATUS] = prop.irpt_cost;
   return MRES_NEW_TEST_SUCCESS;

  default:
   return MRES_NEW_FAIL_STATUS;

 }

 int player_index = mb [MBANK_SY_PLACE_INDEX];

 if (player_index < 0
  || player_index >= w.players)
 {
  start_error(PROGRAM_TYPE_SYSTEM, -1, -1);
  error_string("\nSystem program failed to create process (invalid player index ");
  error_number(mb [MBANK_SY_PLACE_INDEX]);
  error_string(").");
  return MRES_NEW_FAIL_PLAYER;
 }

// now make sure we can make more processes:
 if (w.player[player_index].processes >= w.procs_per_team)
 {
   start_error(PROGRAM_TYPE_SYSTEM, -1, -1);
   error_string("\nSystem program failed to create process (player ");
   error_number(player_index);
   error_string(" has too many processes).");
   return MRES_NEW_FAIL_TOO_MANY_PROCS;
 }

// now work out and check the proc's x/y values:
 al_fixed x = al_itofix((int) mb [MBANK_SY_PLACE_X]);
 al_fixed y = al_itofix((int) mb [MBANK_SY_PLACE_Y]);

// just make sure that the proc is not so close to the edge that a check of the 3x3 blocks centred on it will result in checking an out-of-bounds block
// to do this we make sure it's at least 1<<BLOCK_SIZE_BITSHIFT from the edge
// check_notional_solid_block_collision() below will make sure it isn't within the solid edge blocks
 if (x < al_itofix(BLOCK_SIZE_PIXELS + 10)
  || y < al_itofix(BLOCK_SIZE_PIXELS + 10)
  || x > (w.w_fixed - al_itofix(BLOCK_SIZE_PIXELS + 10))
  || y > (w.h_fixed - al_itofix(BLOCK_SIZE_PIXELS + 10)))
 {
   start_error(PROGRAM_TYPE_SYSTEM, -1, -1);
   error_string("\nSystem failed to create process (location ");
   error_number(al_fixtoi(x));
   error_string(", ");
   error_number(al_fixtoi(y));
   error_string("; world size ");
   error_number(w.w_pixels);
   error_string(", ");
   error_number(w.h_pixels);
   error_string(").");
// unlike the proc version (which should never happen and is a fatal error) this is just a program failure
//   fprintf(stdout, "\ntest_sy_place_bcode(): attempted to create proc outside of world at (%i, %i) (world size %i, %i)", x, y, w.w_pixels, w.h_pixels);
   return MRES_NEW_FAIL_LOCATION;
 }

 al_fixed new_angle = short_angle_to_fixed(mb [MBANK_SY_PLACE_ANGLE]);

// finally make sure the new proc would fit:
 if (check_notional_block_collision_multi(prop.shape, prop.size, x, y, new_angle)
  || check_notional_solid_block_collision(prop.shape, prop.size, x, y, new_angle))
 {
  return MRES_NEW_FAIL_OBSTACLE;
 }

// TO DO: check_notional_block_collision_multi() will not detect collisions if two procs overlap without any vertices colliding!
//  Possible solution: add a special new shape_dat vertex set which includes internal collision points?

// if it was a test, stop here:
 if (mb [MBANK_SY_PLACE_STATUS] == MSTATUS_SY_PLACE_CALL_TEST
  || mb [MBANK_SY_PLACE_STATUS] == MSTATUS_SY_PLACE_T_CALL_TEST)
 {
  return MRES_NEW_TEST_SUCCESS;
 }

// now start creating the new proc:

 int p = find_empty_proc(player_index); // note that find_empty_proc doesn't bounds-check the player index

 if (p == -1)
 {
// not sure this is possible, as the checks above should have caught anything preventing find_empty_proc from working
   return MRES_NEW_FAIL_TOO_MANY_PROCS;
 }

 struct procstruct* new_pr = &w.proc[p];

 initialise_proc(p); // this clears the proc's procstruct so it can be used. It doesn't set anything that would be a problem if the proc creation fails below

 new_pr->player_index = player_index;

// copy bcode
 int i, j;
 j = 0;
 for (i = start_address; i < (end_address + 1); i ++)
 {
  new_pr->bcode.op [j] = source_bcode->op [i];
  j++;
 }
// zero out the rest of new_pr's bcode:
 while (j < PROC_BCODE_SIZE)
 {
  new_pr->bcode.op [j] = 0;
  j++;
 };

 if (!derive_proc_properties_from_bcode(new_pr, &prop, -1))
 {
  start_error(PROGRAM_TYPE_SYSTEM, -1, -1);
  error_string("\nSystem program failed to create process."); // (invalid interface).");
  return MRES_NEW_FAIL_INTERFACE;
 }

 if (!create_single_proc(p, x, y, new_angle))
 {
//  new_proc_fail_cloud(x, y, new_angle, new_proc_shape, new_proc_size);
  return MRES_NEW_FAIL_INTERFACE;
 }

 finish_adding_new_proc(p);

// at this point the proc has been created and this function should not be able to fail.

 mb [MBANK_SY_PLACE_STATUS] = p;

 return MRES_NEW_SUCCESS;

}

/*
// Checks whether a new proc can be placed by a program, with the new proc's bcode coming from part of the program's bcode
// cl is system program
// mb is pointer to first element of placing method's mbank
// assumes mb is valid (including that there's enough space in the mbank array to read all values from it)
// returns MSTATUS (will be MSTATUS_SY_PLACE_TEST_SUCCESS on success, otherwise an error status)
int test_sy_place_from_bcode(struct programstruct* cl, short* mb)
{

 int start_address = mb [MBANK_SY_PLACE_ADDRESS1];
 int end_address = mb [MBANK_SY_PLACE_ADDRESS2];

 int retval = check_start_end_addresses(start_address, end_address);

 switch(retval)
 {
  case 0: break; // success
  case 1: return MSTATUS_SY_PLACE_FAIL_START;
  case 2: return MSTATUS_SY_PLACE_FAIL_END;
 }

 return test_sy_place(&cl->bcode, start_address, end_address, mb);

}
*/



/*
// like test_sy_place_from_bcode, but the proc's bcode comes from a template.
// there is currently no way to use a subprocess of a program in a template
// returns MSTATUS (will be MSTATUS_SY_PLACE_TEST_SUCCESS on success, otherwise an error status)
int test_sy_place_from_template(short* mb)
{

// int t = mb [MBANK_SY_PLACE_ADDRESS1];
 int player_index = mb [MBANK_SY_PLACE_ADDRESS1];
 int templ_index = mb [MBANK_SY_PLACE_ADDRESS2];

 int t = find_template_access_index(player_index, templ_index);

 if (t == -1)
  return MSTATUS_SY_PLACE_FAIL_TEMPLATE;

 if (templ[t].type == TEMPL_TYPE_CLOSED
  || templ[t].loaded == 0
  || templ[t].type != TEMPL_TYPE_PROC) // not possible?
//   && templ[t].type != TEMPL_TYPE_FIRST_PROC))
    return MSTATUS_SY_PLACE_FAIL_TEMPLATE;

 return test_sy_place(&templ[t].bcode, 0, BCODE_MAX - 1, mb);

}
*/

/*

// checks whether a proc can be created from specified bcode by a system program's place method
// assumes:
//  - bcode is valid
//  - start_address and end_address are within bounds
//  - start_address and end_address are such that there will be a full bcode header in bcode
//  - there's enough space in mb for relevant mbank values to be read in from it
// returns MSTATUS (will be MSTATUS_SY_PLACE_TEST_SUCCESS on success, otherwise an error status)
int test_sy_place(struct bcodestruct* bcode, int start_address, int end_address, short* mb)
{

 if (bcode->op [start_address + BCODE_HEADER_ALL_TYPE] != PROGRAM_TYPE_PROCESS)
  return MSTATUS_SY_PLACE_FAIL_TYPE;

 int proc_shape = bcode->op [start_address + BCODE_HEADER_PROC_SHAPE];
 int proc_size = bcode->op [start_address + BCODE_HEADER_PROC_SIZE];

 int retval = check_proc_shape_size(proc_shape, proc_size);

 switch(retval)
 {
  case 0: break; // success
  case 1: return MSTATUS_SY_PLACE_FAIL_SHAPE;
  case 2: return MSTATUS_SY_PLACE_FAIL_SIZE;
 }

// currently no check for irpt/data cost for SY_PLACE method

 if (mb [MBANK_SY_PLACE_INDEX] < 0
  || mb [MBANK_SY_PLACE_INDEX] >= w.players)
 {
  start_error(PROGRAM_TYPE_SYSTEM, -1, -1);
  error_string("\nSystem failed to create new process (invalid player index ");
  error_number(mb [MBANK_SY_PLACE_INDEX]);
  error_string(").");
  return MSTATUS_SY_PLACE_FAIL_INTERFACE;
 }

// now we can assume that mb [MBANK_SY_PLACE_INDEX] is a valid player
 int p = find_empty_proc(mb [MBANK_SY_PLACE_INDEX]);
// find_empty proc just confirms that there's an empty procstruct that can be placed; it doesn't do anything to create the proc, so we can call it in testing

 if (p == -1)
 {
  return MSTATUS_SY_PLACE_FAIL_TOO_MANY_PROCS; // failed
 }

// derive_proc_properties_from_bcode is not called here as there's no point doing all of that work for a test.
//  and also because all of derive_proc_properties_from_bcode's failure modes should be checked here as well.

// now check the proc's x/y values:
 al_fixed x = al_itofix((int) mb [MBANK_SY_PLACE_X]);
 al_fixed y = al_itofix((int) mb [MBANK_SY_PLACE_Y]);

// just make sure that the proc is not so close to the edge that a check of the 3x3 blocks centred on it will result in checking an out-of-bounds block
// to do this we make sure it's at least 1<<BLOCK_SIZE_BITSHIFT from the edge
// check_notional_solid_block_collision() below will make sure it isn't within the solid edge blocks
 if (x < al_itofix(BLOCK_SIZE_PIXELS + 10)
  || y < al_itofix(BLOCK_SIZE_PIXELS + 10)
  || x > (w.w_fixed - al_itofix(BLOCK_SIZE_PIXELS + 10))
  || y > (w.h_fixed - al_itofix(BLOCK_SIZE_PIXELS + 10)))
 {
   start_error(PROGRAM_TYPE_SYSTEM, -1, -1);
   error_string("\nSystem failed to create new process (location ");
   error_number(al_fixtoi(x));
   error_string(", ");
   error_number(al_fixtoi(y));
   error_string("; world size ");
   error_number(w.w_pixels);
   error_string(", ");
   error_number(w.h_pixels);
   error_string(").");
// unlike the proc version (which should never happen and is a fatal error) this is just a program failure
//   fprintf(stdout, "\ntest_sy_place_bcode(): attempted to create proc outside of world at (%i, %i) (world size %i, %i)", x, y, w.w_pixels, w.h_pixels);
   return MSTATUS_SY_PLACE_FAIL_LOCATION;
 }

// finally make sure the new proc would fit:
 if (check_notional_block_collision_multi(proc_shape, proc_size, x, y, mb [MBANK_SY_PLACE_ANGLE])
  || check_notional_solid_block_collision(proc_shape, proc_size, x, y, mb [MBANK_SY_PLACE_ANGLE]))
 {
  return MSTATUS_SY_PLACE_FAIL_OBSTACLE;
 }

// TO DO: check_notional_block_collision_multi() will not detect collisions if two procs overlap without any vertices colliding!
//  Possible solution: add a special new shape_dat vertex set which includes internal collision points?

// I think at this point the test is a success - we can return a success value and the caller can assume that the proc will be able to be created.

 return MSTATUS_SY_PLACE_TEST_SUCCESS;

}
*/

/*

// places a new proc from METH_SY_PLACE, with the new proc's bcode coming from part of the program's bcode
// cl is system program
// mb is pointer to first element of placing method's mbank
// assumes mb is valid (including that there's enough space in the mbank array to read all values from it)
// doesn't assume proc can be placed - called test_sy function to confirm this
// returns MSTATUS (will be MSTATUS_SY_PLACE_TEST_SUCCESS on success, otherwise a failure status)
int sy_place_from_bcode(struct programstruct* cl, short* mb)
{

 int retval = test_sy_place_from_bcode(cl, mb);

 if (retval != MSTATUS_SY_PLACE_TEST_SUCCESS)
  return retval;

// call to test_sy_place_from_bcode means that we can assume start_address, end_address and player index are valid, and that find_empty_proc will be successful
 int start_address = mb [MBANK_SY_PLACE_ADDRESS1];
 int end_address = mb [MBANK_SY_PLACE_ADDRESS2];

 return sy_place(&cl->bcode, start_address, end_address, mb);

}
*/

/*
// a cross between sy_place_from_bcode
int sy_place_from_template(short* mb)
{

 int retval = test_sy_place_from_template(mb);

 if (retval != MSTATUS_SY_PLACE_TEST_SUCCESS)
  return retval;

 int t = find_template_access_index(mb [MBANK_SY_PLACE_ADDRESS1], mb [MBANK_SY_PLACE_ADDRESS2]);

 if (t == -1)
  return MSTATUS_SY_PLACE_FAIL_TEMPLATE;

// call to test_sy_place_from_template means we can assume the template is valid
 return sy_place(&templ[t].bcode, 0, BCODE_MAX - 1, mb);

}

*/

/*

int sy_place(struct bcodestruct* source_bcode, int start_address, int end_address, short* mb)
{

 int p = find_empty_proc(mb [MBANK_SY_PLACE_INDEX]);

Need to consolidate sy_place functions similarly to pr new method functions



 struct procstruct* new_pr = &w.proc[p];

 initialise_proc(p); // this clears the proc's procstruct so it can be used. It doesn't set anything that would be a problem if the proc creation fails below

 new_pr->player_index = mb [MBANK_SY_PLACE_INDEX];

// copy bcode
 int i, j;
 j = 0;
 for (i = start_address; i < (end_address + 1); i ++)
 {
  new_pr->bcode.op [j] = source_bcode->op [i];
  j++;
 }
// zero out the rest of new_pr's bcode:
 while (j < BCODE_MAX)
 {
  new_pr->bcode.op [j] = 0;
  j++;
 };

 struct basic_proc_propertiesstruct bprop;
 int basic_properties = get_basic_proc_properties_from_bcode(&bprop, &new_pr->bcode.op [0]);

  switch(basic_properties)
 {
  case PROC_PROPERTIES_SUCCESS:
   break;
  case PROC_PROPERTIES_FAILURE_PROGRAM_TYPE:
   start_error(PROGRAM_TYPE_SYSTEM, -1, -1);
   error_string("\nSystem program failed to create new process (invalid program type: ");
   error_number(bprop->type);
   error_string(").");
   mb [MBANK_SY_PLACE_STATUS] = MSTATUS_SY_PLACE_FAIL_TYPE;
   return -1;
  case PROC_PROPERTIES_FAILURE_SHAPE:
   start_error(PROGRAM_TYPE_SYSTEM, -1, -1);
   error_string("\nSystem program failed to create new process (invalid shape: ");
   error_number(bprop->shape);
   error_string(").");
   mb [MBANK_SY_PLACE_STATUS] = MSTATUS_SY_PLACE_FAIL_SHAPE;
   return -1;
  case PROC_PROPERTIES_FAILURE_SIZE:
   start_error(PROGRAM_TYPE_SYSTEM, -1, -1);
   error_string("\nSystem program failed to create new process (invalid size: ");
   error_number(bprop->size);
   error_string(").");
   mb [MBANK_SY_PLACE_STATUS] = MSTATUS_SY_PLACE_FAIL_SIZE;
   return -1;
 }


 if (!derive_proc_properties_from_bcode(new_pr, &bprop, -1))
 {
  start_error(PROGRAM_TYPE_SYSTEM, -1, -1);
  error_string("\nSystem program failed to create new process (invalid interface).");
  return MSTATUS_SY_PLACE_FAIL_INTERFACE;
 }

// call to test_sy_place_from_bcode means that we can assume x, y and angle values are okay (although create_single_proc() should check these anyway)
// new_proc is called from create_single_proc (if it's successful)
 if (!create_single_proc(p, al_itofix(mb [MBANK_SY_PLACE_X]), al_itofix(mb [MBANK_SY_PLACE_Y]), short_angle_to_fixed(mb [MBANK_SY_PLACE_ANGLE])))
 {
//  fprintf(stdout, "\n * sy_place failed: MSTATUS_SY_PLACE_FAIL_OBSTACLE");
  start_error(PROGRAM_TYPE_SYSTEM, -1, -1);
  error_string("\nSystem program failed to create new process (collision).");
  return MSTATUS_SY_PLACE_FAIL_OBSTACLE;
 }

 finish_adding_new_proc(p);

 mb [MBANK_SY_PLACE_INDEX] = p;

 return MSTATUS_SY_PLACE_SUCCESS;

}
*/


// runs the method that copies bcode from the system program itself to a template
// returns 1 success/0 failure
int sy_copy_to_template(s16b* mb)
{

// fprintf(stdout, "\n sy_copy_to_template: mb {%i, %i, %i, %i}", mb [0], mb [1], mb [2], mb [3]);

 int t;

  if (mb [MBANK_SY_TEMPLATE_START] < 0
   || mb [MBANK_SY_TEMPLATE_START] >= SYSTEM_BCODE_SIZE - BCODE_HEADER_ALL_END)
  {
   start_error(PROGRAM_TYPE_SYSTEM, -1, -1);
   error_string("\nFailed: bcode to template (invalid start address ");
   error_number(mb [MBANK_SY_TEMPLATE_START]);
   error_string(").");
   return 0;
  }

// first we work out which template we need to target:
 int find_template_type = mb [MBANK_SY_TEMPLATE_TYPE]; // bounds-checked below

 if (find_template_type < 0
  || find_template_type >= FIND_TEMPLATES
  || w.template_reference [find_template_type] == -1)
 {
   start_error(PROGRAM_TYPE_SYSTEM, -1, -1);
   error_string("\nFailed: bcode to template (invalid template type ");
   error_number(mb [MBANK_SY_TEMPLATE_TYPE]);
   error_string(").");
   return 0;
 }

 t = w.template_reference [find_template_type];

// at this stage we should be able to assume t is valid

 clear_template(t);

 if (!check_program_type(w.system_program.bcode.op [mb [MBANK_SY_TEMPLATE_START] + BCODE_HEADER_ALL_TYPE], expected_program_type(templ[t].type)))
 {
   start_error(PROGRAM_TYPE_SYSTEM, -1, -1);
   error_string("\nFailed: bcode to template (invalid program type ");
   error_number(w.system_program.bcode.op [mb [MBANK_SY_TEMPLATE_START] + BCODE_HEADER_ALL_TYPE]);
   error_string("; expected ");
   error_number(expected_program_type(templ[t].type));
   error_string(".");
//  fprintf(stdout, "\n cleared template %i", t);
  return 0;
 }

// fprintf(stdout, "\n copying bcode to template %i", t);
 int retval = copy_bcode_to_template(t, &w.system_program.bcode, PROGRAM_TYPE_SYSTEM, -1, TEMPL_ORIGIN_SYSTEM, mb [MBANK_SY_TEMPLATE_START], mb [MBANK_SY_TEMPLATE_END], mb [MBANK_SY_TEMPLATE_NAME]);
// copy_bcode_to_template does bounds-checking on the start and end addresses

/*

*** not done any more. Now we leave this up to the system program:

// when an observer, client or operator template is copied from bcode, it replaces the existing program.
// this isn't necessary for proc templates, as they can only be used in future to create new procs
 if (retval == 1)
 {
  switch(templ[t].type)
  {
   case TEMPL_TYPE_OBSERVER:
    return copy_template_to_program(t, &w.observer_program, PROGRAM_TYPE_OBSERVER, -1);
   case TEMPL_TYPE_DELEGATE:
    return copy_template_to_program(t, &w.player[templ[t].player].client_program, PROGRAM_TYPE_DELEGATE, templ[t].player);
   case TEMPL_TYPE_OPERATOR:
//    fprintf(stdout, "\n loading operator into template %i for player %i", t, templ[t].player);
//    error_call();
    return copy_template_to_program(t, &w.player[templ[t].player].client_program, PROGRAM_TYPE_OPERATOR, templ[t].player);
  }
 }
*/

 return retval;

}




int run_sy_modify_process_method(s16b* mb)
{

 int proc_index = mb [MBANK_SY_MODIFY_PROC_INDEX];

 if (proc_index < 0
  || proc_index >= w.max_procs)
  return 0;

 struct procstruct* pr = &w.proc[proc_index];

 if (pr->exists <= 0)
  return 0;

 int new_value = mb [MBANK_SY_MODIFY_PROC_VALUE];

 switch(mb [MBANK_SY_MODIFY_PROC_STATUS])
 {

  case MSTATUS_SY_MODIFY_PROC_HP:
   if (new_value <= 0)
    new_value = 1;
   if (new_value > pr->hp_max)
     new_value = pr->hp_max;
   pr->hp = new_value;
   return 1;
  case MSTATUS_SY_MODIFY_PROC_IRPT:
   if (new_value < 0)
    new_value = 0;
   if (new_value > *(pr->irpt_max))
     new_value = *(pr->irpt_max);
   *(pr->irpt) = new_value;
   return 1;
  case MSTATUS_SY_MODIFY_PROC_DATA:
   if (new_value < 0)
    new_value = 0;
   if (new_value > *(pr->data_max))
     new_value = *(pr->data_max);
   *(pr->data) = new_value;
   return 1;
  case MSTATUS_SY_MODIFY_PROC_MB: // uses new_value as mbank index
   if (new_value < 0
    || new_value >= METHOD_BANK)
     return 0;
   pr->regs.mbank [new_value] = mb [MBANK_SY_MODIFY_PROC_VALUE2]; // don't need to bounds-check the value being copied
   return 1;
  case MSTATUS_SY_MODIFY_PROC_BCODE: // uses new_value as bcode index
   if (new_value < 0
    || new_value >= PROC_BCODE_SIZE)
     return 0;
   pr->bcode.op [new_value] = mb [MBANK_SY_MODIFY_PROC_VALUE2]; // don't need to bounds-check the value being copied
   return 1;
  case MSTATUS_SY_MODIFY_PROC_INSTANT: // just increases lifetime to 16 to prevent spawning animation being shown
   if (pr->lifetime < 16)
    pr->lifetime = 16;
   return 1;
 }

 return 0;

}


int run_sy_manage_method(s16b* mb)
{

     switch(mb[0])
     {
      case MSTATUS_SY_MANAGE_PAUSED: // - returns 0 if not soft-paused, 1 if soft-paused
       if (game.pause_soft == 1)
        return 1;
       return 0; // system program can't access hard pause because hard pause means programs don't run
      case MSTATUS_SY_MANAGE_PAUSE_SET: // soft pause
       if (game.phase > GAME_PHASE_PREGAME)
        return 0; // can only manage pause during world/pregame phase
       if (mb [1] < 0 || mb [1] > 1)
        return 0;
       game.pause_soft = mb [1];
       return 1;
      case MSTATUS_SY_MANAGE_FF: // - returns 0 if not fast-forwarding, 1 if fast-forwarding
       if (game.fast_forward != FAST_FORWARD_OFF)
        return 1;
       return 0; // can call this during pregame but it doesn't affect anything (as pregame is always fast-forwarded)
      case MSTATUS_SY_MANAGE_FF_SET: // sets fast-forward to 0 or 1
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
      case MSTATUS_SY_MANAGE_PHASE: // - returns 0 if in world phase, 1 if in interturn phase, 2 if in pregame phase, 3 if game over phase
       return game.phase;
      case MSTATUS_SY_MANAGE_OB_SET: // - allows communication with obs/op program. 64 registers. fields: 1-register 2-value. returns 1 on success, 0 on failure.
       if (mb [1] < 0 || mb [1] >= SYSTEM_COMS)
        return 0;
       w.system_com [mb [1]] = mb [2];
       return 1;
      case MSTATUS_SY_MANAGE_OB_READ: // - inverse of SYSTEM_SET. returns value.
       if (mb [1] < 0 || mb [1] >= SYSTEM_COMS)
        return 0;
       return w.system_com [mb [1]];
      case MSTATUS_SY_MANAGE_TURN: // - returns which turn we're in
       return game.current_turn;
      case MSTATUS_SY_MANAGE_TURNS: // - returns how many turns in game
       return game.turns;
      case MSTATUS_SY_MANAGE_SECONDS_LEFT: // - returns time left in turn (in seconds)
       return game.current_turn_time_left / TICKS_TO_SECONDS_DIVISOR; // this is approximate. can't use *0.03 because we're avoiding floating point stuff in non-display game code
      case MSTATUS_SY_MANAGE_TICKS_LEFT: // - returns time left in turn in ticks (returns 32767 if overflow)
       if (game.current_turn_time_left >= 32767)
        return 32767;
         else
          return game.current_turn_time_left;
      case MSTATUS_SY_MANAGE_TURN_SECONDS: // - returns time for each turn (in seconds)
       return game.minutes_each_turn * 60;
      case MSTATUS_SY_MANAGE_GAMEOVER: //  field: 1-ending status (GAME_END_? enum), 2-value (used by some GAME_END statuses)
       if (game.phase == GAME_PHASE_OVER)
        return 0;
// Calling this doesn't actually end the game until after the system program has finished executing.
// Note that in the game over phase the game is in soft pause, so the system/observer/operator can still run.
// Once game over phase has been initiated, there's no way to restart the game.
       game.phase = GAME_PHASE_OVER;
       if (mb [1] < 0 || mb [1] >= GAME_END_STATES)
       {
        game.game_over_status = GAME_END_ERROR_STATUS;
        game.game_over_value = mb [2]; // this value just used to display error message
        return 0;
       }
       game.game_over_status = mb [1];
       switch(mb[1])
       {
        case GAME_END_PLAYER_WON:
        case GAME_END_PLAYER_LOST:
// for these end statuses, mb[2] indicates which player lost or won
         if (mb[2] < 0 || mb[2] >= w.players)
         {
          game.game_over_status = GAME_END_ERROR_PLAYER;
          game.game_over_value = mb [2]; // this value just used to display error message
          return 0;
         }
         game.game_over_value = mb [2]; // this is the player who won or lost
         break;
       } // end switch for field 1
       return 1; // end MSTATUS_SY_MANAGE_GAMEOVER
      case MSTATUS_SY_MANAGE_END_PREGAME:
       game.pregame_time_left = 0;
       break;
      case MSTATUS_SY_MANAGE_END_TURN:
       game.force_turn_finish = 1;
       break;

      case MSTATUS_SY_MANAGE_START_TURN:
       switch(game.phase)
       {
        case GAME_PHASE_PREGAME:
         start_new_turn();
         return 1;
        case GAME_PHASE_TURN:
         start_new_turn();
         return 1;
       }
       return 0;

      case MSTATUS_SY_MANAGE_LOAD_OBSERVER: // loads observer from template into program
       if (w.template_reference [FIND_TEMPLATE_OBSERVER] == -1
        || templ[w.template_reference [FIND_TEMPLATE_OBSERVER]].contents.loaded == 0)
        return 0; // just fail. Let system program deal with any consequences of failure (probably just wants to ignore it)
       return copy_template_to_program(w.template_reference [FIND_TEMPLATE_OBSERVER], &w.observer_program, PROGRAM_TYPE_OBSERVER, -1);

      case MSTATUS_SY_MANAGE_LOAD_CLIENT: // loads player client from template into program
       if (mb [1] < 0 || mb [1] >= w.players)
        return 0;
       int t = 0;
       switch (mb [1])
       {
        case 0: t = w.template_reference [FIND_TEMPLATE_P0_CLIENT]; break;
        case 1: t = w.template_reference [FIND_TEMPLATE_P1_CLIENT]; break;
        case 2: t = w.template_reference [FIND_TEMPLATE_P2_CLIENT]; break;
        case 3: t = w.template_reference [FIND_TEMPLATE_P3_CLIENT]; break;
       }
       if (w.player_operator == mb [1])
        return copy_template_to_program(t, &w.player[mb [1]].client_program, PROGRAM_TYPE_OPERATOR, mb [1]);
         else
          return copy_template_to_program(t, &w.player[mb [1]].client_program, PROGRAM_TYPE_DELEGATE, mb [1]);

						case MSTATUS_SY_MANAGE_ALLOCATE_EFFECT:
       if (mb [1] < 0 || mb [1] >= w.players)
        return 0;
       if (mb [2] < 0 || mb [2] >= ALLOCATE_EFFECT_TYPES)
								return 0;
							w.player[mb[1]].allocate_effect_type = mb [2];
							return 1;

      default:
       return 0;
    }


 return 1;

}
