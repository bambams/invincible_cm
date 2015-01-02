#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>


#include <stdio.h>
#include <math.h>

#include "m_config.h"
#include "m_globvars.h"

#include "g_header.h"

#include "g_proc.h"
#include "g_cloud.h"
#include "g_world.h"
#include "g_shape.h"

#include "g_misc.h"
#include "m_maths.h"

int check_packet_collision(struct packetstruct* pack, struct blockstruct* bl, al_fixed x, al_fixed y);
void destroy_packet(struct packetstruct* pack);
void packet_explodes(struct packetstruct* pk, struct procstruct* pr_hit);


extern int collision_mask [SHAPES] [COLLISION_MASK_SIZE] [COLLISION_MASK_SIZE]; // in g_shape.c


void init_packets(void)
{

 int pk;

 for (pk = 0; pk < w.max_packets; pk++)
 {
  w.packet[pk].exists = 0;
  w.packet[pk].index = pk;
 }

}

// returns index of new packet on success, -1 on failure
int new_packet(int type, int player_index, al_fixed x, al_fixed y)
{

 int pk;
 struct packetstruct* pack;

 for (pk = 0; pk < w.max_packets; pk++)
 {
  if (!w.packet[pk].exists)
   break;
 }

 if (pk == w.max_packets)
  return -1;
 pack = &w.packet[pk];

 pack->exists = 1;
 pack->player_index = player_index;
 pack->type = type;
 pack->time = 0;
 pack->x = x;
 pack->y = y;
 pack->x_block = fixed_to_block(x);
 pack->y_block = fixed_to_block(y);

 pack->prand_seed = w.world_time + x + y; // can be anything as long as it's just random enough

// pack->tail_x = pack->x;
// pack->tail_y = pack->y;
// pack->tail_count = 0;
 pack->collision_size = 0;

 return pk;

}



void run_packets(void)
{

 int pk;
 struct packetstruct* pack;
 int i, j, bx, by;
 int proc_hit, finished;

 for (pk = 0; pk < w.max_packets; pk++)
 {
  if (!w.packet[pk].exists)
   continue;
  pack = &w.packet[pk];
// see if its time has run out:
  pack->timeout --;
  if (pack->timeout == 0)
  {
   packet_explodes(pack, NULL);
   destroy_packet(pack);
   continue;
  }
  pack->time ++;

  pack->x += pack->x_speed;
  pack->y += pack->y_speed;
/*
  if (pack->x < 0 || pack->x >= w.w_grain
   || pack->y < 0 || pack->y >= w.h_grain)
  {
   destroy_packet(pack);
   continue;
  }*/

  pack->x_block = fixed_to_block(pack->x);
  pack->y_block = fixed_to_block(pack->y);

  if (w.block [pack->x_block] [pack->y_block].block_type == BLOCK_SOLID)
  {
   packet_explodes(pack, NULL);
   destroy_packet(pack);
   continue;
  }

  if (pack->source_proc != -1)
		{
			if (w.proc[pack->source_proc].exists <= 0)
				pack->source_proc = -1;
		}

// check collisions:
  finished = 0;
  for (i = -1; i < 2; i ++)
  {
   bx = pack->x_block + i;
   if (bx < 0 || bx >= w.w_block)
    continue;
   for (j = -1; j < 2; j ++)
   {
    by = pack->y_block + j;
    if (by < 0 || by >= w.w_block)
     continue;
    proc_hit = check_packet_collision(pack, &w.block [bx] [by], pack->x, pack->y);
    if (proc_hit != -1)
    {
     w.proc[proc_hit].hit = 5;
     apply_packet_damage_to_proc(&w.proc[proc_hit], pack->damage, pack->player_index);
     packet_explodes(pack, &w.proc[proc_hit]);
     destroy_packet(pack);
     finished = 1;
     break;
    }
   }
   if (finished)
    break;
  }
  if (finished)
   continue;

//  pack->tail_count++;

// now put it on the blocklist:
// This works just like blocktags for procs - see g_motion.c
  if (w.block [pack->x_block] [pack->y_block].packet_tag != w.blocktag)
  {
   w.block [pack->x_block] [pack->y_block].packet_tag = w.blocktag;
   pack->blocklist_down = NULL;
   pack->blocklist_up = NULL;
   w.block [pack->x_block] [pack->y_block].packet_down = pack;
  }
   else
   {
// The block's packet_blocktag is up to date. So we put the new packet on top and set its downlink to the top packet of the block:
    pack->blocklist_down = w.block [pack->x_block] [pack->y_block].packet_down;
    pack->blocklist_up = NULL;
    w.block [pack->x_block] [pack->y_block].packet_down = pack;
// we also set the previous top proc's uplink to pack:
    if (pack->blocklist_down != NULL) // not sure that this is necessary.
     pack->blocklist_down->blocklist_up = pack;
   }

 }

}


