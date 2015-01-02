
// header for interpreter, assembler and compiler modules

#ifndef H_C_HEADER
#define H_C_HEADER


// revised opcodes, based on compiler:
// (need to revise v_interp and a_asm to account for changes)
enum
{
// basic and arithmetic
OP_nop, // 0 nop
// assembler will have set keyword, which will be turned into one of the following three based on operand type
OP_setrr, // 2 mov [target] [source]
OP_setra, // 2 mov [target] [source]
OP_setar, // 2 mov [target] [source]
//OP_setaa, // 2 mov [target] [source]
OP_setrn, // 2 mov [target] [source]
//OP_setan,
OP_setrd,
OP_setdr,

//OP_copy, // 2 copy [memory:target] [memory:source]
//OP_mcopy, // 3 lcopy [memory:target] [memory:source] [memory?or register?:size of copy]
OP_add, // 2 add [register:target] [register:to add]
OP_sub, // 2 sub [register:target] [register:to sub]
OP_mul, // 2 mul [register:target] [register:to mul]
OP_div, // 2 div [register:target] [register:divisor]
OP_mod, // 2 mod [register:target] [register:divisor]
OP_incr, // 1 incr [register]
OP_decr, // 1 incr [register]

// bitwise
OP_and, // 2 and [register:target] [register:and value]
OP_or, // 2 or [register:target] [register:or value]
OP_xor, // 2 xor [register:target] [register:xor value]
OP_bnot, // 2 bnot [value1] [target] - this is ~ bitwise not
OP_bshiftl, // 3 shiftl [value] [shift] [target] - may be interchangeable with right-shift if shift is -ve
OP_bshiftr, // 3 shiftr [value] [shift] [target]

OP_not, // this is ! not

// branching
// if statements perform the next instruction, which can be any 1 operand operation (currently really just jump),
//  if the statement evaluates to true.
OP_jumpr, // 1 jump [register:destination]
OP_jumpn, // 1 jump [number]

OP_iftrue,
OP_iffalse,
OP_cmpeq, // 2 cmpeq [register:value1] [register:value2]
OP_cmpneq, // 2 cmpneq [register:value1] [register:value2]
OP_cmpgr, // 2 cmpgr [register:value1] [register:value2] (if value1 > value2)
OP_cmpgre, // 2 cmpgre [value1] [value2]
OP_cmpls, // 2 cmpls [value1] [value2]
OP_cmplse, // 2 cmplse [value1] [value2]
OP_exit, // 0 exit

// stack management
OP_push, // 2 push [source] [stack pointer]
OP_pop, // 2 push [target] [stack pointer]

// method instructions
OP_putbr, // puts contents of register into method address
OP_putbn,
OP_putba,
OP_putrr, // puts contents of register into method address; method address is determined by first register r
OP_putrn,
OP_putra,

OP_getrb, // get contents of method address, put in register r
OP_getrr, // get contents of method address determined by second register r, put in first register r

OP_callmr, // note - m here is a method (0-15), not a method address (0-63)!
OP_callrr, // anyway, these instructions call whichever method and put its return value into second operand register r.
//OP_callnr,
/*
putmr (method address) (register) - puts contents of register into method address.
putmn (method address) (number) - puts number into method address
putma (method address) (address) - puts contents of address into method address
putrr, putrn, putra - as above, but the method address is determined by the contents of register r.

getrm (register) (method address) - gets contents of method address, puts in register
getrr (target register) (source register) - gets contents of method address at source register, puts in target register.

callmr (method number (not address)) (register) - call the method; any return value goes in register
callrr (register) (target register) - call method indicated by register; any return value goes in target register.
callm, callr - as above if return value not used (shouldn't allow any method not to have a return value at all)*/


// print commands for debugging
OP_prints, // print string
OP_printr, // print register
OP_printsa, // print string from specified address
OP_printsr, // print string from address specified in register

// Note: shouldn't have more than 63 op types (interferes with the sign bit of the bcode entry)

OP_end
};

/*

Old version; reflects a_asm and v_interp but not c_comp and c_inter

enum
{
// basic and arithmetic
OP_nop, // 0 nop
OP_set, // 2 set [value] [target]
OP_add, // 3 add [value1] [value2] [target]
OP_sub, // 3 sub [value1] [value2 (amount subtracted)] [target]
OP_mul, // 3 mul [value1] [value2] [target]
OP_div, // 3 mul [value1] [divisor] [target]
OP_mod, // 3 modulo [value1] [divisor] [target]

// bitwise
OP_and, // 3 and [value1] [value2] [target]
OP_or, // 3 or [value1] [value2] [target]
OP_xor, // 3 xor [value1] [value2] [target]
OP_not, // 2 not [value1] [target]
// OP_bshiftl, // 3 shiftl [value] [shift] [target] - may be interchangeable with right-shift if shift is -ve
// OP_bshiftr, // 3 shiftr [value] [shift] [target]

// branching
// if statements perform the next instruction, which can be any 1 operand operation (currently really just jump),
//  if the statement evaluates to true.
OP_jump, // 1 jump [destination]
OP_iftrue, // 1 iftrue [value]
OP_iffalse, // 1 iffalse [value]
OP_ifeq, // 2 ifeq [value1] [value2]
OP_ifneq, // 2 ifneq [value1] [value2]
OP_ifgr, // 2 ifgr [value1] [value2] (if value1 > value2)
OP_ifgre, // 2 ifgre [value1] [value2]
OP_ifls, // 2 ifls [value1] [value2]
OP_iflse, // 2 iflse [value1] [value2]
OP_exit, // 0 exit

OP_end
};*/

#define MAX_OPERANDS 3
#define OPCODE_NAME_LENGTH 10

struct opcode_propertiesstruct
{
 char name [OPCODE_NAME_LENGTH];
 int operands;
 int operand_type [MAX_OPERANDS];
};

enum
{
OPERAND_TYPE_NONE,
OPERAND_TYPE_REGISTER,
OPERAND_TYPE_ADDRESS,
OPERAND_TYPE_NUMBER,
OPERAND_TYPE_METHOD, // 0-15
OPERAND_TYPE_MBANK // 0-63



// to be removed
//OPERAND_TYPE_ANY, // the operand's type is indicated by the bits in the address mode indicator part of the operator. Can be register, value or reference
//OPERAND_TYPE_TARGET,
//OPERAND_TYPE_ADDRESS


// these are not used
/*
OPERAND_TYPE_VAL,
OPERAND_TYPE_TARGET,
OPERAND_TYPE_LONG_TARGET,
OPERAND_TYPE_LONG_VAL*/
};

// this is the leftshift needed to get from the rightmost bit to the bit that determines the overall address mode for a single operand operation
//#define SINGLE_OPERAND_ADDRESS_MODE_BIT 10
#define OPCODE_BITSHIFT 9


