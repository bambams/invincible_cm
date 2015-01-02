#include <allegro5/allegro.h>
#include <stdio.h>
#include <stdlib.h>

#include "m_config.h"
#include "g_header.h"
#include "c_header.h"
#include "g_misc.h"
#include "g_method.h"
#include "i_console.h"
#include "i_error.h"

/*

This file contains the bytecode interpreter.

Features:


Instructions:


*/


extern const struct opcode_propertiesstruct opcode_properties [OP_end]; // defined in a_asm.c



// GET_NEXT_INSTR gets the next 16 bit code from op, with bounds-checking

// COST_INSTR reduces instr_left and checks for running out
//  (this should probably return 0 rather than 1, although I don't think the return value is actually used for anything)
//#define COST_INSTR instr_left --; if (instr_left <= 0) return 1;
//#define COST_INSTR_2 instr_left -= 2; if (instr_left <= 0) return 1;
//#define COST_INSTR_3 instr_left -= 3; if (instr_left <= 0) return 1;
//#define COST_INSTR_4 instr_left -= 4; if (instr_left <= 0) return 1;
//#define COST_INSTR_5 instr_left -= 5; if (instr_left <= 0) return 1;

// specific instruction types:
#define COST(cost) if (instr_left < cost) return instr_left; instr_left -= cost;

#define COST_READ 1
#define COST_ADD 1
#define COST_MUL 1
#define COST_DIV 2
#define COST_SET 1
#define COST_REFER 1
#define COST_JUMP 1
#define COST_BITWISE 1
#define COST_IF 1
#define COST_COMPARE 1
#define COST_STACK 1
#define COST_MBANK 1
#define COST_CALL 1

// Should add a BERR error message to this
#define CHECK_REFERENCE_INSTR if (instr < 0 || instr >= bcode_size) return 0;

#define CHECK_REFERENCED_ADDRESS if (referenced_address < 0 || referenced_address >= bcode_size) return 0;

//#define GET_OPCODE_UNSIGNED opcode=(unsigned_instr&31744)>>10;
// 31744 = 0111110000000000

//struct bcodestruct bcode;

//int interpret_bcode(struct bcodestruct* bc, struct registerstruct* regs, struct methodstruct method [METHODS]);

int bcode_error(int berr_type, int bcode_op, struct bcodestruct* bc, struct procstruct* pr, struct programstruct* cl);

enum
{
BERR_ACCESS_OUT_OF_BOUNDS,
BERR_CONTROL_OUT_OF_BOUNDS,
BERR_INVALID_OPCODE,
BERR_STRING_OUT_OF_BOUNDS,
BERR_STRING_TOO_LONG,
BERR_MBANK_ACCESS_OUT_OF_BOUNDS,
BERRS
};

const char *berror_name [BERRS] =
{
 "access out of bounds", // BERR_ACCESS_OUT_OF_BOUNDS
 "control out of bounds", // BERR_CONTROL_OUT_OF_BOUNDS
 "invalid instruction", // BERR_INVALID_OPCODE
 "string out of bounds", // BERR_STRING_OUT_OF_BOUNDS
 "string too long", // BERR_STRING_TOO_LONG
 "method bank access out of bounds", // BERR_MBANK_ACCESS_OUT_OF_BOUNDS,
};



enum
{
ARG_TYPE_VALUE,
ARG_TYPE_REGISTER
};

const char* reg_name [8] =
{
 "A",
 "B",
 "C",
 "D",
 "E",
 "F",
 "G",
 "H"
};

//#define PRINT_ASM


