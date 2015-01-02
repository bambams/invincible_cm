#include <allegro5/allegro.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "m_config.h"
#include "m_globvars.h"

#include "g_header.h"
//#include "g_odata.h"
#include "m_maths.h"
#include "g_shape.h"

#include "g_group.h"
#include "g_misc.h"


//static void change_group(struct procstruct* pr, int g);

static struct groupstruct* form_new_group(struct procstruct* pr1);
static int new_group(struct procstruct* pr1, struct procstruct* pr2, int vertex_1, int vertex_2);
static int add_proc_to_group(struct procstruct* pr1, struct procstruct* new_pr, int vertex_1, int vertex_2);
//int connect_groups(struct procstruct* pr1, struct procstruct* pr2, int vertex_1, int vertex_2);
static void fix_link_methods(struct procstruct* pr1, struct procstruct* pr2, int vertex_1, int vertex_2, int connection_1, int connection_2);
static void fix_link_connections(struct procstruct* pr1, struct procstruct* pr2, int vertex_1, int vertex_2, int connection_1, int connection_2);
static void clear_link_method(struct procstruct* pr, int vertex);
static int check_group_mobile(struct procstruct* pr, struct groupstruct* gr);

static void fix_group(struct procstruct* pr, int g, struct groupstruct* gr, int distance_from_first_member);
static void fix_group2(struct procstruct* pr, struct procstruct* upstream_pr, int connection_index, int g, struct groupstruct* gr, int distance_from_first_member);
static void fix_immobile_group2(struct procstruct* pr, int g, struct groupstruct* gr, int distance_from_first_member);
//static void set_new_group_member_location(struct procstruct* pr, struct procstruct* upstream_pr, int connection_index);

//static void fix_group2(struct procstruct* pr, int g, struct groupstruct* gr, int distance_from_first_member);
static void set_group_positions(struct procstruct* pr, struct groupstruct* gr);

//static void group_changes_group(struct procstruct* pr, int g, struct groupstruct* gr, int distance_from_first_member);

static int find_first_empty_connection(struct procstruct* pr);

//int disconnect_procs(struct procstruct* pr1, struct procstruct* pr2);
static void remove_single_proc_from_group(struct procstruct* pr1, struct procstruct* pr2);
static void remove_group_connection(struct procstruct* pr, struct procstruct* connected);
static void split_group(struct procstruct* pr1, struct procstruct* pr2);

static int find_next_connected_proc(struct procstruct* pr, int connection_index);
static void	fix_resource_values(int p);
static void	fix_resource_values2(struct procstruct* pr, struct groupstruct* gr);

enum
{
RESOURCE_DISTRIB_STATUS_TAKE_ALL,
RESOURCE_DISTRIB_STATUS_TAKE_NONE, // tries to take none (but may end up with some of remainder if others are full)
RESOURCE_DISTRIB_STATUS_SHARE,
RESOURCE_DISTRIB_STATUS_NO_DISTRIB // takes none, and gets no remainder (either because it was destroyed or because it's already been processed)
};

// This struct is set up just before a proc is removed from a group,
// then used immediately afterwards (when all procs have been reassigned
//  to new groups, or left single). Its contents can then be discarded.
// It contains details of how many irpt each proc is left with afterwards.
// May not have an entry for each connection (e.g. if a single connection was severed
//  and the proc was not destroyed, there will only be entries for the proc
//  itself and the one it was separating from)
struct resource_distributionstruct
{
 int irpt_pool;
 int data_pool;
// these arrays are of size GROUP_CONNECTIONS + 1 because they may include the proc itself (actually I'm not sure this is true - no more than two should be used unless the proc is destroyed)
 int connected_proc_index [GROUP_CONNECTIONS + 1]; // will be -1 if empty. Entries are not packed.
 int connected_proc_irpt_status [GROUP_CONNECTIONS + 1]; // irpt status that the formerly connected proc is left with.
 int connected_proc_data_status [GROUP_CONNECTIONS + 1]; // data status that the formerly connected proc is left with.
 int rdist_taking_all_irpt; // The proc with the RESOURCE_DISTRIB_STATUS_TAKE_ALL status. -1 if no such proc.
 int rdist_taking_all_data; // The proc with the RESOURCE_DISTRIB_STATUS_TAKE_ALL status. -1 if no such proc.
};

struct resource_distributionstruct rdist;


// w.current_check
// every time a recursive group function is called, this value is incremented. When a proc has been processed by the function, its group_check
//  value is set to current_check. This way we can make sure a recursive function only visits each proc once.
// current_check and group_check for all procs is set to zero each cycle during pass1 in g_pass1.c (which externs this variable)


/*
 This function connects any two procs together
 It will select the correct function to call depending on whether one, both or neither procs is already
  part of a group
 returns -1 on failure
 returns group number on success

How should angle_offset be worked out?
- It'll need to be worked out before pr2 is even created, so that pr2 isn't created if there's no room. This means it can't be done in any of the group functions.


*/
int connect_procs(struct procstruct* pr1, struct procstruct* pr2, int vertex_1, int vertex_2)
{

 if (pr1->group == pr2->group && pr1->group != -1)
 {
//  fprintf(stdout, "\nconnect_procs: same group fail");
  return -1;
 }

 if (pr1->number_of_group_connections == GROUP_CONNECTIONS
  || pr2->number_of_group_connections == GROUP_CONNECTIONS)
 {
//  fprintf(stdout, "\nconnect_procs: group_connections fail (%i, %i)", pr1->number_of_group_connections, pr2->number_of_group_connections);
  return -1;
 }

 if (pr1->number_of_group_connections == 0
  && pr2->number_of_group_connections == 0)
  {
//   fprintf(stdout, "\n new_group");
   return new_group(pr1, pr2, vertex_1, vertex_2);
  }


 if (pr1->number_of_group_connections > 0
  && pr2->number_of_group_connections == 0)
  {
//   fprintf(stdout, "\n first add_proc_to_group");
   return add_proc_to_group(pr1, pr2, vertex_1, vertex_2);
  }


 if (pr1->number_of_group_connections == 0
  && pr2->number_of_group_connections > 0)
  {
//   fprintf(stdout, "\n second add_proc_to_group");
   return add_proc_to_group(pr2, pr1, vertex_2, vertex_1); // pr1 and pr2 reversed (vertices also reversed)
  }



// okay, so they must both be members of groups:

//  connect_groups(pr1, pr2, vertex_1, vertex_2);
 // not implemented

  return 1;

}

