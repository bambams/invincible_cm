
#ifndef H_C_INIT
#define H_C_INIT

void init_identifiers(int bcode_size);

int check_word_type(char* word); // usde in e_editor.c

#define C_KEYWORDS (KEYWORDS_END - KEYWORD_INT)
#define C_BUILTIN_CFUNCTION_NAMES (BUILTIN_CFUNCTIONS_END - BUILTIN_CFUNCTION_PUT)
#define ASM_KEYWORDS (KEYWORDS_ASM_END - KEYWORD_ASM_INT)


struct c_keywordstruct
{
 char name [IDENTIFIER_MAX_LENGTH];
 int index;

};


#endif
