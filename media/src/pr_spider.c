



/*

pr_spider.c

This is a complex, multi-part process. It is used in mission 6, but can also be built directly.
It can receive commands (see de_m6.c) but not in the standard format.
It doesn't accept commands or interact in any way with a client/observer program.

*/

#ifndef STANDARD_COMMANDS
#include "stand_coms.c"
#endif

#subdefine LEAD_ITERATIONS 2

// This is the process's interface definition. It determines the nature of the process and
// the methods it has available. It must come before any code.
interface
{
 PROGRAM_TYPE_PROCESS, SHAPE_8LONG, 3, 4, // program type, shape, size (from 0-3), base_vertex
 {
  METH_LINK_L1: {MT_PR_LINK, 7},
  METH_LINK_L2: {MT_PR_LINK, 5},
  METH_LINK_R1: {MT_PR_LINK, 1},
  METH_LINK_R2: {MT_PR_LINK, 3},
  METH_LINK_FRONT: {MT_PR_LINK, 0},
  METH_LINK_BACK: {MT_PR_LINK, 4},
  METH_MATHS: {MT_PR_MATHS}, // maths method. allows trigonometry etc.
  METH_STD: {MT_PR_STD}, // information method. allows access to process's current properties (location etc)
  METH_IRPT: {MT_PR_IRPT, 0, 0, 3}, // generates irpt. functions automatically (although can be configured not to)
  METH_NEW: {MT_PR_NEW},
  {0}, // space for new method
  METH_RESTORE: {MT_PR_RESTORE},
  METH_STORAGE: {MT_PR_STORAGE},
  METH_COMMAND: {MT_PR_COMMAND}
 }
}


// Now we declare some global variables.
// Remember that these are initialised (to zero, or to a specified value) when the process is
//  created, but not each time it is run.

int initialised; // When this is zero, it tells the process that it has just been created and needs to initialise.
int pod_data_cost; // cost of building a pod - not totally reliable as they are modified
int generator_data_cost; // cost of building generator

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
int mode_count;
int verbose;

// communication with pods:
int link_message [4] [LINK_MESSAGES] [LINK_MESSAGE_SIZE]; // The LINK method can send and receive messages consisting of two ints.

// messages that this process and its subprocesses send back and forth through links:
#subdefine LM_COMPLETE 1
// LM_COMPLETE means that a pod is complete (so don't need to send data). 2nd value ignored
#subdefine LM_MOVE 2
// LM_MOVE requests MOVE method activation. 2nd value is length of time

// Mode enum - these are for the process' mode value
enum
{
MODE_WANDER, // Process is moving to a known location
MODE_CONNECTED
};

// Function declarations
// Note that "static" in a function declaration means something different from normal C.
//  - It means that all variables in the function are static, which is recommended as
//    static variables are much more efficient than automatic variables (which you
//    shouldn't use without a good reason)
static void start_wandering(void);
static int random(int max);

process pod_process;
process allocator;