/*
This function connects 2 procs that are not presently part of a group

Assumes:
- pr1 and pr2 have no group connections
- pr1 and pr2 have both had their x/y positions set correctly

*/
static int new_group(struct procstruct* pr1, struct procstruct* pr2, int vertex_1, int vertex_2)
{

 int g;
 struct groupstruct* gr;

 for (g = 0; g < w.max_groups; g ++)
 {
  if (w.group[g].exists == 0)
   break;
 }

#ifdef SANITY_CHECK
// max_groups should be such that this cannot happen (there should always be an available group)
 if (g == w.max_groups)
 {
  fprintf(stdout, "g_group.c: failed to create new group");
  error_call();
 }
#endif

 gr = &w.group[g];

 gr->exists = 1;
 gr->first_member = pr1;

 gr->total_members = 2;
 gr->x = pr1->x; // this is updated to be the group's actual centre of mass in fix_group2() (called from fix_group() below)
 gr->y = pr1->y;
 gr->x_speed = pr1->x_speed + pr2->x_speed; // TO DO: deal with inertia and momentum properly
 gr->y_speed = pr1->y_speed + pr2->y_speed;
 gr->angle = 0;
// gr->angle_angle = 0;
 gr->spin = 0;
 gr->spin_change = 0;
// gr->turning = 0;
 gr->stuck = 0;
 gr->drag = DRAG_BASE_FIXED;//DRAG_BASE;

 pr1->group = g;
 pr1->group_connection [0] = pr2;
 pr1->number_of_group_connections = 1;
 pr1->connected_from [0] = 0;
 pr1->connection_vertex [0] = vertex_1;
// if (o1 != -1)
  //cl1->organ [o1].connection_to_other = 0;
 pr1->group_number_from_first = 0;

 pr1->group_angle = al_itofix(0); // TO DO SOON: when gr->x is fixed to centre of mass, fix this too
 pr1->group_distance = 0;
// pr1->connection_angle [0] = (radians_to_angle(atan2((s32b) cl2->y - (s32b) cl1->y, (s32b) cl2->x - (s32b) cl1->x)) - cl1->angle) & ANGLE_MASK;
 pr1->group_angle_offset = pr1->angle;

 pr2->group = g;
 pr2->group_connection [0] = pr1;
 pr2->number_of_group_connections = 1;
 pr2->connected_from [0] = 0;
 pr2->connection_vertex [0] = vertex_2;
// if (o2 != -1)
//  cl2->organ [o2].connection_to_other = 0;
 pr2->group_number_from_first = 1;

 pr2->group_angle = get_angle(pr2->y - gr->y, pr2->x - gr->x);
 pr2->group_distance = distance(pr2->y - gr->y, pr2->x - gr->x);
// pr2->connection_angle [0] = get_angle(pr1->y - pr2->y, pr1->x - pr2->x) - pr2->angle;
// TO DO: optimise these!!
 pr2->group_angle_offset = pr2->angle;

 fix_link_connections(pr1, pr2, vertex_1, vertex_2, 0, 0); // assumes connection indices both 0

// set_new_group_member_location(pr2, pr1, 0); // sets pr2's x/y/angle

 fix_group(pr1, g, gr, 0);

 fix_link_methods(pr1, pr2, vertex_1, vertex_2, 0, 0); // assumes connection indices both 0

// finally, share irpt and data:
 gr->shared_irpt = *(pr1->irpt) + *(pr2->irpt);
 gr->shared_irpt_max = pr1->single_proc_irpt_max + pr2->single_proc_irpt_max;
 gr->total_irpt_gain_max = pr1->irpt_gain_max + pr2->irpt_gain_max;
 pr1->irpt = &gr->shared_irpt;
 pr1->irpt_max = &gr->shared_irpt_max;
 pr2->irpt = &gr->shared_irpt;
 pr2->irpt_max = &gr->shared_irpt_max;

 gr->shared_data = *(pr1->data) + *(pr2->data);
 gr->shared_data_max = pr1->single_proc_data_max + pr2->single_proc_data_max;
 pr1->data = &gr->shared_data;
 pr1->data_max = &gr->shared_data_max;
 pr2->data = &gr->shared_data;
 pr2->data_max = &gr->shared_data_max;

 return g;

}

// this function fixes the MTYPE_PR_LINK methods at the connecting vertices of the two procs
// connection_1 is the index (in pr1's group_connections array) of the connection to pr2. connection_2 is same for pr2.
// it assumes that all values passed to it are valid, and that the methods exist
static void fix_link_methods(struct procstruct* pr1, struct procstruct* pr2, int vertex_1, int vertex_2, int connection_1, int connection_2)
{

 int m1 = pr1->vertex_method[vertex_1];
 int m2 = pr2->vertex_method[vertex_2];

 pr1->method[m1].data [MDATA_PR_LINK_CONNECTION] = connection_1;
 pr2->method[m2].data [MDATA_PR_LINK_CONNECTION] = connection_2;

 pr1->method[m1].data [MDATA_PR_LINK_INDEX] = pr2->index;
 pr2->method[m2].data [MDATA_PR_LINK_INDEX] = pr1->index;

 pr1->method[m1].data [MDATA_PR_LINK_OTHER_METHOD] = m2;
 pr2->method[m2].data [MDATA_PR_LINK_OTHER_METHOD] = m1;


}

static void fix_link_connections(struct procstruct* pr1, struct procstruct* pr2, int vertex_1, int vertex_2, int connection_1, int connection_2)
{

 pr1->connection_angle [connection_1] = get_angle(pr2->y - pr1->y, pr2->x - pr1->x) - pr1->angle;
 pr1->connection_dist [connection_1] = distance(pr2->y - pr1->y, pr2->x - pr1->x);
 pr1->connection_angle_difference [connection_1] = pr2->angle - pr1->angle;

 pr2->connection_angle [connection_2] = get_angle(pr1->y - pr2->y, pr1->x - pr2->x) - pr2->angle;
 pr2->connection_dist [connection_2] = distance(pr1->y - pr2->y, pr1->x - pr2->x);
 pr2->connection_angle_difference [connection_2] = pr1->angle - pr2->angle;

/*
 fprintf(stdout, "\fix_link_connections: pr1 (index %i) at %i,%i pr2 (index %i) at %i,%i pr1->c_a %i",
         pr1->index,
         al_fixtoi(pr1->x),
         al_fixtoi(pr1->y),
         pr2->index,
         al_fixtoi(pr2->x),
         al_fixtoi(pr2->y),
         al_fixtoi(pr1->connection_angle [connection_1]));
*/
}


// this function clears data values of MTYPE_PR_LINK method at vertex.
// use it when procs are disconnected.
// must be called for both ends.
static void clear_link_method(struct procstruct* pr, int vertex)
{

 int m = pr->vertex_method[vertex];

// fprintf(stdout, "\nclear_link_method: pr %i vertex %i m %i MDATA_PR_LINK_CONNECTION %i MDATA_PR_LINK_INDEX %i MDATA_PR_LINK_OTHER_METHOD %i",
//         pr->index, vertex, m, pr->method[m].data [MDATA_PR_LINK_CONNECTION], pr->method[m].data [MDATA_PR_LINK_INDEX], pr->method[m].data [MDATA_PR_LINK_OTHER_METHOD]);

 pr->method[m].data [MDATA_PR_LINK_CONNECTION] = -1;
 pr->method[m].data [MDATA_PR_LINK_INDEX] = -1;
 pr->method[m].data [MDATA_PR_LINK_OTHER_METHOD] = -1;
 pr->method[m].data [MDATA_PR_LINK_RECEIVED] = 0;

}


