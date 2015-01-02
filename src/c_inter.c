#include <allegro5/allegro.h>
#include "m_config.h"
#include "g_header.h"
#include "c_header.h"

#include "g_misc.h"

#include "v_interp.h"
//#include "vac_header.h"
#include "e_log.h"
#include "e_slider.h"
#include "e_header.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>


// This file turns intercode (produced by the compiler in c_comp.c) into precode, which can then be turned into either bcode or assembler
// Precode is basically bcode opcodes with some information from the original source (e.g. identifier indices) to allow the assembly code to retain e.g. variable names

#define CRUNCH_ASM_LINE_THRESHOLD 100

#define TEMPSTR_LENGTH 20

// bnotestruct holds annotations to accompany bcode entries
// during intercode->bcode transcription, it stores information about various values that can't be resolved
// until the end, when memory locations of everything are known (e.g. cfunction call addresses to later cfunctions)
struct bfixstruct
{
 int fix_type;
 int fix_value;
 int src_line;
 int src_file;
 int process; // index of the process that this entry is part of
 int referring_aspace; // index of the aspace that this entry is part of (only relevant sometimes)
 int referred_aspace; // index of the aspace that the referred address is part of (only relevant sometimes; should be same as referring_aspace except when scoped variables are used)
};


extern const struct opcode_propertiesstruct opcode_properties [OP_end]; // in a_asm.c

extern struct astatestruct astate; // in a_asm.c
extern struct scodestruct *scode;

struct bfixstruct bfix [BCODE_MAX];

enum
{
FIX_NONE,
FIX_VARIABLE,
FIX_AUTO_VARIABLE_OFFSET,
FIX_CFUNCTION,
FIX_EXIT_POINT_TRUE,
FIX_EXIT_POINT_FALSE,
FIX_LABEL,
FIX_STACK_INIT,
FIX_LITERAL_NUMBER, // doesn't actually get fixed, but this indicates that it's a value and not an opcode
FIX_PROCESS_START,
FIX_PROCESS_END,
FIX_ASM_ADDRESS,
FIX_ASM_ASPACE_END_ADDRESS,

};


struct icstatestruct
{
 int icode_pos;
 int bcode_pos;
 struct bcodestruct* bcode;
 int error;
 int src_line;
 int src_file;
 int process_scope;

 int target_src_line; // if the target is a sourcestruct (e.g. generating asm from compiled code) this is the line of the target source file we're writing to.
 int target_src_line_length; // this should be updated whenever the target_source is updated
 int generate_asm; // whether we're generating asm into an empty sourcestruct while compiling
 int asm_number_sequence; // whether we're inside an asm number sequence (e.g. "{1, 2, 3}") while writing asm to asm
 int crunch_asm; // forces multiple instructions onto each line (for generating asm from long scodes)
// int last_asm_source_line; // when outputting asm annotated with source code in comments, this is the last line for which source was written.
 struct sourcestruct* target_source; // this is the sourcestruct for writing asm if generate_asm == 1. Will be NULL if generate_asm == 0.

 int current_aspace;
 int current_nspace;
};

struct icstatestruct icstate;

// some things are externed from c_comp.c:
extern struct intercodestruct intercode [INTERCODE_SIZE];
extern struct expointstruct expoint [EXPOINTS];
//extern struct cfunctionstruct cfunction [CFUNCTIONS];
extern struct identifierstruct identifier [IDENTIFIER_MAX];
extern struct array_initstruct array_init;
extern struct process_defstruct process_def [PROCESS_DEFS];


int register_operand_bits(int operand);
int mbank_operand_bits(int operand);
int method_operand_bits(int operand);
int register_operand_bits_special(int bcode_operand, int icode_operand);
int register_operand_bits_arg(int bcode_operand, int reg);
int number_operand_bits(int operand);
int reference_operand_bits(int operand);
int add_bcode_op(int instr);
int inter_error(int error_type);
int inter_error_bfix(int error_type, int bfix_index);
int fix_bcode(void);
int operand_text(int fix_type, int fix_value, int number);
int intercode_process_header(int process);
int write_init_value(int value, int comma);
int write_init_array(int* value_array, int elements, int comma_at_end);

//int intercode_initialisation_before_main(void);
int allocate_static_memory(void);
int write_object_instr(int opcode, int optype1, int op1, int optype2, int op2, int optype3, int op3, int address_offset);
void exit_point_string(char* str, int exp, int true_false);
int write_asm_opcode_to_asm(int ip);
int write_asm_number_to_asm(int ip, int operand);
int write_asm_variable_to_asm(int ip, int operand);
int write_asm_aspace_end_to_asm(int ip, int operand);
int write_asm_scoped_variable_to_asm(int ip, int operand);
int open_asm_number_sequence(void);
int close_asm_number_sequence(int ip);
int add_space_if_needed(void);

int target_source_asm_header(void);
int target_source_newline(void);
int target_source_write(const char* str, int add_space);
int target_source_write_num(s16b num);
int target_source_write_offset(s16b offset);
void strcat_num(char* str, int num);

enum
{
ICERR_ADD_FAIL,
ICERR_STATIC_VAR_FAIL,
ICERR_VAR_NO_BCODE_ADDRESS,
ICERR_CFUNCTION_NO_BCODE_ADDRESS,
ICERR_NO_TRUE_POINT,
ICERR_NO_FALSE_POINT,
ICERR_INTERFACE_NOT_DEFINED,
ICERR_PROCESS_NOT_DEFINED,
ICERR_LABEL_NOT_DEFINED,
ICERR_VAR_NO_STACK_OFFSET,
ICERR_GENERATED_SOURCE_TOO_LONG,
ICERR_GENERATED_SOURCE_LINE_TOO_LONG,
ICERR_ASM_ID_UNDEFINED,
ICERR_ASM_ASPACE_HAS_NO_END,
ICERR_EXPECTED_ASPACE_NAME,
ICERR_ASM_ASPACE_ID_UNDEFINED,
ICERR_PROCESS_TOO_LARGE,

ICERRS

};

enum
{
INSTR_OP_TYPE_NONE,
INSTR_OP_TYPE_REGISTER,
INSTR_OP_TYPE_NUMBER,
INSTR_OP_TYPE_VARIABLE,
INSTR_OP_TYPE_EXIT_POINT_TRUE,
INSTR_OP_TYPE_EXIT_POINT_FALSE,
INSTR_OP_TYPE_CFUNCTION,
INSTR_OP_TYPE_STACK_INIT,
INSTR_OP_TYPE_MBANK,
INSTR_OP_TYPE_METHOD,
INSTR_OP_TYPE_LABEL,
INSTR_OP_TYPE_PROCESS_START,
INSTR_OP_TYPE_PROCESS_END


};


// this is the number of addresses an if op should jump over if false
#define IF_JUMP_SIZE 3

