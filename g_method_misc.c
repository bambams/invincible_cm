#include <allegro5/allegro.h>

#include <stdio.h>
#include <math.h>

#include "m_config.h"


#include "g_header.h"

#include "c_header.h"
#include "e_slider.h"
#include "e_header.h"
#include "e_editor.h"
#include "e_log.h"

#include "g_world.h"
#include "g_misc.h"
#include "g_proc.h"
#include "g_packet.h"
#include "g_motion.h"
#include "g_method_misc.h"
#include "g_client.h"
#include "g_cloud.h"
#include "g_shape.h"
#include "m_globvars.h"
#include "m_maths.h"
#include "t_template.h"
#include "i_error.h"
#include "i_console.h"



/*

This file contains some method functions that might be used by any kind of program.

*/




extern struct viewstruct view; // TO DO: think about putting a pointer to this in the worldstruct instead of externing it

extern struct shapestruct shape_dat [SHAPES] [SHAPES_SIZES]; // set up in g_shape.c
extern char sense_circles [SENSE_CIRCLES] [SENSE_CIRCLE_LENGTH] [2];

extern struct controlstruct control; // defined in i_input.c. Used here for client process methods.

extern const struct mtypestruct mtype [MTYPES]; // defined in g_method.c

extern struct templstruct templ [TEMPLATES]; // defined in t_template.c

static void scan_block(int block_x, int block_y, al_fixed scan_x, al_fixed scan_y, int range, struct procstruct* ignore_proc, s16b* mb);


// This is called by proc creation and also by functions that copy bcode into templates
// returns: 0 success, 1 start_address out of bounds, 2 end_address out of bounds
int check_start_end_addresses(int start_address, int end_address, int bcode_size)
{

 if (start_address >= bcode_size - TOTAL_PROC_HEADER_SIZE - 1 || start_address < 0)
 {
//  fprintf(stdout, "\n * check_start_end_addresses failed: start_address error (%i max %i)", start_address, bcode_size - TOTAL_PROC_HEADER_SIZE);
  return 1; // failed (start_address error)
 }

 if (end_address >= bcode_size
  || end_address < 0
  || end_address <= start_address + TOTAL_PROC_HEADER_SIZE)
 {
//  fprintf(stdout, "\n * check_start_end_addresses failed: end_address error (%i max %i)", end_address, bcode_size - TOTAL_PROC_HEADER_SIZE);
  return 2; // failed (end_address error)
 }

 return 0; // no problem with start/end addresses

}

