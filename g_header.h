
#ifndef H_G_HEADER
#define H_G_HEADER


// BCODE_MAX is the maximum size of any program type's bcode:
#define BCODE_MAX 16384
#define PROC_BCODE_SIZE 4096
#define CLIENT_BCODE_SIZE 8192
#define SYSTEM_BCODE_SIZE 16384

#define METHODS 16
#define METHOD_BANK 64
#define METHOD_SIZE 4
#define METHOD_DATA_VALUES 5
// METHOD_DATA_VALUES is the size of the data array part of the methodstruct
// INTERFACE_METHOD_DATA is the number of values an interface definition in bcode should have for each method (not including type)
#define METHOD_VERTICES 16
#define EXECUTION_COUNT 16

#define DRAG_BASE 1014
#define DRAG_BASE_FIXED (al_itofix(DRAG_BASE) / 1024)
//#define SPIN_DRAG_BASE 1015
#define SPIN_DRAG_BASE 1002
#define SPIN_DRAG_BASE_FIXED (al_itofix(SPIN_DRAG_BASE) / 1024)

#define DEALLOCATE_COUNTER 32

#define MAX_SPEED 4
#define MAX_SPEED_FIXED al_itofix(MAX_SPEED)
#define NEG_MAX_SPEED_FIXED al_itofix(-MAX_SPEED)


enum
{
PROGRAM_TYPE_PROCESS,
PROGRAM_TYPE_DELEGATE,
PROGRAM_TYPE_OPERATOR,
PROGRAM_TYPE_OBSERVER,
PROGRAM_TYPE_SYSTEM,

PROGRAM_TYPES
}; // if changing or adding these, remember to change the numtoken definition in c_prepr.c and the program_type_name string array in g_client.c

// bcode always starts with a header. the first three values are common to all program types:
enum
{
BCODE_HEADER_ALL_JUMP, // jump instruction
BCODE_HEADER_ALL_JUMP2, // jump target (probably start of main function)
BCODE_HEADER_ALL_TYPE, // PROGRAM_TYPE_PROCESS etc.

BCODE_HEADER_ALL_END // different program type interface definitions start here
};

// any changes to bcode header structure may need to be reflected in:
//  intercode_process_header() in c_inter.c
//  parse_interface_definition() in c_comp.c
//  derive_proc_properties_from_bcode() in g_proc.c
//  derive_program_properties_from_bcode() in g_client.c

// different program types may have different fields before the method definition starts:
enum
{
BCODE_HEADER_PROC_SHAPE = BCODE_HEADER_ALL_END,
BCODE_HEADER_PROC_SIZE,
BCODE_HEADER_PROC_BASE_VERTEX,
BCODE_HEADER_PROC_METHODS, // start of method definitions
};

enum
{
BCODE_HEADER_DELEGATE_METHODS = BCODE_HEADER_ALL_END,
};

enum
{
BCODE_HEADER_OBSERVER_METHODS = BCODE_HEADER_ALL_END,
};

enum
{
BCODE_HEADER_OPERATOR_METHODS = BCODE_HEADER_ALL_END,
};

enum
{
BCODE_HEADER_SYSTEM_METHODS = BCODE_HEADER_ALL_END,
};

enum
{
INTERFACE_METHOD_DATA_TYPE,
//INTERFACE_METHOD_DATA_SUBTYPE,
INTERFACE_METHOD_DATA_VERTEX,
INTERFACE_METHOD_DATA_ANGLE,
INTERFACE_METHOD_DATA_EX1,
INTERFACE_METHOD_DATA_EX2,
INTERFACE_METHOD_DATA_EX3,

INTERFACE_METHOD_DATA

};
//#define INTERFACE_METHOD_DATA 5
// maximum total number of extensions:
//#define EXTENSIONS_MAX 6

#define SOURCE_FILES 20
#define FILE_NAME_LENGTH 20
#define FILE_PATH_LENGTH 100
// SOURCE_FILES is the maximum number of files that can be #included by a source file being compiled. Any nested #includes are counted.
// FILE_NAME_LENGTH is the maximum length of a file name string.

// TOTAL_PROC_HEADER_SIZE is the total size of all of the mandatory data at the start of a proc's bcode
// The first 2 is the jump to main
// The second 2 is the size and shape at the start of the interface definition
// Then there's space for method data
// NOTE: may need to change the starting bcode position in both derive_proc_properties_from_bcode and derive_client_properties_from_bcode if this is changed.
#define TOTAL_PROC_HEADER_SIZE (BCODE_HEADER_PROC_METHODS + (METHODS * INTERFACE_METHOD_DATA))

/*
// a bcodenotestruct contains debugging information for a bcodestruct.
// not yet implemented
struct bcodenotestruct
{
 int src_file [BCODE_MAX];
 int src_line [BCODE_MAX];
 char src_file_name [SOURCE_FILES] [FILE_NAME_LENGTH];
};*/

// a bcodestruct contains a program in binary form, capable of being run by the virtual machine interpreter in v_interp.c
struct bcodestruct
{
// REMEMBER: When anything is added to this structure, it may need to be added to load/save routines in f_load.c/f_save.c
 int bcode_size; // number of isntructions in op array. Differs depending on program type.
 s16b *op;
// struct bcodenotestruct* note; // this is debugging information for each op. may or may not be present. Must be NULL if not present. (not yet implemented)
// int method_type [METHODS];
 int type; // one of the PROGRAM_TYPE enums
 int shape; // for procs only - shape of proc (SHAPE_OCTAGON etc)
 int size; // for procs only - size of proc
 int printing; // this records whether a bcode that is being interpreted has called the print command during the current interpretation (it's set to zero in v_interp.c and is used by console action methods)

 int from_a_file;
 char src_file_name [FILE_NAME_LENGTH]; // should be empty if from_a_file == 0
 char src_file_path [FILE_PATH_LENGTH]; // same

 int static_length; // this stores the length of the bcode including any static memory at the end (this is needed because there's no way to tell between a static memory zero and a trailing zero)
 // it is only usable in the editor and compiler, and becomes meaningless as soon as the bcode is loaded into a template or placed in a world.

// Be careful about using pointers in this structure - it is copied using the = operator in copy_template_contents()
//  (as a template contents structure contains a bcode structure).
// copy_template_contents may need to deal with pointers specially, as it does with the op pointer.

};

enum
{
REGISTER_A,
REGISTER_B,
REGISTER_C,
REGISTER_D,
REGISTER_E,
REGISTER_F,
REGISTER_G,
REGISTER_H,

REGISTERS
};


/*
The registerstruct contains both registers and the mbank
The methodstruct contains information about each method available to the proc.
*/

enum
{
MCLASS_PR,
MCLASS_CL,
MCLASS_CLOB,
MCLASS_OB,
MCLASS_SY,

MCLASS_ALL,
MCLASS_END,
MCLASS_ERROR
};

enum
{
MEXT_INTERNAL, // not on a vertex (or not a proc method)
MEXT_EXTERNAL_NO_ANGLE, // on a vertex; doesn't have an angle (e.g. MTYPE_PR_LINK)
MEXT_EXTERNAL_ANGLED // on a vertex; has an angle (e.g. MTYPE_PR_PACKET)
}; // values for mtype.external

// METHOD_EXTENSIONS is the number of types of extensions each method can have
// redefining METHOD_EXTENSIONS requires numerous other changes to be made.
#define METHOD_EXTENSIONS 3

// this structure holds information about the various methods. See g_method.c
struct mtypestruct
{
 char name [30];
 int mclass; // MCLASS_PR etc
 int mbank_size; // this is the amount of space the method takes up in the mbank (in individual addresses)
 int cost_category;
 int external;
// int base_data_cost;
// int base_upkeep_cost; // fixed cost per cycle (not per tick)
 int extension_types; // different types of extension (currently max of 3)
 int max_extensions; // maximum number of extensions (should be 0 if method can't have extensions)
 char extension_name [METHOD_EXTENSIONS] [10];
// int extension_data_cost;
// int extension_upkeep_cost; // fixed cost per cycle per extension
};





enum
{
MTYPE_NONE,

// basic methods:
MTYPE_PR_MOVE,
MTYPE_PR_NEW, // creates a new proc
MTYPE_PR_NEW_SUB, // creates a new proc (with limitations)
MTYPE_PR_PACKET, // creates a packet
MTYPE_PR_DPACKET, // turret: creates a packet
MTYPE_PR_STREAM,
MTYPE_PR_DSTREAM,
MTYPE_PR_SCAN, // scans for nearby procs
MTYPE_PR_IRPT, // generates irpt
MTYPE_PR_ALLOCATE,
//MTYPE_GET_XY, // provides x, y (in pixels) and x_speed, y_speed information
MTYPE_PR_STD, // standard things
MTYPE_PR_MATHS, // performs various trig functions. Is separate from std mostly so std functions can be used as parameters to maths calls.
// when adding a new method, remember to update the mtype struct in g_method.c
// and also update the numtokens in c_prepr.c
MTYPE_PR_DESIGNATE, // targetting
MTYPE_PR_LINK, // allows another proc to be attached at a vertex (both procs must have link methods at that vertex). Currently attaching only allowed at creation.
//MTYPE_PR_TIME, // gets world time (although info also does this) and allows proc to change its execution timing slightly
MTYPE_PR_RESTORE, // self-repair
MTYPE_PR_REDUNDANCY, // armour
MTYPE_PR_BROADCAST,
MTYPE_PR_LISTEN,
MTYPE_PR_YIELD,
MTYPE_PR_STORAGE,
MTYPE_PR_STATIC, // keeps proc immobile (same effect as alloc)
MTYPE_PR_COMMAND, // fields: command index
//MTYPE_PR_ACTION, // fields: mode
MTYPE_PR_VIRTUAL,
//MTYPE_PR_OUTPUT, // controls what happens to stuff printed by the process (to some extent)

// CL methods are available to delegate and operator programs
MTYPE_CL_COMMAND_GIVE, // fields: proc, command index, value
MTYPE_CL_TEMPLATE, // does various things relating to templates, depending on status value
MTYPE_CL_OUTPUT, // controls what happens to stuff printed by the client. MTYPE_OB_CONSOLE can do this too (among many other things)

// CLOB methods are available to client, observer and operator programs
MTYPE_CLOB_POINT, // fields: x/y. Returns -1 on failure, proc index on success
MTYPE_CLOB_QUERY, // fields: proc index, query type, result1, result2. Returns 0/1 success/fail. Query types: team, x/y, speeds, angle, hp etc
MTYPE_CLOB_SCAN, // similar to proc scan but can scan at any point. Possibly allow square scans as well.
MTYPE_CLOB_MATHS, // similar to proc version but lacks turn-towards
MTYPE_CLOB_STD,
MTYPE_CLOB_COMMAND_REC, // fields: proc, command index

// OB methods are available to observer and operator programs
MTYPE_OB_INPUT, // mouse and keyboard input
MTYPE_OB_VIEW, // interaction with user view
MTYPE_OB_CONSOLE, // interaction with consoles
MTYPE_OB_SELECT, // allows display of proc selection etc
MTYPE_OB_CONTROL, // interaction with system

// SY methods are available only to system programs (note that system programs also have the same access to methods as operators)
MTYPE_SY_PLACE_PROC, // when called, places a proc at a specified location
MTYPE_SY_TEMPLATE, // does various things relating to templates, depending on status value
MTYPE_SY_MODIFY_PROC, // changes a proc in some way
//MTYPE_SY_CONNECTION, // causes a proc to join or leave a group
//MTYPE_SY_DESTROY_PROC, // removes proc from world
MTYPE_SY_MANAGE, // game control and interaction with obs/op

// all mtypes that are not valid entries in an interface definition (proc or client) should go after MTYPE_END (as MTYPE_END is used as a boundary in derive_proc_properties_from_bcode in g_proc.c)
MTYPE_END,
MTYPE_SUB,
MTYPE_ERROR_INVALID, // indicates that there was an error in the interface definition for this method
MTYPE_ERROR_VERTEX, // indicates attempt to place method on invalid vertex
MTYPE_ERROR_MASS, // indicates insufficient space for method
MTYPE_ERROR_DUPLICATE, // second instance of method that cannot be duplicated (e.g. redundancy)
MTYPE_ERROR_SUB, // using MTYPE_PR_NEW_SUB method to build process with forbidden methods
MTYPES // must be last

 // if changing or adding mtypes, remember to change the method definition in g_method.c and also the numtoken definition in c_prepr.c

};

// MBANK values set out the identity of the mbank entries (in registerstruct) for each method type. These are read/write, although in some cases they just record a value (e.g. max acceleration) and writing does nothing.
// MDATA values set out the identity of the data entries (in methodstruct) for each method type. These are hidden from the bcode.
// M_IDATA values set out what the values in a proc's interface definition mean. See derive_proc_properties_from_bcode() in g_proc.c.
//  M_IDATA values should always start at 1, because 0 in the interface data array is the method's type

// the mtypestruct mtype in g_method.c describes how much space each method type takes up in the mbank

// Remember: for external methods, the first M_IDATA entry must be the vertex.

// MTYPE_TURN: rotation