/*

enum
{
OPERANDS_NONE, // e.g. nop
OPERANDS_VAL, // e.g. iftrue
OPERANDS_TAR, // e.g. jump
OPERANDS_VAL_TAR, // e.g. set
OPERANDS_VAL_VAL, // e.g. ifeq
OPERANDS_VAL_VAL_TAR, // e.g. add
};


struct opcode_propertiesstruct opcode_properties
{
{0, OPERANDS_NONE}, // OP_nop, // 0 nop
{2, OPERANDS_VAL_TAR}, // OP_set, // 2 set [value] [target]
{3, OPERANDS_VAL_VAL_TAR}, // OP_add, // 3 add [value1] [value2] [target]
{3, OPERANDS_VAL_VAL_TAR}, // OP_sub, // 3 sub [value1] [value2 (amount subtracted)] [target]
{3, OPERANDS_VAL_VAL_TAR}, // OP_mul, // 3 mul [value1] [value2] [target]
{3, OPERANDS_VAL_VAL_TAR}, // OP_div, // 3 mul [value1] [divisor] [target]
{3, OPERANDS_VAL_VAL_TAR}, // OP_mod, // 3 modulo [value1] [divisor] [target]

// bitwise
{3, OPERANDS_VAL_VAL_TAR}, // OP_and, // 3 and [value1] [value2] [target]
{3, OPERANDS_VAL_VAL_TAR}, // OP_or, // 3 or [value1] [value2] [target]
{3, OPERANDS_VAL_VAL_TAR}, // OP_xor, // 3 xor [value1] [value2] [target]

// branching
{1, OPERANDS_TAR}, // OP_jump, // 1 jump [destination]
{1, OPERANDS_VAL}, // OP_iftrue, // 1 iftrue [value]
{1, OPERANDS_VAL}, // OP_iffalse, // 1 iffalse [value]
{2, OPERANDS_VAL_VAL}, // OP_ifeq, // 2 ifeq [value1] [value2]
{2, OPERANDS_VAL_VAL}, // OP_ifneq, // 2 ifneq [value1] [value2]
{2, OPERANDS_VAL_VAL}, // OP_ifgr, // 2 ifgr [value1] [value2] (if value1 > value2)
{2, OPERANDS_VAL_VAL}, // OP_ifgre, // 2 ifgre [value1] [value2]


};


*/

/*

enum
{
ADDRESS_MODE_LITERAL_ZERO,
ADDRESS_MODE_LITERAL_ONE,
ADDRESS_MODE_NEXT_VALUE,
ADDRESS_MODE_NEXT_REFERENCE,
ADDRESS_MODE_BV_IV,
ADDRESS_MODE_BV_IR,
ADDRESS_MODE_BR_IV,
ADDRESS_MODE_BR_IR,
};*/


#define IDENTIFIER_MAX 1000
enum
{
ID_TYPE_INT, // declared with (int)
ID_TYPE_REFERENCE, // declared with (int*) or maybe (ref)? The point of having a different type is to indicate that where the identifier is used as an operand, the address mode should be set to a reference mode.
};

// These have been moved to g_header.h:
//#define SOURCE_FILES 10
//#define FILE_NAME_LENGTH 20
// INCLUDED_FILES is the maximum number of files that can be #included by a source file being compiled. Any nested #includes are counted.
// FILE_NAME_LENGTH is the maximum length of a file name string.
//  these are defined in g_header.h (so they can be used in bcodenotestruct)


struct sourcestruct
{
// lines in the text array should be null-terminated, although actually they don't have to be as each time text is used bounds-checking is done
 char text [SOURCE_TEXT_LINES] [SOURCE_TEXT_LINE_LENGTH];
//  *** text array must be the same as in sourcestruct (as the code that converts bcode to source code assumes that it can treat source.text in the same way as source_edit.text)

// int src_file [SOURCE_TEXT_LINES]; // stores the index of the file that the line came from
 int from_a_file; // is 1 if loaded from a file, 0 otherwise (will be 0 if a new empty source file created in the editor, until it's saved)
 char src_file_name [FILE_NAME_LENGTH]; // should be empty if from_a_file == 0
 char src_file_path [FILE_PATH_LENGTH]; // same
};

// PRECODE_LENGTH doesn't have to be as much as CODE_MAX, because a single precode entry can correspond to several bcode entries
#define PRECODE_LENGTH 1000

enum
{
PRECODE_TYPE_NONE, // empty
PRECODE_TYPE_OPCODE,
PRECODE_TYPE_INT,
PRECODE_TYPE_LITERAL, // not sure if needed


};

enum // operand basic types
{
OPERAND_BTYPE_NUMBER,
OPERAND_BTYPE_IDENTIFIER, // we can't seperate this into variable vs reference, because an operand may be set to an identifier before the type of the identifier is known. This is fixed up in pass2
};

struct precodestruct
{
  int precode_type [PRECODE_LENGTH];
// for PRECODE_TYPE_OPCODE
  int opcode [PRECODE_LENGTH];
  int operand_basic_type [PRECODE_LENGTH] [MAX_OPERANDS]; // similar to operand_type in the interpreter, although can hold some extra values (e.g. can indicate that the operand is an identifier)
  int operand_content [PRECODE_LENGTH] [MAX_OPERANDS] [2]; // the way the content is expressed depends on the type. May be unused if the operand_type is 0 or 1. The [2] is for base/index operands.
  int operand_identifier [PRECODE_LENGTH] [MAX_OPERANDS] [2]; // if the operand is an identifier, this contains the index of the identifier in the identifiers list. The [2] is for base/index operands.
// for PRECODE_TYPE_INT
  int declaration_identifier_index [PRECODE_LENGTH];
// for PRECODE_TYPE_LITERAL
  int literal_value [PRECODE_LENGTH]; // this is used for variable declarations that are initialised to a particular value
// this variable indicates that a label points to this precode address (meaning that the label will need to be updated to point to the corresponding bcode address)
  int labels [PRECODE_LENGTH];
};


// any changes to IDENTIFIER_MAX_LENGTH may need to be reflected in PTOKEN_LENGTH in c_prepr.c
#define IDENTIFIER_MAX_LENGTH 32

enum
{
ID_NONE,

ASM_ID_OPCODE, // opcodes that can be translated directly to instructions
ASM_ID_KEYWORD, // other assembler commands, like set and aspace commands
ASM_ID_GENERIC_UNDEFINED,
ASM_ID_GENERIC_DEFINED,
ASM_ID_GENERIC_ALIAS, // if an identifier has been used within one or more nspaces but is then defined with aspace scope (nspace=-1), the version(s) of the identifier with nspace scope will be converted to aliases that are treated as references to the version with aspace scope.
ASM_ID_ASPACE,
ASM_ID_NSPACE,
//ASM_ID_USER_INT, // this is a variable that has been defined (not just declared) in assembler, so no space is allocated for it at the end of the process as it is for ints not defined in asm.
 // ASM_ID_USER_INT translates into the same ctoken type as a compiled variable, though

//ASM_ID_LITERAL_VALUE, // there aren't actually any identifiers of this type - it's used in token processing in the assembler
//ASM_ID_DECLARE_INT,

C_ID_KEYWORD,

ID_USER_UNTYPED, // temporary type that is used during parsing to indicate that a token is a user-defined identifier before we know what type it is exactly. Later turned into e.g. ID_USER_INT.

ID_USER_INT, // user-defined variable - asm or c (auto variables and arrays are c only, though)
ID_USER_LABEL, // user-defined label - asm or c
ID_USER_LABEL_UNDEFINED, // label used but not yet defined
ID_USER_CFUNCTION, // user-defined function name - compiler only (although probably could use in asm as well)
ID_BUILTIN_CFUNCTION, // built-in cfunctions - e.g. put. compiler only.
ID_PROCESS, // the name of a process
ID_ENUM, // an enum. Is converted to a number by the parser, so the compiler doesn't really know the difference.
};

enum
{
IDENTIFIER_ASM_ONLY,
IDENTIFIER_C_ONLY,
IDENTIFIER_ASM_OR_C
};

#define ARRAY_DIMENSIONS 3
#define ARRAY_DIMENSION_MAX_SIZE 512

