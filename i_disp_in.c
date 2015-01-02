#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>

#include <stdio.h>

#include "m_config.h"
#include "m_globvars.h"

#include "g_header.h"
#include "g_misc.h"
#include "i_disp_in.h"
#include "i_header.h"



extern ALLEGRO_DISPLAY *display;


const int base_col_value [BASIC_COLS] [6] =
{
// first 3 are minimum for base colours. 2nd 3 are maximum.
 {1,1,1, 250,250,250}, // COL_GREY
 {50,5,15, 200,40,80, }, // COL_RED
 {5,50,15, 40,200,80}, // COL_GREEN
 {15,5,50, 120,80,230}, // COL_BLUE
 {40,30,5, 180,170,70}, // COL_YELLOW
 {45,22,5, 190,160,60}, // COL_ORANGE
 {40,5,40, 190,60,170}, // COL_PURPLE

 {5,10,45, 70,120,220}, // COL_TURQUOISE
 {1,20,40, 50,160,180}, // COL_AQUA
 {5,30,50, 30,180,200}, // COL_CYAN
 {15,15,35, 120,120,200}, // COL_DULL
};

const int base_fade_col_value [BASIC_COLS] [6] =
{
// first 3 are minimum for base_fade colours. 2nd 3 are maximum.
 {1,1,1, 250,250,250}, // COL_GREY
 {230,10,5, 250,80,50}, // COL_RED (dark red to yellow)
 {5,230,15, 130,250,60}, // COL_GREEN (dark green to yellow)
 {15,5,230, 130,150,240}, // COL_BLUE (dark blue to white)
 {200,40,5, 230,190,70}, // COL_YELLOW (orange to yellowish-white)
 {210,1,1, 220,150,60}, // COL_ORANGE (deep red to orange)
 {180,5,180, 220,60,220}, // COL_PURPLE (red to purple)

 {5,130,220, 70,150,240}, // COL_TURQUOISE
 {1,130,150, 50,160,180}, // COL_AQUA
 {5,180,200, 30,190,210}, // COL_CYAN
 {120,120,150, 150,150,200}, // COL_DULL
};





const int print_col_value [PRINT_COLS] [3] =
{

 {100,100,100}, // PRINT_COL_DGREY,
 {170,170,170}, // PRINT_COL_LGREY,
 {210,210,210}, // PRINT_COL_WHITE,
 {100,120,180}, // PRINT_COL_LBLUE,
 {70,80,140}, // PRINT_COL_DBLUE,
 {200,130,100}, // PRINT_COL_LRED,
 {150,100,70}, // PRINT_COL_DRED,
 {100,180,110}, // PRINT_COL_LGREEN,
 {70,160,80}, // PRINT_COL_DGREEN,
 {150,100,160}, // PRINT_COL_LPURPLE,
 {120,70,130}, // PRINT_COL_DPURPLE,

};


const int base_team_colours [PLAYERS] [3] =
{

 {60, 170, 210}, // blue-green
 {220, 75, 80}, // red
 {20, 220, 50}, // green
 {200, 200, 200}, // white

};

const int base_packet_colours [PLAYERS] [6] =
{
 {230, 250, // min and max red values
  10, 180, // green
  5, 60}, // blue
 {10, 230, // min and max red values
  10, 240, // green
  230, 250}, // blue
 {230, 250, // min and max red values
  10, 180, // green
  5, 60}, // blue
 {230, 250, // min and max red values
  10, 180, // green
  5, 60} // blue
};

const int base_drive_colours [PLAYERS] [6] =
{
 {230, 250, // min and max red values
  10, 180, // green
  5, 60}, // blue
 {10, 230, // min and max red values
  10, 240, // green
  230, 250}, // blue
 {230, 250, // min and max red values
  10, 180, // green
  5, 60}, // blue
 {230, 250, // min and max red values
  10, 180, // green
  5, 60} // blue
};




//void init_cloud_graphics(void);
//void map_cloud_cols(int col, int rmin, int rmax, int gmin, int gmax, int bmin, int bmax);
//void fade_circles(int centre_x, int centre_y, int col, int size, int core_size);
//void fade_circles2(int centre_x, int centre_y, int col, int size);
//void draw_to_clouds_bmp(ALLEGRO_BITMAP* source_bmp, int cloud_index, int* draw_x, int* draw_y, int source_x, int source_y, int width, int height, int* row_height);

