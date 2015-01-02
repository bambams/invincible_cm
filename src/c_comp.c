
#include <allegro5/allegro.h>
#include "m_config.h"
#include "g_header.h"
#include "c_header.h"

#include "g_misc.h"
#include "c_prepr.h"
#include "c_ctoken.h"
#include "c_inter.h"
#include "c_init.h"
#include "c_asm.h"
#include "c_comp.h"
#include "e_log.h"
#include "e_slider.h"
#include "e_header.h"

#include "v_interp.h"
#include "m_globvars.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>


enum
{
BIND_BASIC,
BIND_COMPARISON,
BIND_ARITHMETIC

};

struct expointstruct expoint [EXPOINTS];


//struct cfunctionstruct cfunction [CFUNCTIONS];



struct cstatestruct cstate;
struct scodestruct *scode;
struct intercodestruct intercode [INTERCODE_SIZE];
struct identifierstruct identifier [IDENTIFIER_MAX]; // externed in c_prepr.c
struct process_defstruct process_def [PROCESS_DEFS];
struct array_initstruct array_init;

struct sourcestruct test_src;

extern struct astatestruct astate; // in a_asm.c


//int compile_source_to_bcode(struct sourcestruct* source, struct bcodestruct* bcode, struct sourcestruct* asm_source, int crunch_asm, const char* defined);
//void init_compiler(void);

int allocate_exit_point(int type);
int add_intercode(int ic_type, int op1, int op2, int op3);
int comp_error(int error_type, struct ctokenstruct* ctoken);
int comp_error_minus1(int error_type, struct ctokenstruct* ctoken);
int new_cfunction(int return_type, int storage_class);
int compile(struct scodestruct* scode_source);

int comp_next(int cmode, int value);
int comp_current(int cmode, int value, struct ctokenstruct* ctoken);
int skip_to_after_closing_bracket(void);

int verify_cfunction_type(int type, int storage_class);
int check_prototype_or_definition(void);
int auto_var_address_to_register(int id_index, int target_register, int offset);
void dereference_loop(int deref_register, int* dereferences);

int parse_expression(int first_register, int second_register, int save_second_register, int exit_point, int accept_assignment_at_end, int binding);
int parse_expression_value(int target_register, int secondary_register, int save_secondary_register, int exit_point, int binding);
int parse_expression_operator(int target_register, int secondary_register, int save_secondary_register, int exit_point, int binding);
int attempt_expression_resolve(int target_register);
int attempt_expression_resolve_to_address(int* id_index, int* offset);

int parse_array_reference(int target_register, int secondary_register, int save_secondary_register, int id_index, int get_address);
int initialise_array_dimension(int id_index, int dim, int dim_offset);

int parse_get(int target_register, int secondary_register, int save_secondary_register);
int parse_put(void);
int parse_get_index(int target_register, int secondary_register, int save_secondary_register);
int parse_put_index(void);
int parse_process_start_or_end(int target_register, int end);
int expression_value_to_mbank(int mb);
int parse_call(int target_register, int secondary_register, int save_secondary_register);

int comp_statement(int statement_exit_point, int final_closing_bracket);
int assignment_operation(int operator_subtype, int reg1, int reg2, int reg3);

int parse_interface_definition(void);
int get_constant_initialiser(int* value);
int parse_system_program_interface(struct interfacestruct* iface);
int parse_preinit_array_field(int* preinit_array, int elements, int default_first_value, int default_other_values, int check_trailing_comma);

int parse_enum_definition(void);

int comp_process(void);
int comp_switch(void);
int find_case_index(int value, int total_values, s16b case_value [SWITCH_CASES]);

int simple_built_in_cfunction_call(int method_type, int method_status, int args, int target_register, int secondary_register, int save_secondary_register);
int find_method_index(int method_type);

enum
{
CM_BASIC,
CM_GLOBAL_INT,
CM_GLOBAL_VOID,
CM_PROTOTYPE,
CM_PROTOTYPE_ARGS,
CM_DECLARE_INT_STATIC,
CM_DECLARE_INT_AUTO,
CM_CFUNCTION,
CM_CFUNCTION_ARGS,
CM_CALL_CFUNCTION,
CM_RETURN_STATEMENT,
CM_RETURN,
//CM_STATEMENT,
//CM_EXPRESSION_WRAPPER,
//CM_EXPRESSION,
//CM_EXPRESSION_VALUE,
//CM_EXPRESSION_OPERATOR_LOOP,
//CM_EXPRESSION_OPERATOR,
CM_IF,
CM_ARRAY_DECLARATION,
CM_ARRAY_DECLARATION_SUB,
CM_ARRAY_REFERENCE,
CM_PRINT,
CM_PRINT_CONTENTS,
CM_DO,
CM_WHILE,
CM_FOR,
CM_INCR_OPERATION,
CM_DECR_OPERATION,
CM_END_ASSIGNMENT_STATEMENT,
CM_ARRAY_INITIALISATION_STATIC,
CM_GOTO,
};

enum
{
EXPR_ERROR,
EXPR_VAL_IN_WORKING,
EXPR_VAL_IN_TEMP,

};



struct sourcestruct* debug_source; // this is a temporary measure to allow error code to print source code. It can't deal with #included files and will need to be fixed.
//struct sourcestruct asm_source; // if an asm version of the code is being written, this stores it

struct scodestruct new_scode;


// This compiles a sourcestruct to bcode
// If asm_source isn't NULL, it also writes asm code to asm_source
// defined is a single predefined macro (e.g. when a mission file is used as an advanced mission, it has the ADVANCED macro defined)
int compile_source_to_bcode(struct sourcestruct* source, struct bcodestruct* bcode, struct sourcestruct* asm_source, int crunch_asm, const char* defined)
{

 debug_source = source;


// now initialise cfunctions (which must come before init_identifiers)
 int i;

 init_assembler(bcode->bcode_size);

// need to init identifiers before preprocessing, as keyword identifiers need to be known to avoid clashes with macro names
 init_identifiers(bcode->bcode_size); // in ac_init.c. Must come after call to init_assembler(), as it calls allocate_nspace() (init_assembler is needed to initialise the nspace arrays)

// now set up the main process:
 cstate.cprocess = &process_def[0];
 cstate.process_scope = 0;
 cstate.scope = SCOPE_GLOBAL;
 process_def[0].defined = 1;
 process_def[0].start_address = 0;
 process_def[0].parent = -1; // index of parent process. Is -1 for process 0.

 if (!preprocess(source, &new_scode, defined))
 {
  return 0;
 }

 write_line_to_log("Finished preprocessing.", MLOG_COL_COMPILER);

 if (!compile(&new_scode))
  return 0;
// calling compile writes the code to the intercode data structures (see c_inter.c) in intercode format

// The last thing to do before generating bcode is to try to find any asm generic identifiers that are not yet aliased to a compiler identifier
 if (!resolve_undefined_asm_generics())
  return 0;

//  print_identifier_list();

// if asm_source is NULL, don't need to produce an asm source:
 if (asm_source == NULL)
 {
  if (!intercode_to_bcode_or_asm(bcode, NULL, 0)) // 0 means don't produce asm
   return 0;
 }
  else
  {
   for (i = 0; i < SOURCE_TEXT_LINES; i ++)
   {
    asm_source->text [i] [0] = '\0';
   }
   asm_source->from_a_file = 0;
   strcpy(asm_source->src_file_name, "asm");
   strcpy(asm_source->src_file_path, "asm");

   if (!intercode_to_bcode_or_asm(bcode, asm_source, 1 + crunch_asm)) // 1 means produce asm (uncrunched). 2 means produced crunched asm.
    return 0;

  }

 strcpy(bcode->src_file_name, source->src_file_name);
 strcpy(bcode->src_file_path, source->src_file_path);

 write_line_to_log("Finished compiling.", MLOG_COL_COMPILER);

//print_identifier_list(); <- uncomment to print a full list after compiling

// If we are compiling a known program type, the compiler will have checked the bcode length against bcode->bcode_size.
// But if we're running a test build, the program type isn't known in advance and bcode->bcode_size will be BCODE_MAX.
// So now we check whether the bcode fits in bcode size for the actual program type:
 int max_bcode_size = bcode->bcode_size;
 switch(bcode->op [BCODE_HEADER_ALL_TYPE])
 {
  case PROGRAM_TYPE_SYSTEM:
   max_bcode_size = SYSTEM_BCODE_SIZE;
   break;
  case PROGRAM_TYPE_DELEGATE:
  case PROGRAM_TYPE_OBSERVER:
  case PROGRAM_TYPE_OPERATOR:
   max_bcode_size = CLIENT_BCODE_SIZE;
   break;
  case PROGRAM_TYPE_PROCESS:
   max_bcode_size = PROC_BCODE_SIZE;
   break;
 }

 if (bcode->static_length > max_bcode_size)
 {
  start_log_line(MLOG_COL_ERROR);
  write_to_log("Total bcode length is ");
  write_number_to_log(bcode->static_length); // worked out in intercode_to_bcode_or_asm()
  write_to_log("/");
  write_number_to_log(max_bcode_size);
  write_to_log(" (too long).");
  finish_log_line();
  return 0;
 }
  else
  {
   start_log_line(MLOG_COL_COMPILER);
   write_to_log("Total bcode length is ");
   write_number_to_log(bcode->static_length); // worked out in intercode_to_bcode_or_asm()
   write_to_log("/");
   write_number_to_log(max_bcode_size);
   write_to_log(".");
   finish_log_line();
   if (bcode->static_length > max_bcode_size - 100)
   {
    start_log_line(MLOG_COL_COMPILER);
    write_to_log("Warning: not much room for stack (only ");
    write_number_to_log(max_bcode_size - bcode->static_length);
    write_to_log(").");
    finish_log_line();
   }
  }

 return 1;

}




int compile(struct scodestruct* scode_source)
{

 scode = scode_source;

 int i;

 for (i = 0; i < EXPOINTS; i ++)
 {
  expoint[i].true_point_icode = -1;
  expoint[i].false_point_icode = -1;
  expoint[i].true_point_bcode = -1;
  expoint[i].false_point_bcode = -1;
  expoint[i].true_point_used = 0;

  expoint[i].false_point_used = 0;
 }
 cstate.expoint_pos = 0;
 cstate.array_init_pos = 0;

// preprocess(source, scode);

 cstate.scode_pos = -1;
 cstate.icode_pos = 0;
 cstate.error = CERR_NONE;
 cstate.recursion_level = 0;

 cstate.running_asm = 0; // is set to 1 and back to 0 as needed in a_asm.c

 int compiler_state;

 do
 {
  compiler_state = comp_next(CM_BASIC, 0);
 }
  while(compiler_state == 1);

// if compiler_state is zero, there's been an error
// if it's 2, the compiler has finished successfully.

 if (compiler_state == 2)
  return 1;

 return 0;

}


/*
globals:
struct scodestruct scode;

struct cstatestruct cstate
{
 int scount;
 int error;
 int recursion_level;
 int scope; // current cfunction. Is -1 if outside a function.
  // need to make sure keywords have universal scope
};


associated functions:

read_next(ctoken) - reads the next token into local ctoken struct
 - remember to make sure that it respects scope
* done (I think)

check_prototype_or_definition() - reads ahead to determine whether an "int" in basic mode is a prototype, a function definition or a variable definition. returns: 0 error 1 prototype 2 fn definition 3 var def
 *** actually not needed - just assume that the first time a function heading is found it's a prototype, and give an error if not.

check_end_of_scode() - returns 1 if we're at the end of the scode (is only called in basic mode)
* done

check_next(int ctoken_type, int ctoken_subtype) - pass a string; if the next ctoken is that string, returns 1 and leaves scode_pos at the end of the string. Otherwise, returns 0 and leaves scode_pos where it was.
- will probably have to pass ctoken type and subtype to check_next, so it can use the same code as read_next to read in the next token
* done

new_cfunction(int return_type):
 - returns the next empty element in the array of cfunctions
 - returns -1 if no space left
 - sets cfunction return type
 - sets cfunction args to zero
 - sets cfunction[].defined to zero (will be set to 1 when cfunction is defined)

read_numbers()
 - reads numbers

add_cfunction_to_intercode(int cfunction_index)
 - adds something to intercode to indicate that a function starts here.
 - in pass2, this will be replaced with enough padding to store any static parameters and static local variables, and the entry point for the cfunction will be set to the end of the padding.
  - actually, it would probably be better to just store all of the static stuff at the start of the whole bcode

add_int_to_intercode(int identifier_index)
 - adds a variable declaration to intercode.
 - note: the size of the variable in memory may be unknown at the time this is called (as the var may be an array).
 - It is worked out later (when intercode is processed) based on information in the identifier entry.
  - actually, could be wrong - I think it will have to be known at this time
 - actually - is this even needed? The intercode parser can just go through the identifier list and reserve memory for all of the variables

int read_literal_numbers(struct ctokenstruct* ctoken)
 - reads scode to find all literal numbers and calculate them - e.g. 5 + (6 * 3)
 - accepts numbers, +*-/, and ()
 - returns 1 when it finds any other thing
 - returns 0 on end of scode.
 - puts the actual value calculated into ctoken.literal_number
 - is used for things like array dimension sizes, that can only be constants
* done

int verify_function_type(int type)
 - peeks at next token (does not update scode position)
 - makes sure that the token identifiers a cfunction that has been prototyped
 - checks that the type parameter (current values are KEYWORD_INT and KEYWORD_VOID) matches the type of that cfunction.
 - returns 1 on success, 0 on failure

int peek_next(struct ctokenstruct *ctoken)
 - doesn't increase scode counter.
* done

Notes:
read_next should read in the next ctoken and:
 - if ctoken is an identifer:
  - if it exists already in the present scope:
   - set ctoken.identifier_index
   - set ctoken.type to CTOKEN_TYPE_IDENTIFIER_(xxxx) where (xxxx) is the type of identifier it is
  - if it doesn't exist in the present scope:
   - create a new identifier with present scope
   - set ctoken.identifier_index
   - set ctoken.type to CTOKEN_TYPE_IDENTIFIER_NEW

if ctoken is CTOKEN_TYPE_IDENTIFIER, read_next must have found (or created) a valid identifier, and ctoken.identifier_index should be set.


*/


int comp_next(int cmode, int value)
{

 if (cstate.error != CERR_NONE)
  return 0;

 cstate.recursion_level ++;

 if (cstate.recursion_level > RECURSION_LIMIT)
	{
  return comp_error(CERR_RECURSION_LIMIT_REACHED_IN_STATEMENT, NULL);
	}

 struct ctokenstruct ctoken;
 strcpy(ctoken.name, "(undefined)");

 int retval = comp_current(cmode, value, &ctoken);
 cstate.recursion_level --;
 return retval;

}


int comp_current(int cmode, int value, struct ctokenstruct* ctoken)
{

 int id_index;
// int cfunction_index;
 int exit_point;
 int exit_point_loop;
 int i, j;
// int dereference = 0; // used if a * dereference prefix is found
// int negative = 0; // used if a - negative prefix (for a literal number) is found
 int array_size;
 int retval;
// int array_reference;
 int save_scode_pos;
// int expression_result;

 if (cstate.error != CERR_NONE)
  return 0;

 switch(cmode)
 {
  case CM_BASIC:
   if (check_end_of_scode())
   {
    return 2; // finished! (This check is only needed in this cmode because if we run out of scode in any other mode it's an error)
   }
   if (!read_next(ctoken))
    return comp_error(CERR_READ_FAIL, ctoken);
   if (ctoken->type == CTOKEN_TYPE_IDENTIFIER_KEYWORD
    || ctoken->type == CTOKEN_TYPE_IDENTIFIER_BUILTIN_CFUNCTION) // builtin cfunctions don't work here, but using some of them gives a special error (used for identifying unimplemented/unsupported C stuff)
   {
    switch(ctoken->identifier_index)
    {
     case KEYWORD_INT:
      if (!comp_next(CM_GLOBAL_INT, STORAGE_STATIC))
       return 0;
      break;
     case KEYWORD_VOID:
      if (!comp_next(CM_GLOBAL_VOID, STORAGE_STATIC))
       return 0;
      break;
     case KEYWORD_STATIC:
      if (!read_next(ctoken))
       return comp_error(CERR_READ_FAIL, ctoken);
      if (ctoken->type != CTOKEN_TYPE_IDENTIFIER_KEYWORD)
       return comp_error(CERR_STORAGE_SPECIFIER_MISUSE, ctoken);
      switch(ctoken->identifier_index)
      {
       case KEYWORD_INT:
        if (!comp_next(CM_GLOBAL_INT, STORAGE_STATIC))
         return 0;
        break;
       case KEYWORD_VOID:
        if (!comp_next(CM_GLOBAL_VOID, STORAGE_STATIC))
         return 0;
        break;
       default:
        return comp_error(CERR_STORAGE_SPECIFIER_MISUSE, ctoken);
      }
      break; // end KEYWORD_STATIC
     case KEYWORD_AUTO:
      if (!read_next(ctoken))
       return comp_error(CERR_READ_FAIL, ctoken);
      if (ctoken->type != CTOKEN_TYPE_IDENTIFIER_KEYWORD)
       return comp_error(CERR_STORAGE_SPECIFIER_MISUSE, ctoken);
      switch(ctoken->identifier_index)
      {
       case KEYWORD_INT:
        if (!comp_next(CM_GLOBAL_INT, STORAGE_AUTO))
         return 0;
        break;
       case KEYWORD_VOID:
        if (!comp_next(CM_GLOBAL_VOID, STORAGE_AUTO))
         return 0;
        break;
       default:
        return comp_error(CERR_STORAGE_SPECIFIER_MISUSE, ctoken);
      }
      break; // end KEYWORD_STATIC
     case KEYWORD_INTERFACE:
      if (!parse_interface_definition())
       return 0;
      break; // end KEYWORD_INTERFACE
     case KEYWORD_PROCESS:
      if (!comp_process())
       return 0;
      break;
     case KEYWORD_ENUM:
      if (!parse_enum_definition())
       return 0;
      break;
     case KEYWORD_ASM:
      if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACE_OPEN))
       return comp_error(CERR_EXPECTED_BRACE_OPEN, ctoken);
      if (!assemble(process_def[cstate.process_scope].asm_only))
       return 0;
      break;
     case KEYWORD_ASM_ONLY:
      process_def[cstate.process_scope].asm_only = 1;
//      if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_SEMICOLON)) // probably don't want to require a ; as asm_only isn't a statement
//       return comp_error(CERR_EXPECTED_SEMICOLON, ctoken);
      break;

     case BUILTIN_UNIMP_CONST:
     case BUILTIN_UNIMP_SIZEOF:
     case BUILTIN_UNIMP_STRUCT:
     case BUILTIN_UNIMP_UNION:
     case BUILTIN_UNIMP_EXTERN:
     case BUILTIN_UNIMP_UNSIGNED:
     case BUILTIN_UNIMP_SIGNED:
     case BUILTIN_UNIMP_TYPEDEF:
     case BUILTIN_UNIMP_INLINE:
     case BUILTIN_UNIMP_REGISTER:
       return comp_error(CERR_UNIMPLEMENTED_KEYWORD, ctoken);

     case BUILTIN_UNSUP_CHAR:
     case BUILTIN_UNSUP_SHORT:
     case BUILTIN_UNSUP_FLOAT:
     case BUILTIN_UNSUP_DOUBLE:
       return comp_error(CERR_UNSUPPORTED_DATA_TYPE, ctoken);

    } // end identifier index switch
   }
    else  // not an identifier
     return comp_error(CERR_SYNTAX_BASIC_MODE, ctoken);
   break; // end case CM_BASIC




//   }
   case CM_GLOBAL_INT: // has just read "int" and is expecting a prototype, cfunction definition or variable declaration. Value is the storage class.
      switch(check_prototype_or_definition())
// probably don't actually need to check this - can assume that the first time a function heading is found, it's a prototype - and give an error if it doesn't end with ;
//  although will still need to look ahead to eliminate possibility of it being an int declaration (but not so far)
      {
       case 0:
        return 0; // comp_error has been called by check_prototype_or_definition
       case 1: // prototype
        if (!comp_next(CM_PROTOTYPE, new_cfunction(CFUNCTION_RETURN_INT, value)))
         return 0;
        break;
       case 2: // cfunction definition
        if (!verify_cfunction_type(CFUNCTION_RETURN_INT, value))
         return comp_error(CERR_CFUNCTION_MISTYPE, ctoken);
        if (!comp_next(CM_CFUNCTION, 0))
         return 0;
        break;
       case 3: // variable declaration
        if (value == STORAGE_AUTO) // can't have automatic global variables
         return comp_error(CERR_GLOBAL_VARIABLE_IS_AUTO, ctoken);
        if (!comp_next(CM_DECLARE_INT_STATIC, 0))
         return 0;
        break;
      }
      break;

    case CM_GLOBAL_VOID: // has just read "void" and is expecting a prototype or cfunction definition. Value is the storage class.
      switch(check_prototype_or_definition())
      {
       case 0:
        return 0; // comp_error has been called by check_prototype_or_definition
       case 1: // prototype
        if (!comp_next(CM_PROTOTYPE, new_cfunction(CFUNCTION_RETURN_VOID, value)))
         return 0;
        cstate.scope = SCOPE_GLOBAL; // return to global scope
        break;
       case 2: // cfunction definition
        if (!verify_cfunction_type(CFUNCTION_RETURN_VOID, value))
         return comp_error(CERR_CFUNCTION_MISTYPE, ctoken);
        if (!comp_next(CM_CFUNCTION, 0))
         return 0;
        cstate.scope = SCOPE_GLOBAL; // return to global scope
        break;

       case 3: // variable declaration
        return comp_error(CERR_VOID_VARIABLE, ctoken); // void type not available for variables
      }
      break;


  case CM_PROTOTYPE:
   if (value == -1)
    return comp_error(CERR_CFUNCTION_FAIL, ctoken);
   save_scode_pos = cstate.scode_pos;
   if (!read_next(ctoken))
    return comp_error(CERR_READ_FAIL, ctoken);
   if (ctoken_compare_text(ctoken, "main"))
    return comp_error(CERR_MAIN_PROTOTYPED, ctoken);
   if (ctoken->type == CTOKEN_TYPE_IDENTIFIER_NEW
				|| ctoken->type == CTOKEN_TYPE_ASM_GENERIC_UNDEFINED)
   {
// assumes read_next set ctoken.identifier_index
// assumes value is the index of the new function in the cfunction array.
// assumes new_cfunction() was called, and the type of the new function has been set.
    identifier[ctoken->identifier_index].type = ID_USER_CFUNCTION;
    identifier[ctoken->identifier_index].cfunction_index = value;
    identifier[ctoken->identifier_index].asm_or_compiler = IDENTIFIER_C_ONLY; // the corresponding nspace will be available for use in asm
    identifier[ctoken->identifier_index].process_scope = cstate.process_scope;
    identifier[ctoken->identifier_index].scope = SCOPE_GLOBAL;
    cstate.cprocess->cfunction[value].identifier_index = ctoken->identifier_index;
    cstate.scope = value;

// now we reset scode_pos and read the name again, this time pretending that we're in asm code. This creates an asm identifier that can be used to create a corresponding namespace:
   cstate.scode_pos = save_scode_pos;
   cstate.running_asm = 1;
   astate.current_aspace = cstate.cprocess->corresponding_aspace;
   astate.current_nspace = -1;

   struct ctokenstruct nspace_ctoken;
   int nspace_index;
   if (!read_next(&nspace_ctoken)) // can recycle ctoken; it's not used again
    return comp_error(CERR_READ_FAIL, &nspace_ctoken);
// check whether the nspace has been implicitly declared (e.g. by an asm scope command)
   if (nspace_ctoken.type == CTOKEN_TYPE_ASM_NSPACE)
   {
    nspace_index = identifier[nspace_ctoken.identifier_index].nspace_index;
    if (astate.nspace[nspace_index].defined)
     return comp_error(CERR_CFUNCTION_NAME_ALREADY_USED_IN_ASM, &nspace_ctoken); // is this the right error? I think so.
   }
    else
    {
// if not implicitly declared, must be an unused identifier:
     if (nspace_ctoken.type != CTOKEN_TYPE_IDENTIFIER_NEW
      && nspace_ctoken.type != CTOKEN_TYPE_ASM_GENERIC_UNDEFINED)
     {
      return comp_error(CERR_CFUNCTION_NAME_ALREADY_USED_IN_ASM, &nspace_ctoken);
     }
//      return comp_error(CERR_SYNTAX, &nspace_ctoken);
// now allocate an nspace to the identifier:
     if (!allocate_nspace(nspace_ctoken.identifier_index)) // allocate_nspace finds an unused nspace
      return 0;
     nspace_index = identifier[nspace_ctoken.identifier_index].nspace_index;
    }
// now allocate an nspace to the identifier:
   cstate.cprocess->cfunction[cstate.scope].corresponding_nspace = nspace_index;
   identifier[nspace_ctoken.identifier_index].scope = SCOPE_GLOBAL; // not sure this means anything
   identifier[nspace_ctoken.identifier_index].type = ASM_ID_NSPACE;
// identifier[ctoken.identifier_index].nspace_index =  - this has already been set by allocate_aspace()
   identifier[nspace_ctoken.identifier_index].aspace_scope = cstate.cprocess->corresponding_aspace;
   identifier[nspace_ctoken.identifier_index].nspace_scope = -1; // the actual name of the nspace has global scope
   identifier[nspace_ctoken.identifier_index].asm_or_compiler = IDENTIFIER_ASM_ONLY;
// add intercode to enter the aspace (doesn't do anything unless asm code is being generated. If inline asm is found within the process, current aspace is derived from the current process)
   astate.nspace [nspace_index].declared = 1; // may already be 1
   astate.nspace [nspace_index].defined = 1;
   astate.nspace [nspace_index].aspace_scope = cstate.cprocess->corresponding_aspace;
   astate.nspace [nspace_index].identifier_index = nspace_ctoken.identifier_index;
   astate.nspace [nspace_index].corresponding_cfunction = cstate.scope;

// now go back to the compiler. don't need to update scode_pos as it will be where it was before the nspace token parsing
   cstate.running_asm = 0;


    if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACKET_OPEN))
     return comp_error(CERR_PROTOTYPE_EXPECTED_BRACKET_OPEN, ctoken);
    if (!comp_next(CM_PROTOTYPE_ARGS, value)) // reads function args up to and including the closing ')'
     return 0;
//    if (!add_cfunction_prototype_to_intercode(value)) // not sure this is needed...
//     return comp_error(1);
    if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_SEMICOLON))
     return comp_error(CERR_PROTOTYPE_EXPECTED_SEMICOLON, ctoken);
    cstate.scope = SCOPE_GLOBAL;
   }
    else
     return comp_error(CERR_PROTOTYPE_EXPECTED_NEW_IDENTIFIER, ctoken);
   break; // end case CM_PROTOTYPE

// reads in the parameters in a cfunction prototype and sets up identifiers and entries in the cfunction struct for the parameters
  case CM_PROTOTYPE_ARGS:
// assumes value is the index of the cfunction in the cfunction array
   cstate.cprocess->cfunction[value].arguments = 0;
   if (!read_next(ctoken))
    return comp_error(CERR_READ_FAIL, ctoken);
   if (ctoken->type == CTOKEN_TYPE_IDENTIFIER_KEYWORD
    && ctoken->identifier_index == KEYWORD_VOID)
   {
     if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACKET_CLOSE))
      return comp_error(CERR_PROTOTYPE_EXPECTED_BRACKET_CLOSE, ctoken);
     break; // cfunction has no arguments (don't need to do anything else as new_cfunction initialises the cfunction with no arguments).
   }
   if (ctoken->type == CTOKEN_TYPE_PUNCTUATION
    && ctoken->subtype == CTOKEN_SUBTYPE_BRACKET_CLOSE)
    break; // () is treated same as (void)
   int storage_class;
   while(TRUE)
   {
// there's a read_next at the end of this loop (the first time through the loop, the ctoken that was read in just before, when we were checking for void, is used)
   storage_class = cstate.cprocess->cfunction[value].storage_class;

// first we check for a storage class specifier
    if (ctoken->type == CTOKEN_TYPE_IDENTIFIER_KEYWORD)
    {
     if (ctoken->identifier_index == KEYWORD_STATIC)
     {
      storage_class = STORAGE_STATIC;
      if (!read_next(ctoken)) // read the variable type
       return comp_error(CERR_READ_FAIL, ctoken);
     }
      else
      {
       if (ctoken->identifier_index == KEYWORD_AUTO)
       {
        if (cstate.cprocess->cfunction[value].storage_class == STORAGE_STATIC)
         return comp_error(CERR_AUTO_VAR_IN_STATIC_CFUNCTION, ctoken);
        if (!read_next(ctoken)) // read the variable type
         return comp_error(CERR_READ_FAIL, ctoken);
       }
      }
    } // finish looking for storage class specifier

    if (ctoken->type == CTOKEN_TYPE_IDENTIFIER_KEYWORD
     && ctoken->identifier_index == KEYWORD_INT)
    {
     if (!read_next(ctoken))
      return comp_error(CERR_READ_FAIL, ctoken);
     if (ctoken->type == CTOKEN_TYPE_IDENTIFIER_NEW
				  || ctoken->type == CTOKEN_TYPE_ASM_GENERIC_UNDEFINED)
     {
      identifier[ctoken->identifier_index].type = ID_USER_INT;
      identifier[ctoken->identifier_index].storage_class = storage_class;
      identifier[ctoken->identifier_index].aspace_scope = cstate.cprocess->corresponding_aspace;
      identifier[ctoken->identifier_index].nspace_scope = cstate.cprocess->cfunction[value].corresponding_nspace;
      identifier[ctoken->identifier_index].asm_or_compiler = IDENTIFIER_ASM_OR_C; // accessible from asm (although if auto, gives the stack offset rather than an address)
      identifier[ctoken->identifier_index].scope = cstate.scope;
      identifier[ctoken->identifier_index].process_scope = cstate.process_scope;
// check for an array
      if (check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_SQUARE_OPEN))
      {
       if (!comp_next(CM_ARRAY_DECLARATION, ctoken->identifier_index))
        return 0;
/*       for (i = 0; i < identifier[ctoken->identifier_index].array_dims; i ++)
       {
        array_size *= identifier[ctoken->identifier_index].array_dim_size [i];
       }*/
// function parameters that are arrays are passed as a special kind of implicit pointer
// yes I know this is wrong
       identifier[ctoken->identifier_index].pointer = 1;
      }
// if an automatic variable, make room on the stack
      if (storage_class == STORAGE_AUTO)
      {
       identifier[ctoken->identifier_index].stack_offset = cstate.cprocess->cfunction[value].size_on_stack + 1;
       cstate.cprocess->cfunction[value].size_on_stack ++;
      }
      cstate.cprocess->cfunction[value].arg_type [cstate.cprocess->cfunction[value].arguments] = CFUNCTION_ARG_INT;
      cstate.cprocess->cfunction[value].arg_identifier [cstate.cprocess->cfunction[value].arguments] = ctoken->identifier_index;
      cstate.cprocess->cfunction[value].arguments ++;

      if (cstate.cprocess->cfunction[value].arguments >= CFUNCTION_ARGS)
       return comp_error(CERR_EXCESS_CFUNCTION_ARGS, ctoken);
     }
      else
       return comp_error(CERR_IDENTIFIER_REUSED, ctoken);
    } // end if (ctoken is an int declaration)
     else
      return comp_error(CERR_PROTOTYPE_PARAMETER_NOT_INT, ctoken);
