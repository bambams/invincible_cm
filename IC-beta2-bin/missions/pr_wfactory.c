

/*

pr_wfactory.c

A factory that produces wanderers that produce factories. Can quickly spread over
the whole map.

*/

#ifndef STANDARD_COMMANDS
#include "stand_coms.c"
#endif

// This is the process's interface definition. It determines the nature of the process and
// the methods it has available. It must come before any code.
interface
{
 PROGRAM_TYPE_PROCESS, SHAPE_8OCTAGON, 3, 0, // program type, shape, size (from 0-3), base_vertex
 {
  METH_IRPT: {MT_PR_IRPT}, // generates irpt
  METH_ALLOCATE: {MT_PR_ALLOCATE, 4}, // allocates data from the surroundings. This is an external method; it will go on vertex 2.
  METH_STD: {MT_PR_STD}, // information method. allows access to process's current properties (location etc)
  METH_NEW: {MT_PR_NEW}, // new process method. Can build new processes.
  {0}, // new process method requires 2 method slots, so this one is empty
  METH_COMMAND: {MT_PR_COMMAND}, // command method allows the process to receive commands from the client program
    // This process doesn't need to be given commends (it builds automatically from template 1) but can
    //  be told to build from a specific template
  METH_STORAGE: {MT_PR_STORAGE, 0, 0, 3}, // storage method. stores additional data. The first two fields are unused (as it's internal); the third
    // is an extension (extension 0 for a storage method is a capacity extension which increases the storage space)
  METH_LINK: {MT_PR_LINK, 0}

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
int build_count; // counts to 4 then builds a wanderer
int data_needed; // this is how much data the factory needs for its current build
int wanderer_data_cost;

// the following values can be set by system etc using asm{scope}
int wanderer_rate = 3; // how often will wanderers be built?
int build_count_init = 0; // how close is the process to building a wanderer when first created?


enum
{
BUILD_NOTHING, // Process isn't building anything
BUILD_TEMPLATE0, // Build whatever is in template 0
BUILD_TEMPLATE1, // Build whatever is in template 1
BUILD_TEMPLATE2, // Build whatever is in template 2
BUILD_TEMPLATE3, // Build whatever is in template 3
BUILD_WANDERER
};

process wanderer;

static void check_data_cost(void);
static int try_building(int template_index);

static void main(void)
{

 int command_received;
 int i;


 if (initialised == 0)
 {
  initialised = 1;
  team = get_team();
  call(METH_NEW,
       MS_PR_NEW_BC_COST_DATA, // MB_PR_NEW_STATUS: means: after this process finishes executing, try to build
       0, // MB_PR_NEW_VERTEX1: Vertex of this process that new process will be created at.
       -1, // MB_PR_NEW_VERTEX2: Vertex of child process (-1 means use child process' base_vertex value)
       0, // MB_PR_NEW_ANGLE: Angle of child process
       process_start(wanderer), // MB_PR_NEW_START: Start address
       process_end(wanderer)); // MB_PR_NEW_END: End address
  wanderer_data_cost = get(METH_NEW, MB_PR_NEW_STATUS);
  build_count = build_count_init; // will build from template 2 and increment this until it's 3, then build a wanderer and set this to 0
  building = BUILD_TEMPLATE1;
  if (build_count >= wanderer_rate) // this can happen if build_count_init is set to a high number by a system program using asm scope
		{
	  building = BUILD_WANDERER;
	  build_count = 0;
		}
  check_data_cost();
// Set up the process' command registers (not strictly necessary as this is an autonomous process, but can't hurt)
  set_command_bit_1(COMREG_PROCESS_DETAILS, COM_PROCESS_DET_HP); // tells operator what details to display
  set_command_bit_1(COMREG_PROCESS_DETAILS, COM_PROCESS_DET_IRPT); // tells operator what details to display
  set_command_bit_1(COMREG_PROCESS_DETAILS, COM_PROCESS_DET_DATA); // tells operator what details to display
  set_command_bit_1(COMREG_PROCESS_DETAILS, COM_PROCESS_DET_ALLOC_EFF); // tells operator what details to display
 }

// Call the allocator to generate some data (unlike the IRPT method, it doesn't run automatically):
 call(METH_ALLOCATE); // can only be called once each cycle

// Now check for a process attached to the LINK method.
 if (call(METH_LINK, MS_PR_LINK_EXISTS)) // returns 1 if connected, 0 otherwise
 {
  return; // finished
 }
// Assume that the attached process will disconnect when it's finished building or accumulating data.

 if (building != BUILD_NOTHING)
 {
  if (get_data() >= data_needed)
   try_building(building);
 }

// Now we check the command registers to see whether the process' client program has given it a command
//  (although it will just build by itself if no commands are given)
 command_received = get_command(COMREG_QUEUE);

 switch(command_received)
 {

  case COMMAND_BUILD:
// Clear the command:
    set_command(COMREG_QUEUE, COMMAND_NONE);
// The source template index should be in command field COMREG_1:
    building = get_command(COMREG_QUEUE + 1) + BUILD_TEMPLATE0;
    check_data_cost();
    break; // end COMMAND_BUILD

  case COMMAND_IDLE:
   building = BUILD_NOTHING; // stop trying to build something
// Clear the command:
   set_command(COMREG_QUEUE, COMMAND_NONE);
   break;

 } // end switch(command_received)

}

// Returns 1 on success, 0 on failure
static int try_building(int template_index)
{

 int build_result;

 if (building == BUILD_WANDERER)
	{
  build_result = call(METH_NEW, MS_PR_NEW_BC_BUILD, 0, -1, 0, process_start(wanderer), process_end(wanderer), 1);
	}
	 else
		{
   build_result = call(METH_NEW,
                     MS_PR_NEW_T_BUILD, // MB_PR_NEW_STATUS: means: after this process finishes executing, try to build
                     0, // MB_PR_NEW_VERTEX1: Vertex of this process that new process will be created at.
                     -1, // MB_PR_NEW_VERTEX2: Vertex of child process (-1 means use child process' base_vertex value)
                     0, // MB_PR_NEW_ANGLE: Angle of child process
                     0, // MB_PR_NEW_START: Start address
                     BCODE_SIZE_PROCESS - 1, // MB_PR_NEW_END: End address
                     1, // MB_PR_NEW_LINK: 1 means try to connect to new process
                     template_index - BUILD_TEMPLATE0); // MB_PR_NEW_TEMPLATE: Source template index
	 }

 if (build_result == MR_NEW_SUCCESS)
 {
 	build_count ++;
 	if (build_count >= wanderer_rate)
		{
			build_count = 0;
			building = BUILD_WANDERER;
   check_data_cost();
		}
			else
			{
				building = BUILD_TEMPLATE1;
    check_data_cost();
			}
  return 1;
 }

 if (build_result == MR_NEW_FAIL_DATA)
 {
// could happen if a template is changed suddenly. Need to recalculate build cost.
  check_data_cost();
 }

 return 0;

}


// Only call this function just after a failed call to the NEW method.
// It assumes that the NEW method's mbank registers are still in the
//  state generated for the failed NEW call.
static void check_data_cost(void)
{

  int test_result;

  if (building == BUILD_WANDERER)
		{
			data_needed = wanderer_data_cost;
			return;
		}

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
                     building - BUILD_TEMPLATE0); // MB_PR_NEW_TEMPLATE: Source template index


   if (test_result == MR_NEW_TEST_SUCCESS)
   {
     data_needed = get(METH_NEW, MB_PR_NEW_STATUS);
     return;
   }

// Not sure what to do if the test fails (it really shouldn't, though)

}

process wanderer
{
#include "pr_wander.c"
} // end process wanderer
