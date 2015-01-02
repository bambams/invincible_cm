#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>

#include <stdio.h>
#include <math.h>

#include "m_config.h"

#include "g_header.h"

#include "g_misc.h"
#include "g_shape.h"
#include "m_maths.h"
#include "i_header.h"



extern struct shapestruct shape_dat [SHAPES] [SHAPES_SIZES]; // in g_motion

void add_vertex_to_shape(int s, int angle, int dist, int vertex_notch_inwards, int vertex_notch_sidewards);
void add_collision_vertex(int s, int angle, int dist, int dist_adjustment);
void add_back_vertex_to_shape(int s, int angle, int dist);
void finish_shape(int s);//, int base_vertex_notch_inwards, int base_vertex_notch_sidewards);
void init_shape_collision_masks(void);
//void test_draw_mask(int s, int test_x, int test_y);
void draw_line_on_mask(int s, int level, int xa, int ya, int xb, int yb);
void set_mask_pixel(int s, int level, int x, int y);
void floodfill_mask(int s, int level, int x, int y);

static int adjust_vertex_distance(int base_distance, int level);

/*
struct shape_typestruct shape_type [SHAPES] =
{
 {
  15, // hp
  32, // method_mass
  8 // shape_mass
 }, // SHAPE_3TRIANGLE

 {
  15, // hp
  160, // irpt
  20, // data
  32, // method_mass
  8 // shape_mass
 }, // SHAPE_OCTAGON
 {
  10, // hp
  110, // irpt
  13, // data
  24, // method_mass
  6 // shape_mass
 }, // SHAPE_DIAMOND

*/

int shape_solidity [SHAPES] =
{
// shape_solidity determines the strength and cost of a shape. Higher means more hp and method mass capacity, and also more base mass.
// 10 is base.
10, // SHAPE_3TRIANGLE,
13, // SHAPE_4SQUARE,
12, // SHAPE_4DIAMOND,
12, // SHAPE_4POINTY,
12, // SHAPE_4TRAP,
12, // SHAPE_4IRREG_L,
12, // SHAPE_4IRREG_R,
12, // SHAPE_4ARROW,
15, // SHAPE_5PENTAGON,
14, // SHAPE_5POINTY,
14, // SHAPE_5LONG,
14, // SHAPE_5WIDE,
17, // SHAPE_6HEXAGON,
16, // SHAPE_6POINTY,
16, // SHAPE_6LONG,
16, // SHAPE_6IRREG_L,
16, // SHAPE_6IRREG_R,
16, // SHAPE_6ARROW,
16, // SHAPE_6STAR,
21, // SHAPE_8OCTAGON,
20, // SHAPE_8POINTY,
20, // SHAPE_8LONG,
20, // SHAPE_8STAR,

/*
10, // SHAPE_3TRIANGLE,
11, // SHAPE_4SQUARE,
10, // SHAPE_4DIAMOND,
11, // SHAPE_4POINTY,
10, // SHAPE_4TRAP,
10, // SHAPE_4IRREG_L,
10, // SHAPE_4IRREG_R,
7, // SHAPE_4ARROW,
12, // SHAPE_5PENTAGON,
11, // SHAPE_5POINTY,
12, // SHAPE_5LONG,
10, // SHAPE_5WIDE,
14, // SHAPE_6HEXAGON,
11, // SHAPE_6POINTY,
12, // SHAPE_6LONG,
11, // SHAPE_6IRREG_L,
11, // SHAPE_6IRREG_R,
10, // SHAPE_6ARROW,
10, // SHAPE_6STAR,
20, // SHAPE_8OCTAGON,
15, // SHAPE_8POINTY,
18, // SHAPE_8LONG,
13, // SHAPE_8STAR,
*/
//16, // SHAPE_8BOX,
//15, // SHAPE_8LONG,

//SHAPES

};




