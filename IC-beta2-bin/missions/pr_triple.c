

/*

pr_triple.c

A multi-part process that is used in a mission.
Needs appropriate commands to operate properly (but doesn't work with an operator).

*/

#ifndef STANDARD_COMMANDS
#include "stand_coms.c"
#endif






#ifndef STANDARD_COMMANDS
#include "stand_coms.c"
#endif

// This is the process's interface definition. It determines the nature of the process and
// the methods it has available. It must come before any code.
interface
{

 PROGRAM_TYPE_PROCESS, SHAPE_6POINTY, 3, 0, // program type, shape, size (from 0-3), base_vertex
 {
  METH_MOVE1: {MT_PR_MOVE, 5, ANGLE_1 - data(DATA_SHAPE_VERTEX_ANGLE, SHAPE_6POINTY, 5), 1}, // acceleration method on vertex 3, pointing backwards
  METH_MOVE2: {MT_PR_MOVE, 1, ANGLE_1 - data(DATA_SHAPE_VERTEX_ANGLE, SHAPE_6POINTY, 1), 1}, // acceleration method on vertex 1, pointing backwards
  METH_LINK_RIGHT: {MT_PR_LINK, 4},
  METH_LINK_LEFT: {MT_PR_LINK, 2},
  METH_LINK_BACK: {MT_PR_LINK, 0},
  METH_COM: {MT_PR_COMMAND}, // command method. allows process to communicate with operator/delegate
  METH_MATHS: {MT_PR_MATHS}, // maths method. allows trigonometry etc.
  METH_STD: {MT_PR_STD}, // information method. allows access to process's current properties (location etc)
  METH_IRPT: {MT_PR_IRPT, 0, 0, 3}, // generates irpt. functions automatically (although can be configured not to)
    // 2 = vertex, 0 = angle, 0 = power extension, 1 = speed extension, 0 = range extension
  METH_NEW_SUB: {MT_PR_NEW_SUB},
  {0}, // space for new method


 }

}


// Now we declare some global variables.
// Remember that these are initialised (to zero, or to a specified value) when the process is
//  created, but not each time it is run.

int initialised; // When this is zero, it tells the process that it has just been created and needs to initialise.

// Initialisation values - these are set once, when the process is first run
int team; // This is set to the process's team (i.e. which player it is controlled by)
int world_size_x, world_size_y; // size of the world (in pixels)
int pod_data_cost;

// Current state values - these are set each time the process runs
int angle; // Angle the process is pointing (in integer format, from 0 to 8192)
int x, y; // Location (in pixels)
int speed_x, speed_y; // Process' speed in pixels per 16 ticks
int spin; // Process' spin (i.e. which way it's turning, and how fast)

// Behaviour/mode values - these determine the process' current behaviour
int mode; // Behavioural mode of the process (see the MODE enums below)
int target_x, target_y; // Where the process is going
int attack_x, attack_y; // Location of target if process is attacking

// Scan values - used to scan for other nearby processes
int scan_result [32]; // actually only 4 values are used, but may want to extend later.
int scan_bitmask; // set during initialisation (can't set it at compile-time because the process' team is not known)

// Mode enum - these are for the process' mode value
enum
{
MODE_IDLE, // Process is sitting around, scanning for something to attack
MODE_MOVE, // Process is moving to a known location (doesn't run scan)
MODE_ATTACK, // Process has found its own target and is attacking it
MODE_ATTACK_COMMAND, // Process has been told to attack
MODE_A_MOVE_COMMAND, // Process has been told to attack-move (i.e. move but attack things along the way)
MODE_A_MOVE_ATTACKING, // Process is attacking something it found during attack-move
MODE_CONNECTED
};

// Function declarations
// Note that "static" in a function declaration means something different from normal C.
//  - It means that all variables in the function are static, which is recommended as
//    static variables are much more efficient than automatic variables (which you
//    shouldn't use without a good reason)
static void execute_command(void);
static void next_command(void);
static void process_action(void);
static void process_attack(int vertex_angle, int vertex_dist, int packet_speed);
static int lead_target_xy(int relative_x, int relative_y,
																							   int relative_speed_x, int relative_speed_y,
																			       int base_angle,
																			       int intercept_speed);
static void process_move(void);

process side_pod;

