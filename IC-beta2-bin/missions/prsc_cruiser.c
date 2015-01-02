


/*

prsc_cruiser.c

This is a complex multi-part process which is very effective at fighting off waves
of small enemies without taking permanent damage.
It doesn't accept commands or interact in any way with a client/observer program.

*/

#ifndef STANDARD_COMMANDS
#include "stand_coms.c"
#endif

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
  METH_MATHS: {MT_PR_MATHS}, // maths method. allows trigonometry etc.
  METH_STD: {MT_PR_STD}, // information method. allows access to process's current properties (location etc)
  METH_IRPT: {MT_PR_IRPT, 0, 0, 2}, // generates irpt. functions automatically (although can be configured not to)
  METH_LINK_BACK: {MT_PR_LINK, 4},
  METH_NEW: {MT_PR_NEW},
  {0}, // space for new method
  METH_COM: {MT_PR_COMMAND},
  METH_MOVE1: {MT_PR_MOVE, 2, data(DATA_SHAPE_VERTEX_ANGLE, SHAPE_8OCTAGON, 2), 3},
  METH_MOVE2: {MT_PR_MOVE, 6, data(DATA_SHAPE_VERTEX_ANGLE, SHAPE_8OCTAGON, 6), 3},
  METH_RESTORE: {MT_PR_RESTORE},
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
MODE_IDLE, // Process is sitting around, scanning for something to attack
MODE_MOVE, // Process is moving to a known location
MODE_ATTACK_COMMAND, // Process has been told to attack
MODE_ARRIVED

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
static void process_move(void);
static void build_pods(void);
static int random(int max);

process pod_process;
process generator;

// Remember that main is called every time the program is run.
static void main(void)
{

// First we get some basic information about the process' current state.
// The "get_*" functions are built-in shortcuts that the compiler turns into calls
//  to the INFO method.
 angle = get_angle(); // this process is reversed, so that its vertex 0 points backwards. Need to adjust the angle by 180 deg (ANGLE_2)
 x = get_x();
 y = get_y();
 speed_x = get_speed_x();
 speed_y = get_speed_y();
 spin = get_spin();
// operator can asked the process to tell the user what it's doing by setting the verbose bit of the client status command register:
 verbose = get_command_bit(COMREG_CLIENT_STATUS, COM_CLIENT_ST_VERBOSE);
 set_command_bit_0(COMREG_CLIENT_STATUS, COM_CLIENT_ST_VERBOSE); // clear the verbose command register (the operator will reset it to 1 if needed)

 if (initialised == 0)
 {
  team = get_team(); // Which player is the process being controlled by? (0-3)
  world_size_x = get_world_x(); // This is the size of the entire playing area, in pixels
  world_size_y = get_world_y();
  initialised = 1; // Now it won't be initialised again
  mode = MODE_IDLE; // Means that the process will just sit around while it gets data.
  set_command_bit_1(COMREG_PROCESS_DETAILS, COM_PROCESS_DET_COMMAND); // tells operator that this process accepts commands.
  set_command_bit_1(COMREG_PROCESS_DETAILS, COM_PROCESS_DET_HP); // tells operator what details to display
  set_command_bit_1(COMREG_PROCESS_DETAILS, COM_PROCESS_DET_IRPT); // tells operator what details to display
  set_command_bit_1(COMREG_PROCESS_DETAILS, COM_PROCESS_DET_DATA); // tells operator what details to display
// Now work out how much a pod costs to build:
  call(METH_NEW,
        MS_PR_NEW_BC_COST_DATA, // MB_PR_NEW_STATUS: MS_PR_NEW_BC_COST_DATA means get data cost.
        1, // MB_PR_NEW_VERTEX1: Vertex of this process that new process will be created at.
        -1, // MB_PR_NEW_VERTEX2: Vertex of child process (-1 means use child process' base_vertex value)
        -1600, // MB_PR_NEW_ANGLE: Angle of child process
        process_start(pod_process), // MB_PR_NEW_START: start address of new process' code
        process_end(pod_process), // MB_PR_NEW_END: end address
        1, // MB_PR_NEW_LINK: 1 means processes will be connected (as long as they both have link methods on mutual vertex)
        0); // MB_PR_NEW_TEMPLATE: Source template index (not relevant here; only used when building from a template)
// Some of the mbank registers aren't relevant to a data cost check, but it doesn't cost too much to set them anyway.
// Here we should be able to ignore init_build_result, as the call shouldn't fail (unless there's an error in the pod process definition).
  pod_data_cost = get(METH_NEW, MB_PR_NEW_STATUS);
// Work out generator process cost:
  call(METH_NEW,
        MS_PR_NEW_BC_COST_DATA, 0, 0, 0,
        process_start(generator),
        process_end(generator), 1, 0);
  generator_data_cost = get(METH_NEW, MB_PR_NEW_STATUS);

// Now send a broadcast announcing that the process is ready to accept data transfers from its parent process:
/*    call(METH_BROADCAST,
         5000, // power (equivalent to range in pixels) - this very high value will be reduced to actual value when the method runs
         MSGID_BUILDER, // message ID - this is a bitfield that will be processed by the LISTEN methods of any processes in range.
         MSG_DATA_PLEASE); // message contents value 1 - asks for data. value 2 (the next mbank register) isn't used here.*/
// Set both engines to maximum power (actual maximum power is about 5, but a higher number will be treated as the maximum)
  put(METH_MOVE1, MB_PR_MOVE_RATE, 100);
  put(METH_MOVE2, MB_PR_MOVE_RATE, 100);
// Set addresses to write messages from pods:
  put(METH_LINK_L1, MB_PR_LINK_MESSAGE_ADDRESS, &link_message [0] [0] [0]);
  put(METH_LINK_R1, MB_PR_LINK_MESSAGE_ADDRESS, &link_message [1] [0] [0]);
  put(METH_LINK_L2, MB_PR_LINK_MESSAGE_ADDRESS, &link_message [2] [0] [0]);
  put(METH_LINK_R2, MB_PR_LINK_MESSAGE_ADDRESS, &link_message [3] [0] [0]);
 }


// First we check whether the process has all of its pods.
// If it doesn't, try to build them if the process has enough data.
 build_pods();

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

 // try to self-repair (does nothing if the process isn't damaged):
 if (get_irpt() > 800)
  call(METH_RESTORE, 2);


} // end main


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
   case COMMAND_A_MOVE: // A-move not properly supported as this process has no scanner.