// returns: 0 success, 1 invalid shape, 2 invalid size
int check_proc_shape_size(int proc_shape, int proc_size)
{

 if (proc_shape < 0 || proc_shape >= SHAPES)
 {
//  fprintf(stdout, "\n * check_proc_shape_size failed: shape %i (max %i)", proc_shape, SHAPES - 1);
  return 1; // invalid shape
 }

 if (proc_size < 0 || proc_size >= SHAPES_SIZES)
 {
//  fprintf(stdout, "\n * check_proc_shape_size failed: size %i (max %i)", proc_size, SHAPES_SIZES - 1);
  return 2; // invalid size
 }

 return 0;

}
/*
enum
{
SCAN_SHAPE_CIRCLE,
SCAN_SHAPE_RECTANGLE
};

// this function is used by both the proc and client scanners
//  - although rectangular scan currently only available for clients
// scan_shape is 0 for a circle centred on scan_x,scan_y; is 1 for a rectangle (scan_x, scan_y, scan_x2, scan_y2; range is ignored)
// range etc should have been worked out already
// scan_x and scan_y are location of scan (a proc's scan is not necessarily centred on the proc)
// *mb is a pointer to the first element of this method's mbank. we should be able to assume that there's enough space in the array (scan methods shouldn't be created otherwise)
// *bcode is the client's or proc's bcode
// *ignore_proc is a pointer to a proc to ignore (i.e. the proc doing the scanning. is NULL if a client is scanning)

int run_circular_scan(struct bcodestruct* bcode, struct procstruct* ignore_proc, al_fixed scan_x, al_fixed scan_y, al_fixed range, s16b* mb, int results_type)
{

 return; // not currently used

 int dist;

 int i;
 int pos;
 int block_x, block_y;
 int circles_to_check;
 struct procstruct* check_proc;
 struct blockstruct* bl;
 struct procstruct* scanlist_proc;
 struct procstruct* scanlist_end;
 struct procstruct* sense_circle_list_top;
 struct procstruct* list_proc;
 int number_found = 0;

 struct procstruct dummy_proc; // this uninitialised dummy procstruct is used as the first link in the chain to build the scanlist linked list below, but only a couple of its scan-relevant fields are used (they are initialised before use).

 int bcode_pos = mb [MBANK_PR_SCAN_START_ADDRESS];
 if (bcode_pos < 0) // upper-bounds checking is done later, before this value is used - see below
  return -1;

 int check = 0;

#ifdef SANITY_CHECK
 int counter_check = 0;
#endif

// TO DO: check for a maximum as well?
 int scan_number = mb [MBANK_PR_SCAN_NUMBER];
 if (scan_number <= 0)
  return -1;

// need to check the following values against mdata maximum values
 al_fixed scan_dist;
 int scan_centre_block_x;
 int scan_centre_block_y;

  scan_dist = al_itofix((int) mb [MBANK_PR_SCAN_SIZE]);
  if (scan_dist <= al_itofix(0))
   return -1;
// scan_centre values that are out-of-bounds are okay, as only blocks within bounds will be checked.
  scan_centre_block_x = fixed_to_block(scan_x);
  scan_centre_block_y = fixed_to_block(scan_y);
  circles_to_check = fixed_to_block(range) + 1;
  if (circles_to_check >= SENSE_CIRCLES)
   circles_to_check = SENSE_CIRCLES - 1;

/ *

How this works:
- While it searches for procs, it assembles a linked list of the procs it finds.

- The scanlist is a list of every proc that is detected by the scanner and passes the bitfield filters.
- dummy_proc is the first member of the list

- sense_circle_list_top is the first member of the current sense_circle being checked (may be null if none have yet been found in the current sense_circle).

TO DO: think about making the innermost sense_circle a bit large to make sure it measures an adequate area around the scan centre (otherwise very small scans may not detect the closest proc)
 - is this really an issue? the +1 in the circles_to_check calculation should deal with this (if not, could use +2?)
TO DO: think about starting sense_circle_list_top at the start of the previous sense_circle, not the current one, to deal with the blockiness of sense circles

* /

 scanlist_proc = &dummy_proc;
 scanlist_end = &dummy_proc;

// When making changes to the following code, make sure that any values that might be taken from dummy_pr are initialised before use!
 dummy_proc.scan_dist = -10000; // to make sure it is always the closest (even though it might not be if the scan's centre is displaced)
 dummy_proc.scanlist_down = NULL;

 i = 0;

 for (i = 0; i < circles_to_check; i ++)
 {
  pos = 0;
  sense_circle_list_top = scanlist_end; // the top of the list for this sense_circle is the end of the scanlist (i.e. the last member of the previous sense_circle list, or pr if nothing else found yet)

  while (sense_circles [i] [pos] [0] != 127) // sense_circles contains a list of x/y displacements for each block (pos element) at a certain distance (i element). x=127 is the last entry for a particular distance.
  {
   block_x = scan_centre_block_x + sense_circles [i] [pos] [0];
   block_y = scan_centre_block_y + sense_circles [i] [pos] [1];
   if (block_x < 0 || block_x >= w.w_block
    || block_y < 0 || block_y >= w.h_block)
    {
     pos ++;
     continue;
    }
   bl = &w.block [block_x] [block_y];
   if (bl->tag != w.blocktag) // nothing is in this block this cycle
   {
    pos ++;
    continue;
   }
   check_proc = bl->blocklist_down;

// This while loop steps through all procs in the target block:
   while(check_proc != NULL)
   {

   check++;
#ifdef SANITY_CHECK
   if (check > 1000) // check for a closed loop
   {
    fprintf(stdout, "\nError: g_method.c: run_scan(): infinite check_proc loop?");
    error_call();
   }
#endif


    if (check_proc == ignore_proc // if a proc is running this scan, ignore_proc is a pointer to itself. If a client is scanning, ignore_proc is NULL.
     || check_proc->exists != 1) // not sure that this is necessary - a non-existent proc shouldn't be on the blocklist. Should make sure anyway.
    {
     check_proc = check_proc->blocklist_down;
     continue;
    }

// apply range maximum:
    if (abs(al_fixtoi(scan_x - check_proc->x)) > al_fixtoi(range)
     || abs(al_fixtoi(scan_y - check_proc->y)) > al_fixtoi(range))
    {
     check_proc = check_proc->blocklist_down;
     continue;
    }

// ************************************************
//  the following code should match the corresponding code for rectangular scans in run_scan_rectangle()

// at least 1 of the WANT bits must match
    if ((check_proc->scan_bitfield & mb [MBANK_PR_SCAN_BITFIELD_WANT]) == 0)
    {
     check_proc = check_proc->blocklist_down;
     continue;
    }

// all of the NEED bits (if any) must match
    if ((check_proc->scan_bitfield & mb [MBANK_PR_SCAN_BITFIELD_NEED]) != mb [MBANK_PR_SCAN_BITFIELD_NEED])
    {
     check_proc = check_proc->blocklist_down;
     continue;
    }

// no REJECT bits (if any) may match
    if (check_proc->scan_bitfield & mb [MBANK_PR_SCAN_BITFIELD_REJECT])
    {
     check_proc = check_proc->blocklist_down;
     continue;
    }

//  end matching code
// ************************************************

    dist = distance(scan_y - check_proc->y, scan_x - check_proc->x);
    check_proc->scan_dist = dist;

// now insert check_proc in the proper position in the linked list, based on its distance:
    if (dist <= scan_dist)
    {
// okay, now we're going to have to traverse the list:
         list_proc = sense_circle_list_top; // remember that sense_circle_list_top is actually the end of the previous sense circle list (or is dummy_pr)

         while(TRUE)
         {
// we will assume that list_proc is closer than check_proc (although this may be based on the assumption that a proc in a closer sense_circle is always closer than one in a further sense_circle, which may not necessarily be true)
// so we check to see if list_proc is the last proc on the list, or if the next proc on the list is further away than check_proc:
          if (list_proc->scanlist_down == NULL)
          {
// We've found the end of the list, so put check_proc at the end:
           check_proc->scanlist_down = NULL;
           list_proc->scanlist_down = check_proc;
           scanlist_end = check_proc;
           break;
          }
           else
           {
            if (list_proc->scanlist_down->scan_dist > dist)
            {
// We've found the point to insert check_proc (between list_proc and list_proc->scanlist_down, which has been confirmed to exist just above)
             check_proc->scanlist_down = list_proc->scanlist_down;
             list_proc->scanlist_down = check_proc;
             break;
            }
           }
// now keep going through the list for the current sense circle
          list_proc = list_proc->scanlist_down;
#ifdef SANITY_CHECK
// Let's just make sure we don't have an infinite loop here
          counter_check++;
          if (counter_check > 5000)
          {
           fprintf(stdout, "\nError: g_method.c: run_scan(): infinite loop in scan list creation?");
           error_call();
          }
#endif
         }; // end of loop traversing the list for the current sense_circle
         number_found++;
// Note: number_found is only checked against scan_number outside of the sense_circle loop, because at this point we don't know for sure that we've found the closest procs in the sense circle
        } // end of if (dist <= scan_dist)
// now find the next proc in this block (NULL pointers are checked in the while loop condition above)
// note that the loop can continue above (if check_proc fails the bitfield filter test)
    check_proc = check_proc->blocklist_down;
   }; // end of check_proc while loop
   pos ++;
  }; // end of loop for this sense_circle

// now check to see whether we can stop scanning because we've found all we need.
// we may have found more than scan_number (as all procs in the last sense_circle to be checked will be in the scanlist)
 if (number_found >= scan_number)
  break;

 } // end of for(i) loop for all sense circles


// FINISHED running through sense circles
// now we have a big list of procs found, in order of distance (which may be slightly rough due to the blockiness of sense circles)
// need to go through them and copy their details into the bcode of the scanning proc/client:


 list_proc = dummy_proc.scanlist_down; // dummy_pr is not checked
 i = 0;
// int data1_bitfield = mb [MBANK_PR_SCAN_BITFIELD1_DATA];

// bcode_pos is initialised to the mbank value above (and confirmed to be positive, although a maximum is not checked as the maximum is checked with each list_proc)

 while(list_proc != NULL) // check against scan_number is below - could probably be used here as well as the list_proc check
 {
  if (bcode_pos >= bcode->bcode_size - 8)
   break; // for safety. Better than bounds-checking every single increment. The 8 is just a safety limit to make sure we don't get too close to the end.

  if (results_type == SCAN_RESULTS_XY)
  {
   bcode->op [bcode_pos] = al_fixtoi(list_proc->x - scan_x);
   bcode_pos ++;
   bcode->op [bcode_pos] = al_fixtoi(list_proc->y - scan_y);
   bcode_pos ++;
  }
   else // SCAN_RESULTS_INDEX (only available to CLOB method)
   {
    bcode->op [bcode_pos] = list_proc->index;
    bcode_pos ++;
   }

  if (ignore_proc != NULL
   && ignore_proc->selected != 0)
   fprintf(stdout, "\nscan found %i at %i,%i (scan_centre %i,%i)", list_proc->index, al_fixtoi(list_proc->x), al_fixtoi(list_proc->y), al_fixtoi(scan_x), al_fixtoi(scan_y));

  i ++;
  if (i >= scan_number)
   break; // scan_number is the maximum the proc/client wants to scan for
  list_proc = list_proc->scanlist_down;
 };

  if (ignore_proc != NULL
   && ignore_proc->selected != 0)
   fprintf(stdout, "\nscan result: %i", i);


 return i;

} // end of run_scan
*/