enum
{
MBANK_PR_MOVE_RATE,
MBANK_PR_MOVE_COUNTER,
MBANK_PR_MOVE_DELAY,
//MBANK_PR_MOVE_ANGLE // this value is only used by MTYPE_ACCEL_ANGLE
};
enum
{
MDATA_PR_MOVE_SETTING_MAX, // this is the maximum value that the mbank for acceleration can be set to (it is reduced to this if greater).
MDATA_PR_MOVE_ANGLE, // angle of acceleration. is used by both MTYPE_ACCEL and MTYPE_ACCEL_ANGLE, but for MTYPE_ACCEL is fixed at time of proc creation.
MDATA_PR_MOVE_FIRING, // whether method currently firing
MDATA_PR_MOVE_IRPT_COST, // irpt used per unit of acceleration
MDATA_PR_MOVE_ACTUAL_RATE, // actual accel rate (after mbank settings and irpt) this affects the size of the drive display
};
enum
{
M_IDATA_PR_MOVE_VERTEX = 1,
M_IDATA_PR_MOVE_RATE
};
// MTYPE_NEW: replication
enum
{

MBANK_PR_NEW_STATUS, // status
MBANK_PR_NEW_VERTEX1, // vertex of parent proc that new proc is created at
MBANK_PR_NEW_VERTEX2, // vertex of new proc
MBANK_PR_NEW_ANGLE, // angle of new proc
MBANK_PR_NEW_START_ADDRESS,
MBANK_PR_NEW_END_ADDRESS,
MBANK_PR_NEW_LINK,
MBANK_PR_NEW_TEMPLATE, // index of template in list of player's proc templates. used only when making new proc from template.
};

enum
{
MBANK_SY_PLACE_STATUS,
MBANK_SY_PLACE_X,
MBANK_SY_PLACE_Y,
MBANK_SY_PLACE_ANGLE,
MBANK_SY_PLACE_ADDRESS1, // start address
MBANK_SY_PLACE_ADDRESS2, // end address
MBANK_SY_PLACE_INDEX, // when creating, this is the player whose proc it will be. when finished successfully, this is the new proc's index
MBANK_SY_PLACE_TEMPLATE // template index (if placing from a template). This is the overall template index (used directly in templ[] array) and is different to the index used by PR_NEW method.
};

enum
{
MSTATUS_PR_NEW_NOTHING,
MSTATUS_PR_NEW_BUILD,
MSTATUS_PR_NEW_TEST,
MSTATUS_PR_NEW_COST_DATA,
MSTATUS_PR_NEW_COST_IRPT,
MSTATUS_PR_NEW_T_BUILD, // statuses with T in them use a template as the source, and MBANK_PR_NEW_TEMPLATE as template index
MSTATUS_PR_NEW_T_TEST,
MSTATUS_PR_NEW_T_COST_DATA,
MSTATUS_PR_NEW_T_COST_IRPT,

MSTATUS_PR_NEW_STATUSES
};

enum
{
// These MRES values are the result of running a new process method, or a system place method.
// They are returned by calling the method.
MRES_NEW_NONE,

MRES_NEW_SUCCESS,
MRES_NEW_TEST_SUCCESS,

MRES_NEW_FAIL_STATUS, // invalid value in MBANK_PR_NEW_STATUS
MRES_NEW_FAIL_TYPE, // invalid program type
MRES_NEW_FAIL_OBSTACLE,
MRES_NEW_FAIL_IRPT,
MRES_NEW_FAIL_DATA,
MRES_NEW_FAIL_START_BOUNDS,
MRES_NEW_FAIL_END_BOUNDS,
MRES_NEW_FAIL_INTERFACE,
MRES_NEW_FAIL_TOO_MANY_PROCS,
//MRES_NEW_FAIL_GEN_LIMIT,
MRES_NEW_FAIL_SHAPE,
MRES_NEW_FAIL_SIZE,
MRES_NEW_FAIL_PARENT_VERTEX,
MRES_NEW_FAIL_CHILD_VERTEX,
MRES_NEW_FAIL_TEMPLATE, // invalid template index
MRES_NEW_FAIL_TEMPLATE_EMPTY, // template not loaded

// The following are only relevant to the SY_PLACE method:
MRES_NEW_FAIL_LOCATION, // too close to edge (or out of bounds entirely)
MRES_NEW_FAIL_PLAYER, // invalid player index

};

enum
{
MDATA_PR_NEW_RESULT, // stores an MRES value
};

enum // remember that SY_PLACE is a called method, not an active method
{
MSTATUS_SY_PLACE_NOTHING,
//MSTATUS_SY_PLACE_ACTIVE, not used for SY_PLACE
MSTATUS_SY_PLACE_CALL,
MSTATUS_SY_PLACE_CALL_TEST,
MSTATUS_SY_PLACE_COST_DATA,
MSTATUS_SY_PLACE_COST_IRPT,
MSTATUS_SY_PLACE_T_CALL,
MSTATUS_SY_PLACE_T_CALL_TEST,
MSTATUS_SY_PLACE_T_COST_DATA,
MSTATUS_SY_PLACE_T_COST_IRPT,

/*
MSTATUS_SY_PLACE_FAIL_STATUS, // invalid value in MBANK_SY_PLACE_STATUS
MSTATUS_SY_PLACE_SUCCESS,
MSTATUS_SY_PLACE_TEST_SUCCESS,
MSTATUS_SY_PLACE_FAIL_OBSTACLE,
//MSTATUS_SY_PLACE_FAIL_IRPT,
//MSTATUS_SY_PLACE_FAIL_DATA,
MSTATUS_SY_PLACE_FAIL_START,
MSTATUS_SY_PLACE_FAIL_END,
MSTATUS_SY_PLACE_FAIL_INTERFACE,
MSTATUS_SY_PLACE_FAIL_TOO_MANY_PROCS,
//MSTATUS_SY_PLACE_FAIL_PARENT_VERTEX,
//MSTATUS_SY_PLACE_FAIL_CHILD_VERTEX,
MSTATUS_SY_PLACE_FAIL_TYPE,
MSTATUS_SY_PLACE_FAIL_SHAPE,
MSTATUS_SY_PLACE_FAIL_SIZE,
MSTATUS_SY_PLACE_FAIL_TEMPLATE, // invalid template (wrong template index, template empty, template wrong type etc.)
MSTATUS_SY_PLACE_FAIL_LOCATION, // too close to edge (or out of bounds entirely)
MSTATUS_SY_PLACE_FAIL_PLAYER, // invalid player index
MSTATUS_SY_PLACE_STATUSES*/
};

// MTYPE_PACKET
enum
{
MBANK_PR_PACKET_COUNTER,
MBANK_PR_PACKET_FRIENDLY
//MBANK_PR_PACKET_ANGLE
};
enum
{
MDATA_PR_PACKET_IRPT_COST,
MDATA_PR_PACKET_RECYCLE,
};
#define PACKET_RECYCLE_TIME 8
// PACKET_RECYCLE_TIME is the number of ticks that have to pass after a packet method has fired before it can fire again.
// In any case, it can only fire once per execution cycle.

enum
{
MBANK_PR_DPACKET_COUNTER,
MBANK_PR_DPACKET_ANGLE,
MBANK_PR_DPACKET_FRIENDLY
};
enum
{
MDATA_PR_DPACKET_IRPT_COST,
MDATA_PR_DPACKET_RECYCLE,
MDATA_PR_DPACKET_ANGLE,
//MDATA_PR_DPACKET_ANGLE_MIN,
//MDATA_PR_DPACKET_ANGLE_MAX,
};


enum
{
M_IDATA_PR_PACKET_VERTEX = 1
};
enum
{
MSTATUS_PR_PACKET_NOTHING,
MSTATUS_PR_PACKET_ACTIVE,
MSTATUS_PR_PACKET_SUCCESS,
MSTATUS_PR_PACKET_FAIL_IRPT,
MSTATUS_PR_PACKET_FAIL_TOO_MANY,
};
enum
{
MEX_PR_PACKET_POWER,
MEX_PR_PACKET_SPEED,
MEX_PR_PACKET_RANGE
};

enum
{
MBANK_PR_STREAM_FIRE, // set to 1 to fire
//MBANK_PR_STREAM_TIME // how long to fire for
};

enum
{
MDATA_PR_STREAM_STATUS, // 0 = inactive, 1 = warmup, 2 = firing, 3 = cooldown
MDATA_PR_STREAM_COUNTER, // countdown to next status
MDATA_PR_STREAM_RECYCLE, // countdown to next time method can fire
MDATA_PR_STREAM_MAX_COUNTER // maximum counter for the current firing (used in display)
};

enum // this isn't used in mbanks (it's used in data and for the stream beam cloud)
{
STREAM_STATUS_INACTIVE,
STREAM_STATUS_WARMUP,
STREAM_STATUS_FIRING,
STREAM_STATUS_COOLDOWN
}; // also used for DSTREAM
enum
{
MEX_PR_STREAM_TIME,
MEX_PR_STREAM_RANGE,
MEX_PR_STREAM_CYCLE
}; // also used for DSTREAM

// These are also used for DSTREAM:
#define STREAM_FIX_STEP_PIXELS al_itofix(5)
//#define STREAM_IRPT_COST 32
#define STREAM_WARMUP_LENGTH 8
#define STREAM_COOLDOWN_LENGTH 16
//#define STREAM_RECYCLE_TIME 64


enum
{
MBANK_PR_DSTREAM_FIRE, // set to 1 to fire
//MBANK_PR_DSTREAM_TIME, // how long to fire for
MBANK_PR_DSTREAM_ANGLE // angle to aim for
};

enum
{
MDATA_PR_DSTREAM_STATUS, // 0 = inactive, 1 = warmup, 2 = firing, 3 = cooldown
MDATA_PR_DSTREAM_COUNTER, // countdown to next status
MDATA_PR_DSTREAM_RECYCLE, // countdown to next time method can fire
MDATA_PR_DSTREAM_MAX_COUNTER, // maximum counter for the current firing (used in display)
MDATA_PR_DSTREAM_ANGLE
};

// MTYPE_PR_SCAN and MTYPE_CLOB_SCAN
enum
{
MBANK_PR_SCAN_STATUS,
MBANK_PR_SCAN_START_ADDRESS, // start address in memory of the array that holds scan results
MBANK_PR_SCAN_NUMBER, // stop scanning after finding this many procs
MBANK_PR_SCAN_X1, // x coord of one corner of scan as offset from proc's position (PR_SCAN) or as absolute position (CLOB_SCAN)
MBANK_PR_SCAN_Y1, // y
MBANK_PR_SCAN_SIZE, // ** see #define just below. PR scan: size of square. CLOB scan: x coord of other corner. Can be less than X1 (if it is, this will be fixed when the method is called)
MBANK_PR_SCAN_Y2, // PR scan: unused. CLOB scan: y2 coord
MBANK_PR_SCAN_BITFIELD_WANT,
MBANK_PR_SCAN_BITFIELD_NEED,
MBANK_PR_SCAN_BITFIELD_REJECT,


};

#define MBANK_PR_SCAN_X2 MBANK_PR_SCAN_SIZE

enum
{
MDATA_PR_SCAN_RANGE, // max range of scan (depends on range extensions)

};

enum
{
// scan results are all below zero because zero and up are used to indicate the number of targets found
MRES_SCAN_FAIL_SIZE = -1, // scan failed because size was < 0
MRES_SCAN_FAIL_RANGE = -2, // scan failed because centre of scan was outside range
MRES_SCAN_FAIL_STATUS = -3, // scan failed because method status invalid
MRES_SCAN_FAIL_ADDRESS = -4, // invalid bcode address for scan results
MRES_SCAN_FAIL_NUMBER = -5, // invalid number of results to receive
MRES_SCAN_FAIL_IRPT = -6, // scan failed because of insufficient irpt
};

enum
{
MBANK_PR_IRPT_STATUS, //
//MBANK_PR_IRPT_CALLED, // this amount is generated by a call to this method
//MBANK_PR_IRPT_AUTO, // this amount will be automatically generated before proc execution (but only for proc's first irpt gen method)
//MBANK_PR_IRPT_LEFT, // this is the amount that was left just before last execution (is stored in mbank so it's easily visible in proc data box)
};

enum
{
MSTATUS_PR_IRPT_SET, // sets method to generate this many irpt each tick (amounts over MAX are ignored).
MSTATUS_PR_IRPT_MAX, // returns max capacity of generator
};

enum
{
MDATA_PR_IRPT_GEN, // gen capacity each tick. Can be set by process to anywhere from 0 to MAX. Defaults to MAX.
MDATA_PR_IRPT_MAX,
};


#define ALLOCATE_INTERFERENCE_RANGE_DIVIDE 16
#define ALLOCATE_INTERFERENCE_BOX_SIZE (ALLOCATE_INTERFERENCE_RANGE_DIVIDE * 100)

enum
{
MDATA_PR_ALLOCATE_COUNTER, // Efficiency is added to this
MDATA_PR_ALLOCATE_EFFICIENCY,
MDATA_PR_ALLOCATE_CLOSEST_ALLOC, // index of closest other proc with allocate method
MDATA_PR_ALLOCATE_ALLOC_DIST // distance of closest other proc with allocate method
};


enum // these values are in the bitfield that is used to check scanned procs
{
MSCAN_BF1_TEAM0,
MSCAN_BF1_TEAM1,
MSCAN_BF1_TEAM2,
MSCAN_BF1_TEAM3,
MSCAN_BF1_SHAPE0, // octagon
MSCAN_BF1_SHAPE1, // diamond
MSCAN_BF1_SHAPE2,
MSCAN_BF1_SHAPE3,
MSCAN_BF1_SHAPE4,
MSCAN_BF1_SHAPE5, // 10
MSCAN_BF1_SIZE0,
MSCAN_BF1_SIZE1,
MSCAN_BF1_SIZE2,
MSCAN_BF1_SIZE3,
MSCAN_BF1_SIZE4,
MSCAN_BF1_SIZE5,
};

