

#include <allegro5/allegro.h>
#include "m_config.h"
#include "g_header.h"
#include "m_globvars.h"
#include "c_header.h"

#include "c_prepr.h"

#include "g_misc.h"
#include "c_ctoken.h"
#include "c_comp.h"
#include "e_slider.h"
#include "e_header.h"
#include "e_log.h"
#include "c_asm.h"

#include "v_interp.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define CODE_MAX 512


extern struct scodestruct *scode; // in c_comp.c

struct identifierstruct identifier [IDENTIFIER_MAX];


static int parse_asm_keyword(struct ctokenstruct* ctoken);
static int enter_aspace(int a_index);
//static int enter_nspace(int n_index);
static int asm_memory_init(void);
static int parse_asm_expression_offset(struct ctokenstruct* ctoken);
static int parse_asm_scope(int asm_intercode_position);
static int asm_get_next_char_from_scode(void);
static void fix_generic_aliases(int id_index);

//void test_precode(struct precodestruct* precode);



enum
{
ASMERR_REACHED_END,
ASMERR_UNRECOGNISED_INSTRUCTION,
ASMERR_EXPECTED_REGISTER_OPERAND,
ASMERR_METHOD_OPERAND_MUST_BE_CONSTANT,
ASMERR_MBANK_OPERAND_MUST_BE_CONSTANT,
ASMERR_METHOD_OPERAND_OUT_OF_BOUNDS,
ASMERR_MBANK_OPERAND_OUT_OF_BOUNDS,
ASMERR_NUMBER_OPERAND_MUST_BE_CONSTANT,
ASMERR_CANNOT_USE_AUTO_VALUE,
ASMERR_EXPECTED_CONSTANT_OR_EXPRESSION,
ASMERR_SYNTAX_ERROR,
ASMERR_OUT_OF_INTERCODE,
ASMERR_EXPECTED_INT_DECLARATION,
ASMERR_EXPECTED_GENERIC_ASM_IDENTIFIER,
ASMERR_ASPACE_NAME,
ASMERR_ASPACE_ALREADY_DEFINED,
ASMERR_NSPACE_ALREADY_DEFINED,
ASMERR_NSPACE_NAME,
ASMERR_UNEXPECTED_WORD,
ASMERR_ALLOCATE_ASPACE_FAILED,
ASMERR_ALLOCATE_NSPACE_FAILED,
ASMERR_EXPECTED_COMMA_AFTER_EXPRESSION,
ASMERR_CANNOT_END_FILE_ASPACE,
ASMERR_EXPECTED_NSPACE_NAME,
ASMERR_CANNOT_END_TOP_NSPACE,
ASMERR_IDENTIFIER_ALREADY_DEFINED,
ASMERR_CANNOT_ALLOCATE_IDENTIFIER,
ASMERR_EXPECTED_BRACKET_OPEN,
ASMERR_EXPECTED_SCOPE_INDICATOR,
ASMERR_EXPECTED_ADDRESS_ID_AFTER_NSPACE,
ASMERR_EXPECTED_BRACKET_CLOSE,
ASMERR_STRING_TOO_LONG,
ASMERR_EXPECTED_CLOSING_BRACE_AFTER_STRING,
ASMERR_UNRESOLVED_GENERIC_REFERENCE,
ASMERRS

};



struct astatestruct astate;

extern struct cstatestruct cstate;
extern struct process_defstruct process_def [PROCESS_DEFS];


