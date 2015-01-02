
/*

prsc_cfactory.c

This process allocates data and uses it to create new processes.
The data allocation method makes it immobile.
It can also transfer data to mobile builder processes, which can build new factories.
It can't operate autonomously and needs to be given commands by a client program.
It relies on the standard command macros (which should be #defined in the system or operator program).

Since it has an allocate method, it shouldn't be placed too close to other processes that also have allocate
 methods (as this will reduce both process' allocator efficiency).

*/

// It's best to put an #ifndef around this #include, even though the included file has one too,
//  to reduce the number of files the preprocessor needs to load.
#ifndef STANDARD_COMMANDS
#include "stand_coms.c"
#endif

// This is the process's interface definition. It determines the nature of the process and
// the methods it has available. It must come before any code.
interface
{
 PROGRAM_TYPE_PROCESS, SHAPE_6POINTY, 3, 0, // program type, shape, size (from 0-3), base vertex
 {
  METH_IRPT: {MT_PR_IRPT}, // generates irpt
  METH_ALLOCATE: {MT_PR_ALLOCATE, 4}, // allocates data from the surroundings. This is an external method; it will go on vertex 2.
  METH_STD: {MT_PR_STD}, // information method. allows access to process's current properties (location etc)
  METH_NEW: {MT_PR_NEW}, // new process method. Can build new processes.
  METH_NEW2: {0}, // new process method requires 2 method slots, so this one is empty
  METH_COMMAND: {MT_PR_COMMAND}, // command method allows the process to receive commands from the client program
  METH_YIELD: {MT_PR_YIELD, 2}, // yield method. process can use this to transfer data/irpt to other nearby processes
  METH_LISTEN: {MT_PR_LISTEN}, // listen method. can receive broadcasts from other nearby processes
  METH_DESIGNATE: {MT_PR_DESIGNATE}, // designate method. used to target builder processes that request data transfer.
  METH_STORAGE: {MT_PR_STORAGE, 0, 0, 3}, // storage method. stores additional data.
  METH_LINK: {MT_PR_LINK, 0}, // link method

 }
}


// These process prototypes declare some subprocesses that are defined later:
process basic_attack_process;
process builder;

// Now we declare some global variables.
// Remember that these are initialised (to zero, or to a specified value) when the process is
//  created, but not each time it is run.

int initialised; // When this is zero, it tells the process that it has just been created and needs to initialise.

// Initialisation values - these are set once, when the process is first run
int team; // This is set to the process's team (i.e. which player it is controlled by)

// Behaviour/mode values - these determine the process' current behaviour
int building; // what the factory is building now. Uses the BUILD_* enums below.
int build_wait; // is 1 if the factory is waiting for enough data/irpt to build
int build_type; // method status for the type of build command we're waiting for (MS_PR_NEW_BC_BUILD or MS_PR_NEW_T_BUILD)
int build_source; // One of the BUILD_* enums below
int build_start_address; // start address of the build we're waiting for
int build_end_address; // end address of the build we're waiting for
int builder_cost;
int need_data; // this is how much data the factory needs for its current build
int keep_building; // if 1, the factory will keep building after it finishes what it's currently building
int verbose; // Is 1 if client has told it to talk about what it's doing.
int transferring_data; // Is 1 if process is transferring data to a builder.

// Listen data - the LISTEN method will write to this if it receives a broadcast
// The array must be large enough to store all messages that might be received
//  (currently this means up to 8 messages of 5 size each)
int listen_data [LISTEN_MESSAGES] [LISTEN_MESSAGE_SIZE];

enum
{
BUILD_UNUSED, // this value not used (as the operator ignores zero actions)
BUILD_NOTHING, // Process isn't building anything
BUILD_BUILDER, // Build builder
BUILD_TEMPLATE1, // Build whatever is in template 1
BUILD_TEMPLATE2, // Build whatever is in template 2
BUILD_TEMPLATE3, // Build whatever is in template 3
BUILD_TEMPLATE4, // Build whatever is in template 4
BUILD_STOP_TRANSFER // Stop transferring data
};

static void need_resources(int build_result, int test_type);
static void query_process(void);
static void execute_command(void);
static void process_action(void);
static void print_build_result(int build_result);