static void map_cloud_cols(int p, int packet_or_drive, int min_col [3], int max_col [3]);
static void map_virtual_cols(int p, int min_col [3], int max_col [3]);
static void colour_fraction(int base_cols [3], int out_cols [3], int fraction, int subtract);
static int check_col(int col_in);


//void init_packet_bmp(void);
static ALLEGRO_COLOR map_rgb(int r, int g, int b);
static ALLEGRO_COLOR map_rgba(int r, int g, int b, int a);


struct coloursstruct colours;

extern ALLEGRO_BITMAP* display_window; // in i_display.c


// Much of the display initialisation is done in main.c
void initialise_display(void)
{



 int i;

 int j;
 int k;
 int r_prop, g_prop, b_prop;

 for (i = 0; i < BASIC_COLS; i ++)
 {
  r_prop = (base_col_value [i] [3] - base_col_value [i] [0]) / BASIC_SHADES;
  g_prop = (base_col_value [i] [4] - base_col_value [i] [1]) / BASIC_SHADES;
  b_prop = (base_col_value [i] [5] - base_col_value [i] [2]) / BASIC_SHADES;
  for (j = 0; j < BASIC_SHADES; j ++)
  {
   colours.base [i] [j] = map_rgb(base_col_value [i] [0] + (r_prop * j),
                                  base_col_value [i] [1] + (g_prop * j),
                                  base_col_value [i] [2] + (b_prop * j));//, 120);
   for (k = 0; k < 3; k ++)
			{
    colours.base_trans [i] [j] [k] = map_rgba(base_col_value [i] [0] + (r_prop * j),
                                  base_col_value [i] [1] + (g_prop * j),
                                  base_col_value [i] [2] + (b_prop * j),
																																		45 + k * 90);
			}
  }
  r_prop = (base_fade_col_value [i] [3] - base_fade_col_value [i] [0]) / CLOUD_SHADES;
  g_prop = (base_fade_col_value [i] [4] - base_fade_col_value [i] [1]) / CLOUD_SHADES;
  b_prop = (base_fade_col_value [i] [5] - base_fade_col_value [i] [2]) / CLOUD_SHADES;
  for (j = 0; j < CLOUD_SHADES; j ++)
  {
   colours.base_fade [i] [j] = map_rgba(base_fade_col_value [i] [0] + (r_prop * j),
                                       base_fade_col_value [i] [1] + (g_prop * j),
                                       base_fade_col_value [i] [2] + (b_prop * j),
																																							10 + ((211 * j) / CLOUD_SHADES));
  }
 }

 colours.black = map_rgb(0,0,0);
 colours.none = map_rgba(0,0,0,0);

 set_default_modifiable_colours();

 al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA);

// work out where the mode buttons go (based on screen size, which cannot change during execution)
 for (i = 0; i < MODE_BUTTONS; i ++)
 {
   settings.mode_button_x [i] = settings.option [OPTION_WINDOW_W] - ((MODE_BUTTON_SPACING + MODE_BUTTON_SIZE) * MODE_BUTTONS) + ((MODE_BUTTON_SPACING + MODE_BUTTON_SIZE) * i);
   settings.mode_button_y [i] = MODE_BUTTON_Y;
 }



 for (i = 0; i < PRINT_COLS; i ++)
 {
   colours.print [i] = map_rgb(print_col_value [i] [0],
                               print_col_value [i] [1],
                               print_col_value [i] [2]);
 }

#define CONSOLE_BACKGROUND_R 10
#define CONSOLE_BACKGROUND_G 10
#define CONSOLE_BACKGROUND_B 30

 colours.console_background = map_rgba(CONSOLE_BACKGROUND_R, CONSOLE_BACKGROUND_G, CONSOLE_BACKGROUND_B, 100);

 for (i = 0; i < PRINT_COLS; i ++)
 {
  int r_top = print_col_value [i] [0] / 2;
  int g_top = print_col_value [i] [1] / 2;
  int b_top = print_col_value [i] [2] / 2;
  for (j = 0; j < CONSOLE_LINE_FADE; j ++)
  {
   r_prop = (((r_top - CONSOLE_BACKGROUND_R) * j) / CONSOLE_LINE_FADE) + CONSOLE_BACKGROUND_R;
   g_prop = (((g_top - CONSOLE_BACKGROUND_G) * j) / CONSOLE_LINE_FADE) + CONSOLE_BACKGROUND_G;
   b_prop = (((b_top - CONSOLE_BACKGROUND_B) * j) / CONSOLE_LINE_FADE) + CONSOLE_BACKGROUND_B;
   colours.print_fade [i] [j] = map_rgba(r_prop, g_prop, b_prop, (j+1) * 6);
  }

 }

}


