/*

This is a basic operator program.
It uses the standard command macros, so it should be able to issue commands to processes
 with the prsc prefix.

*/

// This is the process's interface definition. It determines the nature of the process and
// the methods it has available. It must come before any code.
interface
{
 PROGRAM_TYPE_OPERATOR, // program's type
 {
  METH_INPUT: {MT_OB_INPUT}, // input method - allows reading of user input
  METH_COMMAND_GIVE: {MT_CL_COMMAND_GIVE}, // command method - allows operator to communicate with processes
  METH_COMMAND_REC: {MT_CLOB_COMMAND_REC}, // command method - allows operator to communicate with processes
  METH_POINT: {MT_CLOB_POINT}, // point check - allows operator to find what is at a particular point
  METH_VIEW: {MT_OB_VIEW}, // view - allows interaction with display
  {MT_NONE}, // space for view
  METH_SELECT: {MT_OB_SELECT}, // select - allows operator to set display elements indicating selected process
  METH_CONSOLE: {MT_OB_CONSOLE}, // console - allows operator to control console windows
  METH_STD: {MT_CLOB_STD}, // standard clob method - does various things like give information about the game world
  METH_QUERY: {MT_CLOB_QUERY}, // query - allows operator to get information about processes (similar to MT_PR_INFO)
  METH_MATHS: {MT_CLOB_MATHS}, // maths method (works the same way as MT_PR_MATHS)
  METH_SCAN: {MT_CLOB_SCAN}, // scanner - similar to MT_PR_SCAN but with some additional features
  {MT_NONE}, // makes space for scan method
  {MT_NONE}, // makes space for scan method
  METH_CONTROL: {MT_OB_CONTROL}, // control - allows operator to query various things about the user interface
 }
}


// m_stand_coms.c contains a set of standard command macros that are used by this operator program to communicate with processes
#ifndef STANDARD_COMMANDS
#include "stand_coms.c"
#endif

// Now we declare some global variables.
// Remember that these are initialised (to zero, or to a specified value) when the process is
//  created, but not each time it is run.

int initialised; // When this is zero, it tells the program that it has just been created and needs to initialise.

// Initialisation values - these are set when the program first runs (and may be updated later)
int window_size_x; // size of the current window (can change when loading/saving or when opening/closing editor window)
int window_size_y;
int operator_team; // player number of operator
int first_proc; // index of first process that belongs to this player
int last_proc; // index of last process that belongs to this player
int world_x_min = 256 + 100; // this is the size of the two impassable blocks at the edge of the map plus a bit extra
int world_y_min = 256 + 100; //   so that a process given a move command to the edge of the map doesn't get stuck
int world_x_max; // needs to be derived at initialisation
int world_y_max;
int map_x; // map location on screen
int map_y;
int team_size; // maximum number of processes on each team

// some values used in input functions:
int proc_click;
int proc_select_type;
int mouse_drag;
int mouse_drag_time;
int mouse_drag_x;
int mouse_drag_y;
int pressing_shift;
int pressing_ctrl;

// scan_result holds the results of a scan (which detects processes in a certain area):
int scan_result [100];

// selected_proc is an array holding the indices of the processes that are selected (which may be zero, one or up to MULTISELECT_SIZE processes)
// Multiselect is used when the user has selected more than one process (e.g. with a mouse box drag)
// a #subdefine is like a #define but its scope is limited to this file and files it #includes (and not any file that #includes this file)
#subdefine MULTISELECT_SIZE 20
int selected_proc [MULTISELECT_SIZE+1]; // List of the indices of selected processes. Terminated by -1.
int show_multiselect_target; // Is 1 if user has issued a command to a multiselection and not deselected (so target selector is shown).
int marker_index; // used from time to time to hold marker indices

int target_marker [WAYPOINTS]; // index of current target marker (-1 if none)
int target_map_marker [WAYPOINTS]; // index of current target marker (-1 if none)
int target_line_marker [WAYPOINTS]; // index of marker for line between selected proc and target (-1 if none)
int current_waypoints; // how many waypoints selected_proc [0] has.

enum
{
MOUSE_DRAG_NONE, // user is not dragging the mouse
// the following MOUSE_DRAG values are set depending on where the mouse was when the user first clicked:
MOUSE_DRAG_BOX, // user is drawing a selection box
MOUSE_DRAG_MAP // user is dragging the mouse around the map
};

// MAX_PROCESSES is the maximum number of processes on a team (this is a guess; may need to update)
#subdefine MAX_PROCESSES 200

// If the display is fewer than LARGE_WINDOW_SIZE pixels wide, some elements are not displayed:
#subdefine LARGE_WINDOW_SIZE 600

#define CONSOLE_COMMUNICATE 0
// This console is used to communicate with the user. Its index should be 0 so all messages are printed here by default
#define CONSOLE_COMMAND 1
// This console is used as the OUT2 console for players. Its main use is to display process' command lists.
#define CONSOLE_DETAILS 2
// This console is used to display details of selected processes and similar
#subdefine DETAILS_CONSOLE_WIDTH 20
// Size of details console in characters (each is 6 pixels wide). Actual width of console window
//  is (DETAILS_CONSOLE_WIDTH * 6) + 35
int console_title [8] = {"Console"};
int details_title [8] = {"Details"};
int command_title [8] = {"Command"};

// Function declarations
// Note that "static" in a function declaration means something different from normal C.
//  - It means that all variables in the function are static, which is recommended as
//    static variables are much more efficient than automatic variables (which you
//    shouldn't use without a good reason)
static void mouse_click(int x, int y, int screen_x, int screen_y);
static void select_a_process(int p);
static void run_commands(void);
static void mouse_box_select(int x1, int y1, int x2, int y2);
static void deselect(void);
static void set_target_markers(void);
static void process_action(int console_action, int shift_pressed);
static void clear_details_console(void);
static void run_details_console(void);
static void shift_click_process(int p);
static void print_process_selected(int p);
static void add_destination(int x, int y);
static void add_attack_target(int target_index);
static void keyboard_input(void);
static void give_new_command(int p, int com1, int com2, int com3, int com4);
static void add_command_to_queue(int p, int com1, int com2, int com3, int com4);

