
#ifndef H_G_METHOD_PR
#define H_G_METHOD_PR


//int run_new_process_method(struct procstruct* pr, int m, int parent_vertex, int child_vertex, int child_angle, int start_address, int end_address, int hold);
int run_pr_new_method(struct procstruct* pr, int m, struct bcodestruct* source_bcode);
int run_packet_method(struct procstruct* pr, int m);
int run_dpacket_method(struct procstruct* pr, int m);

int run_designate_point(struct procstruct* pr, int m, s16b* mb);
int run_designate_find_proc(struct procstruct* pr, int m, s16b* mb, int method_status);

s16b run_broadcast(struct procstruct* source_proc, int m);
s16b run_yield(struct procstruct* pr, int m);

void run_stream_method(struct procstruct* pr, int m, int method_type);

//int generate_irpt(struct procstruct* pr, int m, int amount);

#endif