// Read the target coordinates, which the client will have put into other command registers:
    target_x = get_command(COMREG_QUEUE + 1);
    target_y = get_command(COMREG_QUEUE + 2);
    mode = MODE_MOVE;
    if (verbose != 0)
     print("\nMoving to ", target_x, ", ", target_y, ". ");
    break;

   case COMMAND_ATTACK:
// Read the target coordinates, which the client will have put into other command registers:
    target_x = get_command(COMREG_QUEUE + 1) - x;
    target_y = get_command(COMREG_QUEUE + 2) - y;
    mode = MODE_ATTACK_COMMAND;
    if (verbose != 0)
     print("\nAttacking target.");
    break;

   case COMMAND_IDLE:
 			set_command(COMREG_QUEUE, COMMAND_NONE);
    set_command_bit_1(COMREG_PROCESS_STATUS, COM_PROCESS_ST_UPDATE);
// fall-through
   case COMMAND_NONE:
    mode = MODE_IDLE;
    if (verbose != 0)
     print("\nProcess entering idle mode.");
    break;

   case COMMAND_EMPTY: // stop current command and go to next command
				next_command();
				more_commands = 1;
    break;

  } // end switch (command_type)
	} // end while (more_commands)

} // end execute_command()



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
    case MODE_ATTACK_COMMAND: print("attacking"); break;
   }

   if (call(METH_LINK_BACK, MS_PR_LINK_EXISTS) == 1) // still attached
    print("\nProcess incomplete.");

}

static void process_action(void)
{

 switch(mode)
 {

  case MODE_IDLE:
   return; // Finish

  case MODE_MOVE:
   if (call(METH_LINK_BACK, MS_PR_LINK_EXISTS) == 1) // still attached
    break;
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


// MODE_ATTACK_COMMAND is for when the process has received a command to attack.
// It relies on the client process feeding target coordinates through the command registers
//  (which have been checked above)
  case MODE_ATTACK_COMMAND:
   if (call(METH_LINK_BACK, MS_PR_LINK_EXISTS) == 1) // still attached
    break;
// Now tell the process to move towards its target.
    target_x = get_command(COMREG_QUEUE + 1);
    target_y = get_command(COMREG_QUEUE + 2);
// This process doesn't use attack_x/y settings because it doesn't attack its targets directly.
    process_move();
    break;

 }

}