// The parameter declaration must be followed by either ')' or ',':
    if (!read_next(ctoken))
     return comp_error(CERR_READ_FAIL, ctoken);
    if (ctoken->type == CTOKEN_TYPE_PUNCTUATION)
    {
     if (ctoken->subtype == CTOKEN_SUBTYPE_BRACKET_CLOSE)
      break; // finished prototype (semicolon will be read later).
     if (ctoken->subtype != CTOKEN_SUBTYPE_COMMA)
      return comp_error(CERR_PROTOTYPE_PARAMETER_SYNTAX, ctoken);
    }
// okay, at this point we've skipped a comma and can go on to the next parameter:
    if (!read_next(ctoken)) // the start of the loop will expect this ctoken to be int
     return comp_error(CERR_READ_FAIL, ctoken);
   } // end while loop that goes through arguments
   break; // end CM_PROTOTYPE_ARGS

  case CM_DECLARE_INT_STATIC:
// note that this cmode can be called from inside or outside a function (although it isn't used for cfunction parameters)
// the scope of the variable is set (in identifier[].scope) when the identifier entry is created (as a result of calling read_next)
// an int can only be initialised to a constant (deal with special := and ::= operators later)
   if (!read_next(ctoken))
    return comp_error(CERR_READ_FAIL, ctoken);
   if (ctoken->type != CTOKEN_TYPE_IDENTIFIER_NEW
				&& ctoken->type != CTOKEN_TYPE_ASM_GENERIC_UNDEFINED)
    return comp_error(CERR_VAR_NAME_IN_USE, ctoken);
// changes here may need to be reflected in the code for assembler int declarations/definitions in asm_c
   id_index = ctoken->identifier_index;
   identifier[id_index].type = ID_USER_INT;
   identifier[id_index].storage_class = STORAGE_STATIC;
   identifier[id_index].scope = cstate.scope;
   identifier[id_index].process_scope = cstate.process_scope;
// now set up aspace and nspace in case the variable is referred to in inline asm:
   identifier[id_index].aspace_scope = cstate.cprocess->corresponding_aspace;
   identifier[id_index].asm_or_compiler = IDENTIFIER_ASM_OR_C; // accessible from asm
   if (cstate.scope == SCOPE_GLOBAL)
    identifier[id_index].nspace_scope = -1;
     else
      identifier[id_index].nspace_scope = cstate.cprocess->cfunction[cstate.scope].corresponding_nspace;
// the variable's scope is set by read_next when the new identifier is created (see new_c_identifier() in c_ctoken.c)
   if (check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_SQUARE_OPEN))
   {
    if (!comp_next(CM_ARRAY_DECLARATION, id_index))
     return 0;
   }
   if (!read_next(ctoken))
    return comp_error(CERR_READ_FAIL, ctoken);
   if (ctoken->type == CTOKEN_TYPE_PUNCTUATION
    || ctoken->type == CTOKEN_TYPE_OPERATOR_ASSIGN)
   {
    switch(ctoken->subtype)
    {
     case CTOKEN_SUBTYPE_COMMA:
      return comp_next(CM_DECLARE_INT_STATIC, 0);

     case CTOKEN_SUBTYPE_SEMICOLON:
      break; // finished

     case CTOKEN_SUBTYPE_EQ:
      if (identifier[id_index].array_dims > 0
       && identifier[id_index].storage_class == STORAGE_STATIC)
       return comp_next(CM_ARRAY_INITIALISATION_STATIC, id_index);
      if (!read_next_expression_value(ctoken))
       return comp_error(CERR_READ_FAIL, ctoken);
      if (ctoken->type != CTOKEN_TYPE_NUMBER)
       return comp_error(CERR_VAR_INITIALISATION_FAIL, ctoken);
      identifier[id_index].initial_value = ctoken->number_value; // need to deal with arrays here
// at this point, anything other than a ; or , is an error
      if (!read_next(ctoken))
       return comp_error(CERR_READ_FAIL, ctoken);
      if (ctoken->type == CTOKEN_TYPE_PUNCTUATION)
      {
       if (ctoken->subtype == CTOKEN_SUBTYPE_SEMICOLON)
        break;
       if (ctoken->subtype == CTOKEN_SUBTYPE_COMMA)
        return comp_next(CM_DECLARE_INT_STATIC, 0);
      }
     break;

/*
Not currently implemented
    case CTOKEN_SUBTYPE_COLON: // := is a special assignment for static variables: it inserts an assignment at the start of the bcode (so the assignment occurs each runthrough instead of only at compilation)
     if (!check_next(CTOKEN_TYPE_OPERATOR_ASSIGN, CTOKEN_SUBTYPE_EQ))
      return comp_error(CERR_VAR_DECLARATION_FAIL, ctoken);
     if (!read_next_expression_value(ctoken))
      return comp_error(CERR_READ_FAIL, ctoken);
     if (ctoken->type != CTOKEN_TYPE_NUMBER)
      return comp_error(CERR_VAR_INITIALISATION_FAIL, ctoken);
     identifier[id_index].initial_value = ctoken->number_value; // need to deal with arrays here
     identifier[id_index].static_int_assign_at_startup = 1;
// at this point, anything other than a ; or , is an error
     if (!read_next(ctoken))
      return comp_error(CERR_READ_FAIL, ctoken);
     if (ctoken->type == CTOKEN_TYPE_PUNCTUATION)
     {
      if (ctoken->subtype == CTOKEN_SUBTYPE_SEMICOLON)
       break;
      if (ctoken->subtype == CTOKEN_SUBTYPE_COMMA)
       return comp_next(CM_DECLARE_INT_STATIC, 0);
     }
     break;*/

    default:
     return comp_error(CERR_VAR_DECLARATION_FAIL, ctoken);
    } // end ctoken subtype switch for punctuation
   }
    else
     return comp_error(CERR_VAR_DECLARATION_FAIL, ctoken);
   break;  // end CM_DECLARE_STATIC_INT

  case CM_ARRAY_INITIALISATION_STATIC: // value is id_index of array
// TO DO: I don't think I've gotten around to auto array initialisation yet
   if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACE_OPEN))
    return comp_error(CERR_SYNTAX, ctoken);
   identifier[value].static_array_init_offset = cstate.array_init_pos;
   if (!initialise_array_dimension(value, 0, cstate.array_init_pos))
    return 0;
   identifier[value].static_array_initialised = 1;
   cstate.array_init_pos += identifier[value].array_element_size [0] * identifier[value].array_dim_size [0];
   if (cstate.array_init_pos >= ARRAY_INIT_SIZE)
    return comp_error(CERR_ARRAY_INITIALISATION_TOO_LARGE, ctoken);
   if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_SEMICOLON))
    return comp_error(CERR_EXPECTED_SEMICOLON, ctoken);
   break; // end CM_ARRAY_INITIALISATION_STATIC


  case CM_DECLARE_INT_AUTO:
// This cmode can be called only from an automatic cfunction (although it isn't used for cfunction parameters). Assumes that the cfunction is not static.
// the scope of the variable is set (in identifier[].scope) when the identifier entry is created (as a result of calling read_next)
// an int can only be initialised to a constant (deal with special := and ::= operators later)
//   *** actually this isn't a necessary rule for automatic variables. Think about this.
   if (!read_next(ctoken))
    return comp_error(CERR_READ_FAIL, ctoken);
   if (ctoken->type != CTOKEN_TYPE_IDENTIFIER_NEW
				&& ctoken->type != CTOKEN_TYPE_ASM_GENERIC_UNDEFINED)
    return comp_error(CERR_VAR_DECLARATION_FAIL, ctoken);
   id_index = ctoken->identifier_index;
   identifier[id_index].type = ID_USER_INT;
   identifier[id_index].storage_class = STORAGE_AUTO;
// the variable's scope was set by read_next, when the new identifier was created (see new_c_identifier() in c_ctoken.c)
   identifier[id_index].stack_offset = cstate.cprocess->cfunction[cstate.scope].size_on_stack + 1;
   identifier[id_index].asm_or_compiler = IDENTIFIER_ASM_OR_C; // accessible from asm (although as an offset rather than an actual address)
   identifier[id_index].scope = cstate.scope;
   identifier[id_index].process_scope = cstate.process_scope;
   identifier[id_index].aspace_scope = cstate.cprocess->corresponding_aspace;
   if (check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_SQUARE_OPEN))
   {
    if (!comp_next(CM_ARRAY_DECLARATION, id_index))
     return 0;
    array_size = 1;
    for (i = 0; i < identifier[id_index].array_dims; i ++)
    {
     array_size *= identifier[id_index].array_dim_size [i];
    }
    cstate.cprocess->cfunction[cstate.scope].size_on_stack += array_size;
   }
    else
     cstate.cprocess->cfunction[cstate.scope].size_on_stack ++;
   if (!read_next(ctoken))
    return comp_error(CERR_READ_FAIL, ctoken);
   if (ctoken->type == CTOKEN_TYPE_PUNCTUATION)
   {
    if (ctoken->subtype == CTOKEN_SUBTYPE_COMMA)
     return comp_next(CM_DECLARE_INT_AUTO, 0);
    if (ctoken->subtype == CTOKEN_SUBTYPE_SEMICOLON)
     break; // finished
   }
   if (ctoken->type == CTOKEN_TYPE_OPERATOR_ASSIGN
    && ctoken->subtype == CTOKEN_SUBTYPE_EQ)
   {
      if (!parse_expression(REGISTER_WORKING, REGISTER_TEMP, 0, -1, 0, BIND_BASIC))
       return 0; //comp_error(CERR_SYNTAX, ctoken);
// At this point, REGISTER_WORKING holds the result of the expression call above
      if (!auto_var_address_to_register(id_index, REGISTER_TEMP, 0))
       return 0;
// So now we need to copy the contents of REGISTER_WORKING to the address pointed to by REGISTER_TEMP:
      add_intercode(IC_COPY_REGISTER_TO_DEREFERENCED_REGISTER, REGISTER_TEMP, REGISTER_WORKING, 0);
// at this point, anything other than a ; or , is an error
      if (!read_next(ctoken))
       return comp_error(CERR_READ_FAIL, ctoken);
      if (ctoken->type == CTOKEN_TYPE_PUNCTUATION)
      {
       if (ctoken->subtype == CTOKEN_SUBTYPE_SEMICOLON)
        break;
       if (ctoken->subtype == CTOKEN_SUBTYPE_COMMA)
        return comp_next(CM_DECLARE_INT_AUTO, 0);
      }
   }
   return comp_error(CERR_VAR_DECLARATION_FAIL, ctoken); // end CM_DECLARE_INT


// this is called after an opening square bracket is read in a variable declaration (including function parameters)
// value is the identifier index of the variable
  case CM_ARRAY_DECLARATION:
   if (!comp_next(CM_ARRAY_DECLARATION_SUB, value))
    return 0;
   i = identifier[value].array_dims;
   array_size = 1;
// this loop calculates the size of an element of each dimension of the array, starting from the smallest (rightmost)
   while (TRUE)
   {
    i --;
    identifier[value].array_element_size [i] = array_size;
    array_size *= identifier[value].array_dim_size [i];
    if (i == 0)
     break;
   };
   break;

// this is called from inside the CM_ARRAY_DECLARATION wrapper
// is recursive
// value is identifier index of array
  case CM_ARRAY_DECLARATION_SUB:
   if (identifier[value].array_dims >= ARRAY_DIMENSIONS) // - 1)
    return comp_error(CERR_ARRAY_TOO_MANY_DIMENSIONS, ctoken);
   if (!read_next(ctoken))
    return comp_error(CERR_READ_FAIL, ctoken);
   if (ctoken->type != CTOKEN_TYPE_NUMBER)
    return comp_error(CERR_ARRAY_DIMENSION_NOT_A_NUMBER, ctoken);
   if (ctoken->number_value <= 0)
    return comp_error(CERR_ARRAY_DIMENSION_TOO_SMALL, ctoken);
   if (ctoken->number_value >= ARRAY_DIMENSION_MAX_SIZE)
    return comp_error(CERR_ARRAY_DIMENSION_TOO_LARGE, ctoken);
   identifier[value].array_dim_size [identifier[value].array_dims] = ctoken->number_value;
   identifier[value].array_dims ++;
   if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_SQUARE_CLOSE)) // confirm closing square bracket
    return comp_error(CERR_ARRAY_DECLARATION_ERROR, ctoken);
   if (check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_SQUARE_OPEN))
    return comp_next(CM_ARRAY_DECLARATION, value); // recursive call
   break;

  case CM_CFUNCTION: // assumes that scode pos is just after the type of a cfunction definition (although a syntax check will reject a prototype)
// assumes that the cfunction has been prototyped, that the type of the cfunction is correct, and that the function identifier and cfunction entry are valid (checked by verify_cfunction_type)
// does not rely on value having been set.
// Need to treat this separately from prototype because prototype can create identifiers while the definition will use the identifiers created by the prototype.
   if (!read_next(ctoken))
    return comp_error(CERR_READ_FAIL, ctoken);
   if (ctoken->type != CTOKEN_TYPE_IDENTIFIER_USER_CFUNCTION)
    return comp_error(CERR_CFUNCTION_DEFINITION_FAIL, ctoken);
// assumes read_next set ctoken.identifier_index
   cstate.scope = identifier[ctoken->identifier_index].cfunction_index;
   if (cstate.cprocess->cfunction[cstate.scope].defined)
    return comp_error(CERR_CFUNCTION_REDEFINED, ctoken);

   if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACKET_OPEN))
    return comp_error(CERR_CFUNCTION_DEFINITION_FAIL, ctoken);
   if (!comp_next(CM_CFUNCTION_ARGS, cstate.scope)) // reads function args up to and including the closing ')'
    return 0;

// add a new namespace in case inline asm is used in this cfunction:
   add_intercode(IC_ASM_DEFINE_NSPACE, cstate.cprocess->cfunction[cstate.scope].corresponding_nspace, 0, 0);

   add_intercode(IC_CFUNCTION_START, cstate.scope, 0, 0);

//   cfunction[cstate.scope].size_on_stack = 0; // fix when implementing local variables!
   cstate.cprocess->cfunction[cstate.scope].defined = 1;

   if (cstate.scope == 0) // main cfunction
   {
    add_intercode(IC_CFUNCTION_MAIN, 0,0,0);
   }

   if (!comp_statement(-1, 0))
    return 0; // comp_statement handles braces

   if (!comp_next(CM_RETURN, cstate.scope))
    return 0; // TO DO: fix this - it doesn't set a return value for functions of type int. Should really check to see if cfunction ended with a return and just not do anything if it did (and give warning if it didn't and was int type)

   add_intercode(IC_ASM_END_NSPACE, 0, 0, 0); // if we're generating asm, need to finish the cfunction's nspace here

   cstate.scope = SCOPE_GLOBAL; // finished function definition.
// need to return from the function here
// (with return value in register A if needed - should test for return with a value if the function isn't void)
// return address should be at the top of the stack - pop to B, then jump there.
   break; // end case CM_CFUNCTION

// This mode reads in the parameters of a cfunction definition.
// Parameters in a cfunction call are read in by CM_CALL_CFUNCTION
  case CM_CFUNCTION_ARGS:
// assumes value is the index of the cfunction in the cfunction array
   if (!read_next(ctoken))
    return comp_error(CERR_READ_FAIL, ctoken);
   if (ctoken->type == CTOKEN_TYPE_IDENTIFIER_KEYWORD
    && ctoken->identifier_index == KEYWORD_VOID)
//    || (ctoken->type == CTOKEN_TYPE_PUNCTUATION
//     && ctoken->subtype == CTOKEN_SUBTYPE_BRACKET_CLOSE))
   {
     if (cstate.cprocess->cfunction[value].arguments != 0)
      comp_error(CERR_CFUNCTION_ARGS_MISSING, ctoken); // expected arguments
     if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACKET_CLOSE))
      return comp_error(CERR_CFUNCTION_PARAMETER_NO_CLOSE_BRACKET, ctoken);
     break;
   }
   if (cstate.cprocess->cfunction[value].arguments == 0)
    comp_error(CERR_CFUNCTION_EXCESS_ARGS, ctoken); // expected void
   for (i = 0; i < cstate.cprocess->cfunction[value].arguments; i ++)
   {
// don't need to read_next here - we either use the value read above while checking for void, or the value read in at the end of this loop.
// first we check whether the parameter definition starts with a storage class specifier:
    if (ctoken->type == CTOKEN_TYPE_IDENTIFIER_KEYWORD)
    {
     if (ctoken->identifier_index == KEYWORD_STATIC)
     {
      if (identifier[cstate.cprocess->cfunction[value].arg_identifier [i]].storage_class != STORAGE_STATIC)
       return comp_error(CERR_CFUNCTION_PARAMETER_STORAGE_NOT_STATIC, ctoken);
      if (!read_next(ctoken)) // read the variable type
       return comp_error(CERR_READ_FAIL, ctoken);
     }
      else
      {
       if (ctoken->identifier_index == KEYWORD_AUTO)
       {
        if (identifier[cstate.cprocess->cfunction[value].arg_identifier [i]].storage_class != STORAGE_AUTO)
         return comp_error(CERR_CFUNCTION_PARAMETER_STORAGE_NOT_AUTO, ctoken);
        if (!read_next(ctoken)) // read the variable type
         return comp_error(CERR_READ_FAIL, ctoken);
       }
      }
    } // finish looking for storage class specifier

    if (ctoken->type != CTOKEN_TYPE_IDENTIFIER_KEYWORD
     || ctoken->identifier_index != KEYWORD_INT)
      return comp_error(CERR_CFUNCTION_PARAMETER_NOT_INT, ctoken);
    if (!read_next(ctoken))
     return comp_error(CERR_READ_FAIL, ctoken);
    if (ctoken->type != CTOKEN_TYPE_IDENTIFIER_USER_VARIABLE)
     return comp_error(CERR_CFUNCTION_PARAMETER_NOT_VARIABLE, ctoken);
    if (ctoken->identifier_index != cstate.cprocess->cfunction[value].arg_identifier [i])
     return comp_error(CERR_CFUNCTION_PARAMETER_MISMATCH, ctoken);
    j = 0;
    struct ctokenstruct dim_ctoken;
    while (j < identifier[ctoken->identifier_index].array_dims)
    {
     if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_SQUARE_OPEN))
      return comp_error(CERR_CFUNCTION_PARAMETER_ARRAY_NO_SQ_OP, ctoken);
// verify that the parameter matches the prototyped parameter
     if (!read_next(&dim_ctoken))
      return comp_error(CERR_READ_FAIL, &dim_ctoken);
     if (dim_ctoken.type != CTOKEN_TYPE_NUMBER)
      return comp_error(CERR_ARRAY_DIMENSION_NOT_A_NUMBER, &dim_ctoken);
     if (dim_ctoken.number_value != identifier[ctoken->identifier_index].array_dim_size [j])
      return comp_error(CERR_CFUNCTION_PARAMETER_ARRAY_MISMATCH, ctoken);
     identifier[value].array_dim_size [identifier[value].array_dims] = ctoken->number_value;

     if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_SQUARE_CLOSE))
      return comp_error(CERR_CFUNCTION_PARAMETER_ARRAY_NO_SQ_CL, ctoken);
     j ++;
    };
    if (i == cstate.cprocess->cfunction[value].arguments - 1)
     break;
    if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_COMMA))
     return comp_error(CERR_CFUNCTION_PARAMETER_NO_COMMA, ctoken);
    if (!read_next(ctoken)) // the start of the loop will expect this ctoken to be int
     return comp_error(CERR_READ_FAIL, ctoken);
   } // end for loop
     if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACKET_CLOSE))
    return comp_error(CERR_CFUNCTION_PARAMETER_NO_CLOSE_BRACKET, ctoken);
   break;



// really this just needs to parse the arguments and check them against the prototyped ones.
//   break;

  case CM_CALL_CFUNCTION: // value will be the index of the cfunction in the cfunction array (can assume this index is valid)
   exit_point = allocate_exit_point(EXPOINT_TYPE_BASIC); // this exit point is the return address
   if (exit_point == -1)
    return comp_error(CERR_EXIT_POINT_FAIL, ctoken);
   add_intercode(IC_EXIT_POINT_ADDRESS_TO_REGISTER, REGISTER_WORKING, exit_point, 0);
// push the return address
   add_intercode(IC_PUSH_REGISTER, REGISTER_WORKING, REGISTER_STACK, 0);
// push the current stack frame base pointer:
   add_intercode(IC_PUSH_REGISTER, REGISTER_STACK_FRAME, REGISTER_STACK, 0);
// set the stack frame base pointer to the current stack position
   add_intercode(IC_COPY_REGISTER2_TO_REGISTER1, REGISTER_STACK_FRAME, REGISTER_STACK, 0);
// now push the parameters
   if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACKET_OPEN))
    return comp_error(CERR_CFUNCTION_CALL_NO_OPENING_BRACKET, ctoken);
   if (cstate.cprocess->cfunction[value].arguments > 0)
   {
    i = 0;
    while (TRUE)
    {
     if (identifier[cstate.cprocess->cfunction[value].arg_identifier [i]].pointer)
     {
// if the cfunction argument expects a pointer to an array of certain dimensions, accept only that:
      struct ctokenstruct array_arg_ctoken;
      if (!read_next(&array_arg_ctoken))
       return comp_error(CERR_READ_FAIL, &array_arg_ctoken);
// confirm the argument being passed is a variable, and that it's an array with the correct number of dimensions of the correct size:
      if (array_arg_ctoken.type != CTOKEN_TYPE_IDENTIFIER_USER_VARIABLE
       || identifier[array_arg_ctoken.identifier_index].array_dims != identifier[cstate.cprocess->cfunction[value].arg_identifier [i]].array_dims)
        return comp_error(CERR_ARRAY_ARG_TYPE_MISMATCH, &array_arg_ctoken);
      for (j = 0; j < identifier[array_arg_ctoken.identifier_index].array_dims; j ++)
      {
       if (identifier[array_arg_ctoken.identifier_index].array_dim_size [j] != identifier[cstate.cprocess->cfunction[value].arg_identifier [i]].array_dim_size [j])
        return comp_error(CERR_ARRAY_ARG_TYPE_MISMATCH, &array_arg_ctoken);
      }
// the type has been confirmed correct.
// the array will be passed by reference; the cfunction parameter will just be a pointer to the first element (with type information, i.e. dimensions, stored in its identifier struct entry)
// now check to see if the parameter being passed is itself a pointer:
      if (identifier[array_arg_ctoken.identifier_index].pointer)
      {
// if it's a pointer too, can just copy the address directly:
       if (identifier[array_arg_ctoken.identifier_index].storage_class == STORAGE_AUTO)
       {
        if (!auto_var_address_to_register(array_arg_ctoken.identifier_index, REGISTER_WORKING, 0))
         return 0;
        add_intercode(IC_DEREFERENCE_REGISTER_TO_REGISTER, REGISTER_WORKING, REGISTER_WORKING, 0);
       }
         else
          add_intercode(IC_IDENTIFIER_TO_REGISTER, REGISTER_WORKING, array_arg_ctoken.identifier_index, 0);
      }
       else
// parameter is not a pointer, so we need to generate a pointer to the array being passed:
       {
        if (identifier[array_arg_ctoken.identifier_index].storage_class == STORAGE_AUTO)
        {
// for auto variables, this is the same as above but without the need for the dereference
         if (!auto_var_address_to_register(array_arg_ctoken.identifier_index, REGISTER_WORKING, 0))
          return 0;
        }
         else
          add_intercode(IC_ID_ADDRESS_PLUS_OFFSET_TO_REGISTER, REGISTER_WORKING, array_arg_ctoken.identifier_index, 0);
       }
// the previous code will have left the address of the array in the working register.
       if (identifier[cstate.cprocess->cfunction[value].arg_identifier [i]].storage_class == STORAGE_AUTO)
        add_intercode(IC_PUSH_REGISTER, REGISTER_WORKING, REGISTER_STACK, 0);
         else // must be STORAGE_STATIC
          add_intercode(IC_COPY_REGISTER_TO_IDENTIFIER_ADDRESS, cstate.cprocess->cfunction[value].arg_identifier [i], REGISTER_WORKING, 0);
     }
      else
      {
// parameter is not an array, so we pass by value:
       if (!parse_expression(REGISTER_WORKING, REGISTER_TEMP, 0, -1, 0, BIND_BASIC)) // this call ensures that the result is in REG_WORKING
        return 0; // comp_error(CERR_CFUNCTION_CALL_FAIL, ctoken);
       if (identifier[cstate.cprocess->cfunction[value].arg_identifier [i]].storage_class == STORAGE_AUTO)
        add_intercode(IC_PUSH_REGISTER, REGISTER_WORKING, REGISTER_STACK, 0);
         else // must be STORAGE_STATIC
          add_intercode(IC_COPY_REGISTER_TO_IDENTIFIER_ADDRESS, cstate.cprocess->cfunction[value].arg_identifier [i], REGISTER_WORKING, 0);
      }
     i ++;
     if (i == cstate.cprocess->cfunction[value].arguments)
      break;
     if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_COMMA))
      return comp_error(CERR_CFUNCTION_CALL_NOT_ENOUGH_PARAMETERS, ctoken);
    }; // end while
   }
   if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACKET_CLOSE))
    return comp_error(CERR_CFUNCTION_CALL_TOO_MANY_PARAMETERS, ctoken);
// prepare the return address, and push it onto the stack:
// jump to the address of the target function:
   add_intercode(IC_JUMP_TO_CFUNCTION_ADDRESS, value, 0, 0);
   add_intercode(IC_EXIT_POINT_TRUE, exit_point, 0, 0);
//   add_intercode(IC_CFUNCTION_ADDRESS_TO_REGISTER, REGISTER_WORKING, value, 0); // this is the address of the function being called
//   add_intercode(IC_JUMP_TO_REGISTER, REGISTER_WORKING, 0, 0);
// return point should be here.
   break; // CM_CALL_CFUNCTION


  case CM_RETURN_STATEMENT: // has just read return (within a function). value not currently used.
//   if (!read_next(ctoken))
//    return comp_error(CERR_READ_FAIL, ctoken);
   if (cstate.cprocess->cfunction[cstate.scope].return_type == CFUNCTION_RETURN_VOID)
   {
     if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_SEMICOLON))
      return comp_error(CERR_RETURN_FAIL, ctoken);
   }
    else
    {
// must be return int
     if (!parse_expression(REGISTER_WORKING, REGISTER_TEMP, 0, -1, 0, BIND_BASIC)) // this call ensures that the result is in REG_WORKING
      return comp_error(CERR_RETURN_FAIL, ctoken);
// result of statement should be in working register
     if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_SEMICOLON))
      return comp_error(CERR_RETURN_FAIL, ctoken);
    }
    if (!comp_next(CM_RETURN, 0))
     return 0;
    break;

  case CM_RETURN: // this mode adds code to return from a function, without assuming that it is reading a return statement (so it can be used for an implicit return at the end of a function). Doesn't set return value!
// but first we check to see if it's the main function:
   if (cstate.scope == 0)
   {
    add_intercode(IC_STOP, 0, 0, 0);
    break;
   }
// can't use register A from this point as it may hold the return value
// now, set the stack pointer to the returning function's stack frame base
   add_intercode(IC_COPY_REGISTER2_TO_REGISTER1, REGISTER_STACK, REGISTER_STACK_FRAME, 0);
// pop to retrieve stack frame base of calling function
   add_intercode(IC_POP_TO_REGISTER, REGISTER_TEMP, REGISTER_STACK, 0);
// set stack frame base to that value
   add_intercode(IC_COPY_REGISTER2_TO_REGISTER1, REGISTER_STACK_FRAME, REGISTER_TEMP, 0);
// pop to retrieve return address
   add_intercode(IC_POP_TO_REGISTER, REGISTER_TEMP, REGISTER_STACK, 0);
// at this point the stack pointer should be at the top of the calling function's stack frame
// jump to return address
   add_intercode(IC_JUMP_TO_REGISTER, REGISTER_TEMP, 0, 0);
   break;

// value is the statement-level exit point (which will be passed to comp_statement to allow break and continue statements in the conditional code to work)
  case CM_IF:
   exit_point = allocate_exit_point(EXPOINT_TYPE_BASIC);
   if (exit_point == -1)
    return comp_error(CERR_EXIT_POINT_FAIL, ctoken);
   if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACKET_OPEN))
    return comp_error(CERR_SYNTAX_IF, ctoken);
   if (!parse_expression(REGISTER_WORKING, REGISTER_TEMP, 0, exit_point, 0, BIND_BASIC))
    return 0;
   if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACKET_CLOSE))
    return comp_error(CERR_SYNTAX_IF, ctoken);
// so now we check whether the expression is true or false
   add_intercode(IC_IFFALSE_JUMP_TO_EXIT_POINT, REGISTER_WORKING, exit_point, 0);
// here's the "true" exit point for the expression (any exit jumps from || logical operators (that aren't inside sub-expressions) are set to here)
   add_intercode(IC_EXIT_POINT_TRUE, exit_point, 0, 0);
// here's the code that follows from the if statement
   if (!comp_statement(value, 0)) // if the first thing comp_statement finds is {, it will call itself until it finds }
    return 0;
// at this point, assume we've reached the end of the block or statement that followed the if. Check for else:
   if (check_next(CTOKEN_TYPE_IDENTIFIER_KEYWORD, KEYWORD_ELSE))
   {
// first we create a new exit point.
    int avoid_else_exit_point = allocate_exit_point(EXPOINT_TYPE_BASIC);
    if (avoid_else_exit_point == -1)
     return comp_error(CERR_EXIT_POINT_FAIL, ctoken);
// if control reaches the end of the conditional code because the if statement was true and the code has finished executing, we jump past the else code.
    add_intercode(IC_JUMP_EXIT_POINT_TRUE, avoid_else_exit_point, 0, 0); // this is an unconditional jump to the avoid_else exit point
 // this is the point where control will jump to if the if statement was false (as conditional_exit_point remains set from before):
    add_intercode(IC_EXIT_POINT_FALSE, exit_point, 0, 0);
    if (!comp_statement(value, 0)) // if the first thing comp_statement finds is {, it will call itself until it finds }
     return 0;
    add_intercode(IC_EXIT_POINT_TRUE, avoid_else_exit_point, 0, 0); // shouldn't be necessary to set the "false" exit point
// now we return to comp_statement
    break;
   }