/*
enum // these values are in the bitfield that is used to work out what data to store
{
MSCAN_DATA1_DIST, // note that the dist and angle entries are from the centre of the scan, not the scanning proc
MSCAN_DATA1_ANGLE,
MSCAN_DATA1_DIST_X,
MSCAN_DATA1_DIST_Y,
MSCAN_DATA1_SPEED_X,
MSCAN_DATA1_SPEED_Y,
MSCAN_DATA1_TEAM,
MSCAN_DATA1_SHAPE,
MSCAN_DATA1_SIZE

// should have an entry here that allows the scanned proc's bitfields themselves to be saved as data.
};*/
enum
{
M_IDATA_PR_SCAN_RANGE
};

enum
{
MSTATUS_PR_SCAN_SCAN,
MSTATUS_PR_SCAN_EXAMINE,
//MSTATUS_PR_SCAN_RECTANGLE, // client only
};

enum
{
MSTATUS_CLOB_SCAN_SCAN,
MSTATUS_CLOB_SCAN_RECTANGLE, // client only
MSTATUS_CLOB_SCAN_EXAMINE,
MSTATUS_CLOB_SCAN_SCAN_INDEX,
MSTATUS_CLOB_SCAN_RECTANGLE_INDEX,

};

enum
{
SCAN_RESULTS_XY,
SCAN_RESULTS_INDEX
};

enum
{
MBANK_PR_STD_STATUS
};

#define SPEED_INT_ADJUSTMENT 16
// MTYPE_GET_XY
enum
{
MSTATUS_PR_STD_GET_X,
MSTATUS_PR_STD_GET_Y,
MSTATUS_PR_STD_GET_ANGLE,
MSTATUS_PR_STD_GET_SPEED_X, // actual speed is multiplied by SPEED_INT_ADJUSTMENT
MSTATUS_PR_STD_GET_SPEED_Y,
MSTATUS_PR_STD_GET_TEAM,
MSTATUS_PR_STD_GET_HP,
MSTATUS_PR_STD_GET_HP_MAX,
MSTATUS_PR_STD_GET_INSTR,
MSTATUS_PR_STD_GET_INSTR_MAX,
MSTATUS_PR_STD_GET_IRPT,
MSTATUS_PR_STD_GET_IRPT_MAX,
MSTATUS_PR_STD_GET_OWN_IRPT_MAX, // if proc is a group member, returns its single irpt max (not group's shared max)
MSTATUS_PR_STD_GET_DATA,
MSTATUS_PR_STD_GET_DATA_MAX,
MSTATUS_PR_STD_GET_OWN_DATA_MAX,
MSTATUS_PR_STD_GET_SPIN, // this one returns group spin if proc is a group member (TO DO: should I instead somehow work out proc's angle change for this one, and put in a new GET_GR_SPIN mode? Not sure it's worth it)
MSTATUS_PR_STD_GET_GR_X, // returns group's x (or 0 if not a group member)
MSTATUS_PR_STD_GET_GR_Y,
//MSTATUS_PR_STD_GET_GR_ANGLE,
MSTATUS_PR_STD_GET_GR_SPEED_X,
MSTATUS_PR_STD_GET_GR_SPEED_Y,
MSTATUS_PR_STD_GET_GR_MEMBERS, // total number of procs in group (returns 1 if not a group member)
MSTATUS_PR_STD_GET_EX_COUNT, // this is pretty pointless for a process, but clients can use the query version
MSTATUS_PR_STD_GET_WORLD_W,
MSTATUS_PR_STD_GET_WORLD_H,
//MSTATUS_PR_STD_GET_GEN_LIMIT,
//MSTATUS_PR_STD_GET_GEN_NUMBER,
MSTATUS_PR_STD_GET_EFFICIENCY,
MSTATUS_PR_STD_GET_TIME,
// The vertex-related info modes are basically dynamic versions of the data() keyword
//  - use them for processes that can be modified at runtime
MSTATUS_PR_STD_GET_VERTICES,
MSTATUS_PR_STD_GET_VERTEX_ANGLE, // has parameter (vertex number)
MSTATUS_PR_STD_GET_VERTEX_DIST, // has parameter (vertex number)
MSTATUS_PR_STD_GET_VERTEX_ANGLE_PREV, // has parameter (vertex number)
MSTATUS_PR_STD_GET_VERTEX_ANGLE_NEXT, // has parameter (vertex number)
MSTATUS_PR_STD_GET_VERTEX_ANGLE_MIN, // has parameter (vertex number)
MSTATUS_PR_STD_GET_VERTEX_ANGLE_MAX, // has parameter (vertex number)
// The method information modes are mostly there so that clients can use query_method functions.
// But they could also be useful for processes that can be altered by their parent processes.
MSTATUS_PR_STD_GET_METHOD, // returns type of method in slot specified by mb[1]
MSTATUS_PR_STD_GET_METHOD_FIND, // returns slot of first method of type mb[1] (starting with slot given by mb[2], to allow multiples to be found)
MSTATUS_PR_STD_GET_MBANK, // This is really just for query - lets a cl/ob check a process' mbank

// these values are also used for the MTYPE_CLOB_QUERY method

// non-get STD functions:
MSTATUS_PR_STD_ACTION, // attaches an action to lines printed to console
MSTATUS_PR_STD_WAIT, // waits up to 16 ticks for next execution
MSTATUS_PR_STD_COLOUR,
MSTATUS_PR_STD_SET_COMMAND, // sets a command register. The PR_COMMAND method is needed to read command registers.
MSTATUS_PR_STD_COMMAND_BIT_0, // sets a single bit of a command register. The PR_COMMAND method is needed to read command registers.
MSTATUS_PR_STD_COMMAND_BIT_1, // sets a single bit of a command register. The PR_COMMAND method is needed to read command registers.
MSTATUS_PR_STD_PRINT_OUT, // Sets output to default output console (exactly which console this is can be set by the OB_CONSOLE method)
MSTATUS_PR_STD_PRINT_OUT2, // Sets output to alternative output console
MSTATUS_PR_STD_PRINT_ERR, // Sets output to error console

MSTATUS_PR_STD_TEMPLATE_NAME, // Prints name of a template to console

};


enum
{
MBANK_PR_RESTORE_NUMBER,
//MBANK_PR_RESTORE_IRPT
};
enum
{
MDATA_PR_RESTORE_USED, // is 1 if restore already called this tick
MDATA_PR_RESTORE_RATE
};


enum
{
MBANK_PR_COMMAND_STATUS,
MBANK_PR_COMMAND_INDEX,
MBANK_PR_COMMAND_BIT,
};

enum
{
MSTATUS_PR_COMMAND_VALUE, // reads value
MSTATUS_PR_COMMAND_BIT, // reads single bit
};

enum
{
MBANK_CL_COMMAND_STATUS,
MBANK_CL_COMMAND_PROC_INDEX,
MBANK_CL_COMMAND_CMD_INDEX,
MBANK_CL_COMMAND_VALUE,
};

enum
{
MSTATUS_CL_COMMAND_SET_VALUE,
MSTATUS_CL_COMMAND_BIT_0, // uses MBANK_CL_COMMAND_VALUE as bit to set
MSTATUS_CL_COMMAND_BIT_1,
};

enum
{
MSTATUS_CLOB_COMMAND_VALUE,
MSTATUS_CLOB_COMMAND_BIT

};

/*
enum
{
MBANK_PR_ACTION_VALUE
};*/


enum
{
MBANK_PR_VIRTUAL_STATUS,
MBANK_PR_VIRTUAL_VALUE,
MBANK_PR_VIRTUAL_CURRENT, // holds current strength
MBANK_PR_VIRTUAL_MAX // holds max (mostly so it's visible in proc box display)
};

enum
{
MSTATUS_PR_VIRTUAL_CHARGE,
MSTATUS_PR_VIRTUAL_GET_STATE, // returns: -1 if method active, 0 if inactive but able to charge, or if inactive and unable to charge: number of ticks until can charge again
MSTATUS_PR_VIRTUAL_GET_MAX, // returns maximum capacity
MSTATUS_PR_VIRTUAL_DISABLE // turns it off
};

#define VIRTUAL_METHOD_RECYCLE 96
// VIRTUAL_METHOD_RECYCLE is the number of ticks after a virtual method is disrupted until it can be charged again
#define VIRTUAL_METHOD_PULSE_MAX 8
// VIRTUAL_METHOD_PULSE_MAX is the max value of MDATA_PR_VIRTUAL_PULSE

enum
{
MDATA_PR_VIRTUAL_STATE, // holds: negative number if recycling (time left * -1); 0 if off but can be charged, otherwise strength
MDATA_PR_VIRTUAL_MAX, // max strength
MDATA_PR_VIRTUAL_RECHARGED, // amount of recharge so far this tick
MDATA_PR_VIRTUAL_PULSE, // hit
MDATA_PR_VIRTUAL_CHARGE_PULSE
};

/*
enum
{
MBANK_PR_OUTPUT_STATUS
};

enum
{
MSTATUS_PR_OUTPUT_COLOUR
};*/

enum
{
MBANK_CL_OUTPUT_STATUS
};

enum
{
MSTATUS_CL_OUTPUT_COLOUR
};

//enum
//{

//};

/*
How does restore work?

It's an active method
runs just after each execution (dsn't run each tick)

Mbank:
 0: number of points to repair (0 means method off; -1 means always repair)
 1: irpt threshold (only operates if irpt is above this value)

Extensions:
 0: ++ repair speed

Data:
 0: max points to repair each cycle

How does redundancy work?

It can't be called and isn't active
It increases hp_max by 30%
Also increases mass by 30% of shape's base mass (or more?)
What about cost?
 - has just an ordinary irpt cost, not very high
 - but what about data cost?
  - should probably increase cost by 30% of shape's base cost, or something like that (this would also increase mass)

Extensions:
 0: increase hp_max and mass by additional 30% (?)


How does payload work?

It can't be called. Is active.

Mbank:
 0: status. 0 = does nothing. 1 = harmlessly self-destruct. 2 = explode, causing damage. 3 = explode on collision.
 1: bitfield: safety for explode on collision. Won't explode if it hits a proc of team with safety bit set to 1.

Extensions:
 0: increase damage on self-destruct
 1: increase size of self-destruct explosion

*/


enum
{
MSTATUS_CLOB_STD_WORLD_SIZE_X,
MSTATUS_CLOB_STD_WORLD_SIZE_Y,
//MSTATUS_CLOB_WORLD_BLOCKS_X,
//MSTATUS_CLOB_WORLD_BLOCKS_Y,
MSTATUS_CLOB_STD_WORLD_PROCS,
MSTATUS_CLOB_STD_WORLD_TEAM, // only relevant to CL or OP
MSTATUS_CLOB_STD_WORLD_TEAMS,
MSTATUS_CLOB_STD_WORLD_TEAM_SIZE, // procs in team
MSTATUS_CLOB_STD_WORLD_MAX_PROCS_EACH,
MSTATUS_CLOB_STD_WORLD_FIRST_PROC, // only relevant to CL or OP
MSTATUS_CLOB_STD_WORLD_LAST_PROC, // only relevant to CL or OP
//MSTATUS_CLOB_WORLD_GEN_LIMIT,
//MSTATUS_CLOB_WORLD_GEN_NUMBER,
MSTATUS_CLOB_STD_WORLD_INSTR_LEFT,
MSTATUS_CLOB_STD_WORLD_TIME,
MSTATUS_CLOB_STD_TEMPLATE_NAME // prints template name to console
};

enum
{
MSTATUS_OB_SELECT_SET_MARKER,
MSTATUS_OB_SELECT_PLACE_MARKER,
MSTATUS_OB_SELECT_PLACE_MARKER2,
MSTATUS_OB_SELECT_BIND_MARKER, // binds marker to a process
MSTATUS_OB_SELECT_BIND_MARKER2, // for binding other end of line to a process
MSTATUS_OB_SELECT_UNBIND_PROCESS, // causes all markers bound to a process to expire
MSTATUS_OB_SELECT_CLEAR_MARKERS, // deletes all markers immediately
MSTATUS_OB_SELECT_EXPIRE_MARKERS, // all markers fade out
MSTATUS_OB_SELECT_CLEAR_A_MARKER, // deletes specified markers immediately
MSTATUS_OB_SELECT_EXPIRE_A_MARKER, // specified marker fades out
MSTATUS_OB_SELECT_MARKER_SPIN, // sets marker spin (default +48). not relevant for all types
MSTATUS_OB_SELECT_MARKER_ANGLE, // sets marker angle. not relevant for all types
MSTATUS_OB_SELECT_MARKER_SIZE, // 0-16. not relevant for all types

};

enum
{
MSTATUS_CLOB_POINT_EXACT,
MSTATUS_CLOB_POINT_FUZZY,
MSTATUS_CLOB_POINT_ALLOC_EFFIC
};

enum
{
MBANK_CL_TEMPLATE_INDEX, // different from SY_TEMPLATE_TYPE because the cl template method can only copy to its 4 process templates
MBANK_CL_TEMPLATE_START,
MBANK_CL_TEMPLATE_END,
MBANK_CL_TEMPLATE_NAME
};


enum
{
MBANK_SY_TEMPLATE_TYPE,
//MBANK_SY_TEMPLATE_INDEX,
MBANK_SY_TEMPLATE_START,
MBANK_SY_TEMPLATE_END,
MBANK_SY_TEMPLATE_NAME

};


