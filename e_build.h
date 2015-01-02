
#ifndef H_E_BUILD
#define H_E_BUILD

void build_source_tab(int output_type);

int build_source_edit(int output_type, struct source_editstruct* se, struct bcodestruct* target_bcode);

int convert_bcode_to_source_tab(void);
int import_proc_bcode_to_bcode_tab(void);

#endif
