#include <allegro5/allegro.h>
//#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>

#include "m_config.h"

#include <stdio.h>
#include <math.h>
#include <string.h>

#include "g_header.h"
#include "m_globvars.h"
#include "i_header.h"

#include "m_maths.h"
#include "m_input.h"
#include "g_misc.h"
#include "g_shape.h"
#include "i_console.h"
#include "i_buttons.h"
#include "e_inter.h"
#include "t_template.h"
#include "i_sysmenu.h"
#include "i_programs.h"
#include "i_display.h"

void draw_map(void);
void draw_proc_explode_cloud(struct cloudstruct* cl, float x, float y);
void draw_proc_fail_cloud(struct cloudstruct* cl, float x, float y);
void add_proc_diamond(float x, float y, float float_angle, struct shapestruct* sh, int size, ALLEGRO_COLOR fill_col, ALLEGRO_COLOR edge_col);
void add_proc_shape(struct procstruct* pr, float x, float y, float float_angle, struct shapestruct* sh, int shape, int size, ALLEGRO_COLOR fill_col, ALLEGRO_COLOR fill2_col, ALLEGRO_COLOR edge_col);
void add_method_base_diamond(float point_x, float point_y, float f_angle, struct shapestruct* sh, int size, int vertex, ALLEGRO_COLOR fill_col, ALLEGRO_COLOR edge_col);
void add_poly_vertex(float x, float y, ALLEGRO_COLOR col);
static unsigned int proc_rand(struct procstruct* pr, int special, int mod);
static unsigned int packet_rand(struct packetstruct* pack, int mod);

void draw_proc_shape_data(void);
void draw_proc_shape_test(void);

ALLEGRO_DISPLAY* display;
ALLEGRO_BITMAP* display_window;

ALLEGRO_BITMAP* packet_bmp;

struct fontstruct font [FONTS];

extern struct gamestruct game; // in g_game.c
extern struct shapestruct shape_dat [SHAPES] [SHAPES_SIZES]; // defined in g_shape.c
extern const struct mtypestruct mtype [MTYPES];



//#define POLY_BUFFER 10000
//#define POLY_TRIGGER 9900
// - these definitions have been moved to i_header.h

ALLEGRO_VERTEX poly_buffer [POLY_BUFFER];
int poly_pos;

//#define LINE_BUFFER 10000
//#define LINE_TRIGGER 9900
// - these definitions have been moved to i_header.h

ALLEGRO_VERTEX line_buffer [LINE_BUFFER];
int line_pos;

#define DISPLAY_LAYERS 3

#define LAYER_POLY_BUFFER 2000
#define LAYER_POLY_TRIGGER 1900
ALLEGRO_VERTEX layer_poly_buffer [DISPLAY_LAYERS] [LAYER_POLY_BUFFER];
int layer_poly_pos [DISPLAY_LAYERS];

#define LAYER_LINE_BUFFER 2000
#define LAYER_LINE_TRIGGER 1900
ALLEGRO_VERTEX layer_line_buffer [DISPLAY_LAYERS] [LAYER_LINE_BUFFER];
int layer_line_pos [DISPLAY_LAYERS];

#define FAN_BUFFER 3000
#define FAN_BUFFER_TRIGGER 2900
ALLEGRO_VERTEX fan_buffer [FAN_BUFFER];
int fan_buffer_pos;
struct fan_indexstruct
{
 int start_position; // -1 indicates end of list
 int vertices; // number of vertices in this fan
};
#define FAN_INDEX_SIZE 320
#define FAN_INDEX_TRIGGER 300
struct fan_indexstruct fan_index [FAN_INDEX_SIZE];
int fan_index_pos;

static void draw_fans(void);
static void finish_fan(void);
static void finish_fan_open(void);
static void add_fan_vertex(float x, float y, ALLEGRO_COLOR col);
static int start_fan(float x, float y, ALLEGRO_COLOR col);
static void reset_fan_index(void);


#define OUTLINE_BUFFER 1000
#define OUTLINE_TRIGGER 900
struct outline_bufferstruct
{
 int vertices; // the number of vertices to draw as a polygon
 int vertex_start; // start of this polygon in the outline_vertices array
 ALLEGRO_COLOR line_col [3];
};
struct outline_bufferstruct outline_buffer [OUTLINE_BUFFER];
int outline_pos; // current position in outline_buffer

#define OUTLINE_VERTICES 1000
#define OUTLINE_VERTEX_TRIGGER 970
float outline_vertex [OUTLINE_VERTICES]; // the actual number of vertices is half this as each vertex takes up two elements
int outline_vertex_pos; // current position in outline_vertices


#define VERTEX_LIST_SIZE 16
float vertex_list [VERTEX_LIST_SIZE] [2];

void draw_stream_beam(float x1, float by1, float x2, float y2, int col, int status, int counter, int hit);
void draw_dstream_beam(float x1, float by1, float x2, float y2, int col, int status, int counter, int hit);
void draw_thickline(float x1, float by1, float x2, float y2, float thickness, ALLEGRO_COLOR col);
//void draw_allocate_beam(float x1, float by1, float x2, float y2, int col, int counter);
void draw_allocate_beam2(float x1, float by1, float x2, float y2, int col, int prand_seed, int counter);
void draw_yield_beam(float x1, float by1, int target_proc_index, int col, int prand_seed, int counter);
void zap_line(float x1, float by1, float x2, float y2, int col, int prand_seed, int counter, int wave_amplitude);

void add_triangle(float xa, float ya, float xb, float yb, float xc, float yc, ALLEGRO_COLOR col);
void add_triangle_layer(int layer, float xa, float ya, float xb, float yb, float xc, float yc, ALLEGRO_COLOR col);
void add_outline_triangle(float xa, float ya, float xb, float yb, float xc, float yc, ALLEGRO_COLOR col1, ALLEGRO_COLOR col2);
void add_outline_triangle_layer(int layer, float xa, float ya, float xb, float yb, float xc, float yc, ALLEGRO_COLOR col1, ALLEGRO_COLOR col2);
void add_outline_diamond(float xa, float ya, float xb, float yb, float xc, float yc, float xd, float yd, ALLEGRO_COLOR col1, ALLEGRO_COLOR col2);
void add_diamond(float xa, float ya, float xb, float yb, float xc, float yc, float xd, float yd, ALLEGRO_COLOR col1);
void add_diamond_layer(int layer, float xa, float ya, float xb, float yb, float xc, float yc, float xd, float yd, ALLEGRO_COLOR col1);
void add_outline_diamond_layer(int layer, float xa, float ya, float xb, float yb, float xc, float yc, float xd, float yd, ALLEGRO_COLOR col1, ALLEGRO_COLOR col2);
void add_outline_pentagon_layer(int layer, float xa, float ya, float xb, float yb, float xc, float yc, float xd, float yd, float xe, float ye, ALLEGRO_COLOR col1, ALLEGRO_COLOR col2);
void add_outline_hexagon_layer(int layer, float xa, float ya, float xb, float yb, float xc, float yc, float xd, float yd, float xe, float ye, float xf, float yf, ALLEGRO_COLOR col1, ALLEGRO_COLOR col2);
void add_outline_square(float xa, float ya, float xb, float yb, ALLEGRO_COLOR col1, ALLEGRO_COLOR col2);
void add_outline_orthogonal_hexagon(float x, float y, float size, ALLEGRO_COLOR col1, ALLEGRO_COLOR col2);

//void add_outline_shape(float x, float y, float float_angle, struct shapestruct* sh, ALLEGRO_COLOR line_col1, ALLEGRO_COLOR line_col2, ALLEGRO_COLOR line_col3, ALLEGRO_COLOR fill_col);
void add_outline_shape(float x, float y, float float_angle, struct shapestruct* sh, ALLEGRO_COLOR fill_col, ALLEGRO_COLOR edge_col, ALLEGRO_COLOR edge_col2);
void add_outline_shape2(float x, float y, float float_angle, struct shapestruct* sh, ALLEGRO_COLOR line_col1, ALLEGRO_COLOR line_col2, ALLEGRO_COLOR line_col3, ALLEGRO_COLOR fill_col);
void add_redundancy_lines(float x, float y, float float_angle, struct shapestruct* sh, ALLEGRO_COLOR line_col);

void add_scaled_outline_shape(struct shapestruct* sh, float float_angle, float x, float y, ALLEGRO_COLOR fill_col, ALLEGRO_COLOR edge_col, float scale);
void add_scaled_outline(struct shapestruct* sh, float float_angle, float x, float y, ALLEGRO_COLOR edge_col, float scale);

void add_line(float x, float y, float xa, float ya, ALLEGRO_COLOR col);
void add_line_layer(int layer, float x, float y, float xa, float ya, ALLEGRO_COLOR col);
void add_rect_layer(int layer, float x, float y, float xa, float ya, ALLEGRO_COLOR col);

void push_to_poly_buffer(int v, ALLEGRO_COLOR col);
void push_loop_to_line_buffer(int v, ALLEGRO_COLOR col);
void push_to_layer_poly_buffer(int layer, int v, ALLEGRO_COLOR col);
void push_loop_to_layer_line_buffer(int layer, int v, ALLEGRO_COLOR col);

void add_simple_rectangle_layer(int layer, float x, float y, float length, float width, float angle, ALLEGRO_COLOR col);
void add_simple_outline_rectangle_layer(int layer, float x, float y, float length, float width, float angle, ALLEGRO_COLOR fill_col, ALLEGRO_COLOR edge_col);
void add_simple_outline_diamond_layer(int layer, float x, float y, float front_length, float back_length, float width, float angle, ALLEGRO_COLOR fill_col, ALLEGRO_COLOR edge_col);
void add_simple_outline_triangle_layer(int layer, float x, float y, float angle_1, float length_1, float angle_2, float length_2, float angle_3, float length_3, ALLEGRO_COLOR col1, ALLEGRO_COLOR col2);

void check_buffer_sizes(void);
void draw_from_buffers(void);

//void draw_packet(float x, float y, int angle, float size, int random_size, ALLEGRO_COLOR col1, ALLEGRO_COLOR col2, ALLEGRO_COLOR col3);


extern struct viewstruct view;

void run_display(void)
{

// draw_proc_shape_data(); //- see below (this generates an image used in the manual)

// draw_proc_shape_test();

 int p, pk, c;
 struct procstruct* pr;
 struct packetstruct* pack;
 struct cloudstruct* cl;
 float x, y, x2, y2;
// static float gx = 400;
 int i, j;
 int shade;

 al_set_target_bitmap(al_get_backbuffer(display));
 if (settings.edit_window == EDIT_WINDOW_CLOSED)
		al_set_clipping_rectangle(0, 0, view.window_x, view.window_y);
	  else
		  al_set_clipping_rectangle(0, 0, settings.editor_x_split, view.window_y);
// al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_ZERO);
 al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA);

// al_set_target_bitmap(display_window);
 al_set_target_bitmap(al_get_backbuffer(display));
 al_clear_to_color(colours.world_background);//colours.base [COL_GREY] [SHADE_MIN]);


#define SHOW_BLOCKS

// the following code draws the blocks on the screen. It's horribly unoptimised
#ifdef SHOW_BLOCKS
//  int block_size = BLOCK_SIZE_PIXELS;
  int screen_width_in_blocks = view.window_x / BLOCK_SIZE_PIXELS + 2;
  int screen_height_in_blocks = view.window_y / BLOCK_SIZE_PIXELS + 2;
  int bx;
  int by;
  int k;
  struct blockstruct* bl;
  int camera_offset_x = al_fixtoi(view.camera_x);// + view.centre_x);// % BLOCK_SIZE_PIXELS;// - MAGIC_X_OFFSET;
  int camera_offset_y = al_fixtoi(view.camera_y);// + view.centre_y);// % BLOCK_SIZE_PIXELS;// - MAGIC_Y_OFFSET;
  camera_offset_x %= BLOCK_SIZE_PIXELS;
  camera_offset_y %= BLOCK_SIZE_PIXELS;
  camera_offset_x -= al_fixtoi(view.centre_x) % BLOCK_SIZE_PIXELS;
  camera_offset_y -= al_fixtoi(view.centre_y) % BLOCK_SIZE_PIXELS;

//  camera_offset_x += 64;
//  camera_offset_y += 64;
//   bx = fixed_to_block(view.camera_x - view.centre_x + al_itofix(64)) + i;
  //int camera_offset_x = abs((al_fixtoi(view.camera_x - view.centre_x) +63) % BLOCK_SIZE_PIXELS);// - MAGIC_X_OFFSET;
//  int camera_offset_y = abs((al_fixtoi(view.camera_y - view.centre_y) +63) % BLOCK_SIZE_PIXELS);// - MAGIC_Y_OFFSET;
//  int camera_offset_x = ((view.camera_x >> GRAIN) - (view.window_x/2)) % block_size;// - MAGIC_X_OFFSET;
//  int camera_offset_y = ((view.camera_y >> GRAIN) - (view.window_y/2)) % block_size;// - MAGIC_Y_OFFSET;
  int camera_edge_x1 = 0;
  int camera_edge_x2 = view.window_x;
  int camera_edge_y1 = 0;
  int camera_edge_y2 = view.window_y;
#define PROC_DRAW_SIZE 100
  struct procstruct* proc_draw_list [PROC_DRAW_SIZE];
  float proc_draw_x [PROC_DRAW_SIZE]; // used to store positions of drawn procs in case they need selection circles or something later
  float proc_draw_y [PROC_DRAW_SIZE];
  int proc_draw_count = 0;

 al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA);
 //al_set_blender(ALLEGRO_DEST_MINUS_SRC, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA);

// reset the line/poly drawing buffers (must come before these are used)
 poly_pos = 0;
 line_pos = 0;
 for (i = 0; i < DISPLAY_LAYERS; i ++)
 {
  layer_poly_pos [i] = 0;
  layer_line_pos [i] = 0;
 }
 outline_pos = 0;
 outline_vertex_pos = 0;
 reset_fan_index();

#define EDGE_SIZE (BLOCK_SIZE_PIXELS * 2)

#define EDGE_LINE_COL colours.base [COL_GREY] [SHADE_LOW]

 int clip_x1 = 0;
 int clip_y1 = 0;
 int clip_x2 = view.window_x;
 int clip_y2 = view.window_y;
 int reset_clipping = 0;

// First work out the y camera edges, because these are used when displaying the x lines as well:
//  camera_edge_y1 = (view.camera_y / GRAIN_MULTIPLY) - (view.window_y / 2);
  camera_edge_y1 = al_fixtoi(view.camera_y - view.centre_y);
  if (camera_edge_y1 < EDGE_SIZE)
  {
   camera_edge_y1 = (camera_edge_y1 * -1) + EDGE_SIZE;
   camera_edge_y2 = view.window_y;
  }
   else
   {
    camera_edge_y1 = 0;
    camera_edge_y2 = al_fixtoi(view.camera_y + view.centre_y);
    if (camera_edge_y2 > view.w_y_pixel - EDGE_SIZE)
    {
     camera_edge_y2 = view.window_y - (camera_edge_y2 - view.w_y_pixel) - EDGE_SIZE;
    }
   }

// check whether the left side of the map is visible:
//  camera_edge_x1 = (view.camera_x / GRAIN_MULTIPLY) - (view.window_x / 2);
  camera_edge_x1 = al_fixtoi(view.camera_x - view.centre_x);
  if (camera_edge_x1 < EDGE_SIZE)
  {
   camera_edge_x1 = (camera_edge_x1 * -1) + EDGE_SIZE;
     add_line(camera_edge_x1, camera_edge_y1, camera_edge_x1, camera_edge_y2,
              EDGE_LINE_COL);
   clip_x1 = camera_edge_x1 - 1;
   reset_clipping = 1;

  }
   else
   {
    camera_edge_x1 = 0;
    camera_edge_x2 = al_fixtoi(view.camera_x + view.centre_x);
    if (camera_edge_x2 > view.w_x_pixel - EDGE_SIZE)
    {
     camera_edge_x2 = view.window_x - (camera_edge_x2 - view.w_x_pixel) - EDGE_SIZE;
     add_line(camera_edge_x2, camera_edge_y1, camera_edge_x2, camera_edge_y2,
              EDGE_LINE_COL);
     clip_x2 = camera_edge_x2;
     reset_clipping = 1;
    }
     else
      camera_edge_x2 = view.window_x;
   }

// Now draw the y lines:
  if (al_fixtoi(view.camera_y - view.centre_y) < EDGE_SIZE)
  {
   add_line(camera_edge_x1, camera_edge_y1, camera_edge_x2, camera_edge_y1,
            EDGE_LINE_COL);
   clip_y1 = camera_edge_y1 - 1;
   reset_clipping = 1;
  }
   else
   {
    if (al_fixtoi(view.camera_y + view.centre_y) > view.w_y_pixel - EDGE_SIZE)
    {
     add_line(camera_edge_x1, camera_edge_y2, camera_edge_x2, camera_edge_y2,
              EDGE_LINE_COL);
     clip_y2 = camera_edge_y2;
     reset_clipping = 1;
    }
   }

/*
// check whether the left side of the map is visible:
//  camera_edge_x1 = (view.camera_x / GRAIN_MULTIPLY) - (view.window_x / 2);
  camera_edge_x1 = al_fixtoi(view.camera_x - view.centre_x);
  if (camera_edge_x1 < EDGE_SIZE)
  {
   camera_edge_x1 = camera_edge_x1 * -1;
//   al_draw_filled_rectangle(0, 0, camera_edge_x1 + EDGE_SIZE, view.window_y, colours.base [COL_BLUE] [SHADE_MIN]);
     add_line(
           colours.base [COL_BLUE] [SHADE_MIN]);

  }
   else
   {
    camera_edge_x1 = 0;
//    camera_edge_x2 = (view.camera_x / GRAIN_MULTIPLY) + (view.window_x / 2);
    camera_edge_x2 = al_fixtoi(view.camera_x + view.centre_x);
    if (camera_edge_x2 > view.w_x_pixel - EDGE_SIZE)
    {
     camera_edge_x2 = view.window_x - (camera_edge_x2 - view.w_x_pixel) - EDGE_SIZE;
     al_draw_filled_rectangle(camera_edge_x2, 0, view.window_x, view.window_y, colours.base [COL_BLUE] [SHADE_MIN]);
    }
     else
      camera_edge_x2 = view.window_x;
   }

//  camera_edge_y1 = (view.camera_y / GRAIN_MULTIPLY) - (view.window_y / 2);
  camera_edge_y1 = al_fixtoi(view.camera_y - view.centre_y);
  if (camera_edge_y1 < EDGE_SIZE)
  {
   camera_edge_y1 = camera_edge_y1 * -1;
   al_draw_filled_rectangle(camera_edge_x1, 0, camera_edge_x2, camera_edge_y1 + EDGE_SIZE, colours.base [COL_BLUE] [SHADE_MIN]);
  }
   else
   {
//    camera_edge_y2 = (view.camera_y / GRAIN_MULTIPLY) + (view.window_y / 2);
    camera_edge_y2 = al_fixtoi(view.camera_y + view.centre_y);
    if (camera_edge_y2 > view.w_y_pixel - EDGE_SIZE)
    {
     camera_edge_y2 = view.window_y - (camera_edge_y2 - view.w_y_pixel) - EDGE_SIZE;
     al_draw_filled_rectangle(camera_edge_x1, camera_edge_y2, camera_edge_x2, view.window_y, colours.base [COL_BLUE] [SHADE_MIN]);
    }
   }
*/

  if (reset_clipping == 1)
   al_set_clipping_rectangle(clip_x1, clip_y1, clip_x2, clip_y2);

  float bx2, by2;

  for (i = -1; i < screen_width_in_blocks; i ++)
  {

   bx = fixed_to_block(view.camera_x) - fixed_to_block(view.centre_x) + i;

   if (bx < 0)
    continue;
   if (bx >= w.w_block)
    break;

   for (j = -1; j < screen_height_in_blocks; j ++)
   {
    by = fixed_to_block(view.camera_y) - fixed_to_block(view.centre_y) + j;

    if (by < 0)
     continue;
    if (by >= w.h_block)
     break;

    bl = &w.block [bx] [by];

/*    if (bl->block_type == BLOCK_SOLID)
     al_draw_rectangle((i * block_size) - camera_offset_x, (j * block_size) - camera_offset_y,
                      block_size + (i * block_size) - camera_offset_x, block_size + (j * block_size) - camera_offset_y, colours.base [COL_BLUE] [SHADE_MAX], 2);
      else*/
    if (bl->block_type != BLOCK_SOLID)
      {
//       al_draw_rectangle((i * block_size) - camera_offset_x, (j * block_size) - camera_offset_y,
//                      block_size + (i * block_size) - camera_offset_x, block_size + (j * block_size) - camera_offset_y, base_col [COL_BLUE] [SHADE_LOW], 1);
       bx2 = ((i * BLOCK_SIZE_PIXELS) - camera_offset_x);
       by2 = ((j * BLOCK_SIZE_PIXELS) - camera_offset_y);
//       al_draw_circle(bx2 + bl->node_x [0], by2 + bl->node_y [0], 5, base_col [COL_GREY] [SHADE_MED], 1);

       float nsize;
       int nfillcol;

       for (k = 0; k < 9; k ++)
       {
//        al_draw_textf(font, base_col [COL_GREEN] [SHADE_MIN], bx2, by2 + k * 12, 0, "%i, %i, %i", bx2 + bl->node_x [k], by2 + bl->node_y [k], bl->node_size [k]);

        nsize = bl->node_size [k];
        nfillcol = 0;

        if (bl->node_disrupt_timestamp [k] > w.world_time)
        {
         nsize += (bl->node_disrupt_timestamp [k] - w.world_time) / 2;
         nfillcol = (bl->node_disrupt_timestamp [k] - w.world_time) / 2;
         if (nfillcol >= BACK_COL_FADE)
          nfillcol = BACK_COL_FADE - 1;

        }

        nsize += 0.5;

       add_outline_orthogonal_hexagon(bx2 + bl->node_x [k], by2 + bl->node_y [k], nsize,
                                      colours.back_fill [bl->node_team_col [k]] [bl->node_col_saturation [k]] [nfillcol],
                                      colours.back_fill [bl->node_team_col [k]] [bl->node_col_saturation [k] / 2] [nfillcol]);
//                                      colours.back_line [bl->node_team_col [k]] [bl->node_col_saturation [k]]);

//        al_draw_textf(font, base_col [COL_GREEN] [SHADE_MAX], bx2 + bl->node_x [k], by2 + bl->node_y [k], 0, "%i:%i", bl->node_team_col [k], bl->node_col_saturation [k]);
       }



      }



//    al_draw_rectangle(100, 100, 200, 200, basic_colour [2], 1);
//    al_draw_textf(font, basic_colour [5], (i * 64) - camera_offset_x + 5, (j * 64) - camera_offset_y + 5, 0, "%i: %i", bl->tag, w.blocktag);
//    al_draw_textf(font, basic_colour [5], (i * 64) - camera_offset_x + 5, (j * 64) - camera_offset_y + 5, 0, "%i (%i, %i)", bl->alt, bl->slope_accel_x, bl->slope_accel_y);
//    al_draw_textf(font, base_col [COL_YELLOW] [SHADE_HIGH], (i * block_size) - camera_offset_x + 5, (j * block_size) - camera_offset_y + 25, 0, "(%i, %i)", bx, by);

//    al_draw_textf(font, base_col [COL_YELLOW] [SHADE_HIGH], (i * block_size) - camera_offset_x + 5, (j * block_size) - camera_offset_y + 25, 0, "(%i)", bl->block_type);

    if (bl->tag == w.blocktag)
    {
     pr = bl->blocklist_down;
     k = 0;
     while(pr != NULL)
     {
//      al_draw_textf(font, base_col [COL_GREEN] [SHADE_MAX], (i * block_size) - camera_offset_x + 5, (j * block_size) - camera_offset_y + 45 + (k * 10), 0, "%i", pr->index);
      pr = pr->blocklist_down;
      k ++;
     };
    }

   }
  }

  draw_from_buffers();

  if (reset_clipping == 1)
   al_set_clipping_rectangle(0, 0, view.window_x, view.window_y);

/*
  for (i = 0; i < screen_width_in_blocks; i ++)
  {
//   bx = fixed_to_block(view.camera_x - view.centre_x) + i;
   bx = fixed_to_block(view.camera_x) - fixed_to_block(view.centre_x) + i;
//   bx = fixed_to_block(view.camera_x) - fixed_to_block(view.centre_x) + i;

   for (j = 0; j < screen_height_in_blocks; j ++)
   {
//    by = fixed_to_block(view.camera_y - view.centre_y) + j;
//    by = fixed_to_block(view.camera_y - view.centre_y) + j;
    by = fixed_to_block(view.camera_y) - fixed_to_block(view.centre_y) + i;

    bx2 = ((i * BLOCK_SIZE_PIXELS) - camera_offset_x);
    by2 = ((j * BLOCK_SIZE_PIXELS) - camera_offset_y);

    al_draw_textf(font, base_col [COL_GREY] [SHADE_MAX], bx2, by2, ALLEGRO_ALIGN_CENTRE, "%i,%i(%i,%i)", bx, by, i, j);

   }
  }


  al_draw_textf(font, base_col [COL_YELLOW] [SHADE_MAX], 100, 100, 0, "(%i, %i) (%f:%f, %f:%f)", camera_offset_x, camera_offset_y, al_fixtof(view.camera_x), al_fixtof(view.centre_x), al_fixtof(view.camera_y), al_fixtof(view.centre_y));
*/

#endif

 int vertex;
 float vx, vy;
 int team_colour;
 float dg_vx;
 float dg_vy;

 int fill_colour;
 ALLEGRO_COLOR method_fill_col;
// ALLEGRO_COLOR join_fill_col;
 ALLEGRO_COLOR method_edge_col;
 ALLEGRO_COLOR base_fill_col;
 ALLEGRO_COLOR link_fill_col;
 ALLEGRO_COLOR main_fill_col;
 ALLEGRO_COLOR main_edge_col;
 ALLEGRO_COLOR method_medium_col;

// al_lock_bitmap(al_get_backbuffer(display), ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_READWRITE);

 for (p = 0; p < w.max_procs; p ++)
 {
// TO DO: optimise this to use blocks instead of searching the entire proc array!
  if (w.proc[p].exists != 1)
   continue;

  pr = &w.proc [p];

//  pr_player = &w.player [pr->player_index];

  x = al_fixtof(pr->x - view.camera_x); // x is float; pr->x and view.camera_x are int
  y = al_fixtof(pr->y - view.camera_y); // x is float; pr->x and view.camera_x are int
  x += view.window_x / 2;
  y += view.window_y / 2;

//  x = ((pr->x - view.camera_x) >> GRAIN) + (view.window_x/2);
//  y = ((pr->y - view.camera_y) >> GRAIN) + (view.window_y/2);

  if (x < -200 || x > view.window_x + 200
   || y < -200 || y > view.window_y + 200)
    continue;

   team_colour = pr->player_index;
   shade = PROC_FILL_SHADES - 1;
   if (pr->hp < pr->hp_max)
   {
    shade = (pr->hp * 15) / pr->hp_max;
   }
   fill_colour = 0;
   if (pr->hit > 0)
    fill_colour = 1;

// if the proc has just been created, we show a special graphic thing:
   if (pr->lifetime < 14)
   {
    shade = pr->lifetime;
    if (shade > CLOUD_SHADES - 1)
     shade = CLOUD_SHADES - 1;
    method_fill_col = colours.packet [team_colour] [shade];
    method_medium_col = colours.packet [team_colour] [shade / 2];
//    join_fill_col = colours.packet [team_colour] [shade];
    method_edge_col = colours.packet [team_colour] [shade / 2];
    base_fill_col = colours.packet [team_colour] [shade / 2];
    link_fill_col = colours.packet [team_colour] [shade / 2];
    main_fill_col = colours.packet [team_colour] [shade];
    main_edge_col = colours.packet [team_colour] [shade / 2];
   }
    else
    {
     method_fill_col = colours.proc_fill [team_colour] [PROC_FILL_SHADES - 1] [0];
//     join_fill_col = colours.proc_fill [team_colour] [PROC_FILL_SHADES - 5] [0];
     method_edge_col = colours.team [team_colour] [TCOL_METHOD_EDGE];
     method_medium_col = colours.proc_fill [team_colour] [3] [0];
     if (pr->redundancy == 0)
      base_fill_col = colours.team [team_colour] [TCOL_FILL_BASE];
       else
        base_fill_col = colours.proc_fill [team_colour] [12] [fill_colour];
     link_fill_col = colours.team [team_colour] [TCOL_FILL_BASE];
     main_fill_col = colours.proc_fill [team_colour] [shade] [fill_colour];
     main_edge_col = colours.team [team_colour] [TCOL_MAIN_EDGE];

    }


   add_proc_shape(pr, x, y, pr->float_angle, &shape_dat [pr->shape] [pr->size], pr->shape, pr->size,
                  main_fill_col, base_fill_col, main_edge_col);

//    al_draw_textf(font, base_col [COL_YELLOW] [SHADE_HIGH], x + 40, y + 40, 0, "col %i, %i, %i", team_colour, shade, fill_colour);

//    al_draw_textf(font, base_col [COL_YELLOW] [SHADE_HIGH], x + 40, y + 40, 0, "sel %i, %i", pr->selected, pr->map_selected);

/*
  if (pr->redundancy > 0)
  {
   add_redundancy_lines(x, y, pr->float_angle, &shape_dat [pr->shape] [pr->size], main_fill_col);
  }*/


//  add_outline_shape2(x, y, pr->float_angle, &shape_dat [pr->shape] [pr->size],
//                    team_colours [team_colour] [TCOL_LINE1], team_colours [team_colour] [TCOL_LINE2], team_colours [team_colour] [TCOL_LINE3], team_colours [team_colour] [fill_colour]);

//  if (pr->index == 0)
  //{
//   add_line(x, y, ((w.group[pr->group].x - view.camera_x) >> GRAIN) + (view.window_x/2), ((w.group[pr->group].y - view.camera_y) >> GRAIN) + (view.window_y/2), base_col [COL_YELLOW] [SHADE_HIGH]);
//   add_line(((pr->debug_x - view.camera_x) >> GRAIN) + (view.window_x/2), ((pr->debug_y - view.camera_y) >> GRAIN) + (view.window_y/2), ((w.group[pr->group].x - view.camera_x) >> GRAIN) + (view.window_x/2), ((w.group[pr->group].y - view.camera_y) >> GRAIN) + (view.window_y/2), base_col [COL_RED] [SHADE_HIGH]);
//   add_line(((pr->debug_x2 - view.camera_x) >> GRAIN) + (view.window_x/2), ((pr->debug_y2 - view.camera_y) >> GRAIN) + (view.window_y/2), ((w.group[pr->group].x - view.camera_x) >> GRAIN) + (view.window_x/2), ((w.group[pr->group].y - view.camera_y) >> GRAIN) + (view.window_y/2), base_col [COL_RED] [SHADE_HIGH]);

//   add_line(((pr->debug_x - view.camera_x) >> GRAIN) + (view.window_x/2), ((pr->debug_y - view.camera_y) >> GRAIN) + (view.window_y/2), ((w.group[pr->group].x - view.camera_x) >> GRAIN) + (view.window_x/2), ((w.group[pr->group].y - view.camera_y) >> GRAIN) + (view.window_y/2), base_col [COL_RED] [SHADE_HIGH]);
//   add_line(((pr->debug_x2 - view.camera_x) >> GRAIN) + (view.window_x/2), ((pr->debug_y2 - view.camera_y) >> GRAIN) + (view.window_y/2), ((w.group[pr->group].x - view.camera_x) >> GRAIN) + (view.window_x/2), ((w.group[pr->group].y - view.camera_y) >> GRAIN) + (view.window_y/2), base_col [COL_RED] [SHADE_HIGH]);

//  }

//    al_draw_textf(font, base_col [COL_GREY] [SHADE_MAX], x, y, 0, "(%i)", pr->angle);


//  add_outline_shape(x, y, pr->angle, &shape_dat [pr->shape] [pr->size],
//                       COL_BLUE, SHADE_MIN, COL_BLUE, SHADE_MIN);

// void add_outline_shape(int x, int y, int angle, struct shapestruct* sh, ALLEGRO_COLOR line_col1, ALLEGRO_COLOR line_col2, ALLEGRO_COLOR line_col3, ALLEGRO_COLOR fill_col)


//                       COL_GREEN, shade, COL_GREEN, SHADE_MIN);

//  add_line(x, y, x + (pr->x_speed / 100), y + (pr->y_speed / 100), COL_YELLOW, SHADE_HIGH);


// This add_line draws a speed line. Can be removed.
//  add_line(x, y, ((pr->provisional_x - view.camera_x) >> GRAIN) + (view.window_x/2), ((pr->provisional_y - view.camera_y) >> GRAIN) + (view.window_y/2), base_col [COL_YELLOW] [SHADE_HIGH]);


//    al_draw_textf(font, base_col [COL_YELLOW] [SHADE_HIGH], x, y, 0, "(%i, %i)", pr->x_block, pr->y_block);

   float dist_front; //, dist_side, dist_back;
   float f_angle;// = pr->float_angle + angle_to_radians(pr->method [pr->vertex_method [vertex]].ex_angle);

   float pg_vx, pg_vy;

// al_lock_bitmap(al_get_backbuffer(display), ALLEGRO_PIXEL_FORMAT_ANY, ALLEGRO_LOCK_READWRITE);
// if (FALSE)
  for (vertex = 0; vertex < pr->shape_str->vertices; vertex++)
  {
//    vx = x + cos(angle_to_radians(pr->angle + pr->shape_str->vertex_angle [vertex])) * pr->shape_str->vertex_dist_pixel [vertex];
//    vy = y + sin(angle_to_radians(pr->angle + pr->shape_str->vertex_angle [vertex])) * pr->shape_str->vertex_dist_pixel [vertex];
//    vx = x + xpart(pr->angle + pr->shape_str->vertex_angle [vertex], pr->shape_str->vertex_dist_pixel [vertex]);
//    vy = y + ypart(pr->angle + pr->shape_str->vertex_angle [vertex], pr->shape_str->vertex_dist_pixel [vertex]);
//    al_draw_textf(font, base_col [COL_YELLOW] [SHADE_HIGH], vx, vy, 0, "%i", vertex);

//    f_angle = pr->float_angle + pr->shape_str->vertex_angle_float [vertex];
//    float vx2 = x + cos(pr->float_angle + pr->shape_str->vertex_angle_float [vertex]) * pr->shape_str->vertex_dist_pixel [vertex] * 1.2;
//    float vy2 = y + sin(pr->float_angle + pr->shape_str->vertex_angle_float [vertex]) * pr->shape_str->vertex_dist_pixel [vertex] * 1.2;
//    al_draw_textf(font, base_col [COL_YELLOW] [SHADE_HIGH], vx2, vy2, 0, "%i %f", vertex, pr->float_angle);

#ifdef SHOW_VERTEX_EXANGLES
// use this code to check that vertex external angles are being set properly
    f_angle = pr->float_angle + pr->shape_str->vertex_angle_float [vertex];

    float vx2 = x + cos(pr->float_angle + pr->shape_str->vertex_angle_float [vertex]) * pr->shape_str->vertex_dist_pixel [vertex] * 1.2;
    float vy2 = y + sin(pr->float_angle + pr->shape_str->vertex_angle_float [vertex]) * pr->shape_str->vertex_dist_pixel [vertex] * 1.2;

    float vx3 = vx2 + cos(f_angle + fixed_to_radians(pr->shape_str->external_angle [EXANGLE_LOW] [vertex])) * 20;
    float vy3 = vy2 + sin(f_angle + fixed_to_radians(pr->shape_str->external_angle [EXANGLE_LOW] [vertex])) * 20;

    if (pr->shape_str->external_angle_wrapped [vertex] == 0)
     al_draw_line(vx2, vy2, vx3, vy3, colours.team [team_colour] [TCOL_FILL_BASE], 1);
      else
       al_draw_line(vx2, vy2, vx3, vy3, colours.team [team_colour] [TCOL_FILL_BASE], 1);

    vx3 = vx2 + cos(f_angle + fixed_to_radians(pr->shape_str->external_angle [EXANGLE_HIGH] [vertex])) * 20;
    vy3 = vy2 + sin(f_angle + fixed_to_radians(pr->shape_str->external_angle [EXANGLE_HIGH] [vertex])) * 20;

    if (pr->shape_str->external_angle_wrapped [vertex] == 1)
     al_draw_line(vx2, vy2, vx3, vy3, colours.team [team_colour] [TCOL_FILL_BASE], 1);
      else
       al_draw_line(vx2, vy2, vx3, vy3, colours.team [team_colour] [TCOL_FILL_BASE], 1);
#endif

   if (pr->vertex_method [vertex] != -1)
   {
//    vx = x + cos(angle_to_radians(pr->angle + pr->shape_str->vertex_angle [vertex])) * pr->shape_str->vertex_dist_pixel [vertex];
//    vy = y + sin(angle_to_radians(pr->angle + pr->shape_str->vertex_angle [vertex])) * pr->shape_str->vertex_dist_pixel [vertex];
    vx = x + cos(pr->float_angle + pr->shape_str->vertex_angle_float [vertex]) * (pr->shape_str->vertex_dist_pixel [vertex] - 1);
    vy = y + sin(pr->float_angle + pr->shape_str->vertex_angle_float [vertex]) * (pr->shape_str->vertex_dist_pixel [vertex] - 1);
    switch(pr->method [pr->vertex_method [vertex]].type)
    {
     case MTYPE_PR_PACKET:
     f_angle = pr->float_angle + pr->shape_str->vertex_angle_float [vertex];

     pg_vx = vx;// + cos(f_angle) * 3;
     pg_vy = vy;// + sin(f_angle) * 3;

     add_method_base_diamond(pg_vx, pg_vy, f_angle, pr->shape_str, pr->size, vertex, method_fill_col, method_edge_col);


     f_angle = pr->float_angle + fixed_to_radians(pr->method [pr->vertex_method [vertex]].ex_angle); // remember that ex_angle incorporates the vertex angle
/*
     float base_pm_core_front = 3;// + pr->method [pr->vertex_method [vertex]].extension [MEX_PR_PACKET_SPEED];
     float base_pm_core_side = 5;// + pr->method [pr->vertex_method [vertex]].extension [MEX_PR_PACKET_POWER] * 2;
     float base_pm_core_back = 8;// + pr->method [pr->vertex_method [vertex]].extension [MEX_PR_PACKET_RANGE] * 2;
     float base_pm_out_front = 10;// + pr->method [pr->vertex_method [vertex]].extension [MEX_PR_PACKET_SPEED] * 3;
*/

#define PM_CORE_FRONT 3
#define PM_CORE_SIDE (4 + pr->method [pr->vertex_method [vertex]].extension [MEX_PR_PACKET_POWER])
#define PM_CORE_BACK (6 + pr->method [pr->vertex_method [vertex]].extension [MEX_PR_PACKET_RANGE])

#define PM_OUT_FRONT (8 + pr->method [pr->vertex_method [vertex]].extension [MEX_PR_PACKET_SPEED])
// remember that these macros are also used for DPACKET!

/*
#define PM_CORE_FRONT base_pm_core_front
#define PM_CORE_SIDE base_pm_core_side
#define PM_CORE_BACK base_pm_core_back

#define PM_OUT_FRONT base_pm_out_front
*/


#define PM_SIDE_PROPORTION 0.2

     pg_vx -= cos(f_angle) * (3 + (float) pr->method [pr->vertex_method [vertex]].data [MDATA_PR_PACKET_RECYCLE]) / 3;
     pg_vy -= sin(f_angle) * (3 + (float) pr->method [pr->vertex_method [vertex]].data [MDATA_PR_PACKET_RECYCLE]) / 3;

     float pm_front_x = pg_vx + cos(f_angle) * PM_CORE_FRONT;
     float pm_front_y = pg_vy + sin(f_angle) * PM_CORE_FRONT;
     float pm_right_x = pg_vx + cos(f_angle + PI/2) * PM_CORE_SIDE;
     float pm_right_y = pg_vy + sin(f_angle + PI/2) * PM_CORE_SIDE;
     float pm_left_x = pg_vx + cos(f_angle - PI/2) * PM_CORE_SIDE;
     float pm_left_y = pg_vy + sin(f_angle - PI/2) * PM_CORE_SIDE;
     float pm_back_x = pg_vx - cos(f_angle) * PM_CORE_BACK;
     float pm_back_y = pg_vy - sin(f_angle) * PM_CORE_BACK;


     if (pr->method [pr->vertex_method [vertex]].data [MDATA_PR_PACKET_RECYCLE] == 0)
     {
           add_outline_diamond_layer(1,
                               pm_front_x + cos(f_angle) * PM_OUT_FRONT,
                               pm_front_y + sin(f_angle) * PM_OUT_FRONT,
                               pm_left_x + (pm_left_x - pm_back_x) * PM_SIDE_PROPORTION,
                               pm_left_y + (pm_left_y - pm_back_y) * PM_SIDE_PROPORTION,
                               pm_back_x,
                               pm_back_y,
                               pm_right_x + (pm_right_x - pm_back_x) * PM_SIDE_PROPORTION,
                               pm_right_y + (pm_right_y - pm_back_y) * PM_SIDE_PROPORTION,
                               method_fill_col,
                               method_edge_col);

      break;
     }



     add_outline_diamond_layer(1,
                               pm_front_x,
                               pm_front_y,
                               pm_left_x,
                               pm_left_y,
                               pm_back_x,
                               pm_back_y,
                               pm_right_x,
                               pm_right_y,
                               method_fill_col,
                               method_edge_col);



     float recoil_displace_x = 0;
     float recoil_displace_y = 0;

//     if (pr->method [pr->vertex_method [vertex]].data [MDATA_PR_PACKET_RECYCLE] > 0)
     {
      recoil_displace_x = ((pm_left_x - pm_front_x) * (float) pr->method [pr->vertex_method [vertex]].data [MDATA_PR_PACKET_RECYCLE]) / PACKET_RECYCLE_TIME / 2;
      recoil_displace_y = ((pm_left_y - pm_front_y) * (float) pr->method [pr->vertex_method [vertex]].data [MDATA_PR_PACKET_RECYCLE]) / PACKET_RECYCLE_TIME / 2;
     }

     add_outline_diamond_layer(1,
                               pm_front_x + recoil_displace_x,
                               pm_front_y + recoil_displace_y,
                               pm_left_x + recoil_displace_x,
                               pm_left_y + recoil_displace_y,
                               pm_left_x + recoil_displace_x + (pm_left_x - pm_back_x) * PM_SIDE_PROPORTION,
                               pm_left_y + recoil_displace_y + (pm_left_y - pm_back_y) * PM_SIDE_PROPORTION,
                               pm_front_x + (cos(f_angle) * PM_OUT_FRONT) + recoil_displace_x,
                               pm_front_y + (sin(f_angle) * PM_OUT_FRONT) + recoil_displace_y,
                               method_fill_col,
                               method_edge_col);

     recoil_displace_x = 0;
     recoil_displace_y = 0;

//     if (pr->method [pr->vertex_method [vertex]].data [MDATA_PR_PACKET_RECYCLE] > 0)
     {
      recoil_displace_x = ((pm_right_x - pm_front_x) * (float) pr->method [pr->vertex_method [vertex]].data [MDATA_PR_PACKET_RECYCLE]) / PACKET_RECYCLE_TIME / 2;
      recoil_displace_y = ((pm_right_y - pm_front_y) * (float) pr->method [pr->vertex_method [vertex]].data [MDATA_PR_PACKET_RECYCLE]) / PACKET_RECYCLE_TIME / 2;
     }

     add_outline_diamond_layer(1,
                               pm_front_x + recoil_displace_x,
                               pm_front_y + recoil_displace_y,
                               pm_right_x + recoil_displace_x,
                               pm_right_y + recoil_displace_y,
                               pm_right_x + recoil_displace_x + (pm_right_x - pm_back_x) * PM_SIDE_PROPORTION,
                               pm_right_y + recoil_displace_y + (pm_right_y - pm_back_y) * PM_SIDE_PROPORTION,
                               pm_front_x + (cos(f_angle) * PM_OUT_FRONT) + recoil_displace_x,
                               pm_front_y + (sin(f_angle) * PM_OUT_FRONT) + recoil_displace_y,
                               method_fill_col,
                               method_edge_col);



/*
     float recoil_displace_x = 0;
     float recoil_displace_y = 0;

     if (pr->method [pr->vertex_method [vertex]].data [MDATA_PR_PACKET_RECYCLE] > 0)
     {
      recoil_displace_x = ((pm_left_x - pm_front_x) * (float) pr->method [pr->vertex_method [vertex]].data [MDATA_PR_PACKET_RECYCLE]) / PACKET_RECYCLE_TIME / 2;
      recoil_displace_y = ((pm_left_y - pm_front_y) * (float) pr->method [pr->vertex_method [vertex]].data [MDATA_PR_PACKET_RECYCLE]) / PACKET_RECYCLE_TIME / 2;
     }

     add_outline_triangle_layer(1,
                               pm_front_x + recoil_displace_x,
                               pm_front_y + recoil_displace_y,
                               pm_left_x + recoil_displace_x,
                               pm_left_y + recoil_displace_y,
                               pm_front_x + (cos(f_angle) * PM_OUT_FRONT) + recoil_displace_x,
                               pm_front_y + (sin(f_angle) * PM_OUT_FRONT) + recoil_displace_y,
                               method_fill_col,
                               method_edge_col);

     recoil_displace_x = 0;
     recoil_displace_y = 0;

     if (pr->method [pr->vertex_method [vertex]].data [MDATA_PR_PACKET_RECYCLE] > 0)
     {
      recoil_displace_x = ((pm_right_x - pm_front_x) * (float) pr->method [pr->vertex_method [vertex]].data [MDATA_PR_PACKET_RECYCLE]) / PACKET_RECYCLE_TIME / 2;
      recoil_displace_y = ((pm_right_y - pm_front_y) * (float) pr->method [pr->vertex_method [vertex]].data [MDATA_PR_PACKET_RECYCLE]) / PACKET_RECYCLE_TIME / 2;
     }

     add_outline_triangle_layer(1,
                               pm_front_x + recoil_displace_x,
                               pm_front_y + recoil_displace_y,
                               pm_right_x + recoil_displace_x,
                               pm_right_y + recoil_displace_y,
                               pm_front_x + (cos(f_angle) * PM_OUT_FRONT) + recoil_displace_x,
                               pm_front_y + (sin(f_angle) * PM_OUT_FRONT) + recoil_displace_y,
                               method_fill_col,
                               method_edge_col);
*/

/*
     add_simple_outline_triangle_layer(1,
                                       vx + cos(f_angle + PI / 2) * 2,
                                       vy + sin(f_angle + PI / 2) * 2,
                                       f_angle, PM_FRONT,
                                       f_angle + PI, PM_BACK,
                                       f_angle + PI_2, PM_SIDE,
                                       method_fill_col,
                                       method_edge_col);

     add_simple_outline_triangle_layer(1,
                                       vx + cos(f_angle - PI / 2) * 2,
                                       vy + sin(f_angle - PI / 2) * 2,
                                       f_angle, PM_FRONT,
                                       f_angle + PI, PM_BACK,
                                       f_angle - PI_2, PM_SIDE,
                                       method_fill_col,
                                       method_edge_col);
*/
     if (pr->method [pr->vertex_method [vertex]].data [MDATA_PR_PACKET_RECYCLE] > 0)
     {

        dist_front = (pr->method [pr->vertex_method [vertex]].data [MDATA_PR_PACKET_RECYCLE] / 2) + 4;
        int flash_shade = pr->method [pr->vertex_method [vertex]].data [MDATA_PR_PACKET_RECYCLE];
        if (flash_shade > 7)
         flash_shade = 7;
        pg_vx = vx + cos(f_angle) * 8;
        pg_vy = vy + sin(f_angle) * 8;

        add_outline_diamond_layer(1,
                      pg_vx + cos(f_angle) * dist_front * 1.5,
                      pg_vy + sin(f_angle) * dist_front * 1.5,
                      pg_vx + cos(f_angle + PI / 2) * dist_front,
                      pg_vy + sin(f_angle + PI / 2) * dist_front,
                      pg_vx + cos(f_angle + PI) * dist_front * 1.5,
                      pg_vy + sin(f_angle + PI) * dist_front * 1.5,
                      pg_vx + cos(f_angle - PI / 2) * dist_front,
                      pg_vy + sin(f_angle - PI / 2) * dist_front,
                      colours.packet [pr->player_index] [flash_shade],
                      colours.packet [pr->player_index] [flash_shade / 2]);

     }

     break;

     case MTYPE_PR_DPACKET:
     f_angle = pr->float_angle + pr->shape_str->vertex_angle_float [vertex];

     pg_vx = vx;// + cos(f_angle) * 3;
     pg_vy = vy;// + sin(f_angle) * 3;

     add_method_base_diamond(pg_vx, pg_vy, f_angle, pr->shape_str, pr->size, vertex, method_fill_col, method_edge_col);


//     f_angle = pr->float_angle + fixed_to_radians(pr->method [pr->vertex_method [vertex]].ex_angle + short_angle_to_fixed(pr->method [pr->vertex_method [vertex]].data [MDATA_PR_DPACKET_ANGLE]));
     f_angle = pr->float_angle + fixed_to_radians(pr->method [pr->vertex_method [vertex]].ex_angle);

#define PM_FRONT (12 + pr->method [pr->vertex_method [vertex]].extension [MEX_PR_PACKET_SPEED])
#define PM_BACK (8 + pr->method [pr->vertex_method [vertex]].extension [MEX_PR_PACKET_RANGE])
#define PM_SIDE (5 + pr->method [pr->vertex_method [vertex]].extension [MEX_PR_PACKET_POWER])


     pg_vx -= cos(f_angle) * (3 + (float) pr->method [pr->vertex_method [vertex]].data [MDATA_PR_DPACKET_RECYCLE]) / 3;
     pg_vy -= sin(f_angle) * (3 + (float) pr->method [pr->vertex_method [vertex]].data [MDATA_PR_DPACKET_RECYCLE]) / 3;

     float dpm_front_x = pg_vx + cos(f_angle) * PM_CORE_FRONT;
     float dpm_front_y = pg_vy + sin(f_angle) * PM_CORE_FRONT;
     float dpm_right_x = pg_vx + cos(f_angle + PI/2) * PM_CORE_SIDE;
     float dpm_right_y = pg_vy + sin(f_angle + PI/2) * PM_CORE_SIDE;
     float dpm_left_x = pg_vx + cos(f_angle - PI/2) * PM_CORE_SIDE;
     float dpm_left_y = pg_vy + sin(f_angle - PI/2) * PM_CORE_SIDE;
     float dpm_back_x = pg_vx - cos(f_angle) * PM_CORE_BACK;
     float dpm_back_y = pg_vy - sin(f_angle) * PM_CORE_BACK;

     if (pr->method [pr->vertex_method [vertex]].data [MDATA_PR_DPACKET_RECYCLE] == 0)
     {
           add_outline_diamond_layer(1,
                               dpm_front_x + cos(f_angle) * PM_OUT_FRONT,
                               dpm_front_y + sin(f_angle) * PM_OUT_FRONT,
                               dpm_left_x + (dpm_left_x - dpm_back_x) * PM_SIDE_PROPORTION,
                               dpm_left_y + (dpm_left_y - dpm_back_y) * PM_SIDE_PROPORTION,
                               dpm_back_x,
                               dpm_back_y,
                               dpm_right_x + (dpm_right_x - dpm_back_x) * PM_SIDE_PROPORTION,
                               dpm_right_y + (dpm_right_y - dpm_back_y) * PM_SIDE_PROPORTION,
                               method_fill_col,
                               method_edge_col);

      break;
     }


     add_outline_diamond_layer(1,
                               dpm_front_x,
                               dpm_front_y,
                               dpm_left_x,
                               dpm_left_y,
                               dpm_back_x,
                               dpm_back_y,
                               dpm_right_x,
                               dpm_right_y,
                               method_fill_col,
                               method_edge_col);



     float drecoil_displace_x = 0;
     float drecoil_displace_y = 0;

     if (pr->method [pr->vertex_method [vertex]].data [MDATA_PR_PACKET_RECYCLE] > 0)
     {
      drecoil_displace_x = ((dpm_left_x - dpm_front_x) * (float) pr->method [pr->vertex_method [vertex]].data [MDATA_PR_DPACKET_RECYCLE]) / PACKET_RECYCLE_TIME / 2;
      drecoil_displace_y = ((dpm_left_y - dpm_front_y) * (float) pr->method [pr->vertex_method [vertex]].data [MDATA_PR_DPACKET_RECYCLE]) / PACKET_RECYCLE_TIME / 2;
     }

     add_outline_diamond_layer(1,
                               dpm_front_x + drecoil_displace_x,
                               dpm_front_y + drecoil_displace_y,
                               dpm_left_x + drecoil_displace_x,
                               dpm_left_y + drecoil_displace_y,
                               dpm_left_x + drecoil_displace_x + (dpm_left_x - dpm_back_x) * PM_SIDE_PROPORTION,
                               dpm_left_y + drecoil_displace_y + (dpm_left_y - dpm_back_y) * PM_SIDE_PROPORTION,
                               dpm_front_x + (cos(f_angle) * PM_OUT_FRONT) + drecoil_displace_x,
                               dpm_front_y + (sin(f_angle) * PM_OUT_FRONT) + drecoil_displace_y,
                               method_fill_col,
                               method_edge_col);

     drecoil_displace_x = 0;
     drecoil_displace_y = 0;

     if (pr->method [pr->vertex_method [vertex]].data [MDATA_PR_PACKET_RECYCLE] > 0)
     {
      drecoil_displace_x = ((dpm_right_x - dpm_front_x) * (float) pr->method [pr->vertex_method [vertex]].data [MDATA_PR_DPACKET_RECYCLE]) / PACKET_RECYCLE_TIME / 2;
      drecoil_displace_y = ((dpm_right_y - dpm_front_y) * (float) pr->method [pr->vertex_method [vertex]].data [MDATA_PR_DPACKET_RECYCLE]) / PACKET_RECYCLE_TIME / 2;
     }

     add_outline_diamond_layer(1,
                               dpm_front_x + drecoil_displace_x,
                               dpm_front_y + drecoil_displace_y,
                               dpm_right_x + drecoil_displace_x,
                               dpm_right_y + drecoil_displace_y,
                               dpm_right_x + drecoil_displace_x + (dpm_right_x - dpm_back_x) * PM_SIDE_PROPORTION,
                               dpm_right_y + drecoil_displace_y + (dpm_right_y - dpm_back_y) * PM_SIDE_PROPORTION,
                               dpm_front_x + (cos(f_angle) * PM_OUT_FRONT) + drecoil_displace_x,
                               dpm_front_y + (sin(f_angle) * PM_OUT_FRONT) + drecoil_displace_y,
                               method_fill_col,
                               method_edge_col);
/*
     add_simple_outline_triangle_layer(1,
                                       vx + cos(f_angle + PI / 2) * 2,
                                       vy + sin(f_angle + PI / 2) * 2,
                                       f_angle, PM_FRONT,
                                       f_angle + PI, PM_BACK,
                                       f_angle + PI_2, PM_SIDE,
                                       method_fill_col,
                                       method_edge_col);

     add_simple_outline_triangle_layer(1,
                                       vx + cos(f_angle - PI / 2) * 2,
                                       vy + sin(f_angle - PI / 2) * 2,
                                       f_angle, PM_FRONT,
                                       f_angle + PI, PM_BACK,
                                       f_angle - PI_2, PM_SIDE,
                                       method_fill_col,
                                       method_edge_col);
*/
     if (pr->method [pr->vertex_method [vertex]].data [MDATA_PR_PACKET_RECYCLE] > 0)
     {

        dist_front = (pr->method [pr->vertex_method [vertex]].data [MDATA_PR_DPACKET_RECYCLE] / 2) + 4;
        int flash_shade = pr->method [pr->vertex_method [vertex]].data [MDATA_PR_DPACKET_RECYCLE];
        if (flash_shade > 7)
         flash_shade = 7;
        pg_vx = vx + cos(f_angle) * 8;
        pg_vy = vy + sin(f_angle) * 8;

        add_outline_diamond_layer(1,
                      pg_vx + cos(f_angle) * dist_front * 1.5,
                      pg_vy + sin(f_angle) * dist_front * 1.5,
                      pg_vx + cos(f_angle + PI / 2) * dist_front,
                      pg_vy + sin(f_angle + PI / 2) * dist_front,
                      pg_vx + cos(f_angle + PI) * dist_front * 1.5,
                      pg_vy + sin(f_angle + PI) * dist_front * 1.5,
                      pg_vx + cos(f_angle - PI / 2) * dist_front,
                      pg_vy + sin(f_angle - PI / 2) * dist_front,
                      colours.packet [pr->player_index] [flash_shade],
                      colours.packet [pr->player_index] [flash_shade / 2]);

     }
      break;
     case MTYPE_PR_MOVE:
     f_angle = pr->float_angle + pr->shape_str->vertex_angle_float [vertex];

     pg_vx = vx;// + cos(f_angle) * 3;
     pg_vy = vy;// + sin(f_angle) * 3;

     add_method_base_diamond(pg_vx, pg_vy, f_angle, pr->shape_str, pr->size, vertex, method_fill_col, method_edge_col);
/*
      add_outline_diamond_layer(0,
                          pg_vx,
                          pg_vy,
                          pg_vx + cos(f_angle + fixed_to_radians(pr->shape_str->external_angle [EXANGLE_LOW] [vertex])) * 12,
                          pg_vy + sin(f_angle + fixed_to_radians(pr->shape_str->external_angle [EXANGLE_LOW] [vertex])) * 12,
                          pg_vx - cos(f_angle) * 14,
                          pg_vy - sin(f_angle) * 14,
                          pg_vx + cos(f_angle + fixed_to_radians(pr->shape_str->external_angle [EXANGLE_HIGH] [vertex])) * 12,
                          pg_vy + sin(f_angle + fixed_to_radians(pr->shape_str->external_angle [EXANGLE_HIGH] [vertex])) * 12,
                          team_colours [team_colour] [TCOL_FILL1],
                          team_colours [team_colour] [TCOL_LINE1]);*/

     vx += cos(f_angle) * 3;
     vy += sin(f_angle) * 3;



     f_angle = pr->float_angle + fixed_to_radians(pr->method [pr->vertex_method [vertex]].ex_angle);
     dist_front = 12 + pr->method [pr->vertex_method [vertex]].extension [0];
     float dist_side = 5 + pr->method [pr->vertex_method [vertex]].extension [0] / 2;
     float dist_back = 4 + pr->method [pr->vertex_method [vertex]].extension [0];
    	float extension_length = 2 + pr->method [pr->vertex_method [vertex]].extension [0];


       	float left_front_vertex_x = vx + cos(f_angle - PI/28) * dist_front;
       	float left_front_vertex_y = vy + sin(f_angle - PI/28) * dist_front;
       	float right_front_vertex_x = vx + cos(f_angle + PI/28) * dist_front;
       	float right_front_vertex_y = vy + sin(f_angle + PI/28) * dist_front;

       	float left_back_vertex_x = vx + cos(f_angle + PI + PI/6) * dist_back;
       	float left_back_vertex_y = vy + sin(f_angle + PI + PI/6) * dist_back;
       	float right_back_vertex_x = vx + cos(f_angle + PI - PI/6) * dist_back;
       	float right_back_vertex_y = vy + sin(f_angle + PI - PI/6) * dist_back;

       	float left_vertex_x = left_back_vertex_x + cos(f_angle - PI/4) * dist_side;
       	float left_vertex_y = left_back_vertex_y + sin(f_angle - PI/4) * dist_side;

       	float right_vertex_x = right_back_vertex_x + cos(f_angle + PI/4) * dist_side;
       	float right_vertex_y = right_back_vertex_y + sin(f_angle + PI/4) * dist_side;



       if (pr->method [pr->vertex_method [vertex]].data [MDATA_PR_MOVE_FIRING] > 0)
       {

       	float split_amount = (float) pr->method [pr->vertex_method [vertex]].data [MDATA_PR_MOVE_FIRING] * 0.20;
       	float split_displace_left_x = cos(f_angle - PI/18) * split_amount;
       	float split_displace_left_y = sin(f_angle - PI/18) * split_amount;
       	float split_displace_right_x = cos(f_angle + PI/18) * split_amount;
       	float split_displace_right_y = sin(f_angle + PI/18) * split_amount;



              add_outline_hexagon_layer(1,
                      right_front_vertex_x,
                      right_front_vertex_y,
                      left_front_vertex_x,
                      left_front_vertex_y,
                      left_vertex_x,
                      left_vertex_y,
																						left_back_vertex_x,
																						left_back_vertex_y,
																						right_back_vertex_x,
																						right_back_vertex_y,
                      right_vertex_x,
                      right_vertex_y,
                      method_fill_col,
                      method_edge_col);


       add_outline_diamond_layer(1,
                      left_front_vertex_x + split_displace_left_x,
                      left_front_vertex_y + split_displace_left_y,
                      left_front_vertex_x + cos(f_angle - PI/2) * extension_length + split_displace_left_x,
                      left_front_vertex_y + sin(f_angle - PI/2) * extension_length + split_displace_left_y,
																						left_vertex_x + cos(f_angle - PI/6) * extension_length + split_displace_left_x,
																						left_vertex_y + sin(f_angle - PI/6) * extension_length + split_displace_left_y,
																						left_vertex_x + split_displace_left_x,
																						left_vertex_y + split_displace_left_y,
                      method_fill_col,
                      method_edge_col);

       add_outline_diamond_layer(1,
                      right_front_vertex_x + split_displace_right_x,
                      right_front_vertex_y + split_displace_right_y,
                      right_front_vertex_x + cos(f_angle + PI/2) * extension_length + split_displace_right_x,
                      right_front_vertex_y + sin(f_angle + PI/2) * extension_length + split_displace_right_y,
																						right_vertex_x + cos(f_angle + PI/6) * extension_length + split_displace_right_x,
																						right_vertex_y + sin(f_angle + PI/6) * extension_length + split_displace_right_y,
																						right_vertex_x + split_displace_right_x,
																						right_vertex_y + split_displace_right_y,
                      method_fill_col,
                      method_edge_col);

        int firing_shade = pr->method [pr->vertex_method [vertex]].data [MDATA_PR_MOVE_FIRING];
        if (pr->method [pr->vertex_method [vertex]].data [MDATA_PR_MOVE_FIRING] != 8)
         firing_shade /= 2;
        if (firing_shade >= CLOUD_SHADES)
         firing_shade = CLOUD_SHADES - 1;


//        add_simple_outline_diamond_layer(1, vx + cos(f_angle) * dist_front * 1.5, vy + sin(f_angle) * dist_front * 1.5, 18, 6, 6, f_angle, cloud_col [w.player[pr->player_index].drive_colour] [firing_shade / 2], cloud_col [w.player[pr->player_index].drive_colour] [firing_shade]);
//        add_simple_outline_diamond_layer(1, vx + cos(f_angle) * dist_front * 1.5, vy + sin(f_angle) * dist_front * 1.5, 14 + grand(5), 4 + grand(4), 4 + grand(4), f_angle, cloud_col [w.player[pr->player_index].drive_colour] [firing_shade], cloud_col [w.player[pr->player_index].drive_colour] [firing_shade / 2]);
//        int base_drive_size = pr->method [pr->vertex_method [vertex]].data [MDATA_PR_ACCEL_ACTUAL_RATE];
        int along_drive_line_dist = (w.world_time * 2) % 16; // will be further modified later
        int along_drive_line_size = (18 - along_drive_line_dist);
        along_drive_line_size *= 10 + pr->method [pr->vertex_method [vertex]].data [MDATA_PR_MOVE_ACTUAL_RATE];
        along_drive_line_size /= 30;

        along_drive_line_dist *= pr->method [pr->vertex_method [vertex]].data [MDATA_PR_MOVE_ACTUAL_RATE];
        along_drive_line_dist /= 5;

        add_simple_outline_diamond_layer(1,
                                         vx + cos(f_angle) * ((dist_front * 1.5) + along_drive_line_dist),
                                         vy + sin(f_angle) * ((dist_front * 1.5) + along_drive_line_dist),
                                         2 + proc_rand(pr, vertex, 3),
                                         2 + proc_rand(pr, vertex, 3),
                                         along_drive_line_size + proc_rand(pr, vertex, 5),
                                         f_angle,
                                         colours.drive [pr->player_index] [firing_shade / 3], colours.drive [pr->player_index] [firing_shade / 3]);


        add_simple_outline_diamond_layer(1,
                                         vx + cos(f_angle) * ((dist_front * 1.5) + pr->method [pr->vertex_method [vertex]].data [MDATA_PR_MOVE_ACTUAL_RATE] / 4),
                                         vy + sin(f_angle) * ((dist_front * 1.5) + pr->method [pr->vertex_method [vertex]].data [MDATA_PR_MOVE_ACTUAL_RATE] / 4),
                                         pr->method [pr->vertex_method [vertex]].data [MDATA_PR_MOVE_ACTUAL_RATE] * 2 + proc_rand(pr, vertex, 5), // front length
                                         2 + (pr->method [pr->vertex_method [vertex]].data [MDATA_PR_MOVE_ACTUAL_RATE] / 2) + proc_rand(pr, vertex, 4), // back length
                                         2 + (pr->method [pr->vertex_method [vertex]].data [MDATA_PR_MOVE_ACTUAL_RATE] / 2) + proc_rand(pr, vertex, 4), // width
                                         f_angle, colours.drive [pr->player_index] [firing_shade / 2], colours.drive [pr->player_index] [firing_shade / 3]);
        add_simple_outline_diamond_layer(1,
                                         vx + cos(f_angle) * ((dist_front * 1.5) + pr->method [pr->vertex_method [vertex]].data [MDATA_PR_MOVE_ACTUAL_RATE] / 4),
                                         vy + sin(f_angle) * ((dist_front * 1.5) + pr->method [pr->vertex_method [vertex]].data [MDATA_PR_MOVE_ACTUAL_RATE] / 4),
                                         3 + proc_rand(pr, vertex, 5) + pr->method [pr->vertex_method [vertex]].data [MDATA_PR_MOVE_ACTUAL_RATE] / 3,
                                         3 + proc_rand(pr, vertex, 3) + pr->method [pr->vertex_method [vertex]].data [MDATA_PR_MOVE_ACTUAL_RATE] / 4,
                                         2 + proc_rand(pr, vertex, 2) + pr->method [pr->vertex_method [vertex]].data [MDATA_PR_MOVE_ACTUAL_RATE] / 5,
                                         f_angle, colours.drive [pr->player_index] [firing_shade], colours.drive [pr->player_index] [firing_shade / 2]);

//        add_simple_outline_rectangle_layer(1, vx + cos(f_angle) * dist_front * 1.3, vy + sin(f_angle) * dist_front * 1.3, 17, 13, f_angle, cloud_col [w.player[pr->player_index].drive_colour] [firing_shade / 2], cloud_col [w.player[pr->player_index].drive_colour] [firing_shade]);

//        al_draw_filled_circle(vx + cos(f_angle) * dist_front * 1.3, vy + sin(f_angle) * dist_front * 1.3, pr->method [pr->vertex_method [vertex]].data [MDATA_PR_ACCEL_FIRING] * 0.5+ grand(4), cloud_col [w.player[pr->player_index].drive_colour] [firing_shade]);
        firing_shade /= 2;
        if (firing_shade >= CLOUD_SHADES)
         firing_shade = CLOUD_SHADES - 1;
//        al_draw_filled_circle(vx + cos(f_angle) * dist_front * 1.3, vy + sin(f_angle) * dist_front * 1.3, (pr->method [pr->vertex_method [vertex]].data [MDATA_PR_ACCEL_FIRING] * 1) + grand(4), cloud_col [w.player[pr->player_index].drive_colour] [firing_shade]);
//        al_draw_filled_circle(vx + cos(f_angle) * dist_front * 2.5, vy + sin(f_angle) * dist_front * 2.5, (pr->method [pr->vertex_method [vertex]].data [MDATA_PR_ACCEL_FIRING] * 0.3) + grand(3), cloud_col [w.player[pr->player_index].drive_colour] [firing_shade]);


//void add_simple_outline_rectangle_layer(int layer, float x, float y, float length, float width, float angle, ALLEGRO_COLOR fill_col, ALLEGRO_COLOR edge_col)
       }
        else
              add_outline_hexagon_layer(1,
                      right_front_vertex_x + cos(f_angle + PI/2) * extension_length,
                      right_front_vertex_y + sin(f_angle + PI/2) * extension_length,
                      left_front_vertex_x + cos(f_angle - PI/2) * extension_length,
                      left_front_vertex_y + sin(f_angle - PI/2) * extension_length,
                      left_vertex_x + cos(f_angle - PI/6) * extension_length,
                      left_vertex_y + sin(f_angle - PI/6) * extension_length,
																						left_back_vertex_x,
																						left_back_vertex_y,
																						right_back_vertex_x,
																						right_back_vertex_y,
                      right_vertex_x + cos(f_angle + PI/6) * extension_length,
                      right_vertex_y + sin(f_angle + PI/6) * extension_length,
                      method_fill_col,
                      method_edge_col);


/*

     f_angle = pr->float_angle + fixed_to_radians(pr->method [pr->vertex_method [vertex]].ex_angle);
     dist_front = 9 + pr->method [pr->vertex_method [vertex]].extension [0];
     float dist_side = 7 + pr->method [pr->vertex_method [vertex]].extension [0];
     float dist_back = 8 + pr->method [pr->vertex_method [vertex]].extension [0];



       if (pr->method [pr->vertex_method [vertex]].data [MDATA_PR_MOVE_FIRING] > 0)
       {

       	float split_amount = (float) pr->method [pr->vertex_method [vertex]].data [MDATA_PR_MOVE_FIRING] * 0.10;


       add_outline_diamond_layer(1,
                      vx + cos(f_angle - PI/9) * dist_front,
                      vy + sin(f_angle - PI/9) * dist_front,
                      vx + cos(f_angle + PI/9) * dist_front,
                      vy + sin(f_angle + PI/9) * dist_front,
                      vx + cos(f_angle + PI - PI/9) * dist_back,
                      vy + sin(f_angle + PI - PI/9) * dist_back,
                      vx + cos(f_angle + PI + PI/9) * dist_back,
                      vy + sin(f_angle + PI + PI/9) * dist_back,
                      method_fill_col,
                      method_edge_col);

              add_outline_triangle_layer(1,
                      vx + cos(f_angle - PI/9) * dist_front + cos(f_angle - PI / 2) * split_amount + cos(f_angle) * split_amount * 3,
                      vy + sin(f_angle - PI/9) * dist_front + sin(f_angle - PI / 2) * split_amount + sin(f_angle) * split_amount * 3,
                      vx + cos(f_angle + PI + PI/9) * dist_back + cos(f_angle - PI / 2) * split_amount + cos(f_angle) * split_amount * 3,
                      vy + sin(f_angle + PI + PI/9) * dist_back + sin(f_angle - PI / 2) * split_amount + sin(f_angle) * split_amount * 3,
                      vx + cos(f_angle - PI/2 - PI/10) * dist_side + cos(f_angle - PI / 2) * split_amount + cos(f_angle) * split_amount * 3,
                      vy + sin(f_angle - PI/2 - PI/10) * dist_side + sin(f_angle - PI / 2) * split_amount + sin(f_angle) * split_amount * 3,
                      method_fill_col,
                      method_edge_col);

              add_outline_triangle_layer(1,
                      vx + cos(f_angle + PI/9) * dist_front + cos(f_angle + PI / 2) * split_amount + cos(f_angle) * split_amount * 3,
                      vy + sin(f_angle + PI/9) * dist_front + sin(f_angle + PI / 2) * split_amount + sin(f_angle) * split_amount * 3,
                      vx + cos(f_angle + PI - PI/9) * dist_back + cos(f_angle + PI / 2) * split_amount + cos(f_angle) * split_amount * 3,
                      vy + sin(f_angle + PI - PI/9) * dist_back + sin(f_angle + PI / 2) * split_amount + sin(f_angle) * split_amount * 3,
                      vx + cos(f_angle + PI/2 + PI/10) * dist_side + cos(f_angle + PI / 2) * split_amount + cos(f_angle) * split_amount * 3,
                      vy + sin(f_angle + PI/2 + PI/10) * dist_side + sin(f_angle + PI / 2) * split_amount + sin(f_angle) * split_amount * 3,
                      method_fill_col,
                      method_edge_col);


        int firing_shade = pr->method [pr->vertex_method [vertex]].data [MDATA_PR_MOVE_FIRING];
        if (pr->method [pr->vertex_method [vertex]].data [MDATA_PR_MOVE_FIRING] != 8)
         firing_shade /= 2;
        if (firing_shade >= CLOUD_SHADES)
         firing_shade = CLOUD_SHADES - 1;


//        add_simple_outline_diamond_layer(1, vx + cos(f_angle) * dist_front * 1.5, vy + sin(f_angle) * dist_front * 1.5, 18, 6, 6, f_angle, cloud_col [w.player[pr->player_index].drive_colour] [firing_shade / 2], cloud_col [w.player[pr->player_index].drive_colour] [firing_shade]);
//        add_simple_outline_diamond_layer(1, vx + cos(f_angle) * dist_front * 1.5, vy + sin(f_angle) * dist_front * 1.5, 14 + grand(5), 4 + grand(4), 4 + grand(4), f_angle, cloud_col [w.player[pr->player_index].drive_colour] [firing_shade], cloud_col [w.player[pr->player_index].drive_colour] [firing_shade / 2]);
//        int base_drive_size = pr->method [pr->vertex_method [vertex]].data [MDATA_PR_ACCEL_ACTUAL_RATE];
        int along_drive_line_dist = (w.world_time * 2) % 16; // will be further modified later
        int along_drive_line_size = (18 - along_drive_line_dist);
        along_drive_line_size *= pr->method [pr->vertex_method [vertex]].data [MDATA_PR_MOVE_ACTUAL_RATE];
        along_drive_line_size /= 10;

        along_drive_line_dist *= pr->method [pr->vertex_method [vertex]].data [MDATA_PR_MOVE_ACTUAL_RATE];
        along_drive_line_dist /= 5;

        add_simple_outline_diamond_layer(1,
                                         vx + cos(f_angle) * ((dist_front * 1.5) + along_drive_line_dist),
                                         vy + sin(f_angle) * ((dist_front * 1.5) + along_drive_line_dist),
                                         2 + proc_rand(pr, vertex, 3),
                                         2 + proc_rand(pr, vertex, 3),
                                         along_drive_line_size + proc_rand(pr, vertex, 5),
                                         f_angle,
                                         colours.drive [pr->player_index] [firing_shade / 3], colours.drive [pr->player_index] [firing_shade / 3]);


        add_simple_outline_diamond_layer(1,
                                         vx + cos(f_angle) * ((dist_front * 1.5) + pr->method [pr->vertex_method [vertex]].data [MDATA_PR_MOVE_ACTUAL_RATE] / 4),
                                         vy + sin(f_angle) * ((dist_front * 1.5) + pr->method [pr->vertex_method [vertex]].data [MDATA_PR_MOVE_ACTUAL_RATE] / 4),
                                         pr->method [pr->vertex_method [vertex]].data [MDATA_PR_MOVE_ACTUAL_RATE] * 2 + proc_rand(pr, vertex, 5), // front length
                                         2 + (pr->method [pr->vertex_method [vertex]].data [MDATA_PR_MOVE_ACTUAL_RATE] / 2) + proc_rand(pr, vertex, 4), // back length
                                         2 + (pr->method [pr->vertex_method [vertex]].data [MDATA_PR_MOVE_ACTUAL_RATE] / 2) + proc_rand(pr, vertex, 4), // width
                                         f_angle, colours.drive [pr->player_index] [firing_shade / 2], colours.drive [pr->player_index] [firing_shade / 3]);
        add_simple_outline_diamond_layer(1,
                                         vx + cos(f_angle) * ((dist_front * 1.5) + pr->method [pr->vertex_method [vertex]].data [MDATA_PR_MOVE_ACTUAL_RATE] / 4),
                                         vy + sin(f_angle) * ((dist_front * 1.5) + pr->method [pr->vertex_method [vertex]].data [MDATA_PR_MOVE_ACTUAL_RATE] / 4),
                                         3 + proc_rand(pr, vertex, 5) + pr->method [pr->vertex_method [vertex]].data [MDATA_PR_MOVE_ACTUAL_RATE] / 3,
                                         3 + proc_rand(pr, vertex, 3) + pr->method [pr->vertex_method [vertex]].data [MDATA_PR_MOVE_ACTUAL_RATE] / 4,
                                         2 + proc_rand(pr, vertex, 2) + pr->method [pr->vertex_method [vertex]].data [MDATA_PR_MOVE_ACTUAL_RATE] / 5,
                                         f_angle, colours.drive [pr->player_index] [firing_shade], colours.drive [pr->player_index] [firing_shade / 2]);

//        add_simple_outline_rectangle_layer(1, vx + cos(f_angle) * dist_front * 1.3, vy + sin(f_angle) * dist_front * 1.3, 17, 13, f_angle, cloud_col [w.player[pr->player_index].drive_colour] [firing_shade / 2], cloud_col [w.player[pr->player_index].drive_colour] [firing_shade]);

//        al_draw_filled_circle(vx + cos(f_angle) * dist_front * 1.3, vy + sin(f_angle) * dist_front * 1.3, pr->method [pr->vertex_method [vertex]].data [MDATA_PR_ACCEL_FIRING] * 0.5+ grand(4), cloud_col [w.player[pr->player_index].drive_colour] [firing_shade]);
        firing_shade /= 2;
        if (firing_shade >= CLOUD_SHADES)
         firing_shade = CLOUD_SHADES - 1;
//        al_draw_filled_circle(vx + cos(f_angle) * dist_front * 1.3, vy + sin(f_angle) * dist_front * 1.3, (pr->method [pr->vertex_method [vertex]].data [MDATA_PR_ACCEL_FIRING] * 1) + grand(4), cloud_col [w.player[pr->player_index].drive_colour] [firing_shade]);
//        al_draw_filled_circle(vx + cos(f_angle) * dist_front * 2.5, vy + sin(f_angle) * dist_front * 2.5, (pr->method [pr->vertex_method [vertex]].data [MDATA_PR_ACCEL_FIRING] * 0.3) + grand(3), cloud_col [w.player[pr->player_index].drive_colour] [firing_shade]);


//void add_simple_outline_rectangle_layer(int layer, float x, float y, float length, float width, float angle, ALLEGRO_COLOR fill_col, ALLEGRO_COLOR edge_col)
       }
        else
              add_outline_hexagon_layer(1,
                      vx + cos(f_angle - PI/9) * dist_front,
                      vy + sin(f_angle - PI/9) * dist_front,
                      vx + cos(f_angle + PI/9) * dist_front,
                      vy + sin(f_angle + PI/9) * dist_front,
                      vx + cos(f_angle + PI/2 + PI/10) * dist_side,
                      vy + sin(f_angle + PI/2 + PI/10) * dist_side,
                      vx + cos(f_angle + PI - PI/9) * dist_back,
                      vy + sin(f_angle + PI - PI/9) * dist_back,
                      vx + cos(f_angle + PI + PI/9) * dist_back,
                      vy + sin(f_angle + PI + PI/9) * dist_back,
                      vx + cos(f_angle - PI/2 - PI/10) * dist_side,
                      vy + sin(f_angle - PI/2 - PI/10) * dist_side,
                      method_fill_col,
                      method_edge_col);

*/

/*      al_draw_filled_circle(vx, vy, 5, pr_team->colour [TCOL_METHOD_FILL]);
      al_draw_circle(vx, vy, 5, pr_team->colour [TCOL_METHOD_LINE1], 1);
      al_draw_filled_circle(vx + xpart(pr->angle + pr->method [pr->vertex_method [vertex]].ex_angle, 5), vy + ypart(pr->angle + pr->method [pr->vertex_method [vertex]].ex_angle, 5), 2, pr_team->colour [TCOL_METHOD_FILL]);
      al_draw_circle(vx + xpart(pr->angle + pr->method [pr->vertex_method [vertex]].ex_angle, 5), vy + ypart(pr->angle + pr->method [pr->vertex_method [vertex]].ex_angle, 5), 2, pr_team->colour [TCOL_METHOD_LINE1], 1);*/
      break;



     case MTYPE_PR_STREAM:
     case MTYPE_PR_DSTREAM:
     f_angle = pr->float_angle + pr->shape_str->vertex_angle_float [vertex];

     pg_vx = vx;// + cos(f_angle) * 3;
     pg_vy = vy;// + sin(f_angle) * 3;

     add_method_base_diamond(pg_vx, pg_vy, f_angle, pr->shape_str, pr->size, vertex, method_fill_col, method_edge_col);

     f_angle = pr->float_angle + fixed_to_radians(pr->method [pr->vertex_method [vertex]].ex_angle);


#define OL_CORE_FRONT 5
#define OL_CORE_SIDE 5
#define OL_CORE_BACK 7

#define OL_OUT_FRONT 10
#define OL_OUT_SIDE 5
#define OL_OUT_BACK 4

     float ol_out_front_x;// = pg_vx + cos(f_angle - PI/8) * OL_CORE_FRONT;
     float ol_out_front_y;// = pg_vy + sin(f_angle - PI/8) * OL_CORE_FRONT;
     float ol_out_right_x;// = pg_vx + cos(f_angle + PI/2) * OL_CORE_SIDE;
     float ol_out_right_y;// = pg_vy + sin(f_angle + PI/2) * OL_CORE_SIDE;
     float ol_out_left_x;// = pg_vx + cos(f_angle - PI/2) * OL_CORE_SIDE;
     float ol_out_left_y;// = pg_vy + sin(f_angle - PI/2) * OL_CORE_SIDE;
     float ol_out_back1_x;
     float ol_out_back1_y;
     float ol_out_back2_x;
     float ol_out_back2_y;

      ol_out_front_x = pg_vx + cos(f_angle) * (OL_CORE_FRONT + OL_OUT_FRONT);
      ol_out_front_y = pg_vy + sin(f_angle) * (OL_CORE_FRONT + OL_OUT_FRONT);
      ol_out_right_x = pg_vx + cos(f_angle + PI/2) * (OL_CORE_SIDE + OL_OUT_SIDE);
      ol_out_right_y = pg_vy + sin(f_angle + PI/2) * (OL_CORE_SIDE + OL_OUT_SIDE);
      ol_out_left_x = pg_vx + cos(f_angle - PI/2) * (OL_CORE_SIDE + OL_OUT_SIDE);
      ol_out_left_y = pg_vy + sin(f_angle - PI/2) * (OL_CORE_SIDE + OL_OUT_SIDE);

      ol_out_back1_x = pg_vx - cos(f_angle - PI/8) * (OL_CORE_BACK + OL_OUT_BACK);
      ol_out_back1_y = pg_vy - sin(f_angle - PI/8) * (OL_CORE_BACK + OL_OUT_BACK);
      ol_out_back2_x = pg_vx - cos(f_angle + PI/8) * (OL_CORE_BACK + OL_OUT_BACK);
      ol_out_back2_y = pg_vy - sin(f_angle + PI/8) * (OL_CORE_BACK + OL_OUT_BACK);


     if (pr->method[pr->vertex_method [vertex]].data [MDATA_PR_STREAM_STATUS] == STREAM_STATUS_INACTIVE)
     {

      add_outline_pentagon_layer(1,
                                 ol_out_front_x,
                                 ol_out_front_y,
                                 ol_out_right_x,
                                 ol_out_right_y,
                                 ol_out_back1_x,
                                 ol_out_back1_y,
                                 ol_out_back2_x,
                                 ol_out_back2_y,
                                 ol_out_left_x,
                                 ol_out_left_y,
                                 method_fill_col, method_edge_col);

      break;

     }

      float ol_core_front_x = pg_vx + cos(f_angle) * OL_CORE_FRONT;
      float ol_core_front_y = pg_vy + sin(f_angle) * OL_CORE_FRONT;
      float ol_core_right_x = pg_vx + cos(f_angle + PI/2) * OL_CORE_SIDE;
      float ol_core_right_y = pg_vy + sin(f_angle + PI/2) * OL_CORE_SIDE;
      float ol_core_left_x = pg_vx + cos(f_angle - PI/2) * OL_CORE_SIDE;
      float ol_core_left_y = pg_vy + sin(f_angle - PI/2) * OL_CORE_SIDE;
      float ol_core_back_x = pg_vx - cos(f_angle) * OL_CORE_FRONT;
      float ol_core_back_y = pg_vy - sin(f_angle) * OL_CORE_FRONT;

      add_outline_diamond_layer(1,
                                 ol_core_front_x,
                                 ol_core_front_y,
                                 ol_core_right_x,
                                 ol_core_right_y,
                                 ol_core_back_x,
                                 ol_core_back_y,
                                 ol_core_left_x,
                                 ol_core_left_y,
                                 method_fill_col, method_edge_col);


     float base_method_open = 0;
     float method_open = 0;

     switch(pr->method[pr->vertex_method [vertex]].data [MDATA_PR_STREAM_STATUS])
     {
      case STREAM_STATUS_WARMUP:
       base_method_open = (STREAM_WARMUP_LENGTH - pr->method[pr->vertex_method [vertex]].data [MDATA_PR_STREAM_COUNTER]) * 2;
       break;
      case STREAM_STATUS_FIRING:
       base_method_open = STREAM_WARMUP_LENGTH;
       break;
      case STREAM_STATUS_COOLDOWN:
       base_method_open = (pr->method[pr->vertex_method [vertex]].data [MDATA_PR_STREAM_COUNTER]) / (STREAM_COOLDOWN_LENGTH / STREAM_WARMUP_LENGTH);
       break;
     }

     if (base_method_open > STREAM_WARMUP_LENGTH)
      base_method_open = STREAM_WARMUP_LENGTH;

     method_open = base_method_open / 2;

     float ol_offset_x;
     float ol_offset_y;

//     method_open /= 2;

     ol_offset_x = cos(f_angle + PI/4) * method_open;
     ol_offset_y = sin(f_angle + PI/4) * method_open;

// Front right:
     add_outline_diamond_layer(1,
                                ol_core_front_x + ol_offset_x,
                                ol_core_front_y + ol_offset_y,
                                ol_out_front_x + ol_offset_x,
                                ol_out_front_y + ol_offset_y,
                                ol_out_right_x + ol_offset_x,
                                ol_out_right_y + ol_offset_y,
                                ol_core_right_x + ol_offset_x,
                                ol_core_right_y + ol_offset_y,
                                method_fill_col, method_edge_col);

// Front left:
     ol_offset_x = cos(f_angle - (PI/4)) * method_open;
     ol_offset_y = sin(f_angle - (PI/4)) * method_open;

     add_outline_diamond_layer(1,
                                ol_core_front_x + ol_offset_x,
                                ol_core_front_y + ol_offset_y,
                                ol_core_left_x + ol_offset_x,
                                ol_core_left_y + ol_offset_y,
                                ol_out_left_x + ol_offset_x,
                                ol_out_left_y + ol_offset_y,
                                ol_out_front_x + ol_offset_x,
                                ol_out_front_y + ol_offset_y,
                                method_fill_col, method_edge_col);

    method_open = base_method_open / 3;


// back right:
     ol_offset_x = cos(f_angle + PI/2 + (PI/4)) * method_open;
     ol_offset_y = sin(f_angle + PI/2 + (PI/4)) * method_open;

     add_outline_diamond_layer(1,
                                ol_core_right_x + ol_offset_x,
                                ol_core_right_y + ol_offset_y,
                                ol_out_right_x + ol_offset_x,
                                ol_out_right_y + ol_offset_y,
                                ol_out_back1_x + ol_offset_x,
                                ol_out_back1_y + ol_offset_y,
                                ol_core_back_x + ol_offset_x,
                                ol_core_back_y + ol_offset_y,
                                method_fill_col, method_edge_col);

// back_left:
     ol_offset_x = cos(f_angle + PI + (PI/4)) * method_open;
     ol_offset_y = sin(f_angle + PI + (PI/4)) * method_open;

     add_outline_diamond_layer(1,
                                ol_core_left_x + ol_offset_x,
                                ol_core_left_y + ol_offset_y,
                                ol_core_back_x + ol_offset_x,
                                ol_core_back_y + ol_offset_y,
                                ol_out_back2_x + ol_offset_x,
                                ol_out_back2_y + ol_offset_y,
                                ol_out_left_x + ol_offset_x,
                                ol_out_left_y + ol_offset_y,
                                method_fill_col, method_edge_col);


     method_open = base_method_open;

     ol_offset_x = cos(f_angle + (PI)) * method_open;
     ol_offset_y = sin(f_angle + (PI)) * method_open;

     add_outline_triangle_layer(1,
                                ol_core_back_x + ol_offset_x,
                                ol_core_back_y + ol_offset_y,
                                ol_out_back1_x + ol_offset_x,
                                ol_out_back1_y + ol_offset_y,
                                ol_out_back2_x + ol_offset_x,
                                ol_out_back2_y + ol_offset_y,
                                method_fill_col, method_edge_col);

     break;



    case MTYPE_PR_BROADCAST:
     f_angle = pr->float_angle + pr->shape_str->vertex_angle_float [vertex];

     add_method_base_diamond(vx, vy, f_angle, pr->shape_str, pr->size, vertex, method_fill_col, method_edge_col);

     add_line(vx + cos(f_angle) * 3,
              vy + sin(f_angle) * 3,
              vx + cos(f_angle) * 3 + cos(f_angle) * 8,
              vy + sin(f_angle) * 3 + sin(f_angle) * 8,
              method_medium_col);

     float an_x = vx + cos(f_angle) * 11;
     float an_y = vy + sin(f_angle) * 11;

     add_outline_diamond_layer(1,
              an_x,
              an_y,
              an_x + cos(f_angle + PI / 3) * 4,
              an_y + sin(f_angle + PI / 3) * 4,
              an_x + cos(f_angle) * 5,
              an_y + sin(f_angle) * 5,
              an_x + cos(f_angle - PI / 3) * 4,
              an_y + sin(f_angle - PI / 3) * 4,
              method_fill_col,
              method_edge_col);


/*     add_line(vx + cos(f_angle) * 3 + cos(f_angle + fixed_to_radians(pr->shape_str->external_angle [EXANGLE_LOW] [vertex])) * 4,
              vy + sin(f_angle) * 3 + sin(f_angle + fixed_to_radians(pr->shape_str->external_angle [EXANGLE_LOW] [vertex])) * 4,
              vx + cos(f_angle) * 3 + cos(f_angle + fixed_to_radians(pr->shape_str->external_angle [EXANGLE_LOW] [vertex])) * 4 + cos(f_angle) * 8,
              vy + sin(f_angle) * 3 + sin(f_angle + fixed_to_radians(pr->shape_str->external_angle [EXANGLE_LOW] [vertex])) * 4 + sin(f_angle) * 8,
              method_edge_col);


     add_line(vx + cos(f_angle) * 3 + cos(f_angle + fixed_to_radians(pr->shape_str->external_angle [EXANGLE_HIGH] [vertex])) * 4,
              vy + sin(f_angle) * 3 + sin(f_angle + fixed_to_radians(pr->shape_str->external_angle [EXANGLE_HIGH] [vertex])) * 4,
              vx + cos(f_angle) * 3 + cos(f_angle + fixed_to_radians(pr->shape_str->external_angle [EXANGLE_HIGH] [vertex])) * 4 + cos(f_angle) * 8,
              vy + sin(f_angle) * 3 + sin(f_angle + fixed_to_radians(pr->shape_str->external_angle [EXANGLE_HIGH] [vertex])) * 4 + sin(f_angle) * 8,
              method_edge_col);*/

     break;


    case MTYPE_PR_YIELD:
     f_angle = pr->float_angle + pr->shape_str->vertex_angle_float [vertex];


// V1 is width between bits
#define YGDM_V1 7
// V2 is how far bits protrude outside proc shape
#define YGDM_V2 5
// V2A is extra protrusion for front points
#define YGDM_V2A 4
// V2A is extra protrusion for 2nd set of front points
#define YGDM_V2B 4
// V3 is how far along edge to go
#define YGDM_V3 6
// V4 is inward distance
#define YGDM_V4 9


     dg_vx = vx + cos(f_angle + fixed_to_radians(pr->shape_str->external_angle [EXANGLE_LOW] [vertex])) * YGDM_V1;
     dg_vy = vy + sin(f_angle + fixed_to_radians(pr->shape_str->external_angle [EXANGLE_LOW] [vertex])) * YGDM_V1;

     dg_vx += cos(f_angle) * YGDM_V2;
     dg_vy += sin(f_angle) * YGDM_V2;

      add_outline_diamond_layer(1,
                          dg_vx + cos(f_angle) * YGDM_V2A,
                          dg_vy + sin(f_angle) * YGDM_V2A,
                          dg_vx + cos(f_angle) * YGDM_V2A + cos(f_angle + PI/6) * YGDM_V2B,
                          dg_vy + sin(f_angle) * YGDM_V2A + sin(f_angle + PI/6) * YGDM_V2B,
                          dg_vx + cos(f_angle + fixed_to_radians(pr->shape_str->external_angle [EXANGLE_LOW] [vertex])) * YGDM_V3,
                          dg_vy + sin(f_angle + fixed_to_radians(pr->shape_str->external_angle [EXANGLE_LOW] [vertex])) * YGDM_V3,
                          dg_vx - cos(f_angle) * YGDM_V4,
                          dg_vy - sin(f_angle) * YGDM_V4,
                          method_fill_col,
                          method_edge_col);


     dg_vx = vx + cos(f_angle + fixed_to_radians(pr->shape_str->external_angle [EXANGLE_HIGH] [vertex])) * YGDM_V1;
     dg_vy = vy + sin(f_angle + fixed_to_radians(pr->shape_str->external_angle [EXANGLE_HIGH] [vertex])) * YGDM_V1;

     dg_vx += cos(f_angle) * YGDM_V2;
     dg_vy += sin(f_angle) * YGDM_V2;

      add_outline_diamond_layer(1,
                          dg_vx + cos(f_angle) * YGDM_V2A,
                          dg_vy + sin(f_angle) * YGDM_V2A,
                          dg_vx + cos(f_angle) * YGDM_V2A + cos(f_angle - PI/6) * YGDM_V2B,
                          dg_vy + sin(f_angle) * YGDM_V2A + sin(f_angle - PI/6) * YGDM_V2B,
                          dg_vx + cos(f_angle + fixed_to_radians(pr->shape_str->external_angle [EXANGLE_HIGH] [vertex])) * YGDM_V3,
                          dg_vy + sin(f_angle + fixed_to_radians(pr->shape_str->external_angle [EXANGLE_HIGH] [vertex])) * YGDM_V3,
                          dg_vx - cos(f_angle) * YGDM_V4,
                          dg_vy - sin(f_angle) * YGDM_V4,
                          method_fill_col,
                          method_edge_col);

     break;

    case MTYPE_PR_STATIC:
     f_angle = pr->float_angle + pr->shape_str->vertex_angle_float [vertex];

     add_method_base_diamond(vx + cos(f_angle) * 4, vy + sin(f_angle) * 4, f_angle, pr->shape_str, pr->size, vertex, method_fill_col, method_edge_col);
     break;

/*     case MTYPE_PR_ACCEL_ANGLE:
      al_draw_filled_circle(vx, vy, 5, team_colours [team_colour] [TCOL_METHOD_FILL]);
      al_draw_circle(vx, vy, 5, team_colours [team_colour] [TCOL_METHOD_LINE1], 1);
      al_draw_filled_circle(vx + xpart(pr->angle + pr->method [pr->vertex_method [vertex]].ex_angle, 5), vy + ypart(pr->angle + pr->method [pr->vertex_method [vertex]].ex_angle, 5), 2, team_colours [team_colour] [TCOL_METHOD_FILL]);
      al_draw_circle(vx + xpart(pr->angle + pr->method [pr->vertex_method [vertex]].ex_angle, 5), vy + ypart(pr->angle + pr->method [pr->vertex_method [vertex]].ex_angle, 5), 2, team_colours [team_colour] [TCOL_METHOD_LINE1], 1);
      break;*/
     case MTYPE_PR_LINK:
      ;
      int other_proc = pr->method[pr->vertex_method [vertex]].data [MDATA_PR_LINK_INDEX];
/*      if (other_proc == -1
       || other_proc > pr->index)
      {

       f_angle = pr->float_angle + pr->shape_str->vertex_angle_float [vertex];

// V1 is protrusion for front points
#define HM_V1 -2
// V2 is extension along edge
#define HM_V2 5
// V3 is inward distance
#define HM_V3 7

       float h_vx = vx + cos(f_angle) * HM_V1;
       float h_vy = vy + sin(f_angle) * HM_V1;

       add_outline_diamond_layer(0,
                           h_vx,
                           h_vy,
                           h_vx + cos(f_angle + fixed_to_radians(pr->shape_str->external_angle [EXANGLE_LOW] [vertex])) * HM_V2,
                           h_vy + sin(f_angle + fixed_to_radians(pr->shape_str->external_angle [EXANGLE_LOW] [vertex])) * HM_V2,
                           h_vx - cos(f_angle) * HM_V3,
                           h_vy - sin(f_angle) * HM_V3,
                           h_vx + cos(f_angle + fixed_to_radians(pr->shape_str->external_angle [EXANGLE_HIGH] [vertex])) * HM_V2,
                           h_vy + sin(f_angle + fixed_to_radians(pr->shape_str->external_angle [EXANGLE_HIGH] [vertex])) * HM_V2,
                           method_fill_col,
                           method_edge_col);

      }
//       else*/
      if (other_proc != -1
       && other_proc > pr->index)
       {
        f_angle = pr->float_angle + pr->shape_str->vertex_angle_float [vertex];
        int other_proc_method = pr->method[pr->vertex_method [vertex]].data [MDATA_PR_LINK_OTHER_METHOD];
        struct procstruct* other_pr = &w.proc[other_proc];
        int other_proc_vertex = other_pr->method[other_proc_method].ex_vertex;

        float other_x = al_fixtof(other_pr->x - view.camera_x + view.centre_x);
        float other_y = al_fixtof(other_pr->y - view.camera_y + view.centre_y);

        float other_f_angle = other_pr->float_angle + other_pr->shape_str->vertex_angle_float [other_proc_vertex];

        float other_vx = other_x + cos(other_f_angle) * (other_pr->shape_str->vertex_dist_pixel [other_proc_vertex] - 1);
        float other_vy = other_y + sin(other_f_angle) * (other_pr->shape_str->vertex_dist_pixel [other_proc_vertex] - 1);

#define LINK_FILL_COL link_fill_col
        if (!start_fan(vx, vy, LINK_FILL_COL))
         break;

#define JOIN_DIST 12
#define LINK_NOTCH_SEPARATION 2
#define LINK_NOTCH_SIDE_SEP 2
#define LINK_LINE_COL main_edge_col

/*
     method_fill_col = colours.proc_fill [team_colour] [PROC_FILL_SHADES - 1] [0];
     join_fill_col = colours.proc_fill [team_colour] [PROC_FILL_SHADES - 5] [0];
     method_edge_col = colours.team [team_colour] [TCOL_METHOD_EDGE];
     method_medium_col = colours.proc_fill [team_colour] [3] [0];
     if (pr->redundancy == 0)
      base_fill_col = colours.team [team_colour] [TCOL_FILL_BASE];
       else
        base_fill_col = colours.proc_fill [team_colour] [12] [fill_colour];
     main_fill_col = colours.proc_fill [team_colour] [shade] [fill_colour];
     main_edge_col = colours.team [team_colour] [TCOL_MAIN_EDGE];
*/

        add_fan_vertex(vx + cos(f_angle + fixed_to_radians(pr->shape_str->external_angle [EXANGLE_PREVIOUS] [vertex])) * (pr->shape_str->vertex_notch_sidewards [vertex] - (pr->size + LINK_NOTCH_SIDE_SEP)),
                       vy + sin(f_angle + fixed_to_radians(pr->shape_str->external_angle [EXANGLE_PREVIOUS] [vertex])) * (pr->shape_str->vertex_notch_sidewards [vertex] - (pr->size + LINK_NOTCH_SIDE_SEP)),
                       LINK_FILL_COL);
/*        add_line(fan_buffer[fan_buffer_pos-1].x,
                 fan_buffer[fan_buffer_pos-1].y,
                 fan_buffer[fan_buffer_pos-2].x,
                 fan_buffer[fan_buffer_pos-2].y,
                 LINK_LINE_COL);*/

        add_fan_vertex(vx - cos(f_angle) * (pr->shape_str->vertex_notch_inwards [vertex] - (pr->size + LINK_NOTCH_SEPARATION)),
                       vy - sin(f_angle) * (pr->shape_str->vertex_notch_inwards [vertex] - (pr->size + LINK_NOTCH_SEPARATION)),
                       LINK_FILL_COL);
        add_line(fan_buffer[fan_buffer_pos-1].x,
                 fan_buffer[fan_buffer_pos-1].y,
                 fan_buffer[fan_buffer_pos-2].x,
                 fan_buffer[fan_buffer_pos-2].y,
                 LINK_LINE_COL);

        add_fan_vertex(vx + cos(f_angle + fixed_to_radians(pr->shape_str->external_angle [EXANGLE_NEXT] [vertex])) * (pr->shape_str->vertex_notch_sidewards [vertex] - (pr->size + LINK_NOTCH_SIDE_SEP)),
                       vy + sin(f_angle + fixed_to_radians(pr->shape_str->external_angle [EXANGLE_NEXT] [vertex])) * (pr->shape_str->vertex_notch_sidewards [vertex] - (pr->size + LINK_NOTCH_SIDE_SEP)),
                       LINK_FILL_COL);
        add_line(fan_buffer[fan_buffer_pos-1].x,
                 fan_buffer[fan_buffer_pos-1].y,
                 fan_buffer[fan_buffer_pos-2].x,
                 fan_buffer[fan_buffer_pos-2].y,
                 LINK_LINE_COL);

        add_fan_vertex(other_vx + cos(other_f_angle + fixed_to_radians(other_pr->shape_str->external_angle [EXANGLE_PREVIOUS] [other_proc_vertex])) * (other_pr->shape_str->vertex_notch_sidewards [other_proc_vertex] - (other_pr->size + LINK_NOTCH_SIDE_SEP)),
                       other_vy + sin(other_f_angle + fixed_to_radians(other_pr->shape_str->external_angle [EXANGLE_PREVIOUS] [other_proc_vertex])) * (other_pr->shape_str->vertex_notch_sidewards [other_proc_vertex] - (other_pr->size + LINK_NOTCH_SIDE_SEP)),
                       LINK_FILL_COL);
        add_line(fan_buffer[fan_buffer_pos-1].x,
                 fan_buffer[fan_buffer_pos-1].y,
                 fan_buffer[fan_buffer_pos-2].x,
                 fan_buffer[fan_buffer_pos-2].y,
                 LINK_LINE_COL);

        add_fan_vertex(other_vx - cos(other_f_angle) * (other_pr->shape_str->vertex_notch_inwards [other_proc_vertex] - (other_pr->size + LINK_NOTCH_SEPARATION)),
                       other_vy - sin(other_f_angle) * (other_pr->shape_str->vertex_notch_inwards [other_proc_vertex] - (other_pr->size + LINK_NOTCH_SEPARATION)),
                       LINK_FILL_COL);
        add_line(fan_buffer[fan_buffer_pos-1].x,
                 fan_buffer[fan_buffer_pos-1].y,
                 fan_buffer[fan_buffer_pos-2].x,
                 fan_buffer[fan_buffer_pos-2].y,
                 LINK_LINE_COL);

        add_fan_vertex(other_vx + cos(other_f_angle + fixed_to_radians(other_pr->shape_str->external_angle [EXANGLE_NEXT] [other_proc_vertex])) * (other_pr->shape_str->vertex_notch_sidewards [other_proc_vertex] - (other_pr->size + LINK_NOTCH_SIDE_SEP)),
                       other_vy + sin(other_f_angle + fixed_to_radians(other_pr->shape_str->external_angle [EXANGLE_NEXT] [other_proc_vertex])) * (other_pr->shape_str->vertex_notch_sidewards [other_proc_vertex] - (other_pr->size + LINK_NOTCH_SIDE_SEP)),
                       LINK_FILL_COL);
        add_line(fan_buffer[fan_buffer_pos-1].x,
                 fan_buffer[fan_buffer_pos-1].y,
                 fan_buffer[fan_buffer_pos-2].x,
                 fan_buffer[fan_buffer_pos-2].y,
                 LINK_LINE_COL);
        add_line(fan_buffer[fan_buffer_pos-1].x,
                 fan_buffer[fan_buffer_pos-1].y,
                 fan_buffer[fan_index[fan_index_pos].start_position + 1].x,
                 fan_buffer[fan_index[fan_index_pos].start_position + 1].y,
                 LINK_LINE_COL);

        finish_fan();

       }
      break;

     case MTYPE_PR_ALLOCATE:
//     case MTYPE_PR_PACKET:
     f_angle = pr->float_angle + pr->shape_str->vertex_angle_float [vertex];
/*
// V1 is width between bits
#define GDM_V1 4
// V2 is how far bits protrude outside proc shape
#define GDM_V2 3
// V2A is extra protrusion for front points
#define GDM_V2A 0
// V2A is extra protrusion for 2nd set of front points
#define GDM_V2B 6
// V3 is how far along edge to go
#define GDM_V3 17
// V4 is inward distance
#define GDM_V4 19
*/

// V1 is width between bits
#define GDM_V1 5
// V2 is how far bits protrude outside proc shape
#define GDM_V2 4
// V2A is extra protrusion for front points
#define GDM_V2A 0
// V2A is extra protrusion for 2nd set of front points
#define GDM_V2B 7
// V3 is how far along edge to go
#define GDM_V3 9
// V4 is inward distance
#define GDM_V4 12


//     dg_vx = vx + cos(f_angle + fixed_to_radians(pr->shape_str->external_angle [EXANGLE_LOW] [vertex])) * GDM_V1;
//     dg_vy = vy + sin(f_angle + fixed_to_radians(pr->shape_str->external_angle [EXANGLE_LOW] [vertex])) * GDM_V1;

//     dg_vx = vx - cos(f_angle) * 5;// - cos(f_angle) * (pr->shape_str->vertex_notch_inwards [vertex] - 15);
//     dg_vy = vy - sin(f_angle) * 5;// - sin(f_angle) * (pr->shape_str->vertex_notch_inwards [vertex] - 15);
//     dg_vy = vy - sin(f_angle) * 9;

/*
     dg_vx = vx - cos(f_angle) * (pr->shape_str->vertex_notch_inwards [vertex] / 2);
     dg_vy = vy - sin(f_angle) * (pr->shape_str->vertex_notch_inwards [vertex] / 2);


      add_outline_diamond_layer(1,
                          dg_vx,
                          dg_vy,
                          vx + cos(f_angle + fixed_to_radians(pr->shape_str->external_angle [EXANGLE_LOW] [vertex])) * (GDM_V1),
                          vy + sin(f_angle + fixed_to_radians(pr->shape_str->external_angle [EXANGLE_LOW] [vertex])) * (GDM_V1),
                          vx + cos(f_angle) * GDM_V2,
                          vy + sin(f_angle) * GDM_V2,
                          vx + cos(f_angle + fixed_to_radians(pr->shape_str->external_angle [EXANGLE_HIGH] [vertex])) * (GDM_V1),
                          vy + sin(f_angle + fixed_to_radians(pr->shape_str->external_angle [EXANGLE_HIGH] [vertex])) * (GDM_V1),
                          method_fill_col,
                          method_edge_col);
*/

//     add_method_base_diamond(vx, vy, f_angle, pr->shape_str, pr->size, vertex, method_fill_col, method_edge_col);

     dg_vx = vx + cos(f_angle + fixed_to_radians(pr->shape_str->external_angle [EXANGLE_LOW] [vertex])) * GDM_V1;
     dg_vy = vy + sin(f_angle + fixed_to_radians(pr->shape_str->external_angle [EXANGLE_LOW] [vertex])) * GDM_V1;

     dg_vx += cos(f_angle) * GDM_V2;
     dg_vy += sin(f_angle) * GDM_V2;

      add_outline_diamond_layer(1,
                          dg_vx + cos(f_angle) * GDM_V2A,
                          dg_vy + sin(f_angle) * GDM_V2A,
                          dg_vx + cos(f_angle) * GDM_V2A + cos(f_angle + PI/6) * GDM_V2B,
                          dg_vy + sin(f_angle) * GDM_V2A + sin(f_angle + PI/6) * GDM_V2B,
                          dg_vx + cos(f_angle + fixed_to_radians(pr->shape_str->external_angle [EXANGLE_LOW] [vertex])) * GDM_V3,
                          dg_vy + sin(f_angle + fixed_to_radians(pr->shape_str->external_angle [EXANGLE_LOW] [vertex])) * GDM_V3,
                          dg_vx - cos(f_angle) * GDM_V4,
                          dg_vy - sin(f_angle) * GDM_V4,
                          method_fill_col,
                          method_edge_col);


     dg_vx = vx + cos(f_angle + fixed_to_radians(pr->shape_str->external_angle [EXANGLE_HIGH] [vertex])) * GDM_V1;
     dg_vy = vy + sin(f_angle + fixed_to_radians(pr->shape_str->external_angle [EXANGLE_HIGH] [vertex])) * GDM_V1;

     dg_vx += cos(f_angle) * GDM_V2;
     dg_vy += sin(f_angle) * GDM_V2;

      add_outline_diamond_layer(1,
                          dg_vx + cos(f_angle) * GDM_V2A,
                          dg_vy + sin(f_angle) * GDM_V2A,
                          dg_vx + cos(f_angle) * GDM_V2A + cos(f_angle - PI/6) * GDM_V2B,
                          dg_vy + sin(f_angle) * GDM_V2A + sin(f_angle - PI/6) * GDM_V2B,
                          dg_vx + cos(f_angle + fixed_to_radians(pr->shape_str->external_angle [EXANGLE_HIGH] [vertex])) * GDM_V3,
                          dg_vy + sin(f_angle + fixed_to_radians(pr->shape_str->external_angle [EXANGLE_HIGH] [vertex])) * GDM_V3,
                          dg_vx - cos(f_angle) * GDM_V4,
                          dg_vy - sin(f_angle) * GDM_V4,
                          method_fill_col,
                          method_edge_col);

/*
     dg_vx = vx + cos(f_angle + fixed_to_radians(pr->shape_str->external_angle [EXANGLE_LOW] [vertex])) * GDM_V1;
     dg_vy = vy + sin(f_angle + fixed_to_radians(pr->shape_str->external_angle [EXANGLE_LOW] [vertex])) * GDM_V1;

     dg_vx += cos(f_angle) * GDM_V2;
     dg_vy += sin(f_angle) * GDM_V2;

      add_outline_diamond_layer(1,
                          dg_vx + cos(f_angle) * GDM_V2A,
                          dg_vy + sin(f_angle) * GDM_V2A,
                          dg_vx + cos(f_angle) * GDM_V2A + cos(f_angle + PI/6) * GDM_V2B,
                          dg_vy + sin(f_angle) * GDM_V2A + sin(f_angle + PI/6) * GDM_V2B,
                          dg_vx + cos(f_angle + fixed_to_radians(pr->shape_str->external_angle [EXANGLE_LOW] [vertex])) * GDM_V3,
                          dg_vy + sin(f_angle + fixed_to_radians(pr->shape_str->external_angle [EXANGLE_LOW] [vertex])) * GDM_V3,
                          dg_vx - cos(f_angle) * GDM_V4,
                          dg_vy - sin(f_angle) * GDM_V4,
                          method_fill_col,
                          method_edge_col);

     dg_vx = vx + cos(f_angle + fixed_to_radians(pr->shape_str->external_angle [EXANGLE_HIGH] [vertex])) * GDM_V1;
     dg_vy = vy + sin(f_angle + fixed_to_radians(pr->shape_str->external_angle [EXANGLE_HIGH] [vertex])) * GDM_V1;

     dg_vx += cos(f_angle) * GDM_V2;
     dg_vy += sin(f_angle) * GDM_V2;

      add_outline_diamond_layer(1,
                          dg_vx + cos(f_angle) * GDM_V2A,
                          dg_vy + sin(f_angle) * GDM_V2A,
                          dg_vx + cos(f_angle) * GDM_V2A + cos(f_angle - PI/6) * GDM_V2B,
                          dg_vy + sin(f_angle) * GDM_V2A + sin(f_angle - PI/6) * GDM_V2B,
                          dg_vx + cos(f_angle + fixed_to_radians(pr->shape_str->external_angle [EXANGLE_HIGH] [vertex])) * GDM_V3,
                          dg_vy + sin(f_angle + fixed_to_radians(pr->shape_str->external_angle [EXANGLE_HIGH] [vertex])) * GDM_V3,
                          dg_vx - cos(f_angle) * GDM_V4,
                          dg_vy - sin(f_angle) * GDM_V4,
                          method_fill_col,
                          method_edge_col);
*/
      break;

    }
   }
  }


  if (pr->virtual_method != -1)
  {
   if (pr->method[pr->virtual_method].data [MDATA_PR_VIRTUAL_STATE] > 0)
   {
//    int virtual_shade = 1;//(pr->method[pr->virtual_method].data [MDATA_PR_VIRTUAL_STATE] * (CLOUD_SHADES - 1)) / pr->method[pr->virtual_method].data [MDATA_PR_VIRTUAL_MAX];
    int virtual_shade = (pr->method[pr->virtual_method].data [MDATA_PR_VIRTUAL_STATE] * (CLOUD_SHADES - 4)) / pr->method[pr->virtual_method].data [MDATA_PR_VIRTUAL_MAX];
    float pulse_shrink;// = (float) pr->method[pr->virtual_method].data [MDATA_PR_VIRTUAL_PULSE] / 50;
//    if (pulse_shrink > 0.15)
//					 pulse_shrink = 0.15;
//				pulse_shrink = 0;
    virtual_shade += pr->method[pr->virtual_method].data [MDATA_PR_VIRTUAL_PULSE];
    if (virtual_shade > CLOUD_SHADES - 2) // - 2 because it is used with + 1 below
     virtual_shade = CLOUD_SHADES - 2;

    add_scaled_outline_shape(pr->shape_str, pr->float_angle, x, y,
                             colours.virtual_method [pr->player_index] [virtual_shade], colours.virtual_method [pr->player_index] [virtual_shade + 1], 1.2);// - pulse_shrink);

    if (pr->method[pr->virtual_method].data [MDATA_PR_VIRTUAL_CHARGE_PULSE] != 0)
				{
					pulse_shrink = 1.35 - (((float) pr->method[pr->virtual_method].data [MDATA_PR_VIRTUAL_CHARGE_PULSE] - 1) * 0.025);
					virtual_shade = (16 - pr->method[pr->virtual_method].data [MDATA_PR_VIRTUAL_CHARGE_PULSE]);
					if (pulse_shrink > 1.2)
					{
						pulse_shrink = 1.2;
						virtual_shade = pr->method[pr->virtual_method].data [MDATA_PR_VIRTUAL_CHARGE_PULSE] + 2;
					}
     if (virtual_shade > CLOUD_SHADES - 2)
      virtual_shade = CLOUD_SHADES - 2;
					add_scaled_outline(pr->shape_str, pr->float_angle, x, y, colours.virtual_method [pr->player_index] [virtual_shade + 1], pulse_shrink);
				}
   }
    else
    {
     if (pr->method[pr->virtual_method].data [MDATA_PR_VIRTUAL_STATE] < 0)
     {
#define VIRTUAL_DISAPPEAR_DURATION 8
/*      int virtual_disappear_time = VIRTUAL_METHOD_RECYCLE + pr->method[pr->virtual_method].data [MDATA_PR_VIRTUAL_STATE]; // the data value is -ve
      if (virtual_disappear_time < VIRTUAL_DISAPPEAR_DURATION)
      {
       int virtual_disappear_shade = (CLOUD_SHADES - 2) - (virtual_disappear_time * (CLOUD_SHADES - 1)) / VIRTUAL_DISAPPEAR_DURATION;
       if (virtual_disappear_shade < 0)
        virtual_disappear_shade = 0;
       add_scaled_outline_shape(pr->shape_str, pr->float_angle, x, y,
                                colours.packet [pr->player_index] [virtual_disappear_shade], colours.packet [pr->player_index] [virtual_disappear_shade + 1],
                                1.2 + (virtual_disappear_time * 0.2));

      }*/
     }

/*
     if (pr->method[pr->virtual_method].data [MDATA_PR_VIRTUAL_STATE] < 0)
     {
#define VIRTUAL_DISAPPEAR_DURATION 32
      int virtual_disappear_time = VIRTUAL_METHOD_RECYCLE + pr->method[pr->virtual_method].data [MDATA_PR_VIRTUAL_STATE]; // the data value is -ve
      virtual_disappear_time = VIRTUAL_DISAPPEAR_DURATION - virtual_disappear_time;
      if (virtual_disappear_time >= 0)
      {
       int virtual_disappear_shade = (virtual_disappear_time
																																						* (CLOUD_SHADES - 2)) / VIRTUAL_DISAPPEAR_DURATION;
       add_scaled_outline_shape(pr->shape_str, pr->float_angle, x, y,
                                colours.packet [
                                r->player_index] [virtual_disappear_shade], colours.packet [pr->player_index] [virtual_disappear_shade + 1],
                                (1.8 * virtual_disappear_time) / (VIRTUAL_DISAPPEAR_DURATION));

      }
     }
*/
    }

  }


// the proc_draw_list is checked later to see if any procs have things like selection indicators that need to be drawn after procs and packets etc.
  if (proc_draw_count < PROC_DRAW_SIZE)
  {
   proc_draw_list [proc_draw_count] = pr;
   proc_draw_x [proc_draw_count] = x;
   proc_draw_y [proc_draw_count] = y;
   proc_draw_count ++;
  }


//  al_draw_textf(font, base_col [COL_YELLOW] [SHADE_HIGH], x, y, 0, "%i", pr->index); 1/77


/*  add_outline_triangle(x + xpart(pr->angle, 30), y + ypart(pr->angle, 30),
                       x + xpart(pr->angle + ANGLE_3, 14), y + ypart(pr->angle + ANGLE_3, 14),
                       x + xpart(pr->angle - ANGLE_3, 14), y + ypart(pr->angle - ANGLE_3, 14),
                       COL_GREEN, SHADE_LOW, COL_GREEN, SHADE_HIGH);
*/
//    al_draw_filled_circle(x, y, 10, base_col [COL_GREEN] [SHADE_LOW]);
//    al_draw_circle(x, y, 10, base_col [COL_GREEN] [SHADE_MAX], 1);



 }

 draw_from_buffers(); // sends poly_buffer and line_buffer to the screen - do it here so that all packet graphics are drawn after procs
 draw_fans();

// al_unlock_bitmap(al_get_backbuffer(display));

 for (pk = 0; pk < w.max_packets; pk ++)
 {
// TO DO: optimise this to use blocks instead of searching the entire packet array!
  if (w.packet[pk].exists == 0)
   continue;

  pack = &w.packet [pk];

  x = al_fixtof(pack->x - view.camera_x) + (view.window_x/2);
  y = al_fixtof(pack->y - view.camera_y) + (view.window_y/2);

  if (x < -50 || x > view.window_x + 50
   || y < -50 || y > view.window_y + 50)
    continue;

  float front_dist;
  float side_dist;
  float back_dist;

  switch (pack->type)
  {
   default: // shouldn't be needed but avoids a compiler warning
   case PACKET_TYPE_BASIC:
    front_dist = 3 + packet_rand(pack, 3) + (pack->method_extensions [MEX_PR_PACKET_SPEED] * 2);
    side_dist = 2 + packet_rand(pack, 3) + (pack->method_extensions [MEX_PR_PACKET_POWER] * 2);
    back_dist = 6 + packet_rand(pack, 6) + (pack->method_extensions [MEX_PR_PACKET_RANGE] * 4);
    break;
   case PACKET_TYPE_DPACKET:
    front_dist = 2 + packet_rand(pack, 3) + (pack->method_extensions [MEX_PR_PACKET_SPEED] * 2);
    side_dist = 2 + packet_rand(pack, 2) + (pack->method_extensions [MEX_PR_PACKET_POWER] * 2);
    back_dist = 4 + packet_rand(pack, 5) + (pack->method_extensions [MEX_PR_PACKET_RANGE] * 3);
    break;
  }

  if (pack->time < 10)
  {
   front_dist += (10 - pack->time);
   side_dist += (10 - pack->time);

   if (pack->source_proc != -1)
			{
    zap_line(x, y,
										   al_fixtof(w.proc[pack->source_proc].x - view.camera_x) + (view.window_x/2)
													  + cos(w.proc[pack->source_proc].float_angle	+ w.proc[pack->source_proc].shape_str->vertex_angle_float [pack->source_vertex]) * (w.proc[pack->source_proc].shape_str->vertex_dist_pixel [pack->source_vertex])
													  + cos(w.proc[pack->source_proc].float_angle	+ fixed_to_radians(w.proc[pack->source_proc].method[pack->source_method].ex_angle)) * 9,
										   al_fixtof(w.proc[pack->source_proc].y - view.camera_y) + (view.window_y/2)
													  + sin(w.proc[pack->source_proc].float_angle	+ w.proc[pack->source_proc].shape_str->vertex_angle_float [pack->source_vertex]) * (w.proc[pack->source_proc].shape_str->vertex_dist_pixel [pack->source_vertex])
													  + sin(w.proc[pack->source_proc].float_angle	+ fixed_to_radians(w.proc[pack->source_proc].method[pack->source_method].ex_angle)) * 9,
												 pack->colour,
												 pack->prand_seed,
												 10 - pack->time,
												 3);
			}
  }
  add_outline_diamond(x + cos(fixed_to_radians(pack->angle)) * front_dist,
                      y + sin(fixed_to_radians(pack->angle)) * front_dist,
                      x + cos(fixed_to_radians(pack->angle + AFX_ANGLE_4)) * side_dist,
                      y + sin(fixed_to_radians(pack->angle + AFX_ANGLE_4)) * side_dist,
                      x - cos(fixed_to_radians(pack->angle)) * back_dist,
                      y - sin(fixed_to_radians(pack->angle)) * back_dist,
                      x + cos(fixed_to_radians(pack->angle - AFX_ANGLE_4)) * side_dist,
                      y + sin(fixed_to_radians(pack->angle - AFX_ANGLE_4)) * side_dist,
                      colours.packet [pack->colour] [5],
                      colours.packet [pack->colour] [3]);
  switch (pack->type)
  {
   case PACKET_TYPE_BASIC:
    front_dist = (3 + packet_rand(pack, 3)) / 2 + pack->method_extensions [MEX_PR_PACKET_SPEED];
    side_dist = (2 + packet_rand(pack, 3) + pack->method_extensions [MEX_PR_PACKET_POWER]);// / 2;
    back_dist = (4 + packet_rand(pack, 5)) / 2;// + pack->method_extensions [MEX_PR_PACKET_SPEED];
    break;
   case PACKET_TYPE_DPACKET:
    front_dist = (2 + packet_rand(pack, 3)) / 2 + pack->method_extensions [MEX_PR_PACKET_SPEED];
    side_dist = (2 + packet_rand(pack, 2) + pack->method_extensions [MEX_PR_PACKET_POWER]);
    back_dist = (3 + packet_rand(pack, 4)) / 2;// + pack->method_extensions [MEX_PR_PACKET_SPEED];
    break;
  }

  add_outline_diamond(x + cos(fixed_to_radians(pack->angle)) * front_dist,
                      y + sin(fixed_to_radians(pack->angle)) * front_dist,
                      x + cos(fixed_to_radians(pack->angle + AFX_ANGLE_4)) * side_dist,
                      y + sin(fixed_to_radians(pack->angle + AFX_ANGLE_4)) * side_dist,
                      x - cos(fixed_to_radians(pack->angle)) * back_dist,
                      y - sin(fixed_to_radians(pack->angle)) * back_dist,
                      x + cos(fixed_to_radians(pack->angle - AFX_ANGLE_4)) * side_dist,
                      y + sin(fixed_to_radians(pack->angle - AFX_ANGLE_4)) * side_dist,
                      colours.packet [pack->colour] [9],
                      colours.packet [pack->colour] [7]);
/*
  if (pack->method_extensions [MEX_PR_PACKET_RANGE] > 0)
  {
   x -= cos(fixed_to_radians(pack->angle)) * (12 + pack->method_extensions [MEX_PR_PACKET_SPEED]);
   y -= sin(fixed_to_radians(pack->angle)) * (12 + pack->method_extensions [MEX_PR_PACKET_SPEED]);



   front_dist = 1 + packet_rand(pack, 2);//(2 + grand(3));
   side_dist = (1 + packet_rand(pack, 3));
   back_dist = (pack->method_extensions [MEX_PR_PACKET_RANGE] + 2 + packet_rand(pack, 3));


  add_outline_diamond(x + cos(fixed_to_radians(pack->angle)) * front_dist,
                      y + sin(fixed_to_radians(pack->angle)) * front_dist,
                      x + cos(fixed_to_radians(pack->angle + AFX_ANGLE_4)) * side_dist,
                      y + sin(fixed_to_radians(pack->angle + AFX_ANGLE_4)) * side_dist,
                      x - cos(fixed_to_radians(pack->angle)) * back_dist,
                      y - sin(fixed_to_radians(pack->angle)) * back_dist,
                      x + cos(fixed_to_radians(pack->angle - AFX_ANGLE_4)) * side_dist,
                      y + sin(fixed_to_radians(pack->angle - AFX_ANGLE_4)) * side_dist,
                      colours.packet [pack->colour] [3],
                      colours.packet [pack->colour] [1]);


  }*/

 }


 int cl_size;
// int cl_size2;
 int cl_shade;
 int cl_col;
 float cl_angle;

 for (c = 0; c < w.max_clouds; c ++)
 {
// TO DO: optimise this to use blocks instead of searching the entire proc array!
  if (w.cloud[c].exists == 0)
   continue;

  cl = &w.cloud [c];

  x = al_fixtof(cl->x - view.camera_x) + (view.window_x/2);
  y = al_fixtof(cl->y - view.camera_y) + (view.window_y/2);


  if (x < -150 || x > view.window_x + 150
   || y < -150 || y > view.window_y + 150)
    continue;

  switch(cl->type)
  {
   case CLOUD_PROC_EXPLODE:
    draw_proc_explode_cloud(cl, x, y);
    break;
   case CLOUD_DEBUG_LINE:
    al_draw_line(x, y, cl->data [0] - al_fixtoi(view.camera_x) + (view.window_x/2), cl->data [1] - al_fixtoi(view.camera_y) + (view.window_y/2), colours.packet [cl->colour] [8], 1);
    break;
   case CLOUD_FAILED_NEW:
    draw_proc_fail_cloud(cl, x, y);
    break;
   case CLOUD_PACKET_MISS:
    cl_angle = fixed_to_radians(cl->angle) + PI;
    cl_col = cl->colour;

				cl_shade = cl->timeout;

				if (cl_shade >= CLOUD_SHADES - 2)
					cl_shade = CLOUD_SHADES - 3;

    float cl_length = (5 + cl->data [1]) * 2 + (cl->data [2] % 5);
    float cl_width = (3 + cl->data [0]) * 5 + (cl->data [2] % 4);

				float pm_dist_proportion = (float) (cl->timeout) / 16;
				if (pm_dist_proportion > 1)
					 pm_dist_proportion = 1;
				cl_length *= pm_dist_proportion;
				cl_width *= pm_dist_proportion;

    add_diamond_layer(1,
																						x + cos(cl_angle) * cl_length,
																						y + sin(cl_angle) * cl_length,
																						x + cos(cl_angle + PI/2) * cl_width,
																						y + sin(cl_angle + PI/2) * cl_width,
																						x - cos(cl_angle) * cl_length,
																						y - sin(cl_angle) * cl_length,
																						x + cos(cl_angle - PI/2) * cl_width,
																						y + sin(cl_angle - PI/2) * cl_width,
																						colours.packet [cl_col] [cl_shade / 2]);

				cl_length *= 0.7;
				cl_width *= 0.7;


    add_diamond_layer(1,
																						x + cos(cl_angle) * cl_length,
																						y + sin(cl_angle) * cl_length,
																						x + cos(cl_angle + PI/2) * cl_width,
																						y + sin(cl_angle + PI/2) * cl_width,
																						x - cos(cl_angle) * cl_length,
																						y - sin(cl_angle) * cl_length,
																						x + cos(cl_angle - PI/2) * cl_width,
																						y + sin(cl_angle - PI/2) * cl_width,
																						colours.packet [cl_col] [cl_shade]);

			break;

/*
    cl_angle = fixed_to_radians(cl->angle) + PI;
    cl_shade = cl->timeout / 2;
    if (cl_shade >= CLOUD_SHADES - 1)
     cl_shade = CLOUD_SHADES - 1;
    cl_size = cl->timeout;
    cl_size2 = (16 - cl->timeout) * 2 + 6 + cl->data [0];
    cl_col = cl->colour;


     add_simple_outline_diamond_layer(1,
                                     x,
                                     y,
                                     (cl->data [0] + cl_size) / 2,
                                     (cl->data [0] + cl_size) / 2,
                                     (cl->data [0] + cl_size) / 2,
                                     cl_angle,
                                     colours.packet [cl_col] [cl_shade],
                                     colours.packet [cl_col] [cl_shade / 2]);
    for (i = 0; i < 3; i ++)
    {
     add_simple_outline_diamond_layer(1,
                                     x + fxpart(cl_angle + ((PI*2) / 3) * i, cl_size2 + cl->data [0] + cl_size),
                                     y + fypart(cl_angle + ((PI*2) / 3) * i, cl_size2 + cl->data [0] + cl_size),
                                     cl->data [0] + cl_size,
                                     (cl->data [0] + cl_size) * 1.5,
                                     (cl->data [0] + cl_size) / 2,
                                     cl_angle + ((PI*2) / 3) * i,
                                     colours.packet [cl_col] [cl_shade],
                                     colours.packet [cl_col] [cl_shade / 2]);
    }/ *
    break;

/ *    cl_shade = cl->timeout / 3;
    if (cl_shade >= CLOUD_SHADES)
     cl_shade = CLOUD_SHADES - 1;
    al_draw_filled_circle(x, y, cl->timeout + 1, cloud_col [cl->colour] [cl_shade]);
    cl_shade = cl->timeout / 2;
    if (cl_shade >= CLOUD_SHADES)
     cl_shade = CLOUD_SHADES - 1;
    al_draw_filled_circle(x, y, cl->timeout / 2 + 1, cloud_col [cl->colour] [cl_shade]);
    break;*/
   case CLOUD_PACKET_HIT:
    cl_angle = fixed_to_radians(cl->angle) + PI;
    cl_shade = cl->timeout / 2;
    if (cl_shade >= CLOUD_SHADES - 1)
     cl_shade = CLOUD_SHADES - 1;
    cl_size = cl->timeout;
    cl_col = cl->colour;
    add_simple_outline_diamond_layer(1,
                                     x,
                                     y,
                                     (cl->data [0] + cl_size) / 2,
                                     (cl->data [0] + cl_size) / 2,
                                     (cl->data [0] + cl_size) / 2,
                                     cl_angle,
                                     colours.packet [cl_col] [cl_shade],
                                     colours.packet [cl_col] [cl_shade / 2]);

    int time_expended = 16 - cl->timeout;

    float centre_dist = 3 * 8 + time_expended;// + (cl->data [2] % 8);

    float centre_x = x + cos(cl_angle) * centre_dist;
    float centre_y = y + sin(cl_angle) * centre_dist;

    float cl_outer_dist = 7 + cl->data [1] + (cl->data [2] % 5);
    float cl_width_dist = (3 + cl->data [0]) * 3 + (cl->data [2] % 4);
    float cl_inner_dist = (3 + cl->data [1]) * 8;// + (cl->data [2] % 7);//(3 + cl->data [1]) * 8 + (cl->data [2] % 7); ////centre_dist - 6 - (cl->data [2] % 4);

				cl_shade = cl->timeout;

				if (cl_shade >= CLOUD_SHADES - 2)
					cl_shade = CLOUD_SHADES - 3;

				float dist_proportion = (float) (cl->timeout) / 16;
				if (dist_proportion > 1)
					 dist_proportion = 1;
				cl_outer_dist *= dist_proportion;
				cl_width_dist *= dist_proportion;
				cl_inner_dist *= dist_proportion;

//				if (time_expended > 8)
//					 cl_shade -= (time_expended-8);

    add_diamond_layer(1,
																						centre_x + cos(cl_angle) * cl_outer_dist,
																						centre_y + sin(cl_angle) * cl_outer_dist,
																						centre_x + cos(cl_angle + PI/2) * cl_width_dist,
																						centre_y + sin(cl_angle + PI/2) * cl_width_dist,
																						centre_x - cos(cl_angle) * cl_inner_dist,
																						centre_y - sin(cl_angle) * cl_inner_dist,
																						centre_x + cos(cl_angle - PI/2) * cl_width_dist,
																						centre_y + sin(cl_angle - PI/2) * cl_width_dist,
																						colours.packet [cl_col] [cl_shade / 2]);

				cl_outer_dist *= 0.7;
				cl_width_dist *= 0.7;
				cl_inner_dist *= 0.7;


    add_diamond_layer(1,
																						centre_x + cos(cl_angle) * cl_outer_dist,
																						centre_y + sin(cl_angle) * cl_outer_dist,
																						centre_x + cos(cl_angle + PI/2) * cl_width_dist,
																						centre_y + sin(cl_angle + PI/2) * cl_width_dist,
																						centre_x - cos(cl_angle) * cl_inner_dist,
																						centre_y - sin(cl_angle) * cl_inner_dist,
																						centre_x + cos(cl_angle - PI/2) * cl_width_dist,
																						centre_y + sin(cl_angle - PI/2) * cl_width_dist,
																						colours.packet [cl_col] [cl_shade]);
    break;

   case CLOUD_PACKET_HIT_VIRTUAL:
    cl_angle = fixed_to_radians(cl->angle) + PI;
    cl_shade = cl->timeout / 2;
    if (cl_shade >= CLOUD_SHADES - 1)
     cl_shade = CLOUD_SHADES - 1;
    cl_size = cl->timeout;
    cl_col = cl->colour;
/*    add_simple_outline_diamond_layer(1,
                                     x,
                                     y,
                                     (cl->data [0] + cl_size) / 2,
                                     (cl->data [0] + cl_size) / 2,
                                     (cl->data [0] + cl_size) / 2,
                                     cl_angle,
                                     colours.packet [cl_col] [cl_shade],
                                     colours.packet [cl_col] [cl_shade / 2]);
*/
//    int phv_time_expended = 16 - cl->timeout;

    centre_dist = 3 * 8;// + phv_time_expended;// + (cl->data [2] % 8);

    float phv_centre_x = x + cos(cl_angle) * centre_dist;
    float phv_centre_y = y + sin(cl_angle) * centre_dist;

    float phv_cl_outer_dist = 7 + cl->data [1] + (cl->data [2] % 5);
    float phv_cl_width_dist = (3 + cl->data [0]) * 4 + (cl->data [2] % 4);
    float phv_cl_inner_dist = (3 + cl->data [1]) * 8;// + (cl->data [2] % 7);//(3 + cl->data [1]) * 8 + (cl->data [2] % 7); ////centre_dist - 6 - (cl->data [2] % 4);

				float phv_dist_proportion = (float) (cl->timeout) / 16;
				if (phv_dist_proportion > 1)
					 phv_dist_proportion = 1;
				phv_cl_outer_dist *= phv_dist_proportion;
				phv_cl_width_dist *= phv_dist_proportion;
				phv_cl_inner_dist *= phv_dist_proportion;

    add_outline_diamond_layer(1,
																						phv_centre_x + cos(cl_angle) * phv_cl_outer_dist,
																						phv_centre_y + sin(cl_angle) * phv_cl_outer_dist,
																						phv_centre_x + cos(cl_angle + PI/2) * phv_cl_width_dist,
																						phv_centre_y + sin(cl_angle + PI/2) * phv_cl_width_dist,
																						phv_centre_x - cos(cl_angle) * phv_cl_inner_dist,
																						phv_centre_y - sin(cl_angle) * phv_cl_inner_dist,
																						phv_centre_x + cos(cl_angle - PI/2) * phv_cl_width_dist,
																						phv_centre_y + sin(cl_angle - PI/2) * phv_cl_width_dist,
																						colours.packet [cl_col] [2],
																						colours.packet [cl_col] [4]);

    break;


/*
    int time_expended = (8 - cl->timeout) * 2;
    float centre_dist = time_expended * (3 + cl->data [1]);

    float centre_x = x + cos(cl_angle) * centre_dist;
    float centre_y = y + sin(cl_angle) * centre_dist;
    float cl_outer_dist = ((float) cl->timeout / 1) + cl->data [0];

    float cl_width_dist = (time_expended) + 7;
    if (time_expended > 9)
					 cl_width_dist -= (time_expended - 9) * 3;

    float cl_inner_dist = (time_expended - 6) * (2 + cl->data [1]);
    if (cl_inner_dist < 0)
					 cl_inner_dist = 0;
				if (cl_inner_dist > centre_dist - 4)
					 cl_inner_dist = centre_dist - 4;

				cl_shade = 8;

				if (time_expended > 12)
					 cl_shade -= (time_expended-12) * 2;


    add_diamond_layer(1,
																						centre_x + cos(cl_angle) * cl_outer_dist * 3,
																						centre_y + sin(cl_angle) * cl_outer_dist * 3,
																						centre_x + cos(cl_angle + PI/2) * cl_width_dist * 1,
																						centre_y + sin(cl_angle + PI/2) * cl_width_dist * 1,
																						x + cos(cl_angle) * cl_inner_dist * 2,
																						y + sin(cl_angle) * cl_inner_dist * 2,
																						centre_x + cos(cl_angle - PI/2) * cl_width_dist * 1,
																						centre_y + sin(cl_angle - PI/2) * cl_width_dist * 1,
																						colours.packet [cl_col] [cl_shade / 2]);

    add_diamond_layer(1,
																						centre_x + cos(cl_angle) * cl_outer_dist * 2,
																						centre_y + sin(cl_angle) * cl_outer_dist * 2,
																						centre_x + cos(cl_angle + PI/2) * cl_width_dist / 2,
																						centre_y + sin(cl_angle + PI/2) * cl_width_dist / 2,
																						x + cos(cl_angle) * cl_inner_dist * 2,
																						y + sin(cl_angle) * cl_inner_dist * 2,
																						centre_x + cos(cl_angle - PI/2) * cl_width_dist / 2,
																						centre_y + sin(cl_angle - PI/2) * cl_width_dist / 2,
																						colours.packet [cl_col] [cl_shade]);
*/

   case CLOUD_DATA_LINE:
//    cl_shade = cl->timeout;

//    draw_allocate_beam2(x, y, x + cl->data [0], y + cl->data [1], cl->colour, cl->data [0] * cl->data [1], cl->timeout);
    draw_allocate_beam2(x, y, x + cl->data [0], y + cl->data [1], cl->colour, cl->data [2], cl->timeout);
    break;

   case CLOUD_YIELD_LINE:
    draw_yield_beam(x, y, cl->data [0], cl->colour, cl->data [1], cl->timeout);
    break;

/*
//    line_thickness = 1;
    if (cl->data [2] == 0) // first line is vertical
    {
/ *      add_line(x, y, x + cl->data [3], y, cloud_col [cl->colour] [cl_shade]);
      add_line(x + cl->data [3], y, x + cl->data [3], y + cl->data [1], cloud_col [cl->colour] [cl_shade]);
      add_line(x + cl->data [3], y + cl->data [1], x + cl->data [0], y + cl->data [1], cloud_col [cl->colour] [cl_shade]);* /
      al_draw_filled_circle(x, y, cl->timeout + 1, colours.packet [cl->colour] [cl_shade]);
//      al_draw_line(x, y, x + cl->data [3], y, colours.packet [cl->colour] [cl_shade], line_thickness);
//      al_draw_line(x + cl->data [3], y, x + cl->data [3], y + cl->data [1], colours.packet [cl->colour] [cl_shade], line_thickness);
//      al_draw_line(x + cl->data [3], y + cl->data [1], x + cl->data [0], y + cl->data [1], colours.packet [cl->colour] [cl_shade], line_thickness);
      al_draw_line(x, y, x + cl->data [0], y + cl->data [1], colours.packet [cl->colour] [cl_shade], line_thickness);
      al_draw_filled_circle(x + cl->data [0], y + cl->data [1], cl->timeout + 1, colours.packet [cl->colour] [cl_shade]);
    }
     else
     {
/ *      add_line(x, y, x, y + cl->data [3], cloud_col [cl->colour] [cl_shade]);
      add_line(x, y + cl->data [3], x + cl->data [0], y + cl->data [3], cloud_col [cl->colour] [cl_shade]);
      add_line(x + cl->data [0], y + cl->data [3], x + cl->data [0], y + cl->data [1], cloud_col [cl->colour] [cl_shade]);* /
      al_draw_filled_circle(x, y, cl->timeout + 1, colours.packet [cl->colour] [cl_shade]);
//      al_draw_line(x, y, x, y + cl->data [3], colours.packet [cl->colour] [cl_shade], line_thickness);
//      al_draw_line(x, y + cl->data [3], x + cl->data [0], y + cl->data [3], colours.packet [cl->colour] [cl_shade], line_thickness);
//      al_draw_line(x + cl->data [0], y + cl->data [3], x + cl->data [0], y + cl->data [1], colours.packet [cl->colour] [cl_shade], line_thickness);
      al_draw_line(x, y, x + cl->data [0], y + cl->data [1], colours.packet [cl->colour] [cl_shade], line_thickness);
      al_draw_filled_circle(x + cl->data [0], y + cl->data [1], cl->timeout + 1, colours.packet [cl->colour] [cl_shade]);
     }
    break;*/
   case CLOUD_PROC_EXPLODE_LARGE:
    cl_shade = 5;
    int v, next_vertex;
    struct shapestruct* sh = &shape_dat [cl->data[0]] [cl->data[1]];
    float f_angle = fixed_to_radians(cl->angle);
    cl_shade = cl->timeout / 2;
    if (cl_shade > CLOUD_SHADES - 1)
     cl_shade = CLOUD_SHADES - 1;
    cl_size = cl->timeout * 2;
    for (v = 0; v < sh->vertices; v ++)
    {
     next_vertex = v + 1;
     if (next_vertex == sh->vertices)
      next_vertex = 0;
     add_triangle(x, y,
                  x + fxpart(f_angle + sh->vertex_angle_float [v], sh->vertex_dist_pixel [v] + cl_size),
                  y + fypart(f_angle + sh->vertex_angle_float [v], sh->vertex_dist_pixel [v] + cl_size),
                  x + fxpart(f_angle + sh->vertex_angle_float [next_vertex], sh->vertex_dist_pixel [next_vertex] + cl_size),
                  y + fypart(f_angle + sh->vertex_angle_float [next_vertex], sh->vertex_dist_pixel [next_vertex] + cl_size),
                  colours.packet [cl->colour] [cl_shade / 2]);

    }
    cl_size = cl->timeout;
    for (v = 0; v < sh->vertices; v ++)
    {
     next_vertex = v + 1;
     if (next_vertex == sh->vertices)
      next_vertex = 0;
     add_triangle(x, y,
                  x + fxpart(f_angle + sh->vertex_angle_float [v], sh->vertex_dist_pixel [v] + cl_size),
                  y + fypart(f_angle + sh->vertex_angle_float [v], sh->vertex_dist_pixel [v] + cl_size),
                  x + fxpart(f_angle + sh->vertex_angle_float [next_vertex], sh->vertex_dist_pixel [next_vertex] + cl_size),
                  y + fypart(f_angle + sh->vertex_angle_float [next_vertex], sh->vertex_dist_pixel [next_vertex] + cl_size),
                  colours.packet [cl->colour] [cl_shade]);

    }
    int dist;
    int time_up = 23 - cl->timeout;
    for (i = 2; i < CLOUD_DATA; i ++)
    {
     cl_shade = cl->timeout;
     if (cl_shade > CLOUD_SHADES - 3)
      cl_shade = CLOUD_SHADES - 3;
     cl_size = cl->timeout;
//     dist = i * time_up + 15 - time_up * 3;
     dist = xpart(-ANGLE_4 + time_up * (ANGLE_4 / 23), 40 + i *10) + 10 + 2*i;
     add_simple_outline_diamond_layer(1,
                                      x + fxpart(angle_to_radians(cl->data[i]), dist),
                                      y + fypart(angle_to_radians(cl->data[i]), dist),
                                      cl_size, cl_size, cl_size / 2,
                                      angle_to_radians(cl->data [i] + time_up * (i - 7) * 34),
//                                      cl_size, cl_size * 2, cl_size / 2,
//                                      angle_to_radians(cl->data [i]),
                                      colours.packet [cl->colour] [cl_shade], colours.packet [cl->colour] [cl_shade]);
//void add_simple_outline_diamond_layer(int layer, float x, float y, float front_length, float back_length, float width, float angle, colours.packet [cl->colour] [cl_shade], ALLEGRO_COLOR edge_col)
    }
    break;
   case CLOUD_PROC_EXPLODE_SMALL:
    cl_shade = 5;
//    add_simple_outline_diamond_layer(1, x, y, cl->data [0], cl->data [0], cl->data [1], fixed_to_radians(cl->angle), cloud_col [cl->colour] [cl_shade], cloud_col [cl->colour] [cl_shade]);
    add_simple_outline_diamond_layer(1, x, y, cl->data [0] / 2, cl->data [0] / 2, cl->data [1] / 2, fixed_to_radians(cl->angle), colours.team [cl->colour] [TCOL_FILL_BASE], colours.team [cl->colour] [TCOL_MAIN_EDGE]);
    break;
   case CLOUD_PROC_EXPLODE_SMALL2:
    cl_shade = cl->timeout / 2;
    if (cl_shade > CLOUD_SHADES - 1)
     cl_shade = CLOUD_SHADES - 1;
    cl_size = cl->timeout / 4;
    cl_col = cl->data [4]; // cl->data [4] is index of player that destroyed this proc
    add_simple_outline_diamond_layer(1, x, y, cl->data [0] + cl_size, cl->data [0] + cl_size, cl->data [1] + cl_size, fixed_to_radians(cl->angle), colours.packet [cl_col] [cl_shade], colours.packet [cl_col] [cl_shade / 2]);
    break;
   case CLOUD_STREAM:
   case CLOUD_DSTREAM:
    draw_stream_beam(x, y, al_fixtof(cl->x2 - view.camera_x + (view.centre_x)), al_fixtof(cl->y2 - view.camera_y + (view.centre_y)), cl->colour, cl->data[0], cl->data[1], cl->data [2]);
//    al_draw_textf(font, colours.base [COL_GREY] [SHADE_MAX], x + 10, y + 10, ALLEGRO_ALIGN_CENTRE, "%i %i %f,%f %f,%f", cl->data[0], cl->data[1], x, y, al_fixtof(cl->x2 - view.camera_x + (view.centre_x)), al_fixtof(cl->y2 - view.camera_y + (view.centre_y)));
    break;
   case CLOUD_VIRTUAL_BREAK:
// data: 0: shape 2: size
    ;
    struct shapestruct* cl_shape = &shape_dat [cl->data [0]] [cl->data [1]];
    float vb_vx, vb_vy;
//    float vb_count = (16 - cl->timeout);
    cl_angle = fixed_to_radians(cl->angle);
    float vertex_angle;
    float vb_proportion = (float) cl->timeout / 16;
    float vb_dist;
//    float vb_dist2;
    int vb_shade = cl->timeout / 2;// / 2;// / 3;
    if (vb_shade > CLOUD_SHADES - 2)
					 vb_shade = CLOUD_SHADES - 2;
				int v2;
//				float vb_angle_inc = PI / cl_shape->vertices;
				vertex_angle = cl_angle;
    for (i = 0; i < cl_shape->vertices; i ++)
				{

     v = i;// / 2;
     v2 = v + 1;
					if (v2 >= cl_shape->vertices)
					 v2 = 0;
					vertex_angle = cl_angle + cl_shape->vertex_angle_float [v];
					vb_dist = 16 - cl->timeout + 5 + (cl->x + (cl->y * i)) % 37;//20;//((int) (cl->x * vertex_angle) % 37);// * (1-vb_proportion);
//					vb_vx = x + cos(vertex_angle) * (vb_count * 4 + cl_shape->vertex_dist_pixel [i]);
//					vb_vy = y + sin(vertex_angle) * (vb_count * 4 + cl_shape->vertex_dist_pixel [i]);
					vb_vx = x + cos(vertex_angle) * (vb_dist + cl_shape->vertex_dist_pixel [v]);
					vb_vy = y + sin(vertex_angle) * (vb_dist + cl_shape->vertex_dist_pixel [v]);

     add_outline_diamond_layer(1,
																						vb_vx,
																						vb_vy,
																						vb_vx + cos(fixed_to_radians(cl_shape->external_angle [EXANGLE_PREVIOUS] [v]) + vertex_angle) * (cl_shape->dist_to_previous [v]*vb_proportion) * 0.4 * vb_proportion,
																						vb_vy + sin(fixed_to_radians(cl_shape->external_angle [EXANGLE_PREVIOUS] [v]) + vertex_angle) * (cl_shape->dist_to_previous [v]*vb_proportion) * 0.4 * vb_proportion,
																						vb_vx - cos(vertex_angle) * (vb_dist + cl_shape->vertex_dist_pixel [v]) * vb_proportion,
																						vb_vy - sin(vertex_angle) * (vb_dist + cl_shape->vertex_dist_pixel [v]) * vb_proportion,
//																						x + cos(vertex_angle) * vb_dist,
//																						y + sin(vertex_angle) * vb_dist,
																						vb_vx + cos(fixed_to_radians(cl_shape->external_angle [EXANGLE_NEXT] [v]) + vertex_angle) * (cl_shape->dist_to_next [v]*vb_proportion) * 0.4 * vb_proportion,
																						vb_vy + sin(fixed_to_radians(cl_shape->external_angle [EXANGLE_NEXT] [v]) + vertex_angle) * (cl_shape->dist_to_next [v]*vb_proportion) * 0.4 * vb_proportion,
																						colours.virtual_method [cl->colour] [vb_shade],
																						colours.virtual_method [cl->colour] [vb_shade + 1]);

					vertex_angle = cl_angle + ((cl_shape->vertex_angle_float [v] + cl_shape->vertex_angle_float [v2]) * 0.5);
					if (v2 == 0)
						 vertex_angle += PI;
					vb_dist = 16 - cl->timeout + 5 + (cl->y + (cl->x * v2)) % 37;//5;//((int) (cl->x * vertex_angle) % 37);// * (1-vb_proportion);
					vb_vx = x + cos(vertex_angle) * (vb_dist + (cl_shape->vertex_dist_pixel [v] + cl_shape->vertex_dist_pixel [v2]) / 2);
					vb_vy = y + sin(vertex_angle) * (vb_dist + (cl_shape->vertex_dist_pixel [v] + cl_shape->vertex_dist_pixel [v2]) / 2);

     add_outline_triangle_layer(1,
																						vb_vx + cos(fixed_to_radians(cl_shape->external_angle [EXANGLE_NEXT] [v]) + vertex_angle) * (cl_shape->dist_to_next [v]) * 0.4 * vb_proportion,
																						vb_vy + sin(fixed_to_radians(cl_shape->external_angle [EXANGLE_NEXT] [v]) + vertex_angle) * (cl_shape->dist_to_next [v]) * 0.4 * vb_proportion,
																						vb_vx - cos(vertex_angle) * (vb_dist + (cl_shape->vertex_dist_pixel [v] + cl_shape->vertex_dist_pixel [v2]) / 2) * vb_proportion,
																						vb_vy - sin(vertex_angle) * (vb_dist + (cl_shape->vertex_dist_pixel [v] + cl_shape->vertex_dist_pixel [v2]) / 2) * vb_proportion,
																						vb_vx + cos(fixed_to_radians(cl_shape->external_angle [EXANGLE_PREVIOUS] [v2]) + vertex_angle) * (cl_shape->dist_to_previous [v2]) * 0.4 * vb_proportion,
																						vb_vy + sin(fixed_to_radians(cl_shape->external_angle [EXANGLE_PREVIOUS] [v2]) + vertex_angle) * (cl_shape->dist_to_previous [v2]) * 0.4 * vb_proportion,
																						colours.virtual_method [cl->colour] [vb_shade],
																						colours.virtual_method [cl->colour] [vb_shade + 1]);


				}


/*
    for (i = 0; i < cl_shape->vertices * 2; i ++)
				{
					v = i / 2;
					v2 = i + 1;
					if (v2 > cl_shape->vertices)
					 v2 = 0;
					vertex_angle = cl_angle + cl_shape->vertex_angle_float [v];
					vb_dist = (int) (cl->x * vertex_angle) % 37;
//					vb_vx = x + cos(vertex_angle) * (vb_count * 4 + cl_shape->vertex_dist_pixel [i]);
//					vb_vy = y + sin(vertex_angle) * (vb_count * 4 + cl_shape->vertex_dist_pixel [i]);
					vb_vx = x + cos(vertex_angle) * (vb_dist + cl_shape->vertex_dist_pixel [v]);
					vb_vy = y + sin(vertex_angle) * (vb_dist + cl_shape->vertex_dist_pixel [v]);

     add_outline_diamond_layer(1,
																						vb_vx,
																						vb_vy,
																						vb_vx + cos(fixed_to_radians(cl_shape->external_angle [EXANGLE_PREVIOUS] [v]) + vertex_angle) * (cl_shape->dist_to_previous [v]*vb_proportion) * 0.25 * vb_proportion,
																						vb_vy + sin(fixed_to_radians(cl_shape->external_angle [EXANGLE_PREVIOUS] [v]) + vertex_angle) * (cl_shape->dist_to_previous [v]*vb_proportion) * 0.25 * vb_proportion,
																						x + cos(vertex_angle) * vb_dist,
																						y + sin(vertex_angle) * vb_dist,
																						vb_vx + cos(fixed_to_radians(cl_shape->external_angle [EXANGLE_NEXT] [v]) + vertex_angle) * (cl_shape->dist_to_next [v]*vb_proportion) * 0.25 * vb_proportion,
																						vb_vy + sin(fixed_to_radians(cl_shape->external_angle [EXANGLE_NEXT] [v]) + vertex_angle) * (cl_shape->dist_to_next [v]*vb_proportion) * 0.25 * vb_proportion,
																						colours.packet [cl->colour] [vb_shade / 2],
																						colours.packet [cl->colour] [vb_shade]);

					vertex_angle = cl_angle + (cl_shape->vertex_angle_float [v] + cl_shape->vertex_angle_float [v2]) / 2;
					vb_dist = (int) (cl->x * vertex_angle) % 37;
//					vb_vx = x + cos(vertex_angle) * (vb_count * 4 + cl_shape->vertex_dist_pixel [i]);
//					vb_vy = y + sin(vertex_angle) * (vb_count * 4 + cl_shape->vertex_dist_pixel [i]);
					vb_vx = x + cos(vertex_angle) * (vb_dist + cl_shape->vertex_dist_pixel [v]);
					vb_vy = y + sin(vertex_angle) * (vb_dist + cl_shape->vertex_dist_pixel [v]);

     add_outline_triangle_layer(1,
																						vb_vx + cos(fixed_to_radians(cl_shape->external_angle [EXANGLE_NEXT] [v]) + vertex_angle) * (cl_shape->dist_to_next [v]) * 0.25 * vb_proportion,
																						vb_vy + sin(fixed_to_radians(cl_shape->external_angle [EXANGLE_NEXT] [v]) + vertex_angle) * (cl_shape->dist_to_next [v]) * 0.25 * vb_proportion,
																						x + cos(vertex_angle) * vb_dist,
																						y + sin(vertex_angle) * vb_dist,
																						vb_vx + cos(fixed_to_radians(cl_shape->external_angle [EXANGLE_PREVIOUS] [v2]) + vertex_angle) * (cl_shape->dist_to_previous [v2]) * 0.25 * vb_proportion,
																						vb_vy + sin(fixed_to_radians(cl_shape->external_angle [EXANGLE_PREVIOUS] [v2]) + vertex_angle) * (cl_shape->dist_to_previous [v2]) * 0.25 * vb_proportion,
																						colours.packet [cl->colour] [vb_shade / 2],
																						colours.packet [cl->colour] [vb_shade]);


				}
*/
/*
    for (i = 0; i < cl_shape->vertices; i ++)
				{
					vertex_angle = cl_angle + cl_shape->vertex_angle_float [i];
					vb_dist = (int) (cl->x * vertex_angle) % 37;
//					vb_vx = x + cos(vertex_angle) * (vb_count * 4 + cl_shape->vertex_dist_pixel [i]);
//					vb_vy = y + sin(vertex_angle) * (vb_count * 4 + cl_shape->vertex_dist_pixel [i]);
					vb_vx = x + cos(vertex_angle) * (vb_dist + cl_shape->vertex_dist_pixel [i]);
					vb_vy = y + sin(vertex_angle) * (vb_dist + cl_shape->vertex_dist_pixel [i]);

     add_outline_diamond_layer(1,
																						vb_vx,
																						vb_vy,
																						vb_vx + cos(fixed_to_radians(cl_shape->external_angle [EXANGLE_PREVIOUS] [i]) + vertex_angle) * (cl_shape->dist_to_previous [i]*vb_proportion) * 0.5 * vb_proportion,
																						vb_vy + sin(fixed_to_radians(cl_shape->external_angle [EXANGLE_PREVIOUS] [i]) + vertex_angle) * (cl_shape->dist_to_previous [i]*vb_proportion) * 0.5 * vb_proportion,
																						vb_vx - cos(vertex_angle) * cl_shape->vertex_dist_pixel [i] * vb_proportion,
																						vb_vy - sin(vertex_angle) * cl_shape->vertex_dist_pixel [i] * vb_proportion,
																						vb_vx + cos(fixed_to_radians(cl_shape->external_angle [EXANGLE_NEXT] [i]) + vertex_angle) * (cl_shape->dist_to_next [i]*vb_proportion) * 0.5 * vb_proportion,
																						vb_vy + sin(fixed_to_radians(cl_shape->external_angle [EXANGLE_NEXT] [i]) + vertex_angle) * (cl_shape->dist_to_next [i]*vb_proportion) * 0.5 * vb_proportion,
																						colours.packet [cl->colour] [vb_shade / 2],
																						colours.packet [cl->colour] [vb_shade]);

				}
*/
			 break;


  }

//  al_draw_bitmap_region(clouds_bmp, clouds_index[CLOUDS_BMP_BLOT_R_1].x, clouds_index[CLOUDS_BMP_BLOT_R_1].y, clouds_index[CLOUDS_BMP_BLOT_R_1].w, clouds_index[CLOUDS_BMP_BLOT_R_1].h, x - clouds_index[CLOUDS_BMP_BLOT_R_1].centre_x, y - clouds_index[CLOUDS_BMP_BLOT_R_1].centre_y, 0);

 }





// now we go through the proc draw list to check whether any of them have selection graphics or similar that need to be drawn:
 i = 0;

 while (i < proc_draw_count)
 {
  pr = proc_draw_list [i];

  if (pr->selected)
  {
   switch(pr->selected)
   {
// TO DO: different selection types (using a variable other than pr->selected)
    case 1:
    default:
     //al_draw_circle(proc_draw_x [i], proc_draw_y [i], 20, base_col [COL_GREY] [SHADE_HIGH], 1);
     add_line(proc_draw_x [i] - 50, proc_draw_y [i] - 40, proc_draw_x [i] - 40, proc_draw_y [i] - 50, colours.base [COL_GREY] [SHADE_MAX]);
     add_line(proc_draw_x [i] + 50, proc_draw_y [i] - 40, proc_draw_x [i] + 40, proc_draw_y [i] - 50, colours.base [COL_GREY] [SHADE_MAX]);
     add_line(proc_draw_x [i] - 50, proc_draw_y [i] + 40, proc_draw_x [i] - 40, proc_draw_y [i] + 50, colours.base [COL_GREY] [SHADE_MAX]);
     add_line(proc_draw_x [i] + 50, proc_draw_y [i] + 40, proc_draw_x [i] + 40, proc_draw_y [i] + 50, colours.base [COL_GREY] [SHADE_MAX]);
     break;


   }
  }

  i++;
 };

// now draw markers:

		 	float marker_angle;
		 	float marker_dist1 = 0, marker_dist2 = 0;
		 	int marker_shade = 0;
		 	int submarkers = 0;


 for (i = 0; i < MARKERS; i ++)
 {
  if (w.marker[i].active == 0
   || w.marker[i].type >= MARKER_MAP_POINT)
    continue;


// box markers need special treatment as normal bounds checking doesn't work for them
  if (w.marker[i].type == MARKER_BOX)
  {

// assign x, x2, y and y2 so that x < x2 and y < y2
   if (w.marker[i].x < w.marker[i].x2)
   {
    x = al_fixtof(w.marker[i].x - view.camera_x) + (view.window_x/2);
    x2 = al_fixtof(w.marker[i].x2 - view.camera_x) + (view.window_x/2);
   }
    else
    {
     x2 = al_fixtof(w.marker[i].x - view.camera_x) + (view.window_x/2);
     x = al_fixtof(w.marker[i].x2 - view.camera_x) + (view.window_x/2);
    }
   if (w.marker[i].y < w.marker[i].y2)
   {
    y = al_fixtof(w.marker[i].y - view.camera_y) + (view.window_y/2);
    y2 = al_fixtof(w.marker[i].y2 - view.camera_y) + (view.window_y/2);
   }
    else
    {
     y2 = al_fixtof(w.marker[i].y - view.camera_y) + (view.window_y/2);
     y = al_fixtof(w.marker[i].y2 - view.camera_y) + (view.window_y/2);
    }
// exclude boxes completely outside screen:
   if (x > view.window_x
    || x2 < 0
    || y > view.window_y
    || y2 < 0)
     continue;
// box could still be around screen, so make sure the worst case is drawing a box just around the screen:
   if (x < 0)
    x = -1;
   if (x2 > view.window_x)
    x2 = view.window_x;
   if (y < 0)
    y = -1;
   if (y2 > view.window_y)
    y2 = view.window_y;
// now draw:
   add_line(x, y, x2, y, colours.base [w.marker[i].colour] [SHADE_HIGH]);
   add_line(x, y, x, y2, colours.base [w.marker[i].colour] [SHADE_HIGH]);
   add_line(x2, y2, x2, y, colours.base [w.marker[i].colour] [SHADE_HIGH]);
   add_line(x2, y2, x, y2, colours.base [w.marker[i].colour] [SHADE_HIGH]);
   continue;
  }


  x = al_fixtof(w.marker[i].x - view.camera_x) + (view.window_x/2);
  y = al_fixtof(w.marker[i].y - view.camera_y) + (view.window_y/2);

  if (x < -80 || x > view.window_x + 80
   || y < -80 || y > view.window_y + 80)
    continue;

  switch(w.marker[i].type)
  {
  	default:
		 case MARKER_BASIC:
		  submarkers = 2; // the actual number is 2x this
		 case MARKER_BASIC_6:
		 case MARKER_BASIC_8:
		 	if (w.marker[i].type == MARKER_BASIC_6)
					submarkers = 3;
		 	if (w.marker[i].type == MARKER_BASIC_8)
					submarkers = 4;
				marker_angle = angle_to_radians(w.marker[i].angle);
				switch(w.marker[i].phase)
				{
				 case MARKER_PHASE_APPEAR:
					 marker_shade = CLOUD_SHADES - (w.marker[i].phase_counter * 2);
					 if (marker_shade < 0)
							 marker_shade = 0;
					 marker_dist1 = 32 + (w.marker[i].size * 4) + (w.marker[i].phase_counter * 6);
					 marker_dist2 = 12 - (w.marker[i].phase_counter);
					 break;
				 case MARKER_PHASE_EXISTS:
					 marker_shade = CLOUD_SHADES - 1;//CLOUD_SHADES-1;
					 marker_dist1 = 32 + (w.marker[i].size * 4);
					 marker_dist2 = 12;
					 break;
				 case MARKER_PHASE_FADE:
					 marker_shade = w.marker[i].phase_counter * 2;
					 if (marker_shade >= CLOUD_SHADES)
							 marker_shade = CLOUD_SHADES;
					 marker_dist1 = 32 + (w.marker[i].size * 4) + (MARKER_PHASE_TIME - w.marker[i].phase_counter) * 6; // 96 - (w.marker[i].phase_counter * 6);
					 marker_dist2 = (w.marker[i].phase_counter) + 4;
					 break;
				}
				for (j = 0; j < submarkers*2; j++)
				{
					x2 = x + cos(marker_angle) * marker_dist1;
					y2 = y + sin(marker_angle) * marker_dist1;
/*					add_triangle_layer(2,
																								x2,
																								y2,
																								x2 + cos(marker_angle - (PI/6)) * marker_dist2,
																								y2 + sin(marker_angle - (PI/6)) * marker_dist2,
																								x2 + cos(marker_angle + (PI/6)) * marker_dist2,
																								y2 + sin(marker_angle + (PI/6)) * marker_dist2,
																								colours.base [w.marker[i].colour] [marker_shade]);*/
					add_diamond_layer(2,
																								x2,
																								y2,
																								x2 + cos(marker_angle - (PI/6)) * marker_dist2,
																								y2 + sin(marker_angle - (PI/6)) * marker_dist2,
																								x2 + cos(marker_angle) * (marker_dist2 * 1.5),
																								y2 + sin(marker_angle) * (marker_dist2 * 1.5),
																								x2 + cos(marker_angle + (PI/6)) * marker_dist2,
																								y2 + sin(marker_angle + (PI/6)) * marker_dist2,
																								colours.base_fade [w.marker[i].colour] [marker_shade]);
 				marker_angle += PI / ((float) submarkers);
				}
			 break;
			case MARKER_PROC_BOX:
				marker_dist1 = 16 + (w.marker[i].size); // this is actually the size of the box
				switch(w.marker[i].phase)
				{
				 case MARKER_PHASE_APPEAR:
					 marker_shade = CLOUD_SHADES - (w.marker[i].phase_counter * 2);
					 if (marker_shade < 0)
							 marker_shade = 0;
					 marker_dist1 = 16 + (w.marker[i].size) + (w.marker[i].phase_counter * 6);
					 break;
				 case MARKER_PHASE_EXISTS:
					 marker_shade = CLOUD_SHADES - 1;//CLOUD_SHADES-1;
					 marker_dist1 = 16 + (w.marker[i].size);
					 break;
				 case MARKER_PHASE_FADE:
					 marker_shade = w.marker[i].phase_counter * 2;
					 if (marker_shade >= CLOUD_SHADES)
							 marker_shade = CLOUD_SHADES;
					 marker_dist1 = 16 + (w.marker[i].size) + (MARKER_PHASE_TIME - w.marker[i].phase_counter) * 6;
					 break;
				}
				if (w.marker[i].bind_to_proc != -1)
				{
					marker_dist1 += w.proc[w.marker[i].bind_to_proc].size * 12;
				}
    add_rect_layer(2, x - marker_dist1, y - marker_dist1, x + marker_dist1, y + marker_dist1, colours.base_fade [w.marker[i].colour] [marker_shade]);
    break;
		 case MARKER_CORNERS:
				marker_angle = angle_to_radians(w.marker[i].angle);
				switch(w.marker[i].phase)
				{
				 case MARKER_PHASE_APPEAR:
					 marker_shade = CLOUD_SHADES - (w.marker[i].phase_counter * 2);
					 if (marker_shade < 0)
							 marker_shade = 0;
					 marker_dist1 = 24 + (w.marker[i].size * 4) + (w.marker[i].phase_counter * 4);
					 marker_dist2 = 2 + (w.marker[i].size / 4) + (MARKER_PHASE_TIME - w.marker[i].phase_counter) * 2;
					 break;
				 case MARKER_PHASE_EXISTS:
					 marker_shade = CLOUD_SHADES - 1;//CLOUD_SHADES-1;
					 marker_dist1 = 24 + (w.marker[i].size * 4);
					 marker_dist2 = 2 + (w.marker[i].size / 4) + MARKER_PHASE_TIME * 2;
					 break;
				 case MARKER_PHASE_FADE:
					 marker_shade = w.marker[i].phase_counter * 2;
					 if (marker_shade >= CLOUD_SHADES)
							 marker_shade = CLOUD_SHADES;
					 marker_dist1 = 24 + (w.marker[i].size * 4) + ((MARKER_PHASE_TIME - w.marker[i].phase_counter) * 3);
					 marker_dist2 = 2 + (w.marker[i].size / 4) + (w.marker[i].phase_counter) * 2;
					 break;
				}
				for (j = 0; j < 4; j++)
				{
					x2 = x + (cos(marker_angle) * marker_dist1) + (cos(marker_angle + PI/2) * marker_dist2);
					y2 = y + (sin(marker_angle) * marker_dist1) + (sin(marker_angle + PI/2) * marker_dist2);
					add_diamond_layer(2,
																				   x + (cos(marker_angle) * marker_dist1) + (cos(marker_angle + PI/2) * marker_dist2),
																				   y + (sin(marker_angle) * marker_dist1) + (sin(marker_angle + PI/2) * marker_dist2),
																				   x + (cos(marker_angle) * (marker_dist1 + 6)),
																				   y + (sin(marker_angle) * (marker_dist1 + 6)),
																				   x + (cos(marker_angle) * marker_dist1) + (cos(marker_angle - PI/2) * marker_dist2),
																				   y + (sin(marker_angle) * marker_dist1) + (sin(marker_angle - PI/2) * marker_dist2),
																				   x + (cos(marker_angle) * (marker_dist1 + 2)),
																				   y + (sin(marker_angle) * (marker_dist1 + 2)),
																								colours.base_fade [w.marker[i].colour] [marker_shade]);
/*

					add_line_layer(2,
																				x + (cos(marker_angle) * marker_dist1) + (cos(marker_angle + PI/2) * marker_dist2),
																				y + (sin(marker_angle) * marker_dist1) + (sin(marker_angle + PI/2) * marker_dist2),
																				x + (cos(marker_angle) * marker_dist1) + (cos(marker_angle - PI/2) * marker_dist2),
																				y + (sin(marker_angle) * marker_dist1) + (sin(marker_angle - PI/2) * marker_dist2),
																				colours.base_fade [w.marker[i].colour] [marker_shade]);*/
 				marker_angle += PI / 2;
				}
			 break;
		 case MARKER_SHARP:
		  submarkers = 3;
		 case MARKER_SHARP_5:
		 case MARKER_SHARP_7:
		 	if (w.marker[i].type == MARKER_SHARP_5)
					submarkers = 5;
		 	if (w.marker[i].type == MARKER_SHARP_7)
					submarkers = 7;
				marker_angle = angle_to_radians(w.marker[i].angle);
				switch(w.marker[i].phase)
				{
				 case MARKER_PHASE_APPEAR:
					 marker_shade = CLOUD_SHADES - (w.marker[i].phase_counter * 2);
					 if (marker_shade < 0)
							 marker_shade = 0;
					 marker_dist1 = 16 + (w.marker[i].size * 2) + (w.marker[i].phase_counter * 8);
					 marker_dist2 = 12 - (w.marker[i].phase_counter * 2);
					 break;
				 case MARKER_PHASE_EXISTS:
					 marker_shade = CLOUD_SHADES - 1;//CLOUD_SHADES-1;
					 marker_dist1 = 16 + (w.marker[i].size * 2);
					 marker_dist2 = 12;
					 break;
				 case MARKER_PHASE_FADE:
					 marker_shade = w.marker[i].phase_counter * 2;
					 if (marker_shade >= CLOUD_SHADES)
							 marker_shade = CLOUD_SHADES;
					 marker_dist1 = 16 + (w.marker[i].size * 2) + (MARKER_PHASE_TIME - w.marker[i].phase_counter) * 8; // 96 - (w.marker[i].phase_counter * 6);
					 marker_dist2 = 4 + (w.marker[i].phase_counter) * 2;
					 break;
				}
				for (j = 0; j < submarkers; j++)
				{
					x2 = x + cos(marker_angle) * marker_dist1;
					y2 = y + sin(marker_angle) * marker_dist1;
					add_diamond_layer(2,
																								x2 - cos(marker_angle) * (marker_dist2 * 2),
																								y2 - sin(marker_angle) * (marker_dist2 * 2),
																								x2 + cos(marker_angle - (PI/6)) * marker_dist2,
																								y2 + sin(marker_angle - (PI/6)) * marker_dist2,
																								x2 + cos(marker_angle) * (marker_dist2 * 1.5),
																								y2 + sin(marker_angle) * (marker_dist2 * 1.5),
																								x2 + cos(marker_angle + (PI/6)) * marker_dist2,
																								y2 + sin(marker_angle + (PI/6)) * marker_dist2,
																								colours.base_fade [w.marker[i].colour] [marker_shade]);

/*
					add_diamond_layer(2,
																								x + cos(marker_angle) * (marker_dist1 / 3),
																								y + sin(marker_angle) * (marker_dist1 / 3),
																								x2 + cos(marker_angle - (PI/6)) * marker_dist2,
																								y2 + sin(marker_angle - (PI/6)) * marker_dist2,
																								x2 + cos(marker_angle) * (marker_dist2 * 1.5),
																								y2 + sin(marker_angle) * (marker_dist2 * 1.5),
																								x2 + cos(marker_angle + (PI/6)) * marker_dist2,
																								y2 + sin(marker_angle + (PI/6)) * marker_dist2,
																								colours.base_fade [w.marker[i].colour] [marker_shade]);
*/
 				marker_angle += (PI*2) / ((float) submarkers);
				}
			 break;
  }
/*
  add_line(x - 20, y - 20, x - 30, y - 30, colours.base [COL_YELLOW] [SHADE_HIGH]);
  add_line(x + 20, y - 20, x + 30, y - 30, colours.base [COL_YELLOW] [SHADE_HIGH]);
  add_line(x - 20, y + 20, x - 30, y + 30, colours.base [COL_YELLOW] [SHADE_HIGH]);
  add_line(x + 20, y + 20, x + 30, y + 30, colours.base [COL_YELLOW] [SHADE_HIGH]);*/

 }


//  add_outline_shape(view.mouse_x, view.mouse_y, 0, &shape_dat [0] [2],
//                    w.team[0].colour [TCOL_LINE1], w.team[0].colour [TCOL_LINE2], w.team[0].colour [TCOL_LINE3], w.team[0].colour [TCOL_FILL1]);

 draw_from_buffers(); // sends poly_buffer and line_buffer to the screen - do it here to make sure any selection graphics are drawn before the map
 draw_fans();
 draw_map();

//  al_draw_circle(view.window_x / 2, view.window_y / 2, 2, base_col [COL_GREY] [SHADE_MIN], 1);


  int seconds = w.world_time * 0.03;
//  int fps_y = 10; //view.window_y - 50;

  ALLEGRO_COLOR text_col = colours.base [COL_GREY] [SHADE_MAX];
/*
  al_draw_textf(font, text_col, 5, fps_y, 0, "fps %i", view.fps);
  fps_y += 10;
  al_draw_textf(font, text_col, 5, fps_y, 0, "cps %i", view.cycles_per_second);
  fps_y += 10;
  if (seconds > 3599)
   al_draw_textf(font, text_col, 5, fps_y, 0, "time %i:%.2i:%.2i", seconds / 3600, (int) (seconds / 60) % 60, seconds % 60);
    else
     al_draw_textf(font, text_col, 5, fps_y, 0, "time %i:%.2i", (int) (seconds / 60) % 60, seconds % 60);
  fps_y += 10;
  al_draw_textf(font, text_col, 5, fps_y, 0, "ticks %i ", w.world_time);
*/
// now display game status along top line:
 int base_status_x = (view.window_x - 120);
 if (settings.edit_window != EDIT_WINDOW_CLOSED)
  base_status_x = view.window_x - 5;
#define STATUS_X base_status_x

 int sx = STATUS_X;
 int sy = 5;


/* for (i = w.players - 1; i >= 0; i --)
 {
  al_draw_textf(font, text_col, sx, sy, ALLEGRO_ALIGN_RIGHT, "g %i(%i)", w.player[i].gen_number, w.gen_limit);
  sx -= 65;
  al_draw_textf(font, text_col, sx, sy, ALLEGRO_ALIGN_RIGHT, "p %i(%i)", w.player[i].processes, w.procs_per_team);
  sx -= 65;
  al_draw_textf(font, text_col, sx, sy, ALLEGRO_ALIGN_RIGHT, "%s", w.player[i].name);
  sx -= 90;
  if (i == 1)
  {
   sy += 12;
   sx = STATUS_X;
  }
 }
*/


// the strange order of the player displays is so that it draws like this:
// Player 1 Player 2
// Player 3 Player 4

  if (w.players >= 2)
  {
   i = 1;
//   al_draw_textf(font, text_col, sx, sy, ALLEGRO_ALIGN_RIGHT, "g %i(%i)", w.player[i].gen_number, w.gen_limit);
//   sx -= 65;
   al_draw_textf(font[FONT_SQUARE].fnt, text_col, sx, sy, ALLEGRO_ALIGN_RIGHT, "p %i(%i)", w.player[i].processes, w.procs_per_team);
   sx -= 80;
   al_draw_textf(font[FONT_SQUARE].fnt, text_col, sx, sy, ALLEGRO_ALIGN_RIGHT, "%s", w.player[i].name);
   sx -= 90;
  }

  i = 0;
// Player 1
//  al_draw_textf(font, text_col, sx, sy, ALLEGRO_ALIGN_RIGHT, "g %i(%i)", w.player[i].gen_number, w.gen_limit);
//  sx -= 65;
  al_draw_textf(font[FONT_SQUARE].fnt, text_col, sx, sy, ALLEGRO_ALIGN_RIGHT, "p %i(%i)", w.player[i].processes, w.procs_per_team);
  sx -= 80;
  al_draw_textf(font[FONT_SQUARE].fnt, text_col, sx, sy, ALLEGRO_ALIGN_RIGHT, "%s", w.player[i].name);
  sx -= 90;

  if (w.players >= 3)
  {
   sx = STATUS_X;
   sy += 12;

   if (w.players == 4)
   {
    i = 3;
//    al_draw_textf(font, text_col, sx, sy, ALLEGRO_ALIGN_RIGHT, "g %i(%i)", w.player[i].gen_number, w.gen_limit);
//    sx -= 65;
    al_draw_textf(font[FONT_SQUARE].fnt, text_col, sx, sy, ALLEGRO_ALIGN_RIGHT, "p %i(%i)", w.player[i].processes, w.procs_per_team);
    sx -= 80;
    al_draw_textf(font[FONT_SQUARE].fnt, text_col, sx, sy, ALLEGRO_ALIGN_RIGHT, "%s", w.player[i].name);
    sx -= 90;
   }

   i = 2;
//   al_draw_textf(font, text_col, sx, sy, ALLEGRO_ALIGN_RIGHT, "g %i(%i)", w.player[i].gen_number, w.gen_limit);
//   sx -= 65;
   al_draw_textf(font[FONT_SQUARE].fnt, text_col, sx, sy, ALLEGRO_ALIGN_RIGHT, "p %i(%i)", w.player[i].processes, w.procs_per_team);
   sx -= 80;
   al_draw_textf(font[FONT_SQUARE].fnt, text_col, sx, sy, ALLEGRO_ALIGN_RIGHT, "%s", w.player[i].name);
   sx -= 90;
  }



 sx = STATUS_X;
 sy += 12;

 if (seconds > 3599)
  al_draw_textf(font[FONT_SQUARE].fnt, text_col, sx, sy, ALLEGRO_ALIGN_RIGHT, "time %i:%.2i:%.2i", seconds / 3600, (int) (seconds / 60) % 60, seconds % 60);
   else
    al_draw_textf(font[FONT_SQUARE].fnt, text_col, sx, sy, ALLEGRO_ALIGN_RIGHT, "time %i:%.2i", (int) (seconds / 60) % 60, seconds % 60);
 sx -= 90;
 if (game.minutes_each_turn != 0)
 {
  seconds = game.current_turn_time_left * 0.03;
  if (seconds > 3599)
   al_draw_textf(font[FONT_SQUARE].fnt, text_col, sx, sy, ALLEGRO_ALIGN_RIGHT, "turn time %i:%.2i:%.2i", seconds / 3600, (int) (seconds / 60) % 60, seconds % 60);
    else
     al_draw_textf(font[FONT_SQUARE].fnt, text_col, sx, sy, ALLEGRO_ALIGN_RIGHT, "turn time %i:%.2i", (int) (seconds / 60) % 60, seconds % 60);
  sx -= 120;
 }

 if (game.turns == 0)
 {
  al_draw_textf(font[FONT_SQUARE].fnt, text_col, sx, sy, ALLEGRO_ALIGN_RIGHT, "turn %i", game.current_turn);
  sx -= 40;
 }
  else
  {
   al_draw_textf(font[FONT_SQUARE].fnt, text_col, sx, sy, ALLEGRO_ALIGN_RIGHT, "turn %i/%i", game.current_turn, game.turns);
   sx -= 50;
  }

 if (view.focus_proc != NULL)
  draw_proc_box();

 display_consoles();

 switch(settings.edit_window)
 {
  case EDIT_WINDOW_EDITOR:
   draw_edit_bmp();
   break;
  case EDIT_WINDOW_TEMPLATES:
   draw_template_bmp();
   break;
  case EDIT_WINDOW_SYSMENU:
   display_sysmenu();
   break;
  case EDIT_WINDOW_PROGRAMS:
   display_programs_menu();
   break;
 }


// The above functions probably reset the target bitmap.
// Now we need to draw some stuff that doesn't need to be clipped to display_window:
// al_set_target_bitmap(al_get_backbuffer(display));

 draw_mode_buttons();

 if (settings.edit_window == EDIT_WINDOW_CLOSED)
		al_set_clipping_rectangle(0, 0, view.window_x, view.window_y);
	  else
		  al_set_clipping_rectangle(0, 0, settings.editor_x_split, view.window_y);


#define LINE_1_Y 100
#define LINE_2_Y 125
#define LINE_3_Y 150

 switch(game.phase)
 {
  case GAME_PHASE_TURN:
   if (game.current_turn == 0)
   {
    al_draw_textf(font[FONT_SQUARE_LARGE].fnt, colours.base [COL_GREY] [SHADE_MAX], view.window_x / 2, view.window_y / 2 + LINE_1_Y, ALLEGRO_ALIGN_CENTRE, "READY");
//    al_draw_textf(font, colours.base [COL_GREY] [SHADE_MAX], view.window_x / 2, view.window_y / 2 + LINE_2_Y, ALLEGRO_ALIGN_CENTRE, "Use system menu to execute");
   }
    else
    {
     al_draw_textf(font[FONT_SQUARE_LARGE].fnt, colours.base [COL_GREY] [SHADE_MAX], view.window_x / 2, view.window_y / 2 + LINE_1_Y, ALLEGRO_ALIGN_CENTRE, "TURN %i COMPLETE", game.current_turn);
//     al_draw_textf(font, colours.base [COL_GREY] [SHADE_MAX], view.window_x / 2, view.window_y / 2 + LINE_2_Y, ALLEGRO_ALIGN_CENTRE, "Use system menu to start next turn");
    }
   break;
  case GAME_PHASE_PREGAME: // this will probably never actually be displayed as the display doesn't run during pregame.
//   al_draw_textf(font, base_col [COL_GREY] [SHADE_MAX], view.window_x / 2, view.window_y / 2 + LINE_1_Y, ALLEGRO_ALIGN_CENTRE, "READY");
//   al_draw_textf(font, base_col [COL_GREY] [SHADE_MAX], view.window_x / 2, view.window_y / 2 + LINE_2_Y, ALLEGRO_ALIGN_CENTRE, "Use system menu to start");
   break;
  case GAME_PHASE_WORLD:
   if (game.pause_soft)
    al_draw_textf(font[FONT_SQUARE_LARGE].fnt, colours.base [COL_GREY] [SHADE_MAX], view.window_x / 2, view.window_y / 2 + LINE_1_Y, ALLEGRO_ALIGN_CENTRE, "PAUSED");
   if (game.fast_forward > 0)
   {
   	switch(game.fast_forward_type)
   	{
   		case FAST_FORWARD_TYPE_SMOOTH:
      al_draw_textf(font[FONT_SQUARE_LARGE].fnt, colours.base [COL_GREY] [SHADE_MAX], view.window_x / 2, view.window_y / 2 + LINE_2_Y, ALLEGRO_ALIGN_CENTRE, "FAST FORWARD"); break;
   		case FAST_FORWARD_TYPE_SKIPPY:
      al_draw_textf(font[FONT_SQUARE_LARGE].fnt, colours.base [COL_GREY] [SHADE_MAX], view.window_x / 2, view.window_y / 2 + LINE_2_Y, ALLEGRO_ALIGN_CENTRE, "FAST FORWARD (SKIP)"); break;
   		case FAST_FORWARD_TYPE_4X:
      al_draw_textf(font[FONT_SQUARE_LARGE].fnt, colours.base [COL_GREY] [SHADE_MAX], view.window_x / 2, view.window_y / 2 + LINE_2_Y, ALLEGRO_ALIGN_CENTRE, "FAST FORWARD (x4)"); break;
   		case FAST_FORWARD_TYPE_8X:
      al_draw_textf(font[FONT_SQUARE_LARGE].fnt, colours.base [COL_GREY] [SHADE_MAX], view.window_x / 2, view.window_y / 2 + LINE_2_Y, ALLEGRO_ALIGN_CENTRE, "FAST FORWARD (x8)"); break;
   	}
   }
   break;
  case GAME_PHASE_OVER:
// note that HALTED sign may be being displayed (at centre_y + LINE_3_Y)
   al_draw_textf(font[FONT_SQUARE_LARGE].fnt, colours.base [COL_GREY] [SHADE_MAX], view.window_x / 2, view.window_y / 2 + LINE_1_Y, ALLEGRO_ALIGN_CENTRE, "GAME OVER");
   switch(game.game_over_status)
   {
    case GAME_END_BASIC: // nothing
     break;
    case GAME_END_YOU_WON:
     al_draw_textf(font[FONT_SQUARE_LARGE].fnt, colours.base [COL_GREY] [SHADE_MAX], view.window_x / 2, view.window_y / 2 + LINE_2_Y, ALLEGRO_ALIGN_CENTRE, "Status:  VICTORY!");
     break;
    case GAME_END_YOU_LOST:
     al_draw_textf(font[FONT_SQUARE_LARGE].fnt, colours.base [COL_GREY] [SHADE_MAX], view.window_x / 2, view.window_y / 2 + LINE_2_Y, ALLEGRO_ALIGN_CENTRE, "Status:  FAILURE :(");
     break;
    case GAME_END_PLAYER_WON:
     if (game.game_over_value >= 0 && game.game_over_value < w.players) // shouldn't need to check this but do it anyway
      al_draw_textf(font[FONT_SQUARE_LARGE].fnt, colours.base [COL_GREY] [SHADE_MAX], view.window_x / 2, view.window_y / 2 + LINE_2_Y, ALLEGRO_ALIGN_CENTRE, "Status:  %s WINS!", w.player[game.game_over_value].name);
     break;
    case GAME_END_PLAYER_LOST:
     if (game.game_over_value >= 0 && game.game_over_value < w.players) // shouldn't need to check this but do it anyway
      al_draw_textf(font[FONT_SQUARE_LARGE].fnt, colours.base [COL_GREY] [SHADE_MAX], view.window_x / 2, view.window_y / 2 + LINE_2_Y, ALLEGRO_ALIGN_CENTRE, "Status:  %s LOSES!", w.player[game.game_over_value].name);
     break;
    case GAME_END_DRAW:
     al_draw_textf(font[FONT_SQUARE_LARGE].fnt, colours.base [COL_GREY] [SHADE_MAX], view.window_x / 2, view.window_y / 2 + LINE_2_Y, ALLEGRO_ALIGN_CENTRE, "Status:  DRAW");
     break;
    case GAME_END_TIME:
     al_draw_textf(font[FONT_SQUARE_LARGE].fnt, colours.base [COL_GREY] [SHADE_MAX], view.window_x / 2, view.window_y / 2 + LINE_2_Y, ALLEGRO_ALIGN_CENTRE, "Status:  TIME EXPIRED");
     break;
    case GAME_END_ERROR_STATUS:
     al_draw_textf(font[FONT_SQUARE_LARGE].fnt, colours.base [COL_GREY] [SHADE_MAX], view.window_x / 2, view.window_y / 2 + LINE_2_Y, ALLEGRO_ALIGN_CENTRE, "[Invalid game end state: %i]", game.game_over_status);
     break;
    case GAME_END_ERROR_PLAYER:
     al_draw_textf(font[FONT_SQUARE_LARGE].fnt, colours.base [COL_GREY] [SHADE_MAX], view.window_x / 2, view.window_y / 2 + LINE_2_Y, ALLEGRO_ALIGN_CENTRE, "[Invalid player index: %i]", game.game_over_value);
     break;
   }
   break;
 }

 if (game.pause_hard) // can still enter hard pause when in a non-world game phase (as the system/observer/operator will otherwise continue running)
 {
   al_draw_textf(font[FONT_SQUARE_LARGE].fnt, colours.base [COL_GREY] [SHADE_MAX], view.window_x / 2, view.window_y / 2 + LINE_3_Y, ALLEGRO_ALIGN_CENTRE, "HALTED");
 }
/*
//   else
   {
     al_draw_filled_rectangle(game.editor_mode_button_x, game.editor_mode_button_y,
      game.editor_mode_button_x + EDITOR_MODE_BUTTON_SIZE, game.editor_mode_button_y + EDITOR_MODE_BUTTON_SIZE, base_col [COL_GREEN] [SHADE_LOW]);
     al_draw_rectangle(game.editor_mode_button_x, game.editor_mode_button_y,
      game.editor_mode_button_x + EDITOR_MODE_BUTTON_SIZE, game.editor_mode_button_y + EDITOR_MODE_BUTTON_SIZE, base_col [COL_GREEN] [SHADE_MAX], 1);
   }*/

 if (settings.option [OPTION_SPECIAL_CURSOR])
  draw_mouse_cursor();
 al_flip_display();
 al_set_target_bitmap(al_get_backbuffer(display));
// al_clear_to_color(base_col [COL_BLUE] [SHADE_LOW]);
// al_clear_to_color(colours.world_background);//colours.base [COL_GREY] [SHADE_MIN]);


}

// This function produces pseudo-random numbers from proc properties.
// It's only used in display functions where it can't affect gameplay, so its extremely low quality doesn't matter
// Also, it's not saved in saved games.
// Assumes mod is not zero
static unsigned int proc_rand(struct procstruct* pr, int special, int mod)
{

// The mixture of int and al_fixed shouldn't matter.
 return (pr->x + pr->y + special + pr->execution_count + *(pr->irpt)) % mod;

}

static unsigned int packet_rand(struct packetstruct* pack, int mod)
{
 return (pack->x + pack->y) % mod;
}

void draw_mode_buttons(void)
{

	al_set_clipping_rectangle(0, 0, settings.option [OPTION_WINDOW_W], settings.option [OPTION_WINDOW_H]);

 int button_fill_shade;
// int button_edge_shade;
// int button_edge_col;
 int mbutton_mode;
 int i;

 reset_i_buttons();

 for (i = 0; i < MODE_BUTTONS; i ++)
 {
  if (settings.mode_button_available [i] == 0)
   continue;
//  button_edge_shade = SHADE_MAX;
  button_fill_shade = SHADE_LOW;
//  button_edge_col = COL_BLUE;
  mbutton_mode = MBUTTON_TYPE_MODE;

  if (settings.mode_button_highlight [i] == 1)
  {
//   button_edge_shade = SHADE_MAX;
//   button_edge_col = COL_GREY; // see also highlight rectangle below
   mbutton_mode = MBUTTON_TYPE_MODE_HIGHLIGHT;
   switch(i)
   {
    case MODE_BUTTON_PROGRAMS:
     al_draw_textf(font[FONT_SQUARE].fnt, colours.base [COL_GREY] [SHADE_MAX], settings.mode_button_x [i] + (MODE_BUTTON_SIZE / 2), settings.mode_button_y [i] + 24, ALLEGRO_ALIGN_CENTRE, "Program details"); break;
    case MODE_BUTTON_TEMPLATES:
     al_draw_textf(font[FONT_SQUARE].fnt, colours.base [COL_GREY] [SHADE_MAX], settings.mode_button_x [i] + (MODE_BUTTON_SIZE / 2), settings.mode_button_y [i] + 24, ALLEGRO_ALIGN_CENTRE, "Templates"); break;
    case MODE_BUTTON_EDITOR:
     al_draw_textf(font[FONT_SQUARE].fnt, colours.base [COL_GREY] [SHADE_MAX], settings.mode_button_x [i] + (MODE_BUTTON_SIZE / 2), settings.mode_button_y [i] + 24, ALLEGRO_ALIGN_CENTRE, "code Editor"); break;
    case MODE_BUTTON_SYSMENU:
     al_draw_textf(font[FONT_SQUARE].fnt, colours.base [COL_GREY] [SHADE_MAX], settings.mode_button_x [i] + (MODE_BUTTON_SIZE / 2) - 5, settings.mode_button_y [i] + 24, ALLEGRO_ALIGN_CENTRE, "System menu"); break;
    case MODE_BUTTON_CLOSE:
     al_draw_textf(font[FONT_SQUARE].fnt, colours.base [COL_GREY] [SHADE_MAX], settings.mode_button_x [i] + (MODE_BUTTON_SIZE / 2) - 25, settings.mode_button_y [i] + 24, ALLEGRO_ALIGN_CENTRE, "Close panel"); break;
   }
   add_menu_button(settings.mode_button_x [i] - 3, settings.mode_button_y [i] - 1,
    settings.mode_button_x [i] + MODE_BUTTON_SIZE + 3, settings.mode_button_y [i] + MODE_BUTTON_SIZE + 1, colours.base [COL_BLUE] [SHADE_MAX],
    mbutton_mode);
  }
  if (settings.edit_window == i)
  {
   button_fill_shade = SHADE_HIGH;
  }

  add_menu_button(settings.mode_button_x [i] - 2, settings.mode_button_y [i],
   settings.mode_button_x [i] + MODE_BUTTON_SIZE + 2, settings.mode_button_y [i] + MODE_BUTTON_SIZE, colours.base [COL_BLUE] [button_fill_shade],
   mbutton_mode);
//  al_draw_filled_rectangle(settings.mode_button_x [i], settings.mode_button_y [i],
//   settings.mode_button_x [i] + MODE_BUTTON_SIZE, settings.mode_button_y [i] + MODE_BUTTON_SIZE, colours.base [COL_BLUE] [button_fill_shade]);
/*  if (settings.mode_button_highlight [i])
		{
   al_draw_rectangle(settings.mode_button_x [i] - 0.5, settings.mode_button_y [i] - 0.5,
    settings.mode_button_x [i] + MODE_BUTTON_SIZE + 0.5, settings.mode_button_y [i] + MODE_BUTTON_SIZE + 0.5, colours.base [button_edge_col] [button_edge_shade], 1);
		}*/


  switch(i)
  {
   case MODE_BUTTON_PROGRAMS:
   	add_menu_string(settings.mode_button_x [i] + (MODE_BUTTON_SIZE / 2), settings.mode_button_y [i] + 4,
																				&colours.base [COL_GREY] [SHADE_MAX], ALLEGRO_ALIGN_CENTRE, FONT_SQUARE, "Pr"); break;
//    al_draw_textf(font[FONT_SQUARE].fnt, colours.base [COL_GREY] [SHADE_MAX], settings.mode_button_x [i] + (MODE_BUTTON_SIZE / 2), settings.mode_button_y [i] + 4, ALLEGRO_ALIGN_CENTRE, "Pr"); break;
   case MODE_BUTTON_TEMPLATES:
       	add_menu_string(settings.mode_button_x [i] + (MODE_BUTTON_SIZE / 2), settings.mode_button_y [i] + 4,
																				&colours.base [COL_GREY] [SHADE_MAX], ALLEGRO_ALIGN_CENTRE, FONT_SQUARE, "Te"); break;
//    al_draw_textf(font[FONT_SQUARE].fnt, colours.base [COL_GREY] [SHADE_MAX], settings.mode_button_x [i] + (MODE_BUTTON_SIZE / 2), settings.mode_button_y [i] + 4, ALLEGRO_ALIGN_CENTRE, "Te"); break;
   case MODE_BUTTON_EDITOR:
       	add_menu_string(settings.mode_button_x [i] + (MODE_BUTTON_SIZE / 2), settings.mode_button_y [i] + 4,
																				&colours.base [COL_GREY] [SHADE_MAX], ALLEGRO_ALIGN_CENTRE, FONT_SQUARE, "Ed"); break;
//    al_draw_textf(font[FONT_SQUARE].fnt, colours.base [COL_GREY] [SHADE_MAX], settings.mode_button_x [i] + (MODE_BUTTON_SIZE / 2), settings.mode_button_y [i] + 4, ALLEGRO_ALIGN_CENTRE, "Ed"); break;
   case MODE_BUTTON_SYSMENU:
       	add_menu_string(settings.mode_button_x [i] + (MODE_BUTTON_SIZE / 2), settings.mode_button_y [i] + 4,
																				&colours.base [COL_GREY] [SHADE_MAX], ALLEGRO_ALIGN_CENTRE, FONT_SQUARE, "Sy"); break;
//    al_draw_textf(font[FONT_SQUARE].fnt, colours.base [COL_GREY] [SHADE_MAX], settings.mode_button_x [i] + (MODE_BUTTON_SIZE / 2), settings.mode_button_y [i] + 4, ALLEGRO_ALIGN_CENTRE, "Sy"); break;
   case MODE_BUTTON_CLOSE:
       	add_menu_string(settings.mode_button_x [i] + (MODE_BUTTON_SIZE / 2), settings.mode_button_y [i] + 4,
																				&colours.base [COL_GREY] [SHADE_MAX], ALLEGRO_ALIGN_CENTRE, FONT_SQUARE, "X"); break;
//    al_draw_textf(font[FONT_SQUARE].fnt, colours.base [COL_GREY] [SHADE_MAX], settings.mode_button_x [i] + (MODE_BUTTON_SIZE / 2), settings.mode_button_y [i] + 4, ALLEGRO_ALIGN_CENTRE, "X"); break;

  }

 }

 draw_menu_buttons();

}


void draw_stream_beam(float x1, float by1, float x2, float y2, int col, int status, int counter, int hit)
{

 float outer_line_thickness = 0;
 float inner_line_thickness;
 float flash_size = 0;
 int outer_line_shade = 0;
// float angle_width;
 float draw_angle = atan2(y2 - by1, x2 - x1);
 int i;
 float outer_end_point [2] [2];
 float inner_end_point [2] [2];


 switch(status)
 {
  case STREAM_STATUS_WARMUP: // warmup
   outer_line_thickness = ((((float) STREAM_WARMUP_LENGTH - counter) * 4) / STREAM_WARMUP_LENGTH) + 1;
   outer_line_shade = outer_line_thickness;//line_thickness;
   flash_size = outer_line_thickness * 3;
   break;
  case STREAM_STATUS_FIRING: // firing
   outer_line_thickness = (float) 4 + (float) counter / 2;// + grand(3);
   outer_line_shade = 4;
   flash_size = outer_line_thickness * 3;
   break;
  case STREAM_STATUS_COOLDOWN: // cooldown
   outer_line_thickness = (((float) counter * 4) / STREAM_COOLDOWN_LENGTH) + 1;
//   line_thickness = counter;
   outer_line_shade = outer_line_thickness; //line_thickness;
   flash_size = outer_line_thickness * 3;
   break;

  default:
   fprintf(stdout, "\ni_display.c: draw_stream_beam() with invalid status (status %i counter %i)", status, counter);
   error_call();
   return;
 }

/* outer_line_thickness /= 2;

	if (outer_line_thickness < 1)
		 outer_line_thickness = 1;*/

// adjust the end point a bit so it isn't too close or too far:
    float beam_length = hypot(y2 - by1, x2 - x1);
    if (beam_length < 16)
    {
     x2 = x1 + cos(draw_angle) * 16;
     y2 = by1 + sin(draw_angle) * 16;
    }
     else
     {
      if (hit == 1)
      {
       x2 = x1 + cos(draw_angle) * (beam_length - 10);
       y2 = by1 + sin(draw_angle) * (beam_length - 10);
      }
     }

//    angle_width = PI / 8;

    if (!start_fan(x1, by1, colours.packet [col] [outer_line_shade]))
     return; // too many fans (TO DO: this should be a high priority and remove other fans if the array is full)

    add_fan_vertex(x1 + cos(draw_angle - PI_4) * flash_size,
                   by1 + sin(draw_angle - PI_4) * flash_size,
                   colours.packet [col] [outer_line_shade]);

    add_fan_vertex(x1 + (cos(draw_angle) * flash_size * 2) + cos(draw_angle - PI/2) * outer_line_thickness,
                   by1 + (sin(draw_angle) * flash_size * 2) + sin(draw_angle - PI/2) * outer_line_thickness,
                   colours.packet [col] [outer_line_shade]);

    outer_end_point [0] [0] = x2 + cos(draw_angle - PI/2) * outer_line_thickness;
    outer_end_point [0] [1] = y2 + sin(draw_angle - PI/2) * outer_line_thickness;
    outer_end_point [1] [0] = x2 + cos(draw_angle + PI/2) * outer_line_thickness;
    outer_end_point [1] [1] = y2 + sin(draw_angle + PI/2) * outer_line_thickness;

    add_fan_vertex(outer_end_point [0] [0],
                   outer_end_point [0] [1],
                   colours.packet [col] [outer_line_shade]);

    add_fan_vertex(outer_end_point [1] [0],
                   outer_end_point [1] [1],
                   colours.packet [col] [outer_line_shade]);

    add_fan_vertex(x1 + (cos(draw_angle) * flash_size * 2) + cos(draw_angle + PI/2) * outer_line_thickness,
                   by1 + (sin(draw_angle) * flash_size * 2) + sin(draw_angle + PI/2) * outer_line_thickness,
                   colours.packet [col] [outer_line_shade]);

    add_fan_vertex(x1 + cos(draw_angle + PI_4) * flash_size,
                   by1 + sin(draw_angle + PI_4) * flash_size,
                   colours.packet [col] [outer_line_shade]);


    finish_fan_open();

    float end_flash_size = outer_line_thickness * 1.6;
    int end_flash_rand = 0;
    if (hit == 1)
     end_flash_size = outer_line_thickness * 1.9;

    float efx2 = x2 + cos(draw_angle) * end_flash_size;
    float efy2 = y2 + sin(draw_angle) * end_flash_size;


    if (!start_fan(efx2, efy2, colours.packet [col] [outer_line_shade]))
     return; // too many fans (TO DO: this should be a high priority and remove other fans if the array is full)

    add_fan_vertex(outer_end_point [1] [0],
                   outer_end_point [1] [1],
                   colours.packet [col] [outer_line_shade]);

    add_fan_vertex(outer_end_point [0] [0],
                   outer_end_point [0] [1],
                   colours.packet [col] [outer_line_shade]);

    for (i = 1; i < 8; i ++) // note 1 to 7
    {
//     end_flash_rand = grand(3);
     add_fan_vertex(efx2 - cos(draw_angle + (PI / 4) * i) * (end_flash_size + end_flash_rand),
                    efy2 - sin(draw_angle + (PI / 4) * i) * (end_flash_size + end_flash_rand),
                    colours.packet [col] [outer_line_shade]);
    }

    finish_fan();




    if (status == 2)
    {

     inner_line_thickness = (float) 1 + (float) counter / 3; // / 3
//     inner_line_thickness /= 2;
     flash_size = inner_line_thickness * 3;
//     line_shade = 9;

     x1 += cos(draw_angle) * 8;
     by1 += sin(draw_angle) * 8;

     if (!start_fan(x1, by1, colours.packet [col] [9]))
      return; // too many fans (TO DO: this should be a high priority and remove other fans if the array is full)

     add_fan_vertex(x1 + cos(draw_angle - PI_4) * flash_size,
                   by1 + sin(draw_angle - PI_4) * flash_size,
                   colours.packet [col] [9]);

     add_fan_vertex(x1 + (cos(draw_angle) * flash_size * 2) + cos(draw_angle - PI/2) * inner_line_thickness,
                   by1 + (sin(draw_angle) * flash_size * 2) + sin(draw_angle - PI/2) * inner_line_thickness,
                   colours.packet [col] [9]);

    inner_end_point [0] [0] = x2 + cos(draw_angle - PI/2) * inner_line_thickness;
    inner_end_point [0] [1] = y2 + sin(draw_angle - PI/2) * inner_line_thickness;
    inner_end_point [1] [0] = x2 + cos(draw_angle + PI/2) * inner_line_thickness;
    inner_end_point [1] [1] = y2 + sin(draw_angle + PI/2) * inner_line_thickness;

     add_fan_vertex(inner_end_point [0] [0],
                    inner_end_point [0] [1],
                    colours.packet [col] [9]);

     add_fan_vertex(inner_end_point [1] [0],
                    inner_end_point [1] [1],
                    colours.packet [col] [9]);

     add_fan_vertex(x1 + (cos(draw_angle) * flash_size * 2) + cos(draw_angle + PI/2) * inner_line_thickness,
                   by1 + (sin(draw_angle) * flash_size * 2) + sin(draw_angle + PI/2) * inner_line_thickness,
                   colours.packet [col] [9]);

     add_fan_vertex(x1 + cos(draw_angle + PI_4) * flash_size,
                   by1 + sin(draw_angle + PI_4) * flash_size,
                   colours.packet [col] [9]);

     finish_fan_open();

// end flash:

// use old values of end_flash_size
     efx2 = x2 + cos(draw_angle) * end_flash_size;
     efy2 = y2 + sin(draw_angle) * end_flash_size;

     end_flash_size = inner_line_thickness * 1.6;
     if (hit == 1)
     end_flash_size = inner_line_thickness * 1.9;


     if (!start_fan(efx2, efy2, colours.packet [col] [9]))
      return; // too many fans (TO DO: this should be a high priority and remove other fans if the array is full)

     add_fan_vertex(inner_end_point [1] [0],
                    inner_end_point [1] [1],
                    colours.packet [col] [9]);

     add_fan_vertex(inner_end_point [0] [0],
                    inner_end_point [0] [1],
                    colours.packet [col] [9]);

     for (i = 1; i < 8; i ++) // note 1 to 7
     {
//      end_flash_rand = grand(3);
      add_fan_vertex(efx2 - cos(draw_angle + (PI / 4) * i) * (end_flash_size + end_flash_rand),
                     efy2 - sin(draw_angle + (PI / 4) * i) * (end_flash_size + end_flash_rand),
                     colours.packet [col] [9]);
     }

     finish_fan();



    }


/*

 draw_thickline(x1, by1, x2, y2, line_thickness, colours.packet [col] [line_shade]);

 if (status != 2)
  return;

 line_thickness = 1 + grand(2);
 line_shade = 9;

 draw_thickline(x1, by1, x2, y2, line_thickness, colours.packet [col] [line_shade]);
*/

}

void draw_thickline(float x1, float by1, float x2, float y2, float thickness, ALLEGRO_COLOR col)
{
 float draw_angle = atan2(y2 - by1, x2 - x1);
 float box_coordinates [4] [2];
 box_coordinates [0] [0] = x1 + cos(draw_angle + PI/2) * thickness;
 box_coordinates [0] [1] = by1 + sin(draw_angle + PI/2) * thickness;
 box_coordinates [1] [0] = x1 + cos(draw_angle - PI/2) * thickness;
 box_coordinates [1] [1] = by1 + sin(draw_angle - PI/2) * thickness;
 box_coordinates [2] [0] = x2 + cos(draw_angle + PI/2) * thickness;
 box_coordinates [2] [1] = y2 + sin(draw_angle + PI/2) * thickness;
 box_coordinates [3] [0] = x2 + cos(draw_angle - PI/2) * thickness;
 box_coordinates [3] [1] = y2 + sin(draw_angle - PI/2) * thickness;

 add_triangle_layer(1,
                    box_coordinates [0] [0],
                    box_coordinates [0] [1],
                    box_coordinates [1] [0],
                    box_coordinates [1] [1],
                    box_coordinates [2] [0],
                    box_coordinates [2] [1],
                    col);

 add_triangle_layer(1,
                    box_coordinates [2] [0],
                    box_coordinates [2] [1],
                    box_coordinates [3] [0],
                    box_coordinates [3] [1],
                    box_coordinates [1] [0],
                    box_coordinates [1] [1],
                    col);

}





void draw_allocate_beam(float x1, float by1, float x2, float y2, int col, int counter)
{

 float outer_line_thickness = 0;
 float flash_size = 0;
 int outer_line_shade = 0;
// float angle_width = 0;
 float draw_angle = atan2(y2 - by1, x2 - x1);
 int i;
 float outer_end_point [2] [2];

   outer_line_thickness = ((float) counter) + 1;
   outer_line_shade = counter * 2; //outer_line_thickness; //line_thickness;
   if (outer_line_shade >= CLOUD_SHADES)
    outer_line_shade = CLOUD_SHADES - 1;
   flash_size = outer_line_thickness * 3;

// adjust the end point a bit so it isn't too close or too far:
    float beam_length = hypot(y2 - by1, x2 - x1);
    if (beam_length < 16)
    {
     x2 = x1 + cos(draw_angle) * 16;
     y2 = by1 + sin(draw_angle) * 16;
    }

//    angle_width = PI / 8;

    if (!start_fan(x1, by1, colours.packet [col] [outer_line_shade]))
     return; // too many fans

    add_fan_vertex(x1 + cos(draw_angle - PI_4) * flash_size,
                   by1 + sin(draw_angle - PI_4) * flash_size,
                   colours.packet [col] [outer_line_shade]);

    add_fan_vertex(x1 + (cos(draw_angle) * flash_size * 2) + cos(draw_angle - PI/2) * outer_line_thickness,
                   by1 + (sin(draw_angle) * flash_size * 2) + sin(draw_angle - PI/2) * outer_line_thickness,
                   colours.packet [col] [outer_line_shade]);

    outer_end_point [0] [0] = x2 + cos(draw_angle - PI/2) * outer_line_thickness;
    outer_end_point [0] [1] = y2 + sin(draw_angle - PI/2) * outer_line_thickness;
    outer_end_point [1] [0] = x2 + cos(draw_angle + PI/2) * outer_line_thickness;
    outer_end_point [1] [1] = y2 + sin(draw_angle + PI/2) * outer_line_thickness;

    add_fan_vertex(outer_end_point [0] [0],
                   outer_end_point [0] [1],
                   colours.packet [col] [outer_line_shade]);

    add_fan_vertex(outer_end_point [1] [0],
                   outer_end_point [1] [1],
                   colours.packet [col] [outer_line_shade]);

    add_fan_vertex(x1 + (cos(draw_angle) * flash_size * 2) + cos(draw_angle + PI/2) * outer_line_thickness,
                   by1 + (sin(draw_angle) * flash_size * 2) + sin(draw_angle + PI/2) * outer_line_thickness,
                   colours.packet [col] [outer_line_shade]);

    add_fan_vertex(x1 + cos(draw_angle + PI_4) * flash_size,
                   by1 + sin(draw_angle + PI_4) * flash_size,
                   colours.packet [col] [outer_line_shade]);


    finish_fan_open();

    float end_flash_size = outer_line_thickness * 1.6;
    int end_flash_rand = 0;
//    if (hit == 1)
//     end_flash_size = outer_line_thickness * 1.9;

    float efx2 = x2 + cos(draw_angle) * end_flash_size;
    float efy2 = y2 + sin(draw_angle) * end_flash_size;


    if (!start_fan(efx2, efy2, colours.packet [col] [outer_line_shade]))
     return; // too many fans

    add_fan_vertex(outer_end_point [1] [0],
                   outer_end_point [1] [1],
                   colours.packet [col] [outer_line_shade]);

    add_fan_vertex(outer_end_point [0] [0],
                   outer_end_point [0] [1],
                   colours.packet [col] [outer_line_shade]);

    for (i = 1; i < 8; i ++) // note 1 to 7
    {

     add_fan_vertex(efx2 - cos(draw_angle + (PI / 4) * i) * (end_flash_size + end_flash_rand),
                    efy2 - sin(draw_angle + (PI / 4) * i) * (end_flash_size + end_flash_rand),
                    colours.packet [col] [outer_line_shade]);
    }

    finish_fan();


}


void draw_allocate_beam2(float x1, float by1, float x2, float y2, int col, int prand_seed, int counter)
{

#define ALLOC_STEP_LENGTH 8

 float line_angle = atan2(y2 - by1, x2 - x1);

 float step_x = cos(line_angle);
 float step_y = sin(line_angle);
 float base_x = x1;
 float base_y = by1;
 float new_base_x, new_base_y;

 int steps = hypot(y2 - by1, x2 - x1) / ALLOC_STEP_LENGTH;
 int steps_taken = 0;

 float node_x = base_x;
 float node_y = base_y;
 float new_node_x, new_node_y;

 int line_shade;

 int disp_angle;
 int disp_dist;

 int line_segment = 0 - counter;
// int line_shade_add = 10;
 int wave_length = prand_seed % 400; // higher number actually means shorter waves
 int wave_amp = 5 + prand_seed % 32;

 float flash_size = (counter / 2) + 1;

 line_shade = (counter);// + prand_seed % 3;// + line_shade_add;// + 10 - line_shade_add;// + (prand_seed) % 5;
 if (line_shade >= CLOUD_SHADES)
  line_shade = CLOUD_SHADES - 1;
 if (line_shade < 0)
  line_shade = 0;

 add_diamond(base_x + cos(line_angle) * flash_size,
             base_y + sin(line_angle) * flash_size,
             base_x + cos(line_angle + PI/2) * flash_size,
             base_y + sin(line_angle + PI/2) * flash_size,
             base_x + cos(line_angle + PI) * flash_size,
             base_y + sin(line_angle + PI) * flash_size,
             base_x + cos(line_angle + PI/2 + PI) * flash_size,
             base_y + sin(line_angle + PI/2 + PI) * flash_size,
             colours.packet [col] [line_shade]);

             node_x = base_x + cos(line_angle) * flash_size;
             node_y = base_y + sin(line_angle) * flash_size;

 while(TRUE)
 {

  line_segment ++;
  steps_taken ++;

/*  if (line_segment % 4 == 0
   && line_shade_add > 0)
   line_shade_add --;*/

  new_base_x = base_x + (step_x * ALLOC_STEP_LENGTH);
  new_base_y = base_y + (step_y * ALLOC_STEP_LENGTH);

  disp_angle = prand_seed & ANGLE_MASK;
  prand_seed += 3111;
  disp_dist = prand_seed % 6;
  prand_seed += 7111;


  if (prand_seed % 10 == 0)
   wave_length = prand_seed % 400; // higher number actually means shorter waves
  if (prand_seed % 14 == 0)
   wave_amp = 5 + prand_seed % 30;


  new_node_x = new_base_x + xpart(disp_angle, disp_dist) + cos(line_angle + PI/2) * ypart(line_segment * wave_length, wave_amp);
  new_node_y = new_base_y + ypart(disp_angle, disp_dist) + sin(line_angle + PI/2) * ypart(line_segment * wave_length, wave_amp);

  line_shade = (counter);// + prand_seed % 3;// + line_shade_add;// + 10 - line_shade_add;// + (prand_seed) % 5;
//  prand_seed += 3111;
  if (line_shade >= CLOUD_SHADES)
   line_shade = CLOUD_SHADES - 1;
  if (line_shade < 0)
   line_shade = 0;

  add_line(node_x, node_y, new_node_x, new_node_y, colours.packet [col] [line_shade]);

  if (steps_taken >= steps)
//  if (fabs(new_base_x - x2) < (4 + ALLOC_STEP_LENGTH)
//   && fabs(new_base_y - y2) < (4 + ALLOC_STEP_LENGTH))
  {
   add_diamond(x2 + cos(line_angle) * flash_size,
               y2 + sin(line_angle) * flash_size,
               x2 + cos(line_angle + PI/2) * flash_size,
               y2 + sin(line_angle + PI/2) * flash_size,
               x2 + cos(line_angle + PI) * flash_size,
               y2 + sin(line_angle + PI) * flash_size,
               x2 + cos(line_angle + PI/2 + PI) * flash_size,
               y2 + sin(line_angle + PI/2 + PI) * flash_size,
               colours.packet [col] [line_shade]);

    add_line(new_node_x, new_node_y,
             x2 - cos(line_angle) * flash_size,
             y2 - sin(line_angle) * flash_size,
             colours.packet [col] [line_shade]);
    break;
  }

  base_x = new_base_x;
  base_y = new_base_y;
  node_x = new_node_x;
  node_y = new_node_y;

 };


}

void draw_yield_beam(float x1, float by1, int target_proc_index, int col, int prand_seed, int counter)
{

 float x2 = al_fixtof(w.proc[target_proc_index].x - view.camera_x);
 float y2 = al_fixtof(w.proc[target_proc_index].y - view.camera_y);
 x2 += view.window_x / 2;
 y2 += view.window_y / 2;

 float line_angle = atan2(y2 - by1, x2 - x1);
 int steps = hypot(y2 - by1, x2 - x1) / ALLOC_STEP_LENGTH;
 int steps_taken = 0;

 float step_x = cos(line_angle);
 float step_y = sin(line_angle);
 float base_x = x1;
 float base_y = by1;
 float new_base_x, new_base_y;

 float node_x = base_x;
 float node_y = base_y;
 float new_node_x, new_node_y;

 int line_shade;

 int disp_angle;
 int disp_dist;

 int line_segment = 0 + counter;
 int line_shade_add = 10;
 int wave_length = prand_seed % 900;
 int wave_amp = 5 + prand_seed % 32;

 float flash_size = (counter / 2) + 1;

 line_shade = (counter);// + prand_seed % 3;// + line_shade_add;// + 10 - line_shade_add;// + (prand_seed) % 5;
 if (line_shade >= CLOUD_SHADES)
  line_shade = CLOUD_SHADES - 1;
 if (line_shade < 0)
  line_shade = 0;

 add_diamond(base_x + cos(line_angle) * flash_size,
             base_y + sin(line_angle) * flash_size,
             base_x + cos(line_angle + PI/2) * flash_size,
             base_y + sin(line_angle + PI/2) * flash_size,
             base_x + cos(line_angle + PI) * flash_size,
             base_y + sin(line_angle + PI) * flash_size,
             base_x + cos(line_angle + PI/2 + PI) * flash_size,
             base_y + sin(line_angle + PI/2 + PI) * flash_size,
             colours.packet [col] [line_shade]);

             node_x = base_x + cos(line_angle) * flash_size;
             node_y = base_y + sin(line_angle) * flash_size;

 while(TRUE)
 {

  line_segment ++;
  steps_taken ++;

  if (line_segment % 4 == 0
   && line_shade_add > 0)
   line_shade_add --;

  new_base_x = base_x + (step_x * ALLOC_STEP_LENGTH);
  new_base_y = base_y + (step_y * ALLOC_STEP_LENGTH);

  disp_angle = prand_seed & ANGLE_MASK;
  prand_seed += 3111;
  disp_dist = prand_seed % 6;
  prand_seed += 7111;


  if (prand_seed % 10 == 0)
   wave_length = prand_seed % 600;
  if (prand_seed % 14 == 0)
   wave_amp = 5 + prand_seed % 30;


  new_node_x = new_base_x + xpart(disp_angle, disp_dist) + cos(line_angle + PI/2) * ypart(line_segment * wave_length, wave_amp);
  new_node_y = new_base_y + ypart(disp_angle, disp_dist) + sin(line_angle + PI/2) * ypart(line_segment * wave_length, wave_amp);

  line_shade = (counter);// + prand_seed % 3;// + line_shade_add;// + 10 - line_shade_add;// + (prand_seed) % 5;
//  prand_seed += 3111;
  if (line_shade >= CLOUD_SHADES)
   line_shade = CLOUD_SHADES - 1;
  if (line_shade < 0)
   line_shade = 0;

  add_line(node_x, node_y, new_node_x, new_node_y, colours.packet [col] [line_shade]);

  if (steps_taken >= steps)
//      fabs(new_base_x - x2) < (4 + ALLOC_STEP_LENGTH)
//   && fabs(new_base_y - y2) < (4 + ALLOC_STEP_LENGTH))
  {
   add_diamond(x2 + cos(line_angle) * flash_size,
               y2 + sin(line_angle) * flash_size,
               x2 + cos(line_angle + PI/2) * flash_size,
               y2 + sin(line_angle + PI/2) * flash_size,
               x2 + cos(line_angle + PI) * flash_size,
               y2 + sin(line_angle + PI) * flash_size,
               x2 + cos(line_angle + PI/2 + PI) * flash_size,
               y2 + sin(line_angle + PI/2 + PI) * flash_size,
               colours.packet [col] [line_shade]);

    add_line(new_node_x, new_node_y,
             x2 - cos(line_angle) * flash_size,
             y2 - sin(line_angle) * flash_size,
             colours.packet [col] [line_shade]);
    break;
  }

  base_x = new_base_x;
  base_y = new_base_y;
  node_x = new_node_x;
  node_y = new_node_y;

 };



}


void zap_line(float x1, float by1, float x2, float y2, int col, int prand_seed, int counter, int wave_amplitude)
{
/*
 float x2 = al_fixtof(w.proc[target_proc_index].x - view.camera_x);
 float y2 = al_fixtof(w.proc[target_proc_index].y - view.camera_y);
 x2 += view.window_x / 2;
 y2 += view.window_y / 2;*/

 float line_angle = atan2(y2 - by1, x2 - x1);
 int steps = hypot(y2 - by1, x2 - x1) / ALLOC_STEP_LENGTH;
 int steps_taken = 0;

 float step_x = cos(line_angle);
 float step_y = sin(line_angle);
 float base_x = x1;
 float base_y = by1;
 float new_base_x, new_base_y;

 float node_x = base_x;
 float node_y = base_y;
 float new_node_x, new_node_y;

 int line_shade;

 int disp_angle;
 int disp_dist;

 int line_segment = 0 + counter;
 int line_shade_add = 10;
 int wave_length = prand_seed % 900;
 int wave_amp = wave_amplitude + prand_seed % (wave_amplitude * 6);

 float flash_size = (counter / 3) + 1;

 line_shade = (counter);// + prand_seed % 3;// + line_shade_add;// + 10 - line_shade_add;// + (prand_seed) % 5;
 if (line_shade >= CLOUD_SHADES)
  line_shade = CLOUD_SHADES - 1;
 if (line_shade < 0)
  line_shade = 0;

 add_diamond(base_x + cos(line_angle) * flash_size,
             base_y + sin(line_angle) * flash_size,
             base_x + cos(line_angle + PI/2) * flash_size,
             base_y + sin(line_angle + PI/2) * flash_size,
             base_x + cos(line_angle + PI) * flash_size,
             base_y + sin(line_angle + PI) * flash_size,
             base_x + cos(line_angle + PI/2 + PI) * flash_size,
             base_y + sin(line_angle + PI/2 + PI) * flash_size,
             colours.packet [col] [line_shade]);

             node_x = base_x + cos(line_angle) * flash_size;
             node_y = base_y + sin(line_angle) * flash_size;

 while(TRUE)
 {

  line_segment ++;
  steps_taken ++;

  if (line_segment % 4 == 0
   && line_shade_add > 0)
   line_shade_add --;

  new_base_x = base_x + (step_x * ALLOC_STEP_LENGTH);
  new_base_y = base_y + (step_y * ALLOC_STEP_LENGTH);

  disp_angle = prand_seed & ANGLE_MASK;
  prand_seed += 3111;
  disp_dist = prand_seed % 6;
  prand_seed += 7111;


  if (prand_seed % 10 == 0)
   wave_length = prand_seed % 600;
  if (prand_seed % 14 == 0)
   wave_amp = wave_amplitude + prand_seed % (wave_amplitude * 6);

  new_node_x = new_base_x + xpart(disp_angle, disp_dist) + cos(line_angle + PI/2) * ypart(line_segment * wave_length, wave_amp);
  new_node_y = new_base_y + ypart(disp_angle, disp_dist) + sin(line_angle + PI/2) * ypart(line_segment * wave_length, wave_amp);

  line_shade = (counter);// + prand_seed % 3;// + line_shade_add;// + 10 - line_shade_add;// + (prand_seed) % 5;
//  prand_seed += 3111;
  if (line_shade >= CLOUD_SHADES)
   line_shade = CLOUD_SHADES - 1;
  if (line_shade < 0)
   line_shade = 0;

  add_line(node_x, node_y, new_node_x, new_node_y, colours.packet [col] [line_shade]);

  if (steps_taken >= steps)
//      fabs(new_base_x - x2) < (4 + ALLOC_STEP_LENGTH)
//   && fabs(new_base_y - y2) < (4 + ALLOC_STEP_LENGTH))
  {
   add_diamond(x2 + cos(line_angle) * flash_size,
               y2 + sin(line_angle) * flash_size,
               x2 + cos(line_angle + PI/2) * flash_size,
               y2 + sin(line_angle + PI/2) * flash_size,
               x2 + cos(line_angle + PI) * flash_size,
               y2 + sin(line_angle + PI) * flash_size,
               x2 + cos(line_angle + PI/2 + PI) * flash_size,
               y2 + sin(line_angle + PI/2 + PI) * flash_size,
               colours.packet [col] [line_shade]);

    add_line(new_node_x, new_node_y,
             x2 - cos(line_angle) * flash_size,
             y2 - sin(line_angle) * flash_size,
             colours.packet [col] [line_shade]);
    break;
  }

  base_x = new_base_x;
  base_y = new_base_y;
  node_x = new_node_x;
  node_y = new_node_y;

 };



}



// x and y are positions on screen
void draw_proc_explode_cloud(struct cloudstruct* cl, float x, float y)
{

 int timeout = cl->timeout;


 if (timeout > 20)
  timeout = 20;

 int i;
// int angle;
 float f_angle = fixed_to_radians(cl->angle);
 int base_dist1, base_dist2, base_size1, base_size2;
 int dist1, dist2, dist3;
 int size1, size2;

 float shape_x, shape_y;

 int fill_shade = timeout / 2;
 if (fill_shade > 8)
  fill_shade = 8;
 int edge_shade = fill_shade + 2;
 if (edge_shade > 9)
  edge_shade = 9;

// int c_size1 = 40 + timeout;
// int c_size2 = 15 + timeout;
// int c_size3 = 20 + timeout;

// draw_scaled_outline_shape(cl->data [0], cl->data [1], cl->angle, x, y, cloud_col [CLOUD_COL_RED] [fill_shade],cloud_col [CLOUD_COL_RED] [edge_shade], (float) timeout / 20);

 add_scaled_outline_shape(&shape_dat [cl->data [0]] [cl->data [1]], fixed_to_radians(cl->angle), x, y, colours.packet [cl->colour] [fill_shade], colours.packet [cl->colour] [edge_shade], (float) timeout / 20);

/*
  add_outline_diamond(x + cos(f_angle) * c_size1,
                      y + sin(f_angle) * c_size1,
                      x + cos(f_angle - PI/2) * c_size2,
                      y + sin(f_angle - PI/2) * c_size2,
                      x - cos(f_angle) * c_size3,
                      y - sin(f_angle) * c_size3,
                      x + cos(f_angle + PI/2) * c_size2,
                      y + sin(f_angle + PI/2) * c_size2,
                      cloud_col [CLOUD_COL_RED] [fill_shade],
                      cloud_col [CLOUD_COL_RED] [edge_shade]);*/


 base_dist1 = (20 - timeout) * 6;
 base_dist2 = 115 + timeout *2; //130 - (timeout * 2);
 base_size1 = timeout * 2 + 3;
 base_size2 = timeout / 2 + 2;

 for (i = 2; i < 16; i ++) // first two data entries are shape and size
 {
  fill_shade = (timeout + i) / 4;
  if (fill_shade > 8)
   fill_shade = 8;
  if (fill_shade < 0)
   fill_shade = 0;

  edge_shade = fill_shade + 2;
  if (edge_shade > 9)
   edge_shade = 9;
  if (edge_shade < 0)
   edge_shade = 0;

  dist1 = base_dist1 * (i + 4);
  dist1 /= 15;
  dist1 += 10;

  dist2 = base_dist2 * (i + 4);
  dist2 /= 15;
  dist2 += 10;

  dist3 = (dist1 + dist2) / 2;

  size1 = base_size1 * (i + 10);
  size1 /= 15;
  size2 = base_size2 * (i + 10);
  size2 /= 15;

  f_angle = angle_to_radians(cl->data [i]);

  shape_x = x + cos(f_angle) * (dist3);
  shape_y = y + sin(f_angle) * (dist3);

  add_outline_diamond(x + cos(f_angle) * dist2,
                      y + sin(f_angle) * dist2,
                      shape_x + cos(f_angle - PI/2) * size2,
                      shape_y + sin(f_angle - PI/2) * size2,
                      x + cos(f_angle) * dist1,
                      y + sin(f_angle) * dist1,
                      shape_x + cos(f_angle + PI/2) * size2,
                      shape_y + sin(f_angle + PI/2) * size2,
                      colours.packet [0] [fill_shade],
                      colours.packet [0] [edge_shade]);

// vertices must be: front, side1, back, side2
/*
  shape_x = x + cos(f_angle) * (dist3);
  shape_y = y + sin(f_angle) * (dist3);
  poly_buffer [poly_pos].x = shape_x + cos(f_angle - PI/2) * size2;
  poly_buffer [poly_pos].y = shape_y + sin(f_angle - PI/2) * size2;
  poly_buffer [poly_pos].color = cloud_col [CLOUD_COL_RED] [fill_shade];
  poly_pos ++;
  poly_buffer [poly_pos].x = x + cos(f_angle) * dist2;
  poly_buffer [poly_pos].y = y + sin(f_angle) * dist2;
  poly_buffer [poly_pos].color = cloud_col [CLOUD_COL_RED] [fill_shade];
  poly_pos ++;
  poly_buffer [poly_pos].x = shape_x + cos(f_angle + PI/2) * size2;
  poly_buffer [poly_pos].y = shape_y + sin(f_angle + PI/2) * size2;
  poly_buffer [poly_pos].color = cloud_col [CLOUD_COL_RED] [fill_shade];
  poly_pos ++;


  poly_buffer [poly_pos].x = shape_x + cos(f_angle - PI/2) * size2;
  poly_buffer [poly_pos].y = shape_y + sin(f_angle - PI/2) * size2;
  poly_buffer [poly_pos].color = cloud_col [CLOUD_COL_RED] [fill_shade];
  poly_pos ++;
  poly_buffer [poly_pos].x = x + cos(f_angle) * dist1;
  poly_buffer [poly_pos].y = y + sin(f_angle) * dist1;
  poly_buffer [poly_pos].color = cloud_col [CLOUD_COL_RED] [fill_shade];
  poly_pos ++;
  poly_buffer [poly_pos].x = shape_x + cos(f_angle + PI/2) * size2;
  poly_buffer [poly_pos].y = shape_y + sin(f_angle + PI/2) * size2;
  poly_buffer [poly_pos].color = cloud_col [CLOUD_COL_RED] [fill_shade];
  poly_pos ++;
*/
  f_angle += PI/8;

 }


/*
 for (i = 0; i < 16; i ++)
 {
  dist = base_dist * (i + 10);
  dist /= 15;
  size1 = base_size1 * (i + 10);
  size1 /= 15;
  size2 = base_size2 * (i + 10);
  size2 /= 15;
  f_angle = angle_to_radians(cl->data [i]);

  shape_x = x + cos(f_angle) * (dist + 25);
  shape_y = y + sin(f_angle) * (dist + 25);
  poly_buffer [poly_pos].x = shape_x + cos(f_angle - PI/2) * size2;
  poly_buffer [poly_pos].y = shape_y + sin(f_angle - PI/2) * size2;
  poly_buffer [poly_pos].color = cloud_col [CLOUD_COL_RED] [fill_shade];
  poly_pos ++;
  poly_buffer [poly_pos].x = shape_x + cos(f_angle) * size1;
  poly_buffer [poly_pos].y = shape_y + sin(f_angle) * size1;
  poly_buffer [poly_pos].color = cloud_col [CLOUD_COL_RED] [fill_shade];
  poly_pos ++;
  poly_buffer [poly_pos].x = shape_x + cos(f_angle + PI/2) * size2;
  poly_buffer [poly_pos].y = shape_y + sin(f_angle + PI/2) * size2;
  poly_buffer [poly_pos].color = cloud_col [CLOUD_COL_RED] [fill_shade];
  poly_pos ++;


  poly_buffer [poly_pos].x = shape_x + cos(f_angle - PI/2) * size2;
  poly_buffer [poly_pos].y = shape_y + sin(f_angle - PI/2) * size2;
  poly_buffer [poly_pos].color = cloud_col [CLOUD_COL_RED] [fill_shade];
  poly_pos ++;
  poly_buffer [poly_pos].x = shape_x + cos(f_angle + PI) * size1;
  poly_buffer [poly_pos].y = shape_y + sin(f_angle + PI) * size1;
  poly_buffer [poly_pos].color = cloud_col [CLOUD_COL_RED] [fill_shade];
  poly_pos ++;
  poly_buffer [poly_pos].x = shape_x + cos(f_angle + PI/2) * size2;
  poly_buffer [poly_pos].y = shape_y + sin(f_angle + PI/2) * size2;
  poly_buffer [poly_pos].color = cloud_col [CLOUD_COL_RED] [fill_shade];
  poly_pos ++;

  f_angle += PI/8;

 }
*/

 /*
 int i;
 int beams = 4;
 int vertices = 16;
 int angle = cl->angle;

 outline_buffer [outline_pos].vertex_start = outline_vertex_pos;
 outline_buffer [outline_pos].vertices = vertices;
 outline_buffer [outline_pos].line_col [0] = line_col1;
 outline_buffer [outline_pos].line_col [1] = line_col2;
 outline_buffer [outline_pos].line_col [2] = line_col3;
 outline_pos ++;

 for (i = 0; i < beams; i ++)
 {
  poly_buffer [poly_pos].x = x;
  poly_buffer [poly_pos].y = y;
  poly_buffer [poly_pos].z = 0;
  poly_buffer [poly_pos].color = fill_col;
  poly_pos ++;
  poly_buffer [poly_pos].x = x + fxpart(angle + sh->vertex_angle_float [i], sh->vertex_dist_pixel [i]);
  poly_buffer [poly_pos].y = y + fypart(float_angle + sh->vertex_angle_float [i], sh->vertex_dist_pixel [i]);
  poly_buffer [poly_pos].z = 0;
  poly_buffer [poly_pos].color = fill_col;

  outline_vertex [outline_vertex_pos++] = poly_buffer [poly_pos].x;
  outline_vertex [outline_vertex_pos++] = poly_buffer [poly_pos].y;



  poly_pos ++;
//  outline_pos ++;
  poly_buffer [poly_pos].x = x + fxpart(float_angle + sh->vertex_angle_float [i + 1], sh->vertex_dist_pixel [i + 1]);
  poly_buffer [poly_pos].y = y + fypart(float_angle + sh->vertex_angle_float [i + 1], sh->vertex_dist_pixel [i + 1]);
  poly_buffer [poly_pos].z = 0;
  poly_buffer [poly_pos].color = fill_col;
  poly_pos ++;

*/


//  outline_vertex [outline_vertex_pos++] = poly_buffer [poly_pos - 1].x;
//  outline_vertex [outline_vertex_pos++] = poly_buffer [poly_pos - 1].y;


 check_buffer_sizes();



}





// x and y are positions on screen
void draw_proc_fail_cloud(struct cloudstruct* cl, float x, float y)
{

 int timeout = cl->timeout;

 if (timeout > 20)
  timeout = 20;

 int fill_shade = timeout / 2;
 if (fill_shade > 8)
  fill_shade = 8;
 int edge_shade = fill_shade + 2;
 if (edge_shade > 9)
  edge_shade = 9;

 add_scaled_outline_shape(&shape_dat [cl->data [0]] [cl->data [1]], fixed_to_radians(cl->angle), x, y, colours.drive [cl->colour] [fill_shade], colours.drive [cl->colour] [edge_shade], 1);

 al_draw_line(x - 5, y, x + 5, y, colours.base [COL_YELLOW] [SHADE_MAX], 1);
 al_draw_line(x, y - 5, x, y + 5, colours.base [COL_YELLOW] [SHADE_MAX], 1);

 check_buffer_sizes();



}



void add_triangle(float xa, float ya, float xb, float yb, float xc, float yc, ALLEGRO_COLOR col)
{

  poly_buffer [poly_pos].x = xa;
  poly_buffer [poly_pos].y = ya;
  poly_buffer [poly_pos].z = 0;
  poly_buffer [poly_pos].color = col;
  poly_pos ++;

  poly_buffer [poly_pos].x = xb;
  poly_buffer [poly_pos].y = yb;
  poly_buffer [poly_pos].z = 0;
  poly_buffer [poly_pos].color = col;
  poly_pos ++;

  poly_buffer [poly_pos].x = xc;
  poly_buffer [poly_pos].y = yc;
  poly_buffer [poly_pos].z = 0;
  poly_buffer [poly_pos].color = col;
  poly_pos ++;

// check_buffer_sizes();

}

void add_outline_triangle(float xa, float ya, float xb, float yb, float xc, float yc, ALLEGRO_COLOR col1, ALLEGRO_COLOR col2)
{

 vertex_list [0] [0] = xa;
 vertex_list [0] [1] = ya;
 vertex_list [1] [0] = xb;
 vertex_list [1] [1] = yb;
 vertex_list [2] [0] = xc;
 vertex_list [2] [1] = yc;

 push_to_poly_buffer(3, col1);
 push_loop_to_line_buffer(3, col2);
 check_buffer_sizes();

}

void add_outline_triangle_layer(int layer, float xa, float ya, float xb, float yb, float xc, float yc, ALLEGRO_COLOR col1, ALLEGRO_COLOR col2)
{

 vertex_list [0] [0] = xa;
 vertex_list [0] [1] = ya;
 vertex_list [1] [0] = xb;
 vertex_list [1] [1] = yb;
 vertex_list [2] [0] = xc;
 vertex_list [2] [1] = yc;

 push_to_layer_poly_buffer(layer, 3, col1);
 push_loop_to_layer_line_buffer(layer, 3, col2);
 check_buffer_sizes();

}

void add_triangle_layer(int layer, float xa, float ya, float xb, float yb, float xc, float yc, ALLEGRO_COLOR col)
{

 vertex_list [0] [0] = xa;
 vertex_list [0] [1] = ya;
 vertex_list [1] [0] = xb;
 vertex_list [1] [1] = yb;
 vertex_list [2] [0] = xc;
 vertex_list [2] [1] = yc;

 push_to_layer_poly_buffer(layer, 3, col);
 check_buffer_sizes();

}

void add_simple_outline_triangle_layer(int layer, float x, float y, float angle_1, float length_1, float angle_2, float length_2, float angle_3, float length_3, ALLEGRO_COLOR col1, ALLEGRO_COLOR col2)
{

 vertex_list [0] [0] = x + (cos(angle_1) * length_1);
 vertex_list [0] [1] = y + (sin(angle_1) * length_1);
 vertex_list [1] [0] = x + (cos(angle_2) * length_2);
 vertex_list [1] [1] = y + (sin(angle_2) * length_2);
 vertex_list [2] [0] = x + (cos(angle_3) * length_3);
 vertex_list [2] [1] = y + (sin(angle_3) * length_3);

 push_to_layer_poly_buffer(layer, 3, col1);
 push_loop_to_layer_line_buffer(layer, 3, col2);
 check_buffer_sizes();

}


void add_simple_rectangle_layer(int layer, float x, float y, float length, float width, float angle, ALLEGRO_COLOR col)
{
 float end_x = x + (cos(angle) * length);
 float end_y = y + (sin(angle) * length);

 vertex_list [0] [0] = end_x + (cos(angle + PI_2) * width);
 vertex_list [0] [1] = end_y + (sin(angle + PI_2) * width);
 vertex_list [1] [0] = end_x + (cos(angle - PI_2) * width);
 vertex_list [1] [1] = end_y + (sin(angle - PI_2) * width);

 end_x = x - (cos(angle) * length);
 end_y = y - (sin(angle) * length);

 vertex_list [2] [0] = end_x + (cos(angle + PI_2) * width);
 vertex_list [2] [1] = end_y + (sin(angle + PI_2) * width);

 push_to_layer_poly_buffer(layer, 3, col);

 vertex_list [0] [0] = end_x + (cos(angle - PI_2) * width);
 vertex_list [0] [1] = end_y + (sin(angle - PI_2) * width);

 push_to_layer_poly_buffer(layer, 3, col);


}

void add_simple_outline_rectangle_layer(int layer, float x, float y, float length, float width, float angle, ALLEGRO_COLOR fill_col, ALLEGRO_COLOR edge_col)
{
 float end_x = x + (cos(angle) * length);
 float end_y = y + (sin(angle) * length);

 vertex_list [0] [0] = end_x + (cos(angle + PI_2) * width);
 vertex_list [0] [1] = end_y + (sin(angle + PI_2) * width);
 vertex_list [1] [0] = end_x + (cos(angle - PI_2) * width);
 vertex_list [1] [1] = end_y + (sin(angle - PI_2) * width);
// vertex_list [3] needs the first until the f
// vertex_list [3] [0] = end_x + (cos(angle + PI_2) * width);
// vertex_list [3] [1] = end_y + (sin(angle + PI_2) * width);
 vertex_list [3] [0] = vertex_list [0] [0];
 vertex_list [3] [1] = vertex_list [0] [1];

 end_x = x - (cos(angle) * length);
 end_y = y - (sin(angle) * length);

 vertex_list [2] [0] = end_x + (cos(angle + PI_2) * width);
 vertex_list [2] [1] = end_y + (sin(angle + PI_2) * width);

 push_to_layer_poly_buffer(layer, 3, fill_col);

 vertex_list [0] [0] = end_x + (cos(angle - PI_2) * width);
 vertex_list [0] [1] = end_y + (sin(angle - PI_2) * width);

 push_to_layer_poly_buffer(layer, 3, fill_col);

// now need to reassemble the vertices a little to draw a loop around them:

 vertex_list [4] [0] = vertex_list [3] [0];
 vertex_list [4] [1] = vertex_list [3] [1];
 vertex_list [3] [0] = vertex_list [2] [0];
 vertex_list [3] [1] = vertex_list [2] [1];
 vertex_list [2] [0] = vertex_list [4] [0];
 vertex_list [2] [1] = vertex_list [4] [1];

 push_loop_to_layer_line_buffer(layer, 4, edge_col);


}

void add_simple_outline_diamond_layer(int layer, float x, float y, float front_length, float back_length, float width, float angle, ALLEGRO_COLOR fill_col, ALLEGRO_COLOR edge_col)
{

 vertex_list [0] [0] = x + (cos(angle) * front_length);
 vertex_list [0] [1] = y + (sin(angle) * front_length);

 vertex_list [1] [0] = x + (cos(angle + PI_2) * width);
 vertex_list [1] [1] = y + (sin(angle + PI_2) * width);

 vertex_list [2] [0] = x - (cos(angle) * back_length);
 vertex_list [2] [1] = y - (sin(angle) * back_length);

 push_to_layer_poly_buffer(layer, 3, fill_col);

 vertex_list [1] [0] = x + (cos(angle - PI_2) * width);
 vertex_list [1] [1] = y + (sin(angle - PI_2) * width);

 push_to_layer_poly_buffer(layer, 3, fill_col);

 front_length += 0.7;
 back_length += 0.7;
 width += 0.7;

 vertex_list [0] [0] = x + (cos(angle) * front_length);
 vertex_list [0] [1] = y + (sin(angle) * front_length);

 vertex_list [1] [0] = x + (cos(angle + PI_2) * width);
 vertex_list [1] [1] = y + (sin(angle + PI_2) * width);

 vertex_list [2] [0] = x - (cos(angle) * back_length);
 vertex_list [2] [1] = y - (sin(angle) * back_length);

 vertex_list [3] [0] = x + (cos(angle - PI_2) * width);
 vertex_list [3] [1] = y + (sin(angle - PI_2) * width);

 push_loop_to_layer_line_buffer(layer, 4, edge_col);


}

// vertices must be: front, side1, back, side2
void add_diamond(float xa, float ya, float xb, float yb, float xc, float yc, float xd, float yd, ALLEGRO_COLOR col1)
{

// front
 vertex_list [0] [0] = xa;
 vertex_list [0] [1] = ya;
// side1
 vertex_list [1] [0] = xb;
 vertex_list [1] [1] = yb;
// side2
 vertex_list [2] [0] = xd;
 vertex_list [2] [1] = yd;

 push_to_poly_buffer(3, col1);


// back
 vertex_list [0] [0] = xc;
 vertex_list [0] [1] = yc;

 push_to_poly_buffer(3, col1);

// back
// vertex_list [0] [0] = xd;
// vertex_list [0] [1] = yd;
// push_to_poly_buffer(3, col, shade);

 check_buffer_sizes();

}



// vertices must be: front, side1, back, side2
void add_outline_diamond(float xa, float ya, float xb, float yb, float xc, float yc, float xd, float yd, ALLEGRO_COLOR col1, ALLEGRO_COLOR col2)
{

// front
 vertex_list [0] [0] = xa;
 vertex_list [0] [1] = ya;
// side1
 vertex_list [1] [0] = xb;
 vertex_list [1] [1] = yb;
// side2
 vertex_list [2] [0] = xd;
 vertex_list [2] [1] = yd;

 push_to_poly_buffer(3, col1);


// back
 vertex_list [0] [0] = xc;
 vertex_list [0] [1] = yc;

 push_to_poly_buffer(3, col1);

// front (to vertex 2)
 vertex_list [2] [0] = xa;
 vertex_list [2] [1] = ya;
// side2
 vertex_list [3] [0] = xd;
 vertex_list [3] [1] = yd;

 push_loop_to_line_buffer(4, col2);

// back
// vertex_list [0] [0] = xd;
// vertex_list [0] [1] = yd;
// push_to_poly_buffer(3, col, shade);

 check_buffer_sizes();

}


// like add_outline_diamong, but puts it on a layer
// vertices must be: front, side1, back, side2
void add_outline_diamond_layer(int layer, float xa, float ya, float xb, float yb, float xc, float yc, float xd, float yd, ALLEGRO_COLOR col1, ALLEGRO_COLOR col2)
{

// front
 vertex_list [0] [0] = xa;
 vertex_list [0] [1] = ya;
// side1
 vertex_list [1] [0] = xb;
 vertex_list [1] [1] = yb;
// side2
 vertex_list [2] [0] = xd;
 vertex_list [2] [1] = yd;

 push_to_layer_poly_buffer(layer, 3, col1);

// back
 vertex_list [0] [0] = xc;
 vertex_list [0] [1] = yc;

 push_to_layer_poly_buffer(layer, 3, col1);

// front (to vertex 2)
 vertex_list [2] [0] = xa;
 vertex_list [2] [1] = ya;
// side2
 vertex_list [3] [0] = xd;
 vertex_list [3] [1] = yd;

 push_loop_to_layer_line_buffer(layer, 4, col2);

// back
// vertex_list [0] [0] = xd;
// vertex_list [0] [1] = yd;
// push_to_poly_buffer(3, col, shade);

 check_buffer_sizes();

}


// like add_outline_diamond, but puts it on a layer
// vertices must be: front, side1, back, side2
void add_outline_pentagon_layer(int layer, float xa, float ya, float xb, float yb, float xc, float yc, float xd, float yd, float xe, float ye, ALLEGRO_COLOR col1, ALLEGRO_COLOR col2)
{

// front
 vertex_list [0] [0] = xa;
 vertex_list [0] [1] = ya;
// side1
 vertex_list [1] [0] = xb;
 vertex_list [1] [1] = yb;
// side2
 vertex_list [2] [0] = xc;
 vertex_list [2] [1] = yc;

 push_to_layer_poly_buffer(layer, 3, col1);

 vertex_list [1] [0] = xd;
 vertex_list [1] [1] = yd;

 push_to_layer_poly_buffer(layer, 3, col1);

 vertex_list [2] [0] = xe;
 vertex_list [2] [1] = ye;

 push_to_layer_poly_buffer(layer, 3, col1);


 vertex_list [1] [0] = xb;
 vertex_list [1] [1] = yb;

 vertex_list [2] [0] = xc;
 vertex_list [2] [1] = yc;

 vertex_list [3] [0] = xd;
 vertex_list [3] [1] = yd;

 vertex_list [4] [0] = xe;
 vertex_list [4] [1] = ye;

 push_loop_to_layer_line_buffer(layer, 5, col2);

 check_buffer_sizes();

}


// like add_outline_diamond, but puts it on a layer
// vertices must be clockwise from 0
void add_outline_hexagon_layer(int layer, float xa, float ya, float xb, float yb, float xc, float yc, float xd, float yd, float xe, float ye, float xf, float yf, ALLEGRO_COLOR col1, ALLEGRO_COLOR col2)
{


 vertex_list [0] [0] = xf;
 vertex_list [0] [1] = yf;

 vertex_list [1] [0] = xa;
 vertex_list [1] [1] = ya;

 vertex_list [2] [0] = xb;
 vertex_list [2] [1] = yb;

 push_to_layer_poly_buffer(layer, 3, col1);

 vertex_list [1] [0] = xc;
 vertex_list [1] [1] = yc;

 push_to_layer_poly_buffer(layer, 3, col1);

 vertex_list [2] [0] = xe;
 vertex_list [2] [1] = ye;

 push_to_layer_poly_buffer(layer, 3, col1);

 vertex_list [0] [0] = xd;
 vertex_list [0] [1] = yd;

 push_to_layer_poly_buffer(layer, 3, col1);


 vertex_list [1] [0] = xe;
 vertex_list [1] [1] = ye;

 vertex_list [2] [0] = xf;
 vertex_list [2] [1] = yf;

 vertex_list [3] [0] = xa;
 vertex_list [3] [1] = ya;

 vertex_list [4] [0] = xb;
 vertex_list [4] [1] = yb;

 vertex_list [5] [0] = xc;
 vertex_list [5] [1] = yc;

 push_loop_to_layer_line_buffer(layer, 6, col2);

 check_buffer_sizes();

}


// like add_outline_diamong, but puts it on a layer
// vertices must be: front, side1, back, side2
void add_diamond_layer(int layer, float xa, float ya, float xb, float yb, float xc, float yc, float xd, float yd, ALLEGRO_COLOR col1)
{

// front
 vertex_list [0] [0] = xa;
 vertex_list [0] [1] = ya;
// side1
 vertex_list [1] [0] = xb;
 vertex_list [1] [1] = yb;
// side2
 vertex_list [2] [0] = xd;
 vertex_list [2] [1] = yd;

 push_to_layer_poly_buffer(layer, 3, col1);

// back
 vertex_list [0] [0] = xc;
 vertex_list [0] [1] = yc;

 push_to_layer_poly_buffer(layer, 3, col1);

 check_buffer_sizes();

}



void add_outline_square(float xa, float ya, float xb, float yb, ALLEGRO_COLOR col1, ALLEGRO_COLOR col2)
{


// top left (may not actually be top left; doesn't matter)
 vertex_list [0] [0] = xa;
 vertex_list [0] [1] = ya;
// top right
 vertex_list [1] [0] = xb;
 vertex_list [1] [1] = ya;
// bottom right
 vertex_list [2] [0] = xb;
 vertex_list [2] [1] = yb;

 push_to_poly_buffer(3, col1);
// replace top right with bottom left
 vertex_list [1] [0] = xa;
 vertex_list [1] [1] = yb;

 push_to_poly_buffer(3, col1);

// at this point we have top left to bottom left to bottom right
// need to add top right
 vertex_list [3] [0] = xb;
 vertex_list [3] [1] = ya;

 push_loop_to_line_buffer(4, col2);

 check_buffer_sizes();

}

// only works for orthogonal hexagons (i.e. with vertical sides) (otherwise it would require two extra triangles)
void add_outline_orthogonal_hexagon(float x, float y, float size, ALLEGRO_COLOR col1, ALLEGRO_COLOR col2)
{


 float hex_vertex_list [6] [2];

 int i;
 float angle_inc = PI/3;
 float base_angle = PI * -0.5;

 for (i = 0; i < 6; i ++)
 {
  hex_vertex_list [i] [0] = x + cos(base_angle + (angle_inc*i)) * size;
  hex_vertex_list [i] [1] = y + sin(base_angle + (angle_inc*i)) * size;
 }

// elongate the hexagon slightly to make up for the fact that the vertical stacking is not correct (the hexes are stacked as if they were squares)
 hex_vertex_list [0] [1] -= 1;
 hex_vertex_list [1] [1] -= 1;
 hex_vertex_list [2] [1] += 1;
 hex_vertex_list [3] [1] += 1;
 hex_vertex_list [5] [1] -= 1;
 hex_vertex_list [4] [1] += 1;

// top triangle
 vertex_list [0] [0] = hex_vertex_list [0] [0];
 vertex_list [0] [1] = hex_vertex_list [0] [1];

 vertex_list [1] [0] = hex_vertex_list [1] [0];
 vertex_list [1] [1] = hex_vertex_list [1] [1];

 vertex_list [2] [0] = hex_vertex_list [5] [0];
 vertex_list [2] [1] = hex_vertex_list [5] [1];

 push_to_poly_buffer(3, col1);

// move top point to lower left point
 vertex_list [0] [0] = hex_vertex_list [4] [0];
 vertex_list [0] [1] = hex_vertex_list [4] [1];
 push_to_poly_buffer(3, col1);

// move upper left point to lower right
 vertex_list [2] [0] = hex_vertex_list [2] [0];
 vertex_list [2] [1] = hex_vertex_list [2] [1];
 push_to_poly_buffer(3, col1);

// move upper right point to bottom point
 vertex_list [1] [0] = hex_vertex_list [3] [0];
 vertex_list [1] [1] = hex_vertex_list [3] [1];
 push_to_poly_buffer(3, col1);

// now do line loop:
 vertex_list [0] [0] = hex_vertex_list [0] [0];
 vertex_list [0] [1] = hex_vertex_list [0] [1];
 vertex_list [1] [0] = hex_vertex_list [1] [0];
 vertex_list [1] [1] = hex_vertex_list [1] [1];
 vertex_list [2] [0] = hex_vertex_list [2] [0];
 vertex_list [2] [1] = hex_vertex_list [2] [1];
 vertex_list [3] [0] = hex_vertex_list [3] [0];
 vertex_list [3] [1] = hex_vertex_list [3] [1];
 vertex_list [4] [0] = hex_vertex_list [4] [0];
 vertex_list [4] [1] = hex_vertex_list [4] [1];
 vertex_list [5] [0] = hex_vertex_list [5] [0];
 vertex_list [5] [1] = hex_vertex_list [5] [1];

 push_loop_to_line_buffer(6, col2);

 check_buffer_sizes();

}

void add_outline_shape(float x, float y, float float_angle, struct shapestruct* sh, ALLEGRO_COLOR fill_col, ALLEGRO_COLOR edge_col, ALLEGRO_COLOR edge_col2)
{

 int i;


 for (i = 0; i < sh->vertices; i ++)
 {
  poly_buffer [poly_pos].x = x;
  poly_buffer [poly_pos].y = y;
  poly_buffer [poly_pos].z = 0;
  poly_buffer [poly_pos].color = fill_col;
  poly_pos ++;
  poly_buffer [poly_pos].x = x + fxpart(float_angle + sh->vertex_angle_float [i], sh->vertex_dist_pixel [i]);
  poly_buffer [poly_pos].y = y + fypart(float_angle + sh->vertex_angle_float [i], sh->vertex_dist_pixel [i]);
  poly_buffer [poly_pos].z = 0;
  poly_buffer [poly_pos].color = fill_col;

  line_buffer [line_pos].x = poly_buffer [poly_pos].x;
  line_buffer [line_pos].y = poly_buffer [poly_pos].y;
  line_buffer [line_pos].z = 0;
  line_buffer [line_pos].color = edge_col;

  poly_pos ++;
  line_pos ++;

  poly_buffer [poly_pos].x = x + fxpart(float_angle + sh->vertex_angle_float [i + 1], sh->vertex_dist_pixel [i + 1]);
  poly_buffer [poly_pos].y = y + fypart(float_angle + sh->vertex_angle_float [i + 1], sh->vertex_dist_pixel [i + 1]);
  poly_buffer [poly_pos].z = 0;
  poly_buffer [poly_pos].color = fill_col;

  line_buffer [line_pos].x = poly_buffer [poly_pos].x;
  line_buffer [line_pos].y = poly_buffer [poly_pos].y;
  line_buffer [line_pos].z = 0;
  line_buffer [line_pos].color = edge_col;
  line_pos ++;

  poly_pos ++;


 }

#define LENGTH_MOD 0.5

 for (i = 0; i < sh->vertices; i ++)
 {
  line_buffer [line_pos].x = x + fxpart(float_angle + sh->vertex_angle_float [i], sh->vertex_dist_pixel [i] - 1);
  line_buffer [line_pos].y = y + fypart(float_angle + sh->vertex_angle_float [i], sh->vertex_dist_pixel [i] - 1);
  line_buffer [line_pos].z = 0;
  line_buffer [line_pos].color = edge_col2;

  line_pos ++;

  line_buffer [line_pos].x = x + fxpart(float_angle + sh->vertex_angle_float [i + 1], sh->vertex_dist_pixel [i + 1] - 1);
  line_buffer [line_pos].y = y + fypart(float_angle + sh->vertex_angle_float [i + 1], sh->vertex_dist_pixel [i + 1] - 1);
  line_buffer [line_pos].z = 0;
  line_buffer [line_pos].color = edge_col2;

  line_pos ++;


 }


 for (i = 0; i < sh->vertices; i ++)
 {
  line_buffer [line_pos].x = x + fxpart(float_angle + sh->vertex_angle_float [i], sh->vertex_dist_pixel [i] + 1);
  line_buffer [line_pos].y = y + fypart(float_angle + sh->vertex_angle_float [i], sh->vertex_dist_pixel [i] + 1);
  line_buffer [line_pos].z = 0;
  line_buffer [line_pos].color = edge_col2;

  line_pos ++;

  line_buffer [line_pos].x = x + fxpart(float_angle + sh->vertex_angle_float [i + 1], sh->vertex_dist_pixel [i + 1] + 1);
  line_buffer [line_pos].y = y + fypart(float_angle + sh->vertex_angle_float [i + 1], sh->vertex_dist_pixel [i + 1] + 1);
  line_buffer [line_pos].z = 0;
  line_buffer [line_pos].color = edge_col2;

  line_pos ++;


 }


//  outline_vertex [outline_vertex_pos++] = poly_buffer [poly_pos - 1].x;
//  outline_vertex [outline_vertex_pos++] = poly_buffer [poly_pos - 1].y;


 check_buffer_sizes();


}


//void add_scaled_outline_shape(struct shapestruct* sh, float float_angle, float x, float y, ALLEGRO_COLOR fill_col, ALLEGRO_COLOR edge_col, float scale)
//void add_outline_shape(float x, float y, float float_angle, struct shapestruct* sh, ALLEGRO_COLOR line_col1, ALLEGRO_COLOR line_col2, ALLEGRO_COLOR line_col3, ALLEGRO_COLOR fill_col)
/*
// Note: this is not the most efficient way to draw most shapes
void add_outline_shape(float x, float y, float float_angle, struct shapestruct* sh, ALLEGRO_COLOR fill_col, ALLEGRO_COLOR edge_col, ALLEGRO_COLOR edge_col2)
{

 int i;


 for (i = 0; i < sh->vertices; i ++)
 {
  poly_buffer [poly_pos].x = x;
  poly_buffer [poly_pos].y = y;
  poly_buffer [poly_pos].z = 0;
  poly_buffer [poly_pos].color = fill_col;
  poly_pos ++;
  poly_buffer [poly_pos].x = x + fxpart(float_angle + sh->vertex_angle_float [i], sh->vertex_dist_pixel [i]);
  poly_buffer [poly_pos].y = y + fypart(float_angle + sh->vertex_angle_float [i], sh->vertex_dist_pixel [i]);
  poly_buffer [poly_pos].z = 0;
  poly_buffer [poly_pos].color = fill_col;

  line_buffer [line_pos].x = poly_buffer [poly_pos].x;
  line_buffer [line_pos].y = poly_buffer [poly_pos].y;
  line_buffer [line_pos].z = 0;
  line_buffer [line_pos].color = edge_col;

  poly_pos ++;
  line_pos ++;

  poly_buffer [poly_pos].x = x + fxpart(float_angle + sh->vertex_angle_float [i + 1], sh->vertex_dist_pixel [i + 1]);
  poly_buffer [poly_pos].y = y + fypart(float_angle + sh->vertex_angle_float [i + 1], sh->vertex_dist_pixel [i + 1]);
  poly_buffer [poly_pos].z = 0;
  poly_buffer [poly_pos].color = fill_col;

  line_buffer [line_pos].x = poly_buffer [poly_pos].x;
  line_buffer [line_pos].y = poly_buffer [poly_pos].y;
  line_buffer [line_pos].z = 0;
  line_buffer [line_pos].color = edge_col;
  line_pos ++;

  poly_pos ++;


 }

#define LENGTH_MOD 0.5

 for (i = 0; i < sh->vertices; i ++)
 {
  line_buffer [line_pos].x = x + fxpart(float_angle + sh->vertex_angle_float [i], sh->vertex_dist_pixel [i] - 1);
  line_buffer [line_pos].y = y + fypart(float_angle + sh->vertex_angle_float [i], sh->vertex_dist_pixel [i] - 1);
  line_buffer [line_pos].z = 0;
  line_buffer [line_pos].color = edge_col2;

  line_pos ++;

  line_buffer [line_pos].x = x + fxpart(float_angle + sh->vertex_angle_float [i + 1], sh->vertex_dist_pixel [i + 1] - 1);
  line_buffer [line_pos].y = y + fypart(float_angle + sh->vertex_angle_float [i + 1], sh->vertex_dist_pixel [i + 1] - 1);
  line_buffer [line_pos].z = 0;
  line_buffer [line_pos].color = edge_col2;

  line_pos ++;


 }


 for (i = 0; i < sh->vertices; i ++)
 {
  line_buffer [line_pos].x = x + fxpart(float_angle + sh->vertex_angle_float [i], sh->vertex_dist_pixel [i] + 1);
  line_buffer [line_pos].y = y + fypart(float_angle + sh->vertex_angle_float [i], sh->vertex_dist_pixel [i] + 1);
  line_buffer [line_pos].z = 0;
  line_buffer [line_pos].color = edge_col2;

  line_pos ++;

  line_buffer [line_pos].x = x + fxpart(float_angle + sh->vertex_angle_float [i + 1], sh->vertex_dist_pixel [i + 1] + 1);
  line_buffer [line_pos].y = y + fypart(float_angle + sh->vertex_angle_float [i + 1], sh->vertex_dist_pixel [i + 1] + 1);
  line_buffer [line_pos].z = 0;
  line_buffer [line_pos].color = edge_col2;

  line_pos ++;


 }


//  outline_vertex [outline_vertex_pos++] = poly_buffer [poly_pos - 1].x;
//  outline_vertex [outline_vertex_pos++] = poly_buffer [poly_pos - 1].y;


 check_buffer_sizes();


}
*/

#define POINT_INWARD sh->vertex_notch_inwards [v]
#define EDGE_INWARD sh->vertex_notch_sidewards [v]

//void add_scaled_outline_shape(struct shapestruct* sh, float float_angle, float x, float y, ALLEGRO_COLOR fill_col, ALLEGRO_COLOR edge_col, float scale)
//void add_outline_shape(float x, float y, float float_angle, struct shapestruct* sh, ALLEGRO_COLOR line_col1, ALLEGRO_COLOR line_col2, ALLEGRO_COLOR line_col3, ALLEGRO_COLOR fill_col)
void add_proc_diamond(float x, float y, float float_angle, struct shapestruct* sh, int size, ALLEGRO_COLOR fill_col, ALLEGRO_COLOR edge_col)
{

 int i;
 float f_angle;

 float dx [12];
 float dy [12];
 int v = 0;
 f_angle = float_angle + sh->vertex_angle_float [v];

 float inner_shape_separation = 0.8;
#define INNER_SHAPE inner_shape_separation
/*
 switch(size)
 {
  case 0: inner_shape_separation = 0.8; break;
  case 1: inner_shape_separation = 0.8; break;
  case 2: inner_shape_separation = 0.8; break;
  case 3: inner_shape_separation = 0.8; break;
 }*/

 add_diamond(x + fxpart(f_angle + sh->vertex_angle_float [0], sh->vertex_dist_pixel [0] * INNER_SHAPE),
             y + fypart(f_angle + sh->vertex_angle_float [0], sh->vertex_dist_pixel [0] * INNER_SHAPE),
             x + fxpart(f_angle + sh->vertex_angle_float [1], sh->vertex_dist_pixel [1] * INNER_SHAPE),
             y + fypart(f_angle + sh->vertex_angle_float [1], sh->vertex_dist_pixel [1] * INNER_SHAPE),
             x + fxpart(f_angle + sh->vertex_angle_float [2], sh->vertex_dist_pixel [2] * INNER_SHAPE),
             y + fypart(f_angle + sh->vertex_angle_float [2], sh->vertex_dist_pixel [2] * INNER_SHAPE),
             x + fxpart(f_angle + sh->vertex_angle_float [3], sh->vertex_dist_pixel [3] * INNER_SHAPE),
             y + fypart(f_angle + sh->vertex_angle_float [3], sh->vertex_dist_pixel [3] * INNER_SHAPE),
             edge_col);


 dx [0] = x + fxpart(f_angle, sh->vertex_dist_pixel [0] - POINT_INWARD);
 dy [0] = y + fypart(f_angle, sh->vertex_dist_pixel [0] - POINT_INWARD);
 dx [1] = x + fxpart(f_angle, sh->vertex_dist_pixel [0]) + fxpart(f_angle + fixed_to_radians(sh->external_angle [EXANGLE_NEXT] [0]), EDGE_INWARD);
 dy [1] = y + fypart(f_angle, sh->vertex_dist_pixel [0]) + fypart(f_angle + fixed_to_radians(sh->external_angle [EXANGLE_NEXT] [0]), EDGE_INWARD);

 v = 1;
 f_angle = float_angle + sh->vertex_angle_float [v];

 dx [2] = x + fxpart(f_angle, sh->vertex_dist_pixel [1]) + fxpart(f_angle + fixed_to_radians(sh->external_angle [EXANGLE_PREVIOUS] [1]), EDGE_INWARD);
 dy [2] = y + fypart(f_angle, sh->vertex_dist_pixel [1]) + fypart(f_angle + fixed_to_radians(sh->external_angle [EXANGLE_PREVIOUS] [1]), EDGE_INWARD);
 dx [3] = x + fxpart(f_angle, sh->vertex_dist_pixel [1] - POINT_INWARD);
 dy [3] = y + fypart(f_angle, sh->vertex_dist_pixel [1] - POINT_INWARD);
 dx [4] = x + fxpart(f_angle, sh->vertex_dist_pixel [1]) + fxpart(f_angle + fixed_to_radians(sh->external_angle [EXANGLE_NEXT] [1]), EDGE_INWARD);
 dy [4] = y + fypart(f_angle, sh->vertex_dist_pixel [1]) + fypart(f_angle + fixed_to_radians(sh->external_angle [EXANGLE_NEXT] [1]), EDGE_INWARD);

 v = 2;
 f_angle = float_angle + sh->vertex_angle_float [v];

 dx [5] = x + fxpart(f_angle, sh->vertex_dist_pixel [2]) + fxpart(f_angle + fixed_to_radians(sh->external_angle [EXANGLE_PREVIOUS] [2]), EDGE_INWARD);
 dy [5] = y + fypart(f_angle, sh->vertex_dist_pixel [2]) + fypart(f_angle + fixed_to_radians(sh->external_angle [EXANGLE_PREVIOUS] [2]), EDGE_INWARD);
 dx [6] = x + fxpart(f_angle, sh->vertex_dist_pixel [2] - POINT_INWARD);
 dy [6] = y + fypart(f_angle, sh->vertex_dist_pixel [2] - POINT_INWARD);
 dx [7] = x + fxpart(f_angle, sh->vertex_dist_pixel [2]) + fxpart(f_angle + fixed_to_radians(sh->external_angle [EXANGLE_NEXT] [2]), EDGE_INWARD);
 dy [7] = y + fypart(f_angle, sh->vertex_dist_pixel [2]) + fypart(f_angle + fixed_to_radians(sh->external_angle [EXANGLE_NEXT] [2]), EDGE_INWARD);

 v = 3;
 f_angle = float_angle + sh->vertex_angle_float [v];

 dx [8] = x + fxpart(f_angle, sh->vertex_dist_pixel [3]) + fxpart(f_angle + fixed_to_radians(sh->external_angle [EXANGLE_PREVIOUS] [3]), EDGE_INWARD);
 dy [8] = y + fypart(f_angle, sh->vertex_dist_pixel [3]) + fypart(f_angle + fixed_to_radians(sh->external_angle [EXANGLE_PREVIOUS] [3]), EDGE_INWARD);
 dx [9] = x + fxpart(f_angle, sh->vertex_dist_pixel [3] - POINT_INWARD);
 dy [9] = y + fypart(f_angle, sh->vertex_dist_pixel [3] - POINT_INWARD);
 dx [10] = x + fxpart(f_angle, sh->vertex_dist_pixel [3]) + fxpart(f_angle + fixed_to_radians(sh->external_angle [EXANGLE_NEXT] [3]), EDGE_INWARD);
 dy [10] = y + fypart(f_angle, sh->vertex_dist_pixel [3]) + fypart(f_angle + fixed_to_radians(sh->external_angle [EXANGLE_NEXT] [3]), EDGE_INWARD);

 v = 0;
 f_angle = float_angle + sh->vertex_angle_float [v];

 dx [11] = x + fxpart(f_angle, sh->vertex_dist_pixel [0]) + fxpart(f_angle + fixed_to_radians(sh->external_angle [EXANGLE_PREVIOUS] [0]), EDGE_INWARD);
 dy [11] = y + fypart(f_angle, sh->vertex_dist_pixel [0]) + fypart(f_angle + fixed_to_radians(sh->external_angle [EXANGLE_PREVIOUS] [0]), EDGE_INWARD);

 for (i = 0; i < 11; i ++)
 {
  poly_buffer [poly_pos].x = x;
  poly_buffer [poly_pos].y = y;
  poly_buffer [poly_pos].z = 0;
  poly_buffer [poly_pos].color = fill_col;
  poly_pos ++;
  poly_buffer [poly_pos].x = dx [i];
  poly_buffer [poly_pos].y = dy [i];
  poly_buffer [poly_pos].z = 0;
  poly_buffer [poly_pos].color = fill_col;
  poly_pos ++;
  poly_buffer [poly_pos].x = dx [i + 1];
  poly_buffer [poly_pos].y = dy [i + 1];
  poly_buffer [poly_pos].z = 0;
  poly_buffer [poly_pos].color = fill_col;
  poly_pos ++;
/*
  line_buffer [line_pos].x = dx [i];
  line_buffer [line_pos].y = dy [i];
  line_buffer [line_pos].z = 0;
  line_buffer [line_pos].color = edge_col;
  line_pos ++;
  line_buffer [line_pos].x = dx [i + 1];
  line_buffer [line_pos].y = dy [i + 1];
  line_buffer [line_pos].z = 0;
  line_buffer [line_pos].color = edge_col;
  line_pos ++;*/
 }

  poly_buffer [poly_pos].x = x;
  poly_buffer [poly_pos].y = y;
  poly_buffer [poly_pos].z = 0;
  poly_buffer [poly_pos].color = fill_col;
  poly_pos ++;
  poly_buffer [poly_pos].x = dx [11];
  poly_buffer [poly_pos].y = dy [11];
  poly_buffer [poly_pos].z = 0;
  poly_buffer [poly_pos].color = fill_col;
  poly_pos ++;
  poly_buffer [poly_pos].x = dx [0];
  poly_buffer [poly_pos].y = dy [0];
  poly_buffer [poly_pos].z = 0;
  poly_buffer [poly_pos].color = fill_col;
  poly_pos ++;
/*
  line_buffer [line_pos].x = dx [11];
  line_buffer [line_pos].y = dy [11];
  line_buffer [line_pos].z = 0;
  line_buffer [line_pos].color = edge_col;
  line_pos ++;
  line_buffer [line_pos].x = dx [0];
  line_buffer [line_pos].y = dy [0];
  line_buffer [line_pos].z = 0;
  line_buffer [line_pos].color = edge_col;
  line_pos ++;*/




#define SPECIAL (-1)

// edge_col = team_colours [1] [TCOL_LINE2];

 v = 0;
 f_angle = float_angle + sh->vertex_angle_float [v];

 dx [0] = x + fxpart(f_angle, sh->vertex_dist_pixel [0] - POINT_INWARD - SPECIAL);
 dy [0] = y + fypart(f_angle, sh->vertex_dist_pixel [0] - POINT_INWARD - SPECIAL);
 dx [1] = x + fxpart(f_angle, sh->vertex_dist_pixel [0] - SPECIAL) + fxpart(f_angle + fixed_to_radians(sh->external_angle [EXANGLE_NEXT] [0]), EDGE_INWARD);
 dy [1] = y + fypart(f_angle, sh->vertex_dist_pixel [0] - SPECIAL) + fypart(f_angle + fixed_to_radians(sh->external_angle [EXANGLE_NEXT] [0]), EDGE_INWARD);

 v = 1;
 f_angle = float_angle + sh->vertex_angle_float [v];

 dx [2] = x + fxpart(f_angle, sh->vertex_dist_pixel [1] - SPECIAL) + fxpart(f_angle + fixed_to_radians(sh->external_angle [EXANGLE_PREVIOUS] [1]), EDGE_INWARD);
 dy [2] = y + fypart(f_angle, sh->vertex_dist_pixel [1] - SPECIAL) + fypart(f_angle + fixed_to_radians(sh->external_angle [EXANGLE_PREVIOUS] [1]), EDGE_INWARD);
 dx [3] = x + fxpart(f_angle, sh->vertex_dist_pixel [1] - POINT_INWARD - SPECIAL);
 dy [3] = y + fypart(f_angle, sh->vertex_dist_pixel [1] - POINT_INWARD - SPECIAL);
 dx [4] = x + fxpart(f_angle, sh->vertex_dist_pixel [1] - SPECIAL) + fxpart(f_angle + fixed_to_radians(sh->external_angle [EXANGLE_NEXT] [1]), EDGE_INWARD);
 dy [4] = y + fypart(f_angle, sh->vertex_dist_pixel [1] - SPECIAL) + fypart(f_angle + fixed_to_radians(sh->external_angle [EXANGLE_NEXT] [1]), EDGE_INWARD);

 v = 2;
 f_angle = float_angle + sh->vertex_angle_float [v];

 dx [5] = x + fxpart(f_angle, sh->vertex_dist_pixel [2] - SPECIAL) + fxpart(f_angle + fixed_to_radians(sh->external_angle [EXANGLE_PREVIOUS] [2]), EDGE_INWARD);
 dy [5] = y + fypart(f_angle, sh->vertex_dist_pixel [2] - SPECIAL) + fypart(f_angle + fixed_to_radians(sh->external_angle [EXANGLE_PREVIOUS] [2]), EDGE_INWARD);
 dx [6] = x + fxpart(f_angle, sh->vertex_dist_pixel [2] - POINT_INWARD - SPECIAL);
 dy [6] = y + fypart(f_angle, sh->vertex_dist_pixel [2] - POINT_INWARD - SPECIAL);
 dx [7] = x + fxpart(f_angle, sh->vertex_dist_pixel [2] - SPECIAL) + fxpart(f_angle + fixed_to_radians(sh->external_angle [EXANGLE_NEXT] [2]), EDGE_INWARD);
 dy [7] = y + fypart(f_angle, sh->vertex_dist_pixel [2] - SPECIAL) + fypart(f_angle + fixed_to_radians(sh->external_angle [EXANGLE_NEXT] [2]), EDGE_INWARD);

 v = 3;
 f_angle = float_angle + sh->vertex_angle_float [v];

 dx [8] = x + fxpart(f_angle, sh->vertex_dist_pixel [3] - SPECIAL) + fxpart(f_angle + fixed_to_radians(sh->external_angle [EXANGLE_PREVIOUS] [3]), EDGE_INWARD);
 dy [8] = y + fypart(f_angle, sh->vertex_dist_pixel [3] - SPECIAL) + fypart(f_angle + fixed_to_radians(sh->external_angle [EXANGLE_PREVIOUS] [3]), EDGE_INWARD);
 dx [9] = x + fxpart(f_angle, sh->vertex_dist_pixel [3] - POINT_INWARD - SPECIAL);
 dy [9] = y + fypart(f_angle, sh->vertex_dist_pixel [3] - POINT_INWARD - SPECIAL);
 dx [10] = x + fxpart(f_angle, sh->vertex_dist_pixel [3] - SPECIAL) + fxpart(f_angle + fixed_to_radians(sh->external_angle [EXANGLE_NEXT] [3]), EDGE_INWARD);
 dy [10] = y + fypart(f_angle, sh->vertex_dist_pixel [3] - SPECIAL) + fypart(f_angle + fixed_to_radians(sh->external_angle [EXANGLE_NEXT] [3]), EDGE_INWARD);

 v = 0;
 f_angle = float_angle + sh->vertex_angle_float [v];

 dx [11] = x + fxpart(f_angle, sh->vertex_dist_pixel [0] - SPECIAL) + fxpart(f_angle + fixed_to_radians(sh->external_angle [EXANGLE_PREVIOUS] [0]), EDGE_INWARD);
 dy [11] = y + fypart(f_angle, sh->vertex_dist_pixel [0] - SPECIAL) + fypart(f_angle + fixed_to_radians(sh->external_angle [EXANGLE_PREVIOUS] [0]), EDGE_INWARD);
/*
 for (i = 0; i < 11; i ++)
 {

  line_buffer [line_pos].x = dx [i];
  line_buffer [line_pos].y = dy [i];
  line_buffer [line_pos].z = 0;
  line_buffer [line_pos].color = edge_col;
  line_pos ++;
  line_buffer [line_pos].x = dx [i + 1];
  line_buffer [line_pos].y = dy [i + 1];
  line_buffer [line_pos].z = 0;
  line_buffer [line_pos].color = edge_col;
  line_pos ++;
 }

  line_buffer [line_pos].x = dx [11];
  line_buffer [line_pos].y = dy [11];
  line_buffer [line_pos].z = 0;
  line_buffer [line_pos].color = edge_col;
  line_pos ++;
  line_buffer [line_pos].x = dx [0];
  line_buffer [line_pos].y = dy [0];
  line_buffer [line_pos].z = 0;
  line_buffer [line_pos].color = edge_col;
  line_pos ++;

*/



 check_buffer_sizes();


}










//void add_scaled_outline_shape(struct shapestruct* sh, float float_angle, float x, float y, ALLEGRO_COLOR fill_col, ALLEGRO_COLOR edge_col, float scale)
//void add_outline_shape(float x, float y, float float_angle, struct shapestruct* sh, ALLEGRO_COLOR line_col1, ALLEGRO_COLOR line_col2, ALLEGRO_COLOR line_col3, ALLEGRO_COLOR fill_col)
void add_proc_shape(struct procstruct* pr, float x, float y, float float_angle, struct shapestruct* sh, int shape, int size, ALLEGRO_COLOR fill_col, ALLEGRO_COLOR fill2_col, ALLEGRO_COLOR edge_col)
{

 float f_angle;

 int v = 0;
 int next_vertex;

 float inner_shape_separation = 0.8;

//#define INNER_SHAPE inner_shape_separation
/*
 switch(size)
 {
  case 0: inner_shape_separation = 0.8; break;
  case 1: inner_shape_separation = 0.8; break;
  case 2: inner_shape_separation = 0.8; break;
  case 3: inner_shape_separation = 0.8; break;
 }*/

 f_angle = float_angle + sh->vertex_angle_float [0];

 switch(shape)
 {
  case SHAPE_4SQUARE:
  case SHAPE_4POINTY:
  case SHAPE_4DIAMOND:
// diamond can be drawn with 2 triangles instead of as a fan of 4:
    add_diamond(x + fxpart(f_angle + sh->vertex_angle_float [0], sh->vertex_dist_pixel [0] * INNER_SHAPE),
             y + fypart(f_angle + sh->vertex_angle_float [0], sh->vertex_dist_pixel [0] * INNER_SHAPE),
             x + fxpart(f_angle + sh->vertex_angle_float [1], sh->vertex_dist_pixel [1] * INNER_SHAPE),
             y + fypart(f_angle + sh->vertex_angle_float [1], sh->vertex_dist_pixel [1] * INNER_SHAPE),
             x + fxpart(f_angle + sh->vertex_angle_float [2], sh->vertex_dist_pixel [2] * INNER_SHAPE),
             y + fypart(f_angle + sh->vertex_angle_float [2], sh->vertex_dist_pixel [2] * INNER_SHAPE),
             x + fxpart(f_angle + sh->vertex_angle_float [3], sh->vertex_dist_pixel [3] * INNER_SHAPE),
             y + fypart(f_angle + sh->vertex_angle_float [3], sh->vertex_dist_pixel [3] * INNER_SHAPE),
             fill2_col);
    break;
   case SHAPE_8OCTAGON:
   default:
    if (!start_fan(x, y, fill2_col))
     return; // too many fans

    for (v = 0; v < sh->vertices; v ++)
    {
     add_fan_vertex(x + fxpart(f_angle + sh->vertex_angle_float [v], sh->vertex_dist_pixel [v] * INNER_SHAPE),
                    y + fypart(f_angle + sh->vertex_angle_float [v], sh->vertex_dist_pixel [v] * INNER_SHAPE),
                    fill2_col);
    }
    finish_fan();
/*    if (method_notches == 0)
    {
     if (!start_fan(x, y, fill_col))
      return; // too many fans
     for (v = 0; v < sh->vertices; v ++)
     {
      add_fan_vertex(x + fxpart(f_angle + sh->vertex_angle_float [v], sh->vertex_dist_pixel [v]),
                    y + fypart(f_angle + sh->vertex_angle_float [v], sh->vertex_dist_pixel [v]),
                    fill_col);
     }
     finish_fan();
     return;
    }*/
    break;
/*


// Can be used for all shapes drawn as fans:
    for (v = 0; v < sh->vertices; v ++)
    {
     next_vertex = v + 1;
     if (next_vertex == sh->vertices)
      next_vertex = 0;
     add_triangle(x, y,
                  x + fxpart(f_angle + sh->vertex_angle_float [v], sh->vertex_dist_pixel [v] * INNER_SHAPE),
                  y + fypart(f_angle + sh->vertex_angle_float [v], sh->vertex_dist_pixel [v] * INNER_SHAPE),
                  x + fxpart(f_angle + sh->vertex_angle_float [next_vertex], sh->vertex_dist_pixel [next_vertex] * INNER_SHAPE),
                  y + fypart(f_angle + sh->vertex_angle_float [next_vertex], sh->vertex_dist_pixel [next_vertex] * INNER_SHAPE),
                  fill2_col);

    }*/
//    break;
 }


 float vertex_point [10] [4] [2];

 for (v = 0; v < sh->vertices; v ++)
 {

  f_angle = float_angle + sh->vertex_angle_float [v];

  vertex_point [v] [0] [0] = x + fxpart(f_angle, sh->vertex_dist_pixel [v] - POINT_INWARD);
  vertex_point [v] [0] [1] = y + fypart(f_angle, sh->vertex_dist_pixel [v] - POINT_INWARD);

  vertex_point [v] [1] [0] = x + fxpart(f_angle, sh->vertex_dist_pixel [v]) + fxpart(f_angle + fixed_to_radians(sh->external_angle [EXANGLE_NEXT] [v]), EDGE_INWARD);
  vertex_point [v] [1] [1] = y + fypart(f_angle, sh->vertex_dist_pixel [v]) + fypart(f_angle + fixed_to_radians(sh->external_angle [EXANGLE_NEXT] [v]), EDGE_INWARD);

  vertex_point [v] [2] [0] = x + fxpart(f_angle, sh->vertex_dist_pixel [v]) + fxpart(f_angle + fixed_to_radians(sh->external_angle [EXANGLE_PREVIOUS] [v]), EDGE_INWARD);
  vertex_point [v] [2] [1] = y + fypart(f_angle, sh->vertex_dist_pixel [v]) + fypart(f_angle + fixed_to_radians(sh->external_angle [EXANGLE_PREVIOUS] [v]), EDGE_INWARD);

// actual vertex:
  vertex_point [v] [3] [0] = x + fxpart(f_angle, sh->vertex_dist_pixel [v]);
  vertex_point [v] [3] [1] = y + fypart(f_angle, sh->vertex_dist_pixel [v]);

 }

 if (!start_fan(x, y, fill_col))
  return; // too many fans

 for (v = 0; v < sh->vertices; v ++)
 {
  next_vertex = v + 1;
  if (next_vertex == sh->vertices)
   next_vertex = 0;

  if (pr->vertex_method [v] == -1)
  {
   add_fan_vertex(vertex_point [v] [3] [0], vertex_point [v] [3] [1], fill_col);


  add_line(vertex_point [v] [3] [0],
           vertex_point [v] [3] [1],
           vertex_point [v] [2] [0],
           vertex_point [v] [2] [1],
           edge_col);

  add_line(vertex_point [v] [3] [0],
           vertex_point [v] [3] [1],
           vertex_point [next_vertex] [2] [0],
           vertex_point [next_vertex] [2] [1],
           edge_col);

   continue;

  }

   add_fan_vertex(vertex_point [v] [2] [0], vertex_point [v] [2] [1], fill_col);
   add_fan_vertex(vertex_point [v] [0] [0], vertex_point [v] [0] [1], fill_col);
   add_fan_vertex(vertex_point [v] [1] [0], vertex_point [v] [1] [1], fill_col);


  add_line(vertex_point [v] [0] [0],
           vertex_point [v] [0] [1],
           vertex_point [v] [1] [0],
           vertex_point [v] [1] [1],
           edge_col);


  add_line(vertex_point [v] [0] [0],
           vertex_point [v] [0] [1],
           vertex_point [v] [2] [0],
           vertex_point [v] [2] [1],
           edge_col);

  add_line(vertex_point [v] [1] [0],
           vertex_point [v] [1] [1],
           vertex_point [next_vertex] [2] [0],
           vertex_point [next_vertex] [2] [1],
           edge_col);

 }

  finish_fan();

  check_buffer_sizes();



}






/*

void add_proc_shape(struct procstruct* pr, float x, float y, float float_angle, struct shapestruct* sh, int shape, int size, ALLEGRO_COLOR fill_col, ALLEGRO_COLOR fill2_col, ALLEGRO_COLOR edge_col)
{

 float f_angle;

 int v = 0;
 int next_vertex;

 float inner_shape_separation = 0.8;

//#define INNER_SHAPE inner_shape_separation
/ *
 switch(size)
 {
  case 0: inner_shape_separation = 0.8; break;
  case 1: inner_shape_separation = 0.8; break;
  case 2: inner_shape_separation = 0.8; break;
  case 3: inner_shape_separation = 0.8; break;
 }* /

 f_angle = float_angle + sh->vertex_angle_float [0];

 switch(shape)
 {
  case SHAPE_DIAMOND:
// diamond can be drawn with 2 triangles instead of as a fan of 4:
   add_diamond(x + fxpart(f_angle + sh->vertex_angle_float [0], sh->vertex_dist_pixel [0] * INNER_SHAPE),
             y + fypart(f_angle + sh->vertex_angle_float [0], sh->vertex_dist_pixel [0] * INNER_SHAPE),
             x + fxpart(f_angle + sh->vertex_angle_float [1], sh->vertex_dist_pixel [1] * INNER_SHAPE),
             y + fypart(f_angle + sh->vertex_angle_float [1], sh->vertex_dist_pixel [1] * INNER_SHAPE),
             x + fxpart(f_angle + sh->vertex_angle_float [2], sh->vertex_dist_pixel [2] * INNER_SHAPE),
             y + fypart(f_angle + sh->vertex_angle_float [2], sh->vertex_dist_pixel [2] * INNER_SHAPE),
             x + fxpart(f_angle + sh->vertex_angle_float [3], sh->vertex_dist_pixel [3] * INNER_SHAPE),
             y + fypart(f_angle + sh->vertex_angle_float [3], sh->vertex_dist_pixel [3] * INNER_SHAPE),
             fill2_col);
    break;
   case SHAPE_OCTAGON:
// Can be used for all shapes drawn as fans:
    for (v = 0; v < sh->vertices; v ++)
    {
     next_vertex = v + 1;
     if (next_vertex == sh->vertices)
      next_vertex = 0;
     add_triangle(x, y,
                  x + fxpart(f_angle + sh->vertex_angle_float [v], sh->vertex_dist_pixel [v] * INNER_SHAPE),
                  y + fypart(f_angle + sh->vertex_angle_float [v], sh->vertex_dist_pixel [v] * INNER_SHAPE),
                  x + fxpart(f_angle + sh->vertex_angle_float [next_vertex], sh->vertex_dist_pixel [next_vertex] * INNER_SHAPE),
                  y + fypart(f_angle + sh->vertex_angle_float [next_vertex], sh->vertex_dist_pixel [next_vertex] * INNER_SHAPE),
                  fill2_col);

    }
    break;
 }


 float vertex_point [10] [4] [2];

 for (v = 0; v < sh->vertices; v ++)
 {

  f_angle = float_angle + sh->vertex_angle_float [v];

  vertex_point [v] [0] [0] = x + fxpart(f_angle, sh->vertex_dist_pixel [v] - POINT_INWARD);
  vertex_point [v] [0] [1] = y + fypart(f_angle, sh->vertex_dist_pixel [v] - POINT_INWARD);

  vertex_point [v] [1] [0] = x + fxpart(f_angle, sh->vertex_dist_pixel [v]) + fxpart(f_angle + fixed_to_radians(sh->external_angle [EXANGLE_NEXT] [v]), EDGE_INWARD);
  vertex_point [v] [1] [1] = y + fypart(f_angle, sh->vertex_dist_pixel [v]) + fypart(f_angle + fixed_to_radians(sh->external_angle [EXANGLE_NEXT] [v]), EDGE_INWARD);

  vertex_point [v] [2] [0] = x + fxpart(f_angle, sh->vertex_dist_pixel [v]) + fxpart(f_angle + fixed_to_radians(sh->external_angle [EXANGLE_PREVIOUS] [v]), EDGE_INWARD);
  vertex_point [v] [2] [1] = y + fypart(f_angle, sh->vertex_dist_pixel [v]) + fypart(f_angle + fixed_to_radians(sh->external_angle [EXANGLE_PREVIOUS] [v]), EDGE_INWARD);

// actual vertex:
  vertex_point [v] [3] [0] = x + fxpart(f_angle, sh->vertex_dist_pixel [v]);
  vertex_point [v] [3] [1] = y + fypart(f_angle, sh->vertex_dist_pixel [v]);

 }

 for (v = 0; v < sh->vertices; v ++)
 {
  next_vertex = v + 1;
  if (next_vertex == sh->vertices)
   next_vertex = 0;

  if (pr->vertex_method [v] == -1)
  {

   add_triangle(x, y,
                vertex_point [v] [3] [0],
                vertex_point [v] [3] [1],
                vertex_point [v] [2] [0],
                vertex_point [v] [2] [1],
                fill_col);

  add_line(vertex_point [v] [3] [0],
           vertex_point [v] [3] [1],
           vertex_point [v] [2] [0],
           vertex_point [v] [2] [1],
           edge_col);


  add_triangle(x, y,
               vertex_point [v] [3] [0],
               vertex_point [v] [3] [1],
               vertex_point [next_vertex] [2] [0],
               vertex_point [next_vertex] [2] [1],
               fill_col);

  add_line(vertex_point [v] [3] [0],
           vertex_point [v] [3] [1],
           vertex_point [next_vertex] [2] [0],
           vertex_point [next_vertex] [2] [1],
           edge_col);


   continue;

  }

  add_triangle(x, y,
               vertex_point [v] [0] [0],
               vertex_point [v] [0] [1],
               vertex_point [v] [1] [0],
               vertex_point [v] [1] [1],
               fill_col);

  add_line(vertex_point [v] [0] [0],
           vertex_point [v] [0] [1],
           vertex_point [v] [1] [0],
           vertex_point [v] [1] [1],
           edge_col);

  add_triangle(x, y,
               vertex_point [v] [0] [0],
               vertex_point [v] [0] [1],
               vertex_point [v] [2] [0],
               vertex_point [v] [2] [1],
               fill_col);

  add_line(vertex_point [v] [0] [0],
           vertex_point [v] [0] [1],
           vertex_point [v] [2] [0],
           vertex_point [v] [2] [1],
           edge_col);

  add_triangle(x, y,
               vertex_point [v] [1] [0],
               vertex_point [v] [1] [1],
               vertex_point [next_vertex] [2] [0],
               vertex_point [next_vertex] [2] [1],
               fill_col);

  add_line(vertex_point [v] [1] [0],
           vertex_point [v] [1] [1],
           vertex_point [next_vertex] [2] [0],
           vertex_point [next_vertex] [2] [1],
           edge_col);

 }

  check_buffer_sizes();



}

*/

void add_poly_vertex(float x, float y, ALLEGRO_COLOR col)
{

  poly_buffer [poly_pos].x = x;
  poly_buffer [poly_pos].y = y;
  poly_buffer [poly_pos].z = 0;
  poly_buffer [poly_pos].color = col;
  poly_pos ++;

}










void add_method_base_diamond(float point_x, float point_y, float f_angle, struct shapestruct* sh, int size, int vertex, ALLEGRO_COLOR fill_col, ALLEGRO_COLOR edge_col)
{

//#define NOTCH_SEPARATION (size + 1)
#define NOTCH_SEPARATION (size)

 point_x += cos(f_angle) * 3;
 point_y += sin(f_angle) * 3;

//#define NOTCH_SEPARATION (0)

// The order of the vertices is necessary for concave vertices:
              add_outline_diamond(point_x + cos(f_angle + fixed_to_radians(sh->external_angle [EXANGLE_HIGH] [vertex])) * (sh->vertex_notch_sidewards [vertex] - NOTCH_SEPARATION),
                          point_y + sin(f_angle + fixed_to_radians(sh->external_angle [EXANGLE_HIGH] [vertex])) * (sh->vertex_notch_sidewards [vertex] - NOTCH_SEPARATION),
																										point_x,
                          point_y,
                          point_x + cos(f_angle + fixed_to_radians(sh->external_angle [EXANGLE_LOW] [vertex])) * (sh->vertex_notch_sidewards [vertex] - NOTCH_SEPARATION),
                          point_y + sin(f_angle + fixed_to_radians(sh->external_angle [EXANGLE_LOW] [vertex])) * (sh->vertex_notch_sidewards [vertex] - NOTCH_SEPARATION),
                          point_x - cos(f_angle) * (sh->vertex_notch_inwards [vertex] - NOTCH_SEPARATION),
                          point_y - sin(f_angle) * (sh->vertex_notch_inwards [vertex] - NOTCH_SEPARATION),
                          fill_col,
                          edge_col);

/*
              add_outline_diamond(point_x,
                          point_y,
                          point_x + cos(f_angle + fixed_to_radians(sh->external_angle [EXANGLE_LOW] [vertex])) * (sh->vertex_notch_sidewards [vertex] - NOTCH_SEPARATION),
                          point_y + sin(f_angle + fixed_to_radians(sh->external_angle [EXANGLE_LOW] [vertex])) * (sh->vertex_notch_sidewards [vertex] - NOTCH_SEPARATION),
                          point_x - cos(f_angle) * (sh->vertex_notch_inwards [vertex] - NOTCH_SEPARATION),
                          point_y - sin(f_angle) * (sh->vertex_notch_inwards [vertex] - NOTCH_SEPARATION),
                          point_x + cos(f_angle + fixed_to_radians(sh->external_angle [EXANGLE_HIGH] [vertex])) * (sh->vertex_notch_sidewards [vertex] - NOTCH_SEPARATION),
                          point_y + sin(f_angle + fixed_to_radians(sh->external_angle [EXANGLE_HIGH] [vertex])) * (sh->vertex_notch_sidewards [vertex] - NOTCH_SEPARATION),
                          fill_col,
                          edge_col);
*/
}

void add_redundancy_lines(float x, float y, float float_angle, struct shapestruct* sh, ALLEGRO_COLOR line_col)
{

 int i;

#define LINE_MODIFY * 0.85
//#define LINE_MODIFY - 4

 for (i = 0; i < sh->vertices; i ++)
 {

  line_buffer [line_pos].x = x + fxpart(float_angle + sh->vertex_angle_float [i], sh->vertex_dist_pixel [i] LINE_MODIFY);
  line_buffer [line_pos].y = y + fypart(float_angle + sh->vertex_angle_float [i], sh->vertex_dist_pixel [i] LINE_MODIFY);
  line_buffer [line_pos].z = 0;
  line_buffer [line_pos].color = line_col;
  line_pos ++;

  line_buffer [line_pos].x = x + fxpart(float_angle + sh->vertex_angle_float [i + 1], sh->vertex_dist_pixel [i + 1] LINE_MODIFY);
  line_buffer [line_pos].y = y + fypart(float_angle + sh->vertex_angle_float [i + 1], sh->vertex_dist_pixel [i + 1] LINE_MODIFY);
  line_buffer [line_pos].z = 0;
  line_buffer [line_pos].color = line_col;
  line_pos ++;


 }

 check_buffer_sizes();

}


void add_outline_shape2(float x, float y, float float_angle, struct shapestruct* sh, ALLEGRO_COLOR line_col1, ALLEGRO_COLOR line_col2, ALLEGRO_COLOR line_col3, ALLEGRO_COLOR fill_col)
{

 int i;

 outline_buffer [outline_pos].vertex_start = outline_vertex_pos;
 outline_buffer [outline_pos].vertices = sh->vertices;
 outline_buffer [outline_pos].line_col [0] = line_col1;
 outline_buffer [outline_pos].line_col [1] = line_col2;
 outline_buffer [outline_pos].line_col [2] = line_col3;
 outline_pos ++;

 for (i = 0; i < sh->vertices; i ++)
 {
  poly_buffer [poly_pos].x = x;
  poly_buffer [poly_pos].y = y;
  poly_buffer [poly_pos].z = 0;
  poly_buffer [poly_pos].color = fill_col;
  poly_pos ++;
  poly_buffer [poly_pos].x = x + fxpart(float_angle + sh->vertex_angle_float [i], sh->vertex_dist_pixel [i]);
  poly_buffer [poly_pos].y = y + fypart(float_angle + sh->vertex_angle_float [i], sh->vertex_dist_pixel [i]);
  poly_buffer [poly_pos].z = 0;
  poly_buffer [poly_pos].color = fill_col;

  outline_vertex [outline_vertex_pos++] = poly_buffer [poly_pos].x;
  outline_vertex [outline_vertex_pos++] = poly_buffer [poly_pos].y;

  poly_pos ++;
//  outline_pos ++;
  poly_buffer [poly_pos].x = x + fxpart(float_angle + sh->vertex_angle_float [i + 1], sh->vertex_dist_pixel [i + 1]);
  poly_buffer [poly_pos].y = y + fypart(float_angle + sh->vertex_angle_float [i + 1], sh->vertex_dist_pixel [i + 1]);
  poly_buffer [poly_pos].z = 0;
  poly_buffer [poly_pos].color = fill_col;
  poly_pos ++;


 }

  outline_vertex [outline_vertex_pos++] = poly_buffer [poly_pos - 1].x;
  outline_vertex [outline_vertex_pos++] = poly_buffer [poly_pos - 1].y;


 check_buffer_sizes();


}


void add_scaled_outline_shape(struct shapestruct* sh, float float_angle, float x, float y, ALLEGRO_COLOR fill_col, ALLEGRO_COLOR edge_col, float scale)
{

 int i;
/*
 outline_buffer [outline_pos].vertex_start = outline_vertex_pos;
 outline_buffer [outline_pos].vertices = sh->vertices;
 outline_buffer [outline_pos].line_col [0] = line_col1;
 outline_buffer [outline_pos].line_col [1] = line_col2;
 outline_buffer [outline_pos].line_col [2] = line_col3;
 outline_pos ++;*/

 for (i = 0; i < sh->vertices; i ++)
 {
  layer_poly_buffer [2] [layer_poly_pos [2]].x = x;
  layer_poly_buffer [2] [layer_poly_pos [2]].y = y;
  layer_poly_buffer [2] [layer_poly_pos [2]].z = 0;
  layer_poly_buffer [2] [layer_poly_pos [2]].color = fill_col;
  layer_poly_pos [2] ++;
  layer_poly_buffer [2] [layer_poly_pos [2]].x = x + fxpart(float_angle + sh->vertex_angle_float [i], sh->vertex_dist_pixel [i] * scale);
  layer_poly_buffer [2] [layer_poly_pos [2]].y = y + fypart(float_angle + sh->vertex_angle_float [i], sh->vertex_dist_pixel [i] * scale);
  layer_poly_buffer [2] [layer_poly_pos [2]].z = 0;
  layer_poly_buffer [2] [layer_poly_pos [2]].color = fill_col;

  layer_line_buffer [2] [layer_line_pos [2]].x = layer_poly_buffer [2] [layer_poly_pos [2]].x;
  layer_line_buffer [2] [layer_line_pos [2]].y = layer_poly_buffer [2] [layer_poly_pos [2]].y;
  layer_line_buffer [2] [layer_line_pos [2]].z = 0;
  layer_line_buffer [2] [layer_line_pos [2]].color = edge_col;

  layer_poly_pos [2] ++;
  layer_line_pos [2] ++;

  layer_poly_buffer [2] [layer_poly_pos [2]].x = x + fxpart(float_angle + sh->vertex_angle_float [i + 1], sh->vertex_dist_pixel [i + 1] * scale);
  layer_poly_buffer [2] [layer_poly_pos [2]].y = y + fypart(float_angle + sh->vertex_angle_float [i + 1], sh->vertex_dist_pixel [i + 1] * scale);
  layer_poly_buffer [2] [layer_poly_pos [2]].z = 0;
  layer_poly_buffer [2] [layer_poly_pos [2]].color = fill_col;

  layer_line_buffer [2] [layer_line_pos [2]].x = layer_poly_buffer [2] [layer_poly_pos [2]].x;
  layer_line_buffer [2] [layer_line_pos [2]].y = layer_poly_buffer [2] [layer_poly_pos [2]].y;
  layer_line_buffer [2] [layer_line_pos [2]].z = 0;
  layer_line_buffer [2] [layer_line_pos [2]].color = edge_col;
  layer_line_pos [2] ++;

  layer_poly_pos [2] ++;


 }

//  outline_vertex [outline_vertex_pos++] = poly_buffer [poly_pos - 1].x;
//  outline_vertex [outline_vertex_pos++] = poly_buffer [poly_pos - 1].y;


 check_buffer_sizes();


}



void add_scaled_outline(struct shapestruct* sh, float float_angle, float x, float y, ALLEGRO_COLOR edge_col, float scale)
{

 int i;

 for (i = 0; i < sh->vertices; i ++)
 {

  layer_line_buffer [2] [layer_line_pos [2]].x = x + fxpart(float_angle + sh->vertex_angle_float [i], sh->vertex_dist_pixel [i] * scale);
  layer_line_buffer [2] [layer_line_pos [2]].y = y + fypart(float_angle + sh->vertex_angle_float [i], sh->vertex_dist_pixel [i] * scale);
  layer_line_buffer [2] [layer_line_pos [2]].z = 0;
  layer_line_buffer [2] [layer_line_pos [2]].color = edge_col;

  layer_line_pos [2] ++;

  layer_line_buffer [2] [layer_line_pos [2]].x = x + fxpart(float_angle + sh->vertex_angle_float [i + 1], sh->vertex_dist_pixel [i + 1] * scale);
  layer_line_buffer [2] [layer_line_pos [2]].y = y + fypart(float_angle + sh->vertex_angle_float [i + 1], sh->vertex_dist_pixel [i + 1] * scale);
  layer_line_buffer [2] [layer_line_pos [2]].z = 0;
  layer_line_buffer [2] [layer_line_pos [2]].color = edge_col;
  layer_line_pos [2] ++;


 }

 check_buffer_sizes();

}


void add_line(float x, float y, float xa, float ya, ALLEGRO_COLOR col)
{

  line_buffer [line_pos].x = x;
  line_buffer [line_pos].y = y;
  line_buffer [line_pos].z = 0;
  line_buffer [line_pos].color = col;
  line_pos ++;
  line_buffer [line_pos].x = xa;
  line_buffer [line_pos].y = ya;
  line_buffer [line_pos].z = 0;
  line_buffer [line_pos].color = col;
  line_pos ++;

 check_buffer_sizes();


}


void add_line_layer(int layer, float x, float y, float xa, float ya, ALLEGRO_COLOR col)
{

  layer_line_buffer [layer] [layer_line_pos [layer]].x = x;
  layer_line_buffer [layer] [layer_line_pos [layer]].y = y;
  layer_line_buffer [layer] [layer_line_pos [layer]].z = 0;
  layer_line_buffer [layer] [layer_line_pos [layer]].color = col;
  layer_line_pos [layer] ++;

  layer_line_buffer [layer] [layer_line_pos [layer]].x = xa;
  layer_line_buffer [layer] [layer_line_pos [layer]].y = ya;
  layer_line_buffer [layer] [layer_line_pos [layer]].z = 0;
  layer_line_buffer [layer] [layer_line_pos [layer]].color = col;
  layer_line_pos [layer] ++;

 check_buffer_sizes();


}

void add_rect_layer(int layer, float x, float y, float xa, float ya, ALLEGRO_COLOR col)
{

  layer_line_buffer [layer] [layer_line_pos [layer]].x = x;
  layer_line_buffer [layer] [layer_line_pos [layer]].y = y;
  layer_line_buffer [layer] [layer_line_pos [layer]].z = 0;
  layer_line_buffer [layer] [layer_line_pos [layer]].color = col;
  layer_line_pos [layer] ++;

  layer_line_buffer [layer] [layer_line_pos [layer]].x = xa;
  layer_line_buffer [layer] [layer_line_pos [layer]].y = y;
  layer_line_buffer [layer] [layer_line_pos [layer]].z = 0;
  layer_line_buffer [layer] [layer_line_pos [layer]].color = col;
  layer_line_pos [layer] ++;

  layer_line_buffer [layer] [layer_line_pos [layer]].x = xa;
  layer_line_buffer [layer] [layer_line_pos [layer]].y = y;
  layer_line_buffer [layer] [layer_line_pos [layer]].z = 0;
  layer_line_buffer [layer] [layer_line_pos [layer]].color = col;
  layer_line_pos [layer] ++;

  layer_line_buffer [layer] [layer_line_pos [layer]].x = xa;
  layer_line_buffer [layer] [layer_line_pos [layer]].y = ya;
  layer_line_buffer [layer] [layer_line_pos [layer]].z = 0;
  layer_line_buffer [layer] [layer_line_pos [layer]].color = col;
  layer_line_pos [layer] ++;

  layer_line_buffer [layer] [layer_line_pos [layer]].x = xa;
  layer_line_buffer [layer] [layer_line_pos [layer]].y = ya;
  layer_line_buffer [layer] [layer_line_pos [layer]].z = 0;
  layer_line_buffer [layer] [layer_line_pos [layer]].color = col;
  layer_line_pos [layer] ++;

  layer_line_buffer [layer] [layer_line_pos [layer]].x = x;
  layer_line_buffer [layer] [layer_line_pos [layer]].y = ya;
  layer_line_buffer [layer] [layer_line_pos [layer]].z = 0;
  layer_line_buffer [layer] [layer_line_pos [layer]].color = col;
  layer_line_pos [layer] ++;

  layer_line_buffer [layer] [layer_line_pos [layer]].x = x;
  layer_line_buffer [layer] [layer_line_pos [layer]].y = ya;
  layer_line_buffer [layer] [layer_line_pos [layer]].z = 0;
  layer_line_buffer [layer] [layer_line_pos [layer]].color = col;
  layer_line_pos [layer] ++;

  layer_line_buffer [layer] [layer_line_pos [layer]].x = x;
  layer_line_buffer [layer] [layer_line_pos [layer]].y = y;
  layer_line_buffer [layer] [layer_line_pos [layer]].z = 0;
  layer_line_buffer [layer] [layer_line_pos [layer]].color = col;
  layer_line_pos [layer] ++;


 check_buffer_sizes();


}



void push_to_poly_buffer(int v, ALLEGRO_COLOR col)
{

 int i;

 for (i = 0; i < v; i ++)
 {
  poly_buffer [poly_pos].x = vertex_list [i] [0];
  poly_buffer [poly_pos].y = vertex_list [i] [1];
  poly_buffer [poly_pos].z = 0;
  poly_buffer [poly_pos].color = col;
  poly_pos ++;
 }

}

void push_loop_to_line_buffer(int v, ALLEGRO_COLOR col)
{

 int i;

 vertex_list [v] [0] = vertex_list [0] [0];
 vertex_list [v] [1] = vertex_list [0] [1];

 for (i = 0; i < v; i ++)
 {
  line_buffer [line_pos].x = vertex_list [i] [0];
  line_buffer [line_pos].y = vertex_list [i] [1];
  line_buffer [line_pos].z = 0;
  line_buffer [line_pos].color = col;
  line_pos ++;
  line_buffer [line_pos].x = vertex_list [i + 1] [0];
  line_buffer [line_pos].y = vertex_list [i + 1] [1];
  line_buffer [line_pos].z = 0;
  line_buffer [line_pos].color = col;
  line_pos ++;
 }

}


void push_to_layer_poly_buffer(int layer, int v, ALLEGRO_COLOR col)
{

 int i;

 for (i = 0; i < v; i ++)
 {
  layer_poly_buffer [layer] [layer_poly_pos [layer]].x = vertex_list [i] [0];
  layer_poly_buffer [layer] [layer_poly_pos [layer]].y = vertex_list [i] [1];
  layer_poly_buffer [layer] [layer_poly_pos [layer]].z = 0;
  layer_poly_buffer [layer] [layer_poly_pos [layer]].color = col;
  layer_poly_pos [layer] ++;
 }

}


void push_loop_to_layer_line_buffer(int layer, int v, ALLEGRO_COLOR col)
{

 int i;

 vertex_list [v] [0] = vertex_list [0] [0];
 vertex_list [v] [1] = vertex_list [0] [1];

 for (i = 0; i < v; i ++)
 {
  layer_line_buffer [layer] [layer_line_pos [layer]].x = vertex_list [i] [0];
  layer_line_buffer [layer] [layer_line_pos [layer]].y = vertex_list [i] [1];
  layer_line_buffer [layer] [layer_line_pos [layer]].z = 0;
  layer_line_buffer [layer] [layer_line_pos [layer]].color = col;
  layer_line_pos [layer] ++;
  layer_line_buffer [layer] [layer_line_pos [layer]].x = vertex_list [i + 1] [0];
  layer_line_buffer [layer] [layer_line_pos [layer]].y = vertex_list [i + 1] [1];
  layer_line_buffer [layer] [layer_line_pos [layer]].z = 0;
  layer_line_buffer [layer] [layer_line_pos [layer]].color = col;
  layer_line_pos [layer] ++;
 }

}

void check_buffer_sizes(void)
{

 if (poly_pos > POLY_TRIGGER
  || line_pos > LINE_TRIGGER
  || layer_poly_pos [0] > LAYER_POLY_TRIGGER
  || layer_poly_pos [1] > LAYER_POLY_TRIGGER
  || layer_poly_pos [2] > LAYER_POLY_TRIGGER
  || layer_line_pos [0] > LAYER_LINE_TRIGGER
  || layer_line_pos [1] > LAYER_LINE_TRIGGER
  || layer_line_pos [2] > LAYER_LINE_TRIGGER
  || outline_pos > OUTLINE_TRIGGER
  || outline_vertex_pos > OUTLINE_VERTEX_TRIGGER)
//  || fan_index_pos >= FAN_INDEX_TRIGGER this function doesn't deal with fans
  //|| fan_buffer_pos >= FAN_BUFFER_TRIGGER)
 {

  draw_from_buffers();
 }

}

// Doesn't draw fans
void draw_from_buffers(void)
{

/*
 	fprintf(stdout, "\nA %i:%i %i:%i %i:%i %i:%i %i:%i %i:%i %i:%i %i:%i %i:%i %i:%i %i:%i %i:%i ",
											poly_pos, POLY_TRIGGER,
  line_pos, LINE_TRIGGER,
  layer_poly_pos [0],  LAYER_POLY_TRIGGER,
  layer_poly_pos [1],  LAYER_POLY_TRIGGER,
  layer_poly_pos [2], LAYER_POLY_TRIGGER,
  layer_line_pos [0], LAYER_LINE_TRIGGER,
  layer_line_pos [1], LAYER_LINE_TRIGGER,
  layer_line_pos [2], LAYER_LINE_TRIGGER,
  outline_pos, OUTLINE_TRIGGER,
  outline_vertex_pos,  OUTLINE_VERTEX_TRIGGER,
  fan_index_pos, FAN_INDEX_TRIGGER,
  fan_buffer_pos, FAN_BUFFER_TRIGGER);
*/

  if (poly_pos > 0)
  {
   al_draw_prim(poly_buffer, NULL, NULL, 0, poly_pos, ALLEGRO_PRIM_TRIANGLE_LIST);
   poly_pos = 0;
  }



  if (outline_pos > 0)
  {
   int i, j;
   int v;
   for (i = 0; i < outline_pos; i ++)
   {
    v = outline_buffer[i].vertex_start;
    for (j = 0; j < outline_buffer[i].vertices; j ++)
    {

//     al_draw_line(outline_vertex [v], outline_vertex [v + 1], outline_vertex [v + 2], outline_vertex [v + 3],
//      outline_buffer [i].line_col [1], 5);
//     al_draw_line(outline_vertex [v], outline_vertex [v + 1], outline_vertex [v + 2], outline_vertex [v + 3],
//      outline_buffer [i].line_col [2], 3);
     al_draw_line(outline_vertex [v], outline_vertex [v + 1], outline_vertex [v + 2], outline_vertex [v + 3],
      outline_buffer [i].line_col [1], 2);
     al_draw_line(outline_vertex [v], outline_vertex [v + 1], outline_vertex [v + 2], outline_vertex [v + 3],
      outline_buffer [i].line_col [0], 0);
     v += 2;
    }
   }
   outline_pos = 0;
  }


  if (line_pos > 0)
  {
   al_draw_prim(line_buffer, NULL, NULL, 0, line_pos, ALLEGRO_PRIM_LINE_LIST);
   line_pos = 0;
  }

  if (layer_poly_pos [0] > 0)
  {
   al_draw_prim(layer_poly_buffer [0], NULL, NULL, 0, layer_poly_pos [0], ALLEGRO_PRIM_TRIANGLE_LIST);
   layer_poly_pos [0] = 0;
  }

  if (layer_line_pos [0] > 0)
  {
   al_draw_prim(layer_line_buffer [0], NULL, NULL, 0, layer_line_pos [0], ALLEGRO_PRIM_LINE_LIST);
   layer_line_pos [0] = 0;
  }

  if (layer_poly_pos [1] > 0)
  {
   al_draw_prim(layer_poly_buffer [1], NULL, NULL, 0, layer_poly_pos [1], ALLEGRO_PRIM_TRIANGLE_LIST);
   layer_poly_pos [1] = 0;
  }

  if (layer_line_pos [1] > 0)
  {
   al_draw_prim(layer_line_buffer [1], NULL, NULL, 0, layer_line_pos [1], ALLEGRO_PRIM_LINE_LIST);
   layer_line_pos [1] = 0;
  }

  if (layer_poly_pos [2] > 0)
  {
   al_draw_prim(layer_poly_buffer [2], NULL, NULL, 0, layer_poly_pos [2], ALLEGRO_PRIM_TRIANGLE_LIST);
   layer_poly_pos [2] = 0;
  }

  if (layer_line_pos [2] > 0)
  {
   al_draw_prim(layer_line_buffer [2], NULL, NULL, 0, layer_line_pos [2], ALLEGRO_PRIM_LINE_LIST);
   layer_line_pos [2] = 0;
  }

/*

Unfortunately I wrote the following code before I realised that al_draw_polygon isn't in the current stable version of Allegro 5, which is what I'm using.
So nevermind.

    al_draw_polygon(&outline_vertex [outline_buffer [i].vertex_start], outline_buffer [i].vertices,
     ALLEGRO_LINE_JOIN_ROUND, outline_buffer [i].line_col [2], 4, 0);
    al_draw_polygon(&outline_vertex [outline_buffer [i].vertex_start], outline_buffer [i].vertices,
     ALLEGRO_LINE_JOIN_ROUND, outline_buffer [i].line_col [1], 2, 0);
    al_draw_polygon(&outline_vertex [outline_buffer [i].vertex_start], outline_buffer [i].vertices,
     ALLEGRO_LINE_JOIN_ROUND, outline_buffer [i].line_col [0], 0, 0);*/

/*
    al_draw_line(outline_buffer [i].x, outline_buffer [i].y, outline_buffer [i+1].x, outline_buffer [i+1].y,
      outline_buffer [i].line_col [2], 4);
    al_draw_line(outline_buffer [i].x, outline_buffer [i].y, outline_buffer [i+1].x, outline_buffer [i+1].y,
      outline_buffer [i].line_col [1], 2);
    al_draw_line(outline_buffer [i].x, outline_buffer [i].y, outline_buffer [i+1].x, outline_buffer [i+1].y,
      outline_buffer [i].line_col [0], 0);*/

/*
  if (line_pos > 0)
  {
   int i;
   for (i = 0; i < line_pos; i += 2) // note += 2
   {
//    al_draw_line(line_buffer [i].x, line_buffer [i].y, line_buffer [i+1].x, line_buffer [i+1].y,
//      cloud_col [1] [4], 3);
    al_draw_line(line_buffer [i].x, line_buffer [i].y, line_buffer [i+1].x, line_buffer [i+1].y,
      cloud_col [1] [6], 2);
//    al_draw_line(line_buffer [i].x, line_buffer [i].y, line_buffer [i+1].x, line_buffer [i+1].y,
//      cloud_col [1] [9], 0);
//      line_buffer [i].color, 2);
   }


//   al_draw_prim(line_buffer, NULL, NULL, 0, line_pos, ALLEGRO_PRIM_LINE_LIST);
   line_pos = 0;
  }*/

  line_pos = 0;
  outline_pos = 0;
  outline_vertex_pos = 0;
  poly_pos = 0;

}


#define MAP_VERTICES 1000
#define MAP_W view.map_w
#define MAP_H view.map_h
/*
Can't use any of the drawing buffers except the basic line buffer
(to change this, add tests for other buffers to the end of this function)
*/
void draw_map(void)
{

 if (view.map_visible == 0)
  return;

 int i;
 int p;
 float map_base_x = view.map_x;//view.window_x - view.map_w - 50;
 float map_base_y = view.map_y;//view.window_y - view.map_h - 50;

 al_draw_filled_rectangle(map_base_x, map_base_y, map_base_x + MAP_W, map_base_y + MAP_H, colours.base [COL_GREY] [SHADE_MIN]);
 al_draw_rectangle(map_base_x, map_base_y, map_base_x + MAP_W, map_base_y + MAP_H, colours.base [COL_GREY] [SHADE_MED], 1);

 ALLEGRO_VERTEX map_pixel [MAP_VERTICES];

 i = 0;

 struct procstruct* pr;

 float x = al_fixtof(al_fixmul(view.camera_x, view.map_proportion_x));
 float y = al_fixtof(al_fixmul(view.camera_y, view.map_proportion_y));
 float box_size_x = al_fixtof(view.map_proportion_x) * view.window_x;
 float box_size_y = al_fixtof(view.map_proportion_y) * view.window_y;
 float xa, ya;

 al_draw_rectangle(map_base_x + x - box_size_x/2, map_base_y + y - box_size_x/2, map_base_x + x + box_size_x/2, map_base_y + y + box_size_y/2,
   colours.base [COL_GREY] [SHADE_HIGH], 1);

 for (p = 0; p < w.max_procs; p ++)
 {
  pr = &w.proc [p];
  if (pr->exists <= 0)
   continue;

  map_pixel[i].x = map_base_x + al_fixtof(al_fixmul(pr->x, view.map_proportion_x));
  map_pixel[i].y = map_base_y + al_fixtof(al_fixmul(pr->y, view.map_proportion_y));
  map_pixel[i].z = 0;
  map_pixel[i].color = colours.team [pr->player_index] [TCOL_MAP_POINT];

  i ++;

  if (i == MAP_VERTICES)
  {
   al_draw_prim(map_pixel, NULL, NULL, 0, MAP_VERTICES, ALLEGRO_PRIM_POINT_LIST); // May need to put back "-1" after MAP_VERTICES
   i = 0;
  }

 }

// i retains its value here so it can be used for assembling the map selection list:

 int j;

// now draw map selection:
 for (p = 0; p < w.max_procs; p ++)
 {
  pr = &w.proc [p];
  if (pr->exists <= 0
   || pr->map_selected == 0)
   continue;

  x = map_base_x + al_fixtof(al_fixmul(pr->x, view.map_proportion_x));
  y = map_base_y + al_fixtof(al_fixmul(pr->y, view.map_proportion_y));


  for (j = -1; j < 2; j ++)
  {
   map_pixel[i].x = x - 2;
   map_pixel[i].y = y + j;
   map_pixel[i].z = 0;
   map_pixel[i].color = colours.base [COL_GREY] [SHADE_MAX];

   i ++;

   if (i == MAP_VERTICES)
   {
    al_draw_prim(map_pixel, NULL, NULL, 0, MAP_VERTICES, ALLEGRO_PRIM_POINT_LIST); // May need to put back "-1" after MAP_VERTICES
    i = 0;
   }

   map_pixel[i].x = x + 2;
   map_pixel[i].y = y + j;
   map_pixel[i].z = 0;
   map_pixel[i].color = colours.base [COL_GREY] [SHADE_MAX];

   i ++;

   if (i == MAP_VERTICES)
   {
    al_draw_prim(map_pixel, NULL, NULL, 0, MAP_VERTICES, ALLEGRO_PRIM_POINT_LIST); // May need to put back "-1" after MAP_VERTICES
    i = 0;
   }

  }

 }


// finally draw markers:

// i still retains its value:

 int m;

// now draw map selection:
 for (m = 0; m < MARKERS; m ++)
 {
   if (w.marker[m].active == 0
    || w.marker[m].type < MARKER_MAP_POINT)
    continue;

   if (i >= MAP_VERTICES - 4)
   {
    al_draw_prim(map_pixel, NULL, NULL, 0, MAP_VERTICES, ALLEGRO_PRIM_POINT_LIST); // May need to put back "-1" after MAP_VERTICES
    i = 0;
   }

   x = map_base_x + al_fixtof(al_fixmul(w.marker[m].x, view.map_proportion_x));
   y = map_base_y + al_fixtof(al_fixmul(w.marker[m].y, view.map_proportion_y));

   switch(w.marker[m].type)
   {

   	case MARKER_MAP_AROUND_1: // + shape
     map_pixel[i].x = x - 1; map_pixel[i].y = y; map_pixel[i].z = 0; map_pixel[i].color = colours.base [w.marker[m].colour] [SHADE_MAX];
     i ++;
     map_pixel[i].x = x + 1; map_pixel[i].y = y; map_pixel[i].z = 0; map_pixel[i].color = colours.base [w.marker[m].colour] [SHADE_MAX];
     i ++;
     map_pixel[i].x = x; map_pixel[i].y = y - 1; map_pixel[i].z = 0; map_pixel[i].color = colours.base [w.marker[m].colour] [SHADE_MAX];
     i ++;
     map_pixel[i].x = x; map_pixel[i].y = y + 1; map_pixel[i].z = 0; map_pixel[i].color = colours.base [w.marker[m].colour] [SHADE_MAX];
     i ++;
     break;

   	case MARKER_MAP_AROUND_2: // x shape
     map_pixel[i].x = x - 1; map_pixel[i].y = y - 1; map_pixel[i].z = 0; map_pixel[i].color = colours.base [w.marker[m].colour] [SHADE_MAX];
     i ++;
     map_pixel[i].x = x + 1; map_pixel[i].y = y - 1; map_pixel[i].z = 0; map_pixel[i].color = colours.base [w.marker[m].colour] [SHADE_MAX];
     i ++;
     map_pixel[i].x = x - 1; map_pixel[i].y = y + 1; map_pixel[i].z = 0; map_pixel[i].color = colours.base [w.marker[m].colour] [SHADE_MAX];
     i ++;
     map_pixel[i].x = x + 1; map_pixel[i].y = y + 1; map_pixel[i].z = 0; map_pixel[i].color = colours.base [w.marker[m].colour] [SHADE_MAX];
     i ++;
     break;

    case MARKER_MAP_POINT:
     map_pixel[i].x = x; map_pixel[i].y = y; map_pixel[i].z = 0; map_pixel[i].color = colours.base [w.marker[m].colour] [SHADE_MAX];
     i ++;
     break;

    case MARKER_MAP_CROSS:
    	if (x - map_base_x >= 0
						&& x + map_base_x <= MAP_W)
     	add_line(x, map_base_y, x, map_base_y + MAP_H, colours.base [w.marker[m].colour] [SHADE_MAX]);
     if (y - map_base_y >= 0
					 && y + map_base_y <= MAP_H)
    	 add_line(map_base_x, y, map_base_x + MAP_W, y, colours.base [w.marker[m].colour] [SHADE_MAX]);
     break;

    case MARKER_MAP_LINE:
    	xa = map_base_x + al_fixtof(al_fixmul(w.marker[m].x2, view.map_proportion_x));
    	ya = map_base_y + al_fixtof(al_fixmul(w.marker[m].y2, view.map_proportion_y));

    	if (x < map_base_x)
						x = map_base_x;
					if (x > map_base_x + MAP_W)
						 x = map_base_x + MAP_W;
    	if (y < map_base_y)
						y = map_base_y;
					if (y > map_base_y + MAP_H)
						 y = map_base_y + MAP_H;

    	if (xa < map_base_x)
						xa = map_base_x;
					if (xa > map_base_x + MAP_W)
						 xa = map_base_x + MAP_W;
    	if (ya < map_base_y)
						ya = map_base_y;
					if (ya > map_base_y + MAP_H)
						 ya = map_base_y + MAP_H;
    	add_line(x, y, xa, ya, colours.base [w.marker[m].colour] [SHADE_LOW]);
     break;

    case MARKER_MAP_BOX:
    	xa = map_base_x + al_fixtof(al_fixmul(w.marker[m].x2, view.map_proportion_x));
    	ya = map_base_y + al_fixtof(al_fixmul(w.marker[m].y2, view.map_proportion_y));

    	add_line(x, y, x, ya, colours.base [w.marker[m].colour] [SHADE_LOW]);
    	add_line(x, ya, xa, ya, colours.base [w.marker[m].colour] [SHADE_LOW]);
    	add_line(xa, ya, xa, y, colours.base [w.marker[m].colour] [SHADE_LOW]);
    	add_line(xa, y, x, y, colours.base [w.marker[m].colour] [SHADE_LOW]);

					break;

  } // end marker type switch
 } // end marker loop



 if (i > 0)
 {
   al_draw_prim(map_pixel, NULL, NULL, 0, i, ALLEGRO_PRIM_POINT_LIST);
 }

// currently only the basic line buffer is checked here:
 if (line_pos > 0)
 {
  al_draw_prim(line_buffer, NULL, NULL, 0, line_pos, ALLEGRO_PRIM_LINE_LIST);
  line_pos = 0;
 }


}

static void reset_fan_index(void)
{
 fan_index[0].start_position = -1;
 fan_index_pos = 0;
 fan_buffer_pos = 0;
}

static int start_fan(float x, float y, ALLEGRO_COLOR col)
{
 if (fan_index_pos >= FAN_INDEX_TRIGGER
  || fan_buffer_pos >= FAN_BUFFER_TRIGGER)
  {
  return 0; // just fail if there are too many fans
  }
 fan_index[fan_index_pos].start_position = fan_buffer_pos;
 fan_buffer[fan_buffer_pos].x = x;
 fan_buffer[fan_buffer_pos].y = y;
 fan_buffer[fan_buffer_pos].z = 0;
 fan_buffer[fan_buffer_pos].color = col;
 fan_buffer_pos ++;
 return 1;
}

static void add_fan_vertex(float x, float y, ALLEGRO_COLOR col)
{
 fan_buffer[fan_buffer_pos].x = x;
 fan_buffer[fan_buffer_pos].y = y;
 fan_buffer[fan_buffer_pos].z = 0;
 fan_buffer[fan_buffer_pos].color = col;
 fan_buffer_pos ++;
}

static void finish_fan(void)
{
 add_fan_vertex(fan_buffer[fan_index[fan_index_pos].start_position + 1].x,
                fan_buffer[fan_index[fan_index_pos].start_position + 1].y,
                fan_buffer[fan_index[fan_index_pos].start_position + 1].color);
 fan_index[fan_index_pos].vertices = fan_buffer_pos - fan_index[fan_index_pos].start_position;

 fan_index_pos ++;
 fan_index[fan_index_pos].start_position = -1;

}

// finishes a fan without closing it by linking the last vertex to the first
static void finish_fan_open(void)
{
 fan_index[fan_index_pos].vertices = fan_buffer_pos - fan_index[fan_index_pos].start_position;

 fan_index_pos ++;
 fan_index[fan_index_pos].start_position = -1;

}


#define FAN_VERTICES_MAX 64
static void draw_fans(void)
{

 int fan_vertex_list [FAN_VERTICES_MAX];
 int i, j;

 i = 0;

 while(fan_index[i].start_position != -1)
 {

  for (j = 0; j < fan_index[i].vertices; j ++)
  {
   fan_vertex_list [j] = fan_index[i].start_position + j;
  }

  al_draw_indexed_prim(fan_buffer, NULL, NULL, fan_vertex_list, fan_index[i].vertices, ALLEGRO_PRIM_TRIANGLE_FAN);
  i++;
 };

 reset_fan_index();

}


void draw_mouse_cursor(void)
{
 float x = ex_control.mouse_x_pixels;
 float y = ex_control.mouse_y_pixels;

 al_draw_line(x, y, x + 7, y, colours.base [COL_GREY] [SHADE_MAX], 2);
 al_draw_line(x, y, x, y + 7, colours.base [COL_GREY] [SHADE_MAX], 2);

}


//#define DRAW_SHAPE_DATA

#ifdef DRAW_SHAPE_DATA

void draw_a_proc_shape_data(int s, float x, float y);

// This generates a special image for the manual (so a screenshot can be taken of it) then pretends to have an error and exits.
void draw_proc_shape_data(void)
{

 al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP);
 ALLEGRO_BITMAP* shape_bmp = al_create_bitmap(1000, 3000);
 if (!shape_bmp)
	{
		fprintf(stdout, "\nError: shape_bmp not created.");
		error_call();
	}
 al_set_target_bitmap(shape_bmp);

// al_set_target_bitmap(al_get_backbuffer(display));
 al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA);
 al_clear_to_color(colours.base [COL_BLUE] [SHADE_MIN]);

 reset_fan_index();

 int s;
#define DATA_BASE_X 140
 float x = DATA_BASE_X;
 float y = 100;

 for (s = 0; s < SHAPES; s ++)
 {
  draw_a_proc_shape_data(s, x, y);
  x += 240;
  if (x > 1000
			|| s == SHAPE_4SQUARE - 1
			|| s == SHAPE_5PENTAGON - 1
			|| s == SHAPE_6HEXAGON - 1
			|| s == SHAPE_8OCTAGON - 1)
  {
   y += 300;
   x = DATA_BASE_X;
  }
 }

 draw_fans();
 draw_from_buffers();

 al_save_bitmap("shape_test.bmp", shape_bmp);
/*
 if (settings.option [OPTION_SPECIAL_CURSOR])
  draw_mouse_cursor();
 al_flip_display();
*/
 error_call();

}

#define DRAW_TEAM 0
#define DRAW_SIZE 1
#define DRAW_LINE 12

void draw_a_proc_shape_data(int s, float x, float y)
{

 struct procstruct draw_pr; // this just needs to be initialised to the extent that add_proc_shape() uses
 struct shapestruct* sh = &shape_dat [s] [DRAW_SIZE];
 float f_angle = -PI/2;

 int i;

 for (i = 0; i < SHAPES_VERTICES; i ++)
 {
  draw_pr.vertex_method [i] = -1;
 }

 add_proc_shape(&draw_pr, x, y, f_angle, sh, s, DRAW_SIZE,
                colours.proc_fill [DRAW_TEAM] [PROC_FILL_SHADES - 1] [0],
                colours.team [DRAW_TEAM] [TCOL_FILL_BASE],
                colours.team [DRAW_TEAM] [TCOL_MAIN_EDGE]);

 float vx, vy;

 for (i = 0; i < sh->vertices; i ++)
 {
  vx = x + fxpart(f_angle + sh->vertex_angle_float [i], sh->vertex_dist_pixel [i] + 10);
  vy = y + fypart(f_angle + sh->vertex_angle_float [i], sh->vertex_dist_pixel [i] + 10);
  al_draw_textf(font[FONT_SQUARE].fnt, colours.base [COL_GREY] [SHADE_HIGH], vx, vy - 4, ALLEGRO_ALIGN_CENTRE, "%i", i);
 }

 char sh_str [30] = "nothing";

 switch(s)
 {
  case SHAPE_3TRIANGLE: strcpy(sh_str, "SHAPE_3TRIANGLE"); break;
  case SHAPE_4SQUARE: strcpy(sh_str, "SHAPE_4SQUARE"); break;
  case SHAPE_4DIAMOND: strcpy(sh_str, "SHAPE_4DIAMOND"); break;
  case SHAPE_4POINTY: strcpy(sh_str, "SHAPE_4POINTY"); break;
  case SHAPE_4TRAP: strcpy(sh_str, "SHAPE_4TRAP"); break;
  case SHAPE_4IRREG_L: strcpy(sh_str, "SHAPE_4IRREG_L"); break;
  case SHAPE_4IRREG_R: strcpy(sh_str, "SHAPE_4IRREG_R"); break;
  case SHAPE_4ARROW: strcpy(sh_str, "SHAPE_4ARROW"); break;
  case SHAPE_5PENTAGON: strcpy(sh_str, "SHAPE_5PENTAGON"); break;
  case SHAPE_5POINTY: strcpy(sh_str, "SHAPE_5POINTY"); break;
  case SHAPE_5LONG: strcpy(sh_str, "SHAPE_5LONG"); break;
  case SHAPE_5WIDE: strcpy(sh_str, "SHAPE_5WIDE"); break;
  case SHAPE_6HEXAGON: strcpy(sh_str, "SHAPE_6HEXAGON"); break;
  case SHAPE_6POINTY: strcpy(sh_str, "SHAPE_6POINTY"); break;
  case SHAPE_6LONG: strcpy(sh_str, "SHAPE_6LONG"); break;
  case SHAPE_6IRREG_L: strcpy(sh_str, "SHAPE_6IRREG_L"); break;
  case SHAPE_6IRREG_R: strcpy(sh_str, "SHAPE_6IRREG_R"); break;
  case SHAPE_6ARROW: strcpy(sh_str, "SHAPE_6ARROW"); break;
  case SHAPE_6STAR: strcpy(sh_str, "SHAPE_6STAR"); break;
  case SHAPE_8OCTAGON: strcpy(sh_str, "SHAPE_8OCTAGON"); break;
  case SHAPE_8POINTY: strcpy(sh_str, "SHAPE_8POINTY"); break;
  case SHAPE_8LONG: strcpy(sh_str, "SHAPE_8LONG"); break;
  case SHAPE_8STAR: strcpy(sh_str, "SHAPE_8STAR"); break;
 }




#define DRAW_X_COL 30
#define DRAW_X_COL_NAME 30

 float line_y = y + 80;
 al_draw_textf(font[FONT_SQUARE].fnt, colours.base [COL_GREY] [SHADE_HIGH], x, line_y, ALLEGRO_ALIGN_CENTRE, "%s", sh_str);
 line_y += DRAW_LINE + 10;
 al_draw_textf(font[FONT_SQUARE].fnt, colours.base [COL_GREY] [SHADE_HIGH], x - DRAW_X_COL_NAME, line_y, ALLEGRO_ALIGN_RIGHT, "SIZE");
 for (i = 0; i < SHAPES_SIZES; i ++)
 {
  al_draw_textf(font[FONT_SQUARE].fnt, colours.base [COL_GREY] [SHADE_HIGH], x + i * DRAW_X_COL, line_y, ALLEGRO_ALIGN_RIGHT, "%i", i);
 }
 line_y += DRAW_LINE;
 al_draw_textf(font[FONT_SQUARE].fnt, colours.base [COL_GREY] [SHADE_HIGH], x - DRAW_X_COL_NAME, line_y, ALLEGRO_ALIGN_RIGHT, "base mass");
 for (i = 0; i < SHAPES_SIZES; i ++)
 {
  al_draw_textf(font[FONT_SQUARE].fnt, colours.base [COL_GREY] [SHADE_MED], x + i * DRAW_X_COL, line_y, ALLEGRO_ALIGN_RIGHT, "%i", shape_dat [s] [i].shape_mass);
 }
 line_y += DRAW_LINE;
 al_draw_textf(font[FONT_SQUARE].fnt, colours.base [COL_GREY] [SHADE_HIGH], x - DRAW_X_COL_NAME, line_y, ALLEGRO_ALIGN_RIGHT, "max method mass");
 for (i = 0; i < SHAPES_SIZES; i ++)
 {
  al_draw_textf(font[FONT_SQUARE].fnt, colours.base [COL_GREY] [SHADE_MED], x + i * DRAW_X_COL, line_y, ALLEGRO_ALIGN_RIGHT, "%i", shape_dat [s] [i].mass_max - shape_dat [s] [i].shape_mass);
 }
 line_y += DRAW_LINE;
 al_draw_textf(font[FONT_SQUARE].fnt, colours.base [COL_GREY] [SHADE_HIGH], x - DRAW_X_COL_NAME, line_y, ALLEGRO_ALIGN_RIGHT, "max hp");
 for (i = 0; i < SHAPES_SIZES; i ++)
 {
  al_draw_textf(font[FONT_SQUARE].fnt, colours.base [COL_GREY] [SHADE_MED], x + i * DRAW_X_COL, line_y, ALLEGRO_ALIGN_RIGHT, "%i", shape_dat [s] [i].base_hp_max);
 }
 line_y += DRAW_LINE;
 al_draw_textf(font[FONT_SQUARE].fnt, colours.base [COL_GREY] [SHADE_HIGH], x - DRAW_X_COL_NAME, line_y, ALLEGRO_ALIGN_RIGHT, "irpt buffer");
 for (i = 0; i < SHAPES_SIZES; i ++)
 {
  al_draw_textf(font[FONT_SQUARE].fnt, colours.base [COL_GREY] [SHADE_MED], x + i * DRAW_X_COL, line_y, ALLEGRO_ALIGN_RIGHT, "%i", shape_dat [s] [i].base_irpt_max);
 }
 line_y += DRAW_LINE;
 al_draw_textf(font[FONT_SQUARE].fnt, colours.base [COL_GREY] [SHADE_HIGH], x - DRAW_X_COL_NAME, line_y, ALLEGRO_ALIGN_RIGHT, "data buffer");
 for (i = 0; i < SHAPES_SIZES; i ++)
 {
  al_draw_textf(font[FONT_SQUARE].fnt, colours.base [COL_GREY] [SHADE_MED], x + i * DRAW_X_COL, line_y, ALLEGRO_ALIGN_RIGHT, "%i", shape_dat [s] [i].base_data_max);
 }

 int base_hp_max;
 int base_irpt_max;
 int base_data_max;
 int mass_max;
 int shape_mass; // basic mass of shape with no methods
/*
   shape_dat [i] [j].base_hp_max = shape_solidity [i] * (5 + j); //shape_type[i].hp * (5 + j);
   shape_dat [i] [j].base_irpt_max = shape_solidity [i] * (4 + j) * 20; //shape_type[i].irpt * (5 + j);
   shape_dat [i] [j].base_data_max = shape_solidity [i] * (4 + j) * 2; //shape_type[i].data * (5 + j);
   shape_dat [i] [j].method_mass_max = shape_solidity [i] * (8 + (j * 4));
   shape_dat [i] [j].shape_mass = shape_solidity [i] * (3 + j); //shape_type[i].shape_mass * (5 + j);
*/

}




#endif
// ends ifdef DRAW_SHAPE_DATA

//#define DRAW_SHAPE_TEST

#ifdef DRAW_SHAPE_TEST


void draw_a_proc_shape_test(int s, int size, int method_vertices, float x, float y);

// This generates a special image for the manual (so a screenshot can be taken of it) then pretends to have an error and exits.
void draw_proc_shape_test(void)
{

 al_set_target_bitmap(al_get_backbuffer(display));
 al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA);
 al_clear_to_color(colours.base [COL_BLUE] [SHADE_MIN]);

 int s;
 int size;
 float x = 60;
 float y = 40;

 for (s = 2; s < SHAPES; s ++)
 {
 	for (size = 0; size < 4; size ++)
		{
   draw_a_proc_shape_test(s, size, -1, x, y);
   draw_a_proc_shape_test(s, size, 1, x, y + 400);
   y += 60 + (size * 25);
		}
		y = 40;
  x += 90;
 }

 draw_from_buffers();
 draw_fans();

 if (settings.option [OPTION_SPECIAL_CURSOR])
  draw_mouse_cursor();
 al_flip_display();
 al_set_target_bitmap(al_get_backbuffer(display));

 error_call();

}

#define DRAW_TEAM 0
#define DRAW_SIZE 1
#define DRAW_LINE 12

void draw_a_proc_shape_test(int s, int size, int method_vertices, float x, float y)
{

 struct procstruct draw_pr; // this just needs to be initialised to the extent that add_proc_shape() uses
 struct shapestruct* sh = &shape_dat [s] [size];
 float f_angle = -PI/2;

 int i;

 for (i = 0; i < SHAPES_VERTICES; i ++)
 {
  draw_pr.vertex_method [i] = method_vertices;
 }

 add_proc_shape(&draw_pr, x, y, f_angle, sh, s, size,
                colours.proc_fill [DRAW_TEAM] [PROC_FILL_SHADES - 1] [0],
                colours.team [DRAW_TEAM] [TCOL_FILL_BASE],
                colours.team [DRAW_TEAM] [TCOL_MAIN_EDGE]);



}



#endif
