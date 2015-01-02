/*
This file contains functions for setting up missions.

It loads up mission files and sets up menus and worlds based on them.

Missions will work like this:
 - at start, a game program will be loaded in.
 - this program will contain setup routines that will load other programs into hidden game templates (will need to revise template code a bit to deal with this)
  - the hidden templates may just be player zero's templates, which the game program will have access to (along with other templates).
 - it will then be able to make use of these templates as needed.
 - it will also use a special game method to specify what templates the player may use.
  - it will need a set of game methods, only available to game programs, that will allow it to do things like this.
 - after it has run once, it will modify itself so as not to run these setup routines again.
Then, the player will:
 - see a text description of the mission and a "start" button. The player will be able to use the template and editor windows. Templates may come pre-loaded but may be changeable.
 - when clicks "start", proc zero will be placed in world and game will start to run.

*/

#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>

#include <stdio.h>
#include <string.h>

#include "m_config.h"

#include "g_header.h"
#include "m_globvars.h"

#include "g_misc.h"

#include "c_header.h"
#include "c_prepr.h"
#include "e_slider.h"
#include "e_header.h"
#include "e_log.h"

#include "c_init.h"
#include "i_input.h"
#include "i_view.h"
#include "m_input.h"
#include "s_mission.h"
#include "t_files.h"




/*struct missionsstruct
{
	int status [MISSIONS]; // MISSION_STATUS_* enum
	int locked [MISSIONS]; // 0 or 1
};*/

struct missionsstruct missions;

void clear_mission_status(void);
void unlock_basic_missions(void);
void process_mission_locks(void);

// resets mission status - all missions are set to unfinished (avoid doing this if possible!)
void clear_mission_status(void)
{
	int i;

	for (i = 0; i < MISSIONS; i ++)
	{
		missions.status [i] = MISSION_STATUS_UNFINISHED;
		missions.locked [i] = 1;
	}

// unlock the tutorials and the first mission:
 unlock_basic_missions();

}


void unlock_basic_missions(void)
{

	missions.locked [MISSION_TUTORIAL1] = 0;
	missions.locked [MISSION_TUTORIAL2] = 0;
	missions.locked [MISSION_TUTORIAL3] = 0;
	missions.locked [MISSION_TUTORIAL4] = 0;

	missions.locked [MISSION_MISSION1] = 0;
/*	missions.locked [MISSION_MISSION2] = 0;
	missions.locked [MISSION_MISSION3] = 0;
	missions.locked [MISSION_MISSION4] = 0;
	missions.locked [MISSION_MISSION5] = 0;
	missions.locked [MISSION_MISSION6] = 0;*/
	missions.locked [MISSION_ADVANCED1] = 0;
/*	missions.locked [MISSION_ADVANCED2] = 0;
	missions.locked [MISSION_ADVANCED3] = 0;
	missions.locked [MISSION_ADVANCED4] = 0;
	missions.locked [MISSION_ADVANCED5] = 0;
	missions.locked [MISSION_ADVANCED6] = 0;*/

}

