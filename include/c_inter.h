
#ifndef H_C_INTER
#define H_C_INTER

int intercode_to_bcode_or_asm(struct bcodestruct* bcode, struct sourcestruct* target_source, int generate_asm);
int get_current_intercode_index(void);
void modify_intercode_operand(int intercode_index, int operand, int new_value);


#endif