// Remember that main is called every time the program is run.
// Its correct type is static void main(void)
static void main(void)
{

// remember that these are all static variables and will not be automatically reinitialised
 int mouse_status;
 int mouse_x;
 int mouse_y;
 int mouse_screen_x;
 int mouse_screen_y;
 int mouse_l_status, mouse_r_status;
 int i, j;
 int target_x, target_y;

 int display_size_reset;
 display_size_reset = 0; // This separate assignment is needed so that just_resized is set to 0 each time the program is run.

 int fast_forwarding;
 fast_forwarding = call(METH_CONTROL, MS_OB_CONTROL_FF); // returns 1 if the game is in fast forward mode

 keyboard_input();

// If this is the first time the operator has been run, we need to set up some
//  basic information about things like which player is controlling it.
 if (initialised == 0)
 {
  initialised = 1;

// Set up some basic values for the operator:
  operator_team = world_team(); // world_team() is a built-in function that uses the MT_CLOB_STD method. It gets the operator's player index.
  first_proc = world_first_process(); // world_first_process() gets this player's first process' index.
  last_proc = world_last_process(); // last process' index
  team_size = world_processes_each();
  world_x_max = world_x() - 512 - 100; // this is the edge of the whole map minus 512 for the size of the 2 blocks at each edge
  world_y_max = world_y() - 512 - 100; //  minus a bit extra so processes don't get stuck at the edge of the map

  call(METH_VIEW, MS_OB_VIEW_DISPLAY_SIZE); // This call leaves the screen size in method registers 1 and 2
  display_size_reset = 1; // Tells the code below that the display's size has been set.
  window_size_x = get(METH_VIEW, 1); // used later
  window_size_y = get(METH_VIEW, 2);
  selected_proc [0] = -1; // This array is terminated by -1.
  current_waypoints = 0;
  for (i = 0; i < WAYPOINTS; i ++)
		{
   target_marker [i] = -1;
   target_map_marker [i] = -1;
   target_line_marker [i] = -1;
		}
  call(METH_VIEW, MS_OB_VIEW_MAP_VISIBLE, 1); // Turns the map on (it will be placed later)

// The operator uses console 0 to communicate with the user (console 3 is used by the mission system programs).
// First we set the console's style (this must be done before the console is opened):
  call(METH_CONSOLE, MS_OB_CONSOLE_STYLE, CONSOLE_COMMUNICATE, CONSOLE_STYLE_BASIC_UP); // BASIC_UP is a normal console with the header at the bottom
// now colour and title (can be done anytime, but let's do it now):
  call(METH_CONSOLE, MS_OB_CONSOLE_BACKGROUND, CONSOLE_COMMUNICATE, BCOL_BLUE);
  call(METH_CONSOLE, MS_OB_CONSOLE_TITLE, CONSOLE_COMMUNICATE, &console_title [0]);
// Need to open it (its position will be given in the display_size_reset code below)
  call(METH_CONSOLE, MS_OB_CONSOLE_OPEN, CONSOLE_COMMUNICATE);
 	call(METH_CONSOLE, MS_OB_CONSOLE_LINES, CONSOLE_COMMUNICATE, 8);
// Since this is console 0, all print commands will print to here by default.

// Set up the command console:
  call(METH_CONSOLE, MS_OB_CONSOLE_STYLE, CONSOLE_COMMAND, CONSOLE_STYLE_BASIC_UP);
  call(METH_CONSOLE, MS_OB_CONSOLE_BACKGROUND, CONSOLE_COMMAND, BCOL_BLUE);
  call(METH_CONSOLE, MS_OB_CONSOLE_TITLE, CONSOLE_COMMAND, &command_title [0]);
  call(METH_CONSOLE, MS_OB_CONSOLE_OPEN, CONSOLE_COMMAND);
 	call(METH_CONSOLE, MS_OB_CONSOLE_LINES, CONSOLE_COMMAND, 0);
// Assign player 0's alternative output to this console
//  (this means processes can print here by calling the STD method with status MS_PR_STD_PRINT_OUT2)
  call(METH_CONSOLE, MS_OB_CONSOLE_OUT2_PLAYER, CONSOLE_COMMAND, 0);

// Set up the special information console:
  call(METH_CONSOLE, MS_OB_CONSOLE_STYLE, CONSOLE_DETAILS, CONSOLE_STYLE_BOX_UP);
  call(METH_CONSOLE, MS_OB_CONSOLE_SIZE, CONSOLE_DETAILS, DETAILS_CONSOLE_WIDTH, 5);
  call(METH_CONSOLE, MS_OB_CONSOLE_TITLE, CONSOLE_DETAILS, &details_title [0]);
//  call(METH_CONSOLE,

 }
  else
  {
   if (call(METH_VIEW, MS_OB_VIEW_DISPLAY_SIZE)) // Calls view method to check whether screen just opened or resized.
   {
    display_size_reset = 1;
    window_size_x = get(METH_VIEW, 1); // used later
    window_size_y = get(METH_VIEW, 2);
   }
  }

// If the screen size was reset (because the game just started, or was loaded from disk etc.) need to update the display.
 if (display_size_reset == 1)
 {
// Position the operator's consoles;
  call(METH_CONSOLE, MS_OB_CONSOLE_MOVE, CONSOLE_COMMUNICATE, 10, window_size_y - 10); // Places console 0 on the screen (last two parameters are x/y)
  call(METH_CONSOLE, MS_OB_CONSOLE_MOVE, CONSOLE_COMMAND, 10, window_size_y - 162); // Places console 0 on the screen (last two parameters are x/y)

// now position the process data display:
  call(METH_VIEW, MS_OB_VIEW_PROC_DATA_XY, window_size_x - 340, 30);

// Now calculate where the bottom right of the screen is, for the map to go:
  map_x = window_size_x - 190;
  map_y = window_size_y - 190;

  if (window_size_x < LARGE_WINDOW_SIZE) // window may be too small to display everything
  {
   map_y -= 200; // To put it above the console
   call(METH_VIEW, MS_OB_VIEW_PROC_DATA, -1); // This hides proc data display (because there's not enough space)
   call(METH_CONSOLE, MS_OB_CONSOLE_CLOSE, CONSOLE_DETAILS); // Close the proc inform box
  }
   else
   {
// Open the process information console and place it just above the map:
    call(METH_CONSOLE, MS_OB_CONSOLE_OPEN, CONSOLE_DETAILS);
    call(METH_CONSOLE, MS_OB_CONSOLE_MOVE, CONSOLE_DETAILS, window_size_x - 190, map_y - 5);
   }
  call(METH_VIEW, MS_OB_VIEW_MAP_XY, map_x, map_y); // Sets map x/y
  call(METH_VIEW, MS_OB_VIEW_MAP_SIZE, 180, 180); // Sets map w/h

 } // Finish resetting the display.

// The command console should be sized to fit the commands:
 int command_console_lines;
 command_console_lines = call(METH_CONSOLE, MS_OB_CONSOLE_LINES_USED, CONSOLE_COMMAND);
 if (command_console_lines > 10)
	 command_console_lines = 10;
	call(METH_CONSOLE, MS_OB_CONSOLE_LINES, CONSOLE_COMMAND, command_console_lines);

// Since this is an operator program, we need to check user input:

 mouse_status = call(METH_INPUT, MS_OB_INPUT_MOUSE_XY);
// This call returns the status of the mouse pointer as one of the built-in MOUSE_STATUS_? enums.
// If the mouse is on the game display, it also sets method registers 1 and 2 to the coordinates
//  of the mouse pointer in the game world (these are absolute coordinates, not on-screen)

 if (mouse_status == MOUSE_STATUS_AVAILABLE // Mouse is on the game screen and not on a console, the map or the process data box
  || mouse_status == MOUSE_STATUS_MAP // Mouse is on the map
  || mouse_status == MOUSE_STATUS_PROCESS // Mouse if on process data box
  || (mouse_status == MOUSE_STATUS_CONSOLE // Mouse is on console and is being dragged
			&& mouse_drag == MOUSE_DRAG_BOX))
 {

// Now get the absolute coordinates of the mouse pointer, which were set by the MS_OB_INPUT_MODE_MOUSE_XY call:
  mouse_x = get(METH_INPUT, 1); // these coordinates are overall game arena coordinates, in pixels
  mouse_y = get(METH_INPUT, 2);

  call(METH_INPUT, MS_OB_INPUT_MOUSE_SCREEN_XY); // Like MS_OB_INPUT_MOUSE_XY but the registers hold on-screen x/y coordinates.
  mouse_screen_x = get(METH_INPUT, 1);
  mouse_screen_y = get(METH_INPUT, 2);

  call(METH_INPUT, MS_OB_INPUT_MOUSE_BUTTON); // Sets method registers to button status
  mouse_l_status = get(METH_INPUT, 1);
  mouse_r_status = get(METH_INPUT, 2);
// Button statuses (for both mouse and keys) are:
//  BUTTON_JUST_RELEASED (-1) - button was being pressed last tick, but not this tick
//  BUTTON_NOT_PRESSED (0) - button not being pressed, and also not just released
//  BUTTON_JUST_PRESSED (1) - button wasn't being pressed last tick, but now is
//  BUTTON_HELD (2) - button still being pressed
// If mouse is outside game area (mouse_status == MOUSE_STATUS_OUTSIDE) the buttons count as not being pressed.

  if (mouse_l_status == BUTTON_JUST_RELEASED)
  {
   int click_time;
   click_time = 4;
   if (fast_forwarding)
    click_time = 25;

   if (mouse_drag == MOUSE_DRAG_BOX
				&& mouse_drag_time >= click_time)
// If the button was just released after being held for a while treat it as having been dragged:
     mouse_box_select(mouse_x, mouse_y, mouse_drag_x, mouse_drag_y);
// mouse_drag_time and mouse_drag_x/y are global variables that hold their value through execution cycles.
  }

  if (mouse_l_status == BUTTON_JUST_PRESSED)
  {
// If the left mouse button was just pressed, set up some global variables to keep track of where it is and how long it's held for:
   mouse_drag = MOUSE_DRAG_BOX;
   mouse_drag_time = 0;
   mouse_drag_x = mouse_x;
   mouse_drag_y = mouse_y;
   mouse_click(mouse_x, mouse_y, mouse_screen_x, mouse_screen_y);
  }

  if (mouse_l_status == BUTTON_HELD)
		{
			if (mouse_drag == MOUSE_DRAG_BOX)
   {
// User appears to be dragging the mouse.
    mouse_drag_time ++;
// Now we need to set a box marker to show the area that is being selected.
// To do this, we call the SELECT method to set a marker (markers are graphic display elements).
// First we setup the type, duration and colour of the marker:
    marker_index = call(METH_SELECT, MS_OB_SELECT_SET_MARKER, MARKER_BOX, 2, BCOL_BLUE); // timeout 2 = lasts 1 cycle
// If SET_MARKER fails, the call returns -1. We could test for this but the following calls will just silently fail so it doesn't really matter.
// Next we set the location of the marker, using absolute world coordinates (because this is a box, this location is the top left):
    call(METH_SELECT, MS_OB_SELECT_PLACE_MARKER, marker_index, mouse_x, mouse_y);
// Since this is a box, we need to set a second location for the bottom right:
    call(METH_SELECT, MS_OB_SELECT_PLACE_MARKER2, marker_index, mouse_drag_x, mouse_drag_y);
   }
   if (mouse_drag == MOUSE_DRAG_MAP)
			{
    call(METH_INPUT, MS_OB_INPUT_MOUSE_MAP_XY);
    call(METH_VIEW, MS_OB_VIEW_FOCUS_XY, get(METH_INPUT, 1), get(METH_INPUT, 2)); // Relocate view to indicated point in map
			}
		}

// Now we check for the right mouse button:
// If mouse_r_status is BUTTON_JUST_PRESSED, we can assume that the mouse is in the game display (and not outside the window or in the editor window etc.)
  if (mouse_r_status == BUTTON_JUST_PRESSED)
  {
// Right mouse button only does anything if at least one process is selected
    if (selected_proc [0] != -1) // selected_proc [0] will be -1 if nothing is selected.
    {
// This call to the INPUT method with MS_OB_INPUT_MOUSE_MAP_XY returns the mouse status and, if it's on the map, also sets the method
//  registers to the absolute world coordinates corresponding to the mouse's position on the map:
     mouse_status = call(METH_INPUT, MS_OB_INPUT_MOUSE_MAP_XY);

     i = 0;

// User can give move commands (only) by right-clicking on the map:
     if (mouse_status == MOUSE_STATUS_MAP)
     {
// The MS_OB_INPUT_MOUSE_MAP_XY call above will have left the world x/y coordinates corresponding to the mouse's position on the map
//  in the INPUT method's registers:
      target_x = get(METH_INPUT, 1);
      target_y = get(METH_INPUT, 2);
      add_destination(target_x, target_y);
     } // end if mouse_status == MOUSE_STATUS_MAP
      else
      {
// The mouse must not be in the map, so we need to check whether the user is giving an attack command.
// The POINT method lets us check whether a process is at a particular point.
// Using it with the MS_CLOB_POINT_FUZZY status means that it will find a process close-ish to the point.
// The call returns the index of the process (or -1 if nothing found)
       proc_click = call(METH_POINT, MS_CLOB_POINT_FUZZY, mouse_x, mouse_y);

// If the user clicked on a friendly process, treat this as if they'd clicked on an empty area:
       if (proc_click >= first_proc
        && proc_click < last_proc)
         proc_click = -1;

// If proc_click is -1, we give a move command to all selected processes that accept move commands:
       if (proc_click == -1)
       {
         add_destination(mouse_x, mouse_y);
       } // end if proc_click == -1
        else
        {
// User must have clicked on a process.
// If it belongs to another player, we issue attack commands to any process that accepts them.
// Otherwise, we issue move commands:
         add_attack_target(proc_click);
        } // end code for user right-clicking on process
        show_multiselect_target = 1;
      } // end code for mouse being on screen but outside map
    } // end if any processes are selected
  } // end if right mouse button clicked

// If the mouse is near the edge of the screen, we scroll the screen:

// These #subdefines determine the size of the edge area and the speed of scrolling:
#subdefine MOUSE_SCROLL_REGION 40

  int scroll_speed_factor = 1;

  if (fast_forwarding)
   scroll_speed_factor = 16; // slow down scroll if fast-forwarding
    else
					scroll_speed_factor = 1;

// We exclude the top-right edge of the screen, where the mode buttons are:
  if (mouse_screen_x < window_size_x - 120
   || mouse_screen_y > MOUSE_SCROLL_REGION)
  {
// Now check for the mouse being near each edge:
   if (mouse_screen_x < MOUSE_SCROLL_REGION) // scroll left
    call(METH_VIEW, MS_OB_VIEW_SCROLL_XY, ((MOUSE_SCROLL_REGION - mouse_screen_x) * -1) / scroll_speed_factor, 0);

   if (mouse_screen_y < MOUSE_SCROLL_REGION) // scroll up
    call(METH_VIEW, MS_OB_VIEW_SCROLL_XY, 0, ((MOUSE_SCROLL_REGION - mouse_screen_y) * -1) / scroll_speed_factor);

   if (mouse_screen_x > (window_size_x - MOUSE_SCROLL_REGION)) // scroll right
    call(METH_VIEW, MS_OB_VIEW_SCROLL_XY, (MOUSE_SCROLL_REGION - (window_size_x - mouse_screen_x)) / scroll_speed_factor, 0);

   if (mouse_screen_y > (window_size_y - MOUSE_SCROLL_REGION)) // scroll down
    call(METH_VIEW, MS_OB_VIEW_SCROLL_XY, 0, (MOUSE_SCROLL_REGION - (window_size_y - mouse_screen_y)) / scroll_speed_factor);
  }

 }
  else
  {
// If the mouse is outside the screen, cancel any mouse dragging:
   mouse_drag = MOUSE_DRAG_NONE;
  }

// Now check for console events (actions)
// Console events are generated when the user clicks on a line in a console.
// Calling the console method with MS_OB_CONSOLE_ACTION_CHECK (and the console index as the third parameter) returns 1 if
//  there is an event. The method's registers 1-3 are then set to values specific to the action (this depends on the action type):
 int console_event_type;
 console_event_type = call(METH_CONSOLE, MS_OB_CONSOLE_ACTION_CHECK, CONSOLE_COMMAND); // check actions from command console
// If nothing received from command console, check communicate console instead:
 if (console_event_type == CONSOLE_ACTION_NONE)
  console_event_type = call(METH_CONSOLE, MS_OB_CONSOLE_ACTION_CHECK, CONSOLE_COMMUNICATE);
 if (console_event_type == CONSOLE_ACTION_PROCESS)
 {
  int console_event_val1, console_event_val2, console_event_val3;
// Ignore register 0, which just indicates which console generated the action. We're assuming all actions that were generated
//  by processes are relevant to this operator program.
  console_event_val1 = get(METH_CONSOLE, 1); // If CONSOLE_ACTION_PROCESS, this is the proc's player's index. Otherwise, set by the printing program
  console_event_val2 = get(METH_CONSOLE, 2); // If CONSOLE_ACTION_PROCESS, this is the proc's index. Otherwise, set by the program that printed the line
  console_event_val3 = get(METH_CONSOLE, 3); // Set by the program or process that printed the line
// This means that the user has clicked on a line printed by a process.
// There may be a special command attached to the console line:
  if (console_event_val1 == operator_team
   && console_event_val3 != 0) // ignore zero actions (all lines printed by processes have a zero action attached by default)
  {
// We tell the process that an action it set has been activated, and send the process whatever value it set for the action
//  (this value is currently in console_event_val3).
// Leave it to the process to work out what to do with the action.
// To think about: should it be possible to queue this action? It isn't presently.
   give_new_command(console_event_val2, COMMAND_ACTION, console_event_val3, 0, 0);
  }
   else
   {
// If there's no special action to perform, we select the process and focus the camera on it:
    select_a_process(console_event_val2);
    call(METH_VIEW, MS_OB_VIEW_FOCUS_PROC, console_event_val2); // Sets camera to a process' location. Does nothing if console_event_val2 is -1
   }
 }

// Now go through all selected_procs and check for destroyed processes:
 i = 0;
 int proc_index;
 int com_status, com_type;


 if (selected_proc [0] != -1)
	{
		proc_index = selected_proc [0];
		if (selected_proc [1] == -1)
  {
// set its verbose flag (assume that the process will cancel it each cycle)
   command_bit_1(proc_index, COMREG_CLIENT_STATUS, COM_CLIENT_ST_VERBOSE);
// display relevant information in the details console
   run_details_console();
  }
   else
    clear_details_console();

// Check for the process changing its command queue
//  (can happen e.g. when the process reaches a waypoint and removes an entry from its target queue)
  if (check_command_bit(proc_index, COMREG_PROCESS_STATUS, COM_PROCESS_ST_UPDATE) == 1)
		{
   set_target_markers();
   command_bit_0(proc_index, COMREG_PROCESS_STATUS, COM_PROCESS_ST_UPDATE); // reset the bit
		}
	}
  else
   clear_details_console();


// Now manage all process' commands:
 run_commands();


} // end of main()

