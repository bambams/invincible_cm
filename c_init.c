#include <allegro5/allegro.h>
#include "m_config.h"
#include "g_header.h"
#include "m_globvars.h"
#include "c_header.h"

#include "g_misc.h"
#include "c_asm.h"
#include "e_slider.h"
#include "c_init.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>



// these are defined in c_comp.c
extern struct identifierstruct identifier [IDENTIFIER_MAX];
extern struct cfunctionstruct cfunction [CFUNCTIONS];
extern struct array_initstruct array_init;
extern struct process_defstruct process_def [PROCESS_DEFS];
extern struct astatestruct astate; // in a_asm.c

// defined in v_interp.c
extern struct opcode_propertiesstruct opcode_properties [OP_end];

// the index value is declared in an enum in vac_header.h. The position of a keyword in the following declaration must match its position relative to KEYWORD_INT in that enum.
// changes to this list (other than adding or removing members) may need to be reflected in check_word_type() (at the end of this file)
const struct c_keywordstruct c_keyword [C_KEYWORDS] =
{
 {"int", KEYWORD_INT},
 {"void", KEYWORD_VOID},
 {"if", KEYWORD_IF},
 {"else", KEYWORD_ELSE},
 {"return", KEYWORD_RETURN},
 {"static", KEYWORD_STATIC},
 {"auto", KEYWORD_AUTO},
 {"print", KEYWORD_PRINT},
 {"while", KEYWORD_WHILE},
 {"do", KEYWORD_DO},
 {"for", KEYWORD_FOR},
 {"break", KEYWORD_BREAK},
 {"continue", KEYWORD_CONTINUE},
 {"process", KEYWORD_PROCESS},
 {"interface", KEYWORD_INTERFACE},
 {"enum", KEYWORD_ENUM},
 {"goto", KEYWORD_GOTO},
 {"asm", KEYWORD_ASM},
 {"asm_only", KEYWORD_ASM_ONLY},
 {"switch", KEYWORD_SWITCH},
 {"case", KEYWORD_CASE},
 {"default", KEYWORD_DEFAULT},
 {"data", KEYWORD_DATA},
};

