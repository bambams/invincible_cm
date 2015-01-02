

#include <allegro5/allegro.h>

#include <stdio.h>
#include <string.h>

#include "m_config.h"

#include "g_header.h"
#include "c_header.h"
#include "e_header.h"
#include "m_globvars.h"

#include "g_misc.h"
#include "e_log.h"
#include "e_help.h"


//ALLEGRO_COLOR mlog_col [MLOG_COLS];



/*

This file contains things for the right-click help facility.

*/

const char* const help_string [] =
{
"", // HELP_NONE - does nothing when clicked

"Help: Missions\nPlay the single-player missions.", // HELP_MISSION_MENU,
"Help: Advanced Missions\nPlay the single-player advanced missions.\n\nThe advanced missions are the same as the normal missions, but you cannot use an operator program to give you direct control over your processes. They must be able to function autonomously.", // HELP_ADVANCED_MISSION_MENU,
"Help: Tutorial\nLearn how to play the game.", // HELP_TUTORIAL_MENU,
"Help: Use System File\nStart setting up a game from a system file. To do this, you must first load a system file into the system file template. What happens next depends on the system file you use.\n\nThere are some example system programs in the game's /src subdirectory, with file names starting with \"sy_\".", // HELP_USE_SYSFILE,
"Help: Load Saved Game\nLoad a saved game from disk.", // HELP_LOAD,
"Help: Load Gamefile\nStart playing from a gamefile. Gamefiles can be saved from the setup menu you get when starting a game from a system file, and their main purpose is to set up the game start for each player in a multiplayer game. Read the manual for more information about multiplayer.", // HELP_LOAD_GAMEFILE,
"Help: Options\nSet various game options. [not currently implemented]", // HELP_OPTIONS,
"Help: Exit\nExit to your system.", // HELP_MAIN_EXIT,

"Help: Start\nStart the game with the chosen settings.", // HELP_SETUP_START,
"Help: Players\nChoose the number of players (includes any computer-controlled players).\nThe system program may or may not let you change this.", // HELP_SETUP_PLAYERS,
"Help: Turns\nChoose the number of turns in the game. Set to zero for an indefinite number of turns.\nThe system program may or may not let you change this.", // HELP_SETUP_TURNS,
"Help: Minutes per turn\nChoose the number of minutes in each turn. Set to zero for turns of indefinite length.\nThe system program may or may not let you change this.", // HELP_SETUP_MINUTES,
"Help: Processes\nChoose the number of processes each player can have at once.\nThe system program may or may not let you change this.", // HELP_SETUP_PROCS,
"Help: World size (x)\nChoose the width of the game area, in blocks of 128 pixels.\nThe system program may or may not let you change this.", // HELP_SETUP_W_BLOCK,
"Help: World size (y)\nChoose the height of the game area, in blocks of 128 pixels.\nThe system program may or may not let you change this.", // HELP_SETUP_H_BLOCK,
"Help: Back\nReturn to the main menu.", // HELP_SETUP_BACK_TO_START,
"Help: Player name\nChoose a player's name.", // HELP_SETUP_PLAYER_NAME,
"Help: Save gamefile\nSave a .gam file to disk with the current settings and templates. Use this to set up a multiplayer game, to make sure that each player loading the gamefile will start with the game in exactly the same state. Read the manual for more information about multiplayer.", // HELP_SETUP_SAVE_GAMEFILE,

"Help: Tutorial: basics\nLearn the basics of how to control processes when in operator mode (operator mode is used for the standard missions, and allows you to give commands directly to your processes).", // HELP_TUTORIAL_1,
"Help: Tutorial: building + attacking\nLearn how to build new processes and attack when in operator mode.", // HELP_TUTORIAL_2,
"Help: Tutorial: templates\nLearn how to use templates to load new processes and other programs into the game.", // HELP_TUTORIAL_3,
"Help: Tutorial: delegation\nLearn how to use delegate programs in games that do not allow operator programs (such as the advanced missions, and multiplayer).", // HELP_TUTORIAL_4,
"Help: Back\nReturn to the main menu.", // HELP_TUTORIAL_BACK
"Help: Mission\nStart a mission. Some missions start locked; to play locked missions, you will need to complete all previous missions.", // HELP_MISSION,
"Help: Back\nReturn to the main menu.", // HELP_MISSIONS_BACK,
"Help: Advanced mission\nStart an advanced mission. Some missions start locked; to play locked missions, you will need to complete all previous missions.", // HELP_ADVANCED_MISSION,
"Help: Back\nReturn to the main menu.", // HELP_ADVANCED_MISSIONS_BACK,
"Help: Load file\nLoad a \".c\" (source) or \".bc\" (bcode) file from disk into this template. Source files will be automatically compiled when loaded.\nThe file must be the correct type (process, system, observer etc.) for the template.", // HELP_TEMPLATE_OPEN_FILE,
"Help: Import from editor\nImport a file from a tab in the code editor. If it is a source file, it will be automatically compiled.", // HELP_TEMPLATE_IMPORT,
"Help: Clear template\nClear the template so another file can be loaded into it.", // HELP_TEMPLATE_CLEAR,
"Help: Process template\nYou can load process files (such as the pr_*.c files in the src directory) into this template to make them available in the game.", // HELP_TEMPL_PROC,
"Help: System template\nYou can load a system program (such as the sy_*.c files in the src directory) into this template and use it to start a game by selecting Use System File in the main menu.", // HELP_TEMPL_SYSTEM,
"Help: Operator template\nYou can load an operator program (such as the op_basic.c file in the src directory) into this template. An operator program gives you a user interface that lets you give commands to your processes. Some game modes don't allow operator programs.", // HELP_TEMPL_OPERATOR,
"Help: Delegate template\nYou can load a delegate program into this template. A delegate program can give commands to your processes, but cannot accept input from you and must be able to function autonomously.", // HELP_TEMPL_DELEGATE,
"Help: Observer template\nYou can load an observer program (such as the ob_basic.c file in the src directory) into this template. An observer program gives you a user interface that lets you watch a game without giving commands to any processes.", // HELP_TEMPL_OBSERVER,
"Help: Default mission process template\nThe contents of this template will be automatically available in your process templates at the start of a mission.\nThe first two process templates are automatically loaded with basic processes, but you can change these if you want.", // HELP_TEMPL_DEFAULT_PROC,
"Help: Default mission operator template\nThe contents of this template will be automatically loaded as an operator program at the start of a mission. An operator program gives you a user interface that lets you give commands to your processes. Some game modes don't allow operator programs.\nThis template is automatically loaded with the standard op_basic.c operator, but you can change it if you want.", // HELP_TEMPL_DEFAULT_OPERATOR,
"Help: Default mission observer template\nThe contents of this template will be automatically loaded as an observer program at the start of an advanced mission. An observer program gives you a user interface that lets you watch a game without giving commands to any processes.\nThis template is automatically loaded with the standard ob_basic.c observer, but you can change it if you want.", // HELP_TEMPL_DEFAULT_OBSERVER,
"Help: Default mission delegate template\nThe contents of this template will be automatically loaded as a delegate program at the start of an advanced mission. A delegate program can give commands to your processes, but cannot accept input from you and must be able to function autonomously.", // HELP_TEMPL_DEFAULT_DELEGATE,

"Help: Save turnfile\nTurnfiles are used for multiplayer games. See the manual for more information about multiplayer.", // HELP_TURNFILE_BUTTON,

"Help: Ed button (Editor)\nOpen the code editor, which lets you edit and compile programs to use in the game.", // HELP_MODE_BUTTON_EDITOR
"Help: Te button (Templates)\nOpen the templates menu, which lets you load programs into memory so that they can be used by the game.", // HELP_MODE_BUTTON_TEMPLATES
"Help: Pr button (Programs)\nOpen the program menu, which gives basic information about the performance of system, observer and client programs.", // HELP_MODE_BUTTON_PROGRAMS
"Help: Sy button (System Menu)\nOpen the system menu, which lets you save the game, quit and do various other things.", // HELP_MODE_BUTTON_SYSMENU
"Help: X button (Close)\nClose the menu panel.", // HELP_MODE_BUTTON_CLOSE

"Help: New\nCreate new empty source file.", // HELP_SUBMENU_NEW,
"Help: Open\nOpen a \".c\" (source) or \".bc\" (bcode) file from disk.", // HELP_SUBMENU_OPEN,
"Help: Save\nSave file to disk.", // HELP_SUBMENU_SAVE,
"Help: Save as\nSave file with a different name.", // HELP_SUBMENU_SAVE_AS,
"Help: Close\nClose file.", // HELP_SUBMENU_CLOSE_FILE,
"Help: Undo\nUndo last edit.", // HELP_SUBMENU_UNDO,
"Help: Redo\nRedo last undone edit.", // HELP_SUBMENU_REDO,
"Help: Cut\nCut selection.", // HELP_SUBMENU_CUT,
"Help: Copy\nCopy selection.", // HELP_SUBMENU_COPY,
"Help: Paste\nPaste from clipboard.", // HELP_SUBMENU_PASTE,
"Help: Clear\nDelete selection.", // HELP_SUBMENU_CLEAR,
"Help: Find\nFind in current tab.", // HELP_SUBMENU_FIND,
"Help: Find next\nRepeat search.", // HELP_SUBMENU_FIND_NEXT,
"Help: Test build\nAttempt to compile the currently open tab, discarding the results. This is a quick way to make sure that your code will compile correctly when you try to load it into a template.", // HELP_SUBMENU_TEST_BUILD,
"Help: Build bcode\nCompile the currently open tab to bcode.\nThere is usually no reason to do this, because source files are automatically compiled whenever you use them.", // HELP_SUBMENU_BUILD_BCODE,
"Help: Build asm\nCompile the currently open tab to assembly code. Might be useful for debugging. Sometimes produces files that are too long; if you have this problem, try the Crunched asm command instead.", // HELP_SUBMENU_BUILD_ASM,
"Help: Crunched asm\nDoes the same thing as Build asm, but puts multiple commands on each line to reduce the chance of running out of space.", // HELP_SUBMENU_CRUNCH_ASM,
"Help: Convert bcode\nIf you have a tab of bcode (produced by loading a \".bc\" file or using the Build bcode command), this will convert it into text format (as a list of numbers).", // HELP_SUBMENU_CONVERT_BCODE,
"Help: Import bcode\nCopies bcode from a process in the current game. Requires a process to be selected so that its details are being displayed in the process data box.", // HELP_SUBMENU_IMPORT_BCODE


"Help: Pause\nPause the game. Processes and delegates stop running and the game world is frozen in place. Observer and operator programs keep working, so you may get a basic user interface. The system program also runs.", // HELP_SYSMENU_PAUSE
"Help: Halt execution\nHalt the game completely. All programs stop running and nothing happens (including the user interface in your observer or operator program).", // HELP_SYSMENU_HALT
"Help: Quit game\nExit to the main menu.", // HELP_SYSMENU_QUIT
"Help: Return\nThe game is over, so you can return to the main menu now.", // HELP_SYSMENU_RETURN_TO_MAIN
"Help: Confirm quit\nReally quit?", // HELP_SYSMENU_QUIT_CONFIRM
"Help: Save game\nSaves the game to disk. You can reload saved games from the main menu. Saved game files should have the extension '.sav'.", // HELP_SYSMENU_SAVE


};



