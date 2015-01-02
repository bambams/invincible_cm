

// The STANDARD_COMMANDS macros are a set of standardised macros that client programs can use to communicate with processes.
// You don't have to use them, but if you do then your client programs and processes may be interoperable with others
//  that use them as well.
#ifndef STANDARD_COMMANDS

#define STANDARD_COMMANDS

// New command system:

// 0 is bitfield set by process to identify what kind of process it is.
// 1 is bitfield set by operator to do various things, like modify commands given.
// 2-5 are commands given by operator, and also responses from process
// 6-15 is a queue

/*
Here is how to give a command:
- set COMREG_0 to the command type
- set COMREG_1-3 to details of the command (e.g. coordinates)

The process:
- checks COMREG_0 for the command type
 - it ignores COMMAND_NONE, COMMAND_ACK and COMMAND_REJECT
- if the command is accepted, it:
 - copies the command to its command queue (the queue is used both by the process, to work out what to do,
			and the client, to do things like show a series of waypoints)
		- if the COM_CLIENT_QUEUE bit is 1, the process may put the command on the end of its queue (but doesn't have to)
	- sets COMREG_0 to COMMAND_ACK
	- executes the command.
	 - when it finishes executing the command, it drains the command from its queue.
	 - the queue is terminated by COMMAND_NONE.
- if the command is rejected, the process can set COMREG_0 to COMMAND_REJECT
 - but it doesn't strictly have to.


Or another way:

- Don't bother with COMMAND_ACK/REJECT
 - if really necessary they can be dealt with as status bits
- registers are:
COMREG_PROCESS_DETAILS - more or less fixed things about process
COMREG_PROCESS_STATUS - bits that might change from time to time
COMREG_CLIENT_DETAILS - more or less fixed things about client (may not actually be needed)
COMREG_CLIENT_STATUS - things about client that might change from time to time.
COMREG_QUEUE - start of queue
COMREG_END = 15 - end of queue

#define COMREG_QUEUE_SIZE 4
// - size of each queue entry (including the first value containing the command type)

To give a command:
- the client puts it directly in COMREG_QUEUE
- and also sets COM_CLIENT_NEW bit of COMREG_CLIENT_STATUS to 1

The process does these things each cycle:
- check the COM_CLIENT_NEW bit
 - if 1, it reads the entry at the start of the queue and derives target information etc from it
  - does not move queue
  - this can be done as a function: new_command()
  - also resets COM_CLIENT_NEW
	- if 0, does nothing in particular here.

- run the current command/behaviour
 - function: follow_command()
 - it does whatever the command tells it to.
 - if it finishes the command (arrives at destination etc) it calls finished_command()

- finished_command()
 - removes the command at the start of the queue
 - if the process accepts queued commands, copies commands down
 - if the command at the start of the queue is something other than COMMAND_NONE, calls new_command()


*/

#define COMREGS 16
// This reflects the actual size of the command register array, so don't change it.

#define COMREG_PROCESS_DETAILS 0
// COMREG_PROCESS_DETAILS is bitfield set by process to identify what kind of process it is.
#define COMREG_PROCESS_STATUS 1
// COMREG_PROCESS_STATUS is bitfield used by process (and maybe modified by client) telling client what it's doing
#define COMREG_CLIENT_DETAILS 2
// COMREG_CLIENT_DETAILS has details of client (not currently used)
#define COMREG_CLIENT_STATUS 3
// COMREG_CLIENT_STATUS is bitfield set by client (although possibly also modified by process)

#define COMREG_QUEUE 4
// COMREG_QUEUE is the first entry in a command queue taking up the rest of the registers.
#define COMREG_END 15
// last register.

#define COMMAND_QUEUE_SIZE 4
// Each command takes up 4 registers in the queue (including the first one containing the command type)

#define COMMAND_NONE 0
// NONE - no command (doesn't mean "do nothing" - use IDLE for that - although can mean "process is doing nothing")
//  - use this to terminate lists of commands (must be 0 so that an out-of-bounds check_command/get_command returns this)
#define COMMAND_EMPTY 1
// EMPTY - an empty command that unlike COMMAND_NONE does not terminate the command queue.
//  process should go to next command on queue, or treat this as COMMAND_NONE if no more commands.
//  - one use for this is if the operator needs to remove a command (e.g. it finds an attack command aimed at a non-existent process)
#define COMMAND_IDLE 2
// IDLE - do nothing
#define COMMAND_MOVE 3
// MOVE to location. fields: x, y
#define COMMAND_ATTACK 4
// ATTACK target. fields x, y, index
#define COMMAND_ACTION 5
// ACTION defined by process (probably means user clicked on a console line). fields: action number
#define COMMAND_BUILD 6
// BUILD something
#define COMMAND_A_MOVE 7
// A_MOVE to location (scans for targets along the way). fields: x, y

#define WAYPOINTS 3
// WAYPOINTS may not be needed

// The COM_PROCESS macros are bits of the COMREG_PROCESS register.
// They make suggestions to the operator about how the operator
//  can interact with the process. The operator may or may
//  not follow these suggestions.
// The value of each macro is the position in the bitfield

#define COM_PROCESS_DET_HP 0
// Suggests that the user may be interested in knowing the process' hp
#define COM_PROCESS_DET_DATA 1
// Same for data
#define COM_PROCESS_DET_IRPT 2
// irpt
#define COM_PROCESS_DET_ALLOC_EFF 3
// allocator efficiency at current location
#define COM_PROCESS_DET_NO_MULTI 4
// Tells the operator not to select as part of a multi-selection
//  e.g. click-and-drag selection (where only primary processes
//  need to be selected)
// Use for e.g. sub-processes (only the core should be selected)
//  and factories (which don't accept grouped commands)
#define COM_PROCESS_DET_COMMAND  5
// Tells the operator that it accepts standard commands. Even if 0 it may accept some.
#define COM_PROCESS_DET_QUEUE  6
// Tells the operator that it can push commands directly to the queue if it wants to.
//  (e.g. if a commend is given, then another command is given before the process
//  has a chance to move the first command to its queue, the operator will do so)


#define COM_PROCESS_ST_UPDATE 1
// COM_PROCESS_ST_UPDATE indicates that the process just changed its command queue.
// CL/OP should reset to 0 after reading.

#define COM_CLIENT_ST_VERBOSE 0
// COM_CLIENT_ST_VERBOSE suggests that the process might like to tell the user what it's doing.
// process should clear this bit after reading it
#define COM_CLIENT_ST_NEW 1
// COM_CLIENT_ST_NEW indicates client has issued a new command at COMREG_QUEUE (not needed for adding a command to end of queue).
// process should clear this bit after reading it
#define COM_CLIENT_ST_SHIFTED 2
// COM_CLIENT_ST_SHIFTED indicates that shift was being pressed when the command was clicked
// process should set this to zero whenever it reads a command (unless it always ignores this bit)
#define COM_CLIENT_ST_CTRL 3
// Like COM_CLIENT_ST_SHIFTED but for control
#define COM_CLIENT_ST_QUERY 4
// COM_CLIENT_ST_QUERY probably means that the user has clicked on the process, so it might like to say hello.

// MSGID is used for broadcasts.
#define MSGID_BUILDER 1
// MSGID_BUILDER indicates the broadcast is from a builder
//  - should this be a bitfield? probably

#define MSG_DATA_PLEASE 1
// MSG_DATA_PLEASE asks for data from a yield process


#endif