static void main(void)
{

 int build_result;
 int designate_result;;
 int target_x, target_y; // used for data transfer
 int yield_result;
 int command_received;
 int i;

// operator can asked the process to tell the user what it's doing by setting the verbose bit of the client status command register:
 verbose = get_command_bit(COMREG_CLIENT_STATUS, COM_CLIENT_ST_VERBOSE);
 set_command_bit_0(COMREG_CLIENT_STATUS, COM_CLIENT_ST_VERBOSE); // clear the verbose command register (the operator will reset it to 1 if needed)

 if (initialised == 0)
 {
  initialised = 1;
  team = get_team();
  building = BUILD_NOTHING;
  keep_building = 0;
// Set up the process' command registers
  set_command_bit_1(COMREG_PROCESS_DETAILS, COM_PROCESS_DET_COMMAND); // tells operator that this process accepts commands.
  set_command_bit_1(COMREG_PROCESS_DETAILS, COM_PROCESS_DET_HP); // tells operator what details to display
  set_command_bit_1(COMREG_PROCESS_DETAILS, COM_PROCESS_DET_IRPT); // tells operator what details to display
  set_command_bit_1(COMREG_PROCESS_DETAILS, COM_PROCESS_DET_DATA); // tells operator what details to display
  set_command_bit_1(COMREG_PROCESS_DETAILS, COM_PROCESS_DET_ALLOC_EFF); // tells operator what details to display
  set_command_bit_1(COMREG_PROCESS_DETAILS, COM_PROCESS_DET_NO_MULTI); // tells operator not to select as part of a group selection

// Set up the process' listen method registers to accept broadcasts from nearby builder processes, which may request data:
  put(METH_LISTEN, MB_PR_LISTEN_ID_WANT, // MB_PR_LISTEN_ID_WANT is 0
      MSGID_BUILDER, // MB_PR_LISTEN_ID_WANT: accepts messages with this ID (if bitfields match).
      0, // MB_PR_LISTEN_ID_NEED: rejects messages unless all bits in message ID match (0 means this does nothing)
      0, // MB_PR_LISTEN_ID_REJECT: rejects messages if any bits in message ID match (0 means this does nothing)
      &listen_data [0] [0]); // MB_PR_LISTEN_ADDRESS: received messages (up to 8) will be stored in this array.
// Finally, let's work out the cost of the builder sub-process (so we can display it in response to user query):
//  (unlike the costs of building from a template, this should not change at run-time):
  call(METH_NEW,
       MS_PR_NEW_BC_COST_DATA, // gets the cost of building a template
       0, // MB_PR_NEW_VERTEX1: Vertex of this process that new process will be created at.
       -1, // MB_PR_NEW_VERTEX2: Vertex of child process (-1 means use child process' base_vertex interface value)
       0, // MB_PR_NEW_ANGLE: Angle of child process (0 means pointing directly away)
       process_start(builder), // MB_PR_NEW_START: start address of new process' code
       process_end(builder)); // MB_PR_NEW_END: end address
  builder_cost = get(METH_NEW, MB_PR_NEW_STATUS);
 }

// Now call the allocator to generate some data (unlike the IRPT method, it doesn't run automatically):
 call(METH_ALLOCATE); // can only be called once each cycle.

// Now we check the LISTEN method for requests from nearby processes to transfer data (a builder process will broadcast a request when it stops moving):
 int messages_received;
 messages_received = call(METH_LISTEN); // Calling the listen method returns the number of messages received this tick.
// Only process the first message (ignore any others):

 if (messages_received > 0
  && listen_data [0] [LISTEN_RECORD_VALUE1] == MSG_DATA_PLEASE) // Confirm that the message is a request for data
 {
// Request received!
// First, try to designate the source of the message:
   designate_result = call(METH_DESIGNATE,
                      MS_PR_DESIGNATE_ACQUIRE, // This mode saves a target to it can be recovered later by the MS_PR_DESIGNATE_LOCATE mode
                      listen_data [0] [LISTEN_RECORD_SOURCE_X], // This is the location of the target as an x offset from the process
                      listen_data [0] [LISTEN_RECORD_SOURCE_Y]); // Same for y

// The DESIGNATE method returns 1 if it is successful:
   if (designate_result == 1)
   {
    transferring_data = 1;
    if (verbose != 0)
     print("\nData requested.");
   }
    else
    {
     if (verbose != 0)
      print("\nData request source not found.");
    }
 }

// If a builder has requested a data transfer, try to transfer data to it with the YIELD method:
 if (transferring_data == 1)
 {
// First try to locate the target by calling the designate method with LOCATE status:
  if (call(METH_DESIGNATE, MS_PR_DESIGNATE_LOCATE) == 1)
  {
   yield_result = call(METH_YIELD,
                       0, // MB_PR_YIELD_IRPT - don't want to transfer irpt
                       100, // MB_PR_YIELD_DATA - this will be reduced to the actual rate, which is much lower
                       get(METH_DESIGNATE, MB_PR_DESIGNATE_X), // MB_PR_YIELD_X - uses the values still in the DESIGNATE method's registers
                       get(METH_DESIGNATE, MB_PR_DESIGNATE_Y)); // same for Y
   if (yield_result != MR_YIELD_SUCCESS)
   {
// If the transfer fails, stop transferring
    transferring_data = 0;
    if (verbose != 0)
     print("\nStopped data transfer.");
   }
  }
   else
    transferring_data = 0; // lost contact; stop trying to transfer

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

// Now carry out the process' current action:
	process_action();

} // end main


static void query_process(void)
{

   int build_result;

   call(METH_STD, MS_PR_STD_PRINT_OUT2); // redirects output to alternative console
// (expects operator to have set things up so the build list is displayed in a separate console.
//  but if this hasn't been done, the list will just go to console 0)
   int t;
// This command is given when the process is clicked on individually.
// The factory process responds by printing its status and a list of possible building actions to the console.
   print("Status: ");
   if (build_wait == 0)
   {
    print("not building");
   }
   if (build_wait == 1)
   {
    if (build_source == BUILD_BUILDER)
     print("building mobile builder");
      else
       print("building template ", build_source - BUILD_TEMPLATE1);
    if (keep_building == 1)
     print(" (repeat)");
   }
   if (transferring_data != 0)
    print("; transferring data");
// The ACTION method allows the process to attach an action to a printed console line. This causes an event when the user clicks on that line.
// Processes don't have direct access to generated actions, but operator programs do. The operator will need to pass the clicked
//  action back as a command.
   call(METH_STD, MS_PR_STD_COLOUR, COL_LBLUE); // COL_LBLUE is a colour macro.
   call(METH_STD, MS_PR_STD_ACTION, BUILD_BUILDER);
   print("\n >> Click here to build mobile builder (cost ", builder_cost, ") <<");
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
                        t); // MB_PR_NEW_TEMPLATE: Source template index
    if (build_result == MR_NEW_TEST_SUCCESS)
    {
// if the test was a success, the method's status register will hold the data cost of the process:
     call(METH_STD, MS_PR_STD_COLOUR, COL_LBLUE);
     call(METH_STD, MS_PR_STD_ACTION, BUILD_TEMPLATE1 + t);
     print("\n >> Click here to build template ",t," [");
     call(METH_STD, MS_PR_STD_TEMPLATE_NAME, t);
					print("] (cost ", get(METH_NEW, MB_PR_NEW_STATUS), ") <<");
    }
   }
   if (build_wait != 0)
   {
    call(METH_STD, MS_PR_STD_COLOUR, COL_LRED);
    call(METH_STD, MS_PR_STD_ACTION, BUILD_NOTHING);
    print("\n >> Click here to stop building <<");
   }
   if (transferring_data != 0)
   {
    call(METH_STD, MS_PR_STD_COLOUR, COL_LRED);
    call(METH_STD, MS_PR_STD_ACTION, BUILD_STOP_TRANSFER);
    print("\n >> Click here to stop data transfer <<");
   }
   call(METH_STD, MS_PR_STD_COLOUR, COL_LGREY);
   call(METH_STD, MS_PR_STD_ACTION, 0);
   print("\nCtrl-click to set repeat build command.");
   call(METH_STD, MS_PR_STD_PRINT_OUT); // redirects output to default console

}


