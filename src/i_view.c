#include <allegro5/allegro.h>

#include <stdio.h>

#include "m_config.h"

#include "g_header.h"
#include "i_header.h"

#include "g_misc.h"
#include "m_globvars.h"
#include "m_maths.h"
#include "i_view.h"
#include "e_inter.h"
#include "t_template.h"



struct viewstruct view;

extern ALLEGRO_DISPLAY* display; // in i_display.c
extern ALLEGRO_BITMAP* display_window; // in i_display.c

void reset_view_values(int window_w, int window_h);


void init_view_at_startup(int window_w, int window_h)
{
/*
   display_window = al_create_sub_bitmap(al_get_backbuffer(display), 0, 0, window_w, window_h);

   if (display_window == NULL)
   {
    fprintf(stdout, "\nError: i_view.c: init_view_at_startup(): couldn't create display_window sub-bitmap");
    error_call();
   }
*/

 reset_view_values(settings.option [OPTION_WINDOW_W], settings.option [OPTION_WINDOW_H]);

}


// call this when a new world is started
void initialise_view(int window_w, int window_h)
{

 reset_view_values(window_w, window_h);

// view.paused = 0;
// view.pause_advance_pressed = 0;

 view.w_x_pixel = al_fixtoi(w.w_fixed); // width of window in pixels
 view.w_y_pixel = al_fixtoi(w.h_fixed);
 view.camera_x_min = BLOCK_SIZE_FIXED * 2;
 view.camera_y_min = BLOCK_SIZE_FIXED * 2;
 view.camera_x_max = w.w_fixed - (BLOCK_SIZE_FIXED * 2);
 view.camera_y_max = w.h_fixed - (BLOCK_SIZE_FIXED * 2);

// these map values are likely to be overwritten by the observer program
 view.map_visible = 0;
 view.map_x = 500;
 view.map_y = 400;
 view.map_w = 250;
 view.map_h = 250;
 view.map_proportion_x = al_fixdiv(al_itofix(view.map_w), w.w_fixed);
 view.map_proportion_y = al_fixdiv(al_itofix(view.map_h), w.h_fixed);

// as are these proc data window values:
 view.focus_proc = NULL;
// view.data_window_x = 640;
// view.data_window_y = 50;

}


// call this anytime the display window needs to be resized.
void resize_display_window(int window_w, int window_h)
{

   reset_view_values(window_w, window_h);
   change_edit_panel_width();
   reset_template_x_values();

}

void reset_view_values(int window_w, int window_h)
{

   view.just_resized = 1;
   view.window_x = window_w;
   view.window_y = window_h;
   view.centre_x = al_itofix(view.window_x / 2);
   view.centre_y = al_itofix(view.window_y / 2);
//   view.x_block = fixed_to_block(view.camera_x + view.centre_x);
//   view.y_block = fixed_to_block(view.camera_y + view.centre_y);

/*
   view.map_x = 500;
   view.map_y = 400;
   view.map_w = 250;
   view.map_h = 250;
   view.map_proportion_x = view.map_w / (float) (w.w_grain);
   view.map_proportion_y = view.map_h / (float) (w.h_grain);*/

}

