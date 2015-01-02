/*

pr_defend.c

This is a simple process that can attack other processes.
It can't operate autonomously and needs to be given commands by a client program.
It relies on the standard command macros (which should be #defined in the system or operator program)
 and assumes that the client/operator will be feeding it targetting information.

*/

#ifndef STANDARD_COMMANDS
#include "stand_coms.c"
#endif

// This is the process's interface definition. It determines the nature of the process and
// the methods it has available. It must come before any code.
interface
{
 PROGRAM_TYPE_PROCESS, SHAPE_4POINTY, 2, 1, // program type, shape, size (from 0-3), base_vertex
 {
  METH_MOVE1: {MT_PR_MOVE, 3, ANGLE_1 - data(DATA_SHAPE_VERTEX_ANGLE, SHAPE_4POINTY, 3)}, // acceleration method on vertex 3, pointing backwards
  METH_MOVE2: {MT_PR_MOVE, 1, ANGLE_1 - data(DATA_SHAPE_VERTEX_ANGLE, SHAPE_4POINTY, 1)}, // acceleration method on vertex 1, pointing backwards
  METH_COM: {MT_PR_COMMAND}, // command method. allows process to communicate with operator/delegate
  METH_MATHS: {MT_PR_MATHS}, // maths method. allows trigonometry etc.
  METH_STD: {MT_PR_STD}, // information method. allows access to process's current properties (location etc)
  METH_IRPT: {MT_PR_IRPT}, // generates irpt. functions automatically (although can be configured not to)
  METH_PACKET: {MT_PR_PACKET, 2, 0, 0, 1, 1}, // packet method. allows process to attack.
    // 2 = vertex, 0 = angle, 0 = power extension, 1 = speed extension, 0 = range extension
#subdefine PACKET_SPEED (8 * 16)
// packet speed is 6, plus 2 for each speed extension (note that dpackets are slightly slower). Speeds are all *16
  METH_SCAN: {MT_PR_SCAN, 0, 0, 1}, // scan method. allows process to sense its surroundings. takes up 3 method slots
  {0}, // space for scan method
  {0}, // space for scan method
  METH_DESIGNATE: {MT_PR_DESIGNATE, 0, 0, 1}, // designator. allows process to keep track of a target acquired through scan

 }
}

/*
interface
{
 PROGRAM_TYPE_PROCESS, SHAPE_4POINTY, 0, 3, // program type, shape, size (from 0-3), base_vertex
 {
  METH_MOVE1: {MT_PR_MOVE, 3, ANGLE_1 - data(DATA_SHAPE_VERTEX_ANGLE, SHAPE_4POINTY, 3)}, // acceleration method on vertex 3, pointing backwards
  METH_MOVE2: {MT_PR_MOVE, 1, ANGLE_1 - data(DATA_SHAPE_VERTEX_ANGLE, SHAPE_4POINTY, 1)}, // acceleration method on vertex 1, pointing backwards
  METH_COM: {MT_PR_COMMAND}, // command method. allows process to communicate with operator/delegate
  METH_MATHS: {MT_PR_MATHS}, // maths method. allows trigonometry etc.
  METH_INFO: {MT_PR_STD}, // information method. allows access to process's current properties (location etc)
  METH_IRPT: {MT_PR_IRPT}, // generates irpt. functions automatically (although can be configured not to)
  METH_PACKET: {MT_PR_PACKET, 2, 0}, // packet method. allows process to attack
  METH_SCAN: {MT_PR_SCAN}, // scan method. allows process to sense its surroundings. takes up 3 method slots
  {0}, // space for scan method
  {0}, // space for scan method
  METH_DESIGNATE: {MT_PR_DESIGNATE}, // designator. allows process to keep track of a target acquired through scan

 }
}
*/


// Now we declare some global variables.
// Remember that these are initialised (to zero, or to a specified value) when the process is
//  created, but not each time it is run.

int initialised; // When this is zero, it tells the process that it has just been created and needs to initialise.

// Initialisation values - these are set once, when the process is first run
int team; // This is set to the process's team (i.e. which player it is controlled by)
int world_size_x, world_size_y; // size of the world (in pixels)

// Current state values - these are set each time the process runs
int angle; // Angle the process is pointing (in integer format, from 0 to 8192)
int x, y; // Location (in pixels)
int speed_x, speed_y; // Process' speed in pixels per 16 ticks
int spin; // Process' spin (i.e. which way it's turning, and how fast)