struct identifierstruct
{
  char name [IDENTIFIER_MAX_LENGTH]; // name (null-terminated string
  int type; // the type of the identifier
  int pointer; // is it a pointer? (currently only used for arrays received as cfunction parameters)
  int asm_or_compiler; // is IDENTIFIER_ASM_ONLY, IDENTIFIER_C_ONLY, IDENTIFIER_ASM_OR_C
  int address_precode; // the address that the identifier points to (precode index)
  int address_bcode; // the address that it points to in the bcode (actual address)
  int declare_line; // line of source code in which it is declared
  int declare_pos; // pos on line of source code in which it is declared - note that this is the END of the token (i.e. it's where src_pos is when the token has been read in)
  int cfunction_index; // if it's the name of a cfunction, this is its index in the cfunction array for its parent process.
  int process_index; // if it's the name of a process, this is its index in the process_def array.
  int scope; // the index of the cfunction (in the current process). Is -1 for global identifiers within a process.
  int process_scope; // the index of the process. Is -1 for universal identifiers within a source file.
  int stack_offset; // if an auto variable (or auto parameter) local to a cfunction, this is the variable's offset from the cfunction's stack frame base.
  int storage_class; // STORAGE_STATIC or STORAGE_AUTO. For local variables, defaults to the storage class of the cfunction. For all other variables must be STORAGE_STATIC.
  int array_dims;
  int array_dim_size [ARRAY_DIMENSIONS];
  int array_element_size [ARRAY_DIMENSIONS]; // the size of one element of this dimension (e.g. in a [5] [3] [2] the size of an element of the first dim is 6; the second is 2; the last is always 1)
  int initial_value; // if it's e.g. an int variable, this is the value it is initialised to. Not used for arrays.
  int static_int_assign_at_startup; // if 1, is set to initial_value at startup (or to part of the array_init array if an array)
  int static_array_init_offset; // this is the offset of the array in the array_init array, which stores static array initialisation values. This initialisation is compile-time only.
  int static_array_initialised; // is the array initialised on declaration?
  int enum_value; // if it's an enum, this is its value
  int aspace_scope; // address space scope for assembler identifiers
  int nspace_scope; // namespace scope for assembler identifiers
  int aspace_index; // if the identifier is the name of an aspace, this is its index in the array of aspaces
  int nspace_index; // if the identifier is the name of an nspace, this is its index in the array of nspaces
  int asm_alias; // if the identifier is of type ASM_ID_GENERIC_ALIAS this is the defined generic identifier it refers to
// when adding any values to this struct, remember to add initialisation code to ac_init.c!!

};




enum // the names of these keywords are in strings in ac_init.h
{
// asm keywords that are not opcodes:
KEYWORD_ASM_INT = OP_end, // code in ac_init.c assumes that this is the first asm keyword in this enum.
//KEYWORD_ASM_SET,
//KEYWORD_ASM_DECL,
KEYWORD_ASM_DEF,
KEYWORD_ASM_LABEL,
KEYWORD_ASM_ASPACE,
KEYWORD_ASM_ASPACE_END,
KEYWORD_ASM_ASPACE_END_ADDRESS,
KEYWORD_ASM_NSPACE,
KEYWORD_ASM_NSPACE_END,
KEYWORD_ASM_NSPACE_REJOIN,
KEYWORD_ASM_SCOPE,
KEYWORD_ASM_REG_A, // code in a_asm.c assumes that all register keywords are in a continuous list, and in order.
KEYWORD_ASM_REG_B,
KEYWORD_ASM_REG_C,
KEYWORD_ASM_REG_D,
KEYWORD_ASM_REG_E,
KEYWORD_ASM_REG_F,
KEYWORD_ASM_REG_G,
KEYWORD_ASM_REG_H,
KEYWORD_ASM_DATA,
KEYWORDS_ASM_END
};

enum
{
// compiler keywords:
KEYWORD_INT = KEYWORDS_ASM_END, // code in ac_init.c assumes that this is the first compiler keyword in this enum.
KEYWORD_VOID,
KEYWORD_IF,
KEYWORD_ELSE,
KEYWORD_RETURN,
KEYWORD_STATIC,
KEYWORD_AUTO,
KEYWORD_PRINT, // special debugging command
KEYWORD_WHILE,
KEYWORD_DO,
KEYWORD_FOR,
KEYWORD_BREAK,
KEYWORD_CONTINUE,
KEYWORD_PROCESS,
KEYWORD_INTERFACE,
KEYWORD_ENUM,
KEYWORD_GOTO,
KEYWORD_ASM, // this is just "asm" used to call the inline assembler
KEYWORD_ASM_ONLY, // this is "asm_only" used to tell the compiler that a process consists solely of asm (so there's no need to set up the interface, main cfunction, static memory etc)
KEYWORD_SWITCH,
KEYWORD_CASE,
KEYWORD_DEFAULT,
KEYWORD_DATA,

KEYWORDS_END // this is used to indicate the start of the user-defined identifiers.
// Currently the number of compiler keywords is determined, in ac_init.c, by (KEYWORDS_END - KEYWORD_INT).
// So if the significance of either changes, may need to make changes there too.
// Currently, the identifier at KEYWORDS_END will be the identifier for the main cfunction for process 0 (see ac_init.c)

};