const struct opcode_propertiesstruct opcode_properties [OP_end] =
{
// OPERAND_TYPE_REGISTER is a register. Which register it is is specified in the address bits of the instruction. The cost of using a register is covered by the cost of the operation.
// OPERAND_TYPE_NUMBER means that the instruction op is followed by a memory entry with the number to be used in it.
// OPERAND_TYPE_ADDRESS is like number, but is dereferenced when used (i.e. the number in the following memory entry is used as a pointer to the address where the actual operation occurs). Is used for jumps etc.

{"nop", 0}, // OP_nop, // 0 nop
// set operations.
// these take operands of types indicated by the op name
// first operand is target
// the "set" command in asm code is expanded by the assembler into the appropriate version depending on operand type
{"setrr", 2, {OPERAND_TYPE_REGISTER, OPERAND_TYPE_REGISTER}}, // OP_setrr
{"setra", 2, {OPERAND_TYPE_REGISTER, OPERAND_TYPE_ADDRESS}}, // the m means an address in memory (indicated by a number immediately following the instruction)
{"setar", 2, {OPERAND_TYPE_ADDRESS, OPERAND_TYPE_REGISTER}}, //
//{"setaa", 2, {OPERAND_TYPE_ADDRESS, OPERAND_TYPE_ADDRESS}}, // has overhead
{"setrn", 2, {OPERAND_TYPE_REGISTER, OPERAND_TYPE_NUMBER}}, // the n means a number immediately following the instruction
//{"setan", 2, {OPERAND_TYPE_ADDRESS, OPERAND_TYPE_NUMBER}}, // the n means a number immediately following the instruction
{"setrd", 2, {OPERAND_TYPE_REGISTER, OPERAND_TYPE_REGISTER}}, // the d means a dereferenced register. This instruction copies the contents of the memory address pointed to by register d into register r (may be the same)
{"setdr", 2, {OPERAND_TYPE_REGISTER, OPERAND_TYPE_REGISTER}}, // does the reverse

{"add", 3, {OPERAND_TYPE_REGISTER, OPERAND_TYPE_REGISTER, OPERAND_TYPE_REGISTER}}, // OP_add, // 3
{"sub", 3, {OPERAND_TYPE_REGISTER, OPERAND_TYPE_REGISTER, OPERAND_TYPE_REGISTER}}, // OP_sub, // 3 sub A B C means A = B - C. Registers can be the same, e.g. sub A B A. Division works the same way.
{"mul", 3, {OPERAND_TYPE_REGISTER, OPERAND_TYPE_REGISTER, OPERAND_TYPE_REGISTER}}, //
{"div", 3, {OPERAND_TYPE_REGISTER, OPERAND_TYPE_REGISTER, OPERAND_TYPE_REGISTER}}, //
{"mod", 3, {OPERAND_TYPE_REGISTER, OPERAND_TYPE_REGISTER, OPERAND_TYPE_REGISTER}}, //
{"incr", 1, {OPERAND_TYPE_REGISTER}}, //
{"decr", 1, {OPERAND_TYPE_REGISTER}}, //

// bitwise operators are treated in the same way as arithmetic ones
{"and", 3, {OPERAND_TYPE_REGISTER, OPERAND_TYPE_REGISTER, OPERAND_TYPE_REGISTER}}, //
{"or", 3, {OPERAND_TYPE_REGISTER, OPERAND_TYPE_REGISTER, OPERAND_TYPE_REGISTER}}, //
{"xor", 3, {OPERAND_TYPE_REGISTER, OPERAND_TYPE_REGISTER, OPERAND_TYPE_REGISTER}}, //

// binary operations
{"bnot", 2, {OPERAND_TYPE_REGISTER, OPERAND_TYPE_REGISTER}}, // equivalent to ~
{"bshiftl", 3, {OPERAND_TYPE_REGISTER, OPERAND_TYPE_REGISTER, OPERAND_TYPE_REGISTER}}, //
{"bshiftr", 3, {OPERAND_TYPE_REGISTER, OPERAND_TYPE_REGISTER, OPERAND_TYPE_REGISTER}}, //

// logical not
{"not", 2, {OPERAND_TYPE_REGISTER, OPERAND_TYPE_REGISTER}}, // if op2 is true (i.e. not zero; can be +ve or -ve), op1 is set to zero. if op2 is false, op1 is set to 1.

// branching
{"jumpr", 1, {OPERAND_TYPE_REGISTER}}, // OP_jump, // 1 jump [destination]
{"jumpn", 1, {OPERAND_TYPE_NUMBER}}, // OP_jump, // 1 jump [destination]
{"iftrue", 2, {OPERAND_TYPE_REGISTER, OPERAND_TYPE_ADDRESS}}, // OP_iftrue, // 2 iftrue [value] [jump destination if true]
{"iffalse", 2, {OPERAND_TYPE_REGISTER, OPERAND_TYPE_ADDRESS}}, // OP_iffalse, // 2 iffalse [value] [jump destination if false]

{"cmpeq", 2, {OPERAND_TYPE_REGISTER, OPERAND_TYPE_REGISTER}}, // OP_ifeq, // 2 ifeq [value1] [value2] (sets value 0 to 1 or 0 truth value of comparison) (all comparisons work the same way)
{"cmpneq", 2, {OPERAND_TYPE_REGISTER, OPERAND_TYPE_REGISTER}}, // OP_ifneq, // 2
{"cmpgr", 2, {OPERAND_TYPE_REGISTER, OPERAND_TYPE_REGISTER}}, // OP_ifgr, // 2
{"cmpgre", 2, {OPERAND_TYPE_REGISTER, OPERAND_TYPE_REGISTER}}, // OP_ifgre, // 2
{"cmpls", 2, {OPERAND_TYPE_REGISTER, OPERAND_TYPE_REGISTER}}, // OP_ifls, // 2
{"cmplse", 2, {OPERAND_TYPE_REGISTER, OPERAND_TYPE_REGISTER}}, // OP_iflse, // 2
{"exit", 0, {}}, // OP_exit, // 0 exit

{"push", 2, {OPERAND_TYPE_REGISTER, OPERAND_TYPE_REGISTER}}, // OP_push, // 2 push [source] [stack pointer] - compound instruction: increments stack pointer, then copies source to address held by stack pointer
{"pop", 2, {OPERAND_TYPE_REGISTER, OPERAND_TYPE_REGISTER}}, // OP_pop, // 2 pop [target] [stack pointer] - compound instruction: copies value in address held by stack pointer to target, then decrements stack pointer

{"putbr", 2, {OPERAND_TYPE_MBANK, OPERAND_TYPE_REGISTER}}, //
{"putbn", 2, {OPERAND_TYPE_MBANK, OPERAND_TYPE_NUMBER}}, //
{"putba", 2, {OPERAND_TYPE_MBANK, OPERAND_TYPE_ADDRESS}}, //
{"putrr", 2, {OPERAND_TYPE_REGISTER, OPERAND_TYPE_REGISTER}}, //
{"putrn", 2, {OPERAND_TYPE_REGISTER, OPERAND_TYPE_NUMBER}}, //
{"putra", 2, {OPERAND_TYPE_REGISTER, OPERAND_TYPE_ADDRESS}}, //

{"getrb", 2, {OPERAND_TYPE_REGISTER, OPERAND_TYPE_MBANK}}, //
{"getrr", 2, {OPERAND_TYPE_REGISTER, OPERAND_TYPE_REGISTER}}, //

{"callmr", 2, {OPERAND_TYPE_METHOD, OPERAND_TYPE_REGISTER}}, // m is method indicated by certain bits of the instruction
{"callrr", 2, {OPERAND_TYPE_REGISTER, OPERAND_TYPE_REGISTER}}, // method indicated by register
//{"callnr", 1, {OPERAND_TYPE_METHOD}}, // method indicated by next bcode entry. Not currently used

{"prints", 0,  {}}, // OP_prints - special print command for debugging. prints a string
{"printr", 1,  {OPERAND_TYPE_REGISTER}}, // OP_printr - special print command for debugging. prints an int from register
{"printsa", 1,  {OPERAND_TYPE_ADDRESS}}, // OP_printsa - special print command for debugging. prints a string from an address
{"printsr", 1,  {OPERAND_TYPE_REGISTER}}, // OP_printsr - special print command for debugging. prints a string from an address pointed to by a register

//{"printa", 1,  {OPERAND_TYPE_ADDRESS}}, // OP_printa - special print command for debugging. prints an int from memory

};

#define ASM_INTERCODE_SIZE 5
// asm_intercode is a small temporary intercode store that is written to the main intercode array after each opcode is parsed.
struct intercodestruct asm_intercode [ASM_INTERCODE_SIZE];

//void add_intercode_asm_opcode(int instruction);
static int read_asm_instruction(struct ctokenstruct* ctoken);
static int parse_asm_expression(int asm_intercode_position);

static void add_intercode_asm_opcode(int instruction);
static void add_intercode_asm_operand(int ic_type, int operand1, int operand2, int operand3, int asm_intercode_index);
static int add_asm_intercode_to_intercode(int extra_entries);
static int parse_asm_operands(struct ctokenstruct* ctoken);

static int asm_error(int error_type, struct ctokenstruct* ctoken);

// This function initialises the assembler.
// only call it at the start of compilation (and not each time the assembler is called)
void init_assembler(int bcode_size)
{

 int i;

 for (i = 0; i < ASPACES; i ++)
 {
  astate.aspace [i].declared = 0;
  astate.aspace [i].defined = 0;
  astate.aspace [i].bcode_start = -1;
  astate.aspace [i].bcode_end = -1;
  astate.aspace [i].identifier_index = -1;
 }

 for (i = 0; i < NSPACES; i ++)
 {
  astate.nspace [i].declared = 0;
  astate.nspace [i].defined = 0;
 }

// set up file-level aspace:
 astate.aspace [0].declared = 1;
 astate.aspace [0].defined = 1;
 astate.aspace [0].bcode_start = 0;
 astate.aspace [0].bcode_end = bcode_size - 1; // I think this is right (it may be reset later)
 astate.aspace [0].parent = -1;
 astate.aspace [0].identifier_index = -1; // reset in ac_init.c to the index for the aspace's "self" identifier
 astate.aspace [0].corresponding_process = 0; // process_def[0] is set up in ac_init.c

 astate.current_aspace = 0;
 astate.current_nspace = -1;

}


// Call this function when the asm keyword is encountered in scode
//  (so it requires the preprocessor to have already converted source to scode)
// Returns 1 on success
// Returns 0 on failure (includes where end of file reached - this should not happen within assembler, which should be terminated by a }
int assemble(int asm_only_process)
{

 cstate.running_asm = 1;

 struct ctokenstruct ctoken;

 astate.scode = scode; // not sure this is actually used
 asm_intercode [0].instr = IC_END;

 astate.current_aspace = process_def[cstate.process_scope].corresponding_aspace;
 if (cstate.scope == SCOPE_GLOBAL) // in global scope, so not in a specific nspace
  astate.current_nspace = -1;
   else
    astate.current_nspace = process_def[cstate.process_scope].cfunction [cstate.scope].corresponding_nspace;

// int instr;

 while(TRUE)
 {


// check for end of asm code
  if (check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACE_CLOSE))
   break;

// check for memory initialisation sequence
  if (check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACE_OPEN))
  {
   if (!asm_memory_init())
    return 0;
   continue;
  }

  if (!read_asm_instruction(&ctoken))
   return 0; // error

  switch(ctoken.type)
  {
// can assume that ctoken contains an opcode or an asm keyword, as this is checked in read_asm_instruction (although the keyword may not be a valid instruction)
   case CTOKEN_TYPE_ASM_OPCODE:
    if (!parse_asm_operands(&ctoken))
     return 0;
    break;
   case CTOKEN_TYPE_ASM_KEYWORD:
    if (!parse_asm_keyword(&ctoken))
     return 0;
    break;

  }

 };