int intercode_to_bcode_or_asm(struct bcodestruct* bcode, struct sourcestruct* target_source, int generate_asm)
{

 icstate.icode_pos = 0;
 icstate.bcode_pos = 0;
 icstate.bcode = bcode;
 icstate.process_scope = 0; // main process
 icstate.current_aspace = 0; // corresponds to main process
 icstate.current_nspace = -1; // not in any namespace
 icstate.target_src_line = 0;
 icstate.target_src_line_length = 0;
 icstate.generate_asm = generate_asm;
 icstate.target_source = target_source; // will be NULL if !generate_asm
 icstate.asm_number_sequence = 0;
 icstate.crunch_asm = 0;

 if (generate_asm == 2)
  icstate.crunch_asm = 1;

 process_def[0].start_address = 0; // main process always starts at 0. Other process' start addresses are set when they are defined.

 int i, ip;

 for (i = 0; i < bcode->bcode_size; i ++)
 {
  bcode->op [i] = OP_nop;
  bfix[i].fix_type = FIX_NONE;
  bfix[i].src_line = -1;
  bfix[i].src_file = -1;
  bfix[i].process = -1;
 }

 int instr;

 if (icstate.generate_asm != 0
  && !target_source_asm_header()) // adds "asm_only asm {" to generated asm source
   return 0;

 if (!intercode_process_header(0))
  return 0; // intercode_process_header can fail if some things not defined properly (e.g. no interface definition)

 char tempstr [TEMPSTR_LENGTH];

// now go through and convert intercode to bcode:
 for (ip = 0; ip < INTERCODE_SIZE; ip ++)
 {

  icstate.icode_pos = ip;
  icstate.src_line = intercode[ip].src_line; // this is applied to bfix[].src_line in add_bcode_op()
  icstate.src_file = intercode[ip].src_file;

  switch(intercode[ip].instr)
  {
   case IC_COPY_REGISTER2_TO_REGISTER1: // setrr <register_target> <register_source>
    if (!write_object_instr(OP_setrr, INSTR_OP_TYPE_REGISTER, intercode[ip].operand [0], INSTR_OP_TYPE_REGISTER, intercode[ip].operand [1], INSTR_OP_TYPE_NONE, 0, 0))
     return 0;
    break;

   case IC_NUMBER_TO_REGISTER:
    if (!write_object_instr(OP_setrn, INSTR_OP_TYPE_REGISTER, intercode[ip].operand [0], INSTR_OP_TYPE_NUMBER, intercode[ip].operand [1], INSTR_OP_TYPE_NONE, 0, 0))
     return 0;
    break;
   case IC_IDENTIFIER_TO_REGISTER:
    if (!write_object_instr(OP_setra, INSTR_OP_TYPE_REGISTER, intercode[ip].operand [0], INSTR_OP_TYPE_VARIABLE, intercode[ip].operand [1], INSTR_OP_TYPE_NONE, 0, 0))
     return 0;
    break;

   case IC_IDENTIFIER_PLUS_OFFSET_TO_REGISTER:
    if (!write_object_instr(OP_setra, INSTR_OP_TYPE_REGISTER, intercode[ip].operand [0], INSTR_OP_TYPE_VARIABLE, intercode[ip].operand [1], INSTR_OP_TYPE_NONE, 0, intercode[ip].operand [2])) // operand [2] is the address offset
     return 0;
    break;

   case IC_ID_ADDRESS_PLUS_OFFSET_TO_REGISTER:
    if (!write_object_instr(OP_setrn, INSTR_OP_TYPE_REGISTER, intercode[ip].operand [0], INSTR_OP_TYPE_VARIABLE, intercode[ip].operand [1], INSTR_OP_TYPE_NONE, 0, intercode[ip].operand [2])) // operand [2] is the address offset
     return 0;
    break;

   case IC_PUSH_REGISTER:
    if (!write_object_instr(OP_push, INSTR_OP_TYPE_REGISTER, intercode[ip].operand [0], INSTR_OP_TYPE_REGISTER, intercode[ip].operand [1], 0, 0, 0))
     return 0;
    break;
   case IC_POP_TO_REGISTER:
    if (!write_object_instr(OP_pop, INSTR_OP_TYPE_REGISTER, intercode[ip].operand [0], INSTR_OP_TYPE_REGISTER, intercode[ip].operand [1], 0, 0, 0))
     return 0;
    break;
   case IC_JUMP_TO_REGISTER:
    if (!write_object_instr(OP_jumpr, INSTR_OP_TYPE_REGISTER, intercode[ip].operand [0], 0, 0, 0, 0, 0))
     return 0;
    break;
   case IC_COPY_REGISTER_TO_IDENTIFIER_ADDRESS:
    if (!write_object_instr(OP_setar, INSTR_OP_TYPE_VARIABLE, intercode[ip].operand [0], INSTR_OP_TYPE_REGISTER, intercode[ip].operand [1], 0, 0, 0))
     return 0;

    break;

// *** note: when adding any type of IC entry that relies on the existence of an exit point, add code to add_intercode in c_comp.c that sets the used value for that exit point to 1

   case IC_EXIT_POINT_TRUE: // does not add an instruction; instead, indicates that the following bcode op is the target for a particular exit point
    if (icstate.generate_asm // only add a label definition to asm if the expoint is actually used
     && expoint[intercode[ip].operand[0]].true_point_used)
    {
     exit_point_string(tempstr, intercode[ip].operand[0], TRUE);
     if (!target_source_newline())
      return 0;
     if (!target_source_write("def label", 1))
      return 0;
     if (!target_source_write(tempstr, 1))
      return 0;
    }
    expoint[intercode[ip].operand[0]].true_point_bcode = icstate.bcode_pos;
    break;
   case IC_EXIT_POINT_FALSE: // does not add an instruction; instead, indicates that the following bcode op is the target for a particular exit point
    if (icstate.generate_asm // only add a label definition to asm if the expoint is actually used
     && expoint[intercode[ip].operand[0]].false_point_used)
    {
     exit_point_string(tempstr, intercode[ip].operand[0], FALSE);
     if (!target_source_newline())
      return 0;
     if (!target_source_write("def label", 1))
      return 0;
     if (!target_source_write(tempstr, 1))
      return 0;
    }
    expoint[intercode[ip].operand[0]].false_point_bcode = icstate.bcode_pos;
    break;
   case IC_IFTRUE_JUMP_TO_EXIT_POINT:
    if (!write_object_instr(OP_iftrue, INSTR_OP_TYPE_REGISTER, intercode[ip].operand [0], INSTR_OP_TYPE_EXIT_POINT_TRUE, intercode[ip].operand [1], 0, 0, 0))
     return 0;
    break;
   case IC_IFFALSE_JUMP_TO_EXIT_POINT:
    if (!write_object_instr(OP_iffalse, INSTR_OP_TYPE_REGISTER, intercode[ip].operand [0], INSTR_OP_TYPE_EXIT_POINT_FALSE, intercode[ip].operand [1], 0, 0, 0))
     return 0;
    break;
   case IC_JUMP_EXIT_POINT_TRUE: // unconditional jump to true exit point (e.g. when stopping evaluation of an expression because an || has been reached)
    if (!write_object_instr(OP_jumpn, INSTR_OP_TYPE_EXIT_POINT_TRUE, intercode[ip].operand [0], 0, 0, 0, 0, 0))
     return 0;
    break;
   case IC_JUMP_EXIT_POINT_FALSE: // unconditional jump to false exit point (e.g. break within while loop)
    if (!write_object_instr(OP_jumpn, INSTR_OP_TYPE_EXIT_POINT_FALSE, intercode[ip].operand [0], 0, 0, 0, 0, 0))
     return 0;
    break;
   case IC_SWITCH_JUMP:
// this is a complicated one
// operands are: exit point with start of jump table, minimum case value offset (which should be minimum_case * -2)
// this instruction always uses particular registers (we can assume that it is a statement).
// we can assume that the value in the register is within bounds for the jump table (as comp_switch adds checks for this before adding this IC)
// first we need to multiply REGISTER_WORKING by 2 (as each jump table entry is 2 addresses):
    if (!write_object_instr(OP_setrn, INSTR_OP_TYPE_REGISTER, REGISTER_TEMP, INSTR_OP_TYPE_NUMBER, 2, 0, 0, 0)) // 2 is for multiplication below
     return 0;
    if (!write_object_instr(OP_mul, INSTR_OP_TYPE_REGISTER, REGISTER_WORKING, INSTR_OP_TYPE_REGISTER, REGISTER_WORKING, INSTR_OP_TYPE_REGISTER, REGISTER_TEMP, 0))
     return 0; // REGISTER_WORKING = REGISTER_WORKING * REGISTER_TEMP
// next we set REGISTER_TEMP to the jump table base
    if (!write_object_instr(OP_setrn, INSTR_OP_TYPE_REGISTER, REGISTER_TEMP, INSTR_OP_TYPE_EXIT_POINT_TRUE, intercode[ip].operand [0], 0, 0, intercode[ip].operand [1]))
     return 0;
//    fprintf(stdout, "\n IC_SWITCH_JUMP operands: %i, %i  bcode op %i",  intercode[ip].operand [0], intercode[ip].operand [1], (int) icstate.bcode->op [icstate.bcode_pos - 1]);
//   *** after the IC_SWITCH_JUMP intercode entry added, its operand [1] is corrected in comp_switch so it can be used as an address offset for this instruction
//       (this is so the minimum case value offset can be taken into account)

//   *** this means that if any changes are made to this code generation, the calculation of jump_table_address_intercode_address in comp_switch may need to change

// now we add that to REGISTER_WORKING, which contains the target case value:
    if (!write_object_instr(OP_add, INSTR_OP_TYPE_REGISTER, REGISTER_WORKING, INSTR_OP_TYPE_REGISTER, REGISTER_WORKING, INSTR_OP_TYPE_REGISTER, REGISTER_TEMP, 0))
     return 0; // REGISTER_WORKING = REGISTER_WORKING + REGISTER_TEMP
// now we jump to REGISTER_WORKING:
    if (!write_object_instr(OP_jumpr, INSTR_OP_TYPE_REGISTER, REGISTER_WORKING, 0, 0, 0, 0, 0))
     return 0;
    break;

// *** note: when adding any type of IC entry that relies on the existence of an exit point, add code to add_intercode in c_comp.c that sets the used value for that exit point to 1


   case IC_ADD_REG_TO_REG:
    if (!write_object_instr(OP_add, INSTR_OP_TYPE_REGISTER, intercode[ip].operand [0], INSTR_OP_TYPE_REGISTER, intercode[ip].operand [1], INSTR_OP_TYPE_REGISTER, intercode[ip].operand [2], 0))
     return 0;
    break;
   case IC_SUB_REG_FROM_REG:
    if (!write_object_instr(OP_sub, INSTR_OP_TYPE_REGISTER, intercode[ip].operand [0], INSTR_OP_TYPE_REGISTER, intercode[ip].operand [1], INSTR_OP_TYPE_REGISTER, intercode[ip].operand [2], 0))
     return 0;
    break;
   case IC_MUL_REG_BY_REG:
    if (!write_object_instr(OP_mul, INSTR_OP_TYPE_REGISTER, intercode[ip].operand [0], INSTR_OP_TYPE_REGISTER, intercode[ip].operand [1], INSTR_OP_TYPE_REGISTER, intercode[ip].operand [2], 0))
     return 0;
    break;
   case IC_DIV_REG_BY_REG:
    if (!write_object_instr(OP_div, INSTR_OP_TYPE_REGISTER, intercode[ip].operand [0], INSTR_OP_TYPE_REGISTER, intercode[ip].operand [1], INSTR_OP_TYPE_REGISTER, intercode[ip].operand [2], 0))
     return 0;
    break;
   case IC_MOD_REG_BY_REG:
    if (!write_object_instr(OP_mod, INSTR_OP_TYPE_REGISTER, intercode[ip].operand [0], INSTR_OP_TYPE_REGISTER, intercode[ip].operand [1], INSTR_OP_TYPE_REGISTER, intercode[ip].operand [2], 0))
     return 0;
    break;
   case IC_AND_REG_REG: // &
    if (!write_object_instr(OP_and, INSTR_OP_TYPE_REGISTER, intercode[ip].operand [0], INSTR_OP_TYPE_REGISTER, intercode[ip].operand [1], INSTR_OP_TYPE_REGISTER, intercode[ip].operand [2], 0))
     return 0;
    break;
   case IC_OR_REG_REG: // |
    if (!write_object_instr(OP_or, INSTR_OP_TYPE_REGISTER, intercode[ip].operand [0], INSTR_OP_TYPE_REGISTER, intercode[ip].operand [1], INSTR_OP_TYPE_REGISTER, intercode[ip].operand [2], 0))
     return 0;
    break;
   case IC_XOR_REG_REG: // ^
    if (!write_object_instr(OP_xor, INSTR_OP_TYPE_REGISTER, intercode[ip].operand [0], INSTR_OP_TYPE_REGISTER, intercode[ip].operand [1], INSTR_OP_TYPE_REGISTER, intercode[ip].operand [2], 0))
     return 0;
    break;
   case IC_BSHIFTL_REG_REG: // <<
    if (!write_object_instr(OP_bshiftl, INSTR_OP_TYPE_REGISTER, intercode[ip].operand [0], INSTR_OP_TYPE_REGISTER, intercode[ip].operand [1], INSTR_OP_TYPE_REGISTER, intercode[ip].operand [2], 0))
     return 0;
    break;
   case IC_BSHIFTR_REG_REG: // >>
    if (!write_object_instr(OP_bshiftr, INSTR_OP_TYPE_REGISTER, intercode[ip].operand [0], INSTR_OP_TYPE_REGISTER, intercode[ip].operand [1], INSTR_OP_TYPE_REGISTER, intercode[ip].operand [2], 0))
     return 0;
    break;
   case IC_INCR_REGISTER:
    if (!write_object_instr(OP_incr, INSTR_OP_TYPE_REGISTER, intercode[ip].operand [0], 0, 0, 0, 0, 0))
     return 0;
    break;
   case IC_DECR_REGISTER:
    if (!write_object_instr(OP_decr, INSTR_OP_TYPE_REGISTER, intercode[ip].operand [0], 0, 0, 0, 0, 0))
     return 0;
    break;


   case IC_JUMP_TO_CFUNCTION_ADDRESS:
    if (!write_object_instr(OP_jumpn, INSTR_OP_TYPE_CFUNCTION, intercode[ip].operand [0], 0, 0, 0, 0, 0))
     return 0;
    break;
   case IC_COMPARE_REGS_EQUAL: // REGISTER_A, REGISTER_B, 0); // this leaves register A with 1 if true, 0 if false
    if (!write_object_instr(OP_cmpeq, INSTR_OP_TYPE_REGISTER, intercode[ip].operand [0], INSTR_OP_TYPE_REGISTER, intercode[ip].operand [1], 0, 0, 0))
     return 0;
    break;
   case IC_COMPARE_REGS_GR: // REGISTER_A, REGISTER_B, 0); // this leaves register A with 1 if true, 0 if false
    if (!write_object_instr(OP_cmpgr, INSTR_OP_TYPE_REGISTER, intercode[ip].operand [0], INSTR_OP_TYPE_REGISTER, intercode[ip].operand [1], 0, 0, 0))
     return 0;
    break;
   case IC_COMPARE_REGS_GRE: // REGISTER_A, REGISTER_B, 0); // this leaves register A with 1 if true, 0 if false
    if (!write_object_instr(OP_cmpgre, INSTR_OP_TYPE_REGISTER, intercode[ip].operand [0], INSTR_OP_TYPE_REGISTER, intercode[ip].operand [1], 0, 0, 0))
     return 0;
    break;
   case IC_COMPARE_REGS_LS: // REGISTER_A, REGISTER_B, 0); // this leaves register A with 1 if true, 0 if false
    if (!write_object_instr(OP_cmpls, INSTR_OP_TYPE_REGISTER, intercode[ip].operand [0], INSTR_OP_TYPE_REGISTER, intercode[ip].operand [1], 0, 0, 0))
     return 0;
    break;
   case IC_COMPARE_REGS_LSE: // REGISTER_A, REGISTER_B, 0); // this leaves register A with 1 if true, 0 if false
    if (!write_object_instr(OP_cmplse, INSTR_OP_TYPE_REGISTER, intercode[ip].operand [0], INSTR_OP_TYPE_REGISTER, intercode[ip].operand [1], 0, 0, 0))
     return 0;
    break;
   case IC_COMPARE_REGS_NEQUAL: // REGISTER_A, REGISTER_B, 0); // this leaves register A with 1 if true, 0 if false
    if (!write_object_instr(OP_cmpneq, INSTR_OP_TYPE_REGISTER, intercode[ip].operand [0], INSTR_OP_TYPE_REGISTER, intercode[ip].operand [1], 0, 0, 0))
     return 0;
    break;
   case IC_NOT_REG_REG: // !
    if (!write_object_instr(OP_not, INSTR_OP_TYPE_REGISTER, intercode[ip].operand [0], INSTR_OP_TYPE_REGISTER, intercode[ip].operand [1], 0, 0, 0))
     return 0;
    break;
   case IC_BITWISE_NOT_REG_REG: // ~
    if (!write_object_instr(OP_bnot, INSTR_OP_TYPE_REGISTER, intercode[ip].operand [0], INSTR_OP_TYPE_REGISTER, intercode[ip].operand [1], 0, 0, 0))
     return 0;
    break;

   case IC_EXIT_POINT_ADDRESS_TO_REGISTER: // REGISTER_A, 0, 0); // the intercode parser will need to work out what the return address is. Maybe use conditional exit point? Although we should be able to predict the offset.
    if (!write_object_instr(OP_setrn, INSTR_OP_TYPE_REGISTER, intercode[ip].operand [0], INSTR_OP_TYPE_EXIT_POINT_TRUE, intercode[ip].operand [1], 0, 0, 0))
     return 0;
    break;
   case IC_CFUNCTION_START: // cstate.scope, 0, 0);
// The parameters are already put on the stack by the calling code (see c_comp.c code for CM_CALL_CFUNCTION)
// let's make some room on the stack for local variables:
    process_def[icstate.process_scope].cfunction[intercode[ip].operand [0]].bcode_address = icstate.bcode_pos;
    if (process_def[icstate.process_scope].cfunction[intercode[ip].operand [0]].size_on_stack > 0)
    {
// set the working register to the stack size of the cfunction:
    if (!write_object_instr(OP_setrn, INSTR_OP_TYPE_REGISTER, REGISTER_WORKING, INSTR_OP_TYPE_NUMBER, process_def[icstate.process_scope].cfunction[intercode[ip].operand [0]].size_on_stack, 0, 0, 0))
     return 0; // note that the register is identified specifically as the working register
// now add that to the stack pointer register
    if (!write_object_instr(OP_add, INSTR_OP_TYPE_REGISTER, REGISTER_STACK, INSTR_OP_TYPE_REGISTER, REGISTER_WORKING, INSTR_OP_TYPE_REGISTER, REGISTER_STACK, 0))
     return 0; // note that the registers are identified specifically as the working or stack registers
    }
// Now we can just go straight into the code for the function...
    break;
  case IC_CFUNCTION_MAIN:
// These are the things we need to do at the start of the main cfunction.
// First, initialise the stack frame pointer:
   if (!write_object_instr(OP_setrn, INSTR_OP_TYPE_REGISTER, REGISTER_STACK_FRAME, INSTR_OP_TYPE_STACK_INIT, 0, 0, 0, 0))
    return 0;
// now copy the stack frame pointer to the stack pointer
   if (!write_object_instr(OP_setrr, INSTR_OP_TYPE_REGISTER, REGISTER_STACK, INSTR_OP_TYPE_REGISTER, REGISTER_STACK_FRAME, 0, 0, 0))
    return 0; // note that the registers are identified specifically as the stack or stack frame registers
// now initialise any static variables with := assignments (assign at start of execution)
//   if (!intercode_initialisation_before_main())
//    return 0;
   break;
  case IC_STOP:
   if (!write_object_instr(OP_exit, 0, 0, 0, 0, 0, 0, 0))
    return 0;
   break;

  case IC_DEREFERENCE_REGISTER_TO_REGISTER:
   if (!write_object_instr(OP_setrd, INSTR_OP_TYPE_REGISTER, intercode[ip].operand [0], INSTR_OP_TYPE_REGISTER, intercode[ip].operand [1], 0, 0, 0))
    return 0;
   break;

  case IC_COPY_REGISTER_TO_DEREFERENCED_REGISTER:
   if (!write_object_instr(OP_setdr, INSTR_OP_TYPE_REGISTER, intercode[ip].operand [0], INSTR_OP_TYPE_REGISTER, intercode[ip].operand [1], 0, 0, 0))
    return 0;
   break;

  case IC_KNOWN_MBANK_TO_REGISTER:
   if (!write_object_instr(OP_getrb, INSTR_OP_TYPE_REGISTER, intercode[ip].operand [0], INSTR_OP_TYPE_MBANK, intercode[ip].operand [1], 0, 0, 0))
    return 0;
   break;

  case IC_REGISTER_MBANK_TO_REGISTER:
   if (!write_object_instr(OP_getrr, INSTR_OP_TYPE_REGISTER, intercode[ip].operand [0], INSTR_OP_TYPE_REGISTER, intercode[ip].operand [1], 0, 0, 0))
    return 0;
   break;

  case IC_REGISTER_TO_KNOWN_MBANK:
// remember - the register should be the last parameter to add_intercode
   if (!write_object_instr(OP_putbr, INSTR_OP_TYPE_MBANK, intercode[ip].operand [0], INSTR_OP_TYPE_REGISTER, intercode[ip].operand [2], 0, 0, 0))
    return 0; // note that the second operand is in intercode[ip].operand [2], not [1] (for historical reasons)
   break;

  case IC_REGISTER_TO_REGISTER_MBANK:
   if (!write_object_instr(OP_putrr, INSTR_OP_TYPE_REGISTER, intercode[ip].operand [0], INSTR_OP_TYPE_REGISTER, intercode[ip].operand [1], 0, 0, 0))
    return 0;
   break;

  case IC_NUMBER_TO_KNOWN_MBANK:
   if (!write_object_instr(OP_putbn, INSTR_OP_TYPE_MBANK, intercode[ip].operand [0], INSTR_OP_TYPE_NUMBER, intercode[ip].operand [1], 0, 0, 0))
    return 0;
   break;

  case IC_CALL_METHOD_CONSTANT:
   if (!write_object_instr(OP_callmr, INSTR_OP_TYPE_METHOD, intercode[ip].operand [0], INSTR_OP_TYPE_REGISTER, intercode[ip].operand [2], 0, 0, 0))
    return 0;
   break; // note that the second operand is in intercode[ip].operand [2], not [1] (for historical reasons)

  case IC_CALL_METHOD_REGISTER:
   if (!write_object_instr(OP_callrr, INSTR_OP_TYPE_REGISTER, intercode[ip].operand [0], INSTR_OP_TYPE_REGISTER, intercode[ip].operand [1], 0, 0, 0))
    return 0;
   break;

/*  case IC_CALL_METHOD_NUMBER: // not yet implemented
   break;*/

  case IC_LABEL_DEFINITION:
   identifier[intercode[ip].operand [0]].address_bcode = icstate.bcode_pos; // I'm sure it's easy to produce undefined behaviour with labels.
   if (icstate.generate_asm)
   {
    if (!target_source_newline())
     return 0;
    if (!target_source_write("def label ", 0))
     return 0;
    if (!target_source_write(identifier[intercode[ip].operand [0]].name, 1))
     return 0;
   }
   break;

  case IC_GOTO:
   if (!write_object_instr(OP_jumpn, INSTR_OP_TYPE_LABEL, intercode[ip].operand [0], 0, 0, 0, 0, 0))
    return 0;
   break;


  case IC_PROCESS_START_TO_KNOWN_MBANK:
   if (!write_object_instr(OP_putbn, INSTR_OP_TYPE_MBANK, intercode[ip].operand [0], INSTR_OP_TYPE_PROCESS_START, intercode[ip].operand [1], 0, 0, 0))
    return 0;
   break;

  case IC_PROCESS_END_TO_KNOWN_MBANK:
   if (!write_object_instr(OP_putbn, INSTR_OP_TYPE_MBANK, intercode[ip].operand [0], INSTR_OP_TYPE_PROCESS_END, intercode[ip].operand [1], 0, 0, 0))
    return 0;
   break;

  case IC_PROCESS_START: // operand [0] is the process being started
   process_def[intercode[ip].operand [0]].start_address = icstate.bcode_pos;
   icstate.process_scope = intercode[ip].operand [0];
   if (!intercode_process_header(icstate.process_scope))
    return 0; // intercode_process_header can fail if some things not defined properly (e.g. no interface definition)
// the asm to enter the process' aspace should have been written in c_comp.c
   break;

  case IC_PROCESS_END: // operand [0] is the process being ended; [1] is the parent process we are returning to
   if (!allocate_static_memory())
    return 0;
   process_def[intercode[ip].operand [0]].end_address = icstate.bcode_pos; // used to be a -1 at the end but I'm pretty sure it was wrong
// Check for a process being too long for the program type indicated in its interface:
   int subprocess_bcode_size_max = BCODE_MAX;
   switch(process_def[intercode[ip].operand [0]].iface.type)
   {
    case PROGRAM_TYPE_SYSTEM: // unlikely
     subprocess_bcode_size_max = SYSTEM_BCODE_SIZE; break;
    case PROGRAM_TYPE_PROCESS:
     subprocess_bcode_size_max = PROC_BCODE_SIZE; break;
    case PROGRAM_TYPE_DELEGATE:
    case PROGRAM_TYPE_OBSERVER:
    case PROGRAM_TYPE_OPERATOR:
     subprocess_bcode_size_max = CLIENT_BCODE_SIZE; break;
   }
   int actual_subprocess_size = process_def[intercode[ip].operand [0]].end_address - process_def[intercode[ip].operand [0]].start_address;
   if (actual_subprocess_size > subprocess_bcode_size_max)
   {
// This is a kind of mysterious error that doesn't exist in normal C, so let's be a bit helpful and explain what's gone wrong:
     start_log_line(MLOG_COL_ERROR);
     write_to_log("Process ");
     write_to_log(identifier[process_def[intercode[ip].operand [0]].id_index].name);
     write_to_log(" too long (");
     write_number_to_log(actual_subprocess_size);
     write_to_log("/");
     write_number_to_log(subprocess_bcode_size_max);
     write_to_log(").");
     finish_log_line();
     return inter_error(ICERR_PROCESS_TOO_LARGE);
   }
   icstate.process_scope = intercode[ip].operand [1];
// the asm to end the process' aspace should be written in c_comp.c
   break;

  case IC_PROCESS_START_TO_REGISTER:
   if (!write_object_instr(OP_setrn, INSTR_OP_TYPE_REGISTER, intercode[ip].operand [0], INSTR_OP_TYPE_PROCESS_START, intercode[ip].operand [1], 0, 0, 0))
    return 0;
   break;

  case IC_PROCESS_END_TO_REGISTER:
   if (!write_object_instr(OP_setrn, INSTR_OP_TYPE_REGISTER, intercode[ip].operand [0], INSTR_OP_TYPE_PROCESS_END, intercode[ip].operand [1], 0, 0, 0))
    return 0;
   break;

  case IC_PRINT_STRING:
   instr = OP_prints << OPCODE_BITSHIFT;
   if (!add_bcode_op(instr))
    return inter_error(ICERR_ADD_FAIL);
   if (icstate.generate_asm)
   {
    if (icstate.crunch_asm == 0
     || icstate.target_src_line_length > 0) // print instructions should start on a new line even if crunching asm
    {
     if (!target_source_newline())
      return 0;
    }
    if (!target_source_write("prints {\"", 0))
     return 0;
//    if (!target_source_write("\"", 0))
//     return 0;
   }
   break;

  case IC_STRING: // TO DO: this may not be safe on arbitrary input - e.g. it doesn't test for zeros within the string
   instr = intercode[ip].operand [0];
   if (!add_bcode_op(instr))
    return inter_error(ICERR_ADD_FAIL);
   if (icstate.generate_asm)
   {
    if (instr != '\n')
    {
     tempstr [0] = instr;
     tempstr [1] = '\0';
    }
     else
     {
      tempstr [0] = '\\';
      tempstr [1] = 'n';
      tempstr [2] = '\0';
     }
    if (!target_source_write(tempstr, 0))
     return 0;
   }
   break;

  case IC_STRING_END:
   instr = 0;
   if (!add_bcode_op(instr))
    return inter_error(ICERR_ADD_FAIL);
   if (icstate.generate_asm)
   {
    if (!target_source_write("\"}", 0))
     return 0;
   }
   break;

  case IC_ASM_START_STRING: // just writes (to asm) the {" at the start of a memory initialisation sequence that's a string
// this is needed because asm writes the prints directly as an instruction without going through IC_PRINT_STRING
   if (icstate.generate_asm)
   {
    if (!target_source_write("{\"", 0))
     return 0;
   }
   break;


  case IC_PRINT_STRING_A:
   if (!write_object_instr(OP_printsa, INSTR_OP_TYPE_VARIABLE, intercode[ip].operand [0], 0, 0, 0, 0, 0))
    return 0;
   break;

  case IC_PRINT_REGISTER:
   if (!write_object_instr(OP_printr, INSTR_OP_TYPE_REGISTER, intercode[ip].operand [0], 0, 0, 0, 0, 0))
    return 0;
   break;

  case IC_PRINT_REGISTER_A:
   if (!write_object_instr(OP_printsr, INSTR_OP_TYPE_REGISTER, intercode[ip].operand [0], 0, 0, 0, 0, 0))
    return 0;
   break;


// assembler intercode:
  case IC_ASM_OPCODE:
   if (!add_bcode_op(intercode[ip].operand [0]))
    return inter_error(ICERR_ADD_FAIL);
   if (icstate.generate_asm)
   {
    if (!write_asm_opcode_to_asm(ip))
     return inter_error(ICERR_ADD_FAIL);
   }
   break;

  case IC_ASM_NUMBER:
   if (!add_bcode_op(intercode[ip].operand [0]))
    return inter_error(ICERR_ADD_FAIL);
   if (icstate.generate_asm && !intercode[ip].written_to_asm)
   {
    if (!write_asm_number_to_asm(ip, 0))
     return inter_error(ICERR_ADD_FAIL);
   }
   break;

  case IC_ASM_ADDRESS:
   instr = intercode[ip].operand [1]; // this is the offset. the bcode address of identifier[fix_value] will be added to this.
   bfix[icstate.bcode_pos].fix_type = FIX_ASM_ADDRESS;
   bfix[icstate.bcode_pos].referring_aspace = icstate.current_aspace;
   bfix[icstate.bcode_pos].referred_aspace = icstate.current_aspace;
   bfix[icstate.bcode_pos].src_line = icstate.src_line;
   bfix[icstate.bcode_pos].src_file = icstate.src_file;
   if (identifier[intercode[ip].operand [0]].type == ID_USER_INT
    && identifier[intercode[ip].operand [0]].storage_class == STORAGE_AUTO)
     bfix[icstate.bcode_pos].fix_type = FIX_AUTO_VARIABLE_OFFSET; // the variable's stack offset will be used instead of a bcode address
   bfix[icstate.bcode_pos].fix_value = intercode[ip].operand [0];
// check for asm generic aliases and resolve them to the aliased identifier:
   if (identifier[intercode[ip].operand [0]].type == ASM_ID_GENERIC_ALIAS)
    bfix[icstate.bcode_pos].fix_value = identifier[intercode[ip].operand [0]].asm_alias;
   if (!add_bcode_op(instr))
    return inter_error(ICERR_ADD_FAIL);
   if (icstate.generate_asm && !intercode[ip].written_to_asm)
   {
    if (!write_asm_variable_to_asm(ip, 0))
     return inter_error(ICERR_ADD_FAIL);
   }
   break;

  case IC_ASM_ADDRESS_SCOPED: // like IC_ASM_ADDRESS but operand [2] holds the aspace scope (which will be used to calculate an address offset)
   instr = intercode[ip].operand [1]; // this is the offset. the bcode address of identifier[fix_value] will be added to this.
   bfix[icstate.bcode_pos].fix_type = FIX_ASM_ADDRESS;
   bfix[icstate.bcode_pos].referring_aspace = icstate.current_aspace;
   bfix[icstate.bcode_pos].referred_aspace = intercode[ip].operand [2]; //identifier[intercode[ip].operand[0]].aspace_scope; //intercode[ip].operand [2];
   bfix[icstate.bcode_pos].src_line = icstate.src_line;
   bfix[icstate.bcode_pos].src_file = icstate.src_file;
   if (identifier[intercode[ip].operand [0]].type == ID_USER_INT
    && identifier[intercode[ip].operand [0]].storage_class == STORAGE_AUTO)
     bfix[icstate.bcode_pos].fix_type = FIX_AUTO_VARIABLE_OFFSET; // the variable's stack offset will be used instead of a bcode address
   bfix[icstate.bcode_pos].fix_value = intercode[ip].operand [0];
// check for asm generic aliases and resolve them to the aliased identifier:
   if (identifier[intercode[ip].operand [0]].type == ASM_ID_GENERIC_ALIAS)
    bfix[icstate.bcode_pos].fix_value = identifier[intercode[ip].operand [0]].asm_alias;
//   fprintf(stdout, "\nFix scoped asm address: id (%s) in aspace %i (id aspace %i) (current %i) index %i", identifier[intercode[ip].operand[0]].name, intercode[ip].operand [2], identifier[intercode[ip].operand[0]].aspace_scope, icstate.current_aspace, intercode[ip].operand[0]);
   if (!add_bcode_op(instr))
    return inter_error(ICERR_ADD_FAIL);
   if (icstate.generate_asm && !intercode[ip].written_to_asm)
   {
//    if (!write_asm_variable_to_asm(ip, 0))
     if (!write_asm_scoped_variable_to_asm(ip, 0))
     return inter_error(ICERR_ADD_FAIL);
   }
   break;


  case IC_ASM_DEFINE_INT:
   identifier[intercode[ip].operand [0]].address_bcode = icstate.bcode_pos;// - astate.aspace[icstate.current_aspace].bcode_start;
   if (icstate.generate_asm)
   {
    if (!target_source_newline())
     return 0;
    if (!target_source_write("def int ", 0))
     return 0;
    if (!target_source_write(identifier[intercode[ip].operand [0]].name, 0))
     return 0;
   }
   break;

  case IC_ASM_DEFINE_LABEL:
   identifier[intercode[ip].operand [0]].address_bcode = icstate.bcode_pos;// - astate.aspace[icstate.current_aspace].bcode_start;
   if (icstate.generate_asm)
   {
    if (!target_source_newline())
     return 0;
    if (!target_source_write("def label ", 0))
     return 0;
    if (!target_source_write(identifier[intercode[ip].operand [0]].name, 1))
     return 0;
   }
   break;

  case IC_ASM_ENTER_ASPACE: // operand 0 is the aspace index
   icstate.current_aspace = intercode[ip].operand [0];
   icstate.current_nspace = -1;
   identifier[astate.aspace [intercode[ip].operand [0]].identifier_index].address_bcode = icstate.bcode_pos;// - astate.aspace[icstate.current_aspace].bcode_start;
   astate.aspace [intercode[ip].operand [0]].bcode_start = icstate.bcode_pos;
   if (icstate.generate_asm)
   {
    if (!target_source_newline())
     return 0;
    if (!target_source_write("def aspace ", 0))
     return 0;
    if (!target_source_write(identifier[astate.aspace [intercode[ip].operand [0]].identifier_index].name, 1))
     return 0;
   }
   break;

  case IC_ASM_END_ASPACE:
   astate.aspace [intercode[ip].operand [0]].bcode_end = icstate.bcode_pos;
   icstate.current_aspace = astate.aspace [intercode[ip].operand [0]].parent;
   icstate.current_nspace = -1;
   if (icstate.generate_asm)
   {
    if (!target_source_newline())
     return 0;
    if (!target_source_write("aspace_end", 0))
     return 0;
   }
   break;

  case IC_ASM_ASPACE_END_ADDRESS: // operand 0 is id_index of the aspace name. operand 1 is any offset
   instr = intercode[ip].operand [1]; // this is the offset. the bcode address of identifier[fix_value] will be added to this.
   bfix[icstate.bcode_pos].fix_type = FIX_ASM_ASPACE_END_ADDRESS;
   bfix[icstate.bcode_pos].referring_aspace = icstate.current_aspace;
   bfix[icstate.bcode_pos].referred_aspace = icstate.current_aspace;
   bfix[icstate.bcode_pos].src_line = icstate.src_line;
   bfix[icstate.bcode_pos].src_file = icstate.src_file;
   if (identifier[intercode[ip].operand [0]].type == ASM_ID_GENERIC_ALIAS)
   {
    if (identifier[identifier[intercode[ip].operand [0]].asm_alias].type != ASM_ID_ASPACE)
     return inter_error(ICERR_EXPECTED_ASPACE_NAME);
    bfix[icstate.bcode_pos].fix_value = identifier[intercode[ip].operand [0]].asm_alias;
   }
    else
    {
     if (identifier[intercode[ip].operand [0]].type != ASM_ID_ASPACE)
      return inter_error(ICERR_EXPECTED_ASPACE_NAME);
     bfix[icstate.bcode_pos].fix_value = intercode[ip].operand [0];
    }
   if (!add_bcode_op(instr))
    return inter_error(ICERR_ADD_FAIL);
   if (icstate.generate_asm && !intercode[ip].written_to_asm)
   {
    if (!write_asm_aspace_end_to_asm(ip, 0))
     return inter_error(ICERR_ADD_FAIL);
   }
   break;

  case IC_ASM_DEFINE_NSPACE: // operand 0 is the nspace index
   identifier[astate.nspace [intercode[ip].operand [0]].identifier_index].address_bcode = icstate.bcode_pos;// - astate.aspace[icstate.current_aspace].bcode_start;
   icstate.current_nspace = intercode[ip].operand [0];
   if (icstate.generate_asm)
   {
    if (!target_source_newline())
     return 0;
    if (!target_source_write("def nspace ", 0))
     return 0;
    if (!target_source_write(identifier[astate.nspace [intercode[ip].operand [0]].identifier_index].name, 1))
     return 0;
   }
   break;

  case IC_ASM_REJOIN_NSPACE:
   icstate.current_nspace = intercode[ip].operand [0];
   if (icstate.generate_asm)
   {
    if (!target_source_newline())
     return 0;
    if (!target_source_write("nspace_rejoin ", 0))
     return 0;
    if (!target_source_write(identifier[astate.nspace [intercode[ip].operand [0]].identifier_index].name, 0))
     return 0;
   }
   break;

  case IC_ASM_END_NSPACE:
   icstate.current_nspace = -1;
   if (icstate.generate_asm)
   {
    if (!target_source_newline())
     return 0;
    if (!target_source_write("nspace_end ", 0))
     return 0;
   }
   break;

// Will need to record all asm definitions in intercode so they can be put back in when generating asm


  case IC_END: // the end of the intercode (different to IC_STOP, which is an instruction to stop executing)
   ip = INTERCODE_SIZE;
   break;


  }


 }

// now we just need to finish dealing with process 0, which won't have an IC_PROCESS_END entry for it:
 icstate.process_scope = 0;
 if (!process_def[0].asm_only)
 {
  if (!allocate_static_memory())
   return 0;
 }

 process_def[0].end_address = icstate.bcode_pos;
 astate.aspace[process_def[0].corresponding_aspace].bcode_end = icstate.bcode_pos;
 icstate.bcode->static_length = icstate.bcode_pos;

 if (!fix_bcode())
  return 0;

// finally, if generating asm we need to add } to the end to match the "asm {" at the start:
 if (icstate.generate_asm)
 {
  if (!target_source_newline())
   return 0;
  if (!target_source_write("}", 0))
   return 0;
 }


 return 1;

}