enum
{
FIND_TEMPLATE_OBSERVER, // note that overwriting the observer template will not affect the observer program in w.observer_program. Must do this some other way
FIND_TEMPLATE_P0_CLIENT, // client values must be sequential (p1, p2, p3 etc). these also check for operator (sort of - see sy_copy_to_template())
FIND_TEMPLATE_P1_CLIENT,
FIND_TEMPLATE_P2_CLIENT,
FIND_TEMPLATE_P3_CLIENT,
FIND_TEMPLATE_P0_PROC_0,
FIND_TEMPLATE_P0_PROC_1,
FIND_TEMPLATE_P0_PROC_2,
FIND_TEMPLATE_P0_PROC_3,
FIND_TEMPLATE_P1_PROC_0,
FIND_TEMPLATE_P1_PROC_1,
FIND_TEMPLATE_P1_PROC_2,
FIND_TEMPLATE_P1_PROC_3,
FIND_TEMPLATE_P2_PROC_0,
FIND_TEMPLATE_P2_PROC_1,
FIND_TEMPLATE_P2_PROC_2,
FIND_TEMPLATE_P2_PROC_3,
FIND_TEMPLATE_P3_PROC_0,
FIND_TEMPLATE_P3_PROC_1,
FIND_TEMPLATE_P3_PROC_2,
FIND_TEMPLATE_P3_PROC_3,

FIND_TEMPLATES
};


/*enum
{
MBANK_PR_TIME_STATUS
};

enum
{
MSTATUS_PR_TIME_GET, // fills mb [1] and mb [2] with world time (mb [1] is time / 32767; mb [2] is time % 32767)
MSTATUS_PR_TIME_WAIT, // delays next execution of this proc by mb [1]. mb [1] must be from 0 to 15
};*/

/*enum
{
MBANK_GET_XY_X,
MBANK_GET_XY_Y,
MBANK_GET_XY_X_SPEED,
MBANK_GET_XY_Y_SPEED
};*/
// MTYPE_GET_ANGLE
/*enum
{
MBANK_GET_ANGLE_ANGLE,
MBANK_GET_ANGLE_SPIN
};*/
// MTYPE_MATHS_TRIG
enum
{
MBANK_PR_MATHS_STATUS,
MBANK_PR_MATHS_TERM1,
MBANK_PR_MATHS_TERM2,
MBANK_PR_MATHS_TERM3
};
enum
{
MSTATUS_PR_MATHS_NONE,
MSTATUS_PR_MATHS_ATAN2, // returns atan2(TERM2, TERM1); (in angle units)
MSTATUS_PR_MATHS_SIN, // returns sin(TERM1) * TERM2; (multiply needed as sin returns float)
MSTATUS_PR_MATHS_COS, // returns cos(TERM1) * TERM2;
MSTATUS_PR_MATHS_HYPOT, // returns hypot(TERM2, TERM1);
MSTATUS_PR_MATHS_TURN_DIR, // returns -1, 0 or +1 to indicate direction to turn to get from TERM1 to TERM2
MSTATUS_PR_MATHS_ANGLE_DIFF, // returns *absolute* difference between two angles
MSTATUS_PR_MATHS_ANGLE_DIFF_S, // returns *signed* difference between two angles
MSTATUS_PR_MATHS_ABS, // returns abs(TERM1)
MSTATUS_PR_MATHS_SQRT, // returns sqrt(TERM1)
MSTATUS_PR_MATHS_POW, // returns pow(TERM1, TERM2)
MSTATUS_PR_MATHS_RANDOM, // returns rand() % TERM1 (checks for 0) - not currently available



};

enum
{
MBANK_PR_DESIGNATE_STATUS,
MBANK_PR_DESIGNATE_X,
MBANK_PR_DESIGNATE_Y
};

enum
{
MDATA_PR_DESIGNATE_INDEX, // index of proc designated by this method. is -1 if no longer exists (although this information isn't available to proc)
MDATA_PR_DESIGNATE_RANGE, // max range of designate (depends on extensions)
};

enum
{
MSTATUS_PR_DESIGNATE_ACQUIRE,
MSTATUS_PR_DESIGNATE_LOCATE,
MSTATUS_PR_DESIGNATE_SPEED
};

enum
{
M_IDATA_ERROR_TYPE

};

// client process method/mbank things

enum
{
MBANK_OB_INPUT_MODE,
 // the purpose of the other mbank addresses in each mode is set out in g_method.c
};

enum
{
MSTATUS_OB_INPUT_MODE_MOUSE_BUTTON, // fields: button (returns button status)
MSTATUS_OB_INPUT_MODE_MOUSE_XY, // fields: x, y of mouse pointer in world
MSTATUS_OB_INPUT_MODE_MOUSE_SCREEN_XY, // fields: x, y of mouse pointer on screen
MSTATUS_OB_INPUT_MODE_MOUSE_MAP_XY, // fields: if on map then x, y are world coordinates corresponding to mouse's position in map window
//MBANK_CL_INPUT_MODE_MOUSE_SELECT, // fields: proc team, proc # (team and # -1 if none selected)
//MBANK_CL_INPUT_MODE_MOUSE_DRAG1_XY,
//MBANK_CL_INPUT_MODE_MOUSE_DRAG2_XY,
//MBANK_OB_INPUT_KEYBOARD_ANY_KEY,
MSTATUS_OB_INPUT_KEY, // fields: status, key (in KEYS_?)
MSTATUS_OB_INPUT_ANY_KEY, // returns the key being pressed, or -1 if no key being pressed. field [0] is set to status of key (JUST_PRESSED etc). Only detects one key at a time.
};


enum
{
MSTATUS_OB_CONSOLE_OPEN, //  - field 1: console index (0-3)
MSTATUS_OB_CONSOLE_CLOSE, //  - field 1: console index
MSTATUS_OB_CONSOLE_MIN, //  minimises console - field 1: console index
MSTATUS_OB_CONSOLE_STATE, //  - field 1: index //  - field 2: closed = 0, minimised = 1, med = 2, large = 3
MSTATUS_OB_CONSOLE_GET_XY, //  - field 1: index //  - fields 2, 3: x/y
MSTATUS_OB_CONSOLE_MOVE, //  - field 1: index //  - fields 2, 3: x/y
MSTATUS_OB_CONSOLE_SIZE, //field 1: index // field 2: width. field 3 height. resets console
MSTATUS_OB_CONSOLE_LINES, //field 1: index // field 2: lines. doesn't reset console
MSTATUS_OB_CONSOLE_ACTION_SET,
MSTATUS_OB_CONSOLE_ACTION_CHECK,
MSTATUS_OB_CONSOLE_CLEAR, // deletes all messages and actions: field 1: console index
MSTATUS_OB_CONSOLE_STYLE, // sets console's style to a CONSOLE_STYLE_* setting
MSTATUS_OB_CONSOLE_OUT, // sets which console current program's output is sent to. doesn't last beyond current execution. (field 1: console)
MSTATUS_OB_CONSOLE_ERR, // sets which console current program's erro output is sent to. similar to previous (field 1: console)
MSTATUS_OB_CONSOLE_COLOUR, // sets colour of following printed things. not specific to a console (field 1: colour)
MSTATUS_OB_CONSOLE_FONT, //field 1: one of the CFONT values. resets console
MSTATUS_OB_CONSOLE_BACKGROUND, // background colour. field 1: index, field 2: colour (a base_col)
MSTATUS_OB_CONSOLE_TITLE, // title of console. field 2: address of first letter
MSTATUS_OB_CONSOLE_LINES_USED, // returns number of lines in console that have had something printed to them. field 1: console

// These set the default output console for various programs. If called by that program itself, won't take effect until next execution:
MSTATUS_OB_CONSOLE_OUT_SYSTEM, // sets which console system program's output is sent to (field 1: console)
MSTATUS_OB_CONSOLE_OUT_OBSERVER, // sets which console observer program's output is sent to (field 1: console)
MSTATUS_OB_CONSOLE_OUT_PLAYER, // sets which console a player's output is sent to (field 1: console, field 2: player). Affects operator programs
// These set the alternative output console for various programs. If called by that program itself, won't take effect until next execution:
MSTATUS_OB_CONSOLE_OUT2_PLAYER, // sets which console a player's process' output is sent to (field 1: console, field 2: player). Affects operator programs
// Set default error output console
MSTATUS_OB_CONSOLE_ERR_SYSTEM, // sets which console system program's error output is sent to (field 1: console)
MSTATUS_OB_CONSOLE_ERR_OBSERVER, // sets which console observer program's error output is sent to (field 1: console)
MSTATUS_OB_CONSOLE_ERR_PLAYER, // sets which console a player's error output is sent to (field 1: console, field 2: player). Affects operator programs

};

enum
{
MSTATUS_OB_VIEW_FOCUS_XY,  // set camera to xy
MSTATUS_OB_VIEW_FOCUS_PROC, // set camera to proc (just once)
MSTATUS_OB_VIEW_PROC_DATA, // sets proc data display to this proc
MSTATUS_OB_VIEW_PROC_DATA_XY, // moves proc data display
MSTATUS_OB_VIEW_SCROLL_XY, // scrolls display
MSTATUS_OB_VIEW_MAP_VISIBLE, // 0 or 1
MSTATUS_OB_VIEW_MAP_XY, // moves map to x/y
MSTATUS_OB_VIEW_MAP_SIZE, // sets map size
MSTATUS_OB_VIEW_GET_DISPLAY_SIZE, // gets width/height of display window
MSTATUS_OB_VIEW_COLOUR_PROC,
MSTATUS_OB_VIEW_COLOUR_PACKET,
MSTATUS_OB_VIEW_COLOUR_DRIVE,
MSTATUS_OB_VIEW_COLOUR_BACK,
MSTATUS_OB_VIEW_COLOUR_BACK2,
MSTATUS_OB_VIEW_SOUND,

};


enum
{
MSTATUS_OB_CONTROL_PAUSED, // - returns 0 if not soft-paused, 1 if soft-paused
MSTATUS_OB_CONTROL_PAUSE_SET, // sets soft pause
MSTATUS_OB_CONTROL_FF, // - returns 0 if not fast forwarding, 1 if fast forwarding. Sets mb [1] to FF type
MSTATUS_OB_CONTROL_FF_SET, // sets FF
MSTATUS_OB_CONTROL_PHASE, // - returns 0 if in world phase, 1 if in interturn phase, 2 if in pregame phase
MSTATUS_OB_CONTROL_SYSTEM_SET, // - allows communication with system program. 64 registers. fields: 1-register 2-value. returns 1 on success, 0 on failure.
MSTATUS_OB_CONTROL_SYSTEM_READ, // - inverse of SYSTEM_SET. returns value.
MSTATUS_OB_CONTROL_TURN, // - returns which turn we're in
MSTATUS_OB_CONTROL_TURNS, // - returns how many turns in game
MSTATUS_OB_CONTROL_SECONDS_LEFT, // - returns time left in turn (in seconds)
MSTATUS_OB_CONTROL_TICKS_LEFT, // - returns time left in turn in ticks (returns 32767 if overflow)
MSTATUS_OB_CONTROL_TURN_SECONDS, // - returns time for each turn (in seconds)

};


enum
{
MSTATUS_SY_MANAGE_PAUSED, // - returns 0 if not soft-paused, 1 if soft-paused
MSTATUS_SY_MANAGE_PAUSE_SET, // - field 1-set soft pause to 0 or 1 (can't unpause if in interturn/pregame phase)
MSTATUS_SY_MANAGE_FF, // - returns 0 if not fast forwarding, 1 if fast forwarding
MSTATUS_SY_MANAGE_FF_SET, // - field 1-set fast-forward to 0 or 1
MSTATUS_SY_MANAGE_PHASE, // - returns 0 if in world phase, 1 if in interturn phase, 2 if in pregame phase
MSTATUS_SY_MANAGE_OB_SET, // - allows communication with obs/op program (if present).
MSTATUS_SY_MANAGE_OB_READ, //
MSTATUS_SY_MANAGE_TURN, // see OB_CONTROL statuses above
MSTATUS_SY_MANAGE_TURNS, //
MSTATUS_SY_MANAGE_SECONDS_LEFT, //
MSTATUS_SY_MANAGE_TICKS_LEFT, //
MSTATUS_SY_MANAGE_TURN_SECONDS, //
MSTATUS_SY_MANAGE_GAMEOVER, // call this when game is over - will set game phase to GAME_PHASE_GAMEOVER. Fields: 1-ending status (GAME_END_? enum), 2-value (used by some GAME_END statuses)
MSTATUS_SY_MANAGE_END_PREGAME, // finishes the pregame.
MSTATUS_SY_MANAGE_END_TURN, // forces current turn to end
MSTATUS_SY_MANAGE_START_TURN, // starts next turn (only works in preturn phase)
MSTATUS_SY_MANAGE_LOAD_OBSERVER, // loads observer from template into program
MSTATUS_SY_MANAGE_LOAD_CLIENT, // loads player client from template into program
MSTATUS_SY_MANAGE_ALLOCATE_EFFECT, // sets a player's allocate mode (align or disrupt)

};


enum
{
MSTATUS_CL_INFO_GET_TEAM,
};

enum
{
MBANK_CLOB_QUERY_STATUS,
MBANK_CLOB_QUERY_INDEX,
}; // Status uses statuses for PR_GET method

enum
{
MBANK_SY_MODIFY_PROC_STATUS,
MBANK_SY_MODIFY_PROC_INDEX,
MBANK_SY_MODIFY_PROC_VALUE,
MBANK_SY_MODIFY_PROC_VALUE2,
};

enum
{
MSTATUS_SY_MODIFY_PROC_HP,
MSTATUS_SY_MODIFY_PROC_IRPT,
MSTATUS_SY_MODIFY_PROC_DATA,
MSTATUS_SY_MODIFY_PROC_MB, // method bank value
MSTATUS_SY_MODIFY_PROC_BCODE, // bcode
MSTATUS_SY_MODIFY_PROC_INSTANT, // sets lifetime to 16 (so a newly created proc won't have the spawn animation)
};