void init_shapes(void)
{



 int i, j;

 for (i = 0; i < SHAPES; i ++)
 {
  for (j = 0; j < SHAPES_SIZES; j ++)
  {
   shape_dat [i] [j].vertices = 0;
   shape_dat [i] [j].max_length = al_itofix(0);
   shape_dat [i] [j].collision_vertices = 0;
  }
 }


 int shape_index;

// SHAPE_3TRIANGLE
 shape_index = SHAPE_3TRIANGLE;
 add_vertex_to_shape(shape_index, 0, 6, 20, 16);
 add_vertex_to_shape(shape_index, ANGLE_3, 6, 20, 16);
 add_vertex_to_shape(shape_index, -ANGLE_3, 6, 20, 16);
 add_back_vertex_to_shape(shape_index, ANGLE_2, 4); // must come after vertex 0 is added
 add_collision_vertex(shape_index, 0, 0, 0); // add an extra collision vertex at centre of shape
 finish_shape(shape_index);
// vertices (not collision vertices) must be added in clockwise order (for the external_angle calculations in finish_shape)

// SHAPE_4SQUARE
 shape_index = SHAPE_4SQUARE;
 add_vertex_to_shape(shape_index, 0, 5, 20, 15);
 add_vertex_to_shape(shape_index, ANGLE_4, 5, 20, 15);
 add_vertex_to_shape(shape_index, ANGLE_2, 5, 20, 15);
 add_vertex_to_shape(shape_index, -ANGLE_4, 5, 20, 15);
 add_back_vertex_to_shape(shape_index, ANGLE_2, 5); // must come after vertex 0 is added
 add_collision_vertex(shape_index, 0, 0, 0); // add an extra collision vertex at centre of shape
 finish_shape(shape_index);
// vertices (not collision vertices) must be added in clockwise order (for the external_angle calculations in finish_shape)

// SHAPE_4DIAMOND
 shape_index = SHAPE_4DIAMOND;
 add_vertex_to_shape(shape_index, 0, 6, 20, 16);
 add_vertex_to_shape(shape_index, ANGLE_4, 4, 18, 14);
 add_vertex_to_shape(shape_index, ANGLE_2, 6, 20, 16);
 add_vertex_to_shape(shape_index, -ANGLE_4, 4, 18, 14);
 add_back_vertex_to_shape(shape_index, ANGLE_2, 5); // must come after vertex 0 is added
 add_collision_vertex(shape_index, 0, 0, 0); // add an extra collision vertex at centre of shape
 finish_shape(shape_index);

// SHAPE_4POINTY
 shape_index = SHAPE_4POINTY;
 add_vertex_to_shape(shape_index, 0, 9, 20, 17);
 add_vertex_to_shape(shape_index, ANGLE_4, 4, 18, 15);
 add_vertex_to_shape(shape_index, ANGLE_2, 4, 20, 14);
 add_back_vertex_to_shape(shape_index, ANGLE_2, 4); // must come after vertex 0 is added
 add_vertex_to_shape(shape_index, -ANGLE_4, 4, 18, 15);
 add_collision_vertex(shape_index, 0, 0, 0); // add an extra collision vertex at centre of shape
 finish_shape(shape_index);
// vertices (not collision vertices) must be added in clockwise order (for the external_angle calculations in finish_shape)

// SHAPE_4TRAP
 shape_index = SHAPE_4TRAP; // all angles are +ANGLE_8 to make it easier for me to work them out
 add_vertex_to_shape(shape_index, -ANGLE_8 + ANGLE_8, 4, 18, 15);
 add_vertex_to_shape(shape_index, ANGLE_8 + ANGLE_8, 4, 18, 15);
 add_vertex_to_shape(shape_index, ANGLE_4 + ANGLE_16 + ANGLE_8, 6, 18, 15);
 add_back_vertex_to_shape(shape_index, ANGLE_2, 6); // must come after vertex 0 is added
 add_vertex_to_shape(shape_index, ANGLE_2 + ANGLE_4 - ANGLE_16 + ANGLE_8, 6, 18, 15);
 add_collision_vertex(shape_index, 0, 0, 0); // add an extra collision vertex at centre of shape
 finish_shape(shape_index);
// vertices (not collision vertices) must be added in clockwise order (for the external_angle calculations in finish_shape)

// SHAPE_4IRREG_L
 shape_index = SHAPE_4IRREG_L;
 add_vertex_to_shape(shape_index, 0, 8, 20, 17);
 add_vertex_to_shape(shape_index, ANGLE_4, 4, 18, 15);
 add_vertex_to_shape(shape_index, ANGLE_2 + ANGLE_32, 4, 20, 10);
 add_back_vertex_to_shape(shape_index, ANGLE_2, 4); // must come after vertex 0 is added
 add_vertex_to_shape(shape_index, ANGLE_2 + ANGLE_32 + ANGLE_8, 5, 18, 11);
 add_collision_vertex(shape_index, 0, 0, 0); // add an extra collision vertex at centre of shape
 finish_shape(shape_index);
// vertices (not collision vertices) must be added in clockwise order (for the external_angle calculations in finish_shape)

// SHAPE_4IRREG_R
 shape_index = SHAPE_4IRREG_R;
 add_vertex_to_shape(shape_index, 0, 8, 20, 17);
 add_vertex_to_shape(shape_index, ANGLE_2 - ANGLE_32 - ANGLE_8, 5, 18, 11);
 add_vertex_to_shape(shape_index, ANGLE_2 - ANGLE_32, 4, 20, 10);
 add_vertex_to_shape(shape_index, -ANGLE_4, 4, 18, 15);
 add_back_vertex_to_shape(shape_index, ANGLE_2, 4); // must come after vertex 0 is added
 add_collision_vertex(shape_index, 0, 0, 0); // add an extra collision vertex at centre of shape
 finish_shape(shape_index);
// vertices (not collision vertices) must be added in clockwise order (for the external_angle calculations in finish_shape)

// SHAPE_4ARROW
 shape_index = SHAPE_4ARROW;
 add_vertex_to_shape(shape_index, 0, 4, 20, 17);
 add_vertex_to_shape(shape_index, ANGLE_4 + ANGLE_8, 6, 18, 16);
 add_vertex_to_shape(shape_index, ANGLE_2, 2, 20, 16);
 add_back_vertex_to_shape(shape_index, ANGLE_2, 2); // must come after vertex 0 is added
 add_vertex_to_shape(shape_index, -ANGLE_4 - ANGLE_8, 6, 18, 16);
 add_collision_vertex(shape_index, 0, 0, 0); // add an extra collision vertex at centre of shape
 finish_shape(shape_index);
// vertices (not collision vertices) must be added in clockwise order (for the external_angle calculations in finish_shape)

// SHAPE_5PENTAGON
 shape_index = SHAPE_5PENTAGON;
 add_vertex_to_shape(shape_index, 0, 5, 19, 15);
 add_vertex_to_shape(shape_index, ANGLE_5, 5, 19, 15);
 add_vertex_to_shape(shape_index, ANGLE_5 * 2, 5, 19, 15);
 add_vertex_to_shape(shape_index, ANGLE_5 * 3, 5, 19, 15);
 add_vertex_to_shape(shape_index, ANGLE_5 * 4, 5, 19, 15);
 add_back_vertex_to_shape(shape_index, ANGLE_2, 4); // must come after vertex 0 is added
 add_collision_vertex(shape_index, 0, 0, 0); // add an extra collision vertex at centre of shape
 finish_shape(shape_index);

// SHAPE_5POINTY
 shape_index = SHAPE_5POINTY;
 add_vertex_to_shape(shape_index, 0, 7, 20, 16);
 add_vertex_to_shape(shape_index, ANGLE_4 - ANGLE_32, 4, 18, 13);
 add_vertex_to_shape(shape_index, ANGLE_2 - ANGLE_16 - ANGLE_64, 4, 18, 11);
 add_vertex_to_shape(shape_index, ANGLE_2 + ANGLE_16 + ANGLE_64, 4, 18, 11);
 add_vertex_to_shape(shape_index, -ANGLE_4 + ANGLE_32, 4, 18, 13);
 add_back_vertex_to_shape(shape_index, ANGLE_2, 3); // must come after vertex 0 is added
 add_collision_vertex(shape_index, 0, 0, 0); // add an extra collision vertex at centre of shape
 finish_shape(shape_index);

// SHAPE_5LONG
 shape_index = SHAPE_5LONG;
 add_vertex_to_shape(shape_index, 0, 7, 18, 14);
 add_vertex_to_shape(shape_index, ANGLE_5 - ANGLE_32, 4, 18, 14);
 add_vertex_to_shape(shape_index, ANGLE_5 * 2 + ANGLE_32, 5, 18, 13);
 add_vertex_to_shape(shape_index, ANGLE_5 * 3 - ANGLE_32, 5, 18, 13);
 add_vertex_to_shape(shape_index, ANGLE_5 * 4 + ANGLE_32, 4, 18, 14);
 add_back_vertex_to_shape(shape_index, ANGLE_2, 6); // must come after vertex 0 is added
 add_collision_vertex(shape_index, 0, 0, 0); // add an extra collision vertex at centre of shape
 finish_shape(shape_index);

// SHAPE_5WIDE
 shape_index = SHAPE_5WIDE;
 add_vertex_to_shape(shape_index, 0, 4, 18, 14);
 add_vertex_to_shape(shape_index, ANGLE_4 - ANGLE_32, 6, 18, 12);
 add_vertex_to_shape(shape_index, ANGLE_4 + ANGLE_16, 3, 18, 12);
 add_vertex_to_shape(shape_index, -ANGLE_4 - ANGLE_16, 3, 18, 12);
 add_vertex_to_shape(shape_index, -ANGLE_4 + ANGLE_32, 6, 18, 12);
 add_back_vertex_to_shape(shape_index, ANGLE_2, 3); // must come after vertex 0 is added
 add_collision_vertex(shape_index, 0, 0, 0); // add an extra collision vertex at centre of shape
 finish_shape(shape_index);

// SHAPE_6HEXAGON
 shape_index = SHAPE_6HEXAGON;
 add_vertex_to_shape(shape_index, 0, 5, 19, 15);
 add_vertex_to_shape(shape_index, ANGLE_6, 5, 19, 15);
 add_vertex_to_shape(shape_index, ANGLE_6 * 2, 5, 19, 15);
 add_vertex_to_shape(shape_index, ANGLE_6 * 3, 5, 19, 15);
 add_vertex_to_shape(shape_index, ANGLE_6 * 4, 5, 19, 15);
 add_vertex_to_shape(shape_index, ANGLE_6 * 5, 5, 19, 15);
 add_back_vertex_to_shape(shape_index, ANGLE_2, 5); // must come after vertex 0 is added
 add_collision_vertex(shape_index, 0, 0, 0); // add an extra collision vertex at centre of shape
 finish_shape(shape_index);

 // SHAPE_6POINTY
 shape_index = SHAPE_6POINTY;
 add_vertex_to_shape(shape_index, 0, 8, 18, 14);
 add_vertex_to_shape(shape_index, ANGLE_6 - ANGLE_32, 4, 19, 13);
 add_vertex_to_shape(shape_index, ANGLE_6 * 2, 4, 17, 12);
 add_vertex_to_shape(shape_index, ANGLE_6 * 3, 4, 17, 12);
 add_vertex_to_shape(shape_index, ANGLE_6 * 4, 4, 17, 12);
 add_vertex_to_shape(shape_index, ANGLE_6 * 5 + ANGLE_32, 4, 18, 12);
 add_back_vertex_to_shape(shape_index, ANGLE_2, 4); // must come after vertex 0 is added
 add_collision_vertex(shape_index, 0, 0, 0); // add an extra collision vertex at centre of shape
 finish_shape(shape_index);

 // SHAPE_6LONG
 shape_index = SHAPE_6LONG;
 add_vertex_to_shape(shape_index, 0, 7, 18, 13);
 add_vertex_to_shape(shape_index, ANGLE_6 - ANGLE_64, 4, 18, 12);
 add_vertex_to_shape(shape_index, ANGLE_6 * 2 + ANGLE_64, 4, 18, 12);
 add_vertex_to_shape(shape_index, ANGLE_6 * 3, 7, 18, 13);
 add_vertex_to_shape(shape_index, ANGLE_6 * 4 - ANGLE_64, 4, 18, 12);
 add_vertex_to_shape(shape_index, ANGLE_6 * 5 + ANGLE_64, 4, 18, 12);
 add_back_vertex_to_shape(shape_index, ANGLE_2, 6); // must come after vertex 0 is added
 add_collision_vertex(shape_index, 0, 0, 0); // add an extra collision vertex at centre of shape
 finish_shape(shape_index);

 // SHAPE_6IRREG_L
 shape_index = SHAPE_6IRREG_L;
 add_vertex_to_shape(shape_index, 0, 7, 18, 9);
 add_vertex_to_shape(shape_index, ANGLE_4 - ANGLE_8, 4, 18, 11);
 add_vertex_to_shape(shape_index, ANGLE_2 - ANGLE_16, 5, 18, 11);
 add_vertex_to_shape(shape_index, ANGLE_2, 7, 18, 9);
 add_vertex_to_shape(shape_index, ANGLE_2 + ANGLE_4 - ANGLE_8, 4, 18, 11);
 add_vertex_to_shape(shape_index, -ANGLE_16, 5, 18, 11);
 add_back_vertex_to_shape(shape_index, ANGLE_2, 6); // must come after vertex 0 is added
 add_collision_vertex(shape_index, 0, 0, 0); // add an extra collision vertex at centre of shape
 finish_shape(shape_index);

 // SHAPE_6IRREG_R
 shape_index = SHAPE_6IRREG_R;
 add_vertex_to_shape(shape_index, 0, 7, 18, 9);
 add_vertex_to_shape(shape_index, ANGLE_16, 5, 18, 11);
 add_vertex_to_shape(shape_index, ANGLE_2 - ANGLE_4 + ANGLE_8, 4, 18, 11);
 add_vertex_to_shape(shape_index, ANGLE_2, 7, 18, 9);
 add_vertex_to_shape(shape_index, ANGLE_2 + ANGLE_16, 5, 18, 11);
 add_vertex_to_shape(shape_index, -ANGLE_4 + ANGLE_8, 4, 18, 11);
 add_back_vertex_to_shape(shape_index, ANGLE_2, 6); // must come after vertex 0 is added
 add_collision_vertex(shape_index, 0, 0, 0); // add an extra collision vertex at centre of shape
 finish_shape(shape_index);

 // SHAPE_6ARROW
 shape_index = SHAPE_6ARROW;
 add_vertex_to_shape(shape_index, 0, 3, 16, 13);
 add_vertex_to_shape(shape_index, ANGLE_4 + ANGLE_32, 6, 18, 10);
 add_vertex_to_shape(shape_index, ANGLE_4 + ANGLE_8, 6, 18, 10);
 add_vertex_to_shape(shape_index, ANGLE_2, 2, 22, 17);
 add_vertex_to_shape(shape_index, -ANGLE_4 - ANGLE_8, 6, 18, 10);
 add_vertex_to_shape(shape_index, -ANGLE_4 - ANGLE_32, 6, 18, 10);
 add_back_vertex_to_shape(shape_index, ANGLE_2, 6); // must come after vertex 0 is added
 add_collision_vertex(shape_index, 0, 0, 0); // add an extra collision vertex at centre of shape
 finish_shape(shape_index);

// SHAPE_6STAR
 shape_index = SHAPE_6STAR;
 add_vertex_to_shape(shape_index, 0, 8, 19, 18);
 add_vertex_to_shape(shape_index, ANGLE_6, 2, 19, 16);
 add_vertex_to_shape(shape_index, ANGLE_6 * 2, 8, 19, 18);
 add_vertex_to_shape(shape_index, ANGLE_6 * 3, 2, 19, 16);
 add_vertex_to_shape(shape_index, ANGLE_6 * 4, 8, 19, 18);
 add_vertex_to_shape(shape_index, ANGLE_6 * 5, 2, 19, 16);
 add_back_vertex_to_shape(shape_index, ANGLE_2, 3); // must come after vertex 0 is added
 add_collision_vertex(shape_index, 0, 0, 0); // add an extra collision vertex at centre of shape
 finish_shape(shape_index);

// SHAPE_8OCTAGON
 for (i = 0; i < 8; i ++)
 {
//  add_vertex_to_shape(SHAPE_OCTAGON, ANGLE_16 + i * ANGLE_8, 6);
  add_vertex_to_shape(SHAPE_8OCTAGON, i * ANGLE_8, 6, 20, 14);
 }
 add_back_vertex_to_shape(SHAPE_8OCTAGON, 3 * ANGLE_8, 6); // must come after vertex 0 is added
 add_collision_vertex(SHAPE_8OCTAGON, 0, 0, 0); // add an extra collision vertex at centre of shape
 finish_shape(SHAPE_8OCTAGON);
// vertices must be added in clockwise order (for the external_angle calculations in finish_shape)

 // SHAPE_8POINTY
 shape_index = SHAPE_8POINTY;
 add_vertex_to_shape(shape_index, 0, 8, 18, 13);
 add_vertex_to_shape(shape_index, ANGLE_4 - ANGLE_8, 5, 18, 12);
 add_vertex_to_shape(shape_index, ANGLE_4, 5, 18, 11);
 add_vertex_to_shape(shape_index, ANGLE_4 + ANGLE_8 - ANGLE_64, 4, 18, 11);
 add_vertex_to_shape(shape_index, ANGLE_2, 4, 18, 12);
 add_vertex_to_shape(shape_index, ANGLE_2 + ANGLE_8 + ANGLE_64, 4, 18, 11);
 add_vertex_to_shape(shape_index, -ANGLE_4, 5, 18, 11);
 add_vertex_to_shape(shape_index, -ANGLE_4 + ANGLE_8, 5, 18, 12);
 add_back_vertex_to_shape(shape_index, ANGLE_2, 4); // must come after vertex 0 is added
 add_collision_vertex(shape_index, 0, 0, 0); // add an extra collision vertex at centre of shape
 finish_shape(shape_index);

 // SHAPE_8LONG
 shape_index = SHAPE_8LONG;
 add_vertex_to_shape(shape_index, 0, 7, 18, 13);
 add_vertex_to_shape(shape_index, ANGLE_4 - ANGLE_8, 5, 18, 12);
 add_vertex_to_shape(shape_index, ANGLE_4, 5, 18, 12);
 add_vertex_to_shape(shape_index, ANGLE_4 + ANGLE_8, 5, 18, 13);
 add_vertex_to_shape(shape_index, ANGLE_2, 7, 18, 12);
 add_vertex_to_shape(shape_index, ANGLE_2 + ANGLE_8, 5, 18, 12);
 add_vertex_to_shape(shape_index, -ANGLE_4, 5, 18, 12);
 add_vertex_to_shape(shape_index, -ANGLE_4 + ANGLE_8, 5, 18, 12);
 add_back_vertex_to_shape(shape_index, ANGLE_2, 7); // must come after vertex 0 is added
 add_collision_vertex(shape_index, 0, 0, 0); // add an extra collision vertex at centre of shape
 finish_shape(shape_index);


// SHAPE_8STAR
 for (i = 0; i < 8; i ++)
 {
  if (!(i & 1))
   add_vertex_to_shape(SHAPE_8STAR, i * ANGLE_8, 7, 18, 16);
    else
     add_vertex_to_shape(SHAPE_8STAR, i * ANGLE_8, 3, 20, 15);
 }
 add_back_vertex_to_shape(SHAPE_8STAR, 3 * ANGLE_8, 6); // must come after vertex 0 is added
 add_collision_vertex(SHAPE_8STAR, 0, 0, 0); // add an extra collision vertex at centre of shape
 finish_shape(SHAPE_8STAR);

 for (i = 0; i < SHAPES; i ++)
 {
  for (j = 0; j < SHAPES_SIZES; j ++)
  {
   shape_dat [i] [j].base_hp_max = shape_solidity [i] * (5 + j); //shape_type[i].hp * (5 + j);
   shape_dat [i] [j].base_irpt_max = shape_solidity [i] * (4 + j) * 10; //shape_type[i].irpt * (5 + j);
   shape_dat [i] [j].base_data_max = shape_solidity [i] * (5 + j) * 2; //shape_type[i].data * (5 + j);
//   shape_dat [i] [j].mass_max = shape_solidity [i] * (8 + (j * 3));
   shape_dat [i] [j].shape_mass = shape_solidity [i] * (3 + j); //shape_type[i].shape_mass * (5 + j);
   shape_dat [i] [j].shape_mass += shape_dat[i][j].vertices * 2;
   shape_dat [i] [j].shape_mass += j * j * j;
   shape_dat [i] [j].mass_max = shape_dat [i] [j].shape_mass
                                + (shape_solidity [i] * (8 + (j)));
//                                - shape_solidity [i] * (3 + j);
//                                + (shape_solidity [i] * (8 + (j * 3)))
//                                - shape_solidity [i] * (3 + j);


// prop->mass += shape_str->vertices * 2;
// prop->mass += prop->size * (4 + prop->size);;
  }
 }

 init_shape_collision_masks();

// finished shape geometry. now do shape gameplay properties:

// prepare_sense_circles();

}