// Call this to execute the command at the start of the queue
static void execute_command(void)
{

	int command_received;
	int build_command; // is 1 if the command is a kind of build command (and not e.g. stop data transfer)
 int build_result;

 build_command = 0;

// Now we check the command registers to see whether the process' client program has given it a command:
 command_received = get_command(COMREG_QUEUE);

 switch(command_received)
 {

  case COMMAND_ACTION:
// The operator gives the ACTION command when the user clicks a console line that this process attached an action to.
// For this factory process, that means a build command.
    build_wait = 0; // stop waiting to build something (as a new build command has been given)
// Acknowledge the command:
// Command register QUEUE + 1 should contain the action that this process set when the console line was written in response to a query command:
    build_source = get_command(COMREG_QUEUE + 1);
    switch(build_source)
    {
     case BUILD_BUILDER:
     	build_command = 1;
// First we set up some variables that will be used in future if the build call needs to be repeated:
      build_type = MS_PR_NEW_BC_BUILD; // indicates that the process is building from its own bcode
      build_start_address = process_start(builder);
      build_end_address = process_end(builder);
// The user may have given a repeat build order (by pressing ctrl). If so, the operator will have set the CTRL status bit to 1:
      keep_building = get_command_bit(COMREG_CLIENT_STATUS, COM_CLIENT_ST_CTRL);
      set_command_bit_0(COMREG_CLIENT_STATUS, COM_CLIENT_ST_CTRL);
// Now try to build:
      build_result = call(METH_NEW,
                          MS_PR_NEW_BC_BUILD, // MB_PR_NEW_STATUS: MS_PR_NEW_BUILD means try to build from this process' own bcode
                          0, // MB_PR_NEW_VERTEX1: Vertex of this process that new process will be created at.
                          -1, // MB_PR_NEW_VERTEX2: Vertex of child process (-1 means use child process' base_vertex interface value)
                          0, // MB_PR_NEW_ANGLE: Angle of child process (0 means pointing directly away)
                          build_start_address, // MB_PR_NEW_START: start address of new process' code
                          build_end_address, // MB_PR_NEW_END: end address
                          1, // MB_PR_NEW_LINK: 1 means try to connect (does nothing if fails)
                          0); // MB_PR_NEW_TEMPLATE: Source template index (not relevant here; only used when building from a template)
      if (verbose)
      {
       print("\nBuild command: mobile builder");
       if (keep_building)
        print(" (repeat)");
      }
      if (build_result != MR_NEW_SUCCESS
       || keep_building == 1)
      {
// If the build failed, the factory should try again later. need_resources() works out how much data is needed:
       need_resources(build_result, MS_PR_NEW_BC_COST_DATA);
       break;
      }
// Now tell the user the result of the build attempt:
      if (verbose != 0)
       print_build_result(build_result);
      break;

     case BUILD_TEMPLATE1:
     case BUILD_TEMPLATE2:
     case BUILD_TEMPLATE3:
     case BUILD_TEMPLATE4:
     	build_command = 1;
      build_type = MS_PR_NEW_T_BUILD; // indicates that the process is building from a template
      build_start_address = 0;
      build_end_address = BCODE_SIZE_PROCESS - 1;
// The user may have given a repeat build order (by pressing ctrl). If so, the operator will have set the CTRL status bit to 1:
      keep_building = get_command_bit(COMREG_CLIENT_STATUS, COM_CLIENT_ST_CTRL);
      set_command_bit_0(COMREG_CLIENT_STATUS, COM_CLIENT_ST_CTRL);
// Now try to build:
      build_result = call(METH_NEW,
                          MS_PR_NEW_T_BUILD, // MB_PR_NEW_STATUS: MS_PR_NEW_T_BUILD means try to build from a template.
                          0, // MB_PR_NEW_VERTEX1: Vertex of this process that new process will be created at.
                          -1, // MB_PR_NEW_VERTEX2: Vertex of child process (-1 means use child process' base_vertex interface value)
                          0, // MB_PR_NEW_ANGLE: Angle of child process
                          build_start_address, // MB_PR_NEW_START: Start address
                          build_end_address, // MB_PR_NEW_END: End address
                          1, // MB_PR_NEW_LINK: 1 means try to connect (does nothing if fails)
                          build_source - BUILD_TEMPLATE1); // MB_PR_NEW_TEMPLATE: Source template index
       if (verbose)
       {
        print("\nBuild command: template ", build_source - BUILD_TEMPLATE1);
        if (keep_building)
         print(" (repeat)");
       }
       if (build_result != MR_NEW_SUCCESS
        || keep_building == 1)
       {
// If the build failed because of a lack of resources, the factory should try again when it has enough. need_resources() sets this up:
        need_resources(build_result, MS_PR_NEW_T_COST_DATA);
        break;
       }
// Now tell the user the result of the build attempt:
       if (verbose != 0)
        print_build_result(build_result);
       break;

      case BUILD_NOTHING:
       build_wait = 0;
       keep_building = 0;
       print("\nStopped building.");
       break;

      case BUILD_STOP_TRANSFER:
       transferring_data = 0;
       print("\nStopped data transfer.");
       break;

      default:
       if (verbose != 0)
        print("\nUnknown build command.");
       break;

    } // end switch(build_source)
// If keep_building was set, set things so that the factory will keep building:
    if (keep_building == 1
				 && build_command == 1)
    {
     build_wait = 1;
     set_command(COMREG_QUEUE, COMMAND_BUILD); // Tells the client that this process is currently building
    }
    break; // end case COMMAND_ACTION

   default:
    break; // end default

 } // end switch(command_received)


} // end execute_command()

