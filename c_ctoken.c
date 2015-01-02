#include <allegro5/allegro.h>
#include "m_config.h"
#include "g_header.h"
#include "c_header.h"
#include "c_comp.h"
#include "c_data.h"

#include "g_misc.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>


enum
{
INITIAL_CTOKEN_TYPE_SINGLE_CHAR,
INITIAL_CTOKEN_TYPE_ONE_OR_TWO_CHAR,
// I don't think there's a need for INITIAL_CTOKEN_TYPE_DOUBLE_CHAR as I don't think there are any operators etc of this type.
INITIAL_CTOKEN_TYPE_NUMBER, // any number other than 0
INITIAL_CTOKEN_TYPE_ZERO, // may be 0 or a hex or binary number
INITIAL_CTOKEN_TYPE_IDENTIFIER,

INITIAL_CTOKEN_TYPE_INVALID
};

extern struct process_defstruct process_def [PROCESS_DEFS];

int get_ctoken_identifier(struct ctokenstruct* ctoken, char read_char);
int read_ctoken_identifier(struct ctokenstruct* ctoken);
int c_get_next_char_from_scode(void);
int determine_initial_ctoken_type(struct ctokenstruct* ctoken, char read_source);
int get_ctoken_one_or_two_char(struct ctokenstruct* ctoken, char read_char);
int parse_ctoken(struct ctokenstruct* ctoken, int parse_type);
int read_number_operator(int value, int* result);
int read_literal_numbers(struct ctokenstruct* ctoken);
int new_c_identifier(struct ctokenstruct* ctoken, int type);
int read_number_value(int* number);
int get_ctoken_number(struct ctokenstruct* ctoken);
int get_ctoken_number_zero(struct ctokenstruct* ctoken);
int skip_spaces(void);
void print_identifier_list(void);


extern struct scodestruct *scode;
extern struct cstatestruct cstate;
extern struct identifierstruct identifier [IDENTIFIER_MAX];
extern struct astatestruct astate;

enum
{
PARSE_TYPE_FULL, // calling parse_ctoken with this means that it will try to resolve expressions involving multiple literal numbers into a single number
PARSE_TYPE_SINGLE_NUMBER, // just means that if it finds a number it won't try to resolve any further.

};

// This function is called by the compiler. It reads the next ctoken from the scode.
// It starts by determining the basic type of the token from the first character.
// Then:
//  - if the first character is an operator or similar that can only be one character long (e.g. a comma), it fills out ctoken and returns.
//  - if the first character indicates that the token could be one or two characters long (e.g. '<', which could be < or <=) it checks the following character, then fills out ctoken and returns.
//  - if the first character is a number, it reads the token in as a number and puts its value in ctoken->value. Returns error if non-number found. Doesn't fill in name.
//  - if the first character is a valid identifier character, it reads the token in as an identifier and tries to find a matching identifier within current scope, or creates a new one).
//  - otherwise - it returns an error
// returns 1 if it finishes reading the ctoken without finding an error.
// returns 0 on error (including if it finds the end of scode - this shouldn't happen while reading a token).
// Don't call this to read the contents of a string
int read_next(struct ctokenstruct* ctoken)
{

 return parse_ctoken(ctoken, PARSE_TYPE_FULL);


}



// This version of read_next assumes that we're in an expression, so it looks ahead to try to resolve constants if possible.
// since it calls read_literal_numbers, which calls read_number_value, it deals with negative numbers properly
// returns 0 on error, 1 on success
int read_next_expression_value(struct ctokenstruct* ctoken)
{

 strcpy(ctoken->name, "(empty)");

 if (cstate.error != CERR_NONE)
  return 0;

 skip_spaces(); // skips to start of next ctoken, so we can work out whether anything was done by read_literal_numbers by checking scode_pos

 int save_scode_pos = cstate.scode_pos;

 int retval = read_literal_numbers(ctoken);

 if (retval == 0)
  return 0;

 if (cstate.scode_pos != save_scode_pos)
  return 1; // the call to read_literal_numbers must have done something, then. Assume that ctoken is the number read in (or resolved) by read_literal_numbers

// call to read_literal_numbers did nothing, so we call parse_ctoken instead
 return parse_ctoken(ctoken, PARSE_TYPE_FULL);


}