// If this is an asm_only process which is not the file's main process, we need to add an implicit aspace_end
//  so that the process' implicit aspace has a defined end:
 if (asm_only_process == 1
  && astate.current_aspace != 0)
 {
   strcpy(ctoken.name, "(empty)");
   if (!add_intercode(IC_ASM_END_ASPACE, astate.current_aspace, 0, 0))
    return asm_error(ASMERR_OUT_OF_INTERCODE, &ctoken);
 }


 cstate.running_asm = 0;
 return 1; // finished!

}


// reads the next instruction
// updates astate.scode_pos
// returns 1 on success, 0 on failure
static int read_asm_instruction(struct ctokenstruct* ctoken)
{

 if (!read_next(ctoken))
  return asm_error(ASMERR_REACHED_END, ctoken);

 if (ctoken->type != CTOKEN_TYPE_ASM_KEYWORD
  && ctoken->type != CTOKEN_TYPE_ASM_OPCODE)
   return asm_error(ASMERR_UNRECOGNISED_INSTRUCTION, ctoken);

 return 1;

}

// call this function after an opcode has been read into ctoken. Assumes that ctoken contains an opcode.
// reads in operands and writes to intercode.
// returns 1 on success, 0 on failure.
static int parse_asm_operands(struct ctokenstruct* ctoken)
{
 int opcode = ctoken->identifier_index;
 int instruction = opcode << OPCODE_BITSHIFT;

 struct ctokenstruct operand_ctoken;
 strcpy(operand_ctoken.name, "(empty)");

 int i;
 int operand;
 int retval;
 int operand_bit_position = 0;

 int asm_intercode_extra_entries = 0; // the number of asm_intercode entries in addition to the main instruction.

 for (i = 0; i < opcode_properties [opcode].operands; i ++)
 {

  switch(opcode_properties[opcode].operand_type [i])
  {
   case OPERAND_TYPE_REGISTER:
// accept only A, B, C etc.
    if (!read_next(&operand_ctoken))
     return asm_error(ASMERR_REACHED_END, ctoken);
    if (operand_ctoken.type != CTOKEN_TYPE_ASM_KEYWORD
     || operand_ctoken.identifier_index < KEYWORD_ASM_REG_A
     || operand_ctoken.identifier_index > KEYWORD_ASM_REG_H)
      return asm_error(ASMERR_EXPECTED_REGISTER_OPERAND, &operand_ctoken);
    operand = operand_ctoken.identifier_index - KEYWORD_ASM_REG_A;
//    if (opcode == OP_setar)
    instruction |= operand << (ADDRESS_MODE_BITFIELD_SIZE * operand_bit_position);
    operand_bit_position ++;
    break;
   case OPERAND_TYPE_METHOD:
// accept only a constant or an expression that evaluates to a constant (which is what read_literal_numbers does).
    retval = read_literal_numbers(&operand_ctoken);
    if (retval != 2) // 2 means that the expression resolved to a constant
     return asm_error(ASMERR_METHOD_OPERAND_MUST_BE_CONSTANT, &operand_ctoken);
// operand_ctoken.number_value holds the constant:
    operand = operand_ctoken.number_value;
    if (operand < 0 || operand >= METHODS)
     return asm_error(ASMERR_METHOD_OPERAND_OUT_OF_BOUNDS, &operand_ctoken);
    instruction |= operand << (ADDRESS_MODE_BITFIELD_SIZE * i);
    operand_bit_position += 2;
    break;
   case OPERAND_TYPE_MBANK:
// accept only a constant or an expression that evaluates to a constant (which is what read_literal_numbers does).
    retval = read_literal_numbers(&operand_ctoken);
    if (retval != 2) // 2 means that the expression resolved to a constant
     return asm_error(ASMERR_MBANK_OPERAND_MUST_BE_CONSTANT, &operand_ctoken);
// operand_ctoken.number_value holds the constant:
    operand = operand_ctoken.number_value;
    if (operand < 0 || operand >= METHOD_BANK)
     return asm_error(ASMERR_MBANK_OPERAND_OUT_OF_BOUNDS, &operand_ctoken);
    instruction |= operand << (ADDRESS_MODE_BITFIELD_SIZE * i);
    operand_bit_position += 2;
    break;
   case OPERAND_TYPE_NUMBER:
// actually this should accept the same things as OPERAND_TYPE_ADDRESS as both need to be able to use numbers, variables etc
    asm_intercode_extra_entries ++;
    if (!parse_asm_expression(asm_intercode_extra_entries))
     return 0;
    break;
/*
// accept only a constant or an expression that evaluates to a constant (which is what read_literal_numbers does).
    retval = read_literal_numbers(&operand_ctoken);
    if (retval != 2) // 2 means that the expression resolved to a constant
     return asm_error(ASMERR_NUMBER_OPERAND_MUST_BE_CONSTANT, &operand_ctoken);
// operand_ctoken.number_value holds the constant:
    operand = operand_ctoken.number_value;
    add_intercode_asm_operand(IC_ASM_OPERAND_NUMBER, operand, 0, 0, asm_intercode_extra_entries);
    asm_intercode_extra_entries ++;*/

    break;
   case OPERAND_TYPE_ADDRESS:
    asm_intercode_extra_entries ++;
    if (!parse_asm_expression(asm_intercode_extra_entries))
     return 0;
    break;

  }

 }

// now put the instruction into asm_intercode[0]
 add_intercode_asm_opcode(instruction);

// and write the new asm_intercode entries to the main intercode array.
 if (!add_asm_intercode_to_intercode(asm_intercode_extra_entries))
  return 0;

 return 1;

}

