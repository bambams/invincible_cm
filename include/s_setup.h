
#ifndef H_S_SETUP
#define H_S_SETUP




#define DEFAULT_PLAYERS 2
#define MIN_PLAYERS 1
#define MAX_PLAYERS 4

#define DEFAULT_PROCS 100
#define MIN_PROCS_PER_TEAM 1
#define MAX_PROCS_PER_TEAM 200
// MAX_PROCS_IN_WORLD is the maximum number of procs a world can possibly have
#define MAX_PROCS_IN_WORLD (MAX_PROCS_PER_TEAM*MAX_PLAYERS)

//#define DEFAULT_GEN_LIMIT (DEFAULT_PROCS / 2)
//#define MIN_GEN_LIMIT 4
//#define MAX_GEN_LIMIT 100

#define DEFAULT_PACKETS 200
#define MIN_PACKETS 50
#define MAX_PACKETS 300

#define DEFAULT_BLOCKS 30
#define MIN_BLOCKS 20
#define MAX_BLOCKS 80


void setup_default_world_pre_init(struct world_preinitstruct* pre_init);
int derive_pre_init_from_system_program(struct programstruct* cl, int bcode_pos);
int use_sysfile_from_template(void);



#endif
