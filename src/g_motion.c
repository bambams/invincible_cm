#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>


#include <stdio.h>
#include <math.h>

#include "m_config.h"
#include "m_globvars.h"

#include "g_header.h"
#include "g_shape.h"
#include "g_cloud.h"

#include "g_misc.h"
#include "m_maths.h"
#include "g_motion.h"

#define FORCE_DIST_DIVISOR 16
#define COLLISION_FORCE_DIVISOR 4

static void check_block_collision(struct procstruct* pr, struct blockstruct* bl, int notional_x, int notional_y);
static int check_notional_block_collision(struct blockstruct* bl, int notional_shape, int notional_size, al_fixed notional_x, al_fixed notional_y, al_fixed notional_angle);
//static void check_solid_block_collision(struct procstruct* pr, int dir_x, int dir_y, al_fixed notional_x, al_fixed notional_y);

//int check_proc_proc_collision(struct procstruct* pr1, struct procstruct* pr2, int pr2_x, int pr2_y, int pr2_angle);
//static int check_proc_proc_collision(struct procstruct* pr1, struct procstruct* pr2, al_fixed pr1_x, al_fixed pr1_y, al_fixed pr1_angle, al_fixed pr2_x, al_fixed pr2_y, al_fixed pr2_angle);

//static int check_proc_point_collision(struct procstruct* pr, al_fixed x, al_fixed y);


static int check_shape_shape_collision(struct shapestruct* sh1_dat, int sh2_shape, int sh2_size, al_fixed sh1_x, al_fixed sh1_y, al_fixed sh1_angle, al_fixed sh2_x, al_fixed sh2_y, al_fixed sh2_angle);

//int check_notional_block_collision_multi(int notional_shape, int notional_size, al_fixed notional_x, al_fixed notional_y, al_fixed notional_angle);
//int check_notional_solid_block_collision(int notional_shape, int notional_size, al_fixed notional_x, al_fixed notional_y, al_fixed notional_angle);

extern int collision_mask [SHAPES] [COLLISION_MASK_SIZE] [COLLISION_MASK_SIZE]; // in g_shape.c

struct shapestruct shape_dat [SHAPES] [SHAPES_SIZES]; // set up in g_shape.c

//static void apply_spin_to_proc_at_collision_vertex(struct procstruct* pr, int v, int force, al_fixed impulse_angle);
//static void apply_spin_to_proc(struct procstruct* pr, al_fixed x, al_fixed y, int force, al_fixed impulse_angle);
//void add_proc_to_blocklist(struct procstruct* pr);
void apply_impulse_to_proc_at_vector_point(struct procstruct* pr, al_fixed point_angle, al_fixed point_dist, al_fixed force, al_fixed impulse_angle);
void apply_impulse_to_proc_at_collision_vertex(struct procstruct* pr, int cv, al_fixed force, al_fixed impulse_angle);

// group movement functions:
//static void group_set_stationary_test_values(struct procstruct* pr);
//static void group_doesnt_move(struct procstruct* pr, struct groupstruct* gr);
static void move_group(struct procstruct* pr, struct groupstruct* gr);
static void move_group_members(struct procstruct* pr, struct groupstruct* gr);
//void apply_spin_to_group(struct groupstruct* gr, struct procstruct* pr, int force, int impulse_angle);
//static void apply_spin_to_group(struct groupstruct* gr, al_fixed collision_x, al_fixed collision_y, int force, al_fixed impulse_angle);

static void proc_group_collision(struct procstruct* single_pr, struct procstruct* group_pr, int collision_vertex);
static void group_proc_collision(struct procstruct* group_pr, struct procstruct* single_pr, int collision_vertex);
static void group_group_collision(struct procstruct* pr, struct procstruct* pr2, int collision_vertex);


static void check_group_collision(struct groupstruct* gr);
static int test_group_collision(struct procstruct* pr, struct groupstruct* gr);
//static void run_group_member_motion(struct procstruct* pr, struct groupstruct* gr);
//static void run_group_motion(struct procstruct* pr, struct groupstruct* gr);
static void check_block_collision_group_member(struct procstruct* pr, struct blockstruct* bl);

static void set_group_motion_prov_values(struct procstruct* pr, struct groupstruct* gr);
static void set_group_member_prov_values(struct procstruct* pr, struct procstruct* upstream_pr, int connection_index, struct groupstruct* gr);
//static void set_group_member_prov_values(struct procstruct* pr, struct groupstruct* gr);

static void fix_proc_speed(struct procstruct* pr);
static void fix_group_speed(struct groupstruct* gr);