//#define SHAPE_SIZE_FACTOR 5

void add_vertex_to_shape(int s, int angle, int dist, int vertex_notch_inwards, int vertex_notch_sidewards)
{

 int size;

 for (size = 0; size < SHAPES_SIZES; size ++)
 {
  shape_dat[s][size].vertex_angle [shape_dat[s][size].vertices] = short_angle_to_fixed(angle);
  shape_dat[s][size].vertex_angle_float [shape_dat[s][size].vertices] = angle_to_radians(angle & ANGLE_MASK); // used for display
  shape_dat[s][size].vertex_dist_pixel [shape_dat[s][size].vertices] = adjust_vertex_distance(dist, size);
  shape_dat[s][size].vertex_dist [shape_dat[s][size].vertices] = al_itofix(shape_dat[s][size].vertex_dist_pixel [shape_dat[s][size].vertices]);

// The vertex_notch_inwards and _sidewards values are further adjusted in finish_shape
  shape_dat[s][size].vertex_notch_inwards [shape_dat[s][size].vertices] = (float) (vertex_notch_inwards + size * 3);
  shape_dat[s][size].vertex_notch_sidewards [shape_dat[s][size].vertices] = (float) (vertex_notch_sidewards + size * 2);

  if (shape_dat[s][size].max_length < shape_dat[s][size].vertex_dist [shape_dat[s][size].vertices])
   shape_dat[s][size].max_length = shape_dat[s][size].vertex_dist [shape_dat[s][size].vertices];
  shape_dat[s][size].vertices ++;
 }

  add_collision_vertex(s, angle, dist, 0);
  add_collision_vertex(s, angle, dist, -2);
  add_collision_vertex(s, angle, dist, -4);

}