/*
This function adds a proc which is not a member of a group to an existing group.

Assumes that:
 - pr1 is a member of a group
 - pr2 is not a member of a same group

returns -1 if fails
returns group number if successful
Can fail if:
 - pr1 lacks space in group_connection array.
 - new proc positions would result in collisions.
*/
static int add_proc_to_group(struct procstruct* pr1, struct procstruct* new_pr, int vertex_1, int vertex_2)
{

// fprintf(stdout, "\nnew_pr[%i]->x,y %f,%f ", new_pr->index, al_fixtof(new_pr->x), al_fixtof(new_pr->y));

 int g;

 g = pr1->group;
 struct groupstruct* gr = &w.group[g];

// first we test whether it is possible for new_pr to connect to pr1:
 if (gr->total_members + 1 >= GROUP_MAX_MEMBERS
  || pr1->number_of_group_connections == GROUP_CONNECTIONS)
 {
//  fprintf(stdout, "\nadd_proc_to_group: group members fail (total members: %i pr1->number_of_group_connections %i)", gr->total_members, pr1->number_of_group_connections);
  return -1;
 }

// now we need to test whether new_cl can join without colliding with any other proc:
// if (!test_group_position(new_cl, gr->x, gr->y, gr->angle))
  //return 0;

// excellent! now we can start connecting the procs

 int connection = find_first_empty_connection(pr1);

 pr1->group_connection [connection] = new_pr;
 pr1->connected_from [connection] = 0;
 pr1->connection_vertex [connection] = vertex_1;
// if (o1 != -1)
//  cl1->organ [o1].connection_to_other = connection;

// pr1->connection_angle [connection] = (radians_to_angle(atan2((s32b) new_cl->y - (s32b) cl1->y, (s32b) new_cl->x - (s32b) cl1->x)) - cl1->angle) & ANGLE_MASK;

// fprintf(stdout, " A %f,%f ", al_fixtof(new_pr->x), al_fixtof(new_pr->y));

 new_pr->group_connection [0] = pr1;
 new_pr->number_of_group_connections = 1;
 new_pr->connected_from [0] = connection;
 new_pr->connection_vertex [0] = vertex_2;
// if (o2 != -1)
//  new_cl->organ [o2].connection_to_other = 0;
// new_pr->connection_angle [0] = (radians_to_angle(atan2((s32b) cl1->y - (s32b) new_cl->y, (s32b) cl1->x - (s32b) new_cl->x)) - new_cl->angle) & ANGLE_MASK;
 new_pr->group_angle_offset = pr1->angle - gr->angle;

// if (new_cl->roots)
//  gr->roots = 1;

// fprintf(stdout, " B %f,%f ", al_fixtof(new_pr->x), al_fixtof(new_pr->y));

 pr1->number_of_group_connections ++;

 new_pr->group = g;
 gr->total_members++;

 new_pr->group_number_from_first = pr1->group_number_from_first + 1;
/*
 new_pr->x = pr1->x + fixed_xpart(pr1->connection_angle [connection], pr1->connection_dist [connection]);
 new_pr->y = pr1->y + fixed_ypart(pr1->connection_angle [connection], pr1->connection_dist [connection]);
 new_pr->angle = pr1->angle + pr1->connection_angle_difference [connection];*/

 new_pr->group_angle = get_angle(new_pr->y - gr->y, new_pr->x - gr->x);
 new_pr->group_distance = distance(new_pr->y - gr->y, new_pr->x - gr->x);

 fix_link_connections(pr1, new_pr, vertex_1, vertex_2, connection, 0);

// fprintf(stdout, " C %f,%f ", al_fixtof(new_pr->x), al_fixtof(new_pr->y));

// set_new_group_member_location(new_pr, pr1, connection); // sets new_pr's x/y/angle

// fprintf(stdout, " D %f,%f ", al_fixtof(new_pr->x), al_fixtof(new_pr->y));


 fix_group(gr->first_member, g, gr, 0);

// fprintf(stdout, " E %f,%f ", al_fixtof(new_pr->x), al_fixtof(new_pr->y));


 fix_link_methods(pr1, new_pr, vertex_1, vertex_2, connection, 0);

// fprintf(stdout, " F %f,%f ", al_fixtof(new_pr->x), al_fixtof(new_pr->y));


// new_pr->group_angle = atan2(new_pr->y - gr->y, new_pr->x - gr->x);
// new_pr->group_distance = distance(new_pr->y - gr->y, new_pr->x - gr->x);

// finally, fix irpt:
 gr->shared_irpt += *(new_pr->irpt); // is almost certainly 0
 gr->shared_irpt_max += new_pr->single_proc_irpt_max;
 gr->shared_data += *(new_pr->data); // is almost certainly 0
 gr->shared_data_max += new_pr->single_proc_data_max;
 new_pr->irpt = &gr->shared_irpt;
 new_pr->irpt_max = &gr->shared_irpt_max;
 new_pr->data = &gr->shared_data;
 new_pr->data_max = &gr->shared_data_max;

 return g;

}

// This function assumes that there is an empty connection to find
static int find_first_empty_connection(struct procstruct* pr)
{

 int i;

 for (i = 0; i < GROUP_CONNECTIONS; i ++)
 {
  if (pr->group_connection [i] == NULL)
   return i;
 }

#ifdef SANITY_CHECK
  fprintf(stdout, "g_group.c: find_first_empty_connection(): failed to find empty connection");
  error_call();
#endif

 return 0;

}