void run_motion(void)
{


 struct procstruct* pr;
 int p;
 int notional_x_block;// = pr->test_x >> BLOCK_SIZE_BITSHIFT;
 int notional_y_block;// = pr->test_y >> BLOCK_SIZE_BITSHIFT;
 al_fixed notional_x;// = pr->test_x;
 al_fixed notional_y;// = pr->test_y;

 w.actual_procs = 0; // rough counter of proc numbers


 for (p = 0; p < w.max_procs; p ++)
 {

  if (w.proc [p].exists <= 0)
   continue;

  w.actual_procs ++;


  pr = &w.proc [p];

  if (pr->hit > 0)
   pr->hit--;

  pr->hit_edge_this_cycle = 0; // this is done for groups in set_group_motion_prov_values
//  pr->prov = 0;

  if (pr->group != -1)
  {
   if (w.group[pr->group].first_member == pr)
    set_group_motion_prov_values(pr, &w.group[pr->group]);
   continue;
  }

// group members don't get past this point

  pr->x_speed = al_fixmul(pr->x_speed, pr->drag);
  pr->y_speed = al_fixmul(pr->y_speed, pr->drag);

  if (!pr->mobile)
  {
   pr->x_speed = al_itofix(0);
   pr->y_speed = al_itofix(0);
   pr->spin = al_itofix(0);
   pr->spin_change = al_itofix(0);
  }

  pr->provisional_angle = pr->angle;
  pr->test_angle = pr->angle;

  pr->spin += pr->spin_change;
  pr->spin_change = al_itofix(0);

  if (pr->spin > SPIN_MAX_FIXED)
   pr->spin = SPIN_MAX_FIXED;
  if (pr->spin < NEGATIVE_SPIN_MAX_FIXED)
   pr->spin = NEGATIVE_SPIN_MAX_FIXED;

//  if (pr->spin != 0)
  {
   pr->test_angle += pr->spin;
   fix_fixed_angle(&pr->test_angle);
   pr->provisional_angle = pr->test_angle;
  }

  pr->spin = al_fixmul(pr->spin, SPIN_DRAG_BASE_FIXED);

//  pr->instant_spin = 0;

   pr->test_x = pr->x + pr->x_speed;
   pr->test_y = pr->y + pr->y_speed;
   pr->provisional_x = pr->test_x;
   pr->provisional_y = pr->test_y;
   pr->prov = 1; // provisional_angle has been set above

  if (pr->stuck > 5)
  {
   al_fixed stuck_displace = al_itofix(pr->stuck);
   if (stuck_displace > STUCK_DISPLACE_MAX)
    stuck_displace = STUCK_DISPLACE_MAX;
   al_fixed displace_angle = get_angle(pr->y - pr->stuck_against->y, pr->x - pr->stuck_against->x);
   displace_angle += al_fixmul(AFX_ANGLE_16, al_itofix(pr->stuck)); // give it a bit of sort-of randomness
   pr->test_x = pr->x + fixed_xpart(displace_angle, stuck_displace);
   pr->test_y = pr->y + fixed_ypart(displace_angle, stuck_displace);
// test for solid block - if test location would result in the proc being inside a solid block, cancel the test (try again later as displace_angle rotates)
   notional_x_block = fixed_to_block(pr->test_x);
   notional_y_block = fixed_to_block(pr->test_y);
#ifdef SANITY_CHECK
   if (notional_x_block < 0
    || notional_x_block >= w.w_block
    || notional_y_block < 0
    || notional_y_block >= w.h_block)
   {
    fprintf(stdout, "\n g_motion.c: run_motion(): stuck proc at %i,%i test location outside world (blocks %i, %i). stuck %i", pr->test_x, pr->test_y, notional_x_block, notional_y_block, pr->stuck);
    error_call();
   }
#endif
   if (w.block [notional_x_block] [notional_y_block].block_type == BLOCK_SOLID)
   {
    pr->test_x = pr->x;
    pr->test_y = pr->y;
   }
   pr->provisional_x = pr->test_x;
   pr->provisional_y = pr->test_y;

  }

 }


// Second loop: check for any collisions that would occur if all procs moved freely:
// s32b notional_x;
// s32b notional_y;
// int turn;

 for (p = 0; p < w.max_procs; p ++)
 {
  if (w.proc [p].exists <= 0)
   continue;

  pr = &w.proc [p];

#ifdef SANITY_CHECK
  if (pr->x > w.w_fixed - al_itofix(65)
      || pr->y > w.h_fixed - al_itofix(65)
      || pr->x < al_itofix(65)
      || pr->y < al_itofix(65))
//      exit(cl->x);
{
      fprintf(stdout, "\n\rError: proc out of bounds p %i xy(%i,%i) wxy(%i,%i) xs %i ys %i (world_time %i)", p, al_fixtoi(pr->x), al_fixtoi(pr->y), w.w_pixels, w.h_pixels, al_fixtoi(pr->x_speed), al_fixtoi(pr->y_speed), w.world_time);
      error_call();
}
#endif

  if (pr->group != -1)
  {
// only the first member of a group calls run_pass1_group; other members just continue the loop.
   if (pr == w.group[pr->group].first_member)
    check_group_collision(&w.group[pr->group]);
//    run_pass1_group(cl, &w.group [cl->group]);
   continue;
  }

// group members don't get past this point

// Should probably optimise this somehow...
// note that immobile procs still need these collision checks to check whether the immobile proc's vertices collide with another proc
  check_block_collision(pr, &w.block [pr->x_block] [pr->y_block], pr->test_x, pr->test_y);

  check_block_collision(pr, &w.block [pr->x_block] [pr->y_block - 1], pr->test_x, pr->test_y);
  check_block_collision(pr, &w.block [pr->x_block - 1] [pr->y_block - 1], pr->test_x, pr->test_y);
  check_block_collision(pr, &w.block [pr->x_block + 1] [pr->y_block - 1], pr->test_x, pr->test_y);
//  check_block_collision(cl, &w.block [pr->x_block] [pr->y_block], notional_x, notional_y);
  check_block_collision(pr, &w.block [pr->x_block - 1] [pr->y_block], pr->test_x, pr->test_y);
  check_block_collision(pr, &w.block [pr->x_block + 1] [pr->y_block], pr->test_x, pr->test_y);
  check_block_collision(pr, &w.block [pr->x_block] [pr->y_block + 1], pr->test_x, pr->test_y);
  check_block_collision(pr, &w.block [pr->x_block - 1] [pr->y_block + 1], pr->test_x, pr->test_y);
  check_block_collision(pr, &w.block [pr->x_block + 1] [pr->y_block + 1], pr->test_x, pr->test_y);

// note that if cl is a member of a group, will not get up to here.

// now check for collisions against adjacent solid blocks (only need to check if the proc's current block is an edge type)
  notional_x_block = fixed_to_block(pr->test_x);
  notional_y_block = fixed_to_block(pr->test_y);
  notional_x = pr->test_x;
  notional_y = pr->test_y;
  struct shapestruct* pr_shape = &shape_dat [pr->shape] [pr->size];

#define EDGE_ACCEL al_itofix(3)
#define EDGE_ACCEL_MAX al_itofix(4)
#define NEGATIVE_EDGE_ACCEL_MAX al_itofix(-4)

// TO DO: think about using a lookup table with left, right, up and down max_length for each shape for a range of rotations.

 if (w.block [notional_x_block] [notional_y_block].block_type != BLOCK_NORMAL)
 {
  switch(w.block [notional_x_block] [notional_y_block].block_type)
  {
   case BLOCK_SOLID:
#ifdef SANITY_CHECK
    fprintf(stdout, "\nError at time %i: g_motion.c: run_motion(): proc inside solid block.", w.world_time);
//    error_call();
#endif
    break;
   case BLOCK_EDGE_LEFT:
    if (fixed_to_block(notional_x - pr_shape->max_length) != notional_x_block)
    {
     pr->hit_edge_this_cycle = 1;
     pr->x_speed += EDGE_ACCEL;
     if (pr->x_speed > EDGE_ACCEL_MAX)
      pr->x_speed = EDGE_ACCEL_MAX;
    }
    break;
   case BLOCK_EDGE_RIGHT:
    if (fixed_to_block(notional_x + pr_shape->max_length) != notional_x_block)
    {
     pr->hit_edge_this_cycle = 1;
     pr->x_speed -= EDGE_ACCEL;
     if (pr->x_speed < NEGATIVE_EDGE_ACCEL_MAX)
      pr->x_speed = NEGATIVE_EDGE_ACCEL_MAX;
    }
    break;
   case BLOCK_EDGE_UP:
    if (fixed_to_block(notional_y - pr_shape->max_length) != notional_y_block)
    {
     pr->hit_edge_this_cycle = 1;
     pr->y_speed += EDGE_ACCEL;
     if (pr->y_speed > EDGE_ACCEL_MAX)
      pr->y_speed = EDGE_ACCEL_MAX;
    }
    break;
   case BLOCK_EDGE_DOWN:
    if (fixed_to_block(notional_y + pr_shape->max_length) != notional_y_block)
    {
     pr->hit_edge_this_cycle = 1;
     pr->y_speed -= EDGE_ACCEL;
     if (pr->y_speed < NEGATIVE_EDGE_ACCEL_MAX)
      pr->y_speed = NEGATIVE_EDGE_ACCEL_MAX;
    }
    break;
   case BLOCK_EDGE_UP_LEFT:
// up
    if (fixed_to_block(notional_y - pr_shape->max_length) != notional_y_block)
    {
     pr->hit_edge_this_cycle = 1;
     pr->y_speed += EDGE_ACCEL;
     if (pr->y_speed > EDGE_ACCEL_MAX)
      pr->y_speed = EDGE_ACCEL_MAX;
    }
// left
    if (fixed_to_block(notional_x - pr_shape->max_length) != notional_x_block)
    {
     pr->hit_edge_this_cycle = 1;
     pr->x_speed += EDGE_ACCEL;
     if (pr->x_speed > EDGE_ACCEL_MAX)
      pr->x_speed = EDGE_ACCEL_MAX;
    }
    break;
   case BLOCK_EDGE_UP_RIGHT:
// up
    if (fixed_to_block(notional_y - pr_shape->max_length) != notional_y_block)
    {
     pr->hit_edge_this_cycle = 1;
     pr->y_speed += EDGE_ACCEL;
     if (pr->y_speed > EDGE_ACCEL_MAX)
      pr->y_speed = EDGE_ACCEL_MAX;
    }
// right
    if (fixed_to_block(notional_x + pr_shape->max_length) != notional_x_block)
    {
     pr->hit_edge_this_cycle = 1;
     pr->x_speed -= EDGE_ACCEL;
     if (pr->x_speed < NEGATIVE_EDGE_ACCEL_MAX)
      pr->x_speed = NEGATIVE_EDGE_ACCEL_MAX;
    }
    break;
   case BLOCK_EDGE_DOWN_LEFT:
// down
    if (fixed_to_block(notional_y + pr_shape->max_length) != notional_y_block)
    {
     pr->hit_edge_this_cycle = 1;
     pr->y_speed -= EDGE_ACCEL;
     if (pr->y_speed < NEGATIVE_EDGE_ACCEL_MAX)
      pr->y_speed = NEGATIVE_EDGE_ACCEL_MAX;
    }
// left
    if (fixed_to_block(notional_x - pr_shape->max_length) != notional_x_block)
    {
     pr->hit_edge_this_cycle = 1;
     pr->x_speed += EDGE_ACCEL;
     if (pr->x_speed > EDGE_ACCEL_MAX)
      pr->x_speed = EDGE_ACCEL_MAX;
    }
    break;
   case BLOCK_EDGE_DOWN_RIGHT:
// down
    if (fixed_to_block(notional_y + pr_shape->max_length) != notional_y_block)
    {
     pr->hit_edge_this_cycle = 1;
     pr->y_speed -= EDGE_ACCEL;
     if (pr->y_speed < NEGATIVE_EDGE_ACCEL_MAX)
      pr->y_speed = NEGATIVE_EDGE_ACCEL_MAX;
    }
// right
    if (fixed_to_block(notional_x + pr_shape->max_length) != notional_x_block)
    {
     pr->hit_edge_this_cycle = 1;
     pr->x_speed -= EDGE_ACCEL;
     if (pr->x_speed < NEGATIVE_EDGE_ACCEL_MAX)
      pr->x_speed = NEGATIVE_EDGE_ACCEL_MAX;
    }
    break;
/*
  && ((notional_y + (pr_shape->max_length * dir_y)) >> BLOCK_SIZE_BITSHIFT) == notional_y_block)
   return; // no collision

    check_solid_block_collision(pr, -1, -1, pr->test_x, pr->test_y);
    check_solid_block_collision(pr, -1, 0, pr->test_x, pr->test_y);
    check_solid_block_collision(pr, -1, 1, pr->test_x, pr->test_y);
    break;
   case BLOCK_EDGE_RIGHT:
    check_solid_block_collision(pr, 1, -1, pr->test_x, pr->test_y);
    check_solid_block_collision(pr, 1, 0, pr->test_x, pr->test_y);
    check_solid_block_collision(pr, 1, 1, pr->test_x, pr->test_y);
    break;
   case BLOCK_EDGE_UP:
    check_solid_block_collision(pr, -1, -1, pr->test_x, pr->test_y);
    check_solid_block_collision(pr, 0, -1, pr->test_x, pr->test_y);
    check_solid_block_collision(pr, 1, -1, pr->test_x, pr->test_y);
    break;
   case BLOCK_EDGE_DOWN:
    check_solid_block_collision(pr, -1, 1, pr->test_x, pr->test_y);
    check_solid_block_collision(pr, 0, 1, pr->test_x, pr->test_y);
    check_solid_block_collision(pr, 1, 1, pr->test_x, pr->test_y);
    break;
   case BLOCK_EDGE_UP_LEFT:
// up
    check_solid_block_collision(pr, -1, -1, pr->test_x, pr->test_y);
    check_solid_block_collision(pr, 0, -1, pr->test_x, pr->test_y);
    check_solid_block_collision(pr, 1, -1, pr->test_x, pr->test_y);
// left
    check_solid_block_collision(pr, -1, 0, pr->test_x, pr->test_y);
    check_solid_block_collision(pr, -1, 1, pr->test_x, pr->test_y);
    break;
   case BLOCK_EDGE_UP_RIGHT:
// up
    check_solid_block_collision(pr, -1, -1, pr->test_x, pr->test_y);
    check_solid_block_collision(pr, 0, -1, pr->test_x, pr->test_y);
    check_solid_block_collision(pr, 1, -1, pr->test_x, pr->test_y);
// right
    check_solid_block_collision(pr, 1, 0, pr->test_x, pr->test_y);
    check_solid_block_collision(pr, 1, 1, pr->test_x, pr->test_y);
    break;
   case BLOCK_EDGE_DOWN_LEFT:
// down
    check_solid_block_collision(pr, -1, 1, pr->test_x, pr->test_y);
    check_solid_block_collision(pr, 0, 1, pr->test_x, pr->test_y);
    check_solid_block_collision(pr, 1, 1, pr->test_x, pr->test_y);
// left
    check_solid_block_collision(pr, -1, 0, pr->test_x, pr->test_y);
    check_solid_block_collision(pr, -1, -1, pr->test_x, pr->test_y);
    break;
   case BLOCK_EDGE_DOWN_RIGHT:
// down
    check_solid_block_collision(pr, -1, 1, pr->test_x, pr->test_y);
    check_solid_block_collision(pr, 0, 1, pr->test_x, pr->test_y);
    check_solid_block_collision(pr, 1, 1, pr->test_x, pr->test_y);
// right
    check_solid_block_collision(pr, 1, 0, pr->test_x, pr->test_y);
    check_solid_block_collision(pr, 1, -1, pr->test_x, pr->test_y);
    break;*/
  }
 }

//  if (!pr->collided_this_cycle)
//   pr->stuck = 0;

 }

// Third loop: let's move all of the procs that haven't collided:
 for (p = 0; p < w.max_procs; p ++)
 {
  pr = &w.proc [p];

  pr->group_check = 0;

  if (pr->exists <= 0)
   continue;

  if (pr->group != -1)
  {
    if (w.group[pr->group].first_member == pr
     && w.group[pr->group].hit_edge_this_cycle == 0)
     move_group(pr, &w.group[pr->group]);
    continue;
  }

// group members don't get past this point

  if (pr->hit_edge_this_cycle)
  {
   pr->hit_edge_this_cycle = 0;
   pr->float_angle = fixed_to_radians(pr->angle);
   continue;
  }

  if (!pr->mobile)
  {
   continue;
  }

  pr->old_x = pr->x;
  pr->old_y = pr->y;
  pr->old_angle = pr->angle;

  pr->x = pr->test_x;
  pr->y = pr->test_y;
  pr->angle = pr->test_angle;
  pr->float_angle = fixed_to_radians(pr->angle);

  pr->x_block = fixed_to_block(pr->x);
  pr->y_block = fixed_to_block(pr->y);

 }

 w.current_check = 0;

// Finally we update block lists based on all proc movement:

 w.blocktag ++;

 int blocktag = w.blocktag;

 for (p = 0; p < w.max_procs; p ++)
 {
  pr = &w.proc [p];

  if (pr->exists <= 0)
   continue;

  pr->x_block = fixed_to_block(pr->x);
  pr->y_block = fixed_to_block(pr->y);


// if (w.world_time > 26000)


// If the block's blocktag is old, this is the first proc being added to it this tick.
// So, we update the block's blocktag and put the proc on top with no link down.
// We can happily discard any pointers in the w.block struct; it doesn't matter if they are lost.
// We can also assume that x_block and y_block were set above.
  if (w.block [pr->x_block] [pr->y_block].tag != blocktag)
  {
   w.block [pr->x_block] [pr->y_block].tag = blocktag;
   pr->blocklist_down = NULL;
   pr->blocklist_up = NULL;
   w.block [pr->x_block] [pr->y_block].blocklist_down = pr;
  }
   else
   {
// The block's blocktag is up to date. So we put the new proc on top and set its downlink to the top proc of the block:
    pr->blocklist_down = w.block [pr->x_block] [pr->y_block].blocklist_down;
    pr->blocklist_up = NULL;
    w.block [pr->x_block] [pr->y_block].blocklist_down = pr;
// we also set the previous top proc's uplink to cl:
    if (pr->blocklist_down != NULL) // not sure that this is necessary.
     pr->blocklist_down->blocklist_up = pr;
   }

 }



}