// Remember that main is called every time the program is run.
// Its correct type is static void main(void)
static void main(void)
{

// Time for some variables that are just used in main():
 int i, j;
 int build_result;

// First we get some basic information about the process' current state.
// The get_*() functions are built-in shortcuts that the compiler turns into calls
//  to the INFO method.
 angle = get_angle() + ANGLE_2;
 x = get_x();
 y = get_y();
 speed_x = get_speed_x();
 speed_y = get_speed_y();
 spin = get_spin();

// If this is the first time the process has been run, we also need to set up some
//  basic information about things like which player is controlling it.
 if (initialised == 0)
 {
  team = get_team(); // Which player is the process being controlled by? (0-3)
// Now let's tell the process to move away from its parent process (by moving 200 pixels forwards):
  set_command_bit_1(COMREG_PROCESS_DETAILS, COM_PROCESS_DET_COMMAND); // tells operator that this process accepts commands.
  set_command_bit_1(COMREG_PROCESS_DETAILS, COM_PROCESS_DET_HP); // tells operator what details to display
  set_command_bit_1(COMREG_PROCESS_DETAILS, COM_PROCESS_DET_IRPT); // tells operator what details to display
// Other stuff:
  world_size_x = get_world_x(); // This is the size of the entire playing area, in pixels
  world_size_y = get_world_y();
  scan_bitmask = 0b1111 ^ (1<<team); // scan_bitmask is used by the scanner to find processes that belong to a different team.
      // The four least significant bits of scan_bitmask tell the scanner to accept (1) or ignore (0) processes that belong to player 4, 3, 2 and 1.
      // 0b1111 is the binary form of 15. Here it means accept all teams.
      // ^ (1<<team) flips the bit corresponding to this process' team, so the scanner will ignore friendly processes.
      // For example, if this process belongs to player 2 the bits will be 1101.
  mode = MODE_CONNECTED; // Means that the process is still connected to the factory that built it.
// Now work out how much a pod costs to build:
  build_result = call(METH_NEW_SUB,
        MS_PR_NEW_BC_COST_DATA, // MB_PR_NEW_STATUS: MS_PR_NEW_BC_COST_DATA means get data cost.
        4, // MB_PR_NEW_VERTEX1: Vertex of this process that new process will be created at.
        3, // MB_PR_NEW_VERTEX2: Vertex of child process (-1 means use child process' base_vertex value)
        -1100, // MB_PR_NEW_ANGLE: Angle of child process
        process_start(side_pod), // MB_PR_NEW_START: start address of new process' code
        process_end(side_pod), // MB_PR_NEW_END: end address
        1, // MB_PR_NEW_LINK: 1 means processes will be connected (as long as they both have link methods on mutual vertex)
        0); // MB_PR_NEW_TEMPLATE: Source template index (not relevant here; only used when building from a template)
  pod_data_cost = get(METH_NEW_SUB, MB_PR_NEW_STATUS);
  initialised = 1; // Now it won't be initialised again
 }



 if (call(METH_LINK_LEFT, MS_PR_LINK_EXISTS) == 0 // left pod doesn't exist
  && get_data() >= pod_data_cost)
 {
 	 *(process_start(side_pod) + 3) = SHAPE_4IRREG_R;
 	 *(process_start(side_pod) + 7) = 3;
// Try to build the pod:
   build_result = call(METH_NEW_SUB,
        MS_PR_NEW_BC_BUILD, // MB_PR_NEW_STATUS: MS_PR_NEW_BC_BUILD means try to build from bcode
        2, // MB_PR_NEW_VERTEX1: Vertex of this process that new process will be created at.
        3, // MB_PR_NEW_VERTEX2: Vertex of child process (-1 means use child process' base_vertex value)
        -100, // MB_PR_NEW_ANGLE: Angle of child process
        process_start(side_pod), // MB_PR_NEW_START: start address of new process' code
        process_end(side_pod), // MB_PR_NEW_END: end address
        1, // MB_PR_NEW_LINK: 1 means processes will be connected (as long as they both have link methods on mutual vertex)
        0); // MB_PR_NEW_TEMPLATE: Source template index (not relevant here; only used when building from a template)
 }

 if (call(METH_LINK_RIGHT, MS_PR_LINK_EXISTS) == 0 // right pod doesn't exist
  && get_data() >= pod_data_cost)
 {
 	 *(process_start(side_pod) + 3) = SHAPE_4IRREG_L;
 	 *(process_start(side_pod) + 7) = 1;
// Try to build the pod:
   build_result = call(METH_NEW_SUB,
        MS_PR_NEW_BC_BUILD, // MB_PR_NEW_STATUS: MS_PR_NEW_BC_BUILD means try to build from bcode
        4, // MB_PR_NEW_VERTEX1: Vertex of this process that new process will be created at.
        1, // MB_PR_NEW_VERTEX2: Vertex of child process
        100, // MB_PR_NEW_ANGLE: Angle of child process
        process_start(side_pod), // MB_PR_NEW_START: start address of new process' code
        process_end(side_pod), // MB_PR_NEW_END: end address
        1, // MB_PR_NEW_LINK: 1 means processes will be connected (as long as they both have link methods on mutual vertex)
        0); // MB_PR_NEW_TEMPLATE: Source template index (not relevant here; only used when building from a template)
 }

// Client should set a bit of one of the command registers if it issues a new command:
 if (mode != MODE_CONNECTED
		&&	get_command_bit(COMREG_CLIENT_STATUS, COM_CLIENT_ST_NEW) == 1)
	{
  execute_command(); // executes the command currently at the start of the queue
  set_command_bit_0(COMREG_CLIENT_STATUS, COM_CLIENT_ST_NEW); // set the bit back to 0
	}

// Now do whatever the process is doing (move, attack etc)
	process_action();


}