enum
{
MBANK_PR_LINK_STATUS,
MBANK_PR_LINK_VALUE1,
MBANK_PR_LINK_VALUE2,
MBANK_PR_LINK_MESSAGE_ADDRESS,
//MBANK_PR_LINK_DATA,
//MBANK_PR_LINK_SEND
};


enum
{
MDATA_PR_LINK_CONNECTION, // if there's a connection here, this is its index in the proc's group_connection array. -1 if no connection
MDATA_PR_LINK_INDEX, // this is the proc that this proc is connected to. -1 if no connection
MDATA_PR_LINK_OTHER_METHOD, // this is the index (pr->method[index]) of the other proc's link method which is attached to this one
MDATA_PR_LINK_RECEIVED, // this is the number of messages received this tick.
//MDATA_PR_LINK_ACCEPTS
//MDATA_PR_LINK_RECEIVE2, // second part of message
//MDATA_PR_LINK_DISCONNECT, // if 1 if method has been instructed to disconnect
};

enum // status affects what happens when method is called.
{
MSTATUS_PR_LINK_EXISTS, // returns 1 if connection active, 0 if nothing connected
MSTATUS_PR_LINK_MESSAGE, // sends message to other side (returns 1 on success, 0 if no connection)
MSTATUS_PR_LINK_RECEIVED, // returns message sent by other side (will return 0 if no connection)
MSTATUS_PR_LINK_GIVE_IRPT, // sets the process to give irpt through this link if it's destroyed or the link is broken. sets to 0 or 1
MSTATUS_PR_LINK_GIVE_DATA, // same for data
MSTATUS_PR_LINK_TAKE_IRPT, // causes the linked process to set its give value to this connection.
MSTATUS_PR_LINK_TAKE_DATA, //
MSTATUS_PR_LINK_NEXT_EXECUTION, // returns time until connected proc's next execution (0 if no connection)
MSTATUS_PR_LINK_DISCONNECT, // a call to this will tell the method to separate the procs just after execution
};

enum
{
MBANK_PR_BROADCAST_POWER,
MBANK_PR_BROADCAST_ID,
MBANK_PR_BROADCAST_VALUE1,
MBANK_PR_BROADCAST_VALUE2,
};

enum
{
MDATA_PR_BROADCAST_RANGE_MAX,
};

enum
{
MBANK_PR_LISTEN_ID_WANT,
MBANK_PR_LISTEN_ID_NEED,
MBANK_PR_LISTEN_ID_REJECT,
MBANK_PR_LISTEN_ADDRESS,
};

enum
{
MDATA_PR_LISTEN_RECEIVED,
};

#define LISTEN_MESSAGES_MAX 8
enum
{
// This enum sets out the structure of a received message after it is written to the listening proc's bcode:
LISTEN_RECORD_ID,
LISTEN_RECORD_VALUE1,
LISTEN_RECORD_VALUE2,
LISTEN_RECORD_OFFSET_X,
LISTEN_RECORD_OFFSET_Y,
LISTEN_MESSAGE_SIZE
};

#define LINK_MESSAGES_MAX 8
#define LINK_MESSAGE_SIZE 2

enum
{
MBANK_PR_YIELD_IRPT,
MBANK_PR_YIELD_DATA,
MBANK_PR_YIELD_X,
MBANK_PR_YIELD_Y
};

enum
{
MDATA_PR_YIELD_ACTIVATED, // is 1 if activated this tick, 0 otherwise
MDATA_PR_YIELD_RATE_MAX_IRPT,
MDATA_PR_YIELD_RATE_MAX_DATA,
MDATA_PR_YIELD_RANGE
};

enum
{
MRES_YIELD_SUCCESS,
MRES_YIELD_FAIL_ALREADY,
MRES_YIELD_FAIL_SETTINGS,
MRES_YIELD_FAIL_IRPT,
MRES_YIELD_FAIL_DATA,
MRES_YIELD_FAIL_TARGET,
MRES_YIELD_FAIL_FULL,

MRES_YIELD_FAIL_RANGE
};

/*
statuses:
0 - do nothing
1 - active (i.e. run immediately after execution)
2 - test
3 - successfully created new proc
4 - successful test
5 - failed: obstacle
6 - failed: insufficient irpt
7 - failed: insufficient data
8 - failed: start address out of bounds
9 - failed: end address out of bounds
*/

// also need:
//  MBANK_NEW_ADDRESS2, // address to stop at
// actually: probably would be better to have one of these be the address of an array of instructions. Another could then be the number of instructions, then another for activation.
// but use this for now.
// maybe could have one of these as an error flag, to show what went wrong if the copying failed.

// MTYPE_NEW has no hidden values for now

// This struct contains registers and register-related detail for a proc
// it includes processor registers and also the method register bank
struct registerstruct
{
// REMEMBER: When anything is added to this structure, it may need to be added to load/save routines in f_load.c/f_save.c
 s16b reg [REGISTERS];
 s16b mbank [METHOD_BANK];
};


struct methodstruct
{
// REMEMBER: When anything is added to this structure, it may need to be added to load/save routines in f_load.c/f_save.c
 int type;
// hidden contains information about the method that isn't available to the proc, and can't be read or written.
// e.g. it contains the actual recycle counter for attack methods, which is copied to the mbank register each tick.
 int data [METHOD_DATA_VALUES];
 int extension [METHOD_EXTENSIONS]; // METHOD_EXTENSIONS should be 3
 int ex_vertex; // if external, this is the vertex of the proc the method is attached to
 al_fixed ex_angle; // angle, for some kinds of external methods (e.g. MTYPE_ACCEL).

};



// GRAIN is the bitshift to go between pixels and x/y values in the coordinate system
//#define GRAIN 10
//#define GRAIN_MULTIPLY 1024

// size of block as an exponent of 2:
//#define BLOCK_SIZE_BITSHIFT 17
// a bitshift of 16 = a block size of 64 pixels or 2^16 GRAIN units (must be at least equal to largest possible size of a proc in any direction)
// Remember that the 16 incorporates the GRAIN bitshift of 10
//#define BLOCK_SIZE (1<<BLOCK_SIZE_BITSHIFT)
//#define BLOCK_SIZE_PIXELS (1<<(BLOCK_SIZE_BITSHIFT-GRAIN))
#define BLOCK_SIZE_PIXELS 128
#define BLOCK_SIZE_FIXED al_itofix(128)

// STUCK_DISPLACE_MAX is the maximum distance a proc that's stuck against another proc will be displaced in an effort to separate them. Must be smaller than a block
#define STUCK_DISPLACE_MAX al_itofix(120)

//#define BLOCK_MASK ((1<<BLOCK_SIZE_BITSHIFT)-1)

#define SENSE_CIRCLES 20
#define SENSE_CIRCLE_LENGTH 300


#define GROUP_CONNECTIONS 6

#define COMMANDS 16
#define TEAMS 8

enum
{

TCOL_FILL_BASE, // underlying shape colour
TCOL_MAIN_EDGE, // edge of process and method base
TCOL_METHOD_EDGE, // edge of method overlays

TCOL_MAP_POINT, // colour of this team's procs on the map

TCOL_BOX_FILL,
TCOL_BOX_HEADER_FILL,
TCOL_BOX_OUTLINE,
TCOL_BOX_TEXT,
TCOL_BOX_TEXT_FAINT,
TCOL_BOX_TEXT_BOLD,
TCOL_BOX_BUTTON,

TCOLS
};


// programstruct is used for all programs that are not procs (e.g. client, system etc)
struct programstruct
{
// REMEMBER: When anything is added to this structure, it may need to be added to load/save routines in f_load.c/f_save.c
 int active; // 1 if in use, 0 otherwise
 int type; // PROGRAM_TYPE_CLIENT etc.
 int player; // if relevant, this is the player index. Is -1 if program not tied to a player (e.g. neutral observer, system)
 struct bcodestruct bcode;
 struct registerstruct regs;
 struct methodstruct method [METHODS + 1]; // has an extra element for an MTYPE_END at the end

 int irpt; // irpt available to the program
 int available_irpt; // irpt reset to this before program runs
 int instr_left; // instructions left after execution (not used during execution)
 int available_instr; // available instr each cycle

};

#define PLAYER_NAME_LENGTH 13

enum
{
PLAYER_PROGRAM_ALLOWED_NONE,
PLAYER_PROGRAM_ALLOWED_DELEGATE,
PLAYER_PROGRAM_ALLOWED_OPERATOR // also allows client
};

enum
{
ALLOCATE_EFFECT_DISRUPT,
ALLOCATE_EFFECT_ALIGN,
ALLOCATE_EFFECT_TYPES
};

struct playerstruct
{

// REMEMBER: When anything is added to this structure, it may need to be added to load/save routines in f_load.c/f_save.c

 int active; // 1 if player is in game
 char name [PLAYER_NAME_LENGTH];

 int proc_index_start; // where this team starts in the proc array
 int proc_index_end; // all procs on this team should be below this (the maximum is one less)
// int template_proc_index_start; // index in templ[] array of this player's first proc template (used when creating new proc from template using PR_NEW method). Is -1 if no proc templates available.

/*
 int colour; // index in the TEAM_COLOUR array (TEAM_COLOUR_GREEN etc.)
 int packet_colour; // CLOUD_COL_xxx index
 int drive_colour; // CLOUD_COL_xxx index*/

// these colour values store colour information. They're set to defaults at startup but can
//  be changed by the MT_OB_VIEW method. The values in these variables are used by functions
//  in i_disp_in.c to set up the actual colour arrays (which are of type ALLEGRO_COLOR).
// min and max are supposed to be min and max intensity, but min can actually be == max or > max
//  if a particular colour component is to be left the same or reduced as intensity increases.
// currently alpha depends on intensity and can't be set by the VIEW method.
 int proc_colour_min [3]; // used for procs as well as various other display things
 int proc_colour_max [3];
 int packet_colour_min [3];
 int packet_colour_max [3];
 int drive_colour_min [3];
 int drive_colour_max [3];

 struct programstruct client_program; // client.active will be 0 if no client program for this player
 int program_type_allowed; // one of the PLAYER_PROGRAM_ALLOWED enums (basically, operator or delegate)

 int allocate_effect_type;
// struct templstruct templ [TEMPLATES];

 int score;
 int processes; // number of procs - includes deallocating procs

 int output_console; // the worldstruct print values are set to these before execution of any program controlled by this player (incl cl, op and proc)
 int output2_console; // the worldstruct print values are set to these before execution of any program controlled by this player (incl cl, op and proc)
 int error_console; // errors for this player are sent to this console
// int default_print_colour;
// int force_console; // can be set by OB_CONSOLE method; forces this player's programs/procs to use this console. If -1, player is muted.


// variables used for irpt limit:
// int gen_number;


};

#define INSTRUCTIONS_PROC 2048
//#define INSTRUCTIONS_PROGRAM

// Avoid making any of these larger than a signed short (as otherwise programs will have trouble querying how many instructions they have left)
#define PROGRAM_INSTRUCTIONS_SYSTEM 30000
#define PROGRAM_IRPT_SYSTEM 3000
#define PROGRAM_INSTRUCTIONS_OBSERVER 30000
#define PROGRAM_IRPT_OBSERVER 3000
#define PROGRAM_INSTRUCTIONS_OPERATOR 30000
#define PROGRAM_IRPT_OPERATOR 3000
#define PROGRAM_INSTRUCTIONS_DELEGATE 15000
#define PROGRAM_IRPT_DELEGATE 3000


struct procstruct
{
// REMEMBER: When anything is added to this structure, it may need to be added to load/save routines in f_load.c/f_save.c

 int exists; // is 1 if proc exists, 0 if proc doesn't exist, -1 if proc doesn't exist and the deallocating counter hasn't counted down yet
 int deallocating; // for destroyed procs; a countdown for total removal during which the procstruct can't be reused and any attempts to locate or identify the proc will fail (this gives other procs/clients etc time to work out that the proc has ceased to exist before the procstruct is reallocated).
 int lifetime; // number of ticks process has been active for.

 int player_index; // which player does this proc belong to?
 int index; // this is the index in the w.proc array (which is invisible to the player)

 int index_for_client; // this is an index for the proc that's accessible to the client. For now it's the same as index.
 int selected; // used by the client to tell the view which proc/procs are selected
 int select_time; // time at which the proc was selected. Used to animate the selection graphics
 int deselected; // just used to animate the selection graphics going away (not currently used)
 int map_selected; // similar to selected but appears on the map instead of the main view.


 int hit;
 int stuck;
 struct procstruct* stuck_against;
// s16b stuck;

// u16b size; // in GRAIN units - used for collision etc

 int mass;
 int moment; // moment of inertia
 int method_mass; // total mass of methods.
 int hp;
 int hp_max;
 int mobile; // 1 if proc can move, 0 if not (e.g. because of data gen method). Is set in derive_proc_properties_from_bcode() in g_proc.c. Note that a mobile==1 proc may be a member of an immobile group.
 int redundancy; // strength of redundancy method (if any)

 int shape;
 int size;
 int packet_collision_size; // is increased if the proc has a virtual interface
 int base_vertex; // Note that this value is only used during proc creation. It isn't saved to disk and so is not reliable.
 struct shapestruct* shape_str;

 int* irpt; // a pointer to either this proc's single_proc_irpt, or the proc's group's shared_irpt.
 int* irpt_max;
 int single_proc_irpt;
 int single_proc_irpt_max;
// int irpt_gen_number; // gen number for this proc (added to player.gen_number)
// int irpt_use; // irpt used last execution cycle (gross, not net of irpt generated)
// int irpt_base; // used to calculate irpt_use