// Checks the next ctoken (starting at cstate.scode_pos)
// parse_type should be a PARSE_TYPE_x enum. If the parser finds a number, this value determines whether the parser will return immediately or try to resolve a constant expression.
int parse_ctoken(struct ctokenstruct* ctoken, int parse_type)
{

 cstate.recursion_level ++;

 if (cstate.recursion_level > RECURSION_LIMIT)
  return comp_error(CERR_RECURSION_LIMIT_REACHED_BY_PARSER, NULL);

 int return_value = 1;

 strcpy(ctoken->name, "(empty)");

 if (cstate.error != CERR_NONE)
  return 0;

 if (cstate.scode_pos < -1 || cstate.scode_pos >= SCODE_LENGTH)
  return comp_error(CERR_PARSER_SCODE_BOUNDS, ctoken);

// int tp;
 int read_source;

// first we skip any spaces:
 int skipped = skip_spaces();

 if (skipped == 0)
  return 0; // reached end of scode

// tp = 0;
 cstate.src_line = scode->src_line [cstate.scode_pos];
 cstate.src_line_pos = scode->src_line_pos [cstate.scode_pos];
 cstate.src_file = scode->src_file [cstate.scode_pos];

// now we read the first character to work out what kind of ctoken we have here:

 read_source = c_get_next_char_from_scode();
 if (read_source == REACHED_END_OF_SCODE)
  return 0; // reached end of source

 char read_char = (char) read_source;

 int initial_ctoken_type = determine_initial_ctoken_type(ctoken, read_char); // passes ctoken because if it's a single-character token the structure is filled in

 switch(initial_ctoken_type)
 {
  case INITIAL_CTOKEN_TYPE_INVALID:
//   fprintf(stdout, "\nCtoken: invalid");
   ctoken->type = CTOKEN_TYPE_INVALID;
   return 0;
  case INITIAL_CTOKEN_TYPE_SINGLE_CHAR: // In this case, determine_initial_ctoken_type() has already filled in the ctokenstruct, so we can just return.
   ctoken->name [0] = read_char;
   ctoken->name [1] = 0;
//   fprintf(stdout, "\nCtoken: %s (single character)", ctoken->name);
   return_value = 1;
   goto parse_ctoken_success;
  case INITIAL_CTOKEN_TYPE_ONE_OR_TWO_CHAR:
   if (!get_ctoken_one_or_two_char(ctoken, read_char)) // This function can fail (as reading the second character could reach the end of scode, which will be an error)
    return comp_error(CERR_PARSER_SCODE_BOUNDS, ctoken);
//   fprintf(stdout, "\nCtoken: single or double character");
   // if it doesn't fail, the ctokenstruct will have been filled in.
   return_value = 1;
   goto parse_ctoken_success;
  case INITIAL_CTOKEN_TYPE_NUMBER:
   cstate.scode_pos --;
// any changes here may need to be reflected in the enum-reading code below in case INITIAL_CTOKEN_TYPE_IDENTIFIER
   if (parse_type == PARSE_TYPE_SINGLE_NUMBER)
   {
    if (!get_ctoken_number(ctoken))
     return 0;
//    fprintf(stdout, "\nCtoken: %i (single number)", ctoken->number_value);
    return_value = 1;
    goto parse_ctoken_success;
   }
   if (!read_literal_numbers(ctoken)) // This function can fail (as reading further characters could reach the end of scode, which will be an error)
    return comp_error(CERR_PARSER_REACHED_END_WHILE_READING, ctoken);
//   fprintf(stdout, "\nCtoken: %i (multi number)", ctoken->number_value);
   return_value = 1;
   goto parse_ctoken_success;
// if it doesn't fail, the ctokenstruct will have been filled in.
  case INITIAL_CTOKEN_TYPE_ZERO:
   cstate.scode_pos --;
// any changes here may need to be reflected in the enum-reading code below in case INITIAL_CTOKEN_TYPE_IDENTIFIER
   if (parse_type == PARSE_TYPE_SINGLE_NUMBER)
   {
    if (!get_ctoken_number_zero(ctoken))
     return 0;
//    fprintf(stdout, "\nCtoken: %i (single number)", ctoken->number_value);
    return_value = 1;
    goto parse_ctoken_success;
   }
   if (!read_literal_numbers(ctoken)) // This function can fail (as reading further characters could reach the end of scode, which will be an error)
    return comp_error(CERR_PARSER_REACHED_END_WHILE_READING, ctoken);
//   fprintf(stdout, "\nCtoken: %i (multi number)", ctoken->number_value);
   return_value = 1;
   goto parse_ctoken_success;
// if it doesn't fail, the ctokenstruct will have been filled in.
  case INITIAL_CTOKEN_TYPE_IDENTIFIER:
   ctoken->identifier_index = get_ctoken_identifier(ctoken, read_char);
   if (ctoken->identifier_index == -1) // This function can fail (as reading further characters could reach the end of scode, which will be an error)
    return comp_error(CERR_PARSER_REACHED_END_WHILE_READING, ctoken);
//   fprintf(stdout, "\nCtoken: %s (identifier)", ctoken->name);
//   fprintf(stdout, " (type %i)", ctoken->type);

// if it's an enum, it's converted into a number. The code here should correspond to the code for number ctokens above
   if (ctoken->type == CTOKEN_TYPE_ENUM)
   {
    ctoken->number_value = identifier[ctoken->identifier_index].enum_value;
    ctoken->type = CTOKEN_TYPE_NUMBER;
// If we were just looking for a single number, return successfully:
    if (parse_type == PARSE_TYPE_SINGLE_NUMBER)
    {
     return_value = 1;
     goto parse_ctoken_success;
    }
// If we were looking for an expression, continue reading to see if we can resolve to a constant:
    if (!read_literal_numbers(ctoken)) // This function can fail (as reading further characters could reach the end of scode, which will be an error)
     return comp_error(CERR_PARSER_REACHED_END_WHILE_READING, ctoken);
//    fprintf(stdout, "\nCtoken: %i (multi number)", ctoken->number_value);
    return_value = 1;
    goto parse_ctoken_success;
   }
// "data()" calls are turned into numbers
   if (ctoken->identifier_index == KEYWORD_DATA
    || ctoken->identifier_index == KEYWORD_ASM_DATA)
   {
//    fprintf(stdout, "\nFound data(): line %i pos %i", cstate.src_line, cstate.src_line_pos);
// parse_data will attempt to parse the data call and resolve it to a single number. If successful, ctoken's type and number value will have been set.
    if (!parse_data(ctoken))
     return 0;
    if (parse_type == PARSE_TYPE_SINGLE_NUMBER)
    {
     return_value = 1;
     goto parse_ctoken_success;
    }
// If we were looking for an expression, continue reading to see if we can resolve to a constant:
    if (!read_literal_numbers(ctoken)) // This function can fail (as reading further characters could reach the end of scode, which will be an error)
     return comp_error(CERR_PARSER_REACHED_END_WHILE_READING, ctoken);
    return_value = 1;
    goto parse_ctoken_success;
   }
   return_value = 1;
   goto parse_ctoken_success;
// if it doesn't fail, the ctokenstruct will have been filled in.

 }


 return comp_error(CERR_PARSER_UNKNOWN_TOKEN_TYPE, ctoken);

parse_ctoken_success:

 cstate.recursion_level --;
 return return_value;



}



// returns a value from 0 to 255 on success (I think this may not necessarily work on systems that implement char in a different way. Not sure about this)
// returns REACHED_END_OF_SCODE if end of source reached
// leaves src_pos with a new value (which may be at end of the scode struct)
// assumes scode text array is null-terminated.
int c_get_next_char_from_scode(void)
{

 cstate.scode_pos ++;

 if (scode->text [cstate.scode_pos] == '\0') // reached end
  return REACHED_END_OF_SCODE;
/*
 char tstr[2];
 tstr[0] = scode->text [cstate.scode_pos];
 tstr[1] = 0;
 fprintf(stdout, "%s", tstr);*/

 return scode->text [cstate.scode_pos];

}

/*
This function works out what is indicated about a ctoken by its first character.
If the first character is a character that is always an entire token by itself, the function also fills in the ctoken structure.
Otherwise, it does not.
Returns the type of ctoken it is (returns INITIAL_CTOKEN_TYPE_INVALID on invalid token)
*/
int determine_initial_ctoken_type(struct ctokenstruct* ctoken, char read_source)
{

 if ((read_source >= 'a' && read_source <= 'z')
  || (read_source >= 'A' && read_source <= 'Z')
  || read_source == '_')
   return INITIAL_CTOKEN_TYPE_IDENTIFIER;

 if (read_source == '0')
  return INITIAL_CTOKEN_TYPE_ZERO; // may be 0 or a hex/bin number

 if (read_source >= '1' && read_source <= '9')
   return INITIAL_CTOKEN_TYPE_NUMBER;

 switch(read_source)
 {
  case '(': ctoken->type = CTOKEN_TYPE_PUNCTUATION;
            ctoken->subtype = CTOKEN_SUBTYPE_BRACKET_OPEN; return INITIAL_CTOKEN_TYPE_SINGLE_CHAR;
  case ')': ctoken->type = CTOKEN_TYPE_PUNCTUATION;
            ctoken->subtype = CTOKEN_SUBTYPE_BRACKET_CLOSE; return INITIAL_CTOKEN_TYPE_SINGLE_CHAR;
  case '{': ctoken->type = CTOKEN_TYPE_PUNCTUATION;
            ctoken->subtype = CTOKEN_SUBTYPE_BRACE_OPEN; return INITIAL_CTOKEN_TYPE_SINGLE_CHAR;
  case '}': ctoken->type = CTOKEN_TYPE_PUNCTUATION;
            ctoken->subtype = CTOKEN_SUBTYPE_BRACE_CLOSE; return INITIAL_CTOKEN_TYPE_SINGLE_CHAR;
  case '[': ctoken->type = CTOKEN_TYPE_PUNCTUATION;
            ctoken->subtype = CTOKEN_SUBTYPE_SQUARE_OPEN; return INITIAL_CTOKEN_TYPE_SINGLE_CHAR;
  case ']': ctoken->type = CTOKEN_TYPE_PUNCTUATION;
            ctoken->subtype = CTOKEN_SUBTYPE_SQUARE_CLOSE; return INITIAL_CTOKEN_TYPE_SINGLE_CHAR;
//  case '\'': ctoken.type = CTOKEN_TYPE_PUNCTUATION;
//            ctoken.subtype = CTOKEN_SUBTYPE_DIV; return INITIAL_CTOKEN_TYPE_SINGLE_CHAR;
  case '\'': ctoken->type = CTOKEN_TYPE_PUNCTUATION;
            ctoken->subtype = CTOKEN_SUBTYPE_QUOTE; return INITIAL_CTOKEN_TYPE_SINGLE_CHAR;
  case '"': ctoken->type = CTOKEN_TYPE_PUNCTUATION;
            ctoken->subtype = CTOKEN_SUBTYPE_QUOTES; return INITIAL_CTOKEN_TYPE_SINGLE_CHAR;
  case ';': ctoken->type = CTOKEN_TYPE_PUNCTUATION;
            ctoken->subtype = CTOKEN_SUBTYPE_SEMICOLON; return INITIAL_CTOKEN_TYPE_SINGLE_CHAR;
  case ':': ctoken->type = CTOKEN_TYPE_PUNCTUATION;
            ctoken->subtype = CTOKEN_SUBTYPE_COLON; return INITIAL_CTOKEN_TYPE_SINGLE_CHAR;
  case ',': ctoken->type = CTOKEN_TYPE_PUNCTUATION;
            ctoken->subtype = CTOKEN_SUBTYPE_COMMA; return INITIAL_CTOKEN_TYPE_SINGLE_CHAR;
  case '.': ctoken->type = CTOKEN_TYPE_PUNCTUATION;
            ctoken->subtype = CTOKEN_SUBTYPE_FULL_STOP; return INITIAL_CTOKEN_TYPE_SINGLE_CHAR;
  case '$': ctoken->type = CTOKEN_TYPE_PUNCTUATION;
            ctoken->subtype = CTOKEN_SUBTYPE_DOLLAR; return INITIAL_CTOKEN_TYPE_SINGLE_CHAR;
   // these characters are only ever parsed individually, so we can actually fill out ctokenstruct at this point
  case '~': ctoken->type = CTOKEN_TYPE_OPERATOR_ARITHMETIC;
            ctoken->subtype = CTOKEN_SUBTYPE_BITWISE_NOT; return INITIAL_CTOKEN_TYPE_SINGLE_CHAR;

  case '+': // could be + +=
  case '-': // - -= ->
  case '*': // * *=
  case '/': // / /=
  case '=': // = ==
  case '<': // < <= <<
  case '>': // > >= >>
  case '&': // & &= &&
  case '|': // | |= ||
  case '^': // ^ ^=
  case '%': // % %=
  case '!': // ! !=
   return INITIAL_CTOKEN_TYPE_ONE_OR_TWO_CHAR; // these characters might be individual, but might also be part of a 2-char operator or similar (e.g. +=)
// details of these are filled out in get_ctoken_one_or_two_char

  default: return INITIAL_CTOKEN_TYPE_INVALID; // invalid char

 }

 return INITIAL_CTOKEN_TYPE_INVALID; // invalid char

}