// Behaviour/mode values - these determine the process' current behaviour
int mode; // Behavioural mode of the process (see the MODE enums below)
int target_x, target_y; // Where the process is going
int attack_x, attack_y; // Location of target if process is attacking
int verbose; // Is 1 if the client has told it to talk about what it's doing.

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
MODE_A_MOVE_ATTACKING // Process is attacking something it found during attack-move
};

// Function declarations
// Note that "static" in a function declaration means something different from normal C.
//  - It means that all variables in the function are static, which is recommended as
//    static variables are much more efficient than automatic variables (which you
//    shouldn't use without a good reason)
static void execute_command(void);
static void next_command(void);
static void query_process(void);
static void process_action(void);
static void scan_for_target(void);
static void process_attack(int vertex_angle, int vertex_dist, int packet_speed);
static int lead_target_xy(int relative_x, int relative_y,
																							   int relative_speed_x, int relative_speed_y,
																			       int base_angle,
																			       int intercept_speed);
static void process_move(void);

// Remember that main is called every time the program is run.
// Its correct type is static void main(void)
static void main(void)
{

// Time for some variables that are just used in main():
 int i, j;

// First we get some basic information about the process' current state.
// The get_*() functions are built-in shortcuts that the compiler turns into calls
//  to the INFO method.
 angle = get_angle() + ANGLE_2;
 x = get_x();
 y = get_y();
 speed_x = get_speed_x();
 speed_y = get_speed_y();
 spin = get_spin();
// operator can asked the process to tell the user what it's doing by setting the verbose bit of the client status command register:
 verbose = get_command_bit(COMREG_CLIENT_STATUS, COM_CLIENT_ST_VERBOSE);
 set_command_bit_0(COMREG_CLIENT_STATUS, COM_CLIENT_ST_VERBOSE); // clear the verbose command register (the operator will reset it to 1 if needed)

// If this is the first time the process has been run, we also need to set up some
//  basic information about things like which player is controlling it.
 if (initialised == 0)
 {
  team = get_team(); // Which player is the process being controlled by? (0-3)
// Now let's tell the process to move away from its parent process (by moving 200 pixels forwards):
  mode = MODE_MOVE;
  target_x = x + cos(angle, 200); // target_x and target_y are used in the movement code below
  target_y = y + sin(angle, 200); // cos and sin are built-in functions that call the MATHS method.
                                  // These calls multiply the result of cos/sin by 200 (to get the x/y components
                                  //  of a line extending 200 pixels to the front of the process)
  set_command(COMREG_QUEUE, COMMAND_MOVE); // Tells the client that this process is currently moving
  set_command(COMREG_QUEUE + 1, target_x); // tells the client where it's moving to
  set_command(COMREG_QUEUE + 2, target_y);
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
  initialised = 1; // Now it won't be initialised again
 }


 if (get_command_bit(COMREG_CLIENT_STATUS, COM_CLIENT_ST_QUERY) == 1)
	{
		query_process();
		set_command_bit_0(COMREG_CLIENT_STATUS, COM_CLIENT_ST_QUERY);
	}

// Client should set a bit of one of the command registers if it issues a new command:
 if (get_command_bit(COMREG_CLIENT_STATUS, COM_CLIENT_ST_NEW) == 1)
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

   case COMMAND_MOVE:
// Read the target coordinates, which the client will have put into other command registers:
    target_x = get_command(COMREG_QUEUE + 1);
    target_y = get_command(COMREG_QUEUE + 2);
    mode = MODE_MOVE;
    if (verbose != 0)
     print("\nMoving to ", target_x, ", ", target_y, ". ");
    break;

   case COMMAND_ATTACK:
// Read the target coordinates, which the client will have put into other command registers:
    attack_x = get_command(COMREG_QUEUE + 1) - x;
    attack_y = get_command(COMREG_QUEUE + 2) - y;
    mode = MODE_ATTACK_COMMAND;
    if (verbose != 0)
     print("\nAttacking target.");
    break;

   case COMMAND_A_MOVE:
// Read the target coordinates, which the client will have put into other command registers:
    target_x = get_command(COMREG_QUEUE + 1);
    target_y = get_command(COMREG_QUEUE + 2);
    mode = MODE_A_MOVE_COMMAND;
    if (verbose != 0)
     print("\nA-moving to ", target_x, ", ", target_y, ". ");
    break;

   case COMMAND_IDLE:
 			set_command(COMREG_QUEUE, COMMAND_NONE);
    set_command_bit_1(COMREG_PROCESS_STATUS, COM_PROCESS_ST_UPDATE);
// fall-through
   case COMMAND_NONE:
    mode = MODE_IDLE;
    put(METH_MOVE1, MB_PR_MOVE_RATE, 0); // turn off the move methods
    put(METH_MOVE2, MB_PR_MOVE_RATE, 0); // turn off the move methods
    if (verbose != 0)
     print("\nProcess entering idle mode.");
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

static void query_process(void)
{

   print("\nStatus: ");

   switch(mode)
   {
    case MODE_IDLE: print("idle"); break;
    case MODE_MOVE: print("moving"); break;
    case MODE_A_MOVE_COMMAND:
    case MODE_A_MOVE_ATTACKING: print("a-moving"); break;
    case MODE_ATTACK:
    case MODE_ATTACK_COMMAND: print("attacking"); break;
   }

}


static void process_action(void)
{

 switch(mode)
 {

  case MODE_IDLE:
// In IDLE mode, the process sits around scanning for a target.
   scan_for_target(); // If scan_for_target finds something to attack, it changes the mode to MODE_ATTACK.
   return; // return from main() (finishes)

  case MODE_MOVE:
// In MOVE mode, the process moves towards target_x/y then, when near, it executes the next command (which may be COMMAND_NONE)
   if (abs(y - target_y) + abs(x - target_x) < 100) // abs is a built-in function that calls the MATHS method
   {
    if (verbose != 0)
     print("\nReached destination.");
   	next_command();
   	execute_command();
    break;
			}
			 else
     process_move();
   break;

// MODE_ATTACK is for when the process itself has found something to attack.
// It attacks it for as long as it is within scan/designate range, then tries to execute a command.
  case MODE_ATTACK:
// In ATTACK mode, the process moves towards its designated target and attacks it.
// Calling the DESIGNATE method with status MS_PR_DESIGNATE_LOCATE does the following:
//  - If the process has no currently designated target, or if the target is out of range, it returns 0
//  - Otherwise, it sets the DESIGNATE method's method registers 1 and 2 to the x/y coordinates of
//    the target (relative to the current process' location), then returns 1.
   if (call(METH_DESIGNATE, MS_PR_DESIGNATE_LOCATE) == 1)
   {
// Read method registers:
    attack_x = get(METH_DESIGNATE, MB_PR_DESIGNATE_X);
    attack_y = get(METH_DESIGNATE, MB_PR_DESIGNATE_Y);
// Possibly attack the target (if in range and have sufficient irpt)
//  - process_attack also updates attack_x/y to lead the target
    process_attack(0, data(DATA_SHAPE_VERTEX_DIST, SHAPE_4POINTY, 3, 2), PACKET_SPEED);
// Now tell the process to move towards its target.
// The attack_x/y values are relative to the process, so they need to be made absolute.
    target_x = x + attack_x;
    target_y = y + attack_y;
    process_move();
   }
    else
    {
// If the process can't find its target, it gives up and tries to execute its current command.
     if (verbose != 0)
      print("\nTarget lost.");
     execute_command();
    }
   break;

// MODE_ATTACK_COMMAND is for when the process has received a command to attack.
// It relies on the client process feeding target coordinates through the command registers
  case MODE_ATTACK_COMMAND:
// Get target coordinations (client should have updated them):
    attack_x = get_command(COMREG_QUEUE + 1) - x;
    attack_y = get_command(COMREG_QUEUE + 2) - y;
// Possibly attack the target (if in range and have sufficient irpt)
//  - process_attack also updates attack_x/y to lead the target
    process_attack(0, data(DATA_SHAPE_VERTEX_DIST, SHAPE_4POINTY, 3, 2), PACKET_SPEED);
// Now tell the process to move towards its target.
// The attack_x/y values are relative to the process, so they need to be made absolute.
    target_x = x + attack_x;
    target_y = y + attack_y;
    process_move();
    break;

  case MODE_A_MOVE_COMMAND:
// A_MOVE mode is like MOVE move but the process attacks anything it finds along the way
   if (abs(y - target_y) + abs(x - target_x) < 100) // abs is a built-in function that calls the MATHS method
   {
    if (verbose != 0)
     print("\nReached destination.");
   	next_command();
   	execute_command();
    break;
			}
			 else
				{
					scan_for_target();
     process_move();
				}
			break;

 } // end of switch(mode)

}





// This function looks for a nearby enemy to attack.
// If it finds one, it designates it as a target and changes the process' mode to MODE_ATTACK:
static void scan_for_target(void)
{

 int designated;

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
                    0, // x offset of scan centre (MB_PR_SCAN_CENTRE_X) - 0 means scan is centred on process
                    0, // y offset of scan centre (MB_PR_SCAN_CENTRE_Y) - 0 means scan is centred on process
                    10000, // range of scan (in pixels) (MBANK_PR_SCAN_DISTANCE) - very high value will be reduced to maximum range (which is about 1000)
                    0, // (MBANK_PR_SCAN_DISTANCE2) - not relevant to this type of scan
                    scan_bitmask, // this bitmask indicates which targets will be found (MBANK_PR_SCAN_BITFIELD_WANT) - set up at initialisation
                    0, // bitmask for accepting only certain targets (MBANK_PR_SCAN_BITFIELD_NEED) - not used here
                    0); // bitmask for rejecting certain targets (MBANK_PR_SCAN_BITFIELD_REJECT) - not used here

// scan_return now holds the number of processes that the scan found (which will be no more than 1, because of the MB_PR_SCAN_NUMBER set above)
// scan_result [0] and [1] now hold the x/y coordinates (as offsets from the process performing the scan)
//  - if more than one target is found (impossible with the settings used here),
//    they will be in scan_result in ascending order of distance from the process.

 if (scan_return > 0)
 {
// We found something! So now we tell the DESIGNATE method to lock onto the target.
//  (the DESIGNATE method allows the process to keep track of its target between execution cycles without having to scan again)
  designated = call(METH_DESIGNATE,
                    MS_PR_DESIGNATE_ACQUIRE, // This mode saves a target to it can be recovered later by the MS_PR_DESIGNATE_READ mode
                    scan_result [0], // This is the location of the target as an x offset from the process
                    scan_result [1]); // Same for y

// The DESIGNATE method returns 1 if it is successful (which should always be the case here, but let's check anyway):
  if (designated == 1)
  {
   mode = MODE_ATTACK;
// Note that this doesn't change the process' command.
// It will return to executing the current command when it finishes attacking.

// Also set process' current target:
   target_x = x + scan_result [0];
   target_y = y + scan_result [1];
   if (verbose != 0)
    print("\nTarget acquired.");
  }

 }

}