/*
Wrapper around fix_group2 and set_group_positions recursive functions.
Assumes that pr is first member of group. (or does it? not sure)
*/
static void fix_group(struct procstruct* pr, int g, struct groupstruct* gr, int distance_from_first_member)
{

 gr->total_members = 0;
 gr->total_mass = 0;
 gr->moment = 0; // will be calculated in set_group_positions
// gr->roots = 0;
// gr->angle_angle = radians_to_angle(gr->angle);
 gr->test_x = al_itofix(0); // used to calculate centre of mass of group
 gr->test_y = al_itofix(0);
 gr->centre_of_mass_test_x = 0;
 gr->centre_of_mass_test_y = 0;

 gr->mobile = 1; // this will be set to 0 in check_group_mobile if any group member is immobile

 w.current_check ++;
 if (check_group_mobile(pr, gr) == 0)
 {
// if the group is immobile, we don't need to update positions.
  w.current_check ++;
  fix_immobile_group2(pr, g, gr, distance_from_first_member);
 }
  else
  {
   w.current_check ++;
   fix_group2(pr, NULL, 0, g, gr, distance_from_first_member);
  }

 gr->centre_of_mass_test_x /= gr->total_mass;
 gr->x = al_itofix(gr->centre_of_mass_test_x);
 gr->centre_of_mass_test_y /= gr->total_mass;
 gr->y = al_itofix(gr->centre_of_mass_test_y);

// TO DO: this can actually get fairly close to MAXINT with a large enough group - but rounding it too much introduces inaccuracies that affect the centre of mass.
//  (which needs to be pretty precise so that e.g. torque generated by MOVE methods on opposite sides of group balances out)
// Think about rounding it bit by bit
//  e.g. could divide the test values by 4 or something when adding them up, then << (GRAIN - 2)

 w.current_check ++;
 set_group_positions(pr, gr);

// fprintf(stdout, "\ntest (%i, %i)", gr->test_x, gr->test_y);

}

/*
recursive function that updates some basic group values.

only call this from fix_group!
*/
static void fix_group2(struct procstruct* pr, struct procstruct* upstream_pr, int connection_index, int g, struct groupstruct* gr, int distance_from_first_member)
{
 int i;

 pr->group = g;
 gr->total_members ++;
 gr->total_mass += pr->mass;

 if (upstream_pr != NULL)
 {
/*
// this exists somewhere else!!
#define NEW_DISTANCE 4

  struct shapestruct* parent_shape = upstream_pr->shape_str;
  struct shapestruct* child_shape = pr->shape_str;
  int parent_vertex = upstream_pr->connection_vertex [connection_index];
  int child_vertex = pr->connection_vertex [upstream_pr->connected_from [connection_index]];

  al_fixed parent_vertex_x = upstream_pr->x + fixed_xpart(upstream_pr->angle + parent_shape->vertex_angle [parent_vertex], (parent_shape->vertex_dist [parent_vertex] + al_itofix(NEW_DISTANCE)));
  al_fixed parent_vertex_y = upstream_pr->y + fixed_ypart(upstream_pr->angle + parent_shape->vertex_angle [parent_vertex], (parent_shape->vertex_dist [parent_vertex] + al_itofix(NEW_DISTANCE)));

  al_fixed new_angle = upstream_pr->angle + upstream_pr->angle + parent_shape->vertex_angle [parent_vertex] + (AFX_ANGLE_2 - child_shape->vertex_angle [child_vertex]);
  fix_fixed_angle(&new_angle);

  pr->x = parent_vertex_x - fixed_xpart(new_angle + child_shape->vertex_angle [child_vertex], child_shape->vertex_dist [child_vertex]);
  pr->y = parent_vertex_y - fixed_ypart(new_angle + child_shape->vertex_angle [child_vertex], child_shape->vertex_dist [child_vertex]);*/

  pr->x = upstream_pr->x + fixed_xpart(upstream_pr->angle + upstream_pr->connection_angle [connection_index], upstream_pr->connection_dist [connection_index]);
  pr->y = upstream_pr->y + fixed_ypart(upstream_pr->angle + upstream_pr->connection_angle [connection_index], upstream_pr->connection_dist [connection_index]);
  pr->angle = upstream_pr->angle + upstream_pr->connection_angle_difference [connection_index];
 }

// gr->test_x += pr->x * al_itofix(pr->mass);
// gr->test_y += pr->y * al_itofix(pr->mass);
 gr->centre_of_mass_test_x += al_fixtoi(pr->x) * pr->mass;
 gr->centre_of_mass_test_y += al_fixtoi(pr->y) * pr->mass;

 pr->group_number_from_first = distance_from_first_member;
 pr->group_angle_offset = pr->angle - gr->angle;
 pr->group_check = w.current_check;

 gr->mobile &= pr->mobile; // if any group member is immobile, whole group is (gr->mobile is set to 1 in fix_group())

 for (i = 0; i < GROUP_CONNECTIONS; i ++)
 {
  if (pr->group_connection [i] != NULL
   && pr->group_connection [i]->group_check != w.current_check)
   fix_group2(pr->group_connection [i], pr, i, g, gr, distance_from_first_member + 1);
 }

}


/*
recursive function that updates some basic group values for immobile groups.

only call this from fix_group!
*/
static void fix_immobile_group2(struct procstruct* pr, int g, struct groupstruct* gr, int distance_from_first_member)
{

 int i;

 pr->group = g;
 gr->total_members ++;
 gr->total_mass += pr->mass;


// Some values that for mobile procs are changed each tick can just be set here and left:
 pr->test_x = pr->x;
 pr->test_y = pr->y;
 pr->test_angle = pr->angle;//get_fixed_fixed_angle(pr->group_angle_offset + gr->angle);
 pr->provisional_x = pr->test_x;
 pr->provisional_y = pr->test_y;
 pr->provisional_angle = pr->test_angle;
 pr->test_x_block = fixed_to_block(pr->test_x);
 pr->test_y_block = fixed_to_block(pr->test_y);

/*
 if (upstream_pr != NULL)
 {
  pr->x = upstream_pr->x + fixed_xpart(upstream_pr->connection_angle [connection_index], upstream_pr->connection_dist [connection_index]);
  pr->y = upstream_pr->y + fixed_ypart(upstream_pr->connection_angle [connection_index], upstream_pr->connection_dist [connection_index]);
  pr->angle = upstream_pr->angle + upstream_pr->connection_angle_difference [connection_index];
 }*/

// gr->test_x += pr->x * al_itofix(pr->mass);
// gr->test_y += pr->y * al_itofix(pr->mass);
 gr->centre_of_mass_test_x += al_fixtoi(pr->x) * pr->mass;
 gr->centre_of_mass_test_y += al_fixtoi(pr->y) * pr->mass;

 pr->group_number_from_first = distance_from_first_member;
 pr->group_angle_offset = pr->angle - gr->angle;
 pr->group_check = w.current_check;

// gr->mobile &= pr->mobile; // if any group member is immobile, whole group is (gr->mobile is set to 1 in fix_group())

 for (i = 0; i < GROUP_CONNECTIONS; i ++)
 {
  if (pr->group_connection [i] != NULL
   && pr->group_connection [i]->group_check != w.current_check)
   fix_immobile_group2(pr->group_connection [i], g, gr, distance_from_first_member + 1);
 }

}




/*
This function sets a new group member's location.
Assumes pr is not the first member of the group, and therefore that upstream_pr is not NULL.
Is probably only useful for new members of immobile groups, which will not have their locations updated each tick or each time fix_group is called and so need to have them set up when they join the group.
* /
static void set_new_group_member_location(struct procstruct* pr, struct procstruct* upstream_pr, int connection_index)
{
return;
  pr->x = upstream_pr->x + fixed_xpart(upstream_pr->angle + upstream_pr->connection_angle [connection_index], upstream_pr->connection_dist [connection_index]);
  pr->y = upstream_pr->y + fixed_ypart(upstream_pr->angle + upstream_pr->connection_angle [connection_index], upstream_pr->connection_dist [connection_index]);
  pr->angle = upstream_pr->angle + upstream_pr->connection_angle_difference [connection_index];

}
*/



