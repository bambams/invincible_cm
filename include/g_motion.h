
#ifndef H_G_MOTION
#define H_G_MOTION

void run_motion(void);

void add_proc_to_blocklist(struct procstruct* pr);
//int check_proc_point_collision(struct procstruct* pr, al_fixed x, al_fixed y);

int check_notional_block_collision_multi(int notional_shape, int notional_size, al_fixed notional_x, al_fixed notional_y, al_fixed notional_angle);
void apply_impulse_to_proc_at_vertex(struct procstruct* pr, int v, al_fixed force, al_fixed impulse_angle);

void apply_impulse_to_group(struct groupstruct* gr, al_fixed x, al_fixed y, al_fixed force, al_fixed impulse_angle);
void apply_impulse_to_group_at_member_vertex(struct groupstruct* gr, struct procstruct* pr, int vertex, al_fixed force, al_fixed impulse_angle);
int check_notional_solid_block_collision(int notional_shape, int notional_size, al_fixed notional_x, al_fixed notional_y, al_fixed notional_angle);

#endif


