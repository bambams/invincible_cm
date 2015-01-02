
#include <allegro5/allegro.h>
#include <allegro5/allegro_native_dialog.h>

#include <stdio.h>
#include <string.h>

#include "m_config.h"

#include "g_header.h"
#include "m_globvars.h"

#include "g_misc.h"

#include "c_header.h"
#include "c_prepr.h"

#include "e_slider.h"
#include "e_header.h"
#include "c_init.h"
#include "e_log.h"
#include "e_files.h"

#include "e_editor.h"

#include "c_comp.h"

extern struct editorstruct editor;
extern struct gamestruct game;
extern struct viewstruct view;

int compile_source_tab_test(struct sourcestruct* test_src, struct bcodestruct* target_bcode);
int compile_source_tab_to_asm_tab(struct sourcestruct* src, struct bcodestruct* target_bcode, int crunch_asm);
int compile_source_tab_to_bcode_tab(struct sourcestruct* src, struct bcodestruct* target_bcode);
int build_source_edit(int output_type, struct source_editstruct* se, struct bcodestruct* target_bcode);

struct bcodestruct test_bcode; // Used only for testing. Contents are discarded unless we're compiling to bcode.
s16b test_bcode_op [BCODE_MAX]; // used for testing (as the op array for target_bcode). Size is BCODE_MAX but target_bcode.bcode_size can be smaller.

void build_source_tab(int output_type)
{

 reset_log();

 struct source_editstruct* se = get_current_source_edit();

 if (se == NULL
  || se->type != SOURCE_EDIT_TYPE_SOURCE)
 {
  write_line_to_log("Current tab is not a source file - can't build.", MLOG_COL_ERROR);
  return;
 }

 test_bcode.op = test_bcode_op;
 test_bcode.bcode_size = BCODE_MAX;

 build_source_edit(output_type, se, &test_bcode);

// if (output_type == COMPILE_OUTPUT_BCODE_TAB)
// {

// }

}


// this function is used for the test build command in the build menu
//  and also for building source_edits to templates and other edit tabs
// target_bcode must be a valid bcodestruct even if its contents are to be ignored
// returns 1 on success, 0 on failure
int build_source_edit(int output_type, struct source_editstruct* se, struct bcodestruct* target_bcode)
{

 if (se->type != SOURCE_EDIT_TYPE_SOURCE)
 {
  write_line_to_log("Can only run test build on source file.", MLOG_COL_ERROR);
  return 0;
 }

 start_log_line(MLOG_COL_COMPILER);
 write_to_log("Starting to compile ");
 write_to_log(se->src_file_name);
 write_to_log(".");
 finish_log_line();

// need to have a target bcode, although since this is a test it's just discarded
// struct bcodestruct target_bcode;

// need to translate source_edit to source
 struct sourcestruct src;

 if (!source_edit_to_source(&src, se))
 {
  write_line_to_log("Failed to convert source file to source data structure.", MLOG_COL_ERROR); // I don't think this is actually possible but can't hurt to check
  return 0;
 }


// now run compiler
 int compile_result = 0;

 switch (output_type)
 {
  case COMPILE_OUTPUT_TEST:
   compile_result = compile_source_tab_test(&src, target_bcode); // may leave results of compilation in target_bcode but I'm not sure. It's discarded anyway.
   break;
  case COMPILE_OUTPUT_ASM_TAB:
   compile_result = compile_source_tab_to_asm_tab(&src, target_bcode, 0); // creates a new tab containing asm code. 0 means no crunch
   break;
  case COMPILE_OUTPUT_ASM_TAB_CRUNCH:
   compile_result = compile_source_tab_to_asm_tab(&src, target_bcode, 1); // creates a new tab containing asm code. 1 means crunch
   break;
  case COMPILE_OUTPUT_TEMPLATE:
   compile_result = compile_source_to_bcode(&src, target_bcode, NULL, 0, NULL); // leaves results of compilation in target_bcode
   break;
  case COMPILE_OUTPUT_BCODE_TAB:
   compile_result = compile_source_tab_to_bcode_tab(&src, target_bcode); // creates a new tab containing bcode
   break;
 }

 flush_game_event_queues();

 if (compile_result)
 {
  start_log_line(MLOG_COL_COMPILER);
  write_to_log(se->src_file_name);
  write_to_log(" successfully compiled.");
  finish_log_line();
  return 1;
 }
  else
  {
   start_log_line(MLOG_COL_COMPILER);
   write_to_log(se->src_file_name);
   write_to_log(" could not be compiled.");
   finish_log_line();
   return 0;
  }


}

int compile_source_tab_test(struct sourcestruct* test_src, struct bcodestruct* target_bcode)
{

 return compile_source_to_bcode(test_src, target_bcode, NULL, 0, NULL); // if this fails it will have written its own error message to the log

}