/*
This function is called if a character that could be a one- or two-character token is found.
Returns 1 on success (read a token) or 0 on error (which I guess can only happen if the end of scode is reached)
*/
int get_ctoken_one_or_two_char(struct ctokenstruct* ctoken, char read_char)
{

 int read_source2 = c_get_next_char_from_scode();
 if (read_source2 == REACHED_END_OF_SCODE)
  return 0; // reached end of scode (error)

 char read_char2 = (char) read_source2;

 ctoken->type = CTOKEN_TYPE_OPERATOR_ARITHMETIC; // default

 switch(read_char)
 {
  case '+': // could be + ++ +=
   switch(read_char2)
   {
    case '=': ctoken->type = CTOKEN_TYPE_OPERATOR_ASSIGN;
     ctoken->subtype = CTOKEN_SUBTYPE_PLUSEQ; return 1;
    case '+': ctoken->type = CTOKEN_TYPE_OPERATOR_ASSIGN;
     ctoken->subtype = CTOKEN_SUBTYPE_INCREMENT; return 1;
   }
   ctoken->subtype = CTOKEN_SUBTYPE_PLUS;
   cstate.scode_pos--;
   return 1;
  case '-': // - -= ->
   switch(read_char2)
   {
    case '=': ctoken->type = CTOKEN_TYPE_OPERATOR_ASSIGN;
     ctoken->subtype = CTOKEN_SUBTYPE_MINUSEQ; return 1;
    case '-': ctoken->type = CTOKEN_TYPE_OPERATOR_ASSIGN;
     ctoken->subtype = CTOKEN_SUBTYPE_DECREMENT; return 1;
   }
// need to deal with the possibility that this is a negative number!
   ctoken->subtype = CTOKEN_SUBTYPE_MINUS;
   cstate.scode_pos--;
   return 1;
  case '*': // could be * *= *(pointer)
   switch(read_char2)
   {
    case '=': ctoken->type = CTOKEN_TYPE_OPERATOR_ASSIGN;
     ctoken->subtype = CTOKEN_SUBTYPE_MULEQ; return 1;
   }
   ctoken->subtype = CTOKEN_SUBTYPE_MUL;
   cstate.scode_pos--;
   return 1;
  case '/': // / /=
   switch(read_char2)
   {
    case '=': ctoken->type = CTOKEN_TYPE_OPERATOR_ASSIGN;
     ctoken->subtype = CTOKEN_SUBTYPE_DIVEQ; return 1;
   }
   ctoken->subtype = CTOKEN_SUBTYPE_DIV;
   cstate.scode_pos--;
   return 1;
  case '=': // = ==
   switch(read_char2)
   {
    case '=': ctoken->type = CTOKEN_TYPE_OPERATOR_COMPARISON;
     ctoken->subtype = CTOKEN_SUBTYPE_EQ_EQ; return 1;
   }
   ctoken->type = CTOKEN_TYPE_OPERATOR_ASSIGN;
   ctoken->subtype = CTOKEN_SUBTYPE_EQ;
   cstate.scode_pos--;
   return 1;
  case '<': // < <= <<
   switch(read_char2)
   {
    case '=': ctoken->type = CTOKEN_TYPE_OPERATOR_COMPARISON;
     ctoken->subtype = CTOKEN_SUBTYPE_LESEQ; return 1;
    case '<': ctoken->subtype = CTOKEN_SUBTYPE_BITSHIFT_L; return 1;
   }
   ctoken->type = CTOKEN_TYPE_OPERATOR_COMPARISON;
   ctoken->subtype = CTOKEN_SUBTYPE_LESS;
   cstate.scode_pos--;
   return 1;
  case '>': // > >= >>
   switch(read_char2)
   {
    case '=': ctoken->type = CTOKEN_TYPE_OPERATOR_COMPARISON;
     ctoken->subtype = CTOKEN_SUBTYPE_GREQ; return 1;
    case '<': ctoken->subtype = CTOKEN_SUBTYPE_BITSHIFT_R; return 1;
   }
   ctoken->type = CTOKEN_TYPE_OPERATOR_COMPARISON;
   ctoken->subtype = CTOKEN_SUBTYPE_GR;
   cstate.scode_pos--;
   return 1;
  case '&': // & &= &&
   switch(read_char2)
   {
    case '=': ctoken->type = CTOKEN_TYPE_OPERATOR_ASSIGN;
     ctoken->subtype = CTOKEN_SUBTYPE_BITWISE_AND_EQ; return 1;
    case '&': ctoken->type = CTOKEN_TYPE_OPERATOR_LOGICAL;
     ctoken->subtype = CTOKEN_SUBTYPE_LOGICAL_AND; return 1;
   }
   ctoken->subtype = CTOKEN_SUBTYPE_BITWISE_AND;
   cstate.scode_pos--;
   return 1;
  case '|': // | |= ||
   switch(read_char2)
   {
    case '=': ctoken->type = CTOKEN_TYPE_OPERATOR_ASSIGN;
     ctoken->subtype = CTOKEN_SUBTYPE_BITWISE_OR_EQ; return 1;
    case '|': ctoken->type = CTOKEN_TYPE_OPERATOR_LOGICAL;
     ctoken->subtype = CTOKEN_SUBTYPE_LOGICAL_OR; return 1;
   }
   ctoken->subtype = CTOKEN_SUBTYPE_BITWISE_OR;
   cstate.scode_pos--;
   return 1;
  case '^': // ^ ^=
   switch(read_char2)
   {
    case '=': ctoken->type = CTOKEN_TYPE_OPERATOR_ASSIGN;
     ctoken->subtype = CTOKEN_SUBTYPE_BITWISE_XOR_EQ; return 1;
   }
   ctoken->subtype = CTOKEN_SUBTYPE_BITWISE_XOR;
   cstate.scode_pos--;
   return 1;
  case '~': // ~ ~=
   switch(read_char2)
   {
    case '=': ctoken->type = CTOKEN_TYPE_OPERATOR_ASSIGN;
     ctoken->subtype = CTOKEN_SUBTYPE_BITWISE_NOT_EQ; return 1;
   }
   ctoken->subtype = CTOKEN_SUBTYPE_BITWISE_NOT;
   cstate.scode_pos--;
   return 1;
  case '%': // % %=
   switch(read_char2)
   {
    case '=': ctoken->type = CTOKEN_TYPE_OPERATOR_ASSIGN;
     ctoken->subtype = CTOKEN_SUBTYPE_MODEQ; return 1;
   }
   ctoken->subtype = CTOKEN_SUBTYPE_MOD;
   cstate.scode_pos--;
   return 1;
  case '!': // ! !=
   switch(read_char2)
   {
    case '=': ctoken->type = CTOKEN_TYPE_OPERATOR_COMPARISON;
     ctoken->subtype = CTOKEN_SUBTYPE_COMPARE_NOT; return 1;
   }
   ctoken->subtype = CTOKEN_SUBTYPE_NOT;
   cstate.scode_pos--;
   return 1;


 }

 return 0;

}

