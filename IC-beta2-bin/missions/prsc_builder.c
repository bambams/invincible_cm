
/*

prsc_builder.c

This is a simple process that can move around and build other processes in new locations.
It can't operate autonomously and needs to be given commands by a client program.
It relies on the standard command macros (which should be #defined in the system or operator program).
When near a factory, it requests data transfer (through a broadcast method).

*/


// This is the process's interface definition. It determines the nature of the process and
// the methods it has available. It must come before any code.
interface
{
 PROGRAM_TYPE_PROCESS, SHAPE_6HEXAGON, 3, 3, // program type, shape, size (from 0-3), base vertex
 {
  METH_MOVE1: {MT_PR_MOVE, 2, ANGLE_2 - data(DATA_SHAPE_VERTEX_ANGLE, SHAPE_6HEXAGON, 2)},
  METH_MOVE2: {MT_PR_MOVE, 4, ANGLE_2 - data(DATA_SHAPE_VERTEX_ANGLE, SHAPE_6HEXAGON, 4)},
  METH_COM: {MT_PR_COMMAND}, // command method. allows process to communicate with operator/delegate
  METH_MATHS: {MT_PR_MATHS}, // maths method. allows trigonometry etc.
  METH_STD: {MT_PR_STD}, // information etc method. among other things allows access to process's current properties (location etc)
  METH_IRPT: {MT_PR_IRPT}, // generates irpt
  METH_BROADCAST: {MT_PR_BROADCAST, 3, 0}, // broadcast method. allows process to communicate directly with other processes.
  METH_NEW: {MT_PR_NEW}, // new process method. Produces
  {0}, // new process method requires 2 method slots, so this one is empty
  METH_STORAGE: {MT_PR_STORAGE, 0, 0, 3}, // stores additional data
  METH_LINK: {MT_PR_LINK, 0}, // stores additional data

 }
}

#include "stand_coms.c"

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
int mode; // Behavioural mode of the process (see the MODE enums below)
int target_x, target_y; // Where the process is going
int build_result;
int build_source_template;
int verbose; // Is 1 if client has told it to talk about what it's doing.

// Mode enum - these are for the process' mode value
enum
{
MODE_IDLE, // Process is sitting around. May be receiving data transfer.
MODE_MOVE, // Process is moving to a known location
MODE_ARRIVED // Process has arrived at destination but is still moving. Will request data transfer when it has almost stopped moving.
};

static void next_command(void);
static void query_process(void);
static void execute_command(void);
static void process_action(void);
static void process_move(void);


