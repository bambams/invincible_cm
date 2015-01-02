


/*

pr_pfactory.c

This process allocates data and uses it to create new processes.
It also builds defensive pods around it, and an extended builder pod.

The data allocation method makes it immobile.
If left to its own devices it will just produce processes from process template 1,  *** not 0!
 but it will also accept commands using the standard commands macros.
It will not build anything until it is at maximum data (so that if it is attacked it has plenty of data to rebuild)
It is not set up to interact with a user
 (see m_pr_cfactory.c for another version of a factory that is designed to work with an operator program)

*/

#ifndef STANDARD_COMMANDS
#include "stand_coms.c"
#endif


// This is the process's interface definition. It determines the nature of the process and
// the methods it has available. It must come before any code.
interface
{
 PROGRAM_TYPE_PROCESS, SHAPE_8OCTAGON, 3, 3, // program type, shape, size (from 0-3), base_vertex
 {
  METH_IRPT: {MT_PR_IRPT, 0, 0, 4}, // generates irpt
  METH_ALLOCATE: {MT_PR_ALLOCATE, 2}, // allocates data from the surroundings. This is an external method; it will go on vertex 2.
  METH_STD: {MT_PR_STD}, // information method. allows access to process's current properties (location etc)
  METH_MATHS: {MT_PR_MATHS},
  METH_NEW: {MT_PR_NEW}, // new process method. Can build new processes.
  {0}, // new process method requires 2 method slots, so this one is empty
  METH_SCAN: {MT_PR_SCAN, 0, 0, 4}, // scan method
  {0}, // scan method requires extra method slots
  {0},
  METH_COMMAND: {MT_PR_COMMAND}, // command method allows the process to receive commands from the client program
  METH_RESTORE: {MT_PR_RESTORE},
  METH_LINK_BUILD: {MT_PR_LINK, 0},
  METH_LINK1: {MT_PR_LINK, 1},
  METH_LINK2: {MT_PR_LINK, 3},
  METH_LINK3: {MT_PR_LINK, 5},
  METH_LINK4: {MT_PR_LINK, 7},
 }
}



// Now we declare some global variables.
// Remember that these are initialised (to zero, or to a specified value) when the process is
//  created, but not each time it is run.

int initialised; // When this is zero, it tells the process that it has just been created and needs to initialise.

// Initialisation values - these are set once, when the process is first run
int team; // This is set to the process's team (i.e. which player it is controlled by)
int angle;
int x, y;
int scan_bitmask;


// Behaviour/mode values - these determine the process' current behaviour
int data_needed; // this is how much data the factory needs for its current build
int p_pod_cost; // cost of a pod. Set up at initialisation.

static void scan_for_pod_target(int pod_vertex, int m, int scan_angle, int scan_distance);
static void build_p_pod(int v, int child_vertex, int pod_shape, int pod_angle);

process build_pod;
process p_pod;