// Assumes that scode_pos is set so that the next char read will be the first char of a number (could be the only char of the number).
// assumes that if the number is negative and starts with -, the - has already been read (see read_number_value())
int get_ctoken_number(struct ctokenstruct* ctoken)
{

 int number [10] = {0,0,0,0,0,0,0,0,0,0};
 int i;
 int read_source2;
 char read_char;

 i = 0;

 do
 {
  read_source2 = c_get_next_char_from_scode();
  if (read_source2 == REACHED_END_OF_SCODE)
   return comp_error(CERR_PARSER_REACHED_END_WHILE_READING, ctoken);

  read_char = (char) read_source2;

  if (read_char >= '0' && read_char <= '9')
  {
   if (i > 5) // maximum length of 16-bit int as a decimal string
    return comp_error(CERR_PARSER_NUMBER_TOO_LARGE, ctoken);
   number [i] = read_char - '0';
   i ++;
   continue;
  }

  if ((read_char >= 'a' && read_char <= 'z')
   || (read_char >= 'A' && read_char <= 'Z')
   || read_char == '_')
    return comp_error(CERR_PARSER_LETTER_IN_NUMBER, ctoken);

// if it's not a number or a letter, it's probably a space or an operator. So we stop reading the number, decrement *scode_pos and let whatever's been found be dealt with as the next ctoken:
  cstate.scode_pos --;
  break;

 } while (TRUE);

 int ctoken_value = 0;
// at this point we assume that the number of digits in the number is (i + 1):
 int j = 0;
 int multiplier = 1;

 for (j = i - 1; j >= 0; j --)
 {
  ctoken_value += number [j] * multiplier;
  multiplier *= 10;
 }

 if (ctoken_value > BCODE_VALUE_MAXIMUM
  || ctoken_value < BCODE_VALUE_MINIMUM)
   return comp_error(CERR_PARSER_NUMBER_TOO_LARGE, ctoken);

 ctoken->type = CTOKEN_TYPE_NUMBER;
 ctoken->number_value = ctoken_value;

// fprintf(stdout, "\nread number: %i", ctoken_value);

 return 1;


}

#define NUMBER_STRING_SIZE 20
// NUMBER_STRING_SIZE must be large enough to store a 16-digit binary number

// Like get_ctoken_number, but is called when first digit of a number is 0.
// This can mean it's just a zero, or that it's a hex number in 0x form.
int get_ctoken_number_zero(struct ctokenstruct* ctoken)
{

 int number [NUMBER_STRING_SIZE];
 int i;
 int read_source2;
 char read_char;
 int ctoken_value;
 int j;
 int multiplier;

 for (i = 0; i < NUMBER_STRING_SIZE; i ++)
 {
  number [i] = 0;
 }

 i = 0;

// read the zero again:
 read_source2 = c_get_next_char_from_scode();
 if (read_source2 == REACHED_END_OF_SCODE)
  return comp_error(CERR_PARSER_REACHED_END_WHILE_READING, ctoken);

// fprintf(stdout, "\nget_ctoken_number_zero");

// the zero has been read in already, so there are various possibilites as to what comes next:
//  - the next character is x, which means this is a hex number
//  - the next character is invalid (e.g. out of bounds) so this is an error
//  - the next character is also a digit, so this is an error (decimal numbers shouldn't start with 0)
//  - any other case - the number is just zero.

 read_source2 = c_get_next_char_from_scode();
 if (read_source2 == REACHED_END_OF_SCODE)
  return comp_error(CERR_PARSER_REACHED_END_WHILE_READING, ctoken);

 read_char = (char) read_source2;

 if (read_char >= '0' && read_char <= '9')
  return comp_error(CERR_PARSER_NUMBER_STARTS_WITH_ZERO, ctoken);


// May be a number in 0x hex form:
 if (read_char == 'x'
  || read_char == 'X')
 {
  do
  {
   read_source2 = c_get_next_char_from_scode();
   if (read_source2 == REACHED_END_OF_SCODE)
    return comp_error(CERR_PARSER_REACHED_END_WHILE_READING, ctoken);

   read_char = (char) read_source2;

   if (read_char >= '0' && read_char <= '9')
   {
    if (i > 4) // maximum length of 16-bit int as a hex string
     return comp_error(CERR_PARSER_NUMBER_TOO_LARGE, ctoken);
    number [i] = read_char - '0';
    i ++;
    continue;
   }

   if (read_char >= 'a' && read_char <= 'f')
   {
    if (i > 4) // maximum length of 16-bit int as a hex string
     return comp_error(CERR_PARSER_NUMBER_TOO_LARGE, ctoken);
    number [i] = read_char - 'a' + 10;
    i ++;
    continue;
   }

   if (read_char >= 'A' && read_char <= 'F')
   {
    if (i > 4) // maximum length of 16-bit int as a hex string
     return comp_error(CERR_PARSER_NUMBER_TOO_LARGE, ctoken);
    number [i] = read_char - 'A' + 10;
    i ++;
    continue;
   }

   if ((read_char > 'f' && read_char <= 'z')
    || (read_char > 'F' && read_char <= 'Z')
    || read_char == '_')
     return comp_error(CERR_PARSER_LETTER_IN_HEX_NUMBER, ctoken);

// if it's not a number or a letter, it's probably a space or an operator. So we stop reading the number, decrement *scode_pos and let whatever's been found be dealt with as the next ctoken:
   cstate.scode_pos --;
   break;

  } while (TRUE);

  ctoken_value = 0;
// at this point we assume that the number of digits in the number is (i + 1):
  j = 0;
  multiplier = 1;

  for (j = i - 1; j >= 0; j --)
  {
   ctoken_value += number [j] * multiplier;
   multiplier *= 16;
  }

  if (ctoken_value > BCODE_VALUE_MAXIMUM
   || ctoken_value < BCODE_VALUE_MINIMUM)
    return comp_error(CERR_PARSER_NUMBER_TOO_LARGE, ctoken);

  ctoken->type = CTOKEN_TYPE_NUMBER;
  ctoken->number_value = ctoken_value;

  return 1;
 } // end hex




// may be a binary number (0b prefix)
 if (read_char == 'b'
  || read_char == 'B')
 {

  do
  {
   read_source2 = c_get_next_char_from_scode();
   if (read_source2 == REACHED_END_OF_SCODE)
    return comp_error(CERR_PARSER_REACHED_END_WHILE_READING, ctoken);

   read_char = (char) read_source2;

   if (read_char == '0' || read_char == '1')
   {
    if (i > 15) // maximum length of 16-bit int
     return comp_error(CERR_PARSER_NUMBER_TOO_LARGE, ctoken);
    number [i] = read_char - '0';
    i ++;
    continue;
   }

   if (read_char >= '2' && read_char <= '9')
     return comp_error(CERR_INVALID_NUMBER_IN_BINARY_NUMBER, ctoken);

   if ((read_char >= 'a' && read_char <= 'z')
    || (read_char >= 'A' && read_char <= 'Z')
    || read_char == '_')
     return comp_error(CERR_PARSER_LETTER_IN_BINARY_NUMBER, ctoken);

// if it's not a number or a letter, it's probably a space or an operator. So we stop reading the number, decrement *scode_pos and let whatever's been found be dealt with as the next ctoken:
   cstate.scode_pos --;
   break;

  } while (TRUE);

  ctoken_value = 0;
// at this point we assume that the number of digits in the number is (i + 1):
  j = 0;
  multiplier = 0;

  for (j = i - 1; j >= 0; j --)
  {
   if (number [j] == 1)
    ctoken_value += 1 << multiplier;
   multiplier ++;
  }

  if (ctoken_value > BCODE_VALUE_MAXIMUM
   || ctoken_value < BCODE_VALUE_MINIMUM)
    return comp_error(CERR_PARSER_NUMBER_TOO_LARGE, ctoken);

  ctoken->type = CTOKEN_TYPE_NUMBER;
  ctoken->number_value = ctoken_value;

  return 1;


 }


// number was just a zero followed by something else, so we set up a zero number ctoken and return.
  ctoken->type = CTOKEN_TYPE_NUMBER;
  ctoken->number_value = 0;
  cstate.scode_pos --;
  return 1;

}


