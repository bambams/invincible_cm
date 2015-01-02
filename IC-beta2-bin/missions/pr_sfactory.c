
/*

pr_sfactory.c

This process allocates data and uses it to create new processes.
It also builds defensive pods (but is much less effective than pr_pfactory).
The data allocation method makes it immobile.
If left to its own devices it will just produce processes from process template 1,  *** not 0!
 but it will also accept commands using the standard commands macros.
It is not set up to interact with a user
 (see m_pr_cfactory.c for another version of a factory that is designed to work with an operator program)

*/


#include "stand_coms.c"


// This is the process's interface definition. It determines the nature of the process and
// the methods it has available. It must come before any code.
interface
{
 PROGRAM_TYPE_PROCESS, SHAPE_6HEXAGON, 3, 3, // program type, shape, size (from 0-3), base_vertex
 {
  METH_IRPT: {MT_PR_IRPT, 0, 0, 4}, // generates irpt
  METH_ALLOCATE: {MT_PR_ALLOCATE, 2}, // allocates data from the surroundings. This is an external method; it will go on vertex 2.
  METH_STD: {MT_PR_STD}, // information method. allows access to process's current properties (location etc)
  METH_NEW: {MT_PR_NEW}, // new process method. Can build new processes.
  {0}, // new process method requires 2 method slots, so this one is empty
  METH_COMMAND: {MT_PR_COMMAND}, // command method allows the process to receive commands from the client program
  METH_STORAGE: {MT_PR_STORAGE, 0, 0, 0}, // storage method. stores additional data. The first two fields are unused (as it's internal); the third
    // is an extension (extension 0 for a storage method is a capacity extension which increases the storage space)
  METH_RESTORE: {MT_PR_RESTORE},
  METH_LINK0: {MT_PR_LINK, 0},
  METH_LINK1: {MT_PR_LINK, 1},
  METH_LINK2: {MT_PR_LINK, 3},
  METH_LINK3: {MT_PR_LINK, 5},
 }
}



// Now we declare some global variables.
// Remember that these are initialised (to zero, or to a specified value) when the process is
//  created, but not each time it is run.

int initialised; // When this is zero, it tells the process that it has just been created and needs to initialise.

// Initialisation values - these are set once, when the process is first run
int team; // This is set to the process's team (i.e. which player it is controlled by)

// Behaviour/mode values - these determine the process' current behaviour
int building; // what the factory is building now. Uses the BUILD_* enums below.
int build_wait; // is 1 if the factory is waiting for enough data/irpt to build
int data_needed; // this is how much data the factory needs for its current build
int spike_pod_cost; // cost of a pod. Set up at initialisation.

enum
{
BUILD_NOTHING, // Process isn't building anything
BUILD_TEMPLATE1, // Build whatever is in template 1
BUILD_TEMPLATE2, // Build whatever is in template 2
BUILD_TEMPLATE3, // Build whatever is in template 3
BUILD_TEMPLATE4 // Build whatever is in template 4
};

static void check_data_cost(int template_index);
static int try_building(int template_index);
static void build_spike_pod(int v);

process spike_pod;

