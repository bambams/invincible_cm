
#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_native_dialog.h>

#include <stdio.h>
#include <string.h>

#include "m_config.h"

#include "g_header.h"
#include "e_header.h"
#include "e_editor.h"
#include "e_slider.h"
#include "e_inter.h"
#include "m_globvars.h"
#include "m_input.h"
#include "i_input.h"
#include "i_header.h"
#include "e_log.h"

#include "g_misc.h"

#include "f_load.h"
#include "f_save.h"
#include "s_turn.h"

extern struct gamestruct game;
extern struct viewstruct view;


struct progmenustatestruct
{

  ALLEGRO_BITMAP* bmp;
  int panel_x, panel_y;
  int button_highlight;
  int quit_confirm;

//  ALLEGRO_BITMAP* log_sub_bmp;

};

struct progmenustatestruct progmstate;

extern struct editorstruct editor;
extern struct fontstruct font [FONTS];
extern ALLEGRO_DISPLAY* display;

extern struct logstruct mlog; // in e_log.c

static void programs_menu_input(void);
static void draw_program_information(struct programstruct* program, int x, int y);

void init_programs_menu(void)
{

// progmstate.bmp = editor.sub_bmp;
// progmstate.log_sub_bmp = editor.log_sub_bmp;

 progmstate.panel_x = settings.editor_x_split;
 progmstate.panel_y = 0;

}

void open_programs_window(void)
{

 settings.edit_window = EDIT_WINDOW_PROGRAMS;
 settings.keyboard_capture = INPUT_EDITOR;

}


void display_programs_menu(void)
{

// al_set_target_bitmap(al_get_backbuffer(display));
 al_set_clipping_rectangle(editor.panel_x, editor.panel_y, editor.panel_w, editor.panel_h);

 al_clear_to_color(colours.base [COL_BLUE] [SHADE_MIN]);

 al_draw_textf(font[FONT_SQUARE_BOLD].fnt, colours.base [COL_GREY] [SHADE_MAX], editor.panel_x + 5, 2, ALLEGRO_ALIGN_LEFT, "Programs");

#define PROGRAM_DISPLAY_H 50
 int x = editor.panel_x + 40;
 int y = 70;

 if (w.system_program.active == 1)
 {
  al_draw_textf(font[FONT_SQUARE_BOLD].fnt, colours.base [COL_BLUE] [SHADE_MAX], x, y, ALLEGRO_ALIGN_LEFT, "System");
  draw_program_information(&w.system_program, x, y);
  y += PROGRAM_DISPLAY_H;
 }

 if (w.observer_program.active == 1)
 {
  al_draw_textf(font[FONT_SQUARE_BOLD].fnt, colours.base [COL_BLUE] [SHADE_MAX], x, y, ALLEGRO_ALIGN_LEFT, "Observer");
  draw_program_information(&w.observer_program, x, y);
  y += PROGRAM_DISPLAY_H;
 }

 int i;

 for (i = 0; i < w.players; i ++)
 {
  if (w.player[i].active
   && w.player[i].client_program.active)
  {
   if (w.actual_operator_player == i)
    al_draw_textf(font[FONT_SQUARE_BOLD].fnt, colours.base [COL_BLUE] [SHADE_MAX], x, y, ALLEGRO_ALIGN_LEFT, "%s (Operator)", w.player[i].name);
     else
      al_draw_textf(font[FONT_SQUARE_BOLD].fnt, colours.base [COL_BLUE] [SHADE_MAX], x, y, ALLEGRO_ALIGN_LEFT, "%s (Delegate)", w.player[i].name);
   draw_program_information(&w.player[i].client_program, x, y);
   y += PROGRAM_DISPLAY_H;
  }
 }

 display_log(editor.panel_x + EDIT_WINDOW_X, editor.panel_y + editor.mlog_window_y);
 al_set_clipping_rectangle(editor.panel_x, editor.panel_y, editor.panel_w, editor.panel_h);
// al_set_target_bitmap(progmstate.bmp);
 draw_scrollbar(&mlog.scrollbar_v, 0, 0);

// al_set_target_bitmap(al_get_backbuffer(display));
// al_draw_bitmap(progmstate.bmp, progmstate.panel_x, progmstate.panel_y, 0); // TO DO!!!!: need to treat the progmenu bitmap as a subbitmap of the display backbuffer, which would let us save this drawing operation

 if (ex_control.panel_drag_ready == 1
		|| ex_control.mouse_dragging_panel == 1)
		draw_panel_drag_ready_line();


}

static void draw_program_information(struct programstruct* program, int x, int y)
{
 y += 12;
 al_draw_textf(font[FONT_SQUARE_BOLD].fnt, colours.base [COL_BLUE] [SHADE_HIGH], x, y, ALLEGRO_ALIGN_LEFT, "Instructions:");
 al_draw_textf(font[FONT_SQUARE_BOLD].fnt, colours.base [COL_BLUE] [SHADE_HIGH], x + 200, y, ALLEGRO_ALIGN_RIGHT, "%i(%i)", program->instr_left, program->available_instr);
 y += 12;
 al_draw_textf(font[FONT_SQUARE_BOLD].fnt, colours.base [COL_BLUE] [SHADE_HIGH], x, y, ALLEGRO_ALIGN_LEFT, "Interrupts:");
 al_draw_textf(font[FONT_SQUARE_BOLD].fnt, colours.base [COL_BLUE] [SHADE_HIGH], x + 200, y, ALLEGRO_ALIGN_RIGHT, "%i(%i)", program->irpt, program->available_irpt);

}



void run_programs_menu(void)
{

 programs_menu_input();

}

static void programs_menu_input(void)
{

 run_slider(&mlog.scrollbar_v, 0, 0);

 // check for the mouse pointer being in the game window:
 if (ex_control.mouse_x_pixels < settings.editor_x_split)
 {
  if (settings.keyboard_capture == INPUT_EDITOR
   && ex_control.mb_press [0] == BUTTON_JUST_PRESSED)
  {
   initialise_control();
   settings.keyboard_capture = INPUT_WORLD;
  }
// needs work - see also equivalent in e_editor.c and t_template.c
  return;
 }

}

void close_programs_window(void)
{

 settings.edit_window = EDIT_WINDOW_CLOSED;
 settings.keyboard_capture = INPUT_WORLD;


}