/*
This function adds a proc to the blocklist of the correct block in world w.

It needs to be called any time a proc is added to a world, but only once for each proc (the block lists are subsequently maintained in run_pass1 above)

Assumes w.blocktag is correct (should be a safe assumption)
*/
void add_proc_to_blocklist(struct procstruct* pr)
{

// First we find which block the proc is in:
  pr->x_block = fixed_to_block(pr->x);
  pr->y_block = fixed_to_block(pr->y);

  if (w.block [pr->x_block] [pr->y_block].tag != w.blocktag)
  {
// The block's blocktag is old, so we just put the new proc on top.
   w.block [pr->x_block] [pr->y_block].tag = w.blocktag;
   pr->blocklist_down = NULL;
   pr->blocklist_up = NULL;
   w.block [pr->x_block] [pr->y_block].blocklist_down = pr;
  }
   else
   {
// The block's blocktag is up to date. So we put the new proc on top and set its downlink to the top proc of the block:
    pr->blocklist_down = w.block [pr->x_block] [pr->y_block].blocklist_down;
    pr->blocklist_up = NULL;
    w.block [pr->x_block] [pr->y_block].blocklist_down = pr;
// we also set the previous top proc's uplink to cl:
    if (pr->blocklist_down != NULL) // not sure that this is necessary.
     pr->blocklist_down->blocklist_up = pr;
   }

}

// Any changes to this function should be reflected in check_block_collision_group_member() below
// also see test_group_collision below
// returns 1 if collision, 0 if not
static void check_block_collision(struct procstruct* pr, struct blockstruct* bl, al_fixed notional_x, al_fixed notional_y)
{


 if (bl->tag != w.blocktag)
  return;

 struct procstruct* check_proc;
 check_proc = bl->blocklist_down;

 int collision_vertex;
 al_fixed collision_x, collision_y, vertex_speed_x, vertex_speed_y, impulse_angle, force;

 struct shapestruct* pr1_shape = &shape_dat [pr->shape] [pr->size];

 while(check_proc != NULL)
 {
  if (check_proc != pr // i.e. we don't check the proc against itself
   && check_proc->exists == 1) // could be deallocated (exists == -1)
  {

    if (check_proc->x + check_proc->max_length > pr->provisional_x - pr->max_length
     && check_proc->x - check_proc->max_length < pr->provisional_x + pr->max_length
     && check_proc->y + check_proc->max_length > pr->provisional_y - pr->max_length
     && check_proc->y - check_proc->max_length < pr->provisional_y + pr->max_length)
     {

//      if ((collision_vertex = check_proc_proc_collision(pr, check_proc, cp_x, cp_y, cp_angle)) != -1)
//      collision_vertex = check_proc_proc_collision(pr, check_proc, pr->provisional_x, pr->provisional_y, pr->provisional_angle, check_proc->provisional_x, check_proc->provisional_y, check_proc->provisional_angle);

//      check_shape_shape_collision(pr1_shape, check_proc->shape, check_proc->size, pr->x, pr->y, pr->angle, check_proc->x, check_proc->y, check_proc->angle);
      collision_vertex = check_shape_shape_collision(pr1_shape, check_proc->shape, check_proc->size, pr->provisional_x, pr->provisional_y, pr->provisional_angle, check_proc->provisional_x, check_proc->provisional_y, check_proc->provisional_angle);


      if (collision_vertex == -1)
      {
//       collision_vertex = check_proc_proc_collision(pr, check_proc, pr->x, pr->y, pr->angle, check_proc->provisional_x, check_proc->provisional_y, check_proc->provisional_angle);
       collision_vertex = check_shape_shape_collision(pr1_shape, check_proc->shape, check_proc->size, pr->x, pr->y, pr->angle, check_proc->provisional_x, check_proc->provisional_y, check_proc->provisional_angle);

       if (collision_vertex == -1)
       {
//        collision_vertex = check_proc_proc_collision(pr, check_proc, pr->provisional_x, pr->provisional_y, pr->provisional_angle, check_proc->x, check_proc->y, check_proc->angle);
        collision_vertex = check_shape_shape_collision(pr1_shape, check_proc->shape, check_proc->size, pr->provisional_x, pr->provisional_y, pr->provisional_angle, check_proc->x, check_proc->y, check_proc->angle);
       }
      }


/*
To do next: use check_shape_shape_collision to:
 - check collisions for procs added to world at start
 - check collisions for procs added to groups.

- also need to deal with angles for procs added to groups*/


//      if ((collision_vertex = check_proc_proc_collision(pr, check_proc, cp_x, cp_y, cp_angle)) != -1)
      if (collision_vertex != -1)
      {
       pr->hit = 3;
       check_proc->hit = 3;

      if (check_proc->group != -1)
      {
       proc_group_collision(pr, check_proc, collision_vertex);
      }
      else
      {

       collision_x = pr->provisional_x + fixed_xpart(pr->provisional_angle + pr->shape_str->collision_vertex_angle [collision_vertex], pr->shape_str->collision_vertex_dist [collision_vertex]);
       collision_y = pr->provisional_y + fixed_ypart(pr->provisional_angle + pr->shape_str->collision_vertex_angle [collision_vertex], pr->shape_str->collision_vertex_dist [collision_vertex]);
// calculate the velocity of the vertex of pr that hit check_proc:
       vertex_speed_x = collision_x
                      - (pr->x + fixed_xpart(pr->angle + pr->shape_str->collision_vertex_angle [collision_vertex], pr->shape_str->collision_vertex_dist [collision_vertex]));
       vertex_speed_y = collision_y
                      - (pr->y + fixed_ypart(pr->angle + pr->shape_str->collision_vertex_angle [collision_vertex], pr->shape_str->collision_vertex_dist [collision_vertex]));

       al_fixed struck_point_angle = get_angle(collision_y - check_proc->provisional_y, collision_x - check_proc->provisional_x);
       al_fixed struck_point_dist = distance(collision_y - check_proc->provisional_y, collision_x - check_proc->provisional_x);
       al_fixed struck_point_old_x = check_proc->x + fixed_xpart(struck_point_angle - check_proc->spin, struck_point_dist);
       al_fixed struck_point_old_y = check_proc->y + fixed_ypart(struck_point_angle - check_proc->spin, struck_point_dist);
       al_fixed struck_point_speed_x = collision_x - struck_point_old_x;
       al_fixed struck_point_speed_y = collision_y - struck_point_old_y;

       al_fixed collision_speed_x = vertex_speed_x - struck_point_speed_x;
       al_fixed collision_speed_y = vertex_speed_y - struck_point_speed_y;

       al_fixed collision_speed = distance(collision_speed_y, collision_speed_x);

       impulse_angle = get_angle(collision_y - check_proc->provisional_y, collision_x - check_proc->provisional_x);
       force = (collision_speed * check_proc->mass) / COLLISION_FORCE_DIVISOR;//al_itofix(100); //pr_force_share / FORCE_DIVISOR;
       apply_impulse_to_proc_at_collision_vertex(pr, collision_vertex, force, impulse_angle);
//       pr->collided_this_cycle = 1;

       impulse_angle = get_angle(collision_y - pr->provisional_y, collision_x - pr->provisional_x);
       force = (collision_speed * pr->mass) / COLLISION_FORCE_DIVISOR;//al_itofix(100); //check_proc_force_share / FORCE_DIVISOR;
       apply_impulse_to_proc_at_vector_point(check_proc, struck_point_angle, struck_point_dist, force, impulse_angle);
//       check_proc->collided_this_cycle = 1;

/*       pr->stuck ++;
       pr->stuck_against = check_proc;
       check_proc->stuck ++;
       check_proc->stuck_against = pr;*/

       fix_proc_speed(pr);
       fix_proc_speed(check_proc);

      } // end of "not a group member" code
/*
I think what I need to do is:
- work out the line perpendicular to the collision (like:   O|O  where O=proc, |=line)
- then work out the component of each proc's velocity that is perpendicular to that line
- then reflect that component away from the line
 - if one of the procs is moving away from the other (but more slowly, obviously), don't need to reflect
- each proc gets a proportion (deflection_ratio) of the velocity
- + an additional mutual repulsion
*/


      }

     }

//   dist = distance(notional_y - check_proc->y, notional_x - check_proc->x);
   // TO DO: put nearby_distance back
//   if (dist < pr->size + check_proc->size)
   {
//    proc_bounce(pr, check_proc);
   }
  }
  check_proc = check_proc->blocklist_down;
 };

}


// Any changes to this function should be reflected in check_block_collision() above
// also see test_group_collision below
// returns 1 if collision, 0 if not
static void check_block_collision_group_member(struct procstruct* pr, struct blockstruct* bl)
{

 if (bl->tag != w.blocktag)
  return;

 struct procstruct* check_proc;
 check_proc = bl->blocklist_down;
 //int dist;
 int collision_vertex;
// static int x_dist, y_dist, x_v_diff, y_v_diff;
// static float mass_ratio, dist_ratio, dvx2;
// static float deflection_ratio;

 struct shapestruct* pr1_shape = &shape_dat [pr->shape] [pr->size];

 while(check_proc != NULL)
 {
  if (check_proc != pr // i.e. we don't check the proc against itself
   && check_proc->group != pr->group // or against members of its own group
   && check_proc->exists == 1) // and proc is not deallocating
  {

    if (check_proc->x + check_proc->max_length > pr->provisional_x - pr->max_length
     && check_proc->x - check_proc->max_length < pr->provisional_x + pr->max_length
     && check_proc->y + check_proc->max_length > pr->provisional_y - pr->max_length
     && check_proc->y - check_proc->max_length < pr->provisional_y + pr->max_length)
     {
//      if ((collision_vertex = check_proc_proc_collision(pr, check_proc, cp_x, cp_y, cp_angle)) != -1)
//      collision_vertex = check_proc_proc_collision(pr, check_proc, pr->provisional_x, pr->provisional_y, pr->provisional_angle, check_proc->provisional_x, check_proc->provisional_y, check_proc->provisional_angle);

//      check_shape_shape_collision(pr1_shape, check_proc->shape, check_proc->size, pr->x, pr->y, pr->angle, check_proc->x, check_proc->y, check_proc->angle);
      collision_vertex = check_shape_shape_collision(pr1_shape, check_proc->shape, check_proc->size, pr->provisional_x, pr->provisional_y, pr->provisional_angle, check_proc->provisional_x, check_proc->provisional_y, check_proc->provisional_angle);


      if (collision_vertex == -1)
      {
//       collision_vertex = check_proc_proc_collision(pr, check_proc, pr->x, pr->y, pr->angle, check_proc->provisional_x, check_proc->provisional_y, check_proc->provisional_angle);
       collision_vertex = check_shape_shape_collision(pr1_shape, check_proc->shape, check_proc->size, pr->x, pr->y, pr->angle, check_proc->provisional_x, check_proc->provisional_y, check_proc->provisional_angle);

       if (collision_vertex == -1)
       {
//        collision_vertex = check_proc_proc_collision(pr, check_proc, pr->provisional_x, pr->provisional_y, pr->provisional_angle, check_proc->x, check_proc->y, check_proc->angle);
        collision_vertex = check_shape_shape_collision(pr1_shape, check_proc->shape, check_proc->size, pr->provisional_x, pr->provisional_y, pr->provisional_angle, check_proc->x, check_proc->y, check_proc->angle);
       }
// TO DO: optimise by not checking provisional values for immobile procs
      }




//      if ((collision_vertex = check_proc_proc_collision(pr, check_proc, cp_x, cp_y, cp_angle)) != -1)
      if (collision_vertex != -1)
      {
       pr->hit = 3;
       check_proc->hit = 3;

       if (check_proc->group != -1)
       {
        group_group_collision(pr, check_proc, collision_vertex);
       }
        else
        {
         group_proc_collision(pr, check_proc, collision_vertex);
        }

      }

     }

  }
  check_proc = check_proc->blocklist_down;
 };

}