// SCANLIST_MAX_SIZE is the maximum size of the scanlist
// #define SCANLIST_MAX_SIZE 64 - now defined in header
// MAX_SCAN_NUMBER is the maximum number of procs that can be found by a scan. It's smaller than the scanlist max size because the scanlist
//  may need to hold excess members near the end of a scan

struct scanlist_nodestruct
{
 int proc_index;
 int dist;
 int previous;
 int next;
};

struct scanliststruct
{
 struct scanlist_nodestruct node [SCANLIST_MAX_SIZE];
 int array_pos;
 int list_end;
};

struct scanliststruct scanlist;

// This function performs a scan of a square of blocks
// The scan is performed in a spiral from the centre
// Assumes that:
//  x,y is the centre of a square that is within the world bounds (so that x,y can be used to get block references)
//  The square has been checked against the maximum range of the scanning method
//  max_number has been checked and is from 1 to a reasonable number
// Doesn't assume that it's a proc running the scan - could be a client etc.
int run_scan_square_spiral(struct bcodestruct* bcode, struct procstruct* ignore_proc,
                    int x, int y, int range,
                    al_fixed fix_x, al_fixed fix_y, al_fixed fix_range,
                    int max_number,
                    s16b* mb, int results_type)
{
//if (ignore_proc->selected != 0)
// fprintf(stdout, "\nscan at %i,%i (offset %i,%i) range %i ", x, y, al_fixtoi(ignore_proc->x - fix_x), al_fixtoi(ignore_proc->y - fix_y), range);


 int i;
 int block_x, block_y;

 int bcode_pos = mb [MBANK_PR_SCAN_START_ADDRESS];
 if (bcode_pos <= 0) // upper-bounds checking is done later, before this value is used - see below
  return MRES_SCAN_FAIL_ADDRESS;

// Prepare to start building the scanlist:
 scanlist.array_pos = 1; // next node will be 1
 scanlist.list_end = 0; // current end of the list is 0
 scanlist.node [0].proc_index = -1;
 scanlist.node [0].previous = -1;
 scanlist.node [0].next = -1;
 scanlist.node [0].dist = 0;

 int scan_centre_block_x = x / BLOCK_SIZE_PIXELS;
 int scan_centre_block_y = y / BLOCK_SIZE_PIXELS;

 i = 0;

// scan the centre:
 scan_block(scan_centre_block_x, scan_centre_block_y, fix_x, fix_y, range, ignore_proc, mb);

 int spiral_size = 1;
 int spiral_max_size = (range / BLOCK_SIZE_PIXELS) + 1;

 int limit;

 while(spiral_size < spiral_max_size
    && scanlist.array_pos <= max_number) // only check scanlist.array_pos against max_number once each time around the spiral
 {

  block_x = (scan_centre_block_x - spiral_size) + 1; // the top left corner is checked last
  block_y = scan_centre_block_y - spiral_size;

// One right of top left to top right:
  limit = scan_centre_block_x + spiral_size;
  while(block_x < limit)
  {
   block_x ++;
   scan_block(block_x, block_y, fix_x, fix_y, range, ignore_proc, mb);
  };

// One below top right to bottom right:
  limit = scan_centre_block_y + spiral_size;
  while(block_y < limit)
  {
   block_y ++;
   scan_block(block_x, block_y, fix_x, fix_y, range, ignore_proc, mb);
  };

// One to the left of bottom right to bottom left:
  limit = scan_centre_block_x - spiral_size;
  while(block_x > limit)
  {
   block_x --;
   scan_block(block_x, block_y, fix_x, fix_y, range, ignore_proc, mb);
  };

// One above bottom left to top left:
  limit = (scan_centre_block_y - spiral_size) + 1;
  while(block_y > limit)
  {
   block_y --;
   scan_block(block_x, block_y, fix_x, fix_y, range, ignore_proc, mb);
  };

  spiral_size ++;

 }; // end while loop


// FINISHED assembling the scanlist
// now we have a list of procs found, in order of (orthogonal) distance
// need to go through them and copy their details into the bcode of the scanning proc/client:

// Start with the node after the dummy 0 node:
 i = scanlist.node[0].next;
 int number_written = 0;

// bcode_pos is initialised to the mbank value above (and confirmed to be positive, although a maximum is not checked as the maximum is checked with each list_proc)

 while(i != -1
    && number_written < max_number)
 {
  if (bcode_pos >= bcode->bcode_size - 8)
   break; // for safety. Better than bounds-checking every single increment. The 8 is just a safety limit to make sure we don't get too close to the end.

  if (results_type == SCAN_RESULTS_XY)
  {
   bcode->op [bcode_pos] = al_fixtoi(w.proc[scanlist.node[i].proc_index].x - fix_x);
   bcode_pos ++;
   bcode->op [bcode_pos] = al_fixtoi(w.proc[scanlist.node[i].proc_index].y - fix_y);
   bcode_pos ++;
  }
   else // SCAN_RESULTS_INDEX (only available to CLOB method)
   {
    bcode->op [bcode_pos] = scanlist.node[i].proc_index;
    bcode_pos ++;
   }

/*  if (ignore_proc != NULL
   && ignore_proc->selected != 0)
   fprintf(stdout, "\nscan found %i at %i,%i (scan_centre %i,%i)", scanlist.node[i].proc_index, al_fixtoi(w.proc[scanlist.node[i].proc_index].x), al_fixtoi(w.proc[scanlist.node[i].proc_index].y), al_fixtoi(fix_x), al_fixtoi(fix_y));*/

//   fprintf(stdout, "\nscan found %i at %i,%i (scan_centre %i,%i)", scanlist.node[i].proc_index, al_fixtoi(w.proc[scanlist.node[i].proc_index].x), al_fixtoi(w.proc[scanlist.node[i].proc_index].y), al_fixtoi(fix_x), al_fixtoi(fix_y));

  i = scanlist.node[i].next;
  number_written ++;
 };

/*  if (ignore_proc != NULL
   && ignore_proc->selected != 0)
  {
   fprintf(stdout, "\nscan at %i,%i number %i range %i want %i need %i reject %i result: %i", x,y,max_number,range, mb [MBANK_PR_SCAN_BITFIELD_WANT], mb [MBANK_PR_SCAN_BITFIELD_NEED],mb [MBANK_PR_SCAN_BITFIELD_REJECT],i);
  }*/

 if (ignore_proc->selected != 0)
	{
		fprintf(stdout, "found %i",
												number_written);
	}


 return number_written;

} // end of run_scan