/*
Returns index of identifier on success
read_char is the first character of the token (this function starts from the second)
Returns -1 on failure
*/
int get_ctoken_identifier(struct ctokenstruct* ctoken, char read_char)
{

 int i;

// cstate.scode_pos --; // read back to the character just read in.

// int read_result = read_ctoken_identifier(ctoken);

// if (read_result == 0) // error
//  return -1;

// if (read_result == 2) // out-of-scope identifier
//  return out_of_scope_reference(ctoken);

// if read_result == 1, we've found an ordinary identifier so we continue on to read it.


 int read_source2;
 char read_char2;

 ctoken->name [0] = read_char;
 ctoken->name [1] = '\0';
 ctoken->type = CTOKEN_TYPE_IDENTIFIER_NEW; // this value can be replaced below

 i = 0;

 do
 {
  read_source2 = c_get_next_char_from_scode();
  if (read_source2 == REACHED_END_OF_SCODE)
   return -1; // reached end of scode (error)

  read_char2 = (char) read_source2;

  if ((read_char2 >= 'a' && read_char2 <= 'z')
   || (read_char2 >= 'A' && read_char2 <= 'Z')
   || (read_char2 >= '0' && read_char2 <= '9') // an identifier name can contain numbers; it just can't start with a number
   || read_char2 == '_')
  {
   i ++;
   if (i >= IDENTIFIER_MAX_LENGTH)
    return comp_error_minus1(CERR_PARSER_TOKEN_TOO_LONG, ctoken);
   ctoken->name [i] = read_char2;
   ctoken->name [i + 1] = '\0';
   continue;
  }

//  if (read_char2 == '.') // check for out-of-scope references
//  {
//   return out_of_scope_reference(ctoken);
//  }

// if it's not a number or a letter, it's probably a space or an operator. So we stop reading the number and let whatever's been found be dealt with as the next ctoken:
  cstate.scode_pos --;
  break;

 } while (TRUE);

 for (i = 0; i < IDENTIFIER_MAX; i ++)
 {
  if (identifier[i].type == ID_NONE)
   continue;
// ignore assembler code unless we're in the assembler:
  if (cstate.running_asm == 0)
  {
   if (identifier[i].asm_or_compiler == IDENTIFIER_ASM_ONLY)
    continue;
// ignore variable names that are outside current scope (i.e. local variables when outside relevant cfunction, or any variables when outside relevant process scope)
   if (identifier[i].type == ID_USER_INT
    && ((identifier[i].scope != SCOPE_GLOBAL
     && identifier[i].scope != cstate.scope)
    || identifier[i].process_scope != cstate.process_scope))
     continue;
// ignore cfunction names that are outside current process scope:
   if (identifier[i].type == ID_USER_CFUNCTION
    && identifier[i].process_scope != cstate.process_scope)
     continue;
// ignore label names that are outside current cfunction scope (cannot be used in global scope):
   if (identifier[i].type == ID_USER_LABEL
    || identifier[i].type == ID_USER_LABEL_UNDEFINED)
   {
    if (identifier[i].scope != cstate.scope
    || identifier[i].process_scope != cstate.process_scope)
     continue;
   }
   if (identifier[i].type == ID_PROCESS
    && identifier[i].process_scope != cstate.process_scope)
     continue;
   if (identifier[i].type == ID_ENUM
    && identifier[i].process_scope != cstate.process_scope)
     continue;
 }
  else // we must be in the assembler, so scope works differently:
  {
// ignore compiler-only things:
   if (identifier[i].asm_or_compiler == IDENTIFIER_C_ONLY)
    continue;
// ignore anything outside current aspace
   if (identifier[i].aspace_scope != -1 // don't exclude anything in universal aspace
    && identifier[i].aspace_scope != astate.current_aspace)
     continue;
// ignore anything outside current namespace
   if (identifier[i].nspace_scope != -1 // don't exclude anything in universal nspace
    && identifier[i].nspace_scope != astate.current_nspace)
     continue;
// Note that astate.current_aspace and current_nspace can be manipulated when parsing out-of-scope references
  }


  if (strcmp(ctoken->name, identifier[i].name) == 0) // match found!
  {
  if (i < KEYWORDS_END)
   ctoken->type = CTOKEN_TYPE_IDENTIFIER_KEYWORD; // this is incorrect for asm keywords and asm opcodes, but is fixed below
// some identifier types allow the ctoken type to be derived directly:
  switch(identifier[i].type)
  {
   case ID_USER_CFUNCTION:
    ctoken->type = CTOKEN_TYPE_IDENTIFIER_USER_CFUNCTION; break;
   case ID_USER_INT:
    ctoken->type = CTOKEN_TYPE_IDENTIFIER_USER_VARIABLE; break;
   case ID_BUILTIN_CFUNCTION:
    ctoken->type = CTOKEN_TYPE_IDENTIFIER_BUILTIN_CFUNCTION; break;
   case ID_PROCESS:
    ctoken->type = CTOKEN_TYPE_PROCESS;
//    fprintf(stdout, "\nprocess (type %i name %s)", identifier[i].type, identifier[i].name);
//    print_identifier_list();
    break;
   case ID_ENUM:
    ctoken->type = CTOKEN_TYPE_ENUM;
    //fprintf(stdout, "\nenum");
    break;
   case ID_USER_LABEL:
    ctoken->type = CTOKEN_TYPE_LABEL; break;
   case ID_USER_LABEL_UNDEFINED:
    ctoken->type = CTOKEN_TYPE_LABEL_UNDEFINED; break; // an undefined label can be found if e.g. it's used in multiple gotos before it's defined.
   case ASM_ID_KEYWORD:
    ctoken->type = CTOKEN_TYPE_ASM_KEYWORD; break;
   case ASM_ID_OPCODE:
    ctoken->type = CTOKEN_TYPE_ASM_OPCODE; break;
   case ASM_ID_GENERIC_DEFINED:
    ctoken->type = CTOKEN_TYPE_ASM_GENERIC_DEFINED; break;
   case ASM_ID_GENERIC_UNDEFINED:
    ctoken->type = CTOKEN_TYPE_ASM_GENERIC_UNDEFINED; break;
   case ASM_ID_GENERIC_ALIAS:
    ctoken->type = CTOKEN_TYPE_ASM_GENERIC_ALIAS; break;
   case ASM_ID_ASPACE:
    ctoken->type = CTOKEN_TYPE_ASM_ASPACE; break;
   case ASM_ID_NSPACE:
    ctoken->type = CTOKEN_TYPE_ASM_NSPACE; break;
// otherwise will remain the type set above
  }

   return i;
  }
 }

 int retval = new_c_identifier(ctoken, ID_USER_UNTYPED);

 if (!retval)
  return 0;

 return ctoken->identifier_index;

// return new_c_identifier(ctoken, ID_USER_UNTYPED); // return value is -1 on failure


}