 int* data; // a pointer to either this proc's single_proc_data, or the proc's group's shared_data.
 int* data_max;
 int single_proc_data;
 int single_proc_data_max;

 int base_cost; // irpt cost of proc each *cycle*. proc takes damage if not enough irpt to pay
 int irpt_gain; // irpt gain each *tick* because of irpt method
 int irpt_gain_max; // max irpt gain (used to calculate group's max irpt gain capacity)
 int instructions_each_execution; // instructions each execution cycle (determined at creation)
 int instructions_left_after_last_exec; // stores instr left after proc's last execution (so that CLOB_QUERY method can get this information)

 al_fixed x, y;
 int x_block, y_block;
 al_fixed x_speed, y_speed;
 al_fixed angle;
 al_fixed spin;
 al_fixed spin_change; // this value is set to zero each tick then accumulates changes to spin that need to be precise (e.g. engine thrust where there might be counterbalanced engines), and is finally added back to spin
 int hit_edge_this_cycle;

// these old values are the x/y/angle values for the previous frame. Used in e.g. calculating the velocity of a vertex
 al_fixed old_x, old_y;
 al_fixed old_angle;

 // provisional values are
 al_fixed provisional_x, provisional_y, provisional_angle;
 int prov; // if zero, has not had provisional values calculated this tick (can't compare indices because groups result in procs' movement being processed out-of-order)
// the difference between provisional values and test values is that test values have many more uses and need to be used carefully to avoid interrupting something else
// provisional values have only one use: telling how a proc will move this cycle.

 al_fixed max_length; // this is the maximum radius (from centre point, i.e. the point of (pr->x,pr->y)) of the proc. Is derived from its shape and size.

 al_fixed drag; // this is determined by the surface organ.

// these pointers are used in collision detection, which builds a linked list of procs in each occupied block:
 struct procstruct* blocklist_up;
 struct procstruct* blocklist_down;

// s16b target_angle;
// u16b turning;
// s16b pulse_turn; // the direction the cl will turn in this pulse

 int group;
 struct procstruct* group_connection [GROUP_CONNECTIONS];
 int connection_vertex [GROUP_CONNECTIONS]; // which vertex connection is at
 int connected_from [GROUP_CONNECTIONS]; // the other side of the group_connection array - holds the index (from 0 to GROUP_CONNECTIONS-1) of this proc in the group_connection array of the other proc

 al_fixed connection_angle [GROUP_CONNECTIONS]; // the angle of the connection (compared to proc's own angle)
 al_fixed connection_dist [GROUP_CONNECTIONS]; // the distance between the procs
 al_fixed connection_angle_difference [GROUP_CONNECTIONS]; // the difference between the angles of the procs (compared to proc's own angle)
 int number_of_group_connections; // this value must be kept up to date
 al_fixed group_angle;
 float float_angle; // this is the angle used for display if the proc is in a group (it's more accurate than the int angle)
 al_fixed group_distance;
 int group_number_from_first; // each group member has a number that indicates its distance from 1st member. Needs to be kept up to date.
 al_fixed group_angle_offset; // offset of proc's angle from group angle

// these test values are used in various places where a group is being moved or assembled and we need to
// check that all procs fit where they need to go.
 al_fixed test_group_distance;
 al_fixed test_x, test_y, test_angle;
 int test_x_block, test_y_block;
 unsigned int group_check;


// code-related data:
 struct bcodestruct bcode;
 struct registerstruct regs;
 struct methodstruct method [METHODS + 1]; // has an extra element for an MTYPE_END at the end
 int execution_count; // countdown to next code execution for this proc
// int active_method_list_permanent [METHODS]; // this is a linked list of active methods, built on proc creation.
// int active_method_list [METHODS]; // this is a linked list of active methods currently doing something, built after each execution and modified
// int active_method_list_back [METHODS]; // backwards links corresponding to active_method_list, to make it a double linked list
 // although the active_method_list arrays have METHODS elements, this is just to ensure sufficient space - the elements don't necessarily line up with the method struct array.
 int listen_method; // is index of proc's listen method, or -1 if it doesn't have one.
 int allocate_method; // -1 if no allocate method
 int virtual_method; // -1 if no virtual method
 int give_irpt_link_method; // defaults to -1. process can set this to direct resource flow after group separation
 int give_data_link_method;
// int special_method_penalty; // multiplier for certain method costs. Is usually 1 but can be increased by e.g. virtual method
 s16b command [COMMANDS];

// may need something similar for passive methods

// these list pointers are used to create linked lists in the scanner function in g_method.c
// struct procstruct* scanlist_down;
// struct procstruct* sense_circle_list_down;
// al_fixed scan_dist; // distance of proc from centre of current scan
 int scan_bitfield; // set up at proc creation and updated if changed. Basically this is a summary of proc characteristics for a scan to check against.

 int vertex_method [METHOD_VERTICES]; // indicates which method is on this vertex. -1 if no method.
// int central_method; // which method is central. -1 if no central method.

// int debug_x, debug_y, debug_x2, debug_y2;
};


//#define NO_PACKETS 500

enum
{
PACKET_TYPE_BASIC,
PACKET_TYPE_DPACKET,

PACKET_TYPES
};

struct packetstruct
{
// REMEMBER: When anything is added to this structure, it may need to be added to load/save routines in f_load.c/f_save.c
 int exists;
 int timeout; // counts down to zero
 int time; // counts up from zero (used in graphics)
 int index;
 int type;
 int player_index;

 int source_proc; // index of source proc. -1 if source proc doesn't exist
 int source_method; // source method
 int source_vertex; // vertex of source method
 int prand_seed; // pseudorandom seed set at packet creation; used for graphics

 int damage;
 int team_safe; // which team will it not hit? -1 if hits all teams.
 int colour;
 int method_extensions [METHOD_EXTENSIONS];

// int size;
 int collision_size; // additional sizes checked against collision mask. 0 = point, 1 = extra 3 pixels, 2 = 6 pixels etc.

 al_fixed x, y;
 int x_block, y_block;
 al_fixed x_speed, y_speed;
 al_fixed angle;

 struct packetstruct* blocklist_up;
 struct packetstruct* blocklist_down;

// int tail_count;
// al_fixed tail_x;
// al_fixed tail_y;


};

// currently w.max_clouds is set to CLOUDS
#define CLOUDS 600

#define CLOUD_DATA 8

struct cloudstruct
{
// REMEMBER: When anything is added to this structure, it may need to be added to load/save routines in f_load.c/f_save.c
 int exists;
 int timeout;

 int type;
 int colour;
// other details can be stored in data array below

 al_fixed x, y;
 al_fixed x2, y2;
 al_fixed x_speed, y_speed;
 al_fixed angle;

 int data [CLOUD_DATA]; // various things specific to particular cloud types

};

enum
{
CLOUD_NONE,
CLOUD_PROC_EXPLODE,
CLOUD_PACKET_MISS,
CLOUD_PACKET_HIT,
CLOUD_PACKET_HIT_VIRTUAL,
CLOUD_FAILED_NEW, // outline of failed new proc
CLOUD_DATA_LINE, // line from gen data method to block node
CLOUD_YIELD_LINE, // line from yield method target proc
CLOUD_VIRTUAL_BREAK,

CLOUD_PROC_EXPLODE_LARGE,
CLOUD_PROC_EXPLODE_SMALL,
CLOUD_PROC_EXPLODE_SMALL2,

CLOUD_STREAM,
CLOUD_DSTREAM,

CLOUD_DEBUG_LINE,
CLOUD_TYPES

};


#define MARKERS 64

enum
{
MARKER_NONE,
MARKER_BASIC, // 4 triangles pointing inwards; rotate; animations: inwards + fade in, outwards + fade out
MARKER_BASIC_6, // 6 triangles pointing inwards; rotate; animations: inwards + fade in, outwards + fade out
MARKER_BASIC_8, // 8 triangles pointing inwards; rotate; animations: inwards + fade in, outwards + fade out
MARKER_PROC_BOX, // box around proc (size depends on size of proc it's bound to); animations: outwards/inwards
MARKER_CORNERS, // lines at corners
MARKER_SHARP,
MARKER_SHARP_5,
MARKER_SHARP_7,

// special markers
MARKER_BOX, // box with x1,y1/x2,y2

// all MAP markers must come after MARKER_MAP_POINT (and all others must come before it) - see bounds-checking used in i_display.c
MARKER_MAP_POINT, // single point on map
MARKER_MAP_AROUND_1, // up, down, left, right
MARKER_MAP_AROUND_2, // points
MARKER_MAP_CROSS, // hor/ver lines
MARKER_MAP_LINE,
MARKER_MAP_BOX, // like MARKER_BOX but displays on the map

MARKER_TYPES
};

enum
{
MARKER_PHASE_APPEAR,
MARKER_PHASE_EXISTS,
MARKER_PHASE_FADE,
MARKER_PHASE_CONSTANT, // for types of marker that don't fade in/out
MARKER_PHASES
};

#define MARKER_PHASE_TIME 4
#define MARKER_SIZE_MAX 16
#define MARKER_SPIN_MAX 128


struct markerstruct // markers are signs that appear on the screen or map. observer/system program can allocate them
{
// REMEMBER: When anything is added to this structure, it may need to be added to load/save routines in f_load.c/f_save.c
 int active;
 int timeout;
 int phase; // MARKER_PHASE_APPEAR, MARKER_PHASE_EXISTS, MARKER_PHASE_FADE
 int phase_counter; // counts marker phases
 int type;
 int colour; // index in base colour array
 int size; // 0 to MARKER_SIZE_MAX (currently 16) - only relevant for some kinds of procs. Default is 8 but can be changed
 int angle;
 int spin;
 al_fixed x, y;
 al_fixed x2, y2; // for box
 int bind_to_proc;
 int bind_to_proc2; // for lines bound at both ends
};

#define BLOCK_NODES 9
#define NODE_COLS 2

struct blockstruct
{
// REMEMBER: When anything is added to this structure, it may need to be added to load/save routines in f_load.c/f_save.c
  unsigned int tag;
  struct procstruct* blocklist_down;
  unsigned int packet_tag;
  struct packetstruct* packet_down;
  int block_type;

  int node_x [BLOCK_NODES]; // offset from top left corner of block, in pixels
  int node_y [BLOCK_NODES];
  int node_size [BLOCK_NODES];
//  int node_col [BLOCK_NODES] [NODE_COLS];
  int node_team_col [BLOCK_NODES];
  int node_col_saturation [BLOCK_NODES];
// if node is disrupted:
  unsigned int node_disrupt_timestamp [BLOCK_NODES];

  int node_x_base [BLOCK_NODES];
  int node_y_base [BLOCK_NODES];

};

enum
{
BLOCK_NORMAL, // normal empty block
BLOCK_SOLID, // can't be passed
BLOCK_EDGE_LEFT, // put this to the right of a vertical line of solid blocks. acts like a normal block, except that procs in it run collision detection against the three adjacent solid blocks
BLOCK_EDGE_RIGHT,
BLOCK_EDGE_UP,
BLOCK_EDGE_DOWN,
BLOCK_EDGE_UP_LEFT, // put this to the right of a vertical line of solid blocks and below a horizontal line (e.g. top left corner of map).
BLOCK_EDGE_UP_RIGHT,
BLOCK_EDGE_DOWN_LEFT,
BLOCK_EDGE_DOWN_RIGHT,

BLOCK_TYPES
// Note: there is not yet any way to have a convex corner of solid blocks, only concave (like at the corners of the map)

/*
BLOCK_LEFT, // these push procs away from the edge
BLOCK_RIGHT,
BLOCK_TOP,
BLOCK_BOTTOM,
BLOCK_FAR_LEFT,
BLOCK_FAR_RIGHT,
BLOCK_FAR_TOP,
BLOCK_FAR_BOTTOM,*/
};


// if this changes, need to change:
//  - the number of player name and colour setting menu elements in s_menu.c
//  - the FIND_TEMPLATE_ enum and code using it in sy_copy_to_template in g_method_ex.c
#define PLAYERS 4
#define SYSTEM_COMS 64

struct worldstruct
{
// REMEMBER: When anything is added to this structure, it may need to be added to load/save routines in f_load.c/f_save.c
  int allocated; // is 0 if world not allocated (and so doesn't need to be freed before using); 1 if allocated (must call deallocate_world() before allocating new world)

  struct procstruct* proc;
  struct groupstruct* group;
  struct packetstruct* packet;
  struct cloudstruct* cloud;
  unsigned int blocktag;
  struct blockstruct** block;
  unsigned int world_time;
  unsigned int total_time;
//  struct blockstruct block [BLOCK_X] [BLOCK_Y];

//  int gen_limit; // max irpt gen for each player

  int max_procs;
  int max_groups; // this number is derived from max_procs (currently it's (max_procs / 2) + 1)
  int max_packets;
  int max_clouds; // currently this is set to CLOUDS
  int actual_procs; // rough counter of proc numbers - not reliable
  int procs_per_team; // max procs for each player
  unsigned int w_block, h_block; // size of world in blocks (currently 2^16 grain = 1 block)
//  unsigned int w_grain, h_grain; // size of world in grain (currently 2^16 grain = 1 block)
  unsigned int w_pixels, h_pixels; // size of world in pixels (currently 2^10 grain = 1 pixel)
  al_fixed w_fixed, h_fixed;

  unsigned int current_check;