static void process_move(void)
{

	int target_angle, turn_dir, angle_diff;

 target_angle = atan2(target_y - y, target_x - x);
 turn_dir = turn_direction(angle, target_angle); // turn_direction() is a built-in MATHS method function.
    // It returns the direction of the shortest turning distance between angle and target_angle, taking bounds into account.
    // Return values are -1, 0 (if angles are equal) or 1
 angle_diff = angle_difference(angle, target_angle); // angle_difference returns the absolute difference between two angles

 int future_turn_dir;

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
	{
  put(METH_MOVE1, MB_PR_MOVE_DELAY, angle_diff / 30); // DELAY waits for a while before the engine starts. 30 is a tuning factor.
  call(METH_LINK_R2, MS_PR_LINK_MESSAGE, LM_MOVE, angle_diff / 30);
	}
  else
		{
   put(METH_MOVE1, MB_PR_MOVE_DELAY, 0); // No delay - process is either turning anti-clockwise, or firing both engines to move forwards.
   call(METH_LINK_R2, MS_PR_LINK_MESSAGE, LM_MOVE, 0);
		}

// Now we do the same for the left-hand engine:
 put(METH_MOVE2, MB_PR_MOVE_RATE, 40);
 put(METH_MOVE2, MB_PR_MOVE_COUNTER, 40);
 if (turn_dir < 0)
	{
  put(METH_MOVE2, MB_PR_MOVE_DELAY, angle_diff / 30);
  call(METH_LINK_L2, MS_PR_LINK_MESSAGE, LM_MOVE, angle_diff / 30);
	}
  else
		{
   put(METH_MOVE2, MB_PR_MOVE_DELAY, 0);
   call(METH_LINK_L2, MS_PR_LINK_MESSAGE, LM_MOVE, 0);
		}

}


// A very bad pseudorandom number generator that might just be good enough for this purpose:
static int random(int max)
{

// As this is a static function, seed is a static variable that is initialised when the process is created.
 int seed = 7477;
 seed += 4639;

 return abs(seed + get_speed_x() + get_speed_y()) % max;

}



