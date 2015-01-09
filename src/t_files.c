
#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>

#include <stdio.h>
#include <string.h>

#include "m_config.h"

#include "g_header.h"
#include "m_globvars.h"

#include "g_misc.h"

#include "c_header.h"
#include "e_slider.h"
#include "e_header.h"
#include "e_editor.h"
#include "e_log.h"
#include "e_files.h"
#include "e_build.h"

#include "g_client.h"

#include "c_init.h"
#include "c_prepr.h"
#include "c_comp.h"
#include "t_template.h"
#include "t_files.h"


extern struct templstruct templ [TEMPLATES]; // in t_template.c
extern struct tstatestruct tstate;
extern struct templstruct default_mission_template [TEMPL_MISSION_DEFAULTS];
extern s16b default_mission_bcode_op [CLIENT_BCODE_SIZE];
extern s16b default_mission_bcode_ob [CLIENT_BCODE_SIZE];
extern s16b default_mission_bcode_de [CLIENT_BCODE_SIZE];
extern s16b default_mission_bcode_proc [TEMPLATES_PER_PLAYER] [PROC_BCODE_SIZE];


int expected_program_type(int templ_type);
//int load_known_file_into_template(int t, const char* file_path, int file_type);
int open_file_into_template(int t);
void finish_loading_binary_file_into_template(int t);
int copy_template_to_program(int t, struct programstruct* cl, int expect_program_type, int player);
void import_template_from_list(int line, int t);
int load_known_source_file_into_template(int t, const char* file_path);

// returns expected program type for a particular templ type
// calls error_call() if no expected type (this is probably an error because I forgot to update this function when adding a new templ type or program type)
int expected_program_type(int templ_type)
{

 switch(templ_type)
 {
  case TEMPL_TYPE_DELEGATE:
  case TEMPL_TYPE_DEFAULT_DELEGATE:
   return PROGRAM_TYPE_DELEGATE;
  case TEMPL_TYPE_OBSERVER:
  case TEMPL_TYPE_DEFAULT_OBSERVER:
   return PROGRAM_TYPE_OBSERVER;
  case TEMPL_TYPE_OPERATOR:
		case TEMPL_TYPE_DEFAULT_OPERATOR:
   return PROGRAM_TYPE_OPERATOR;
  case TEMPL_TYPE_PROC:
  case TEMPL_TYPE_DEFAULT_PROC:
   return PROGRAM_TYPE_PROCESS;
  case TEMPL_TYPE_SYSTEM:
   return PROGRAM_TYPE_SYSTEM;
 }

// template type not found - error. Shouldn't happen.

 fprintf(stdout, "\nError: t_template.c: expected_program_type(): template type %i not found", templ_type);
 error_call();

 return 0; // unreachable as error_call exits

}