static void main(void)
{

 int build_result;
 int build_source = 1;
 int command_received;
 int i;


 if (initialised == 0)
 {
  initialised = 1;
  team = get_team();
  building = BUILD_TEMPLATE2;
  check_data_cost(building);
// work out the cost of a spike pod:
  call(METH_NEW,
       MS_PR_NEW_BC_COST_DATA, // MB_PR_NEW_STATUS
       0, // MB_PR_NEW_VERTEX1: Vertex of this process that new process will be created at.
       -1, // MB_PR_NEW_VERTEX2: Vertex of child process (-1 means use child process' base_vertex value)
       0, // MB_PR_NEW_ANGLE: Angle of child process
       process_start(spike_pod), // MB_PR_NEW_START: Start address
       process_end(spike_pod)); // MB_PR_NEW_END: End address
  spike_pod_cost = get(METH_NEW, MB_PR_NEW_STATUS);
// Set up the process' command registers
  set_command_bit_1(COMREG_PROCESS_DETAILS, COM_PROCESS_DET_HP); // tells operator what details to display
  set_command_bit_1(COMREG_PROCESS_DETAILS, COM_PROCESS_DET_IRPT); // tells operator what details to display
  set_command_bit_1(COMREG_PROCESS_DETAILS, COM_PROCESS_DET_DATA); // tells operator what details to display
  set_command_bit_1(COMREG_PROCESS_DETAILS, COM_PROCESS_DET_ALLOC_EFF); // tells operator what details to display
 }

// Call the allocator to generate some data (unlike the IRPT method, it doesn't run automatically):
 call(METH_ALLOCATE); // can only be called once each cycle

 call(METH_RESTORE, 1);

// Now check for whether any of the spike_pods need to be built or rebuild:
 if (call(METH_LINK1, MS_PR_LINK_EXISTS) == 0)
 {
  build_spike_pod(1);
  return;
 }
 if (call(METH_LINK2, MS_PR_LINK_EXISTS) == 0)
 {
  build_spike_pod(3);
  return;
 }
 if (call(METH_LINK3, MS_PR_LINK_EXISTS) == 0)
 {
  build_spike_pod(5);
  return;
 }

 if (building != BUILD_NOTHING)
 {
  if (get_data() >= data_needed)
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
                     0, // MB_PR_NEW_VERTEX1: Vertex of this process that new process will be created at.
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
                     0, // MB_PR_NEW_VERTEX1: Vertex of this process that new process will be created at.
                     -1, // MB_PR_NEW_VERTEX2: Vertex of child process (-1 means use child process' base_vertex value)
                     0, // MB_PR_NEW_ANGLE: Angle of child process
                     0, // MB_PR_NEW_START: Start address
                     BCODE_SIZE_PROCESS - 1, // MB_PR_NEW_END: End address
                     1, // MB_PR_NEW_LINK: 1 means try to connect to new process
                     template_index); // MB_PR_NEW_TEMPLATE: Source template index

//   test_result = call(METH_NEW,
//                      MS_PR_NEW_T_COST_DATA);

   if (test_result == MR_NEW_TEST_SUCCESS)
   {
     build_wait = 1;
     data_needed = get(METH_NEW, MB_PR_NEW_STATUS);
     return;
   }

}


static void build_spike_pod(int v)
{

 if (get_data() < spike_pod_cost)
  return;

                call(METH_NEW,
                     MS_PR_NEW_BC_BUILD, // MB_PR_NEW_STATUS
                     v, // MB_PR_NEW_VERTEX1: Vertex of this process that new process will be created at.
                     -1, // MB_PR_NEW_VERTEX2: Vertex of child process (-1 means use child process' base_vertex value)
                     0, // MB_PR_NEW_ANGLE: Angle of child process
                     process_start(spike_pod), // MB_PR_NEW_START: Start address
                     process_end(spike_pod), // MB_PR_NEW_END: End address
                     1); // MB_PR_NEW_LINK: 1 means try to connect to new process

}