// no else, so we just set the "false" exit point for the if statement and go back to comp_statement
   add_intercode(IC_EXIT_POINT_FALSE, exit_point, 0, 0);
   break; // end CM_IF

 case CM_WHILE:
   exit_point = allocate_exit_point(EXPOINT_TYPE_BASIC); // exit point for expression evaluated by while()
   exit_point_loop = allocate_exit_point(EXPOINT_TYPE_LOOP); // exit point for loop as a whole. This is passed to comp_statement for break/continue statements
   if (exit_point == -1
   || exit_point_loop == -1)
    return comp_error(CERR_EXIT_POINT_FAIL, ctoken);
   if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACKET_OPEN))
    return comp_error(CERR_WHILE_MISSING_OPENING_BRACKET, ctoken);
// this exit point is jumped to when the loop continues
   add_intercode(IC_EXIT_POINT_TRUE, exit_point_loop, 0, 0);
   if (!parse_expression(REGISTER_WORKING, REGISTER_TEMP, 0, exit_point, 0, BIND_BASIC))
    return 0;
   if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACKET_CLOSE))
    return comp_error(CERR_WHILE_MISSING_CLOSING_BRACKET, ctoken);
// so now we check whether the expression is true or false
   add_intercode(IC_IFFALSE_JUMP_TO_EXIT_POINT, REGISTER_WORKING, exit_point, 0);
// here's the "true" exit point for the while expression (exit jumps from || logical operators (that aren't inside sub-expressions) are set to here)
   add_intercode(IC_EXIT_POINT_TRUE, exit_point, 0, 0);
// here's the code within the while loop
   if (!comp_statement(exit_point_loop, 0)) // if the first thing comp_statement finds is {, it will call itself until it finds }
    return 0;
// at this point, assume we've reached the end of the block or statement that followed the while.
// go back and evaluate the while expression:
   add_intercode(IC_JUMP_EXIT_POINT_TRUE, exit_point_loop, 0, 0);
// both exit points have false exits right at the end:
   add_intercode(IC_EXIT_POINT_FALSE, exit_point, 0, 0);
   add_intercode(IC_EXIT_POINT_FALSE, exit_point_loop, 0, 0);
   break; // end CM_WHILE
  case CM_DO:
   exit_point = allocate_exit_point(EXPOINT_TYPE_BASIC); // exit point for expression evaluated by while()
   exit_point_loop = allocate_exit_point(EXPOINT_TYPE_LOOP); // exit point for loop as a whole. This is passed to comp_statement for break/continue statements
   if (exit_point == -1
    || exit_point_loop == -1)
    return comp_error(CERR_EXIT_POINT_FAIL, ctoken);
// here's the "true" exit point for the while expression (exit jumps from || logical operators (that aren't inside sub-expressions) are set to here)
   add_intercode(IC_EXIT_POINT_TRUE, exit_point, 0, 0);
// here's the code within the while loop
   if (!comp_statement(exit_point_loop, 0)) // if the first thing comp_statement finds is {, it will call itself until it finds }
    return 0;
   if (!check_next(CTOKEN_TYPE_IDENTIFIER_KEYWORD, KEYWORD_WHILE))
    return comp_error(CERR_DO_WITHOUT_WHILE, ctoken);
   if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACKET_OPEN))
    return comp_error(CERR_WHILE_MISSING_OPENING_BRACKET, ctoken);
// this exit point is jumped to from any continue statement within the loop
   add_intercode(IC_EXIT_POINT_TRUE, exit_point_loop, 0, 0);
   if (!parse_expression(REGISTER_WORKING, REGISTER_TEMP, 0, exit_point, 0, BIND_BASIC))
    return 0;
   if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACKET_CLOSE))
    return comp_error(CERR_WHILE_MISSING_CLOSING_BRACKET, ctoken);
   if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_SEMICOLON))
    return comp_error(CERR_EXPECTED_SEMICOLON, ctoken);
// so now we check whether the expression is true or false
   add_intercode(IC_IFTRUE_JUMP_TO_EXIT_POINT, REGISTER_WORKING, exit_point, 0); // if true, jumps to start of loop. If false, falls through
// both exit points have false exits right at the end:
   add_intercode(IC_EXIT_POINT_FALSE, exit_point, 0, 0);
   add_intercode(IC_EXIT_POINT_FALSE, exit_point_loop, 0, 0);
   break; // end CM_DO

/*

How for loops will work:

for (i = 0; i < 10; i ++) {}
 ... or
for (statement 1; expression; statement 2) statement 3

allocate exit_point
allocate exit_point_loop
allocate exit_point3
execute statement 1 (before start of loop); call with exit point -1
set true exit_point3
evaluate expression
 iffalse goto false exit_point
set true exit_point
execute statement 3
set true exit_point_loop
execute statement 2
goto true exit_point3
set false exit_point
set false exit_point_loop

this means statement 2 and statement 3 will need to be parsed out of order.
do this by setting save_scode_pos before statement 2, then going back and parsing it later
will need to make sure comp_statement can deal with a statement that ends in )


*/
  case CM_FOR:
   exit_point = allocate_exit_point(EXPOINT_TYPE_BASIC); // exit point for expression evaluated by for
   exit_point_loop = allocate_exit_point(EXPOINT_TYPE_LOOP); // exit point for loop as a whole. This is passed to comp_statement for break/continue statements
   int exit_point3 = allocate_exit_point(EXPOINT_TYPE_BASIC); // exit point for returning to the start of the loop
   if (exit_point == -1
    || exit_point_loop == -1
    || exit_point3 == -1)
    return comp_error(CERR_EXIT_POINT_FAIL, ctoken);
   if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACKET_OPEN))
    return comp_error(CERR_FOR_MISSING_OPENING_BRACKET, ctoken);
// parse the first statement (i = 0)
   if (!comp_statement(-1, 0)) // will read in the semicolon
    return 0;
// this is the point where the loop jumps to after finishing. It's not used for continue statements because continue statements need to execute the i++ statement
   add_intercode(IC_EXIT_POINT_TRUE, exit_point3, 0, 0);
// now evaluate the expression (i < 10) and read in the semicolon
   if (!parse_expression(REGISTER_WORKING, REGISTER_TEMP, 0, exit_point, 0, BIND_BASIC))
    return 0;
   if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_SEMICOLON))
    return comp_error(CERR_EXPECTED_SEMICOLON, ctoken);
// now we check whether the expression is true or false
   add_intercode(IC_IFFALSE_JUMP_TO_EXIT_POINT, REGISTER_WORKING, exit_point, 0);
// set the true exit point for exit_point (the exit point used in the expression)
   add_intercode(IC_EXIT_POINT_TRUE, exit_point, 0, 0);
// now save scode position so we can come back later and parse the i++ statement:
   save_scode_pos = cstate.scode_pos;
// skip past the statement:
   if (!skip_to_after_closing_bracket())
    return 0;
// parse the statements within the loop (which will probably be a block within braces, which is treated as a single statement):
   if (!comp_statement(exit_point_loop, 0)) // exit_point_loop is the exit point target for continue (true) and break (false) statements
    return 0;
// set true exit_point_loop (target for continue statements)
   add_intercode(IC_EXIT_POINT_TRUE, exit_point_loop, 0, 0);
// now go back and compile the i++ statement, after saving the current scode_pos:
   int save_scode_pos2 = cstate.scode_pos;
   cstate.scode_pos = save_scode_pos;
   if (!comp_statement(exit_point_loop, 1)) // the 1 means closing bracket expected. note that a continue statement here will result in a loop that will run until the instructions are exhausted. So don't do that.
    return 0;
// now add a jump to exit_point3 true, which is just before the expression (i < 10)
   cstate.scode_pos = save_scode_pos2;
   add_intercode(IC_JUMP_EXIT_POINT_TRUE, exit_point3, 0, 0);
// finally, add false exit points for exit_point (target if the expression (i < 10) evaluates false) and exit_point_loop (target for break statements)
   add_intercode(IC_EXIT_POINT_FALSE, exit_point, 0, 0);
   add_intercode(IC_EXIT_POINT_FALSE, exit_point_loop, 0, 0);
   break; // end CM_FOR

  case CM_INCR_OPERATION: // value is identifier_index of variable to be incr/decr. Mustn't be an array.
       if (identifier[value].storage_class == STORAGE_STATIC)
       {
// now copy REGISTER_WORKING to the variable's address
        add_intercode(IC_IDENTIFIER_TO_REGISTER, REGISTER_WORKING, value, 0);
        add_intercode(IC_INCR_REGISTER, REGISTER_WORKING, 0, 0);
        add_intercode(IC_COPY_REGISTER_TO_IDENTIFIER_ADDRESS, value, REGISTER_WORKING, 0);
       }
        else
        {
         if (!auto_var_address_to_register(value, REGISTER_TEMP, 0))
          return 0;
// Copy value of variable to REGISTER_WORKING
         add_intercode(IC_DEREFERENCE_REGISTER_TO_REGISTER, REGISTER_WORKING, REGISTER_TEMP, 0);
// increment REGISTER_WORKING
         add_intercode(IC_INCR_REGISTER, REGISTER_WORKING, 0, 0);
// copy contents of REGISTER_WORKING to the memory address referred to be REGISTER_TEMP
         add_intercode(IC_COPY_REGISTER_TO_DEREFERENCED_REGISTER, REGISTER_TEMP, REGISTER_WORKING, 0);
        }
      break; // end CM_INCR_DECR_OPERATION

  case CM_DECR_OPERATION: // value is identifier_index of variable to be incr/decr. Mustn't be an array.
       if (identifier[value].storage_class == STORAGE_STATIC)
       {
// now copy REGISTER_WORKING to the variable's address
        add_intercode(IC_IDENTIFIER_TO_REGISTER, REGISTER_WORKING, value, 0);
        add_intercode(IC_DECR_REGISTER, REGISTER_WORKING, 0, 0);
        add_intercode(IC_COPY_REGISTER_TO_IDENTIFIER_ADDRESS, value, REGISTER_WORKING, 0);
       }
        else
        {
         if (!auto_var_address_to_register(value, REGISTER_TEMP, 0))
          return 0;
// Copy value of variable to REGISTER_WORKING
         add_intercode(IC_DEREFERENCE_REGISTER_TO_REGISTER, REGISTER_WORKING, REGISTER_TEMP, 0);
// increment REGISTER_WORKING
         add_intercode(IC_DECR_REGISTER, REGISTER_WORKING, 0, 0);
// copy contents of REGISTER_WORKING to the memory address referred to be REGISTER_TEMP
         add_intercode(IC_COPY_REGISTER_TO_DEREFERENCED_REGISTER, REGISTER_TEMP, REGISTER_WORKING, 0);
        }
      break; // end CM_INCR_DECR_OPERATION

 case CM_PRINT:
  if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACKET_OPEN))
   return comp_error(CERR_EXPECTED_BRACKET_OPEN, ctoken);
  do
  {
   if (!comp_next(CM_PRINT_CONTENTS, 0))
    return 0;
  } while (check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_COMMA));
  if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACKET_CLOSE))
   return comp_error(CERR_NO_CLOSE_BRACKET, ctoken);
  if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_SEMICOLON))
   return comp_error(CERR_SYNTAX, ctoken);
  break;


 case CM_PRINT_CONTENTS:
//  if (check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACKET_CLOSE))
//   break;
  if (check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_QUOTES))
  {
// string
   add_intercode(IC_PRINT_STRING, 0, 0, 0);
   int string_length = 0;
   while(TRUE)
   {
    retval = c_get_next_char_from_scode();
    if (retval == REACHED_END_OF_SCODE)
     return comp_error(CERR_READ_FAIL, ctoken);
    if (retval == '"')
     break;
    if (retval == '\\')
    {
     retval = c_get_next_char_from_scode();
     if (retval == REACHED_END_OF_SCODE)
      return comp_error(CERR_READ_FAIL, ctoken);
     switch(retval)
     {
      case 'n': retval = '\n'; break;
      case '"': retval = '"'; break;
      case '\\': retval = '\\'; break;
     }
    }
    add_intercode(IC_STRING, retval, 0, 0);
    string_length ++;
    if (string_length == 80)
     return comp_error(CERR_STRING_TOO_LONG, ctoken);
   };
   add_intercode(IC_STRING_END, 0, 0, 0);
   break;
  } // end if string
// may be printing a string:
  if (check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_DOLLAR))
  {
   retval = parse_expression(REGISTER_WORKING, REGISTER_TEMP, 0, -1, 0, BIND_BASIC);
   if (retval == EXPR_ERROR)
    return 0;
   add_intercode(IC_PRINT_REGISTER_A, REGISTER_WORKING, 0, 0);
   break;
  }
// must be printing a number:
  retval = parse_expression(REGISTER_WORKING, REGISTER_TEMP, 0, -1, 0, BIND_BASIC);
  if (retval == EXPR_ERROR)
   return 0;
  add_intercode(IC_PRINT_REGISTER, REGISTER_WORKING, 0, 0);
  break;

  case CM_END_ASSIGNMENT_STATEMENT: // value is 1 if a bracket is expected, 0 if a semicolon is expected
       if (value)
       {
        if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACKET_CLOSE))
         return comp_error(CERR_EXPECTED_CLOSING_BRACKET_IN_FOR, ctoken); // bracket-expected is really only for the final statement in a for statement
       }
        else
         if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_SEMICOLON))
          return comp_error(CERR_EXPECTED_SEMICOLON, ctoken);
       break;

  case CM_GOTO:
// goto should be followed by a label
   if (!read_next(ctoken))
    return comp_error(CERR_READ_FAIL, ctoken);
   switch(ctoken->type)
   {
    case CTOKEN_TYPE_LABEL: // this means that the label has already been defined
    case CTOKEN_TYPE_LABEL_UNDEFINED: // this means that the label hasn't yet been defined, but has been used in at least one previous goto
     add_intercode(IC_GOTO, ctoken->identifier_index, 0, 0);
     break;
    case CTOKEN_TYPE_IDENTIFIER_NEW: // label hasn't been used or defined previously
     identifier[ctoken->identifier_index].type = ID_USER_LABEL_UNDEFINED;
     add_intercode(IC_GOTO, ctoken->identifier_index, 0, 0);
     break;
    default:
     return comp_error(CERR_EXPECTED_LABEL, ctoken);
   }
   if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_SEMICOLON))
    return comp_error(CERR_EXPECTED_SEMICOLON, ctoken);
   break;


 } // end huge cmode switch

 return 1; // continue

}



/*
call this after reading the { to indicate the start of the dimension
reads in the } at the end

dim is the dimension that we're initialising
dim_offset is the start of this particular element of this dimension as an offset from the first element of the whole array
*/
int initialise_array_dimension(int id_index, int dim, int dim_offset)
{

 struct ctokenstruct ctoken;
 strcpy(ctoken.name, "(empty)");

 if (dim >= identifier[id_index].array_dims)
  return comp_error(CERR_TOO_MANY_ARRAY_DIMENSIONS, &ctoken);

 int i = 0;
 int found_comma;

// now check whether we're still at a dimension other than the final one (where dim will be array_dims-1)
// if we are, we need to call this function for each each subdimension (and again for each sub-subdimension)
 if (dim < identifier[id_index].array_dims - 1)
 {
  if (check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACE_CLOSE))
   return 1;
  while (TRUE)
  {
// if dimension doesn't finish here, expect an opening brace for the next subdimension
   if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACE_OPEN))
    return comp_error(CERR_SYNTAX, &ctoken);
   if (i >= identifier[id_index].array_dim_size [dim])
    return comp_error(CERR_ARRAY_DIM_OUT_OF_BOUNDS, &ctoken);
   if (!initialise_array_dimension(id_index, dim + 1, dim_offset + (i*identifier[id_index].array_element_size [dim])))
    return 0;
   i ++;
// this found_comma stuff lets the compiler ignore a comma after an initialiser but directly before a closing brace
   found_comma = 0;
   if (check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_COMMA))
    found_comma = 1;

   if (check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACE_CLOSE))
    return 1; // finished this dimension
// now expect a comma
   if (!found_comma)
    return comp_error(CERR_SYNTAX, &ctoken);
  }; // end while loop
 }

/*
 - if at the last dimension:
  - if a }, returns 1

  - then expects a number (must be resolvable to a constant, maybe unless using :=)
  - writes the number to array_init
  - bounds-checks array
  - checks for } or comma - if comma found, loops back to expecting a number
*/
// must be at the last dimension, so start expecting numbers

// if (check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACE_CLOSE))
//  return 1; // finished this dimension


// check for a string initiliaser
// This code does not presently produce a string when compiling to asm - it just produces numbers. TO DO: fix this?
  if (check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_QUOTES))
  {
//   add_intercode(IC_STRING_INIT, 0, 0, 0);
   int string_length = 0;
   int retval = 0;
   while(TRUE)
   {
    retval = c_get_next_char_from_scode();
    if (retval == REACHED_END_OF_SCODE)
     return comp_error(CERR_READ_FAIL, &ctoken);
    if (retval == '"')
     break;
    if (retval == '\\')
    {
     retval = c_get_next_char_from_scode();
     if (retval == REACHED_END_OF_SCODE)
      return comp_error(CERR_READ_FAIL, &ctoken);
     switch(retval)
     {
      case 'n': retval = '\n'; break;
      case '"': retval = '"'; break;
      case '\\': retval = '\\'; break;
     }
    }

    array_init.value [dim_offset + string_length] = retval;

//    add_intercode(IC_STRING, retval, 0, 0);
    string_length ++;
    if (string_length >= identifier[id_index].array_dim_size [dim])
     return comp_error(CERR_STRING_TOO_LONG, NULL);
   };
//   add_intercode(IC_STRING_END, 0, 0, 0);

// dimension should finish here: (TO DO: should be able to have multiple strings in a single dimension
   if (check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACE_CLOSE))
    return 1; // finished

   return comp_error(CERR_EXPECTED_BRACE_CLOSE, NULL);
  } // end if string


 i = 0;

 while(TRUE)
 {
// at this point should be reading a number, so we use read_next_expression_value
  if (!read_next_expression_value(&ctoken))
   return comp_error(CERR_READ_FAIL, &ctoken);
  if (ctoken.type != CTOKEN_TYPE_NUMBER)
   return comp_error(CERR_ARRAY_INITIALISER_NOT_CONSTANT, &ctoken);
// bounds-check it
  if (i >= identifier[id_index].array_dim_size [dim])
   return comp_error(CERR_ARRAY_DIM_OUT_OF_BOUNDS, &ctoken);
// put the value in the right place in array_init
  array_init.value [dim_offset + i] = ctoken.number_value;

  found_comma = 0;
  if (check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_COMMA))
   found_comma = 1;

// if the next thing is }, we've finished this dimension
  if (check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACE_CLOSE))
   return 1; // finished
// increment element index
  i ++;
// not closed by a brace, so expect a comma then go around again
  if (!found_comma)
   return comp_error(CERR_SYNTAX, &ctoken);
 };

 return 1;

}



/*
parse_expression functions:

parse_expression is the basic one.
It can be called recursively
It can call comp_next
It returns 1 on success, 0 on failure (and will have called comp_error if it fails)
It needs to know:
 - which register to put the result in (maybe will always be working register? Not sure)
 - whether the working register holds anything useful that needs to be stored on the stack if the working register is used for calculations
 - the exit point index. If -1, parse_expression sets its own exit point (at the end of the expression)

The first thing it does is try to optimise the expression:
 - it checks whether the expression can be resolved to a constant. If so, easy!
 - it checks whether the expression can be resolved to a single memory address. Almost as easy!
 - no luck so far. If working_register_free is 0, push the working register


*/

// maybe need an extra parameter: test_truth - jump to true or false exit point depending on truth value of expression.
//  may be needed because currently CM_IF assumes the expression will contain jumps - not currently true if the expression doesn't contain logical operators
//   - actually not sure that's true - CM_IF tests the truth value of the working register. So should be okay.


/*
returns:
EXPR_ERROR on failure (error)
EXPR_SIMPLE_COMPLETE if a simple expression was found. In this case, the result will be in simple_expression_target register.
EXPR_COMPLEX_COMPLETE if an expression requiring calculation was found. In this case, the result will always be in the working register.
 - also EXPR_COMPLEX_COMPLETE: if save_working_register == 1, the contents of REGISTER_WORKING will have been put on the stack and need to be popped to the temp register to be used.

// can't return EXPR_..._INCOMPLETE - will either parse a complete expression or produce an error

simple_expression_target is used to tell this function which register to leave a result in if it can be worked out using a single register.
 - this is useful because sometimes an expression should always leave its result in the working register (e.g. function return value) while sometimes
   leaving a simple result in the temp register without touching the working register can be useful if possible, but cannot be predicted (e.g. subexpressions)

*/



/*
This function parses an expression and arranges for the result to be left in first_register.
If possible, it will just use first_register for any calculations.
 - if it needs to use second_register, and if save_second_register==1, will push second register and pop it again afterwards.
exit_point should be -1 unless it is being called in a context where exit points have already been set, such as an if statement or a while loop.
accept_assignment_at_end indicates that an assignment operator is a valid end to the expression. I think this is only relevant where an assignment is being made through indirection
 (i.e. a statement like *a = 1;)
*/
int parse_expression(int first_register, int second_register, int save_second_register, int exit_point, int accept_assignment_at_end, int binding)
{

 cstate.recursion_level ++;

 if (cstate.recursion_level > RECURSION_LIMIT)
  return comp_error(CERR_RECURSION_LIMIT_REACHED_IN_EXPRESSION, NULL);

 if (cstate.error != CERR_NONE)
  return 0;

 struct ctokenstruct ctoken;
 strcpy(ctoken.name, "(undefined)");
// int save_scode_pos;
 int retval;

// first set an exit point in case the expression includes logical operators (unless the calling function has already set an exit point, e.g. if it's from CM_IF)
 int fix_exit_point = 0; // if 1 this variable indicates that exit points need to be added by this function (because the calling function won't add them)

 if (exit_point == -1)
 {
   exit_point = allocate_exit_point(EXPOINT_TYPE_BASIC);
   if (exit_point == -1)
    return comp_error(CERR_EXIT_POINT_FAIL, NULL);
   fix_exit_point = 1;
 }

// now read the initial value into a register:
 retval = parse_expression_value(first_register, second_register, save_second_register, exit_point, binding);

 if (retval == 0)
  return 0;

// now loop through any further operators and values in the expression:
  while(TRUE)
  {
   if (!peek_next(&ctoken))
    comp_error(CERR_READ_FAIL, &ctoken);
   if (ctoken.type == CTOKEN_TYPE_PUNCTUATION
    && (ctoken.subtype == CTOKEN_SUBTYPE_BRACKET_CLOSE
     || ctoken.subtype == CTOKEN_SUBTYPE_SEMICOLON
     || ctoken.subtype == CTOKEN_SUBTYPE_SQUARE_CLOSE
     || ctoken.subtype == CTOKEN_SUBTYPE_COMMA))
     break; // finished
   if (ctoken.type == CTOKEN_TYPE_OPERATOR_ASSIGN
    && accept_assignment_at_end)
    break; // may be finished
// parse_expression_operator reads the following operator then reads the next value into second_register (passed as the first parameter)
// if it needs to use first_register for anything, it needs to save it first then pop it afterwards
   retval = parse_expression_operator(first_register, second_register, save_second_register, exit_point, binding);
   if (!retval)
    return 0; // error
   if (retval == 2)
    break; // this means a logical operator has been found, but there are unresolved comparisons still to be performed.
// otherwise retval must be 1 so we go through the loop again

//   if (!parse_expression_operator(first_register, second_register, save_second_register, exit_point, binding))
//    return 0; // error
  };

// if an exit point was allocated above, set its true and false points at the end of the expression:
 if (fix_exit_point)
 {
   add_intercode(IC_EXIT_POINT_TRUE, exit_point, 0, 0);
   add_intercode(IC_EXIT_POINT_FALSE, exit_point, 0, 0);
 }


 cstate.recursion_level --;
 return 1; // success!

} // end parse_expression


/*
returns:
same as parse_expression


binding is unused because operator precedence hasn't been implemented properly. I'm planning to fix this sometime, so I've left it here.
*/
int parse_expression_value(int target_register, int secondary_register, int save_secondary_register, int exit_point, int binding)
{

 if (cstate.error != CERR_NONE)
  return 0;

 struct ctokenstruct ctoken;
 strcpy(ctoken.name, "(undefined)");
 int dereference = 0;
 int retval;
 int cfunction_index;
 int apply_not = 0, apply_bitwise_not = 0;
 int fix_exit_point = 0;

 if (check_next(CTOKEN_TYPE_OPERATOR_ARITHMETIC, CTOKEN_SUBTYPE_BITWISE_NOT)) // ~
 {
  apply_bitwise_not = 1;
 }

 if (check_next(CTOKEN_TYPE_OPERATOR_ARITHMETIC, CTOKEN_SUBTYPE_NOT)) // !
 {
  apply_not = 1;
 }

// now check for ~ again in case it came after the !
 if (check_next(CTOKEN_TYPE_OPERATOR_ARITHMETIC, CTOKEN_SUBTYPE_BITWISE_NOT)) // ~
 {
  apply_bitwise_not = 1;
 }

// check for dereference prefixes - *
 while (check_next(CTOKEN_TYPE_OPERATOR_ARITHMETIC, CTOKEN_SUBTYPE_MUL))
 {
  dereference ++;
 };

// check to see if the entire expression can be resolved without further calculations:
 if (attempt_expression_resolve(target_register))
 {
  dereference_loop(target_register, &dereference);
  if (apply_not)
   add_intercode(IC_NOT_REG_REG, target_register, target_register, 0);
  if (apply_bitwise_not)
   add_intercode(IC_BITWISE_NOT_REG_REG, target_register, target_register, 0);
  return 1;
 }

// Test for an error here (because a failure by attempt_expression_resolve() isn't necessarily an error)
 if (cstate.error != CERR_NONE)
  return 0;

 if (!read_next_expression_value(&ctoken))
  comp_error(CERR_READ_FAIL, &ctoken);

   switch(ctoken.type)
   {

    case CTOKEN_TYPE_PUNCTUATION: // only ( is accepted
     if (ctoken.subtype != CTOKEN_SUBTYPE_BRACKET_OPEN) // this is the only punctuation accepted at this point.
      return comp_error(CERR_SYNTAX_PUNCTUATION_IN_EXPRESSION, &ctoken);

     if (attempt_expression_resolve(target_register))
     {
      dereference_loop(target_register, &dereference);
      if (apply_not)
       add_intercode(IC_NOT_REG_REG, target_register, target_register, 0);
      if (apply_bitwise_not)
       add_intercode(IC_BITWISE_NOT_REG_REG, target_register, target_register, 0);
      return 1;
     }

// Test for an error here (because a failure by attempt_expression_resolve() isn't necessarily an error)
 if (cstate.error != CERR_NONE)
  return 0;

// since the bracketted subexpression can't be completely resolved without further calculations, we may need to push the working register:
//     if (!first_value)
//      add_intercode(IC_PUSH_REGISTER, REGISTER_WORKING, REGISTER_STACK, 0);


     if (exit_point == -1)
     {
      exit_point = allocate_exit_point(EXPOINT_TYPE_BASIC);
      if (exit_point == -1)
       return comp_error(CERR_EXIT_POINT_FAIL, &ctoken);
      fix_exit_point = 1;
     }

     retval = parse_expression(target_register, secondary_register, save_secondary_register, exit_point, 0, BIND_BASIC);  // -1 for the exit point parameter tells parse_expression to set its own exit points

     if (retval == 0)
      return 0;

     if (fix_exit_point)
     {
      add_intercode(IC_EXIT_POINT_TRUE, exit_point, 0, 0);
      add_intercode(IC_EXIT_POINT_FALSE, exit_point, 0, 0);
     }

     if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACKET_CLOSE))
      return comp_error(CERR_NO_CLOSE_BRACKET, &ctoken);

     dereference_loop(REGISTER_WORKING, &dereference);
     if (apply_not)
      add_intercode(IC_NOT_REG_REG, target_register, target_register, 0);

     return 1; // end case CTOKEN_TYPE_PUNCTUATION

    case CTOKEN_TYPE_NUMBER:
     add_intercode(IC_NUMBER_TO_REGISTER, target_register, ctoken.number_value, 0);
     dereference_loop(target_register, &dereference);
     if (apply_not)
      add_intercode(IC_NOT_REG_REG, target_register, target_register, 0);
     if (apply_bitwise_not)
      add_intercode(IC_BITWISE_NOT_REG_REG, target_register, target_register, 0);
     return 1;

    case CTOKEN_TYPE_IDENTIFIER_USER_VARIABLE:
     if (identifier[ctoken.identifier_index].array_dims > 0)
     {
      retval = parse_array_reference(target_register, secondary_register, save_secondary_register, ctoken.identifier_index, 0); // the ,0 means that we want the value of the element, not the address
      dereference_loop(target_register, &dereference);
      if (apply_not)
       add_intercode(IC_NOT_REG_REG, target_register, target_register, 0);
      if (apply_bitwise_not)
       add_intercode(IC_BITWISE_NOT_REG_REG, target_register, target_register, 0);
      return 1;
     } // end code for arrays

     if (identifier[ctoken.identifier_index].storage_class == STORAGE_STATIC)
     {
      add_intercode(IC_IDENTIFIER_TO_REGISTER, target_register, ctoken.identifier_index, 0);
      dereference_loop(target_register, &dereference);
      if (apply_not)
       add_intercode(IC_NOT_REG_REG, target_register, target_register, 0);
      if (apply_bitwise_not)
       add_intercode(IC_BITWISE_NOT_REG_REG, target_register, target_register, 0);
      return 1;
     }
// must be an automatic variable...
// get the address of the variable (note that auto_var_address_to_register uses only one register)
     if (!auto_var_address_to_register(ctoken.identifier_index, target_register, 0))
      return 0;
// Now we just need to copy the contents of that address to REGISTER_WORKING
     add_intercode(IC_DEREFERENCE_REGISTER_TO_REGISTER, target_register, target_register, 0);
     dereference_loop(target_register, &dereference);
     if (apply_not)
      add_intercode(IC_NOT_REG_REG, target_register, target_register, 0);
     if (apply_bitwise_not)
      add_intercode(IC_BITWISE_NOT_REG_REG, target_register, target_register, 0);
     return 1;


    case CTOKEN_TYPE_IDENTIFIER_USER_CFUNCTION:
     cfunction_index = identifier[ctoken.identifier_index].cfunction_index;
     if (cstate.cprocess->cfunction[cfunction_index].return_type == CFUNCTION_RETURN_VOID)
      return comp_error(CERR_VOID_RETURN_USED, &ctoken);