/*
static void fix_group2(struct procstruct* pr, int g, struct groupstruct* gr, int distance_from_first_member)
{
 int i;

 pr->group = g;
 gr->total_members ++;
 gr->total_mass += pr->mass;

// gr->test_x += pr->x * al_itofix(pr->mass);
// gr->test_y += pr->y * al_itofix(pr->mass);
 gr->centre_of_mass_test_x += al_fixtoi(pr->x) * pr->mass;
 gr->centre_of_mass_test_y += al_fixtoi(pr->y) * pr->mass;

 pr->group_number_from_first = distance_from_first_member;
 pr->group_angle_offset = pr->angle - gr->angle;
 pr->group_check = w.current_check;

 gr->mobile &= pr->mobile; // if any group member is immobile, whole group is (gr->mobile is set to 1 in fix_group())

 for (i = 0; i < GROUP_CONNECTIONS; i ++)
 {
  if (pr->group_connection [i] != NULL
   && pr->group_connection [i]->group_check != w.current_check)
   fix_group2(pr->group_connection [i], g, gr, distance_from_first_member + 1);
 }

}*/


/*
recursive function that updates positions of group members, based on the group centre calculated by fix_group and fix_group2.

It also calculates moment of inertia for the group.

only call this from fix_group, and must be after call to fix_group2!
*/
static void set_group_positions(struct procstruct* pr, struct groupstruct* gr)
{
 int i;

 pr->group_angle = get_angle(pr->y - gr->y, pr->x - gr->x) - gr->angle;
 pr->group_distance = distance(pr->y - gr->y, pr->x - gr->x);

 pr->group_check = w.current_check;

// gr->moment += pr->mass * (pr->group_distance >> 12) * (pr->group_distance >> 12);
 gr->moment += (pr->mass * al_fixtoi(al_fixmul((pr->group_distance), (pr->group_distance))) / 800); // 800 is an arbitrary factor

 for (i = 0; i < GROUP_CONNECTIONS; i ++)
 {
  if (pr->group_connection [i] != NULL
   && pr->group_connection [i]->group_check != w.current_check)
   set_group_positions(pr->group_connection [i], gr);

 }

}

// recursive function that sets gr->mobile to 0 if any proc is immobile.
// set gr->mobile to 1 before calling
static int check_group_mobile(struct procstruct* pr, struct groupstruct* gr)
{

 if (pr->mobile == 0)
 {
  gr->mobile = 0;
  return 0;
 }

 pr->group_check = w.current_check;

 int i;

 for (i = 0; i < GROUP_CONNECTIONS; i ++)
 {
  if (pr->group_connection [i] != NULL
   && pr->group_connection [i]->group_check != w.current_check)
  {
   if (!check_group_mobile(pr->group_connection [i], gr))
    return 0;
  }
 }

 return 1;

}

/*

Group removal functions!

*/

/*

This function severs the connection between any two procs.

It is a wrapper function around other functions that are called depending on whether each proc will remain a member of a group,
 so it's the best function to call to disconnect procs from each other

Assumes: both procs are in same group and are connected to each other.


*/
int disconnect_procs(struct procstruct* pr1, struct procstruct* pr2)
{


#ifdef SANITY_CHECK
 if (pr1->group != pr2->group)
 {
  fprintf(stdout, "g_group.c: disconnect_procs: procs %i and %i not in same group: %i, %i", pr1->index, pr2->index, pr1->group, pr2->group);
  error_call();
 }
#endif

// Note: prepare_shared_resource_distribution() must have been called before this!

// first clear data from both procs' link methods:
 int i;

 for (i = 0; i < METHODS; i ++)
 {
  if (pr1->method[i].type == MTYPE_PR_LINK
   && pr1->method[i].data [MDATA_PR_LINK_INDEX] == pr2->index)
  {
   clear_link_method(pr1, pr1->method[i].ex_vertex);
   break;
  }
 }

 for (i = 0; i < METHODS; i ++)
 {
  if (pr2->method[i].type == MTYPE_PR_LINK
   && pr2->method[i].data [MDATA_PR_LINK_INDEX] == pr1->index)
  {
   clear_link_method(pr2, pr2->method[i].ex_vertex);
   break;
  }
 }


 if (pr2->number_of_group_connections == 1)
 {
  remove_single_proc_from_group(pr1, pr2);
  return 1;
 }

 if (pr1->number_of_group_connections == 1)
 {
  remove_single_proc_from_group(pr2, pr1); // note reversed order
  return 1;
 }

 if (pr1->group_number_from_first < pr2->group_number_from_first)
 {
  split_group(pr1, pr2);
  return 1;
 }

 if (pr1->group_number_from_first > pr2->group_number_from_first) // should be no possibility of these values being equal
 {
  split_group(pr2, pr1); // note reversed order
  return 1;
 }


#ifdef SANITY_CHECK
 if (pr1->group != pr2->group)
 {
  fprintf(stdout, "g_group.c: disconnect_groups: all conditions failed? ");
  error_call();
 }
#endif

 return 0;

}

/*

This function extracts a single proc from a group. Is called directly e.g. if the proc dies.

Assumes: pr1 is a member of a group.

*/
int extract_destroyed_proc_from_group(struct procstruct* pr1)
{

#ifdef SANITY_CHECK
 if (pr1->group == -1)
 {
  fprintf(stdout, "\nError: g_group.c: extract_single_proc: proc %i not in group", pr1->index);
  error_call();
 }
#endif

// prepare to distribute resources among the surviving procs:
 prepare_shared_resource_distribution(pr1, 1, -1);

 int i;

 for (i = 0; i < GROUP_CONNECTIONS; i ++)
 {
  if (pr1->group_connection [i] != NULL)
   disconnect_procs(pr1, pr1->group_connection [i]);
 }

// finish the resource distribution:
 fix_resource_pools();

 if (pr1->group == -1)
  return 1;

 return 0;

}


/*
Call this function just before removed_pr is removed from a group.
It will set up the resource_distrib structure with details of where the
group's irpt and data should go after the proc is removed.

If destroyed==1, this function assumes that removed_pr will be destroyed
and so should not get any of the resources. connection_severed is ignored.

If destroyed==0, connection_severed is the broken connection (as an index in group_connections). Can't be -1.

Don't call for non-group-members.
*/

