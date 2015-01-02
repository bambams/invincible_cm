#include <allegro5/allegro.h>

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#include "m_config.h"

#include "g_misc.h"
#include "g_header.h"

#include "g_client.h"
#include "g_group.h"
#include "g_motion.h"
#include "c_header.h"
#include "e_slider.h"
#include "e_header.h"
#include "e_log.h"
#include "s_setup.h"

#include "m_maths.h"

extern const struct mtypestruct mtype [MTYPES]; // in g_method.c


// call this function to initialise a programstruct before code is copied or compiled into it
void init_program(struct programstruct* cl, int type, int player_index)
{

 cl->active = 0;
 cl->type = type;
 cl->player = player_index; // will be -1 if this program isn't a player client/operator

 int i, j;

 for (i = 0; i < METHODS + 1; i ++)
 {
  cl->method [i].type = MTYPE_END;

  for (j = 0; j < METHOD_DATA_VALUES; j ++)
  {
   cl->method[i].data[j] = 0;
  }
 }

 for (i = 0; i < cl->bcode.bcode_size; i ++)
 {
  cl->bcode.op [i] = 0;
 }

 for (i = 0; i < METHOD_BANK; i ++)
 {
  cl->regs.mbank[i] = 0;
 }

 for (i = 0; i < REGISTERS; i ++)
 {
  cl->regs.reg [i] = 0;
 }

 cl->irpt = 0;
 cl->instr_left = 0;

 switch(type)
 {
  case PROGRAM_TYPE_SYSTEM:
   cl->available_irpt = PROGRAM_IRPT_SYSTEM;
   cl->available_instr = PROGRAM_INSTRUCTIONS_SYSTEM;
   break;
  case PROGRAM_TYPE_DELEGATE:
   cl->available_irpt = PROGRAM_IRPT_DELEGATE;
   cl->available_instr = PROGRAM_INSTRUCTIONS_DELEGATE;
   break;
  case PROGRAM_TYPE_OBSERVER:
   cl->available_irpt = PROGRAM_IRPT_OBSERVER;
   cl->available_instr = PROGRAM_INSTRUCTIONS_OBSERVER;
   break;
  case PROGRAM_TYPE_OPERATOR:
   cl->available_irpt = PROGRAM_IRPT_OPERATOR;
   cl->available_instr = PROGRAM_INSTRUCTIONS_OPERATOR;
   break;
 }

}

// assumes the name follows "a"
const char *a_program_type_name [PROGRAM_TYPES] =
{
" process", // PROGRAM_TYPE_PROCESS,
" delegate", // PROGRAM_TYPE_DELEGATE,
"n operator", // PROGRAM_TYPE_OPERATOR,
"n observer", // PROGRAM_TYPE_OBSERVER,
" system", // PROGRAM_TYPE_SYSTEM,
};