// This function is called when the user clicks the left mouse button somewhere on the screen.
// It is also called when the user releases the left mouse button after holding it for a very short time.
// x/y are mouse locations in world (in pixels)
// screen_x/y are mouse locations on screen (in pixels)
static void mouse_click(int x, int y, int screen_x, int screen_y)
{

  int mouse_status;

// Find out where the mouse is; if it's on the map, find out where:
  mouse_status = call(METH_INPUT, MS_OB_INPUT_MOUSE_MAP_XY);

// The MOUSE_STATUS_* macros are built-in macros
  if (mouse_status == MOUSE_STATUS_MAP)
  {
// If call(METH_INPUT, MS_OB_INPUT_MOUSE_MAP_XY) returned MOUSE_STATUS_MAP, method registers 1 and 2 will hold the world coordinates (in pixels)
//  of the point clicked on in the map:
    call(METH_VIEW, MS_OB_VIEW_FOCUS_XY, get(METH_INPUT, 1), get(METH_INPUT, 2)); // Relocate view to indicated point in map
    mouse_drag = MOUSE_DRAG_MAP;
    return;
  }

  if (mouse_status != MOUSE_STATUS_AVAILABLE)
   return;
    // MOUSE_STATUS_AVAILABLE means the mouse is on the screen somewhere (but not on the editor/template etc panel, if it's open)


// Now try to find whether user clicked on a process.
// Calling METH_POINT with MS_CLOB_POINT_FUZZY tries to find a process at or near the point x,y,
//  and returns its index or -1 if nothing found.
  proc_click = call(METH_POINT, MS_CLOB_POINT_FUZZY, x, y);

  if (proc_click != -1)
  {
      if (pressing_shift == 1)
        shift_click_process(proc_click);
         else
          select_a_process(proc_click);
  }
   else
   {
// User didn't click on a process, so deselect any current selection:
    deselect();
    call(METH_VIEW, MS_OB_VIEW_PROC_DATA, -1); // Cancels the process data display, if it's open
   }

}