// may need to push the secondary register before a cfunction call:
     if (save_secondary_register)
      add_intercode(IC_PUSH_REGISTER, secondary_register, REGISTER_STACK, 0);
     if (!comp_next(CM_CALL_CFUNCTION, cfunction_index))
      return 0;
     // the function will leave its return value in REGISTER_WORKING
     dereference_loop(REGISTER_WORKING, &dereference); // not sure it's really necessary to allow this, but I guess it can't hurt (right?)
     if (apply_not)
      add_intercode(IC_NOT_REG_REG, REGISTER_WORKING, REGISTER_WORKING, 0);
     if (apply_bitwise_not)
      add_intercode(IC_BITWISE_NOT_REG_REG, target_register, target_register, 0);
// function calls always leave the result in REGISTER_WORKING. May need to copy it to REGISTER_TEMP:
     if (target_register == REGISTER_TEMP)
      add_intercode(IC_COPY_REGISTER2_TO_REGISTER1, REGISTER_TEMP, REGISTER_WORKING, 0);
// may need to recover the secondary register
     if (save_secondary_register)
      add_intercode(IC_POP_TO_REGISTER, secondary_register, REGISTER_STACK, 0);
     return 1; // end CTOKEN_TYPE_IDENTIFIER_USER_CFUNCTION

    case CTOKEN_TYPE_IDENTIFIER_BUILTIN_CFUNCTION:
     switch(ctoken.identifier_index)
     {
      case BUILTIN_CFUNCTION_PUT:
      case BUILTIN_CFUNCTION_PUT_INDEX:
//      case BUILTIN_CFUNCTION_NEW_PROCESS: // should this return the status? TO DO: yes it should!
       return comp_error(CERR_VOID_RETURN_USED, &ctoken);
      case BUILTIN_CFUNCTION_GET:
       return parse_get(target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_GET_INDEX:
       return parse_get_index(target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_PROCESS_START:
       return parse_process_start_or_end(target_register, 0);
      case BUILTIN_CFUNCTION_PROCESS_END:
       return parse_process_start_or_end(target_register, 1);
      case BUILTIN_CFUNCTION_CALL:
       return parse_call(target_register, secondary_register, save_secondary_register);
// info functions
      case BUILTIN_CFUNCTION_GET_X:
       return simple_built_in_cfunction_call(MTYPE_PR_STD, MSTATUS_PR_STD_GET_X, 0, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_GET_Y:
       return simple_built_in_cfunction_call(MTYPE_PR_STD, MSTATUS_PR_STD_GET_Y, 0, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_GET_ANGLE:
       return simple_built_in_cfunction_call(MTYPE_PR_STD, MSTATUS_PR_STD_GET_ANGLE, 0, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_GET_SPEED_X:
       return simple_built_in_cfunction_call(MTYPE_PR_STD, MSTATUS_PR_STD_GET_SPEED_X, 0, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_GET_SPEED_Y:
       return simple_built_in_cfunction_call(MTYPE_PR_STD, MSTATUS_PR_STD_GET_SPEED_Y, 0, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_GET_TEAM:
       return simple_built_in_cfunction_call(MTYPE_PR_STD, MSTATUS_PR_STD_GET_TEAM, 0, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_GET_HP:
       return simple_built_in_cfunction_call(MTYPE_PR_STD, MSTATUS_PR_STD_GET_HP, 0, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_GET_HP_MAX:
       return simple_built_in_cfunction_call(MTYPE_PR_STD, MSTATUS_PR_STD_GET_HP_MAX, 0, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_GET_INSTR:
       return simple_built_in_cfunction_call(MTYPE_PR_STD, MSTATUS_PR_STD_GET_INSTR, 0, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_GET_INSTR_MAX:
       return simple_built_in_cfunction_call(MTYPE_PR_STD, MSTATUS_PR_STD_GET_INSTR_MAX, 0, target_register, secondary_register, save_secondary_register);

      case BUILTIN_CFUNCTION_GET_IRPT:
       return simple_built_in_cfunction_call(MTYPE_PR_STD, MSTATUS_PR_STD_GET_IRPT, 0, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_GET_IRPT_MAX:
       return simple_built_in_cfunction_call(MTYPE_PR_STD, MSTATUS_PR_STD_GET_IRPT_MAX, 0, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_GET_OWN_IRPT_MAX:
       return simple_built_in_cfunction_call(MTYPE_PR_STD, MSTATUS_PR_STD_GET_OWN_IRPT_MAX, 0, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_GET_DATA:
       return simple_built_in_cfunction_call(MTYPE_PR_STD, MSTATUS_PR_STD_GET_DATA, 0, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_GET_DATA_MAX:
       return simple_built_in_cfunction_call(MTYPE_PR_STD, MSTATUS_PR_STD_GET_DATA_MAX, 0, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_GET_OWN_DATA_MAX:
       return simple_built_in_cfunction_call(MTYPE_PR_STD, MSTATUS_PR_STD_GET_OWN_DATA_MAX, 0, target_register, secondary_register, save_secondary_register);

      case BUILTIN_CFUNCTION_GET_SPIN:
       return simple_built_in_cfunction_call(MTYPE_PR_STD, MSTATUS_PR_STD_GET_SPIN, 0, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_GET_GR_X:
       return simple_built_in_cfunction_call(MTYPE_PR_STD, MSTATUS_PR_STD_GET_GR_X, 0, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_GET_GR_Y:
       return simple_built_in_cfunction_call(MTYPE_PR_STD, MSTATUS_PR_STD_GET_GR_Y, 0, target_register, secondary_register, save_secondary_register);
//      case BUILTIN_CFUNCTION_GET_GR_ANGLE:
//       return simple_built_in_cfunction_call(MTYPE_PR_INFO, MSTATUS_PR_STD_GET_GR_ANGLE, 0, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_GET_GR_SPEED_X:
       return simple_built_in_cfunction_call(MTYPE_PR_STD, MSTATUS_PR_STD_GET_GR_SPEED_X, 0, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_GET_GR_SPEED_Y:
       return simple_built_in_cfunction_call(MTYPE_PR_STD, MSTATUS_PR_STD_GET_GR_SPEED_Y, 0, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_GET_GR_MEMBERS:
       return simple_built_in_cfunction_call(MTYPE_PR_STD, MSTATUS_PR_STD_GET_GR_MEMBERS, 0, target_register, secondary_register, save_secondary_register);

      case BUILTIN_CFUNCTION_GET_WORLD_X:
       return simple_built_in_cfunction_call(MTYPE_PR_STD, MSTATUS_PR_STD_GET_WORLD_W, 0, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_GET_WORLD_Y:
       return simple_built_in_cfunction_call(MTYPE_PR_STD, MSTATUS_PR_STD_GET_WORLD_H, 0, target_register, secondary_register, save_secondary_register);
//      case BUILTIN_CFUNCTION_GET_GEN_LIMIT:
//       return simple_built_in_cfunction_call(MTYPE_PR_INFO, MSTATUS_PR_STD_GET_GEN_LIMIT, 0, target_register, secondary_register, save_secondary_register);
//      case BUILTIN_CFUNCTION_GET_GEN_NUMBER:
//       return simple_built_in_cfunction_call(MTYPE_PR_INFO, MSTATUS_PR_STD_GET_GEN_NUMBER, 0, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_GET_EX_TIME:
       return simple_built_in_cfunction_call(MTYPE_PR_STD, MSTATUS_PR_STD_GET_EX_COUNT, 0, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_GET_EFFICIENCY:
       return simple_built_in_cfunction_call(MTYPE_PR_STD, MSTATUS_PR_STD_GET_EFFICIENCY, 0, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_GET_TIME:
       return simple_built_in_cfunction_call(MTYPE_PR_STD, MSTATUS_PR_STD_GET_TIME, 0, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_GET_VERTICES:
       return simple_built_in_cfunction_call(MTYPE_PR_STD, MSTATUS_PR_STD_GET_VERTICES, 0, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_GET_VERTEX_ANGLE:
       return simple_built_in_cfunction_call(MTYPE_PR_STD, MSTATUS_PR_STD_GET_VERTEX_ANGLE, 1, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_GET_VERTEX_DIST:
       return simple_built_in_cfunction_call(MTYPE_PR_STD, MSTATUS_PR_STD_GET_VERTEX_DIST, 1, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_GET_VERTEX_ANGLE_NEXT:
       return simple_built_in_cfunction_call(MTYPE_PR_STD, MSTATUS_PR_STD_GET_VERTEX_ANGLE_NEXT, 1, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_GET_VERTEX_ANGLE_PREV:
       return simple_built_in_cfunction_call(MTYPE_PR_STD, MSTATUS_PR_STD_GET_VERTEX_ANGLE_PREV, 1, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_GET_VERTEX_ANGLE_MIN:
       return simple_built_in_cfunction_call(MTYPE_PR_STD, MSTATUS_PR_STD_GET_VERTEX_ANGLE_MIN, 1, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_GET_VERTEX_ANGLE_MAX:
       return simple_built_in_cfunction_call(MTYPE_PR_STD, MSTATUS_PR_STD_GET_VERTEX_ANGLE_MAX, 1, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_GET_METHOD:
       return simple_built_in_cfunction_call(MTYPE_PR_STD, MSTATUS_PR_STD_GET_METHOD, 1, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_GET_METHOD_FIND:
       return simple_built_in_cfunction_call(MTYPE_PR_STD, MSTATUS_PR_STD_GET_METHOD_FIND, 2, target_register, secondary_register, save_secondary_register);


// world functions (use MTYPE_CLOB_WORLD):
      case BUILTIN_CFUNCTION_WORLD_WORLD_X:
       return simple_built_in_cfunction_call(MTYPE_CLOB_STD, MSTATUS_CLOB_STD_WORLD_SIZE_X, 0, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_WORLD_WORLD_Y:
       return simple_built_in_cfunction_call(MTYPE_CLOB_STD, MSTATUS_CLOB_STD_WORLD_SIZE_Y, 0, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_WORLD_PROCS:
       return simple_built_in_cfunction_call(MTYPE_CLOB_STD, MSTATUS_CLOB_STD_WORLD_PROCS, 0, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_WORLD_TEAM:
       return simple_built_in_cfunction_call(MTYPE_CLOB_STD, MSTATUS_CLOB_STD_WORLD_TEAM, 0, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_WORLD_TEAMS:
       return simple_built_in_cfunction_call(MTYPE_CLOB_STD, MSTATUS_CLOB_STD_WORLD_TEAMS, 0, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_WORLD_TEAM_SIZE:
       return simple_built_in_cfunction_call(MTYPE_CLOB_STD, MSTATUS_CLOB_STD_WORLD_TEAM_SIZE, 0, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_WORLD_PROCS_EACH:
       return simple_built_in_cfunction_call(MTYPE_CLOB_STD, MSTATUS_CLOB_STD_WORLD_MAX_PROCS_EACH, 0, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_WORLD_FIRST_PROC:
       return simple_built_in_cfunction_call(MTYPE_CLOB_STD, MSTATUS_CLOB_STD_WORLD_FIRST_PROC, 0, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_WORLD_LAST_PROC:
       return simple_built_in_cfunction_call(MTYPE_CLOB_STD, MSTATUS_CLOB_STD_WORLD_LAST_PROC, 0, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_WORLD_TIME:
       return simple_built_in_cfunction_call(MTYPE_CLOB_STD, MSTATUS_CLOB_STD_WORLD_TIME, 0, target_register, secondary_register, save_secondary_register);
//      case BUILTIN_CFUNCTION_WORLD_GEN_LIMIT:
//       return simple_built_in_cfunction_call(MTYPE_CLOB_STD, MSTATUS_CLOB_WORLD_GEN_LIMIT, 0, target_register, secondary_register, save_secondary_register);
//      case BUILTIN_CFUNCTION_WORLD_GEN_NUMBER:
//       return simple_built_in_cfunction_call(MTYPE_CLOB_STD, MSTATUS_CLOB_WORLD_GEN_NUMBER, 0, target_register, secondary_register, save_secondary_register);

// proc command functions (use MTYPE_PR_COMMAND):
      case BUILTIN_CFUNCTION_GET_COMMAND: // arg is command index
       return simple_built_in_cfunction_call(MTYPE_PR_COMMAND, MSTATUS_PR_COMMAND_VALUE, 1, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_GET_COMMAND_BIT: // arg is command index
       return simple_built_in_cfunction_call(MTYPE_PR_COMMAND, MSTATUS_PR_COMMAND_BIT, 2, target_register, secondary_register, save_secondary_register);
// client command functions (use MTYPE_CLOB_COMMAND)
      case BUILTIN_CFUNCTION_CHECK_COMMAND:  // args are proc index and command index
       return simple_built_in_cfunction_call(MTYPE_CLOB_COMMAND_REC, MSTATUS_CLOB_COMMAND_VALUE, 2, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_CHECK_COMMAND_BIT:  // args are proc index and command index
       return simple_built_in_cfunction_call(MTYPE_CLOB_COMMAND_REC, MSTATUS_CLOB_COMMAND_BIT, 3, target_register, secondary_register, save_secondary_register);

// The functions for setting commands are elsewhere (as they don't return anything useful)

// query functions
      case BUILTIN_CFUNCTION_QUERY_X:
       return simple_built_in_cfunction_call(MTYPE_CLOB_QUERY, MSTATUS_PR_STD_GET_X, 1, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_QUERY_Y:
       return simple_built_in_cfunction_call(MTYPE_CLOB_QUERY, MSTATUS_PR_STD_GET_Y, 1, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_QUERY_ANGLE:
       return simple_built_in_cfunction_call(MTYPE_CLOB_QUERY, MSTATUS_PR_STD_GET_ANGLE, 1, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_QUERY_SPEED_X:
       return simple_built_in_cfunction_call(MTYPE_CLOB_QUERY, MSTATUS_PR_STD_GET_SPEED_X, 1, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_QUERY_SPEED_Y:
       return simple_built_in_cfunction_call(MTYPE_CLOB_QUERY, MSTATUS_PR_STD_GET_SPEED_Y, 1, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_QUERY_TEAM:
       return simple_built_in_cfunction_call(MTYPE_CLOB_QUERY, MSTATUS_PR_STD_GET_TEAM, 1, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_QUERY_HP:
       return simple_built_in_cfunction_call(MTYPE_CLOB_QUERY, MSTATUS_PR_STD_GET_HP, 1, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_QUERY_HP_MAX:
       return simple_built_in_cfunction_call(MTYPE_CLOB_QUERY, MSTATUS_PR_STD_GET_HP_MAX, 1, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_QUERY_IRPT:
       return simple_built_in_cfunction_call(MTYPE_CLOB_QUERY, MSTATUS_PR_STD_GET_IRPT, 1, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_QUERY_IRPT_MAX:
       return simple_built_in_cfunction_call(MTYPE_CLOB_QUERY, MSTATUS_PR_STD_GET_IRPT_MAX, 1, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_QUERY_OWN_IRPT_MAX:
       return simple_built_in_cfunction_call(MTYPE_CLOB_QUERY, MSTATUS_PR_STD_GET_OWN_IRPT_MAX, 1, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_QUERY_DATA:
       return simple_built_in_cfunction_call(MTYPE_CLOB_QUERY, MSTATUS_PR_STD_GET_DATA, 1, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_QUERY_DATA_MAX:
       return simple_built_in_cfunction_call(MTYPE_CLOB_QUERY, MSTATUS_PR_STD_GET_DATA_MAX, 1, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_QUERY_OWN_DATA_MAX:
       return simple_built_in_cfunction_call(MTYPE_CLOB_QUERY, MSTATUS_PR_STD_GET_OWN_DATA_MAX, 1, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_QUERY_SPIN:
       return simple_built_in_cfunction_call(MTYPE_CLOB_QUERY, MSTATUS_PR_STD_GET_SPIN, 1, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_QUERY_GR_X:
       return simple_built_in_cfunction_call(MTYPE_CLOB_QUERY, MSTATUS_PR_STD_GET_GR_X, 1, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_QUERY_GR_Y:
       return simple_built_in_cfunction_call(MTYPE_CLOB_QUERY, MSTATUS_PR_STD_GET_GR_Y, 1, target_register, secondary_register, save_secondary_register);
//      case BUILTIN_CFUNCTION_QUERY_GR_ANGLE:
//       return simple_built_in_cfunction_call(MTYPE_CLOB_QUERY, MSTATUS_PR_STD_GET_GR_ANGLE, 1, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_QUERY_GR_SPEED_X:
       return simple_built_in_cfunction_call(MTYPE_CLOB_QUERY, MSTATUS_PR_STD_GET_GR_SPEED_X, 1, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_QUERY_GR_SPEED_Y:
       return simple_built_in_cfunction_call(MTYPE_CLOB_QUERY, MSTATUS_PR_STD_GET_GR_SPEED_Y, 1, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_QUERY_GR_MEMBERS:
       return simple_built_in_cfunction_call(MTYPE_CLOB_QUERY, MSTATUS_PR_STD_GET_GR_MEMBERS, 1, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_QUERY_EX_TIME:
       return simple_built_in_cfunction_call(MTYPE_CLOB_QUERY, MSTATUS_PR_STD_GET_EX_COUNT, 1, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_QUERY_EFFICIENCY:
       return simple_built_in_cfunction_call(MTYPE_CLOB_QUERY, MSTATUS_PR_STD_GET_EFFICIENCY, 1, target_register, secondary_register, save_secondary_register);
// Remember that queries should generally accept 1 more parameter than the equivalent get call, because they need a process index.
      case BUILTIN_CFUNCTION_QUERY_VERTICES:
       return simple_built_in_cfunction_call(MTYPE_CLOB_QUERY, MSTATUS_PR_STD_GET_VERTICES, 1, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_QUERY_VERTEX_ANGLE:
       return simple_built_in_cfunction_call(MTYPE_CLOB_QUERY, MSTATUS_PR_STD_GET_VERTEX_ANGLE, 2, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_QUERY_VERTEX_DIST:
       return simple_built_in_cfunction_call(MTYPE_CLOB_QUERY, MSTATUS_PR_STD_GET_VERTEX_DIST, 2, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_QUERY_VERTEX_ANGLE_NEXT:
       return simple_built_in_cfunction_call(MTYPE_CLOB_QUERY, MSTATUS_PR_STD_GET_VERTEX_ANGLE_NEXT, 2, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_QUERY_VERTEX_ANGLE_PREV:
       return simple_built_in_cfunction_call(MTYPE_CLOB_QUERY, MSTATUS_PR_STD_GET_VERTEX_ANGLE_PREV, 2, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_QUERY_VERTEX_ANGLE_MIN:
       return simple_built_in_cfunction_call(MTYPE_CLOB_QUERY, MSTATUS_PR_STD_GET_VERTEX_ANGLE_MIN, 2, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_QUERY_VERTEX_ANGLE_MAX:
       return simple_built_in_cfunction_call(MTYPE_CLOB_QUERY, MSTATUS_PR_STD_GET_VERTEX_ANGLE_MAX, 2, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_QUERY_METHOD:
       return simple_built_in_cfunction_call(MTYPE_CLOB_QUERY, MSTATUS_PR_STD_GET_METHOD, 2, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_QUERY_METHOD_FIND:
       return simple_built_in_cfunction_call(MTYPE_CLOB_QUERY, MSTATUS_PR_STD_GET_METHOD_FIND, 3, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_QUERY_MBANK:
       return simple_built_in_cfunction_call(MTYPE_CLOB_QUERY, MSTATUS_PR_STD_GET_MBANK, 2, target_register, secondary_register, save_secondary_register);

// maths functions
      case BUILTIN_CFUNCTION_ATAN2:
       return simple_built_in_cfunction_call(MTYPE_PR_MATHS, MSTATUS_PR_MATHS_ATAN2, 2, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_SIN:
       return simple_built_in_cfunction_call(MTYPE_PR_MATHS, MSTATUS_PR_MATHS_SIN, 2, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_COS:
       return simple_built_in_cfunction_call(MTYPE_PR_MATHS, MSTATUS_PR_MATHS_COS, 2, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_HYPOT:
       return simple_built_in_cfunction_call(MTYPE_PR_MATHS, MSTATUS_PR_MATHS_HYPOT, 2, target_register, secondary_register, save_secondary_register);

      case BUILTIN_CFUNCTION_TURN_DIR:
       return simple_built_in_cfunction_call(MTYPE_PR_MATHS, MSTATUS_PR_MATHS_TURN_DIR, 2, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_ANGLE_DIFF:
       return simple_built_in_cfunction_call(MTYPE_PR_MATHS, MSTATUS_PR_MATHS_ANGLE_DIFF, 2, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_ANGLE_DIFF_S:
       return simple_built_in_cfunction_call(MTYPE_PR_MATHS, MSTATUS_PR_MATHS_ANGLE_DIFF_S, 2, target_register, secondary_register, save_secondary_register);

      case BUILTIN_CFUNCTION_ABS:
       return simple_built_in_cfunction_call(MTYPE_PR_MATHS, MSTATUS_PR_MATHS_ABS, 1, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_SQRT:
       return simple_built_in_cfunction_call(MTYPE_PR_MATHS, MSTATUS_PR_MATHS_SQRT, 1, target_register, secondary_register, save_secondary_register);
      case BUILTIN_CFUNCTION_POW:
       return simple_built_in_cfunction_call(MTYPE_PR_MATHS, MSTATUS_PR_MATHS_POW, 2, target_register, secondary_register, save_secondary_register);
//      case BUILTIN_CFUNCTION_RANDOM:
//       return simple_built_in_cfunction_call(MTYPE_PR_MATHS, MSTATUS_PR_MATHS_RANDOM, 1, target_register, secondary_register, save_secondary_register);
      case BUILTIN_UNIMP_CONST:
      case BUILTIN_UNIMP_SIZEOF:
      case BUILTIN_UNIMP_STRUCT:
      case BUILTIN_UNIMP_UNION:
      case BUILTIN_UNIMP_EXTERN:
      case BUILTIN_UNIMP_UNSIGNED:
      case BUILTIN_UNIMP_SIGNED:
      case BUILTIN_UNIMP_TYPEDEF:
      case BUILTIN_UNIMP_INLINE:
      case BUILTIN_UNIMP_REGISTER:
       return comp_error(CERR_UNIMPLEMENTED_KEYWORD, &ctoken);

      case BUILTIN_UNSUP_CHAR:
      case BUILTIN_UNSUP_SHORT:
      case BUILTIN_UNSUP_FLOAT:
      case BUILTIN_UNSUP_DOUBLE:
       return comp_error(CERR_UNSUPPORTED_DATA_TYPE, &ctoken);

     }
     break; // end CTOKEN_TYPE_IDENTIFIER_BUILTIN_CFUNCTION

    case CTOKEN_TYPE_OPERATOR_ARITHMETIC: // only & is accepted
     if (ctoken.subtype == CTOKEN_SUBTYPE_BITWISE_AND)
     {
// can't combine address-of prefix with dereferencing (*) or negation (!)
      if (dereference > 0)
       return comp_error(CERR_DEREFERENCED_ADDRESS_OF, &ctoken);
      if (apply_not > 0 || apply_bitwise_not > 0)
       return comp_error(CERR_NEGATED_ADDRESS_OF, &ctoken);
// get address of variable:
      if (!read_next_expression_value(&ctoken))
       comp_error(CERR_READ_FAIL, &ctoken);
      if (ctoken.type != CTOKEN_TYPE_IDENTIFIER_USER_VARIABLE)
       return comp_error(CERR_ADDRESS_OF_PREFIX_MISUSE, &ctoken);
      if (identifier[ctoken.identifier_index].array_dims == 0)
      {
// simplest case: address of non-array static variable:
       if (identifier[ctoken.identifier_index].storage_class == STORAGE_STATIC)
       {
        add_intercode(IC_ID_ADDRESS_PLUS_OFFSET_TO_REGISTER, target_register, ctoken.identifier_index, 0);
        return 1;
       }
// address of non-array automatic variable:
       if (!auto_var_address_to_register(ctoken.identifier_index, target_register, 0))
        return 0;
       return 1;
      }
// for an array, can use parse_array_reference
      return parse_array_reference(target_register, secondary_register, save_secondary_register, ctoken.identifier_index, 1); // ,1 means the address of the element will be in the register

     } // end address-of operator
     return comp_error(CERR_SYNTAX_EXPRESSION_VALUE, &ctoken); // end CTOKEN_TYPE_OPERATOR_ARITHMETIC

    default:
     return comp_error(CERR_SYNTAX_EXPRESSION_VALUE, &ctoken);
   }

     return comp_error(CERR_SYNTAX_EXPRESSION_VALUE, &ctoken);

} // end parse_expression_value


int parse_expression_operator(int target_register, int secondary_register, int save_secondary_register, int exit_point, int binding)
{

   cstate.recursion_level ++;

   if (cstate.recursion_level > RECURSION_LIMIT)
    return comp_error(CERR_RECURSION_LIMIT_REACHED_IN_EXPRESSION, NULL);

   if (cstate.error != CERR_NONE)
    return 0;

   int retval = 0;
   int return_value = 1;
   struct ctokenstruct ctoken_operator;
//   struct ctokenstruct ctoken_value;
//   int expression_complete = 0;

   int save_scode_pos = cstate.scode_pos;

   if (!read_next(&ctoken_operator))
    return comp_error(CERR_READ_FAIL, &ctoken_operator);

   switch(ctoken_operator.type)
   {
    case CTOKEN_TYPE_OPERATOR_ARITHMETIC:
     if (save_secondary_register)
      add_intercode(IC_PUSH_REGISTER, secondary_register, REGISTER_STACK, 0);

     retval = parse_expression_value(secondary_register, target_register, 1, exit_point, binding); // note reverse order; we want the result in the secondary register so we can use it to affect target_register

     if (retval == 0)
      return 0;
/*
     switch(binding)
     {
      case BIND_ARITHMETIC:
       retval = parse_expression_value(secondary_register, target_register, 1, exit_point, BIND_ARITHMETIC); // note reverse order; we want the result in the secondary register so we can use it to affect target_register
       break;
      case BIND_COMPARISON:
       return 1;
     }*/
// fall through here
    case CTOKEN_TYPE_OPERATOR_COMPARISON: // this is very similar to arithmetic operators, but differing precedence means it calls parse_expression rather than parse_expression_value
     if (ctoken_operator.type == CTOKEN_TYPE_OPERATOR_COMPARISON)
     {
      if (save_secondary_register)
       add_intercode(IC_PUSH_REGISTER, secondary_register, REGISTER_STACK, 0);
//      retval = parse_expression(secondary_register, target_register, 1, exit_point, 0, BIND_COMPARISON); // note reverse order of registers
//      if (binding == BIND_
      retval = parse_expression(secondary_register, target_register, 1, exit_point, 0, BIND_COMPARISON); // note reverse order of registers

     }
     if (retval == 0)
      return 0;

       switch(ctoken_operator.subtype)
       {
        case CTOKEN_SUBTYPE_PLUS:
         add_intercode(IC_ADD_REG_TO_REG, target_register, target_register, secondary_register);
         break;
        case CTOKEN_SUBTYPE_MINUS:
         add_intercode(IC_SUB_REG_FROM_REG, target_register, target_register, secondary_register);
         break;
        case CTOKEN_SUBTYPE_MUL:
         add_intercode(IC_MUL_REG_BY_REG, target_register, target_register, secondary_register);
         break;
        case CTOKEN_SUBTYPE_DIV:
         add_intercode(IC_DIV_REG_BY_REG, target_register, target_register, secondary_register);
         break;
        case CTOKEN_SUBTYPE_MOD:
         add_intercode(IC_MOD_REG_BY_REG, target_register, target_register, secondary_register);
         break;
        case CTOKEN_SUBTYPE_BITWISE_AND:
         add_intercode(IC_AND_REG_REG, target_register, target_register, secondary_register);
         break;
        case CTOKEN_SUBTYPE_BITWISE_XOR:
         add_intercode(IC_XOR_REG_REG, target_register, target_register, secondary_register);
         break;
        case CTOKEN_SUBTYPE_BITWISE_OR:
         add_intercode(IC_OR_REG_REG, target_register, target_register, secondary_register);
         break;
        case CTOKEN_SUBTYPE_BITWISE_NOT:
         add_intercode(IC_NOT_REG_REG, target_register, target_register, secondary_register);
         break;
        case CTOKEN_SUBTYPE_BITSHIFT_L:
         add_intercode(IC_BSHIFTL_REG_REG, target_register, target_register, secondary_register);
         break;
        case CTOKEN_SUBTYPE_BITSHIFT_R:
         add_intercode(IC_BSHIFTR_REG_REG, target_register, target_register, secondary_register);
         break;


        case CTOKEN_SUBTYPE_EQ_EQ: // ==
         add_intercode(IC_COMPARE_REGS_EQUAL, target_register, secondary_register, 0); // this leaves register A with 1 if true, 0 if false
         break;
        case CTOKEN_SUBTYPE_GR: // >
         add_intercode(IC_COMPARE_REGS_GR, target_register, secondary_register, 0); // this leaves register A with 1 if true, 0 if false
         break;
        case CTOKEN_SUBTYPE_GREQ: // >=
         add_intercode(IC_COMPARE_REGS_GRE, target_register, secondary_register, 0); // this leaves register A with 1 if true, 0 if false
         break;
        case CTOKEN_SUBTYPE_LESS: // <
         add_intercode(IC_COMPARE_REGS_LS, target_register, secondary_register, 0); // this leaves register A with 1 if true, 0 if false
         break;
        case CTOKEN_SUBTYPE_LESEQ: // <=
         add_intercode(IC_COMPARE_REGS_LSE, target_register, secondary_register, 0); // this leaves register A with 1 if true, 0 if false
         break;
        case CTOKEN_SUBTYPE_COMPARE_NOT: // !=
         add_intercode(IC_COMPARE_REGS_NEQUAL, target_register, secondary_register, 0); // this leaves register A with 1 if true, 0 if false
         break;

        default:
         fprintf(stdout, "\nUnimplemented operator found?");
         error_call();
         break;
       }

     if (save_secondary_register)
      add_intercode(IC_POP_TO_REGISTER, secondary_register, REGISTER_STACK, 0);

     break; // end CTOKEN_TYPE_OPERATOR_ARITHMETIC

    case CTOKEN_TYPE_OPERATOR_LOGICAL:
     switch(ctoken_operator.subtype)
     {
      case CTOKEN_SUBTYPE_LOGICAL_AND:
       if (binding == 1)
       {
        cstate.scode_pos = save_scode_pos;
        return_value = 2;
        goto parse_expression_operator_success;
// binding will be 1 if there is a comparison that needs to be resolved before the logical operator can be processed.
       }
       add_intercode(IC_IFFALSE_JUMP_TO_EXIT_POINT, target_register, exit_point, 0); // value is the conditional exit point for this level of the expression
       return_value = parse_expression(target_register, secondary_register, 0, -1, 0, BIND_BASIC); // I don't think it's necessary to save the working register - we should be able to discard it as its only purpose is served by the iffalse
       goto parse_expression_operator_success;
      case CTOKEN_SUBTYPE_LOGICAL_OR:
       if (binding == 1)
       {
        cstate.scode_pos = save_scode_pos;
        return_value = 2;
        goto parse_expression_operator_success;
       }
       add_intercode(IC_IFTRUE_JUMP_TO_EXIT_POINT, target_register, exit_point, 0); // value is the conditional exit point for this level of the expression
       return_value = parse_expression(target_register, secondary_register, 0, exit_point, 0, BIND_BASIC); // I don't think it's necessary to save the working register - we should be able to discard it as its only purpose is served by the iffalse
       goto parse_expression_operator_success;

/*
This code for logical operators is poorly optimised, because in many cases it will cause a single t/f value to be tested by
iffalse/iftrue instructions multiple times. This happens especially where subexpressions are enclosed in brackets and separated by
|| (and maybe &&).

I could improve this by making it so that before an iftrue/iffalse instruction was added the compiler looked back at
immediately preceding exit point labels and:
 - if the instruction's truth value matched the exit point's - replacing jumps to the exit point with jumps to the instruction's exit point;
 - if not - putting the label after the instruction.
Should probably do this!


*/


     }
     break;

   }

   parse_expression_operator_success:

   cstate.recursion_level --;

   return return_value;

} // end parse_expression_operator

















// if the expression can be resolved to one or more operations that can be performed with a single register, this will produce intercode to do so.
// returns 1 on success, 0 on failure (failure doesn't necessarily mean an error was found, just that the expression could not be resolved so easily)
int attempt_expression_resolve(int target_register)
{

 struct ctokenstruct ctoken;
 int save_scode_pos;
 int retval;

// first check whether the expression can be resolved into a constant:

 save_scode_pos = cstate.scode_pos;

 retval = read_literal_numbers(&ctoken);

 switch(retval)
 {
  case 2: // expression was completely resolved. simple.
   add_intercode(IC_NUMBER_TO_REGISTER, target_register, ctoken.number_value, 0);
   return 1;

  case 1: // expression was unresolved, or only partly resolved. Can't use a partially resolved expression at this point, so reset scode_pos and try again.
   cstate.scode_pos = save_scode_pos;
   break;

  case 0: // error of some kind
   return comp_error(CERR_SYNTAX, &ctoken);
 }

// now check whether the expression can be resolved to a single memory address

 int id_index = -1;
 int offset = 0;

 retval = attempt_expression_resolve_to_address(&id_index, &offset);

// Test for an error here (because a failure by attempt_express_resolve_to_address() isn't necessarily an error)
 if (cstate.error != CERR_NONE)
  return 0;

 if (retval) // success!
 {
  add_intercode(IC_IDENTIFIER_PLUS_OFFSET_TO_REGISTER, target_register, id_index, offset); // offset may be 0
//  add_intercode(IC_, target_register, id_index);
//  add_intercode(IC_DEREFERENCE_REGISTER_TO_REGISTER, target_register, target_register, 0); I don't think this is needed
  return 1;
 }

 return 0; // couldn't resolve


} // end attempt_expression_resolve

// This function attempts to resolve a value to a single memory address.
// Can only resolve direct references to static variables, and direct references to constant elements of static arrays.
// returns 0 on failure (may not be an error).
// returns 1 if *id_index contains an address and *offset contains an offset that the expression resolves to (offset may be zero)
int attempt_expression_resolve_to_address(int* id_index, int* offset)
{

 (*offset) = 0;

 int retval, i, save_scode_pos;
 struct ctokenstruct ctoken;

 save_scode_pos = cstate.scode_pos;

 if (!read_next(&ctoken))
  return comp_error(CERR_READ_FAIL, &ctoken);

 if (ctoken.type == CTOKEN_TYPE_PUNCTUATION
  && ctoken.subtype == CTOKEN_SUBTYPE_BRACKET_OPEN)
 {
  retval = attempt_expression_resolve_to_address(id_index, offset);
  if (retval == 0)
  {
   cstate.scode_pos = save_scode_pos;
   return 0;
  }
  if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACKET_CLOSE))
   return 0; // not an error, as this might just mean that a variable name is followed by an operator
  return 1;
 }

 if (ctoken.type == CTOKEN_TYPE_IDENTIFIER_USER_VARIABLE
  && identifier[ctoken.identifier_index].storage_class == STORAGE_STATIC)
 {

   (*id_index) = ctoken.identifier_index;

   if (identifier[ctoken.identifier_index].array_dims == 0)
   {
    if (peek_check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACKET_CLOSE)
     || peek_check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_SQUARE_CLOSE)
     || peek_check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_SEMICOLON))
      return 1; // expression complete!

   }
    else
    {
// must be an array. Can we resolve it?

     int unresolved_elements = identifier[(*id_index)].array_dims;

// first we check through the dimensions to see which ones can be resolved into a simple constant:
     for (i = 0; i < identifier[(*id_index)].array_dims; i ++)
     {
      if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_SQUARE_OPEN))
       return comp_error(CERR_ARRAY_EXPECTED_SQUARE_OPEN, &ctoken);
      if (read_literal_numbers(&ctoken) == 2)
      {
       (*offset) += ctoken.number_value * identifier[(*id_index)].array_element_size [i];
       unresolved_elements --;
      }
       else
        break;
      if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_SQUARE_CLOSE)) // confirm closing square bracket
      {
       cstate.scode_pos = save_scode_pos; // to make sure error message displays correct position
       return comp_error(CERR_NO_CLOSE_SQUARE_BRACKET, &ctoken);
      }
     }
// maybe all of the elements have been resolved! Great! (although this may have been checked for before CM_ARRAY_REFERENCE was called)
     if (unresolved_elements == 0
      && (peek_check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACKET_CLOSE)
       || peek_check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_SEMICOLON)))
     {
      return 1; // expression complete!
     }

    }


 }

 cstate.scode_pos = save_scode_pos;
 return 0;

}



/*
Call this function when scode_pos is after the variable name identifier (whose index is id_index) and before the first square bracket.

returns 1 on success, 0 on failure.

if get_address is 1, the absolute address of the element will be left instead of the contents

*/
int parse_array_reference(int target_register, int secondary_register, int save_secondary_register, int id_index, int get_address)
{

   int i;
   struct ctokenstruct ctoken;
   int save_scode_pos = cstate.scode_pos;

   int array_offset = 0;
   int element_resolved [ARRAY_DIMENSIONS];
   int unresolved_elements = identifier[id_index].array_dims;
   int retval;
   for (i = 0; i < ARRAY_DIMENSIONS; i ++)
   {
    element_resolved [i] = 0;
   }

// first we check through the dimensions to see which ones can be resolved into a simple constant:
   for (i = 0; i < identifier[id_index].array_dims; i ++)
   {
    if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_SQUARE_OPEN))
     return comp_error(CERR_ARRAY_EXPECTED_SQUARE_OPEN, &ctoken);
    if (read_literal_numbers(&ctoken) == 2)
    {
     array_offset += ctoken.number_value * identifier[id_index].array_element_size [i];
     element_resolved [i] = 1;
     unresolved_elements --;
     if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_SQUARE_CLOSE)) // confirm closing square bracket
      return comp_error(CERR_NO_CLOSE_SQUARE_BRACKET, &ctoken);
    }
     else
     {
// if read_literal_numbers couldn't resolve the element, we still need to get to the end of it somehow.
// this code reads through everything until it finds the element's closing square bracket.
// it returns a "no close square bracket" on read_next failure because this is probably the
// correct error even if read_next reaches the end of the code.
      int scode_pos_of_square_open = cstate.scode_pos; // used to reset scode_pos for any error message.
      int bracket_count = 1;
      do
      {
       if (!read_next(&ctoken))
       {
        cstate.scode_pos = scode_pos_of_square_open;
        return comp_error(CERR_NO_CLOSE_SQUARE_BRACKET, &ctoken);
       }
       if (ctoken.type == CTOKEN_TYPE_PUNCTUATION)
       {
        if (ctoken.subtype == CTOKEN_SUBTYPE_SQUARE_CLOSE)
         bracket_count --;
        if (ctoken.subtype == CTOKEN_SUBTYPE_SQUARE_OPEN)
         bracket_count ++;
       }
      } while (bracket_count > 0);
     }
   }

// maybe all of the elements have been resolved! Great!
   if (unresolved_elements == 0)
   {
// check if the array is a pointer:
    if (identifier[id_index].pointer)
    {
// first deal with the case where array_offset is zero, as this can be dealt with easily:
     if (array_offset == 0)
     {
      if (identifier[id_index].storage_class == STORAGE_STATIC)
      {
// if the pointer is static, it holds the address of the array.
// so we can just move the pointer's value to the register (and dereference it if we want the value of the array's first element)
       add_intercode(IC_IDENTIFIER_TO_REGISTER, target_register, id_index, 0);
       if (!get_address)
        add_intercode(IC_DEREFERENCE_REGISTER_TO_REGISTER, target_register, target_register, 0); // this provides the first element of the array
       return 1;
      }
// must be automatic:
      if (!auto_var_address_to_register(id_index, target_register, 0))
       return 0;
// the value in target_register is the address of the pointer.
// dereference once to get the value of the pointer (which is the address of the first element of the array)
      add_intercode(IC_DEREFERENCE_REGISTER_TO_REGISTER, target_register, target_register, 0);
// can dereference again to get the value of the first element of the array:
      if (!get_address)
       add_intercode(IC_DEREFERENCE_REGISTER_TO_REGISTER, target_register, target_register, 0);
      return 1;
     } // end if (array_offset == 0)
// if it is a pointer with an offset, we need to add the offset at runtime.
// this means both registers will be needed:
     if (save_secondary_register == 1)
       add_intercode(IC_PUSH_REGISTER, secondary_register, REGISTER_STACK, 0);
     if (identifier[id_index].storage_class == STORAGE_STATIC)
     {
// first move the pointer's value to the target register
      add_intercode(IC_IDENTIFIER_TO_REGISTER, target_register, id_index, 0);
// now move the array offset to the other register, and add it to the target register:
      add_intercode(IC_NUMBER_TO_REGISTER, secondary_register, array_offset, 0);
      add_intercode(IC_ADD_REG_TO_REG, target_register, secondary_register, target_register);
// target_register now contains the address of the first element of the array.
      if (!get_address)
       add_intercode(IC_DEREFERENCE_REGISTER_TO_REGISTER, target_register, target_register, 0);
// if needed, retrieve the secondary_register
      if (save_secondary_register == 1)
        add_intercode(IC_POP_TO_REGISTER, secondary_register, REGISTER_STACK, 0);
      return 1;
     }
// must be automatic.
// first get the address of the pointer
     if (!auto_var_address_to_register(id_index, target_register, 0))
      return 0;
// now dereference it to find the address of the array it points to
     add_intercode(IC_DEREFERENCE_REGISTER_TO_REGISTER, target_register, target_register, 0);
// now add array_offset to the address
     add_intercode(IC_NUMBER_TO_REGISTER, secondary_register, array_offset, 0);
     add_intercode(IC_ADD_REG_TO_REG, target_register, secondary_register, target_register);
// target_register now contains the address of the first element of the array.
     if (!get_address)
      add_intercode(IC_DEREFERENCE_REGISTER_TO_REGISTER, target_register, target_register, 0);
     if (save_secondary_register== 1)
      add_intercode(IC_POP_TO_REGISTER, secondary_register, REGISTER_STACK, 0);
     return 1;
    }
// not a pointer (i.e. the identifier refers directly to the relevant array):
    if (identifier[id_index].storage_class == STORAGE_STATIC)
    {
     if (get_address)
      add_intercode(IC_ID_ADDRESS_PLUS_OFFSET_TO_REGISTER, target_register, id_index, array_offset);
       else
        add_intercode(IC_IDENTIFIER_PLUS_OFFSET_TO_REGISTER, target_register, id_index, array_offset);
     return 1;
    }
    if (!auto_var_address_to_register(id_index, target_register, array_offset))
     return 0;
    if (!get_address)
     add_intercode(IC_DEREFERENCE_REGISTER_TO_REGISTER, target_register, target_register, 0);
    if (save_secondary_register== 1)
     add_intercode(IC_POP_TO_REGISTER, secondary_register, REGISTER_STACK, 0);
    return 1;
   }

// NOTE: code below assumes that any elements that have been resolved by this point can be read past with a call to read_literal_numbers
//  If any other method of resolving them is added above, need to deal with this!

// okay, at least some of the dimensions remain unresolved. Need to work them out.
// reset scode_pos to begin from the first element again:
   cstate.scode_pos = save_scode_pos;
   if (save_secondary_register)
    add_intercode(IC_PUSH_REGISTER, secondary_register, REGISTER_STACK, 0);
// Now put the base address of the array data (including any constant offsets in array_offset) into target_register
   if (identifier[id_index].storage_class == STORAGE_STATIC)
   {
// if the identifier is actually a pointer to the array, need to use its contents rather than its address:
    if (identifier[id_index].pointer)
    {
     add_intercode(IC_IDENTIFIER_TO_REGISTER, target_register, id_index, 0);
// that will put the address pointed at by the pointer into REGISTER_WORKING. now need to add the offset:
     if (array_offset > 0)
     {
      add_intercode(IC_NUMBER_TO_REGISTER, secondary_register, array_offset, 0);
      add_intercode(IC_ADD_REG_TO_REG, target_register, secondary_register, target_register);
     }
    }
      else
// if the identifier is the array itself, not a pointer, can just put its address plus the offset directly into the register:
       add_intercode(IC_ID_ADDRESS_PLUS_OFFSET_TO_REGISTER, target_register, id_index, array_offset);
   }
     else
     {
// if the identifier is actually a pointer to the array, need to use its contents rather than its address:
      if (identifier[id_index].pointer)
      {
       if (!auto_var_address_to_register(id_index, target_register, 0))
        return 0;
// need to dereference the pointer:
       add_intercode(IC_DEREFERENCE_REGISTER_TO_REGISTER, target_register, target_register, 0);
// that will put the address pointed at by the pointer into target_register. now may need to add the offset:
       if (array_offset > 0)
       {
        add_intercode(IC_NUMBER_TO_REGISTER, secondary_register, array_offset, 0);
        add_intercode(IC_ADD_REG_TO_REG, target_register, secondary_register, target_register);
       }
      }
       else
       {
// if the identifier is the array itself, not a pointer, can just treat it as any other auto variable but with an offset:
        if (!auto_var_address_to_register(id_index, target_register, array_offset))
         return 0;
       }
     }

// now we have the array's base address plus any constant offset in target_register.
// so go through and add offsets for each element that hasn't yet been calculated:
// note that, at this stage, if save_secondary_register==1 the previous contents of secondary_register will be on the stack.
   for (i = 0; i < identifier[id_index].array_dims; i ++)
   {
    if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_SQUARE_OPEN))
     return comp_error(CERR_ARRAY_EXPECTED_SQUARE_OPEN, &ctoken);
// skip any elements resolved above
    if (element_resolved [i] == 0)
    {
     retval = parse_expression(secondary_register, target_register, 1, -1, 0, BIND_BASIC);
     if (retval == 0)
      return 0;
// parse_expression will have put result into secondary_register
       if (identifier[id_index].array_element_size [i] == 1)
       {
// simple - just add secondary_register to the address calculated so far
         add_intercode(IC_ADD_REG_TO_REG, target_register, secondary_register, target_register);
       }
        else
        {
// need to push target_register, set it to the element size, then multiply element size into secondary_register
         add_intercode(IC_PUSH_REGISTER, target_register, REGISTER_STACK, 0);
         add_intercode(IC_NUMBER_TO_REGISTER, target_register, identifier[id_index].array_element_size [i], 0);
         add_intercode(IC_MUL_REG_BY_REG, secondary_register, target_register, secondary_register);
// now retrieve the running total and add the offset in secondary_register to it
         add_intercode(IC_POP_TO_REGISTER, target_register, REGISTER_STACK, 0);
         add_intercode(IC_ADD_REG_TO_REG, target_register, target_register, secondary_register);
// this is really inefficient. Is there a better way? Maybe I could use a second temp register? That would save the stack operations.
        }
    } // end if element_resolved[i]==0
     else
      read_literal_numbers(&ctoken); // this will just read past the number, which has already been processed and added to array_reference
    if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_SQUARE_CLOSE))
     return comp_error(CERR_NO_CLOSE_SQUARE_BRACKET, &ctoken);
   }

// at this point we have the address of the element in target_register, and if save_secondary_register==1 the previous contents of secondary_register are on the stack.
// if get_address is 0, just need to dereference the target register to get the result:
 if (!get_address)
  add_intercode(IC_DEREFERENCE_REGISTER_TO_REGISTER, target_register, target_register, 0);
 if (save_secondary_register)
  add_intercode(IC_POP_TO_REGISTER, secondary_register, REGISTER_STACK, 0);

 return 1;

} // end parse_array_reference


// leaves the address of an auto variable in target_register.
// doesn't change any other registers
// if the variable is an array, the address will be to its first element
// then offset is added to allow array indexing
// returns 0 on failure (possibly cannot fail), 1 on success
int auto_var_address_to_register(int id_index, int target_register, int offset)
{

// First check whether the variable's stack offset is zero, which makes things easy:
     if (identifier[id_index].stack_offset == 0 && offset == 0)
     {
      add_intercode(IC_COPY_REGISTER2_TO_REGISTER1, target_register, REGISTER_STACK_FRAME, 0);
      return 1;
     }

// Copy the variable's stack offset plus any array index offset to target_register
     add_intercode(IC_NUMBER_TO_REGISTER, target_register, identifier[id_index].stack_offset + offset, 0);
// Now add the stack frame:
     add_intercode(IC_ADD_REG_TO_REG, target_register, REGISTER_STACK_FRAME, target_register);

     return 1;
}

void dereference_loop(int deref_register, int* dereferences)
{

  while (*dereferences)
  {
   add_intercode(IC_DEREFERENCE_REGISTER_TO_REGISTER, deref_register, deref_register, 0);
   (*dereferences) --;
  };

}

// This is the simple version of get that takes two parameters - method and offset from method's base mbank register. both parameters must be constants.
// call this just after "get" ctoken has been read in.
// returns 1 on success, 0 on failure
// secondary_register and save_secondary_register are currently unused, but may be used in future when get takes variables as parameters
int parse_get(int target_register, int secondary_register, int save_secondary_register)
{

 struct ctokenstruct ctoken;
 strcpy(ctoken.name, "(empty)");

// expect opening bracket
 if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACKET_OPEN))
  return comp_error(CERR_EXPECTED_BRACKET_OPEN, &ctoken);

 int method_index = 0;
 int mbank_index = 0;

// read in source method index
 int retval = read_literal_numbers(&ctoken);

 switch(retval)
 {
  case 0: // error of some kind
   return comp_error(CERR_SYNTAX, &ctoken);

  case 1:  // not a constant
   return comp_error(CERR_GET_PARAMETER_NOT_CONSTANT, &ctoken);

  case 2: // expression was completely resolved to a number. simple.
   method_index = ctoken.number_value;
   break;

 }

 if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_COMMA))
  return comp_error(CERR_EXPECTED_COMMA_AFTER_VALUE, &ctoken);

 retval = read_literal_numbers(&ctoken);

 switch(retval)
 {
  case 0: // error of some kind
   return comp_error(CERR_SYNTAX, &ctoken);

  case 1:  // not a constant
   return comp_error(CERR_GET_PARAMETER_NOT_CONSTANT, &ctoken);

  case 2: // expression was completely resolved to a number. simple.
   mbank_index = (method_index * METHOD_SIZE) + ctoken.number_value;
   if (mbank_index < 0 || mbank_index >= METHOD_BANK)
    return comp_error(CERR_MBANK_ADDRESS_OUT_OF_BOUNDS, &ctoken);
   add_intercode(IC_KNOWN_MBANK_TO_REGISTER, target_register, mbank_index, 0);
   break;

 }

// expect closing bracket
 if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACKET_CLOSE))
  return comp_error(CERR_EXPECTED_BRACKET_CLOSE, &ctoken);

// success!
 return 1;

}

// This version of get just uses a single number, which is the index of an mbank register (out of the whole 64 of them). Number doesn't have to be a constant.
// call this just after "get" ctoken has been read in.
// returns 1 on success, 0 on failure
int parse_get_index(int target_register, int secondary_register, int save_secondary_register)
{

 struct ctokenstruct ctoken;
 strcpy(ctoken.name, "(empty)");

// expect opening bracket
 if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACKET_OPEN))
  return comp_error(CERR_EXPECTED_BRACKET_OPEN, &ctoken);

// read in source mbank address
// if the address can be resolved to a constant, it can be contained in 6 bits of the instruction itself.
 int retval = read_literal_numbers(&ctoken);

 switch(retval)
 {
  case 0: // error of some kind
   return comp_error(CERR_SYNTAX, &ctoken);

  case 1: // expression was unresolved, or only partly resolved. Can't use a partially resolved expression here, so parse the expression.
//   if (!parse_expression(save_secondary_register, target_register, secondary_register, -1, 0, BIND_BASIC))
   if (!parse_expression(target_register, secondary_register, save_secondary_register, -1, 0, BIND_BASIC))
    return 0;
   add_intercode(IC_REGISTER_MBANK_TO_REGISTER, target_register, target_register, 0);
   break;

  case 2: // expression was completely resolved to a number. simple.
   add_intercode(IC_KNOWN_MBANK_TO_REGISTER, target_register, ctoken.number_value, 0);
   break;

 }

// expect closing bracket
 if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACKET_CLOSE))
  return comp_error(CERR_EXPECTED_BRACKET_CLOSE, &ctoken);

// success!
 return 1;

}


// like get
// call this just after "put" ctoken has been read in.
// returns 1 on success, 0 on failure
// (the put itself is void return)
int parse_put(void)
{

 struct ctokenstruct ctoken;
 strcpy(ctoken.name, "(empty)");

// expect opening bracket
 if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACKET_OPEN))
  return comp_error(CERR_EXPECTED_BRACKET_OPEN, &ctoken);

 int method_index = 0;
 int mbank_index = 0;

// read in target method index
 int retval = read_literal_numbers(&ctoken);

 switch(retval)
 {
  case 0: // error of some kind
   return comp_error(CERR_SYNTAX, &ctoken);

  case 1:  // not a constant
   return comp_error(CERR_PUT_PARAMETER_NOT_CONSTANT, &ctoken);

  case 2: // expression was completely resolved to a number. simple.
   method_index = ctoken.number_value;
   break;

 }

 if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_COMMA))
  return comp_error(CERR_EXPECTED_COMMA_AFTER_VALUE, &ctoken);

 retval = read_literal_numbers(&ctoken);

 switch(retval)
 {
  case 0: // error of some kind
   return comp_error(CERR_SYNTAX, &ctoken);

  case 1:  // not a constant
   return comp_error(CERR_PUT_PARAMETER_NOT_CONSTANT, &ctoken);

  case 2: // expression was completely resolved to a number. simple.
   mbank_index = (method_index * METHOD_SIZE) + ctoken.number_value;
   if (mbank_index < 0 || mbank_index >= METHOD_BANK)
    return comp_error(CERR_MBANK_ADDRESS_OUT_OF_BOUNDS, &ctoken);
   break;

 }

// check for comma after mbank number:
  if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_COMMA))
   return comp_error(CERR_SYNTAX, &ctoken);

// now go through all values being put, incrementing the target each time
  while (TRUE)
  {
   if (!parse_expression(REGISTER_WORKING, REGISTER_TEMP, 0, -1, 0, BIND_BASIC))
    return 0;
   if (mbank_index < 0 || mbank_index >= METHOD_BANK)
    return comp_error(CERR_MBANK_ADDRESS_OUT_OF_BOUNDS, &ctoken);
   add_intercode(IC_REGISTER_TO_KNOWN_MBANK, mbank_index, 0, REGISTER_WORKING); // middle operand not used here
   mbank_index ++;
   if (check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACKET_CLOSE))
    break;
   if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_COMMA))
    return comp_error(CERR_EXPECTED_COMMA_AFTER_VALUE, &ctoken);
  };

  if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_SEMICOLON))
   return comp_error(CERR_EXPECTED_SEMICOLON, &ctoken);

  return 1;


}


// like get_index but for put
// call this just after "put" ctoken has been read in.
// returns 1 on success, 0 on failure
// (the put itself is void return)
int parse_put_index(void)
{

 struct ctokenstruct ctoken;
 strcpy(ctoken.name, "(empty)");

// expect opening bracket
 if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACKET_OPEN))
  return comp_error(CERR_EXPECTED_BRACKET_OPEN, &ctoken);

// read in target mbank address
 int retval = read_literal_numbers(&ctoken);

 if (retval == 0) // error in trying to read literal numbers
  return comp_error(CERR_SYNTAX, &ctoken);

// if the address can be resolved to a constant, it can be contained in 6 bits of the instruction itself.
 if (retval == 2) // could resolve to known constant
 {

  int mbank_target = ctoken.number_value;

// check for comma after mbank number:
  if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_COMMA))
   return comp_error(CERR_SYNTAX, &ctoken);

// now go through all values being put, incrementing the target each time
  while (TRUE)
  {
   if (!parse_expression(REGISTER_WORKING, REGISTER_TEMP, 0, -1, 0, BIND_BASIC))
    return 0;
   if (mbank_target < 0 || mbank_target >= METHOD_BANK)
    return comp_error(CERR_MBANK_ADDRESS_OUT_OF_BOUNDS, &ctoken);
   add_intercode(IC_REGISTER_TO_KNOWN_MBANK, mbank_target, 0, REGISTER_WORKING); // middle operand not used here
   mbank_target ++;
   if (check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACKET_CLOSE))
    break;
   if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_COMMA))
    return comp_error(CERR_SYNTAX, &ctoken);
  };

  if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_SEMICOLON))
   return comp_error(CERR_EXPECTED_SEMICOLON, &ctoken);

  return 1;

 }


// if the mbank address can't be resolved to a known constant, we have to use a less efficient method:
 if (retval == 1) // could not resolve to known constant
 {
// first, read the mbank address into REGISTER_WORKING:
  if (!parse_expression(REGISTER_WORKING, REGISTER_TEMP, 0, -1, 0, BIND_BASIC))
   return 0;
// check for comma after mbank number:
  if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_COMMA))
   return comp_error(CERR_SYNTAX, &ctoken);
// now go through all values being put, incrementing the target each time
  while (TRUE)
  {
// read the expression into REGISTER_TEMP, saving REGISTER_WORKING (which contains the mbank address)
   if (!parse_expression(REGISTER_TEMP, REGISTER_WORKING, 1, -1, 0, BIND_BASIC))
    return 0;
   add_intercode(IC_REGISTER_TO_REGISTER_MBANK, REGISTER_WORKING, REGISTER_TEMP, 0);
// check for comma or closing bracket
   if (check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACKET_CLOSE))
    break;
   if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_COMMA))
    return comp_error(CERR_SYNTAX, &ctoken);
// must be a comma, so increment REGISTER_WORKING and go through the loop again:
   add_intercode(IC_INCR_REGISTER, REGISTER_WORKING, 0, 0);
  };

  if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_SEMICOLON))
   return comp_error(CERR_EXPECTED_SEMICOLON, &ctoken);

  return 1;

 }

// shouldn't be possible to end up here
 return 0;

}


// call this just after "call" ctoken has been read in.
// returns 1 on success, 0 on failure
// (the call itself is treated as int return by the compiler, with the result going in target_register)
int parse_call(int target_register, int secondary_register, int save_secondary_register)
{

 struct ctokenstruct ctoken;
 strcpy(ctoken.name, "(empty)");

// expect opening bracket
 if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACKET_OPEN))
  return comp_error(CERR_EXPECTED_BRACKET_OPEN, &ctoken);

// read in method being called:
 int retval = read_literal_numbers(&ctoken);

 if (retval == 0) // error in trying to read literal numbers
  return comp_error(CERR_SYNTAX, &ctoken);

// if the method can be resolved to a constant, it can be contained in 4 bits of the instruction itself.
 if (retval == 2) // could resolve to known constant
 {

  int method_target = ctoken.number_value;

  if (method_target < 0 || method_target >= METHODS)
   return comp_error(CERR_METHOD_OUT_OF_BOUNDS, &ctoken);

  if (check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACKET_CLOSE))
  {
// simple - e.g. call(1);. Can just add the call to intercode and return.
   add_intercode(IC_CALL_METHOD_CONSTANT, method_target, 0, target_register);
   return 1;
  }

// Not followed by a close bracket - must be a complex call (e.g. call(1, 2, 1);). check for comma after method number:
  if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_COMMA))
   return comp_error(CERR_SYNTAX, &ctoken);

  int mbank_target = method_target * METHOD_SIZE;

// now go through all values being put, incrementing the target each time
  while (TRUE)
  {
   if (!parse_expression(target_register, secondary_register, save_secondary_register, -1, 0, BIND_BASIC))
    return 0;
   if (mbank_target >= METHOD_BANK)
    return comp_error(CERR_MBANK_ADDRESS_OUT_OF_BOUNDS, &ctoken);
   add_intercode(IC_REGISTER_TO_KNOWN_MBANK, mbank_target, 0, target_register); // middle operand not used here
   mbank_target ++;
   if (check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACKET_CLOSE))
    break;
   if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_COMMA))
    return comp_error(CERR_SYNTAX, &ctoken);
  };

  add_intercode(IC_CALL_METHOD_CONSTANT, method_target, 0, target_register);

  return 1;

 }


// if the method number can't be resolved to a known constant, we have to use a less efficient method.
//  also, this method does not currently allow the target method expression to be followed by values to be put
 if (retval == 1) // could not resolve to known constant
 {
// first, read the method address into REGISTER_WORKING:
   if (!parse_expression(target_register, secondary_register, save_secondary_register, -1, 0, BIND_BASIC))
    return 0;
// check for comma after method number (not allowed when using this call type):
   if (check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_COMMA))
    return comp_error(CERR_COMPLEX_CALL_MUST_BE_CONSTANT, &ctoken);

   add_intercode(IC_CALL_METHOD_REGISTER, target_register, target_register, 0);

// check for ); at end
   if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACKET_CLOSE))
    return comp_error(CERR_EXPECTED_BRACKET_CLOSE, &ctoken);

   return 1;
 }

// shouldn't be possible to end up here
 return 0;

}



// call this just after "process_start" or "process_end" ctoken has been read in.
// "end" value is 0 for process_start, 1 for process_end
// returns 1 on success, 0 on failure
int parse_process_start_or_end(int target_register, int end)
{

 struct ctokenstruct ctoken;
 strcpy(ctoken.name, "(empty)");

// expect opening bracket
 if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACKET_OPEN))
  return comp_error(CERR_EXPECTED_BRACKET_OPEN, &ctoken);

 if (!read_next(&ctoken))
  return comp_error(CERR_READ_FAIL, &ctoken);
 if (ctoken.type != CTOKEN_TYPE_PROCESS)
  return comp_error(CERR_EXPECTED_PROCESS_NAME, &ctoken);

 if (end)
  add_intercode(IC_PROCESS_END_TO_REGISTER, target_register, identifier[ctoken.identifier_index].process_index, 0);
   else
    add_intercode(IC_PROCESS_START_TO_REGISTER, target_register, identifier[ctoken.identifier_index].process_index, 0);

 if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACKET_CLOSE))
  return comp_error(CERR_EXPECTED_BRACKET_CLOSE, &ctoken);

 return 1;

}


/*
This function expects an expression, and adds intercode to assign the result of the expression to mbank address mb.

*/
int expression_value_to_mbank(int mb)
{

 if (!parse_expression_value(REGISTER_WORKING, REGISTER_TEMP, 0, -1, BIND_BASIC))
  return 0;
 add_intercode(IC_REGISTER_TO_KNOWN_MBANK, mb, 0, REGISTER_WORKING);
 return 1;
}

// read in source mbank address
// if the address can be resolved to a constant, it can be contained in 6 bits of the instruction itself.
/* int retval = read_literal_numbers(&ctoken);

 switch(retval)
 {
  case 0: // error of some kind
   return comp_error(CERR_SYNTAX, &ctoken);

  case 1: // expression was unresolved, or only partly resolved. Can't use a partially resolved expression here, so parse the expression.
   if (!parse_expression(save_secondary_register, target_register, secondary_register, -1))
    return 0;
   add_intercode(IC_REGISTER_MBANK_TO_REGISTER, target_register, target_register, 0);
   break;

  case 2: // expression was completely resolved to a number. simple.
   add_intercode(IC_KNOWN_MBANK_TO_REGISTER, target_register, ctoken.number_value, 0);
   break;

 }

// expect closing bracket
 if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACKET_CLOSE))
  return comp_error(CERR_EXPECTED_BRACKET_CLOSE, &ctoken);

// success!
 return 1;
*/



/*
Call this function to parse a single statement, or multiple statements encased in {}
statement_exit_point indicates the exit point for break and continue statements within a loop.
statement_exit_point is -1 if not in a loop
if final_closing_bracket==1 and the statement is an assignment, a closing bracket will be accepted at the end instead of a semicolon (this is for the last statement in a for loop)
if in_switch==1, break is treated as breaking a case (usually it's treated as breaking a loop).

returns 1 on success, 0 on error (in which case comp_error will have been called)

*/

int comp_statement(int statement_exit_point, int final_closing_bracket)
{

 cstate.recursion_level ++; // this will be decremented later on (see comp_statement_success label) if the function returns successfully

 if (cstate.recursion_level >= RECURSION_LIMIT)
	{
  return comp_error(CERR_RECURSION_LIMIT_REACHED_IN_STATEMENT, NULL);
	}

 struct ctokenstruct ctoken;
 strcpy(ctoken.name, "(empty)");

 struct ctokenstruct ctoken_operator;
 int retval = 1;
 int save_scode_pos, save_scode_pos2;

   if (!read_next(&ctoken))
    return comp_error(CERR_READ_FAIL, &ctoken);

   switch(ctoken.type)
   {

    case CTOKEN_TYPE_PUNCTUATION:
     if (ctoken.subtype == CTOKEN_SUBTYPE_SEMICOLON)
     {
      retval = 1;
      goto comp_statement_success; // does nothing
     }
     if (ctoken.subtype != CTOKEN_SUBTYPE_BRACE_OPEN) // a brace is the only other punctuation that's accepted at the start of a line
      return comp_error(CERR_SYNTAX_AT_STATEMENT_START, &ctoken);
     while(TRUE)
     {
      if (check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACE_CLOSE))
       break;
// maybe put icstate.just_returned = 0 here? (and set it to 1 in the KEYWORD_RETURN case below?)
      if (!comp_statement(statement_exit_point, 0))
       return 0;
     };
     break;

    case CTOKEN_TYPE_IDENTIFIER_KEYWORD:
     switch(ctoken.identifier_index)
     {
      case KEYWORD_INT:
       if (cstate.cprocess->cfunction[cstate.scope].storage_class == STORAGE_AUTO)
       {
        if (!comp_next(CM_DECLARE_INT_AUTO, 0))
         return 0;
       }
        else
        {
         if (!comp_next(CM_DECLARE_INT_STATIC, 0))
          return 0;
        }
       break;
      case KEYWORD_STATIC:
       if (!check_next(CTOKEN_TYPE_IDENTIFIER_KEYWORD, KEYWORD_INT)) // expect the next ctoken to be "int"
        return comp_error(CERR_VAR_DECLARATION_FAIL, &ctoken);
       if (!comp_next(CM_DECLARE_INT_STATIC, 0))
        return 0;
       break;
      case KEYWORD_AUTO:
       if (cstate.cprocess->cfunction[cstate.scope].storage_class == STORAGE_STATIC)
        return comp_error(CERR_AUTO_VAR_IN_STATIC_CFUNCTION, &ctoken);
       if (!check_next(CTOKEN_TYPE_IDENTIFIER_KEYWORD, KEYWORD_INT)) // expect the next ctoken to be "int"
        return comp_error(CERR_VAR_DECLARATION_FAIL, &ctoken);
       if (!comp_next(CM_DECLARE_INT_AUTO, 0))
        return 0;
       break;
      case KEYWORD_IF:
       if (!comp_next(CM_IF, statement_exit_point))
        return 0;
       break;
      case KEYWORD_RETURN:
       if (!comp_next(CM_RETURN_STATEMENT, 0))
        return 0;
       break;
      case KEYWORD_PRINT:
       if (!comp_next(CM_PRINT, 0))
        return 0;
       break;
      case KEYWORD_WHILE:
       if (!comp_next(CM_WHILE, 0))
        return 0;
       break;
      case KEYWORD_DO:
       if (!comp_next(CM_DO, 0))
        return 0;
       break;
      case KEYWORD_FOR:
       if (!comp_next(CM_FOR, 0))
        return 0;
       break;
      case KEYWORD_BREAK:
       if (statement_exit_point == -1)
        return comp_error(CERR_BREAK_WITHOUT_LOOP_ETC, &ctoken);
       if (expoint[statement_exit_point].type != EXPOINT_TYPE_LOOP
        && expoint[statement_exit_point].type != EXPOINT_TYPE_SWITCH)
         return comp_error(CERR_BREAK_WITHOUT_LOOP_ETC, &ctoken); // this probably shouldn't be possible but check for it anyway
       add_intercode(IC_JUMP_EXIT_POINT_FALSE, statement_exit_point, 0, 0);
       if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_SEMICOLON))
        return comp_error(CERR_SYNTAX, &ctoken);
       break;
      case KEYWORD_CONTINUE:
       if (statement_exit_point == -1)
        return comp_error(CERR_CONTINUE_WITHOUT_LOOP, &ctoken);
       if (expoint[statement_exit_point].type != EXPOINT_TYPE_LOOP)
         return comp_error(CERR_CONTINUE_WITHOUT_LOOP, &ctoken); // e.g. continue used in switch statement (in which case statement_exit_point will be EXPOINT_TYPE_SWITCH)
       add_intercode(IC_JUMP_EXIT_POINT_TRUE, statement_exit_point, 0, 0);
       if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_SEMICOLON))
        return comp_error(CERR_SYNTAX, &ctoken);
       break;
      case KEYWORD_GOTO:
       if (!comp_next(CM_GOTO, 0))
        return 0;
       break;
      case KEYWORD_ASM:
       if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACE_OPEN))
        return comp_error(CERR_EXPECTED_BRACE_OPEN, &ctoken);
       if (!assemble(process_def[cstate.process_scope].asm_only))
        return 0;
       break;
      case KEYWORD_SWITCH:
       if (!comp_switch())
        return 0;
       break;
      case KEYWORD_CASE: // a case label within a switch should have been intercepted by comp_switch
       return comp_error(CERR_CASE_WITHOUT_SWITCH, &ctoken);
      case KEYWORD_DEFAULT: // a default label within a switch should have been intercepted by comp_switch
       return comp_error(CERR_DEFAULT_WITHOUT_SWITCH, &ctoken);
//      case KEYWORD_PROCESS: - not here - process only allowed in CM_BASIC (see comp_next)
//       return comp_process();
      default: return comp_error(CERR_UNEXPECTED_KEYWORD, &ctoken);
     }
     break; // end CTOKEN_TYPE_IDENTIFIER_KEYWORD

    case CTOKEN_TYPE_IDENTIFIER_USER_CFUNCTION:
     if (!comp_next(CM_CALL_CFUNCTION, identifier[ctoken.identifier_index].cfunction_index))
      return 0;
     if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_SEMICOLON))
      return comp_error(CERR_EXPECTED_SEMICOLON, &ctoken);
     break; // end CTOKEN_TYPE_IDENTIFIER_USER_CFUNCTION

    case CTOKEN_TYPE_IDENTIFIER_BUILTIN_CFUNCTION:
     switch(ctoken.identifier_index)
     {
      case BUILTIN_CFUNCTION_GET:
      case BUILTIN_CFUNCTION_GET_INDEX:
      case BUILTIN_CFUNCTION_PROCESS_START:
      case BUILTIN_CFUNCTION_PROCESS_END:
       return comp_error(CERR_BUILTIN_CFUNCTION_OUTSIDE_EXPRESSION, &ctoken);
      case BUILTIN_CFUNCTION_PUT:
       retval = parse_put();
       goto comp_statement_success;
      case BUILTIN_CFUNCTION_PUT_INDEX:
       retval = parse_put_index();
       goto comp_statement_success;
      case BUILTIN_CFUNCTION_CALL:
       retval = parse_call(REGISTER_WORKING, REGISTER_TEMP, 0);
       if (retval == 0)
        return 0; // must have been an error
       if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_SEMICOLON))
        return comp_error(CERR_EXPECTED_SEMICOLON, &ctoken);
       goto comp_statement_success;