static void build_pods(void)
{

 int new_process_vertex;
 int build_result;

 if (get_data() >= generator_data_cost
		&& call(METH_LINK_FRONT, MS_PR_LINK_EXISTS) == 0)
	{
   build_result = call(METH_NEW, MS_PR_NEW_BC_BUILD, 0, 0, 0, process_start(generator), process_end(generator), 1, 0);
	}

 if (get_data() >= pod_data_cost)
 {
  if (call(METH_LINK_L1, MS_PR_LINK_EXISTS) == 0)
  {
// tell the pod which position it's in
asm
{
setrn A 0
setar scope.pod_process::position A
}
// link to parent is at vertex 2:
   *(process_start(pod_process)+2+4+(0*6)+1) = 2;
   new_process_vertex = 2;
// link to shield1 is at vertex 0
   *(process_start(pod_process)+2+4+(1*6)+1) = 0;
// link to shield2 is at vertex 4
   *(process_start(pod_process)+2+4+(2*6)+0) = MT_PR_LINK;
   *(process_start(pod_process)+2+4+(2*6)+1) = 4;
// DPACKET1 is at vertex 5
   *(process_start(pod_process)+2+4+(3*6)+1) = 5;
// DPACKET2 is at vertex 3
   *(process_start(pod_process)+2+4+(4*6)+0) = MT_PR_DPACKET;
   *(process_start(pod_process)+2+4+(4*6)+1) = 3;
   *(process_start(pod_process)+2+4+(4*6)+3) = 0; // power extension
   *(process_start(pod_process)+2+4+(4*6)+4) = 1; // speed extension
   *(process_start(pod_process)+2+4+(4*6)+5) = 2; // range extension
// Try to build the pod:
   build_result = call(METH_NEW,
        MS_PR_NEW_BC_BUILD, // MB_PR_NEW_STATUS: MS_PR_NEW_BC_BUILD means try to build from bcode
        7, // MB_PR_NEW_VERTEX1: Vertex of this process that new process will be created at.
        new_process_vertex, // MB_PR_NEW_VERTEX2: Vertex of child process (-1 means use child process' base_vertex value)
//        -630, // MB_PR_NEW_ANGLE: Angle of child process
	       0,
        process_start(pod_process), // MB_PR_NEW_START: start address of new process' code
        process_end(pod_process), // MB_PR_NEW_END: end address
        1, // MB_PR_NEW_LINK: 1 means processes will be connected (as long as they both have link methods on mutual vertex)
        0); // MB_PR_NEW_TEMPLATE: Source template index (not relevant here; only used when building from a template)
   return;
  }

  if (call(METH_LINK_R1, MS_PR_LINK_EXISTS) == 0)
  {
// tell the pod which position it's in
asm
{
setrn A 1
setar scope.pod_process::position A
}
// link to parent is at vertex 4:
   *(process_start(pod_process)+2+4+(0*6)+1) = 4;
   new_process_vertex = 4;
// link to shield1 is at vertex 0
   *(process_start(pod_process)+2+4+(1*6)+1) = 0;
// link to shield2 is at vertex 2
   *(process_start(pod_process)+2+4+(2*6)+0) = MT_PR_LINK;
   *(process_start(pod_process)+2+4+(2*6)+1) = 2;
// DPACKET1 is at vertex 1
   *(process_start(pod_process)+2+4+(3*6)+1) = 1;
// DPACKET2 is at vertex 3
   *(process_start(pod_process)+2+4+(4*6)+0) = MT_PR_DPACKET;
   *(process_start(pod_process)+2+4+(4*6)+1) = 3;
   *(process_start(pod_process)+2+4+(4*6)+3) = 0; // power extension
   *(process_start(pod_process)+2+4+(4*6)+4) = 1; // speed extension
   *(process_start(pod_process)+2+4+(4*6)+5) = 2; // range extension
// Try to build the pod:
   build_result = call(METH_NEW,
        MS_PR_NEW_BC_BUILD, // MB_PR_NEW_STATUS: MS_PR_NEW_BC_BUILD means try to build from bcode
        1, // MB_PR_NEW_VERTEX1: Vertex of this process that new process will be created at.
        new_process_vertex, // MB_PR_NEW_VERTEX2: Vertex of child process (-1 means use child process' base_vertex value)
//        630, // MB_PR_NEW_ANGLE: Angle of child process
        0,
        process_start(pod_process), // MB_PR_NEW_START: start address of new process' code
        process_end(pod_process), // MB_PR_NEW_END: end address
        1, // MB_PR_NEW_LINK: 1 means processes will be connected (as long as they both have link methods on mutual vertex)
        0); // MB_PR_NEW_TEMPLATE: Source template index (not relevant here; only used when building from a template)
   return;
  }

  if (call(METH_LINK_L2, MS_PR_LINK_EXISTS) == 0)
  {
// tell the pod which position it's in
asm
{
setrn A 2
setar scope.pod_process::position A
}
// link to parent is at vertex 4:
   *(process_start(pod_process)+2+4+(0*6)+1) = 4;
   new_process_vertex = 4;
// link to shield1 is at vertex 1
   *(process_start(pod_process)+2+4+(1*6)+1) = 1;
// no shield2
   *(process_start(pod_process)+2+4+(2*6)+0) = MT_NONE;
//   *(process_start(pod_process)+2+4+(2*6)+1) = 4;
// DPACKET1 is at vertex 0
   *(process_start(pod_process)+2+4+(3*6)+1) = 0;
// DPACKET2 is replaced by a MOVE method at vertex 2
   *(process_start(pod_process)+2+4+(4*6)+0) = MT_PR_MOVE;
   *(process_start(pod_process)+2+4+(4*6)+1) = 2;
   *(process_start(pod_process)+2+4+(4*6)+2) = -2400;
   *(process_start(pod_process)+2+4+(4*6)+3) = 3; // power extension
// Try to build the pod:
   build_result = call(METH_NEW,
        MS_PR_NEW_BC_BUILD, // MB_PR_NEW_STATUS: MS_PR_NEW_BC_BUILD means try to build from bcode
        5, // MB_PR_NEW_VERTEX1: Vertex of this process that new process will be created at.
        new_process_vertex, // MB_PR_NEW_VERTEX2: Vertex of child process (-1 means use child process' base_vertex value)
        -220, // MB_PR_NEW_ANGLE: Angle of child process
        process_start(pod_process), // MB_PR_NEW_START: start address of new process' code
        process_end(pod_process), // MB_PR_NEW_END: end address
        1, // MB_PR_NEW_LINK: 1 means processes will be connected (as long as they both have link methods on mutual vertex)
        0); // MB_PR_NEW_TEMPLATE: Source template index (not relevant here; only used when building from a template)
   return;
  }

  if (call(METH_LINK_R2, MS_PR_LINK_EXISTS) == 0)
  {
// tell the pod which position it's in
asm
{
setrn A 3
setar scope.pod_process::position A
}
// link to parent is at vertex 2:
   *(process_start(pod_process)+2+4+(0*6)+1) = 2;
   new_process_vertex = 2;
// link to shield1 is at vertex 5
   *(process_start(pod_process)+2+4+(1*6)+1) = 5;
// no shield2
   *(process_start(pod_process)+2+4+(2*6)+0) = MT_NONE;
//   *(process_start(pod_process)+2+4+(2*6)+1) = 4;
// DPACKET1 is at vertex 0
   *(process_start(pod_process)+2+4+(3*6)+1) = 0;
// DPACKET2 is replaced by a MOVE method at vertex 4
   *(process_start(pod_process)+2+4+(4*6)+0) = MT_PR_MOVE;
   *(process_start(pod_process)+2+4+(4*6)+1) = 4;
   *(process_start(pod_process)+2+4+(4*6)+2) = 2400;
   *(process_start(pod_process)+2+4+(4*6)+3) = 3; // power extension
// Try to build the pod:
   build_result = call(METH_NEW,
        MS_PR_NEW_BC_BUILD, // MB_PR_NEW_STATUS: MS_PR_NEW_BC_BUILD means try to build from bcode
        3, // MB_PR_NEW_VERTEX1: Vertex of this process that new process will be created at.
        new_process_vertex, // MB_PR_NEW_VERTEX2: Vertex of child process (-1 means use child process' base_vertex value)
        220, // MB_PR_NEW_ANGLE: Angle of child process
        process_start(pod_process), // MB_PR_NEW_START: start address of new process' code
        process_end(pod_process), // MB_PR_NEW_END: end address
        1, // MB_PR_NEW_LINK: 1 means processes will be connected (as long as they both have link methods on mutual vertex)
        0); // MB_PR_NEW_TEMPLATE: Source template index (not relevant here; only used when building from a template)
   return;
  }

 } // end pod build tests

// Now check whether the pods have signalled that they are complete:
 int pods_are_finished;
 pods_are_finished = 1;

 if (call(METH_LINK_L1, MS_PR_LINK_RECEIVED) == 0
		|| link_message [0] [0] [0] != LM_COMPLETE)
	{
  pods_are_finished = 0;
	}
 if (call(METH_LINK_R1, MS_PR_LINK_RECEIVED) == 0
		|| link_message [1] [0] [0] != LM_COMPLETE)
	{
  pods_are_finished = 0;
	}
 if (call(METH_LINK_L2, MS_PR_LINK_RECEIVED) == 0
		|| link_message [2] [0] [0] != LM_COMPLETE)
	{
  pods_are_finished = 0;
	}
 if (call(METH_LINK_R2, MS_PR_LINK_RECEIVED) == 0
		|| link_message [3] [0] [0] != LM_COMPLETE)
	{
  pods_are_finished = 0;
	}


// if pods are all finished, disconnect from data source:
 if (pods_are_finished == 1)
  call(METH_LINK_BACK, MS_PR_LINK_DISCONNECT); // does nothing if nothing connected.

} // end build_pods()