enum
{
BUILTIN_CFUNCTION_PUT = KEYWORDS_END, // code in c_init.c assumes that this is the first builtin cfunction
BUILTIN_CFUNCTION_GET,
BUILTIN_CFUNCTION_PUT_INDEX,
BUILTIN_CFUNCTION_GET_INDEX,
BUILTIN_CFUNCTION_CALL,
BUILTIN_CFUNCTION_PROCESS_START,
BUILTIN_CFUNCTION_PROCESS_END,
//BUILTIN_CFUNCTION_NEW_PROCESS,

// the get functions rely on MTYPE_PR_STD
BUILTIN_CFUNCTION_GET_X,
BUILTIN_CFUNCTION_GET_Y,
BUILTIN_CFUNCTION_GET_ANGLE,
BUILTIN_CFUNCTION_GET_SPEED_X,
BUILTIN_CFUNCTION_GET_SPEED_Y,
BUILTIN_CFUNCTION_GET_TEAM,
BUILTIN_CFUNCTION_GET_HP,
BUILTIN_CFUNCTION_GET_HP_MAX,
BUILTIN_CFUNCTION_GET_INSTR,
BUILTIN_CFUNCTION_GET_INSTR_MAX,
BUILTIN_CFUNCTION_GET_IRPT,
BUILTIN_CFUNCTION_GET_IRPT_MAX,
BUILTIN_CFUNCTION_GET_OWN_IRPT_MAX,
BUILTIN_CFUNCTION_GET_DATA,
BUILTIN_CFUNCTION_GET_DATA_MAX,
BUILTIN_CFUNCTION_GET_OWN_DATA_MAX,
BUILTIN_CFUNCTION_GET_SPIN,
BUILTIN_CFUNCTION_GET_GR_X,
BUILTIN_CFUNCTION_GET_GR_Y,
//BUILTIN_CFUNCTION_GET_GR_ANGLE,
BUILTIN_CFUNCTION_GET_GR_SPEED_X,
BUILTIN_CFUNCTION_GET_GR_SPEED_Y,
BUILTIN_CFUNCTION_GET_GR_MEMBERS,
BUILTIN_CFUNCTION_GET_WORLD_X,
BUILTIN_CFUNCTION_GET_WORLD_Y,
//BUILTIN_CFUNCTION_GET_GEN_LIMIT,
//BUILTIN_CFUNCTION_GET_GEN_NUMBER,
BUILTIN_CFUNCTION_GET_EX_TIME,
BUILTIN_CFUNCTION_GET_EFFICIENCY,
BUILTIN_CFUNCTION_GET_TIME,
BUILTIN_CFUNCTION_GET_VERTICES,
BUILTIN_CFUNCTION_GET_VERTEX_ANGLE, // these vertex modes use an extra parameter
BUILTIN_CFUNCTION_GET_VERTEX_DIST,
BUILTIN_CFUNCTION_GET_VERTEX_ANGLE_NEXT,
BUILTIN_CFUNCTION_GET_VERTEX_ANGLE_PREV,
BUILTIN_CFUNCTION_GET_VERTEX_ANGLE_MIN,
BUILTIN_CFUNCTION_GET_VERTEX_ANGLE_MAX,
BUILTIN_CFUNCTION_GET_METHOD,
BUILTIN_CFUNCTION_GET_METHOD_FIND,

// these are used by procs to deal with their own command arrays. require MTYPE_PR_COMMAND
BUILTIN_CFUNCTION_GET_COMMAND,
BUILTIN_CFUNCTION_GET_COMMAND_BIT,
BUILTIN_CFUNCTION_SET_COMMAND,
BUILTIN_CFUNCTION_SET_COMMAND_BIT_1,
BUILTIN_CFUNCTION_SET_COMMAND_BIT_0,

// these are used by clients to deal with procs' command arrays
BUILTIN_CFUNCTION_CHECK_COMMAND, // requires MTYPE_CLOB_COMMAND_REC
BUILTIN_CFUNCTION_CHECK_COMMAND_BIT, // requires MTYPE_CLOB_COMMAND_REC
BUILTIN_CFUNCTION_COMMAND, // requires MTYPE_CL_COMMAND_GIVE
BUILTIN_CFUNCTION_COMMAND_BIT_1,
BUILTIN_CFUNCTION_COMMAND_BIT_0,

// the query functions rely on MTYPE_CLOB_QUERY
BUILTIN_CFUNCTION_QUERY_X,
BUILTIN_CFUNCTION_QUERY_Y,
BUILTIN_CFUNCTION_QUERY_ANGLE,
BUILTIN_CFUNCTION_QUERY_SPEED_X,
BUILTIN_CFUNCTION_QUERY_SPEED_Y,
BUILTIN_CFUNCTION_QUERY_TEAM,
BUILTIN_CFUNCTION_QUERY_HP,
BUILTIN_CFUNCTION_QUERY_HP_MAX,
BUILTIN_CFUNCTION_QUERY_IRPT,
BUILTIN_CFUNCTION_QUERY_IRPT_MAX,
BUILTIN_CFUNCTION_QUERY_OWN_IRPT_MAX,
BUILTIN_CFUNCTION_QUERY_DATA,
BUILTIN_CFUNCTION_QUERY_DATA_MAX,
BUILTIN_CFUNCTION_QUERY_OWN_DATA_MAX,
BUILTIN_CFUNCTION_QUERY_SPIN,
BUILTIN_CFUNCTION_QUERY_GR_X,
BUILTIN_CFUNCTION_QUERY_GR_Y,
BUILTIN_CFUNCTION_QUERY_GR_ANGLE,
BUILTIN_CFUNCTION_QUERY_GR_SPEED_X,
BUILTIN_CFUNCTION_QUERY_GR_SPEED_Y,
BUILTIN_CFUNCTION_QUERY_GR_MEMBERS,
BUILTIN_CFUNCTION_QUERY_EX_TIME,
BUILTIN_CFUNCTION_QUERY_EFFICIENCY,
// some of the get_* proc cfunctions don't have query equivalents (because the CLOB_STD method deals with them)

BUILTIN_CFUNCTION_QUERY_VERTICES,
BUILTIN_CFUNCTION_QUERY_VERTEX_ANGLE, // these vertex modes use an extra parameter
BUILTIN_CFUNCTION_QUERY_VERTEX_DIST,
BUILTIN_CFUNCTION_QUERY_VERTEX_ANGLE_NEXT,
BUILTIN_CFUNCTION_QUERY_VERTEX_ANGLE_PREV,
BUILTIN_CFUNCTION_QUERY_VERTEX_ANGLE_MIN,
BUILTIN_CFUNCTION_QUERY_VERTEX_ANGLE_MAX,
BUILTIN_CFUNCTION_QUERY_METHOD,
BUILTIN_CFUNCTION_QUERY_METHOD_FIND,
BUILTIN_CFUNCTION_QUERY_MBANK,

// these functions rely on the CLOB_STD method
BUILTIN_CFUNCTION_WORLD_WORLD_X,
BUILTIN_CFUNCTION_WORLD_WORLD_Y,
BUILTIN_CFUNCTION_WORLD_PROCS,
BUILTIN_CFUNCTION_WORLD_TEAM,
BUILTIN_CFUNCTION_WORLD_TEAMS,
BUILTIN_CFUNCTION_WORLD_TEAM_SIZE,
BUILTIN_CFUNCTION_WORLD_PROCS_EACH,
BUILTIN_CFUNCTION_WORLD_FIRST_PROC,
BUILTIN_CFUNCTION_WORLD_LAST_PROC,
BUILTIN_CFUNCTION_WORLD_TIME,
//BUILTIN_CFUNCTION_WORLD_GEN_LIMIT,
//BUILTIN_CFUNCTION_WORLD_GEN_NUMBER,

// these functions rely on the maths methods
BUILTIN_CFUNCTION_HYPOT,
BUILTIN_CFUNCTION_SQRT,
BUILTIN_CFUNCTION_POW,
BUILTIN_CFUNCTION_ABS,
BUILTIN_CFUNCTION_ANGLE_DIFF,
BUILTIN_CFUNCTION_ANGLE_DIFF_S,
BUILTIN_CFUNCTION_TURN_DIR,
BUILTIN_CFUNCTION_SIN,
BUILTIN_CFUNCTION_COS,
BUILTIN_CFUNCTION_ATAN2,
//BUILTIN_CFUNCTION_RANDOM,

// these are not builtin functions; they're unimplemented C stuff that generates an error
// only unimplemented/unsupported things should come after here (as they are excluded from code completion)
BUILTIN_UNIMP_CONST, // this should be the first unimplemented thing
BUILTIN_UNIMP_SIZEOF,
BUILTIN_UNIMP_STRUCT,
BUILTIN_UNIMP_UNION,
BUILTIN_UNIMP_EXTERN,
BUILTIN_UNIMP_UNSIGNED,
BUILTIN_UNIMP_SIGNED,
BUILTIN_UNIMP_TYPEDEF,
BUILTIN_UNIMP_INLINE,
BUILTIN_UNIMP_REGISTER,

// unsupported data types
BUILTIN_UNSUP_CHAR,
BUILTIN_UNSUP_SHORT,
BUILTIN_UNSUP_FLOAT,
BUILTIN_UNSUP_DOUBLE,

// only unimplemented/unsupported things should be here at the end (after BUILTIN_UNIMP_CONST)

BUILTIN_CFUNCTIONS_END


};


#define USER_IDENTIFIERS BUILTIN_CFUNCTIONS_END

enum
{
LINE_WRAP_NO,
LINE_WRAP_YES
};

//#define REACHED_END_OF_LINE -1000
#define REACHED_END_OF_SCODE -1001




#define SCODE_LENGTH 120000

// scode contains information from a sourcestruct that has been through the preprocessor.
struct scodestruct
{
 char text [SCODE_LENGTH]; // this is the source code, in one giant string.
 s16b src_line [SCODE_LENGTH]; // this is the line number of each character in the text string.
 s16b src_line_pos [SCODE_LENGTH]; // same for position within the line
 s16b src_file [SCODE_LENGTH]; // file it came from
 char src_file_name [SOURCE_FILES] [FILE_NAME_LENGTH]; // name of the source file (SOURCE_FILES is the number of files that an scode can be derived from; it's the original file plus any #included files)

};