int add_bcode_op(int instr)
{
 if (icstate.bcode_pos >= icstate.bcode->bcode_size - 2)
 {
  return 0;
 }
 icstate.bcode->op [icstate.bcode_pos] = instr;
// icstate.bcode->note->src_line [icstate.bcode_pos] = icstate.src_line;
 bfix[icstate.bcode_pos].process = icstate.process_scope;
 bfix[icstate.bcode_pos].src_line = icstate.src_line;
 bfix[icstate.bcode_pos].src_file = icstate.src_file;
 icstate.bcode_pos ++;
 return 1;


}

// this function is called when an asm opcode intercode entry is found.
// it turns the opcode back into asm, then reads ahead to find operands which it turns into asm as well.
int write_asm_opcode_to_asm(int ip)
{
 u16b instr = abs(intercode[ip].operand [0]);
 u16b opcode = abs(instr >> OPCODE_BITSHIFT);

 if (opcode >= OP_end)
  return 0; // shouldn't be possible as this function should only be called when generating asm from asm


 if (icstate.crunch_asm)
 {
  if (icstate.target_src_line_length > CRUNCH_ASM_LINE_THRESHOLD
   && !target_source_newline())
    return 0;
 }
  else
  {
   if (!target_source_newline())
    return 0;
  }
 if (!target_source_write(opcode_properties[opcode].name, 1))
  return 0;

/* if (!target_source_newline())
  return 0;
 if (!target_source_write(opcode_properties[opcode].name, 1))
  return 0;*/

 int j;
 int operand_position = 0;
 int address_position = 1;
 char tempstr [TEMPSTR_LENGTH];
 int op;

 for (j = 0; j < opcode_properties[opcode].operands; j ++)
 {
  switch(opcode_properties[opcode].operand_type [j])
  {
   case OPERAND_TYPE_REGISTER:
    op = (instr >> (ADDRESS_MODE_BITFIELD_SIZE * operand_position)) & ADDRESS_MODE_MASK_ALL;
    tempstr [0] = op + 'A';
    tempstr [1] = '\0';
    if (!target_source_write(tempstr, 1))
     return 0;
    operand_position ++;
    break;
   case OPERAND_TYPE_MBANK:
    op = (instr >> (ADDRESS_MODE_BITFIELD_SIZE * operand_position)) & MBANK_ADDRESS_MODE_MASK;
    if (!target_source_write_num(op))
     return 0;
    if (!target_source_write(" ", 0))
     return 0;
    operand_position += 2;
    break;
   case OPERAND_TYPE_METHOD:
    op = (instr >> (ADDRESS_MODE_BITFIELD_SIZE * operand_position)) & METHOD_ADDRESS_MODE_MASK;
    if (!target_source_write_num(op))
     return 0;
    if (!target_source_write(" ", 0))
     return 0;
    operand_position += 2;
    break;
   case OPERAND_TYPE_NUMBER:
   case OPERAND_TYPE_ADDRESS:
// can assume that ip+address_position is valid, as this would have been checked when the intercode was being generated
// can't assume these kinds of operand are in a particular form.
// the exact type of the operand is indicated by the instr field of the intercode entry for the operand:
    switch(intercode[ip + address_position].instr)
    {
     case IC_ASM_ADDRESS:
      if (!write_asm_variable_to_asm(ip + address_position, 1))
       return 0;
      intercode[ip+address_position].written_to_asm = 1;
      address_position++;
      break;
     case IC_ASM_NUMBER:
      if (!target_source_write_num(intercode[ip + address_position].operand [0]))
       return 0;
      if (!target_source_write(" ", 0))
       return 0;
      intercode[ip+address_position].written_to_asm = 1;
      address_position++;
      break;
     case IC_ASM_ASPACE_END_ADDRESS:
      if (!write_asm_aspace_end_to_asm(ip + address_position, 1))
       return 0;
      intercode[ip+address_position].written_to_asm = 1;
      address_position++;
      break;
     case IC_ASM_ADDRESS_SCOPED:
      if (!write_asm_scoped_variable_to_asm(ip + address_position, 1))
       return 0;
      intercode[ip+address_position].written_to_asm = 1;
      address_position++;
      break;

    }
    break; // end OPERAND_TYPE_NUMBER and OPERAND_TYPE_ADDRESS
  }
 }

 return 1;

}