static void scan_block(int block_x, int block_y, al_fixed scan_x, al_fixed scan_y, int range, struct procstruct* ignore_proc, s16b* mb)
{

   int dist;

   if (block_x < 2
    || block_y < 2
    || block_x >= w.w_block - 2
    || block_y >= w.h_block - 2)
     return;

   struct blockstruct* bl = &w.block [block_x] [block_y];
   if (bl->tag != w.blocktag) // nothing is in this block this cycle
    return;

   if (scanlist.array_pos >= SCANLIST_MAX_SIZE - 1)
    return; // no room in scanlist. This is also checked in the check_proc loop below:

   struct procstruct* check_proc = bl->blocklist_down;

#ifdef SANITY_CHECK
   int check = 0; // used for sanity check below
#endif

// This while loop steps through all procs in the target block:
   while(check_proc != NULL)
   {

#ifdef SANITY_CHECK
   check++;

   if (check > 1000) // check for a closed loop
   {
    fprintf(stdout, "\nError: g_method.c: run_scan(): infinite check_proc loop?");
    error_call();
   }
#endif

    if (check_proc == ignore_proc // if a proc is running this scan, ignore_proc is a pointer to itself. If a client is scanning, ignore_proc is NULL.
     || check_proc->exists != 1) // not sure that this is necessary - a non-existent proc shouldn't be on the blocklist. Should make sure anyway.
    {
     check_proc = check_proc->blocklist_down;
     continue;
    }

// Work out distance (which is longest orthogonal distance):
    if (abs(al_fixtoi(scan_y - check_proc->y)) < abs(al_fixtoi(scan_x - check_proc->x)))
     dist = abs(al_fixtoi(scan_x - check_proc->x));
      else
       dist = abs(al_fixtoi(scan_y - check_proc->y));

// apply range maximum:
    if (dist > range)
    {
     check_proc = check_proc->blocklist_down;
     continue;
    }

// ************************************************
//  the following code should match the corresponding code for rectangular scans in run_scan_rectangle()

// at least 1 of the WANT bits must match
    if ((check_proc->scan_bitfield & mb [MBANK_PR_SCAN_BITFIELD_WANT]) == 0)
    {
     check_proc = check_proc->blocklist_down;
     continue;
    }

// all of the NEED bits (if any) must match
    if ((check_proc->scan_bitfield & mb [MBANK_PR_SCAN_BITFIELD_NEED]) != mb [MBANK_PR_SCAN_BITFIELD_NEED])
    {
     check_proc = check_proc->blocklist_down;
     continue;
    }

// no REJECT bits (if any) may match
    if (check_proc->scan_bitfield & mb [MBANK_PR_SCAN_BITFIELD_REJECT])
    {
     check_proc = check_proc->blocklist_down;
     continue;
    }

//  end matching code
// ************************************************

// now insert check_proc in the proper position in the linked list, based on its distance.
// start from the end of the list:
    int list_point = scanlist.list_end; //scanlist.array_pos;
    int new_point = scanlist.array_pos;
    if (scanlist.array_pos >= SCANLIST_MAX_SIZE - 2)
     return;
    scanlist.array_pos ++;

    scanlist.node[new_point].proc_index = check_proc->index;
    scanlist.node[new_point].dist = dist;

// search back through the list (from the end) and insert the new proc just after the next closest proc
    while (TRUE) // should never fail as node[0].dist is always 0 which should always be smaller than any other distance.
    {
#ifdef SANITY_CHECK
     if (list_point == -1)
     {
      fprintf(stdout, "\ng_method_misc.c: scan_block(): list_point is -1.");
      error_call();
     }
#endif
      if (scanlist.node[list_point].dist < dist)
      {
       scanlist.node[new_point].next = scanlist.node[list_point].next;
       scanlist.node[list_point].next = new_point;
       scanlist.node[new_point].previous = list_point;
       break; // go back to checking through the block
      }
      list_point = scanlist.node[list_point].previous;
    }; // end scanlist traversal while loop

    check_proc = check_proc->blocklist_down;
   }; // end of check_proc blocklist traversal while loop

} // end of scan_block