void print_help(int help_type)
{

#ifdef SANITY_CHECK

 if (help_type < 0 || help_type >= HELP_STRINGS)
	{
		fprintf(stdout, "\nError: e_help.c: print_help(): invalid help string %i (should be 0 to %i).", help_type, HELP_STRINGS);
		error_call();
	}

#endif

// Need to go through and break down the help_string strings into printable bits:

// PRINT_STR_LENGTH is the number of characters that fit on a single line.
#define PRINT_STR_LENGTH 73

 if (help_type == HELP_NONE)
		return; // does nothing at all (this is for things like clicking on a heading in the start menu

 reset_log();

 int i, j, k;
 char print_str [PRINT_STR_LENGTH] = "";
 char print_word [30] = "";

 i = 0;
 j = 0;
 k = 0;

		while(TRUE)
		{
		 print_word [k] = help_string [help_type] [i];
			if (help_string [help_type] [i] == ' ')
			{
				if (k + j >= PRINT_STR_LENGTH - 1)
				{
					write_line_to_log(print_str, MLOG_COL_HELP);
					print_word [k + 1] = '\0';
			  strcpy(print_str, print_word);
					j = k + 1;
			  k = -1;
				}
				 else
					{
 					print_word [k + 1] = '\0';
						strcat(print_str, print_word);
						j += k + 1;
						k = -1;
					}
			}
			if (help_string [help_type] [i] == '\n'
				|| help_string [help_type] [i] == '\0')
			{
				if (k + j >= PRINT_STR_LENGTH - 1)
				{
					write_line_to_log(print_str, MLOG_COL_HELP);
					print_word [k] = '\0';
			  strcpy(print_str, print_word);
					write_line_to_log(print_str, MLOG_COL_HELP);
					print_word [0] = '\0';
					print_str [0] = '\0';
					j = 0;//k;
			  k = -1;
				}
				 else
					{
 					print_word [k] = '\0';
						strcat(print_str, print_word);
 					write_line_to_log(print_str, MLOG_COL_HELP);
 					print_str [0] = '\0';
						j = 0;
						k = -1;
					}
			}
			if (help_string [help_type] [i] == '\0')
			{
				if (strlen(print_str) != 0)
					write_line_to_log(print_str, MLOG_COL_HELP);
				break;
			}

		 i ++;
		 k ++;

		};




}