// Call this function to select process p (and only process p)
static void select_a_process(int p)
{

 int current_status;
 int i;

 deselect();
 selected_proc [0] = p;
 selected_proc [1] = -1; // terminates the list

// Clear console 0
 call(METH_CONSOLE, MS_OB_CONSOLE_CLEAR, CONSOLE_COMMUNICATE);
 call(METH_CONSOLE, MS_OB_CONSOLE_CLEAR, CONSOLE_COMMAND);

 print_process_selected(selected_proc [0]);

// Now tell the process it's just been clicked on.
// If it wants to, the process can tell the user what kinds of orders it accepts, give a list of build orders, etc.
 command_bit_1(selected_proc [0], COMREG_CLIENT_STATUS, COM_CLIENT_ST_QUERY);

// Tell the process to let the user know what it's doing:
 command_bit_1(selected_proc [0], COMREG_CLIENT_STATUS, COM_CLIENT_ST_VERBOSE);

// If the display window is large enough, open the process data display:
 if (window_size_x > LARGE_WINDOW_SIZE)
  call(METH_VIEW, MS_OB_VIEW_PROC_DATA, selected_proc [0]);

// Set a marker (type 1 is an on-screen marker) that will last until removed:
 marker_index = call(METH_SELECT, MS_OB_SELECT_SET_MARKER, MARKER_CORNERS, -1, BCOL_YELLOW);
// Once a marker has been created by the set call, it must be placed somewhere.
// This call binds it to the selected process:
 call(METH_SELECT, MS_OB_SELECT_BIND_MARKER, marker_index, selected_proc [0]);
// Now set a similar marker for the map:
 marker_index = call(METH_SELECT, MS_OB_SELECT_SET_MARKER, MARKER_MAP_AROUND_1, -1, BCOL_YELLOW);
 call(METH_SELECT, MS_OB_SELECT_BIND_MARKER, marker_index, selected_proc [0]);

 set_target_markers();

 call(METH_VIEW, MS_OB_VIEW_SOUND, 0, 12, 100);


}