// Call this to execute the command at the start of the queue
// It doesn't assume that COM_CLIENT_ST_NEW bit is 1, so it can be used e.g.
//  if the process is momentarily distracted and needs to restart executing its current command
static void execute_command(void)
{

// NOTE: this function is called recursively, but is static.
// Be careful about using local variables!

 int command_type;
 int more_commands;
 more_commands = 1;

 while (more_commands == 1)
	{
// Now we check the command registers to see whether the process' client program has given it a command:
  command_type = get_command(COMREG_QUEUE);
  more_commands = 0; // will be set to 1 if a command needs to run more commands

// Check for a command:
  switch(command_type)
  {

   case COMMAND_A_MOVE:
   case COMMAND_MOVE:
// Read the target coordinates, which the client will have put into other command registers:
    target_x = get_command(COMREG_QUEUE + 1);
    target_y = get_command(COMREG_QUEUE + 2);
    mode = MODE_MOVE;
    break;

   case COMMAND_ATTACK:
// Read the target coordinates, which the client will have put into other command registers:
    attack_x = get_command(COMREG_QUEUE + 1) - x;
    attack_y = get_command(COMREG_QUEUE + 2) - y;
    mode = MODE_ATTACK_COMMAND;
    break;

   case COMMAND_IDLE:
 			set_command(COMREG_QUEUE, COMMAND_NONE);
    set_command_bit_1(COMREG_PROCESS_STATUS, COM_PROCESS_ST_UPDATE);
// fall-through
   case COMMAND_NONE:
    mode = MODE_IDLE;
//    put(METH_MOVE1, MB_PR_MOVE_RATE, 0); // turn off the move methods
//    put(METH_MOVE2, MB_PR_MOVE_RATE, 0); // turn off the move methods
    break;

   case COMMAND_EMPTY: // stop current command and go to next command
				next_command();
				more_commands = 1;
    break;

  } // end switch(command_received)
	} // end while more_commands == 1

}

// Call this to delete the current command and pull the next command to the front of the queue.
static void next_command(void)
{
	int i;

	i = COMREG_QUEUE;

	while (i < COMREGS - COMMAND_QUEUE_SIZE + 1)
	{
		set_command(i, get_command(i + COMMAND_QUEUE_SIZE));
// it's okay to use get_command as a parameter to set_command because they use different methods (get uses PR_COMMAND, set uses PR_STD)
  i ++;
	}
// Note that the last run-through of this loop sets register 12 to the value of register 16
// Because register 16 doesn't exist, this will always be 0 and will terminate the queue with COMMAND_NONE (which is 0)

 set_command_bit_1(COMREG_PROCESS_STATUS, COM_PROCESS_ST_UPDATE);

}