// This function calculates the angle the process needs to point in to hit its target.
// It modifies the attack_x/y global variables (so it affects both firing and also movement)
//  - it's equivalent to run_dpacket_method() (for processes with dpacket methods)
// m is the method index (e.g. METH_PACKET)
// vertex_angle is the angle from the process's centre to the vertex the method is on.
// vertex_dist is the distance to the vertex.
// It assumes that attack_x/y contain the target's current location (an an offset from the process' own location).
// It returns 1 if it attacked, or 0
static void process_attack(int vertex_angle, int vertex_dist, int packet_speed)
{

 int target_angle; // angle to fire (adjusted by lead_target() below)
 int source_x, source_y; // location of packet source vertex with respect to process
 int old_source_x, old_source_y; // location of packet source vertex with respect to process, last tick
 int source_speed_x, source_speed_y; // speed of packet source vertex
 int dist; // distance to target
 int i;
 int method_angle;

// Find the distance between the process and its target:
 dist = hypot(attack_x, attack_y); // hypot is a built-in function that calls the MATHS method.
 if (dist > 900)
  return; // too far away

// Now we need to aim. First we work out the source speed.
// Because the process may be rotating, we need to work out the speed of the vertex:

 source_x = cos(angle + vertex_angle, vertex_dist); // process itself is at 0,0
 source_y = sin(angle + vertex_angle, vertex_dist); // process itself is at 0,0
 old_source_x = (0 - speed_x) + cos(angle + vertex_angle - spin, vertex_dist);
 old_source_y = (0 - speed_y) + sin(angle + vertex_angle - spin, vertex_dist);
 source_speed_x = source_x - old_source_x;
 source_speed_y = source_y - old_source_y;

// To find the target's speed, we use the SCAN method's secondary EXAMINE mode.
// Because most of the SCAN method's registers aren't relevant to EXAMINE mode, we set the ones that are used
//  individually instead of putting them all in as arguments to a big call().
// We need to set the following:
 put(METH_SCAN, MB_PR_SCAN_STATUS, MS_PR_SCAN_EXAMINE); // Sets the method's status to EXAMINE
 put(METH_SCAN, MB_PR_SCAN_X1, attack_x); // Location of target, as an offset (in pixels) from process
 put(METH_SCAN, MB_PR_SCAN_Y1, attack_y); // y offset
 put(METH_SCAN, MB_PR_SCAN_START_ADDRESS, &scan_result [0]); // Results are stored in scan_result
  // results are: scan_result [0] x [1] y [2] speed_x [3] speed_y
  // (note that x and y in the results are offsets from the centre of the examine, not from this process)
  // (also, the speed results are in pixels per tick, multiplied by 16)

// run the examine scan:
 if (call(METH_SCAN) == 0)
		return; // examine shouldn't fail if designate succeeded, but check anyway

// now find the angle to the target, adjusting for relative speeds:
 target_angle = angle + lead_target_xy(attack_x - source_x, // need to adjust target offset by vertex location
																		       attack_y - source_y,
																				     scan_result [2] - source_speed_x, // relative speed - ideally we'd calculate the speed of the vertex, but this is good enough.
																				     scan_result [3] - source_speed_y,
																		       angle + vertex_angle, // base_angle
																		       packet_speed);
// lead_target_xy returns the angle to the target (as an offset from the current angle)
//  and also updates attack_x and attack_y (which are used for movement/rotation)
 if (angle_difference(target_angle, angle) < ANGLE_8)
//		&&	get_irpt() > 100) // get_irpt() is a built-in function that calls the STD method.
  put(METH_PACKET, MB_PR_PACKET_COUNTER, 1); // this tells the method to fire the next tick.

 return;

}