// this process just sits at the front and pushes irpt to the main process
process generator
{

interface
{
 PROGRAM_TYPE_PROCESS, SHAPE_4DIAMOND, 3, 0,
	{
		METH_LINK: {MT_PR_LINK, 0},
		METH_IRPT: {MT_PR_IRPT, 0, 0, 4},
		METH_RESTORE: {MT_PR_RESTORE},
		METH_STD: {MT_PR_STD}
	}
}

int initialised;

static void main(void)
{

	if (initialised == 0)
	{
		set_command_bit_1(COMREG_PROCESS_DETAILS, COM_PROCESS_DET_HP); // tells operator what details to display
  set_command_bit_1(COMREG_PROCESS_DETAILS, COM_PROCESS_DET_IRPT); // tells operator what details to display
  set_command_bit_1(COMREG_PROCESS_DETAILS, COM_PROCESS_DET_NO_MULTI); // tells operator that this is a sub-process
  initialised = 1;
	}

// try to self-repair (does nothing if the process isn't damaged):
 call(METH_RESTORE, 2);

}

} // end process generator

process pod_process
{

interface
{
 PROGRAM_TYPE_PROCESS, SHAPE_6LONG, 3, 1,
 {
// Details of the first several methods are modified by the main process before the bcode is copied out into the new process.
  METH_LINK_PARENT: {MT_PR_LINK, 1, 0}, // this is always a link to the parent process, although the vertex may be changed
  METH_LINK_SHIELD1: {MT_PR_LINK, 3, 0}, // this is always a link to a shield subprocess
  METH_LINK_SHIELD2: {MT_PR_LINK, 5, 0}, // this is replaced by a MOVE method for the back processes
  METH_DPACKET1: {MT_PR_DPACKET, 2, 0, 1, 1, 2},
  METH_DPACKET2: {MT_PR_DPACKET, 0, 0, 1, 1, 2},
  METH_STD: {MT_PR_STD},
  METH_MATHS: {MT_PR_MATHS},
  METH_RESTORE: {MT_PR_RESTORE},
  METH_NEW_SUB: {MT_PR_NEW_SUB},
  {0}, // space for new method
  METH_SCAN: {MT_PR_SCAN, 0, 0, 1},
  {0},
  {0},
  METH_IRPT: {MT_PR_IRPT, 0, 0, 0},
  METH_DESIGNATE1: {MT_PR_DESIGNATE, 0, 0, 1},
  METH_DESIGNATE2: {MT_PR_DESIGNATE, 0, 0, 1},
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

int x, y;
int angle, spin;
int speed_x;
int speed_y;
int dpacket_attacking [2];
int dpacket_vertex [2];
int dpacket_vertex_angle [2];
int dpacket_vertex_dist [2];
int dpacket_vertex_angle_min [2];
int dpacket_vertex_angle_max [2];

int build_result;

// Scan values - used to scan for other nearby processes
int scan_result [8];
int scan_bitmask;

static int scan_for_target_multi(int designate_method, int offset_angle, int offset_dist, int scan_size);
static int run_dpacket_method_multi(int m, int vertex_angle, int vertex_dist, int min_angle, int max_angle, int designate_method,
																																				int packet_speed, int delay);
static int check_pod(int link_method, int pod_shape, int link_vertex, int pod_vertex, int pod_angle);
static int random(int max);
static int lead_target(int relative_x, int relative_y,
																							int relative_speed_x, int relative_speed_y,
																			    int base_angle,
																			    int intercept_speed);


process shield_process;

static void main(void)
{

 x = get_x();
 y = get_y();
 angle = get_angle();
 spin = get_spin();
 speed_x = get_speed_x();
 speed_y = get_speed_y();
 counter++;

 if (initialised == 0)
 {
  team = get_team();
  scan_bitmask = 0b1111 ^ (1<<team);
  build_result = call(METH_NEW_SUB,
        MS_PR_NEW_BC_COST_DATA, // MB_PR_NEW_STATUS: MS_PR_NEW_BC_COST_DATA means get data cost.
        1, // MB_PR_NEW_VERTEX1: Vertex of this process that new process will be created at.
        -1, // MB_PR_NEW_VERTEX2: Vertex of child process (-1 means use child process' base_vertex value)
        -1600, // MB_PR_NEW_ANGLE: Angle of child process
        process_start(shield_process), // MB_PR_NEW_START: start address of new process' code
        process_end(shield_process), // MB_PR_NEW_END: end address
        1, // MB_PR_NEW_LINK: 1 means processes will be connected (as long as they both have link methods on mutual vertex)
        0); // MB_PR_NEW_TEMPLATE: Source template index (not relevant here; only used when building from a template)
// Some of the mbank registers aren't relevant to a data cost check, but it doesn't cost too much to set them anyway.
// Here we should be able to ignore build_result, as the call shouldn't fail (unless there's an error in the pod process definition).
  pod_data_cost = get(METH_NEW_SUB, MB_PR_NEW_STATUS);
  set_command_bit_1(COMREG_PROCESS_DETAILS, COM_PROCESS_DET_HP); // tells operator what details to display
  set_command_bit_1(COMREG_PROCESS_DETAILS, COM_PROCESS_DET_IRPT); // tells operator what details to display
  set_command_bit_1(COMREG_PROCESS_DETAILS, COM_PROCESS_DET_NO_MULTI); // tells operator that this is a sub-process
  initialised = 1;
  put(METH_LINK_PARENT, MB_PR_LINK_MESSAGE_ADDRESS, &link_message [0] [0]);
// all positions have at least one dpacket method:
	 dpacket_vertex [0] = *(6+(METH_DPACKET1*6)+1);
  dpacket_vertex_angle [0] = get_vertex_angle(*(6+(METH_DPACKET1*6)+1));
  dpacket_vertex_dist [0] = get_vertex_dist(*(6+(METH_DPACKET1*6)+1));
  dpacket_vertex_angle_min [0] = get_vertex_angle_min(*(6+(METH_DPACKET1*6)+1));
  dpacket_vertex_angle_max [0] = get_vertex_angle_max(*(6+(METH_DPACKET1*6)+1));;
// now set up other details:
  switch(position)
  {
		 case POS_L1:
		 	verbose = 1;
			 left_right = 0;
			 front_back = 0;
			 dpacket_vertex [1] = *(6+(METH_DPACKET2*6)+1);
    dpacket_vertex_angle [1] = get_vertex_angle(*(6+(METH_DPACKET2*6)+1));
    dpacket_vertex_dist [1] = get_vertex_dist(*(6+(METH_DPACKET2*6)+1));
    dpacket_vertex_angle_min [1] = get_vertex_angle_min(*(6+(METH_DPACKET2*6)+1));
    dpacket_vertex_angle_max [1] = get_vertex_angle_max(*(6+(METH_DPACKET2*6)+1));
			 break;
		 case POS_R1:
			 left_right = 1;
			 front_back = 0;
			 dpacket_vertex [1] = *(6+(METH_DPACKET2*6)+1);
    dpacket_vertex_angle [1] = get_vertex_angle(*(6+(METH_DPACKET2*6)+1));
    dpacket_vertex_dist [1] = get_vertex_dist(*(6+(METH_DPACKET2*6)+1));
    dpacket_vertex_angle_min [1] = get_vertex_angle_min(*(6+(METH_DPACKET2*6)+1));
    dpacket_vertex_angle_max [1] = get_vertex_angle_max(*(6+(METH_DPACKET2*6)+1));;
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
 	if (front_back == 0) // front
		{
   if (left_right == 1) // right
   {
    process_complete |= check_pod(METH_LINK_SHIELD1, SHAPE_6IRREG_L, *(6+(METH_LINK_SHIELD1*6)+1), 1, 350);
    process_complete |= check_pod(METH_LINK_SHIELD2, SHAPE_6IRREG_R, *(6+(METH_LINK_SHIELD2*6)+1), 2, 400);
   }
    else // left
    {
     process_complete |= check_pod(METH_LINK_SHIELD1, SHAPE_6IRREG_R, *(6+(METH_LINK_SHIELD1*6)+1), 5, -350);
     process_complete |= check_pod(METH_LINK_SHIELD2, SHAPE_6IRREG_L, *(6+(METH_LINK_SHIELD2*6)+1), 4, -400);
    }
		}
		 else
			{

    if (left_right == 1) // right
    {
     process_complete |= check_pod(METH_LINK_SHIELD1, SHAPE_6IRREG_L, *(6+(METH_LINK_SHIELD1*6)+1), 2, 700);
    }
     else // left
     {
      process_complete |= check_pod(METH_LINK_SHIELD1, SHAPE_6IRREG_R, *(6+(METH_LINK_SHIELD1*6)+1), 4, -700);
     }
			}
 }

// if process is complete, tell the parent process:
 if (process_complete != 0)
 {
  call(METH_LINK_PARENT, MS_PR_LINK_MESSAGE, LM_COMPLETE);
 }

// if it's at the back, check whether the parent wants it to use its move method:
 	if (front_back == 1)
		{
			if (call(METH_LINK_PARENT, MS_PR_LINK_RECEIVED) != 0
				&& link_message [0] [0] == LM_MOVE)
			{
// if this process has a MOVE method, it will have replaced METH_DPACKET2:
		  put(METH_DPACKET2, MB_PR_MOVE_RATE, 100);
		  put(METH_DPACKET2, MB_PR_MOVE_COUNTER, 16);
		  put(METH_DPACKET2, MB_PR_MOVE_DELAY, link_message [0] [1]);
			}
		}


// scan for enemies:
 if (dpacket_attacking [0] == 1 // currently attacking a target
		&& counter % 4 != 0) // scan again every 4 cycles
  dpacket_attacking [0] = run_dpacket_method_multi(METH_DPACKET1, dpacket_vertex_angle [0], dpacket_vertex_dist [0],
                                             dpacket_vertex_angle_min [0],
                                             dpacket_vertex_angle_max [0], METH_DESIGNATE1,
//                                             7 * 16, // packet speed - 7 is 4 base speed plus 3 for 1 speed extension. speeds are generally * 16.
                                             8 * 16, // packet speed - 6 is 4 base speed plus 2 for 1 speed extension. speeds are generally * 16.
                                             1); // delay (1 means no delay)
   else
    dpacket_attacking [0] = scan_for_target_multi(METH_DESIGNATE1, dpacket_vertex_angle [0], 200, 1000);

	if (front_back == 0) // if it's 1, this process only has one dpacket
	{
  if (dpacket_attacking [1] == 1
			&& counter % 4 != 0)
   dpacket_attacking [1] = run_dpacket_method_multi(METH_DPACKET2, dpacket_vertex_angle [1], dpacket_vertex_dist [1],
                                              dpacket_vertex_angle_min [1],
																																														dpacket_vertex_angle_max [1], METH_DESIGNATE2,
                                              7 * 16, // packet speed - 7 is 4 base speed plus 3 for 1 speed extension. speeds are generally * 16.
                                              1); // delay (1 means no delay)
			 else
     dpacket_attacking [1] = scan_for_target_multi(METH_DESIGNATE2, dpacket_vertex_angle [1], 0, 1000);
 }

// finally, try to self-repair (does nothing if the process isn't damaged):
 if (get_irpt() > 3000) // repair is expensive - only do it if there's plenty of irpt
  call(METH_RESTORE, 2); // try to repair 2hp

} // end main

// This function uses the scan method to find a target.
//  - it accomodates processes with multiple designate methods (e.g for multiple dpacket methods)
// If a target is found, designate_method is used to acquire it.
// The scan can be off-centre from the process (although this can reduce the size of the scan):
//  offset_angle is an offset from the process's angle
//  offset_dist is the distance from the centre
// scan_size is the size of the scan (can be set to a high value as the method will just cap it if it's too high)
// Assumes that angle and scan_bitmask variables are set correctly (see the main() function above)
// Assumes that scan_result is an array with at least 2 elements.
// returns 1 if target found and acquired, 0 otherwise.
static int scan_for_target_multi(int designate_method, int offset_angle, int offset_dist, int scan_size)
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
  int mbank_index;
  mbank_index = designate_method * 4; // this is the first mbank register for designate_method
  put_index(mbank_index, MS_PR_DESIGNATE_ACQUIRE);
  mbank_index++;
  put_index(mbank_index, scan_result [0] + scan_x); // need to add scan_x because scan_result is an offset from the centre of the scan, not the process.
  mbank_index++;
  put_index(mbank_index, scan_result [1] + scan_y);

// The DESIGNATE method returns 1 if it is successful (which should always be the case here, but let's check anyway):
  if (call(designate_method) == 1)
   return 1;

 }

 return 0; // failed to find target.

}


// This function runs a dpacket method.
//  - it accommodates multiple dpacket methods and is meant for a process that does not lose its overall target if a dpacket method loses its own target.
//  - it assumes speed_x, speed_y and spin are set up.
// m is the method index (e.g. METH_DPACKET1)
// vertex_angle is the angle from the process's centre to the vertex the method is on.
// vertex_dist is the distance to the vertex.
// min_angle and max_angle are the min and max angles of the method.
// designate_method should be the method index of a designate method dedicated to the dpacket method.
// delay should be 1 or a higher number to stagger firing of multiple methods (e.g. to spread irpt load)
// It returns 1 if it attacked, or 0
static int run_dpacket_method_multi(int m, int vertex_angle, int vertex_dist, int min_angle, int max_angle, int designate_method, int packet_speed, int delay)
{

 int attack_x, attack_y; // location of target with respect to process
 int source_x, source_y; // location of packet source vertex with respect to process
 int old_source_x, old_source_y; // location of packet source vertex with respect to process, last tick
 int source_speed_x, source_speed_y; // speed of packet source vertex
 int dist; // distance to target
 int i;
 int method_angle;
 int target_angle;

// prepare to use the designate method to locate its current target:
 put_index(designate_method*4, MS_PR_DESIGNATE_LOCATE);

 if (call(designate_method) == 0)
  goto cancel_attack; // target out of range or destroyed.

 attack_x = get_index((designate_method*4) + 1); // gets DESIGNATE method's registers
 attack_y = get_index((designate_method*4) + 2); //  - these registers contain target's location relative to this process

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
		goto cancel_attack; // examine shouldn't fail if designate succeeded, but check anyway

// now find the angle to the target, adjusting for relative speeds:
 target_angle = lead_target(attack_x - source_x, // need to adjust target offset by vertex location
																		          attack_y - source_y,
																						      scan_result [2] - source_speed_x, // relative speed - ideally we'd calculate the speed of the vertex, but this is good enough.
																						      scan_result [3] - source_speed_y,
																		          angle + vertex_angle, // base_angle
																		          packet_speed);

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

 intercept_angle = atan2(modified_target_y, modified_target_x);
 intercept_angle = signed_angle_difference(base_angle, intercept_angle); // remember - can't use a maths function as an argument of another maths function.

	return intercept_angle;

}



static int check_pod(int link_method, int pod_shape, int link_vertex, int pod_vertex, int pod_angle)
{
 put_index((link_method*4) + MB_PR_LINK_STATUS, MS_PR_LINK_EXISTS);

 if (call(link_method) == 0) // pod doesn't exist
 {
// Try to build the pod
// First we need to set the pod's shape and link method vertex to the correct values
   *(process_start(shield_process)+3) = pod_shape;
   *(process_start(shield_process)+7) = pod_vertex;
   build_result = call(METH_NEW_SUB,
        MS_PR_NEW_BC_BUILD, // MB_PR_NEW_STATUS: MS_PR_NEW_BC_BUILD means try to build from bcode
        link_vertex, // MB_PR_NEW_VERTEX1: Vertex of this process that new process will be created at.
        pod_vertex, // MB_PR_NEW_VERTEX2: Vertex of child process (-1 means use child process' base_vertex value)
        pod_angle, // MB_PR_NEW_ANGLE: Angle of child process
        process_start(shield_process), // MB_PR_NEW_START: start address of new process' code
        process_end(shield_process), // MB_PR_NEW_END: end address
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
 seed += 4639;

 return abs(seed + get_speed_x() + get_speed_y()) % max;

}



process shield_process
{


interface
{
 PROGRAM_TYPE_PROCESS, SHAPE_6LONG, 3, 2,
 {
  METH_LINK: {MT_PR_LINK, 2},
  METH_STD: {MT_PR_STD},
  METH_RESTORE: {MT_PR_RESTORE},
  METH_VIRTUAL: {MT_PR_VIRTUAL, 0, 0, 4, 0}, // was 4, 0
  METH_REDUNDANCY: {MT_PR_REDUNDANCY, 0, 0, 0},
 }
}

int initialised;

// All this process needs to do is activate its restore and virtual methods
static void main(void)
{

	if (initialised == 0)
	{
		set_command_bit_1(COMREG_PROCESS_DETAILS, COM_PROCESS_DET_HP); // tells operator what details to display
  set_command_bit_1(COMREG_PROCESS_DETAILS, COM_PROCESS_DET_IRPT); // tells operator what details to display
  set_command_bit_1(COMREG_PROCESS_DETAILS, COM_PROCESS_DET_NO_MULTI); // tells operator that this is a sub-process
  initialised = 1;
	}

 if (get_irpt() > 800)
 {
  call(METH_VIRTUAL, MS_PR_VIRTUAL_CHARGE, 100);
  if (get_irpt() > 3000)
   call(METH_RESTORE, 2);
 }



}

}


} // end process pod_process



