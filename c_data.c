#include <allegro5/allegro.h>
#include "m_config.h"
#include "g_header.h"
#include "c_header.h"

#include "m_maths.h"
//#include "g_misc.h"
//#include "c_prepr.h"
#include "c_ctoken.h"
#include "c_comp.h"
#include "g_shape.h"
//#include "c_init.h"

#include "c_data.h"

//#include "m_globvars.h"

#include <stdlib.h>
#include <stdio.h>

extern struct shapestruct shape_dat [SHAPES] [SHAPES_SIZES]; // defined in g_shape.c


// The maximum number of parameters (not including the CDATA_x data type) accepted
#define MAX_DATA_PARAMETERS 5

static int verify_data_shape_vertex(int shape, int vertex);
static int verify_data_shape_size(int size_value);
static int verify_data_shape(int shape_value);


// call this just after reading "data".
// it will try to resolve the data call into a number, and if successful will turn ctoken into a number ctoken with that value.
// returns 1 success/0 failure
int parse_data(struct ctokenstruct* ctoken)
{

  if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACKET_OPEN))
   return comp_error(CERR_EXPECTED_BRACKET_OPEN, ctoken);

// the next ctoken should be a number indicating the type of data value we're after:
  struct ctokenstruct data_ctoken;
// We use the data type information to work out how many further parameters to expect:
  struct ctokenstruct param_ctoken;
  int expect_params = 0;
  int data_param [MAX_DATA_PARAMETERS];
  int i;

  strcpy(data_ctoken.name, "(data)");

  if (!read_next(&data_ctoken))
   return 0;

  if (data_ctoken.type != CTOKEN_TYPE_NUMBER)
   return comp_error(CERR_DATA_PARAMETER_NOT_NUMBER, &data_ctoken);

  switch(data_ctoken.number_value)
  {
   case CDATA_SHAPE_VERTICES:
    expect_params = 1; break;
   case CDATA_SHAPE_VERTEX_ANGLE:
    expect_params = 2; break;
   case CDATA_SHAPE_VERTEX_DIST:
    expect_params = 3; break;
   case CDATA_SHAPE_VERTEX_ANGLE_PREV:
   case CDATA_SHAPE_VERTEX_ANGLE_NEXT:
   case CDATA_SHAPE_VERTEX_ANGLE_MIN:
   case CDATA_SHAPE_VERTEX_ANGLE_MAX:
    expect_params = 2; break;

// If number is known at this point, could just accept closing bracket then return here.

   default:
    return comp_error(CERR_DATA_UNRECOGNISED_DATA_TYPE, &data_ctoken);
  }

  for (i = 0; i < expect_params; i ++)
  {
   if (check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACKET_CLOSE)) // too few parameters
    return comp_error(CERR_DATA_NOT_ENOUGH_PARAMETERS, NULL);
   if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_COMMA))
    return comp_error(CERR_EXPECTED_COMMA_AFTER_VALUE, NULL);
   if (!read_next(&param_ctoken))
    return 0;
   if (param_ctoken.type != CTOKEN_TYPE_NUMBER)
    return comp_error(CERR_DATA_PARAMETER_NOT_NUMBER, &param_ctoken);
   data_param [i] = param_ctoken.number_value;
  }

