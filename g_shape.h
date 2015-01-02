
#ifndef H_G_SHAPE
#define H_G_SHAPE

void init_shapes(void);
void prepare_sense_circles(void);
void test_draw_mask(int s, int test_x, int test_y);

enum
{
SHAPE_3TRIANGLE,
SHAPE_4SQUARE,
SHAPE_4DIAMOND,
SHAPE_4POINTY,
SHAPE_4TRAP,
SHAPE_4IRREG_L,
SHAPE_4IRREG_R,
SHAPE_4ARROW,
SHAPE_5PENTAGON,
SHAPE_5POINTY,
SHAPE_5LONG,
SHAPE_5WIDE,
SHAPE_6HEXAGON,
SHAPE_6POINTY,
SHAPE_6LONG,
SHAPE_6IRREG_L,
SHAPE_6IRREG_R,
SHAPE_6ARROW,
SHAPE_6STAR,
SHAPE_8OCTAGON,
SHAPE_8POINTY,
SHAPE_8LONG,
SHAPE_8STAR,
//SHAPE_8BOX,
//SHAPE_8LONG,
SHAPES
};

#define SHAPES_SIZES 4
#define SHAPES_VERTICES 16
// any changes in SHAPES_VERTICES may need to be reflected in the value of METHOD_VERTICES, defined above
#define COLLISION_VERTICES 32

enum
{
EXANGLE_LOW,
EXANGLE_HIGH,
EXANGLE_PREVIOUS,
EXANGLE_NEXT,
EXANGLE_VALUES
};

struct shapestruct
{
// basic geometric properties
 int vertices; // available through data call
 al_fixed vertex_angle [SHAPES_VERTICES]; // available through data call (converted to int)
 int vertex_method_angle_min [SHAPES_VERTICES];
 int vertex_method_angle_max [SHAPES_VERTICES];
 int dist_to_previous [SHAPES_VERTICES];
 int dist_to_next [SHAPES_VERTICES];
 float vertex_angle_float [SHAPES_VERTICES]; // used for display
 int vertex_dist_pixel [SHAPES_VERTICES]; // in pixels - available through data call
 al_fixed vertex_dist [SHAPES_VERTICES]; // from centre of shape
 al_fixed external_angle [EXANGLE_VALUES] [SHAPES_VERTICES]; // details of the directions an external method can face (as they're not allowed to face inwards)
 int external_angle_wrapped [SHAPES_VERTICES]; // is 1 if the high and low external angles are wrapped around 0
 float vertex_notch_inwards [SHAPES_VERTICES]; // depth of the notch thing in the shape display
 float vertex_notch_sidewards [SHAPES_VERTICES]; // notch width

 int collision_vertices;
 al_fixed collision_vertex_angle [COLLISION_VERTICES];
 int collision_vertex_dist_pixels [COLLISION_VERTICES]; // used in collision mask checking
 al_fixed collision_vertex_dist [COLLISION_VERTICES]; // used in collision physics

 al_fixed max_length; // longest radius (from zero point) of any point in the shape (in GRAIN units)
// back_point is the point furthest from vertex 0. It's used for newly created procs, in the display functions.
 float back_point_angle_float; // angle to back point (from vertex 0 and also from centre)
 float back_point_dist_float; // dist from centre to back point
 float back_point_to_front; // dist from back point to vertex 0
// gameplay properties
 int base_hp_max;
 int base_irpt_max;
 int base_data_max;
 int mass_max;
 int shape_mass; // basic mass of shape with no methods
};



struct shape_typestruct
{
 int hp;
// int irpt;
// int data;
// int method_mass;
// int shape_mass;
};



#endif