/*
Call this function whenever an expression is expected:
 - when reading an address or number operand for an instruction (call with asm_intercode_position = number of addresses after the instruction the expression should go - e.g. setra would have this as 1 for the a expression)
 - when reading memory initialisation values (e.g. in a variable definition) (call with asm_intercode_position = 0 and remember to call add_asm_intercode_to_intercode(1))

It accepts:
 - a static/global variable name (easy)
  - if the variable is an array, the address will be the address of the first element
 - a static/global variable name plus a constant
 - a static/global variable name minus a constant (not sure if this is actually needed)
The variable must be within the current scope, as set by the compiler.
The address referred to will be fixed according to process/aspace offsets later.

Also accepts the following:
 - a label (treated as a variable name)
 - a process name (treated as a variable name)
 - a constant
 - a scoped variable (beginning with the scope keyword)




*/
static int parse_asm_expression(int asm_intercode_position)
{

// int save_scode_pos;
 int id_index;

 struct ctokenstruct ctoken;

 if (!read_next(&ctoken))
  return asm_error(ASMERR_REACHED_END, &ctoken);

 switch(ctoken.type)
 {
  case CTOKEN_TYPE_NUMBER:
   add_intercode_asm_operand(IC_ASM_NUMBER, ctoken.number_value, 0, 0, asm_intercode_position);
   break;

  case CTOKEN_TYPE_OPERATOR_ARITHMETIC:
   if (ctoken.subtype != CTOKEN_SUBTYPE_MINUS) // negative numbers only
    return asm_error(ASMERR_SYNTAX_ERROR, &ctoken);
// now read in the number
   if (!read_next(&ctoken))
    return asm_error(ASMERR_REACHED_END, &ctoken);
   if (ctoken.type != CTOKEN_TYPE_NUMBER)
    return asm_error(ASMERR_SYNTAX_ERROR, &ctoken);
// now add it, multiplied by -1
   add_intercode_asm_operand(IC_ASM_NUMBER, ctoken.number_value * -1, 0, 0, asm_intercode_position);
   break;

  case CTOKEN_TYPE_IDENTIFIER_USER_VARIABLE: // this is a compiler variable accessed from asm code
   id_index = ctoken.identifier_index;
// this is designed to work with static/global variables
//  *** currently auto variables are accepted, but they give their stack offset rather than their address (the stack frame pointer must be added to this to find the actual address, then it must be dereferenced to find the value)
   if (!parse_asm_expression_offset(&ctoken)) // this call may update ctoken
    return 0;
   add_intercode_asm_operand(IC_ASM_ADDRESS, id_index, ctoken.number_value, 0, asm_intercode_position);
   break; // end CTOKEN_TYPE_IDENTIFIER_USER_VARIABLE

  case CTOKEN_TYPE_IDENTIFIER_NEW: // not previously used. Treat as an undefined asm generic
   id_index = ctoken.identifier_index;
   identifier[id_index].type = ASM_ID_GENERIC_UNDEFINED;
   identifier[id_index].storage_class = STORAGE_STATIC;
   identifier[id_index].asm_or_compiler = IDENTIFIER_ASM_OR_C; // the compiler can read undefined generics because it may want to turn them into actual identifiers
   identifier[id_index].aspace_scope = astate.current_aspace;
   identifier[id_index].nspace_scope = astate.current_nspace;
// fall through...
  case CTOKEN_TYPE_ASM_GENERIC_UNDEFINED:
  case CTOKEN_TYPE_ASM_GENERIC_DEFINED:
  case CTOKEN_TYPE_ASM_GENERIC_ALIAS:
  case CTOKEN_TYPE_ASM_ASPACE:
  case CTOKEN_TYPE_ASM_NSPACE:
   id_index = ctoken.identifier_index;
   if (!parse_asm_expression_offset(&ctoken)) // checks for an offset to the address. this call may update ctoken (so that its number_value can be used in the add_intercode call)
    return 0;
   add_intercode_asm_operand(IC_ASM_ADDRESS, id_index, ctoken.number_value, 0, asm_intercode_position);
   break; // end CTOKEN_TYPE_IDENTIFIER_USER_VARIABLE

  case CTOKEN_TYPE_ASM_KEYWORD:
   id_index = ctoken.identifier_index;
   switch(id_index)
   {
    case KEYWORD_ASM_ASPACE_END_ADDRESS:
     if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACKET_OPEN))
      return asm_error(ASMERR_SYNTAX_ERROR, &ctoken);
// read in the name of the aspace:
     if (!read_next(&ctoken))
      return asm_error(ASMERR_SYNTAX_ERROR, &ctoken);
     if (ctoken.type != CTOKEN_TYPE_ASM_GENERIC_UNDEFINED
      && ctoken.type != CTOKEN_TYPE_ASM_ASPACE)
       return asm_error(ASMERR_SYNTAX_ERROR, &ctoken);
     id_index = ctoken.identifier_index;
// now check for an offset:
     if (!parse_asm_expression_offset(&ctoken)) // checks for an offset to the address. this call may update ctoken (so that its number_value can be used in the add_intercode call)
      return 0;
     add_intercode_asm_operand(IC_ASM_ASPACE_END_ADDRESS, id_index, ctoken.number_value, 0, asm_intercode_position);
     if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACKET_CLOSE))
      return asm_error(ASMERR_SYNTAX_ERROR, &ctoken);
     break;  // end KEYWORD_ASM_ASPACE_END_ADDRESS
    case KEYWORD_ASM_SCOPE:
     if (!parse_asm_scope(asm_intercode_position))
      return 0;
     break; // end KEYWORD_ASM_SCOPE

    default:
     return asm_error(ASMERR_SYNTAX_ERROR, &ctoken);
   }
   break; // end CTOKEN_TYPE_ASM_KEYWORD

  default:
   return asm_error(ASMERR_SYNTAX_ERROR, &ctoken);

 }

 return 1;

}

// call this function after reading a ctoken containing a word that refers to an address
// it checks for a following + or - sign and reads in any expression that follows
//  (it probably also accepts +- as the same as -)
// value of the expression is stored in ctoken.number_value
// returns 1/0 success/fail
static int parse_asm_expression_offset(struct ctokenstruct* ctoken)
{

 int retval, number_sign = 0;

 if (check_next(CTOKEN_TYPE_OPERATOR_ARITHMETIC, CTOKEN_SUBTYPE_PLUS))
 {
  number_sign = 1;
 }
  else
  {
   if (check_next(CTOKEN_TYPE_OPERATOR_ARITHMETIC, CTOKEN_SUBTYPE_MINUS))
    number_sign = -1;
  }

 if (number_sign == 0) // no offset found
 {
  ctoken->number_value = 0;
  return 1;
 }

// a variable name can be followed by + and a number or an expression that can be resolved to a constant
// if after the + or - is an open bracket, accept an expression:
     if (check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACKET_OPEN))
     {
       retval = read_literal_numbers(ctoken);
       if (retval != 2) // 2 means that read_literal_numbers successfully resolved ctoken to a constant
        return asm_error(ASMERR_EXPECTED_CONSTANT_OR_EXPRESSION, ctoken);
// check for closing bracket
       if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACKET_CLOSE))
        return asm_error(ASMERR_EXPECTED_CONSTANT_OR_EXPRESSION, ctoken);
// fix for + or - operator:
       ctoken->number_value *= number_sign;
// so now we can return with the value of the expression stored in ctoken->number_value
       return 1;
     }
      else
      {
// no bracket, so it should be a single number
// now read in the number
       if (!read_next(ctoken))
        return asm_error(ASMERR_REACHED_END, ctoken);
       if (ctoken->type != CTOKEN_TYPE_NUMBER)
        return asm_error(ASMERR_EXPECTED_CONSTANT_OR_EXPRESSION, ctoken);
       ctoken->number_value *= number_sign;
      }

 return 1;

}