static void process_action(void)
{

	int build_result;

    if (build_wait == 1)
    {
// If build_wait is 1, the process tried to build something and failed for lack of resources.
// Check whether the process has enough data (need_data was set after the failed build attempt):
     if (get_data() >= need_data)
     {
// Need to set up the NEW method registers again (as the user may have queried this process, which resets some of the registers):
      build_result = call(METH_NEW,
                          build_type,
                          0, // MB_PR_NEW_VERTEX1: Vertex of this process that new process will be created at.
                          -1, // MB_PR_NEW_VERTEX2: Vertex of child process (-1 means use child process' base_vertex interface value)
                          0, // MB_PR_NEW_ANGLE: Angle of child process
                          build_start_address, // MB_PR_NEW_START: Start address
                          build_end_address, // MB_PR_NEW_END: End address
                          1, // MB_PR_NEW_LINK: 1 means try to connect (does nothing if fails)
                          build_source - BUILD_TEMPLATE1); // MB_PR_NEW_TEMPLATE: Source template index (irrelevant if building from bcode)
      if (build_result == MR_NEW_SUCCESS
       && keep_building == 0)
      {
       build_wait = 0;
       set_command(COMREG_QUEUE, COMMAND_IDLE); // Tells the client that this process is currently idle (which will be true when it has built)
      }
      if (verbose != 0)
       print_build_result(build_result);
     }
    }


}


