
#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>

#include <stdio.h>
#include <string.h>

#include "m_config.h"

#include "g_header.h"
#include "c_header.h"
#include "e_slider.h"
#include "e_header.h"
#include "m_globvars.h"

#include "g_misc.h"
#include "i_header.h"


extern struct fontstruct font [FONTS];

extern struct editorstruct editor; // defined in e_editor.c

ALLEGRO_COLOR mlog_col [MLOG_COLS];

/*

This file contains functions for running the compiler mlog.
It's mostly based on i_console.c but with some differences

*/

struct logstruct mlog;


void write_to_log(const char* str);
void start_log_line(int mcol);
void finish_log_line(void);
void reset_log(void);

// call this at startup, or if the size of the log window changes for some reason
void init_log(int w_pixels, int h_pixels)
{
 mlog.h_lines = h_pixels / LOG_LINE_HEIGHT;
 mlog.w_letters = w_pixels / LOG_LETTER_WIDTH;
 mlog.h_pixels = h_pixels;
 mlog.w_pixels = w_pixels;

 mlog.window_pos = LOG_LINES - mlog.h_lines;//27;//mlog.h_lines - 1;

// fprintf(stdout, "\nh_lines %i (pix %i def %i)", mlog.h_lines, h_pixels, LOG_LINE_LENGTH);
// error_call();

 reset_log();


// init_slider(&mlog.scrollbar_v, &mlog.window_pos, SLIDEDIR_VERTICAL, 0, LOG_LINES - mlog.h_lines, h_pixels, 1, mlog.h_lines - 1, mlog.h_lines, EDIT_WINDOW_X + game.editor_x_split + editor.edit_window_w, EDIT_WINDOW_Y + editor.edit_window_h + SLIDER_BUTTON_SIZE, SLIDER_BUTTON_SIZE);
 init_slider(&mlog.scrollbar_v, &mlog.window_pos, SLIDEDIR_VERTICAL, 0, LOG_LINES - mlog.h_lines, h_pixels, 1, mlog.h_lines - 1, mlog.h_lines, EDIT_WINDOW_X + settings.editor_x_split + editor.edit_window_w, editor.mlog_window_y, SLIDER_BUTTON_SIZE, COL_BLUE, 0);

 mlog_col [MLOG_COL_EDITOR] = al_map_rgb(120, 120, 220);
 mlog_col [MLOG_COL_TEMPLATE] = al_map_rgb(150, 100, 200);
 mlog_col [MLOG_COL_COMPILER] = al_map_rgb(100, 150, 200);
 mlog_col [MLOG_COL_FILE] = al_map_rgb(100, 200, 150);
 mlog_col [MLOG_COL_ERROR] = al_map_rgb(200, 120, 120);
 mlog_col [MLOG_COL_HELP] = al_map_rgb(200, 200, 200);




}

void reset_log(void)
{

 mlog.lpos = 0;
 mlog.window_pos = LOG_LINES - mlog.h_lines;//27;//mlog.h_lines - 1;

 int i;

 for (i = 0; i < LOG_LINES; i ++)
 {
  mlog.log_line[i].used = 0;
  mlog.log_line[i].text [0] = 0;
  mlog.log_line[i].source = -1;
  mlog.log_line[i].source_line = -1;
 }

}

// str should be null terminated, although no more than mlog.w_letters will be used anyway
void write_to_log(const char* str)
{

/*
// if the new text comes from a different source to the current line, make a new line even if we haven't been told to do so
 if (mlog.log_line[mlog.lpos].source != source
  || mlog.log_line[mlog.lpos].source_line != source_line)
 {
  finish_log_line();
  start_log_line(source, source_line);
 }*/

 int current_length = strlen(mlog.log_line[mlog.lpos].text);

 int space_left = mlog.w_letters - current_length;

 if (space_left <= 1) // leave 1 for null character at end
  return;

 int i = current_length;
 int j = 0;
 int counter = 160; // maximum number of characters printed at once

 while(str [j] != '\0'
    && counter > 0)
 {
  if (i >= mlog.w_letters - 2)
   break;
/*
  if (str[j] == '\n' || str[j] == '\r' || i >= mlog.w_letters - 1)
  {
   mlog.log_line[mlog.lpos].text [i] = '\0';
   finish_log_line();
   start_log_line(source, source_line);
   i = 0;
   j ++;
   continue;
  }*/
  mlog.log_line[mlog.lpos].text [i] = str [j];
  j ++;
  i ++;
  counter --;
 };

 mlog.log_line[mlog.lpos].text [i] = '\0';
 mlog.log_line[mlog.lpos].text [mlog.w_letters - 1] = '\0';

}