void add_back_vertex_to_shape(int s, int angle, int dist)
{

 int size;

 for (size = 0; size < SHAPES_SIZES; size ++)
 {
  shape_dat[s][size].back_point_angle_float = angle_to_radians(angle & ANGLE_MASK); // used for display
  shape_dat[s][size].back_point_dist_float = adjust_vertex_distance(dist, size);
  shape_dat[s][size].back_point_to_front = adjust_vertex_distance(dist, size) + shape_dat[s][size].vertex_dist_pixel [0];

 }


}

static int adjust_vertex_distance(int base_distance, int size)
{
// if this is changed, need to change the " + level * 3" code in the collision mask init code below to make
//  sure the collision mask levels that should correspond to shape sizes do so correctly.
 return base_distance * (size + 4);
}

// dist is in pixels, not grains
void add_collision_vertex(int s, int angle, int dist, int dist_adjustment)
{

 if (shape_dat[s][0].collision_vertices >= (COLLISION_VERTICES - 1)) //SHAPES_SIZES))
 {
  fprintf(stdout, "\ng_shape.c:add_collision_vertex(): Error: too many collision vertices (shape %i)\n", s);
  error_call();
 }

 int size;

 for (size = 0; size < SHAPES_SIZES; size ++)
 {
  shape_dat[s][size].collision_vertex_angle [shape_dat[s][size].collision_vertices] = short_angle_to_fixed(angle);
  shape_dat[s][size].collision_vertex_dist_pixels [shape_dat[s][size].collision_vertices] = ((dist * (size + 4)) + dist_adjustment); // used for collision mask checking
  shape_dat[s][size].collision_vertex_dist [shape_dat[s][size].collision_vertices] = al_itofix((dist * (size + 4)) + dist_adjustment); // used for collision physics calculations
  if (shape_dat[s][size].max_length < shape_dat[s][size].collision_vertex_dist [shape_dat[s][size].collision_vertices])
   shape_dat[s][size].max_length = shape_dat[s][size].collision_vertex_dist [shape_dat[s][size].collision_vertices];
  shape_dat[s][size].collision_vertices ++;
 }

}