void prepare_shared_resource_distribution(struct procstruct* removed_pr, int destroyed, int connection_severed)
{
 int rdist_connection_index = 0;
 int proc_connection_index;
 int i;

 rdist.connected_proc_index [0] = -1;

 rdist.irpt_pool = *(removed_pr->irpt);
 rdist.rdist_taking_all_irpt = -1;
 rdist.data_pool = *(removed_pr->data);
 rdist.rdist_taking_all_data = -1;

 struct procstruct* other_pr;

	if (!destroyed)
	{
	 other_pr = removed_pr->group_connection[connection_severed];
  rdist.connected_proc_index [0] = removed_pr->index;
  rdist.connected_proc_index [1] = other_pr->index;
		rdist.connected_proc_index [2] = -1;
// If the proc is not being destroyed, this is relatively straightforward - only one connection can be severed at once,
// The default is that the proc that called for the disconnection gives all irpt/data to the proc being left
//  - exception: if the proc being left has set its give method to removed_pr:
  if (other_pr->give_irpt_link_method != -1
		 && other_pr->method[other_pr->give_irpt_link_method].data [MDATA_PR_LINK_INDEX] == removed_pr->index)
		{
// other_proc wants to give all irpt to removed_proc (which is rdist index 0):
   rdist.connected_proc_irpt_status [0] = RESOURCE_DISTRIB_STATUS_TAKE_ALL;
			rdist.rdist_taking_all_irpt = 0;
			rdist.connected_proc_irpt_status [1] = RESOURCE_DISTRIB_STATUS_TAKE_NONE;
 	}
	  else
		 {
// default: removed_pr gives all irpt to other_pr:
    rdist.connected_proc_irpt_status [0] = RESOURCE_DISTRIB_STATUS_TAKE_NONE;
			 rdist.connected_proc_irpt_status [1] = RESOURCE_DISTRIB_STATUS_TAKE_ALL;
			 rdist.rdist_taking_all_irpt = 1;
		 }
// now treat data the same way:
  if (other_pr->give_data_link_method != -1
		 && other_pr->method[other_pr->give_data_link_method].data [MDATA_PR_LINK_INDEX] == removed_pr->index)
		{
// other_proc wants to give all data to removed_proc (which is rdist index 0):
   rdist.connected_proc_data_status [0] = RESOURCE_DISTRIB_STATUS_TAKE_ALL;
			rdist.rdist_taking_all_data = 0;
			rdist.connected_proc_data_status [1] = RESOURCE_DISTRIB_STATUS_TAKE_NONE;
//			fprintf(stdout, "\nremoved_proc %i (give %i) takes all (other_proc %i (give %i) takes none)", removed_pr->index, removed_pr->give_data_link_method, other_pr->index, other_pr->give_data_link_method);
 	}
	  else
		 {
// default: removed_pr gives all data to other_pr:
    rdist.connected_proc_data_status [0] = RESOURCE_DISTRIB_STATUS_TAKE_NONE;
			 rdist.connected_proc_data_status [1] = RESOURCE_DISTRIB_STATUS_TAKE_ALL;
			 rdist.rdist_taking_all_data = 1;
// 			fprintf(stdout, "\nremoved_proc %i (give %i) gives all to other_proc %i (give %i)", removed_pr->index, removed_pr->give_data_link_method, other_pr->index, other_pr->give_data_link_method);
		 }
		return;
	}

// If the distribution is occurring because removed_pr is being destroyed, this can be more complicated.
// First check for the simple case: proc has only one connection:
 if (removed_pr->number_of_group_connections == 1)
	{
// We can't use connection_severed, so we have to search for the connection that was broken:
		proc_connection_index = find_next_connected_proc(removed_pr, 0);
#ifdef SANITY_CHECK
   if (proc_connection_index == -1)
			{
				fprintf(stdout, "\nError in g_group.c:prepare_shared_resource_distribution(): invalid connection index.");
				error_call();
			}
#endif
	 other_pr = removed_pr->group_connection[proc_connection_index];
		rdist.connected_proc_index [0] = other_pr->index;
		rdist.connected_proc_irpt_status [0] = RESOURCE_DISTRIB_STATUS_TAKE_ALL;
		rdist.connected_proc_data_status [0] = RESOURCE_DISTRIB_STATUS_TAKE_ALL;
		rdist.rdist_taking_all_irpt = 0;
		rdist.rdist_taking_all_data = 0;
		rdist.connected_proc_index [1] = -1;
		return;
	}
// Now need to check all connections.
// We build a list of connections in the same order as the order of removed_pr's LINK methods:
 for (i = 0; i < METHODS; i++)
	{
		if (removed_pr->method[i].type == MTYPE_END)
			break;
		if (removed_pr->method[i].type != MTYPE_PR_LINK
   || removed_pr->method[i].data [MDATA_PR_LINK_INDEX] == -1)
			continue;
	 other_pr = &w.proc[removed_pr->method[i].data [MDATA_PR_LINK_INDEX]];
	 rdist.connected_proc_index [rdist_connection_index] = removed_pr->method[i].data [MDATA_PR_LINK_INDEX];
// first do irpt:
	 if (removed_pr->give_irpt_link_method == i)
		{
			rdist.connected_proc_irpt_status [rdist_connection_index] = RESOURCE_DISTRIB_STATUS_TAKE_ALL;
			rdist.rdist_taking_all_irpt = rdist_connection_index;
		}
		 else
			{
				if (other_pr->give_irpt_link_method != -1
					&& other_pr->method[other_pr->give_irpt_link_method].data [MDATA_PR_LINK_INDEX] == removed_pr->index)
   			rdist.connected_proc_irpt_status [rdist_connection_index] = RESOURCE_DISTRIB_STATUS_TAKE_NONE;
   			 else
   			  rdist.connected_proc_irpt_status [rdist_connection_index] = RESOURCE_DISTRIB_STATUS_SHARE;
			}
// now do data:
	 if (removed_pr->give_data_link_method == i)
		{
			rdist.connected_proc_data_status [rdist_connection_index] = RESOURCE_DISTRIB_STATUS_TAKE_ALL;
			rdist.rdist_taking_all_data = rdist_connection_index;
		}
		 else
			{
				if (other_pr->give_data_link_method != -1
					&& other_pr->method[other_pr->give_data_link_method].data [MDATA_PR_LINK_INDEX] == removed_pr->index)
   			rdist.connected_proc_data_status [rdist_connection_index] = RESOURCE_DISTRIB_STATUS_TAKE_NONE;
   			 else
   			  rdist.connected_proc_data_status [rdist_connection_index] = RESOURCE_DISTRIB_STATUS_SHARE;
			}
 		rdist_connection_index++;
	}

// now may need to terminate rdist list:
 if (rdist_connection_index < GROUP_CONNECTIONS)
	 rdist.connected_proc_index [rdist_connection_index] = -1;

}

