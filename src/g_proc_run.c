
#include <allegro5/allegro.h>

#include <stdio.h>

#include "m_config.h"
#include "m_globvars.h"

#include "g_misc.h"
#include "g_header.h"

#include "g_group.h"
#include "g_motion.h"
#include "g_method.h"
#include "g_code.h"
#include "g_proc.h"
#include "g_shape.h"

#include "g_method_pr.h"

#include "v_interp.h"

extern struct shapestruct shape_dat [SHAPES] [SHAPES_SIZES]; // set up in g_shape.c
extern const struct mtypestruct mtype [MTYPES]; // in g_method.c



void run_procs(void)
{

 run_motion();

 int p;
 struct procstruct* pr;
// int instructions;

 for (p = 0; p < w.max_procs; p ++)
 {
  pr = &w.proc[p];

// if the proc has just been destroyed, it may still be deallocating (if so exists will be -1). A deallocating proc doesn't do anything:
  if (pr->exists == -1)
  {
   pr->deallocating--;
   if (pr->deallocating == 0)
   {
    pr->exists = 0;
    w.player[pr->player_index].processes --;
    continue;
   }
  }



  if (pr->exists <= 0)
   continue;

//   fprintf(stdout, "\nexists %i", pr->index);

// run code:

  pr->execution_count --;
  pr->lifetime ++;

  *(pr->irpt) += pr->irpt_gain;
  if (*(pr->irpt) > *(pr->irpt_max))
   *(pr->irpt) = *(pr->irpt_max);

  if (pr->execution_count <= 0)
  {
   w.current_output_console = w.player[pr->player_index].output_console;
   w.current_error_console = w.player[pr->player_index].error_console;
   w.print_colour = PRINT_COL_LGREY; //w.player[pr->player_index].default_print_colour;
   w.print_proc_action_value3 = 0; // can be set by the MTYPE_PR_STD method

// calculate IRPT use rate:
//   pr->irpt_use = pr->irpt_base - *(pr->irpt);
// if proc has an irpt gen method, it may be set to automatically generate irpt just before execution:
/*   if (pr->auto_irpt_method != -1
    && pr->regs.mbank [(pr->auto_irpt_method * METHOD_SIZE) + MBANK_PR_IRPT_AUTO] > 0)
     generate_irpt(pr, pr->auto_irpt_method, pr->regs.mbank [(pr->auto_irpt_method * METHOD_SIZE) + MBANK_PR_IRPT_AUTO]);*/
// now save pr's base irpt (used to calculate irpt use rate)
//   pr->irpt_base = *(pr->irpt); // irpt_base can be modified by irpt generated during execution
// now take out basic upkeep costs (set at proc generation - see g_proc.c):
   *(pr->irpt) -= pr->base_cost;
/*   if (pr->virtual_method != -1
    && pr->method[pr->virtual_method].data [MDATA_PR_VIRTUAL_STATE] > 0)
    pr->irpt -= pr->base_cost;*/
   if (*(pr->irpt) <= 0)
   {
    int irpt_damage = (abs(*(pr->irpt)) / 16) + 1;
//    fprintf(stdout, "\nirpt damage: proc %i has irpt %i; hp %i damage %i", p, pr->irpt, pr->hp, irpt_damage);
    *(pr->irpt) = 0;
    hurt_proc(p, irpt_damage, pr->player_index);
    if (pr->exists == -1)
     continue;
   }
   pr->execution_count = EXECUTION_COUNT; // set this before calling interpret_bcode in case proc calls std method to delay execution
   pr->instructions_left_after_last_exec = interpret_bcode(pr, NULL, &pr->bcode, &pr->regs, pr->method, pr->instructions_each_execution); // interpret_bcode returns unused instructions
//   fprintf(stdout, "\nexecuted %i", pr->index);
   active_method_pass_after_execution(pr); // currently this is called immediately after each execution. If this is ever changed so that it runs after all procs have executed, may need to change the way some things work (like LISTEN)
  }

/*

* need to store irpt value each ex cycle just after auto irpt generation.
 - just before auto irpt generation, current irpt is compared to the stored value.


How will instructions convert to irpt, and vice versa?



*/



 } // end of for p loop


 active_method_pass_each_tick(); // deals with methods that do stuff even when the proc isn't executing (e.g. acceleration, which accelerates for a certain duration)


}