static void process_action(void)
{

 switch(mode)
 {

  case MODE_CONNECTED:
// In CONNECTED mode, the process is still connected to the factory and absorbing data.
// It will disconnect when both pods have been built:
   if (call(METH_LINK_LEFT, MS_PR_LINK_EXISTS) == 1 // left pod exists
    && call(METH_LINK_RIGHT, MS_PR_LINK_EXISTS) == 1) // right pod exists
   {
    call(METH_LINK_BACK, MS_PR_LINK_DISCONNECT); // disconnects from factory
    mode = MODE_MOVE; // Tell the process to move forwards 200 pixels
    target_x = x + cos(angle, 200); // target_x and target_y are used in the movement code below
    target_y = y + sin(angle, 200); // cos and sin are built-in functions that call the MATHS method.
                                    // These calls multiply the result of cos/sin by 200 (to get the x/y components
                                    //  of a line extending 200 pixels to the front of the process)
   }
   break;


  case MODE_MOVE:
// In MOVE mode, the process moves towards target_x/y then, when near, it executes the next command (which may be COMMAND_NONE)
   if (abs(y - target_y) + abs(x - target_x) < 100) // abs is a built-in function that calls the MATHS method
   {
   	next_command();
   	execute_command();
    break;
			}
			 else
     process_move();
   break;

  case MODE_A_MOVE_COMMAND:
// A_MOVE mode is like MOVE move but the process attacks anything it finds along the way
   if (abs(y - target_y) + abs(x - target_x) < 100) // abs is a built-in function that calls the MATHS method
   {
   	next_command();
   	execute_command();
    break;
			}
			 else
				{
     process_move();
				}
			break;

 } // end of switch(mode)

}




static void process_move(void)
{


// At this point, all we have left to do is make the process move towards its target, which is at (target_x, target_y)

 int target_angle;
 int angle_diff;
 int turn_dir;
 int future_turn_dir;

// Work out the angle to the target:
 target_angle = atan2(target_y - y, target_x - x);
// Work out which way the process has to turn to get there:
 turn_dir = turn_direction(angle, target_angle); // turn_direction() is a built-in MATHS method function.
    // It returns the direction of the shortest turning distance between angle and target_angle, taking bounds into account.
    // Return values are -1, 0 (if angles are equal) or 1
 angle_diff = angle_difference(angle, target_angle);
// Note that these maths functions (atan2, turn_direction, angle_difference) cannot take themselves or other maths built-in functions as arguments
//  (as this messes up the method bank registers)

// Now, because the process will spin more or less freely, we can't just tell it to rotate towards the target angle.
// Instead, we need to work out whether its current spin will result in it overshooting its target angle:

// future_turn_dir is the direction it would have to rotate to point towards target_angle if it turned in turn_dir,
//  taking account of its current spin:
 future_turn_dir = turn_direction(angle + (turn_dir * 1000) + (spin * 8), target_angle); // (100 and 8 are approximation factors)
// If future_turn_dir is different, that means we will overshoot the target. So don't turn:

 if (turn_dir != future_turn_dir)
 {
  turn_dir = 0;
 }

// Now we know which way we're turning, we need to tell the move methods (i.e. engines) what to do.

// METH_MOVE1 is the right-hand engine, which will turn the process anti-clockwise if run alone.
 put(METH_MOVE1, MB_PR_MOVE_RATE, 40); // This is its power level (40 is very high, so the method will just run at maximum capacity, which is about 5)
 put(METH_MOVE1, MB_PR_MOVE_COUNTER, 16); // Will run for 16 ticks
 if (turn_dir > 0) // If turn_dir > 0, the process is trying to rotate clockwise. So we suppress this engine for a while:
  put(METH_MOVE1, MB_PR_MOVE_DELAY, angle_diff / 40); // DELAY waits for a while before the engine starts. 30 is a tuning factor.
   else
    put(METH_MOVE1, MB_PR_MOVE_DELAY, 0); // No delay - process is either turning anti-clockwise, or firing both engines to move forwards.

// Now we do the same for the left-hand engine:
 put(METH_MOVE2, MB_PR_MOVE_RATE, 40);
 put(METH_MOVE2, MB_PR_MOVE_COUNTER, 16);
 if (turn_dir < 0)
  put(METH_MOVE2, MB_PR_MOVE_DELAY, angle_diff / 40);
   else
    put(METH_MOVE2, MB_PR_MOVE_DELAY, 0);


}