// returns 0 on error, 1 on success
// Note that pr will be NULL if a client program is being interpreted, so do not dereference it unless it is known to be valid
int interpret_bcode(struct procstruct* pr, struct programstruct* cl, struct bcodestruct* bc, struct registerstruct* regs, struct methodstruct* methods, int instr_left)
{

 int c; // this is the bcode counter
// int instr_left = 1000;
 s16b* op = bc->op;
 int instr = 0;
 unsigned int unsigned_instr;
 unsigned int unsigned_instr2;
 int bcode_size = bc->bcode_size;

 u16b opcode;
 u16b instruction_op;

// always initialise registers to zero
 int i;
 for (i = 0; i < REGISTERS; i ++)
 {
  regs->reg [i]= 0;
 }

 bc->printing = 0;

 c = -1; // this is the current position in the bcode - it's used in the various macros. Is -1 because it's incremented before being used

 while (instr_left > 0)
 {
// note that at this point c (the instruction position counter) may be -1 (as a result of a jump to position zero).
// as c is not bounds-checked below zero, it must be incremented before being used.
// c may also be above bcode_size.

  c ++;
  if (c >= bcode_size)
  {
   bcode_error(BERR_CONTROL_OUT_OF_BOUNDS, c, bc, pr, cl);
   return instr_left;
  }

  instruction_op = op [c];//abs(op [c]);
  opcode = abs(instruction_op >> OPCODE_BITSHIFT); //instruction_op;// >> OPCODE_BITSHIFT; // the >> should get rid of everything except the relevant bits that indicate which instruction it is.
  COST(COST_READ);
/*
  if (opcode >= 0 && opcode < OP_end)
  {
   fprintf(stdout, "\n%i: %s", c, opcode_properties[opcode].name);

  }
   else
    fprintf(stdout, "\n%i: invalid opcode", c);*/

/*  fprintf(stdout, "\n%i op: %i (%i) bin: ", c, unsigned_instr, opcode);
  print_binary(unsigned_instr);
  fprintf(stdout, ", ");
  print_binary(opcode);*/

//  if (opcode >= OP_end) - use default case instead
//   return 3; // invalid opcode

//  if (pr != NULL && pr->index == 1)
//   fprintf(stdout, "(%i:%u)", instruction_op, opcode);

  switch(opcode)
  {
// TO DO: think about cost for all of these
   case OP_nop:
#ifdef PRINT_ASM
    fprintf(stdout, "%i nop ", c);
#endif
//    COST_INSTR  no cost for nop - already paid 1 to read instruction
    break;

#define REG_OP_1 regs->reg[instruction_op & ADDRESS_MODE_MASK_ALL]
#define REG_OP_2 (regs->reg[(instruction_op >> ADDRESS_MODE_BITFIELD_SIZE) & ADDRESS_MODE_MASK_ALL])
#define REG_OP_3 (regs->reg[(instruction_op >> (ADDRESS_MODE_BITFIELD_SIZE*2)) & ADDRESS_MODE_MASK_ALL])
// Note that if an instruction has only one register operand, it will be REG_OP_1 (and REG_1) even if it isn't the first operand.
// E.g. setar uses REG_OP_1 even though the register is the second operand.

#define REG_1 (instruction_op & ADDRESS_MODE_MASK_ALL)
#define REG_2 ((instruction_op >> ADDRESS_MODE_BITFIELD_SIZE) & ADDRESS_MODE_MASK_ALL)
#define REG_3 ((instruction_op >> (ADDRESS_MODE_BITFIELD_SIZE*2)) & ADDRESS_MODE_MASK_ALL)

#ifdef USE_GCC_EXPECT
// Uses GCC's __builtin_expect() to optimise bounds-checking for bcode memory addresses (which should almost always be within bounds)

#define GET_NEXT_INSTR c ++; if (__builtin_expect(c >= bcode_size, 0)) { bcode_error(BERR_CONTROL_OUT_OF_BOUNDS, c, bc, pr, cl); return instr_left;; } instr = op [c];
#define GET_NEXT_INSTR_UNSIGNED c ++; if (__builtin_expect(c >= bcode_size, 0)) { bcode_error(BERR_CONTROL_OUT_OF_BOUNDS, c, bc, pr, cl); return instr_left; } unsigned_instr = (unsigned int) op [c];
#define GET_NEXT_INSTR_UNSIGNED_ADDRESS c ++; if (__builtin_expect(c >= bcode_size, 0)) { bcode_error(BERR_CONTROL_OUT_OF_BOUNDS, c, bc, pr, cl); return instr_left; } unsigned_instr = op [c]; if (__builtin_expect(unsigned_instr >= bcode_size, 0))  { bcode_error(BERR_ACCESS_OUT_OF_BOUNDS, c, bc, pr, cl); return instr_left; }
// IGNORE_NEXT_INSTR is like GET_NEXT_INSTR, but nothing is done with the content of the instruction
#define IGNORE_NEXT_INSTR c ++; if (__builtin_expect(c >= bcode_size, 0)) { bcode_error(BERR_CONTROL_OUT_OF_BOUNDS, c, bc, pr, cl); return instr_left; }

#else
// Unoptimised :(

#define GET_NEXT_INSTR c ++; if (c >= bcode_size) { bcode_error(BERR_CONTROL_OUT_OF_BOUNDS, c, bc, pr, cl); return instr_left;; } instr = op [c];
#define GET_NEXT_INSTR_UNSIGNED c ++; if (c >= bcode_size) { bcode_error(BERR_CONTROL_OUT_OF_BOUNDS, c, bc, pr, cl); return instr_left; } unsigned_instr = (unsigned int) op [c];
#define GET_NEXT_INSTR_UNSIGNED_ADDRESS c ++; if (c >= bcode_size) { bcode_error(BERR_CONTROL_OUT_OF_BOUNDS, c, bc, pr, cl); return instr_left; } unsigned_instr = op [c]; if (unsigned_instr >= bcode_size)  { bcode_error(BERR_ACCESS_OUT_OF_BOUNDS, c, bc, pr, cl); return instr_left; }
// IGNORE_NEXT_INSTR is like GET_NEXT_INSTR, but nothing is done with the content of the instruction
#define IGNORE_NEXT_INSTR c ++; if (c >= bcode_size) { bcode_error(BERR_CONTROL_OUT_OF_BOUNDS, c, bc, pr, cl); return instr_left; }

#endif

//#define PRINT_ASM

// arithmetic & similar
   case OP_setrr: // setrr [target register] [source register]
// no additional cost for pure register operation
#ifdef PRINT_ASM
    fprintf(stdout, "\n%i setrr %s %s (%i)", c, reg_name [REG_1], reg_name [REG_2], REG_OP_2);
#endif
    REG_OP_1 = REG_OP_2;
    break;
   case OP_setra: // setrm [target register] [address of value to be put into target register]
    COST(COST_READ + COST_REFER);
#ifdef PRINT_ASM
    fprintf(stdout, "\n%i ", c);
#endif
    GET_NEXT_INSTR_UNSIGNED_ADDRESS // reads next op into unsigned_instr and confirms that it points to a valid memory address
#ifdef PRINT_ASM
    fprintf(stdout, "setra %s %i (%i)", reg_name [REG_1], unsigned_instr, op [unsigned_instr]);
#endif
    REG_OP_1 = op [unsigned_instr];
    break;
   case OP_setar: // setrm [target address] [source register]
    COST(COST_READ + COST_REFER);
#ifdef PRINT_ASM
    fprintf(stdout, "\n%i ", c);
#endif
    GET_NEXT_INSTR_UNSIGNED_ADDRESS // reads next op into unsigned_instr and confirms that it points to a valid memory address
#ifdef PRINT_ASM
    if (pr != NULL)
     fprintf(stdout, "\n%i: setar %i %s (%i)", c, unsigned_instr, reg_name [REG_1], REG_OP_1);
#endif

    op [unsigned_instr] = REG_OP_1;
    break;
/*   case OP_setaa: // setmm [target address] [source address]
    COST(COST_REFER * 2);
#ifdef PRINT_ASM
    fprintf(stdout, "\n%i ", c);
#endif
    GET_NEXT_INSTR_UNSIGNED_ADDRESS // reads next op into unsigned_instr and confirms that it points to a valid memory address
    unsigned_instr2 = unsigned_instr;
    GET_NEXT_INSTR_UNSIGNED_ADDRESS // reads next op into unsigned_instr and confirms that it points to a valid memory address
#ifdef PRINT_ASM
    fprintf(stdout, "setaa %i %i (%i)", unsigned_instr2, unsigned_instr, op [unsigned_instr]);
#endif
    op [unsigned_instr2] = op [unsigned_instr];
    break;*/
   case OP_setrn: // setrn [target register] [number]
    COST(COST_READ);
#ifdef PRINT_ASM
    fprintf(stdout, "\n%i ", c);
#endif
    GET_NEXT_INSTR // reads next op into instr (as a signed int)
#ifdef PRINT_ASM
    fprintf(stdout, "setrn %s %i", reg_name [REG_1], instr);
#endif
    REG_OP_1 = instr;
//    COST_INSTR
    break;
/*   case OP_setan:
    COST(COST_REFER);
#ifdef PRINT_ASM
    fprintf(stdout, "\n%i ", c);
#endif
    GET_NEXT_INSTR_UNSIGNED_ADDRESS // reads next op into unsigned_instr and confirms that it points to a valid memory address
    GET_NEXT_INSTR // reads next op into instr (as a signed int)
    op [unsigned_instr] = instr;
    COST_INSTR_4 // TO DO: think about this cost
#ifdef PRINT_ASM
    fprintf(stdout, "setan %i %i", unsigned_instr, instr);
#endif
    break;*/
   case OP_setrd: // setrd [target register] [register to be dereferenced]
    COST(COST_REFER);
#ifdef PRINT_ASM
    fprintf(stdout, "\n%i ", c);
#endif
    unsigned_instr = REG_OP_2;
    if (unsigned_instr >= bcode_size)
    {
     bcode_error(BERR_ACCESS_OUT_OF_BOUNDS, c, bc, pr, cl);
     return instr_left;
    }
#ifdef PRINT_ASM
    fprintf(stdout, "setrd %s %s (%i: %i)", reg_name [REG_1], reg_name [REG_2], unsigned_instr, op [unsigned_instr]);
#endif
    REG_OP_1 = op [unsigned_instr];
    break;
   case OP_setdr: // setdr [register containing target address] [source register]
    COST(COST_REFER);
#ifdef PRINT_ASM
    fprintf(stdout, "\n%i ", c);
#endif
    unsigned_instr = REG_OP_1;
    if (unsigned_instr >= bcode_size)
    {
     bcode_error(BERR_ACCESS_OUT_OF_BOUNDS, c, bc, pr, cl);
     return instr_left;
    }
#ifdef PRINT_ASM
    fprintf(stdout, "setdr %s %s (%i: %i)", reg_name [REG_1], reg_name [REG_2], unsigned_instr, REG_OP_2);
#endif
    op [unsigned_instr] = REG_OP_2;
    break;
// Arithmetic
   case OP_add: // add [target register] [register1] [register2]
    COST(COST_ADD);
#ifdef PRINT_ASM
    fprintf(stdout, "\n%i add %s %s %s (%i + %i = ", c, reg_name [REG_1], reg_name [REG_2], reg_name [REG_3], REG_OP_2, REG_OP_3);
#endif
    REG_OP_1 = REG_OP_2 + REG_OP_3;
#ifdef PRINT_ASM
    fprintf(stdout, "%i)", REG_OP_1);
#endif
    break;
   case OP_sub:
    COST(COST_ADD);
#ifdef PRINT_ASM
    fprintf(stdout, "\n%i sub %s %s %s (%i - %i = ", c, reg_name [REG_1], reg_name [REG_2], reg_name [REG_3], REG_OP_2, REG_OP_3);
#endif
    REG_OP_1 = REG_OP_2 - REG_OP_3;
#ifdef PRINT_ASM
    fprintf(stdout, "%i)", REG_OP_1);
#endif
    break;
   case OP_mul:
    COST(COST_MUL);
#ifdef PRINT_ASM
    fprintf(stdout, "\n%i mul %s %s %s (%i * %i = ", c, reg_name [REG_1], reg_name [REG_2], reg_name [REG_3], REG_OP_2, REG_OP_3);
#endif
    REG_OP_1 = REG_OP_2 * REG_OP_3;
#ifdef PRINT_ASM
    fprintf(stdout, "%i)", REG_OP_1);
#endif
    break;
   case OP_div:
    COST(COST_DIV);
#ifdef PRINT_ASM
    fprintf(stdout, "\n%i div %s %s %s (%i / %i = ", c, reg_name [REG_1], reg_name [REG_2], reg_name [REG_3], REG_OP_2, REG_OP_3);
#endif
    instr = REG_OP_3;
    if (instr == 0)
     REG_OP_1 = 0;
      else
       REG_OP_1 = REG_OP_2 / REG_OP_3;
#ifdef PRINT_ASM
    fprintf(stdout, "%i)", REG_OP_1);
#endif
    break;
   case OP_mod:
    COST(COST_DIV);
#ifdef PRINT_ASM
    fprintf(stdout, "\n%i mod %s %s %s (%i mod %i = ", c, reg_name [REG_1], reg_name [REG_2], reg_name [REG_3], REG_OP_2, REG_OP_3);
#endif
    instr = REG_OP_3;
    if (instr == 0)
     REG_OP_1 = 0;
      else
       REG_OP_1 = REG_OP_2 % REG_OP_3;
#ifdef PRINT_ASM
    fprintf(stdout, "%i)", REG_OP_1);
#endif
    break;
   case OP_incr:
//    COST(COST_ADD);
#ifdef PRINT_ASM
    fprintf(stdout, "\n%i incr %s (%i++ = ", c, reg_name [REG_1], REG_OP_1);
#endif
    REG_OP_1 ++;
#ifdef PRINT_ASM
    fprintf(stdout, "%i)", REG_OP_1);
#endif
    break;
   case OP_decr:
//    COST(COST_ADD);
#ifdef PRINT_ASM
    fprintf(stdout, "\n%i decr %s (%i-- = ", c, reg_name [REG_1], REG_OP_1);
#endif
    REG_OP_1 --;
#ifdef PRINT_ASM
    fprintf(stdout, "%i)", REG_OP_1);
#endif
    break;

// bitwise
   case OP_and:
    COST(COST_BITWISE);
#ifdef PRINT_ASM
    fprintf(stdout, "\n%i and %s %s %s (%i & %i = ", c, reg_name [REG_1], reg_name [REG_2], reg_name [REG_3], REG_OP_2, REG_OP_3);
#endif
    REG_OP_1 = REG_OP_2 & REG_OP_3;
#ifdef PRINT_ASM
    fprintf(stdout, "%i)", REG_OP_1);
#endif
    break;
   case OP_or:
    COST(COST_BITWISE);
#ifdef PRINT_ASM
    fprintf(stdout, "\n%i or %s %s %s (%i | %i = ", c, reg_name [REG_1], reg_name [REG_2], reg_name [REG_3], REG_OP_2, REG_OP_3);
#endif
    REG_OP_1 = REG_OP_2 | REG_OP_3;
#ifdef PRINT_ASM
    fprintf(stdout, "%i)", REG_OP_1);
#endif
    break;
   case OP_xor:
    COST(COST_BITWISE);
#ifdef PRINT_ASM
    fprintf(stdout, "\n%i xor %s %s %s (%i ^ %i = ", c, reg_name [REG_1], reg_name [REG_2], reg_name [REG_3], REG_OP_2, REG_OP_3);
#endif
    REG_OP_1 = REG_OP_2 ^ REG_OP_3;
#ifdef PRINT_ASM
    fprintf(stdout, "%i)", REG_OP_1);
#endif
    break;
// not
   case OP_bnot:
    COST(COST_BITWISE);
#ifdef PRINT_ASM
    fprintf(stdout, "\n%i bnot %s %s (~%i = ", c, reg_name [REG_1], reg_name [REG_2], REG_OP_2);
#endif
    REG_OP_1 = ~REG_OP_2;
#ifdef PRINT_ASM
    fprintf(stdout, "%i)", REG_OP_1);
#endif
    break;
   case OP_not:
    COST(COST_BITWISE); // not actually bitwise but that's okay
#ifdef PRINT_ASM
    fprintf(stdout, "\n%i not %s %s (!%i = ", c, reg_name [REG_1], reg_name [REG_2], REG_OP_2);
#endif
    REG_OP_1 = !REG_OP_2;
#ifdef PRINT_ASM
    fprintf(stdout, "%i)", REG_OP_1);
#endif
    break;
   case OP_bshiftl:
    COST(COST_BITWISE);
#ifdef PRINT_ASM
    fprintf(stdout, "\n%i bshiftl %s %s %s (%i << %i = ", c, reg_name [REG_1], reg_name [REG_2], reg_name [REG_3], REG_OP_2, REG_OP_3);
#endif
    REG_OP_1 = REG_OP_2 << REG_OP_3;
#ifdef PRINT_ASM
    fprintf(stdout, "%i)", REG_OP_1);
#endif
    break;
   case OP_bshiftr:
    COST(COST_BITWISE);
#ifdef PRINT_ASM
    fprintf(stdout, "\n%i bshiftr %s %s %s (%i >> %i = ", c, reg_name [REG_1], reg_name [REG_2], reg_name [REG_3], REG_OP_2, REG_OP_3);
#endif
    REG_OP_1 = REG_OP_2 >> REG_OP_3;
#ifdef PRINT_ASM
    fprintf(stdout, "%i)", REG_OP_1);
#endif
    break;
// branching
   case OP_jumpr:
    COST(COST_JUMP);
    unsigned_instr = REG_OP_1;
    if (unsigned_instr >= bcode_size)
    {
     bcode_error(BERR_ACCESS_OUT_OF_BOUNDS, c, bc, pr, cl);
     return instr_left;
    }
#ifdef PRINT_ASM
    fprintf(stdout, "\n%i jumpr %s (%i)", c, reg_name [REG_1], REG_OP_1);
#endif
    c = unsigned_instr - 1;
    break;
   case OP_jumpn:
    COST(COST_READ + COST_JUMP);
#ifdef PRINT_ASM
    fprintf(stdout, "\n%i ", c);
#endif
    GET_NEXT_INSTR_UNSIGNED_ADDRESS // reads next op into unsigned_instr and confirms that it points to a valid memory address
#ifdef PRINT_ASM
    fprintf(stdout, "jumpn %i", unsigned_instr);
#endif
    c = unsigned_instr - 1;
    break;
   case OP_iftrue: // iftrue [register to check for truth] [memory address to jump to if true]
    COST(COST_IF);
    c++;
    if (c >= bcode_size)
    {
     bcode_error(BERR_ACCESS_OUT_OF_BOUNDS, c, bc, pr, cl);
     return instr_left;
    }
    if (REG_OP_1)
    {
     if (op [c] >= 0 && op [c] < bcode_size)
     {
      COST(COST_JUMP);
      c = op [c] - 1;
     }
       else
       {
        bcode_error(BERR_ACCESS_OUT_OF_BOUNDS, c, bc, pr, cl);
        return instr_left;
       }
    }
/*     else
     {
      COST_INSTR
     }*/
#ifdef PRINT_ASM
    fprintf(stdout, "\n%i iftrue %s (%i)", c, reg_name [REG_1], REG_OP_1);
#endif
    break;
   case OP_iffalse: // like iftrue, but false
    COST(COST_IF);
    c++;
    if (c >= bcode_size)
    {
     bcode_error(BERR_ACCESS_OUT_OF_BOUNDS, c, bc, pr, cl);
     return instr_left;
    }
    if (!(REG_OP_1))
    {
     if (op [c] >= 0 && op [c] < bcode_size)
     {
      COST(COST_JUMP);
      c = op [c] - 1;
     }
       else
       {
        bcode_error(BERR_ACCESS_OUT_OF_BOUNDS, c, bc, pr, cl);
        return instr_left;
       }
    }
/*     else
     {
      COST_INSTR
     }*/
#ifdef PRINT_ASM
    fprintf(stdout, "\n%i iffalse %s (%i)", c, reg_name [REG_1], REG_OP_1);
#endif
    break;
   case OP_cmpeq: // cmpeq [register1] [register2]  - compares reg1 and reg2, puts result (true or false) in register1
#ifdef PRINT_ASM
    fprintf(stdout, "\n%i cmpeq %s %s ((%i == %i) == ", c, reg_name [REG_1], reg_name [REG_2], REG_OP_1, REG_OP_2);
#endif
    COST(COST_COMPARE);
    if (REG_OP_1 == REG_OP_2)
     REG_OP_1 = 1;
      else
       REG_OP_1 = 0;
#ifdef PRINT_ASM
    fprintf(stdout, "%i)", REG_OP_1);
#endif
    break;
   case OP_cmpneq:
    COST(COST_COMPARE);
#ifdef PRINT_ASM
    fprintf(stdout, "\n%i cmpneq %s %s ((%i != %i) == ", c, reg_name [REG_1], reg_name [REG_2], REG_OP_1, REG_OP_2);
#endif
    if (REG_OP_1 != REG_OP_2)
     REG_OP_1 = 1;
      else
       REG_OP_1 = 0;
#ifdef PRINT_ASM
    fprintf(stdout, "%i)", REG_OP_1);
#endif
    break;
   case OP_cmpgr:
    COST(COST_COMPARE);
#ifdef PRINT_ASM
    fprintf(stdout, "\n%i cmpgr %s %s ((%i > %i) == ", c, reg_name [REG_1], reg_name [REG_2], REG_OP_1, REG_OP_2);
#endif
    if (REG_OP_1 > REG_OP_2)
     REG_OP_1 = 1;
      else
       REG_OP_1 = 0;
#ifdef PRINT_ASM
    fprintf(stdout, "%i)", REG_OP_1);
#endif
    break;
   case OP_cmpgre:
    COST(COST_COMPARE);
#ifdef PRINT_ASM
    fprintf(stdout, "\n%i cmpgre %s %s ((%i >= %i) == ", c, reg_name [REG_1], reg_name [REG_2], REG_OP_1, REG_OP_2);
#endif
    if (REG_OP_1 >= REG_OP_2)
     REG_OP_1 = 1;
      else
       REG_OP_1 = 0;
#ifdef PRINT_ASM
    fprintf(stdout, "%i)", REG_OP_1);
#endif
    break;
   case OP_cmpls:
    COST(COST_COMPARE);
#ifdef PRINT_ASM
    fprintf(stdout, "\n%i cmpls %s %s ((%i < %i) == ", c, reg_name [REG_1], reg_name [REG_2], REG_OP_1, REG_OP_2);
#endif
    if (REG_OP_1 < REG_OP_2)
     REG_OP_1 = 1;
      else
       REG_OP_1 = 0;
#ifdef PRINT_ASM
    fprintf(stdout, "%i)", REG_OP_1);
#endif
    break;
   case OP_cmplse:
    COST(COST_COMPARE);
#ifdef PRINT_ASM
    fprintf(stdout, "\n%i cmplse %s %s ((%i <= %i) == ", c, reg_name [REG_1], reg_name [REG_2], REG_OP_1, REG_OP_2);
#endif
    if (REG_OP_1 <= REG_OP_2)
     REG_OP_1 = 1;
      else
       REG_OP_1 = 0;
#ifdef PRINT_ASM
    fprintf(stdout, "%i)", REG_OP_1);
#endif
    break;
   case OP_exit:
/*    if (pr != NULL)
    {
//  fprintf(stdout, "\nProgram started with %i, ", start_instr);
     fprintf(stdout, "\nProc %i started with %i, ", pr->index, start_instr);
     fprintf(stdout, "finished with %i (used %i)", *instr_left, start_instr - *instr_left);
    }*/
    return instr_left; // success!
   case OP_push: // push [register to push] [stack pointer]
    COST(COST_STACK);
    REG_OP_2 ++;
    unsigned_instr = REG_OP_2;
    if (unsigned_instr >= bcode_size)
    {
     bcode_error(BERR_ACCESS_OUT_OF_BOUNDS, c, bc, pr, cl);
     return instr_left;
    }
    op [unsigned_instr] = REG_OP_1;
//    COST_INSTR
#ifdef PRINT_ASM
    fprintf(stdout, "\n%i push %s %s (%i to %i)", c, reg_name [REG_1], reg_name [REG_2], REG_OP_1, REG_OP_2 - 1);
#endif
    break;
   case OP_pop:
    COST(COST_STACK);
    unsigned_instr = REG_OP_2;
    if (unsigned_instr >= bcode_size)
    {
     bcode_error(BERR_ACCESS_OUT_OF_BOUNDS, c, bc, pr, cl);
     return instr_left;
    }
    REG_OP_1 = op [unsigned_instr];
    REG_OP_2 --;
//    fprintf(stdout, "\n%i pop %i %i", c, REG_OP_1, REG_OP_2);
#ifdef PRINT_ASM
    fprintf(stdout, "\n%i pop %s %s (%i from %i)", c, reg_name [REG_1], reg_name [REG_2], REG_OP_1, REG_OP_2 + 1);
#endif
    break;


// to do now: finish these mbank instructions


// Method and method bank instructions
   case OP_putbr:
    COST(COST_MBANK);
    unsigned_instr = instruction_op & MBANK_ADDRESS_MODE_MASK;
    regs->mbank [unsigned_instr] = REG_OP_3;
#ifdef PRINT_ASM
    fprintf(stdout, "\n%i putbr %i %s (%i)", c, unsigned_instr, reg_name [REG_3], REG_OP_3);
#endif
    break;
   case OP_putbn:
    COST(COST_MBANK + COST_READ);
#ifdef PRINT_ASM
    fprintf(stdout, "\n%i ", c);
#endif
    unsigned_instr = instruction_op & MBANK_ADDRESS_MODE_MASK;
    GET_NEXT_INSTR // reads next op into instr (as a signed int)
    regs->mbank [unsigned_instr] = instr;
#ifdef PRINT_ASM
    fprintf(stdout, "putbn %i %i", unsigned_instr, instr);
#endif
    break;
   case OP_putba:
    COST(COST_MBANK + COST_READ + COST_REFER);
#ifdef PRINT_ASM
    fprintf(stdout, "\n%i ", c);
#endif
    unsigned_instr2 = instruction_op & MBANK_ADDRESS_MODE_MASK;
    GET_NEXT_INSTR_UNSIGNED_ADDRESS // reads next op into unsigned_instr and confirms that it points to a valid memory address
    regs->mbank [unsigned_instr2] = op [unsigned_instr];
#ifdef PRINT_ASM
    fprintf(stdout, "putba %i %i (%i)", unsigned_instr2, unsigned_instr, op [unsigned_instr]);
#endif
    break;
   case OP_putrr:
    COST(COST_MBANK);
    unsigned_instr = REG_OP_1;
    if (unsigned_instr >= METHOD_BANK)
    {
     bcode_error(BERR_MBANK_ACCESS_OUT_OF_BOUNDS, c, bc, pr, cl);
     return instr_left;
    }
    regs->mbank [unsigned_instr] = REG_OP_2;
#ifdef PRINT_ASM
    fprintf(stdout, "\n%i putrr %s %s (%i to mbank %i)", c, reg_name [REG_1], reg_name [REG_2], REG_OP_2, unsigned_instr);
#endif
    break;
   case OP_putrn:
    COST(COST_MBANK + COST_READ);
#ifdef PRINT_ASM
    fprintf(stdout, "\n%i ", c);
#endif
    unsigned_instr = REG_OP_1;
    if (unsigned_instr >= METHOD_BANK)
    {
     bcode_error(BERR_MBANK_ACCESS_OUT_OF_BOUNDS, c, bc, pr, cl);
     return instr_left;
    }
    GET_NEXT_INSTR // reads next op into instr (as a signed int)
    regs->mbank [unsigned_instr] = instr;
#ifdef PRINT_ASM
    fprintf(stdout, "putrn %s %i (%i to mbank %i)", reg_name [REG_1], instr, instr, unsigned_instr);
#endif
    break;
   case OP_putra:
    COST(COST_MBANK + COST_READ + COST_REFER);
#ifdef PRINT_ASM
    fprintf(stdout, "\n%i ", c);
#endif
    unsigned_instr2 = REG_OP_1;
    if (unsigned_instr2 >= METHOD_BANK)
    {
     bcode_error(BERR_MBANK_ACCESS_OUT_OF_BOUNDS, c, bc, pr, cl);
     return instr_left;
    }
    GET_NEXT_INSTR_UNSIGNED_ADDRESS // reads next op into unsigned_instr and confirms that it points to a valid memory address
    regs->mbank [unsigned_instr2] = op [unsigned_instr];
#ifdef PRINT_ASM
    fprintf(stdout, "putra %s %i (%i to %i)", reg_name [REG_1], unsigned_instr, op [unsigned_instr], unsigned_instr2);
#endif
    break;
   case OP_getrb: // get contents of method address, put in register r
    COST(COST_MBANK);
    unsigned_instr = (instruction_op >> ADDRESS_MODE_BITFIELD_SIZE) & MBANK_ADDRESS_MODE_MASK;
    REG_OP_1 = regs->mbank [unsigned_instr];
#ifdef PRINT_ASM
    fprintf(stdout, "\n%i getrb %s %i (%i)", c, reg_name [REG_1], unsigned_instr, regs->mbank [unsigned_instr]);
#endif
    break;
   case OP_getrr: // get contents of method address determined by second register r, put in first register r. both registers can be the same.
    COST(COST_MBANK); // should this cost more than getrb?
    unsigned_instr = REG_OP_2;
    if (unsigned_instr >= METHOD_BANK)
    {
     bcode_error(BERR_MBANK_ACCESS_OUT_OF_BOUNDS, c, bc, pr, cl);
     return instr_left;
    }
    REG_OP_1 = regs->mbank [unsigned_instr];
#ifdef PRINT_ASM
    fprintf(stdout, "\n%i getrr %s %s (%i from %i)", c, reg_name [REG_1], reg_name [REG_2], regs->mbank [unsigned_instr], unsigned_instr);
#endif
    break;

   case OP_callmr:
    COST(COST_CALL); // actual method may cost more
    unsigned_instr = instruction_op & METHOD_ADDRESS_MODE_MASK;
    REG_OP_3 = call_method(pr, cl, unsigned_instr, methods, &instr_left);

#ifdef PRINT_ASM
    fprintf(stdout, "\n%i callmr %i %s (%i)", c, unsigned_instr, reg_name [REG_3], REG_OP_3);
#endif
    break;

   case OP_callrr:
    COST(COST_CALL); // actual method may cost more
/*    fprintf(stdout, "\n%i callrr %s %s (%i %i) mb(%i,%i,%i,%i)", c, reg_name [REG_1], reg_name [REG_2], REG_OP_1, REG_OP_2,
            pr->regs.mbank [REG_OP_1],
            pr->regs.mbank [REG_OP_1 + 1],
            pr->regs.mbank [REG_OP_1 + 2],
            pr->regs.mbank [REG_OP_1 + 3]
            );*/
    REG_OP_2 = call_method(pr, cl, REG_OP_1, methods, &instr_left);
#ifdef PRINT_ASM
    fprintf(stdout, "\n%i callrr %s %s (%i %i)", c, reg_name [REG_1], reg_name [REG_2], REG_OP_1, REG_OP_2);
#endif
    break;




/*
Still to do:

OP_callmr, // note - m here is a method (0-15), not a method address (0-63)!
OP_callrr, // anyway, these instructions call whichever method and put its return value into second operand register r.
OP_callm, // these are similar but don't return anything.
OP_callr,
*/


   case OP_prints:
   case OP_printsa:
   case OP_printsr:
    COST(COST_READ); // not sure about this one
    instr = 0;
    char pr_string [90];
    int string_counter = 0;
    int string_c = c;
    if (opcode == OP_printsa)
    {
     GET_NEXT_INSTR_UNSIGNED_ADDRESS // reads next op into unsigned_instr and confirms that it points to a valid memory address
     string_c = unsigned_instr - 1;
    }
     else
     {
      if (opcode == OP_printsr)
      {
       string_c = REG_OP_1 - 1;
       if (string_c < 0
        || string_c >= bcode_size)
       {
        bcode_error(BERR_STRING_OUT_OF_BOUNDS, c, bc, pr, cl);
        return instr_left;
       }
      }
     }
    for (string_counter = 0; string_counter < 90; string_counter ++)
    {
     string_c ++;
     if (string_c >= bcode_size)
     {
      bcode_error(BERR_STRING_OUT_OF_BOUNDS, string_c, bc, pr, cl);
      return instr_left;
     }
     pr_string [string_counter] = op [string_c];
     if (op [string_c] == 0)
      break;
     if (string_counter >= 80)
     {
      bcode_error(BERR_STRING_TOO_LONG, string_c, bc, pr, cl);
      return instr_left;
     }
    }
//    fprintf(stdout, "\n *** %i prints %s", c, pr_string);
    if (cl == NULL)
     program_write_to_console(pr_string, PROGRAM_TYPE_PROCESS, pr->index, pr->player_index);
      else
      {
       switch(cl->bcode.type)
       {
        case PROGRAM_TYPE_DELEGATE:
         program_write_to_console(pr_string, PROGRAM_TYPE_DELEGATE, cl->player, cl->player); break;
        case PROGRAM_TYPE_OPERATOR:
         program_write_to_console(pr_string, PROGRAM_TYPE_OPERATOR, cl->player, cl->player); break;
        case PROGRAM_TYPE_SYSTEM:
         program_write_to_console(pr_string, PROGRAM_TYPE_SYSTEM, 0, -1); break;
        case PROGRAM_TYPE_OBSERVER:
         program_write_to_console(pr_string, PROGRAM_TYPE_OBSERVER, 0, -1); break;
       }
      }
    bc->printing = 1;
// prints moves control to the end of the string afterwards. Other types don't need to do this:
    if (opcode == OP_prints)
    {
     c = string_c;
    }

//    write_to_console(pr_string, pr->index);
//    fprintf(stdout, pr_string);
    break;
   case OP_printr:
    COST(COST_READ); // not sure about this one
    instr = REG_OP_1;
    char num_string [10];
    sprintf(num_string, "%i", REG_OP_1);
    if (cl == NULL)
     program_write_to_console(num_string, PROGRAM_TYPE_PROCESS, pr->index, pr->player_index);
      else
      {
       switch(cl->bcode.type)
       {
        case PROGRAM_TYPE_DELEGATE:
         program_write_to_console(num_string, PROGRAM_TYPE_DELEGATE, cl->player, cl->player); break;
        case PROGRAM_TYPE_OPERATOR:
         program_write_to_console(num_string, PROGRAM_TYPE_OPERATOR, cl->player, cl->player); break;
        case PROGRAM_TYPE_SYSTEM:
         program_write_to_console(num_string, PROGRAM_TYPE_SYSTEM, 0, -1); break;
        case PROGRAM_TYPE_OBSERVER:
         program_write_to_console(num_string, PROGRAM_TYPE_OBSERVER, 0, -1); break;
       }
      }
    bc->printing = 1;
// remember that the source_index parameter of write_to_console should be a valid index for the relevant program type (except PROGRAM_TYPE_SYSTEM or PROGRAM_TYPE_OBSERVER)

//    fprintf(stdout, "%i", REG_OP_1);
  //  fprintf(stdout, "\n *** %i printr %s (%i)", c, reg_name [REG_1], REG_OP_1);
//    fprintf(stdout, "\nPrintr %i", instr);
    break;



   default:
#ifdef PRINT_ASM
    fprintf(stdout, "\ninvalid %u ", opcode);
#endif
    bcode_error(BERR_INVALID_OPCODE, c, bc, pr, cl);
    return instr_left;

  }
// TO DO: consider having out-of-bounds references just wrap around using a BCODE_MAX that's a power of 2 and bitmasking c constantly - this should be much faster

// note that c may have been changed here if the opcode was a jump or an if.
//  *** if the jump is to address zero, c will be -1 at this point. c is not bounds-checked below zero, so it must not be used without incrementing it
//  *** c may also be above BCODE_MAX as the result of an if (will be bounds-checked before being used)

 };


 return instr_left; // should never reach here

}


