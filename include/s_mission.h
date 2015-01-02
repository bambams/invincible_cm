

#ifndef H_S_MISSION
#define H_S_MISSION




int use_mission_system_file(int mission);

void save_mission_status_file(void);
void load_mission_status_file(void);

enum
{
MISSION_STATUS_UNFINISHED,
MISSION_STATUS_1STAR,
MISSION_STATUS_2STARS,
MISSION_STATUS_3STARS,

MISSION_STATUSES // used for bounds-checking loaded values
};

enum
{
MISSION_TUTORIAL1,
MISSION_TUTORIAL2,
MISSION_TUTORIAL3,
MISSION_TUTORIAL4,
MISSION_MISSION1,
MISSION_MISSION2,
MISSION_MISSION3,
MISSION_MISSION4,
MISSION_MISSION5,
MISSION_MISSION6,
MISSION_ADVANCED1, // currently it is assumed that all advanced missions, and nothing else, come after this one
MISSION_ADVANCED2,
MISSION_ADVANCED3,
MISSION_ADVANCED4,
MISSION_ADVANCED5,
MISSION_ADVANCED6,
MISSIONS
};

struct missionsstruct
{
	int status [MISSIONS]; // MISSION_STATUS_* enum
	int locked [MISSIONS]; // 0 or 1
};

struct missionsstruct missions;

#endif

