
#ifndef H_I_CONSOLE
#define H_I_CONSOLE

#include "e_slider.h"

//#define CONSOLE_X 10
//#define CONSOLE_Y 10
#define CONSOLE_BASIC_HEADER_H 15
#define CONSOLE_BOX_HEADER_H 15
#define CONSOLE_BUTTON_SIZE 10
#define CONSOLE_BUTTON_SEPARATION 15

#define CLINE_LENGTH 70
#define CLINES 120

#define LINE_HEIGHT 15

// CONSOLE_LINE_OFFSET is a few pixels added to the top of the console
#define CONSOLE_LINE_OFFSET 6

#define CONSOLE_BUTTONS 1

#define CONSOLE_LETTERS_MIN 10
#define CONSOLE_LETTERS_MAX CLINE_LENGTH
#define CONSOLE_LINES_MIN 0
#define CONSOLE_LINES_MAX 40


enum
{
CONSOLE_CLOSED,
CONSOLE_OPEN_MIN,
CONSOLE_OPEN_MAX
};

// CONSOLES defined in g_header.h


enum
{

CONSOLE_ACTION_NONE, // nothing
CONSOLE_ACTION_SYSTEM, // sent by system - other two values can be anything
CONSOLE_ACTION_OBSERVER, // sent by observer - other two values can be anything
CONSOLE_ACTION_OPERATOR, // sent by operator - other two values can be anything
//CONSOLE_ACTION_CLIENT, // sent by client - next value is player index, last value is anything
CONSOLE_ACTION_PROC, // sent by proc - next value is proc index, last value is anything

CONSOLE_ACTIONS
};

struct consolelinestruct
{
 int used; // whether the line contains anything
 int colour; // index in PRINT_COLS. value in this field should be bounds-checked (as it would have been taken from w.print_colour, which is bounds-checked)
 char text [CLINE_LENGTH];
 int source_program_type; // type of program that produced console line. -1 if no source. Is used to work out whether to start a new line (which is done automatically each time the source changes; otherwise is done manually)
 int source_index; // index of player or proc

 int action_type; // CONSOLE_ACTION_TYPE enum
 s16b action_val1; // first value - client: player index; proc: proc index; obs/sys: any
 s16b action_val2; // second value: anything
 s16b action_val3; // second value: anything

 unsigned int time_stamp; // w.total_time when it was printed + 50 (adjusted because w.total_time is unsigned and can't be compared to time_stamp - 50)

};

enum
{
CONSOLE_STYLE_BASIC, // simplest form of console. header at the top
CONSOLE_STYLE_BASIC_UP, // same but header is at the bottom
CONSOLE_STYLE_CLEAR_CENTRED, // no border or background. text just appears on screen. Centred.
CONSOLE_STYLE_BOX, // box with header and border but with no highlighting or scrollbar.
CONSOLE_STYLE_BOX_UP, // same but header is at the bottom
CONSOLE_STYLES
};

#define CONSOLE_TITLE_LENGTH 16



struct consolestruct
{

// put basic parameters of the console here:

 int x, y;
 int h_lines; // height in lines
 int w_letters; // width in letters
 int h_pixels; // height in pixels
 int w_pixels;
 int open; // 0 = closed, 1 = open   // small, 2 = large
 int printed_this_tick; // is 1 if the console has been printed to this tick (since console_tick_rollover was last called)
 int line_highlight; // if the mouse is over a line in the console
 int button_x [CONSOLE_BUTTONS]; // x position (offset from CONSOLE_X) of buttons that open/close console
 int button_highlight; // indicates mouse is over one of the buttons (-1 if no highlight)

 int style;
 int font_index;
 int colour_index;

 struct sliderstruct scrollbar_v;
 char title [CONSOLE_TITLE_LENGTH];

// put the contents and immediate status of the console below here:

 int cpos; // position in the cline array
 int window_pos; // the cline at the bottom of the window, as an offset from cpos

 struct consolelinestruct cline [CLINES];


};



#define PBOX_HEADER_HEIGHT 15
#define PBOX_W 320
#define PBOX_LINE 12

struct proc_boxstruct
{
 int x1, y1, x2, y2;
 int button_highlight;
 int maximised;
 int button_x1, button_y1, button_x2, button_y2;
};





void init_consoles(void);
void setup_consoles(void);

void program_write_to_console(const char* str, int source_program_type, int source_index, int source_player);
void write_error_to_console(const char* str, int source_program_type, int source_index, int source_player);

void run_consoles(void);
int run_console_method(struct programstruct* cl, int m);
void display_consoles(void);
void check_mouse_on_consoles_etc(int x, int y, int just_pressed);

void run_score_boxes(void);
void display_score_boxes(void);

void draw_proc_box(void);
void place_proc_box(int x, int y, struct procstruct* pr);
void reset_proc_box_height(struct procstruct* pr);

void set_console_size_etc(int c, int new_w, int new_h, int font_index, int force_update);

#endif
