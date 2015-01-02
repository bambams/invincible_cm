
#ifndef H_G_METHOD_CLOB
#define H_G_METHOD_CLOB


int set_marker(int type, int timeout, int colour);
//int place_marker(al_fixed x, al_fixed y);
//int place_marker2(al_fixed x2, al_fixed y2);
void clear_markers(void);
void expire_markers(void);

s16b run_query(s16b* mb);
int bind_marker(int m, int proc_index);
int bind_marker2(int m, int proc_index);
int unbind_process(int proc_index);
void expire_a_marker(int m);

s16b cl_copy_to_template(struct programstruct* cl, s16b* mb);

#endif