int use_mission_system_file(int mission)
{

 char mission_file_path [FILE_PATH_LENGTH];

 if (missions.locked [mission] == 1)
	{
   write_line_to_log("Mission locked (complete preceding missions to unlock).", MLOG_COL_HELP);
   return 0;
	}

 switch(mission)
 {
  case MISSION_TUTORIAL1:
   strcpy(mission_file_path, "missions/mt1_sy.c");
   break;
  case MISSION_TUTORIAL2:
   strcpy(mission_file_path, "missions/mt2_sy.c");
   break;
  case MISSION_TUTORIAL3:
   strcpy(mission_file_path, "missions/mt3_sy.c");
   break;
  case MISSION_TUTORIAL4:
   strcpy(mission_file_path, "missions/mt4_sy.c");
   break;
  case MISSION_MISSION1:
  case MISSION_ADVANCED1: // same file is used for advanced missions, but compiled with the ADVANCED macro set
   strcpy(mission_file_path, "missions/m1_sy.c");
   break;
  case MISSION_MISSION2:
  case MISSION_ADVANCED2:
   strcpy(mission_file_path, "missions/m2_sy.c");
   break;
  case MISSION_MISSION3:
  case MISSION_ADVANCED3:
   strcpy(mission_file_path, "missions/m3_sy.c");
   break;
  case MISSION_MISSION4:
  case MISSION_ADVANCED4:
   strcpy(mission_file_path, "missions/m4_sy.c");
   break;
  case MISSION_MISSION5:
  case MISSION_ADVANCED5:
   strcpy(mission_file_path, "missions/m5_sy.c");
   break;
  case MISSION_MISSION6:
  case MISSION_ADVANCED6:
   strcpy(mission_file_path, "missions/m6_sy.c");
   break;
  default:
   write_line_to_log("Couldn't start mission (invalid mission number).", MLOG_COL_ERROR);
   return 0; // shouldn't happen

 }

 struct sourcestruct mission_src;

 if (!load_source_file(mission_file_path, &mission_src, 0, 0))
 {
    write_line_to_log("Couldn't open mission file (file not found?).", MLOG_COL_ERROR);
    return 0;
 }

 int load_result;

 if (mission >= MISSION_ADVANCED1)
		load_result = finish_loading_source_file_into_template(&mission_src, 0, "ADVANCED");
	  else
		  load_result = finish_loading_source_file_into_template(&mission_src, 0, NULL);

 if (!load_result)
 {
    write_line_to_log("Couldn't load mission file into template.", MLOG_COL_ERROR);
    return 0;
 }

 return 1;

}







void load_mission_status_file(void)
{

 clear_mission_status();

#define MISSIONFILE_SIZE 128

 FILE *missionfile;
 char buffer [MISSIONFILE_SIZE];

 missionfile = fopen("msn.dat", "rb");

 if (!missionfile)
 {
  fprintf(stdout, "\nNo mission data found. Starting with default mission status.");
  return;
 }

 int read_in = fread(buffer, 1, MISSIONFILE_SIZE, missionfile);

 if (ferror(missionfile)
  || read_in == 0)
 {
  fprintf(stdout, "\nFailed to read mission status from msn.dat. Starting with default mission status.");
  fclose(missionfile);
  return;
 }

 int i;

 for (i = 0; i < MISSIONS; i ++)
	{
		missions.status [i] = buffer [i];
		if (missions.status [i] < 0
		 || missions.status [i] >= MISSION_STATUSES)
		{
			fprintf(stdout, "\nFailure: mission %i status (%i) invalid (should be %i to %i).\nStating with default missions status.", i, missions.status [i], 0, MISSION_STATUSES);
   fclose(missionfile);
   clear_mission_status();
   return;
		}
	}

// Now process the loaded statuses to produce the correct locked/unlocked settings:
 unlock_basic_missions(); // unlocks tutorials and mission 1

 process_mission_locks(); // unlocks any mission if the previous mission has been completed.


 fclose(missionfile);

}


void process_mission_locks(void)
{

 int i;

 for (i = MISSION_MISSION1; i < MISSIONS - 1; i ++)
	{
  if (missions.status [i] != MISSION_STATUS_UNFINISHED)
	 	missions.locked [i + 1] = 0;
	}

}

void save_mission_status_file(void)
{

 process_mission_locks(); // this makes sure that any change in missions statuses is immediately reflected in unlocks

 char buffer [MISSIONFILE_SIZE];

 FILE *file;

// open the file:
 file = fopen("msn.dat", "wb");

 if (!file)
 {
  fprintf(stdout, "\nFailed to save mission status to msn.dat: couldn't open file.");
  return;
 }

 int i;

 for (i = 0; i < MISSIONS; i ++)
	{
		buffer [i] = missions.status [i];
	}

 int written = fwrite(buffer, 1, MISSIONS, file);

 if (written != MISSIONS)
 {
  fprintf(stdout, "\nFailed to save mission status to msn.dat: couldn't write data (tried to write %i; wrote %i).", MISSIONS, written);
  fclose(file);
  return;
 }

 fclose(file);


}