// this is the bit size of the register address indicators in an opcode entry
#define ADDRESS_MODE_BITFIELD_SIZE 3
// if bit in ADDRESS_MODE_MASK_REGISTER position is 1, the last 3 bits of the address mode tell us which register to use
//#define ADDRESS_MODE_BIT_REGISTER 16
//#define ADDRESS_MODE_MASK_REGISTER 7
// if the ADDRESS_MODE_MASK_REGISTER bit is 1, the result is dereferenced. Can be used on either registers or values.
//#define ADDRESS_MODE_BIT_REFERENCE 8

// ADDRESS_MODE_MASK_ALL is used as an &-mask to extract all of the bits of an address mode (assuming that the address mode bits have been bitshifted to start at 1)
#define ADDRESS_MODE_MASK_ALL 7

#define MBANK_ADDRESS_MODE_MASK (METHOD_BANK-1)
#define METHOD_ADDRESS_MODE_MASK (METHODS-1)



#define CFUNCTIONS 32
#define CFUNCTION_ARGS 16
#define CFUNCTION_AUTO 16
#define CFUNCTION_STATIC 16

enum
{
CFUNCTION_RETURN_INT,
CFUNCTION_RETURN_VOID
};

enum
{
CFUNCTION_ARG_NONE,
CFUNCTION_ARG_INT
};

#define INTERCODE_OPERANDS 3

struct intercodestruct
{

 int instr; // instruction - what is this icode entry doing?

 int operand [INTERCODE_OPERANDS];

 int src_line; // this is the line of the original source that the intercode entry is from
 int src_file; // this is the source file the line is in

 int written_to_asm; // c_inter.c can generate asm from asm. written_to_asm indicates that an intercode entry has been written as an operand and doesn't need to be written as a separate value.


};


enum
{
IC_COPY_REGISTER2_TO_REGISTER1,// REGISTER_C, REGISTER_D, 0); // copies second operand to first operand
IC_NUMBER_TO_REGISTER,// REGISTER_A, ctoken.number_value, 0);
IC_IDENTIFIER_TO_REGISTER,// REGISTER_A, ctoken.identifier_index, 0);
IC_PUSH_REGISTER,// REGISTER_A, 0, 0);
IC_POP_TO_REGISTER,// REGISTER_B, 0, 0);
IC_JUMP_TO_REGISTER,// REGISTER_A, 0,0);
IC_COPY_REGISTER_TO_IDENTIFIER_ADDRESS, // REGISTER_A, identifier address, 0);
IC_EXIT_POINT_TRUE,// conditional_exit_point, 0, 0);
IC_EXIT_POINT_FALSE,// conditional_exit_point, 0, 0);
IC_IFFALSE_JUMP_TO_EXIT_POINT,// REGISTER_A, value, 0); // value is the conditional exit point for this level of the expression
IC_IFTRUE_JUMP_TO_EXIT_POINT,// REGISTER_A, value, 0); // value is the conditional exit point for this level of the expression
IC_JUMP_EXIT_POINT_TRUE,// avoid_else_exit_point, 0, 0); // this is an unconditional jump to the avoid_else exit point
IC_JUMP_EXIT_POINT_FALSE,
IC_ADD_REG_TO_REG,// REGISTER_B, REGISTER_A, 0);
IC_SUB_REG_FROM_REG,// REGISTER_B, REGISTER_A, 0);
IC_MUL_REG_BY_REG,// REGISTER_B, REGISTER_A, 0);
IC_DIV_REG_BY_REG,// REGISTER_B, REGISTER_A, 0);
IC_MOD_REG_BY_REG,// REGISTER_B, REGISTER_A, 0);
IC_OR_REG_REG, // |
IC_AND_REG_REG, // &
IC_XOR_REG_REG, // ^
IC_BITWISE_NOT_REG_REG, // ~ (see IC_NOT_REG_REG for !)
IC_BSHIFTL_REG_REG, // <<
IC_BSHIFTR_REG_REG, // >>
IC_JUMP_TO_CFUNCTION_ADDRESS, // value, 0, 0); // value is the index in cfunction array
IC_COMPARE_REGS_EQUAL,// REGISTER_A, REGISTER_B, REGISTER_A); // this leaves register A with 1 if true, 0 if false
IC_COMPARE_REGS_NEQUAL,
IC_COMPARE_REGS_GR,
IC_COMPARE_REGS_GRE,
IC_COMPARE_REGS_LS,
IC_COMPARE_REGS_LSE,
IC_NOT_REG_REG, // ! (see IC_BITWISE_NOT_REG_REG for ~)
IC_EXIT_POINT_ADDRESS_TO_REGISTER, // REGISTER_A, exit_point, 0); //
IC_CFUNCTION_START,// cstate.scope, 0, 0);
IC_CFUNCTION_MAIN, // 0,0,0); this is set just after ic_cfunction_start; it adds any code needed at startup (e.g. initialises stack pointers)
IC_STOP, // 0,0,0); finishes the process
IC_DEREFERENCE_REGISTER_TO_REGISTER, // register1, register2, 0); // copies the contents of the address pointed at by register2 to register1. Both registers can be the same.
IC_COPY_REGISTER_TO_DEREFERENCED_REGISTER, // register1, register2, 0); // copies register2 to the address pointed at by register1
IC_IDENTIFIER_PLUS_OFFSET_TO_REGISTER, // register, identifier, offset); // copies (value of identifier + offset) to register.
IC_ID_ADDRESS_PLUS_OFFSET_TO_REGISTER, // register, identifier, offset); // copies (address of identifier + offset) to register.
IC_INCR_REGISTER,
IC_DECR_REGISTER,
IC_SWITCH_JUMP, // calculates the start of a switch jump table and jumps there

IC_REGISTER_MBANK_TO_REGISTER,
IC_KNOWN_MBANK_TO_REGISTER,
IC_REGISTER_TO_KNOWN_MBANK,
IC_REGISTER_TO_REGISTER_MBANK,
IC_NUMBER_TO_KNOWN_MBANK,
IC_CALL_METHOD_CONSTANT,
IC_CALL_METHOD_REGISTER,

IC_PRINT_STRING,
IC_STRING,
IC_STRING_END,
IC_PRINT_REGISTER,
IC_PRINT_STRING_A,
IC_PRINT_REGISTER_A,

IC_PROCESS_START, // indicates start address of a process definition
IC_PROCESS_END, // same for end
IC_PROCESS_START_TO_REGISTER, // puts the start address of a process definition (relative to start of current process!) into a register
IC_PROCESS_END_TO_REGISTER, // same for end
IC_PROCESS_START_TO_KNOWN_MBANK, // puts the address of the process's start (relative to start of current process!) into a register
IC_PROCESS_END_TO_KNOWN_MBANK,

IC_LABEL_DEFINITION,
IC_GOTO,

IC_ASM_OPCODE,
IC_ASM_NUMBER,
IC_ASM_ADDRESS,
IC_ASM_DEFINE_INT,
IC_ASM_DEFINE_LABEL,
IC_ASM_ENTER_ASPACE,
IC_ASM_END_ASPACE,
IC_ASM_ASPACE_END_ADDRESS,
IC_ASM_DEFINE_NSPACE,
IC_ASM_REJOIN_NSPACE,
IC_ASM_END_NSPACE,
IC_ASM_ADDRESS_SCOPED,
IC_ASM_START_STRING,

IC_END // this is the end of the intercode being written

// leave in the operands for now to help write the add_intercode function

};

