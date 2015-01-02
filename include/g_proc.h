
#ifndef H_G_PROC
#define H_G_PROC


struct basic_proc_propertiesstruct
{
 int type; // should be PROGRAM_TYPE_PROCESS (but this value can be different if get_basic_proc_properties_from_bcode is recording an error)
 int shape;
 int size;
 int base_vertex;
 int mass;
 int method_mass;
 int base_cost;
 int method_type [METHODS+1];
 int method_data [METHODS+1];
 int method_vertex [METHODS+1];
 int method_extension [METHODS+1] [METHOD_EXTENSIONS];
 int method_extensions [METHODS+1]; // total
 int vertex_method [METHOD_VERTICES];
 int method_error; // if 1, proc has at least one method error (doesn't prevent creation)

 int data_cost;
 int irpt_cost;


};


int new_proc(int p);
void initialise_proc(int p);

int create_single_proc(int p, int x, int y, int angle);
int create_new_group_member(struct procstruct* pr, int new_p, int source_vertex, int pr2_vertex, int pivot_angle);

int find_empty_proc(int team_index);

int derive_proc_properties_from_bcode(struct procstruct* pr, struct basic_proc_propertiesstruct* bprop, int parent_proc_index);
void finish_adding_new_proc(int p);
void apply_packet_damage_to_proc(struct procstruct* pr, int damage, int cause_team);
void hurt_proc(int p, int damage, int cause_team);
void virtual_method_break(struct procstruct* pr);
void proc_explodes(struct procstruct* pr, int destroyer_team);
int destroy_proc(struct procstruct* pr);

int check_valid_proc_method(int method_type, int loading);

int get_point_alloc_efficiency(al_fixed x, al_fixed y);

int get_basic_proc_properties_from_bcode(struct basic_proc_propertiesstruct* prop, s16b* bcode_op, int limited_methods);

enum
{
PROC_PROPERTIES_SUCCESS,
PROC_PROPERTIES_FAILURE_PROGRAM_TYPE,
PROC_PROPERTIES_FAILURE_SHAPE,
PROC_PROPERTIES_FAILURE_SIZE,

};


#endif
