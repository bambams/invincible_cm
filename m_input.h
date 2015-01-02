
#ifndef H_M_INPUT
#define H_M_INPUT

struct ex_controlstruct // this struct holds information taken directly from input, and is updated every frame. It is used to fill in the controlstruct and also for editor input. See m_input.c
{
 int mouse_x_pixels;
 int mouse_y_pixels;
 int mb_press [2];
 int mousewheel_pos;
 int mousewheel_change;
 int mouse_dragging_panel; // is the mouse dragging the editor panel?
 int panel_drag_ready; // is the mouse in the right position to drag the editor panel? (shows a special line thing)
 int key_press [ALLEGRO_KEY_MAX];
 int keys_pressed;
 int key_being_pressed;
 int mouse_on_display;

 int using_slider; // is 1 if mouse is interacting with a slider, 0 otherwise. This is used to prevent simultaneous interaction with multiple sliders.
 // the sl.hold_type value of a slider indicates whether it's the one being used.

 int console_action_type; // console_action values are set when the user clicks on a line in a console that has an action attached to it - see i_console.c
 int console_action_console; // which console was clicked on
 int console_action_val1; // they can be used by the observer console method
 int console_action_val2;
 int console_action_val3;

};


void get_ex_control(int close_button_status);
void init_ex_control(void);
void init_key_type(void);


enum
{
 KEY_TYPE_LETTER, // a-z
 KEY_TYPE_NUMBER, // 0-9
 KEY_TYPE_SYMBOL, // any other symbol key e.g. ;
 KEY_TYPE_CURSOR, // moves the cursor e.g. arrow keys or end, or makes some change like deleting something
// KEY_TYPE_NUMPAD, // like cursor but affected by numlock - not presently used
 KEY_TYPE_MOD, // modifier e.g. shift
 KEY_TYPE_OTHER, // ignored keys
};

struct key_typestruct
{
 int type; // basic type e.g. KEY_TYPE_LETTER
 char unshifted;
 char shifted;
};

extern struct key_typestruct key_type [ALLEGRO_KEY_MAX];


void start_text_input_box(int b, char* input_str, int max_length);
int accept_text_box_input(int b);

enum
{
TEXT_BOX_EDITOR_FIND,
TEXT_BOX_PLAYER_NAME,
TEXT_BOXES
};

#endif
