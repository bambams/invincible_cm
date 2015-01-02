
#ifndef H_G_CLIENT
#define H_G_CLIENT

void init_program(struct programstruct* cl, int type, int player_index);

int derive_program_properties_from_bcode(struct programstruct* cl, int expect_program_type);
int check_program_type(int program_type, int expect_program_type);
int check_method_type(int method_type, int program_type, int loading);

#endif