int run_scan_rectangle(struct bcodestruct* bcode, al_fixed scan_x, al_fixed scan_y, al_fixed scan_x2, al_fixed scan_y2, s16b* mb, int results_type)
{

 int block_x, block_y;
 struct procstruct* check_proc;
 struct blockstruct* bl;
 int number_found = 0;
 int check = 0;

 int bcode_pos = mb [MBANK_PR_SCAN_START_ADDRESS];
 if (bcode_pos <= 0) // upper-bounds checking is done later, before this value is used - see below
  return MRES_SCAN_FAIL_ADDRESS;

 int scan_number = mb [MBANK_PR_SCAN_NUMBER];
 if (scan_number <= 0)
  return MRES_SCAN_FAIL_NUMBER;
// NOTE: no maximum check. Results will just overflow into bcode (until near the end of the bcode, which is checked for below)

 al_fixed scan_rect_x1, scan_rect_y1, scan_rect_x2, scan_rect_y2;
 int scan_rect_block_x1, scan_rect_block_y1, scan_rect_block_x2, scan_rect_block_y2;

// first set scan_rect values so that x1 < x2, y1 < y2 (doesn't matter if ==)
  if (scan_x < scan_x2)
  {
   scan_rect_x1 = scan_x;
   scan_rect_x2 = scan_x2;
  }
   else
   {
    scan_rect_x1 = scan_x2;
    scan_rect_x2 = scan_x;
   }
  if (scan_y < scan_y2)
  {
   scan_rect_y1 = scan_y;
   scan_rect_y2 = scan_y2;
  }
   else
   {
    scan_rect_y1 = scan_y2;
    scan_rect_y2 = scan_y;
   }

// scanned procs will be checked against both scan_rect_x1 etc and also scan_rect_block_x1 etc
  scan_rect_block_x1 = fixed_to_block(scan_rect_x1);
  scan_rect_block_y1 = fixed_to_block(scan_rect_y1);
  scan_rect_block_x2 = fixed_to_block(scan_rect_x2);
  scan_rect_block_y2 = fixed_to_block(scan_rect_y2);

// make sure scan not *too* big:
  if (scan_rect_block_x2 - scan_rect_block_x1 >= 32)
   scan_rect_block_x2 = scan_rect_block_x1 + 32;
  if (scan_rect_block_y2 - scan_rect_block_y1 >= 32)
   scan_rect_block_y2 = scan_rect_block_y1 + 32;

// check bounds now so we don't have to do it again for each block
   if (scan_rect_block_x1 < 2)
    scan_rect_block_x1 = 2;
   if (scan_rect_block_x1 > w.w_block - 2)
    scan_rect_block_x1 = w.w_block - 2;
   if (scan_rect_block_x2 < 2)
    scan_rect_block_x2 = 2;
   if (scan_rect_block_x2 > w.w_block - 2)
    scan_rect_block_x2 = w.w_block - 2;
   if (scan_rect_block_y1 < 2)
    scan_rect_block_y1 = 2;
   if (scan_rect_block_y1 > w.h_block - 2)
    scan_rect_block_y1 = w.h_block - 2;
   if (scan_rect_block_y2 < 2)
    scan_rect_block_y2 = 2;
   if (scan_rect_block_y2 > w.h_block - 2)
    scan_rect_block_y2 = w.h_block - 2;
// don't need to check non-block dimensions as it doesn't matter if they're out of bounds

  block_x = scan_rect_block_x1;
  block_y = scan_rect_block_y1;


/*

rectangle scan is simpler than spiral scan because it just scans from left to right and doesn't have to worry about distance from a central point.
This means that we can avoid assembling a list of procs and just write any proc found directly into bcode

*/


  while(TRUE)
  {

     block_x ++;

     if (block_x > scan_rect_block_x2)
     {
      block_y ++;
      if (block_y > scan_rect_block_y2) // finished completely
       break;
      block_x = scan_rect_block_x1;
     }

// shouldn't need to bounds-check block_x/y as scan_rect_block values were checked above
#ifdef SANITY_CHECK
     if (block_x <= 1 || block_x >= w.w_block - 1
      || block_y <= 1 || block_y >= w.h_block - 1)
     {
       fprintf(stdout, "\ng_method_misc.c:run_scan(): block out of bounds in rectangle scan (%i,%i)", block_x, block_y);
       error_call();
     }
#endif

     bl = &w.block [block_x] [block_y];
     if (bl->tag != w.blocktag) // nothing is in this block this cycle
     {
      continue;
     }

    check_proc = bl->blocklist_down;

   check = 0;

// This while loop steps through all procs in the target block:
   while(check_proc != NULL)
   {

   check++;
#ifdef SANITY_CHECK
   if (check > 1000) // check for a closed loop
   {
    fprintf(stdout, "\nError: g_method.c: run_scan(): infinite check_proc loop?");
    error_call();
   }
#endif

    if (check_proc->exists != 1) // not sure that this is necessary - a non-existent proc shouldn't be on the blocklist at this point. Should make sure anyway.
    {
     check_proc = check_proc->blocklist_down;
     continue;
    }

// ************************************************
//  the following code should match the corresponding code for block scans in scan_block()

// at least 1 of the WANT bits must match
    if ((check_proc->scan_bitfield & mb [MBANK_PR_SCAN_BITFIELD_WANT]) == 0)
    {
     check_proc = check_proc->blocklist_down;
     continue;
    }

// all of the NEED bits (if any) must match
    if ((check_proc->scan_bitfield & mb [MBANK_PR_SCAN_BITFIELD_NEED]) != mb [MBANK_PR_SCAN_BITFIELD_NEED])
    {
     check_proc = check_proc->blocklist_down;
     continue;
    }

// no REJECT bits (if any) may match
    if (check_proc->scan_bitfield & mb [MBANK_PR_SCAN_BITFIELD_REJECT])
    {
     check_proc = check_proc->blocklist_down;
     continue;
    }

//  matching code ends here
// ************************************************

// for a rectangle, we check whether check_proc is inside the actual box being checked (because this probably doesn't line up exactly with the blocks being checked)
// we do this instead of checking distance
       if (check_proc->x + check_proc->shape_str->max_length > scan_rect_x1
        && check_proc->x - check_proc->shape_str->max_length < scan_rect_x2
        && check_proc->y + check_proc->shape_str->max_length > scan_rect_y1
        && check_proc->y - check_proc->shape_str->max_length < scan_rect_y2)
       {
        if (bcode_pos >= bcode->bcode_size - 8)
         break; // for safety. Better than bounds-checking every single increment. The 8 is just a safety limit to make sure we don't get too close to the end.

          if (results_type == SCAN_RESULTS_XY)
          {
           bcode->op [bcode_pos] = al_fixtoi(check_proc->x); // note that in run_scan, scan_x is subtracted, but scan_rectangle gives absolute values
           bcode_pos ++;
           bcode->op [bcode_pos] = al_fixtoi(check_proc->y);
           bcode_pos ++;
          }
           else // SCAN_RESULTS_INDEX (only available to CLOB method)
           {
            bcode->op [bcode_pos] = check_proc->index;
            bcode_pos ++;
           }

        number_found ++;

//        fprintf(stdout, "\nrectangle scan found %i at %i,%i (scan rectangle %i,%i to %i, %i)", check_proc->index, check_proc->x / GRAIN_MULTIPLY, check_proc->y / GRAIN_MULTIPLY, scan_x / GRAIN_MULTIPLY, scan_y / GRAIN_MULTIPLY, scan_x2 / GRAIN_MULTIPLY, scan_y2 / GRAIN_MULTIPLY);

        if (number_found >= scan_number)
        {
//         fprintf(stdout, "\n1st rectangle scan return number_found %i (%i)", number_found, scan_number);
         return number_found;
        }


       } // end of box check

// now find the next proc in this block (NULL pointers are checked in the while loop condition above)
// note that the loop can continue above (if check_proc fails the bitfield filter test)
    check_proc = check_proc->blocklist_down;
   }; // end of check_proc while loop that goes through all procs in the block one by one
  }; // end of while(TRUE) loop that goes through blocks one by one

// fprintf(stdout, "\n2nd rectangle scan return number_found %i (%i)", number_found, scan_number);
 return number_found;

} // end of run_scan_rectangle



