
#ifndef H_I_HEADER
#define H_I_HEADER

#include <allegro5/allegro_font.h>

#define PROC_FILL_SHADES 16
#define CLOUD_SHADES 10

struct coloursstruct
{

// These player-related colours can be changed by the MT_OB_VIEW method during gameplay.
 ALLEGRO_COLOR team [PLAYERS] [TCOLS];
 ALLEGRO_COLOR proc_fill [PLAYERS] [PROC_FILL_SHADES] [2];
 ALLEGRO_COLOR packet [PLAYERS] [CLOUD_SHADES];
 ALLEGRO_COLOR drive [PLAYERS] [CLOUD_SHADES];
 ALLEGRO_COLOR virtual_method [PLAYERS] [CLOUD_SHADES];
 ALLEGRO_COLOR world_background;
 ALLEGRO_COLOR back_fill [PLAYERS] [BACK_COL_SATURATIONS] [BACK_COL_FADE];
 ALLEGRO_COLOR back_line [PLAYERS] [BACK_COL_SATURATIONS];

// These interface colours are fixed at startup
 ALLEGRO_COLOR base [BASIC_COLS] [BASIC_SHADES];
 ALLEGRO_COLOR base_trans [BASIC_COLS] [BASIC_SHADES] [BASIC_TRANS];
 ALLEGRO_COLOR base_fade [BASIC_COLS] [CLOUD_SHADES];
 ALLEGRO_COLOR print [PRINT_COLS];
 ALLEGRO_COLOR print_fade [PRINT_COLS] [CONSOLE_LINE_FADE];
 ALLEGRO_COLOR console_background;

 ALLEGRO_COLOR black;
 ALLEGRO_COLOR none;

};

extern struct coloursstruct colours;

struct fontstruct
{
	ALLEGRO_FONT* fnt;
	int width; // fixed width fonts only
	int height; // specified height may be ignored
};

enum
{
FONT_BASIC,
FONT_BASIC_BOLD,
FONT_SQUARE,
FONT_SQUARE_BOLD,
FONT_SQUARE_LARGE,
FONTS
};

// the following are used for the size of display buffers declared in i_display.c and used in one or two other places:
#define POLY_BUFFER 10000
#define POLY_TRIGGER 9900
#define LINE_BUFFER 10000
#define LINE_TRIGGER 9900

#endif