// now we know what kind of information we're seeking:
  int data_value = 0;

  switch(data_ctoken.number_value)
  {
   case CDATA_SHAPE_VERTICES: // fields: shape
    if (!verify_data_shape(data_param [0]))
     return 0;
    data_value = shape_dat[data_param [0]] [0].vertices;
    break;
   case CDATA_SHAPE_VERTEX_ANGLE: // fields: shape, vertex
    if (!verify_data_shape(data_param [0]))
     return 0;
    if (!verify_data_shape_vertex(data_param [0], data_param [1]))
     return 0;
    data_value = fixed_angle_to_short(shape_dat[data_param [0]] [0].vertex_angle [data_param [1]]);
    break;
   case CDATA_SHAPE_VERTEX_DIST: // fields: shape, size, vertex
    if (!verify_data_shape(data_param [0]))
     return 0;
    if (!verify_data_shape_size(data_param [1]))
     return 0;
    if (!verify_data_shape_vertex(data_param [0], data_param [2]))
     return 0;
    data_value = shape_dat[data_param [0]] [data_param [1]].vertex_dist_pixel [data_param [2]];
    break;
   case CDATA_SHAPE_VERTEX_ANGLE_PREV: // fields: shape, vertex
    if (!verify_data_shape(data_param [0]))
     return 0;
    if (!verify_data_shape_vertex(data_param [0], data_param [1]))
     return 0;
    data_value = fixed_angle_to_int(shape_dat[data_param [0]] [0].external_angle [EXANGLE_PREVIOUS] [data_param [1]]);
    break;
   case CDATA_SHAPE_VERTEX_ANGLE_NEXT: // fields: shape, vertex
    if (!verify_data_shape(data_param [0]))
     return 0;
    if (!verify_data_shape_vertex(data_param [0], data_param [1]))
     return 0;
    data_value = fixed_angle_to_int(shape_dat[data_param [0]] [0].external_angle [EXANGLE_NEXT] [data_param [1]]);
    break;
   case CDATA_SHAPE_VERTEX_ANGLE_MIN: // fields: shape, vertex
    if (!verify_data_shape(data_param [0]))
     return 0;
    if (!verify_data_shape_vertex(data_param [0], data_param [1]))
     return 0;
    data_value = shape_dat[data_param [0]] [0].vertex_method_angle_min [data_param [1]];
    /*
    if (shape_dat [data_param [0]] [0].external_angle_wrapped [data_param [1]] == 0)
     data_value = fixed_angle_to_int(shape_dat[data_param [0]] [0].external_angle [EXANGLE_LOW] [data_param [1]]) - ANGLE_1;
      else
       data_value = fixed_angle_to_int(shape_dat[data_param [0]] [0].external_angle [EXANGLE_HIGH] [data_param [1]]) - ANGLE_1;*/
    break;
   case CDATA_SHAPE_VERTEX_ANGLE_MAX: // fields: shape, vertex
    if (!verify_data_shape(data_param [0]))
     return 0;
    if (!verify_data_shape_vertex(data_param [0], data_param [1]))
     return 0;
    data_value = shape_dat[data_param [0]] [0].vertex_method_angle_max [data_param [1]];
/*    if (shape_dat [data_param [0]] [0].external_angle_wrapped [data_param [1]] == 0)
     data_value = fixed_angle_to_int(shape_dat[data_param [0]] [0].external_angle [EXANGLE_HIGH] [data_param [1]]);
      else
       data_value = fixed_angle_to_int(shape_dat[data_param [0]] [0].external_angle [EXANGLE_LOW] [data_param [1]]);*/
    break;

// no need for default case as that's been checked above.
  }

// read closing bracket:
   if (!check_next(CTOKEN_TYPE_PUNCTUATION, CTOKEN_SUBTYPE_BRACKET_CLOSE))
    return comp_error(CERR_EXPECTED_BRACKET_CLOSE, NULL);

// convert the "data" ctoken into a number, and return successfully:
  ctoken->number_value = data_value;
  ctoken->type = CTOKEN_TYPE_NUMBER;
  return 1;

}

// confirms that shape_value is a valid shape
// returns 1 on success, comp_error on failure
static int verify_data_shape(int shape_value)
{

 if (shape_value < 0
  || shape_value >= SHAPES)
   return comp_error(CERR_DATA_INVALID_SHAPE, NULL);

 return 1;

}

// confirms that size_value is a valid size
// returns 1 on success, comp_error on failure
static int verify_data_shape_size(int size_value)
{

 if (size_value < 0
  || size_value >= SHAPES_SIZES)
   return comp_error(CERR_DATA_INVALID_SIZE, NULL);

 return 1;

}

// confirms that vertex is a valid vertex for shape
// assumes that shape is a valid shape (should have called verify_data_shape() first)
// returns 1 on success, comp_error on failure
static int verify_data_shape_vertex(int shape, int vertex)
{
 if (vertex < 0
  || vertex >= shape_dat[shape] [0].vertices)
   return comp_error(CERR_DATA_INVALID_VERTEX, NULL);

 return 1;
}
