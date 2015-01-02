

#ifndef H_C_ASM
#define H_C_ASM

int assemble(int asm_only_process);
void test_assembler(void);
//void init_identifiers(void);
void init_assembler(int bcode_size);

int allocate_aspace(int id_index);
int allocate_nspace(int id_index);

int resolve_undefined_asm_generics(void);

#endif
