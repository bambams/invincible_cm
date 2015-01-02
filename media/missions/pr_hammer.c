

/*

pr_hammer.c

This is a basic multi-part process. It remains connected to its factory until complete,
then wanders around looking for targets.
It doesn't accept commands or interact in any way with a client/observer program.

*/





// This is the process's interface definition. It determines the nature of the process and
// the methods it has available. It must come before any code.
interface
{
 PROGRAM_TYPE_PROCESS, SHAPE_6POINTY, 3, 0, // program type, shape, size (from 0-3), base_vertex
 {
  METH_MATHS: {MT_PR_MATHS}, // maths method. allows trigonometry etc.
  METH_STD: {MT_PR_STD}, // information method. allows access to process's current properties (location etc)
  METH_IRPT: {MT_PR_IRPT, 0, 0, 3}, // generates irpt. functions automatically (although can be configured not to)
  METH_SCAN: {MT_PR_SCAN, 0, 0, 2}, // scan method. allows process to sense its surroundings. takes up 3 method slots
  {0}, // space for scan method
  {0}, // space for scan method
  METH_DESIGNATE: {MT_PR_DESIGNATE}, // designator. allows process to keep track of a target acquired through scan
  METH_LINK_LEFT: {MT_PR_LINK, 4, 0},
  METH_LINK_RIGHT: {MT_PR_LINK, 2, 0},
  METH_LINK_BACK: {MT_PR_LINK, 0, 0},
  METH_NEW_SUB: {MT_PR_NEW_SUB},
  {0}, // space for new method
  METH_MOVE1: {MT_PR_MOVE, 5, - data(DATA_SHAPE_VERTEX_ANGLE, SHAPE_6POINTY, 5)},
  METH_MOVE2: {MT_PR_MOVE, 1, - data(DATA_SHAPE_VERTEX_ANGLE, SHAPE_6POINTY, 1)},
  METH_RESTORE: {MT_PR_RESTORE},

 }
}


// Now we declare some global variables.
// Remember that these are initialised (to zero, or to a specified value) when the process is
//  created, but not each time it is run.

int initialised; // When this is zero, it tells the process that it has just been created and needs to initialise.
int pod_data_cost; // cost of building a pod

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

// Scan values - used to scan for other nearby processes
int scan_result [32]; // actually only 4 values are used, but may want to extend later.
int scan_bitmask;


// Mode enum - these are for the process' mode value
enum
{
MODE_IDLE, // Process is sitting around, scanning for something to attack
MODE_WANDER, // Process is wandering randomly and scanning for something to attack
MODE_MOVE, // Process is moving to a known location
MODE_ATTACK, // Process has a designated target and is attacking it
MODE_CONNECTED, // Process still connected to parent and receiving data to build its pods
MODE_ARRIVED

};

// Function declarations
// Note that "static" in a function declaration means something different from normal C.
//  - It means that all variables in the function are static, which is recommended as
//    static variables are much more efficient than automatic variables (which you
//    shouldn't use without a good reason)
static void scan_for_target(void);
static void start_wandering(void);
static void lead_target(int attack_x_ptr, int attack_y_ptr);
static int random(int max);

process pod_left;
process pod_right;

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
 fire_instruction = 0;

 int pod_angle;