// Just calls check_notional_block_collision for a 3x3 set of blocks
//  doesn't check for collision with solid blocks (use check_notional_solid_block_collision() for that; it doesn't need to be called 9 times)
// assumes notional_size, shape, x and y are valid
int check_notional_block_collision_multi(int notional_shape, int notional_size, al_fixed notional_x, al_fixed notional_y, al_fixed notional_angle)
{

 int notional_block_x = fixed_to_block(notional_x);
 int notional_block_y = fixed_to_block(notional_y);

  if (check_notional_block_collision(&w.block [notional_block_x] [notional_block_y], notional_shape, notional_size, notional_x, notional_y, notional_angle))
   return 1;

  if (check_notional_block_collision(&w.block [notional_block_x - 1] [notional_block_y - 1], notional_shape, notional_size, notional_x, notional_y, notional_angle))
   return 1;
  if (check_notional_block_collision(&w.block [notional_block_x - 1] [notional_block_y], notional_shape, notional_size, notional_x, notional_y, notional_angle))
   return 1;
  if (check_notional_block_collision(&w.block [notional_block_x - 1] [notional_block_y + 1], notional_shape, notional_size, notional_x, notional_y, notional_angle))
   return 1;

  if (check_notional_block_collision(&w.block [notional_block_x] [notional_block_y - 1], notional_shape, notional_size, notional_x, notional_y, notional_angle))
   return 1;
//  check_notional_block_collision(&w.block [notional_block_x] [notional_block_y], notional_shape, notional_size, notional_x, notional_y, notional_angle);
  if (check_notional_block_collision(&w.block [notional_block_x] [notional_block_y + 1], notional_shape, notional_size, notional_x, notional_y, notional_angle))
   return 1;

  if (check_notional_block_collision(&w.block [notional_block_x + 1] [notional_block_y - 1], notional_shape, notional_size, notional_x, notional_y, notional_angle))
   return 1;
  if (check_notional_block_collision(&w.block [notional_block_x + 1] [notional_block_y], notional_shape, notional_size, notional_x, notional_y, notional_angle))
   return 1;
  if (check_notional_block_collision(&w.block [notional_block_x + 1] [notional_block_y + 1], notional_shape, notional_size, notional_x, notional_y, notional_angle))
   return 1;

  return 0;

}

//  also see test_group_collision and collision_check_ignore_group below

/*
This function checks whether a non-existent proc with notional properties would collide with another proc if created
It will generally be called 9 times for complete coverage of a 3x3 area

Doesn't check for collision with a solid block (use check_notional_solid_block_collision for that)

returns 1 if collision, 0 if no collision
*/
static int check_notional_block_collision(struct blockstruct* bl, int notional_shape, int notional_size, al_fixed notional_x, al_fixed notional_y, al_fixed notional_angle)
{

 if (bl->tag != w.blocktag)
  return 0; // nothing currently in this block

 struct procstruct* check_proc;
 check_proc = bl->blocklist_down;

// static int collision_x, collision_y, force, impulse_angle, vertex_speed_x, vertex_speed_y, collision_vertex;

 struct shapestruct* notional_shape_dat = &shape_dat [notional_shape] [notional_size];

 while(check_proc != NULL)
 {
  //if (check_proc != pr) // i.e. we don't check the proc against itself   <---- not needed for notional check
  if (check_proc->exists == 1)
  {

    if (check_proc->x + check_proc->max_length > notional_x - notional_shape_dat->max_length
     && check_proc->x - check_proc->max_length < notional_x + notional_shape_dat->max_length
     && check_proc->y + check_proc->max_length > notional_y - notional_shape_dat->max_length
     && check_proc->y - check_proc->max_length < notional_y + notional_shape_dat->max_length)
     {

// This works differently to the usual check_block_collision function.
// It doesn't need to check provisional_x/y values, as the relevant procs will not move until after the next time provisional values are calculated
// However, it does need to call check_shape_shape_collision for both procs (so that both procs' vertices are taken into account)

// TO DO: This will NOT currently detect procs overlapping unless at least one vertex is overlapping, which may not be true.

//      collision_vertex = check_proc_proc_collision(pr, check_proc, pr->provisional_x, pr->provisional_y, pr->provisional_angle, check_proc->provisional_x, check_proc->provisional_y, check_proc->provisional_angle);
      if (check_shape_shape_collision(notional_shape_dat, check_proc->shape, check_proc->size, notional_x, notional_y, notional_angle, check_proc->x, check_proc->y, check_proc->angle) != -1)
       return 1;
// need to do the reverse as well:
      if (check_shape_shape_collision(&shape_dat [check_proc->shape] [check_proc->size], notional_shape, notional_size, check_proc->x, check_proc->y, check_proc->angle, notional_x, notional_y, notional_angle) != -1)
       return 1;

     }

  }
  check_proc = check_proc->blocklist_down;
 };

 return 0;

}

int check_notional_solid_block_collision(int notional_shape, int notional_size, al_fixed notional_x, al_fixed notional_y, al_fixed notional_angle)
{

  int notional_x_block = fixed_to_block(notional_x);
  int notional_y_block = fixed_to_block(notional_y);
  struct shapestruct* pr_shape = &shape_dat [notional_shape] [notional_size];

// TO DO: think about using a lookup table with left, right, up and down max_length for each shape for a range of rotations.

  switch(w.block [notional_x_block] [notional_y_block].block_type)
  {
   case BLOCK_NORMAL: break; // do nothing
   case BLOCK_SOLID:
#ifdef SANITY_CHECK
//    fprintf(stdout, "\nError: g_motion.c: check_notional_solid_block_collision(): proc inside solid block.");
//    error_call();
#endif
    return 1; // not an error - this function can be called when one proc is trying to create another and the new proc would be in a solid block.
   case BLOCK_EDGE_LEFT:
    if (fixed_to_block(notional_x - pr_shape->max_length) != notional_x_block)
    {
     return 1;
    }
    break;
   case BLOCK_EDGE_RIGHT:
    if (fixed_to_block(notional_x + pr_shape->max_length) != notional_x_block)
    {
     return 1;
    }
    break;
   case BLOCK_EDGE_UP:
    if (fixed_to_block(notional_y - pr_shape->max_length) != notional_y_block)
    {
     return 1;
    }
    break;
   case BLOCK_EDGE_DOWN:
    if (fixed_to_block(notional_y + pr_shape->max_length) != notional_y_block)
    {
     return 1;
    }
    break;
   case BLOCK_EDGE_UP_LEFT:
// up
    if (fixed_to_block(notional_y - pr_shape->max_length) != notional_y_block)
    {
     return 1;
    }
// left
    if (fixed_to_block(notional_x - pr_shape->max_length) != notional_x_block)
    {
     return 1;
    }
    break;
   case BLOCK_EDGE_UP_RIGHT:
// up
    if (fixed_to_block(notional_y - pr_shape->max_length) != notional_y_block)
    {
     return 1;
    }
// right
    if (fixed_to_block(notional_x + pr_shape->max_length) != notional_x_block)
    {
     return 1;
    }
    break;
   case BLOCK_EDGE_DOWN_LEFT:
// down
    if (fixed_to_block(notional_y + pr_shape->max_length) != notional_y_block)
    {
     return 1;
    }
// left
    if (fixed_to_block(notional_x - pr_shape->max_length) != notional_x_block)
    {
     return 1;
    }
    break;
   case BLOCK_EDGE_DOWN_RIGHT:
// down
    if (fixed_to_block(notional_y + pr_shape->max_length) != notional_y_block)
    {
     return 1;
    }
// right
    if (fixed_to_block(notional_x + pr_shape->max_length) != notional_x_block)
    {
     return 1;
    }
    break;

  }

  return 0; // no collision


}