//      case BUILTIN_CFUNCTION_NEW_PROCESS:
//       return parse_new_process();
      case BUILTIN_CFUNCTION_SET_COMMAND: // args are command index and value
       retval = simple_built_in_cfunction_call(MTYPE_PR_STD, MSTATUS_PR_STD_SET_COMMAND, 2, REGISTER_WORKING, REGISTER_TEMP, 0);
       if (retval == 0)
        return 0; // must have been an error
       if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_SEMICOLON))
        return comp_error(CERR_EXPECTED_SEMICOLON, &ctoken);
       goto comp_statement_success;
      case BUILTIN_CFUNCTION_SET_COMMAND_BIT_0: // args are command index and field
       retval = simple_built_in_cfunction_call(MTYPE_PR_STD, MSTATUS_PR_STD_COMMAND_BIT_0, 2, REGISTER_WORKING, REGISTER_TEMP, 0);
       if (retval == 0)
        return 0; // must have been an error
       if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_SEMICOLON))
        return comp_error(CERR_EXPECTED_SEMICOLON, &ctoken);
       goto comp_statement_success;
      case BUILTIN_CFUNCTION_SET_COMMAND_BIT_1: // args are command index and field
       retval = simple_built_in_cfunction_call(MTYPE_PR_STD, MSTATUS_PR_STD_COMMAND_BIT_1, 2, REGISTER_WORKING, REGISTER_TEMP, 0);
       if (retval == 0)
        return 0; // must have been an error
       if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_SEMICOLON))
        return comp_error(CERR_EXPECTED_SEMICOLON, &ctoken);