// call this function when ctoken has been confirmed to contain an asm keyword
static int parse_asm_keyword(struct ctokenstruct* ctoken)
{

 int keyword = ctoken->identifier_index;
 int id_index, aspace_index, nspace_index;

 struct ctokenstruct next_ctoken;
 struct ctokenstruct third_ctoken;

 switch(keyword)
 {
  case KEYWORD_ASM_DEF:
// def can be followed by a type specifier: aspace, nspace, label, int
   if (!read_next(&next_ctoken))
    return asm_error(ASMERR_REACHED_END, ctoken);
//   if (next_ctoken.type != CTOKEN_TYPE_ASM_KEYWORD)
//    return asm_error(ASMERR_EXPECTED_INT_DECLARATION, ctoken);
// now read in the name of the thing being defined:
   if (!read_next(&third_ctoken))
    return asm_error(ASMERR_REACHED_END, ctoken);
// exactly what is accepted at this point depends on the specified type:
   switch(next_ctoken.identifier_index)
   {
    case KEYWORD_ASM_ASPACE: // def aspace
     if (third_ctoken.type != CTOKEN_TYPE_ASM_ASPACE // already been implicitly declared as an aspace
     && third_ctoken.type != CTOKEN_TYPE_ASM_GENERIC_UNDEFINED // already been implicitly declared as a generic asm label
     && third_ctoken.type != CTOKEN_TYPE_IDENTIFIER_NEW) // not previously used
      return asm_error(ASMERR_ASPACE_NAME, &third_ctoken);
// so we have an identifier
     id_index = third_ctoken.identifier_index;
     if (third_ctoken.type == CTOKEN_TYPE_ASM_ASPACE)
     {
// if the identifier has already been implicitly declared as an aspace, make sure it hasn't been defined:
      if (astate.aspace[identifier[id_index].aspace_index].defined)
       return asm_error(ASMERR_ASPACE_ALREADY_DEFINED, &third_ctoken);
     }
      else
       {
// if the identifier hasn't previously been implicitly declared as an aspace, need to allocate an aspace index for it:
        if (!allocate_aspace(third_ctoken.identifier_index))
         return 0;
       }
// now we can assume identifier[id_index].aspace_index is valid. Start setting up the aspace structure:
     aspace_index = identifier[id_index].aspace_index;
     astate.aspace [aspace_index].defined = 1;
     astate.aspace [aspace_index].declared = 1; // may already be 1
     astate.aspace [aspace_index].parent = astate.current_aspace;
     astate.aspace [aspace_index].identifier_index = id_index;
     astate.aspace [aspace_index].corresponding_process = -1; // an aspace defined in asm will not have a corresponding process, and cannot be referred to in compiled code
// now set up the identifier structure:
     identifier[id_index].type = ASM_ID_ASPACE;
     identifier[id_index].asm_or_compiler = IDENTIFIER_ASM_ONLY;
     identifier[id_index].aspace_scope = astate.current_aspace;
     identifier[id_index].nspace_scope = -1; // aspace is unaffacted by nspace
//     if (astate.current_nspace == -1) - not relevant for aspace name identifiers
     fix_generic_aliases(id_index);

     if (!enter_aspace(aspace_index)) // now actually enter the aspace
      return 0;
     break;
    case KEYWORD_ASM_NSPACE: // def nspace
     if (third_ctoken.type != CTOKEN_TYPE_ASM_NSPACE // already been implicitly declared as an nspace
     && third_ctoken.type != CTOKEN_TYPE_ASM_GENERIC_UNDEFINED // already been implicitly declared as a generic asm label
     && third_ctoken.type != CTOKEN_TYPE_IDENTIFIER_NEW) // not previously used
      return asm_error(ASMERR_NSPACE_NAME, &third_ctoken);
// so we have an identifier
     id_index = third_ctoken.identifier_index;
     if (third_ctoken.type == CTOKEN_TYPE_ASM_NSPACE)
     {
// if the identifier has already been declared as an nspace, make sure it hasn't been defined:
      if (astate.nspace[identifier[id_index].nspace_index].defined)
       return asm_error(ASMERR_NSPACE_ALREADY_DEFINED, &third_ctoken);
     }
      else
       {
// if the identifier hasn't previously been implicitly declared as an nspace, need to allocate an nspace index for it:
        if (!allocate_nspace(third_ctoken.identifier_index))
         return 0;
       }
// now we can assume identifier[id_index].aspace_index is valid. Start setting up the aspace structure:
     nspace_index = identifier[id_index].nspace_index;
     astate.nspace [nspace_index].declared = 1;
     astate.nspace [nspace_index].defined = 1;
     astate.nspace [nspace_index].aspace_scope = astate.current_aspace;
     astate.nspace [nspace_index].identifier_index = id_index;
     astate.nspace [nspace_index].corresponding_cfunction = -1; // an nspace defined in asm will not have a corresponding cfunction, and cannot be referred to in compiled code
// now set up the identifier structure:
     identifier[id_index].type = ASM_ID_NSPACE;
     identifier[id_index].asm_or_compiler = IDENTIFIER_ASM_ONLY;
     identifier[id_index].aspace_scope = astate.current_aspace;
     identifier[id_index].nspace_scope = -1; // can't have nested nspaces
// the nspace's name may have been used as an undefined generic within another nspace, so fix any generic aliases.
//     if (astate.current_nspace == -1) - this line is not relevant for nspace name identifiers as they always have aspace scope
     fix_generic_aliases(id_index);
// now actually enter the nspace:
     astate.current_nspace = nspace_index;
     if (!add_intercode(IC_ASM_DEFINE_NSPACE, nspace_index, 0, 0))
      return asm_error(ASMERR_OUT_OF_INTERCODE, ctoken);
     break;
    case KEYWORD_ASM_INT: // def int
    case KEYWORD_ASM_LABEL: // def label - currently treated the same as def int
     if (third_ctoken.type != CTOKEN_TYPE_ASM_GENERIC_UNDEFINED // has been implicitly declared as a generic asm label, but not defined
      && third_ctoken.type != CTOKEN_TYPE_IDENTIFIER_NEW) // not previously used
       return asm_error(ASMERR_IDENTIFIER_ALREADY_DEFINED, &third_ctoken);
     id_index = third_ctoken.identifier_index;
     identifier[id_index].type = ASM_ID_GENERIC_DEFINED;
     identifier[id_index].asm_or_compiler = IDENTIFIER_ASM_ONLY;
     identifier[id_index].storage_class = STORAGE_STATIC;
     identifier[id_index].array_dims = 0;
     identifier[id_index].aspace_scope = astate.current_aspace;
     identifier[id_index].nspace_scope = astate.current_nspace;
// if this variable is not in an nspace but has previously been referred to in nspaces, there will be undefined generic identifiers floating around.
// need to find them and fix them to refer to this identifier
     if (astate.current_nspace == -1)
      fix_generic_aliases(id_index);
     if (next_ctoken.identifier_index == KEYWORD_ASM_INT)
     {
      if (!add_intercode(IC_ASM_DEFINE_INT, third_ctoken.identifier_index, 0, 0))
       return asm_error(ASMERR_OUT_OF_INTERCODE, ctoken);
     }
      else
      {
       if (!add_intercode(IC_ASM_DEFINE_LABEL, third_ctoken.identifier_index, 0, 0))
        return asm_error(ASMERR_OUT_OF_INTERCODE, ctoken);
      }
     break;
    default:
//     return asm_error(ASMERR_UNEXPECTED_WORD, &next_ctoken);
     return asm_error(ASMERR_SYNTAX_ERROR, ctoken);

//    case KEYWORD_ASM_NSPACE:
//     break;
   }
// the type specifier must be followed by a token of the correct type, or either an ID_USER_UNTYPED or an ASM_ID_USER_GENERIC (for tokens that have already been implicitly declared)
   break; // end KEYWORD_ASM_DEF

  case KEYWORD_ASM_ASPACE_END:
// this keyword closes an aspace. This means setting the end-point of the aspace and re-entering the aspace's parent aspace
   if (astate.current_aspace == 0) // cannot use aspace_end in file-level aspace
    return asm_error(ASMERR_CANNOT_END_FILE_ASPACE, ctoken);
   if (!add_intercode(IC_ASM_END_ASPACE, astate.current_aspace, 0, 0))
    return asm_error(ASMERR_OUT_OF_INTERCODE, ctoken);
   astate.current_aspace = astate.aspace [astate.current_aspace].parent;
   astate.current_nspace = -1; // also implicitly ends current nspace
   break;
  case KEYWORD_ASM_NSPACE_END:
   if (astate.current_nspace == -1) // cannot use nspace_end in top-level nspace
    return asm_error(ASMERR_CANNOT_END_TOP_NSPACE, ctoken);
   if (!add_intercode(IC_ASM_END_NSPACE, astate.current_nspace, 0, 0))
    return asm_error(ASMERR_OUT_OF_INTERCODE, ctoken);
   astate.current_nspace = -1;
   break;
  case KEYWORD_ASM_NSPACE_REJOIN:
// form: nspace_rejoin nspace1
// re-enters a previously defined nspace. (note that there is no equivalent for aspaces)
// first we get rid of any current nspace scope (because there's likely to be a sequence of nspace_rejoin commands and we don't want to have to put nspace_end before every nspace_rejoin):
   astate.current_nspace = -1;
   if (!read_next(ctoken))
    return asm_error(ASMERR_REACHED_END, ctoken);
   if (ctoken->type != CTOKEN_TYPE_ASM_NSPACE)
    return asm_error(ASMERR_EXPECTED_NSPACE_NAME, ctoken);
   astate.current_nspace = identifier[ctoken->identifier_index].nspace_index;
   if (!add_intercode(IC_ASM_REJOIN_NSPACE, astate.current_nspace, 0, 0))
    return asm_error(ASMERR_OUT_OF_INTERCODE, ctoken);
   break;



  default:
   return asm_error(ASMERR_UNEXPECTED_WORD, ctoken);

 }

 return 1;

}