// operand is currently unused but may be used later
int write_asm_number_to_asm(int ip, int operand)
{

 if (!open_asm_number_sequence())
  return 0;

 if (!target_source_write_num(intercode[ip].operand [0]))
  return 0;

 if (!close_asm_number_sequence(ip))
  return 0;

 return 1;

}


// writes an asm variable to asm
// if operand is 1, the variable's being used as an operand. If 0 it's being used as memory initialisation so a {...} sequence should be used
int write_asm_variable_to_asm(int ip, int operand)
{

/*
// if the variable is being used as an operand and is a local variable, or an asm identifier with namespace scope, it should be written with explicit scope (otherwise if this is its first mention it will be implicitly declared as global):
 if (operand
  && (identifier[intercode[ip].operand [0]].scope != SCOPE_GLOBAL
   || identifier[intercode[ip].operand [0]]. != SCOPE_GLOBAL

 *** no - generic aliases are used to deal with this (see ASM_ID_GENERIC_ALIAS enum in c_header.h)
*/

 if (!operand
  && !open_asm_number_sequence())
   return 0;


// Do we need to verify the identifier reference? I'm pretty sure we don't.

    if (!target_source_write(identifier[intercode[ip].operand [0]].name, 0))
     return 0;
//    intercode[ip].written_to_asm = 1;
    if (!target_source_write_offset(intercode[ip].operand [1]))
     return 0;

 if (!operand
  && !close_asm_number_sequence(ip))
   return 0;

 if (!target_source_write(" ", 0))
  return 0;

 return 1;

}


int write_asm_aspace_end_to_asm(int ip, int operand)
{

 if (!operand
  && !open_asm_number_sequence())
   return 0;


// Do we need to verify the identifier reference? I'm pretty sure we don't.

    if (!target_source_write("aspace_end_address(", 0))
     return 0;
    if (intercode[ip].operand [0] == icstate.current_aspace)
    {
     if (!target_source_write("self", 0))
      return 0;
    }
     else
     {
      if (!target_source_write(identifier[intercode[ip].operand [0]].name, 0))
       return 0;
     }
    if (!target_source_write(")", 0))
     return 0;
    if (!target_source_write_offset(intercode[ip].operand [1]))
     return 0;
    if (!target_source_write(" ", 0))
     return 0;

 if (!operand
  && !close_asm_number_sequence(ip))
   return 0;

 return 1;

}