// returns next connection with something connected to it (starting from connection_index).
// safe to call if connection_index >= GROUP_CONNECTIONS (just returns -1)
// returns -1 if no more connections.
static int find_next_connected_proc(struct procstruct* pr, int connection_index)
{

	while(connection_index < GROUP_CONNECTIONS)
	{
		if (pr->group_connection [connection_index] != NULL)
			return connection_index;
	 connection_index ++;
	};

	return -1;

}

// This function uses the rdist struct set up by prepare_shared_resource_distribution to distribute
//  resources after a group separation.
// It assumes that all procs and groups resulting from the separation have been set up
//  (so that e.g. all pr->irpt pointers are correct)
void fix_resource_pools(void)
{

// First we need to set irpt/data to zero and fix irpt_max/data_max values for all procs to be affected:
	int i;

	for (i = 0; i < GROUP_CONNECTIONS; i ++)
	{
		if (rdist.connected_proc_index [i] == -1)
		 break;
		fix_resource_values(rdist.connected_proc_index [i]);
	}

// IRPT
 if (rdist.irpt_pool == 0)
	 goto finished_irpt_distribution;

// first, if there is a proc with the TAKE_ALL status, try to give it as much of the shared irpt as possible:
	if (rdist.rdist_taking_all_irpt != -1)
	{
		if (rdist.irpt_pool <= *(w.proc[rdist.connected_proc_index [rdist.rdist_taking_all_irpt]].irpt_max))
		{
// Easy - it can accept all irpt:
			*(w.proc[rdist.connected_proc_index [rdist.rdist_taking_all_irpt]].irpt) = rdist.irpt_pool;
			goto finished_irpt_distribution;
		}
		*(w.proc[rdist.connected_proc_index [rdist.rdist_taking_all_irpt]].irpt) = *(w.proc[rdist.connected_proc_index [rdist.rdist_taking_all_irpt]].irpt_max);
		rdist.irpt_pool -= *(w.proc[rdist.connected_proc_index [rdist.rdist_taking_all_irpt]].irpt);
		rdist.connected_proc_irpt_status [rdist.rdist_taking_all_irpt] = RESOURCE_DISTRIB_STATUS_NO_DISTRIB;
	}

// Now share the remainder among the procs with the SHARE status

	for (i = 0; i < GROUP_CONNECTIONS+1; i ++)
	{
		if (rdist.connected_proc_index [i] == -1)
		 break;
	 if (rdist.connected_proc_irpt_status [i] == RESOURCE_DISTRIB_STATUS_SHARE)
		{
		 if (rdist.irpt_pool <= *(w.proc[rdist.connected_proc_index [i]].irpt_max))
		 {
// Easy - it can accept all irpt:
			 *(w.proc[rdist.connected_proc_index [i]].irpt) = rdist.irpt_pool;
			 goto finished_irpt_distribution;
		 }
		 *(w.proc[rdist.connected_proc_index [i]].irpt) = *(w.proc[rdist.connected_proc_index [i]].irpt_max);
		 rdist.irpt_pool -= *(w.proc[rdist.connected_proc_index [i]].irpt);
		 rdist.connected_proc_irpt_status [i] = RESOURCE_DISTRIB_STATUS_NO_DISTRIB;
		}
	}

 if (rdist.irpt_pool == 0)
	 goto finished_irpt_distribution;

// Last, share any remainder among the procs with the TAKE_NONE status

	for (i = 0; i < GROUP_CONNECTIONS+1; i ++)
	{
		if (rdist.connected_proc_index [i] == -1)
		 break;
	 if (rdist.connected_proc_irpt_status [i] == RESOURCE_DISTRIB_STATUS_TAKE_NONE)
		{
		 if (rdist.irpt_pool <= *(w.proc[rdist.connected_proc_index [i]].irpt_max))
		 {
// Easy - it can accept all irpt:
			 *(w.proc[rdist.connected_proc_index [i]].irpt) = rdist.irpt_pool;
			 goto finished_irpt_distribution;
		 }
		 *(w.proc[rdist.connected_proc_index [i]].irpt) = *(w.proc[rdist.connected_proc_index [i]].irpt_max);
		 rdist.irpt_pool -= *(w.proc[rdist.connected_proc_index [i]].irpt);
		 rdist.connected_proc_irpt_status [i] = RESOURCE_DISTRIB_STATUS_NO_DISTRIB;
		}
	}


// DATA
finished_irpt_distribution:

 if (rdist.data_pool == 0)
	 return;

// first, if there is a proc with the TAKE_ALL status, try to give it as much of the shared data as possible:
	if (rdist.rdist_taking_all_data != -1)
	{
		if (rdist.data_pool <= *(w.proc[rdist.connected_proc_index [rdist.rdist_taking_all_data]].data_max))
		{
// Easy - it can accept all data:
			*(w.proc[rdist.connected_proc_index [rdist.rdist_taking_all_data]].data) = rdist.data_pool;
			return;
		}
		*(w.proc[rdist.connected_proc_index [rdist.rdist_taking_all_data]].data) = *(w.proc[rdist.connected_proc_index [rdist.rdist_taking_all_data]].data_max);
		rdist.data_pool -= *(w.proc[rdist.connected_proc_index [rdist.rdist_taking_all_data]].data);
		rdist.connected_proc_data_status [rdist.rdist_taking_all_data] = RESOURCE_DISTRIB_STATUS_NO_DISTRIB;
	}

// Now share the remainder among the procs with the SHARE status

	for (i = 0; i < GROUP_CONNECTIONS+1; i ++)
	{
		if (rdist.connected_proc_index [i] == -1)
		 break;
	 if (rdist.connected_proc_data_status [i] == RESOURCE_DISTRIB_STATUS_SHARE)
		{
		 if (rdist.data_pool <= *(w.proc[rdist.connected_proc_index [i]].data_max))
		 {
// Easy - it can accept all data:
			 *(w.proc[rdist.connected_proc_index [i]].data) = rdist.data_pool;
			 return;
		 }
		 *(w.proc[rdist.connected_proc_index [i]].data) = *(w.proc[rdist.connected_proc_index [i]].data_max);
		 rdist.data_pool -= *(w.proc[rdist.connected_proc_index [i]].data);
		 rdist.connected_proc_data_status [i] = RESOURCE_DISTRIB_STATUS_NO_DISTRIB;
		}
	}

 if (rdist.data_pool == 0)
	 return;

// Last, share any remainder among the procs with the TAKE_NONE status

	for (i = 0; i < GROUP_CONNECTIONS+1; i ++)
	{
		if (rdist.connected_proc_index [i] == -1)
		 break;
	 if (rdist.connected_proc_data_status [i] == RESOURCE_DISTRIB_STATUS_TAKE_NONE)
		{
		 if (rdist.data_pool <= *(w.proc[rdist.connected_proc_index [i]].data_max))
		 {
// Easy - it can accept all data:
			 *(w.proc[rdist.connected_proc_index [i]].data) = rdist.data_pool;
			 return;
		 }
		 *(w.proc[rdist.connected_proc_index [i]].data) = *(w.proc[rdist.connected_proc_index [i]].data_max);
		 rdist.data_pool -= *(w.proc[rdist.connected_proc_index [i]].data);
		 rdist.connected_proc_data_status [i] = RESOURCE_DISTRIB_STATUS_NO_DISTRIB;
		}
	}


}

