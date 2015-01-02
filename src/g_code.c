#include <allegro5/allegro.h>

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#include "m_config.h"
#include "m_globvars.h"

#include "g_misc.h"
#include "g_header.h"
#include "g_method.h"
#include "c_header.h"

#include "v_interp.h"

#include "m_maths.h"


// This function runs the system, observer and operator programs.
// It is not affected by a user pause (the programs continue to run, allowing interface interaction)
//  - This means that these kinds of programs need to anticipate the possibility that they are running while the game is paused (they are able to tell whether it is, and can also change the pause state)
void run_system_code(void)
{

 w.current_output_console = w.system_output_console;
 w.current_error_console = w.system_error_console;
 w.print_colour = PRINT_COL_LGREY;

 int instr_left;

 if (w.system_program.active)
 {
  instr_left = w.system_program.available_instr;
  w.system_program.irpt = w.system_program.available_irpt;
  w.system_program.instr_left = interpret_bcode(NULL, &w.system_program, &w.system_program.bcode, &w.system_program.regs, w.system_program.method, instr_left);
 }

 w.current_output_console = w.observer_output_console;
 w.current_error_console = w.observer_error_console;
 w.print_colour = PRINT_COL_LGREY;

 if (w.observer_program.active)
 {
  instr_left = w.observer_program.available_instr;
  w.observer_program.irpt = w.observer_program.available_irpt;
  w.observer_program.instr_left = interpret_bcode(NULL, &w.observer_program, &w.observer_program.bcode, &w.observer_program.regs, w.observer_program.method, instr_left);
 }

 if (w.actual_operator_player != -1) // this means that w.actual_operator_player's client program is actually an operator.
 {
// This code should run the operator player's client program *only* if that program actually is an operator program.
//  Otherwise it will run it as a client program even if the game is paused (only operators/observers should run while paused).
//  Client code execution is in run_client_code() below
  if (w.player[w.actual_operator_player].active
   && w.player[w.actual_operator_player].client_program.active)
  {
    w.current_output_console = w.player[w.actual_operator_player].output_console;
    w.current_error_console = w.player[w.actual_operator_player].error_console;
    w.print_colour = PRINT_COL_LGREY; //w.player[w.actual_operator_player].default_print_colour;
    instr_left = w.player[w.actual_operator_player].client_program.available_instr;
    w.player[w.actual_operator_player].client_program.irpt = w.player[w.actual_operator_player].client_program.available_irpt;
    w.player[w.actual_operator_player].client_program.instr_left = interpret_bcode(NULL, &w.player[w.actual_operator_player].client_program, &w.player[w.actual_operator_player].client_program.bcode, &w.player[w.actual_operator_player].client_program.regs, w.player[w.actual_operator_player].client_program.method, instr_left);
  }
 }

}

// This function runs the player client programs, except for an operator program (it doesn't run the observer either)
// It is called only if the game is not paused.
void run_delegate_code(void)
{

 int c;
 int instr_left;

 for (c = 0; c < w.players; c ++)
 {
  if (w.player[c].active
   && c != w.actual_operator_player)
  {
   if (w.player[c].client_program.active)
   {
    w.current_output_console = w.player[c].output_console;
    w.current_error_console = w.player[c].error_console;
    w.print_colour = PRINT_COL_LGREY; //w.player[c].default_print_colour;
    instr_left = w.player[c].client_program.available_instr;
    w.player[c].client_program.irpt = w.player[c].client_program.available_irpt;
    w.player[c].client_program.instr_left = interpret_bcode(NULL, &w.player[c].client_program, &w.player[c].client_program.bcode, &w.player[c].client_program.regs, w.player[c].client_program.method, instr_left);
   }
  }
 }

}