//       retval = simple_built_in_cfunction_call(MTYPE_PR_COMMAND, 1, 2, REGISTER_WORKING, REGISTER_TEMP, 0);
       goto comp_statement_success;
      case BUILTIN_CFUNCTION_COMMAND:  // args are proc index, command index and value
       retval = simple_built_in_cfunction_call(MTYPE_CL_COMMAND_GIVE, MSTATUS_CL_COMMAND_SET_VALUE, 3, REGISTER_WORKING, REGISTER_TEMP, 0);
       if (retval == 0)
        return 0; // must have been an error
       if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_SEMICOLON))
        return comp_error(CERR_EXPECTED_SEMICOLON, &ctoken);
       goto comp_statement_success;
      case BUILTIN_CFUNCTION_COMMAND_BIT_0:  // args are proc index, command index and bit field
       retval = simple_built_in_cfunction_call(MTYPE_CL_COMMAND_GIVE, MSTATUS_CL_COMMAND_BIT_0, 3, REGISTER_WORKING, REGISTER_TEMP, 0);
       if (retval == 0)
        return 0; // must have been an error
       if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_SEMICOLON))
        return comp_error(CERR_EXPECTED_SEMICOLON, &ctoken);
       goto comp_statement_success;
      case BUILTIN_CFUNCTION_COMMAND_BIT_1:  // args are proc index, command index and bit field
       retval = simple_built_in_cfunction_call(MTYPE_CL_COMMAND_GIVE, MSTATUS_CL_COMMAND_BIT_1, 3, REGISTER_WORKING, REGISTER_TEMP, 0);
       if (retval == 0)
        return 0; // must have been an error
       if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_SEMICOLON))
        return comp_error(CERR_EXPECTED_SEMICOLON, &ctoken);
       goto comp_statement_success;
// remember to add a semicolon check if calling a function that can parse both statements and expressions
//  (as it can't check for a semicolon if it's parsing an expression)

      case BUILTIN_UNIMP_CONST:
      case BUILTIN_UNIMP_SIZEOF:
      case BUILTIN_UNIMP_STRUCT:
      case BUILTIN_UNIMP_UNION:
      case BUILTIN_UNIMP_EXTERN:
      case BUILTIN_UNIMP_UNSIGNED:
      case BUILTIN_UNIMP_SIGNED:
      case BUILTIN_UNIMP_TYPEDEF:
      case BUILTIN_UNIMP_INLINE:
      case BUILTIN_UNIMP_REGISTER:
       return comp_error(CERR_UNIMPLEMENTED_KEYWORD, &ctoken);

      case BUILTIN_UNSUP_CHAR:
      case BUILTIN_UNSUP_SHORT:
      case BUILTIN_UNSUP_FLOAT:
      case BUILTIN_UNSUP_DOUBLE:
       return comp_error(CERR_UNSUPPORTED_DATA_TYPE, &ctoken);

     }
     break;

    case CTOKEN_TYPE_IDENTIFIER_USER_VARIABLE:
     if (identifier[ctoken.identifier_index].array_dims == 0)
     {
      if (!read_next(&ctoken_operator))
       return comp_error(CERR_READ_FAIL, &ctoken_operator);
      if (ctoken_operator.type != CTOKEN_TYPE_OPERATOR_ASSIGN)
       return comp_error(CERR_EXPECTED_ASSIGNMENT_OPERATOR, &ctoken_operator);
// check for ++ or --
      if (ctoken_operator.subtype == CTOKEN_SUBTYPE_INCREMENT
       || ctoken_operator.subtype == CTOKEN_SUBTYPE_DECREMENT)
      {
       if (ctoken_operator.subtype == CTOKEN_SUBTYPE_INCREMENT
        && !comp_next(CM_INCR_OPERATION, ctoken.identifier_index))
        return 0;
       if (ctoken_operator.subtype == CTOKEN_SUBTYPE_DECREMENT
        && !comp_next(CM_DECR_OPERATION, ctoken.identifier_index))
        return 0;
       if (!comp_next(CM_END_ASSIGNMENT_STATEMENT, final_closing_bracket))
        return 0;
       break; // finished
      } // end ++/--
      if (!parse_expression(REGISTER_WORKING, REGISTER_TEMP, 0, -1, 0, BIND_BASIC))
       return 0; //comp_error(CERR_SYNTAX, &ctoken);
      if (!comp_next(CM_END_ASSIGNMENT_STATEMENT, final_closing_bracket))
       return 0;
// At this point, REGISTER_WORKING holds the result of the expression call above
// now we check to see whether the variable is static or auto:
      if (identifier[ctoken.identifier_index].storage_class == STORAGE_STATIC)
      {
       if (ctoken_operator.subtype != CTOKEN_SUBTYPE_EQ)
       {
// must be +=, -= etc
// so put variable into REGISTER_TEMP:
        add_intercode(IC_IDENTIFIER_TO_REGISTER, REGISTER_TEMP, ctoken.identifier_index, 0);
// perform operation:
        if (!assignment_operation(ctoken_operator.subtype, REGISTER_WORKING, REGISTER_TEMP, REGISTER_WORKING)) // this leaves the result in REGISTER_WORKING
         return 0;
       }
// now copy REGISTER_WORKING to the variable's address
       add_intercode(IC_COPY_REGISTER_TO_IDENTIFIER_ADDRESS, ctoken.identifier_index, REGISTER_WORKING, 0);
      } // end static variable
       else
       {
        if (!auto_var_address_to_register(ctoken.identifier_index, REGISTER_TEMP, 0))
         return 0;
        if (ctoken_operator.subtype != CTOKEN_SUBTYPE_EQ)
        {
// must be +=, -= etc
// need to save REGISTER_TEMP:
         add_intercode(IC_PUSH_REGISTER, REGISTER_TEMP, REGISTER_STACK, 0);
// dereference it:
         add_intercode(IC_DEREFERENCE_REGISTER_TO_REGISTER, REGISTER_TEMP, REGISTER_TEMP, 0);
// perform operation:
        if (!assignment_operation(ctoken_operator.subtype, REGISTER_WORKING, REGISTER_TEMP, REGISTER_WORKING)) // this leaves the result in REGISTER_WORKING
         return 0;
// retrieve variable address into REGISTER_TEMP:
         add_intercode(IC_POP_TO_REGISTER, REGISTER_TEMP, REGISTER_STACK, 0);
        }
// So now we need to copy the contents of REGISTER_WORKING to the address pointed to by REGISTER_TEMP:
        add_intercode(IC_COPY_REGISTER_TO_DEREFERENCED_REGISTER, REGISTER_TEMP, REGISTER_WORKING, 0);
       } // end auto variable
      break;
     } // end if (not an array)

// assigning to an element of an array is a little more complicated.
// first we save scode position so that we can come back and parse the array reference later
    save_scode_pos = cstate.scode_pos;
// now we skip past the array reference:
    while(TRUE)
    {
     if (!read_next(&ctoken_operator)
    || (ctoken_operator.type == CTOKEN_TYPE_PUNCTUATION
      && ctoken_operator.subtype == CTOKEN_SUBTYPE_SEMICOLON))
     {
      cstate.scode_pos = save_scode_pos; // this is where the error really occurred
      return comp_error(CERR_EXPECTED_ASSIGNMENT_OPERATOR, &ctoken_operator);
     }
     if (ctoken_operator.type == CTOKEN_TYPE_OPERATOR_ASSIGN)
      break;
    };
// parse the following expression:
    retval = parse_expression(REGISTER_WORKING, REGISTER_TEMP, 1, -1, 0, BIND_BASIC);
    if (retval == 0)
     return 0;

// now we go back and put the target address in REGISTER_WORKING
// (note that parse_array_reference deals with pointers, and obtains the actual address of the array):
     save_scode_pos2 = cstate.scode_pos;
     cstate.scode_pos = save_scode_pos;
     if (!parse_array_reference(REGISTER_TEMP, REGISTER_WORKING, 1, ctoken.identifier_index, 1)) // ,1 means the address of the element is left in REGISTER_WORKING
      return 0;
// This is not efficient; it would be more efficient for parse_array_reference to set a setra intercode entry that puts REGISTER_WORKING directly into the array address
// Do this later.

// value to be assigned is in working register. Address of target is in temp register
      if (ctoken_operator.subtype != CTOKEN_SUBTYPE_EQ)
      {
// must be +=, -= etc
// need to save REGISTER_TEMP:
       add_intercode(IC_PUSH_REGISTER, REGISTER_TEMP, REGISTER_STACK, 0);
// dereference the value left there from before:
       add_intercode(IC_DEREFERENCE_REGISTER_TO_REGISTER, REGISTER_TEMP, REGISTER_TEMP, 0);
// perform operation:
      if (!assignment_operation(ctoken_operator.subtype, REGISTER_WORKING, REGISTER_TEMP, REGISTER_WORKING)) // this leaves the result in REGISTER_TEMP
       return 0;
// retrieve variable address into REGISTER_TEMP:
       add_intercode(IC_POP_TO_REGISTER, REGISTER_TEMP, REGISTER_STACK, 0);
      }
// Copy the value to the address referred to by REGISTER_TEMP
      add_intercode(IC_COPY_REGISTER_TO_DEREFERENCED_REGISTER, REGISTER_TEMP, REGISTER_WORKING, 0);
// check for end of statement:
     cstate.scode_pos = save_scode_pos2;
     if (!comp_next(CM_END_ASSIGNMENT_STATEMENT, final_closing_bracket))
      return 0;
     break; // end CTOKEN_TYPE_IDENTIFIER_USER_VARIABLE

    case CTOKEN_TYPE_OPERATOR_ARITHMETIC:
// the only arithmetic operator accepted here is *, which can be used in assignment instructions (to assign indirectly)
     if (ctoken.subtype != CTOKEN_SUBTYPE_MUL)
      return comp_error(CERR_SYNTAX_AT_STATEMENT_START, &ctoken);
// read in the expression following the *
// this will leave the target address in REGISTER_WORKING
     if (!parse_expression(REGISTER_WORKING, REGISTER_TEMP, 0, -1, 1, BIND_BASIC)) // this is probably the only place the last parameter (accept_assignment_at_end) is 1
      return 0; // comp_error(CERR_SYNTAX, &ctoken);
// read the assignment operator into ctoken_operator:
// note that ctoken_operator will be used later in the call to assignment_operation
     if (!read_next(&ctoken_operator))
      return comp_error(CERR_READ_FAIL, &ctoken_operator);
// now parse the following expression, leaving the result in REGISTER_TEMP (and saving REGISTER_WORKING if needed):
     if (!parse_expression(REGISTER_TEMP, REGISTER_WORKING, 1, -1, 0, BIND_BASIC))
      return 0; // comp_error(CERR_SYNTAX, &ctoken);
// if the operator is =, assign the result to the dereferenced working register:
     if (ctoken_operator.subtype == CTOKEN_SUBTYPE_EQ)
     {
      add_intercode(IC_COPY_REGISTER_TO_DEREFERENCED_REGISTER, REGISTER_WORKING, REGISTER_TEMP, 0);
     }
      else
      {
// This is a little more complicated.
// First, push the working register, because we'll need the target address later:
       add_intercode(IC_PUSH_REGISTER, REGISTER_WORKING, REGISTER_STACK, 0);
// Now dereference the working register to get the value it points to:
       add_intercode(IC_DEREFERENCE_REGISTER_TO_REGISTER, REGISTER_WORKING, REGISTER_WORKING, 0);
// Now perform the operation (assignment_operation() confirms that the ctoken subtype is a valid operator):
       if (!assignment_operation(ctoken_operator.subtype, REGISTER_WORKING, REGISTER_TEMP, REGISTER_WORKING)) // this leaves the result in REGISTER_WORKING
        return 0;
// Now we need to retrieve the address that used to be in the working register:
       add_intercode(IC_POP_TO_REGISTER, REGISTER_TEMP, REGISTER_STACK, 0);
// and assign the result of assignment_operation to the memory location it points to:
       add_intercode(IC_COPY_REGISTER_TO_DEREFERENCED_REGISTER, REGISTER_TEMP, REGISTER_WORKING, 0);
      }
// finally, expect a semicolon:
     if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_SEMICOLON))
      return comp_error(CERR_EXPECTED_SEMICOLON, &ctoken);
     break; // end CTOKEN_TYPE_OPERATOR_ARITHMETIC

    case CTOKEN_TYPE_IDENTIFIER_NEW: // is probably intended to be a label definition (although it could be something else, mistyped)
    case CTOKEN_TYPE_LABEL_UNDEFINED: // a label that's been used in a goto but not yet defined
    case CTOKEN_TYPE_ASM_GENERIC_UNDEFINED:
     if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_COLON))
      return comp_error(CERR_SYNTAX_AT_STATEMENT_START, &ctoken);
     identifier[ctoken.identifier_index].type = ID_USER_LABEL;
     identifier[ctoken.identifier_index].scope = cstate.scope;
     identifier[ctoken.identifier_index].process_scope = cstate.process_scope;
     identifier[ctoken.identifier_index].aspace_scope = cstate.cprocess->corresponding_aspace;
     add_intercode(IC_LABEL_DEFINITION, ctoken.identifier_index, 0, 0);
     break; // end CTOKEN_TYPE_IDENTIFIER_NEW and CTOKEN_TYPE_LABEL_UNDEFINED

    case CTOKEN_TYPE_LABEL:
     return comp_error(CERR_LABEL_ALREADY_DEFINED, &ctoken);

    default:
     return comp_error(CERR_SYNTAX_AT_STATEMENT_START, &ctoken);

   }

comp_statement_success:
 cstate.recursion_level --;
 return retval;

} // end comp_statement()

int skip_to_after_closing_bracket(void)
{

 int bracket_nest = 1;
 struct ctokenstruct ctoken;

 do
 {
  if (!read_next(&ctoken))
   return comp_error(CERR_READ_FAIL, &ctoken);
  if (ctoken.type == CTOKEN_TYPE_PUNCTUATION)
  {
   if (ctoken.subtype == CTOKEN_SUBTYPE_BRACKET_CLOSE)
    bracket_nest --;
   if (ctoken.subtype == CTOKEN_SUBTYPE_BRACKET_OPEN)
    bracket_nest ++;
  }

 } while (bracket_nest > 0);

 return 1;

}




int comp_switch(void)
{

 s16b case_value [SWITCH_CASES]; // stores the value for this case (i.e. the number after "case", which must be a constant).
 int case_expoint [SWITCH_CASES]; // stores the exit point that leads to the code for this case. is -1 if this case not in use

 int i;

 for (i = 0; i < SWITCH_CASES; i ++)
 {
  case_value [i] = -1; // this doesn't really matter but can't hurt
  case_expoint [i] = -1;
 }

 int case_index = 0; // this is which entry in the case array we're up to. It doesn't relate directly to the actual value of a case.
 int minimum_case = (1<<17); // this just needs to be high enough that any s16b will be less than it
 int maximum_case = 0 - (1<<17);
 s16b case_label_value = 0;
 int retval;
 int is_default = 0; // is set to 1 if there is an express default case

 struct ctokenstruct ctoken;
 strcpy(ctoken.name, "(undefined)");

// we will need a special exit point for break (including default if no default set) and default:
 int switch_exit_point = allocate_exit_point(EXPOINT_TYPE_SWITCH); // EXPOINT_TYPE_SWITCH means that break will work but continue won't
 if (switch_exit_point == -1)
  return comp_error(CERR_EXIT_POINT_FAIL, NULL);
// true is default, false is break

// we'll also need an exit point for the start of the jump table (only true is used):
 int jump_table_exit_point = allocate_exit_point(EXPOINT_TYPE_BASIC);
 if (jump_table_exit_point == -1)
  return comp_error(CERR_EXIT_POINT_FAIL, NULL);

 if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACKET_OPEN))
  return comp_error(CERR_EXPECTED_BRACKET_OPEN, NULL);

 if (!parse_expression(REGISTER_WORKING, REGISTER_TEMP, 0, -1, 0, 0))
  return 0;

 if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACKET_CLOSE))
  return comp_error(CERR_EXPECTED_BRACKET_CLOSE, NULL);

 if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACE_OPEN))
  return comp_error(CERR_EXPECTED_BRACE_OPEN, NULL);

// add intercode for bounds check and index modification here
// remember that the case index (not adjusted for minimum case) is in REGISTER_WORKING

// first put minimum into spare register.
// we need to get the intercode address of this instruction so that we can change it later:
 int min_value_intercode_address = cstate.icode_pos;
 add_intercode(IC_NUMBER_TO_REGISTER, REGISTER_TEMP, 0, 0); // the number will be set later
// compare it against the value - the minimum should not be greater than the value (the comparison leaves the result in the first register operand):
 add_intercode(IC_COMPARE_REGS_GR, REGISTER_TEMP, REGISTER_WORKING, 0);
// if comparison true, jump straight to default (or end if no default) - the comparison is set up so that true results in an exit because the default is the true exit point for switch_exit_point
 add_intercode(IC_IFTRUE_JUMP_TO_EXIT_POINT, REGISTER_TEMP, switch_exit_point, 0);

// now do the same for max
 int max_value_intercode_address = cstate.icode_pos;
 add_intercode(IC_NUMBER_TO_REGISTER, REGISTER_TEMP, 0, 0); // the number will be set later
// compare it against the value - the maximum should not be less than the value (the comparison leaves the result in the first register operand):
 add_intercode(IC_COMPARE_REGS_LS, REGISTER_TEMP, REGISTER_WORKING, 0);
// if comparison true, jump straight to default (or end if no default) - the comparison is set up so that true results in an exit because the default is the true exit point for switch_exit_point
 add_intercode(IC_IFTRUE_JUMP_TO_EXIT_POINT, REGISTER_TEMP, switch_exit_point, 0);

// now we know that the value (still stored in REGISTER_WORKING) is in bounds.
 int switch_jump_intercode_address = cstate.icode_pos; // operand [1] of the switch_jump intercode entry needs to be modified with an offset, once we know the minimum case value (see later in this function)
 add_intercode(IC_SWITCH_JUMP, jump_table_exit_point, 0, 0);

// now we read the cases.
// the following code sets up the case_value and case_expoint arrays as *unsorted* lists of cases (they'll be in the order of appearance).
 while(TRUE)
 {
// first check for end of switch:

  if (check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACE_CLOSE))
   break; // finished reading cases

// this outer loop accepts only case label, default or closing brace:
  if (!read_next(&ctoken))
   return comp_error(CERR_READ_FAIL, &ctoken);

  if (ctoken.type == CTOKEN_TYPE_IDENTIFIER_KEYWORD)
  {
   switch (ctoken.identifier_index)
   {
    case KEYWORD_CASE:
// first make sure we have enough space for this case:
     if (case_index >= SWITCH_CASES)
      return comp_error(CERR_TOO_MANY_CASES_IN_SWITCH, &ctoken);
// now parse the case's value:
     retval = read_literal_numbers(&ctoken);
     if (retval != 2) // 2 means an expression consisting solely of (or reducible to) a literal number
      return comp_error(CERR_CASE_VALUE_NOT_CONSTANT, &ctoken);
     case_label_value = ctoken.number_value;
// confirm colon present:
     if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_COLON))
      return comp_error(CERR_EXPECTED_COLON_AFTER_CASE, &ctoken);