enum
{
EXPOINT_TYPE_BASIC, // can't be used with break or continue
EXPOINT_TYPE_LOOP, // break jumps to false point; continue jumps to true point
EXPOINT_TYPE_SWITCH // break jumps to false point; can't be used with continue
};

struct expointstruct
{
 int type;
 int true_point_icode; // location in intercode
 int false_point_icode;
 int true_point_bcode; // location in bcode
 int false_point_bcode;
 int true_point_used; // 1 if the point is used, 0 if not (this determines whether it will be defined if asm is generated)
 int false_point_used;
// int offset; // is added to the address of the expoint (currently only used by switch statements; the offset is the minimum case * -1)
};

enum
{
STORAGE_AUTO,
STORAGE_STATIC
};

struct cfunctionstruct
{
 int exists; // may exist even if not prototyped, e.g. through an out-of-bounds reference
 int prototyped;
 int defined;
 int source_line; // this is the line of the original source that the function starts in.
 int intercode_address; // intercode index of the start of the function
 int identifier_index; // index of the cfunction's name in the identifier array
 int bcode_address; // this is an absolute value, not adjusted for the cfunction's process's start address (references to this address are adjusted in fix_bcode in c_inter.c)
 int return_type; // currently can be CFUNCTION_RETURN_INT or _VOID
 int size_on_stack; // how much space does this function and its automatic variables take up on the stack?
  // is calculated at the start of the intercode parser
 int storage_class; // either STORAGE_AUTO or STORAGE_STATIC

 int arguments;
 int arg_type [CFUNCTION_ARGS];
 int arg_identifier [CFUNCTION_ARGS];

 int auto_variables;
 int auto_type [CFUNCTION_AUTO];
 int auto_identifier [CFUNCTION_AUTO];

 int static_variables;
 int static_type [CFUNCTION_STATIC];
 int static_identifier [CFUNCTION_STATIC];

 int corresponding_nspace; // the nspace (used in asm) that corresponds to the scope of this cfunction. This is assigned when the cfunction definition intercode is processed in c_inter.c.

// anything added to this struct may need to be reflected in ac_init.c, where the main function is prototyped

};

struct ctokenstruct
{
 char name [IDENTIFIER_MAX_LENGTH + 10];

 int type; // what basic type?
 int subtype; // if the type is CTOKEN_TYPE_OPERATOR, this holds the type of operator it is

 int number_value; // if the type is CTOKEN_TYPE_NUMBER, holds the value

 int identifier_index; // if ctoken type is CTOKEN_TYPE_IDENTIFIER, this holds the index in the identifier array.

 int src_line; // line in original source code on which the ctoken occurs.
};

#define INTERCODE_SIZE (BCODE_MAX+100)

#define EXPOINTS 4000

#define REGISTER_WORKING 4
#define REGISTER_TEMP 5
#define REGISTER_STACK 6
#define REGISTER_STACK_FRAME 7




enum
{
CTOKEN_TYPE_NUMBER,
CTOKEN_TYPE_OPERATOR_ARITHMETIC,
CTOKEN_TYPE_OPERATOR_LOGICAL,
CTOKEN_TYPE_OPERATOR_COMPARISON,
CTOKEN_TYPE_OPERATOR_ASSIGN, // includes = += ++ etc

CTOKEN_TYPE_IDENTIFIER_NEW, // newly created identifier of unknown type.

CTOKEN_TYPE_IDENTIFIER_KEYWORD, // built-in keywords only
CTOKEN_TYPE_IDENTIFIER_USER_CFUNCTION,
CTOKEN_TYPE_IDENTIFIER_USER_VARIABLE,
//CTOKEN_TYPE_IDENTIFIER_USER_VARIABLE_UNDEFINED, // not used
CTOKEN_TYPE_IDENTIFIER_BUILTIN_CFUNCTION,
CTOKEN_TYPE_PROCESS, // the name of a process. Default process is called "self"
CTOKEN_TYPE_ENUM, // is converted to a number
CTOKEN_TYPE_LABEL, // a code label (used for goto) that has been defined
CTOKEN_TYPE_LABEL_UNDEFINED, // a code label that has been used (in a goto) but not yet defined

CTOKEN_TYPE_PUNCTUATION,

CTOKEN_TYPE_ASM_OPCODE,
CTOKEN_TYPE_ASM_KEYWORD,
CTOKEN_TYPE_ASM_GENERIC_UNDEFINED,
CTOKEN_TYPE_ASM_GENERIC_DEFINED,
CTOKEN_TYPE_ASM_GENERIC_ALIAS,
CTOKEN_TYPE_ASM_ASPACE,
CTOKEN_TYPE_ASM_NSPACE,

CTOKEN_TYPE_INVALID // error state

};

/*
Here's how out-of-scope variable references will work:

e.g. process1.process2.cfunction:variable
or cfunction:variable
or process1.variable


processes:
 - must be in the current process scope, or in a sub-process

cfunctions:
 - must be in the current process scope, or the process scope indicated by process references.

Question:
 - How to deal with references completely outside process scope? These won't do anything
  - not sure exactly
 - How to deal with references that precede process or cfunction declaration or definition?
  - since the complete scope path is provided, should be able to create dummy undefined entries.

- need to remember to deal with process offsets correctly!

Could use extern somehow.
But the advantage of the process.variable approach is that it can be used in asm code as well.


*/

enum
{
CTOKEN_SUBTYPE_PLUS, // +
CTOKEN_SUBTYPE_PLUSEQ, // +=
CTOKEN_SUBTYPE_INCREMENT, // ++
CTOKEN_SUBTYPE_MINUS, // -
CTOKEN_SUBTYPE_MINUSEQ, // -=
CTOKEN_SUBTYPE_DECREMENT, // --
CTOKEN_SUBTYPE_MULEQ, // *=
CTOKEN_SUBTYPE_MUL, // *
CTOKEN_SUBTYPE_DIVEQ, // /=
CTOKEN_SUBTYPE_DIV, // /
CTOKEN_SUBTYPE_EQ_EQ, // ==
CTOKEN_SUBTYPE_EQ, // =
CTOKEN_SUBTYPE_LESEQ, // <=
CTOKEN_SUBTYPE_BITSHIFT_L, // <<
CTOKEN_SUBTYPE_BITSHIFT_L_EQ, // <<=
CTOKEN_SUBTYPE_LESS, // <
CTOKEN_SUBTYPE_GREQ, // >=
CTOKEN_SUBTYPE_BITSHIFT_R, // >>
CTOKEN_SUBTYPE_BITSHIFT_R_EQ, // >>=
CTOKEN_SUBTYPE_GR, // >
CTOKEN_SUBTYPE_BITWISE_AND_EQ, // &=
CTOKEN_SUBTYPE_LOGICAL_AND, // &&
CTOKEN_SUBTYPE_BITWISE_AND, // &
CTOKEN_SUBTYPE_BITWISE_OR_EQ, // |=
CTOKEN_SUBTYPE_LOGICAL_OR, // ||
CTOKEN_SUBTYPE_BITWISE_OR, // |
CTOKEN_SUBTYPE_BITWISE_XOR_EQ, // ^=
CTOKEN_SUBTYPE_BITWISE_XOR, // ^
CTOKEN_SUBTYPE_BITWISE_NOT_EQ, // ~=
CTOKEN_SUBTYPE_BITWISE_NOT, // ~
CTOKEN_SUBTYPE_MODEQ, // %=
CTOKEN_SUBTYPE_MOD, // %
CTOKEN_SUBTYPE_COMPARE_NOT, // !=
CTOKEN_SUBTYPE_NOT, // !

CTOKEN_SUBTYPE_BRACE_OPEN, // {
CTOKEN_SUBTYPE_BRACE_CLOSE, // }
CTOKEN_SUBTYPE_BRACKET_OPEN, // (
CTOKEN_SUBTYPE_BRACKET_CLOSE, // )
CTOKEN_SUBTYPE_SQUARE_OPEN, // [
CTOKEN_SUBTYPE_SQUARE_CLOSE, // ]
CTOKEN_SUBTYPE_COMMA, // ,
CTOKEN_SUBTYPE_SEMICOLON, // ;
CTOKEN_SUBTYPE_COLON, // :

CTOKEN_SUBTYPE_QUOTE, // '
CTOKEN_SUBTYPE_QUOTES, // "
CTOKEN_SUBTYPE_FULL_STOP, // .
CTOKEN_SUBTYPE_DOLLAR, // $

};