static void main(void)
{

 int build_result;
 int build_source = 1;
 int command_received;
 int i;


 if (initialised == 0)
 {
  initialised = 1;
  angle = get_angle();
  x = get_x();
  y = get_y();
  team = get_team();
// work out the cost of a pod:
  call(METH_NEW,
       MS_PR_NEW_BC_COST_DATA, // MB_PR_NEW_STATUS
       2, // MB_PR_NEW_VERTEX1: Vertex of this process that new process will be created at.
       -1, // MB_PR_NEW_VERTEX2: Vertex of child process (-1 means use child process' base_vertex value)
       0, // MB_PR_NEW_ANGLE: Angle of child process
       process_start(p_pod), // MB_PR_NEW_START: Start address
       process_end(p_pod)); // MB_PR_NEW_END: End address
  p_pod_cost = get(METH_NEW, MB_PR_NEW_STATUS);
  scan_bitmask = 0b1111 ^ (1<<team); // scan_bitmask is used by the scanner to find processes that belong to a different team.
      // The four least significant bits of scan_bitmask tell the scanner to accept (1) or ignore (0) processes that belong to player 4, 3, 2 and 1.
      // 15 means accept all teams (1111).
      // ^ (1<<team) flips the bit corresponding to this process' team, so the scanner will ignore friendly processes.
      // For example, if this process belongs to player 2 the bits will be 1101.
// Set up the process' command registers
  set_command_bit_1(COMREG_PROCESS_DETAILS, COM_PROCESS_DET_HP); // tells operator what details to display
  set_command_bit_1(COMREG_PROCESS_DETAILS, COM_PROCESS_DET_IRPT); // tells operator what details to display
  set_command_bit_1(COMREG_PROCESS_DETAILS, COM_PROCESS_DET_DATA); // tells operator what details to display
  set_command_bit_1(COMREG_PROCESS_DETAILS, COM_PROCESS_DET_ALLOC_EFF); // tells operator what details to display
// the +4 is for minimum separation between linked processes
 }

// Call the allocator to generate some data (unlike the IRPT method, it doesn't run automatically):
 call(METH_ALLOCATE); // can only be called once each cycle

 call(METH_RESTORE, 1);

// Now check for whether any of the pods need to be built or rebuild:
 if (call(METH_LINK1, MS_PR_LINK_EXISTS) == 0)
 {
  build_p_pod(1, 2, SHAPE_6IRREG_R, ANGLE_8 + ANGLE_32);
//  build_p_pod(1, 4, SHAPE_6IRREG_L, -ANGLE_8);
 }
  else
			scan_for_pod_target(1, METH_LINK1, angle + data(DATA_SHAPE_VERTEX_ANGLE, SHAPE_8OCTAGON, 1), 400);
 if (call(METH_LINK2, MS_PR_LINK_EXISTS) == 0)
 {
  build_p_pod(3, 5, SHAPE_6IRREG_R, ANGLE_8 - ANGLE_32);
 }
  else
			scan_for_pod_target(3, METH_LINK2, angle + data(DATA_SHAPE_VERTEX_ANGLE, SHAPE_8OCTAGON, 3), 400);
 if (call(METH_LINK3, MS_PR_LINK_EXISTS) == 0)
 {
  build_p_pod(5, 1, SHAPE_6IRREG_L, -ANGLE_8 + ANGLE_32);
 }
  else
			scan_for_pod_target(5, METH_LINK3, angle + data(DATA_SHAPE_VERTEX_ANGLE, SHAPE_8OCTAGON, 5), 400);
 if (call(METH_LINK4, MS_PR_LINK_EXISTS) == 0)
 {
  build_p_pod(7, 4, SHAPE_6IRREG_L, -ANGLE_8 - ANGLE_32);
 }
  else
			scan_for_pod_target(7, METH_LINK4, angle + data(DATA_SHAPE_VERTEX_ANGLE, SHAPE_8OCTAGON, 7), 400);
 if (call(METH_LINK_BUILD, MS_PR_LINK_EXISTS) == 0)
 {
                  call(METH_NEW,
                     MS_PR_NEW_BC_BUILD, // MB_PR_NEW_STATUS
                     0, // MB_PR_NEW_VERTEX1: Vertex of this process that new process will be created at.
                     -1, // MB_PR_NEW_VERTEX2: Vertex of child process (-1 means use child process' base_vertex value)
                     0, // MB_PR_NEW_ANGLE: Angle of child process
                     process_start(build_pod), // MB_PR_NEW_START: Start address
                     process_end(build_pod), // MB_PR_NEW_END: End address
                     1); // MB_PR_NEW_LINK: 1 means try to connect to new process

 }

}


