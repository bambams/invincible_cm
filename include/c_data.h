
#ifndef H_C_DATA
#define H_C_DATA

int parse_data(struct ctokenstruct* ctoken);

enum
{
// CDATA enums are the numbers used in the first parameter of a data() reference. They correspond to DAT_ enums

// SHAPE data calls are used to get information about shapes.
//  TO DO: If the shape parameter is -1 when data() is used in a process, should give data for own shape as defined in interface.
CDATA_SHAPE_VERTICES, // further parameters: shape
CDATA_SHAPE_VERTEX_ANGLE, // further parameters: shape, vertex
CDATA_SHAPE_VERTEX_DIST, // further parameters: shape, size, vertex
CDATA_SHAPE_VERTEX_ANGLE_PREV, // angle to previous vertex - further parameters: shape, vertex
CDATA_SHAPE_VERTEX_ANGLE_NEXT, // angle to next vertex - further parameters: shape, vertex
CDATA_SHAPE_VERTEX_ANGLE_MIN, // minimum angle offset (same as angle offset used for eg dpacket) - further parameters: shape, vertex
CDATA_SHAPE_VERTEX_ANGLE_MAX, // maximum angle offset (same as angle offset used for eg dpacket) - further parameters: shape, vertex
// when adding a CDATA value, need to add a number of expected parameters to parse_cdata() in c_data.c

};


#endif