#define SCOPE_GLOBAL -1

#define ARRAY_INIT_SIZE 500

struct array_initstruct
{
 int value [ARRAY_INIT_SIZE];
// int string_start [ARRAY_INIT_SIZE]
// int offset [ARRAY_INIT_SIZE];
};

// Now let's deal with processes and interfaces:

// this is the max number of processes (including nested processes) that can be defined by a source file:
#define PROCESS_DEFS 24


// each process has an interfacestruct
struct interfacestruct
{
 int defined;
 int type; // one of the PROGRAM_TYPE enums (see g_header.h)
 int shape;
 int size;
 int base_vertex;
 int method [METHODS+1] [INTERFACE_METHOD_DATA]; // INTERFACE_METHOD_DATA is defined in g_header.h
  // the data in the method array is information about each method. Sometimes, but not always, it is used to initialise mbank entries - see derive_proc_properties_from_bcode() in g_proc.c
// everything in this struct must be initialised to zero (or whatever) in ac_init.c

 struct world_preinitstruct system_program_preinit; // if the process is a system program, its interface definition should contain details about the world_preinit state

};

// there are PROCESS_DEFS process_def structs available
struct process_defstruct
{
 int defined; // whether this process exists or not
 int prototyped; // whether the process has been prototyped
 int start_address; // the start of this process in bcode. Is set when the intercode parser gets to the start of the process definition.
 int end_address; // the end of this process in bcode. Is set when the intercode parser gets to the end of the process definition. Used to set stack base.
 int parent; // index of parent process. Is -1 for process 0.
 struct interfacestruct iface; // the interface definition for this process. I would have used "interface" but it seems to be a C++ keyword or something.
// struct array_initstruct array_init;
 struct cfunctionstruct cfunction [CFUNCTIONS];
 int id_index; // index in the identifiers array of the name of the process (default process is called "self" - see ac_init.c)

 int corresponding_aspace; // the asm address space corresponding to this process

 int asm_only; // is 1 if the process is asm only (which tells the compiler not to set up the process with a main cfunction, an interface etc)

};






struct cstatestruct
{
 int scope; // index of the current function in the current process's cfunction array, or SCOPE_GLOBAL (currently -1)
 int scode_pos; // current position in scode
 int icode_pos; // current position in icode
 int src_line; // source line of current position
 int src_line_pos; // position in that line
 int src_file; // which file the line is in
 int expoint_pos;
 int array_init_pos; // position in array initialisation array

 int error;
 int recursion_level;
 int just_returned; // is 1 if the last statement was return

 int process_scope; // index of current process
 struct process_defstruct* cprocess; // pointer to current process

 int running_asm; // is 1 if we're currently in inline assembler (tells the parser to recognise assembler-only identifiers, and not recognise compiler-only identifiers)
};

struct aspacestruct
{
 int declared;
 int defined; // if defined==0, bcode_start will be unusable. parent should be usable as long as declared==1
 int bcode_start;
 int bcode_end; // final bcode of aspace - can be obtained in asm by the aspace_end_address() keyword
 int parent; // is -1 if no parent
 int corresponding_process; // is -1 if there is no corresponding process. Otherwise is the index of the corresponding process in the process_def array
 int identifier_index; // for the name of the aspace
 int self_identifier_index; // for the self identifier (used for any references to the aspace within itself, as its own name is not in scope there)
};

struct nspacestruct
{
 int declared;
 int defined;
 int bcode_start;
 int aspace_scope;
 int corresponding_cfunction; // is -1 if there is no corresponding cfunction. Otherwise is the index of the corresponding cfunction in the corresponding_process of aspace[aspace_scope].
 int identifier_index; // for the name of the nspace
};


#define ASPACES 20
#define NSPACES 120


struct astatestruct
{
 struct scodestruct* scode;
// int scode_pos;

// int src_line;
// int src_line_pos;
 struct aspacestruct aspace [ASPACES];
 struct nspacestruct nspace [NSPACES];
 int current_aspace;
 int current_nspace;

 int error;
};


// SWITCH_CASES is the number of express cases a switch can have (not counting default, and not counting gaps between case values)
#define SWITCH_CASES 100
// SWITCH_MAX_SIZE is the maximum difference between the smallest and largest case in a switch
#define SWITCH_MAX_SIZE 50

// RECURSION_LIMIT is used to limit the recursive calling of compiler functions, to avoid running out of stack space if the source is malformed or malicious (although I'm not sure how much of a problem stack space really is)
#define RECURSION_LIMIT 2000