// This function closes the loop by adding an extra vertex at the start point (so the drawing functions complete the shape)
// also sets external_angle
// Doesn't deal with collision vertices at all (they don't need a closed loop)
void finish_shape(int s) //, int base_vertex_notch_inwards, int base_vertex_notch_sidewards)
{


 int size;

 for (size = 0; size < SHAPES_SIZES; size ++)
 {
  shape_dat[s][size].vertex_angle [shape_dat[s][size].vertices] = shape_dat[s][size].vertex_angle [0];
  shape_dat[s][size].vertex_angle_float [shape_dat[s][size].vertices] = shape_dat[s][size].vertex_angle_float [0];
  shape_dat[s][size].vertex_dist [shape_dat[s][size].vertices] = shape_dat[s][size].vertex_dist [0];
  shape_dat[s][size].vertex_dist_pixel [shape_dat[s][size].vertices] = shape_dat[s][size].vertex_dist_pixel [0];
  shape_dat[s][size].max_length += al_itofix(5); // to give plenty of space for collision detection
//  shape_dat[s][size].vertices ++;
 }

// Note that finish_shape does not increment vertices - it assumes that the drawing function will use [vertices + 1] to close the shape

 int i;
 al_fixed current_angle;
 al_fixed current_x, current_y;

 int previous_vertex;
 al_fixed previous_angle;
 al_fixed previous_x, previous_y;
 al_fixed angle_to_previous;

 int next_vertex;
 al_fixed next_angle;
 al_fixed next_x, next_y;
 al_fixed angle_to_next;
 int angle_size; // total size of angle (in angle)

 for (size = 0; size < SHAPES_SIZES; size ++)
 {
  for (i = 0; i < shape_dat[s][size].vertices; i ++)
  {
   current_angle = shape_dat[s][size].vertex_angle [i];
   current_x = fixed_xpart(current_angle, shape_dat[s][size].vertex_dist [i]);
   current_y = fixed_ypart(current_angle, shape_dat[s][size].vertex_dist [i]);

   previous_vertex = i - 1;
   if (previous_vertex == -1)
    previous_vertex = shape_dat[s][size].vertices - 1;
   previous_angle = shape_dat[s][size].vertex_angle [previous_vertex];
   previous_x = fixed_xpart(previous_angle, shape_dat[s][size].vertex_dist [previous_vertex]);
   previous_y = fixed_ypart(previous_angle, shape_dat[s][size].vertex_dist [previous_vertex]);

   next_vertex = i + 1; // don't need to bounds check as there is an extra vertex at the end
   next_angle = shape_dat[s][size].vertex_angle [next_vertex];
   next_x = fixed_xpart(next_angle, shape_dat[s][size].vertex_dist [next_vertex]);
   next_y = fixed_ypart(next_angle, shape_dat[s][size].vertex_dist [next_vertex]);

   angle_to_previous = (get_angle(previous_y - current_y, previous_x - current_x) - current_angle) & AFX_MASK;
   angle_to_next = (get_angle(next_y - current_y, next_x - current_x) - current_angle) & AFX_MASK;

   shape_dat[s][size].external_angle [EXANGLE_PREVIOUS] [i] = angle_to_previous;
   shape_dat[s][size].external_angle [EXANGLE_NEXT] [i] = angle_to_next;

   if (angle_to_previous < angle_to_next)
   {
    shape_dat[s][size].external_angle_wrapped [i] = 0;
    shape_dat[s][size].external_angle [EXANGLE_LOW] [i] = angle_to_previous;
    shape_dat[s][size].external_angle [EXANGLE_HIGH] [i] = angle_to_next;
    angle_size = fixed_angle_to_int(al_fixsub(angle_to_next, angle_to_previous));
    shape_dat[s][size].vertex_method_angle_min [i] = fixed_angle_to_int(angle_to_previous) - ANGLE_1;
    shape_dat[s][size].vertex_method_angle_max [i] = fixed_angle_to_int(angle_to_next);
   }
    else
    {
     shape_dat[s][size].external_angle_wrapped [i] = 1;
     shape_dat[s][size].external_angle [EXANGLE_LOW] [i] = angle_to_next;
     shape_dat[s][size].external_angle [EXANGLE_HIGH] [i] = angle_to_previous;
     angle_size = fixed_angle_to_int(al_fixsub(angle_to_previous, angle_to_next));
     shape_dat[s][size].vertex_method_angle_min [i] = fixed_angle_to_int(angle_to_previous) - ANGLE_1;
     shape_dat[s][size].vertex_method_angle_max [i] = fixed_angle_to_int(angle_to_next);
    }

   shape_dat[s][size].dist_to_previous [i] = al_fixtoi(distance(previous_y - current_y, previous_x - current_x));
   shape_dat[s][size].dist_to_next [i] = al_fixtoi(distance(next_y - current_y, next_x - current_x));


   angle_size &= ANGLE_MASK;
// now work out the size of the notch:
   shape_dat[s][size].vertex_notch_inwards [i] -= (float) angle_size / 300;
   shape_dat[s][size].vertex_notch_sidewards [i] -= (float) angle_size / 500;
//   shape_dat[s][size].vertex_notch_inwards [i] = (float) (base_vertex_notch_inwards + size * 3) - angle_size / 300;
//   shape_dat[s][size].vertex_notch_sidewards [i] = (float) (base_vertex_notch_sidewards + size * 3) - angle_size / 500;
//   fprintf(stdout, "\nShape %i size %i vertex %i angle_size %i inwards %f sidewards %f", s, size, i, angle_size, shape_dat[s][size].vertex_notch_inwards [i], shape_dat[s][size].vertex_notch_sidewards [i]);
  }
 }


// this is poorly optimised; there's no need to calculate external_angles separately for each size (although this function is only called once anyway).
//  TO DO: fix this.

}