// Call this once at setup. It mostly sets up the bcode structure for each default process template.
void initialise_default_mission_templates(void)
{

// Catch obvious mistake:
//#if ((TEMPL_MISSION_DEFAULT_PROC0 + TEMPLATES_PER_PLAYER) > TEMPL_MISSION_DEFAULTS)
//#error "wrong number of templates?"
//#endif

// fprintf(stdout, "\nTemplates: %i + %i > %i?", TEMPL_MISSION_DEFAULT_PROC0, TEMPLATES_PER_PLAYER, TEMPL_MISSION_DEFAULTS);

	int i, j;

 default_mission_template[TEMPL_MISSION_DEFAULT_OPERATOR].contents.bcode.op = default_mission_bcode_op;
 default_mission_template[TEMPL_MISSION_DEFAULT_OPERATOR].contents.bcode.bcode_size = CLIENT_BCODE_SIZE;
	for (j = 0; j < CLIENT_BCODE_SIZE; j ++)
	{
	 default_mission_template[TEMPL_MISSION_DEFAULT_OPERATOR].contents.bcode.op [j] = 0;
	}

 default_mission_template[TEMPL_MISSION_DEFAULT_OBSERVER].contents.bcode.op = default_mission_bcode_ob;
 default_mission_template[TEMPL_MISSION_DEFAULT_OBSERVER].contents.bcode.bcode_size = CLIENT_BCODE_SIZE;
	for (j = 0; j < CLIENT_BCODE_SIZE; j ++)
	{
	 default_mission_template[TEMPL_MISSION_DEFAULT_OBSERVER].contents.bcode.op [j] = 0;
	}

 default_mission_template[TEMPL_MISSION_DEFAULT_DELEGATE].contents.bcode.op = default_mission_bcode_de;
 default_mission_template[TEMPL_MISSION_DEFAULT_DELEGATE].contents.bcode.bcode_size = CLIENT_BCODE_SIZE;
	for (j = 0; j < CLIENT_BCODE_SIZE; j ++)
	{
	 default_mission_template[TEMPL_MISSION_DEFAULT_DELEGATE].contents.bcode.op [j] = 0;
	}



 for (i = 0; i < TEMPLATES_PER_PLAYER; i++)
	{
		default_mission_template[TEMPL_MISSION_DEFAULT_PROC0 + i].contents.bcode.op = default_mission_bcode_proc [i];
		default_mission_template[TEMPL_MISSION_DEFAULT_PROC0 + i].contents.bcode.bcode_size = PROC_BCODE_SIZE;
	}

	for (i = 0; i < TEMPLATES_PER_PLAYER; i ++)
	{
 	for (j = 0; j < PROC_BCODE_SIZE; j ++)
 	{
		 default_mission_template[TEMPL_MISSION_DEFAULT_PROC0 + i].contents.bcode.op [j] = 0;
		}
	}

// Now open standard templates:
 setup_templates_for_default_template_init();
// and load them up:
 load_known_source_file_into_template(0, "missions/op_basic.c");
 load_known_source_file_into_template(1, "missions/ob_basic.c");
 load_known_source_file_into_template(3, "missions/prsc_cfactory.c");
 load_known_source_file_into_template(4, "missions/prsc_defend.c");

// copy these templates into the mission default templates:
 copy_mission_templates_to_default(1, 1);

// done.

}

int load_known_source_file_into_template(int t, const char* file_path)
{

 struct sourcestruct temp_src;

 if (!load_source_file(ic_resource(file_path), &temp_src, 0, 0))
 {
 	fprintf(stdout, "\nFailed: couldn't open default mission file [path: %s] (file not found?).", file_path);
 	return 0; // could call error_call() but this doesn't have to be a fatal error.
 }

 if (!finish_loading_source_file_into_template(&temp_src, t, NULL))
 {
   	fprintf(stdout, "\nFailed: couldn't load default mission file [path: %s] into template %i.", file_path, t);
    return 0;
 }

 return 1;

}