// Remember that main is called every time the program is run.
static void main(void)
{

 int target_angle;
 int angle_diff;
 int turn_dir;
 int attack_x;
 int attack_y;
 int attack_angle;
 int left_instruction, right_instruction, fire_instruction;
 int build_result;
 left_instruction = 0;
 right_instruction = 0;

 int pod_angle;

// First we get some basic information about the process' current state.
// The "get_*" functions are built-in shortcuts that the compiler turns into calls
//  to the INFO method.
 angle = get_angle(); // this process is reversed, so that its vertex 0 points backwards. Need to adjust the angle by 180 deg (ANGLE_2)
 x = get_x();
 y = get_y();
 speed_x = get_speed_x();
 speed_y = get_speed_y();
 spin = get_spin();
// verbose = get_command(COMREG_VERBOSE); // This can be set by an operator programm



// set_command(COMREG_VERBOSE, 0); // clear the verbose flag

 if (initialised == 0)
 {
  team = get_team(); // Which player is the process being controlled by? (0-3)
  world_size_x = get_world_x(); // This is the size of the entire playing area, in pixels
  world_size_y = get_world_y();
  initialised = 1; // Now it won't be initialised again
  mode = MODE_CONNECTED; // Means that the process will just sit around while it gets data.
//  set_command(COMREG_DETAILS, DETAIL_HP | DETAIL_IRPT | DETAIL_DATA); // tells operator what details to display
// Now work out how much a pod costs to build:
  build_result = call(METH_NEW,
        MS_PR_NEW_BC_COST_DATA, // MB_PR_NEW_STATUS: MS_PR_NEW_BC_COST_DATA means get data cost.
        1, // MB_PR_NEW_VERTEX1: Vertex of this process that new process will be created at.
        -1, // MB_PR_NEW_VERTEX2: Vertex of child process (-1 means use child process' base_vertex value)
        -1600, // MB_PR_NEW_ANGLE: Angle of child process
        process_start(pod_process), // MB_PR_NEW_START: start address of new process' code
        process_end(pod_process), // MB_PR_NEW_END: end address
        1, // MB_PR_NEW_LINK: 1 means processes will be connected (as long as they both have link methods on mutual vertex)
        0); // MB_PR_NEW_TEMPLATE: Source template index (not relevant here; only used when building from a template)
// Some of the mbank registers aren't relevant to a data cost check, but it doesn't cost too much to set them anyway.
// Here we should be able to ignore build_result, as the call shouldn't fail (unless there's an error in the pod process definition).
  pod_data_cost = get(METH_NEW, MB_PR_NEW_STATUS);
// Work out generator process cost:

// Set addresses to write messages from pods:
  put(METH_LINK_L1, MB_PR_LINK_MESSAGE_ADDRESS, &link_message [0] [0] [0]);
  put(METH_LINK_R1, MB_PR_LINK_MESSAGE_ADDRESS, &link_message [1] [0] [0]);
  put(METH_LINK_L2, MB_PR_LINK_MESSAGE_ADDRESS, &link_message [2] [0] [0]);
  put(METH_LINK_R2, MB_PR_LINK_MESSAGE_ADDRESS, &link_message [3] [0] [0]);
 }

// First check whether the process is complete and has data.
// If it's incomplete and has little or no data, build an allocator:
 if (call(METH_LINK_BACK, MS_PR_LINK_EXISTS) == 0)
	{
  if (get_efficiency() == 100)
  {
	  call(METH_NEW,
        MS_PR_NEW_BC_BUILD,
        4, // MB_PR_NEW_VERTEX1: Vertex of this process that new process will be created at.
        -1, // MB_PR_NEW_VERTEX2: Vertex of child process (-1 means use child process' base_vertex value)
        0, // MB_PR_NEW_ANGLE: Angle of child process
        process_start(allocator), // MB_PR_NEW_START: start address of new process' code
        process_end(allocator), // MB_PR_NEW_END: end address
        1, // MB_PR_NEW_LINK: 1 means processes will be connected (as long as they both have link methods on mutual vertex)
        0); // MB_PR_NEW_TEMPLATE: Source template index (not relevant here; only used when building from a template)
   call(METH_LINK_BACK, MS_PR_LINK_TAKE_DATA, 1); // tells the allocator to give all of its data to spider when disconnecting
  }
	}

	if (call(METH_LINK_BACK, MS_PR_LINK_EXISTS) == 1)
	{
		mode = MODE_CONNECTED;
		if (get_group_members() >= 13
		 && get_data() > 2000)
	 {
   call(METH_LINK_BACK, MS_PR_LINK_DISCONNECT); // does nothing if not connected
   start_wandering();
	 }
	}
	 else
		{
// this could happen if the allocator is destroyed while the spider is connected to it.
			if (mode == MODE_CONNECTED)
    start_wandering();
		}

// Now check whether the process has all of its pods.
// If it doesn't, try to build them if the process has enough data.

  int new_process_vertex;

 if (get_data() >= pod_data_cost)
 {
  if (call(METH_LINK_L1, MS_PR_LINK_EXISTS) == 0)
  {
// tell the pod which position it's in - 0 = left front
asm
{
setrn A 0
setar scope.pod_process::position A
}
// Left front
// Set base vertex and method 0 (link to parent) vertices to 2:
   *(process_start(pod_process) + BCODE_BASE_VERTEX) = 2;
   *(process_start(pod_process) + BCODE_METHOD_VERTEX) = 2;
// Set subprocess vertices to 4 and 5:
   *(process_start(pod_process) + BCODE_METHOD_VERTEX + (BCODE_METHOD_SIZE)) = 4;
   *(process_start(pod_process) + BCODE_METHOD_VERTEX + (BCODE_METHOD_SIZE * 2)) = 5;
// Set move method (method 3):
   *(process_start(pod_process) + BCODE_METHOD_VERTEX + (BCODE_METHOD_SIZE * 3)) = 3;
   *(process_start(pod_process) + BCODE_METHOD_ANGLE + (BCODE_METHOD_SIZE * 3)) = -600;


// Try to build the pod:
   build_result = call(METH_NEW,
        MS_PR_NEW_BC_BUILD, // MB_PR_NEW_STATUS: MS_PR_NEW_BC_BUILD means try to build from bcode
        7, // MB_PR_NEW_VERTEX1: Vertex of this process that new process will be created at.
        -1, // MB_PR_NEW_VERTEX2: Vertex of child process (-1 means use child process' base_vertex value)
//        -630, // MB_PR_NEW_ANGLE: Angle of child process
	       200,
        process_start(pod_process), // MB_PR_NEW_START: start address of new process' code
        process_end(pod_process), // MB_PR_NEW_END: end address
        1, // MB_PR_NEW_LINK: 1 means processes will be connected (as long as they both have link methods on mutual vertex)
        0); // MB_PR_NEW_TEMPLATE: Source template index (not relevant here; only used when building from a template)
   goto finished_building_pods;
  }

  if (call(METH_LINK_R1, MS_PR_LINK_EXISTS) == 0)
  {
// tell the pod which position it's in
asm
{
setrn A 1
setar scope.pod_process::position A
}
// Right front
// Set base vertex and method 0 (link to parent) vertices to 4:
   *(process_start(pod_process) + BCODE_BASE_VERTEX) = 4;
   *(process_start(pod_process) + BCODE_METHOD_VERTEX) = 4;
// Set subprocess vertices to 2 and 1:
   *(process_start(pod_process) + BCODE_METHOD_VERTEX + (BCODE_METHOD_SIZE)) = 2;
   *(process_start(pod_process) + BCODE_METHOD_VERTEX + (BCODE_METHOD_SIZE * 2)) = 1;
// Set move method (method 3):
   *(process_start(pod_process) + BCODE_METHOD_VERTEX + (BCODE_METHOD_SIZE * 3)) = 3;
   *(process_start(pod_process) + BCODE_METHOD_ANGLE + (BCODE_METHOD_SIZE * 3)) = 600;

// Try to build the pod:
   build_result = call(METH_NEW,
        MS_PR_NEW_BC_BUILD, // MB_PR_NEW_STATUS: MS_PR_NEW_BC_BUILD means try to build from bcode
        1, // MB_PR_NEW_VERTEX1: Vertex of this process that new process will be created at.
        -1, // MB_PR_NEW_VERTEX2: Vertex of child process (-1 means use child process' base_vertex value)
//        630, // MB_PR_NEW_ANGLE: Angle of child process
        -200,
        process_start(pod_process), // MB_PR_NEW_START: start address of new process' code
        process_end(pod_process), // MB_PR_NEW_END: end address
        1, // MB_PR_NEW_LINK: 1 means processes will be connected (as long as they both have link methods on mutual vertex)
        0); // MB_PR_NEW_TEMPLATE: Source template index (not relevant here; only used when building from a template)
   goto finished_building_pods;
  }

  if (call(METH_LINK_L2, MS_PR_LINK_EXISTS) == 0)
  {
asm
{
setrn A 2
setar scope.pod_process::position A
}
// left back
// Set base vertex and method 0 (link to parent) vertices to 1:
   *(process_start(pod_process) + BCODE_BASE_VERTEX) = 1;
   *(process_start(pod_process) + BCODE_METHOD_VERTEX) = 1;
// Set subprocess vertices to 4 and 5:
   *(process_start(pod_process) + BCODE_METHOD_VERTEX + (BCODE_METHOD_SIZE)) = 4;
   *(process_start(pod_process) + BCODE_METHOD_VERTEX + (BCODE_METHOD_SIZE * 2)) = 5;
// Set move method (method 3):
   *(process_start(pod_process) + BCODE_METHOD_VERTEX + (BCODE_METHOD_SIZE * 3)) = 3;
   *(process_start(pod_process) + BCODE_METHOD_ANGLE + (BCODE_METHOD_SIZE * 3)) = 600;

// Try to build the pod:
   build_result = call(METH_NEW,
        MS_PR_NEW_BC_BUILD, // MB_PR_NEW_STATUS: MS_PR_NEW_BC_BUILD means try to build from bcode
        5, // MB_PR_NEW_VERTEX1: Vertex of this process that new process will be created at.
        -1, // MB_PR_NEW_VERTEX2: Vertex of child process (-1 means use child process' base_vertex value)
        -200, // MB_PR_NEW_ANGLE: Angle of child process
        process_start(pod_process), // MB_PR_NEW_START: start address of new process' code
        process_end(pod_process), // MB_PR_NEW_END: end address
        1, // MB_PR_NEW_LINK: 1 means processes will be connected (as long as they both have link methods on mutual vertex)
        0); // MB_PR_NEW_TEMPLATE: Source template index (not relevant here; only used when building from a template)
   goto finished_building_pods;
  }

  if (call(METH_LINK_R2, MS_PR_LINK_EXISTS) == 0)
  {
asm
{
setrn A 3
setar scope.pod_process::position A
}
// Right back
// Set base vertex and method 0 (link to parent) vertices to 5:
   *(process_start(pod_process) + BCODE_BASE_VERTEX) = 5;
   *(process_start(pod_process) + BCODE_METHOD_VERTEX) = 5;
// Set subprocess vertices to 2 and 1:
   *(process_start(pod_process) + BCODE_METHOD_VERTEX + (BCODE_METHOD_SIZE)) = 2;
   *(process_start(pod_process) + BCODE_METHOD_VERTEX + (BCODE_METHOD_SIZE * 2)) = 1;
// Set move method (method 3):
   *(process_start(pod_process) + BCODE_METHOD_VERTEX + (BCODE_METHOD_SIZE * 3)) = 3;
   *(process_start(pod_process) + BCODE_METHOD_ANGLE + (BCODE_METHOD_SIZE * 3)) = -600;

// Try to build the pod:
   build_result = call(METH_NEW,
        MS_PR_NEW_BC_BUILD, // MB_PR_NEW_STATUS: MS_PR_NEW_BC_BUILD means try to build from bcode
        3, // MB_PR_NEW_VERTEX1: Vertex of this process that new process will be created at.
        -1, // MB_PR_NEW_VERTEX2: Vertex of child process (-1 means use child process' base_vertex value)
        200, // MB_PR_NEW_ANGLE: Angle of child process
        process_start(pod_process), // MB_PR_NEW_START: start address of new process' code
        process_end(pod_process), // MB_PR_NEW_END: end address
        1, // MB_PR_NEW_LINK: 1 means processes will be connected (as long as they both have link methods on mutual vertex)
        0); // MB_PR_NEW_TEMPLATE: Source template index (not relevant here; only used when building from a template)
   goto finished_building_pods;
  }

 } // end pod build tests

finished_building_pods:

// What the process does next depends on which mode it is in.
 switch(mode)
 {


  case MODE_WANDER:
  	int com0, com1;
  	com0 = get_command(0);
  	com1 = get_command(1);
  	if (com0 == 0)
			{
				if (mode_count <= 0)
 				start_wandering();
 				 else
							mode_count --;
			}
			 else
				{
					target_x = com0;
					target_y = com1;
				}
   break;

  case MODE_CONNECTED:
			 goto finished; // don't run MOVE methods


 } // end of switch(mode)

// If process is in MODE_IDLE, it will have jumped directly to the finished label below.

 target_angle = atan2(target_y - y, target_x - x);
 turn_dir = turn_direction(angle, target_angle); // turn_direction() is a built-in MATHS method function.
    // It returns the direction of the shortest turning distance between angle and target_angle, taking bounds into account.
    // Return values are -1, 0 (if angles are equal) or 1
 angle_diff = angle_difference(angle, target_angle); // angle_difference returns the absolute difference between two angles

 int future_turn_dir;

// future_turn_dir is the direction it would have to rotate to point towards target_angle if it turned in turn_dir,
//  taking account of its current spin:
 future_turn_dir = turn_direction(angle + (turn_dir * 20) + (spin * 8), target_angle); // (100 and 8 are approximation factors)
// If future_turn_dir is different, that means we will overshoot the target. So don't turn:
 if (turn_dir != future_turn_dir
			|| angle_diff < 1400)
 {
  turn_dir = 0;
 }


// Now we know which way we're turning, we need to tell the move methods (i.e. engines) what to do.

 if (turn_dir > 0) // If turn_dir > 0, the process is trying to rotate clockwise. So we suppress this engine for a while:
	{
  call(METH_LINK_R1, MS_PR_LINK_MESSAGE, LM_MOVE, angle_diff / 80);
  call(METH_LINK_R2, MS_PR_LINK_MESSAGE, LM_MOVE, angle_diff / 80);
	}
  else
		{
   call(METH_LINK_R1, MS_PR_LINK_MESSAGE, LM_MOVE, 0);
   call(METH_LINK_R2, MS_PR_LINK_MESSAGE, LM_MOVE, 0);
		}

// Now we do the same for the left-hand engine:
 if (turn_dir < 0)
	{
  call(METH_LINK_L1, MS_PR_LINK_MESSAGE, LM_MOVE, angle_diff / 80);
  call(METH_LINK_L2, MS_PR_LINK_MESSAGE, LM_MOVE, angle_diff / 80);
	}
  else
		{
   call(METH_LINK_L1, MS_PR_LINK_MESSAGE, LM_MOVE, 0);
   call(METH_LINK_L2, MS_PR_LINK_MESSAGE, LM_MOVE, 0);
		}

 finished:

 // try to self-repair (does nothing if the process isn't damaged):
 if (get_irpt() > 800)
  call(METH_RESTORE, 2);

}