// changes to this list (other than adding or removing members) may need to be reflected in check_word_type() (at the end of this file)
const struct c_keywordstruct c_builtin_cfunction [C_BUILTIN_CFUNCTION_NAMES] =
{
 {"put", BUILTIN_CFUNCTION_PUT},
 {"get", BUILTIN_CFUNCTION_GET},
 {"put_index", BUILTIN_CFUNCTION_PUT_INDEX},
 {"get_index", BUILTIN_CFUNCTION_GET_INDEX},
 {"call", BUILTIN_CFUNCTION_CALL},
 {"process_start", BUILTIN_CFUNCTION_PROCESS_START},
 {"process_end", BUILTIN_CFUNCTION_PROCESS_END},
// {"new_process", BUILTIN_CFUNCTION_NEW_PROCESS},

 {"get_x", BUILTIN_CFUNCTION_GET_X},
 {"get_y", BUILTIN_CFUNCTION_GET_Y},
 {"get_angle", BUILTIN_CFUNCTION_GET_ANGLE},
 {"get_speed_x", BUILTIN_CFUNCTION_GET_SPEED_X},
 {"get_speed_y", BUILTIN_CFUNCTION_GET_SPEED_Y},
 {"get_team", BUILTIN_CFUNCTION_GET_TEAM},
 {"get_hp", BUILTIN_CFUNCTION_GET_HP},
 {"get_hp_max", BUILTIN_CFUNCTION_GET_HP_MAX},
 {"get_instr", BUILTIN_CFUNCTION_GET_INSTR},
 {"get_instr_max", BUILTIN_CFUNCTION_GET_INSTR_MAX},
 {"get_irpt", BUILTIN_CFUNCTION_GET_IRPT},
 {"get_irpt_max", BUILTIN_CFUNCTION_GET_IRPT_MAX},
 {"get_own_irpt_max", BUILTIN_CFUNCTION_GET_OWN_IRPT_MAX},
 {"get_data", BUILTIN_CFUNCTION_GET_DATA},
 {"get_data_max", BUILTIN_CFUNCTION_GET_DATA_MAX},
 {"get_own_data_max", BUILTIN_CFUNCTION_GET_OWN_DATA_MAX},
 {"get_spin", BUILTIN_CFUNCTION_GET_SPIN},
 {"get_group_x", BUILTIN_CFUNCTION_GET_GR_X},
 {"get_group_y", BUILTIN_CFUNCTION_GET_GR_Y},
// {"get_group_angle", BUILTIN_CFUNCTION_GET_GR_ANGLE},
 {"get_group_x_speed", BUILTIN_CFUNCTION_GET_GR_SPEED_X},
 {"get_group_y_speed", BUILTIN_CFUNCTION_GET_GR_SPEED_Y},
 {"get_group_members", BUILTIN_CFUNCTION_GET_GR_MEMBERS},
 {"get_world_x", BUILTIN_CFUNCTION_GET_WORLD_X},
 {"get_world_y", BUILTIN_CFUNCTION_GET_WORLD_Y},
// {"get_gen_limit", BUILTIN_CFUNCTION_GET_GEN_LIMIT},
// {"get_gen_number", BUILTIN_CFUNCTION_GET_GEN_NUMBER},
 {"get_ex_time", BUILTIN_CFUNCTION_GET_EX_TIME},
 {"get_efficiency", BUILTIN_CFUNCTION_GET_EFFICIENCY},
 {"get_time", BUILTIN_CFUNCTION_GET_TIME},
 {"get_vertices", BUILTIN_CFUNCTION_GET_VERTICES},
 {"get_vertex_angle", BUILTIN_CFUNCTION_GET_VERTEX_ANGLE},
 {"get_vertex_dist", BUILTIN_CFUNCTION_GET_VERTEX_DIST},
 {"get_vertex_angle_next", BUILTIN_CFUNCTION_GET_VERTEX_ANGLE_NEXT},
 {"get_vertex_angle_prev", BUILTIN_CFUNCTION_GET_VERTEX_ANGLE_PREV},
 {"get_vertex_angle_min", BUILTIN_CFUNCTION_GET_VERTEX_ANGLE_MIN},
 {"get_vertex_angle_max", BUILTIN_CFUNCTION_GET_VERTEX_ANGLE_MAX},
 {"get_method", BUILTIN_CFUNCTION_GET_METHOD},
 {"get_method_find", BUILTIN_CFUNCTION_GET_METHOD_FIND},

 {"get_command", BUILTIN_CFUNCTION_GET_COMMAND},
 {"get_command_bit", BUILTIN_CFUNCTION_GET_COMMAND_BIT},
 {"set_command", BUILTIN_CFUNCTION_SET_COMMAND},
 {"set_command_bit_1", BUILTIN_CFUNCTION_SET_COMMAND_BIT_1},
 {"set_command_bit_0", BUILTIN_CFUNCTION_SET_COMMAND_BIT_0},

 {"check_command", BUILTIN_CFUNCTION_CHECK_COMMAND},
 {"check_command_bit", BUILTIN_CFUNCTION_CHECK_COMMAND_BIT},
 {"command", BUILTIN_CFUNCTION_COMMAND},
 {"command_bit_1", BUILTIN_CFUNCTION_COMMAND_BIT_1},
 {"command_bit_0", BUILTIN_CFUNCTION_COMMAND_BIT_0},

 {"query_x", BUILTIN_CFUNCTION_QUERY_X},
 {"query_y", BUILTIN_CFUNCTION_QUERY_Y},
 {"query_angle", BUILTIN_CFUNCTION_QUERY_ANGLE},
 {"query_speed_x", BUILTIN_CFUNCTION_QUERY_SPEED_X},
 {"query_speed_y", BUILTIN_CFUNCTION_QUERY_SPEED_Y},
 {"query_team", BUILTIN_CFUNCTION_QUERY_TEAM},
 {"query_hp", BUILTIN_CFUNCTION_QUERY_HP},
 {"query_hp_max", BUILTIN_CFUNCTION_QUERY_HP_MAX},
 {"query_irpt", BUILTIN_CFUNCTION_QUERY_IRPT},
 {"query_irpt_max", BUILTIN_CFUNCTION_QUERY_IRPT_MAX},
 {"query_own_irpt_max", BUILTIN_CFUNCTION_QUERY_OWN_IRPT_MAX},
 {"query_data", BUILTIN_CFUNCTION_QUERY_DATA},
 {"query_data_max", BUILTIN_CFUNCTION_QUERY_DATA_MAX},
 {"query_own_data_max", BUILTIN_CFUNCTION_QUERY_OWN_DATA_MAX},
 {"query_spin", BUILTIN_CFUNCTION_QUERY_SPIN},
 {"query_group_x", BUILTIN_CFUNCTION_QUERY_GR_X},
 {"query_group_y", BUILTIN_CFUNCTION_QUERY_GR_Y},
 {"query_group_angle", BUILTIN_CFUNCTION_QUERY_GR_ANGLE},
 {"query_group_x_speed", BUILTIN_CFUNCTION_QUERY_GR_SPEED_X},
 {"query_group_y_speed", BUILTIN_CFUNCTION_QUERY_GR_SPEED_Y},
 {"query_group_members", BUILTIN_CFUNCTION_QUERY_GR_MEMBERS},
 {"query_ex_time", BUILTIN_CFUNCTION_QUERY_EX_TIME},
 {"query_efficiency", BUILTIN_CFUNCTION_QUERY_EFFICIENCY},
// no query functions to correspond to get_world_x, get_world_y (as these are available through the CLOB_SETTINGS method)
 {"query_vertices", BUILTIN_CFUNCTION_QUERY_VERTICES},
 {"query_vertex_angle", BUILTIN_CFUNCTION_QUERY_VERTEX_ANGLE},
 {"query_vertex_dist", BUILTIN_CFUNCTION_QUERY_VERTEX_DIST},
 {"query_vertex_angle_next", BUILTIN_CFUNCTION_QUERY_VERTEX_ANGLE_NEXT},
 {"query_vertex_angle_prev", BUILTIN_CFUNCTION_QUERY_VERTEX_ANGLE_PREV},
 {"query_vertex_angle_min", BUILTIN_CFUNCTION_QUERY_VERTEX_ANGLE_MIN},
 {"query_vertex_angle_max", BUILTIN_CFUNCTION_QUERY_VERTEX_ANGLE_MAX},
 {"query_method", BUILTIN_CFUNCTION_QUERY_METHOD},
 {"query_method_find", BUILTIN_CFUNCTION_QUERY_METHOD_FIND},
 {"query_mbank", BUILTIN_CFUNCTION_QUERY_MBANK},

 {"world_x", BUILTIN_CFUNCTION_WORLD_WORLD_X},
 {"world_y", BUILTIN_CFUNCTION_WORLD_WORLD_Y},
 {"world_processes", BUILTIN_CFUNCTION_WORLD_PROCS},
 {"world_team", BUILTIN_CFUNCTION_WORLD_TEAM},
 {"world_teams", BUILTIN_CFUNCTION_WORLD_TEAMS},
 {"world_team_size", BUILTIN_CFUNCTION_WORLD_TEAM_SIZE},
 {"world_processes_each", BUILTIN_CFUNCTION_WORLD_PROCS_EACH},
 {"world_first_process", BUILTIN_CFUNCTION_WORLD_FIRST_PROC},
 {"world_last_process", BUILTIN_CFUNCTION_WORLD_LAST_PROC},
// {"world_gen_limit", BUILTIN_CFUNCTION_WORLD_GEN_LIMIT},
// {"world_gen_number", BUILTIN_CFUNCTION_WORLD_GEN_NUMBER},
 {"world_time", BUILTIN_CFUNCTION_WORLD_TIME},

 {"hypot", BUILTIN_CFUNCTION_HYPOT},
 {"sqrt", BUILTIN_CFUNCTION_SQRT},
 {"pow", BUILTIN_CFUNCTION_POW},
 {"abs", BUILTIN_CFUNCTION_ABS},
 {"angle_difference", BUILTIN_CFUNCTION_ANGLE_DIFF},
 {"signed_angle_difference", BUILTIN_CFUNCTION_ANGLE_DIFF_S},
 {"turn_direction", BUILTIN_CFUNCTION_TURN_DIR},
 {"sin", BUILTIN_CFUNCTION_SIN},
 {"cos", BUILTIN_CFUNCTION_COS},
 {"atan2", BUILTIN_CFUNCTION_ATAN2},
// {"random", BUILTIN_CFUNCTION_RANDOM},
// {"turn_towards_angle", BUILTIN_CFUNCTION_}, // may need to be more complex
// {"turn_towards_xy", BUILTIN_CFUNCTION_}, // may need to be more complex

 {"const", BUILTIN_UNIMP_CONST},
 {"sizeof", BUILTIN_UNIMP_SIZEOF},
 {"struct", BUILTIN_UNIMP_STRUCT},
 {"union", BUILTIN_UNIMP_UNION},
 {"extern", BUILTIN_UNIMP_EXTERN},
 {"unsigned", BUILTIN_UNIMP_UNSIGNED},
 {"signed", BUILTIN_UNIMP_SIGNED},
 {"typedef", BUILTIN_UNIMP_TYPEDEF},
 {"inline", BUILTIN_UNIMP_INLINE},
 {"register", BUILTIN_UNIMP_REGISTER},

 {"char", BUILTIN_UNSUP_CHAR},
 {"short", BUILTIN_UNSUP_SHORT},
 {"float", BUILTIN_UNSUP_FLOAT},
 {"double", BUILTIN_UNSUP_DOUBLE},

};