static void scan_for_pod_target(int pod_vertex, int m, int scan_angle, int scan_distance)
{

 int scan_return, scan_x, scan_y;
 int scan_result [8]; // really just need 2 but let's be safe

 scan_x = cos(scan_angle, scan_distance);
 scan_y = sin(scan_angle, scan_distance);

 scan_return = call(METH_SCAN, // method's index
                    MS_PR_SCAN_SCAN, // status register (MB_PR_SCAN_STATUS) - tells the method to run scan
                    &scan_result [0], // memory address to put results of scan (MB_PR_SCAN_START_ADDRESS)
                    1, // number of targets to find (MB_PR_SCAN_NUMBER) - scanner will stop after finding 1
                    scan_x, // x offset of scan centre (MB_PR_SCAN_X1)
                    scan_y, // y offset of scan centre (MB_PR_SCAN_Y1) - scan is centred on process
                    10000, // size of scan (in pixels) (MBANK_PR_SCAN_SIZE) - very high value means maximum range (which is about 1000)
                    0, // (MBANK_PR_SCAN_Y2) - not relevant to this type of scan
                    scan_bitmask, // this bitmask indicates which targets will be found (MBANK_PR_SCAN_BITFIELD_WANT) - set up at initialisation
                    0, // bitmask for accepting only certain targets (MBANK_PR_SCAN_BITFIELD_NEED) - not used here
                    0); // bitmask for rejecting certain targets (MBANK_PR_SCAN_BITFIELD_REJECT) - not used here

// if we found something for the pod to attack, send it a message:
 if (scan_return > 0)
 {
 	put_index(m * 4, MS_PR_LINK_MESSAGE);
 	put_index((m * 4) + 1, scan_result [0] + scan_x + x);
 	put_index((m * 4) + 2, scan_result [1] + scan_y + y);
  call(m);
 }

}


static void build_p_pod(int v, int child_vertex, int pod_shape, int pod_angle)
{

 if (get_data() < p_pod_cost)
  return;

// Set the vertex p_pod's link method will be on
 *process_start(p_pod) + (3) = pod_shape;
 *process_start(p_pod) + (7) = child_vertex;

                call(METH_NEW,
                     MS_PR_NEW_BC_BUILD, // MB_PR_NEW_STATUS
                     v, // MB_PR_NEW_VERTEX1: Vertex of this process that new process will be created at.
                     child_vertex, // MB_PR_NEW_VERTEX2: Vertex of child process (-1 means use child process' base_vertex value)
                     pod_angle, // MB_PR_NEW_ANGLE: Angle of child process
                     process_start(p_pod), // MB_PR_NEW_START: Start address
                     process_end(p_pod), // MB_PR_NEW_END: End address
                     1); // MB_PR_NEW_LINK: 1 means try to connect to new process

}