// Remember that main is called every time the program is run.
// Its correct type is static void main(void)
static void main(void)
{

// These variables are used below:
 int target_angle;
 int angle_diff;
 int turn_dir;
 int future_turn_dir;
 int command_received;
 int i, j;

// First we get some basic information about the process' current state.
// The "get_*" functions are built-in shortcuts that the compiler turns into calls
//  to the INFO method.
 angle = get_angle();
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
  target_x = x + cos(angle, 5); // target_x and target_y are used in the movement code below
  target_y = y + sin(angle, 5); // cos and sin are built-in functions that call the MATHS method.
                                  // These calls multiply the result of cos/sin by 5 (to get the x/y components
                                  //  of a line extending 5 pixels to the front of the process)
  set_command(COMREG_QUEUE, COMMAND_MOVE); // Tells the client that this process is currently moving
  set_command(COMREG_QUEUE + 1, target_x); // tells the client where it's moving to
  set_command(COMREG_QUEUE + 2, target_y);
  set_command_bit_1(COMREG_PROCESS_DETAILS, COM_PROCESS_DET_COMMAND); // tells operator that this process accepts commands.
  set_command_bit_1(COMREG_PROCESS_DETAILS, COM_PROCESS_DET_HP); // tells operator what details to display
  set_command_bit_1(COMREG_PROCESS_DETAILS, COM_PROCESS_DET_IRPT); // tells operator what details to display
  set_command_bit_1(COMREG_PROCESS_DETAILS, COM_PROCESS_DET_DATA); // tells operator what details to display
  set_command_bit_1(COMREG_PROCESS_DETAILS, COM_PROCESS_DET_ALLOC_EFF); // tells operator what details to display
// Other stuff:
  world_size_x = get_world_x(); // This is the size of the entire playing area, in pixels
  world_size_y = get_world_y();
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


static void query_process(void)
{

   call(METH_STD, MS_PR_STD_PRINT_OUT2); // redirects output to alternative console
// This command is given when the process is clicked on individually.
// The builder process responds by printing a list of possible building actions to the console.
// First, though, it prints the allocator efficiency at this location (in case the user is thinking of building an allocator process)
//  (this will give approximate results as the new process will not be at exactly the same place as this process)
   print("\nAllocator efficiency here: ", get_efficiency(), ".");
   int t;
// The STD method allows the process to attach an action to a printed console line. This causes an event when the user clicks on that line.
// Processes don't have direct access to generated actions, but operator programs do. The operator will need to pass the clicked
//  action back as a command.
   for (t = 0; t < PROCESS_TEMPLATES; t ++)
   {
// Now we need to ask the NEW method to work out whether this template can be built, and how much it will cost.
// Need to recalculate this each time because the user may change the templates.
    build_result = call(METH_NEW,
                        MS_PR_NEW_T_COST_DATA, // gets the cost of building a template
                        0, // MB_PR_NEW_VERTEX1: Vertex of this process that new process will be created at.
                        -1, // MB_PR_NEW_VERTEX2: Vertex of child process (-1 means use child process' base_vertex interface value)
                        0, // MB_PR_NEW_ANGLE: Angle of child process (0 means pointing directly away)
                        0, // MB_PR_NEW_START: start address of new process' code
                        BCODE_SIZE_PROCESS - 1, // MB_PR_NEW_END: end address
                        0, // MB_PR_NEW_LINK: 1 means try to connect (does nothing if fails)
                        t); // MB_PR_NEW_TEMPLATE: Source template index (not relevant here; only used when building from a template)
    if (build_result == MR_NEW_TEST_SUCCESS)
    {
// if the test was a success, the method's status register will hold the data cost of the process:
     call(METH_STD, MS_PR_STD_COLOUR, COL_LBLUE);
     call(METH_STD, MS_PR_STD_ACTION, t + 1);
     print("\n >> Click here to build template ",t," [");
     call(METH_STD, MS_PR_STD_TEMPLATE_NAME, t);
					print("] (cost ", get(METH_NEW, MB_PR_NEW_STATUS), ") <<");

    }
   }

   call(METH_STD, MS_PR_STD_ACTION, 0); // stop assigned actions to printed lines

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
   case COMMAND_ATTACK: // This process can't attack, so it interprets an attack command as a move command.
// Read the target coordinates, which the client will have put into other command registers:
    target_x = get_command(COMREG_QUEUE + 1);
    target_y = get_command(COMREG_QUEUE + 2);
    mode = MODE_MOVE;
    if (verbose != 0)
     print("\nMoving to ", target_x, ", ", target_y, ". ");
    break;

   case COMMAND_IDLE:
 			set_command(COMREG_QUEUE, COMMAND_NONE);
    set_command_bit_1(COMREG_PROCESS_STATUS, COM_PROCESS_ST_UPDATE);
// fall-through
   case COMMAND_NONE:
    mode = MODE_ARRIVED; // process has arrived - will request data transfer next cycle.
    put(METH_MOVE1, MB_PR_MOVE_RATE, 0); // turn off the move methods
    put(METH_MOVE2, MB_PR_MOVE_RATE, 0); // turn off the move methods
    break;

   case COMMAND_EMPTY: // stop current command and go to next command
				next_command();
				more_commands = 1;
    break;

  case COMMAND_ACTION:
// This command is used when the user clicks on a console line that this process has attached an action to (at least, that's how
//  the current basic operator program is set up).
// For this builder process, it means that the user has asked it to build something:
   build_source_template = get_command(COMREG_QUEUE+1) - 1;
   if (build_source_template == -1)
    break; // this probably means that the user has clicked on a line printed by this process without a specific action
   if (verbose != 0)
    print("\nBuilding template ", build_source_template, ".");
// So now we try to build from the specified template:
    build_result = call(METH_NEW,
                   MS_PR_NEW_T_BUILD, // MB_PR_NEW_STATUS: means try to build a new process from a template.
                   0, // MB_PR_NEW_VERTEX1: Vertex of this process that new process will be created at.
                   -1, // MB_PR_NEW_VERTEX2: Vertex of child process (-1 means use child process' base_vertex value)
                   0, // MB_PR_NEW_ANGLE: Angle of child process (0 means its vertex 0 will point directly away)
                   0, // MB_PR_NEW_START: Start address (in template code)
                   BCODE_SIZE_PROCESS - 1, // MB_PR_NEW_END: End address (here we are copying the whole template)
                   0, // MB_PR_NEW_LINK: 0 means don't want processes to be connected
                   build_source_template); // MB_PR_NEW_TEMPLATE: Source template index
// now print the results of the build call:
    if (build_result == MR_NEW_SUCCESS)
				{
					print("\nBuild successful.");
				}
				 else
					{
						print("\nBuild error: ");
      switch(build_result)
      {
       case MR_NEW_FAIL_STATUS: print("status?"); break; // Shouldn't happen
       case MR_NEW_FAIL_TYPE: print("type."); break;
       case MR_NEW_FAIL_OBSTACLE: print("collision."); break;
       case MR_NEW_FAIL_IRPT: print("not enough irpt."); break;
       case MR_NEW_FAIL_DATA:
        print("not enough data.");
// If there's not enough data, it would be useful to know how much is needed.
// So we call the new process method again, with the MS_PR_NEW_T_COST_DATA status (the rest of the method's registers
//  are unchanged from the previous call).
// If successful, this replaces the status register with the data cost of the specified process.
        build_result = call(METH_NEW,
                          MS_PR_NEW_T_COST_DATA);
        if (build_result == MR_NEW_TEST_SUCCESS)
         print("\n(Have ", get_data(), "/", get(METH_NEW, MB_PR_NEW_STATUS), ").");
        break;
       case MR_NEW_FAIL_START_BOUNDS: print("start address."); break;
       case MR_NEW_FAIL_END_BOUNDS: print("end address."); break;
       case MR_NEW_FAIL_INTERFACE: print("interface."); break;
       case MR_NEW_FAIL_TOO_MANY_PROCS: print("too many processes."); break;
       case MR_NEW_FAIL_SHAPE: print("shape."); break;
       case MR_NEW_FAIL_SIZE: print("size."); break;
       case MR_NEW_FAIL_PARENT_VERTEX: print("parent vertex."); break;
       case MR_NEW_FAIL_CHILD_VERTEX: print("child vertex."); break;
       case MR_NEW_FAIL_TEMPLATE: print("template."); break;
       case MR_NEW_FAIL_TEMPLATE_EMPTY: print("template not loaded."); break;
       default: print("unknown reason."); break;
      }
     }
   break;

  } // end switch(command_received)
	} // end while more_commands == 1

} // end execute_command()