// writes a scoped asm variable to asm
// if operand is 1, the variable's being used as an operand. If 0 it's being used as memory initialisation so a {...} sequence should be used
int write_asm_scoped_variable_to_asm(int ip, int operand)
{

 if (!operand
  && !open_asm_number_sequence())
   return 0;

// Do we need to verify the identifier reference? I'm pretty sure we don't.

// to assemble a scoped asm expression, we need to find a path through aspace definitions from the aspace of the target address to the aspace of the referring expression.
    int aspace_list [ASPACES];
    int id_index = intercode[ip].operand [0];
    int aspace_track = identifier[id_index].aspace_scope;
    int aspace_target = intercode[ip].operand [2]; // the aspace in which the expression is located
    int i = 0;

//   fprintf(stdout, "\naspace_track %i aspace_target %i", aspace_track, aspace_target);

// this loop fills aspace_list with a list of aspace scopes between the address and the expression
//  - the loop doesn't run at all if the scope is within the same aspace (but a difference nspace)
    while(aspace_track != aspace_target)
    {
     aspace_list [i] = aspace_track;
     aspace_track = astate.aspace[aspace_track].parent;
     if (aspace_track == aspace_target)
      break;
     i ++;
#ifdef SANITY_CHECK
     if (aspace_track == -1)
     {
      fprintf(stdout, "c_inter.c: write_asm_scoped_variable_to_asm(): aspace_track reached -1");
      error_call();
     }
     if (i >= ASPACES)
     {
      fprintf(stdout, "c_inter.c: write_asm_scoped_variable_to_asm(): i reached ASPACES");
      error_call();
     }
#endif
    };

// now write the aspace part of the scope reference:

    if (!target_source_write("scope", 0))
     return 0;

    int j;

//    fprintf(stdout, "\nScoped variable: index %i name (%s) aspace_scope %i in aspace %i current %i; nspace_scope %i current %i", id_index, identifier[id_index].name, identifier[id_index].aspace_scope, aspace_target, icstate.current_aspace, identifier[id_index].nspace_scope, icstate.current_nspace);
//    fprintf(stdout, " list (%i,%i,%i,%i,%i) track %i i %i", aspace_list [0], aspace_list [1],aspace_list [2],aspace_list [3],aspace_list [4], aspace_track, i);

    if (identifier[id_index].aspace_scope != intercode[ip].operand [2]) // don't need to write any aspace name if we're just referring to a different nspace in the same aspace
    {
     for (j = i; j > -1; j --)
     {
      if (!target_source_write(".", 0))
       return 0;
      if (!target_source_write(identifier[astate.aspace[aspace_list [j]].identifier_index].name, 0))
       return 0;
     }
    }

// now check whether the address is in a particular nspace:
    if (identifier[id_index].nspace_scope != -1)
    {
     if (!target_source_write(":", 0))
      return 0;
     if (!target_source_write(identifier[astate.nspace[identifier[id_index].nspace_scope].identifier_index].name, 0))
      return 0;
    }

// now write the name of the scoped address:
     if (!target_source_write("::", 0))
      return 0;
    if (!target_source_write(identifier[id_index].name, 0))
     return 0;
//    if (!target_source_write(")", 0))
//     return 0;
    if (!target_source_write_offset(intercode[ip].operand [1]))
     return 0;

 if (!operand
  && !close_asm_number_sequence(ip))
   return 0;

 if (!target_source_write(" ", 0))
  return 0;

 return 1;

}



// checks whether it's necessary to start a memory init sequence, and if so starts one
int open_asm_number_sequence(void)
{

  if (icstate.asm_number_sequence)
  {
    if (!target_source_write(", ", 0))
     return 0;
  }
   else
   {
    if (!target_source_newline())
     return 0;
    if (!target_source_write(" {", 0))
     return 0;
    }

  if (icstate.target_src_line_length > SOURCE_TEXT_LINE_LENGTH - 15) // make sure there's space
  {
   if (!target_source_newline())
    return 0;
  }

  return 1;
}

// checks whether it's necessary to stop a memory init sequence, and if so starts one
int close_asm_number_sequence(int ip)
{

  icstate.asm_number_sequence = 1;

  if (ip < INTERCODE_SIZE - 1)
  {
   if (intercode[ip + 1].instr != IC_ASM_NUMBER
    && intercode[ip + 1].instr != IC_ASM_ADDRESS
    && intercode[ip + 1].instr != IC_ASM_ADDRESS_SCOPED
    && intercode[ip + 1].instr != IC_ASM_ASPACE_END_ADDRESS)
   {
    icstate.asm_number_sequence = 0;
    if (!target_source_write("}", 0))
     return 0;
   }
  }

 return 1;

}

/*
this function adds code to bcode for the start of a process
the code added is:
 - a jump to main address
 - interface definition (based on an interfacestruct built up during compilation)
note that this function is not called for an asm_only process (and neither is the parse_interface_definition code in c_comp.c) so the interface definition will need to be just a long data definition.
 - to use a standard interface definition in an asm file you could leave out asm_only and have the only compiled code as the interface definition and a main function (which contains all asm code). I think that should work.

** remember - any changes in this function may need to be reflected in parse_system_program_interface() in c_comp.c and derive_pre_init_from_system_program() in s_setup.c

returns 1 on success, 0 on error
*/
int intercode_process_header(int process)
{


 if (process_def[process].asm_only)
  return 1; // no need for a process header

 int i;

// first two instructions are always jump opcode and destination (usually start of main function) - these are BCODE_HEADER_ALL_JUMP and BCODE_HEADER_ALL_JUMP2
 if (!write_object_instr(OP_jumpn, INSTR_OP_TYPE_CFUNCTION, 0, 0, 0, 0, 0, 0))
  return 0;

// now add interface data:
 struct interfacestruct* iface = &process_def[icstate.process_scope].iface;

 if (!iface->defined) // will be zero if this process has not had an interface definition
  return inter_error(ICERR_INTERFACE_NOT_DEFINED);

 if (icstate.generate_asm)
 {
  if (!target_source_newline())
   return 0;
  if (!target_source_write("def int interface {", 0)) // closing brace will be written at end of this function
   return 0;
 }

 if (!write_init_value(iface->type, 1))
  return 0;

 switch(iface->type) // we should be able to assume the type value is valid as it will have been checked during compilation
 {
  default: // all types other than processes and system programs
   break; // nothing here. go straight to method definitions.

  case PROGRAM_TYPE_PROCESS:
   if (!write_init_value(iface->shape, 1)) return 0;
   if (!write_init_value(iface->size, 1)) return 0;
   if (!write_init_value(iface->base_vertex, 1)) return 0;
   break;

  case PROGRAM_TYPE_SYSTEM:
// [PI_VALUES] arrays
   if (!write_init_array(iface->system_program_preinit.players, PI_VALUES, 1)) return 0;
   if (!write_init_array(iface->system_program_preinit.game_turns, PI_VALUES, 1)) return 0;
   if (!write_init_array(iface->system_program_preinit.game_minutes_each_turn, PI_VALUES, 1)) return 0;
   if (!write_init_array(iface->system_program_preinit.procs_per_team, PI_VALUES, 1)) return 0;
//   if (!write_init_array(iface->system_program_preinit.gen_limit, PI_VALUES, 1)) return 0;
   if (!write_init_array(iface->system_program_preinit.packets_per_team, PI_VALUES, 1)) return 0;
   if (!write_init_array(iface->system_program_preinit.w_block, PI_VALUES, 1)) return 0;
   if (!write_init_array(iface->system_program_preinit.h_block, PI_VALUES, 1)) return 0;
// now add a couple of values (add a newline to separate from preceding arrays)
   if (icstate.generate_asm)
   {
    if (!target_source_newline())
     return 0;
   }
// single values
   if (!write_init_value(iface->system_program_preinit.allow_player_clients, 1)) return 0;
   if (!write_init_value(iface->system_program_preinit.player_operator, 1)) return 0;
   if (!write_init_value(iface->system_program_preinit.allow_user_observer, 1)) return 0;
// write_init_array inserts newline here
// [PLAYERS] arrays
   if (!write_init_array(iface->system_program_preinit.may_change_client_template, PLAYERS, 1)) return 0;
   if (!write_init_array(iface->system_program_preinit.may_change_proc_templates, PLAYERS, 1)) return 0;
   break;

 } // end switch(iface_def->type)

 for (i = 0; i < METHODS; i ++)
 {
  if (i == METHODS - 1)
  {
   if (!write_init_array(iface->method [i], INTERFACE_METHOD_DATA, 0)) // no trailing comma
    return 0;
  }
   else
   {
    if (!write_init_array(iface->method [i], INTERFACE_METHOD_DATA, 1)) // trailing comma
     return 0;
   }
 }


 if (icstate.generate_asm)
 {
  if (!target_source_write("}", 0))
   return 0;
 }

// ** remember - any changes in this function should be reflected in parse_system_program_interface() in c_comp.c

// TO DO: Think about: instead of having the interface data here, have the address of the interface data here.
//  That would allow the interface data to be put in the stack space, where it doesn't take up any space after creation (as the stack will just overwrite it)

 return 1;

}



// this function writes a value to bcode
// also, if icstate.generate_asm == 1, it writes the value to asm. If comma == 1, it also writes a comma afterwards
// returns 1/0 success/fail
int write_init_value(int value, int comma)
{

 if (icstate.generate_asm)
 {
  if (icstate.target_src_line_length > SOURCE_TEXT_LINE_LENGTH - 15) // make sure there's space
  {
   if (!target_source_newline())
    return 0;
  }

  if (!target_source_write_num(value)) // TO DO: should be able to write the enum word instead of just a number
   return 0;
  if (comma)
  {
   if (!target_source_write(", ", 0))
    return 0;
  }
 }

 if (!add_bcode_op(value))
  return inter_error(ICERR_ADD_FAIL);

 return 1;

}

// this function writes (elements) values from (value_array) to bcode
// also, if icstate.generate_asm == 1, it writes the values to asm as a string of numbers preceded by a newline
//  if comma_at_end == 1, it also writes a comma after the final value (it always writes commas between the values)
// returns 1/0 success/fail
int write_init_array(int* value_array, int elements, int comma_at_end)
{

 if (icstate.generate_asm)
 {
  if (!target_source_newline())
   return 0;
 }

 int i;
 int retval;

 for (i = 0; i < elements; i ++)
 {
  if (i < elements - 1
   || comma_at_end)
   retval = write_init_value(value_array [i], 1);
    else
     retval = write_init_value(value_array [i], 0);
  if (!retval)
   return 0;
 }

 return 1;

}

#define MAX_OPERANDS 3
#define OPCODE_NAME_LENGTH 10


// this is the leftshift needed to get from the rightmost bit to the bit that determines the overall address mode for a single operand operation
//#define SINGLE_OPERAND_ADDRESS_MODE_BIT 10 not currently used
//#define OPCODE_BITSHIFT 10

/*

Each opcode will expect operands of certain types, usually register and register, register and memory or memory and register
Register will use 4 bits
Memory has a whole 6 bits:
bit 0: if 1, next 4 bits refer to a register, the contents of which are dereferenced to find result
otherwise bits 1-5 produce a number (0-31):
0: read number from bcode, use as value
1: dereference next number




*/


