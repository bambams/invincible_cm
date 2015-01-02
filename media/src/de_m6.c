
/*

de_m6.c

This file contains the delegate program that controls the enemy processes in mission 6.
It is given as an example of a delegate program, and would need plenty of changes to
 work outside of the context of mission 6.

*/


interface
{
 PROGRAM_TYPE_DELEGATE, // program's type
 {
  METH_COMMAND_GIVE: {MT_CL_COMMAND_GIVE}, // command method - allows operator to communicate with processes
  METH_COMMAND_REC: {MT_CLOB_COMMAND_REC}, // command method - allows operator to communicate with processes
  METH_POINT: {MT_CLOB_POINT}, // point check - allows operator to find what is at a particular point
  METH_STD: {MT_CLOB_STD}, // standard clob method - does various things like give information about the game world
  METH_QUERY: {MT_CLOB_QUERY}, // query - allows operator to get information about processes (similar to MT_PR_INFO)
  METH_MATHS: {MT_CLOB_MATHS}, // maths method (works the same way as MT_PR_MATHS)
 }
}

int initialised;
int team;
int first_proc;
int last_proc;
int team_size;
int world_size_x, world_size_y;
int counter;

int rand_seed;
int spider_x, spider_y, spider_angle;

static int random(int rand_max);



static void main(void)
{

 if (initialised == 0)
	{

  initialised = 1;
  counter = 5000; // so that the target values are set at the start
  team = world_team(); // world_team() is a built-in function that uses the MT_CLOB_STD method. It gets the operator's player index.
  first_proc = world_first_process(); // world_first_process() gets this player's first process' index.
  last_proc = world_last_process(); // last process' index
  team_size = world_processes_each();
  world_size_x = world_x();
  world_size_y = world_y();
  rand_seed = world_size_x / 2;

	}

 counter ++;
 rand_seed += world_processes() * world_time();
 spider_x = query_x(first_proc); // assumes first_proc is spider core
 spider_y = query_y(first_proc);
 spider_angle = query_angle(first_proc);

 int i;

// Every few seconds, look for a nearby allocator and tell the spider to attack it:
 if (counter % 131 == 0)
	{
		int closest_allocator, closest_distance, dist, target_x, target_y, angle_to_target;
		closest_allocator = -1;
		closest_distance = 30000;
		for (i = 0; i < team_size; i ++) // we can assume that opponent is player 0 so processes will be 0 to team_size-1
		{
			if (query_hp(i) >= 0
			 && query_method_find(i, MT_PR_ALLOCATE, 0) != -1)
			{
				target_x = query_x(i);
				target_y = query_y(i);
				angle_to_target = atan2(target_y - spider_y, target_x - spider_x);
				dist = hypot(spider_y - target_y, spider_x - target_x) + (angle_difference(angle_to_target, spider_angle) / 4);
				if (dist < closest_distance)
				{
					closest_allocator = i;
					closest_distance = dist;
				}
			}
		} // end for i
  if (closest_allocator != -1)
		{
			int spider_target_x, spider_target_y;
			spider_target_x = query_x(closest_allocator);
			spider_target_y = query_y(closest_allocator);
#subdefine SPIDER_EDGE 2000
			if (spider_target_x < SPIDER_EDGE)
				spider_target_x = SPIDER_EDGE;
			if (spider_target_x > world_size_x - SPIDER_EDGE)
				spider_target_x = world_size_x - SPIDER_EDGE;
			if (spider_target_y < SPIDER_EDGE)
				spider_target_y = SPIDER_EDGE;
			if (spider_target_y > world_size_y - SPIDER_EDGE)
				spider_target_y = world_size_y - SPIDER_EDGE;
			command(first_proc, 0, spider_target_x);
			command(first_proc, 1, spider_target_y);
		}
		 else
			{
			 command(first_proc, 0, 0); // will wander randomly
			}
	}

// Now tell the other processes where the spider is, so they can orbit it.
 for (i = first_proc + 1; i < last_proc; i ++) // first_proc + 1 means ignore spider core, which we know is first_proc
	{
		if (query_hp(i) < 0) // process doesn't exist
			continue;

  command(i, 0, spider_x);
  command(i, 1, spider_y);
  command(i, 2, -2900 + (i * 30)); // circling distance

	} // end process loop

} // end main()

static int random(int rand_max)
{

 rand_seed += world_processes() * world_time();

 return abs(rand_seed) % rand_max;

}