// changes to this list (other than adding or removing members) may need to be reflected in check_word_type() (at the end of this file)
const struct c_keywordstruct asm_keyword [ASM_KEYWORDS] =
{
 {"int", KEYWORD_ASM_INT},
// {"set", KEYWORD_ASM_SET},
// {"decl", KEYWORD_ASM_DECL},
 {"def", KEYWORD_ASM_DEF},
 {"label", KEYWORD_ASM_LABEL},
 {"aspace", KEYWORD_ASM_ASPACE},
 {"aspace_end", KEYWORD_ASM_ASPACE_END},
 {"aspace_end_address", KEYWORD_ASM_ASPACE_END_ADDRESS},
 {"nspace", KEYWORD_ASM_NSPACE},
 {"nspace_end", KEYWORD_ASM_NSPACE_END},
 {"nspace_rejoin", KEYWORD_ASM_NSPACE_REJOIN},
 {"scope", KEYWORD_ASM_SCOPE},
 {"A", KEYWORD_ASM_REG_A},
 {"B", KEYWORD_ASM_REG_B},
 {"C", KEYWORD_ASM_REG_C},
 {"D", KEYWORD_ASM_REG_D},
 {"E", KEYWORD_ASM_REG_E},
 {"F", KEYWORD_ASM_REG_F},
 {"G", KEYWORD_ASM_REG_G},
 {"H", KEYWORD_ASM_REG_H},
 {"data", KEYWORD_ASM_DATA},

};