// first check for duplicate case values:
     for (i = 0; i < case_index; i ++)
     {
      if (case_value [i] == case_label_value)
       return comp_error(CERR_DUPLICATE_CASE, &ctoken);
     }
// now may need to reset min/max:
     if (case_label_value < minimum_case)
      minimum_case = case_label_value;
     if (case_label_value > maximum_case)
      maximum_case = case_label_value;
// now set up the case value array:
     case_value [case_index] = case_label_value;
// and the exit point for this case:
     case_expoint [case_index] = allocate_exit_point(EXPOINT_TYPE_BASIC); // this exit point is not EX_POINT_TYPE_SWITCH (only switch_exit_point is) because it is only used for case jumps, not break jumps
     if (case_expoint [case_index] == -1)
      return comp_error(CERR_EXIT_POINT_FAIL, &ctoken);
     add_intercode(IC_EXIT_POINT_TRUE, case_expoint [case_index], 0, 0);
// now compile statements until reaching break or a fall-through case:
     while(TRUE)
     {
// before we start to read the statements following the case label, we check for: fall-through case label, default label, and closing brace:
      if (!peek_next(&ctoken))
       comp_error(CERR_READ_FAIL, &ctoken);
      if (ctoken.type == CTOKEN_TYPE_IDENTIFIER_KEYWORD)
      {
       if (ctoken.identifier_index == KEYWORD_CASE
        || ctoken.identifier_index == KEYWORD_DEFAULT)
        break;
      }
      if (ctoken.type == CTOKEN_TYPE_PUNCTUATION
       && ctoken.subtype == CTOKEN_SUBTYPE_BRACE_CLOSE)
       break; // the closing brace hasn't been passed (due to use of peek_next rather than check_next) so it will be picked up by the check_next in the main switch loop
      if (!comp_statement(switch_exit_point, 0))
       return 0;
// if comp_statement finds a break, it will set it to jump to switch_exit_point's false point (which will be after the end of the switch altogether)
// TO DO: at present, code after an unconditional break will continue to be compiled even though it can't be reached. Is this okay? (actually I suppose it could be reached by goto)
     };
// increment the case array counter:
     case_index++;
     break;
    case KEYWORD_DEFAULT:
// confirm colon present:
     if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_COLON))
      return comp_error(CERR_EXPECTED_COLON_AFTER_CASE, &ctoken);
// make sure default not already used:
     if (is_default)
      return comp_error(CERR_DUPLICATE_DEFAULT, &ctoken);
     is_default = 1;
// and the exit point for default, which is the true exit point for the main switch:
     add_intercode(IC_EXIT_POINT_TRUE, switch_exit_point, 0, 0);
// now compile statements until reaching break or a fall-through case:
     while(TRUE)
     {
// before we start to read the statements following the case label, we check for: fall-through case label, default label, and closing brace:
      if (!peek_next(&ctoken))
       comp_error(CERR_READ_FAIL, &ctoken);
      if (ctoken.type == CTOKEN_TYPE_IDENTIFIER_KEYWORD)
      {
       if (ctoken.identifier_index == KEYWORD_CASE
        || ctoken.identifier_index == KEYWORD_DEFAULT)
        break;
      }
      if (ctoken.type == CTOKEN_TYPE_PUNCTUATION
       && ctoken.subtype == CTOKEN_SUBTYPE_BRACE_CLOSE)
       break; // the closing brace hasn't been passed (due to use of peek_next rather than check_next) so it will be picked up by the check_next in the main switch loop
      if (!comp_statement(switch_exit_point, 0))
       return 0;
// if comp_statement finds a break, it will set it to jump to switch_exit_point's false point (which will be after the end of the switch altogether)
// TO DO: at present, code after an unconditional break will continue to be compiled even though it can't be reached. Is this okay? (actually I suppose it could be reached by goto)
     };
     break;
    default:
     return comp_error(CERR_EXPECTED_CASE_OR_DEFAULT, &ctoken);
   } // end switch ctoken.identifier_index keywords

  }
   else
    return comp_error(CERR_EXPECTED_CASE_OR_DEFAULT, NULL);

 };

// now we have:
//  - case_value holding a list of case label values
//  - case_expoint holding a list of exit points (one for each case_value)
//  - minimum_case and maximum_case set up
//  - case_index holding the # of cases (not including any gaps)

// need to:
//  - make sure there is at least one case
//  - test minimum_case and maximum_case to see if valid (gap not too large)
//  - build jump table
//  - go back and fix up bits before case code, including jump targets

//  - assign true point for switch_exit_point to default (or after switch if no default)
//  - assign false point for switch_exit_point to after switch

 if (case_index == 0)
  return comp_error(CERR_SWITCH_HAS_NO_CASES, &ctoken); // not sure this really needs to be an error but it's pointless so it can't hurt

 int total_case_space = maximum_case - minimum_case; // this is the total number of cases, including gaps (but not including default)

 if (total_case_space >= SWITCH_MAX_SIZE)
  return comp_error(CERR_SWITCH_TOO_LARGE, &ctoken);

 int index_of_case_i; // this is where the current actual case value (i) is in the case_value and case_expoint arrays

// set the exit point for the start of the jump table:
 add_intercode(IC_EXIT_POINT_TRUE, jump_table_exit_point, 0, 0);

 for (i = minimum_case; i < maximum_case + 1; i ++)
 {
// need to find where the value i is in the case_value array (as the cases may be out of order or not all present)
  index_of_case_i = find_case_index(i, case_index, case_value);
  if (index_of_case_i == -1) // not found - use default
  {
   if (is_default)
    add_intercode(IC_JUMP_EXIT_POINT_TRUE, switch_exit_point, 0, 0);
     else
      add_intercode(IC_JUMP_EXIT_POINT_FALSE, switch_exit_point, 0, 0);
   continue;
  }
  add_intercode(IC_JUMP_EXIT_POINT_TRUE, case_expoint [index_of_case_i], 0, 0);
 }

// finished the jump table!

 add_intercode(IC_EXIT_POINT_FALSE, switch_exit_point, 0, 0);
 if (!is_default)
  add_intercode(IC_EXIT_POINT_TRUE, switch_exit_point, 0, 0);

// now just need to fix up the stuff at the start:
// expoint[jump_table_exit_point].offset = minimum_case * -2; // this is added to the address of the exit point in c_inter.c. It is -2 because each value takes up two bcode spaces (1 for jumpn, 1 for operand)

 modify_intercode_operand(min_value_intercode_address, 1, minimum_case);
 modify_intercode_operand(max_value_intercode_address, 1, maximum_case);
 modify_intercode_operand(switch_jump_intercode_address, 1, minimum_case * -2);

// NOTE: shouldn't need to deal specially with fall-throughs - they should just work (as the cases can be in any order; it's only the jump table entries that must be in order)

 return 1;

}

// finds value in case_value; returns index in case_value array
// returns -1 if not found
int find_case_index(int value, int total_values, s16b case_value [SWITCH_CASES])
{

 int i;

 for (i = 0; i < total_values; i ++)
 {
  if (case_value [i] == value)
   return i;
 }

// not found
 return -1;

}




// call this function when the keyword "interface" begins a statement.
// this function checks for clashes with a previous interface definition (which is an error)
// returns 1 on success, 0 on error
int parse_interface_definition(void)
{

 struct ctokenstruct ctoken;
 strcpy(ctoken.name, "(empty)");
 int found_comma;

 if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACE_OPEN))
  return comp_error(CERR_EXPECTED_BRACE_OPEN, &ctoken);

 if (cstate.cprocess->iface.defined)
  return comp_error(CERR_INTERFACE_ALREADY_DEFINED, &ctoken); // this process already has an interface definition

 cstate.cprocess->iface.defined = 1;

 struct interfacestruct* iface = &cstate.cprocess->iface;

// read program type
 if (!get_constant_initialiser(&iface->type))
  return 0;

// need to verify the program type as this affects how the rest of the interface definition is dealt with
 if (iface->type < 0 || iface->type >= PROGRAM_TYPES)
  return comp_error(CERR_UNRECOGNISED_PROGRAM_TYPE, &ctoken);

 switch(iface->type)
 {
  case PROGRAM_TYPE_PROCESS:
// need to check for comma after type
   if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_COMMA))
    return comp_error(CERR_EXPECTED_COMMA_AFTER_VALUE, &ctoken);
// read size
   if (!get_constant_initialiser(&iface->shape))
    return 0;
   if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_COMMA))
    return comp_error(CERR_EXPECTED_COMMA_AFTER_VALUE, &ctoken);
// read shape
   if (!get_constant_initialiser(&iface->size))
    return 0;
   if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_COMMA))
    return comp_error(CERR_EXPECTED_COMMA_AFTER_VALUE, &ctoken);
// read base_vertex
   if (!get_constant_initialiser(&iface->base_vertex))
    return 0;
   break;
  case PROGRAM_TYPE_SYSTEM:
// need to check for comma after type
   if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_COMMA))
    return comp_error(CERR_EXPECTED_COMMA_AFTER_VALUE, &ctoken);
   if (!parse_system_program_interface(iface))
    return 0;
   break;

// NOTE: don't read comma after final header value (as the header may be followed by a closing brace if there are no methods for some reason)
//  - although I think parse_system_program_interface does expect a comma here.
 }

  if (check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACE_CLOSE))
   return 1; // finished, although without methods

  if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_COMMA))
   return comp_error(CERR_EXPECTED_COMMA_AFTER_VALUE, &ctoken);

// if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_COMMA))
//  return comp_error(CERR_EXPECTED_COMMA_AFTER_VALUE, &ctoken);

 if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACE_OPEN))
  return comp_error(CERR_EXPECTED_BRACE_OPEN, &ctoken);

 int m = 0;
 int n;

// now go through method definitions one by one, finishing when a closing brace found
 while(TRUE)
 {
  if (m >= METHODS)
   return comp_error(CERR_IFACE_TOO_MANY_METHODS, &ctoken);
  if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACE_OPEN))
//   return comp_error(CERR_EXPECTED_BRACE_OPEN, &ctoken);
  {
    /* No opening brace, may be labelled method LABEL : { DATA, DATA... } */
    read_next(&ctoken);
    if (ctoken.type != CTOKEN_TYPE_IDENTIFIER_NEW)
     return comp_error(CERR_EXPECTED_OPEN_BRACE_OR_METHOD_LABEL, &ctoken);
    /* Got a label, look for colon TO DO this is not the right CERR_... constant. */
    if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_COLON))
     return comp_error(CERR_EXPECTED_COLON_AFTER_METHOD_LABEL, &ctoken);
    /* Now try again for the opening brace */
    if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACE_OPEN))
     return comp_error(CERR_EXPECTED_BRACE_OPEN, &ctoken);
    /* OK now assign it to an enum */
    ctoken.type = CTOKEN_TYPE_ENUM;
    ctoken.number_value = m;
    struct identifierstruct* id = &identifier[ctoken.identifier_index];
    id->type = ID_ENUM;
    id->enum_value = m;

    id->scope = SCOPE_GLOBAL;
    id->process_scope = cstate.process_scope;
    id->aspace_scope = cstate.cprocess->corresponding_aspace;
    id->nspace_scope = -1;
    id->asm_or_compiler = IDENTIFIER_ASM_OR_C; // accessible from asm
   } // method label patch by Peter Hull


  n = 0;
// now go through method data definitions one by one, finishing when a closing brace found
  while(TRUE)
  {
   if (n >= INTERFACE_METHOD_DATA)
    return comp_error(CERR_IFACE_METHOD_TOO_MUCH_DATA, &ctoken);
// stop if } found
   if (check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACE_CLOSE))
    break;
// read value, expecting a number constant
   if (!get_constant_initialiser(&iface->method [m] [n]))
    return 0;
   found_comma = 0;
   if (check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_COMMA))
    found_comma = 1;

// stop if } found
   if (check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACE_CLOSE))
    break;
// if no }, expect a comma
   if (!found_comma)
    return comp_error(CERR_EXPECTED_COMMA_AFTER_VALUE, &ctoken);
   n++;
  };

// accept a trailing comma
  found_comma = 0;
  if (check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_COMMA))
   found_comma = 1;

// check for closing brace indicating end of all method definitions
  if (check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACE_CLOSE))
   break;
// no closing brace found, so expect a comma followed by additional definitions
  if (!found_comma)
   return comp_error(CERR_EXPECTED_COMMA_AFTER_VALUE, &ctoken);
  m++;
 };

// finished now, so expect a closing brace
 if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACE_CLOSE))
  return comp_error(CERR_EXPECTED_BRACE_CLOSE, &ctoken);


 return 1;

}



/*
Call this function just after reading PROGRAM_TYPE_SYSTEM in the type field of an interface definition.
It will fill in a world_preinit struct with details of the system program's world initialisation values.
This struct will later be used to generate bcode.

Note that the values read in are not bounds-checked (TO DO: think about adding this, maybe with compiler warnings)
Bounds-checking is done when the system program's bcode is used to generate a world_preinit struct

** remember - any changes in this function may need to be reflected in intercode_process_header() in c_inter.c and derive_pre_init_from_system_program() in s_setup.c

returns 1 success/0 fail
*/
int parse_system_program_interface(struct interfacestruct* iface)
{

 int i;
 struct ctokenstruct ctoken;
 strcpy(ctoken.name, "(empty)");



// set some default values (note that any PI_VALUE arrays are set to defaults by calls to parse_preinit_array_field()
 iface->system_program_preinit.allow_player_clients = 0;
 iface->system_program_preinit.player_operator = -1;
 iface->system_program_preinit.allow_user_observer = 0;

 for (i = 0; i < PLAYERS; i ++)
 {
  iface->system_program_preinit.may_change_client_template [i] = 1;
  iface->system_program_preinit.may_change_proc_templates [i] = 1;
 }

// now we read in each of the PI_VALUES init values:
 // PI_VALUES is # of elements in array. 1 is default first value (PI_OPTION). -1 is default remaining values (-1 will be fixed in derive_program_properties_from_bcode(), which will set it to default values). final 1 is expect trailing comma
 if (!parse_preinit_array_field(iface->system_program_preinit.players, PI_VALUES, 1, -1, 1))
  return 0;
 if (!parse_preinit_array_field(iface->system_program_preinit.game_turns, PI_VALUES, 1, -1, 1))
  return 0;
 if (!parse_preinit_array_field(iface->system_program_preinit.game_minutes_each_turn, PI_VALUES, 1, -1, 1))
  return 0;
 if (!parse_preinit_array_field(iface->system_program_preinit.procs_per_team, PI_VALUES, 1, -1, 1))
  return 0;
// if (!parse_preinit_array_field(iface->system_program_preinit.gen_limit, PI_VALUES, 1, -1, 1))
  //return 0;
 if (!parse_preinit_array_field(iface->system_program_preinit.packets_per_team, PI_VALUES, 1, -1, 1))
  return 0;
 if (!parse_preinit_array_field(iface->system_program_preinit.w_block, PI_VALUES, 1, -1, 1))
  return 0;
 if (!parse_preinit_array_field(iface->system_program_preinit.h_block, PI_VALUES, 1, -1, 1))
  return 0;
// parse_preinit_array_field() has already checked for a comma

// now we read in some more values:
 if (!get_constant_initialiser(&iface->system_program_preinit.allow_player_clients))
  return 0;
 if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_COMMA))
  return comp_error(CERR_EXPECTED_COMMA_AFTER_VALUE, &ctoken);

 if (!get_constant_initialiser(&iface->system_program_preinit.player_operator))
  return 0;
 if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_COMMA))
  return comp_error(CERR_EXPECTED_COMMA_AFTER_VALUE, &ctoken);

 if (!get_constant_initialiser(&iface->system_program_preinit.allow_user_observer))
  return 0;
 if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_COMMA))
  return comp_error(CERR_EXPECTED_COMMA_AFTER_VALUE, &ctoken);

// now set some values particular to individual players.
// these values are in the form {player1.value1, player2.value1}, {player1.value2 etc.,
// they could be in the form {player1 values}, {player2 values}, etc. but that would require all players to always be expressly initialised
 if (!parse_preinit_array_field(iface->system_program_preinit.may_change_client_template, PLAYERS, 1, 1, 1))
  return 0;
 if (!parse_preinit_array_field(iface->system_program_preinit.may_change_proc_templates, PLAYERS, 1, 1, 0)) // note final zero means trailing comma is not checked (this is checked after this function returns)
  return 0;

// remember - any changes in this function should be reflected in intercode_process_header() in c_inter.c

// finished reading things specific to system programs - now return so we can read in methods:
 return 1;

}

/*
pass a pointer to a preinit [elements] array and it will read in an initialiser in the form:
 {0, 1, 2, 3}
doesn't check trailing comma after }
doesn't require all (or any) values to be set. any unspecified values are left with defaults set by parse_system_program_interface()
returns 1/0 success/fail
*/
int parse_preinit_array_field(int* preinit_array, int elements, int default_first_value, int default_other_values, int check_trailing_comma)
{

 int i;

 preinit_array [0] = default_first_value;
// note that this loop is for i = 1
 for (i = 1; i < elements; i ++)
 {
  preinit_array [i] = default_other_values;
 }

 struct ctokenstruct ctoken;

 strcpy(ctoken.name, "(empty)");

 if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACE_OPEN))
  return comp_error(CERR_EXPECTED_BRACE_OPEN, &ctoken);

 for (i = 0; i < elements; i ++)
 {

  if (check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACE_CLOSE))
   break;

  if (!get_constant_initialiser(&preinit_array [i]))
   return 0;

// value must be followed by a closed brace or comma
  if (check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACE_CLOSE))
   break;

// last value must be followed by closed brace, so this function should have already returned
  if (i == elements - 1)
   return comp_error(CERR_EXPECTED_BRACE_CLOSE, &ctoken);

// still reading values - expect them to be separated by commas
  if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_COMMA))
   return comp_error(CERR_EXPECTED_COMMA_AFTER_VALUE, &ctoken);

 }
// shouldn't leave loop except by break statements, due to if (i == elements - 1) check

 if (check_trailing_comma
  && !check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_COMMA))
  return comp_error(CERR_EXPECTED_COMMA_AFTER_VALUE, &ctoken);

 return 1; // values will be left at default

}


/*
Call this function from CM_BASIC when the keyword "process" is found.
Detects whether it's a prototype or a definition.
Calls comp_next recursively
Can itself be recursive, as processes may be nested

returns 1 on success, 0 on failure
*/
int comp_process(void)
{

 struct ctokenstruct ctoken;

// first we save the scode position so that we can read in the process name a second time in asm mode, to generate an aspace identifier
 int save_scode_pos = cstate.scode_pos;

// now we check whether this is a definition or a prototype.
// read in the name of the process:
 if (!read_next(&ctoken))
  return comp_error(CERR_READ_FAIL, &ctoken);

 int p_index;

 if (ctoken.type == CTOKEN_TYPE_IDENTIFIER_NEW
	 || ctoken.type == CTOKEN_TYPE_ASM_GENERIC_UNDEFINED) // process not previously prototyped
 {
// must be a prototype.
// first, find an empty entry in the process list:
   for (p_index = 0; p_index < PROCESS_DEFS; p_index ++)
   {
    if (process_def[p_index].prototyped == 0)
     break;
   }
   if (p_index == PROCESS_DEFS)
    return comp_error(CERR_TOO_MANY_PROCESSES, &ctoken);
// now set up the identifier and the process_def:
   identifier[ctoken.identifier_index].process_scope = cstate.process_scope; // this is the parent process scope
   identifier[ctoken.identifier_index].scope = SCOPE_GLOBAL;
   identifier[ctoken.identifier_index].type = ID_PROCESS;
   identifier[ctoken.identifier_index].process_index = p_index;
   identifier[ctoken.identifier_index].asm_or_compiler = IDENTIFIER_C_ONLY; // can't refer to a process in asm. Use the corresponding aspace identifier instead (see below)
   identifier[ctoken.identifier_index].aspace_scope = cstate.cprocess->corresponding_aspace;
   process_def[p_index].prototyped = 1;
   process_def[p_index].id_index = ctoken.identifier_index;
   process_def[p_index].parent = cstate.process_scope; // need to verify this when process is defined.
// now we need to create a special "self" identifier just for use within this process:
   struct ctokenstruct self_ctoken;
   strcpy(self_ctoken.name, "self");
   if (!new_c_identifier(&self_ctoken, ID_PROCESS))
    return comp_error(CERR_NEW_IDENTIFIER_FAIL, &self_ctoken);
   identifier[self_ctoken.identifier_index].asm_or_compiler = IDENTIFIER_C_ONLY;
   identifier[self_ctoken.identifier_index].process_scope = p_index;
// Unfortunately it's difficult to set the aspace scope of this self ctoken correctly. I'll deal with this later.
   identifier[self_ctoken.identifier_index].scope = SCOPE_GLOBAL;
   identifier[self_ctoken.identifier_index].process_index = p_index;
// now check for ;
   if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_SEMICOLON))
    return comp_error(CERR_EXPECTED_SEMICOLON_AFTER_PROCESS_PROTOTYPE, &ctoken);
   return 1;
 }

 if (ctoken.type == CTOKEN_TYPE_PROCESS  // process previously prototyped implicitly
  && process_def[identifier[ctoken.identifier_index].process_index].defined == 0
  && process_def[identifier[ctoken.identifier_index].process_index].prototyped == 0)
 {
// should be a prototype of a process that has been implicitly declared previously.
   p_index = identifier[ctoken.identifier_index].process_index;
// we should be able to assume that process_scope matches, or read_next would not have identified it as a process ctoken.
// should also be able to assume that identifier scope, type and process_index have been set properly.
//   identifier[ctoken.identifier_index].scope = SCOPE_GLOBAL;
//   identifier[ctoken.identifier_index].type = ID_PROCESS;
//   identifier[ctoken.identifier_index].process_index = p_index;
   process_def[p_index].prototyped = 1;
//   process_def[p_index].id_index = ctoken.identifier_index;
//   process_def[p_index].parent = cstate.process_scope; // need to verify this when process is defined.
   if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_SEMICOLON))
    return comp_error(CERR_EXPECTED_SEMICOLON_AFTER_PROCESS_PROTOTYPE, &ctoken);
   return 1;
 }

/*
// it could have been implicitly declared by an out-of-scope reference, in which case it may not have been prototyped:
//  (actually I don't think this could happen, as out-of-scope references have not been implemented in the compiler)
 if (!process_def[p_index].prototyped)
 {
  fprintf(stdout, "\nprocess %i prototyped %i defined %i parent %i", p_index, process_def[p_index].prototyped, process_def[p_index].defined, process_def[p_index].parent);
  return comp_error(CERR_PROCESS_NOT_PROTOTYPED, &ctoken);
// This maybe could be accepted, but I'll call it an error for now.
 }*/


// not a prototype, so confirm that it is a definition:
 if (ctoken.type != CTOKEN_TYPE_PROCESS) // should have been prototyped previously
  return comp_error(CERR_PROCESS_NAME_INVALID, &ctoken);

 p_index = identifier[ctoken.identifier_index].process_index;
 if (process_def[p_index].defined)
 {
  return comp_error(CERR_PROCESS_ALREADY_DEFINED, &ctoken);
 }

// enter new process_scope
 cstate.process_scope = p_index;
 cstate.cprocess = &process_def[cstate.process_scope];
 process_def[p_index].defined = 1;
// Also need to set up a corresponding address space in case there is inline asm in the process:
// First need to setup an identifier for the aspace. We start this by resetting scode_pos to just before the process name, and re-reading it in asm mode.
// - this produces a new untyped identifier that we can use in asm:
 cstate.scode_pos = save_scode_pos;
// tell the ctoken parser to pretend we're running the assembler
 cstate.running_asm = 1;
// also need to set aspace and nspace
 astate.current_aspace = process_def[cstate.cprocess->parent].corresponding_aspace;
 astate.current_nspace = -1;

 struct ctokenstruct aspace_ctoken;
 if (!read_next(&aspace_ctoken))
  return comp_error(CERR_READ_FAIL, &aspace_ctoken);
// first check whether this aspace has already been implicitly declared by an asm scope reference
 int aspace_index;
 if (aspace_ctoken.type == CTOKEN_TYPE_ASM_ASPACE)
 {
  aspace_index = identifier[aspace_ctoken.identifier_index].aspace_index;
  if (astate.aspace[aspace_index].defined)
   return comp_error(CERR_PROCESS_NAME_ALREADY_USED_IN_ASM, &aspace_ctoken); // is this the right error? I think so.
// error occurs in previous line
 }
  else
  {
// if not implicitly declared, must be an unused identifier:
   if (aspace_ctoken.type != CTOKEN_TYPE_IDENTIFIER_NEW
    && aspace_ctoken.type != CTOKEN_TYPE_ASM_GENERIC_UNDEFINED)
    return comp_error(CERR_PROCESS_NAME_ALREADY_USED_IN_ASM, &aspace_ctoken);
// now allocate an aspace to the identifier:
   if (!allocate_aspace(aspace_ctoken.identifier_index)) // allocate_aspace finds an unused aspace, allocates it and sets the identifier's aspace_index. It also creates a "self" identifier with scope of the aspace itself.
    return 0;
   aspace_index = identifier[aspace_ctoken.identifier_index].aspace_index;
  }
 process_def[p_index].corresponding_aspace = aspace_index;
 identifier[aspace_ctoken.identifier_index].process_scope = cstate.process_scope;
 identifier[aspace_ctoken.identifier_index].scope = SCOPE_GLOBAL; // not sure this means anything
 identifier[aspace_ctoken.identifier_index].type = ASM_ID_ASPACE;
// identifier[ctoken.identifier_index].aspace_index =  - this has already been set by allocate_aspace()
// should be able to assume that process_def[p_index].parent refers to a valid process, as p_index cannot be 0 - process 0 is the main file process, which is defined implicitly
 identifier[aspace_ctoken.identifier_index].aspace_scope = process_def[process_def[p_index].parent].corresponding_aspace;
 identifier[aspace_ctoken.identifier_index].nspace_scope = -1;
 identifier[aspace_ctoken.identifier_index].asm_or_compiler = IDENTIFIER_ASM_ONLY;
// add intercode to enter the aspace (doesn't do anything unless asm code is being generated. If inline asm is found within the process, current aspace is derived from the current process)
 add_intercode(IC_ASM_ENTER_ASPACE, aspace_index, 0, 0);
 astate.aspace [aspace_index].declared = 1; // may already be 1

 astate.aspace [aspace_index].defined = 1;
 astate.aspace [aspace_index].parent = identifier[aspace_ctoken.identifier_index].aspace_scope;
 astate.aspace [aspace_index].identifier_index = aspace_ctoken.identifier_index;
 astate.aspace [aspace_index].corresponding_process = p_index;

// finally, we set the aspace_scope for the namespace of the main cfunction for this aspace
 astate.nspace [process_def[p_index].cfunction[0].corresponding_nspace].aspace_scope = aspace_index;

// now go back to the compiler. don't need to update scode_pos as it will be where it was before the aspace token parsing
 cstate.running_asm = 0;

// the IC_PROCESS_START must come after the IC_ASM_ENTER_ASPACE because IC_PROCESS_START adds some asm code that needs to come after the aspace is entered
 add_intercode(IC_PROCESS_START, p_index, 0, 0);

// check for open brace:
 if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACE_OPEN))
  return comp_error(CERR_EXPECTED_BRACE_OPEN, &ctoken);

 int compiler_state;

 while(TRUE)
 {
  if (check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACE_CLOSE))
   break;
  compiler_state = comp_next(CM_BASIC, 0);
// comp_next will deal with matched pairs of open/closing braces itself, but should return here after each statement or block of code in CM_BASIC mode (i.e. outside of a function or similar) so that
//  the following check for a closing brace can catch the end of the process definition:
  if (compiler_state == 0)
   return 0; // error
  if (compiler_state == 2) // reached end of code while in CM_BASIC - this is an error when compiling a subprocess
   return comp_error(CERR_REACHED_END_IN_SUBPROCESS, &ctoken);
// otherwise compiler_state == 1, which means it's still compiling
 };
// finished compiling the process!
 add_intercode(IC_PROCESS_END, p_index, process_def[p_index].parent, 0);
 add_intercode(IC_ASM_END_ASPACE, 0, 0, 0);

 cstate.process_scope = process_def[p_index].parent;
 cstate.cprocess = &process_def[cstate.process_scope];
// if (cstate.process_scope == -1) should not be possible!
//  return comp_error
 if (cstate.process_scope == -1)
 {
  fprintf(stdout, "\nc_comp.c in comp_process(): cstate.process_scope is -1 after reverting to parent scope (should not be!)");
  error_call();
 }

 return 1;

}


/*
Call this when expecting a constant initialisation value
returns 1 on success, 0 on failure
leaves result in *value
*/
int get_constant_initialiser(int* value)
{

 struct ctokenstruct ctoken;
 strcpy(ctoken.name, "(empty)");

 if (!read_next_expression_value(&ctoken))
  return comp_error(CERR_READ_FAIL, &ctoken);
 if (ctoken.type != CTOKEN_TYPE_NUMBER)
  return comp_error(CERR_EXPECTED_NUMBER_CONSTANT, &ctoken);

 (*value) = ctoken.number_value;

 return 1;
}


/*
Call this to perform the operation part of an assignment operation, e.g. += -=
Doesn't deal with =, ++ or --
The three register parameters are the registers that will be passed to add_intercode,
 so they should be in the form of e.g. A = B - C

*/
int assignment_operation(int operator_subtype, int reg1, int reg2, int reg3)
{

 switch(operator_subtype)
 {
/*  case CTOKEN_SUBTYPE_EQ:
   add_intercode(IC_COPY_REGISTER2_TO_REGISTER1, reg1, reg2, 0);
   return 1;*/ // this doesn't really work
  case CTOKEN_SUBTYPE_PLUSEQ:
   add_intercode(IC_ADD_REG_TO_REG, reg1, reg2, reg3);
   return 1;
  case CTOKEN_SUBTYPE_MINUSEQ:
   add_intercode(IC_SUB_REG_FROM_REG, reg1, reg2, reg3);
   return 1;
  case CTOKEN_SUBTYPE_MULEQ:
   add_intercode(IC_MUL_REG_BY_REG, reg1, reg2, reg3);
   return 1;
  case CTOKEN_SUBTYPE_DIVEQ:
   add_intercode(IC_DIV_REG_BY_REG, reg1, reg2, reg3);
   return 1;
  case CTOKEN_SUBTYPE_MODEQ:
   add_intercode(IC_MOD_REG_BY_REG, reg1, reg2, reg3);
   return 1;
  case CTOKEN_SUBTYPE_BITWISE_AND_EQ:
   add_intercode(IC_AND_REG_REG, reg1, reg2, reg3);
   return 1;
  case CTOKEN_SUBTYPE_BITWISE_OR_EQ:
   add_intercode(IC_OR_REG_REG, reg1, reg2, reg3);
   return 1;
  case CTOKEN_SUBTYPE_BITWISE_XOR_EQ:
   add_intercode(IC_XOR_REG_REG, reg1, reg2, reg3);
   return 1;
  case CTOKEN_SUBTYPE_BITWISE_NOT_EQ:
   add_intercode(IC_NOT_REG_REG, reg1, reg2, reg3);
   return 1;

 }

 struct ctokenstruct error_ctoken;
 strcpy(error_ctoken.name, "(empty)");

 return comp_error(CERR_EXPECTED_ASSIGNMENT_OPERATOR, &error_ctoken);

}