static void shift_click_process(int p)
{

 if (selected_proc [0] == -1)
 {
  select_a_process(p);
  return;
 }

 int i;

// First make sure the process is not already selected:
 for (i = 0; i < MULTISELECT_SIZE; i ++)
	{
		if (selected_proc [i] == -1)
		 break;
	 if (selected_proc[i] == p)
		{
			if (p == selected_proc [0]
		  && selected_proc [1] == -1)
			{
				deselect();
				return;
			}
   call(METH_SELECT, MS_OB_SELECT_UNBIND_PROCESS, selected_proc [i]);
			selected_proc[i] = -2;
   call(METH_VIEW, MS_OB_VIEW_SOUND, 0, 10, 100);
			return;
		}
	}

 for (i = 0; i < MULTISELECT_SIZE; i ++)
 {
  if (selected_proc [i] == -1)
  {
   selected_proc [i] = p;
   selected_proc [i+1] = -1; // selected_proc's size is MULTISELECT_SIZE+1
   print_process_selected(p);
   marker_index = call(METH_SELECT, MS_OB_SELECT_SET_MARKER, MARKER_CORNERS, -1, BCOL_YELLOW);
   call(METH_SELECT, MS_OB_SELECT_BIND_MARKER, marker_index, selected_proc [i]);
   marker_index = call(METH_SELECT, MS_OB_SELECT_SET_MARKER, MARKER_MAP_AROUND_1, -1, BCOL_YELLOW);
   call(METH_SELECT, MS_OB_SELECT_BIND_MARKER, marker_index, selected_proc [i]);
   call(METH_VIEW, MS_OB_VIEW_SOUND, 0, 12, 100);
   return;
  }
 }


}

static void print_process_selected(int p)
{

  print("\nProcess ", p, " selected.");

  if (p >= first_proc
		 && p < last_proc
			&&	check_command_bit(selected_proc [0], COMREG_PROCESS_DETAILS, COM_PROCESS_DET_COMMAND) == 0)
			 print("\nCommands not acknowledged.");

}

// Call this function when the user has dragged a selection box over an area of the screen.
// It selects friendly processes in the box.
static void mouse_box_select(int x1, int y1, int x2, int y2)
{

// ignore if box is very small (probably indicates user didn't mean to drag):
 if (abs(x1 - x2) + abs(y1 - y2) < 16)
		return;

 deselect();
 call(METH_VIEW, MS_OB_VIEW_PROC_DATA, -1); // Cancels the process data display

// This scan_bitmask tells the scanner to find only friendly processes.
 int scan_bitmask;
 scan_bitmask = 1<<operator_team;

 int found;

// Now call the operator's scanner to find processes in the selected area:
 found = call(METH_SCAN,
              MS_CLOB_SCAN_RECTANGLE_INDEX, // Scans a rectangle and puts process indices (rather than coordinates) in scan_result
              &scan_result [0], // Address of results array
              40, // Maximum number of processes to find. This is more than MULTISELECT_SIZE because it includes secondary processes.
              x1, // coordinates of rectangle
              y1,
              x2,
              y2,
              scan_bitmask); // The WANT bitmask. The NEED and REJECT bitmasks can be left at zero as they're not needed here.

 if (found == 0)
  return; // Didn't find anything

 if (found == 1)
	{
		select_a_process(scan_result [0]);
		return; // found exactly one process
	}

 int i;
 int select_index;
 i = 0;
 select_index = 0;

// Now go through and select the processes found by the scan.
 while (i < found)
 {
// But don't select any that have COM_PROCESS_DET_NO_MULTI set.
// This means that they are e.g. subprocesses that don't accept commands independently.
  if (check_command_bit(scan_result [i], COMREG_PROCESS_DETAILS, COM_PROCESS_DET_NO_MULTI) == 0)
  {
   selected_proc [select_index] = scan_result [i];
   marker_index = call(METH_SELECT, MS_OB_SELECT_SET_MARKER, MARKER_CORNERS, -1, BCOL_YELLOW);
   call(METH_SELECT, MS_OB_SELECT_BIND_MARKER, marker_index, selected_proc [select_index]);
   marker_index = call(METH_SELECT, MS_OB_SELECT_SET_MARKER, MARKER_MAP_AROUND_1, -1, BCOL_YELLOW);
   call(METH_SELECT, MS_OB_SELECT_BIND_MARKER, marker_index, selected_proc [select_index]);
   select_index++;
   if (select_index >= MULTISELECT_SIZE)
    break;
  }
  i++;
 };

 if (select_index == 1)
 {
  call(METH_CONSOLE, MS_OB_CONSOLE_CLEAR, CONSOLE_COMMUNICATE);
  print_process_selected(selected_proc [0]);
 }
  else
  {
   if (select_index > 1)
   {
    call(METH_CONSOLE, MS_OB_CONSOLE_CLEAR, CONSOLE_COMMUNICATE);
    print(select_index, " processes selected.");
   }
  }

// Terminate the list of selected processes:
 selected_proc [select_index] = -1;

 call(METH_VIEW, MS_OB_VIEW_SOUND, 0, 12, 100);

}

