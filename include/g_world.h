
#ifndef H_G_WORLD
#define H_G_WORLD

void start_world(void);

void initialise_world(void);
void new_world_from_world_init(void);
void deallocate_world(void);

void run_world(void);

void disrupt_block_nodes(al_fixed x, al_fixed y, int player_cause, int size);
void disrupt_single_block_node(al_fixed x, al_fixed y, int player_cause, int size);

void change_block_node(struct blockstruct* bl, int i, int move_x, int move_y, int size_change);
void change_block_node_colour(struct blockstruct* bl, int i, int player_cause);
void align_block_node(struct blockstruct* bl, int i);

void run_markers(void);

#endif