process p_pod
{
// All this process does is scan a nearby area and protect its factory, while repairing itself if needed.

interface
{
 PROGRAM_TYPE_PROCESS, SHAPE_6IRREG_L, 3, 2, // if shape or size are changed, fix turret_x/y calculations below
 {
  METH_LINK_PARENT: {MT_PR_LINK, 2},
  METH_STD: {MT_PR_STD},
  METH_DPACKET1: {MT_PR_DPACKET, 0, 0, 0, 1, 3},
  METH_DPACKET2: {MT_PR_DPACKET, 3, 0, 0, 1, 3},
  METH_MATHS: {MT_PR_MATHS},
  METH_DESIGNATE: {MT_PR_DESIGNATE}, // designator. allows process to keep track of a target acquired through scan
  METH_IRPT: {MT_PR_IRPT, 0, 0, 0}, // generates irpt
  METH_RESTORE: {MT_PR_RESTORE},
 }
}



// Now we declare some global variables.
// Remember that these are initialised (to zero, or to a specified value) when the process is
//  created, but not each time it is run.

int initialised; // When this is zero, it tells the process that it has just been created and needs to initialise.

// Initialisation values - these are set once, when the process is first run
int team; // This is set to the process's team (i.e. which player it is controlled by)

// Current state values - these are set each time the process runs
int angle; // Angle the process is pointing (in integer format, from 0 to ANGLE_1 (currently 8192))
int x, y; // Location

// Behaviour/mode values - these determine the process' current behaviour
int mode; // Behavioural mode of the process (see the MODE enums below)
int mode_count; // Time the process will stay in current mode (only relevant for some modes)
int attack_x;
int attack_y;
int target_speed_x;
int target_speed_y;
int link_message [LINK_MESSAGES] [LINK_MESSAGE_SIZE]; // The LINK method can send and receive messages consisting of two ints.


int dpacket_vertex [2];
int dpacket_vertex_angle [2];
int dpacket_vertex_dist [2];
int dpacket_vertex_angle_min [2];
int dpacket_vertex_angle_max [2];


static int scan_for_target_multi(int designate_method, int offset_angle, int offset_dist, int scan_size);
static int run_dpacket_method_multi(int m, int vertex_angle, int vertex_dist, int min_angle, int max_angle, int packet_speed, int delay);
static int lead_target(int relative_x, int relative_y,
																							int relative_speed_x, int relative_speed_y,
																			    int base_angle,
																			    int intercept_speed,
																			    int delay);

// Mode enum - these are for the process' mode value
enum
{
MODE_IDLE, // Process is sitting around, scanning for something to attack
MODE_ATTACK // Process has a designated target and is attacking it

};

// Function declarations
// Note that "static" in a function declaration means something different from normal C.
//  - It means that all variables in the function are static, which is recommended as
//    static variables are much more efficient than automatic variables (which you
//    shouldn't use without a good reason)
static void run_dstream_method(int m, int target_angle, int delay);

// Remember that main is called every time the program is run.
// Its correct type is static void main(void)
static void main(void)
{

// These variables are used below:
 int target_angle;
 int angle_diff;
 int turn_dir;
 int attack_angle;
 int future_turn_dir;

// If this is the first time the process has been run, we also need to set up some
//  basic information about things like which player is controlling it.
 if (initialised == 0)
 {
  team = get_team(); // Which player is the process being controlled by? (0-3)
  mode = MODE_IDLE;
// We can also set up some things that normally would be updated each cycle, but don't need to be as this process is immobile:
  angle = get_angle();
  x = get_x();
  y = get_y();
//  turret_x_offset = cos(angle, data(DATA_SHAPE_VERTEX_DIST, SHAPE_6LONG, 3, 0));
//  turret_y_offset = sin(angle, data(DATA_SHAPE_VERTEX_DIST, SHAPE_6LONG, 3, 0));
  put(METH_LINK_PARENT, MB_PR_LINK_MESSAGE_ADDRESS, &link_message [0] [0]);

	 dpacket_vertex [0] = *(6+(METH_DPACKET1*6)+1);
  dpacket_vertex_angle [0] = get_vertex_angle(*(6+(METH_DPACKET1*6)+1));
  dpacket_vertex_dist [0] = get_vertex_dist(*(6+(METH_DPACKET1*6)+1));
  dpacket_vertex_angle_min [0] = get_vertex_angle_min(*(6+(METH_DPACKET1*6)+1));
  dpacket_vertex_angle_max [0] = get_vertex_angle_max(*(6+(METH_DPACKET1*6)+1));;

	 dpacket_vertex [1] = *(6+(METH_DPACKET2*6)+1);
  dpacket_vertex_angle [1] = get_vertex_angle(*(6+(METH_DPACKET2*6)+1));
  dpacket_vertex_dist [1] = get_vertex_dist(*(6+(METH_DPACKET2*6)+1));
  dpacket_vertex_angle_min [1] = get_vertex_angle_min(*(6+(METH_DPACKET2*6)+1));
  dpacket_vertex_angle_max [1] = get_vertex_angle_max(*(6+(METH_DPACKET2*6)+1));;


  initialised = 1; // Now it won't be initialised again
 }

// call(METH_RESTORE, 2);

// parent will scan for targets and tell this process if it finds any, using the link method to send messages
	if (call(METH_LINK_PARENT, MS_PR_LINK_RECEIVED) != 0)
	{
// acquire target using new coordinates:
  if (call(METH_DESIGNATE, MS_PR_DESIGNATE_ACQUIRE, link_message [0] [0] - x, link_message [0] [1] - y) != 0)
		{
			mode = MODE_ATTACK;
		}
	}

// What the process does next depends on which mode it is in.
 switch(mode)
 {
  case MODE_IDLE:
   put(METH_DPACKET1, MB_PR_DPACKET_ANGLE, 0); // tells the method to return to centre
   put(METH_DPACKET2, MB_PR_DPACKET_ANGLE, 0); // tells the method to return to centre
// only repair if not under attack
   if (get_irpt() > 2000)
		  call(METH_RESTORE, 1);
   break;

  case MODE_ATTACK:
   if (call(METH_DESIGNATE, MS_PR_DESIGNATE_LOCATE) == 1)
   {
    attack_x = get(METH_DESIGNATE, 1); // gets DESIGNATE method's register 1
    attack_y = get(METH_DESIGNATE, 2); // gets DESIGNATE method's register 2
    call(METH_DESIGNATE, MS_PR_DESIGNATE_SPEED);
    target_speed_x = get(METH_DESIGNATE, 1);
    target_speed_y = get(METH_DESIGNATE, 2);
// If the target is roughly nearby this process, consider firing at it:
    if (abs(attack_y) + abs(attack_x) < 2000) // Could use hypot(), but that's more expensive
    {
     run_dpacket_method_multi(METH_DPACKET1, dpacket_vertex_angle [0], dpacket_vertex_dist [0],
																														dpacket_vertex_angle_min [0], dpacket_vertex_angle_max [0], 8 * 16, 1);
     run_dpacket_method_multi(METH_DPACKET2, dpacket_vertex_angle [1], dpacket_vertex_dist [1],
																														dpacket_vertex_angle_min [1], dpacket_vertex_angle_max [1], 8 * 16, 4);
    }
   }
    else
    {
// If the process can't find its target, it changes its mode to WANDER.
// At this stage target_x/y are not reset, so it will go to the last place it saw its target before wandering randomly.
     mode = MODE_IDLE;
    }
   mode_count --;
   if (mode_count <= 0)
    mode = MODE_IDLE;
   break;

 } // end of switch(mode)


}


static int run_dpacket_method_multi(int m, int vertex_angle, int vertex_dist, int min_angle, int max_angle, int packet_speed, int delay)
{

 int source_x, source_y; // location of packet source vertex with respect to process
 int old_source_x, old_source_y; // location of packet source vertex with respect to process, last tick
 int source_speed_x, source_speed_y; // speed of packet source vertex
 int dist; // distance to target
 int i;
 int method_angle;
 int target_angle;
// int target_x, target_y;
// target_x = attack_x + ((target_speed_x * delay) / 16);
// target_y = attack_y + ((target_speed_y * delay) / 16);

// Now we need to aim. First we work out the location and speed of the source vertex:
//  (can't use the process's speed as it may be spinning)

 source_x = cos(angle + vertex_angle, vertex_dist); // process itself is at 0,0
 source_y = sin(angle + vertex_angle, vertex_dist); // process itself is at 0,0

// now find the angle to the target, adjusting for relative speeds:
 target_angle = lead_target(attack_x, // need to adjust target offset by vertex location
																		          attack_y,
																						      target_speed_x,
																						      target_speed_y,
																		          angle + vertex_angle, // base_angle
																		          packet_speed,
																		          delay);

 if (target_angle < min_angle
  || target_angle > max_angle)
  goto cancel_attack; // out of dpacket method's rotation range

 method_angle = call(m); // this returns the method's angle as an offset from the angle pointing directly from the centre of the process.

 put_index((m * 4) + MB_PR_DPACKET_ANGLE, target_angle); // tells the method to rotate to this angle

// make sure the method is pointing in approx the right direction
 if (angle_difference(target_angle, method_angle) < ANGLE_8) // this returns the magnitude (unsigned) of the difference between the two angles.
 {
// Make sure we've got enough irpt to fire:
  if (get_irpt() > 100) // get_irpt() is a built-in INFO function.
   put_index((m * 4) + MB_PR_DPACKET_COUNTER, delay); // this tells the method to fire after <delay> ticks.
    // delay should be at least 1 (can be more to stagger the firing of multiple methods)
 }

 return 1; // attack succeeded, or failed in a way that shouldn't stop the process from trying again next cycle

 cancel_attack: // stop attacking
  put_index((m*4) + MB_PR_DPACKET_ANGLE, 0); // return to centre
  return 0; // attack failed - stop trying

}



// Basic target-leading algorithm that returns an offset from base_angle (which can e.g. be put
//  directly into the angle register of a dpacket method).
// relative_x/y are the x/y distance from the source (e.g. the vertex with the dpacket method)
//  to the target.
// relative_speed_x/y are the target's speeds minus the source's speed.
// base_angle is the angle of the source (e.g. the angle from a process to a vertex)
// intercept_speed is the speed of the projectile/intercepting process.
static int lead_target(int relative_x, int relative_y,
																							int relative_speed_x, int relative_speed_y,
																			    int base_angle,
																			    int intercept_speed,
																			    int delay)
{

#subdefine LEAD_ITERATIONS 2
// Testing indicates that only 2 iterations are really needed.
// (also, the loop calls the expensive hypot function)

 int i;
 int flight_time;
 int modified_target_x, modified_target_y;
 int dist;
 int intercept_angle;

 modified_target_x = relative_x + ((relative_speed_x * delay) / 16);
 modified_target_y = relative_y + ((relative_speed_y * delay) / 16);

 for (i = 0; i < LEAD_ITERATIONS; i ++)
	{
  dist = hypot(modified_target_x, modified_target_y); // hypot is a built-in function that calls the MATHS method.

  flight_time = dist / (intercept_speed / 16);

	 modified_target_x = relative_x + ((relative_speed_x * (flight_time)) / 16); // / 16 is needed because speed is * 16 for accuracy
	 modified_target_y = relative_y + ((relative_speed_y * (flight_time)) / 16);
	}

 intercept_angle = atan2(modified_target_y, modified_target_x);
 intercept_angle = signed_angle_difference(base_angle, intercept_angle); // remember - can't use a maths function as an argument of another maths function.

	return intercept_angle;

}


} // end process p_pod