process side_pod
{


// This is the process's interface definition. It determines the nature of the process and
// the methods it has available. It must come before any code.
interface
{
#subdefine PROCESS_SHAPE SHAPE_4IRREG_L
 PROGRAM_TYPE_PROCESS, PROCESS_SHAPE, 3, 1, // program type, shape, size (from 0-3), base_vertex
 {
 	METH_LINK: {MT_PR_LINK, 2},
  METH_MATHS: {MT_PR_MATHS}, // maths method. allows trigonometry etc.
  METH_STD: {MT_PR_STD}, // information method. allows access to process's current properties (location etc)
  METH_DESIGNATE1: {MT_PR_DESIGNATE}, // designator. allows process to keep track of a target acquired through scan
  METH_DPACKET1: {MT_PR_DPACKET, 0, 0, 1, 1, 2}, // directional packet method. allows process to attack
  METH_DESIGNATE2: {MT_PR_DESIGNATE}, // designator. allows process to keep track of a target acquired through scan
  METH_DPACKET2: {MT_PR_DPACKET, 2, 0, 1, 1, 2}, // directional packet method. allows process to attack
  METH_SCAN: {MT_PR_SCAN, 0, 0, 1}, // scan method. allows process to sense its surroundings. takes up 3 method slots
  {0}, // space for scan method
  {0}, // space for scan method

 }
}


// Now we declare some global variables.
// Remember that these are initialised (to zero, or to a specified value) when the process is
//  created, but not each time it is run.

int initialised; // When this is zero, it tells the process that it has just been created and needs to initialise.

// Initialisation values - these are set once, when the process is first run
int team; // This is set to the process's team (i.e. which player it is controlled by)
int world_size_x, world_size_y; // size of the world (in pixels)

// Current state values - these are set each time the process runs
int angle; // Angle the process is pointing (in integer format, from 0 to ANGLE_1 (currently 8192))
int x, y; // Location
int speed_x, speed_y; // process' speed in pixels per execution cycle (not per tick)
int spin; // Direction and magnitude of process' spin (i.e. which way it's turning)

// Behaviour/mode values - these determine the process' current behaviour
int target_x, target_y; // Where the process is going
int mode; // Behavioural mode of the process (see the MODE enums below)
int mode_count; // Time the process will stay in current mode (only relevant for some modes)
int counter;

// Scan values - used to scan for other nearby processes
int scan_result [32]; // actually only 4 values are used, but may want to extend later.
int scan_bitmask;



// Function declarations
// Note that "static" in a function declaration means something different from normal C.
//  - It means that all variables in the function are static, which is recommended as
//    static variables are much more efficient than automatic variables (which you
//    shouldn't use without a good reason)
static void scan_for_target(int designate_method, int scan_angle, int dist);
static void lead_target(int attack_x_ptr, int attack_y_ptr);
static void run_dpacket_method(int m, int designate_method, int vertex_angle, int vertex_dist, int delay);
static int random(int max);

// Remember that main is called every time the program is run.
// Its correct type is static void main(void)
static void main(void)
{

// These variables are used below:
 int target_angle;
 int angle_diff;

// First we get some basic information about the process' current state.
// The "get_*" functions are built-in shortcuts that the compiler turns into calls
//  to the INFO method.
 angle = get_angle();
 x = get_x();
 y = get_y();
 speed_x = get_speed_x();
 speed_y = get_speed_y();
 spin = get_spin();
 counter++;

// If this is the first time the process has been run, we also need to set up some
//  basic information about things like which player is controlling it.
 if (initialised == 0)
 {
  team = get_team(); // Which player is the process being controlled by? (0-3)
  world_size_x = get_world_x(); // This is the size of the entire playing area, in pixels
  world_size_y = get_world_y();
  scan_bitmask = 15 ^ (1<<team); // scan_bitmask is used by the scanner to find processes that belong to a different team.
      // The four least significant bits of scan_bitmask tell the scanner to accept (1) or ignore (0) processes that belong to player 4, 3, 2 and 1.
      // 15 means accept all teams (1111).
      // ^ (1<<team) flips the bit corresponding to this process' team, so the scanner will ignore friendly processes.
      // For example, if this process belongs to player 2 the bits will be 1101.
  initialised = 1; // Now it won't be initialised again
 }

 if (counter & 1)
  scan_for_target(METH_DESIGNATE1, angle, 200);
   else
    scan_for_target(METH_DESIGNATE2, angle + ANGLE_2, 200);

   run_dpacket_method(METH_DPACKET1, METH_DESIGNATE1,
																						data(DATA_SHAPE_VERTEX_ANGLE, PROCESS_SHAPE, 0), data(DATA_SHAPE_VERTEX_DIST, PROCESS_SHAPE, 3, 0), 1);
   run_dpacket_method(METH_DPACKET2, METH_DESIGNATE2,
																						data(DATA_SHAPE_VERTEX_ANGLE, PROCESS_SHAPE, 2), data(DATA_SHAPE_VERTEX_DIST, PROCESS_SHAPE, 3, 2), 7);


}

// This function looks for a nearby enemy to attack.
// If it finds one, it designates it as a target and changes the process' mode to MODE_ATTACK:
static void scan_for_target(int designate_method, int scan_angle, int dist)
{

 int des_mbank_base;
 int scan_x, scan_y;

 scan_x = cos(scan_angle, dist);
 scan_y = sin(scan_angle, dist);

// Operating the scanner is a little complicated.
// Basically, we need to set up the SCAN method's registers with the parameters of the scan,
//  then call the method.
// The call() built-in function can be used to do this (its first argument is the method's index, followed
//  by values that will be put into the method's registers, in order, before the method is run)

 int scan_return;

 scan_return = call(METH_SCAN, // method's index
                    MS_PR_SCAN_SCAN, // status register (MB_PR_SCAN_STATUS) - tells the method to run scan
                    &scan_result [0], // memory address to put results of scan (MB_PR_SCAN_START_ADDRESS)
                    1, // number of targets to find (MB_PR_SCAN_NUMBER) - scanner will stop after finding 1
                    scan_x, // x offset of scan centre (MB_PR_SCAN_X1) - scan is centred on process
                    scan_y, // y offset of scan centre (MB_PR_SCAN_Y1) - scan is centred on process
                    10000, // size of scan (in pixels) (MBANK_PR_SCAN_SIZE) - very high value means maximum range (which is about 1000)
                    0, // (MBANK_PR_SCAN_Y2) - not relevant to this type of scan
                    scan_bitmask, // this bitmask indicates which targets will be found (MBANK_PR_SCAN_BITFIELD_WANT) - set up at initialisation
                    0, // bitmask for accepting only certain targets (MBANK_PR_SCAN_BITFIELD_NEED) - not used here
                    0); // bitmask for rejecting certain targets (MBANK_PR_SCAN_BITFIELD_REJECT) - not used here

// scan_return now holds the number of processes that the scan found (which will be no more than 1, because of the MB_PR_SCAN_NUMBER set above)
// scan_result [0] and [1] now hold the x/y coordinates (as offsets from the process performing the scan)
//  - if more than one target is found (impossible with the settings used here),
//    they will be in scan_result in *roughly* ascending order of distance from the process.

 if (scan_return > 0)
 {
// We found something! So now we tell the DESIGNATE method to lock onto the target.
//  (the DESIGNATE method allows the process to keep track of its target between execution cycles)
  des_mbank_base = designate_method * 4;
  put_index(des_mbank_base, MS_PR_DESIGNATE_ACQUIRE);
  put_index(des_mbank_base + 1, scan_result [0] + scan_x);
  put_index(des_mbank_base + 2, scan_result [1] + scan_y);

  call(designate_method);

// The dpacket methods of this process automatically fire at their designated target.

 }


}

// This function adjusts attack_x_ptr and attack_y_ptr to take account of process and target movement.
//  attack_x_ptr and attack_y_ptr are are pointers to the target coordinates.
//  The coordinates must be relative to the process' own location.
// Note that the compiler's somewhat limited type system doesn't recognise pointers. Instead, you can use
//  ordinary ints as pointers.
static void lead_target(int attack_x_ptr, int attack_y_ptr)
{

 int dist;
 int scan_return;
 int attack_x, attack_y;
 attack_x = *attack_x_ptr;
 attack_y = *attack_y_ptr;

// To find the target's speed, we use the SCAN method's secondary EXAMINE mode.
// Because most of the SCAN method's registers aren't relevant to EXAMINE mode, we set the ones that are used
//  individually instead of putting them all in as arguments to a big call().
// We need to set the following:
 put(METH_SCAN, MB_PR_SCAN_STATUS, MS_PR_SCAN_EXAMINE); // sets the method's status to EXAMINE
 put(METH_SCAN, MB_PR_SCAN_X1, attack_x); // location of target, as an offset (in pixels) from process
 put(METH_SCAN, MB_PR_SCAN_Y1, attack_y); // y offset
 put(METH_SCAN, MB_PR_SCAN_START_ADDRESS, &scan_result [0]); // results are also stored in scan_result
  // results are: [0] x [1] y [2] speed_x [3] speed_y
  // x and y are offsets from the x/y centre of the examine

 scan_return = call(METH_SCAN); // returns 1 (success), 0 (failure)

 if (scan_return == 1)
 {
// Find the distance between the process and its target:
  dist = hypot(attack_y, attack_x) / 120; // hypot is a built-in function that calls the MATHS method. 150 is an approximation factor.
  *attack_x_ptr += (scan_result [2] - speed_x) * dist; // x speed (obtained during examine call above)
  *attack_y_ptr += (scan_result [3] - speed_y) * dist; // y speed (obtained during examine call above)
 }

}

static void run_dpacket_method(int m, int designate_method, int vertex_angle, int vertex_dist, int delay)
{
	  int target_angle;
	  int attack_x, attack_y;

	  int des_mbank_base;
	  des_mbank_base = designate_method * 4;
	  int dpacket_mbank_base;
   dpacket_mbank_base = m * 4;


	  put_index(des_mbank_base, MS_PR_DESIGNATE_LOCATE);
   if (call(designate_method) == 0)
			{
// reset the dpacket angle so that it returns to centre:
    put_index(dpacket_mbank_base + MB_PR_DPACKET_ANGLE, 0);
				return;
			}

   attack_x = get_index(des_mbank_base + 1); // gets DESIGNATE method's register 1
   attack_y = get_index(des_mbank_base + 2); // gets DESIGNATE method's register 2
// target_x/y is used for both movement and attacking.
// If the target is (roughly) within 800 pixels of this process, consider firing at it:
    if (abs(attack_y) + abs(attack_x) > 1800) // Could use hypot(), but that's more expensive
					return;


    lead_target(&attack_x, &attack_y); // lead_target() tries to point the process towards where the target is going.
        // attack_x and attack_y are updated by lead_target()
    target_angle = atan2(attack_y, attack_x); // atan2 is a built-in function that calls the MATHS method.
          // It gets the absolute angle from the process to its target.
// Now check each of the two dpacket methods to see if they're pointing at the target:

    int adjusted_y;
    int adjusted_x;

    adjusted_y = attack_y - sin(angle + vertex_angle, vertex_dist);
    adjusted_x = attack_x - cos(angle + vertex_angle, vertex_dist);

    target_angle = atan2(adjusted_y, adjusted_x); // atan2 is a built-in function that calls the MATHS method.

    target_angle = signed_angle_difference(vertex_angle, target_angle - angle);

 int method_angle;

 method_angle = call(m);
// target_angle -= data(DATA_SHAPE_VERTEX_ANGLE, SHAPE_6HEXAGON, method_vertex);

 put_index(dpacket_mbank_base + MB_PR_DPACKET_ANGLE, target_angle - 200 + random(400));

 if (angle_difference(target_angle, method_angle) < ANGLE_16) // angle_difference is another MATHS built-in function.
          // It gets the absolute difference between two angles (taking bounds into account).
 {
// Make sure we've got enough irpt to fire:
//  if (get_irpt() > 100) // get_irpt() is a built-in INFO function.
   put_index(dpacket_mbank_base + MB_PR_DPACKET_COUNTER, delay);
// put sets a method's register to a specified value.
// This put sets register 0 (MB_PR_PACKET_COUNTER) to 1. This tells the packet method to
//  fire as soon as possible.
 }

}



// A very bad pseudorandom number generator that might just be good enough for this purpose:
static int random(int max)
{

// As this is a static function, seed is a static variable that is initialised when the process is created.
 int seed = 7477;
// seed += 4639 * max;
// return abs((get_angle() * seed) + get_speed_x() + get_speed_y()) % max;

 seed = seed * get_time() + get_x() + seed;

 return abs(seed) % max;

}


} // end process side_pod