/*
This function is based on derive_proc_properties_from_bcode() in g_proc.c,
 but is used for program types other than procs (e.g. clients, observers etc)
Call this function after the program's bcode has been written.
It sets up various things based on the interface data in the bcode.

Assumes expect_program_type is valid

returns 1 on success, 0 on failure
on failure it also sends an error message to the mlog
*/
int derive_program_properties_from_bcode(struct programstruct* cl, int expect_program_type)
{

 int bcode_pos;

 bcode_pos = BCODE_HEADER_ALL_TYPE; // start of interface definition in bcode, ignoring the initial jump instruction
// note that bcode_pos isn't bounds-checked in this function; we assume that bcode is at least large enough for the interface definition (otherwise nothing would work)

 cl->bcode.type = cl->bcode.op [bcode_pos];

 if (!check_program_type(cl->bcode.type, expect_program_type))
  return 0;

 if (cl->bcode.type == PROGRAM_TYPE_PROCESS)
 {
  fprintf(stdout, "\ng_client.c: derive_program_properties_from_bcode(): program type should not be PROGRAM_TYPE_PROCESS");
   // TO DO: need to direct these messages appropriately (probably to the console - should probably allow client program to direct these kinds of errors)
  return 0;
 }

 bcode_pos++; // move on from program type...

// system programs have a much more detailed interface header, containing initialisation information about the world (the init information is fed into the world_pre_init which is used to build a world setup menu)
 if (cl->bcode.type == PROGRAM_TYPE_SYSTEM)
 {
  bcode_pos = derive_pre_init_from_system_program(cl, bcode_pos); // in s_setup.c
  if (bcode_pos == -1)
   return 0; // failed for some reason (not sure this is actually possible)
 }

// now read in method definitions
 int i;
 int type;
 int large_method_counter = 0;

 for (i = 0; i < METHODS; i ++)
 {
  large_method_counter --;
// check if we're making space for a method that takes up multiple method slots:
  if (large_method_counter > 0)
  {
   cl->method[i].type = MTYPE_NONE;
   bcode_pos += INTERFACE_METHOD_DATA;
   continue;
  }

  type = cl->bcode.op [bcode_pos];

// first make sure the method type is valid and there's enough space for it in the mbank:
  if (!check_method_type(type, expect_program_type, 0)
   || ((i + mtype[type].mbank_size) * METHOD_SIZE > METHOD_BANK))
  {
//   fprintf(stdout, "\nMethod error: i %i type %i size %i (%i, %i)", i, type, mtype[type].mbank_size, (i + mtype[type].mbank_size) * METHOD_SIZE, METHOD_BANK);
   cl->method[i].type = MTYPE_ERROR_INVALID;
   cl->method[i].data [M_IDATA_ERROR_TYPE] = type;
   bcode_pos += INTERFACE_METHOD_DATA;
   large_method_counter = mtype[MTYPE_ERROR_INVALID].mbank_size;
   continue;
  }

  cl->method[i].type = type;

// now go through the interface method data.
// this does different things depending on the type of method it is
//  bcode_pos++; // don't need this - INTERFACE_METHOD_DATA includes the type, and M_IDATA_ACCEL_RATE etc. start at 1 to allow for it

// no external method code here (as client methods can't be external; that would be silly)

// now fill in the remaining data:
/*  switch(type)
  {
// not currently used
  }*/

  if (cl->method[i].type != MTYPE_ERROR_INVALID)
  {
   bcode_pos += INTERFACE_METHOD_DATA;
   large_method_counter = mtype[type].mbank_size;
  }
 }

// now add MTYPE_CL_END at the end of the list of methods:
 i = METHODS; // this is METHODS, and not METHODS-1, because the method array has a spare element at the end for the final MTYPE_CL_END if all other methods are full.
 cl->method[METHODS].type = MTYPE_END;
 while(TRUE)
 {
  if (i == 0)
  {
   cl->method[i].type = MTYPE_END;
   break;
  }
  if ((cl->method[i].type == MTYPE_NONE
				|| cl->method[i].type == MTYPE_END)
   && (cl->method[i - 1].type != MTYPE_NONE
				|| cl->method[i - 1].type != MTYPE_END))
  {
   cl->method[i].type = MTYPE_END;
   break;
  }
  i --;
 };

 return 1;

}