// This sets up the list of identifiers for a new compilation by clearing it then adding in keywords and similar
// Must be called before each compilation
// Some built-in tokens (mostly various numbers like program types and method types) are dealt with by the preprocessor (see numtoken code in c_prepr.c)
void init_identifiers(int bcode_size)
{

 int id, i, j, k;

// first set up the process_def struct
 for (i = 0; i < PROCESS_DEFS; i ++)
 {
  process_def[i].defined = 0;
  process_def[i].prototyped = 0;
  process_def[i].start_address = 0; // for processes > 0, set in c_inter.c when end of process definition found
  process_def[i].end_address = -1; // set in c_inter.c when end of process definition found
  process_def[i].parent = -1;
  process_def[i].id_index = -1; // see below for definition of default process and its identifier ("self")
  process_def[i].asm_only = 0;
// cfunctions are initialised here. Further down in this function the "main" cfunction name identifier is declared for each process
  for (j = 0; j < CFUNCTIONS; j ++)
  {
   process_def[i].cfunction[j].exists = 0;
   process_def[i].cfunction[j].defined = 0;
   process_def[i].cfunction[j].bcode_address = -1; // this is used to catch declared but undefined cfunctions
   process_def[i].cfunction[j].storage_class = STORAGE_AUTO; // this is where the default is determined.
   process_def[i].cfunction[j].corresponding_nspace = -1;
  }
// now initialise the process_def's interface structure
// have to initialise all values here because they are not set if not expressly defined by the user
  process_def[i].iface.defined = 0;
  process_def[i].iface.shape = 0;
  process_def[i].iface.size = 0;
  for (j = 0; j < METHODS; j ++)
  {
   for (k = 0; k < INTERFACE_METHOD_DATA; k ++)
   {
    process_def[i].iface.method [j] [k] = 0;
   }
  }
 }


// set up the identifier list with asm opcodes and compiler keywords here


 for (id = 0; id < IDENTIFIER_MAX; id ++)
 {
  identifier[id].name [0] = '\0';
  identifier[id].asm_or_compiler = IDENTIFIER_ASM_OR_C;
  identifier[id].type = ID_NONE;
  identifier[id].address_bcode = -1;
  identifier[id].address_precode = -1;
  identifier[id].declare_line = -1;
  identifier[id].declare_pos = -1;
  identifier[id].static_int_assign_at_startup = 0;
  identifier[id].static_array_initialised = 0;
  identifier[id].array_dims = 0;
  identifier[id].pointer = 0;
  identifier[id].process_scope = -1; // universal to all processes
  identifier[id].scope = SCOPE_GLOBAL;
  identifier[id].stack_offset = -1;
  identifier[id].storage_class = STORAGE_STATIC;
  identifier[id].initial_value = 0;
  identifier[id].static_array_init_offset = 0;
  identifier[id].enum_value = 0;
  identifier[id].process_index = 0;
  identifier[id].cfunction_index = 0;
  identifier[id].aspace_scope = -1; // file aspace
  identifier[id].nspace_scope = -1; // universal nspace
  identifier[id].aspace_index = -1; // none
  identifier[id].nspace_index = -1; // none
  for (i = 0; i < ARRAY_DIMENSIONS; i ++)
  {
   identifier[id].array_dim_size [i] = 0;
   identifier[id].array_element_size [i] = 0;
  }
 }

// now that the identifier array has been cleared, we need to set it up with various built-in things like keywords and opcodes

// first set up the assembler opcodes (see v_interp.c for opcode_properties)
// code in a_asm.c assumes that the opcodes come first, and that the index in the identifier array corresponds to the opcode index.
 for (id = 0; id < OP_end; id ++)
 {
  strcpy(identifier[id].name, opcode_properties[id].name);
  identifier[id].type = ASM_ID_OPCODE;
  identifier[id].asm_or_compiler = IDENTIFIER_ASM_ONLY;
  identifier[id].array_dims = 0;
  identifier[id].pointer = 0;
  identifier[id].process_scope = -1; // universal to all processes
 }

// now the assembler keywords (various assembler commands that don't translate directly to opcodes):
 int ak;

 for (id = KEYWORD_ASM_INT; id < KEYWORDS_ASM_END; id ++)
 {
  ak = id - KEYWORD_ASM_INT;
  if (asm_keyword[ak].index != id) // let's just verify that the lists match up
  {
   fprintf(stdout, "\nac_init.c: init_identifiers(): Error - asm_keyword[%i] index (%i) does not match keyword list entry (%i)\n", ak, asm_keyword[ak].index, id);
   error_call();
  }

  strcpy(identifier[id].name, asm_keyword[ak].name);
  identifier[id].type = ASM_ID_KEYWORD;
  identifier[id].asm_or_compiler = IDENTIFIER_ASM_ONLY;
  identifier[id].process_scope = -1; // universal to all processes
  identifier[id].array_dims = 0;
  identifier[id].pointer = 0;
 }

// now compiler keywords
 int ck;

 for (id = KEYWORD_INT; id < KEYWORDS_END; id ++)
 {
  ck = id - KEYWORD_INT;
  if (c_keyword[ck].index != id) // let's just verify that the lists match up
  {
   fprintf(stdout, "\nac_init.c: init_identifiers(): Error - c_keyword[%i] index (%i) does not match keyword list entry (%i)\n", ck, c_keyword[ck].index, id);
   error_call();
  }

  strcpy(identifier[id].name, c_keyword[ck].name);
  identifier[id].type = C_ID_KEYWORD;
  identifier[id].asm_or_compiler = IDENTIFIER_C_ONLY;
  identifier[id].process_scope = -1; // universal to all processes

 }

// now built-in compiler functions:

 for (id = BUILTIN_CFUNCTION_PUT; id < BUILTIN_CFUNCTIONS_END; id ++)
 {
  ck = id - BUILTIN_CFUNCTION_PUT;
  if (c_builtin_cfunction[ck].index != id) // let's just verify that the lists match up
  {
   fprintf(stdout, "\nac_init.c: init_identifiers(): Error - builtin_cfunction[%i] index (%i) does not match builtin cfunction list entry (%i)\n", ck, c_builtin_cfunction[ck].index, id);
   error_call();
  }

  strcpy(identifier[id].name, c_builtin_cfunction[ck].name);
  identifier[id].type = ID_BUILTIN_CFUNCTION;
  identifier[id].asm_or_compiler = IDENTIFIER_C_ONLY;
  identifier[id].process_scope = -1; // universal to all processes

 }

// note: value of id is used below:

// now we need to prototype the main cfunction for each possible process
// these definitions don't clash because their process_scopes are all different
 int nspace_index; // need to allocate an nspace for each function

 for (i = 0; i < PROCESS_DEFS; i ++)
 {
  strcpy(identifier[id].name, "main");
  identifier[id].type = ID_USER_CFUNCTION;
  identifier[id].address_bcode = -1;
  identifier[id].cfunction_index = 0;
  identifier[id].process_scope = i;
  identifier[id].asm_or_compiler = IDENTIFIER_C_ONLY;
  process_def[i].cfunction[0].exists = 1;
  process_def[i].cfunction[0].arguments = 0;
  process_def[i].cfunction[0].return_type = CFUNCTION_RETURN_VOID;
  process_def[i].cfunction[0].identifier_index = id;
  process_def[i].cfunction[0].storage_class = STORAGE_STATIC;
  id ++;
// now set up an nspace to correspond to the main cfunction (for other cfunctions this is done at prototyping)
  strcpy(identifier[id].name, "main");
  identifier[id].type = ASM_ID_NSPACE;
  identifier[id].address_bcode = -1;
  identifier[id].asm_or_compiler = IDENTIFIER_ASM_ONLY;
  allocate_nspace(id); // allocate_nspace finds an unused nspace, allocates it and sets the identifier's nspace_index. Shouldn't be possible for it to fail here as init_assembler should've been called.
  nspace_index = identifier[id].nspace_index;
  astate.nspace[nspace_index].declared = 1;
//  astate.nspace[nspace_index].defined = 1; // I think this should stay zero
  astate.nspace[nspace_index].aspace_scope = -1; // this is updated when the relevant process has an aspace defined (as it can't be known before then)
  astate.nspace[nspace_index].corresponding_cfunction = 0;
  astate.nspace[nspace_index].identifier_index = id;
  process_def[i].cfunction[0].corresponding_nspace = nspace_index;
  id ++;
 }




// now set up the default process 0
 strcpy(identifier[id].name, "self");
 identifier[id].type = ID_PROCESS;
 identifier[id].scope = SCOPE_GLOBAL;
 identifier[id].asm_or_compiler = IDENTIFIER_C_ONLY;
 identifier[id].process_scope = 0;
 identifier[id].process_index = 0; // self is process 0
 id++;
 process_def[0].defined = 1;
 process_def[0].prototyped = 1;
 process_def[0].start_address = 0;
 process_def[0].end_address = bcode_size - 1; // not sure about this - could probably set this to the base of the stack
 process_def[0].parent = -1;
 process_def[0].id_index = id;
 process_def[0].corresponding_aspace = 0; // this is set up in init_assembler in a_asm.c
// now need to set up a self identifier for aspace 0
 strcpy(identifier[id].name, "self"); // each aspace has a "self" identifier (because its own name is not otherwise within its own aspace scope)
 identifier[id].type = ASM_ID_ASPACE;
 identifier[id].aspace_scope = 0;
 identifier[id].nspace_scope = -1;
 identifier[id].asm_or_compiler = IDENTIFIER_ASM_ONLY;
 identifier[id].aspace_index = 0; // self for process 0 is aspace 0
 identifier[id].address_bcode = 0;
 astate.aspace [0].identifier_index = id; // not sure about this - probably doesn't matter/is unnecessary
 astate.aspace [0].self_identifier_index = id;
 id++;

// identifier[id].scope = 0; I don't think this is correct for a cfunction identifier



 for (i = 0; i < ARRAY_INIT_SIZE; i ++)
 {
  array_init.value [i] = 0;
 }

}