// Call this function just after a call to the NEW method that failed because of insufficient resources.
// It assumes that the NEW method's mbank registers are still in the state they were in for the failed NEW call.
// build_result is the return value of the NEW call.
// test_type is either MS_PR_NEW_T_COST_DATA or MS_PR_NEW_BC_COST_DATA depending on whether we're building from a template or from bcode.
static void need_resources(int build_result, int test_type)
{
  int test_result;

// We are assuming that the method bank registers are the same from the earlier failed call, so we don't need to set up the registers again.
// test_type should have been set to a method status that tells the method to find out the data cost of the process that the factory just tried to build.
    test_result = call(METH_NEW,
                       test_type); // need to set status to test_type to indicate whether we're testing a bcode build or a template build
    if (test_result == MR_NEW_TEST_SUCCESS)
    {
      build_wait = 1;
      set_command(COMREG_QUEUE, COMMAND_BUILD); // Tells the client that this process is currently trying to build something.
// Calling the NEW method in data test mode sets its status register to the amount of data we need to build the process:
      need_data = get(METH_NEW, MB_PR_NEW_STATUS);
      print("\n(Data ", get_data(), "/", need_data, ").");
      return;
    }

    print("\n(Failed to calculate cost).");

}

static void print_build_result(int build_result)
{

  if (build_result == MR_NEW_SUCCESS)
  {
   print("\nBuild successful.");
   return;
  }

  print("\nBuild error: ");

  switch(build_result)
  {
     case MR_NEW_FAIL_STATUS: print("status?"); break; // Shouldn't happen
     case MR_NEW_FAIL_TYPE: print("type."); break;
     case MR_NEW_FAIL_OBSTACLE: print("collision."); break;
//     case MR_NEW_FAIL_IRPT: print("not enough irpt."); break;
     case MR_NEW_FAIL_DATA: print("need ", need_data, " data, have ", get_data(), "."); break;
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

     default: print("unknown reason ", build_result, "."); break;

  }

}


// This process definition contains another entire C program, which is incorporated into
//  this program during compilation. This program can then build it as a new process.
// m_pr_builder.c contains a basic builder process that can move around and create new processes from templates.
// Its main purpose is to build new factories or other stationary processes.
process builder
{

#include "prsc_builder.c"

}



