void set_default_modifiable_colours(void)
{

 w.background_colour [0] = 1;
	w.background_colour [1] = 1;
	w.background_colour [2] = 1;
 map_background_colour();
	w.hex_base_colour [0] = 80; // hex_base_colour is mapped in map_player_base_colours
	w.hex_base_colour [1] = 80;
	w.hex_base_colour [2] = 80;

 int p, i;

 for (p = 0; p < PLAYERS; p ++)
 {
  for (i = 0; i < 3; i ++)
  {
// for default proc colours, max and min are the same (only alpha varies with intensity)
   w.player[p].proc_colour_min [i] = base_team_colours [p] [i];
   w.player[p].proc_colour_max [i] = base_team_colours [p] [i];
// for packet and drive colours, max and min are different:
   w.player[p].packet_colour_min [i] = base_packet_colours [p] [i * 2];
   w.player[p].packet_colour_max [i] = base_packet_colours [p] [(i * 2) + 1];
   w.player[p].drive_colour_min [i] = base_drive_colours [p] [i * 2];
   w.player[p].drive_colour_max [i] = base_drive_colours [p] [(i * 2) + 1];
  }
  map_player_base_colours(p);
  map_player_packet_colours(p);
  map_player_drive_colours(p);
  map_player_virtual_colours(p);
 }

}

void map_background_colour(void)
{

	 colours.world_background = map_rgb(w.background_colour [0],
																												         w.background_colour [1],
																												         w.background_colour [2]);
}