int collision_mask [SHAPES] [COLLISION_MASK_SIZE] [COLLISION_MASK_SIZE];

void init_shape_collision_masks(void)
{

 int x, y, s, v;
 al_fixed xa, ya, xb, yb;
 int level = 0;

  int total_area = 0;
//  int base_area = 0;


 for (s = 0; s < SHAPES; s ++)
 {
  for (x = 0; x < COLLISION_MASK_SIZE; x ++)
  {
   for (y = 0; y < COLLISION_MASK_SIZE; y ++)
   {
    collision_mask [s] [x] [y] = COLLISION_MASK_LEVELS;
   }
  }

// count backwards through the levels. This draws a series of shapes, each one smaller than the last.
  for (level = COLLISION_MASK_LEVELS - 1; level >= 0; level --)
  {

// we start by drawing lines around the edges of the shape:
   for (v = 0; v < shape_dat [s] [0].vertices; v ++)
   {
    xa = MASK_CENTRE_FIXED + fixed_xpart(shape_dat [s] [0].vertex_angle [v], (shape_dat [s] [0].vertex_dist [v] / 4) * (level + 4));
    xa = al_itofix(al_fixtoi(xa) >> COLLISION_MASK_BITSHIFT);
    ya = MASK_CENTRE_FIXED + fixed_ypart(shape_dat [s] [0].vertex_angle [v], (shape_dat [s] [0].vertex_dist [v] / 4) * (level + 4));
    ya = al_itofix(al_fixtoi(ya) >> COLLISION_MASK_BITSHIFT);
    xb = MASK_CENTRE_FIXED + fixed_xpart(shape_dat [s] [0].vertex_angle [v+1], (shape_dat [s] [0].vertex_dist [v+1] / 4) * (level + 4));
    xb = al_itofix(al_fixtoi(xb) >> COLLISION_MASK_BITSHIFT);
    yb = MASK_CENTRE_FIXED + fixed_ypart(shape_dat [s] [0].vertex_angle [v+1], (shape_dat [s] [0].vertex_dist [v+1] / 4) * (level + 4));
    yb = al_itofix(al_fixtoi(yb) >> COLLISION_MASK_BITSHIFT);
    draw_line_on_mask(s, level, xa, ya, xb, yb);

   }

   floodfill_mask(s, level, MASK_CENTRE >> COLLISION_MASK_BITSHIFT, MASK_CENTRE >> COLLISION_MASK_BITSHIFT);

// if (s == SHAPE_DIAMOND)
// test_draw_mask(s, size);



  }
  int i, j;
  total_area = 0;
  for (i = 0; i < COLLISION_MASK_SIZE; i ++)
		{
			for (j = 0; j < COLLISION_MASK_SIZE; j ++)
			{
				if (collision_mask [s] [i] [j] <= 2)
					total_area ++;
			}
		}

//		if (s == SHAPE_3TRIANGLE)
		//{
//			base_area = total_area;
//		}
/*

This code prints out details of shape init values and may be useful

		fprintf(stdout, "\nShape ");
		switch(s)
		{
			case SHAPE_3TRIANGLE: fprintf(stdout, "SHAPE_3TRIANGLE"); break;
			case SHAPE_4SQUARE: fprintf(stdout, "SHAPE_4SQUARE"); break;
			case SHAPE_4DIAMOND: fprintf(stdout, "SHAPE_4DIAMOND"); break;
			case SHAPE_4POINTY: fprintf(stdout, "SHAPE_4POINTY"); break;
			case SHAPE_4TRAP: fprintf(stdout, "SHAPE_4TRAP"); break;
			case SHAPE_4IRREG_L: fprintf(stdout, "SHAPE_4IRREG_L"); break;
			case SHAPE_4IRREG_R: fprintf(stdout, "SHAPE_4IRREG_R"); break;
			case SHAPE_4ARROW: fprintf(stdout, "SHAPE_4ARROW"); break;
			case SHAPE_5PENTAGON: fprintf(stdout, "SHAPE_5PENTAGON"); break;
			case SHAPE_5POINTY: fprintf(stdout, "SHAPE_5POINTY"); break;
			case SHAPE_5LONG: fprintf(stdout, "SHAPE_5LONG"); break;
			case SHAPE_5WIDE: fprintf(stdout, "SHAPE_5WIDE"); break;
			case SHAPE_6HEXAGON: fprintf(stdout, "SHAPE_6HEXAGON"); break;
			case SHAPE_6POINTY: fprintf(stdout, "SHAPE_6POINTY"); break;
			case SHAPE_6LONG: fprintf(stdout, "SHAPE_6LONG"); break;
			case SHAPE_6IRREG_L: fprintf(stdout, "SHAPE_6IRREG_L"); break;
			case SHAPE_6IRREG_R: fprintf(stdout, "SHAPE_6IRREG_R"); break;
			case SHAPE_6ARROW: fprintf(stdout, "SHAPE_6ARROW"); break;
			case SHAPE_6STAR: fprintf(stdout, "SHAPE_6STAR"); break;
			case SHAPE_8OCTAGON: fprintf(stdout, "SHAPE_8OCTAGON"); break;
			case SHAPE_8POINTY: fprintf(stdout, "SHAPE_8POINTY"); break;
			case SHAPE_8LONG: fprintf(stdout, "SHAPE_8LONG"); break;
			case SHAPE_8STAR: fprintf(stdout, "SHAPE_8STAR"); break;

		}
//		fprintf(stdout, "\nShape %i %i total area %i proportion %f", s, base_area, total_area, (float) total_area / (float) base_area);
//		fprintf(stdout, " total area %i proportion %f  ", total_area, (float) total_area / (float) base_area);
		fprintf(stdout, " sm %i-%i mm %i-%i d %i-%i", shape_dat[s][0].shape_mass, shape_dat[s][3].shape_mass, shape_dat[s][0].mass_max, shape_dat[s][3].mass_max,
												shape_dat[s][0].mass_max - shape_dat[s][0].shape_mass, shape_dat[s][3].mass_max - shape_dat[s][3].shape_mass);

*/

 }


}


