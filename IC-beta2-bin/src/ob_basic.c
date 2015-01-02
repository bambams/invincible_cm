/*

This is a basic observer program, based on op_basic.c.

*/

// This is the process's interface definition. It determines the nature of the process and
// the methods it has available. It must come before any code.
interface
{
 PROGRAM_TYPE_OBSERVER, // program's type
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
int first_proc; // index of first process that belongs to this player
int last_proc; // index of last process that belongs to this player
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

#subdefine MULTISELECT_SIZE 20
int selected_proc; // List of the indices of selected processes. Terminated by -1.
int marker_index; // used from time to time to hold marker indices

enum
{
MOUSE_DRAG_NONE, // user is not dragging the mouse
MOUSE_DRAG_MAP // user is dragging the mouse around the map
};

// MAX_PROCESSES is the maximum number of processes on a team (this is a guess; may need to update)
#subdefine MAX_PROCESSES 200

// If the display is fewer than LARGE_WINDOW_SIZE pixels wide, some elements are not displayed:
#subdefine LARGE_WINDOW_SIZE 600

#define CONSOLE_COMMUNICATE 0
// This console is used to communicate with the user. Its index should be 0 so all messages are printed here by default
#define CONSOLE_DETAILS 1
// This console is used to display details of selected processes and similar
#subdefine DETAILS_CONSOLE_WIDTH 20
// Size of details console in characters (each is 6 pixels wide). Actual width of console window
//  is (DETAILS_CONSOLE_WIDTH * 6) + 35
int console_title [8] = {"Console"};
int details_title [8] = {"Details"};

// Function declarations
// Note that "static" in a function declaration means something different from normal C.
//  - It means that all variables in the function are static, which is recommended as
//    static variables are much more efficient than automatic variables (which you
//    shouldn't use without a good reason)
static void mouse_click(int x, int y, int screen_x, int screen_y);
static void select_a_process(int p);
static void deselect(void);
static void clear_details_console(void);
static void run_details_console(void);
static void print_process_selected(int p);
static void keyboard_input(void);

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

  team_size = world_processes_each();

  call(METH_VIEW, MS_OB_VIEW_DISPLAY_SIZE); // This call leaves the screen size in method registers 1 and 2
  display_size_reset = 1; // Tells the code below that the display's size has been set.
  window_size_x = get(METH_VIEW, 1); // used later
  window_size_y = get(METH_VIEW, 2);
  selected_proc = -1;
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

// Set up the special information console:
  call(METH_CONSOLE, MS_OB_CONSOLE_STYLE, CONSOLE_DETAILS, CONSOLE_STYLE_BOX_UP);
  call(METH_CONSOLE, MS_OB_CONSOLE_SIZE, CONSOLE_DETAILS, DETAILS_CONSOLE_WIDTH, 5);
  call(METH_CONSOLE, MS_OB_CONSOLE_TITLE, CONSOLE_DETAILS, &details_title [0]);

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

// Since this is an operator program, we need to check user input:

 mouse_status = call(METH_INPUT, MS_OB_INPUT_MOUSE_XY);
// This call returns the status of the mouse pointer as one of the built-in MOUSE_STATUS_? enums.
// If the mouse is on the game display, it also sets method registers 1 and 2 to the coordinates
//  of the mouse pointer in the game world (these are absolute coordinates, not on-screen)

 if (mouse_status == MOUSE_STATUS_AVAILABLE // Mouse is on the game screen and not on a console, the map or the process data box
  || mouse_status == MOUSE_STATUS_MAP // Mouse is on the map
  || mouse_status == MOUSE_STATUS_PROCESS)
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

  }

  if (mouse_l_status == BUTTON_JUST_PRESSED)
  {
// If the left mouse button was just pressed, set up some global variables to keep track of where it is and how long it's held for:
   mouse_drag_time = 0;
   mouse_drag_x = mouse_x;
   mouse_drag_y = mouse_y;
   mouse_click(mouse_x, mouse_y, mouse_screen_x, mouse_screen_y);
  }

  if (mouse_l_status == BUTTON_HELD)
		{
   if (mouse_drag == MOUSE_DRAG_MAP)
			{
    call(METH_INPUT, MS_OB_INPUT_MOUSE_MAP_XY);
    call(METH_VIEW, MS_OB_VIEW_FOCUS_XY, get(METH_INPUT, 1), get(METH_INPUT, 2)); // Relocate view to indicated point in map
			}
		}

// If the mouse is near the edge of the screen, we scroll the screen:

// These #subdefines determine the size of the edge area and the speed of scrolling:
#subdefine MOUSE_SCROLL_REGION 40

  int scroll_speed_factor = 1;

  if (fast_forwarding)
   scroll_speed_factor = 4; // slow down scroll if fast-forwarding

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
  select_a_process(console_event_val2);
  call(METH_VIEW, MS_OB_VIEW_FOCUS_PROC, console_event_val2); // Sets camera to a process' location. Does nothing if console_event_val2 is -1
 }

// Now go through all selected_procs and check for destroyed processes:
 i = 0;
 int proc_index;
 int com_status, com_type;

 if (selected_proc != -1)
	{
// display relevant information in the details console
   run_details_console();
	}
  else
   clear_details_console();


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
 selected_proc = p;