// First we get some basic information about the process' current state.
// The "get_*" functions are built-in shortcuts that the compiler turns into calls
//  to the INFO method.
 angle = get_angle() + ANGLE_2; // this process is reversed, so that its vertex 0 points backwards. Need to adjust the angle by 180 deg (ANGLE_2)
 x = get_x();
 y = get_y();
 speed_x = get_speed_x();
 speed_y = get_speed_y();
 spin = get_spin();

 if (initialised == 0)
 {
  team = get_team(); // Which player is the process being controlled by? (0-3)
  world_size_x = get_world_x(); // This is the size of the entire playing area, in pixels
  world_size_y = get_world_y();
  scan_bitmask = 0b1111 ^ (1<<team); // scan_bitmask is used by the scanner to find processes that belong to a different team.
      // The four least significant bits of scan_bitmask tell the scanner to accept (1) or ignore (0) processes that belong to player 4, 3, 2 and 1.
      // ^ (1<<team) flips the bit corresponding to this process' team, so the scanner will ignore friendly processes.
      // For example, if this process belongs to player 2 the bits will be 1101.
  initialised = 1; // Now it won't be initialised again
  mode = MODE_CONNECTED; // Means that the process is still connected to the factory that built it.
// Now work out how much a pod costs to build:
  build_result = call(METH_NEW_SUB,
        MS_PR_NEW_BC_COST_DATA, // MB_PR_NEW_STATUS: MS_PR_NEW_BC_COST_DATA means get data cost.
        4, // MB_PR_NEW_VERTEX1: Vertex of this process that new process will be created at.
        -1, // MB_PR_NEW_VERTEX2: Vertex of child process (-1 means use child process' base_vertex value)
        0, // MB_PR_NEW_ANGLE: Angle of child process
        process_start(pod_left), // MB_PR_NEW_START: start address of new process' code
        process_end(pod_left), // MB_PR_NEW_END: end address
        1, // MB_PR_NEW_LINK: 1 means processes will be connected (as long as they both have link methods on mutual vertex)
        0); // MB_PR_NEW_TEMPLATE: Source template index (not relevant here; only used when building from a template)
// Some of the mbank registers aren't relevant to a data cost check, but it doesn't cost too much to set them anyway.
// Here we should be able to ignore build_result, as the call shouldn't fail (unless there's an error in the pod process definition).
  pod_data_cost = get(METH_NEW_SUB, MB_PR_NEW_STATUS);
// Set both engines to maximum power (actual maximum power is about 5, but a higher number will be treated as the maximum)
  put(METH_MOVE1, MB_PR_MOVE_RATE, 100);
  put(METH_MOVE2, MB_PR_MOVE_RATE, 100);
 }

// First we check whether the process has both of its pods.
// If it doesn't, try to build them if the process has enough data.

 if (call(METH_LINK_LEFT, MS_PR_LINK_EXISTS) == 0 // left pod doesn't exist
  && get_data() >= pod_data_cost)
 {
// Try to build the pod:
   build_result = call(METH_NEW_SUB,
        MS_PR_NEW_BC_BUILD, // MB_PR_NEW_STATUS: MS_PR_NEW_BC_BUILD means try to build from bcode
        4, // MB_PR_NEW_VERTEX1: Vertex of this process that new process will be created at.
        -1, // MB_PR_NEW_VERTEX2: Vertex of child process (-1 means use child process' base_vertex value)
        -1300, // MB_PR_NEW_ANGLE: Angle of child process
        process_start(pod_left), // MB_PR_NEW_START: start address of new process' code
        process_end(pod_left), // MB_PR_NEW_END: end address
        1, // MB_PR_NEW_LINK: 1 means processes will be connected (as long as they both have link methods on mutual vertex)
        0); // MB_PR_NEW_TEMPLATE: Source template index (not relevant here; only used when building from a template)
 }

 if (call(METH_LINK_RIGHT, MS_PR_LINK_EXISTS) == 0 // right pod doesn't exist
  && get_data() >= pod_data_cost)
 {
// Try to build the pod:
   build_result = call(METH_NEW_SUB,
        MS_PR_NEW_BC_BUILD, // MB_PR_NEW_STATUS: MS_PR_NEW_BC_BUILD means try to build from bcode
        2, // MB_PR_NEW_VERTEX1: Vertex of this process that new process will be created at.
        -1, // MB_PR_NEW_VERTEX2: Vertex of child process
        1300, // MB_PR_NEW_ANGLE: Angle of child process
        process_start(pod_right), // MB_PR_NEW_START: start address of new process' code
        process_end(pod_right), // MB_PR_NEW_END: end address
        1, // MB_PR_NEW_LINK: 1 means processes will be connected (as long as they both have link methods on mutual vertex)
        0); // MB_PR_NEW_TEMPLATE: Source template index (not relevant here; only used when building from a template)
 }

// What the process does next depends on which mode it is in.
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
// We could also wait until the process has enough data to rebuild one of its pods if destroyed.
// Maybe make that optional?
   goto finished; // finish executing

  case MODE_IDLE:
// In IDLE mode, the process sits around scanning for a target.
   scan_for_target(); // If scan_for_target finds something to attack, it changes the mode.
   goto finished; // Finish

  case MODE_MOVE:
// In MOVE mode, the process moves towards target_x/y then, when near, it enters the wandering mode
   mode_count --;
   if (abs(y - target_y) + abs(x - target_x) < 100 // abs is a built-in function that calls the MATHS method
    || mode_count < 0)
    start_wandering();
   break;

  case MODE_WANDER:
// WANDER mode is like MOVE mode, but the process scans for targets while moving.
   mode_count --;
   if (abs(y - target_y) + abs(x - target_x) < 100 // abs is a built-in function that calls the MATHS method
    || mode_count < 0)
     start_wandering();
   scan_for_target();
   break;

  case MODE_ATTACK:
   scan_for_target();
// In ATTACK mode, the process moves towards its designated target and attacks it.
// Calling the DESIGNATE method with status MS_PR_DESIGNATE_READ does the following:
//  - If the process has no currently designated target, it returns 0
//  - Otherwise, it sets the DESIGNATE method's method registers 1 and 2 to the x/y coordinates of
//    the target (relative to the current process' location), then returns 1.
   if (call(METH_DESIGNATE, MS_PR_DESIGNATE_LOCATE) == 1)
   {
    attack_x = get(METH_DESIGNATE, 1); // gets DESIGNATE method's register 1
    attack_y = get(METH_DESIGNATE, 2); // gets DESIGNATE method's register 2
// target_x/y is used for both movement and attacking.
// If the target is (roughly) within 800 pixels of this process, consider firing at it:
    if (abs(attack_y) + abs(attack_x) < 800) // Could use hypot(), but that's more expensive
    {
      lead_target(&attack_x, &attack_y); // lead_target() tries to point the process towards where the target is going.
        // attack_x and attack_y are updated by lead_target()
      attack_angle = atan2(attack_y, attack_x); // atan2 is a built-in function that calls the MATHS method.
          // It gets the absolute angle from the process to its target.
      if (angle_difference(attack_angle, angle) < ANGLE_16) // angle_difference is another MATHS built-in function.
          // It gets the absolute difference between two angles (taking bounds into account).
      {
       // tell the pods to fire:
       fire_instruction = 1; // this will be sent to the pods later
      }

    }
// Now tell the process to move towards its target.
// The attack_x/y values are relative to the process, so they need to be made absolute.
    target_x = x + attack_x;
    target_y = y + attack_y;
   }
    else
    {
// If the process can't find its target, it changes its mode to WANDER.
// At this stage target_x/y are not reset, so it will go to the last place it saw its target before wandering randomly.
     mode = MODE_WANDER;
    }
   break;

 } // end of switch(mode)

// If process is in MODE_CONNECTED, it will have jumped directly to the finished label below.

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

 left_instruction = 0;
 right_instruction = 0;

 if (turn_dir > 0)
  left_instruction = (angle_diff / 40);

 if (turn_dir < 0)
  right_instruction = (angle_diff / 40);

// Tell the pods to operate their move and packet methods:
 call(METH_LINK_LEFT, MS_PR_LINK_MESSAGE, left_instruction, fire_instruction);
 call(METH_LINK_RIGHT, MS_PR_LINK_MESSAGE, right_instruction, fire_instruction * 8); // stagger firing a bit

// Set up the main process' move methds:
 put(METH_MOVE1, MB_PR_MOVE_COUNTER, 16);
 put(METH_MOVE1, MB_PR_MOVE_DELAY, left_instruction);
 put(METH_MOVE2, MB_PR_MOVE_COUNTER, 16);
 put(METH_MOVE2, MB_PR_MOVE_DELAY, right_instruction);


 finished:

 // try to self-repair (does nothing if the process isn't damaged):
 call(METH_RESTORE, 2);



}