// Identifiers may be used undefined inside an nspace.
// They then get turned into ASM_ID_GENERIC_UNDEFINED with nspace scope.
// When an identifier is defined with aspace scope (nspace=-1), this function is called.
// It searches for any matching ASM_ID_GENERIC_UNDEFINED identifiers with nspace scope, and converts them into aliases for the aspace scope identifier.
// Identifier can be of various types (variables, labels, process names etc.)
static void fix_generic_aliases(int id_index)
{

 int i;

 for (i = 0; i < IDENTIFIER_MAX; i ++)
 {
  if (identifier[i].type == ASM_ID_GENERIC_UNDEFINED
   && identifier[i].aspace_scope == astate.current_aspace
   && strcmp(identifier[i].name, identifier[id_index].name) == 0) // match!
  {
   identifier[i].type = ASM_ID_GENERIC_ALIAS;
   identifier[i].asm_alias = id_index;
  }
 }

}


enum
{
REF_TYPE_ASPACE,
REF_TYPE_NSPACE,
REF_TYPE_FINAL
};


// a "scope" keyword has been read in at the start of an asm expression
// this function parses it and also deals with any following offset
// it will implicitly declare aspaces and nspaces, with appropriate aspace scope, as needed. Later definitions should find the identifiers created here
// returns 1 success/0 fail
static int parse_asm_scope(int asm_intercode_position)
{

 struct ctokenstruct ctoken;
 strcpy(ctoken.name, "scope");

// if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACKET_OPEN))
//  return asm_error(ASMERR_EXPECTED_BRACKET_OPEN, &ctoken);

 int ref_type;
 int finished = 0;
 int id_index;

 int save_astate_aspace = astate.current_aspace;
 int save_astate_nspace = astate.current_nspace;
 int read_nspace = 0; // if 1, means that we've read an nspace - so can't go back to aspace or another nspace

 astate.current_nspace = -1;

 while(TRUE)
 {
  if (!read_next(&ctoken))
   return asm_error(ASMERR_REACHED_END, &ctoken);
// expects: . (aspace) : (nspace) :: (identifier)

  if (ctoken.type != CTOKEN_TYPE_PUNCTUATION)
   return asm_error(ASMERR_EXPECTED_SCOPE_INDICATOR, &ctoken);

  switch(ctoken.subtype)
  {
   case CTOKEN_SUBTYPE_FULL_STOP:
    ref_type = REF_TYPE_ASPACE;
    if (read_nspace)
     return asm_error(ASMERR_EXPECTED_ADDRESS_ID_AFTER_NSPACE, &ctoken);
    break;
   case CTOKEN_SUBTYPE_COLON:
    ref_type = REF_TYPE_NSPACE;
    if (check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_COLON))
     ref_type = REF_TYPE_FINAL;
      else
      {
       if (read_nspace)
        return asm_error(ASMERR_EXPECTED_ADDRESS_ID_AFTER_NSPACE, &ctoken);
      }
    break;
   default:
    return asm_error(ASMERR_EXPECTED_SCOPE_INDICATOR, &ctoken);
  }

// could just have one switch. Oh well, too late.
  switch(ref_type)
  {
   case REF_TYPE_ASPACE:
// expect an aspace name, an undefined generic or a new identifier:
    if (!read_next(&ctoken))
     return asm_error(ASMERR_REACHED_END, &ctoken);
    switch(ctoken.type)
    {
     case CTOKEN_TYPE_ASM_ASPACE:
// this aspace is already known in the current scope. Easy!
      astate.current_aspace = identifier[ctoken.identifier_index].aspace_index;
      astate.current_nspace = -1;
      break;

     case CTOKEN_TYPE_ASM_GENERIC_UNDEFINED:
     case CTOKEN_TYPE_IDENTIFIER_NEW:
// this aspace is not known. Need to allocate it:
      id_index = ctoken.identifier_index;
      if (!allocate_aspace(id_index))
       return 0;
      identifier[id_index].type = ASM_ID_ASPACE;
      identifier[id_index].aspace_scope = astate.current_aspace;
      identifier[id_index].nspace_scope = -1;
      astate.aspace [identifier[id_index].aspace_index].parent = astate.current_aspace; // must come before astate.current_aspace is updated
      astate.current_aspace = identifier[id_index].aspace_index;
      astate.current_nspace = -1;
      astate.aspace [astate.current_aspace].corresponding_process = -1;
      break;
    }
    break; // end REF_TYPE_ASPACE

   case REF_TYPE_NSPACE:
// expect an nspace name, an undefined generic or a new identifier:
    if (!read_next(&ctoken))
     return asm_error(ASMERR_REACHED_END, &ctoken);
    switch(ctoken.type)
    {
     case CTOKEN_TYPE_ASM_NSPACE:
// this nspace is already known in the current scope. Easy!
      astate.current_nspace = identifier[ctoken.identifier_index].nspace_index;
      break;

     case CTOKEN_TYPE_ASM_GENERIC_UNDEFINED:
     case CTOKEN_TYPE_IDENTIFIER_NEW:
// this nspace is not known. Need to allocate it:
      id_index = ctoken.identifier_index;
      if (!allocate_nspace(id_index))
       return 0;
      identifier[id_index].type = ASM_ID_NSPACE;
      identifier[id_index].aspace_scope = astate.current_aspace;
      identifier[id_index].nspace_scope = -1;
      identifier[id_index].asm_or_compiler = IDENTIFIER_ASM_ONLY;
      astate.current_nspace = identifier[id_index].nspace_index;
      astate.nspace [astate.current_nspace].corresponding_cfunction = -1;
      astate.nspace [astate.current_nspace].aspace_scope = astate.current_aspace;
      break;
    }
    break; // end REF_TYPE_NSPACE

   case REF_TYPE_FINAL:
// expect: compiler variable, asm generic (defined or undefined), new identifier. Probably accept aspace and nspace names as well as they're valid addresses
    if (!read_next(&ctoken))
     return asm_error(ASMERR_REACHED_END, &ctoken);
    id_index = ctoken.identifier_index; // may not be valid, if ctoken not an identifier - but if not an identifier is not used below
// if it's a generic alias, need to change it to the actual identifier:
    if (ctoken.type == CTOKEN_TYPE_ASM_GENERIC_ALIAS)
     id_index = identifier[id_index].asm_alias; // actually I don't think this does anything - this ctoken type is not accepted by the switch below, and generates an error
// can't necessarily rely on ctoken from this point (may be okay in errors)
    switch(ctoken.type)
    {
     case CTOKEN_TYPE_IDENTIFIER_NEW:
      identifier[id_index].type = ASM_ID_GENERIC_UNDEFINED;
      identifier[id_index].storage_class = STORAGE_STATIC;
      identifier[id_index].asm_or_compiler = IDENTIFIER_ASM_OR_C; //IDENTIFIER_ASM_ONLY;
      identifier[id_index].aspace_scope = astate.current_aspace;
      identifier[id_index].nspace_scope = astate.current_nspace;
//      if (astate.current_aspace == )
//       identifier[id_index].scope = SCOPE_GLOBAL;
//      if (astate.current_nspace == -1)
//       identifier[id_index].scope = SCOPE_GLOBAL;
// fall through...
     case CTOKEN_TYPE_ASM_GENERIC_UNDEFINED:
     case CTOKEN_TYPE_ASM_GENERIC_DEFINED:
     case CTOKEN_TYPE_IDENTIFIER_USER_VARIABLE:
     case CTOKEN_TYPE_ASM_ASPACE:
     case CTOKEN_TYPE_ASM_NSPACE:
      if (!parse_asm_expression_offset(&ctoken)) // checks for an offset to the address. this call may update ctoken (so that its number_value can be used in the add_intercode call)
       return 0;
// IC_ASM_ADDRESS_SCOPED is used because it applies operand [2] as an aspace when calculating the address space offset (i.e. the offset used will be the one for the current aspace, not the temporarily scoped aspace)
      add_intercode_asm_operand(IC_ASM_ADDRESS_SCOPED, id_index, ctoken.number_value, save_astate_aspace, asm_intercode_position);
      finished = 1;
      break; // end REF_TYPE_FINAL

     default:
      return asm_error(ASMERR_UNEXPECTED_WORD, &ctoken);
    }
    break;

   default:
    return asm_error(ASMERR_UNEXPECTED_WORD, &ctoken);

  }

  if (finished)
   break;

 };

 astate.current_aspace = save_astate_aspace;
 astate.current_nspace = save_astate_nspace;

 return 1;

}