// returns index of proc if collision, -1 if not
int check_packet_collision(struct packetstruct* pack, struct blockstruct* bl, al_fixed x, al_fixed y)
{

 if (bl->tag != w.blocktag)
  return -1;

 struct procstruct* check_proc;
 check_proc = bl->blocklist_down;
// static int collision_x, collision_y;

 int size_check;

 while(check_proc != NULL)
 {
// first check team safety and do a bounding box
    if (check_proc->player_index != pack->team_safe
     && check_proc->exists == 1
     && check_proc->x + check_proc->max_length > x
     && check_proc->x - check_proc->max_length < x
     && check_proc->y + check_proc->max_length > y
     && check_proc->y - check_proc->max_length < y)
     {

      al_fixed dist = distance(y - check_proc->y, x - check_proc->x);

      al_fixed angle = get_angle(y - check_proc->y, x - check_proc->x);
      al_fixed angle_diff = angle - check_proc->angle;

      size_check = check_proc->size + pack->collision_size;

      unsigned int mask_x = MASK_CENTRE + al_fixtoi(fixed_xpart(angle_diff, dist));
      mask_x >>= COLLISION_MASK_BITSHIFT;
      unsigned int mask_y = MASK_CENTRE + al_fixtoi(fixed_ypart(angle_diff, dist));
      mask_y >>= COLLISION_MASK_BITSHIFT;

      if (mask_x < COLLISION_MASK_SIZE
       && mask_y < COLLISION_MASK_SIZE)
       {
        if (collision_mask [check_proc->shape] [mask_x] [mask_y] <= size_check)
        {
         return check_proc->index;
        }
       }

     }

  check_proc = check_proc->blocklist_down;
 };

 return -1;

}

// Call this when a packet explodes (because it hit something, or because it ran out of time)
// If pr_hit is NULL, the packet is treated as having missed
void packet_explodes(struct packetstruct* pk, struct procstruct* pr_hit)
{

 struct cloudstruct* cl;

 switch(pk->type)
 {
  case PACKET_TYPE_BASIC:
  case PACKET_TYPE_DPACKET:
   if (pr_hit != NULL)
   {
    cl = new_cloud(CLOUD_PACKET_HIT, pk->x, pk->y);

    if (cl != NULL)
    {
      cl->timeout = 16;
      cl->angle = get_angle(al_fixsub(pr_hit->y, pk->y), al_fixsub(pr_hit->x, pk->x));
      cl->colour = pk->colour;
      cl->data [0] = pk->method_extensions [MEX_PR_PACKET_POWER];
      cl->data [1] = pk->method_extensions [MEX_PR_PACKET_SPEED];
      cl->data [2] = w.world_time + pk->x_speed; // used as random seed
    	 if (pr_hit->virtual_method != -1
							&& pr_hit->method[pr_hit->virtual_method].data [MDATA_PR_VIRTUAL_STATE] > 0)
						{
								cl->type = CLOUD_PACKET_HIT_VIRTUAL;
								cl->colour = pr_hit->player_index;
								cl->x -= pk->x_speed * 4;
								cl->y -= pk->y_speed * 4;
						}
    }
   }
    else
    {
     cl = new_cloud(CLOUD_PACKET_MISS, pk->x, pk->y);

     if (cl != NULL)
     {
      cl->timeout = 16;
      cl->angle = pk->angle;
      cl->colour = pk->colour;
      cl->data [0] = pk->method_extensions [MEX_PR_PACKET_POWER];
      cl->data [1] = pk->method_extensions [MEX_PR_PACKET_SPEED];
      cl->data [2] = w.world_time + pk->x_speed; // used as random seed
//      cl->data [1] = pk->size * 2;
//      cl->data [2] = pk->size;
     }
    }
//    disrupt_block_nodes(pk->x, pk->y, 5);
    disrupt_single_block_node(pk->x, pk->y, pk->player_index, 5);
   break;

 } // end switch

} // end packet_explodes



void destroy_packet(struct packetstruct* pack)
{

 pack->exists = 0;

}