/*

//void plot_point_on_mask(int s, int size, int p1x, int p1y, int p2x, int p2y);

// check to see if any of pr1's vertices overlay pr2
// TO DO: will probably need to check more than just vertices here (will probably need extra points to deal with e.g. particularly thin shapes).
// the pr2_x etc parameters are used because we might need to use either x/y or provisional_x/y
// returns -1 if no collision
// otherwise returns the vertex of pr1 that collided with pr2
static int check_proc_proc_collision(struct procstruct* pr1, struct procstruct* pr2, al_fixed pr1_x, al_fixed pr1_y, al_fixed pr1_angle, al_fixed pr2_x, al_fixed pr2_y, al_fixed pr2_angle)
{

// fprintf(stdout, "\ncollision %i vs %i", pr1->index, pr2->index);

 al_fixed dist = distance(pr1_y - pr2_y, pr1_x - pr2_x);

 al_fixed angle = get_angle(pr1_y - pr2_y, pr1_x - pr2_x);
 al_fixed angle_diff = angle - pr2_angle;

 int v, mask_x, mask_y;

 al_fixed pr1_centre_x = MASK_CENTRE_FIXED + fixed_xpart(angle_diff, dist);
 al_fixed pr1_centre_y = MASK_CENTRE_FIXED + fixed_ypart(angle_diff, dist);


 struct shapestruct* pr1_shape = &shape_dat [pr1->shape] [pr1->size];

 for (v = 0; v < pr1_shape->collision_vertices; v ++)
 {
  mask_x = al_fixtoi(pr1_centre_x + fixed_xpart(pr1_angle + pr1_shape->collision_vertex_angle [v] - pr2_angle, pr1_shape->collision_vertex_dist [v] + al_itofix(1)));
  mask_y = al_fixtoi(pr1_centre_y + fixed_ypart(pr1_angle + pr1_shape->collision_vertex_angle [v] - pr2_angle, pr1_shape->collision_vertex_dist [v] + al_itofix(1)));

//  fprintf(stdout, " mask %i,%i", mask_x, mask_y);

  if (mask_x >= 0
   && mask_y >= 0
   && mask_x < COLLISION_MASK_SIZE
   && mask_y < COLLISION_MASK_SIZE)
   {
    if (collision_mask [pr2->shape] [pr2->size] [mask_x] [mask_y])
    {
     pr1->hit = 3;
     pr2->hit = 3;
     return v;
    }
   }
 }

 return -1;

}
*/
/*

This function checks whether any of shape shape_dat_1's vertices collide with shape shape_dat_2
It doesn't require either shape to exist or not exist
Has no side effects, and can be used whether either shape is notional or belongs to an actual proc

returns vertex index of shape_1 if collision
returns -1 if no collision

*** Note: DOESN'T check whether any of shape_2's vertices collide with shape_1! Must call in reverse as well to do this.
*/
static int check_shape_shape_collision(struct shapestruct* sh1_dat, int sh2_shape, int sh2_size, al_fixed sh1_x, al_fixed sh1_y, al_fixed sh1_angle, al_fixed sh2_x, al_fixed sh2_y, al_fixed sh2_angle)
{


 al_fixed dist = distance(sh1_y - sh2_y, sh1_x - sh2_x);

 al_fixed angle = get_angle(sh1_y - sh2_y, sh1_x - sh2_x);
 al_fixed angle_diff = angle - sh2_angle;

 unsigned int v, mask_x, mask_y;

 al_fixed sh1_centre_x = MASK_CENTRE_FIXED + fixed_xpart(angle_diff, dist);
 al_fixed sh1_centre_y = MASK_CENTRE_FIXED + fixed_ypart(angle_diff, dist);

 for (v = 0; v < sh1_dat->collision_vertices; v ++)
 {

  mask_x = al_fixtoi(sh1_centre_x + fixed_xpart(sh1_angle + sh1_dat->collision_vertex_angle [v] - sh2_angle, sh1_dat->collision_vertex_dist [v] + al_itofix(1)));
  mask_x >>= COLLISION_MASK_BITSHIFT;
  mask_y = al_fixtoi(sh1_centre_y + fixed_ypart(sh1_angle + sh1_dat->collision_vertex_angle [v] - sh2_angle, sh1_dat->collision_vertex_dist [v] + al_itofix(1)));
  mask_y >>= COLLISION_MASK_BITSHIFT;

//  if (v == 0)
//   fprintf(stdout, "\nmask %i, %i  sh1_centre %f, %f  dist %f", mask_x, mask_y, al_fixtof(sh1_centre_x), al_fixtof(sh1_centre_y), al_fixtof(dist));


  if (mask_x < COLLISION_MASK_SIZE // don't check for < 0 because they're unsigned
   && mask_y < COLLISION_MASK_SIZE)
   {
//    if (pr1->index == 77 && pr2->index == 1)
//     plot_point_on_mask(pr2->shape, pr2->size, mask_x, mask_y, pr1_centre_x, pr1_centre_y);

//     fprintf(stdout, "\nTest: shape %i size %i mask %i,%i (centre %i,%i) result %i", sh2_shape, sh2_size, mask_x, mask_y, MASK_CENTRE>>COLLISION_MASK_BITSHIFT, MASK_CENTRE>>COLLISION_MASK_BITSHIFT, collision_mask [sh2_shape] [mask_x] [mask_y]);
 //test_draw_mask(sh2_shape, mask_x, mask_y);
// wait_for_space();
    if (collision_mask [sh2_shape] [mask_x] [mask_y] <= sh2_size)
    {
     return v;
    }
   }
 }

 return -1;

}





/*
static int check_proc_point_collision(struct procstruct* pr, al_fixed x, al_fixed y)
{


 al_fixed dist = distance(pr->y - y, pr->x - x);

 al_fixed angle = get_angle(y - pr->y, x - pr->x);
 al_fixed angle_diff = angle - pr->angle;
 int mask_x = al_fixtoi(MASK_CENTRE_FIXED + fixed_xpart(angle_diff, dist));
 int mask_y = al_fixtoi(MASK_CENTRE_FIXED + fixed_ypart(angle_diff, dist));

 if (mask_x >= 0
  && mask_y >= 0
  && mask_x < COLLISION_MASK_SIZE
  && mask_y < COLLISION_MASK_SIZE)
  {
   if (collision_mask [pr->shape] [pr->size] [mask_x] [mask_y])
    return 1;
  }

 return 0;



}
*/

#ifdef NOT_USED
extern ALLEGRO_DISPLAY* display;
extern ALLEGRO_COLOR base_col [BASIC_COLS] [BASIC_SHADES];

#define PLOT_VERTICES 1000
// debug function - no longer used
void plot_point_on_mask(int s, int size, int p1x, int p1y, int p2x, int p2y)
{

// al_set_target_bitmap(display_window);

// al_clear_to_color(base_col [0] [0]);

 ALLEGRO_VERTEX plot_pixel [PLOT_VERTICES];


 int x, y, shade, col2;
 int xa, ya;
 int i = 0;

 for (x = 0; x < COLLISION_MASK_SIZE; x ++)
 {
  for (y = 0; y < COLLISION_MASK_SIZE; y ++)
  {
    shade = SHADE_LOW;
    col2 = COL_GREEN;
    if (collision_mask [s] [size] [x] [y]) // totally wrong now
     shade = SHADE_HIGH;
    if (x == p1x && y == p1y)
    {
     col2 = COL_BLUE;
     shade = SHADE_HIGH;
    }
    if (x == p2x && y == p2y)
    {
     col2 = COL_BLUE;
     shade = SHADE_MAX;
    }
    xa = 500 + x;
    ya = 10 + y;
    if (shade != SHADE_LOW)
    {
     //al_draw_pixel(xa, ya, base_col [col2] [shade]);
     plot_pixel[i].x = xa;
     plot_pixel[i].y = ya;
     plot_pixel[i].z = 0;
     plot_pixel[i].color = colours.base [col2] [shade];
     i ++;
    }
//    al_draw_filled_rectangle(xa, ya, xa + 2, ya + 2, base_col [COL_GREEN] [shade]);
  if (i == PLOT_VERTICES)
  {
   al_draw_prim(plot_pixel, NULL, NULL, 0, PLOT_VERTICES, ALLEGRO_PRIM_POINT_LIST); // May need to put back "-1" after MAP_VERTICES
   i = 0;
  }


  }
 }


 if (i > 0)
 {
   al_draw_prim(plot_pixel, NULL, NULL, 0, i, ALLEGRO_PRIM_POINT_LIST);
 }

 if (settings.option [OPTION_SPECIAL_CURSOR])
  draw_mouse_cursor();
 al_flip_display();

// error_call();

}

#endif




/*
pr is a non-group member, one of whose vertices has collided with a group member (pr2).
(when a group member's vertex hits a non-group proc, use group_proc_collision)
pr2 is a group member.

This function does not handle the reverse situation (group member's vertex collides with non-group member). See group_proc_collision.

*/
static void proc_group_collision(struct procstruct* single_pr, struct procstruct* group_pr, int collision_vertex)
{

       struct groupstruct* gr2 = &w.group[group_pr->group];

       al_fixed collision_x = single_pr->provisional_x + fixed_xpart(single_pr->provisional_angle + single_pr->shape_str->collision_vertex_angle [collision_vertex], single_pr->shape_str->collision_vertex_dist [collision_vertex]);
       al_fixed collision_y = single_pr->provisional_y + fixed_ypart(single_pr->provisional_angle + single_pr->shape_str->collision_vertex_angle [collision_vertex], single_pr->shape_str->collision_vertex_dist [collision_vertex]);
// calculate the velocity of the vertex of pr that hit check_proc:
       al_fixed vertex_speed_x = collision_x
                      - (single_pr->x + fixed_xpart(single_pr->angle + single_pr->shape_str->collision_vertex_angle [collision_vertex], single_pr->shape_str->collision_vertex_dist [collision_vertex]));
       al_fixed vertex_speed_y = collision_y
                      - (single_pr->y + fixed_ypart(single_pr->angle + single_pr->shape_str->collision_vertex_angle [collision_vertex], single_pr->shape_str->collision_vertex_dist [collision_vertex]));

       al_fixed struck_point_angle = get_angle(collision_y - group_pr->provisional_y, collision_x - group_pr->provisional_x);
       al_fixed struck_point_dist = distance(collision_y - group_pr->provisional_y, collision_x - group_pr->provisional_x);
       al_fixed struck_point_old_x = group_pr->x + fixed_xpart(struck_point_angle - (group_pr->provisional_angle - group_pr->angle), struck_point_dist);
       al_fixed struck_point_old_y = group_pr->y + fixed_ypart(struck_point_angle - (group_pr->provisional_angle - group_pr->angle), struck_point_dist);
       al_fixed struck_point_speed_x = collision_x - struck_point_old_x;
       al_fixed struck_point_speed_y = collision_y - struck_point_old_y;

       al_fixed collision_speed_x = vertex_speed_x - struck_point_speed_x;
       al_fixed collision_speed_y = vertex_speed_y - struck_point_speed_y;

       al_fixed collision_speed = distance(collision_speed_y, collision_speed_x);

       al_fixed impulse_angle = get_angle(collision_y - group_pr->provisional_y, collision_x - group_pr->provisional_x);
       al_fixed force = (collision_speed * gr2->total_mass) / COLLISION_FORCE_DIVISOR;//al_itofix(100); //pr_force_share / FORCE_DIVISOR;
       apply_impulse_to_proc_at_collision_vertex(single_pr, collision_vertex, force, impulse_angle);
//       single_pr->collided_this_cycle = 1;

       impulse_angle = get_angle(collision_y - single_pr->provisional_y, collision_x - single_pr->provisional_x);
       force = (collision_speed * single_pr->mass) / COLLISION_FORCE_DIVISOR;//al_itofix(100); //check_proc_force_share / FORCE_DIVISOR;
       apply_impulse_to_group(gr2, struck_point_old_x, struck_point_old_y, force, impulse_angle);
//       group_pr->collided_this_cycle = 1;
//       gr2->collided_this_cycle = 1;

       fix_proc_speed(single_pr);
       fix_group_speed(gr2);




}




