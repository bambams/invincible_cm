
#ifndef H_C_COMP
#define H_C_COMP

int add_intercode(int ic_type, int op1, int op2, int op3);

int compile_source_to_bcode(struct sourcestruct* source, struct bcodestruct* bcode, struct sourcestruct* asm_source, int crunch_asm, const char* defined);

int comp_error(int error_type, struct ctokenstruct* ctoken);
int comp_error_minus1(int error_type, struct ctokenstruct* ctoken);

#endif
