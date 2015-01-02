/*

pr_wander.c

This is a simple process that can move around and build other processes in new locations,
then remain connected until it has enough data to build another factory.

*/


// This is the process's interface definition. It determines the nature of the process and
// the methods it has available. It must come before any code.
interface
{
 PROGRAM_TYPE_PROCESS, SHAPE_6POINTY, 2, 3, // program type, shape, size (from 0-3), base vertex
 {
  METH_MOVE1: {MT_PR_MOVE, 2, ANGLE_2 - data(DATA_SHAPE_VERTEX_ANGLE, SHAPE_6HEXAGON, 2)},
  METH_MOVE2: {MT_PR_MOVE, 4, ANGLE_2 - data(DATA_SHAPE_VERTEX_ANGLE, SHAPE_6HEXAGON, 4)},
  METH_MATHS: {MT_PR_MATHS}, // maths method. allows trigonometry etc.
  METH_STD: {MT_PR_STD}, // information method etc. among other things allows access to process's current properties (location etc)
  METH_IRPT: {MT_PR_IRPT}, // generates irpt
  METH_NEW: {MT_PR_NEW}, // new process method. Produces
  {0}, // new process method requires 2 method slots, so this one is empty
  METH_STORAGE: {MT_PR_STORAGE, 0, 0, 3}, // stores additional data
  METH_LINK: {MT_PR_LINK, 3},
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
int mode; // Behavioural mode of the process (see the MODE enums below)
int mode_count;
int target_x, target_y; // Where the process is going

// Mode enum - these are for the process' mode value
enum
{
MODE_WANDER, // Process is moving to a known location
MODE_CONNECTED, // Process is connected to another process and is drawing data from it
MODE_BUILD // Process has found an area of high allocation efficiency and is preparing to build
};

static void start_wandering(void);
static void try_building(void);
static int random(int max);

// Remember that main is called every time the program is run.
// Its correct type is static void main(void)
static void main(void)
{

// These variables are used below:
 int target_angle;
 int angle_diff;
 int turn_dir;
 int future_turn_dir;
 int build_source_template;
 int build_result;
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

// If this is the first time the process has been run, we also need to set up some
//  basic information about things like which player is controlling it.
 if (initialised == 0)
 {
  team = get_team(); // Which player is the process being controlled by? (0-3)
// Now let's tell the process to move away from its parent process (by moving 200 pixels forwards):
  mode = MODE_CONNECTED;
  target_x = x + cos(angle, 50); // target_x and target_y are used in the movement code below
  target_y = y + sin(angle, 50); // cos and sin are built-in functions that call the MATHS method.
                                  // These calls multiply the result of cos/sin by 200 (to get the x/y components
                                  //  of a line extending 200 pixels to the front of the process)
// Other stuff:
  world_size_x = get_world_x(); // This is the size of the entire playing area, in pixels
  world_size_y = get_world_y();
		call(METH_LINK, MS_PR_LINK_TAKE_DATA, 1);
  initialised = 1; // Now it won't be initialised again
 }


// What the process does next depends on which mode it is in.
 switch(mode)
 {

  case MODE_WANDER:
  	mode_count --;
   if (mode_count <= 0
			 || abs(y - target_y) + abs(x - target_x) < 100) // abs is a built-in function that calls the MATHS method
   	start_wandering();
 		if (get_efficiency() >= 100)
			{
				mode = MODE_BUILD;
				mode_count = 2;
			}
   break;

  case MODE_BUILD:
  	 if (get_efficiency() < 100)
				{
					start_wandering();
					break;
				}
  	 mode_count --;
  	 if (mode_count <= 0)
				{
				 try_building();
				}
			 break;

		case MODE_CONNECTED:
			if (get_data() >= get_own_data_max())
			{
				call(METH_LINK, MS_PR_LINK_DISCONNECT);
				start_wandering();
			}
			return; //exits

 } // end of switch(mode)

// At this point, all we have left to do is make the process move towards its target, which is at (target_x, target_y)
// If the process isn't moving, it will have returned by now.

// *** NOTE: if process is connected, it won't reach here!

// Work out the angle to the target:
 target_angle = atan2(target_y - y, target_x - x);
// Work out which way the process has to turn to get there:
 turn_dir = turn_direction(angle, target_angle); // turn_direction() is a built-in MATHS method function.
    // It returns the direction of the shortest turning distance between angle and target_angle, taking bounds into account.
    // Return values are -1, 0 (if angles are equal) or 1
 angle_diff = angle_difference(angle, target_angle);

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

// adjust angle_diff so that small differences in angle don't make the engines shut off.
 angle_diff -= 50;

// METH_MOVE1 is the right-hand engine, which will turn the process anti-clockwise if run alone.
 put(METH_MOVE1, MB_PR_MOVE_RATE, 40); // This is its power level (40 is very high, so the method will just run at maximum capacity, which is about 5)
 put(METH_MOVE1, MB_PR_MOVE_COUNTER, 40); // Will run for 40 ticks (also very high as the process will execute again in less than that. Doesn't matter)
 if (turn_dir > 0) // If turn_dir > 0, the process is trying to rotate clockwise. So we suppress this engine for a while:
  put(METH_MOVE1, MB_PR_MOVE_DELAY, angle_diff / 20); // DELAY waits for a while before the engine starts. 20 is an approximation factor.
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


// This function makes the process start wandering and gives it a random destination:
static void start_wandering(void)
{

     mode = MODE_WANDER;
     target_x = random(world_size_x - 600) + 300;
     target_y = random(world_size_y - 600) + 300;

     mode_count = 120; // will wander for 120 cycles (about 60 seconds)

}

static void try_building(void)
{
	 call(METH_NEW,
       MS_PR_NEW_T_BUILD, // MB_PR_NEW_STATUS: means: after this process finishes executing, try to build
       3, // MB_PR_NEW_VERTEX1: Vertex of this process that new process will be created at.
       -1, // MB_PR_NEW_VERTEX2: Vertex of child process (-1 means use child process' base_vertex value)
       0, // MB_PR_NEW_ANGLE: Angle of child process
       0, // MB_PR_NEW_START: Start address
       BCODE_SIZE_PROCESS - 1, // MB_PR_NEW_END: End address
       1, // MB_PR_NEW_LINK: 1 means try to connect to new process
       0); // MB_PR_NEW_TEMPLATE: Source template index (currently always builds from template 0)

  if (call(METH_LINK, MS_PR_LINK_EXISTS) == 1)
		{
			mode = MODE_CONNECTED;
			call(METH_LINK, MS_PR_LINK_TAKE_DATA, 1);
		}

}

// A very bad pseudorandom number generator that might just be good enough for this purpose:
static int random(int max)
{

// As this is a static function, seed is a static variable that is initialised when the process is created.
 int seed = 7477;
 seed = seed * get_time() + get_x() + seed;
 return abs(seed) % max;

}