/*
Reads in a ctoken
Assumes that the first character of the ctoken is a valid character for an identifier
 (so is currently only called after this has been confirmed)

Returns 1 on success, 0 on failure
Returns 2 if '.' found (indicating an out-of-scope reference)

*/
int read_ctoken_identifier(struct ctokenstruct* ctoken)
{

 int i;
 int read_source2;
 char read_char2;

// ctoken->name [0] = read_char;
// ctoken->name [1] = '\0';
 ctoken->type = CTOKEN_TYPE_IDENTIFIER_NEW; // this value can be replaced later

 i = 0;

 do
 {
  read_source2 = c_get_next_char_from_scode();
  if (read_source2 == REACHED_END_OF_SCODE)
   return 0; // reached end of scode (error)

  read_char2 = (char) read_source2;

  if ((read_char2 >= 'a' && read_char2 <= 'z')
   || (read_char2 >= 'A' && read_char2 <= 'Z')
   || (read_char2 >= '0' && read_char2 <= '9') // an identifier name can contain numbers; it just can't start with a number
   || read_char2 == '_')
  {
   i ++;
   if (i >= IDENTIFIER_MAX_LENGTH - 1)
    return comp_error(CERR_PARSER_TOKEN_TOO_LONG, ctoken);
   ctoken->name [i] = read_char2;
   ctoken->name [i + 1] = '\0';
   continue;
  }

// if it's not a number or a letter, it's probably a space or an operator. So we stop reading the number and let whatever's been found be dealt with as the next ctoken:
  cstate.scode_pos --;
  break;

 } while (TRUE);

 return 1;

}


int ctoken_compare_text(struct ctokenstruct* ctoken, const char *comp_str)
{

  if (strcmp(ctoken->name, comp_str) == 0) // match found!
   return 1;

  return 0;

}




// finds a new identifier and puts its index in ctoken->identifier_index
// expects ctoken->name to have been filled in
// returns 1/0 on success/failure
int new_c_identifier(struct ctokenstruct* ctoken, int type)
{
  int i;

  for (i = 0; i < IDENTIFIER_MAX; i ++)
  {
   if (identifier[i].type == ID_NONE)
    break;
  }

  if (i == IDENTIFIER_MAX) // no space for a new identifier
   return 0; // error

  strcpy(identifier[i].name, ctoken->name);
  identifier[i].type = type;
 // these addresses may not be valid (they may be -1 at this point if the identifier has been used but not yet defined). They must be used only when it is known that they're safe:
//  if (identifier[i].address_intercode == -1)
//   identifier[i].address_intercode = cstate.icode_pos;

  identifier[i].asm_or_compiler = IDENTIFIER_ASM_OR_C; // change later if needed
  identifier[i].scope = cstate.scope;
  identifier[i].process_scope = cstate.process_scope;
/*  if (cstate.process_scope == -1)
			identifier[i].aspace_scope = -1;
		  else
     identifier[i].aspace_scope = process_def[cstate.process_scope].corresponding_aspace;
  if (cstate.process_scope == -1
			||	cstate.scope == -1)
			identifier[i].nspace_scope = -1;
		  else
     identifier[i].nspace_scope = process_def[cstate.process_scope].cfunction[cstate.scope].corresponding_nspace;*/
  identifier[i].initial_value = 0;

  ctoken->type = CTOKEN_TYPE_IDENTIFIER_NEW;
  ctoken->identifier_index = i;

//  fprintf(stdout, "\nNew identifier %i: %s scope %i", i, identifier[i].name, identifier[i].scope);

  return 1;

}




int check_end_of_scode(void)
{

 if (cstate.scode_pos < -1 || cstate.scode_pos >= SCODE_LENGTH)
  return 1; // found end
//  return comp_error(CERR_PARSER_TOKEN_TOO_LONG, ctoken); - may not be an error

 int read_source;

// we skip any spaces:
 while (TRUE)
 {
  read_source = c_get_next_char_from_scode(); // note scode_pos is incremented by get_next_char_from_scode
  if (read_source == REACHED_END_OF_SCODE)
   return 1; // has reached end (i.e. found a null terminator)
  if (read_source != ' ')
  {
   (cstate.scode_pos) --;
   break;
  }
 };

 return 0; // found something other than a space or null

}


// this function checks whether the next ctoken is of ctoken_type and ctoken_subtype
// returns 1 on success, zero on failure.
// doesn't fill in a ctoken struct for use by calling function, just checks it
// will create a new identifier if needed
// if ctoken_type is CTOKEN_TYPE_IDENTIFIER_KEYWORD, ctoken_subtype is tested against identifier_index
// if ctoken_subtype is -1, only checks type
int check_next(int ctoken_type, int ctoken_subtype)
{

 int save_scode_pos = cstate.scode_pos;
 struct ctokenstruct ctoken;


 int retval = parse_ctoken(&ctoken, PARSE_TYPE_FULL);

// fprintf(stdout, "\ncompare (%i, %i) to (%i, %i)", ctoken_type, ctoken_subtype, ctoken.type, ctoken.subtype);

 if (retval == 0)
 {
  cstate.scode_pos = save_scode_pos;
  return 0;
 }

 if (ctoken.type != ctoken_type)
 {
  cstate.scode_pos = save_scode_pos;
  return 0;
 }

 switch(ctoken_type)
 {
  case CTOKEN_TYPE_OPERATOR_ARITHMETIC:
  case CTOKEN_TYPE_OPERATOR_ASSIGN:
  case CTOKEN_TYPE_OPERATOR_LOGICAL:
  case CTOKEN_TYPE_OPERATOR_COMPARISON:
  case CTOKEN_TYPE_PUNCTUATION:
   if (ctoken.subtype == ctoken_subtype
    || ctoken_subtype == -1)
    return 1;
   break;
  case CTOKEN_TYPE_IDENTIFIER_KEYWORD:
   if (ctoken.identifier_index == ctoken_subtype
    || ctoken_subtype == -1)
    return 1;
   break;
  case CTOKEN_TYPE_IDENTIFIER_NEW:
   break;
  case CTOKEN_TYPE_IDENTIFIER_USER_CFUNCTION:
  case CTOKEN_TYPE_NUMBER:
  case CTOKEN_TYPE_IDENTIFIER_USER_VARIABLE:
   break;

 }

 cstate.scode_pos = save_scode_pos;
 return 0;

}

// peek next looks at the next ctoken and fills in the ctoken struct, but does not advance scode_pos
int peek_next(struct ctokenstruct* ctoken)
{

 int save_scode_pos = cstate.scode_pos;

 int retval = parse_ctoken(ctoken, PARSE_TYPE_SINGLE_NUMBER);

 cstate.scode_pos = save_scode_pos;

 return retval;

}