/*
// Not sure this function still works (it's not currently used)
int load_known_file_into_template(int t, const char* file_path, int file_type)
{

 struct sourcestruct load_src;

 if (file_type == FILE_TYPE_SOURCE)
 {
// if the file type is a source file, it has to be loaded in and then compiled to bcode for the template.
  if (!load_source_file(file_path, &load_src, 0, 0)) // last two values are for preprocessor
  {
   write_line_to_log("Error: file not found.", MLOG_COL_ERROR);

   start_log_line(MLOG_COL_ERROR);
   write_to_log("(path: ");
   write_to_log(file_path);
   write_to_log(").");
   finish_log_line();

   write_line_to_log("Failed to load file into template.", MLOG_COL_ERROR);
   return 0;
  }


  return finish_loading_source_file_into_template(&load_src, t);

 } // end file_type == FILE_TYPE_SOURCE

 if (file_type == FILE_TYPE_BINARY)
 {
// if the file type is a binary file, it can be loaded directly into the template's bcode struct.
  if (load_binary_file(file_path, &templ[t].contents.bcode, 0, 0) == -1) // last two values are for preprocessor
  {
   write_line_to_log("Error: file not found.", MLOG_COL_ERROR);

   start_log_line(MLOG_COL_ERROR);
   write_to_log("(path: ");
   write_to_log(file_path);
   write_to_log(").");
   finish_log_line();

   write_line_to_log("Failed to load file into template.", MLOG_COL_ERROR);
   return 0;
  }

  finish_loading_binary_file_into_template(t);

  return 1;

 } // end file_type == FILE_TYPE_BINARY

 return 0; // shouldn't happen

}
*/
int open_file_into_template(int t)
{

 reset_log();

 struct sourcestruct load_src;

 int opened = open_file_into_source_or_binary(&load_src, &templ[t].contents.bcode, 0); // 0 indicates no checking against editor tabs for files already open

 switch(opened)
 {
  case OPEN_FAIL:
//  write_line_to_log("Didn't load file into template.", MLOG_COL_ERROR); - shouldn't write an error msg if user cancels file open.
// just rely on the error messages in open_file_into_source_or_binary()
   return 0;
// shouldn't need to check for OPENED_ALREADY as that doesn't apply to templates

  case OPENED_SOURCE:
   return finish_loading_source_file_into_template(&load_src, t, NULL); // end case OPENED_SOURCE

  case OPENED_BINARY:
// binary code has been written directly to template's bcodestruct, so just need to finish setting up the template
   finish_loading_binary_file_into_template(t);
   return 1; // end case OPENED_BINARY

 }

 return 0;

}


int finish_loading_source_file_into_template(struct sourcestruct* load_src, int t, const char* defined)
{

   clear_template(t);

   start_log_line(MLOG_COL_TEMPLATE);
   write_to_log("Starting to compile ");
   write_to_log(load_src->src_file_name);
   write_to_log(".");
   finish_log_line();


   if (!compile_source_to_bcode(load_src, &templ[t].contents.bcode, NULL, 0, defined)) // NULL indicates no asm output; 0 is crunch_asm
   {
    write_line_to_log("Failed to load file into template.", MLOG_COL_ERROR);
// compiler will also have written error to mlog
    return 0;
   }

// make sure the file is the correct type of file for this template:
    if (!check_program_type(templ[t].contents.bcode.op [BCODE_HEADER_ALL_TYPE], expected_program_type(templ[t].type)))
    {
     write_line_to_log("Failed to load file into template.", MLOG_COL_ERROR);
 // if check_program_type() fails it also writes to the mlog
     return 0;
    }

   templ[t].contents.origin = TEMPL_ORIGIN_FILE;
   templ[t].contents.loaded = 1;

   strcpy(templ[t].contents.file_name, templ[t].contents.bcode.src_file_name);
   strcpy(templ[t].contents.file_path, templ[t].contents.bcode.src_file_path);

   start_log_line(MLOG_COL_TEMPLATE);
   write_to_log("File ");
   write_to_log(templ[t].contents.bcode.src_file_name);
   write_to_log(" was opened into template successfully.");
   finish_log_line();

   return 1;

}

void finish_loading_binary_file_into_template(int t)
{

   templ[t].contents.origin = TEMPL_ORIGIN_FILE;
   templ[t].contents.loaded = 1;

   strcpy(templ[t].contents.file_name, templ[t].contents.bcode.src_file_name);
   strcpy(templ[t].contents.file_path, templ[t].contents.bcode.src_file_path);

   start_log_line(MLOG_COL_TEMPLATE);
   write_to_log("File ");
   write_to_log(templ[t].contents.bcode.src_file_name);
   write_to_log(" was opened into template successfully.");
   finish_log_line();

}