// This function sets up player p's proc and team colours (but not packet or drive colours)
// It is called by init functions in this file, which set the colours to default values,
//  and also by MT_OB_VIEW method functions in g_method_clob (which allow the observer to change the colours)
// It assumes that the player[p].proc_colour_min and proc_colour_max values have been set up
void map_player_base_colours(int p)
{

 int col [3]; // passed to colour_fraction to store results
 int base_cols [3];
 int max_cols [3];
 int a; // alpha
 int j;

 base_cols [0] = w.player[p].proc_colour_min [0];
 base_cols [1] = w.player[p].proc_colour_min [1];
 base_cols [2] = w.player[p].proc_colour_min [2];
 max_cols [0] = w.player[p].proc_colour_max [0];
 max_cols [1] = w.player[p].proc_colour_max [1];
 max_cols [2] = w.player[p].proc_colour_max [2];


// colour_fraction(base_cols, col, 100, 10);

 for (j = 0; j < PROC_FILL_SHADES; j ++)
 {
  colours.proc_fill [p] [j] [0] = map_rgba(base_cols [0] + ((max_cols [0] - base_cols [0]) * j) / PROC_FILL_SHADES,
                                           base_cols [1] + ((max_cols [1] - base_cols [1]) * j) / PROC_FILL_SHADES,
                                           base_cols [2] + ((max_cols [2] - base_cols [2]) * j) / PROC_FILL_SHADES,
                                           40 + (j * 10));
  colours.proc_fill [p] [j] [1] = map_rgba((base_cols [0] + ((max_cols [0] - base_cols [0]) * j) / PROC_FILL_SHADES) * 1.4,
                                           (base_cols [1] + ((max_cols [1] - base_cols [1]) * j) / PROC_FILL_SHADES) * 1.4,
                                           (base_cols [2] + ((max_cols [2] - base_cols [2]) * j) / PROC_FILL_SHADES) * 1.4,
                                           50 + (j * 10));

//  colours.proc_fill [p] [j] [0] = map_rgba(col [0], col [1], col [2], 40 + (j * 10));
//  colours.proc_fill [p] [j] [1] = map_rgba(col [0] * 1.3, col [1] * 1.3, col [2] * 1.3, 40 + (j * 10));
 }

 map_hex_colours(p);

 colour_fraction(base_cols, col, 100, 10);
 a = 180;
 colours.team [p] [TCOL_FILL_BASE] = map_rgba(col [0] / 1, col [1] / 1, col [2] / 1, a / 2);
 colours.team [p] [TCOL_MAIN_EDGE] = map_rgba(col [0] / 2, col [1] / 2, col [2] / 2, a);
 colours.team [p] [TCOL_METHOD_EDGE] = map_rgba(col [0] / 4, col [1] / 4, col [2] / 4, a / 2);
// colours.team [p] [TCOL_MAIN_EDGE] = map_rgba(200, 200, 200,200);//col [0] / 2, col [1] / 2, col [2] / 2, a);
// colours.team [p] [TCOL_METHOD_EDGE] = map_rgba(200, 200, 200,200);//map_rgba(col [0] / 4, col [1] / 4, col [2] / 4, a / 2);

// the box colours are colours for score boxes and proc info boxes for each player
 colour_fraction(base_cols, col, 50, 0);
 a = 50;
 colours.team [p] [TCOL_BOX_FILL] = map_rgba(col [0], col [1], col [2], a);

 colour_fraction(base_cols, col, 45, -10);
 a = 100;
 colours.team [p] [TCOL_BOX_HEADER_FILL] = map_rgba(col [0], col [1], col [2], a);


// From this point we set colours that can't be too dark (e.g. because text and map points need to be
//  visible against the backgrounds).
// Need to increase brightness if too dark:

 int base_col_proportion;

// need to avoid divide-by-zero:
 if (base_cols [0] <= 0)
	 base_cols [0] = 1;
 if (base_cols [1] <= 0)
	 base_cols [1] = 1;
 if (base_cols [2] <= 0)
	 base_cols [2] = 1;

 if (base_cols [0] >= base_cols [1]
		&& base_cols [0] >= base_cols [2])
	{
		base_col_proportion = 2000 / base_cols [0];
	}
	 else
		{
			if (base_cols [1] >= base_cols [2])
				base_col_proportion = 2000 / base_cols [1];
			  else
				  base_col_proportion = 2000 / base_cols [2];
		}

 base_cols [0] = (base_cols [0] * base_col_proportion) / 10;
 base_cols [1] = (base_cols [1] * base_col_proportion) / 10;
 base_cols [2] = (base_cols [2] * base_col_proportion) / 10;


 colours.team [p] [TCOL_MAP_POINT] = map_rgb(base_cols [0], base_cols [1], base_cols [2]);

// the box colours are colours for score boxes and proc info boxes for each player
 colour_fraction(base_cols, col, 30, -10);
 a = 150;
 colours.team [p] [TCOL_BOX_BUTTON] = map_rgba(col [0], col [1], col [2], a);

 colour_fraction(base_cols, col, 80, -20);
 a = 100;
 colours.team [p] [TCOL_BOX_OUTLINE] = map_rgba(col [0], col [1], col [2], a);

// colour_fraction(base_cols, col, 80, -10);
 colour_fraction(base_cols, col, 85, -20);
 a = 150;
 colours.team [p] [TCOL_BOX_TEXT] = map_rgba(col [0], col [1], col [2], a);

 colour_fraction(base_cols, col, 60, 0);
 a = 140;
 colours.team [p] [TCOL_BOX_TEXT_FAINT] = map_rgba(col [0], col [1], col [2], a);

 colour_fraction(base_cols, col, 90, -30);
 a = 160;
 colours.team [p] [TCOL_BOX_TEXT_BOLD] = map_rgba(col [0], col [1], col [2], a);

// Remember that base_cols has been fixed at this point to brighten very dark colours

/*
fields:
 [4 = one for each team] [5 = saturation] [8 = fade]

*/


}