int bcode_error(int berr_type, int bcode_op, struct bcodestruct* bc, struct procstruct* pr, struct programstruct* cl)
{

 fprintf(stdout, "\nBcode error!");

  if (pr != NULL)
  {
   start_error(PROGRAM_TYPE_PROCESS, pr->index, pr->player_index);
   error_string("\nError in process ");
   error_number(pr->index);
  }
  if (cl != NULL)
  {
   switch(cl->type)
   {
    case PROGRAM_TYPE_DELEGATE:
     start_error(PROGRAM_TYPE_DELEGATE, cl->player, cl->player);
     error_string("\nError in player ");
     error_number(cl->player);
     error_string("'s delegate program");
     break;
    case PROGRAM_TYPE_OPERATOR:
     start_error(PROGRAM_TYPE_OPERATOR, cl->player, cl->player);
     error_string("\nError in player ");
     error_number(cl->player);
     error_string("'s operator program");
     break;
    case PROGRAM_TYPE_OBSERVER:
     start_error(PROGRAM_TYPE_OBSERVER, 0, -1);
     error_string("\nError in observer program");
     break;
    case PROGRAM_TYPE_SYSTEM:
     start_error(PROGRAM_TYPE_SYSTEM, 0, -1);
     error_string("\nError in system program");
     break;
    default: // should never happen
#ifdef SANITY_CHECK
     fprintf(stdout, "\nError: v_interp.c: bcode_error(): error in unknown program??? type %i player %i", cl->type, cl->player);
     error_call();
#endif
     break;
   }
  }

/*  if (bc->note != NULL)
   fprintf(stdout, "\nInterpreter error: %s at %i (source line %i)", berror_name [berr_type], bcode_op, bc->note->src_line [bcode_op]);
    else*/
     error_string(": ");
     error_string(berror_name [berr_type]);
     error_string(" at address ");
     error_number(bcode_op);
     error_string(".");
/*
 if (bcode_op >= 0 && bcode_op < bc->bcode_size)
 {
  fprintf(stdout, "\n address %i instruction %i (opcode %i: %s)", bcode_op, bc->op[bcode_op], abs(bc->op[bcode_op] >> OPCODE_BITSHIFT), opcode_properties[abs(bc->op[bcode_op] >> OPCODE_BITSHIFT)].name);
 }*/
//wait_for_space();
 return -1;

}