// This function makes the process start wandering and gives it a random destination:
static void start_wandering(void)
{

     mode = MODE_WANDER;
     target_x = random(world_size_x - 1800) + 900;
     target_y = random(world_size_y - 1800) + 900;

     mode_count = 240; // will wander for 240 cycles (about 2 mins)

}

// A very bad pseudorandom number generator that might just be good enough for this purpose:
static int random(int max)
{

// As this is a static function, seed is a static variable that is initialised when the process is created.
 int seed = 7477;
 seed += 4639 + get_x() + get_y();

 return abs(seed) % max;

}


process pod_process
{

interface
{
 PROGRAM_TYPE_PROCESS, SHAPE_6HEXAGON, 3, 0,
 {
  METH_LINK_PARENT: {MT_PR_LINK, 2, 0}, // this is always a link to the parent process, although the vertex may be changed
  METH_LINK_SUB1: {MT_PR_LINK, 4, 0},
  METH_LINK_SUB2: {MT_PR_LINK, 5, 0},
  METH_MOVE: {MT_PR_MOVE, 3, 0, 4},
  METH_STD: {MT_PR_STD},
  METH_MATHS: {MT_PR_MATHS},
  METH_RESTORE: {MT_PR_RESTORE},
  METH_NEW_SUB: {MT_PR_NEW_SUB},
  {0}, // space for new method
  METH_IRPT: {MT_PR_IRPT, 0, 0, 4},
 }
}

int position; // this is set by the main process just before this process is built.
int left_right; // 0 if pod is on left, 1 if on right
int front_back; // 0 if pod is at front, 1 if at back
enum
{
POS_L1,
POS_R1,
POS_L2,
POS_R2
};

int team;
int initialised;
int pod_data_cost;
int process_complete;
int link_message [LINK_MESSAGES] [LINK_MESSAGE_SIZE]; // The LINK method can send and receive messages consisting of two ints.
int counter;
int verbose;

int angle;
int speed_x;
int speed_y;

int build_result;

// Scan values - used to scan for other nearby processes
int scan_result [8];
int scan_bitmask;

static int check_pod(int link_method, int pod_shape, int link_vertex, int pod_vertex, int pod_angle,
																					int process_start_address, int process_end_address);
static int random(int max);

process end_process;
process p_pod;
process s_pod;

static void main(void)
{

 angle = get_angle();
 speed_x = get_speed_x();
 speed_y = get_speed_y();
 counter++;

 if (initialised == 0)
 {
  team = get_team();
  build_result = call(METH_NEW_SUB,
        MS_PR_NEW_BC_COST_DATA, // MB_PR_NEW_STATUS: MS_PR_NEW_BC_COST_DATA means get data cost.
        1, // MB_PR_NEW_VERTEX1: Vertex of this process that new process will be created at.
        -1, // MB_PR_NEW_VERTEX2: Vertex of child process (-1 means use child process' base_vertex value)
        -1600, // MB_PR_NEW_ANGLE: Angle of child process
        process_start(p_pod), // MB_PR_NEW_START: start address of new process' code
        process_end(p_pod), // MB_PR_NEW_END: end address
        1, // MB_PR_NEW_LINK: 1 means processes will be connected (as long as they both have link methods on mutual vertex)
        0); // MB_PR_NEW_TEMPLATE: Source template index (not relevant here; only used when building from a template)
// Some of the mbank registers aren't relevant to a data cost check, but it doesn't cost too much to set them anyway.
// Here we should be able to ignore build_result, as the call shouldn't fail (unless there's an error in the pod process definition).
  pod_data_cost = get(METH_NEW_SUB, MB_PR_NEW_STATUS);
//  set_command(COMREG_DETAILS, DETAIL_HP | DETAIL_IRPT | DETAIL_SECONDARY); // tells operator what details to display
  initialised = 1;
  put(METH_LINK_PARENT, MB_PR_LINK_MESSAGE_ADDRESS, &link_message [0] [0]);
// now set up other details:
  switch(position)
  {
		 case POS_L1:
			 left_right = 0;
			 front_back = 0;
			 break;
		 case POS_R1:
			 left_right = 1;
			 front_back = 0;
			 break;
		 case POS_L2:
			 left_right = 0;
			 front_back = 1;
			 break;
		 case POS_R2:
			 left_right = 1;
			 front_back = 1;
			 break;
  }
 }

// pod will send parent process a message saying it's complete
// when all shield pods are built and it has enough data to build another one.
 process_complete = 0;

 if (get_data() >= pod_data_cost)
 {
//  *(process_start(p_pod) + BCODE_SHAPE) = SHAPE_4IRREG_R;
  if (front_back == 0)
   process_complete += check_pod(METH_LINK_SUB1, SHAPE_6LONG, *(6+(METH_LINK_SUB1*6)+1), 1, 0,
																																process_start(s_pod), process_end(s_pod)); // *(6+(METH_LINK_SUB1*6)+1) gets the vertex this link is on
				else
     process_complete += check_pod(METH_LINK_SUB1, SHAPE_6LONG, *(6+(METH_LINK_SUB1*6)+1), 1, 0,
																																process_start(p_pod), process_end(p_pod)); // *(6+(METH_LINK_SUB1*6)+1) gets the vertex this link is on

     process_complete += check_pod(METH_LINK_SUB2, SHAPE_6LONG, *(6+(METH_LINK_SUB2*6)+1), 1, 0,
  																																process_start(p_pod), process_end(p_pod)); // *(6+(METH_LINK_SUB1*6)+1) gets the vertex this link is on

 }

			if (call(METH_LINK_PARENT, MS_PR_LINK_RECEIVED) != 0
				&& link_message [0] [0] == LM_MOVE)
			{
		  put(METH_MOVE, MB_PR_MOVE_RATE, 100);
		  put(METH_MOVE, MB_PR_MOVE_COUNTER, 16);
		  put(METH_MOVE, MB_PR_MOVE_DELAY, link_message [0] [1]);
			}

// finally, try to self-repair (does nothing if the process isn't damaged):
 if (get_irpt() > 2000)
  call(METH_RESTORE, 2);

} // end main




static int check_pod(int link_method, int pod_shape, int link_vertex, int pod_vertex, int pod_angle,
																					int process_start_address, int process_end_address)
{

 put_index((link_method*4) + MB_PR_LINK_STATUS, MS_PR_LINK_EXISTS);

 if (call(link_method) == 0) // pod doesn't exist
 {
// Try to build the pod
// First we need to set the pod's shape and link method vertex to the correct values
   build_result = call(METH_NEW_SUB,
        MS_PR_NEW_BC_BUILD, // MB_PR_NEW_STATUS: MS_PR_NEW_BC_BUILD means try to build from bcode
        link_vertex, // MB_PR_NEW_VERTEX1: Vertex of this process that new process will be created at.
        -1, // MB_PR_NEW_VERTEX2: Vertex of child process (-1 means use child process' base_vertex value)
        0, // MB_PR_NEW_ANGLE: Angle of child process
        process_start_address, // MB_PR_NEW_START: start address of new process' code
        process_end_address, // MB_PR_NEW_END: end address
        1, // MB_PR_NEW_LINK: 1 means processes will be connected (as long as they both have hold methods on mutual vertex)
        0); // MB_PR_NEW_TEMPLATE: Source template index (not relevant here; only used when building from a template)
   if (build_result != MR_NEW_SUCCESS)
			 return 0;
 }

 return 1; // pod exists

}


// A very bad pseudorandom number generator that might just be good enough for this purpose:
static int random(int max)
{

// As this is a static function, seed is a static variable that is initialised when the process is created.
 int seed = 7477;
 seed += 4639 + get_speed_x() + get_speed_y();

 return abs(seed) % max;

}



process p_pod
{
// All this process does is scan a nearby area and protect its factory, while repairing itself if needed.

interface
{
 PROGRAM_TYPE_PROCESS, SHAPE_6POINTY, 3, 0, // if shape or size are changed, fix turret_x/y calculations below
 {
  METH_LINK_PARENT: {MT_PR_LINK, 0},
  METH_STD: {MT_PR_STD},
  METH_DPACKET1: {MT_PR_DPACKET, 2, 0, 0, 1, 3},
  METH_DPACKET2: {MT_PR_DPACKET, 4, 0, 0, 1, 3},
//  METH_DPACKET3: {MT_PR_DPACKET, 4, 0, 0, 1, 2},
  METH_MATHS: {MT_PR_MATHS},
  METH_DESIGNATE: {MT_PR_DESIGNATE}, // designator. allows process to keep track of a target acquired through scan
  METH_RESTORE: {MT_PR_RESTORE},
  METH_VIRTUAL: {MT_PR_VIRTUAL},
  METH_SCAN: {MT_PR_SCAN, 0, 0, 2}, // generates irpt
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
int speed_x, speed_y;
int spin;

// Behaviour/mode values - these determine the process' current behaviour
int mode; // Behavioural mode of the process (see the MODE enums below)
int mode_count; // Time the process will stay in current mode (only relevant for some modes)
int attack_x;
int attack_y;
int target_speed_x;
int target_speed_y;
int link_message [LINK_MESSAGES] [LINK_MESSAGE_SIZE]; // The LINK method can send and receive messages consisting of two ints.

// Scan values - used to scan for other nearby processes
int scan_result [32]; // actually only 4 values are used, but may want to extend later.
int scan_bitmask; // set during initialisation (can't set it at compile-time because the process' team is not known)

int dpacket_vertex [3];
int dpacket_vertex_angle [3];
int dpacket_vertex_dist [3];
int dpacket_vertex_angle_min [3];
int dpacket_vertex_angle_max [3];


static void scan_for_target(int offset_angle, int offset_dist, int scan_size);
static int run_dpacket_method_multi(int m, int vertex_angle, int vertex_dist, int min_angle, int max_angle,
																																				int designate_method, int packet_speed, int delay);
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

 angle = get_angle();
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
  mode = MODE_IDLE;
// We can also set up some things that normally would be updated each cycle, but don't need to be as this process is immobile:
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
/*
	 dpacket_vertex [2] = *(6+(METH_DPACKET3*6)+1);
  dpacket_vertex_angle [2] = get_vertex_angle(*(6+(METH_DPACKET3*6)+1));
  dpacket_vertex_dist [2] = get_vertex_dist(*(6+(METH_DPACKET3*6)+1));
  dpacket_vertex_angle_min [2] = get_vertex_angle_min(*(6+(METH_DPACKET3*6)+1));
  dpacket_vertex_angle_max [2] = get_vertex_angle_max(*(6+(METH_DPACKET3*6)+1));;*/

  scan_bitmask = 0b1111 ^ (1<<team); // scan_bitmask is used by the scanner to find processes that belong to a different team.

  initialised = 1; // Now it won't be initialised again
 }

// call(METH_RESTORE, 2);
 call(METH_VIRTUAL, MS_PR_VIRTUAL_CHARGE, 100);

 scan_for_target(ANGLE_2, 300, 10000);

/*
// parent will scan for targets and tell this process if it finds any, using the link method to send messages
	if (call(METH_LINK_PARENT, MS_PR_LINK_RECEIVED) != 0)
	{
// acquire target using new coordinates:
  if (call(METH_DESIGNATE, MS_PR_DESIGNATE_ACQUIRE, link_message [0] [0] - x, link_message [0] [1] - y) != 0)
		{
			mode = MODE_ATTACK;
		}
	}*/

// What the process does next depends on which mode it is in.
 switch(mode)
 {
  case MODE_IDLE:
   put(METH_DPACKET1, MB_PR_DPACKET_ANGLE, 0); // tells the method to return to centre
   put(METH_DPACKET2, MB_PR_DPACKET_ANGLE, 0); // tells the method to return to centre
//   put(METH_DPACKET3, MB_PR_DPACKET_ANGLE, 0); // tells the method to return to centre
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
																														dpacket_vertex_angle_min [0], dpacket_vertex_angle_max [0], METH_DESIGNATE, 8 * 16, 1);
     run_dpacket_method_multi(METH_DPACKET2, dpacket_vertex_angle [1], dpacket_vertex_dist [1],
																														dpacket_vertex_angle_min [1], dpacket_vertex_angle_max [1], METH_DESIGNATE, 8 * 16, 4);
/*     run_dpacket_method_multi(METH_DPACKET3, dpacket_vertex_angle [2], dpacket_vertex_dist [2],
																														dpacket_vertex_angle_min [2], dpacket_vertex_angle_max [1], METH_DESIGNATE, 8 * 16, 8);*/

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


// This function uses the scan method to find a target.
//  - it is designed for a process with a single designate method (otherwise, see scan_for_target_multi() in other processes)
// The scan can be off-centre from the process (although this can reduce the size of the scan):
//  offset_angle is an offset from the process's angle
//  offset_dist is the distance from the centre
// scan_size is the size of the scan (can be set to a high value as the method will just cap it if it's too high)
// Assumes that angle and scan_bitmask variables are set correctly (see the main() function above)
// Assumes that scan_result is an array with at least 2 elements.
// returns 1 if target found and acquired, 0 otherwise.
static void scan_for_target(int offset_angle, int offset_dist, int scan_size)
{

 int scan_x;
 int scan_y;
 scan_x = cos(angle + offset_angle, offset_dist);
 scan_y = sin(angle + offset_angle, offset_dist);

 if (call(METH_SCAN, // method's index
          MS_PR_SCAN_SCAN, // status register (MB_PR_SCAN_STATUS) - tells the method to run scan
          &scan_result [0], // memory address to put results of scan (MB_PR_SCAN_START_ADDRESS)
          1, // number of targets to find (MB_PR_SCAN_NUMBER) - scanner will stop after finding 1
          scan_x, // x offset of scan centre (MB_PR_SCAN_X1)
          scan_y, // y offset of scan centre (MB_PR_SCAN_Y1) - scan is centred on process
          scan_size, // size of scan (in pixels) (MBANK_PR_SCAN_SIZE)
          0, // (MBANK_PR_SCAN_Y2) - not relevant to this type of scan
          scan_bitmask) // this bitmask indicates which targets will be found (MBANK_PR_SCAN_BITFIELD_WANT) - set up at initialisation
					  != 0) // scan returns 0 if nothing found
 {
// We found something! So now we tell the DESIGNATE method to lock onto the target.
//  (the DESIGNATE method allows the process to keep track of its target between execution cycles)
  if (call(METH_DESIGNATE,
						    	MS_PR_DESIGNATE_ACQUIRE,
							    scan_result [0] + scan_x, // need to add scan_x because scan_result is an offset from the centre of the scan, not the process.
							    scan_result [1] + scan_y)
						== 1) // The DESIGNATE method returns 1 if it is successful (which should always be the case here, but let's check anyway):
  {
   mode = MODE_ATTACK;
  }
 }


}




static int run_dpacket_method_multi(int m, int vertex_angle, int vertex_dist, int min_angle, int max_angle, int designate_method, int packet_speed, int delay)
{

// int attack_x, attack_y; // location of target with respect to process
 int source_x, source_y; // location of packet source vertex with respect to process
 int old_source_x, old_source_y; // location of packet source vertex with respect to process, last tick
 int source_speed_x, source_speed_y; // speed of packet source vertex
 int dist; // distance to target
 int i;
 int method_angle;
 int target_angle;
/*
// prepare to use the designate method to locate its current target:
 put_index(designate_method*4, MS_PR_DESIGNATE_LOCATE);

 if (call(designate_method) == 0)
  goto cancel_attack; // target out of range or destroyed.

 attack_x = get_index((designate_method*4) + 1); // gets DESIGNATE method's registers
 attack_y = get_index((designate_method*4) + 2); //  - these registers contain target's location relative to this process

 put_index(designate_method*4, MS_PR_DESIGNATE_SPEED);

 call(designate_method);

 target_speed_x = get_index((designate_method*4) + 1); // gets DESIGNATE method's registers
 target_speed_y = get_index((designate_method*4) + 2); // gets DESIGNATE method's registers
*/
// Find the distance between the process and its target:
 dist = hypot(attack_x, attack_y); // hypot is a built-in function that calls the MATHS method.
 if (dist > 900)
  goto cancel_attack; // too far away

// Now we need to aim. First we work out the location and speed of the source vertex:
//  (can't use the process's speed as it may be spinning)

 source_x = cos(angle + vertex_angle, vertex_dist); // process itself is at 0,0
 source_y = sin(angle + vertex_angle, vertex_dist); // process itself is at 0,0
 old_source_x = (0 - speed_x) + cos(angle + vertex_angle - spin, vertex_dist);
 old_source_y = (0 - speed_y) + sin(angle + vertex_angle - spin, vertex_dist);
 source_speed_x = source_x - old_source_x;
 source_speed_y = source_y - old_source_y;

// now find the angle to the target, adjusting for relative speeds:
 target_angle = lead_target(attack_x - source_x, // need to adjust target offset by vertex location
																		          attack_y - source_y,
																						      target_speed_x - source_speed_x, // relative speed - ideally we'd calculate the speed of the vertex, but this is good enough.
																						      target_speed_y - source_speed_y,
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

/*

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

*/

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

//#subdefine LEAD_ITERATIONS 2
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





process s_pod
{


interface
{
 PROGRAM_TYPE_PROCESS, SHAPE_6POINTY, 3, 3, // if shape or size are changed, fix turret_x/y calculations below
 {
  METH_LINK_PARENT: {MT_PR_LINK, 3},
  METH_STD: {MT_PR_STD},
  METH_DSTREAM: {MT_PR_DSTREAM, 0, 0, 1, 3, 0},
//  METH_DPACKET3: {MT_PR_DPACKET, 4, 0, 0, 1, 2},
  METH_MATHS: {MT_PR_MATHS},
  METH_DESIGNATE: {MT_PR_DESIGNATE}, // designator. allows process to keep track of a target acquired through scan
  METH_RESTORE: {MT_PR_RESTORE},
  METH_VIRTUAL: {MT_PR_VIRTUAL},
  METH_SCAN: {MT_PR_SCAN, 0, 0, 2}, // generates irpt
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
int speed_x, speed_y;
int spin;

// Behaviour/mode values - these determine the process' current behaviour
int mode; // Behavioural mode of the process (see the MODE enums below)
int mode_count; // Time the process will stay in current mode (only relevant for some modes)
int attack_x;
int attack_y;
int target_speed_x;
int target_speed_y;
int link_message [LINK_MESSAGES] [LINK_MESSAGE_SIZE]; // The LINK method can send and receive messages consisting of two ints.

// Scan values - used to scan for other nearby processes
int scan_result [32]; // actually only 4 values are used, but may want to extend later.
int scan_bitmask; // set during initialisation (can't set it at compile-time because the process' team is not known)


static void scan_for_target(int offset_angle, int offset_dist, int scan_size);
static int run_dstream_method_multi(int m, int vertex_angle, int vertex_dist, int min_angle,
																																				int max_angle, int designate_method, int packet_speed, int delay);
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

 angle = get_angle();
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
  mode = MODE_IDLE;
// We can also set up some things that normally would be updated each cycle, but don't need to be as this process is immobile:
//  turret_x_offset = cos(angle, data(DATA_SHAPE_VERTEX_DIST, SHAPE_6LONG, 3, 0));
//  turret_y_offset = sin(angle, data(DATA_SHAPE_VERTEX_DIST, SHAPE_6LONG, 3, 0));
  put(METH_LINK_PARENT, MB_PR_LINK_MESSAGE_ADDRESS, &link_message [0] [0]);

  scan_bitmask = 0b1111 ^ (1<<team); // scan_bitmask is used by the scanner to find processes that belong to a different team.

  initialised = 1; // Now it won't be initialised again
 }

// call(METH_RESTORE, 2);
 call(METH_VIRTUAL, MS_PR_VIRTUAL_CHARGE, 100);

 scan_for_target(0, 300, 10000);

// What the process does next depends on which mode it is in.
 switch(mode)
 {
  case MODE_IDLE:
   put(METH_DSTREAM, MB_PR_DSTREAM_ANGLE, 0); // tells the method to return to centre
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
     run_dstream_method_multi(METH_DSTREAM, data(DATA_SHAPE_VERTEX_ANGLE, SHAPE_6POINTY, 0),
																														data(DATA_SHAPE_VERTEX_DIST, SHAPE_6POINTY, 3, 0),
																														data(DATA_SHAPE_VERTEX_ANGLE_MIN, SHAPE_6POINTY, 0),
																														data(DATA_SHAPE_VERTEX_ANGLE_MAX, SHAPE_6POINTY, 0), METH_DESIGNATE, 8 * 16, 1);

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


// This function uses the scan method to find a target.
//  - it is designed for a process with a single designate method (otherwise, see scan_for_target_multi() in other processes)
// The scan can be off-centre from the process (although this can reduce the size of the scan):
//  offset_angle is an offset from the process's angle
//  offset_dist is the distance from the centre
// scan_size is the size of the scan (can be set to a high value as the method will just cap it if it's too high)
// Assumes that angle and scan_bitmask variables are set correctly (see the main() function above)
// Assumes that scan_result is an array with at least 2 elements.
// returns 1 if target found and acquired, 0 otherwise.
static void scan_for_target(int offset_angle, int offset_dist, int scan_size)
{

 int scan_x;
 int scan_y;
 scan_x = cos(angle + offset_angle, offset_dist);
 scan_y = sin(angle + offset_angle, offset_dist);

 if (call(METH_SCAN, // method's index
          MS_PR_SCAN_SCAN, // status register (MB_PR_SCAN_STATUS) - tells the method to run scan
          &scan_result [0], // memory address to put results of scan (MB_PR_SCAN_START_ADDRESS)
          1, // number of targets to find (MB_PR_SCAN_NUMBER) - scanner will stop after finding 1
          scan_x, // x offset of scan centre (MB_PR_SCAN_X1)
          scan_y, // y offset of scan centre (MB_PR_SCAN_Y1) - scan is centred on process
          scan_size, // size of scan (in pixels) (MBANK_PR_SCAN_SIZE)
          0, // (MBANK_PR_SCAN_Y2) - not relevant to this type of scan
          scan_bitmask) // this bitmask indicates which targets will be found (MBANK_PR_SCAN_BITFIELD_WANT) - set up at initialisation
					  != 0) // scan returns 0 if nothing found
 {
// We found something! So now we tell the DESIGNATE method to lock onto the target.
//  (the DESIGNATE method allows the process to keep track of its target between execution cycles)
  if (call(METH_DESIGNATE,
						    	MS_PR_DESIGNATE_ACQUIRE,
							    scan_result [0] + scan_x, // need to add scan_x because scan_result is an offset from the centre of the scan, not the process.
							    scan_result [1] + scan_y)
						== 1) // The DESIGNATE method returns 1 if it is successful (which should always be the case here, but let's check anyway):
  {
   mode = MODE_ATTACK;
  }
 }


}




static int run_dstream_method_multi(int m, int vertex_angle, int vertex_dist, int min_angle, int max_angle, int designate_method, int packet_speed, int delay)
{

// int attack_x, attack_y; // location of target with respect to process
 int source_x, source_y; // location of packet source vertex with respect to process
 int old_source_x, old_source_y; // location of packet source vertex with respect to process, last tick
 int source_speed_x, source_speed_y; // speed of packet source vertex
 int dist; // distance to target
 int i;
 int method_angle;
 int target_angle;

// Find the distance between the process and its target:
 dist = hypot(attack_x, attack_y); // hypot is a built-in function that calls the MATHS method.
 if (dist > 900)
  goto cancel_attack; // too far away

// Now we need to aim. First we work out the location and speed of the source vertex:
//  (can't use the process's speed as it may be spinning)

 source_x = cos(angle + vertex_angle, vertex_dist); // process itself is at 0,0
 source_y = sin(angle + vertex_angle, vertex_dist); // process itself is at 0,0
 old_source_x = (0 - speed_x) + cos(angle + vertex_angle - spin, vertex_dist);
 old_source_y = (0 - speed_y) + sin(angle + vertex_angle - spin, vertex_dist);
 source_speed_x = source_x - old_source_x;
 source_speed_y = source_y - old_source_y;

// now find the angle to the target, adjusting for relative speeds:
 target_angle = lead_target(attack_x - source_x, // need to adjust target offset by vertex location
																		          attack_y - source_y,
																						      target_speed_x - source_speed_x, // relative speed - ideally we'd calculate the speed of the vertex, but this is good enough.
																						      target_speed_y - source_speed_y,
																		          angle + vertex_angle, // base_angle
																		          packet_speed,
																		          delay);

 if (target_angle < min_angle
  || target_angle > max_angle)
  goto cancel_attack; // out of dpacket method's rotation range

 method_angle = call(m); // this returns the method's angle as an offset from the angle pointing directly from the centre of the process.

 put_index((m * 4) + MB_PR_DSTREAM_ANGLE, target_angle); // tells the method to rotate to this angle

// make sure the method is pointing in approx the right direction
 if (angle_difference(target_angle, method_angle) < ANGLE_8) // this returns the magnitude (unsigned) of the difference between the two angles.
 {
// Make sure we've got enough irpt to fire:
  if (get_irpt() > 100) // get_irpt() is a built-in INFO function.
   put_index((m * 4) + MB_PR_DSTREAM_FIRE, delay); // this tells the method to fire after <delay> ticks.
    // delay should be at least 1 (can be more to stagger the firing of multiple methods)
 }

 return 1; // attack succeeded, or failed in a way that shouldn't stop the process from trying again next cycle

 cancel_attack: // stop attacking
  put_index((m*4) + MB_PR_DSTREAM_ANGLE, 0); // return to centre
  return 0; // attack failed - stop trying

}

/*

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

*/

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

//#subdefine LEAD_ITERATIONS 2
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


} // end process s_pod






} // end process pod_process



process allocator
{

interface
{
 PROGRAM_TYPE_PROCESS, SHAPE_4POINTY, 2, 0, // program type, shape, size (from 0-3), base_vertex
 {
  METH_LINK: {MT_PR_LINK, 0},
  METH_ALLOC: {MT_PR_ALLOCATE, 2},
  METH_IRPT: {MT_PR_IRPT},
  METH_NEW: {MT_PR_NEW},
  {0},
  {MT_PR_STORAGE, 0, 0, 3}
 }
}


static void main(void)
{

// Builds a series of processes from template 1

	call(METH_ALLOC);

	if (call(METH_LINK, MS_PR_LINK_EXISTS) == 0)
	{

  call(METH_NEW,
      MS_PR_NEW_T_BUILD, // MB_PR_NEW_STATUS: means: after this process finishes executing, try to build
      0, // MB_PR_NEW_VERTEX1: Vertex of this process that new process will be created at.
      -1, // MB_PR_NEW_VERTEX2: Vertex of child process (-1 means use child process' base_vertex value)
      0, // MB_PR_NEW_ANGLE: Angle of child process
      0, // MB_PR_NEW_START: Start address
      BCODE_SIZE_PROCESS - 1, // MB_PR_NEW_END: End address
      0, // MB_PR_NEW_LINK: 1 means try to connect to new process
      1); // MB_PR_NEW_TEMPLATE: Source template index

	}

}


}