void draw_line_on_mask(int s, int level, al_fixed xa, al_fixed ya, al_fixed xb, al_fixed yb)
{

// fprintf(stdout, "line s %i size %i (%i, %i) to (%i, %i)\n", s, size, xa, ya, xb, yb);
 int x, y;

 int inc_x, inc_y;

 set_mask_pixel(s, level, al_fixtoi(xa), al_fixtoi(ya));
 set_mask_pixel(s, level, al_fixtoi(xb), al_fixtoi(yb));


 if (xa == xb)
 {
  if (ya == yb)
  {
//   collision_mask [s] [size] [xa] [ya] = 1;
//   set_mask_pixel(s, size, xa, ya);
   return;
  }
  if (ya < yb)
   inc_y = 1;
    else
     inc_y = -1;
  y = al_fixtoi(ya);
  set_mask_pixel(s, level, al_fixtoi(xa), y);
  while (y != al_fixtoi(yb))
  {
   y += inc_y;
   set_mask_pixel(s, level, al_fixtoi(xa), y);
  };
  return;
 }

 if (ya == yb)
 {
// don't need to allow for xa == xb as that will have been caught above
  if (xa < xb)
   inc_x = 1;
    else
     inc_x = -1;
  x = al_fixtoi(xa);
  set_mask_pixel(s, level, x, al_fixtoi(ya));
  while (x != al_fixtoi(xb))
  {
   x += inc_x;
   set_mask_pixel(s, level, x, al_fixtoi(ya));
  };
  return;
 }

 al_fixed dx, dy;
 al_fixed fx, fy;

 al_fixed temp_fixed;
 int timeout = 1000;

 if (xb < xa)
 {
// flip coordinates
  temp_fixed = xb;
  xb = xa;
  xa = temp_fixed;
  temp_fixed = yb;
  yb = ya;
  ya = temp_fixed;
 }

 fx = xa;
 fy = ya;
 x = al_fixtoi(xa);
 y = al_fixtoi(ya);
 dx = al_fixdiv(xb - xa, yb - ya);
 dy = al_fixdiv(yb - ya, xb - xa);

// fprintf(stdout, "\ndx = %f dy = %f\n", al_fixtof(dx), al_fixtof(dy));

 if (dy < al_itofix(1) && dy > al_itofix(-1))
 {
//  dy /= dx;
  while(x != al_fixtoi(xb) || y != al_fixtoi(yb))
  {
   x ++;
   fy += dy;
   y = al_fixtoi(fy);
   set_mask_pixel(s, level, x, y);
//   fprintf(stdout, "1st loop: (%i,%i) for (%i,%i) f(%f,%f)\n", x, y, xb, yb, dx, dy);
   timeout --;
   if (!timeout)
   {
//    test_draw_mask(s, 0, 0);
    fprintf(stdout, "\n line draw timed out (shape %i level %i xy %i,%i xb,yb %i,%i xa,ya %i,%i dx,dy %f,%f).", s, level, x, y, al_fixtoi(xb), al_fixtoi(yb), al_fixtoi(xa), al_fixtoi(ya), al_fixtof(dx), al_fixtof(dy));
    error_call();
   }
   if (x == al_fixtoi(xb) && (y == al_fixtoi(yb) - 1 || y == al_fixtoi(yb) + 1))
    break;
   if (y == al_fixtoi(yb) && (x == al_fixtoi(xb) - 1 || x == al_fixtoi(xb) + 1))
    break;
  };
  set_mask_pixel(s, level, al_fixtoi(xb), al_fixtoi(yb));
  return;
 }
  else
  {
//   dx /= dy;
   if (dx < al_itofix(0))
    dx *= -1;
   while(x != al_fixtoi(xb) || y != al_fixtoi(yb))
   {
    if (dy > al_itofix(0))
     y ++;
      else
       y --;
    fx += dx;
    x = al_fixtoi(fx);
    set_mask_pixel(s, level, x, y);
//   fprintf(stdout, "2nd loop: (%i,%i) for (%i,%i) f(%f,%f)\n", x, y, xb, yb, dx, dy);
   if (x == al_fixtoi(xb) && (y == al_fixtoi(yb) - 1 || y == al_fixtoi(yb) + 1))
    break;
   if (y == al_fixtoi(yb) && (x == al_fixtoi(xb) - 1 || x == al_fixtoi(xb) + 1))
    break;
/*   timeout --;
   if (!timeout)
    error_call();*/
   };
   set_mask_pixel(s, level, al_fixtoi(xb), al_fixtoi(yb));
   return;
  }

/*
line drawing routine works like this:

we start off assuming that xa < xb (although ya may be > yb)

then we work out whether dx > dy

if dy is smaller:

then we add dy to fy
if (int) fy doesn't increase, we x++ and draw a point
if (int) fy increases by 1 we x++ and draw a point

if dx is smaller, we do the same but x/y flipped

*/

}

