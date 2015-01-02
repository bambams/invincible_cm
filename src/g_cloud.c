#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>


#include <stdio.h>
#include <math.h>

#include "m_config.h"

#include "g_header.h"
#include "m_globvars.h"

#include "g_proc.h"

#include "g_misc.h"
#include "m_maths.h"
#include "g_world.h"

#include "g_cloud.h"


static void destroy_cloud(struct cloudstruct* cl);

void init_clouds(void)
{

 int c;

 for (c = 0; c < w.max_clouds; c++)
 {
  w.cloud[c].exists = 0;
 }

}

// returns index of new packet on success, -1 on failure
struct cloudstruct* new_cloud(int type, al_fixed x, al_fixed y)
{

 int c;
 struct cloudstruct* cl;

 for (c = 0; c < w.max_clouds; c++)
 {
  if (!w.cloud[c].exists)
   break;
 }

 if (c == w.max_clouds)
  return NULL;
 cl = &w.cloud[c];

 cl->exists = 1;
 cl->type = type;
 cl->x = x;
 cl->y = y;
// cl->x_block = x >> BLOCK_SIZE_BITSHIFT;
// cl->y_block = y >> BLOCK_SIZE_BITSHIFT;

 return cl;

}



void run_clouds(void)
{

 int c;
 struct cloudstruct* cl;
// int i, j, bx, by;
// int finished;

 for (c = 0; c < w.max_clouds; c++)
 {
  if (!w.cloud[c].exists)
   continue;
  cl = &w.cloud[c];
// see if its time has run out:
  cl->timeout --;
//  cl->timeout --;
  if (cl->timeout <= 0)
  {
   destroy_cloud(cl);
   continue;
  }

  switch(cl->type)
  {
   case CLOUD_PROC_EXPLODE_SMALL:
    cl->x_speed *= 950;
    cl->x_speed /= 1000;
    cl->y_speed *= 950;
    cl->y_speed /= 1000;
    cl->angle += int_angle_to_fixed(cl->data [2]);
    if (abs(cl->x_speed) + abs(cl->y_speed) < al_itofix(1)
     || cl->x <= (BLOCK_SIZE_FIXED * 2)
     || cl->y <= (BLOCK_SIZE_FIXED * 2)
     || cl->x >= w.w_fixed - (BLOCK_SIZE_FIXED * 2)
     || cl->y >= w.h_fixed - (BLOCK_SIZE_FIXED * 2))
    {
     cl->type = CLOUD_PROC_EXPLODE_SMALL2;
     cl->timeout = 30;
     cl->x_speed = 0;
     cl->y_speed = 0;
     disrupt_single_block_node(cl->x, cl->y, cl->data [4], 5);
    }
    break;
   case CLOUD_YIELD_LINE:
// one end of a yield line is the centre of a proc, so we need to make sure the proc still exists:
    if (w.proc[cl->data[0]].exists <= 0)
    {
     cl->exists = 0;
     // it doesn't really matter if some of the cloud's values are changed later in this loop
    }
    break;
  }

  cl->x += cl->x_speed;
  cl->y += cl->y_speed;

 }

}



static void destroy_cloud(struct cloudstruct* cl)
{

 cl->exists = 0;

}