// Clear console 0
 call(METH_CONSOLE, MS_OB_CONSOLE_CLEAR, CONSOLE_COMMUNICATE);

 print_process_selected(selected_proc);

// If the display window is large enough, open the process data display:
 if (window_size_x > LARGE_WINDOW_SIZE)
  call(METH_VIEW, MS_OB_VIEW_PROC_DATA, selected_proc);

// Set a marker (type 1 is an on-screen marker) that will last until removed:
 marker_index = call(METH_SELECT, MS_OB_SELECT_SET_MARKER, MARKER_CORNERS, -1, BCOL_YELLOW);
// Once a marker has been created by the set call, it must be placed somewhere.
// This call binds it to the selected process:
 call(METH_SELECT, MS_OB_SELECT_BIND_MARKER, marker_index, selected_proc);
// Now set a similar marker for the map:
 marker_index = call(METH_SELECT, MS_OB_SELECT_SET_MARKER, MARKER_MAP_AROUND_1, -1, BCOL_YELLOW);
 call(METH_SELECT, MS_OB_SELECT_BIND_MARKER, marker_index, selected_proc);

 call(METH_VIEW, MS_OB_VIEW_SOUND, 0, 12, 100);

}

static void print_process_selected(int p)
{

  print("\nProcess ", p, " selected.");


}


// Call this function anytime at least one process is selected and needs to stop being selected.
// Doesn't matter if it's called unnecessarily.
static void deselect(void)
{
 int i;

 selected_proc = -1;
 call(METH_SELECT, MS_OB_SELECT_EXPIRE_ALL); // causes all markers to expire

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

 if (selected_proc == -1)
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

 print("\nprocess ", selected_proc);
 detail_lines ++;

// if (check_command_bit(selected_proc, COMREG_PROCESS_DETAILS, COM_PROCESS_DET_HP) == 1) // always show at least hp
 {
  print("\nhp  ", query_hp(selected_proc), "(", query_hp_max(selected_proc), ")");
  detail_lines ++;
 }
 if (check_command_bit(selected_proc, COMREG_PROCESS_DETAILS, COM_PROCESS_DET_IRPT) == 1)
 {
  print("\nirpt  ", query_irpt(selected_proc), "(", query_irpt_max(selected_proc), ")");
  detail_lines ++;
 }
 if (check_command_bit(selected_proc, COMREG_PROCESS_DETAILS, COM_PROCESS_DET_DATA) == 1)
 {
  print("\ndata  ", query_data(selected_proc), "(", query_data_max(selected_proc), ")");
  detail_lines ++;
 }

 if (check_command_bit(selected_proc, COMREG_PROCESS_DETAILS, COM_PROCESS_DET_ALLOC_EFF) != 0
		|| query_method_find(selected_proc, MT_PR_ALLOCATE, 0) != -1)
 {
  print("\neff  ", query_efficiency(selected_proc));
  detail_lines ++;
  marker_index = call(METH_SELECT, MS_OB_SELECT_SET_MARKER, MARKER_MAP_BOX, 2, BCOL_BLUE); // timeout 2 = lasts 1 cycle
  call(METH_SELECT, MS_OB_SELECT_PLACE_MARKER, marker_index,
							query_x(selected_proc) - ALLOCATE_INTERFERENCE_RANGE, query_y(selected_proc) - ALLOCATE_INTERFERENCE_RANGE);
  call(METH_SELECT, MS_OB_SELECT_PLACE_MARKER2, marker_index,
							query_x(selected_proc) + ALLOCATE_INTERFERENCE_RANGE, query_y(selected_proc) + ALLOCATE_INTERFERENCE_RANGE);
 }

 int method_index;

// Always show strength of virtual method, if present:
 method_index = query_method_find(selected_proc, MT_PR_VIRTUAL, 0);
 if (method_index != -1)
 {
  print("\nvir  ", query_mbank(selected_proc, (method_index * 4) + 2),
								"(",query_mbank(selected_proc, (method_index * 4) + 3),")");
  detail_lines ++;
 }

 call(METH_CONSOLE, MS_OB_CONSOLE_LINES, CONSOLE_DETAILS, detail_lines); // makes the console small

 call(METH_CONSOLE, MS_OB_CONSOLE_OUT, CONSOLE_COMMUNICATE);

}

// Call this early on so that the shift values can be used.
static void keyboard_input(void)
{


	if (call(METH_INPUT, MS_OB_INPUT_KEY, KEY_LSHIFT) > 0
		|| call(METH_INPUT, MS_OB_INPUT_KEY, KEY_RSHIFT) > 0)
			pressing_shift = 1; // could do the same thing for control and alt, but these are not currently used.
			 else
			  pressing_shift = 0;

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

// KEY_G starts skippy fast forward:
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
  call(METH_CONSOLE, MS_OB_CONSOLE_CLEAR, CONSOLE_COMMUNICATE);
		print("\nControls:");
		print("\nL-click: select");
		print("\nF: fast-forward (x4)  G: fast-forward (x8)");
		print("\nP: pause              A: pause advance");
		print("\n?: help");
  return;
 }

// That's all of the valid keypresses.

 if (any_key < KEY_LSHIFT)
	{
		print("\nPress ? for help.");
		return;
	}


}