// writes a simple instruction to bcode and, if icstate.generate_asm is set to 1, also as asm to icstate.target_source.
// currently assumes that there's only one address_offset needed for an instruction (it will be used for any op of address type)
// returns 1 on success, zero on failure
int write_object_instr(int opcode, int optype1, int op1, int optype2, int op2, int optype3, int op3, int address_offset)
{

 int instr, i;
 char tempstr [TEMPSTR_LENGTH];
 int ops [3];
 ops [0] = op1;
 ops [1] = op2;
 ops [2] = op3;
 int optype [3];
 optype [0] = optype1;
 optype [1] = optype2;
 optype [2] = optype3;
 int bit_offset = 0; // the offset for new operands that are incorporated in the instruction. when an operand is added, this value increases by the number of spaces it takes up (1 for register, 2 for mbank and method)

 instr = opcode << OPCODE_BITSHIFT;

 if (icstate.generate_asm)
 {
  if (icstate.crunch_asm)
  {
   if (icstate.target_src_line_length > CRUNCH_ASM_LINE_THRESHOLD
    && !target_source_newline())
     return 0;
  } else
    {
     if (!target_source_newline())
      return 0;
    }
  add_space_if_needed();
  if (!target_source_write(opcode_properties[opcode].name, 1))
   return 0;
 }

// this loop goes through and processes the operand types that are incorporated into bits of the instruction itself.
// it also adds asm for all operand types.
// the loop after it adds successive bcode ops used by the instruction.
 for (i = 0; i < opcode_properties[opcode].operands; i ++)
 {

  switch(optype [i])
  {


   case INSTR_OP_TYPE_REGISTER:
    if (icstate.generate_asm)
    {
     tempstr [0] = ops [i] + 'A';
     tempstr [1] = '\0';
//     sprintf(tempstr, "(%i:%i)", i, ops [i]);
     if (!target_source_write(tempstr, 1))
      return 0;
    }
    instr |= ops [i] << (bit_offset * ADDRESS_MODE_BITFIELD_SIZE);
    bit_offset++;
    break;
   case INSTR_OP_TYPE_NUMBER:
    if (icstate.generate_asm)
    {
     if (!target_source_write_num(ops [i]))
      return 0;
     if (!target_source_write_offset(address_offset))
      return 0;
     if (!target_source_write(" ", 0))
      return 0;
    }
    break;
   case INSTR_OP_TYPE_VARIABLE:
// adds to bcode later
    if (icstate.generate_asm)
    {
     if (identifier [ops [i]].nspace_scope != -1
      && identifier [ops [i]].nspace_scope != icstate.current_nspace)
     {
      if (!target_source_write("scope:", 0))
       return 0;
      if (!target_source_write(identifier[astate.nspace[identifier[ops [i]].nspace_scope].identifier_index].name, 0))
       return 0;
      if (!target_source_write("::", 0))
       return 0;
     }

     if (!target_source_write(identifier [ops [i]].name, 0))
      return 0;
/*
     if (identifier [ops [i]].nspace_scope != -1
      && identifier [ops [i]].nspace_scope != icstate.current_nspace)
     {
       if (!target_source_write(")", 0))
        return 0;
     }*/

     if (!target_source_write_offset(address_offset))
      return 0;

     if (!target_source_write(" ", 0))
         return 0;

    }
    break;
   case INSTR_OP_TYPE_EXIT_POINT_TRUE:
    if (icstate.generate_asm)
    {
     exit_point_string(tempstr, ops [i], TRUE);
     if (!target_source_write(tempstr, 1))
      return 0;

     if (!target_source_write_offset(address_offset))
      return 0;

    }
    break;
   case INSTR_OP_TYPE_EXIT_POINT_FALSE:
    if (icstate.generate_asm)
    {
     exit_point_string(tempstr, ops [i], FALSE);
     if (!target_source_write(tempstr, 1))
      return 0;

     if (!target_source_write_offset(address_offset))
      return 0;

    }
    break;
   case INSTR_OP_TYPE_CFUNCTION:
    if (icstate.generate_asm)
    {
     if (!target_source_write(identifier[process_def[icstate.process_scope].cfunction[ops [i]].identifier_index].name, 1))
      return 0;
    }
    break;
   case INSTR_OP_TYPE_STACK_INIT:
    if (icstate.generate_asm)
    {
     if (!target_source_write("aspace_end_address(self) ", 0))
      return 0;
/*     if (!target_source_write("aspace_end_address(", 0))
      return 0;
     if (
     if (!target_source_write(identifier[astate.aspace[icstate.current_aspace].identifier_index].name, 0))
      return 0;
     if (!target_source_write(") ", 0))
      return 0;*/
    }
    break;
   case INSTR_OP_TYPE_METHOD:
   case INSTR_OP_TYPE_MBANK:
    if (icstate.generate_asm)
    {
     if (!target_source_write_num(ops [i]))
      return 0;
     if (!target_source_write(" ", 0))
      return 0;
    }
    instr |= ops [i] << (bit_offset * ADDRESS_MODE_BITFIELD_SIZE);
    bit_offset += 2;
    break;
   case INSTR_OP_TYPE_LABEL:
    if (identifier[ops [i]].type != ID_USER_LABEL)
     return inter_error(ICERR_LABEL_NOT_DEFINED);
    if (icstate.generate_asm)
    {
     if (!target_source_write(identifier[ops [i]].name, 1))
      return 0;
    }
    break;
   case INSTR_OP_TYPE_PROCESS_START:
    if (icstate.generate_asm)
    {
     if (!target_source_write(identifier[process_def[ops [i]].id_index].name, 1))
      return 0;
//     if (!target_source_write(".start", 1))
//      return 0;
    }
    break;
   case INSTR_OP_TYPE_PROCESS_END:
    if (icstate.generate_asm)
    {
     if (!target_source_write("aspace_end_address(", 0))
      return 0;
     if (!target_source_write(identifier[process_def[ops [i]].id_index].name, 0))
      return 0;
     if (!target_source_write(")", 1))
      return 0;
    }
    break;

  } // end switch
 } // end first for i loop (adding operands that are stored in the instruction, and adding asm)

// now we write the instruction so far:
 if (!add_bcode_op(instr))
  return inter_error(ICERR_ADD_FAIL);

// now we need to go back through the operands and add any addresses or numbers to bcode, with appropriate bfix values if needed:
// don't need to add any more asm
 for (i = 0; i < opcode_properties[opcode].operands; i ++)
 {
  switch(optype [i])
  {
   case INSTR_OP_TYPE_REGISTER: // already included in instruction
    break;
   case INSTR_OP_TYPE_NUMBER:
//    bfix[icstate.bcode_pos].fix_type = FIX_VARIABLE; // only needed for debugging - can remove
//    bfix[icstate.bcode_pos].fix_value = ops [i];
    if (!add_bcode_op(ops [i] + address_offset))
     return inter_error(ICERR_ADD_FAIL);
    break;
   case INSTR_OP_TYPE_VARIABLE:
    instr = address_offset; // the memory address of the variable with identifier_index of ops [i] will be added to this
    bfix[icstate.bcode_pos].fix_type = FIX_VARIABLE;
    bfix[icstate.bcode_pos].fix_value = ops [i];
    bfix[icstate.bcode_pos].src_line = icstate.src_line;
    bfix[icstate.bcode_pos].src_file = icstate.src_file;
    if (!add_bcode_op(instr))
     return inter_error(ICERR_ADD_FAIL);
    break;
   case INSTR_OP_TYPE_EXIT_POINT_TRUE:
    instr = address_offset;
    bfix[icstate.bcode_pos].fix_type = FIX_EXIT_POINT_TRUE;
    bfix[icstate.bcode_pos].fix_value = ops [i];
    bfix[icstate.bcode_pos].src_line = icstate.src_line;
    bfix[icstate.bcode_pos].src_file = icstate.src_file;
    if (!add_bcode_op(instr))
     return inter_error(ICERR_ADD_FAIL);
    break;
   case INSTR_OP_TYPE_EXIT_POINT_FALSE:
    instr = address_offset;
    bfix[icstate.bcode_pos].fix_type = FIX_EXIT_POINT_FALSE;
    bfix[icstate.bcode_pos].fix_value = ops [i];
    bfix[icstate.bcode_pos].src_line = icstate.src_line;
    bfix[icstate.bcode_pos].src_file = icstate.src_file;
    if (!add_bcode_op(instr))
     return inter_error(ICERR_ADD_FAIL);
    break;
   case INSTR_OP_TYPE_CFUNCTION:
    bfix[icstate.bcode_pos].fix_type = FIX_CFUNCTION;
    bfix[icstate.bcode_pos].fix_value = ops [i];
    bfix[icstate.bcode_pos].src_line = icstate.src_line;
    bfix[icstate.bcode_pos].src_file = icstate.src_file;
    if (!add_bcode_op(0))
     return inter_error(ICERR_ADD_FAIL);
    break;
   case INSTR_OP_TYPE_STACK_INIT:
    instr = address_offset;
    bfix[icstate.bcode_pos].fix_type = FIX_STACK_INIT;
    bfix[icstate.bcode_pos].fix_value = ops [i];
    bfix[icstate.bcode_pos].src_line = icstate.src_line;
    bfix[icstate.bcode_pos].src_file = icstate.src_file;
    if (!add_bcode_op(instr))
     return inter_error(ICERR_ADD_FAIL);
    break;
   case INSTR_OP_TYPE_MBANK: // already included in instruction
   case INSTR_OP_TYPE_METHOD:
    break;
   case INSTR_OP_TYPE_LABEL:
    instr = address_offset;
    bfix[icstate.bcode_pos].fix_type = FIX_LABEL;
    bfix[icstate.bcode_pos].fix_value = ops [i];
    bfix[icstate.bcode_pos].src_line = icstate.src_line;
    bfix[icstate.bcode_pos].src_file = icstate.src_file;
    if (!add_bcode_op(instr))
     return inter_error(ICERR_ADD_FAIL);
    break;
   case INSTR_OP_TYPE_PROCESS_START:
    instr = address_offset;
    bfix[icstate.bcode_pos].fix_type = FIX_PROCESS_START;
    bfix[icstate.bcode_pos].fix_value = ops [i];
    bfix[icstate.bcode_pos].src_line = icstate.src_line;
    bfix[icstate.bcode_pos].src_file = icstate.src_file;
    if (!add_bcode_op(instr))
     return inter_error(ICERR_ADD_FAIL);
    break;
   case INSTR_OP_TYPE_PROCESS_END:
    instr = address_offset;
    bfix[icstate.bcode_pos].fix_type = FIX_PROCESS_END;
    bfix[icstate.bcode_pos].fix_value = ops [i];
    bfix[icstate.bcode_pos].src_line = icstate.src_line;
    bfix[icstate.bcode_pos].src_file = icstate.src_file;
    if (!add_bcode_op(instr))
     return inter_error(ICERR_ADD_FAIL);
    break;

  } // end switch
 } // end second for i loop

 return 1;

}

// this function produces a string of the format _exp(number)(t/f) e.g. _exp15t
void exit_point_string(char* str, int exp, int true_false)
{
 strcpy(str, "_exp");
#ifdef SANITY_CHECK
 if (exp < 0 || exp >= EXPOINTS)
 {
  fprintf(stdout, "\nc_inter.c: exit_point_string(): exp out of bounds (%i; max %i)", exp, EXPOINTS);
  error_call();
 }
#endif

 strcat_num(str, exp);

 if (true_false)
  strcat(str, "t");
   else
    strcat(str, "f");

}

void strcat_num(char* str, int num)
{

 char tempstr [TEMPSTR_LENGTH];

 sprintf(tempstr, "%i", num);

 strcat(str, tempstr);

}


//extern struct sourcestruct test_source_c;
//extern struct sourcestruct test_src;