void write_number_to_log(int num) //, int source, int source_line)
{
/*
// if the new text comes from a different source to the current line, make a new line even if we haven't been told to do so
 if (mlog.log_line[mlog.lpos].source != source
  || mlog.log_line[mlog.lpos].source_line != source_line)
 {
  finish_log_line();
  start_log_line(source, source_line);
 }*/

 int current_length = strlen(mlog.log_line[mlog.lpos].text);

 int space_left = mlog.w_letters - current_length;

 char num_str [10];

 snprintf(num_str, 9, "%i", num);

 if (space_left <= strlen(num_str) + 1) // make sure number will fit on line
  return;

 strcat(mlog.log_line[mlog.lpos].text, num_str);


}


void start_log_line(int mcol)//int source, int source_line)
{

 mlog.log_line[mlog.lpos].text [0] = '\0';

 mlog.log_line[mlog.lpos].used = 1;
// mlog.log_line[mlog.lpos].source = source;
// mlog.log_line[mlog.lpos].source_line = source_line;
 mlog.log_line[mlog.lpos].colour = mcol;

}


void finish_log_line(void)
{

 mlog.lpos ++;
 if (mlog.lpos >= LOG_LINES)
  mlog.lpos = 0;

}

// a wrapper around other log functions that writes a whole line at once
void write_line_to_log(char* str, int mcol)
{
 start_log_line(mcol);
 write_to_log(str);
 finish_log_line();
}

// this function assumes target bitmap has been set
void display_log(int x1, int y1)
{

// al_set_target_bitmap(al_get_backbuffer(display));
 al_set_clipping_rectangle(editor.panel_x + EDIT_WINDOW_X, editor.panel_y + editor.mlog_window_y, editor.edit_window_w, LOG_WINDOW_H);
// editor.log_sub_bmp = al_create_sub_bitmap(editor.sub_bmp, EDIT_WINDOW_X, editor.mlog_window_y, editor.edit_window_w, LOG_WINDOW_H);

 int x2 = x1 + mlog.w_pixels + 10;
 int y2 = y1 + mlog.h_pixels + 20;
 int x, y;

 int lines_printed = mlog.h_lines;

 al_draw_filled_rectangle(x1, y1, x2, y2, colours.base [COL_GREY] [SHADE_MIN]);
// al_draw_rectangle(editor.panel_x + EDIT_WINDOW_X + 1, editor.panel_y + editor.mlog_window_y + 1, editor.edit_window_w - 1, LOG_WINDOW_H - 1, colours.base [COL_BLUE] [SHADE_MED], 1);
 al_draw_rectangle(x1, y1, x2 - 1, y2 - 20, colours.base [COL_BLUE] [SHADE_MED], 1);

 int log_line_pos = mlog.window_pos + mlog.lpos + mlog.h_lines;

// log_line_pos %= LOG_LINES;

 while (log_line_pos >= LOG_LINES)
 {
  log_line_pos -= LOG_LINES;
 }
 while (log_line_pos < 0)
 {
  log_line_pos += LOG_LINES;
 }

 int display_line_pos = lines_printed;

 while (display_line_pos > 0)
 {
  x = x1 + 10;
  y = y1 + (display_line_pos * LOG_LINE_HEIGHT);

  al_draw_textf(font[FONT_BASIC].fnt, mlog_col [mlog.log_line[log_line_pos].colour], x, y, 0, " %s", mlog.log_line[log_line_pos].text); // must use " %s"; can't use text string directly as it may contain % characters
  display_line_pos --;
  log_line_pos --;
  if (log_line_pos == -1)
   log_line_pos = LOG_LINES - 1;
 };




}