/*
Opposite of proc_group_collision - here, a group member's vertex collides with a non-group-member

*/
static void group_proc_collision(struct procstruct* group_pr, struct procstruct* single_pr, int collision_vertex)
{


       struct groupstruct* gr = &w.group[group_pr->group];

       al_fixed collision_x = group_pr->provisional_x + fixed_xpart(group_pr->provisional_angle + group_pr->shape_str->collision_vertex_angle [collision_vertex], group_pr->shape_str->collision_vertex_dist [collision_vertex]);
       al_fixed collision_y = group_pr->provisional_y + fixed_ypart(group_pr->provisional_angle + group_pr->shape_str->collision_vertex_angle [collision_vertex], group_pr->shape_str->collision_vertex_dist [collision_vertex]);
// calculate the velocity of the vertex of pr that hit check_proc:
       al_fixed vertex_speed_x = collision_x
                      - (group_pr->x + fixed_xpart(group_pr->angle + group_pr->shape_str->collision_vertex_angle [collision_vertex], group_pr->shape_str->collision_vertex_dist [collision_vertex]));
       al_fixed vertex_speed_y = collision_y
                      - (group_pr->y + fixed_ypart(group_pr->angle + group_pr->shape_str->collision_vertex_angle [collision_vertex], group_pr->shape_str->collision_vertex_dist [collision_vertex]));

       al_fixed struck_point_angle = get_angle(collision_y - single_pr->provisional_y, collision_x - single_pr->provisional_x);
       al_fixed struck_point_dist = distance(collision_y - single_pr->provisional_y, collision_x - single_pr->provisional_x);
       al_fixed struck_point_old_x = single_pr->x + fixed_xpart(struck_point_angle - (single_pr->provisional_angle - single_pr->angle), struck_point_dist);
       al_fixed struck_point_old_y = single_pr->y + fixed_ypart(struck_point_angle - (single_pr->provisional_angle - single_pr->angle), struck_point_dist);
       al_fixed struck_point_speed_x = collision_x - struck_point_old_x;
       al_fixed struck_point_speed_y = collision_y - struck_point_old_y;

       al_fixed collision_speed_x = vertex_speed_x - struck_point_speed_x;
       al_fixed collision_speed_y = vertex_speed_y - struck_point_speed_y;

       al_fixed collision_speed = distance(collision_speed_y, collision_speed_x);

       al_fixed impulse_angle = get_angle(collision_y - single_pr->provisional_y, collision_x - single_pr->provisional_x);
       al_fixed force = (collision_speed * single_pr->mass) / COLLISION_FORCE_DIVISOR;//al_itofix(100); //pr_force_share / FORCE_DIVISOR;
       apply_impulse_to_group_at_member_vertex(gr, group_pr, collision_vertex, force, impulse_angle);
//       apply_impulse_to_proc_at_collision_vertex(pr, collision_vertex, force, impulse_angle);
//       group_pr->collided_this_cycle = 1;
//       gr->collided_this_cycle = 1;

       impulse_angle = get_angle(collision_y - group_pr->provisional_y, collision_x - group_pr->provisional_x);
       force = (collision_speed * gr->total_mass) / COLLISION_FORCE_DIVISOR;//al_itofix(100); //check_proc_force_share / FORCE_DIVISOR;
       apply_impulse_to_proc_at_vector_point(single_pr, struck_point_angle, struck_point_dist, force, impulse_angle);
//       apply_impulse_to_group(gr2, struck_point_old_x, struck_point_old_y, force, impulse_angle);
//       single_pr->collided_this_cycle = 1;

       fix_proc_speed(single_pr);
       fix_group_speed(gr);


}


/*

Group/group collisions.
First group is the group whose member's vertex hit a member of the second group

*/
static void group_group_collision(struct procstruct* vertex_pr, struct procstruct* other_pr, int collision_vertex)
{

       struct groupstruct* gr1 = &w.group[vertex_pr->group];
       struct groupstruct* gr2 = &w.group[other_pr->group];




       al_fixed collision_x = vertex_pr->provisional_x + fixed_xpart(vertex_pr->provisional_angle + vertex_pr->shape_str->collision_vertex_angle [collision_vertex], vertex_pr->shape_str->collision_vertex_dist [collision_vertex]);
       al_fixed collision_y = vertex_pr->provisional_y + fixed_ypart(vertex_pr->provisional_angle + vertex_pr->shape_str->collision_vertex_angle [collision_vertex], vertex_pr->shape_str->collision_vertex_dist [collision_vertex]);
// calculate the velocity of the vertex of pr that hit check_proc:
       al_fixed vertex_speed_x = collision_x
                      - (vertex_pr->x + fixed_xpart(vertex_pr->angle + vertex_pr->shape_str->collision_vertex_angle [collision_vertex], vertex_pr->shape_str->collision_vertex_dist [collision_vertex]));
       al_fixed vertex_speed_y = collision_y
                      - (vertex_pr->y + fixed_ypart(vertex_pr->angle + vertex_pr->shape_str->collision_vertex_angle [collision_vertex], vertex_pr->shape_str->collision_vertex_dist [collision_vertex]));

       al_fixed struck_point_angle = get_angle(collision_y - other_pr->provisional_y, collision_x - other_pr->provisional_x);
       al_fixed struck_point_dist = distance(collision_y - other_pr->provisional_y, collision_x - other_pr->provisional_x);
       al_fixed struck_point_old_x = other_pr->x + fixed_xpart(struck_point_angle - (other_pr->provisional_angle - other_pr->angle), struck_point_dist);
       al_fixed struck_point_old_y = other_pr->y + fixed_ypart(struck_point_angle - (other_pr->provisional_angle - other_pr->angle), struck_point_dist);
       al_fixed struck_point_speed_x = collision_x - struck_point_old_x;
       al_fixed struck_point_speed_y = collision_y - struck_point_old_y;

       al_fixed collision_speed_x = vertex_speed_x - struck_point_speed_x;
       al_fixed collision_speed_y = vertex_speed_y - struck_point_speed_y;

       al_fixed collision_speed = distance(collision_speed_y, collision_speed_x);

       al_fixed impulse_angle = get_angle(collision_y - other_pr->provisional_y, collision_x - other_pr->provisional_x);
       al_fixed force = (collision_speed * gr2->total_mass) / COLLISION_FORCE_DIVISOR;//al_itofix(100); //pr_force_share / FORCE_DIVISOR;
       apply_impulse_to_group_at_member_vertex(gr1, vertex_pr, collision_vertex, force, impulse_angle);
//       vertex_pr->collided_this_cycle = 1;
//       gr1->collided_this_cycle = 1;

       impulse_angle = get_angle(collision_y - vertex_pr->provisional_y, collision_x - vertex_pr->provisional_x);
       force = (collision_speed * gr1->total_mass) / COLLISION_FORCE_DIVISOR;//al_itofix(100); //check_proc_force_share / FORCE_DIVISOR;
       apply_impulse_to_group(gr2, struck_point_old_x, struck_point_old_y, force, impulse_angle);
//       other_pr->collided_this_cycle = 1;
//       gr2->collided_this_cycle = 1;

       fix_group_speed(gr1);
       fix_group_speed(gr2);


}


/*
static void apply_spin_to_proc(struct procstruct* pr, al_fixed x, al_fixed y, al_fixed force, al_fixed impulse_angle)
{

 if (!pr->mobile)
  return;

 al_fixed force_dist_from_centre = distance(y - pr->y, x - pr->x) / FORCE_DIST_DIVISOR;
 al_fixed lever_angle = get_angle(y - pr->y, x - pr->x);

 al_fixed torque = al_fixmul(al_fixmul(al_fixsin(lever_angle - impulse_angle), force_dist_from_centre), force);

 pr->spin_change -= al_fixdiv(torque, al_itofix(pr->moment));

}

// this is like the previous function, but since the impulse is applied at a vertex we can save some calculations
static void apply_spin_to_proc_at_collision_vertex(struct procstruct* pr, int v, al_fixed force, al_fixed impulse_angle)
{

 if (!pr->mobile)
  return;

 al_fixed force_dist_from_centre = pr->shape_str->collision_vertex_dist [v] / FORCE_DIST_DIVISOR;
 al_fixed lever_angle = pr->shape_str->collision_vertex_angle [v] + pr->angle;

 al_fixed torque = al_fixmul(al_fixmul(al_fixsin(lever_angle - impulse_angle), force_dist_from_centre), force);

 pr->spin_change -= al_fixdiv(torque, al_itofix(pr->moment));

}
*/

// assumes pr is not a member of a group
void apply_impulse_to_proc_at_vertex(struct procstruct* pr, int v, al_fixed force, al_fixed impulse_angle)
{

 if (!pr->mobile)
  return;

// force = al_fixmul(force, al_itofix(100));

 al_fixed force_dist_from_centre = pr->shape_str->vertex_dist [v] / FORCE_DIST_DIVISOR;
 al_fixed lever_angle = pr->shape_str->vertex_angle [v] + pr->angle;

 al_fixed torque = al_fixmul(al_fixmul(al_fixsin(lever_angle - impulse_angle), force_dist_from_centre), force); //al_fixdiv(force, TORQUE_DIVISOR_FIXED)));

// pr->spin_change -= al_fixdiv(torque, al_itofix(pr->moment));
 pr->spin_change -= torque / pr->moment;

// fprintf(stdout, "\nproc_at_vertex: v %i impulse_angle %i torque %i spin_change %f", v, fixed_angle_to_int(impulse_angle), fixed_angle_to_int(torque), fixed_to_radians(pr->spin_change));

 pr->x_speed += al_fixdiv(fixed_xpart(impulse_angle, force), al_itofix(pr->mass));
 pr->y_speed += al_fixdiv(fixed_ypart(impulse_angle, force), al_itofix(pr->mass));

 fix_proc_speed(pr);


}

// assumes pr is not a member of a group
void apply_impulse_to_proc_at_collision_vertex(struct procstruct* pr, int cv, al_fixed force, al_fixed impulse_angle)
{

 if (!pr->mobile)
  return;

// force = al_fixmul(force, al_itofix(100));

 al_fixed force_dist_from_centre = pr->shape_str->collision_vertex_dist [cv] / FORCE_DIST_DIVISOR;
 al_fixed lever_angle = pr->shape_str->collision_vertex_angle [cv] + pr->angle;

 al_fixed torque = al_fixmul(al_fixmul(al_fixsin(lever_angle - impulse_angle), force_dist_from_centre), force); //al_fixdiv(force, TORQUE_DIVISOR_FIXED)));

// pr->spin_change -= al_fixdiv(torque, al_itofix(pr->moment));
 pr->spin_change -= torque / pr->moment;

// fprintf(stdout, "\nproc_at_cvertex: pr %i cv %i impulse_angle %i force %f torque %f spin_change %f", pr->index, cv, fixed_angle_to_int(impulse_angle), al_fixtof(force), al_fixtof(torque), fixed_to_radians(pr->spin_change));

 pr->x_speed += al_fixdiv(fixed_xpart(impulse_angle, force), al_itofix(pr->mass));
 pr->y_speed += al_fixdiv(fixed_ypart(impulse_angle, force), al_itofix(pr->mass));

 fix_proc_speed(pr);


}



// assumes pr is not a member of a group
// point_angle and point_dist are a vector from the proc's centre to the collision point.
void apply_impulse_to_proc_at_vector_point(struct procstruct* pr, al_fixed point_angle, al_fixed point_dist, al_fixed force, al_fixed impulse_angle)
{

 if (!pr->mobile)
  return;

// force = al_fixmul(force, al_itofix(100));

 al_fixed torque = al_fixmul(al_fixmul(al_fixsin(point_angle - impulse_angle), point_dist), force); //al_fixdiv(force, TORQUE_DIVISOR_FIXED)));

// pr->spin_change -= al_fixdiv(torque, al_itofix(pr->moment));
 pr->spin_change -= torque / pr->moment;

// fprintf(stdout, "\nproc_at_point: pr %i ang,dis %i,%i impulse_angle %i force %f torque %f spin_change %f", pr->index, fixed_angle_to_int(point_angle), al_fixtoi(point_dist), fixed_angle_to_int(impulse_angle), al_fixtof(force), al_fixtof(torque), fixed_to_radians(pr->spin_change));

 pr->x_speed += al_fixdiv(fixed_xpart(impulse_angle, force), al_itofix(pr->mass));
 pr->y_speed += al_fixdiv(fixed_ypart(impulse_angle, force), al_itofix(pr->mass));

 fix_proc_speed(pr);


}