int fix_bcode(void)
{

 int i;
// int storage_size;
// int array_base;

 icstate.process_scope = 0; // start from process zero.
// icstate.process_scope is updated if we read a bcode entry with a different process_scope.
// some other things (e.g. stack_base) are also updated.

// this allocates static memory at the end of process zero. It should have been called at the end of any other process definition.

 int operands_left = 0;

// ASM_PRINT prints some debugging information to stdout while writing asm code (if generate_asm == 1)
//#define ASM_PRINT
#define ASM_PRINT_START 0
#define ASM_PRINT_END 5

#ifdef ASM_PRINT
 int j;
 int operand_bits;
 unsigned int opcode;
#endif

//int previous_src_line = -1;

 for (i = 0; i < icstate.bcode->bcode_size; i ++)
 {

#ifdef ASM_PRINT
  if (operands_left == 0 && (i >= ASM_PRINT_START && i < ASM_PRINT_END))
  {
/*   if (icstate.bcode->op [i] < 0)
    opcode = icstate.bcode->op [i] * -1;
     else
      opcode = icstate.bcode->op [i];*/
   opcode = icstate.bcode->op [i] >> OPCODE_BITSHIFT;

   if (opcode < 0 || opcode >= OP_end)
   {
/*    if (icstate.generate_asm)
    {
     target_source_newline();
     target_source_write(" // (not an instruction: ");
     target_source_write_num(opcode);
    }*/
    continue;
   }



   for (j = 0; j < opcode_properties [opcode].operands; j ++)
   {
    operand_bits = icstate.bcode->op [i] & ADDRESS_MODE_MASK_ALL;
    if (j > 0)
     operand_bits = ((icstate.bcode->op [i]) >> (ADDRESS_MODE_BITFIELD_SIZE*j)) & ADDRESS_MODE_MASK_ALL;

    switch (opcode_properties [opcode].operand_type [j])
    {
     case OPERAND_TYPE_REGISTER:
/*      if (icstate.generate_asm)
      {
       tempstr [0] = operand_bits + 'A';
       tempstr [1] = 0;
       target_source_write(tempstr);
       target_source_write(" ");
      }*/
      break;
     case OPERAND_TYPE_MBANK:
/*      if (icstate.generate_asm)
      {
       if (j == 0)
        operand_bits = icstate.bcode->op [i] & MBANK_ADDRESS_MODE_MASK;
         else
          operand_bits = (icstate.bcode->op [i] >> (ADDRESS_MODE_BITFIELD_SIZE*j)) & MBANK_ADDRESS_MODE_MASK;
       target_source_write_num(operand_bits);
       target_source_write(" ");
      }*/
      break;
     case OPERAND_TYPE_ADDRESS:
     case OPERAND_TYPE_NUMBER:
      if (i < icstate.bcode->bcode_size + 1)
      {
// this just prints out the operand's name, and doesn't actually need to be called.
       if (!operand_text(bfix[i+1].fix_type, bfix[i+1].fix_value, icstate.bcode->op [i+1]))
        return 0;
      }
      operands_left ++;
      break;
    }
   } // end for j loop
  } // end if operands_left == 0

// end of ifdef ASM_PRINT
#endif

// reg_operand &= ADDRESS_MODE_MASK_REGISTER;
// int bitshift = operand * ADDRESS_MODE_BITFIELD_SIZE;

// return (reg_operand << bitshift);

  if (bfix[i].process != icstate.process_scope)
   icstate.process_scope = bfix[i].process; // do we need any other state changes here? not sure yet.

  switch(bfix[i].fix_type)
  {
   case FIX_NONE:
    break;
   case FIX_VARIABLE: // fix_value is the index of the identifier
    if (identifier[bfix[i].fix_value].address_bcode == -1)
     return inter_error_bfix(ICERR_VAR_NO_BCODE_ADDRESS, i);
    icstate.bcode->op [i] += identifier[bfix[i].fix_value].address_bcode - process_def[icstate.process_scope].start_address; // uses += in case the op has already been set up with an offset
// subprocess start offset already dealt with in allocate_static_memory
//    icstate.bcode->op [i] -= process_def[icstate.process_scope].start_address; // fix the address for a subprocess
    operands_left --;
    if (!operand_text(bfix[i].fix_type, bfix[i].fix_value, icstate.bcode->op [i]))
     return 0;
    break;
   case FIX_AUTO_VARIABLE_OFFSET: // fix_value is the index of the identifier
    if (identifier[bfix[i].fix_value].stack_offset == -1)
     return inter_error_bfix(ICERR_VAR_NO_STACK_OFFSET, i);
    icstate.bcode->op [i] += identifier[bfix[i].fix_value].stack_offset; // uses += in case the op has already been set up with an offset
    operands_left --;
    if (!operand_text(bfix[i].fix_type, bfix[i].fix_value, icstate.bcode->op [i]))
     return 0;
    break;
   case FIX_CFUNCTION: // fix value is the index of the cfunction
    if (process_def[icstate.process_scope].cfunction[bfix[i].fix_value].bcode_address == -1)
     return inter_error_bfix(ICERR_CFUNCTION_NO_BCODE_ADDRESS, i);
    icstate.bcode->op [i] += process_def[icstate.process_scope].cfunction[bfix[i].fix_value].bcode_address;
    icstate.bcode->op [i] -= process_def[icstate.process_scope].start_address; // fix the address for a subprocess
    operands_left --;
    if (!operand_text(bfix[i].fix_type, bfix[i].fix_value, 0))
     return 0;
    break;
   case FIX_LABEL:
// FIX_LABEL is used to fix the target address for a goto
// First we need to make sure that the label has been defined (as it's okay to goto a label before the label is defined):
    if (identifier[bfix[i].fix_value].type == ID_USER_LABEL_UNDEFINED)
     return inter_error_bfix(ICERR_LABEL_NOT_DEFINED, i);
    icstate.bcode->op [i] += identifier[bfix[i].fix_value].address_bcode; // accepts offset
    icstate.bcode->op [i] -= process_def[icstate.process_scope].start_address; // fix the address for a subprocess
//    fprintf(stdout, "\nfix label %s with target address %i result %i", identifier[bfix[i].fix_value].name, identifier[bfix[i].fix_value].address_bcode, icstate.bcode->op [i]);
    operands_left --;
    if (!operand_text(bfix[i].fix_type, bfix[i].fix_value, 0))
     return 0;
    break;
   case FIX_EXIT_POINT_TRUE:
    if (expoint[bfix[i].fix_value].true_point_bcode == -1
     || expoint[bfix[i].fix_value].true_point_used == 0)
     return inter_error_bfix(ICERR_NO_TRUE_POINT, i);
    icstate.bcode->op [i] += expoint[bfix[i].fix_value].true_point_bcode;
    icstate.bcode->op [i] -= process_def[icstate.process_scope].start_address; // fix the address for a subprocess
//    icstate.bcode->op [i] += expoint[bfix[i].fix_value].offset; // probably zero
    operands_left --;
    if (!operand_text(bfix[i].fix_type, bfix[i].fix_value, 0))
     return 0;
    break;
   case FIX_EXIT_POINT_FALSE:
    if (expoint[bfix[i].fix_value].false_point_bcode == -1
     || expoint[bfix[i].fix_value].false_point_used == 0)
     return inter_error_bfix(ICERR_NO_FALSE_POINT, i);
    icstate.bcode->op [i] += expoint[bfix[i].fix_value].false_point_bcode;
    icstate.bcode->op [i] -= process_def[icstate.process_scope].start_address; // fix the address for a subprocess
//    icstate.bcode->op [i] += expoint[bfix[i].fix_value].offset; // probably zero
    operands_left --;
    if (!operand_text(bfix[i].fix_type, bfix[i].fix_value, 0))
     return 0;
    break;
   case FIX_STACK_INIT: // this fix value is used only for the stack initialisation operation at the start of the main cfunction
    icstate.bcode->op [i] = process_def[icstate.process_scope].end_address;
    icstate.bcode->op [i] -= process_def[icstate.process_scope].start_address;
    operands_left --;
    if (!operand_text(bfix[i].fix_type, bfix[i].fix_value, icstate.bcode->op [i]))
     return 0;
    break;
   case FIX_LITERAL_NUMBER:
    operands_left --;
    if (!operand_text(bfix[i].fix_type, bfix[i].fix_value, icstate.bcode->op [i]))
     return 0;
    break;
   case FIX_PROCESS_START:
    if (process_def[bfix[i].fix_value].defined == 0)
    {
//     fprintf(stdout, "\nundefined process: %i %i %i", bfix[i].fix_value, process_def[bfix[i].fix_value].prototyped, process_def[bfix[i].fix_value].defined);
//     error_call();
     if (process_def[bfix[i].fix_value].id_index != -1)
     {
      start_log_line(MLOG_COL_ERROR);
      write_to_log("Process ");
      write_to_log(identifier[process_def[bfix[i].fix_value].id_index].name);
      write_to_log(" not defined.");
      finish_log_line();
     }
     return inter_error_bfix(ICERR_PROCESS_NOT_DEFINED, i);
    }
    icstate.bcode->op [i] += process_def[bfix[i].fix_value].start_address - process_def[icstate.process_scope].start_address;
// subtracting the start address allows nested process definitions.
// it won't work properly if a process outside the current process is referred to - probably should check for this.
    operands_left --;
    if (!operand_text(bfix[i].fix_type, bfix[i].fix_value, icstate.bcode->op [i]))
     return 0;
    break;
   case FIX_PROCESS_END:
    if (process_def[bfix[i].fix_value].defined == 0)
    {
     if (process_def[bfix[i].fix_value].id_index != -1)
     {
      start_log_line(MLOG_COL_ERROR);
      write_to_log("Process ");
      write_to_log(identifier[process_def[bfix[i].fix_value].id_index].name);
      write_to_log(" not defined.");
      finish_log_line();
     }
     return inter_error_bfix(ICERR_PROCESS_NOT_DEFINED, i);
    }
    icstate.bcode->op [i] += process_def[bfix[i].fix_value].end_address - process_def[icstate.process_scope].start_address;
    operands_left --;
    if (!operand_text(bfix[i].fix_type, bfix[i].fix_value, icstate.bcode->op [i]))
     return 0;
/*
     write_to_log("Process ");
//     write_number_to_log(intercode[ip].operand [0]);
     write_to_log(identifier[process_def[bfix[i].fix_value].id_index].name);
     write_to_log(" length ");
     write_number_to_log(process_def[bfix[i].fix_value].end_address - process_def[bfix[i].fix_value].start_address);
     write_to_log(".");
     finish_log_line();
*/
         break;
   case FIX_ASM_ADDRESS: // this can be any asm identifier: generic, aspace or nspace
    if (identifier[bfix[i].fix_value].type == ASM_ID_GENERIC_UNDEFINED
     || identifier[bfix[i].fix_value].address_bcode == -1)
    {
//     fprintf(stdout, "\nname %s index %i type %i aspace_scope %i nspace_scope %i", identifier[bfix[i].fix_value].name, bfix[i].fix_value,identifier[bfix[i].fix_value].type, identifier[bfix[i].fix_value].aspace_scope, identifier[bfix[i].fix_value].nspace_scope);
      start_log_line(MLOG_COL_ERROR);
      write_to_log("Not found: ");
      write_to_log(identifier[bfix[i].fix_value].name);
      write_to_log(".");
      finish_log_line();

     return inter_error_bfix(ICERR_ASM_ID_UNDEFINED, i);
    }
    icstate.bcode->op [i] += identifier[bfix[i].fix_value].address_bcode - astate.aspace[bfix[i].referring_aspace].bcode_start;// + astate.aspace[bfix[i].referred_aspace].bcode_start; // uses += in case the op has already been set up with an offset
//    fprintf(stdout, "\nname %s address %i aspace.bcode_start %i referring_aspace %i referred_aspace %i target %i", identifier[bfix[i].fix_value].name, identifier[bfix[i].fix_value].address_bcode, astate.aspace[bfix[i].referring_aspace].bcode_start, bfix[i].referring_aspace, bfix[i].referred_aspace, icstate.bcode->op [i]);
    operands_left --;
    if (!operand_text(bfix[i].fix_type, bfix[i].fix_value, icstate.bcode->op [i]))
     return 0;
    break;
   case FIX_ASM_ASPACE_END_ADDRESS:
    if (identifier[bfix[i].fix_value].type != ASM_ID_ASPACE)
     return inter_error_bfix(ICERR_ASM_ASPACE_ID_UNDEFINED, i);
//    fprintf(stdout, "\naspace %s index %i id_index %i aspace_index %i scope %i", identifier[bfix[i].fix_value].name, bfix[i].fix_value, identifier[bfix[i].fix_value].aspace_index, identifier[bfix[i].fix_value].aspace_index, identifier[bfix[i].fix_value].aspace_scope);
    if (astate.aspace [identifier[bfix[i].fix_value].aspace_index].bcode_end == -1)
     return inter_error_bfix(ICERR_ASM_ASPACE_HAS_NO_END, i);
    icstate.bcode->op [i] += astate.aspace [identifier[bfix[i].fix_value].aspace_index].bcode_end - astate.aspace[bfix[i].referring_aspace].bcode_start; // uses += in case the op has already been set up with an offset
    operands_left --;
    break;


  }

#ifdef ASM_PRINT
  if (i >= ASM_PRINT_START && i < ASM_PRINT_END)
  {
//   fprintf(stdout, "      ");
//   print_binary(icstate.bcode->op [i]);
  }
  for (j = 0; j < IDENTIFIER_MAX; j ++)
  {
   if (identifier[j].type != ID_NONE
    && identifier[j].address_bcode == i)
   {
     fprintf(stdout, " : %s [", identifier[j].name);
     print_binary(i);
     fprintf(stdout, "]");
   }
   if (identifier[j].type == ID_USER_CFUNCTION
    && identifier[j].process_scope == icstate.process_scope
    && process_def[icstate.process_scope].cfunction[identifier[j].cfunction_index].bcode_address == i)
   {
     fprintf(stdout, " : fn(%s) [", identifier[j].name);
     print_binary(i);
     fprintf(stdout, "]");
   }

  }
#endif
 }

 return 1;


}

// this is a debugging function used to print asm
// can be removed (along with all calls to it)
int operand_text(int fix_type, int fix_value, int number)
{

#ifndef ASM_PRINT
return 1;
#endif

  switch(fix_type)
  {
   case FIX_NONE:
    break;
   case FIX_VARIABLE: // fix_value is the index of the identifier
    if (identifier[fix_value].address_bcode == -1)
     return inter_error(ICERR_VAR_NO_BCODE_ADDRESS);
    fprintf(stdout, " %s", identifier[fix_value].name); // remember that the binary is printed before the actual value is fixed, so it will always be 0000000000000000
    if (number != 0)
     fprintf(stdout, "+%i", number); // remember that the binary is printed before the actual value is fixed, so it will always be 0000000000000000
    break;
   case FIX_AUTO_VARIABLE_OFFSET: // fix_value is the index of the identifier
    if (identifier[fix_value].stack_offset == -1)
     return inter_error(ICERR_VAR_NO_STACK_OFFSET);
    fprintf(stdout, " %s", identifier[fix_value].name); // remember that the binary is printed before the actual value is fixed, so it will always be 0000000000000000
    if (number != 0)
     fprintf(stdout, "+%i", number); // remember that the binary is printed before the actual value is fixed, so it will always be 0000000000000000
    break;
   case FIX_CFUNCTION: // fix value is the index of the cfunction
    if (process_def[icstate.process_scope].cfunction[fix_value].bcode_address == -1)
     return inter_error(ICERR_CFUNCTION_NO_BCODE_ADDRESS);
    fprintf(stdout, " %s()", identifier[process_def[icstate.process_scope].cfunction[fix_value].identifier_index].name);
    break;
   case FIX_LABEL:
    if (identifier[fix_value].address_bcode == -1)
     return inter_error(ICERR_LABEL_NOT_DEFINED);
    fprintf(stdout, " %s", identifier[fix_value].name); // remember that the binary is printed before the actual value is fixed, so it will always be 0000000000000000
    break;
   case FIX_EXIT_POINT_TRUE:
    if (expoint[fix_value].true_point_bcode == -1)
     return inter_error(ICERR_NO_TRUE_POINT); // this check (and similar checks following) duplicate other checks in fix_bcode
    fprintf(stdout, " (TX %i)", expoint[fix_value].true_point_bcode);
    break;
   case FIX_EXIT_POINT_FALSE:
    if (expoint[fix_value].false_point_bcode == -1)
     return inter_error(ICERR_NO_FALSE_POINT);
    fprintf(stdout, " (FX %i)", expoint[fix_value].false_point_bcode);
    break;
   case FIX_STACK_INIT: // this fix value is used only for the stack initialisation operation at the start of the main cfunction
    fprintf(stdout, " (stack base %i)", number);
    break;
   case FIX_LITERAL_NUMBER:
    fprintf(stdout, " %i", number);
    break;
   case FIX_PROCESS_START:
    fprintf(stdout, " (process_start %i: %s)", fix_value, identifier[process_def[fix_value].id_index].name);
    break;
   case FIX_PROCESS_END:
    fprintf(stdout, " (process_end %i: %s)", fix_value, identifier[process_def[fix_value].id_index].name);
    break;

  }

 return 1;

}


// adds "asm_only asm {" to start of asm source when running "build asm"
int target_source_asm_header(void)
{
 if (!target_source_write("asm_only", 0))
  return 0;
 if (!target_source_newline())
  return 0;
 if (!target_source_newline())
  return 0;
 if (!target_source_write("asm", 0))
  return 0;
 if (!target_source_newline())
  return 0;
 if (!target_source_write("{", 0))
  return 0;
// if (!target_source_newline())
//  return 0;

// fprintf(stdout, "\nasm header written");

 return 1;
}

int target_source_newline(void)
{
 icstate.target_src_line ++;
 icstate.target_src_line_length = 0;
 if (icstate.target_src_line >= SOURCE_TEXT_LINES)
 {
/*  int i;
  for (i = 0; i < SOURCE_TEXT_LINES; i ++)
  {
    fprintf(stdout, "\n%i- %s", i, icstate.target_source->text [i]);
  }*/
//  fprintf(stdout, "\nSize: %i lines %i", icstate.target_src_line, icstate.target_src_line_length);
  return inter_error(ICERR_GENERATED_SOURCE_TOO_LONG);
 }

 icstate.target_source->text [icstate.target_src_line] [0] = '\0';

 return 1;
}

