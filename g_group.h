
#ifndef H_G_GROUP
#define H_G_GROUP

int connect_procs(struct procstruct* pr1, struct procstruct* pr2, int vertex_1, int vertex_2);
int disconnect_procs(struct procstruct* pr1, struct procstruct* pr2);
void prepare_shared_resource_distribution(struct procstruct* removed_pr, int destroyed, int connection_severed);
void fix_resource_pools(void);

int extract_destroyed_proc_from_group(struct procstruct* pr1);

#endif