/*
This function copies a program stored in template t to programstruct cl.
program_type is the type of program it is expected to be (not sure this is actually necessary as long as the correct template is being used, but check it anyway)

Returns 1 on success, 0 on failure
*/
int copy_template_to_program(int t, struct programstruct* cl, int expect_program_type, int player)
{

// First confirm that the template is loaded.
// If it isn't, don't do anything:
 if (templ[t].contents.loaded == 0)
  return 0;

 int i;

// first clear the program, including the program's registerstruct and methodstructs:
 init_program(cl, expect_program_type, player); // this sets cl->player to player. It also sets cl->type but that will be reset and checked by derive_program_properties_from_bcode

// now copy bcode from the template into the program's bcode field:
 for (i = 0; i < cl->bcode.bcode_size; i ++)
 {
  cl->bcode.op [i] = templ[t].contents.bcode.op [i];
 }

// now derive program properties (method types etc) from the bcode
// if the program is a system program, this will also fill in the global w_pre_init struct which can then be used to generate a game setup menu
// derive_program_properties_from_bcode() also verifies the bcode's program type (although it doesn't verify anything else about the bcode)
 if (!derive_program_properties_from_bcode(cl, expect_program_type))
  return 0;

 if (cl->type == PROGRAM_TYPE_OPERATOR) // derive_program_properties_from_bcode has confirmed that the cl->type is valid for the particular program we're dealing with (based on the template type)
 {
  w.actual_operator_player = player;
 }
  else
  {
// if this player could be the operator, but is instead using a client program, w.actual_operator_player will be set to -1
   if (cl->type == PROGRAM_TYPE_DELEGATE
    && w.player[player].program_type_allowed == PLAYER_PROGRAM_ALLOWED_OPERATOR)
     w.actual_operator_player = -1; // this code deals with the possibility that a player's client program might change - not sure whether to allow this.
  }

// activate the program!
 cl->active = 1;

 return 1;

}



// assumes line and t are valid, and also tstate.import_list [line]
void import_template_from_list(int line, int t)
{

  reset_log();

  struct source_editstruct* se = tstate.import_list[line].se;
  int build_result;
  int i;

  switch(se->type)
  {

   case SOURCE_EDIT_TYPE_SOURCE:

    build_result = build_source_edit(COMPILE_OUTPUT_TEMPLATE, se, &templ[t].contents.bcode);

    if (!build_result)
    {
     write_line_to_log("Failed to import file into template.", MLOG_COL_ERROR);
     return; // failed; do nothing
    }

    // make sure the file is the correct type of file for this template:
    if (!check_program_type(templ[t].contents.bcode.op [BCODE_HEADER_ALL_TYPE], expected_program_type(templ[t].type)))
    {
     write_line_to_log("Failed to import file into template.", MLOG_COL_ERROR);
     return; // if check_program_type() fails it writes to the mlog
    }

    templ[t].contents.origin = TEMPL_ORIGIN_EDITOR;
    templ[t].contents.loaded = 1;

    strcpy(templ[t].contents.file_name, se->src_file_name);
    strcpy(templ[t].contents.file_path, "no path");

    set_opened_template(t, templ[t].type, 0);

    write_line_to_log("File imported into template successfully.", MLOG_COL_TEMPLATE);

    break; // end case SOURCE_EDIT_TYPE_SOURCE

   case SOURCE_EDIT_TYPE_BINARY:

    for (i = 0; i < templ[t].contents.bcode.bcode_size; i ++)
    {
     templ[t].contents.bcode.op [i] = se->bcode.op [i];
    }

    // make sure the file is the correct type of file for this template:
    if (!check_program_type(templ[t].contents.bcode.op [BCODE_HEADER_ALL_TYPE], expected_program_type(templ[t].type)))
    {
     write_line_to_log("Failed to import file into template.", MLOG_COL_ERROR);
     return; // if check_program_type() fails it writes to the mlog
    }

    templ[t].contents.origin = TEMPL_ORIGIN_EDITOR;
    templ[t].contents.loaded = 1;

    strcpy(templ[t].contents.file_name, se->src_file_name);
    strcpy(templ[t].contents.file_path, "no path");

    set_opened_template(t, templ[t].type, 0);

    write_line_to_log("File imported into template successfully.", MLOG_COL_TEMPLATE);
    break;

  }

}