// like peek_next, but will resolve constants if possible
int peek_next_expression_value(struct ctokenstruct* ctoken)
{

 int save_scode_pos = cstate.scode_pos;

 int retval = read_next_expression_value(ctoken);

 cstate.scode_pos = save_scode_pos;

 return retval;

}


// like check_next, but doesn't advance scode_pos even if finds wanted token
int peek_check_next(int ctoken_type, int ctoken_subtype)
{

 int save_scode_pos = cstate.scode_pos;

 int retval = check_next(ctoken_type, ctoken_subtype);

 cstate.scode_pos = save_scode_pos;

 return retval;


}



// This function reads in literal numbers from an expression
// resolves certain types of arithmetic that can be calculated at compile time
// it can be recursive
/*

Works like this:
If parse_ctoken finds a number, it:
 - decrements scode_pos
 - calls this function with value zero
This function
 - calls read_ctoken_number
  - read_ctoken_number reads the number (returns 0 on failure) and writes value to ctoken->number_value
 - calls read_number_operator, passing value and primary_expression=1
  - read_number_operator contains a while loop which:
   - saves scode_pos
   - peeks at the next ctoken
   - if ctoken is ; or ), returns 1 (finished)
   - if ctoken is not an operator, returns -1 (error)
   - if ctoken is an operator, but not an arithmetical operator, returns 1 (finished)
   - reads the next ctoken
   - if ctoken is (
    - save scode_pos again
    - call read_number_operator but this time primary_expression=0
    - if returns 1, apply the operator to ctoken_number
    - if returns 0, reset scode_pos to recent value and return 1 (has resolved as much as it can)
    - if returns -1 (error), return -1
   - if ctoken is not a literal number:
    - if primary_expression == 1:
     - resets scode_pos to just before operator and returns 1 (has resolved as much as it can)
    - if primary_expression == 0 (i.e. inside brackets)
     - just returns 0; the sub-expression inside the brackets will be processed later, and if it contains constants they will be dealt with again
   - if ctoken is a literal number, apply operator and continue through loop.
When this function passes control back to parse_ctoken, the number_value field of ctoken should be filled in.
If read_next


if recursive_call == 1, this means that we're inside a subexpression (inside brackets). If a non-number value is found, returns 0 as the subexpression cannot be folded into the main expression (the subexpression
 will, however, be parsed in later so parts of it might be resolved then)
if recursive_call == 0, we're not inside a subexpression of the expression that read_literal_numbers was called on (may still be inside a subexpression though). Returns 1 on finding a non-number.


This function starts the process of reading literal numbers in an expression, or recursively in a subexpression.
Does not assume that the next ctoken is a number, though, so it can be used anytime a number is expected or may occur.
Returns:
0 on error.
1 if the expression could not be totally resolved. Will leave the calculation so far in ctoken->number_value. cstate.scode_pos will be just before the operator followed by a non-resolvable token.
2 if the expression was totally resolved. Will leave the result in ctoken->number_value. cstate.scode_pos will be at the end (just before any closing punctuation).

*/
int read_literal_numbers(struct ctokenstruct* ctoken)
{

 int retval, save_scode_pos;
 s16b save_number_value;
 if (ctoken->type != CTOKEN_TYPE_ENUM)
  strcpy(ctoken->name, "(number)");

 save_scode_pos = cstate.scode_pos;
 save_number_value = ctoken->number_value;
// first, read in the number that was found:
 retval = read_number_value(&(ctoken->number_value));

// fprintf(stdout, "\nread_literal_numbers: %i", ctoken->number_value);

 switch(retval)
 {
  case -1:
   return 0; // error
  case 0: // expression was partly resolved
   cstate.scode_pos = save_scode_pos; // leaves cstate.scode_pos before the ctoken or subexpression that couldn't be resolved
   ctoken->type = CTOKEN_TYPE_NUMBER;
   ctoken->number_value = save_number_value;
   return 1; // could not resolve
  case 1: // success - now continue calculating
   break;
 }


 while(TRUE)
 {
  save_scode_pos = cstate.scode_pos;
  save_number_value = ctoken->number_value;

// now start reading in operators:
  retval = read_number_operator(ctoken->number_value, &(ctoken->number_value));
// read_number_operator reads the next operator and then performs it on the value after that, if it's a number.
// leaves the result in ctoken->number_value
// calls this function (i.e. read_literal_numbers()) recursively if it finds an open bracket.

  switch(retval)
  {
   case -1: // error
    return 0;
   case 0: // found non-number that prevents resolution
    cstate.scode_pos = save_scode_pos; // leaves cstate.scode_pos before the next operator
    ctoken->type = CTOKEN_TYPE_NUMBER;
    ctoken->number_value = save_number_value;
    return 1;
   case 1: // carry on
    break;
   case 2: // found apparently terminating punctuation
    ctoken->type = CTOKEN_TYPE_NUMBER;
    ctoken->number_value = save_number_value;
    return 2;
  }

// retval is 1 and we can continue resolving the numbers, so continue through the loop
 };

 return 0; // should not reach this point.

}




// TO DO: I think this might not work if it finds (). Should probably check for this.
// value is the number that the expression is up to so far.
// result is a pointer to the variable that is to hold the result.
// returns:
// -1 on error
// 0 if it finds a non-number that prevents it from resolving further
// 1 if it completed successfully and can continue resolving further
// 2 if it found punctuation that appears to terminate the expression (does not verify the punctuation; assumes the main parser will do that)
int read_number_operator(int value, int* result)
{

 int scode_save;
 struct ctokenstruct ctoken;
 int read_number = 0;
 int op_type;

 scode_save = cstate.scode_pos;

// fprintf(stdout, "\nread_number_operator(%i, %i)", value, *result);

// now we check for the operator, without advancing scode_pos:
  if (!peek_next(&ctoken))
   return -1;
// what kind of operator is it?
  switch(ctoken.type)
  {
   case CTOKEN_TYPE_PUNCTUATION:
    if (ctoken.subtype == CTOKEN_SUBTYPE_BRACKET_CLOSE
     || ctoken.subtype == CTOKEN_SUBTYPE_SEMICOLON
     || ctoken.subtype == CTOKEN_SUBTYPE_COLON
     || ctoken.subtype == CTOKEN_SUBTYPE_COMMA
     || ctoken.subtype == CTOKEN_SUBTYPE_SQUARE_CLOSE
     || ctoken.subtype == CTOKEN_SUBTYPE_BRACE_CLOSE)
      return 2; // finished successfully (assuming the closing punctuation is correct; this will be checked elsewhere)
    if (cstate.running_asm)
     return 2; // anything is accepted as finished an asm expression
    return -1; // error
   case CTOKEN_TYPE_OPERATOR_ASSIGN:
   case CTOKEN_TYPE_OPERATOR_LOGICAL:
   case CTOKEN_TYPE_OPERATOR_COMPARISON:
    return 0; // finished, but could not resolve (does not deal with possibility of calculating a value for a logical or comparison operator - should probably implement this!)
   case CTOKEN_TYPE_OPERATOR_ARITHMETIC:
    break; // go ahead
   default:
    if (cstate.running_asm)
     return 2; // anything is accepted as finished an asm expression
    return -1; // error
  }

// at this point we know the operator is an arithmetic operator, so we can read it in:
  if (!read_next(&ctoken))
   return -1; // this just reads in the arithmetic operator (because the previous read was a peek which didn't advance scode_pos)

  op_type = ctoken.subtype;

// now we read the next ctoken. read_number_value checks for minus prefixes and makes the number negative if it finds one.
  int retval = read_number_value(&read_number);

  switch(retval)
  {
   case -1: return -1; // error
   case 0: // found something it couldn't resolve
    cstate.scode_pos = scode_save;
    return 0;
   case 1: // resolved successfully (including any subexpressions)
    break;
  }

// read_number will contain the number read in by read_number_value.
  switch(op_type)
  {
   case CTOKEN_SUBTYPE_PLUS:
    *result = value + read_number;
    break;
   case CTOKEN_SUBTYPE_MINUS:
    *result = value - read_number;
    break;
   case CTOKEN_SUBTYPE_MUL:
    *result = value * read_number;
    break;
   case CTOKEN_SUBTYPE_DIV:
    if (read_number == 0)
     *result = 0;
      else
       *result = value / read_number;
    break;
   case CTOKEN_SUBTYPE_MOD:
    if (read_number == 0)
     *result = 0;
      else
       *result = value % read_number;
    break;
   case CTOKEN_SUBTYPE_BITSHIFT_L:
    *result = value << read_number;
    break;
   case CTOKEN_SUBTYPE_BITSHIFT_R:
    *result = value >> read_number;
    break;
   case CTOKEN_SUBTYPE_BITWISE_AND:
    *result = value & read_number;
    break;
   case CTOKEN_SUBTYPE_BITWISE_OR:
    *result = value | read_number;
    break;
   case CTOKEN_SUBTYPE_BITWISE_XOR:
    *result = value ^ read_number;
    break;
/*   case CTOKEN_SUBTYPE_BITWISE_NOT:
    *result = value ~ read_number;
    break;*/

  }

 return 1; // success; continue trying to read numbers.

}