static void fix_proc_speed(struct procstruct* pr)
{

       if (pr->x_speed > MAX_SPEED_FIXED)
        pr->x_speed = MAX_SPEED_FIXED;
       if (pr->x_speed < NEG_MAX_SPEED_FIXED)
        pr->x_speed = NEG_MAX_SPEED_FIXED;
       if (pr->y_speed > MAX_SPEED_FIXED)
        pr->y_speed = MAX_SPEED_FIXED;
       if (pr->y_speed < NEG_MAX_SPEED_FIXED)
        pr->y_speed = NEG_MAX_SPEED_FIXED;

}

static void fix_group_speed(struct groupstruct* gr)
{

        if (gr->x_speed > MAX_SPEED_FIXED)
        gr->x_speed = MAX_SPEED_FIXED;
       if (gr->x_speed < NEG_MAX_SPEED_FIXED)
        gr->x_speed = NEG_MAX_SPEED_FIXED;
       if (gr->y_speed > MAX_SPEED_FIXED)
        gr->y_speed = MAX_SPEED_FIXED;
       if (gr->y_speed < NEG_MAX_SPEED_FIXED)
        gr->y_speed = NEG_MAX_SPEED_FIXED;

}


void apply_impulse_to_group(struct groupstruct* gr, al_fixed x, al_fixed y, al_fixed force, al_fixed impulse_angle)
{

 if (!gr->mobile)
  return;

 al_fixed force_dist_from_centre = hypot(y - gr->y, x - gr->x) / FORCE_DIST_DIVISOR;
 al_fixed lever_angle = get_angle(y - gr->y, x - gr->x);
 al_fixed torque = al_fixmul(al_fixmul(al_fixsin(lever_angle - impulse_angle), force_dist_from_centre), force); //al_fixdiv(force, TORQUE_DIVISOR_FIXED)));

// gr->spin_change -= al_fixdiv(torque, al_itofix(gr->moment));
 gr->spin_change -= torque / gr->moment;

 gr->x_speed += al_fixdiv(fixed_xpart(impulse_angle, force), al_itofix(gr->total_mass));
 gr->y_speed += al_fixdiv(fixed_ypart(impulse_angle, force), al_itofix(gr->total_mass));

 fix_group_speed(gr);

}


void apply_impulse_to_group_at_member_vertex(struct groupstruct* gr, struct procstruct* pr, int vertex, al_fixed force, al_fixed impulse_angle)
{

 if (!gr->mobile)
  return;

 al_fixed x = pr->x + fixed_xpart(pr->angle + pr->shape_str->vertex_angle [vertex], pr->shape_str->vertex_dist [vertex]);
 al_fixed y = pr->y + fixed_ypart(pr->angle + pr->shape_str->vertex_angle [vertex], pr->shape_str->vertex_dist [vertex]);

 al_fixed force_dist_from_centre = distance(y - gr->y, x - gr->x) / FORCE_DIST_DIVISOR;
 al_fixed lever_angle = get_angle(y - gr->y, x - gr->x);
 al_fixed torque = al_fixmul(al_fixmul(al_fixsin(lever_angle - impulse_angle), force_dist_from_centre), force); //al_fixdiv(force, TORQUE_DIVISOR_FIXED)));

// gr->spin_change -= al_fixdiv(torque, al_itofix(gr->moment));
 gr->spin_change -= torque / gr->moment;

// fprintf(stdout, "\ngroup: v %i impulse_angle %i torque %i spin_change %f", vertex, fixed_angle_to_int(impulse_angle), fixed_angle_to_int(torque), fixed_to_radians(gr->spin_change));

 gr->x_speed += al_fixdiv(fixed_xpart(impulse_angle, force), al_itofix(gr->total_mass));
 gr->y_speed += al_fixdiv(fixed_ypart(impulse_angle, force), al_itofix(gr->total_mass));

 fix_group_speed(gr);

}



// Assumes that pr is the first member of the group
static void set_group_motion_prov_values(struct procstruct* pr, struct groupstruct* gr)
{

// fprintf(stdout, "\nset_group_motion_prov_values %i", pr->index);

 gr->hit_edge_this_cycle = 0;

 if (!gr->mobile)
 {
  gr->x_speed = al_itofix(0);
  gr->y_speed = al_itofix(0);
  gr->spin = al_itofix(0);
// TO DO: can't return here as we need to set up provisional values. But this is inefficient for immobile procs.
//  - maybe just set them up once (or each time group composition changes)? They shouldn't change otherwise

//  return;
 }

  gr->x_speed = al_fixmul(gr->x_speed, gr->drag);
  gr->y_speed = al_fixmul(gr->y_speed, gr->drag);

// if (gr->stuck < 30)
// {
// drag group:
//  gr->x_speed = (gr->x_speed * gr->drag) / GRAIN_MULTIPLY;
//  gr->y_speed = (gr->y_speed * gr->drag) / GRAIN_MULTIPLY;
// gr->drag will have been set by run_pass1_group_member in the previous cycle (or initialised in group.c).
// this isn't wonderfully precise, but it's good enough (as at least gr->drag will always be a valid value)

  gr->test_x = gr->x + gr->x_speed;
  gr->test_y = gr->y + gr->y_speed;
  gr->test_angle = gr->angle; // spin is added below
// }
//  else
//   jitter_group(gr);


// gr->test_angle
 gr->drag = DRAG_BASE_FIXED;

// fprintf(stdout, "\ngr angle %f spin %f spin_change %f", al_fixtof(gr->angle), al_fixtof(gr->spin), al_fixtof(gr->spin_change));

  gr->spin += gr->spin_change;
//  fprintf(stdout, "\nspin_change %f", gr->spin_change);
  gr->spin_change = 0;

  if (gr->spin != 0)
  {
   if (gr->spin > SPIN_MAX_FIXED)
    gr->spin = SPIN_MAX_FIXED;
   if (gr->spin < NEGATIVE_SPIN_MAX_FIXED)
    gr->spin = NEGATIVE_SPIN_MAX_FIXED;


   gr->test_angle += gr->spin;

   fix_fixed_angle(&gr->test_angle);

   gr->spin = al_fixmul(gr->spin, SPIN_DRAG_BASE_FIXED);

  }


// if (gr->mobile != 0)
 {
  w.current_check ++;
// This call must be after group movement values set:
  set_group_member_prov_values(pr, NULL, 0, gr); // recursive function
 }


}


static void check_group_collision(struct groupstruct* gr)
{

 w.current_check ++;

 test_group_collision(gr->first_member, gr);


}

// recursive function called from set_group_motion_prov_values: sets group member x/y/angle prov/test values based on test values for group
// w.current_check must be incremented before this function is called by another function
static void set_group_member_prov_values(struct procstruct* pr, struct procstruct* upstream_pr, int connection_index, struct groupstruct* gr)
{
 int i;

if (gr->mobile)
{
 if (upstream_pr != NULL)
 {
  pr->test_x = upstream_pr->test_x + fixed_xpart(upstream_pr->connection_angle [connection_index] + upstream_pr->test_angle, upstream_pr->connection_dist [connection_index]);
  pr->test_y = upstream_pr->test_y + fixed_ypart(upstream_pr->connection_angle [connection_index] + upstream_pr->test_angle, upstream_pr->connection_dist [connection_index]);
  pr->test_angle = upstream_pr->test_angle + upstream_pr->connection_angle_difference [connection_index];
 }
  else
  {
   pr->test_x = gr->test_x + fixed_xpart(pr->group_angle + gr->test_angle, pr->group_distance);
   pr->test_y = gr->test_y + fixed_ypart(pr->group_angle + gr->test_angle, pr->group_distance);
   pr->test_angle = get_fixed_fixed_angle(pr->group_angle_offset + gr->angle);
  }
}
 else
 {
  pr->test_x = pr->x;
  pr->test_y = pr->y;
  pr->test_angle = pr->angle;
 }

 pr->provisional_x = pr->test_x;
 pr->provisional_y = pr->test_y;
 pr->provisional_angle = pr->test_angle;

 pr->test_x_block = fixed_to_block(pr->test_x);
 pr->test_y_block = fixed_to_block(pr->test_y);

// fprintf(stdout, "\nproc %i test_xy (%i, %i) block (%i, %i)", pr->index, pr->test_x, pr->test_y, pr->test_x_block, pr->test_y_block);

 pr->group_check = w.current_check;

// static int timeout = 0;

 for (i = 0; i < GROUP_CONNECTIONS; i ++)
 {
  if (pr->group_connection [i] != NULL
   && pr->group_connection [i]->group_check != pr->group_check)
   {
    set_group_member_prov_values(pr->group_connection [i], pr, i, gr);
   }
 }

}





/*
static void set_group_member_prov_values(struct procstruct* pr, struct groupstruct* gr)
{
 int i;

 pr->test_x = gr->test_x + fixed_xpart(pr->group_angle + gr->test_angle, pr->group_distance);
 pr->test_y = gr->test_y + fixed_ypart(pr->group_angle + gr->test_angle, pr->group_distance);
 pr->provisional_x = pr->test_x;
 pr->provisional_y = pr->test_y;
 pr->test_angle = get_fixed_fixed_angle(pr->group_angle_offset + gr->angle);
 pr->provisional_angle = pr->test_angle;

 pr->test_x_block = fixed_to_block(pr->test_x);
 pr->test_y_block = fixed_to_block(pr->test_y);

// fprintf(stdout, "\nproc %i test_xy (%i, %i) block (%i, %i)", pr->index, pr->test_x, pr->test_y, pr->test_x_block, pr->test_y_block);

 pr->group_check = w.current_check;

// static int timeout = 0;

 for (i = 0; i < GROUP_CONNECTIONS; i ++)
 {
  if (pr->group_connection [i] != NULL
   && pr->group_connection [i]->group_check != pr->group_check)
   {
    set_group_member_prov_values(pr->group_connection [i], gr);
   }
 }

}*/