/*
This function doesn't really belong here, but it needs access to the names of predefined keywords.
It's used by the syntax highlighter (in e_editor.c) to work out whether a particular word is a keyword.

It goes through the lists of various kinds of keywords and searches for the word.
It's not optimised at all.

*/
#include "e_header.h"

int check_word_type(char* word)
{
 int i;
// fprintf(stdout, "\nCheck: [%s]", word);

 for (i = 0; i < OP_end; i ++)
 {
  if (strcmp(word, opcode_properties [i].name) == 0)
   return STOKEN_TYPE_ASM_OPCODE;
 }

 for (i = 0; i < (KEYWORDS_END - KEYWORD_INT); i ++)
 {
  if (strcmp(word, c_keyword [i].name) == 0)
   return STOKEN_TYPE_KEYWORD;
 }

 for (i = 0; i < (BUILTIN_CFUNCTIONS_END - BUILTIN_CFUNCTION_PUT); i ++)
 {
  if (strcmp(word, c_builtin_cfunction [i].name) == 0)
   return STOKEN_TYPE_KEYWORD;
 }

 for (i = 0; i < (KEYWORDS_ASM_END - KEYWORD_ASM_INT); i ++)
 {
  if (strcmp(word, asm_keyword [i].name) == 0)
   return STOKEN_TYPE_ASM_KEYWORD;
 }

 return STOKEN_TYPE_WORD;

}