static void process_action(void)
{

 switch(mode)
 {

  case MODE_IDLE:
// In IDLE mode, the process sits around doing nothing
   break;

  case MODE_MOVE:
// In MOVE mode, the process moves towards target_x/y then, when near, it executes the next command (which may be COMMAND_NONE)
   if (abs(y - target_y) + abs(x - target_x) < 100) // abs is a built-in function that calls the MATHS method
   {
    if (verbose != 0)
     print("\nReached destination.");
   	next_command();
   	execute_command();
// If the command didn't result in the process doing anything, this probably means that the process has finished moving.
//  So we set its mode to ARRIVED (so it will request data transfer)
   	if (mode == MODE_IDLE)
					mode = MODE_ARRIVED;
    break;
			}
			 else
     process_move();
   break;

  case MODE_ARRIVED:
// In arrived mode, process waits until it stops moving (almost) then broadcasts a request for nearby allocators to transfer data.
   if (abs(speed_x) + abs(speed_y) < 16)
   {
    mode = MODE_IDLE;
    set_command(COMREG_QUEUE, COMMAND_IDLE); // Tells the client that this process is currently idle
// Now send a broadcast announcing that the process is ready to accept data transfers if near an allocator process:
    call(METH_BROADCAST,
         5000, // power (equivalent to range in pixels) - this very high value will be reduced to actual value when the method runs
         MSGID_BUILDER, // message ID - this is a bitfield that will be processed by the LISTEN methods of any processes in range.
         MSG_DATA_PLEASE); // message contents value 1 - asks for data. value 2 (the next mbank register) isn't used here.
// MSG_* macros are part of the standard command structure
    if (verbose != 0)
     print("\nEntering idle mode. Accepting data transfer.");
   }
   return; // return from main() (finishes)

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
  put(METH_MOVE1, MB_PR_MOVE_DELAY, angle_diff / 20); // DELAY waits for a while before the engine starts. 30 is a tuning factor.
   else
    put(METH_MOVE1, MB_PR_MOVE_DELAY, 0); // No delay - process is either turning anti-clockwise, or firing both engines to move forwards.

// Now we do the same for the left-hand engine:
 put(METH_MOVE2, MB_PR_MOVE_RATE, 40);
 put(METH_MOVE2, MB_PR_MOVE_COUNTER, 40);
 if (turn_dir < 0)
  put(METH_MOVE2, MB_PR_MOVE_DELAY, angle_diff / 20);
   else
    put(METH_MOVE2, MB_PR_MOVE_DELAY, 0);


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