void set_mask_pixel(int s, int level, int x, int y)
{

// fprintf(stdout, " (%i,%i)", x, y);

 if (x >= 0 && y >= 0 && x < COLLISION_MASK_SIZE && y < COLLISION_MASK_SIZE)
  collision_mask [s] [x] [y] = level;

}

// recursive floodfill function
void floodfill_mask(int s, int level, int x, int y)
{

#ifdef SANITY_CHECK

 if (x < 0
  || x >= COLLISION_MASK_SIZE
  || y < 0
  || y >= COLLISION_MASK_SIZE)
 {
  fprintf(stdout, "\nError: g_shape.c: floodfill_mask(): out of bounds");
  return;
//  error_call();
 }

#endif

 if (collision_mask [s] [x] [y] == level)
  return;

 collision_mask [s] [x] [y] = level;

 floodfill_mask(s, level, x - 1, y);
 floodfill_mask(s, level, x + 1, y);
 floodfill_mask(s, level, x, y - 1);
 floodfill_mask(s, level, x, y + 1);



}

//#define TEST_DRAW_MASK

#ifdef TEST_DRAW_MASK

extern ALLEGRO_DISPLAY* display;

// debug function
void test_draw_mask(int s, int test_x, int test_y)
{

 al_set_target_bitmap(al_get_backbuffer(display));

 al_clear_to_color(colours.base [0] [0]);

 ALLEGRO_COLOR mask_colour [25];
 int i;
 for (i = 0; i < 25; i ++)
 {
  mask_colour [i] = al_map_rgb(i * 10, i * ((i % 2)) * 5, 100);
 }

 int x, y;
 int xa, ya;

 for (x = 0; x < COLLISION_MASK_SIZE; x ++)
 {
  for (y = 0; y < COLLISION_MASK_SIZE; y ++)
  {
    xa = 10 + x * 3;
    ya = 10 + y * 3;
    if (x == test_x && y == test_y)
     al_draw_filled_rectangle(xa, ya, xa + 2, ya + 2, colours.base [COL_BLUE] [SHADE_MAX]);
      else
       al_draw_filled_rectangle(xa, ya, xa + 2, ya + 2, mask_colour [collision_mask [s] [x] [y]]);
  }
 }

 if (settings.option [OPTION_SPECIAL_CURSOR])
  draw_mouse_cursor();
 al_flip_display();

// error_call();
// wait_for_space();

}

#endif


#define SENSE_CIRCLES 20
#define SENSE_CIRCLE_LENGTH 300

/*
sense_circles is a list of the x/y coordinates of blocks at certain distances from a centre block.
the first dimension (SENSE_CIRCLES) indicates the distance (in blocks) from the centre
the second dimension is the coordinate, in order
So e.g. searching for nearby procs at block range 1 might look like this:
 - check sense_circles [0]
  - check +0, +0
 - check sense_circles [1]
  - check -1, -1
  - check 0, -1
  - check 1, -1
  - check -1, 0
  - etc.
the end is indicated by an x value of 127

the checked_circle variable in prepare_sense_circles is used to prevent double counting; a particular set of coordinates can only
be in one sense_circle


*/
//char sense_circles [SENSE_CIRCLES] [SENSE_CIRCLE_LENGTH] [2];


void prepare_sense_circles(void)
{

 return; // no longer used

/*

 int i;
 int j;

 int x, y;
 int pos;
 unsigned int blocksize = BLOCK_SIZE_PIXELS;
 int dist;
 int range;

 char checked_circle [SENSE_CIRCLES * 2 + 1] [SENSE_CIRCLES * 2 + 1];

 for (i = 0; i < SENSE_CIRCLES * 2 + 1; i ++)
 {
  for (j = 0; j < SENSE_CIRCLES * 2 + 1; j ++)
  {
   checked_circle [i] [j] = 0;
  }
 }

 for (i = 0; i < SENSE_CIRCLES; i ++)
 {
  pos = 0;
  range = (i + 2) * blocksize;
  for (x = -SENSE_CIRCLES; x < SENSE_CIRCLES + 1; x ++)
  {
   for (y = -SENSE_CIRCLES; y < SENSE_CIRCLES + 1; y ++)
   {
    if (checked_circle [x + SENSE_CIRCLES] [y + SENSE_CIRCLES] == 1)
     continue;
    dist = hypot(abs(y * blocksize), abs(x * blocksize));
    if (dist < range)
    {
     sense_circles [i] [pos] [0] = x;
     sense_circles [i] [pos] [1] = y;
     pos ++;
     if (pos >= SENSE_CIRCLE_LENGTH)
     {
      fprintf(stdout, "\nError: g_shape.c: prepare_sense_circles(): pos %i too large (>= %i) i %i dist %i range %i", pos, SENSE_CIRCLE_LENGTH, i, dist, range);
      error_call();
     }
     checked_circle [x + SENSE_CIRCLES] [y + SENSE_CIRCLES] = 1;
    }
   }
  }
  sense_circles [i] [pos] [0] = 127;



 }
*/
// error_call();

}