enum
{
CERR_NONE,
CERR_READ_FAIL,
CERR_SYNTAX,
CERR_INVALID_DECLARATION,
CERR_CFUNCTION_MISTYPE,
CERR_VOID_VARIABLE,
CERR_CFUNCTION_FAIL,
CERR_PROTOTYPE_FAIL,
CERR_EXCESS_CFUNCTION_ARGS,
CERR_IDENTIFIER_REUSED,
CERR_CFUNCTION_PARAMETER_FAIL,
CERR_VAR_DECLARATION_FAIL,
CERR_VAR_INITIALISATION_FAIL,
CERR_CFUNCTION_DEFINITION_FAIL,
CERR_CFUNCTION_ARGS_MISSING,
CERR_CFUNCTION_EXCESS_ARGS,
CERR_EXIT_POINT_FAIL,
CERR_CFUNCTION_CALL_NO_OPENING_BRACKET,
CERR_CFUNCTION_CALL_NOT_ENOUGH_PARAMETERS,
CERR_CFUNCTION_CALL_TOO_MANY_PARAMETERS,
CERR_RETURN_FAIL,
CERR_UNEXPECTED_KEYWORD,
CERR_EXPECTED_ASSIGNMENT_OPERATOR,
CERR_EXCESS_CLOSE_BRACKET,
CERR_NO_CLOSE_BRACKET,
CERR_VOID_RETURN_USED,
CERR_SYNTAX_BASIC_MODE,
CERR_SYNTAX_AT_STATEMENT_START,
CERR_SYNTAX_PUNCTUATION_IN_EXPRESSION,
CERR_SYNTAX_EXPRESSION_VALUE,
CERR_SYNTAX_IF,
CERR_PROTOTYPE_EXPECTED_BRACKET_OPEN,
CERR_PROTOTYPE_EXPECTED_SEMICOLON,
CERR_PROTOTYPE_EXPECTED_NEW_IDENTIFIER,
CERR_PROTOTYPE_EXPECTED_BRACKET_CLOSE,
CERR_MAIN_PROTOTYPED,
CERR_CFUNCTION_REDEFINED,
CERR_AUTO_VAR_IN_STATIC_CFUNCTION,
CERR_CFUNCTION_PARAMETER_STORAGE_NOT_STATIC,
CERR_CFUNCTION_PARAMETER_STORAGE_NOT_AUTO,
CERR_STORAGE_SPECIFIER_MISUSE,
CERR_GLOBAL_VARIABLE_IS_AUTO,
CERR_PROTOTYPE_PARAMETER_NOT_INT,
CERR_PROTOTYPE_PARAMETER_SYNTAX,
CERR_CFUNCTION_PARAMETER_NO_CLOSE_BRACKET,
CERR_CFUNCTION_PARAMETER_NOT_INT,
CERR_CFUNCTION_PARAMETER_NOT_VARIABLE,
CERR_CFUNCTION_PARAMETER_MISMATCH, // identifier of parameter doesn't match prototype
CERR_CFUNCTION_PARAMETER_NO_COMMA,
CERR_DEREFERENCED_CONSTANT_OUT_OF_BOUNDS,
CERR_NEGATIVE_NOT_A_NUMBER,
CERR_ARRAY_TOO_MANY_DIMENSIONS,
CERR_ARRAY_DIMENSION_NOT_A_NUMBER,
CERR_ARRAY_DIMENSION_TOO_SMALL,
CERR_ARRAY_DIMENSION_TOO_LARGE,
CERR_ARRAY_DECLARATION_ERROR,
CERR_ARRAY_REFERENCE_ERROR,
CERR_NO_CLOSE_SQUARE_BRACKET,
CERR_ARRAY_EXPECTED_SQUARE_OPEN,
CERR_ADDRESS_OF_PREFIX_MISUSE,
CERR_DEREFERENCED_ADDRESS_OF,
CERR_NEGATED_ADDRESS_OF,
CERR_CFUNCTION_PARAMETER_ARRAY_NO_SQ_OP,
CERR_CFUNCTION_PARAMETER_ARRAY_NO_SQ_CL,
CERR_CFUNCTION_PARAMETER_ARRAY_MISMATCH,
CERR_ARRAY_ARG_TYPE_MISMATCH,
CERR_STRING_TOO_LONG,
CERR_EXPECTED_BRACKET_OPEN,
CERR_WHILE_MISSING_OPENING_BRACKET,
CERR_WHILE_MISSING_CLOSING_BRACKET,
CERR_BREAK_WITHOUT_LOOP_ETC,
CERR_CONTINUE_WITHOUT_LOOP,
CERR_EXPECTED_SEMICOLON,
CERR_DO_WITHOUT_WHILE,
CERR_FOR_MISSING_OPENING_BRACKET,
CERR_EXPECTED_CLOSING_BRACKET_IN_FOR,
CERR_EXPECTED_BRACKET_CLOSE,
CERR_MBANK_ADDRESS_OUT_OF_BOUNDS,
CERR_ARRAY_INITIALISATION_TOO_LARGE,
CERR_ARRAY_DIM_OUT_OF_BOUNDS,
CERR_TOO_MANY_ARRAY_DIMENSIONS,
CERR_ARRAY_INITIALISER_NOT_CONSTANT,
CERR_EXPECTED_BRACE_OPEN,
CERR_EXPECTED_BRACE_CLOSE,
CERR_INTERFACE_ALREADY_DEFINED,
CERR_EXPECTED_NUMBER_CONSTANT,
CERR_EXPECTED_COMMA_AFTER_VALUE,
CERR_IFACE_TOO_MANY_METHODS,
CERR_IFACE_METHOD_TOO_MUCH_DATA,
CERR_BUILTIN_CFUNCTION_OUTSIDE_EXPRESSION,
CERR_TOO_MANY_PROCESSES,
CERR_PROCESS_NAME_INVALID,
CERR_PROCESS_ALREADY_DEFINED,
CERR_REACHED_END_IN_SUBPROCESS,
CERR_EXPECTED_PROCESS_NAME,
CERR_NEW_PROCESS_METHOD_NUMBER,
CERR_EXPECTED_COMMA_AFTER_PROCESS_NAME,
CERR_IDENTIFIER_ALREADY_USED,
CERR_EXPECTED_COMMA_AFTER_ENUM,
CERR_METHOD_OUT_OF_BOUNDS,
CERR_COMPLEX_CALL_MUST_BE_CONSTANT,
CERR_LABEL_ALREADY_DEFINED,
CERR_EXPECTED_LABEL,
CERR_PROCESS_NOT_PROTOTYPED,
CERR_PROCESS_NAME_ALREADY_USED_IN_ASM,
CERR_CFUNCTION_NAME_ALREADY_USED_IN_ASM,
CERR_NEW_IDENTIFIER_FAIL,
CERR_UNRECOGNISED_PROGRAM_TYPE,
CERR_EXPECTED_SEMICOLON_AFTER_PROCESS_PROTOTYPE,
CERR_GET_PARAMETER_NOT_CONSTANT,
CERR_PUT_PARAMETER_NOT_CONSTANT,
CERR_CASE_WITHOUT_SWITCH,
CERR_DEFAULT_WITHOUT_SWITCH,
CERR_TOO_MANY_CASES_IN_SWITCH,
CERR_CASE_VALUE_NOT_CONSTANT,
CERR_EXPECTED_COLON_AFTER_CASE,
CERR_DUPLICATE_CASE,
CERR_EXPECTED_CASE_OR_DEFAULT,
CERR_SWITCH_TOO_LARGE,
CERR_SWITCH_HAS_NO_CASES,
CERR_DUPLICATE_DEFAULT,
CERR_BUILTIN_CFUNCTION_TOO_FEW_ARGS,
CERR_NO_MTYPE_PR_MATHS,
CERR_NO_MTYPE_CL_MATHS,
CERR_NO_MTYPE_PR_STD,
CERR_NO_METHOD_FOR_BUILTIN_CFUNCTION,
CERR_NO_MTYPE_CLOB_QUERY,
CERR_NO_MTYPE_PR_COMMAND,
CERR_NO_MTYPE_CL_COMMAND_GIVE,
CERR_NO_MTYPE_CLOB_COMMAND_REC,
CERR_UNIMPLEMENTED_KEYWORD,
CERR_UNSUPPORTED_DATA_TYPE,
CERR_VAR_NAME_IN_USE,
CERR_RECURSION_LIMIT_REACHED_IN_STATEMENT,
CERR_RECURSION_LIMIT_REACHED_IN_EXPRESSION,
CERR_RECURSION_LIMIT_REACHED_BY_PARSER,
CERR_EXPECTED_OPEN_BRACE_OR_METHOD_LABEL,
CERR_EXPECTED_COLON_AFTER_METHOD_LABEL,

// errors from the parser (in c_ctoken.c):
CERR_PARSER_SCODE_BOUNDS,
CERR_PARSER_REACHED_END_WHILE_READING,
CERR_PARSER_UNKNOWN_TOKEN_TYPE,
CERR_PARSER_NUMBER_TOO_LARGE,
CERR_PARSER_LETTER_IN_NUMBER,
CERR_PARSER_TOKEN_TOO_LONG,
CERR_PARSER_NUMBER_STARTS_WITH_ZERO,
CERR_PARSER_LETTER_IN_HEX_NUMBER,
CERR_INVALID_NUMBER_IN_BINARY_NUMBER,
CERR_PARSER_LETTER_IN_BINARY_NUMBER,

// errors from parse_data (in c_data.c):
CERR_DATA_UNRECOGNISED_DATA_TYPE,
CERR_DATA_PARAMETER_NOT_NUMBER,
CERR_DATA_INVALID_SHAPE,
CERR_DATA_INVALID_SIZE,
CERR_DATA_INVALID_VERTEX,
CERR_DATA_NOT_ENOUGH_PARAMETERS,

CERRS
};




#endif
