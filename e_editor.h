
#ifndef H_E_EDITOR
#define H_E_EDITOR

#include "c_header.h"

void init_editor(void);

void run_editor(void);

void open_editor(void);
void close_editor(void);

int add_char(char added_char, int check_completion);

void update_source_lines(struct source_editstruct* se, int sline, int lines);
void window_find_cursor(struct source_editstruct* se);
int insert_empty_lines(struct source_editstruct* se, int before_line, int lines);
void delete_lines(struct source_editstruct* se, int start_line, int lines);
int is_something_selected(struct source_editstruct* se);
int delete_selection(void);
struct source_editstruct* get_current_source_edit(void);
void open_tab(int tab);
void change_tab(int new_tab);
int source_edit_to_source(struct sourcestruct* src, struct source_editstruct* se);
int get_current_source_edit_index(void);

void open_overwindow(int ow_type);

void init_source_editstruct(struct source_editstruct* se);
int source_to_editor(struct sourcestruct* src, int esource);
int binary_to_editor(struct bcodestruct* bcode, int esource);

void flush_game_event_queues(void);

int source_line_highlight_syntax(struct source_editstruct* se, int src_line, int in_a_comment);
int get_source_char_type(char read_source);

#endif