  int current_output_console; // set to default just before each program runs. If this is -1, prints are muted.
  int current_error_console; // set to default just before each program runs. If this is -1, prints are muted.
  int print_colour;
  int print_proc_action_value3; // this is the value3 that will be set for console actions attached to lines printed by processes. It's set to 0 before each process runs but can be changed by the MT_EX_ACTION method.

  int players; // number of active players (including any computer-controlled ones)

// the following values are retained from w_init and not used during play, but are used when setting up the game from a loaded save:
  int may_change_client_template [PLAYERS]; // this information is only used when starting a game or loading. It should match values in equivalent w_init variables.
  int may_change_proc_templates [PLAYERS];
  int template_reference [FIND_TEMPLATES]; // this array relates a FIND_TEMPLATE_* value to a templ[] index. Is -1 if no corresponding template. Is set while world is being started (so don't use it before then), or while a saved game is being loaded.
  int player_clients;
  int allow_observer;
  int player_operator;
//  int system_program;

//  int permit_operator_player; // is -1 if no player can be operator, or player index if a player can be.
    // Allows the operator player to load an operator program into their client template.
    // Is set from w_init values and doesn't change.
    // May not be bounds-checked (shouldn't matter if it is out of bounds)

  int actual_operator_player; // is -1 if no player operator, or index of player if there is an operator.
    // Should only contain a player index if that player's client program is actually an operator (not if it's just a client program)
    //  - this is because the program should only be run during soft pause if it is an operator program
    // See copy_template_to_program() in t_template.c
    // Should be bounds-checked (from -1 to w.players-1)

// player structures contain client/operator programs, if present
  struct playerstruct player [PLAYERS]; // playerstructs must be filled from zero onwards, without any gaps
  s16b player_bcode_op [PLAYERS] [CLIENT_BCODE_SIZE]; // each player's programstruct will have a pointer to this as its bcode

// the world may have a program that runs everything:
  struct programstruct system_program; // system_program.active will be 0 if no system program
  int system_output_console; // default console for system program. -1 (or any other invalid console index) if muted.
  int system_error_console; // error console for system program. -1 (or any other invalid console index) if muted.
  s16b system_bcode_op [SYSTEM_BCODE_SIZE]; // system's programstruct will have a pointer to this as its bcode
  s16b system_com [SYSTEM_COMS]; // allows communication between system and obs/op programs

// there may be an observer program to run the user's interface (unless there's an operator program)
  struct programstruct observer_program; // observer_program.active will be 0 if no observer program
  int observer_output_console; // default console for observer program. -1 (or any other invalid index) if muted.
  int observer_error_console; // error console for observer program. -1 (or any other invalid index) if muted.
  s16b observer_bcode_op [CLIENT_BCODE_SIZE]; // observer's programstruct will have a pointer to this as its bcode

// these are markers that appear on the map - this isn't really a world thing (as they're just part of the interface), but they don't really fit in viewstruct either
  struct markerstruct marker [MARKERS];
//  int last_marker; // this is the last marker put in the world (used to work out which marker subsequent calls to place_marker() should apply to - see SELECT method code in g_method.c and marker code in g_method_clob.c)

 int background_colour [3];
 int hex_base_colour [3];

 int playing_mission; // is index of mission (MISSION_MISSION1 etc) is playing a mission. Otherwise is -1.

};




// TO DO: it should be possible to have multiple viewstructs in multiple panels. So don't put anything here that would prevent that (put general interfact stuff in controlstruct control instead
struct viewstruct
{
// REMEMBER: When anything is added to this structure, it may need to be added to load/save routines in f_load.c/f_save.c
 al_fixed camera_x, camera_y;
// int x_block, y_block;

 int window_x, window_y;
 int just_resized; // is 1 if the window has just been resized, or if it's just been loaded up. Otherwise zero. This value is available to client program through METH_VIEW call.
 al_fixed centre_x, centre_y;
// al_fixed mouse_x, mouse_y;
 int fps;
 int cycles_per_second;
// int paused;
// int pause_advance_pressed;

 struct procstruct* focus_proc; // this used to be the proc being focused on, but now camera focus is delegated to the observer program. Now is the proc being shown in the data window.
// int data_window_x, data_window_y; // location of proc data window (can be changed with observer view method)

 int w_x_pixel, w_y_pixel; // size of world in pixels
 al_fixed camera_x_min;
 al_fixed camera_y_min;
 al_fixed camera_x_max;
 al_fixed camera_y_max;

 int map_visible; // 0 or 1
 int map_x, map_y;
 int map_w, map_h;

 al_fixed map_proportion_x;
 al_fixed map_proportion_y;

// ALLEGRO_BITMAP* bmp;

};


// MBB_PER_BUTTON is how many bits there are for each button
#define MBB_PER_BUTTON 3

enum // mouse button bitfield entries (for control.mbutton_bitfield)
{
MBB_BUT1_PRESSED, // is button 1 (left) being pressed?
MBB_BUT1_JUST_PRESSED, // was button 1 pressed since the previous tick? (PRESSED will also be 1)
MBB_BUT1_JUST_RELEASED, // was button 1 released since the previous tick? (PRESSED will be 0)
MBB_BUT2_PRESSED,
MBB_BUT2_JUST_PRESSED,
MBB_BUT2_JUST_RELEASED,
// need wheel things too
MBB_BITS
};

enum
{
MOUSE_STATUS_OUTSIDE, // outside (e.g. on editor window)
MOUSE_STATUS_AVAILABLE, // on screen, no other status applies
MOUSE_STATUS_MAP, // in the map
MOUSE_STATUS_CONSOLE, // on a console
//MOUSE_STATUS_SCORE, // on a score box
MOUSE_STATUS_PROCESS, // on the focus_proc information box
};

enum
{
KEY_0,
KEY_1,
KEY_2,
KEY_3,
KEY_4,
KEY_5,
KEY_6,
KEY_7,
KEY_8,
KEY_9,

KEY_A,
KEY_B,
KEY_C,
KEY_D,
KEY_E,
KEY_F,
KEY_G,
KEY_H,
KEY_I,
KEY_J,
KEY_K,
KEY_L,
KEY_M,
KEY_N,
KEY_O,
KEY_P,
KEY_Q,
KEY_R,
KEY_S,
KEY_T,
KEY_U,
KEY_V,
KEY_W,
KEY_X,
KEY_Y,
KEY_Z,

KEY_MINUS,
KEY_EQUALS,
KEY_SBRACKET_OPEN,
KEY_SBRACKET_CLOSE,
KEY_BACKSLASH,
KEY_SEMICOLON,
KEY_APOSTROPHE,
KEY_COMMA,
KEY_PERIOD,
KEY_SLASH,

KEY_UP,
KEY_DOWN,
KEY_LEFT,
KEY_RIGHT,

KEY_ENTER,
KEY_BACKSPACE,
KEY_INSERT,
KEY_HOME,
KEY_PGUP,
KEY_PGDN,
KEY_DELETE,
KEY_END,
KEY_TAB,
// KEY_ESCAPE is not available to user programs

KEY_PAD_0,
KEY_PAD_1,
KEY_PAD_2,
KEY_PAD_3,
KEY_PAD_4,
KEY_PAD_5,
KEY_PAD_6,
KEY_PAD_7,
KEY_PAD_8,
KEY_PAD_9,
KEY_PAD_MINUS,
KEY_PAD_PLUS,
KEY_PAD_ENTER,
KEY_PAD_DELETE,

// These are last so that any_key is set to them only if they're the only thing being pressed
KEY_LSHIFT,
KEY_RSHIFT,
KEY_LCTRL,
KEY_RCTRL,

KEYS

};


struct controlstruct // this struct holds constantly updated user interface information, for use by client/system processes. It is derived from the ex_control struct, which is for use by game code.
{
// REMEMBER: When anything is added to this structure, it may need to be added to load/save routines in f_load.c/f_save.c
 int mouse_status; // what part of the display the mouse pointer is in
 int mouse_x_world_pixels, mouse_y_world_pixels; // position in the world (not on the screen), in pixels
 int mouse_x_screen_pixels, mouse_y_screen_pixels; // position on the screen, in pixels
// int mbutton_bitfield;
// int mouse_hold_x_pixels [2], mouse_hold_y_pixels [2]; // if button is being held, this is the x/y position where it was when the hold started. x is -1 if the hold started offscreen or when mouse input not accepted.
// int mb_bits [MBB_BITS];
 int mbutton_press [2];
 int key_press [KEYS];
 int any_key;
};

enum
{
BUTTON_JUST_RELEASED = -1,
BUTTON_NOT_PRESSED,
BUTTON_JUST_PRESSED,
BUTTON_HELD
};



// a group can't exceed this size
#define GROUP_MAX_MEMBERS 24

struct groupstruct
{
// REMEMBER: When anything is added to this structure, it may need to be added to load/save routines in f_load.c/f_save.c
 int exists;
 struct procstruct* first_member;
 int shared_irpt;
 int shared_irpt_max;
 int shared_data;
 int shared_data_max;
 int total_irpt_gain_max; // irpt gain per tick if all irpt methods in group are generating at max capacity

 int total_mass;
 int total_members;
 int moment; // moment of inertia - see calculate_group_moment_of_inertia() in g_group.c.
 int mobile; // 1 if group can move, 0 otherwise (is 0 if at least one proc member has mobile==0)

// x, y values: for mobile groups, are the group's centre of mass. For groups with roots (immobile), is the first member
 al_fixed x, y, test_x, test_y;
 int centre_of_mass_test_x, centre_of_mass_test_y; // used for calculating centre of mass (which is likely to cause an al_fixed value to overflow)
 al_fixed x_speed, y_speed;
 al_fixed drag; // is equal to the lowest (i.e. most draggy) drag of all group members
 al_fixed angle, test_angle;
// int angle_angle; // angle in angle units
 al_fixed spin;
 al_fixed spin_change; // this value is set to zero each tick then accumulates changes to spin that need to be precise (e.g. engine thrust where there might be counterbalanced engines), and is finally added back to spin

 int hit_edge_this_cycle;

// this value is incremented if the group is stuck against a proc or another group. Once it gets high enough, we try to free the group by jittering it around randomly.
 int stuck;

};

/*

Every cl has the following variables:

group - contains the group it's a member of (1 only). -1 if no group
next_in_group - index of next member of group


*/





enum
{
COL_GREY,
COL_RED,
COL_GREEN,
COL_BLUE,
COL_YELLOW,
COL_ORANGE,
COL_PURPLE,
COL_TURQUOISE,
COL_AQUA,
COL_CYAN,
COL_DULL,
BASIC_COLS
};

#define BASIC_SHADES 5
// the shade values can be adjusted as needed if BASIC_SHADES is changed
#define SHADE_MIN 0
#define SHADE_LOW 1
#define SHADE_MED 2
#define SHADE_HIGH 3
#define SHADE_MAX 4

enum
{
TRANS_FAINT,
TRANS_MED,
TRANS_THICK,
BASIC_TRANS
};

//#define MAP_W 150
//#define MAP_H 150

// MAP_EDGE_DISTANCE is the distance from the edge of the screen to the edge of the map
#define MAP_EDGE_DISTANCE 50

#define COLLISION_MASK_SIZE 120
#define COLLISION_MASK_BITSHIFT 1
#define COLLISION_MASK_LEVELS 10

// MASK_CENTRE needs to be adjusted because COLLISION_MASK_BITSHIFT will be applied to it before it
//  is translated to an actual position on the collision mask
#define MASK_CENTRE ((COLLISION_MASK_SIZE/2)<<COLLISION_MASK_BITSHIFT)
#define MASK_CENTRE_FIXED al_itofix(MASK_CENTRE)


// settingsstruct stuff

#define MODE_BUTTON_SIZE 16
#define MODE_BUTTON_SPACING 7
#define MODE_BUTTON_Y 5

enum
{
OPTION_WINDOW_W, // resolution of monitor or size of window
OPTION_WINDOW_H,
OPTION_FULLSCREEN,
OPTION_VOL_MUSIC,
OPTION_VOL_EFFECT,
OPTION_SPECIAL_CURSOR, // draws a special mouse cursor instead of the system one. Included because of a report that the mouse cursor was not displaying correctly.
OPTION_DEBUG, // can be used to set certain debug values without recompiling.
OPTIONS
};

enum
{
SELECTOR_NONE,
SELECTOR_BASIC
};

enum
{
INPUT_WORLD,
INPUT_EDITOR

};

enum
{
EDIT_WINDOW_PROGRAMS,
EDIT_WINDOW_TEMPLATES,
EDIT_WINDOW_EDITOR,
EDIT_WINDOW_SYSMENU,
EDIT_WINDOW_CLOSED,
EDIT_WINDOWS
// EDIT_WINDOW enum must match MODE_BUTTON enum
};

enum
{
MODE_BUTTON_PROGRAMS,
MODE_BUTTON_TEMPLATES,
MODE_BUTTON_EDITOR,
MODE_BUTTON_SYSMENU,
MODE_BUTTON_CLOSE,
MODE_BUTTONS
// MODE_BUTTON enum must match EDIT_WINDOW enum
};


// This is a set of overarching values like system options and a record of what state the game is in etc
struct settingsstruct
{
// REMEMBER: When anything is added to this structure, it may need to be added to load/save routines in f_load.c/f_save.c
 int status; // contains a GAME_STATUS enum value which indicates what the user is doing

 int edit_window; // EDIT_WINDOW_CLOSED, _EDITOR or _TEMPLATES

 int keyboard_capture; // which window (world or editor) keyboard use is sent to (changed by clicking on the appropriate window)

