
#ifndef H_M_MATHS
#define H_M_MATHS



void init_trig(void);

int xpart(int angle, int length);
int ypart(int angle, int length);

float fxpart(float angle, float length);
float fypart(float angle, float length);

/*
inline float fxpart(float angle, float length);
inline float fypart(float angle, float length);

inline float fxpart(float angle, float length)
{
 return (cos(angle) * length);
}

inline float fypart(float angle, float length)
{
 return (sin(angle) * length);
}
*/


float angle_to_radians(int angle);
int radians_to_angle(double angle);

int angle_difference_int(int a1, int a2);
int angle_difference_signed(int a1, int a2);

int get_angle8(int y, int x);
int get_angle_coord(int x, int y, int x2, int y2);

int delta_turn_towards_angle(int angle, int tangle, int turning);

void init_nearby_distance(void);
int nearby_distance(int xa, int ya, int xb, int yb);

al_fixed fixed_xpart(al_fixed angle, al_fixed length);
al_fixed fixed_ypart(al_fixed angle, al_fixed length);

al_fixed distance(al_fixed y, al_fixed x);
al_fixed get_angle(al_fixed y, al_fixed x);
void fix_fixed_angle(al_fixed* fix_angle);
al_fixed get_fixed_fixed_angle(al_fixed fix_angle);
al_fixed angle_difference(al_fixed a1, al_fixed a2);
float fixed_to_radians(al_fixed fix_angle);

int get_angle_int(int y, int x);
int fixed_to_block(al_fixed fix_num);
al_fixed block_to_fixed(int block);
s16b fixed_angle_to_short(al_fixed fix_angle);
al_fixed short_angle_to_fixed(s16b short_angle);
al_fixed int_angle_to_fixed(int int_angle);
int fixed_angle_to_int(al_fixed fix_angle);


#endif
