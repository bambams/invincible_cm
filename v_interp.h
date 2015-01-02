
#ifndef H_V_INTERP
#define H_V_INTERP

//int interpret_bcode(struct bcodestruct* bc, struct registerstruct* regs, struct methodstruct method [METHODS]);
int interpret_bcode(struct procstruct* pr, struct programstruct* cl, struct bcodestruct* bc, struct registerstruct* regs, struct methodstruct* methods, int instr_left);
void run_interpreter(struct procstruct* proc);

#endif