extern int collision_mask [SHAPES] [COLLISION_MASK_SIZE] [COLLISION_MASK_SIZE]; // in g_shape.c


// returns index of proc if collision, -1 if not
// x/y should be a point in the map (but okay if outside; this is bounds-checked), in pixels
int check_point_collision(al_fixed x, al_fixed y)
{

 int i, j;
 int x_block = fixed_to_block(x);
 int y_block = fixed_to_block(y);
 int bx, by;
 struct blockstruct* bl;
 struct procstruct* check_proc;
 al_fixed dist;
 al_fixed angle;
 al_fixed angle_diff;
 unsigned int mask_x; // these are ints as they are used as array indices below
 unsigned int mask_y;

// check collisions:
  for (i = -1; i < 2; i ++)
  {
   bx = x_block + i;
   if (bx < 0 || bx >= w.w_block)
    continue;
   for (j = -1; j < 2; j ++)
   {
    by = y_block + j;
    if (by < 0 || by >= w.w_block)
     continue;

    bl = &w.block [bx] [by];

    if (bl->tag != w.blocktag)
     continue;

    check_proc = bl->blocklist_down;

    while(check_proc != NULL)
    {
// first do a bounding box
     if (check_proc->exists == 1
      && check_proc->x + check_proc->max_length > x
      && check_proc->x - check_proc->max_length < x
      && check_proc->y + check_proc->max_length > y
      && check_proc->y - check_proc->max_length < y)
      {

       dist = distance(y - check_proc->y, x - check_proc->x);

       angle = get_angle(y - check_proc->y, x - check_proc->x);
       angle_diff = angle - check_proc->angle;

       mask_x = MASK_CENTRE + al_fixtoi(fixed_xpart(angle_diff, dist));
       mask_x >>= COLLISION_MASK_BITSHIFT;
       mask_y = MASK_CENTRE + al_fixtoi(fixed_ypart(angle_diff, dist));
       mask_y >>= COLLISION_MASK_BITSHIFT;

       if (mask_x < COLLISION_MASK_SIZE
        && mask_y < COLLISION_MASK_SIZE)
        {
         if (collision_mask [check_proc->shape] [mask_x] [mask_y] <= check_proc->size)
         {
          return check_proc->index;
         }
        }

      } // end bounding box check

     check_proc = check_proc->blocklist_down;
    }; // end while


   }
  }

 return -1;

}