// returns:
// -1 on error
// 0 on finding something that can't be resolved
// 1 on success (leaves result so far in *number)
// expects the calling function will reset scode_pos if this function returns 0
// can call itself if it finds an open bracket
int read_number_value(int* number)
{

  struct ctokenstruct ctoken;
  int retval;

//  fprintf(stdout, "\nread_number_value");

  if (!parse_ctoken(&ctoken, PARSE_TYPE_SINGLE_NUMBER))
   return -1;

//  fprintf(stdout, "\nread_number_value: called parse_ctoken");

  if (ctoken.type == CTOKEN_TYPE_OPERATOR_ARITHMETIC
   && ctoken.subtype == CTOKEN_SUBTYPE_MINUS)
  {
// negative number
    if (!parse_ctoken(&ctoken, PARSE_TYPE_SINGLE_NUMBER))
     return -1;
    if (ctoken.type != CTOKEN_TYPE_NUMBER)
     return -1;

    *number = ctoken.number_value * -1;
    return 1;
  }

/*  if (ctoken.type == CTOKEN_TYPE_OPERATOR_ARITHMETIC
   && ctoken.subtype == CTOKEN_SUBTYPE_BITWISE_NOT)
  {
// negative number
    if (!parse_ctoken(&ctoken, PARSE_TYPE_SINGLE_NUMBER))
     return -1;
    if (ctoken.type != CTOKEN_TYPE_NUMBER)
     return -1;

    *number = ~ctoken.number_value;
    return 1;
  }*/

  if (ctoken.type == CTOKEN_TYPE_PUNCTUATION
   && ctoken.subtype == CTOKEN_SUBTYPE_BRACKET_OPEN)
  {
//   fprintf(stdout, "\nrecursive call to read_literal_numbers");

   retval = read_literal_numbers(&ctoken); // recursive
   switch(retval)
   {
    case 0: return -1; // error
    case 1: return 0; // couldn't be fully resolved
    case 2: // resolved completely
     if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACKET_CLOSE))
      return -1; // error! no closing bracket
     *number = ctoken.number_value;
     return 1;
   }
  }

  if (ctoken.type != CTOKEN_TYPE_NUMBER)
   return 0; // cannot resolve further

  *number = ctoken.number_value;

  return 1;


}

// this function checks whether a reference to an element of an array can be resolved to a simple reference to a constant memory address, and does so if possible
// for example, a[5][3] can be resolved. a[b][3] can't.
// should be called just after the variable name read.
// returns -1 if cannot resolve (or on error, that will be picked up later)
// if successfully resolved, returns the offset for the element specified by the dimensions.
int attempt_array_resolve(int id_index)
{
 int i;
 int save_scode_pos = cstate.scode_pos;
 int reference_offset = 0;

 struct identifierstruct* array_id = &identifier[id_index];
 struct ctokenstruct ctoken;

 for (i = 0; i < array_id->array_dims; i ++)
 {
  if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_SQUARE_OPEN))
  {
   cstate.scode_pos = save_scode_pos;
   return -1;
  }
  if (read_literal_numbers(&ctoken) < 2)
   return -1; // r_l_n returns 2 if fully resolved, 1 if unable to fully resolve, or 0 on error
// now we multiply the element size by the number just read in to find the offset for this dimension of the array:
  reference_offset += array_id->array_element_size [i] * ctoken.number_value;
  if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_SQUARE_CLOSE))
  {
   cstate.scode_pos = save_scode_pos;
   return -1; // error - will be picked up in comp_next when the array parser tries to deal with the array index as an expression
  }
 }

 return reference_offset;

}

int skip_spaces(void)
{

 int read_source;

 while (TRUE)
 {
  read_source = c_get_next_char_from_scode(); // note scode_pos is a pointer, and is incremented by get_next_char_from_scode
  if (read_source == REACHED_END_OF_SCODE)
   return 0;
  if (read_source != ' ')
  {
   (cstate.scode_pos) --;
   return 1;
  }
 };

 return 0; // should never be reached
}

// debugging function - not currently used
void print_identifier_list(void)
{
 int i;

   fprintf(stdout, "\n Identifier list");


 for (i = 0; i < IDENTIFIER_MAX; i ++)
 {
  if (identifier[i].type != ID_NONE)
  {
   fprintf(stdout, "\nid %2d ", i);
   switch(identifier[i].type)
   {
    case C_ID_KEYWORD: fprintf(stdout, "keyword"); break;
    case ID_USER_UNTYPED: fprintf(stdout, "untyped"); break;
    case ID_USER_INT: fprintf(stdout, "int"); break;
    case ID_USER_LABEL: fprintf(stdout, "label"); break;
    case ID_USER_LABEL_UNDEFINED: fprintf(stdout, "undefined label"); break;
    case ID_USER_CFUNCTION: fprintf(stdout, "cfunction"); break;
    case ID_PROCESS: fprintf(stdout, "process"); break;
    case ID_ENUM: fprintf(stdout, "enum"); break;
    case ID_BUILTIN_CFUNCTION: fprintf(stdout, "built-in cfunction"); break;
    case ASM_ID_OPCODE: fprintf(stdout, "asm opcode"); break;
    case ASM_ID_KEYWORD: fprintf(stdout, "asm keyword"); break;
    case ASM_ID_GENERIC_UNDEFINED: fprintf(stdout, "asm generic undefined"); break;
    case ASM_ID_GENERIC_DEFINED: fprintf(stdout, "asm generic defined"); break;
    case ASM_ID_GENERIC_ALIAS: fprintf(stdout, "asm generic alias"); break;
    case ASM_ID_ASPACE: fprintf(stdout, "asm aspace"); break;
    case ASM_ID_NSPACE: fprintf(stdout, "asm nspace"); break;
    default: fprintf(stdout, "unknown (%i)", identifier[i].type);
   }
   fprintf(stdout, " \"%s\" pr_scope %i scope %i as_scope %i ns_scope %i", identifier[i].name, identifier[i].process_scope, identifier[i].scope, identifier[i].aspace_scope, identifier[i].nspace_scope);

  }

 }

// error_call();
}

