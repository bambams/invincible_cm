
/*

pr_double.c

This is a simple process that wanders around looking for a target, then attacks it.
Unlike pr_attack and pr_attack2, it has rotating packet methods that can attack separately.
It doesn't accept commands or interact in any way with a client/observer program.

*/

#ifndef STANDARD_COMMANDS
#include "stand_coms.c"
#endif

// This is the process's interface definition. It determines the nature of the process and
// the methods it has available. It must come before any code.
interface
{
 PROGRAM_TYPE_PROCESS, SHAPE_6HEXAGON, 3, 3, // program type, shape, size (from 0-3), base_vertex
 {
  METH_MOVE1: {MT_PR_MOVE, 2, ANGLE_2 - data(DATA_SHAPE_VERTEX_ANGLE, SHAPE_6HEXAGON, 2), 0}, // acceleration method on vertex 1
  METH_MOVE2: {MT_PR_MOVE, 4, ANGLE_2 - data(DATA_SHAPE_VERTEX_ANGLE, SHAPE_6HEXAGON, 4), 0}, // acceleration method on vertex 3
//  {MT_EX_COMMAND}, // command method. allows process to communicate with operator/delegate
  METH_MATHS: {MT_PR_MATHS}, // maths method. allows trigonometry etc.
  METH_STD: {MT_PR_STD}, // information method. allows access to process's current properties (location etc)
  METH_IRPT: {MT_PR_IRPT, 0, 0, 1}, // generates irpt. functions automatically (although can be configured not to)
  METH_DPACKET1: {MT_PR_DPACKET, 1, 0, 0, 1, 2}, // directional packet method. allows process to attack
  METH_DPACKET2: {MT_PR_DPACKET, 5, 0, 0, 1, 2}, // directional packet method. allows process to attack
  METH_SCAN: {MT_PR_SCAN, 0, 0, 0}, // scan method. allows process to sense its surroundings. takes up 3 method slots
  {0}, // space for scan method
  {0}, // space for scan method
  METH_DESIGNATE: {MT_PR_DESIGNATE}, // designator. allows process to keep track of a target acquired through scan
  METH_COMMAND: {MT_PR_COMMAND}, // information method. allows access to process's current properties (location etc)

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

// Scan values - used to scan for other nearby processes
int scan_result [32]; // actually only 4 values are used, but may want to extend later.
int scan_bitmask;


// Mode enum - these are for the process' mode value
enum
{
MODE_IDLE, // Process is sitting around, scanning for something to attack
MODE_WANDER, // Process is wandering randomly and scanning for something to attack
MODE_MOVE, // Process is moving to a known location (doesn't run scan)
MODE_ATTACK // Process has a designated target and is attacking it

};

// Function declarations
// Note that "static" in a function declaration means something different from normal C.
//  - It means that all variables in the function are static, which is recommended as
//    static variables are much more efficient than automatic variables (which you
//    shouldn't use without a good reason)
static void scan_for_target(void);
static void start_wandering(void);
static void lead_target(int attack_x_ptr, int attack_y_ptr);
static void run_dpacket_method(int m, int target_angle, int delay);
static int random(int max);

// Remember that main is called every time the program is run.
// Its correct type is static void main(void)
static void main(void)
{

// These variables are used below:
 int target_angle;
 int angle_diff;
 int turn_dir;
 int attack_x;
 int attack_y;
 int attack_angle;
 int future_turn_dir;

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
  mode = MODE_MOVE; // Tell the process to move forwards 200 pixels
  target_x = x + cos(angle, 200); // target_x and target_y are used in the movement code below
  target_y = y + sin(angle, 200); // cos and sin are built-in functions that call the MATHS method.
                                  // These calls multiply the result of cos/sin by 200 (to get the x/y components
                                  //  of a line extending 200 pixels to the front of the process)
  world_size_x = get_world_x(); // This is the size of the entire playing area, in pixels
  world_size_y = get_world_y();
  scan_bitmask = 15 ^ (1<<team); // scan_bitmask is used by the scanner to find processes that belong to a different team.
      // The four least significant bits of scan_bitmask tell the scanner to accept (1) or ignore (0) processes that belong to player 4, 3, 2 and 1.
      // 15 means accept all teams (1111).
      // ^ (1<<team) flips the bit corresponding to this process' team, so the scanner will ignore friendly processes.
      // For example, if this process belongs to player 2 the bits will be 1101.
  initialised = 1; // Now it won't be initialised again
 }

// What the process does next depends on which mode it is in.
 switch(mode)
 {
  case MODE_IDLE:
// In IDLE mode, the process sits around scanning for a target.
   scan_for_target(); // If scan_for_target finds something to attack, it changes the mode.
   return; // Finish

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
// Now check each of the two dpacket methods to see if they're pointing at the target:

      int adjusted_y;
      int adjusted_x;

      adjusted_y = attack_y - sin(data(DATA_SHAPE_VERTEX_ANGLE, SHAPE_6HEXAGON, 1) + angle, data(DATA_SHAPE_VERTEX_DIST, SHAPE_6HEXAGON, 2, 1));
      adjusted_x = attack_x - cos(data(DATA_SHAPE_VERTEX_ANGLE, SHAPE_6HEXAGON, 1) + angle, data(DATA_SHAPE_VERTEX_DIST, SHAPE_6HEXAGON, 2, 1));

      attack_angle = atan2(adjusted_y, adjusted_x); // atan2 is a built-in function that calls the MATHS method.

      run_dpacket_method(METH_DPACKET1, signed_angle_difference(data(DATA_SHAPE_VERTEX_ANGLE, SHAPE_6HEXAGON, 1), attack_angle - angle), 1);

      adjusted_y = attack_y - sin(data(DATA_SHAPE_VERTEX_ANGLE, SHAPE_6HEXAGON, 5) + angle, data(DATA_SHAPE_VERTEX_DIST, SHAPE_6HEXAGON, 2, 5));
      adjusted_x = attack_x - cos(data(DATA_SHAPE_VERTEX_ANGLE, SHAPE_6HEXAGON, 5) + angle, data(DATA_SHAPE_VERTEX_DIST, SHAPE_6HEXAGON, 2, 5));

      attack_angle = atan2(adjusted_y, adjusted_x);

      run_dpacket_method(METH_DPACKET2, signed_angle_difference(data(DATA_SHAPE_VERTEX_ANGLE, SHAPE_6HEXAGON, 5), attack_angle - angle), 9);
//      run_dpacket_method(METH_DPACKET2, ((attack_angle - angle) - data(DATA_SHAPE_VERTEX_ANGLE, SHAPE_6HEXAGON, 5)) & ANGLE_MASK, 9);
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


// At this point, all we have left to do is make the process move towards its target, which is at (target_x, target_y)

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

// also, reset the dpacket angles so that they return to centre:
     put(METH_DPACKET1, MB_PR_DPACKET_ANGLE, 0);
     put(METH_DPACKET2, MB_PR_DPACKET_ANGLE, 0);


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
                    10000, // size of scan (in pixels) (MBANK_PR_SCAN_SIZE) - very high value means maximum range (which is about 1000)
                    0, // (MBANK_PR_SCAN_Y2) - not relevant to this type of scan
                    scan_bitmask, // this bitmask indicates which targets will be found (MBANK_PR_SCAN_BITFIELD_WANT) - set up at initialisation
                    0, // bitmask for accepting only certain targets (MBANK_PR_SCAN_BITFIELD_NEED) - not used here
                    0); // bitmask for rejecting certain targets (MBANK_PR_SCAN_BITFIELD_REJECT) - not used here

// scan_return now holds the number of processes that the scan found (which will be no more than 1, because of the MB_PR_SCAN_NUMBER set above)
// scan_result [0] and [1] now hold the x/y coordinates (as offsets from the process performing the scan)
//  - if more than one target is found (impossible with the settings used here),
//    they will be in scan_result in *roughly* ascending order of distance from the process.

/*
 put(METH_SCAN, MB_PR_SCAN_STATUS, MS_PR_SCAN_SCAN); // scan (not examine)
 put(METH_SCAN, MB_PR_SCAN_NUMBER, 1); // number of procs to find
 put(METH_SCAN, MB_PR_SCAN_CENTRE_X, 0); // need to clear target values that were used for examine mode
 put(METH_SCAN, MB_PR_SCAN_CENTRE_Y, 0);

 put(METH_SCAN, MB_PR_SCAN_START_ADDRESS, &scan_result [0]);
 put(METH_SCAN, MB_PR_SCAN_BITFIELD_WANT, scan_bitmask);
 put(METH_SCAN, MB_PR_SCAN_DISTANCE, 22000);
 scan_return = call(METH_SCAN);*/

 if (scan_return > 0)
 {
// We found something! So now we tell the DESIGNATE method to lock onto the target.
//  (the DESIGNATE method allows the process to keep track of its target between execution cycles)
  designated = call(METH_DESIGNATE,
                    MS_PR_DESIGNATE_ACQUIRE, // This mode saves a target to it can be recovered later by the MS_PR_DESIGNATE_READ mode
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

static void run_dpacket_method(int m, int target_angle, int delay)
{

 int method_angle;

 method_angle = call(m);
// target_angle -= data(DATA_SHAPE_VERTEX_ANGLE, SHAPE_6HEXAGON, method_vertex);

 put_index((m * 4) + MB_PR_DPACKET_ANGLE, target_angle - 200 + random(400));

 if (angle_difference(target_angle, method_angle) < ANGLE_16) // angle_difference is another MATHS built-in function.
          // It gets the absolute difference between two angles (taking bounds into account).
 {
// Make sure we've got enough irpt to fire:
  if (get_irpt() > 100) // get_irpt() is a built-in INFO function.
   put_index((m * 4) + MB_PR_DPACKET_COUNTER, delay);
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