// Call this when:
//  - a single process is selected
//  - a new move or attack command is given
// But not when a new waypoint is added!
static void set_target_markers(void)
{

  if (selected_proc [0] < 0)
			return;

		int i;
		int dest_x, dest_y;
  int last_target_index;
  int last_x, last_y;
 	int target_index;
 	int com_status;
 	int line_colour;

		int waypoint;
		waypoint = 0;
		int comreg;
		comreg = COMREG_QUEUE;

  for (i = 0; i < WAYPOINTS; i ++)
		{
			call(METH_SELECT, MS_OB_SELECT_EXPIRE, target_marker [i]);
			target_marker [i] = -1;
			call(METH_SELECT, MS_OB_SELECT_EXPIRE, target_map_marker [i]);
			target_map_marker [i] = -1;
			call(METH_SELECT, MS_OB_SELECT_EXPIRE, target_line_marker [i]);
			target_line_marker [i] = -1;
		}

  com_status = check_command(selected_proc [0], COMREG_QUEUE);
  last_target_index = selected_proc [0];

  while(com_status == COMMAND_MOVE
					|| com_status == COMMAND_ATTACK
					|| com_status == COMMAND_A_MOVE)
		{

   if (com_status == COMMAND_MOVE
			 || com_status == COMMAND_A_MOVE)
			{
     dest_x = check_command(selected_proc [0], comreg + 1);
     dest_y = check_command(selected_proc [0], comreg + 2);

     target_marker [waypoint] = call(METH_SELECT, MS_OB_SELECT_SET_MARKER, MARKER_SHARP, -1, BCOL_GREEN);
// Once a marker has been created by the set call, it must be placed somewhere.
// This call places it (or does nothing if the set call failed):
     call(METH_SELECT, MS_OB_SELECT_PLACE_MARKER, target_marker [waypoint], dest_x, dest_y);

// Now create a map marker:
     target_map_marker [waypoint] = call(METH_SELECT, MS_OB_SELECT_SET_MARKER, MARKER_MAP_AROUND_2, -1, BCOL_GREY);
     call(METH_SELECT, MS_OB_SELECT_PLACE_MARKER, target_map_marker [waypoint], dest_x, dest_y);

     line_colour = BCOL_GREY;

     if (com_status == COMMAND_A_MOVE)
						line_colour = BCOL_ORANGE;

// And a map line marker:
     target_line_marker [waypoint] = call(METH_SELECT, MS_OB_SELECT_SET_MARKER, MARKER_MAP_LINE, -1, line_colour);
//     call(METH_SELECT, MS_OB_SELECT_PLACE_MARKER, marker_index, query_x(selected_proc [0]), query_y(selected_proc [0]));
     if (last_target_index != -1)
      call(METH_SELECT, MS_OB_SELECT_BIND_MARKER, target_line_marker [waypoint], last_target_index);
       else
        call(METH_SELECT, MS_OB_SELECT_PLACE_MARKER, target_line_marker [waypoint], last_x, last_y);
     call(METH_SELECT, MS_OB_SELECT_PLACE_MARKER2, target_line_marker [waypoint], dest_x, dest_y);

     last_target_index = -1; // indicates this command ends at a location, not a process
     last_x = dest_x;
     last_y = dest_y;
			}
			 else
				{
					if (com_status == COMMAND_ATTACK)
					{

	     target_index = check_command(selected_proc [0], comreg + 3); // Should be index of target. It's set by operator when attack command given.
// Verify target_index:
      if (target_index < 0
			    || query_hp(target_index) == -2)
       return; // should probably do something more intelligent here

      if (target_marker [waypoint] != -1)
				   call(METH_SELECT, MS_OB_SELECT_EXPIRE, target_marker [waypoint]);
      target_marker [waypoint] = call(METH_SELECT, MS_OB_SELECT_SET_MARKER, MARKER_BASIC, -1, BCOL_RED);
      call(METH_SELECT, MS_OB_SELECT_BIND_MARKER, target_marker [waypoint], target_index);

// Now create a map marker:
      if (target_map_marker [waypoint] != -1)
				   call(METH_SELECT, MS_OB_SELECT_EXPIRE, target_map_marker [waypoint]);
      target_map_marker [waypoint] = call(METH_SELECT, MS_OB_SELECT_SET_MARKER, MARKER_MAP_AROUND_2, -1, BCOL_RED);
      call(METH_SELECT, MS_OB_SELECT_BIND_MARKER, target_map_marker [waypoint], target_index);

      if (target_line_marker [waypoint] != -1)
				   call(METH_SELECT, MS_OB_SELECT_EXPIRE, target_line_marker [waypoint]);
      target_line_marker [waypoint] = call(METH_SELECT, MS_OB_SELECT_SET_MARKER, MARKER_MAP_LINE, -1, BCOL_RED);
// lines can have a binding for each end:
      if (last_target_index != -1)
       call(METH_SELECT, MS_OB_SELECT_BIND_MARKER, target_line_marker [waypoint], last_target_index);
        else
//         call(METH_SELECT, MS_OB_SELECT_PLACE_MARKER, target_line_marker [waypoint],
//			           check_command(selected_proc [0], comreg + 1), check_command(selected_proc [0], comreg + 2));
         call(METH_SELECT, MS_OB_SELECT_PLACE_MARKER, target_line_marker [waypoint], last_x, last_y);

      call(METH_SELECT, MS_OB_SELECT_BIND_MARKER2, target_line_marker [waypoint], target_index);

      last_target_index = target_index;
//      last_x = dest_x;
//      last_y = dest_y;

					}
				}

			waypoint++;
			comreg += COMMAND_QUEUE_SIZE;
			com_status = check_command(selected_proc [0], comreg);
		};

}


// Attempts to give a move command to all selected processes.
// If waypoint==1, will try to add a waypoint instead of replacing the existing comment:
static void add_destination(int x, int y)
{

	int i, j;
	int p;
	int com_status, com_type;
	i = 0;

	if (x < world_x_min)
		x = world_x_min;
	if (y < world_y_min)
		y = world_y_min;
	if (x > world_x_max)
		x = world_x_max;
	if (y > world_y_max)
		y = world_y_max;

	     while (selected_proc [i] != -1) // -1 is list terminator
      {
      	p = selected_proc [i];
       if (p != -2) // -2 means that the process that was selected has been destroyed or deselected.
       {
       	if (pressing_ctrl == 0)
								 give_new_command(p, COMMAND_MOVE, x, y, 0);
								  else
								   give_new_command(p, COMMAND_A_MOVE, x, y, 0);

       }
       if (i == 0)
							{
        set_target_markers();
							}
       i ++;
      }; // back through the selected_proc loop

}


static void add_attack_target(int target_index)
{

	int i;
	i = 0;
	int proc_index;
	int tx, ty;
	tx = query_x(target_index);
	ty = query_y(target_index);

// Here we either:
//  - replace a move command with an attack command; or
//  - add an attack waypoint to the end of a series of move waypoints.

         while (selected_proc [i] != -1) // -1 is the end of the list.
         {
         	proc_index = selected_proc [i];
          if (proc_index == -2) // Means that process has been destroyed or deselected
          {
           i++;
           continue;
          }
          give_new_command(proc_index, COMMAND_ATTACK, tx, ty, target_index);
          if (i == 0)
							   {
           show_multiselect_target = 1; // show_multiselect_target indicates that a group has just been given a command that should be indicated.
           set_target_markers();
							   }
          i++;
         }; // end selected_proc loop

//          print("Attack command given: target ", proc_click, " location ", mouse_x, ", ", mouse_y, ".");

} // end add_attack_target