int target_source_write(const char* str, int add_space)
{

#ifdef SANITY_CHECK
 if (icstate.target_source == NULL)
 {
  fprintf(stdout, "\nError in c_inter.c: tried to write string to NULL target source.");
  error_call();
 };
#endif

 if (strlen(str) + strlen(icstate.target_source->text [icstate.target_src_line]) + (add_space) >= SOURCE_TEXT_LINE_LENGTH)
  return inter_error(ICERR_GENERATED_SOURCE_LINE_TOO_LONG);

 strcat(icstate.target_source->text [icstate.target_src_line], str);
 if (add_space)
  strcat(icstate.target_source->text [icstate.target_src_line], " ");
 icstate.target_src_line_length = strlen(icstate.target_source->text [icstate.target_src_line]);
 return 1;

}

// This is used when generating crunched asm
// It adds a space to the current line only if:
//  - the line is longer than zero; and
//  - the end of the line is not currently a space
int add_space_if_needed(void)
{

#ifdef SANITY_CHECK
 if (icstate.target_source == NULL)
 {
  fprintf(stdout, "\nError in c_inter.c: tried to write string to NULL target source.");
  error_call();
 };
#endif

 if (icstate.target_source->text [icstate.target_src_line] [0] == '\0')
		return 1; // don't need to do anything

 if (strlen(icstate.target_source->text [icstate.target_src_line]) >= SOURCE_TEXT_LINE_LENGTH - 1)
		return inter_error(ICERR_GENERATED_SOURCE_LINE_TOO_LONG);

 strcat(icstate.target_source->text [icstate.target_src_line], " ");
 icstate.target_src_line_length ++;

 return 1;

}

int target_source_write_num(s16b num)
{

#ifdef SANITY_CHECK
 if (icstate.target_source == NULL)
 {
  fprintf(stdout, "\nError in c_inter.c: tried to write string to NULL target source.");
  error_call();
 };
#endif

 char temp_str [TEMPSTR_LENGTH];

 sprintf(temp_str, "%i", num);

 if (strlen(temp_str) + strlen(icstate.target_source->text [icstate.target_src_line]) >= SOURCE_TEXT_LINE_LENGTH)
  return inter_error(ICERR_GENERATED_SOURCE_LINE_TOO_LONG);

 strcat(icstate.target_source->text [icstate.target_src_line], temp_str);
 icstate.target_src_line_length = strlen(icstate.target_source->text [icstate.target_src_line]);
 return 1;

}

// this function writes an offset after an operand (e.g. the +1 in "setrn A address+1")
int target_source_write_offset(s16b offset)
{
 if (offset == 0)
  return 1; // nothing to do

 if (offset < 0)
 {
  if (!target_source_write("-", 0))
   return 0;
  offset *= -1;
 }
  else
  {
   if (!target_source_write("+", 0))
    return 0;
  }

 if (!target_source_write_num(offset))
  return 0;

 return 1;

}


/*
// This function returns the current icstate.icode_pos
// It's used with modify_intercode_operand() to allow functions in c_comp.c to go back and modify intercode written earlier
// a bit hacky
int get_current_intercode_index(void)
{
 return icstate.icode_pos;
}

 - not used, as it was wrong - it's actually cstate.icode_pos (not icstate) that holds the intercode index during c_comp.c compilation

*/


void modify_intercode_operand(int intercode_index, int operand, int new_value)
{
#ifdef SANITY_CHECK
 if (intercode_index < 0
  || intercode_index >= INTERCODE_SIZE)
 {
  fprintf(stdout, "\n Error: c_inter.c: modify_intercode_operand(): intercode_index invalid (%i, %i, %i)", intercode_index, operand, new_value);
  error_call();
 }
#endif
// fprintf(stdout, "\nmodify_intercode_operand(%i, %i, %i) current value %i", intercode_index, operand, new_value, intercode[intercode_index].operand [operand]);
 intercode[intercode_index].operand [operand] = new_value;
// Remember that this function modifies an intercode entry, not a bcode instruction!!
// A single intercode entry may produce multiple bcode instructions
}




/*
Call this function when the end of a process definition is reached.
It adds enough space for all static variables to be stored at the end of the process's bcode,
then sets process_def[].end_address to wherever bcode_pos ends up.
end_address is used to set the start of the stack.

assumes icstate.process_scope is correct.

returns 1 on success, 0 on error
*/
int allocate_static_memory(void)
{

 if (process_def[icstate.process_scope].asm_only)
  return 1;

 int i, storage_size;
 int j;
 icstate.current_nspace = -1; // for asm generation. namespace -1 is global scope (i.e. not in a function) in the current aspace, which we assume to be the aspace for the current process (behaviour undefined if this is wrong, which should only happen if the aspace has been changed in inline asm)

// first, we need to allocate some space at the end of the bcode already written, but before the stack.
// this space is used for all static variables (global and local) in the current process:
//  (note: it doesn't deal with variables used or defined in inline asm code)
 for (i = 0; i < IDENTIFIER_MAX; i ++)
 {
  if (identifier[i].type == ID_USER_INT
   && (identifier[i].asm_or_compiler == IDENTIFIER_C_ONLY || identifier[i].asm_or_compiler == IDENTIFIER_ASM_OR_C)
   && identifier[i].storage_class == STORAGE_STATIC
   && identifier[i].process_scope == icstate.process_scope)
  {
    storage_size = 1;
    if (identifier[i].array_dims > 0)
     storage_size = identifier[i].array_element_size [0] * identifier[i].array_dim_size [0]; // the element size of the first dimension includes the size of any subdimensions
    if (icstate.bcode_pos + storage_size >= icstate.bcode->bcode_size - 2)
     return inter_error(ICERR_STATIC_VAR_FAIL);
// set the variable's bcode address, subtracting the start address of the current process (0 for main process)
    identifier[i].address_bcode = icstate.bcode_pos;// - process_def[icstate.process_scope].start_address;
//    fprintf(stdout, "\nMaking space for variable %s at bcode address %i (size %i) (actual address %i).", identifier[i].name, icstate.bcode_pos, storage_size, icstate.bcode_pos + process_def[icstate.process_scope].start_address);
    if (icstate.generate_asm)
    {
// if the variable's namespace is different from the current nspace, need to change nspace:
     if (identifier[i].nspace_scope != icstate.current_nspace)
     {
      if (!target_source_newline())
       return 0;
      if (identifier[i].nspace_scope == -1)
      {
// return to global nspace
       if (!target_source_write("nspace_end", 0))
        return 0;
       icstate.current_nspace = -1;
      }
       else
       {
// change nspace to match identifier's nspace:
// But first we need to deal with the possibility that the variable is a local variable of a cfunction that was prototyped but not defined.
// Probably we should just not initialise these, but for now we treat them as a new nspace.
        if (identifier[i].scope != -1
         && process_def[icstate.process_scope].cfunction [identifier[i].scope].defined == 0)
        {
         if (!target_source_write("def nspace ", 0))
          return 0;
        }
         else
         {
          if (!target_source_write("nspace_rejoin ", 0))
           return 0;
         }
        if (!target_source_write(identifier[astate.nspace[identifier[i].nspace_scope].identifier_index].name, 0))
         return 0;
        icstate.current_nspace = identifier[i].nspace_scope;
       }
     }
    }
// now initialise variables that are arrays
    if (identifier[i].array_dims > 0)
    {
//
     if (icstate.generate_asm)
     {
      if (!target_source_newline())
       return 0;
      if (!target_source_write("def int ", 0))
       return 0;
      if (!target_source_write(identifier[i].name, 1))
       return 0;
// since asm code doesn't recognise arrays, all this def command does is allocate a single address for the variable
// so we need to expressly initialise each element to make sure enough space is left:
      if (!target_source_write("{", 0))
       return 0;
      for (j = 0; j < storage_size; j ++)
      {
       if (icstate.target_src_line_length >= SOURCE_TEXT_LINE_LENGTH - 20) //  make sure there's space
       {
        if (!target_source_newline())
         return 0;
       }
       if (identifier[i].static_array_initialised)
       {
        if (!target_source_write_num(array_init.value [identifier[i].static_array_init_offset + j]))
         return 0;
       }
        else
        {
         if (!target_source_write_num(0))
          return 0;
        }
       if (j < storage_size - 1)
       {
        if (!target_source_write(", ", 0))
         return 0;
       }
      }
      if (!target_source_write("}", 0))
       return 0;
     }
// okay, enough asm generation. Back to compiling bcode:
     if (identifier[i].static_array_initialised) // an array is not necessarily expressly initialised on definition (if it isn't, it will simply use the default values for each address - i.e. zero)
     {
//      fprintf(stdout, "\nArray initialisation: %s size %i offset %i", identifier[i].name, storage_size, identifier[i].static_array_init_offset);
      for (j = 0; j < storage_size; j ++)
      {
       icstate.bcode->op [icstate.bcode_pos] = array_init.value [identifier[i].static_array_init_offset + j];
       icstate.bcode_pos ++;
       if (icstate.bcode_pos + storage_size >= icstate.bcode->bcode_size - 2)
        return inter_error(ICERR_STATIC_VAR_FAIL);
// shouldn't have to bounds-check array_init.value as it would have been checked when array initialised
      }
     }
      else
       icstate.bcode_pos += storage_size;
    }
     else // must not be an array
     {
        icstate.bcode->op [icstate.bcode_pos] = identifier[i].initial_value;
        icstate.bcode_pos += storage_size;
        if (icstate.generate_asm)
        {
         if (!target_source_newline())
          return 0;
         if (!target_source_write("def int ", 0))
          return 0;
         if (!target_source_write(identifier[i].name, 1))
          return 0;
         if (!target_source_write("{", 0))
          return 0;
         if (!target_source_write_num(identifier[i].initial_value))
          return 0;
         if (!target_source_write("}", 1))
          return 0;
        }
     }
  }
 }

 return 1;

}

const char *ic_error_name [ICERRS] =
{
"bcode too large", // ICERR_ADD_FAIL,
"no space in bcode for static variable storage", // ICERR_STATIC_VAR_FAIL,
"variable has no address specified", // ICERR_VAR_NO_BCODE_ADDRESS,
"cfunction has no address specified", // ICERR_CFUNCTION_NO_BCODE_ADDRESS,
"missing true exit point", // ICERR_NO_TRUE_POINT,
"missing false exit point", // ICERR_NO_FALSE_POINT,
"process without interface definition", // ICERR_INTERFACE_NOT_DEFINED
"undefined process", // ICERR_PROCESS_NOT_DEFINED
"label not defined", // ICERR_LABEL_NOT_DEFINED - this error is more likely than most of the others to occur
"automatic variable has no stack offset specified", // ICERR_VAR_NO_STACK_OFFSET - not sure that this is possible
"generated source file is too long", // ICERR_GENERATED_SOURCE_TOO_LONG,
"line of generated source file is too long", // ICERR_GENERATED_SOURCE_LINE_TOO_LONG,
"identifier not defined in this address space or name space", // ICERR_ASM_ID_UNDEFINED
"aspace definition has no end?", // ICERR_ASM_ASPACE_HAS_NO_END
"expected name of aspace", // ICERR_EXPECTED_ASPACE_NAME
"aspace name not defined in this scope", // ICERR_ASM_ASPACE_ID_UNDEFINED
"process too large", // ICERR_PROCESS_TOO_LARGE
};

int inter_error(int error_type)
{

     start_log_line(MLOG_COL_ERROR);
     write_to_log("Code generation error at line ");
     write_number_to_log(intercode[icstate.icode_pos].src_line + 1);
     if (intercode[icstate.icode_pos].src_file == 0)
      write_to_log(" of file ");
       else
        write_to_log(" of included file ");
     write_to_log(scode->src_file_name [intercode[icstate.icode_pos].src_file]);
     write_to_log(".");
     finish_log_line();

     start_log_line(MLOG_COL_ERROR);
     write_to_log("Error: ");
     write_to_log(ic_error_name [error_type]);
     write_to_log(".");
     finish_log_line();



//     fprintf(stdout, "\nIntercode processor: error (%s) at line %i icode pos %i.\n", ic_error_name [error_type], intercode[icstate.icode_pos].src_line, icstate.icode_pos);
     icstate.error = error_type;
//     error_call();
     return 0;
}


// like inter_error but src_line and src_file are specified rather than relying on icstate.icode_pos
// (used e.g. through fix-up pass that sets values)
int inter_error_bfix(int error_type, int bfix_index)
{

     start_log_line(MLOG_COL_ERROR);
     write_to_log("Code generation error at line ");
     write_number_to_log(bfix[bfix_index].src_line + 1);
     if (bfix[bfix_index].src_file == -1)
      write_to_log(" of file");
       else
       {
        if (bfix[bfix_index].src_file == 0)
         write_to_log(" of file ");
          else
           write_to_log(" of included file ");
        write_to_log(scode->src_file_name [bfix[bfix_index].src_file]);
       }
     write_to_log(".");
     finish_log_line();

     start_log_line(MLOG_COL_ERROR);
     write_to_log("Error: ");
     write_to_log(ic_error_name [error_type]);
     write_to_log(".");
     finish_log_line();


//     fprintf(stdout, "\nIntercode processor: error (%s) at line %i icode pos %i.\n", ic_error_name [error_type], intercode[icstate.icode_pos].src_line, icstate.icode_pos);
     icstate.error = error_type;
//     error_call();
     return 0;
}