void map_hex_colours(int p)
{


 int base_r, base_g, base_b;
 int team_r, team_g, team_b;
 int k, j;

 int base_cols [3];

 base_cols [0] = w.player[p].proc_colour_min [0];
 base_cols [1] = w.player[p].proc_colour_min [1];
 base_cols [2] = w.player[p].proc_colour_min [2];

 base_r = w.hex_base_colour [0];
 base_g = w.hex_base_colour [1];
 base_b = w.hex_base_colour [2];

 team_r = base_cols [0] / 6;
 team_g = base_cols [1] / 6;
 team_b = base_cols [2] / 6;

 for (j = 0; j < BACK_COL_SATURATIONS; j ++)
 {
  colours.back_line [p] [j] = map_rgba(base_r + (team_r * (j+0)) - (20),
                                           base_g + (team_g * (j+0)) - (20),
                                           base_b + (team_b * (j+0)) - (20),
                                           60);
//                                           120);
  for (k = 0; k < BACK_COL_FADE; k ++)
  {
   colours.back_fill [p] [j] [k] = map_rgba((base_r + (team_r * (j*2)) + (k * 4)) / 3,
                                           (base_g + (team_g * (j*2)) + (k * 4)) / 3,
                                           (base_b + (team_b * (j*2)) + (k * 4)) / 3,
                                           60 + k * 8);

  }
 }

}


void map_player_packet_colours(int p)
{

   map_cloud_cols(p, 0, w.player[p].packet_colour_min, w.player[p].packet_colour_max);

}

void map_player_drive_colours(int p)
{

   map_cloud_cols(p, 1, w.player[p].drive_colour_min, w.player[p].drive_colour_max);

}

void map_player_virtual_colours(int p)
{

   map_virtual_cols(p, w.player[p].packet_colour_min, w.player[p].packet_colour_max);

}


static void map_cloud_cols(int p, int packet_or_drive, int min_col [3], int max_col [3])
{

  int i;
  int r, g, b;//, a;

  for (i = 0; i < CLOUD_SHADES; i ++)
  {
   r = min_col [0] + (((max_col [0] - min_col [0]) * i) / CLOUD_SHADES);
   g = min_col [1] + (((max_col [1] - min_col [1]) * i) / CLOUD_SHADES);
   b = min_col [2] + (((max_col [2] - min_col [2]) * i) / CLOUD_SHADES);
//   a = 10 + ((211 * i) / CLOUD_SHADES);

   if (packet_or_drive == 0)
    colours.packet [p] [i] = map_rgba(r, g, b, 10 + ((211 * i) / CLOUD_SHADES));
     else
      colours.drive [p] [i] = map_rgba(r, g, b, 50 + ((171 * i) / CLOUD_SHADES));
 }

}

static void map_virtual_cols(int p, int min_col [3], int max_col [3])
{

  int i;
  int r, g, b, a;

  for (i = 0; i < CLOUD_SHADES; i ++)
  {
   r =  min_col [0] + (((max_col [0] - min_col [0]) * i) / (CLOUD_SHADES * 1));
   g =  min_col [1] + (((max_col [1] - min_col [1]) * i) / (CLOUD_SHADES * 1));
   b =  min_col [2] + (((max_col [2] - min_col [2]) * i) / (CLOUD_SHADES * 1));
   a = 10 + ((100 * i) / CLOUD_SHADES);

   colours.virtual_method [p] [i] = map_rgba(r, g, b, a);
 }

}



static ALLEGRO_COLOR map_rgb(int r, int g, int b)
{

 if (r < 0)
  r = 0;
 if (r > 255)
  r = 255;
 if (g < 0)
  g = 0;
 if (g > 255)
  g = 255;
 if (b < 0)
  b = 0;
 if (b > 255)
  b = 255;

 return al_map_rgb(r, g, b);

}

static ALLEGRO_COLOR map_rgba(int r, int g, int b, int a)
{

 if (r < 0)
  r = 0;
 if (r > 255)
  r = 255;
 if (g < 0)
  g = 0;
 if (g > 255)
  g = 255;
 if (b < 0)
  b = 0;
 if (b > 255)
  b = 255;
 if (a < 0)
  a = 0;
 if (a > 255)
  a = 255;


 return al_map_rgba(r, g, b, a);

}




static void colour_fraction(int base_cols [3], int out_cols [3], int fraction, int subtract)
{

 out_cols [0] = check_col(((base_cols [0] * fraction) / 100) - subtract);
 out_cols [1] = check_col(((base_cols [1] * fraction) / 100) - subtract);
 out_cols [2] = check_col(((base_cols [2] * fraction) / 100) - subtract);

}

static int check_col(int col_in)
{
 if (col_in < 0)
  col_in = 0;
 if (col_in > 255)
  col_in = 255;
 return col_in;
}