// This function makes the process start wandering and gives it a random destination:
static void start_wandering(void)
{

     mode = MODE_WANDER;
     target_x = random(world_size_x - 600) + 300;
     target_y = random(world_size_y - 600) + 300;

     mode_count = 120; // will wander for 120 cycles (about 60 seconds)

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
                    0, // x offset of scan centre (MB_PR_SCAN_X1) - scan is centred on process
                    0, // y offset of scan centre (MB_PR_SCAN_Y1) - scan is centred on process
                    10000, // size of scan (in pixels) (MB_PR_SCAN_SIZE) - very high value means maximum range (which is about 1000)
                    0, // (MBANK_PR_SCAN_SIZE2) - not relevant to this type of scan
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
//  (the DESIGNATE method allows the process to keep track of its target between execution cycles, without needing to scan again)
  designated = call(METH_DESIGNATE,
                    MS_PR_DESIGNATE_ACQUIRE, // This mode saves a target to it can be recovered later by the MS_PR_DESIGNATE_LOCATE mode
                    scan_result [0], // This is the location of the target as an x offset from the process
                    scan_result [1]); // Same for y

// The DESIGNATE method returns 1 if it is successful (which should always be the case here, but let's check anyway):
  if (designated == 1)
  {
   mode = MODE_ATTACK;
// Also set process' current target:
   target_x = x + scan_result [0];
   target_y = y + scan_result [1];
  }

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
 put(METH_SCAN, MB_PR_SCAN_START_ADDRESS, &scan_result [0]); // results are stored in scan_result
  // results are: [0] x [1] y [2] speed_x [3] speed_y
  // x and y are offsets from the x/y location of the examine

 scan_return = call(METH_SCAN); // returns 1 (success), 0 (failure)

 if (scan_return == 1)
 {
// Find the distance between the process and its target:
  dist = hypot(attack_y, attack_x) / 150; // hypot is a built-in function that calls the MATHS method. 150 is an approximation factor.
  *attack_x_ptr += (scan_result [2] - speed_x) * dist; // x speed (obtained during examine call above)
  *attack_y_ptr += (scan_result [3] - speed_y) * dist; // y speed (obtained during examine call above)
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




// The main process spawns a pod on either side. Each pod needs its own separate process definition.
// The pods are quite simple; each just needs to receive move and attack commands from the main process and perform them.
process pod_left
{

interface
{
 PROGRAM_TYPE_PROCESS, SHAPE_4POINTY, 2, 1,
 {
  METH_LINK: {MT_PR_LINK, 1, 0},
  METH_STD: {MT_PR_STD},
  METH_PACKET: {MT_PR_PACKET, 3, -200, 2, 0, 1},
  METH_MOVE: {MT_PR_MOVE, 0, 1950},
  METH_RESTORE: {MT_PR_RESTORE},
  METH_REDUNDANCY: {MT_PR_REDUNDANCY},
 }
}


int initialised;

int link_message [LINK_MESSAGES] [LINK_MESSAGE_SIZE]; // The LINK method can send and receive messages consisting of two ints.
// messages received from the parent process will be put in the link_message array.

static void main(void)
{

 if (initialised == 0)
 {
  put(METH_MOVE, MB_PR_MOVE_RATE, 40);
  put(METH_LINK, MB_PR_LINK_MESSAGE_ADDRESS, &link_message [0] [0]); // messages sent by the parent process will be stored in this address
  initialised = 1;
 }

// Check for messages from the main process:
 if (call(METH_LINK, MS_PR_LINK_RECEIVED))
 {
  put(METH_MOVE, MB_PR_MOVE_COUNTER, 16);
  put(METH_MOVE, MB_PR_MOVE_DELAY, link_message [0] [0]);
  put(METH_PACKET, MB_PR_PACKET_COUNTER, link_message [0] [1]);
 }

// try to self-repair (does nothing if the process isn't damaged):
 call(METH_RESTORE, 2);

} // end main

} // end process left_pod


// This is basically the same as the left pod, but its move and packet methods are on different vertices.

process pod_right
{

interface
{
 PROGRAM_TYPE_PROCESS, SHAPE_4POINTY, 2, 3,
 {
  METH_LINK: {MT_PR_LINK, 3, 0},
  METH_STD: {MT_PR_STD},
  METH_PACKET: {MT_PR_PACKET, 1, 200, 2, 0, 1},
  METH_MOVE: {MT_PR_MOVE, 0, -1950},
  METH_RESTORE: {MT_PR_RESTORE},
  METH_REDUNDANCY: {MT_PR_REDUNDANCY},
 }
}


int initialised;

int link_message [LINK_MESSAGES] [LINK_MESSAGE_SIZE]; // The LINK method can send and receive messages consisting of two ints.
// messages received from the parent process will be put here.

static void main(void)
{

 if (initialised == 0)
 {
  put(METH_MOVE, MB_PR_MOVE_RATE, 40);
  put(METH_LINK, MB_PR_LINK_MESSAGE_ADDRESS, &link_message [0] [0]); // messages sent by the parent process will be stored in this address
  initialised = 1;
 }

 if (call(METH_LINK, MS_PR_LINK_RECEIVED))
 {
  put(METH_MOVE, MB_PR_MOVE_COUNTER, 16);
  put(METH_MOVE, MB_PR_MOVE_DELAY, link_message [0] [0]);
  put(METH_PACKET, MB_PR_PACKET_COUNTER, link_message [0] [1]);
 }

 call(METH_RESTORE, 2);

} // end main

} // end process pod_right