// This function is called after a { is found (which indicates that memory is being written to directly - e.g. with a variable definition)
// returns 1/0 success/fail
static int asm_memory_init(void)
{

 struct ctokenstruct ctoken;
 strcpy(ctoken.name, "(empty)");

// check for a string:
 if (check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_QUOTES))
 {
  int read_char;
  int string_length = 0;
  int started_string = 0;
  while(TRUE)
  {
    read_char = asm_get_next_char_from_scode();

    if (read_char == REACHED_END_OF_SCODE)
     return asm_error(ASMERR_REACHED_END, &ctoken);
    if (read_char == '"')
     break;
    if (read_char == '\\')
    {
     read_char = asm_get_next_char_from_scode();
     if (read_char == REACHED_END_OF_SCODE)
      return asm_error(ASMERR_REACHED_END, &ctoken);
     switch(read_char)
     {
      case 'n': read_char = '\n'; break;
      case '"': read_char = '"'; break;
      case '\\': read_char = '\\'; break;
     }
    }
    if (started_string == 0)
    {
     add_intercode(IC_ASM_START_STRING, 0, 0, 0); // just makes sure the {" is written to asm generated from asm
     started_string = 1;
    }
    add_intercode(IC_STRING, read_char, 0, 0);
    string_length ++;
    if (string_length == 80)
     return asm_error(ASMERR_STRING_TOO_LONG, &ctoken);
  };
  if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACE_CLOSE))
   return asm_error(ASMERR_EXPECTED_CLOSING_BRACE_AFTER_STRING, &ctoken);
  add_intercode(IC_STRING_END, 0, 0, 0);
  return 1;
 }

 while(TRUE)
 {
// read in an asm expression
  if (!parse_asm_expression(0))
   return 0;
// write the asm expression to intercode
  if (!add_asm_intercode_to_intercode(0)) // 0 means no extra entries (just 0)
   return 0;
// check for } (end of initialisation sequence)
  if (check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACE_CLOSE))
   return 1;
// check for absence of comma (syntax error)
  if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_COMMA))
   return asm_error(ASMERR_EXPECTED_COMMA_AFTER_EXPRESSION, &ctoken);
// now check for the next expression
 };

 return 1; // should be unreachable

}

// This function goes through the identifier list looking for ASM_ID_GENERIC_UNDEFINED and tries to assign them aliases.
// If unaliased so far, they are probably scoped variables used prior to declaration (and need to be assigned as aliases of
//  the declared variable)
int resolve_undefined_asm_generics(void)
{

 int i, j;

 for (i = 0; i < IDENTIFIER_MAX; i ++)
 {
  if (identifier[i].type != ASM_ID_GENERIC_UNDEFINED)
   continue;
  for (j = 0; j < IDENTIFIER_MAX; j ++)
  {
   if (identifier[j].aspace_scope == identifier[i].aspace_scope
    && identifier[j].nspace_scope == identifier[i].nspace_scope)
   {

    if ((identifier[j].type == ID_USER_INT
      || identifier[j].type == ID_USER_LABEL
      || identifier[j].type == ID_USER_CFUNCTION)
       && strcmp(identifier[i].name, identifier[j].name) == 0)
    {
     identifier[i].type = ASM_ID_GENERIC_ALIAS;
     identifier[i].asm_alias = j;
     break;
    }
   }
   if (j == IDENTIFIER_MAX)
   {
    start_log_line(MLOG_COL_ERROR);
    write_to_log("Unresolved: ");
    write_to_log(identifier[i].name);
    write_to_log(".");
    finish_log_line();
    return asm_error(ASMERR_UNRESOLVED_GENERIC_REFERENCE, NULL);
   }
  }
 }

 return 1; // success

}









static int asm_get_next_char_from_scode(void)
{

 cstate.scode_pos ++;

 if (astate.scode->text [cstate.scode_pos] == '\0') // reached end
  return REACHED_END_OF_SCODE;

 return astate.scode->text [cstate.scode_pos];

}


// this function finds an empty space in the aspace array and allocates it.
// it also sets identifier[id_index].aspace_index to the index of the aspace in the aspace array
int allocate_aspace(int id_index)
{

 int i;

 for (i = 0; i < ASPACES; i ++)
 {
  if (astate.aspace [i].declared == 0)
  {
   identifier[id_index].aspace_index = i;
// the new aspace needs a "self" identifier as well:
   int self_id_index = 0;
   struct ctokenstruct self_ctoken;
   strcpy(self_ctoken.name, "self"); // must be before call to new_c_identifier
   if (!new_c_identifier(&self_ctoken, ASM_ID_ASPACE))
    return asm_error(ASMERR_CANNOT_ALLOCATE_IDENTIFIER, &self_ctoken);
   self_id_index = self_ctoken.identifier_index;
   identifier[self_id_index].type = ASM_ID_ASPACE;
   identifier[self_id_index].aspace_scope = i;
   identifier[self_id_index].nspace_scope = -1;
   identifier[self_id_index].asm_or_compiler = IDENTIFIER_ASM_ONLY;
   identifier[self_id_index].aspace_index = i;
   identifier[self_id_index].address_bcode = 0; // ????
   astate.aspace [i].self_identifier_index = self_id_index;
   astate.aspace [i].declared = 1;
   return 1;
  }
 }

// didn't find an empty space in the array:

 struct ctokenstruct ctoken;
 strcpy(ctoken.name, "(empty)");

 return asm_error(ASMERR_ALLOCATE_ASPACE_FAILED, &ctoken);

}