static void give_new_command(int p, int com1, int com2, int com3, int com4)
{

   if (pressing_ctrl == 1)
				command_bit_1(p, COMREG_CLIENT_STATUS, COM_CLIENT_ST_CTRL); // built-in function: sets bit COM_CLIENT_ST_CTRL in COMREG_CLIENT_STATUS

   if (pressing_shift == 1)
			{
				command_bit_1(p, COMREG_CLIENT_STATUS, COM_CLIENT_ST_SHIFTED);
				add_command_to_queue(p, com1, com2, com3, com4);
				return;
			}

   command(p, COMREG_QUEUE, com1);
   command(p, COMREG_QUEUE+1, com2);
   command(p, COMREG_QUEUE+2, com3);
   command(p, COMREG_QUEUE+3, com4);
   command(p, COMREG_QUEUE+4, COMMAND_NONE); // terminates the queue
   command_bit_1(p, COMREG_CLIENT_STATUS, COM_CLIENT_ST_NEW);

   call(METH_VIEW, MS_OB_VIEW_SOUND, 0, 12, 100); // 0 is sample (currently only 0 is available), 12 is tone (on chromatic scale), 100 is volume (1-100)

}

// call this to add a command to the end of the queue
// (although it will put the command at the front of the queue if the queue is empty)
static void add_command_to_queue(int p, int com1, int com2, int com3, int com4)
{
	int c;
	c = COMREG_QUEUE;

	if (check_command(p, COMREG_QUEUE) == COMMAND_NONE)
	{
// if no current command, need to tell process that there's a new command:
		command_bit_1(p, COMREG_CLIENT_STATUS, COM_CLIENT_ST_NEW);
	}

// Now find an empty space in the queue:
	while (c < COMREG_END)
	{
  if (check_command(p, c) == COMMAND_NONE)
		{
			command(p, c, com1);
			command(p, c+1, com2);
			command(p, c+2, com3);
			command(p, c+3, com4);
   call(METH_VIEW, MS_OB_VIEW_SOUND, 0, 12, 100); // 0 is sample (currently only 0 is available), 12 is tone (on chromatic scale), 100 is volume (1-100)
			return;
		}
		c += COMMAND_QUEUE_SIZE;
	};

}


// This function checks each friendly process and manages its commands.
// For example, a process that is attacking another process may need to have its target destination commands updated.
static void run_commands(void)
{

 int i, j;
 int proc_index;
 int target_index;
 int execution_count;
 int comreg_base, command_type;

 for (i = 0; i < team_size; i ++)
 {
  proc_index = i + first_proc;

// Here we need to query the proc to see if it exists - it may just have been destroyed.
// The query_* functions are built-in functions to simplify calls to the QUERY method.

  if (query_hp(proc_index) == -2) // query_hp() returns -2 if the process was recently destroyed.
  {
// May have to remove it from selected_proc array:
// First check whether it was selected_proc [0], which case we either remove selection
//  or find a new process to be selected_proc [0]:
   if (proc_index == selected_proc [0])
			{
 					j = 1;
// Need to find another process to replace selected_proc [0]
//  Or deselect all if none remain selected.
 					while(selected_proc [j] == -2)
						{
							j ++;
						};
						if (selected_proc [j] == -1)
						{
					  deselect();	// None remain selected
						}
						 else
							{
//								print("\nReplacing: ",selected_proc [0]," destroyed, ",selected_proc [j]," (",j,")");
								selected_proc [0] = selected_proc [j];
								selected_proc [j] = -2;
				    set_target_markers(); // set waypoints using new selected_proc [0] as a basis
							}
			}
   j = 0;
   while (selected_proc [j] != -1)
   {
    if (selected_proc [j] == proc_index)
    {
     selected_proc [j] = -2; // indicates a destroyed process (but doesn't terminate the list)
     break;
    }
    j++;
   };
   continue;
  }

// Only update the process if it is just about to execute:
//  (this will also catch processes that do not exist and have not been recently destroyed)
  if (query_ex_time(proc_index) != 1)
   continue;

// Now go through and check each process's command queue to:
//  - update coordinates of attack target
//  - remove attack targets that no longer exist
  comreg_base = COMREG_QUEUE;
  command_type = check_command(proc_index, COMREG_QUEUE);
  while(command_type != COMMAND_NONE)
		{
			if (command_type == COMMAND_ATTACK) // currently COMMAND_ATTACK is the only one that uses process indices
			{
    target_index = check_command(proc_index, comreg_base + 3); // this is index of target. It's set by operator when attack command given.
// Verify target_index:
    if (target_index < 0)
     goto next_loop;
// Check to see whether the target still exists:
    if (query_hp(target_index) == -2)
    {
// Target has been destroyed!
     command(proc_index, comreg_base, COMMAND_EMPTY); // finish the current command and go to the next queued command (or idle if none)
     if (comreg_base == COMREG_QUEUE) // if this is the start of the queue, tell process its commands have been updated
      command_bit_1(proc_index, COMREG_CLIENT_STATUS, COM_CLIENT_ST_NEW);
     goto next_loop;
    }
// Target exists, so update the process' target coordinates with its new position:
    command(proc_index, comreg_base + 1, query_x(target_index));
    command(proc_index, comreg_base + 2, query_y(target_index));
			} // end if (command_type == COMMAND_ATTACK)
next_loop: // continues through while(command_type != COMMAND_NONE)
		 comreg_base += COMMAND_QUEUE_SIZE;
   command_type = check_command(proc_index, comreg_base);
// don't need to bounds-check comreg_base as check_command with an invalid command index just returns 0 (== COMMAND_NONE)
		} // end while (command_type != COMMAND_NONE) loop
 } // end for i loop through all friendly processes

}

// Call this function anytime at least one process is selected and needs to stop being selected.
// Doesn't matter if it's called unnecessarily.
static void deselect(void)
{
 int i;

 call(METH_CONSOLE, MS_OB_CONSOLE_CLEAR, CONSOLE_COMMAND);

 selected_proc [0] = -1;
 show_multiselect_target = 0;
 for (i = 0; i < WAYPOINTS; i ++)
	{
  target_marker [i] = -1;
  target_map_marker [i] = -1;
  target_line_marker [i] = -1;
	}
	current_waypoints = 0;
 call(METH_SELECT, MS_OB_SELECT_EXPIRE_ALL); // causes all markers to expire

// the operator assumes that if the verbose flag was set, the process will clear it itself

}


static void clear_details_console(void)
{
  call(METH_CONSOLE, MS_OB_CONSOLE_SIZE, CONSOLE_DETAILS, DETAILS_CONSOLE_WIDTH, 1);
  call(METH_CONSOLE, MS_OB_CONSOLE_CLEAR, CONSOLE_DETAILS);
  call(METH_CONSOLE, MS_OB_CONSOLE_OUT, CONSOLE_DETAILS);
  print("\n(no details)");
  call(METH_CONSOLE, MS_OB_CONSOLE_OUT, CONSOLE_COMMUNICATE);

}