// this function sets irpt/data values for proc p, or its group, to 0 so they can be reset by fix_resource_pools
//  and if it is a group member also calculates irpt_max/data_max value for its group
static void	fix_resource_values(int p)
{
	if (w.proc[p].group == -1)
	{
		w.proc[p].single_proc_irpt = 0;
		w.proc[p].single_proc_data = 0;
		w.proc[p].irpt = &w.proc[p].single_proc_irpt;
		w.proc[p].irpt_max = &w.proc[p].single_proc_irpt_max;
		w.proc[p].data = &w.proc[p].single_proc_data;
		w.proc[p].data_max = &w.proc[p].single_proc_data_max;
		return;
	}

// now reset the shared group values:
 struct groupstruct* gr = &w.group[w.proc[p].group];

	gr->shared_irpt = 0;
	gr->shared_irpt_max = 0;
	gr->shared_data = 0;
	gr->shared_data_max = 0;

	w.current_check++;
// now call the recursive function:
	fix_resource_values2(&w.proc[p], gr);

}


static void	fix_resource_values2(struct procstruct* pr, struct groupstruct* gr)
{

	pr->group_check = w.current_check;

 gr->shared_irpt_max += pr->single_proc_irpt_max;
 gr->shared_data_max += pr->single_proc_data_max;

// now set pr's irpt/data pointers to gr's values (as gr may be a new group created by the separation)
	pr->irpt = &gr->shared_irpt;
	pr->irpt_max = &gr->shared_irpt_max;
	pr->data = &gr->shared_data;
	pr->data_max = &gr->shared_data_max;

 int i;

 for (i = 0; i < GROUP_CONNECTIONS; i ++)
 {
  if (pr->group_connection [i] != NULL
   && pr->group_connection [i]->group_check != w.current_check)
   fix_resource_values2(pr->group_connection [i], gr);
 }

}


/*
This function removes a single proc (pr2) from a group by breaking its connection with pr1

Assumes:
- pr1 and pr2 are part of the same group and are connected to each other
- pr2 is not connected to any other proc

Works whether or not pr1 is connected to any other proc (will destroy group if not)
Works even if pr2 is the first member of the group

*/
static void remove_single_proc_from_group(struct procstruct* pr1, struct procstruct* pr2)
{

 int g;

 g = pr1->group;
 struct groupstruct* gr = &w.group[g];

 gr->total_members --;

 pr2->group = -1;
 remove_group_connection(pr2, pr1);
 remove_group_connection(pr1, pr2);

 pr1->number_of_group_connections --;
 pr2->number_of_group_connections --;

 if (pr1->number_of_group_connections == 0)
 {
  pr1->group = -1;
  gr->exists = 0;
#ifdef SANITY_CHECK
 if (gr->total_members > 1)
 {
  fprintf(stdout, "g_group.c: remove_single_proc_from_group: group has > 1 members (gr->total_members %i pr1->connections %i pr1->index %i (%i, %i)", gr->total_members, pr1->number_of_group_connections, pr1->index, pr1->x, pr1->y);
  error_call();
 }
#endif
 }
  else
  {
// So the group has survived, but we need to deal with the possibility that pr2 was the first member of the group:
   if (gr->first_member == pr2)
   {
    gr->first_member = pr1;
    fix_group(pr1, g, gr, 0);
   }
    else
    {
     fix_group(gr->first_member, g, gr, 0);
    }
  }

// return 1;

}


/*
Scans pr1's group_connection array to find a link to connected, then removes it.
// Also, if the connection is based on an organ (i.e. connection_organ is not -1), sets a value for that organ to indicate connection no longer exists.
Doesn't deal with backwards link from connected - need to call this function again with parameters reversed.
Assumes link to connected exists.
*/
static void remove_group_connection(struct procstruct* pr, struct procstruct* connected)
{


 int i;

 for (i = 0; i < GROUP_CONNECTIONS; i ++)
 {
  if (pr->group_connection [i] == connected)
  {
   pr->group_connection [i] = NULL;
/*   if (pr->connection_organ [i] != -1)
   {
    cl->organ [cl->connection_organ [i]].connection_to_other = -1;
   }*/
   return;
  }
 }

#ifdef SANITY_CHECK
  fprintf(stdout, "g_group.c: remove_group_connection: failed to find connection");
  error_call();
#endif

}





/*
This function removes a proc (pr2) from a group by breaking its connection with pr1

Assumes:
- pr1 and pr2 are part of the same group and are connected to each other
- pr1 and pr2 each have other connections
- pr1 is closer to the first member than pr2 (i.e. has a lower group_number_from_first). pr1 can be the first member.

*/
static void split_group(struct procstruct* pr1, struct procstruct* pr2)
{

 int g1;

 g1 = pr1->group;
 struct groupstruct* gr1 = &w.group[g1];

 remove_group_connection(pr2, pr1);
 remove_group_connection(pr1, pr2);

 pr1->number_of_group_connections --;
 pr2->number_of_group_connections --;

// struct groupstruct* gr2 =   <-- not currently needed
 form_new_group(pr2);

 fix_group(gr1->first_member, g1, gr1, 0); // should be before pr2's connection to pr1 is established   <-- not sure what this comment means

// return 1;

}


/*
This function tries to form a new group starting with proc pr1

Assumes:
- pr1 has at least one group_connection
- if pr1 is being split from another proc, this has already happened and pr1's group connections have been packed


Fails (and returns 0) if:
- new group fails to be created
*/
static struct groupstruct* form_new_group(struct procstruct* pr1)
{

 int g;

 for (g = 0; g < w.max_groups; g ++)
 {
  if (w.group[g].exists == 0)
   break;
 }

#ifdef SANITY_CHECK
// max_groups should be such that this cannot happen (there should always be an available group)
 if (g == w.max_groups)
 {
  fprintf(stdout, "g_group.c: split_group(): failed to create new group");
  error_call();
 }
#endif

 struct groupstruct* gr = &w.group[g];

 gr->exists = 1;
 gr->first_member = pr1;

 gr->total_members = 0;
// gr->x = pr1->x; // P
// gr->y = pr1->y;
 gr->x_speed = pr1->x_speed; // TO DO: deal with inertia and momentum properly
 gr->y_speed = pr1->y_speed;
 gr->angle = al_itofix(0);
 gr->spin = al_itofix(0);
// gr->turning = 0;

 fix_group(pr1, g, gr, 0);

 return gr;

}