// just like allocate_aspace, but for nspaces
int allocate_nspace(int id_index)
{

 int i;

 for (i = 0; i < NSPACES; i ++)
 {
  if (astate.nspace [i].declared == 0)
  {
   identifier[id_index].nspace_index = i;
   astate.nspace [i].declared = 1;
   return 1;
  }
 }

// didn't find an empty space in the array:
 struct ctokenstruct ctoken;
 strcpy(ctoken.name, "(empty)");

 return asm_error(ASMERR_ALLOCATE_NSPACE_FAILED, &ctoken);

}

// enters a new aspace. assumes a_index is valid.
static int enter_aspace(int a_index)
{

 struct ctokenstruct ctoken;
 strcpy(ctoken.name, "(empty)");

 astate.current_aspace = a_index;
 astate.current_nspace = -1;

 if (!add_intercode(IC_ASM_ENTER_ASPACE, a_index, 0, 0))
  return asm_error(ASMERR_OUT_OF_INTERCODE, &ctoken);

 return 1;

}


// This function adds an IC_ASM_OPCODE intercode entry.
// The intercode parser will put instruction directly into bcode (so it needs to be set up with appropriate bits).
static void add_intercode_asm_opcode(int instruction)
{

 asm_intercode[0].instr = IC_ASM_OPCODE;

 asm_intercode[0].operand [0] = instruction;

 asm_intercode[0].src_line = cstate.src_line;

 asm_intercode[0].src_file = cstate.src_file;

}

// adds an operand to intercode.
// the value of the operand may need further processing in c_inter.c (as it may refer to a memory address that will need to be fixed at the end of compilation)
static void add_intercode_asm_operand(int ic_type, int operand1, int operand2, int operand3, int asm_intercode_index)
{
 asm_intercode[asm_intercode_index].instr = ic_type;

 asm_intercode[asm_intercode_index].operand [0] = operand1;
 asm_intercode[asm_intercode_index].operand [1] = operand2;
 asm_intercode[asm_intercode_index].operand [2] = operand3;

 asm_intercode[asm_intercode_index].src_line = cstate.src_line;
 asm_intercode[asm_intercode_index].src_file = cstate.src_file;

}

// asm intercode needs to be built for each instruction before it's added to the main intercode array because it can't be dealt with in strict order.
// call this function to put things from the asm_intercode array into the main intercode array.
// note that this function is only for instructions - some asm commands call add_intercode directly
static int add_asm_intercode_to_intercode(int extra_entries)
{

 struct ctokenstruct temp_ctoken;
 strcpy(temp_ctoken.name, "(empty)");

 cstate.src_line = asm_intercode[0].src_line; // so add_intercode records src_line correctly (for debugging purposes)
 cstate.src_file = asm_intercode[0].src_file; // so add_intercode records src_file correctly (for debugging purposes)
 cstate.src_line_pos = 0; // not sure about this, though

 if (!add_intercode(asm_intercode[0].instr, asm_intercode[0].operand [0], asm_intercode[0].operand [1], asm_intercode[0].operand [2]))
  return asm_error(ASMERR_OUT_OF_INTERCODE, &temp_ctoken);

 int i;

 for (i = 0; i < extra_entries; i ++)
 {
  cstate.src_line = asm_intercode[i + 1].src_line; // so add_intercode records src_line correctly (for debugging purposes)
  cstate.src_file = asm_intercode[i + 1].src_file; // so add_intercode records src_line correctly (for debugging purposes)

  if (!add_intercode(asm_intercode[i + 1].instr, asm_intercode[i + 1].operand [0], asm_intercode[i + 1].operand [1], asm_intercode[i + 1].operand [2]))
   return asm_error(ASMERR_OUT_OF_INTERCODE, &temp_ctoken);
 }

 return 1;

}

const char *asm_error_name [ASMERRS] =
{
"reached end of source within asm block", // ASMERR_REACHED_END,
"unrecognised instruction", // ASMERR_UNRECOGNISED_INSTRUCTION,
"expected a register as operand", // ASMERR_EXPECTED_REGISTER_OPERAND,
"this opcode requires method operand to be constant", // ASMERR_METHOD_OPERAND_MUST_BE_CONSTANT,
"this opcode requires method bank operand to be constant", // ASMERR_MBANK_OPERAND_MUST_BE_CONSTANT,
"method operand out of bounds", // ASMERR_METHOD_OPERAND_OUT_OF_BOUNDS,
"mbank operand out of bounds", // ASMERR_MBANK_OPERAND_OUT_OF_BOUNDS,
"number operand must be a constant (or an expression that resolves to a constant)", // ASMERR_NUMBER_OPERAND_MUST_BE_CONSTANT,
"cannot use an automatic variable here", // ASMERR_CANNOT_USE_AUTO_VALUE,
"expected a constant (or an expression that resolves to a constant)", // ASMERR_EXPECTED_CONSTANT_OR_EXPRESSION,
"syntax error", // ASMERR_SYNTAX_ERROR,
"too much code!", // ASMERR_OUT_OF_INTERCODE,
"expected int declaration", // ASMERR_EXPECTED_INT_DECLARATION,
"expected generic asm identifier", // ASMERR_EXPECTED_GENERIC_ASM_IDENTIFIER,
"address space naming error", // ASMERR_ASPACE_NAME,
"address space already defined in this scope?", // ASMERR_ASPACE_ALREADY_DEFINED,
"name space already defined in this address space?", // ASMERR_NSPACE_ALREADY_DEFINED
"name space naming error", // ASMERR_NSPACE_NAME,
"unexpected token", // ASMERR_UNEXPECTED_WORD,
"failed to allocate address space?", // ASMERR_ALLOCATE_ASPACE_FAILED,
"failed to allocate name space?", // ASMERR_ALLOCATE_NSPACE_FAILED,
"expected comma after asm expression", // ASMERR_EXPECTED_COMMA_AFTER_EXPRESSION
"cannot end top-level address space", // ASMERR_CANNOT_END_FILE_ASPACE
"expected nspace name", // ASMERR_EXPECTED_NSPACE_NAME
"cannot end top-level nspace", // ASMERR_CANNOT_END_TOP_NSPACE,
"identifier already defined", // ASMERR_IDENTIFIER_ALREADY_DEFINED
"failed to allocate new identifier", // ASMERR_CANNOT_ALLOCATE_IDENTIFIER
"expected opening bracket", // ASMERR_EXPECTED_BRACKET_OPEN,
"expected scope indicator (\".\", \":\" or \"::\")", // ASMERR_EXPECTED_SCOPE_INDICATOR,
"invalid identifier after nspace specified", // ASMERR_EXPECTED_ADDRESS_ID_AFTER_NSPACE,
"expected closing bracket", // ASMERR_EXPECTED_BRACKET_CLOSE,
"string is too long (maximum 80 characters)", // ASMERR_STRING_TOO_LONG,
"expected closing brace after string", // ASMERR_EXPECTED_CLOSING_BRACE_AFTER_STRING,
"couldn't resolve generic reference" // ASMERR_UNRESOLVED_GENERIC_REFERENCE
};


//extern struct sourcestruct* debug_source; // this is a temporary measure to allow error code to print source code. It can't deal with #included files and will need to be fixed.
// defined in comp.c

static int asm_error(int error_type, struct ctokenstruct* ctoken)
{


     start_log_line(MLOG_COL_ERROR);
     write_to_log("Assembler error at line ");
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
     write_to_log(asm_error_name [error_type]);
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

     astate.error = error_type;
     return 0;
}









