// This is a version of lead_target() designed for processes with fixed packet methods.
// As well as returning an angle, it updates global variables attack_x and attack_y with
//  the target's adjusted location.
// relative_x/y are the x/y distance from the source (e.g. the vertex with the packet method)
//  to the target.
// Assumes attack_x/attack_y contain current location of target
// relative_x/y are the x/y distance from the source (e.g. the vertex with the dpacket method)
//  to the target.
// relative_speed_x/y are the target's speeds minus the source's speed.
// base_angle is the angle of the source (e.g. the angle from a process to a vertex)
// intercept_speed is the speed of the projectile/intercepting process.
static int lead_target_xy(int relative_x, int relative_y,
																							   int relative_speed_x, int relative_speed_y,
																			       int base_angle,
																			       int intercept_speed)
{

#subdefine LEAD_ITERATIONS 2
// Testing indicates that only 2 iterations are really needed.
// (also, the loop calls the expensive hypot function)

 int i;
 int flight_time;
 int modified_target_x, modified_target_y;
 int dist;
 int intercept_angle;

 modified_target_x = relative_x;
 modified_target_y = relative_y;

 for (i = 0; i < LEAD_ITERATIONS; i ++)
	{
  dist = hypot(modified_target_x, modified_target_y); // hypot is a built-in function that calls the MATHS method.

  flight_time = dist / (intercept_speed / 16);

	 modified_target_x = relative_x + ((relative_speed_x * flight_time) / 16); // / 16 is needed because speed is * 16 for accuracy
	 modified_target_y = relative_y + ((relative_speed_y * flight_time) / 16);
	}

	attack_x = modified_target_x;
	attack_y = modified_target_y;

 intercept_angle = atan2(modified_target_y, modified_target_x);
 intercept_angle = signed_angle_difference(base_angle, intercept_angle); // remember - can't use a maths function as an argument of another maths function.

	return intercept_angle;

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
 future_turn_dir = turn_direction(angle + (turn_dir * 100) + (spin * 8), target_angle); // (100 and 8 are approximation factors)
// If future_turn_dir is different, that means we will overshoot the target. So don't turn:
 if (turn_dir != future_turn_dir)
 {
  turn_dir = 0;
 }

// Now we know which way we're turning, we need to tell the move methods (i.e. engines) what to do.

// METH_MOVE1 is the right-hand engine, which will turn the process anti-clockwise if run alone.
 put(METH_MOVE1, MB_PR_MOVE_RATE, 40); // This is its power level (40 is very high, so the method will just run at maximum capacity, which is about 5)
 put(METH_MOVE1, MB_PR_MOVE_COUNTER, 40); // Will run for 40 ticks (also very high as the process will execute again in less than that. Doesn't matter)
 if (turn_dir > 0) // If turn_dir > 0, the process is trying to rotate clockwise. So we suppress this engine for a while:
  put(METH_MOVE1, MB_PR_MOVE_DELAY, angle_diff / 30); // DELAY waits for a while before the engine starts. 30 is a tuning factor.
   else
    put(METH_MOVE1, MB_PR_MOVE_DELAY, 0); // No delay - process is either turning anti-clockwise, or firing both engines to move forwards.

// Now we do the same for the left-hand engine:
 put(METH_MOVE2, MB_PR_MOVE_RATE, 40);
 put(METH_MOVE2, MB_PR_MOVE_COUNTER, 40);
 if (turn_dir < 0)
  put(METH_MOVE2, MB_PR_MOVE_DELAY, angle_diff / 30);
   else
    put(METH_MOVE2, MB_PR_MOVE_DELAY, 0);


}