static void run_details_console(void)
{

 call(METH_CONSOLE, MS_OB_CONSOLE_OUT, CONSOLE_DETAILS);

 if (selected_proc [0] == -1
  || selected_proc [1] != -1)
 {
  call(METH_CONSOLE, MS_OB_CONSOLE_CLEAR, CONSOLE_DETAILS);
  call(METH_CONSOLE, MS_OB_CONSOLE_LINES, CONSOLE_DETAILS, 1); // makes the console small
  print("\n(no details)");
  call(METH_CONSOLE, MS_OB_CONSOLE_OUT, CONSOLE_COMMUNICATE);
  return;
 }

 int detail_lines;
 detail_lines = 0;

 call(METH_CONSOLE, MS_OB_CONSOLE_CLEAR, CONSOLE_DETAILS);

 print("\nprocess ", selected_proc [0]);
 detail_lines ++;

// if (check_command_bit(selected_proc [0], COMREG_PROCESS_DETAILS, COM_PROCESS_DET_HP) == 1) // always show at least hp
 {
  print("\nhp  ", query_hp(selected_proc [0]), "(", query_hp_max(selected_proc [0]), ")");
  detail_lines ++;
 }
 if (check_command_bit(selected_proc [0], COMREG_PROCESS_DETAILS, COM_PROCESS_DET_IRPT) == 1)
 {
  print("\nirpt  ", query_irpt(selected_proc [0]), "(", query_irpt_max(selected_proc [0]), ")");
  detail_lines ++;
 }
 if (check_command_bit(selected_proc [0], COMREG_PROCESS_DETAILS, COM_PROCESS_DET_DATA) == 1)
 {
  print("\ndata  ", query_data(selected_proc [0]), "(", query_data_max(selected_proc [0]), ")");
  detail_lines ++;
 }

 if (check_command_bit(selected_proc [0], COMREG_PROCESS_DETAILS, COM_PROCESS_DET_ALLOC_EFF) != 0
		|| query_method_find(selected_proc [0], MT_PR_ALLOCATE, 0) != -1)
 {
  print("\neff  ", query_efficiency(selected_proc [0]));
  detail_lines ++;
  marker_index = call(METH_SELECT, MS_OB_SELECT_SET_MARKER, MARKER_MAP_BOX, 2, BCOL_BLUE); // timeout 2 = lasts 1 cycle
  call(METH_SELECT, MS_OB_SELECT_PLACE_MARKER, marker_index,
							query_x(selected_proc [0]) - ALLOCATE_INTERFERENCE_RANGE, query_y(selected_proc [0]) - ALLOCATE_INTERFERENCE_RANGE);
  call(METH_SELECT, MS_OB_SELECT_PLACE_MARKER2, marker_index,
							query_x(selected_proc [0]) + ALLOCATE_INTERFERENCE_RANGE, query_y(selected_proc [0]) + ALLOCATE_INTERFERENCE_RANGE);
 }

 int method_index;

// Always show strength of virtual method, if present:
 method_index = query_method_find(selected_proc [0], MT_PR_VIRTUAL, 0);
 if (method_index != -1)
 {
  print("\nvir  ", query_mbank(selected_proc [0], (method_index * 4) + 2),
								"(",query_mbank(selected_proc [0], (method_index * 4) + 3),")");
  detail_lines ++;
 }

 call(METH_CONSOLE, MS_OB_CONSOLE_LINES, CONSOLE_DETAILS, detail_lines); // makes the console small

 call(METH_CONSOLE, MS_OB_CONSOLE_OUT, CONSOLE_COMMUNICATE);

}

// Call this early on so that the shift values can be used.
static void keyboard_input(void)
{

	pressing_shift = 0;

	if (call(METH_INPUT, MS_OB_INPUT_KEY, KEY_LSHIFT) > 0
		|| call(METH_INPUT, MS_OB_INPUT_KEY, KEY_RSHIFT) > 0)
			pressing_shift = 1;

	pressing_ctrl = 0;

	if (call(METH_INPUT, MS_OB_INPUT_KEY, KEY_LCTRL) > 0
		|| call(METH_INPUT, MS_OB_INPUT_KEY, KEY_RCTRL) > 0)
			pressing_ctrl = 1;

	// now manage fast-forward and pausing

 int step_forward;

 if (step_forward == 1)
 {
  call(METH_CONTROL, MS_OB_CONTROL_PAUSE_SET, 1); // pause
  step_forward = 0;
 }

 int any_key;

 any_key = call(METH_INPUT, MS_OB_INPUT_ANY_KEY);

 if (any_key == -1)
		return; // no keys being pressed; return

// after the call just above, the input register's method 0 holds the
//  BUTTON_* value of the key that is being pressed.
	if (get(METH_INPUT, 0) != BUTTON_JUST_PRESSED)
		return;

// it would be nice to use a switch here, but it would take up too much space.

// KEY_F starts smooth fast forward:
 if (any_key == KEY_F)
	{
  if (call(METH_CONTROL, MS_OB_CONTROL_FF) == 0) // if not FF
   call(METH_CONTROL, MS_OB_CONTROL_FF_SET, 1, 2); // smooth FF
    else
     call(METH_CONTROL, MS_OB_CONTROL_FF_SET, 0); // unFF
  return;
	}

// KEY_G starts very fast forward:
 if (any_key == KEY_G)
	{
  if (call(METH_CONTROL, MS_OB_CONTROL_FF) == 0) // if not FF
   call(METH_CONTROL, MS_OB_CONTROL_FF_SET, 1, 3); // skippy FF
    else
     call(METH_CONTROL, MS_OB_CONTROL_FF_SET, 0); // unFF
  return;
	}

 if (any_key == KEY_P)
 {
  if (call(METH_CONTROL, MS_OB_CONTROL_PAUSED) == 0) // if not paused
   call(METH_CONTROL, MS_OB_CONTROL_PAUSE_SET, 1); // pause
    else
     call(METH_CONTROL, MS_OB_CONTROL_PAUSE_SET, 0); // unpause
  call(METH_VIEW, MS_OB_VIEW_SOUND, 0, 10, 50); // 0 is sample (currently only 0 is available), 12 is tone (on chromatic scale), last number is volume (1-100)
  return;
 }

 if (any_key == KEY_A)
 {
  call(METH_CONTROL, MS_OB_CONTROL_PAUSE_SET, 0); // unpause
  step_forward = 1;
  return;
 }

 if (any_key == KEY_SLASH
		&& pressing_shift == 1)
 {
		print("\n\n\nControls:");
		print("\nL-click: select       L-click+shift: add/remove selection");
		print("\nR-click: move/attack  R-click+shift: set waypoint");
		print("\nR-click+ctrl: attack-move or special command");
		print("\nF: fast-forward (x4)  G: fast-forward (x8)");
		print("\nP: pause              A: pause advance");
		print("\n?: help");
  call(METH_VIEW, MS_OB_VIEW_SOUND, 0, 12, 50); // 0 is sample (currently only 0 is available), 12 is tone (on chromatic scale), last number is volume (1-100)
  return;
 }

// That's all of the valid keypresses.

 if (any_key < KEY_LSHIFT)
	{
		print("\nPress ? for help.");
  call(METH_VIEW, MS_OB_VIEW_SOUND, 0, 10, 50); // 0 is sample (currently only 0 is available), 12 is tone (on chromatic scale), last number is volume (1-100)
		return;
	}


}