process build_pod
{



// This is the process's interface definition. It determines the nature of the process and
// the methods it has available. It must come before any code.
interface
{
 PROGRAM_TYPE_PROCESS, SHAPE_4DIAMOND, 2, 0, // program type, shape, size (from 0-3), base_vertex
 {
  METH_STD: {MT_PR_STD}, // information method. allows access to process's current properties (location etc)
  METH_MATHS: {MT_PR_MATHS},
  METH_NEW: {MT_PR_NEW}, // new process method. Can build new processes.
  {0}, // new process method requires 2 method slots, so this one is empty
  METH_COMMAND: {MT_PR_COMMAND}, // command method allows the process to receive commands from the client program
  METH_RESTORE: {MT_PR_RESTORE},
  METH_LINK_BUILD: {MT_PR_LINK, 2},
  METH_LINK_PARENT: {MT_PR_LINK, 0},
 }
}



// Now we declare some global variables.
// Remember that these are initialised (to zero, or to a specified value) when the process is
//  created, but not each time it is run.

int initialised; // When this is zero, it tells the process that it has just been created and needs to initialise.


// Behaviour/mode values - these determine the process' current behaviour
int building; // what the factory is building now. Uses the BUILD_* enums below.
int build_wait; // is 1 if the factory is waiting for enough data/irpt to build
int data_needed; // this is how much data the factory needs for its current build

enum
{
BUILD_NOTHING, // Process isn't building anything
BUILD_TEMPLATE0, // Build whatever is in template 1
BUILD_TEMPLATE1, // Build whatever is in template 2
BUILD_TEMPLATE2, // Build whatever is in template 3
BUILD_TEMPLATE3 // Build whatever is in template 4
};

static void check_data_cost(int template_index);
static int try_building(int template_index);

static void main(void)
{

 int build_result;
 int build_source = 1;
 int command_received;
 int i;


 if (initialised == 0)
 {
  initialised = 1;
  building = BUILD_TEMPLATE1;
  check_data_cost(building);
// Set up the process' command registers
  set_command_bit_1(COMREG_PROCESS_DETAILS, COM_PROCESS_DET_HP); // tells operator what details to display
  set_command_bit_1(COMREG_PROCESS_DETAILS, COM_PROCESS_DET_IRPT); // tells operator what details to display
  set_command_bit_1(COMREG_PROCESS_DETAILS, COM_PROCESS_DET_DATA); // tells operator what details to display
 }


 call(METH_RESTORE, 1);

 if (building != BUILD_NOTHING)
 {
  if (get_data() == get_data_max())
   try_building(building - 1);
 }

// Now we check the command registers to see whether the process' client program has given it a command:
 if (get_command_bit(COMREG_CLIENT_STATUS, COM_CLIENT_ST_QUERY) == 1)
	{
		set_command_bit_0(COMREG_CLIENT_STATUS, COM_CLIENT_ST_QUERY);
  command_received = get_command(COMREG_QUEUE);

  switch(command_received)
  {

   case COMMAND_BUILD:
// The source template index should be in command field COMREG_QUEUE + 1:
     building = get_command(COMREG_QUEUE + 1) + BUILD_TEMPLATE1;
     check_data_cost(building);
     set_command(COMREG_QUEUE, COMMAND_NONE);
     break; // end COMMAND_BUILD

   case COMMAND_IDLE:
    building = BUILD_NOTHING; // stop trying to build something
// Acknowledge the command:
    set_command(COMREG_QUEUE, COMMAND_NONE);
    break;

  } // end switch(command_received)
	} // end if new command received


}


// Returns 1 on success, 0 on failure
static int try_building(int template_index)
{

 int build_result;

 build_result = call(METH_NEW,
                     MS_PR_NEW_T_BUILD, // MB_PR_NEW_STATUS: means: after this process finishes executing, try to build
                     2, // MB_PR_NEW_VERTEX1: Vertex of this process that new process will be created at.
                     -1, // MB_PR_NEW_VERTEX2: Vertex of child process (-1 means use child process' base_vertex value)
                     0, // MB_PR_NEW_ANGLE: Angle of child process
                     0, // MB_PR_NEW_START: Start address
                     BCODE_SIZE_PROCESS - 1, // MB_PR_NEW_END: End address
                     1, // MB_PR_NEW_LINK: 1 means try to connect to new process
                     template_index); // MB_PR_NEW_TEMPLATE: Source template index

 if (build_result == MR_NEW_SUCCESS)
 {
  return 1;
 }

 if (build_result == MR_NEW_FAIL_DATA)
 {
  check_data_cost(template_index);
 }

 return 0;

}


// Only call this function just after a failed call to the NEW method.
// It assumes that the NEW method's mbank registers are still in the
//  state generated for the failed NEW call.
static void check_data_cost(int template_index)
{

  int test_result;

// Calling the NEW method with status MS_PR_NEW_T_COST_DATA returns an MR_* result value and sets the method's status
//  to the amount of data needed to build the process:
  test_result = call(METH_NEW,
                     MS_PR_NEW_T_COST_DATA, // MB_PR_NEW_STATUS: means: after this process finishes executing, try to build
                     2, // MB_PR_NEW_VERTEX1: Vertex of this process that new process will be created at.
                     -1, // MB_PR_NEW_VERTEX2: Vertex of child process (-1 means use child process' base_vertex value)
                     0, // MB_PR_NEW_ANGLE: Angle of child process
                     0, // MB_PR_NEW_START: Start address
                     BCODE_SIZE_PROCESS - 1, // MB_PR_NEW_END: End address
                     1, // MB_PR_NEW_LINK: 1 means try to connect to new process
                     template_index); // MB_PR_NEW_TEMPLATE: Source template index

   if (test_result == MR_NEW_TEST_SUCCESS)
   {
     build_wait = 1;
     data_needed = get(METH_NEW, MB_PR_NEW_STATUS);
     return;
   }

}




}