/*

recursive function that checks whether a group can move to a new position.
requires group test position values to have been filled in; this function then fills in the test position values for each proc. If the movement is possible, these test position values are used.

returns 1 if collision, 0 if no collision

*/
static int test_group_collision(struct procstruct* pr, struct groupstruct* gr)
{

// return 0;

 int i;
 int x, y;
 struct blockstruct* bl;
// struct procstruct* check_proc;
// int dist;


 for (x = -1; x < 2; x ++)
 {
  for (y = -1; y < 2; y ++)
  {

   bl = &w.block [pr->test_x_block + x] [pr->test_y_block + y];

//   fprintf(stdout, "\ntest_group_collision: pr %i at (%i, %i) block (%i, %i) (group first member %i at %i, %i)", pr->index, pr->test_x, pr->test_y, pr->test_x_block, pr->test_y_block, gr->first_member->index, gr->test_x, gr->test_y);
//   error_call();

   if (bl->tag != w.blocktag)
    continue;

   check_block_collision_group_member(pr, bl);
//    return 1; (is now void) // actually I don't think this will ever return 1. Probably okay because we should be going through all tests even after a collision is found.

  }
 }

// now let's check for a collision with the edge of the map:

  al_fixed notional_x = pr->test_x;
  al_fixed notional_y = pr->test_y;
  struct shapestruct* pr_shape = &shape_dat [pr->shape] [pr->size];

  if (w.block [pr->test_x_block] [pr->test_y_block].block_type != BLOCK_NORMAL)
  {
  switch(w.block [pr->test_x_block] [pr->test_y_block].block_type)
  {
   case BLOCK_SOLID:
#ifdef SANITY_CHECK
    fprintf(stdout, "\nError: g_motion.c: test_group_collision(): proc inside solid block (proc %i at (%i,%i) block (%i,%i)).", pr->index, notional_x, notional_y, pr->test_x_block, pr->test_y_block);
    error_call();
#endif
    break;
   case BLOCK_EDGE_LEFT:
    if (fixed_to_block(notional_x - pr_shape->max_length) != pr->test_x_block)
    {
     pr->hit_edge_this_cycle = 1;
     gr->hit_edge_this_cycle = 1;
     gr->x_speed += EDGE_ACCEL;
     if (gr->x_speed > EDGE_ACCEL_MAX)
      gr->x_speed = EDGE_ACCEL_MAX;
    }
    break;
   case BLOCK_EDGE_RIGHT:
    if (fixed_to_block(notional_x + pr_shape->max_length) != pr->test_x_block)
    {
     pr->hit_edge_this_cycle = 1;
     gr->hit_edge_this_cycle = 1;
     gr->x_speed -= EDGE_ACCEL;
     if (gr->x_speed < NEGATIVE_EDGE_ACCEL_MAX)
      gr->x_speed = NEGATIVE_EDGE_ACCEL_MAX;
    }
    break;
   case BLOCK_EDGE_UP:
    if (fixed_to_block(notional_y - pr_shape->max_length) != pr->test_y_block)
    {
     pr->hit_edge_this_cycle = 1;
     gr->hit_edge_this_cycle = 1;
     gr->y_speed += EDGE_ACCEL;
     if (gr->y_speed > EDGE_ACCEL_MAX)
      gr->y_speed = EDGE_ACCEL_MAX;
    }
    break;
   case BLOCK_EDGE_DOWN:
    if (fixed_to_block(notional_y + pr_shape->max_length) != pr->test_y_block)
    {
     pr->hit_edge_this_cycle = 1;
     gr->hit_edge_this_cycle = 1;
     gr->y_speed -= EDGE_ACCEL;
     if (gr->y_speed < NEGATIVE_EDGE_ACCEL_MAX)
      gr->y_speed = NEGATIVE_EDGE_ACCEL_MAX;
    }
    break;
   case BLOCK_EDGE_UP_LEFT:
// up
    if (fixed_to_block(notional_y - pr_shape->max_length) != pr->test_y_block)
    {
     pr->hit_edge_this_cycle = 1;
     gr->hit_edge_this_cycle = 1;
     gr->y_speed += EDGE_ACCEL;
     if (gr->y_speed > EDGE_ACCEL_MAX)
      gr->y_speed = EDGE_ACCEL_MAX;
    }
// left
    if (fixed_to_block(notional_x - pr_shape->max_length) != pr->test_x_block)
    {
     pr->hit_edge_this_cycle = 1;
     gr->hit_edge_this_cycle = 1;
     gr->x_speed += EDGE_ACCEL;
     if (gr->x_speed > EDGE_ACCEL_MAX)
      gr->x_speed = EDGE_ACCEL_MAX;
    }
    break;
   case BLOCK_EDGE_UP_RIGHT:
// up
    if (fixed_to_block(notional_y - pr_shape->max_length) != pr->test_y_block)
    {
     pr->hit_edge_this_cycle = 1;
     gr->hit_edge_this_cycle = 1;
     gr->y_speed += EDGE_ACCEL;
     if (gr->y_speed > EDGE_ACCEL_MAX)
      gr->y_speed = EDGE_ACCEL_MAX;
    }
// right
    if (fixed_to_block(notional_x + pr_shape->max_length) != pr->test_x_block)
    {
     pr->hit_edge_this_cycle = 1;
     gr->hit_edge_this_cycle = 1;
     gr->x_speed -= EDGE_ACCEL;
     if (gr->x_speed < NEGATIVE_EDGE_ACCEL_MAX)
      gr->x_speed = NEGATIVE_EDGE_ACCEL_MAX;
    }
    break;
   case BLOCK_EDGE_DOWN_LEFT:
// down
    if (fixed_to_block(notional_y + pr_shape->max_length) != pr->test_y_block)
    {
     pr->hit_edge_this_cycle = 1;
     gr->hit_edge_this_cycle = 1;
     gr->y_speed -= EDGE_ACCEL;
     if (gr->y_speed < NEGATIVE_EDGE_ACCEL_MAX)
      gr->y_speed = NEGATIVE_EDGE_ACCEL_MAX;
    }
// left
    if (fixed_to_block(notional_x - pr_shape->max_length) != pr->test_x_block)
    {
     pr->hit_edge_this_cycle = 1;
     gr->hit_edge_this_cycle = 1;
     gr->x_speed += EDGE_ACCEL;
     if (gr->x_speed > EDGE_ACCEL_MAX)
      gr->x_speed = EDGE_ACCEL_MAX;
    }
    break;
   case BLOCK_EDGE_DOWN_RIGHT:
// down
    if (fixed_to_block(notional_y + pr_shape->max_length) != pr->test_y_block)
    {
     pr->hit_edge_this_cycle = 1;
     gr->hit_edge_this_cycle = 1;
     gr->y_speed -= EDGE_ACCEL;
     if (gr->y_speed < NEGATIVE_EDGE_ACCEL_MAX)
      gr->y_speed = NEGATIVE_EDGE_ACCEL_MAX;
    }
// right
    if (fixed_to_block(notional_x + pr_shape->max_length) != pr->test_x_block)
    {
     pr->hit_edge_this_cycle = 1;
     gr->hit_edge_this_cycle = 1;
     gr->x_speed -= EDGE_ACCEL;
     if (gr->x_speed < NEGATIVE_EDGE_ACCEL_MAX)
      gr->x_speed = NEGATIVE_EDGE_ACCEL_MAX;
    }
    break;
   }
  }


 pr->group_check = w.current_check;

 for (i = 0; i < GROUP_CONNECTIONS; i ++)
 {
  if (pr->group_connection [i] != NULL
   && pr->group_connection [i]->group_check != pr->group_check)
   {
    if (test_group_collision(pr->group_connection [i], gr) == 1)
     return 1;
   }
 }

 return 0;

}

/*
static void apply_spin_to_group(struct groupstruct* gr, al_fixed collision_x, al_fixed collision_y, al_fixed force, al_fixed impulse_angle)
{

 if (!gr->mobile)
  return;

 al_fixed force_dist_from_centre = distance(collision_y - gr->y, collision_x - gr->x) / FORCE_DIST_DIVISOR;
// int lever_angle = radians_to_angle(atan2(collision_y - gr->y, collision_x - gr->x));
 al_fixed lever_angle = get_angle(collision_y - gr->y, collision_x - gr->x);

 force /= gr->moment;

// float torque = ypart(lever_angle - impulse_angle, dist_from_centre * force) / TORQUE_DIVISOR;
// float torque = sin(lever_angle - impulse_angle) * force_dist_from_centre * force / TORQUE_DIVISOR;
// al_fixed torque = al_fixsin(al_fixmul(al_fixmul(lever_angle - impulse_angle, force_dist_from_centre), force / TORQUE_DIVISOR));
 al_fixed torque = al_fixmul(al_fixmul(al_fixsin(lever_angle - impulse_angle), force_dist_from_centre), force); //al_fixdiv(force, TORQUE_DIVISOR_FIXED)));

 fprintf(stdout, "\nastgp: (%i, %i) force %i dist %i la %i impa %i torque %i +spin %f", al_fixtoi(collision_x), al_fixtoi(collision_y), al_fixtoi(force), al_fixtoi(force_dist_from_centre),
         fixed_angle_to_int(lever_angle), fixed_angle_to_int(impulse_angle), al_fixtoi(torque), fixed_to_radians((torque) / gr->moment));

// gr->spin_change += torque / gr->moment;
 gr->spin_change += torque; //al_fixdiv(torque, al_itofix(gr->moment));


}
*/

/*
recursive function that moves a group to a new position.
requires group test position values to have been filled in.
*/
static void move_group(struct procstruct* pr, struct groupstruct* gr)
{

  gr->x = gr->test_x;
  gr->y = gr->test_y;
  gr->angle = gr->test_angle;
//  gr->angle_angle = gr->angle; // this value is used in move_group. It's not super-reliable
  w.current_check ++;
  move_group_members(pr, gr);


}

// Recursive function called from move_group
// Moves the members of the group based on test values set in set_group_member_prov_values
static void move_group_members(struct procstruct* pr, struct groupstruct* gr)
{

 int i;

 pr->x_speed = pr->test_x - pr->x;
 pr->y_speed = pr->test_y - pr->y;

 pr->old_x = pr->x;
 pr->old_y = pr->y;
 pr->old_angle = pr->angle;

 pr->x = pr->test_x;
 pr->y = pr->test_y;

 pr->angle = pr->test_angle;//(pr->group_angle_offset + gr->angle_angle) & ANGLE_MASK;
 pr->float_angle = fixed_to_radians(pr->group_angle_offset + gr->angle); // used in display

 pr->x_block = pr->test_x_block;
 pr->y_block = pr->test_y_block;

 pr->group_check = w.current_check;

 for (i = 0; i < GROUP_CONNECTIONS; i ++)
 {
  if (pr->group_connection [i] != NULL
   && pr->group_connection [i]->group_check != pr->group_check)
   {
    move_group_members(pr->group_connection [i], gr);
   }
 }



}


/*

recursive function that sets speed of all group members to zero (after a collision).
doesn't do anything to group speed, so the group can start moving again next cycle.
* /
static void group_doesnt_move(struct procstruct* pr, struct groupstruct* gr)
{

 int i;

 pr->x_speed = al_itofix(0);
 pr->y_speed = al_itofix(0);
 pr->test_x = pr->x;
 pr->test_y = pr->y;
 pr->provisional_x = pr->test_x;
 pr->provisional_y = pr->test_y;

 pr->angle = get_fixed_fixed_angle(pr->group_angle_offset + gr->angle);
 pr->float_angle = fixed_to_radians(pr->group_angle_offset + gr->angle); // used in display

 pr->x_block = pr->test_x_block;
 pr->y_block = pr->test_y_block;

// pr->collided_this_cycle = 1;

 pr->group_check = w.current_check;

 for (i = 0; i < GROUP_CONNECTIONS; i ++)
 {
  if (pr->group_connection [i] != NULL
   && pr->group_connection [i]->group_check != pr->group_check)
   {
    group_doesnt_move(pr->group_connection [i], gr);
   }
 }

}
*/


/*

recursive function that sets test_x/y values for all group members to actual x/y (for groups that aren't moving).

Not currently used.
* /
static void group_set_stationary_test_values(struct procstruct* pr)
{

 int i;

 pr->test_x = pr->x;
 pr->test_y = pr->y;
 pr->provisional_x = pr->test_x;
 pr->provisional_y = pr->test_y;

 pr->group_check = w.current_check;

 for (i = 0; i < GROUP_CONNECTIONS; i ++)
 {
  if (pr->group_connection [i] != NULL
   && pr->group_connection [i]->group_check != pr->group_check)
   {
    group_set_stationary_test_values(pr->group_connection [i]);
   }
 }

}
*/