// Call this program to check whether a program type (probably specified in a program's bcode) is a valid program type, and whether it is the expected type
// expect_program_type is the expected type
//  - note that some values of expect_program_type accept other types, although there may be a warning message
// writes all error/warning messages to mlog
// returns 1 success, 0 fail
//  on failure it also sends an error message to the mlog
// this function is called from derive_program_properties_from_bcode() above and also from template loading functions in t_template.c and g_method_ex.c
int check_program_type(int program_type, int expect_program_type)
{

 if (program_type < 0 || program_type >= PROGRAM_TYPES)
 {
  start_log_line(MLOG_COL_ERROR);
  write_to_log("Error: invalid program type (");
  write_number_to_log(program_type);
  write_to_log(") - check program's interface definition.");
  finish_log_line();
  return 0;
 }

// from this point we can assume that program_type is valid

 switch(expect_program_type)
 {
  case PROGRAM_TYPE_DELEGATE: // accepts PROGRAM_TYPE_DELEGATE only
   if (program_type != PROGRAM_TYPE_DELEGATE)
   {
    start_log_line(MLOG_COL_ERROR);
    write_to_log("Error: expected a delegate program, but this is a");
    write_to_log(a_program_type_name [program_type]);
    write_to_log(" program.");
    finish_log_line();
    return 0;
   }
   break;
  case PROGRAM_TYPE_OPERATOR: // accepts PROGRAM_TYPE_OPERATOR, PROGRAM_TYPE_CLIENT, PROGRAM_TYPE_OBSERVER
   switch(program_type)
   {
    case PROGRAM_TYPE_OPERATOR:
     break;
    case PROGRAM_TYPE_DELEGATE: // not an error
     start_log_line(MLOG_COL_EDITOR);
     write_to_log("Using a delegate program as an operator (observer functions may be unavailable)."); // TO DO: actually observer functions will probably not be available. Fix this.
     finish_log_line();
     break;
    case PROGRAM_TYPE_OBSERVER: // not an error
     start_log_line(MLOG_COL_EDITOR);
     write_to_log("Using an observer program as an operator (no client program will be used).");
     finish_log_line();
     break;
    default:
     start_log_line(MLOG_COL_ERROR);
     write_to_log("Error: expected an operator program, but this is a");
     write_to_log(a_program_type_name [program_type]);
     write_to_log(" program.");
     finish_log_line();
     return 0;
   }
   break;
  case PROGRAM_TYPE_OBSERVER:
   if (program_type != PROGRAM_TYPE_OBSERVER)
   {
    start_log_line(MLOG_COL_ERROR);
    write_to_log("Error: expected an observer program, but this is a");
    write_to_log(a_program_type_name [program_type]);
    write_to_log(" program.");
    finish_log_line();
    return 0;
   }
   break;
  case PROGRAM_TYPE_SYSTEM:
   if (program_type != PROGRAM_TYPE_SYSTEM)
   {
    start_log_line(MLOG_COL_ERROR);
    write_to_log("Error: expected a system program, but this is a");
    write_to_log(a_program_type_name [program_type]);
    write_to_log(" program.");
    finish_log_line();
    return 0;
   }
   break;
  case PROGRAM_TYPE_PROCESS:
   if (program_type != PROGRAM_TYPE_PROCESS)
   {
    start_log_line(MLOG_COL_ERROR);
    write_to_log("Error: expected a process program, but this is a");
    write_to_log(a_program_type_name [program_type]);
    write_to_log(" program.");
    finish_log_line();
    return 0;
   }
   break;

 } // end switch(expect_program_type)

 return 1; // success!

}

// checks whether a particular method can be used by a particular program type
//  - program_type can't be PROGRAM_TYPE_PROC (the check for procs is in g_proc and is simpler)
// checks validity of method_type and program_type (invalid program_type is a fatal error as this should never happen)
// loading accepts any method that a program could have (including sub and error methods)
// returns 1 valid, 0 invalid
int check_method_type(int method_type, int program_type, int loading)
{

// when loading the game, accept error methods (as they may be present as a result of errors in proc/program creation)
 if (loading
  && (method_type >= 0 && method_type < MTYPES)
  && (mtype[method_type].mclass == MCLASS_ERROR
   || mtype[method_type].mclass == MCLASS_ALL
   || mtype[method_type].mclass == MCLASS_END))
   return 1;

 if (method_type < 0 // should accept MTYPE_NONE (for methods that are the later parts of a large method)
  || method_type >= MTYPE_END)
  return 0;

 switch(program_type)
 {
  case PROGRAM_TYPE_DELEGATE:
   if (mtype[method_type].mclass == MCLASS_CL
    || mtype[method_type].mclass == MCLASS_CLOB
    || mtype[method_type].mclass == MCLASS_ALL)
     return 1;
   return 0;
  case PROGRAM_TYPE_OBSERVER:
   if (mtype[method_type].mclass == MCLASS_OB
    || mtype[method_type].mclass == MCLASS_CLOB
    || mtype[method_type].mclass == MCLASS_ALL)
     return 1;
   return 0;
  case PROGRAM_TYPE_OPERATOR:
   if (mtype[method_type].mclass == MCLASS_OB
    || mtype[method_type].mclass == MCLASS_CL
    || mtype[method_type].mclass == MCLASS_CLOB
    || mtype[method_type].mclass == MCLASS_ALL)
     return 1;
   return 0;
  case PROGRAM_TYPE_SYSTEM:
   if (mtype[method_type].mclass == MCLASS_SY
    || mtype[method_type].mclass == MCLASS_CL
    || mtype[method_type].mclass == MCLASS_OB
    || mtype[method_type].mclass == MCLASS_CLOB
    || mtype[method_type].mclass == MCLASS_ALL)
     return 1;
   return 0;

  default: // this should never happen (but the error message is useful for debugging)
   fprintf(stdout, "\nError: g_client.c: check_method_type(): invalid program type (%i) (method_type %i) (note: can't use check_method_type() for processes)", program_type, method_type);
   error_call();
   break;

 }


 return 0;

}