// This function is similar to check_point_collision, but instead of checking a single point precisely it checks an area (currently a square the size of a block)
//  and returns the nearest proc to the centre (measuring from the proc's centre, not its closest point)
// returns index of proc if collision, -1 if not
// x/y should be a point in the map (but okay if outside), in grain units
int check_fuzzy_point_collision(al_fixed x, al_fixed y)
{

 int i, j;
 int x_block = fixed_to_block(x);
 int y_block = fixed_to_block(y);
 int bx, by;
 struct blockstruct* bl;
 struct procstruct* check_proc;
 al_fixed dist;
 al_fixed closest_dist = al_itofix(60);
 int closest_proc_index = -1;

// check collisions:
  for (i = -1; i < 2; i ++)
  {
   bx = x_block + i;
   if (bx < 0 || bx >= w.w_block)
    continue;
   for (j = -1; j < 2; j ++)
   {
    by = y_block + j;
    if (by < 0 || by >= w.w_block)
     continue;

    bl = &w.block [bx] [by];

    if (bl->tag != w.blocktag)
     continue;

    check_proc = bl->blocklist_down;

    while(check_proc != NULL)
    {
// first do a bounding box
     if (check_proc->exists == 1
      && check_proc->x + BLOCK_SIZE_FIXED > x
      && check_proc->x - BLOCK_SIZE_FIXED < x
      && check_proc->y + BLOCK_SIZE_FIXED > y
      && check_proc->y - BLOCK_SIZE_FIXED < y)
      {

       dist = distance(y - check_proc->y, x - check_proc->x);

       if (dist < closest_dist)
       {
        closest_dist = dist;
        closest_proc_index = check_proc->index;
       }

      } // end bounding box check

     check_proc = check_proc->blocklist_down;
    }; // end while


   }
  }

 return closest_proc_index;

}



// returns index of proc if collision, -1 if not
// x/y should be a point in the map (but okay if outside; this is bounds-checked), in pixels
int check_point_collision_ignore_team(al_fixed x, al_fixed y, int ignore_team)
{

 int i, j;
 int x_block = fixed_to_block(x);
 int y_block = fixed_to_block(y);
 int bx, by;
 struct blockstruct* bl;
 struct procstruct* check_proc;
 al_fixed dist;
 al_fixed angle;
 al_fixed angle_diff;
 unsigned int mask_x; // these are ints as they are used as array indices below
 unsigned int mask_y;

// check collisions:
  for (i = -1; i < 2; i ++)
  {
   bx = x_block + i;
   if (bx < 0 || bx >= w.w_block)
    continue;
   for (j = -1; j < 2; j ++)
   {
    by = y_block + j;
    if (by < 0 || by >= w.w_block)
     continue;

    bl = &w.block [bx] [by];

    if (bl->tag != w.blocktag)
     continue;

    check_proc = bl->blocklist_down;

    while(check_proc != NULL)
    {
// do a bounding box and check team
     if (check_proc->exists == 1
      && check_proc->player_index != ignore_team
      && check_proc->x + check_proc->max_length > x
      && check_proc->x - check_proc->max_length < x
      && check_proc->y + check_proc->max_length > y
      && check_proc->y - check_proc->max_length < y)
      {

       dist = distance(y - check_proc->y, x - check_proc->x);

       angle = get_angle(y - check_proc->y, x - check_proc->x);
       angle_diff = angle - check_proc->angle;

       mask_x = MASK_CENTRE + al_fixtoi(fixed_xpart(angle_diff, dist));
       mask_x >>= COLLISION_MASK_BITSHIFT;
       mask_y = MASK_CENTRE + al_fixtoi(fixed_ypart(angle_diff, dist));
       mask_y >>= COLLISION_MASK_BITSHIFT;

       if (mask_x < COLLISION_MASK_SIZE
        && mask_y < COLLISION_MASK_SIZE)
        {
         if (collision_mask [check_proc->shape] [mask_x] [mask_y] <= check_proc->size)
         {
          return check_proc->index;
         }
        }

      } // end bounding box check

     check_proc = check_proc->blocklist_down;
    }; // end while


   }
  }

 return -1;

}



// this function is used by both the proc and client scanners when they are called with mode MSTATUS_PR_SCAN_EXAMINE
// it runs the scan method in examine mode (which gives detailed data about a single proc at a particular location)
// range etc should have been worked out already
// x and y are location of target (a proc's scan is not necessarily centred on the proc)
// *mb is a pointer to the first element of this method's mbank. we should be able to assume that there's enough space in the array (scan methods shouldn't be created otherwise)
// *bcode is the client's or proc's bcode