int compile_source_tab_to_asm_tab(struct sourcestruct* src, struct bcodestruct* target_bcode, int crunch_asm)
{

 struct sourcestruct asm_source; // asm code is written to this

// try to open a new tab:
 int tab = new_empty_source_tab();

 if (tab == -1)
  return 0;

 int sei = editor.tab_index [tab];

// run the compiler - this will leave the asm output in asm_source (we can ignore target_bcode)
 int compile_result = compile_source_to_bcode(src, target_bcode, &asm_source, crunch_asm, NULL);

 if (!compile_result)
 {
  close_source_tab(tab, 1); // 1 is force close (don't ask about saving)
  return 0;
 }

 source_to_editor(&asm_source, sei);
 editor.source_edit[sei].saved = 0;
 editor.source_edit[sei].from_a_file = 0;

// copy the file name to the tab name:
 give_source_edit_name_to_tab(tab, &editor.source_edit [sei]);

 open_tab(tab);

 return 1;

}

int compile_source_tab_to_bcode_tab(struct sourcestruct* src, struct bcodestruct* target_bcode)
{

// try to open a new tab:
 int tab = new_empty_source_tab(); // is changed to binary type below

 if (tab == -1)
  return 0;

 int sei = editor.tab_index [tab];

 struct source_editstruct* se = &editor.source_edit [sei];

 editor.tab_type [tab] = TAB_TYPE_BINARY;
 se->type = SOURCE_EDIT_TYPE_BINARY;

// run the compiler - this will leave the bcode output in target_bcode
 int compile_result = compile_source_to_bcode(src, target_bcode, NULL, 0, NULL); // 0 is crunch_asm

 if (!compile_result)
 {
  close_source_tab(tab, 1); // 1 is force close (don't ask about saving)
  return 0;
 }

 strcpy(target_bcode->src_file_name, "bcode");
 strcpy(target_bcode->src_file_path, "bcode");

 binary_to_editor(target_bcode, sei);

// copy the file name to the tab name:
 give_source_edit_name_to_tab(tab, se);
 se->from_a_file = 0;
 se->saved = 0;

 open_tab(tab);

 return 1;

}

int convert_bcode_to_source_tab(void)
{

 reset_log();

 int i;
 struct source_editstruct* old_se = get_current_source_edit();

 if (old_se == NULL
  || old_se->type != SOURCE_EDIT_TYPE_BINARY)
 {
  write_line_to_log("Current tab is not a binary file - can't convert.", MLOG_COL_ERROR);
  return 0;
 }

// try to open a new tab:
 int tab = new_empty_source_tab();

 if (tab == -1)
  return 0;

 int sei = editor.tab_index [tab];
 struct source_editstruct* new_se = &editor.source_edit[sei];

 convert_bcode_to_asm_source_initialiser(&old_se->bcode, new_se->text); // shouldn't be able to fail

// now set up the source_editstruct
 new_se->saved = 0;
 new_se->from_a_file = 0;
 strcpy(new_se->src_file_name, "converted");
 strcpy(new_se->src_file_path, "converted");

// copy the file name to the tab name:
 give_source_edit_name_to_tab(tab, new_se);

// apply syntax highlighting and put the lines of code in order:
 int in_a_comment = 0;
 for (i = 0; i < SOURCE_TEXT_LINES; i ++)
 {
  new_se->line_index [i] = i; // may change through editing
  in_a_comment = source_line_highlight_syntax(new_se, i, in_a_comment);
// if this code is changed, see corresponding code in source_to_editor() in e_editor.c
 }

// open the new tab
 open_tab(tab);

 write_line_to_log("Conversion successful.", MLOG_COL_EDITOR);

 return 1;

}

// Imports bcode from the process currently being watched by the process data box
int import_proc_bcode_to_bcode_tab(void)
{

	if (game.phase != GAME_PHASE_TURN
		&& game.phase != GAME_PHASE_WORLD)
	{
  write_line_to_log("Failed: not in game phase.", MLOG_COL_ERROR);
  return 0;
	}

	if (view.focus_proc == NULL)
	{
  write_line_to_log("Failed: no process data box (try selecting a process?).", MLOG_COL_ERROR);
  return 0;
	}


// try to open a new tab:
 int tab = new_empty_source_tab(); // is changed to binary type below

 if (tab == -1)
  return 0;

 int sei = editor.tab_index [tab];

 struct source_editstruct* se = &editor.source_edit [sei];

 editor.tab_type [tab] = TAB_TYPE_BINARY;
 se->type = SOURCE_EDIT_TYPE_BINARY;

 if (!binary_to_editor(&view.focus_proc->bcode, sei))
	{
  close_source_tab(tab, 1); // 1 is force close (don't ask about saving)
  return 0;
	}

 strcpy(se->src_file_name, "converted");
 strcpy(se->src_file_path, "converted");

// copy the file name to the tab name:
 give_source_edit_name_to_tab(tab, se);
 se->from_a_file = 0;
 se->saved = 0;

 open_tab(tab);

 return 1;



}