// Call this when a new function is prototyped.
// Returns index of new function in cfunction array on success
// relies on cfunction[].identifier_index being filled in later (after the cfunction name has been read)
// On failure, returns -1
int new_cfunction(int return_type, int storage_class)
{

 int i;

 for (i = 0; i < CFUNCTIONS; i ++)
 {
  if (cstate.cprocess->cfunction[i].exists == 0)
   break;
 }

 if (i == CFUNCTIONS)
  return -1;

 cstate.cprocess->cfunction[i].exists = 1;
 cstate.cprocess->cfunction[i].prototyped = 1;
// cfunction[i].identifier_index = identifier_index;
 cstate.cprocess->cfunction[i].intercode_address = cstate.icode_pos;
 cstate.cprocess->cfunction[i].arguments = 0;
 cstate.cprocess->cfunction[i].return_type = return_type;
 cstate.cprocess->cfunction[i].storage_class = storage_class;
 cstate.cprocess->cfunction[i].size_on_stack = 0;

 return i;

}

// This function is called when int or void has been found at the start of a line in global scope
// It looks ahead to work out whether it's a cfunction prototype, a cfunction definition or (if int) a variable declaration
// returns 0 on error
// returns 1 if function prototype
// returns 2 if function definition
// returns 3 if variable declaration
int check_prototype_or_definition(void)
{
 int save_scode_pos = cstate.scode_pos;

 struct ctokenstruct ctoken;
 int name_ctoken_type;

// read past the name:
 if (!read_next(&ctoken))
  return comp_error(CERR_READ_FAIL, &ctoken);

 name_ctoken_type = ctoken.type;

 if (name_ctoken_type != CTOKEN_TYPE_IDENTIFIER_NEW
  && name_ctoken_type != CTOKEN_TYPE_IDENTIFIER_USER_CFUNCTION
  && name_ctoken_type != CTOKEN_TYPE_ASM_GENERIC_UNDEFINED)
 {
// 	fprintf(stdout, "\nctoken name (%s) ctoken.type %i process_scope %i scope %i aspace_scope %i nspace_scope %i", ctoken.name, ctoken.type, identifier[ctoken.identifier_index].process_scope, identifier[ctoken.identifier_index].scope,
//											identifier[ctoken.identifier_index].aspace_scope, identifier[ctoken.identifier_index].nspace_scope);
   return comp_error(CERR_IDENTIFIER_REUSED, &ctoken);
 }

// check what follows the name. Should be either brackets (for a cfunction) or a comma, semicolon or square brackets (for a variable)
  if (!read_next(&ctoken))
   return comp_error(CERR_READ_FAIL, &ctoken);
  if (ctoken.type == CTOKEN_TYPE_PUNCTUATION
   || ctoken.type == CTOKEN_TYPE_OPERATOR_ASSIGN)
  {
   if (ctoken.subtype == CTOKEN_SUBTYPE_COMMA
   || ctoken.subtype == CTOKEN_SUBTYPE_SEMICOLON
   || ctoken.subtype == CTOKEN_SUBTYPE_SQUARE_OPEN
   || ctoken.subtype == CTOKEN_SUBTYPE_EQ)
   {
    cstate.scode_pos = save_scode_pos;
    return 3; // int declaration (if the variable is actually being declared at type void, an error will occur after this return)
   }
   if (ctoken.subtype != CTOKEN_SUBTYPE_BRACKET_OPEN)
    return comp_error(CERR_EXPECTED_BRACKET_OPEN, &ctoken);
  }
   else
    return comp_error(CERR_INVALID_DECLARATION, &ctoken);

// now we know it's a cfunction, but not whether it's a prototype or definition.

 cstate.scode_pos = save_scode_pos;

 if (name_ctoken_type == CTOKEN_TYPE_IDENTIFIER_NEW
		|| name_ctoken_type == CTOKEN_TYPE_ASM_GENERIC_UNDEFINED)
  return 1; // prototype

 if (name_ctoken_type == CTOKEN_TYPE_IDENTIFIER_USER_CFUNCTION)
  return 2; // definition

 return comp_error(CERR_INVALID_DECLARATION, &ctoken);

}


// this function just checks to make sure that the type of a cfunction definition matches the type of its prototype
// it's called just after the type has been read, and after check_prototype_or_definition() has confirmed that it is a cfunction definition
// returns 0 on error
// returns 1 on success
int verify_cfunction_type(int type, int storage_class)
{

 int save_scode_pos = cstate.scode_pos;

 struct ctokenstruct ctoken;

// read the name:
 if (!read_next(&ctoken))
  return 0;
 if (ctoken.type != CTOKEN_TYPE_IDENTIFIER_USER_CFUNCTION)
   return 0;
// if (!cstate.cprocess->cfunction[identifier[ctoken.identifier_index].cfunction_index].prototyped)
//  return comp_error(CERR_IMPLICIT_CFUNCTION_NOT_PROTOTYPED, &ctoken);

 if (cstate.cprocess->cfunction[identifier[ctoken.identifier_index].cfunction_index].return_type != type)
  return 0;

 if (cstate.cprocess->cfunction[identifier[ctoken.identifier_index].cfunction_index].storage_class != storage_class)
  return 0;

 cstate.scode_pos = save_scode_pos;
 return 1;



}

enum
{
EXVAL_UNKNOWN, // found an unexpected character or end of file. Leave it to the main parser function to work out whether it's an error etc.
EXVAL_SUBEXPRESSION, // found brackets around a subexpression that can't be resolved into a simple value.
EXVAL_CFUNCTION_CALL, // found a cfunction call
EXVAL_NUMBER, // found a number than can be stored as a literal and used directly. Could be a series of numbers that can be resolved into a single number. (e.g. 1 + 2 + 3)
EXVAL_VARIABLE, // found a variable that can be used directly
};

// this is called just after the enum keyword is found
// Note that there is no such thing as an enumerated type yet.
// returns 1 on success, 0 on failure
int parse_enum_definition(void)
{

 struct ctokenstruct ctoken;
 struct ctokenstruct assign_ctoken;

 strcpy(ctoken.name, "(unknown)");
 strcpy(assign_ctoken.name, "(unknown)");

 if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACE_OPEN))
  return comp_error(CERR_EXPECTED_BRACE_OPEN, &ctoken);

 int count = 0;
 int found_comma;

 while(TRUE)
 {
  if (!read_next(&ctoken))
   return comp_error(CERR_READ_FAIL, &ctoken);

  if (ctoken.type != CTOKEN_TYPE_IDENTIFIER_NEW)
  {
   return comp_error(CERR_IDENTIFIER_ALREADY_USED, &ctoken);
  }

  if (check_next(CTOKEN_TYPE_OPERATOR_ASSIGN, CTOKEN_SUBTYPE_EQ))
  {
   if (!read_literal_numbers(&assign_ctoken))
    return comp_error(CERR_EXPECTED_NUMBER_CONSTANT, &assign_ctoken);
   count = assign_ctoken.number_value;
  }

  identifier[ctoken.identifier_index].type = ID_ENUM;
  identifier[ctoken.identifier_index].enum_value = count;
  identifier[ctoken.identifier_index].scope = SCOPE_GLOBAL;
  identifier[ctoken.identifier_index].process_scope = cstate.process_scope;
  identifier[ctoken.identifier_index].aspace_scope = cstate.cprocess->corresponding_aspace;
  identifier[ctoken.identifier_index].nspace_scope = -1;
  identifier[ctoken.identifier_index].asm_or_compiler = IDENTIFIER_ASM_OR_C; // accessible from asm (although if auto, gives the stack offset rather than an address)

  found_comma = 0;
  if (check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_COMMA))
  {
   found_comma = 1;
  }

// if we reach a }, we stop:
  if (check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACE_CLOSE))
   break; // finished


  if (!found_comma)
   return comp_error(CERR_EXPECTED_COMMA_AFTER_ENUM, &ctoken);

  count++;
 };

 check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_SEMICOLON); // accept, but do not require, ;

 return 1;

}




// call this function after one of the basic built-in functions (e.g. abs, get_hp etc) is called
// some built-in functions are dealt with elsewhere (e.g. new_process)
// method_type is e.g. MTYPE_PR_MATHS
// method_status is e.g. MSTATUS_PR_MATHS_ABS (or -1 if this call does not have a method status - in which case args start being put in the first mbank register rather than second)
// args is number of args to be put in mbank registers (e.g. abs(a) has 1). can be zero.
//  method_status doesn't count as an arg, whether or not it's -1
int simple_built_in_cfunction_call(int method_type, int method_status, int args, int target_register, int secondary_register, int save_secondary_register)
{

 struct ctokenstruct ctoken;
 strcpy(ctoken.name, "(empty)");

// make sure the current process has the required method:
 int method_index = find_method_index(method_type);

 if (method_index == -1) // not found
 {
  switch(method_type)
  {
   case MTYPE_PR_MATHS:
    return comp_error(CERR_NO_MTYPE_PR_MATHS, &ctoken);
   case MTYPE_CLOB_MATHS:
    return comp_error(CERR_NO_MTYPE_CL_MATHS, &ctoken);
   case MTYPE_PR_STD:
    return comp_error(CERR_NO_MTYPE_PR_STD, &ctoken);
   case MTYPE_CLOB_QUERY:
    return comp_error(CERR_NO_MTYPE_CLOB_QUERY, &ctoken);
   case MTYPE_PR_COMMAND:
    return comp_error(CERR_NO_MTYPE_PR_COMMAND, &ctoken);
   case MTYPE_CL_COMMAND_GIVE:
    return comp_error(CERR_NO_MTYPE_CL_COMMAND_GIVE, &ctoken);
   case MTYPE_CLOB_COMMAND_REC:
    return comp_error(CERR_NO_MTYPE_CLOB_COMMAND_REC, &ctoken);
   default:
    return comp_error(CERR_NO_METHOD_FOR_BUILTIN_CFUNCTION, &ctoken);
  }
 }

// expect opening bracket
  if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACKET_OPEN))
   return comp_error(CERR_EXPECTED_BRACKET_OPEN, &ctoken);

  int mbank_target = method_index * METHOD_SIZE;

// may need to put a method status to the first mbank
  if (method_status != -1)
  {
   add_intercode(IC_NUMBER_TO_KNOWN_MBANK, mbank_target, method_status, 0);
   mbank_target ++;
  }

  int read_arg = 0;

// now go through all arguments (if there are any), incrementing the target each time
  while (read_arg < args)
  {
// TO DO: optimise this (and other similar functions) by checking for a constant and putting that directly into the mbank rather than doing so via a register
   if (!parse_expression(target_register, secondary_register, save_secondary_register, -1, 0, BIND_BASIC))
    return 0;
   if (mbank_target >= METHOD_BANK)
    return comp_error(CERR_MBANK_ADDRESS_OUT_OF_BOUNDS, &ctoken);
   add_intercode(IC_REGISTER_TO_KNOWN_MBANK, mbank_target, 0, target_register); // middle operand not used here
   mbank_target ++;
   read_arg ++;
   if (read_arg < args)
   {
    if (check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACKET_CLOSE)) // this is only one possible error, but let's be nice and give a useful error message for this particular one
     return comp_error(CERR_BUILTIN_CFUNCTION_TOO_FEW_ARGS, &ctoken);
    if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_COMMA))
     return comp_error(CERR_SYNTAX, &ctoken);
   }
  };

  if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACKET_CLOSE))
   return comp_error(CERR_EXPECTED_BRACKET_CLOSE, &ctoken);

// Note: can't check for a semicolon here as this could be in an expression

  add_intercode(IC_CALL_METHOD_CONSTANT, method_index, 0, target_register);

  return 1;


}



// This function finds a method of method_type in the current process' interface definition
// It returns the index of that method in the definition, or -1 if not present
int find_method_index(int method_type)
{
 int i;

 for (i = 0; i < METHODS; i ++)
 {
  if (process_def[cstate.process_scope].iface.method [i] [INTERFACE_METHOD_DATA_TYPE] == method_type)
   return i;
// special case - MTYPE_PR_MATHS and MTYPE_CLOB_MATHS work basically the same way (at least for now):
  if (method_type == MTYPE_PR_MATHS
   && process_def[cstate.process_scope].iface.method [i] [INTERFACE_METHOD_DATA_TYPE] == MTYPE_CLOB_MATHS)
   return i;
 }

 return -1;

}





int allocate_exit_point(int type)
{

 cstate.expoint_pos ++;
 if (cstate.expoint_pos >= EXPOINTS)
  return -1;

 expoint[cstate.expoint_pos].type = type;

 return cstate.expoint_pos;

}

const char *error_name [CERRS] =
{
"(no error)", // CERR_NONE
"read error", // CERR_READ_FAIL,
"syntax error", // CERR_SYNTAX,
"declaration error", // CERR_INVALID_DECLARATION,
"function not prototyped (or does not match prototype)", // CERR_CFUNCTION_MISTYPE,
"variable declared as void", // CERR_VOID_VARIABLE,
"function error", // CERR_CFUNCTION_FAIL,
"function prototype error", // CERR_PROTOTYPE_FAIL,
"function has too many arguments", // CERR_EXCESS_CFUNCTION_ARGS,
"identifier reused", // CERR_IDENTIFIER_REUSED,
"function parameter error", // CERR_CFUNCTION_PARAMETER_FAIL,
"variable declaration error", // CERR_VAR_DECLARATION_FAIL,
"variable initialisation error", // CERR_VAR_INITIALISATION_FAIL,
"function definition error", // CERR_CFUNCTION_DEFINITION_FAIL,
"too few function parameters", // CERR_CFUNCTION_ARGS_MISSING,
"too many function parameters", // CERR_CFUNCTION_EXCESS_ARGS,
"failed to allocate exit point (code too complex)", // CERR_EXIT_POINT_FAIL,
"expected opening bracket after function name", // CERR_CFUNCTION_CALL_NO_OPENING_BRACKET
"not enough parameters in function call", // CERR_CFUNCTION_CALL_NOT_ENOUGH_PARAMETERS
"too many parameters in function call", // CERR_CFUNCTION_CALL_TOO_MANY_PARAMETERS
"return statement error", // CERR_RETURN_FAIL,
"unexpected keyword", // CERR_UNEXPECTED_KEYWORD,
"expected assignment after variable in statement", // CERR_EXPECTED_ASSIGNMENT_OPERATOR,
"expected semicolon at end of expression", // CERR_EXCESS_CLOSE_BRACKET,
"expected closing bracket in expressions", // CERR_NO_CLOSE_BRACKET,
"return value of void function used", // CERR_VOID_RETURN_USED
"syntax error in basic mode", // CERR_SYNTAX_BASIC_MODE,
"syntax error at start of statement", // CERR_SYNTAX_AT_STATEMENT_START,
"syntax error with punctuation in expression", // CERR_SYNTAX_PUNCTUATION_IN_EXPRESSION,
"syntax error with value in expression", // CERR_SYNTAX_EXPRESSION_VALUE,
"syntax error in if statement", // CERR_SYNTAX_IF,
"function prototype error: expected open bracket", // CERR_PROTOTYPE_EXPECTED_BRACKET_OPEN,
"function prototype error: expected semicolon", // CERR_PROTOTYPE_EXPECTED_SEMICOLON,
"function prototype error: expected new identifier", // CERR_PROTOTYPE_EXPECTED_NEW_IDENTIFIER,
"function prototype error: expected closing bracket", // CERR_PROTOTYPE_EXPECTED_BRACKET_CLOSE
"cannot prototype main (must be defined as void main(void))", // CERR_PROTOTYPED_MAIN,
"function redefined", // CERR_CFUNCTION_REDEFINED
"automatic variable or parameter in static function", // CERR_AUTO_VAR_IN_STATIC_CFUNCTION
"static function parameter redefined as automatic", // CERR_CFUNCTION_PARAMETER_STORAGE_NOT_STATIC,
"automatic function parameter redefined as static", // CERR_CFUNCTION_PARAMETER_STORAGE_NOT_AUTO,
"storage class specifier not followed by prototype or declaration", // CERR_STORAGE_SPECIFIER_MISUSE
"global variable cannot be automatic", // CERR_GLOBAL_VARIABLE_IS_AUTO
"expected type of function parameter (as int)", // CERR_PROTOTYPE_PARAMETER_NOT_INT,
"function parameter syntax error", // CERR_PROTOTYPE_PARAMETER_SYNTAX,
"function parameter: no closing bracket?", // CERR_CFUNCTION_PARAMETER_NO_CLOSE_BRACKET,
"function parameter not declared as int", // CERR_CFUNCTION_PARAMETER_NOT_INT,
"function parameter is not a variable?", // CERR_CFUNCTION_PARAMETER_NOT_VARIABLE,
"function parameter does not match prototype parameter", // CERR_CFUNCTION_PARAMETER_MISMATCH, // identifier of parameter doesn't match prototype
"function parameter not followed by comma?", // CERR_CFUNCTION_PARAMETER_NO_COMMA,
"dereferenced constant is out of bounds", // CERR_DEREFERENCED_CONSTANT_OUT_OF_BOUNDS
"misplaced minus sign (not used as an operator or to indicate a negative number?)", // CERR_NEGATIVE_NOT_A_NUMBER
"array has too many dimensions", // CERR_ARRAY_TOO_MANY_DIMENSIONS
"array dimension not a numerical constant", // CERR_ARRAY_DIMENSION_NOT_A_NUMBER,
"array dimension too small (at or below zero)", // CERR_ARRAY_DIMENSION_TOO_SMALL,
"array dimension too large", // CERR_ARRAY_DIMENSION_TOO_LARGE,
"array declaration error", // CERR_ARRAY_DECLARATION_ERROR
"array reference error", // CERR_ARRAY_REFERENCE_ERROR
"expected closing square bracket in array reference", // CERR_NO_CLOSE_SQUARE_BRACKET
"expected opening square bracket in array reference", // CERR_ARRAY_EXPECTED_SQUARE_OPEN
"& (address-of prefix) can only be used with a variable", // CERR_ADDRESS_OF_PREFIX_MISUSE
"cannot combine dereferencing with address-of", // CERR_DEREFERENCED_ADDRESS_OF
"cannot use ! negation with address-of prefix", // CERR_NEGATED_ADDRESS_OF
"array parameter missing opening square brackets?", // CERR_CFUNCTION_PARAMETER_ARRAY_NO_SQ_OP,
"array parameter missing closing square brackets?", // CERR_CFUNCTION_PARAMETER_ARRAY_NO_SQ_CL,
"array parameter dimension size doesn't match function prototype", // CERR_CFUNCTION_PARAMETER_ARRAY_MISMATCH,
"dimensions of array parameter do not match function definition", // CERR_ARRAY_ARG_TYPE_MISMATCH
"string too long (exceeds maximum of 80 characters)", // CERR_STRING_TOO_LONG,
"expected opening bracket", // CERR_EXPECTED_BRACKET_OPEN,
"expected opening bracket after while", // CERR_WHILE_MISSING_OPENING_BRACKET
"expected closing bracket after while expression", // CERR_WHILE_MISSING_CLOSING_BRACKET
"break not inside loop", // CERR_BREAK_WITHOUT_LOOP_ETC
"continue not inside loop", // CERR_CONTINUE_WITHOUT_LOOP
"expected semicolon", // CERR_EXPECTED_SEMICOLON,
"do without while", // CERR_DO_WITHOUT_WHILE;
"for statement without opening bracket", // CERR_FOR_MISSING_OPENING_BRACKET,
"for statement does not end with closing bracket?", // CERR_EXPECTED_CLOSING_BRACKET_IN_FOR,
"expected closing bracket", // CERR_EXPECTED_BRACKET_CLOSE
"method bank address out of bounds (must be 0 to 63)", // CERR_MBANK_ADDRESS_OUT_OF_BOUNDS
"out of space in array initialisation buffer", // CERR_ARRAY_INITIALISATION_TOO_LARGE,
"array dimension out of bounds", // CERR_ARRAY_DIM_OUT_OF_BOUNDS,
"array doesn't have this many dimensions", // CERR_TOO_MANY_ARRAY_DIMENSIONS,
"element of array initialised to non-constant", // CERR_ARRAY_INITIALISER_NOT_CONSTANT,
"expected opening brace", // CERR_EXPECTED_BRACE_OPEN,
"expected closing brace", // CERR_EXPECTED_BRACE_CLOSE,
"interface for this process already defined", // CERR_INTERFACE_ALREADY_DEFINED,
"expected constant number as initialiser", // CERR_EXPECTED_NUMBER_CONSTANT,
"expected comma after initialiser", // CERR_EXPECTED_COMMA_AFTER_VALUE,
"too many methods defined for interface", // CERR_IFACE_TOO_MANY_METHODS,
"too many values for method in interface definition", // CERR_IFACE_METHOD_TOO_MUCH_DATA,
"this built-in function can only be used for its return value", //CERR_BUILTIN_CFUNCTION_OUTSIDE_EXPRESSION
"too many processes", // CERR_TOO_MANY_PROCESSES,
"invalid process name (name already in use?)", // CERR_PROCESS_NAME_INVALID,
"process already defined", // CERR_PROCESS_ALREADY_DEFINED,
"end of source reached inside process definition", // CERR_REACHED_END_IN_SUBPROCESS,
"expected process name", // CERR_EXPECTED_PROCESS_NAME
"new_process method must be a constant from 0 to 14", // CERR_NEW_PROCESS_METHOD_NUMBER
"expected comma after  process name", // CERR_EXPECTED_COMMA_AFTER_PROCESS_NAME
"identifier already in use", // CERR_IDENTIFIER_ALREADY_USED
"expected comma after enum definition", // CERR_EXPECTED_COMMA_AFTER_ENUM
"constant method index must be within 0 to 15", // CERR_METHOD_OUT_OF_BOUNDS,
"call with parameters requires a constant method index", // CERR_COMPLEX_CALL_MUST_BE_CONSTANT,
"label already defined", // CERR_LABEL_ALREADY_DEFINED
"expected label after goto", // CERR_EXPECTED_LABEL
"process has only been prototyped implicitly", // CERR_PROCESS_NOT_PROTOTYPED
"process name already used in inline assembler?", // CERR_PROCESS_NAME_ALREADY_USED_IN_ASM
"function name already used in inline assembler?", // CERR_CFUNCTION_NAME_ALREADY_USED_IN_ASM
"failed to create new identifier (too many identifiers?)", // CERR_NEW_IDENTIFIER_FAIL
"first interface value must be a recognised program type", // CERR_UNRECOGNISED_PROGRAM_TYPE
"process must be prototyped before definition", // CERR_EXPECTED_SEMICOLON_AFTER_PROCESS_PROTOTYPE
"get parameter not a recognised constant", // CERR_GET_PARAMETER_NOT_CONSTANT,
"put parameter not a recognised constant", // CERR_PUT_PARAMETER_NOT_CONSTANT,
"case without switch", // CERR_CASE_WITHOUT_SWITCH,
"default without switch", // CERR_DEFAULT_WITHOUT_SWITCH,
"too many cases in switch", // CERR_TOO_MANY_CASES_IN_SWITCH,
"case value not constant", // CERR_CASE_VALUE_NOT_CONSTANT,
"expected colon after case or default", // CERR_EXPECTED_COLON_AFTER_CASE,
"duplicate care", // CERR_DUPLICATE_CASE,
"expected case or default", // CERR_EXPECTED_CASE_OR_DEFAULT,
"difference between smallest and largest case too large", // CERR_SWITCH_TOO_LARGE,
"switch has no cases", // CERR_SWITCH_HAS_NO_CASES
"switch already has a default case", // CERR_DUPLICATE_DEFAULT
"too few arguments to built-in functions", // CERR_BUILTIN_CFUNCTION_TOO_FEW_ARGS,
"built-in function needs MT_PR_MATHS method", // CERR_NO_MTYPE_PR_MATHS,
"built-in function needs MT_CLOB_MATHS method", // CERR_NO_MTYPE_CL_MATHS,
"built-in function needs MT_PR_STD method", // CERR_NO_MTYPE_PR_STD,
"built-in function needs a method that is not present", // CERR_NO_METHOD_FOR_BUILTIN_CFUNCTION,
"built-in function needs MT_CLOB_QUERY method", // CERR_NO_MTYPE_CLOB_QUERY
"built-in function needs MT_PR_COMMAND method", // CERR_NO_MTYPE_PR_COMMAND
"built-in function needs MT_CL_COMMAND method", // CERR_NO_MTYPE_CL_COMMAND_GIVE
"built-in function needs MT_CLOB_STD method", // CERR_NO_MTYPE_CLOB_COMMAND_REC
"this C keyword not implemented, sorry :(", // CERR_UNIMPLEMENTED_KEYWORD
"data type not supported (int only)", // CERR_UNSUPPORTED_DATA_TYPE
"identifier name already in use", // CERR_VAR_NAME_IN_USE
"code complexity error (statement is too complex or recursive?)", // CERR_RECURSION_LIMIT_REACHED_IN_STATEMENT
"code complexity error (expression is too complex or recursive?)", // CERR_RECURSION_LIMIT_REACHED_IN_EXPRESSION
"code complexity error in parser (something is too complex or recursive?)", // CERR_RECURSION_LIMIT_REACHED_BY_PARSER
"expected open brace or method label", // CERR_EXPECTED_OPEN_BRACE_OR_METHOD_LABEL
"expected colon after method label", // CERR_EXPECTED_COLON_AFTER_METHOD_LABEL

// parser errors
"scode range error?", // CERR_PARSER_SCODE_BOUNDS (probably should never occur)
"parser reached end of code while reading token", // CERR_PARSER_REACHED_END_WHILE_READING
"unknown token type", // CERR_PARSER_UNKNOWN_TOKEN_TYPE
"number too large", // CERR_PARSER_NUMBER_TOO_LARGE
"letter or underscore found in decimal number", // CERR_PARSER_LETTER_IN_NUMBER
"token too long", // CERR_PARSER_TOKEN_TOO_LONG
"number starts with 0 but is not hex or binary", // CERR_PARSER_NUMBER_STARTS_WITH_ZERO,
"invalid letter or underscore found in hex number", // CERR_PARSER_LETTER_IN_HEX_NUMBER,
"invalid digit in binary number", // CERR_INVALID_NUMBER_IN_BINARY_NUMBER,
"letter in binary number", // CERR_PARSER_LETTER_IN_BINARY_NUMBER,

// data() errors
"unrecognised first parameter in data()", // CERR_DATA_UNRECOGNISED_DATA_TYPE,
"data() parameter must be constant number", // CERR_DATA_PARAMETER_NOT_NUMBER,
"invalid shape in data()", // CERR_DATA_INVALID_SHAPE,
"invalid shape size in data()", // CERR_DATA_INVALID_SIZE,
"invalid shape vertex in data()", // CERR_DATA_INVALID_VERTEX,
"too few parameters for this kind of data()", // CERR_DATA_NOT_ENOUGH_PARAMETERS

};

int comp_error(int error_type, struct ctokenstruct* ctoken)
{
/*
     start_log_line(0, cstate.source_line); // TO DO: change 0 to some source index
     write_to_log("Error (line ", 0, cstate.source_line);
     write_number_to_log(cstate.source_line, 0, cstate.source_line);
     write_to_log(error_name [error_type], 0, cstate.source_line);
     if (ctoken != NULL)
     {
      write_to_log(" (", 0, cstate.source_line);
      write_to_log(ctoken->name, 0, cstate.source_line);
      write_to_log(")", 0, cstate.source_line);
     }*/



     start_log_line(MLOG_COL_ERROR);
     write_to_log("Compiler error at line ");
     write_number_to_log(cstate.src_line + 1);
     if (scode->src_file [cstate.scode_pos] == 0)
      write_to_log(" of file ");
       else
        write_to_log(" of included file ");
     write_to_log(scode->src_file_name [scode->src_file [cstate.scode_pos]]);
     write_to_log(".");
     finish_log_line();

     start_log_line(MLOG_COL_ERROR);
     write_to_log("Error: ");
     write_to_log(error_name [error_type]);
     write_to_log(".");
     finish_log_line();

     if (ctoken != NULL)
     {
      start_log_line(MLOG_COL_COMPILER);
      write_to_log("Last token read: ");
      write_to_log(ctoken->name);
      write_to_log(".");
      finish_log_line();
     }

     cstate.error = error_type;
//     error_call();
     return 0;
}


// call when comp_error is called from a function that returns -1 on failure (and so needs to return -1)
int comp_error_minus1(int error_type, struct ctokenstruct* ctoken)
{

 comp_error(error_type, ctoken);
 return -1;

}


// returns 1 on success
// returns 0 if out of space (or some other error, maybe)
// can be called from a_asm.c as well as from this file.
int add_intercode(int ic_type, int op1, int op2, int op3)
{

 if (cstate.icode_pos >= INTERCODE_SIZE - 2)
  return 0; // TO DO: currently trying to add too much intercode will just silently fail. Can't just put a comp_error here because it might display huge numbers of them. Need to do something about this!

 int ip = cstate.icode_pos;

 intercode[ip].instr = ic_type;

 intercode[ip].operand [0] = op1;
 intercode[ip].operand [1] = op2;
 intercode[ip].operand [2] = op3;

 intercode[ip].src_line = cstate.src_line;
 intercode[ip].src_file = cstate.src_file;

 intercode[ip].written_to_asm = 0;

// some add_intercode types do extra stuff
 switch(ic_type)
 {
  case IC_JUMP_EXIT_POINT_TRUE:
   expoint[op1].true_point_used = 1; // this indicates that the exit point should be written to asm (if asm is being generated)
   break;
  case IC_JUMP_EXIT_POINT_FALSE:
   expoint[op1].false_point_used = 1;
   break;
  case IC_IFTRUE_JUMP_TO_EXIT_POINT:
   expoint[op2].true_point_used = 1; // note op2
   break;
  case IC_IFFALSE_JUMP_TO_EXIT_POINT:
   expoint[op2].false_point_used = 1; // note op2
   break;
  case IC_EXIT_POINT_ADDRESS_TO_REGISTER:
   expoint[op2].true_point_used = 1; // note op2
   break;
  case IC_SWITCH_JUMP:
   expoint[op1].true_point_used = 1;
   break;
 }


 cstate.icode_pos ++;
 intercode[cstate.icode_pos].instr = IC_END;

 return 1;

}