//working on the following plan for system program methods:

/*

What special methods will a system program need?

Note that system program methods are all called, and take effect immediately. Don't need to wait for the mbank values to be checked after execution.

The system program should also have access to some (all?) client, operator and observer methods

MTYPE_SY_PLACE_PROC, // when called, places a proc at a specified location
 - size 2
 - values:
  - mode
  - player owner
  - template
  - x
  - y
  - angle
  - x/y_speed?
 - modes:
  - test
  - actual
  - join test (this changes remaining values)
  - join actual
 - returns:
  - test mode: 1 on success, 0 on failure
  - actual mode: proc index on success, -1 on failure
 - if mode is join, the values are:
  - mode
  - template
  - proc to join
  - parent vertex
  - child vertex
  - child angle

MTYPE_SY_LOAD_TEMPLATE, // when called, loads a file into a specified template. loads files from the STANDARD_FILE list only.
 - size 1
 - values:
  - template
  - standard file index
 - returns 1 on success, 0 on template index bounds error, template type mismatch or file index bounds error
 - calls error_call() on failure to load or compile etc (as this should not be possible if file index is valid)

MTYPE_SY_IMPORT_TEMPLATE, // loads a subprocess of the system program to a specified template
 - size 1
 - values:
  - template index
  - start address
  - end address
 - returns 0 if error in values or if subprocess program type is wrong; otherwise 1
  *** remember to check subprocess program type

MTYPE_SY_CHANGE_PROC, // changes a proc in some way
 - size 1
 - values:
  - proc index
  - value to change
  - new value
 - value to change can be en, data, hp, speed etc.
 - maybe can change max values of these things too (considering that the system program should be omnipotent)
  *** remember to perform bounds checking!
 - values can also be x, y, angle, x_speed, y_speed etc.
  - if proc is a group member, move whole group so that proc's x, y, angle values are as specified
   - if don't want to move whole group, disconnect proc first

MTYPE_SY_CONNECTION, // causes a proc to join or leave a group
 - size 2
 - values:
  - mode
  - proc
 - mode can be connect or disconnect
 - if connect, remaining values are:
  - second proc
  - parent vertex
  - child vertex
  - angle
 - if disconnect:
  - vertex to disconnect
 - if either proc is already a group member, the groups are joined (unless too large)
 - returns 1/0 success/fail

MTYPE_SY_DESTROY_PROC, // removes proc from world
 - size 1
 - values:
  - mode
  - proc index
 - modes:
  - destroy proc only
  - destroy whole group




New client methods!!!
* indicates that this method already exists
^ indicates that it exists but will need to be changed somehow

// these all exist and are to some extent implemented:
*MTYPE_CL_COMMAND, // fields: mode (read/write), proc, command index, value (if write)

Methods available to both clients and observers (and also operators):
^MTYPE_CLOB_SELECT, // allows display of proc selection etc
^MTYPE_CLOB_QUERY, // same as client version
^MTYPE_CLOB_CHECK_POINT, // fields: x/y. Returns -1 on failure, proc index on success
MTYPE_CLOB_SCAN // similar to proc scan but can scan at any point. Possibly allow square scans as well.
MTYPE_CLOB_MATHS, // similar to proc version but lacks turn-towards
 - possibly need to separate basic maths method from a maths method that has modes that assume a proc is calling it
MTYPE_CLOB_MAP, // reads the map in various ways (block types etc)
MTYPE_CLOB_SETTINGS, // reads various settings (world size etc)

Observer methods:
MTYPE_OB_INPUT,
MTYPE_OB_VIEW, // interaction with user view


Operator methods:
 - operator can use all client and observer methods
 - probably don't need any special operator methods

System methods:
 - system programs should probably be able to use all client and observer methods
 - and also some special system ones


*/