process spike_pod
{
// All this process does is scan a nearby area and protect its factory, while repairing itself if needed.

interface
{
 PROGRAM_TYPE_PROCESS, SHAPE_4POINTY, 3, 2, // if shape or size are changed, fix turret_x/y calculations below
 {
  METH_LINK: {MT_PR_LINK, 2},
  METH_STD: {MT_PR_STD},
  METH_DSTREAM: {MT_PR_DSTREAM, 0, 0, 0, 2, 0},
  METH_RESTORE: {MT_PR_RESTORE},
  METH_MATHS: {MT_PR_MATHS},
  METH_SCAN: {MT_PR_SCAN, 0, 0, 2}, // scan method. allows process to sense its surroundings. takes up 3 method slots
  {0}, // space for scan method
  {0}, // space for scan method
  METH_DESIGNATE: {MT_PR_DESIGNATE}, // designator. allows process to keep track of a target acquired through scan

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
int turret_x_offset, turret_y_offset; // location of DPACKET method as offsets from process x/y

// Behaviour/mode values - these determine the process' current behaviour
int mode; // Behavioural mode of the process (see the MODE enums below)
int mode_count; // Time the process will stay in current mode (only relevant for some modes)
int attack_x;
int attack_y;

// Scan values - used to scan for other nearby processes
int scan_result [16]; // actually only 4 values are used, but may want to extend later.
int scan_bitmask;


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
static void scan_for_target(void);
static void lead_target(void);
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
  scan_bitmask = 0b1111 ^ (1<<team); // scan_bitmask is used by the scanner to find processes that belong to a different team.
      // The four least significant bits of scan_bitmask tell the scanner to accept (1) or ignore (0) processes that belong to player 4, 3, 2 and 1.
      // 15 means accept all teams (1111).
      // ^ (1<<team) flips the bit corresponding to this process' team, so the scanner will ignore friendly processes.
      // For example, if this process belongs to player 2 the bits will be 1101.
// We can also set up some things that normally would be updated each cycle, but don't need to be as this process is immobile:
  angle = get_angle();
  x = get_x();
  y = get_y();
  turret_x_offset = cos(angle, data(DATA_SHAPE_VERTEX_DIST, SHAPE_4POINTY, 2, 0));
  turret_y_offset = sin(angle, data(DATA_SHAPE_VERTEX_DIST, SHAPE_4POINTY, 2, 0));
  initialised = 1; // Now it won't be initialised again
 }

 call(METH_RESTORE, 2);

// What the process does next depends on which mode it is in.
 switch(mode)
 {
  case MODE_IDLE:
// In IDLE mode, the process sits around scanning for a target.
   scan_for_target(); // If scan_for_target finds something to attack, it changes the mode.
   return; // Finish

  case MODE_ATTACK:
   if (call(METH_DESIGNATE, MS_PR_DESIGNATE_LOCATE) == 1)
   {
    attack_x = get(METH_DESIGNATE, 1) - turret_x_offset; // gets DESIGNATE method's register 1
    attack_y = get(METH_DESIGNATE, 2) - turret_y_offset; // gets DESIGNATE method's register 2
//    attack_x -= turret_x_offset;
//    attack_y -= turret_y_offset;
// target_x/y is used for both movement and attacking.
// If the target is roughly nearby this process, consider firing at it:
    if (abs(attack_y) + abs(attack_x) < 1000) // Could use hypot(), but that's more expensive
    {

      lead_target(); // lead_target() tries to point the process towards where the target is going.
        // attack_x and attack_y are updated by lead_target()

      attack_angle = atan2(attack_y, attack_x); // atan2 is a built-in function that calls the MATHS method.
          // It gets the absolute angle from the process to its target.
//      run_dpacket_method(METH_DPACKET, attack_angle - angle, 1);
      run_dstream_method(METH_DSTREAM, signed_angle_difference(angle, attack_angle), 1);

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

 int scan_return, scan_x, scan_y;

 scan_x = turret_x_offset * 4;
 scan_y = turret_y_offset * 4;
// scan_x = 0;
// scan_y = 0;

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

// scan_return now holds the number of processes that the scan found (which will be no more than 1, because of the MB_PR_SCAN_NUMBER set above)
// scan_result [0] and [1] now hold the x/y coordinates (as offsets from the process performing the scan)
//  - if more than one target is found (impossible with the settings used here),
//    they will be in scan_result in *roughly* ascending order of distance from the process.

 if (scan_return > 0)
 {
// We found something! So now we tell the DESIGNATE method to lock onto the target.
//  (the DESIGNATE method allows the process to keep track of its target between execution cycles)
  designated = call(METH_DESIGNATE,
                    MS_PR_DESIGNATE_ACQUIRE, // This mode saves a target to it can be recovered later by the MS_PR_DESIGNATE_LOCATE mode
                    scan_result [0] + scan_x, // This is the location of the target as an x offset from the process
                    scan_result [1] + scan_y); // Same for y

// The DESIGNATE method returns 1 if it is successful (which should always be the case here, but let's check anyway):
  if (designated == 1)
  {
   mode = MODE_ATTACK;
   mode_count = 12;
  }

 }
  else
  {
     put(METH_DSTREAM, MB_PR_DSTREAM_ANGLE, 0);
  }


}

// This function adjusts attack_x_ptr and attack_y_ptr to take account of process and target movement.
//  attack_x_ptr and attack_y_ptr are are pointers to the target coordinates.
//  The coordinates must be relative to the process' own location.
// Note that the compiler's somewhat limited type system doesn't recognise pointers. Instead, you can use
//  ordinary ints as pointers.
static void lead_target(void)
{

 int dist;
 int scan_return;

// To find the target's speed, we use the SCAN method's secondary EXAMINE mode.
// Because most of the SCAN method's registers aren't relevant to EXAMINE mode, we set the ones that are used
//  individually instead of putting them all in as arguments to a big call().
// We need to set the following:
 put(METH_SCAN, MB_PR_SCAN_STATUS, MS_PR_SCAN_EXAMINE); // sets the method's status to EXAMINE
 put(METH_SCAN, MB_PR_SCAN_X1, attack_x + turret_x_offset); // location of target, as an offset (in pixels) from process
 put(METH_SCAN, MB_PR_SCAN_Y1, attack_y + turret_y_offset); // y offset
 put(METH_SCAN, MB_PR_SCAN_START_ADDRESS, &scan_result [0]); // results are also stored in scan_result
  // results are: [0] x [1] y [2] speed_x [3] speed_y
  // x and y are offsets from the x/y centre of the examine

 scan_return = call(METH_SCAN); // returns 1 (success), 0 (failure)

 if (scan_return == 1)
 {
// Find the distance between the process and its target:
  dist = hypot(attack_y, attack_x) / 110; // hypot is a built-in function that calls the MATHS method. The constant is an approximation factor.
  attack_x += (scan_result [2]) * dist; // x speed (obtained during examine call above)
  attack_y += (scan_result [3]) * dist; // y speed (obtained during examine call above)
 }

}

static void run_dstream_method(int m, int target_angle, int delay)
{

 int method_angle;
 int rand_seed;

 method_angle = call(m);

 put_index((m * 4) + MB_PR_DSTREAM_ANGLE, target_angle - 200 + (rand_seed % 400)); // - 400 + random(800));
 rand_seed += 430;

 if (angle_difference(target_angle, method_angle) < ANGLE_16) // angle_difference is another MATHS built-in function.
          // It gets the absolute difference between two angles (taking bounds into account).
 {
// Make sure we've got enough irpt to fire:
  if (get_irpt() > 300) // get_irpt() is a built-in INFO function.
  {
   put(METH_DSTREAM, MB_PR_DSTREAM_FIRE, 1);
  }
// put sets a method's register to a specified value.
// This put sets register 0 (MB_PR_PACKET_COUNTER) to 1. This tells the packet method to
//  fire as soon as possible.
 }

}





}


/*
process spike_pod
{
// All this process does is scan a nearby area and protect its factory, while repairing itself if needed.

interface
{
 PROGRAM_TYPE_PROCESS, SHAPE_4POINTY, 2, 2, // if shape or size are changed, fix turret_x/y calculations below
 {
  {MT_PR_LINK, 2},
  {MT_PR_INFO},
  {MT_PR_DPACKET, 0, 0, 0, 1, 3},
  {MT_PR_RESTORE},
  {MT_PR_MATHS},
  {MT_PR_SCAN, 0, 0, 2}, // scan method. allows process to sense its surroundings. takes up 3 method slots
  {0}, // space for scan method
  {0}, // space for scan method
  {MT_PR_DESIGNATE}, // designator. allows process to keep track of a target acquired through scan

 }
}

enum
{
METH_LINK,
METH_INFO,
METH_DPACKET,
METH_RESTORE,
METH_MATHS,
METH_SCAN,
METH_SCAN2,
METH_SCAN3,
METH_DESIGNATE
};


// Now we declare some global variables.
// Remember that these are initialised (to zero, or to a specified value) when the process is
//  created, but not each time it is run.

int initialised; // When this is zero, it tells the process that it has just been created and needs to initialise.

// Initialisation values - these are set once, when the process is first run
int team; // This is set to the process's team (i.e. which player it is controlled by)

// Current state values - these are set each time the process runs
int angle; // Angle the process is pointing (in integer format, from 0 to ANGLE_1 (currently 8192))
int x, y; // Location
int turret_x_offset, turret_y_offset; // location of DPACKET method as offsets from process x/y

// Behaviour/mode values - these determine the process' current behaviour
int mode; // Behavioural mode of the process (see the MODE enums below)
int mode_count; // Time the process will stay in current mode (only relevant for some modes)
int attack_x;
int attack_y;

// Scan values - used to scan for other nearby processes
int scan_result [16]; // actually only 4 values are used, but may want to extend later.
int scan_bitmask;


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
static void scan_for_target(void);
static void lead_target(void);
static void run_dpacket_method(int m, int target_angle, int delay);

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
  scan_bitmask = 0b1111 ^ (1<<team); // scan_bitmask is used by the scanner to find processes that belong to a different team.
      // The four least significant bits of scan_bitmask tell the scanner to accept (1) or ignore (0) processes that belong to player 4, 3, 2 and 1.
      // 15 means accept all teams (1111).
      // ^ (1<<team) flips the bit corresponding to this process' team, so the scanner will ignore friendly processes.
      // For example, if this process belongs to player 2 the bits will be 1101.
// We can also set up some things that normally would be updated each cycle, but don't need to be as this process is immobile:
  angle = get_angle();
  x = get_x();
  y = get_y();
  turret_x_offset = cos(angle, data(DATA_SHAPE_VERTEX_DIST, SHAPE_4POINTY, 2, 0));
  turret_y_offset = sin(angle, data(DATA_SHAPE_VERTEX_DIST, SHAPE_4POINTY, 2, 0));
  initialised = 1; // Now it won't be initialised again
 }

 call(METH_RESTORE, 2);

// What the process does next depends on which mode it is in.
 switch(mode)
 {
  case MODE_IDLE:
// In IDLE mode, the process sits around scanning for a target.
   scan_for_target(); // If scan_for_target finds something to attack, it changes the mode.
   return; // Finish

  case MODE_ATTACK:
   if (call(METH_DESIGNATE, MS_PR_DESIGNATE_LOCATE) == 1)
   {
    attack_x = get(METH_DESIGNATE, 1) - turret_x_offset; // gets DESIGNATE method's register 1
    attack_y = get(METH_DESIGNATE, 2) - turret_y_offset; // gets DESIGNATE method's register 2
//    attack_x -= turret_x_offset;
//    attack_y -= turret_y_offset;
// target_x/y is used for both movement and attacking.
// If the target is roughly nearby this process, consider firing at it:
    if (abs(attack_y) + abs(attack_x) < 1000) // Could use hypot(), but that's more expensive
    {

      lead_target(); // lead_target() tries to point the process towards where the target is going.
        // attack_x and attack_y are updated by lead_target()

      attack_angle = atan2(attack_y, attack_x); // atan2 is a built-in function that calls the MATHS method.
          // It gets the absolute angle from the process to its target.
//      run_dpacket_method(METH_DPACKET, attack_angle - angle, 1);
      run_dpacket_method(METH_DPACKET, signed_angle_difference(angle, attack_angle), 1);

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

 int scan_return, scan_x, scan_y;

 scan_x = turret_x_offset * 4;
 scan_y = turret_y_offset * 4;
// scan_x = 0;
// scan_y = 0;

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

// scan_return now holds the number of processes that the scan found (which will be no more than 1, because of the MB_PR_SCAN_NUMBER set above)
// scan_result [0] and [1] now hold the x/y coordinates (as offsets from the process performing the scan)
//  - if more than one target is found (impossible with the settings used here),
//    they will be in scan_result in *roughly* ascending order of distance from the process.

 if (scan_return > 0)
 {
// We found something! So now we tell the DESIGNATE method to lock onto the target.
//  (the DESIGNATE method allows the process to keep track of its target between execution cycles)
  designated = call(METH_DESIGNATE,
                    MS_PR_DESIGNATE_ACQUIRE, // This mode saves a target to it can be recovered later by the MS_PR_DESIGNATE_LOCATE mode
                    scan_result [0] + scan_x, // This is the location of the target as an x offset from the process
                    scan_result [1] + scan_y); // Same for y

// The DESIGNATE method returns 1 if it is successful (which should always be the case here, but let's check anyway):
  if (designated == 1)
  {
   mode = MODE_ATTACK;
   mode_count = 12;
  }

 }
  else
  {
     put(METH_DPACKET, MB_PR_DPACKET_ANGLE, 0);
  }


}

// This function adjusts attack_x_ptr and attack_y_ptr to take account of process and target movement.
//  attack_x_ptr and attack_y_ptr are are pointers to the target coordinates.
//  The coordinates must be relative to the process' own location.
// Note that the compiler's somewhat limited type system doesn't recognise pointers. Instead, you can use
//  ordinary ints as pointers.
static void lead_target(void)
{

 int dist;
 int scan_return;

// To find the target's speed, we use the SCAN method's secondary EXAMINE mode.
// Because most of the SCAN method's registers aren't relevant to EXAMINE mode, we set the ones that are used
//  individually instead of putting them all in as arguments to a big call().
// We need to set the following:
 put(METH_SCAN, MB_PR_SCAN_STATUS, MS_PR_SCAN_EXAMINE); // sets the method's status to EXAMINE
 put(METH_SCAN, MB_PR_SCAN_X1, attack_x + turret_x_offset); // location of target, as an offset (in pixels) from process
 put(METH_SCAN, MB_PR_SCAN_Y1, attack_y + turret_y_offset); // y offset
 put(METH_SCAN, MB_PR_SCAN_START_ADDRESS, &scan_result [0]); // results are also stored in scan_result
  // results are: [0] x [1] y [2] speed_x [3] speed_y
  // x and y are offsets from the x/y centre of the examine

 scan_return = call(METH_SCAN); // returns 1 (success), 0 (failure)

 if (scan_return == 1)
 {
// Find the distance between the process and its target:
  dist = hypot(attack_y, attack_x) / 110; // hypot is a built-in function that calls the MATHS method. The constant is an approximation factor.
  attack_x += (scan_result [2]) * dist; // x speed (obtained during examine call above)
  attack_y += (scan_result [3]) * dist; // y speed (obtained during examine call above)
 }

}

static void run_dpacket_method(int m, int target_angle, int delay)
{

 int method_angle;
 int rand_seed;

 method_angle = call(m);

 put_index((m * 4) + MB_PR_DPACKET_ANGLE, target_angle - 200 + (rand_seed % 400)); // - 400 + random(800));
 rand_seed += 430;

 if (angle_difference(target_angle, method_angle) < ANGLE_16) // angle_difference is another MATHS built-in function.
          // It gets the absolute difference between two angles (taking bounds into account).
 {
// Make sure we've got enough irpt to fire:
  if (get_irpt() > 100) // get_irpt() is a built-in INFO function.
   put(METH_DPACKET, MB_PR_DPACKET_COUNTER, delay);
// put sets a method's register to a specified value.
// This put sets register 0 (MB_PR_PACKET_COUNTER) to 1. This tells the packet method to
//  fire as soon as possible.
 }

}





}
*/