 int mode_button_available [MODE_BUTTONS]; // 0 or 1. If 0, mode button not displayed and cannot be clicked
 int mode_button_x [MODE_BUTTONS];
 int mode_button_y [MODE_BUTTONS];
 int mode_button_highlight [MODE_BUTTONS];

 int editor_x_split; // x location of split between game and editor sides of the screen (when editor is up)
 int edit_window_columns;
// int font_text_width; // pixel width of basic fixed-width font

 int sound_on; // if 1, plays sound. If 0, won't play sound at all (is set to 0 if Allegro audio addon fails to initialise)

 int option [OPTIONS];

};


// gamestruct stuff

enum
{
GAME_PHASE_WORLD, // the world is open
GAME_PHASE_PREGAME, // pregame phase (mostly like world phase but with some differences)
// WORLD and PREGAME phases should be 0 and 1 (as some comparisons use if (game.phase > GAME_PHASE_PREGAME) to detect all other phases
GAME_PHASE_TURN, // world is open; waiting for players to complete a turn.
GAME_PHASE_OVER, // game is over

GAME_PHASE_MENU, // user is still in start menus (this value won't be encountered much, if ever, in practice)

GAME_PHASES // used in game loading bounds-check
};


enum
{
// these are used by SY_MANAGE method to indicate game ending states
GAME_END_BASIC, // just displays game over sign
GAME_END_YOU_WON, // displays "You won" message or something. *** if playing a mission/tutorial, finishing with this status sets the mission status to complete
GAME_END_YOU_LOST, // displays failure message
GAME_END_PLAYER_WON, // displays victory message for player indicated by field 2
GAME_END_PLAYER_LOST, // displays failure message for player indicated by field 2
GAME_END_DRAW, // displays draw message
GAME_END_TIME, // displays "out of time" message

GAME_END_ERROR_STATUS, // error in game over SY_MANAGE call: invalid game_end status (ends game anyway)
GAME_END_ERROR_PLAYER, // invalid player index for PLAYER_WON/LOST status

GAME_END_STATES

// remember that the system program can display more detailed win/loss/etc information in a console.
// Don't use these for a mission! Use the MTYPE_SY_MISSION method (which allows unlocking of later missions)
};


enum
{
GAME_TYPE_INDEFINITE, // game will simply continue, without turns, until user quits or system program declares victory
GAME_TYPE_TURNS_LIMITED_TIMED,
GAME_TYPE_TURNS_LIMITED_UNTIMED,
GAME_TYPE_TURNS_UNLIMITED_TIMED,
GAME_TYPE_TURNS_UNLIMITED_UNTIMED,

GAME_TYPES // used in game loading bounds-check
};

enum
{
FAST_FORWARD_OFF,
FAST_FORWARD_JUST_STARTED, // first frame in which FF activated (this status indicates that one more frame should be drawn, to avoid up to a second pause before the "FAST FORWARD" message appears on screen)
FAST_FORWARD_ON
};


enum
{
// These are not yet implemented. All fast forwarding is "skippy"
FAST_FORWARD_TYPE_SMOOTH, // runs the game at max speed, drawing a frame for each tick but not waiting to draw the next tick
FAST_FORWARD_TYPE_SKIPPY, // Fastest type. Runs the game at max speed for 1 second, then draws a frame, then runs again at max speed etc.
FAST_FORWARD_TYPE_4X, // runs the game at 4x speed, drawing a frame for each tick if there's time (and at least one each second)
FAST_FORWARD_TYPE_8X, // runs the game at 8x speed, drawing a frame for each tick if there's time (and at least one each second)

FAST_FORWARD_TYPES
};

// number of ticks in pregame (unless ended earlier by system program)
#define PREGAME_TIME 16

// gamestruct game is initialised in g_game.c
struct gamestruct
{
// REMEMBER: When anything is added to this structure, it may need to be added to load/save routines in f_load.c/f_save.c
 int phase; // GAME_PHASE_? enums
 int type; // GAME_TYPE_? enums
//  file names may be used in future if file naming conventions are adopted, allowing automatic turnfile loading.
// char file_path [FILE_PATH_LENGTH]; // path where turnfiles etc are stored. Is determined by path to gamefile/savefile.
// char name [FILE_NAME_LENGTH]; // name of game (actually limited to just a few letters). Is put at start of all turn files.

 int turns; // number of turns in game - is 0 for unlimited turns
 int current_turn; // which turn is it now (starts at 0 during pregame; 1 is first actual turn; is always 0 for indefinite game)

 int minutes_each_turn; // how long each turn is (in minutes) - is 0 for turns of unlimited length
 int current_turn_time_left; // how much time is left in current turn (in ticks)

 int pause_soft; // if 1, game is in soft pause. observer, operator and system programs still run but no others do and nothing else happens
 int pause_hard; // if 1, game is in hard pause. no programs run and ex_control input is not converted to control input.
 int fast_forward; // uses FAST_FORWARD_? enums
 int fast_forward_type; // uses FAST_FORWARD_TYPE_? enums
 int force_turn_finish; // if 1, forces the turn to end (even if it hasn't run out of time, or is indefinited). Can be set by SY_MANAGE method

 int pregame_time_left; // ticks remaining in pregame phase

 int game_over_status; // GAME_END_? enum. Ignored unless game.phase is GAME_PHASE_OVER
 int game_over_value; // some game end statuses use this (e.g. to indicate which player won)

// Note that the sound values aren't saved as part of a saved game
 int play_sound;
 int play_sound_sample;
 int play_sound_note;
 int play_sound_vol;
 int play_sound_counter; // prevents sounds being played too frequently by keeping a minimum number of ticks between them.

};

/*

New: pregame

Pregame won't be based on the menu anymore. Instead, it will be controlled by the system program.
When starting, the world will run for a while without updating the display or allowing user input.
Client/observer/operator programs will not run, however (system program can load them into templates, and they will be copied to programs at start of first turn).

- World will run for something like 100 ticks (or fewer if the system program cancels using manage method)
- During this time, soft pause will be available (system program can pause world if it likes) but not hard pause or fast forward (fast forward will be applied in any case)


To do:
- at end of each turn, clear all client/observer/operator templates (but don't clear programs)
- at start of each turn, check each of those templates. If empty, the existing program continues. If not empty, copy from template to program.


How will turns work?

Probably like this:
- at start of game, a gamefile is generated and can be saved.
- if a player loads a gamefile, it may come with templates pre-loaded.
 - player can then:
  - load programs into their templates
  - save a turnfile containing a single player's templates
  - load a turnfile for each player, which fills the templates for that player
  - generate a new gamefile containing all of the loaded templates
  - generate a saved game containing all game data including templates (saved game will load at turn interface)

How will the interface work?
- at end of turn, game will go into soft pause.
- a message box will appear in middle of game screen telling player what's happening
- system menu allows:
 - start next turn
 - save game (will save in interturn mode)
- player can now go into templates and mess around with them.
- turnfile templates will be available. These allow player to load and save turnfiles (*.t).


File naming conventions:
- none for now. Should probably add something, but for now players will have to load each turnfile manually. Can use saved game to limit the need to do this if needed.

*/

enum
{
PI_OPTION, // if 1, user can set this between PI_MIN and PI_MAX (starts at PI_DEFAULT). if 0, it will be fixed at PI_DEFAULT
PI_DEFAULT,
PI_MIN,
PI_MAX,
PI_VALUES
};



// MAX_LIMITED_GAME_TURNS is the maximum number of turns in a game with limited turns
#define MAX_LIMITED_GAME_TURNS 16
// MAX_UNLIMITED_GAME_TURNS is the maximum number of turns in a game without limited turns (can be almost any amount really, although > 32767 would mean programs would lose track of it)
#define MAX_UNLIMITED_GAME_TURNS 30000
#define MAX_TURN_LENGTH_MINUTES 60

// this is a structure that determines what options the user has in setting up a game.
// it's either set by default (in which case it gives all or at least most options) or is determined by a system program
// is used in setting up a world_initstruct
// * PI_VALUES values have 3 preinit values - the first is default value, second min, third max
//   if default value is -1, defaults are used for all 3.
//   if min is -1 (or all 3 numbers the same), default value is mandatory
struct world_preinitstruct
{
 int players [PI_VALUES];

// int game_type [PI_VALUES]; - this is derived from game_turns
 int game_turns [PI_VALUES];
 int game_minutes_each_turn [PI_VALUES];

 int procs_per_team [PI_VALUES];
// int gen_limit [PI_VALUES];
 int packets_per_team [PI_VALUES];
 //int max_clouds [3]; not sure this should even be set by user
 int w_block [PI_VALUES];
 int h_block [PI_VALUES];


 int choose_col; // is 0 if colours can't be chosen, 1 otherwise
 //int team_colour [PLAYERS];

// int allow_user_player;
 int allow_player_clients; // (0 = no, 1 = yes)
 int allow_user_observer; //  (if 1, allows user to set observer program (unless player 1 is operator) and set it to default if unspecified and player 1's client isn't an operator) (if 0, no observer - although system program can perform observer functions)
 int player_operator; // (-1 = no player, otherwise index of player who is operator)
 int may_change_client_template [PLAYERS]; // if 1, the user may change this player's templates. If 0, only the system program may do so.
 int may_change_proc_templates [PLAYERS];
 char player_name [PLAYERS] [PLAYER_NAME_LENGTH];

// if adding or changing anything, changes must be reflected in:
//  - parse_system_program_interface() in c_comp.c
//  - init_menu_elements() in s_menu.c
//  - setup_default_world_pre_init() in s_setup.c
//  - verify_world_pre_init() in s_setup.c
//  - derive_pre_init_from_system_program() in s_setup.c
};

// this is a structure to contain all world initialisation values - i.e. independent values that may be set by a user or system program.
// a world can be created based on these values.
// the parameters within which the values are set are determined by a world_preinitstruct, which is determined by the system program (or is left to default values)
struct world_initstruct
{
// REMEMBER: When anything is added to this structure, it may need to be added to load/save routines in f_load.c/f_save.c
 int players;

// game parameters
 int game_turns; // 0 means indefinite
 int game_minutes_each_turn; // ignored if indefinite

// basic world parameters
 int procs_per_team;
// int gen_limit;
 int packets_per_team;
 int max_clouds;
 int w_block;
 int h_block;

// team colours
// int team_colour [PLAYERS]; // which team colour each player gets
 int may_change_client_template [PLAYERS]; // whether a player may change the player's client template
 int may_change_proc_templates [PLAYERS]; // whether a player may change the player's proc templates
 char player_name [PLAYERS] [PLAYER_NAME_LENGTH];

// game/client details
// int user_player; // if a player is the user, this is the player index. -1 if user is not controlling any player directly.
  // user_player not really used for now
 int player_clients; // is 1 if players have client programs, 0 otherwise
 int allow_observer; // is 1 if observer allowed, 0 otherwise (may be 0 if e.g. the system program is handling the interface)
 int player_operator; // is -1 if no operator, or the index of the player
 int system_program; // is 1 if there is a system program

// when adding anything to this, may need to add to f_load and f_save.

};


// TEMPLATES_PER_PLAYER is how many proc templates each player gets
#define TEMPLATES_PER_PLAYER 4

enum
{
TEMPL_TYPE_CLOSED, // cannot use this template slot for anything
TEMPL_TYPE_DELEGATE,
TEMPL_TYPE_OBSERVER,
TEMPL_TYPE_OPERATOR,
TEMPL_TYPE_PROC,
TEMPL_TYPE_DEFAULT_OPERATOR, // default operator for missions
TEMPL_TYPE_DEFAULT_OBSERVER, // default observer for advanced missions
TEMPL_TYPE_DEFAULT_DELEGATE, // default delegate for advanced missions
TEMPL_TYPE_DEFAULT_PROC, // default operator for missions
TEMPL_TYPE_SYSTEM,
// if changing this list, remember to fix the list in expected_program_type() below

TEMPL_TYPES

};



#define CONSOLES 8

enum
{
PRINT_COL_DGREY,
PRINT_COL_LGREY,
PRINT_COL_WHITE,
PRINT_COL_LBLUE,
PRINT_COL_DBLUE,
PRINT_COL_LRED,
PRINT_COL_DRED,
PRINT_COL_LGREEN,
PRINT_COL_DGREEN, // rgb values are in print_col_value [] in i_disp.c
PRINT_COL_LPURPLE,
PRINT_COL_DPURPLE,
PRINT_COLS
};

enum
{
BACK_COL1,
BACK_COL2,
BACK_COLS

};

#define CONSOLE_LINE_FADE 16

#define BACK_COL_SATURATIONS 4
#define BACK_COL_FADE 8

#define MAP_MINIMUM_SIZE 30


// TORQUE_DIVISOR is an arbitrary value to fine-tune the rotation physics
#define TORQUE_DIVISOR 10000
#define TORQUE_DIVISOR_FIXED al_itofix(10)
// FORCE_DIVISOR is an arbitrary value to fine-tune the bounce physics
#define FORCE_DIVISOR 4
#define FORCE_DIVISOR_FIXED al_itofix(FORCE_DIVISOR)
// SPIN_MAX is the maximum spin for procs and groups.
#define SPIN_MAX 64
#define SPIN_MAX_FIXED al_itofix(2)
#define NEGATIVE_SPIN_MAX_FIXED al_itofix(-2)
// al_itofix(2) is AFX_ANGLE_128. ANGLE_128 is 64.

// need to implement GROUP_MAX_DISTANCE properly! Currently it's only applied in f_load.c
#define GROUP_MAX_DISTANCE al_itofix(600)

#define NODE_DISRUPT_TIME_CHANGE 16

// I think 33 is correct
#define TICKS_TO_SECONDS_DIVISOR 33



#endif
// don't put anything after this