int run_examine(struct bcodestruct* bcode, al_fixed x, al_fixed y, al_fixed base_x, al_fixed base_y, s16b* mb)
{

 struct procstruct* check_proc;

 int bcode_pos = mb [MBANK_PR_SCAN_START_ADDRESS];
 if (bcode_pos <= 0
  || bcode_pos >= bcode->bcode_size - 8) // 8 should be a safe buffer
  return MRES_SCAN_FAIL_ADDRESS;

 int examine_proc = check_point_collision(x, y);

 if (examine_proc == -1)
  return 0;

 check_proc = &w.proc[examine_proc];

 bcode->op [bcode_pos] = al_fixtoi(check_proc->x - base_x);
 bcode_pos ++;
 bcode->op [bcode_pos] = al_fixtoi(check_proc->y - base_y);
 bcode_pos ++;
 bcode->op [bcode_pos] = al_fixtoi(check_proc->x_speed * SPEED_INT_ADJUSTMENT);
 bcode_pos ++;
 bcode->op [bcode_pos] = al_fixtoi(check_proc->y_speed * SPEED_INT_ADJUSTMENT);
 bcode_pos ++;

 return 1;

} // end of run_examine




// returns index in templ[] array of player[player_index]'s process template number [template_number]
// checks to make sure that the returned index is valid and that the template it refers to contains a loaded process bcode
// returns -1 on failure
int get_player_template_index(int player_index, int template_number)
{

   if (template_number < 0
    || template_number >= TEMPLATES_PER_PLAYER)
     return -1;

   int template_reference_index = -1;

   switch(player_index)
   {
    case 0:
     template_reference_index = FIND_TEMPLATE_P0_PROC_0 + template_number; break;
    case 1:
     template_reference_index = FIND_TEMPLATE_P1_PROC_0 + template_number; break;
    case 2:
     template_reference_index = FIND_TEMPLATE_P2_PROC_0 + template_number; break;
    case 3:
     template_reference_index = FIND_TEMPLATE_P3_PROC_0 + template_number; break;
   }

   if (template_reference_index == -1)
    return -1;

   int t = w.template_reference [template_reference_index];

   if (t == -1
    || templ[t].contents.loaded == 0
    || templ[t].type == TEMPL_TYPE_CLOSED)
     return -1;

   return t;

}

void print_player_template_name(int source_program_type, int source_index, int source_player, int template_number)
{

	int t = get_player_template_index(source_player, template_number);

	if (t == -1
		|| templ[t].contents.file_name [0] == '\0')
	{
  program_write_to_console("[unknown]", source_program_type, source_index, source_player);
		return; // failed - should print an error
	}

	program_write_to_console(templ[t].contents.file_name, source_program_type, source_index, source_player);

}



// This function creates an explosion for proc pr. It doesn't destroy the proc.
void new_proc_fail_cloud(al_fixed x, al_fixed y, int angle, int shape, int size, int player_owner)
{

 struct cloudstruct* cl = new_cloud(CLOUD_FAILED_NEW, x, y);

 if (cl != NULL)
 {
  cl->timeout = 22;
  cl->angle = angle;
  cl->colour = player_owner;

  cl->data [0] = shape;
  cl->data [1] = size;

 }

}



// This function copies bcode from a program (system, client etc) to template t
// Assumes t is valid and that the bcode represents a program of the correct type, so make sure this is checked beforehand
// Assumes template has been cleared
// Assumes source program has authority to change the template
// Does bounds-checking on start and end addresses, and made_address
// returns 1 success/0 failure
int copy_bcode_to_template(int t, struct bcodestruct* bcode, int source_program_type, int source_player_index, int template_origin, int start_address, int end_address, int name_address)
{

// fprintf(stdout, "\n t %i start_address %i end_address %i", t, start_address, end_address);
// error_call();

 int retval = check_start_end_addresses(start_address, end_address, bcode->bcode_size);

 switch(retval)
 {
  case 0: break; // success
  case 1:
   start_error(source_program_type, -1, source_player_index);
   error_string("\nFailed: bcode to template (invalid start address ");
   error_number(start_address);
   error_string(").");
   return 0;
  case 2:
   start_error(source_program_type, -1, source_player_index);
   error_string("\nFailed: bcode to template (invalid end address ");
   error_number(end_address);
   error_string(").");
   return 0;
 }

 int i, j;
 i = start_address;
 j = 0;

 while(i < end_address)
 {
  templ[t].contents.bcode.op [j] = bcode->op [i];
  i ++;
  j ++;
 };

// now zero out the rest of the template's bcode:

 while (j < templ[t].contents.bcode.bcode_size)
 {
  templ[t].contents.bcode.op [j] = 0;
  j ++;
 };

// now set up details of the template:
 templ[t].contents.origin = template_origin;
 templ[t].contents.loaded = 1;
 strcpy(templ[t].contents.file_name, "Unnamed");

// If the source program specified a name for the template, copy it here:
 if (name_address > 0
  && name_address < bcode->bcode_size - FILE_NAME_LENGTH - 1)
 {
  for (i = 0; i < FILE_NAME_LENGTH; i ++)
  {
   if (bcode->op [name_address + i] == 0
    || bcode->op [name_address + i] == '\n'
    || bcode->op [name_address + i] == '\r')
   {
    templ[t].contents.file_name [i] = 0;
    break;
   }
   templ[t].contents.file_name [i] = bcode->op [name_address + i];
  }
  templ[t].contents.file_name [FILE_NAME_LENGTH - 1] = 0;
 }

 templ[t].buttons = 1;
 templ[t].button_type [0] = TEMPL_BUTTON_CLEAR;
 if (templ[t].fixed != 0)
  templ[t].buttons = 0;

 return 1;

}

