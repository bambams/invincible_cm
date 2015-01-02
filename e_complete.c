



#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "m_config.h"

#include "g_header.h"

#include "g_misc.h"

#include "c_header.h"
#include "e_header.h"
#include "e_editor.h"
#include "e_complete.h"
#include "c_prepr.h"
#include "c_init.h"
#include "i_header.h"

// word lists:
extern const struct numtokenstruct numtoken [NUMTOKENS];
extern const struct c_keywordstruct c_keyword [C_KEYWORDS];
extern const struct c_keywordstruct c_builtin_cfunction [C_BUILTIN_CFUNCTION_NAMES];
extern const struct c_keywordstruct asm_keyword [ASM_KEYWORDS];



extern struct editorstruct editor; // defined in e_editor.c
extern struct coloursstruct colours;
struct fontstruct font [FONTS];

struct completionstruct completion;

#define COMPLETION_TABLE_SIZE 1200
#define COMPLETION_TABLE_STRING_LENGTH 36

// completion_table is an alphabetically sorted list of all code completion tokens
char completion_table [1200] [COMPLETION_TABLE_STRING_LENGTH];

int alphabetical_comparison(const void* str1, const void* str2);


void init_code_completion(void)
{

	int i = 0;
	int j = 0;

	while(numtoken[j].name [0] != '\0')
	{
		strcpy(completion_table [i], numtoken[j].name);
		i++;
		j++;
		if (j >= NUMTOKENS)
		{
			fprintf(stdout, "\nError: e_complete.c: init_code_completion(): passed end of numtoken list?");
			error_call();
		}
		if (i >= COMPLETION_TABLE_SIZE)
		{
			fprintf(stdout, "\nError: e_complete.c: init_code_completion(): ran out of space in completion table.");
			error_call();
		}
	};

 j = 0; // don't reset i

	while(c_keyword[j].name [0] != '\0'
				&& j < C_KEYWORDS)
	{
		strcpy(completion_table [i], c_keyword[j].name);
		i++;
		j++;
/*		if (j >= C_KEYWORDS)
		{
			fprintf(stdout, "\nError: e_complete.c: init_code_completion(): passed end of c_keyword list?");
			error_call();
		}*/
		if (i >= COMPLETION_TABLE_SIZE)
		{
			fprintf(stdout, "\nError: e_complete.c: init_code_completion(): ran out of space in completion table.");
			error_call();
		}
	};

 j = 0; // don't reset i

	while(c_builtin_cfunction[j].name [0] != '\0'
				&& j < C_BUILTIN_CFUNCTION_NAMES)
	{
		strcpy(completion_table [i], c_builtin_cfunction[j].name);
		i++;
		j++;
		if (c_builtin_cfunction[j].index >= BUILTIN_UNIMP_CONST)
			break; // don't include unimplemented/unsupported things
/*		if (j >= C_BUILTIN_CFUNCTION_NAMES)
		{
			fprintf(stdout, "\nError: e_complete.c: init_code_completion(): passed end of c_builtin_cfunction list?");
			error_call();
		}*/
		if (i >= COMPLETION_TABLE_SIZE)
		{
			fprintf(stdout, "\nError: e_complete.c: init_code_completion(): ran out of space in completion table.");
			error_call();
		}
	};

 j = 0; // don't reset i

	while(asm_keyword[j].name [0] != '\0'
				&& j < ASM_KEYWORDS)
	{
		strcpy(completion_table [i], asm_keyword[j].name);
		i++;
		j++;
/*		if (j >= ASM_KEYWORDS)
		{
			fprintf(stdout, "\nError: e_complete.c: init_code_completion(): passed end of asm_keyword list?");
			error_call();
		}*/
		if (i >= COMPLETION_TABLE_SIZE)
		{
			fprintf(stdout, "\nError: e_complete.c: init_code_completion(): ran out of space in completion table.");
			error_call();
		}
	};

	completion.table_size = i;

 qsort(completion_table, completion.table_size, COMPLETION_TABLE_STRING_LENGTH, alphabetical_comparison);
/*
 for (i = 0; i < completion.table_size; i ++)
	{
		fprintf(stdout, "\n%i: %s", i, completion_table [i]);
	}*/

}



int alphabetical_comparison(const void* str1, const void* str2)
{
 return strcmp(str1, str2);
}



// call this when the user types a letter or equivalent symbol
void check_code_completion(struct source_editstruct* se, int from_backspace)
{

 int i;
 int check_word_length = 0;

 int check_pos = se->cursor_pos;
 int cursor_pos = se->cursor_pos;

 if (from_backspace)
		cursor_pos --;

 completion.list_size = 0;

 if (check_pos < MIN_COMPLETION_LENGTH)
	 return;

 char check_word [IDENTIFIER_MAX_LENGTH + 1] = "";
 char read_char;
 int read_char_type;
 int word_pos = 0;

// we look left from the cursor to find how long the current word is:

 while(TRUE)
	{
		if (check_pos <= 0)
			break;
		read_char = se->text [se->line_index [se->cursor_line]] [check_pos];
		read_char_type = get_source_char_type(read_char);
		if (read_char_type != SCHAR_LETTER
			&& read_char_type != SCHAR_NUMBER)
		{
			check_pos++;
	  break;
		}
		check_pos --;
	};

	check_word_length = cursor_pos - check_pos + 1;

	if (check_word_length >= IDENTIFIER_MAX_LENGTH - 1)
		return; // word too long
	if (check_word_length <= MIN_COMPLETION_LENGTH)
		return; // word too short

	completion.word_length = check_word_length;

// now we have a start and end position for a word.
// read the word into check_word:
 while(TRUE)
	{
		if (check_pos > cursor_pos)
			break;
		check_word [word_pos] = se->text [se->line_index [se->cursor_line]] [check_pos];
		check_word [word_pos + 1] = '\0';
		word_pos ++;
		check_pos ++;
	};


// fprintf(stdout, "\nword [%s] length %i", check_word, check_word_length);


// Now check_word contains the current word (ignoring anything that appears after the cursor)
// Use it to build a code completion list.
 completion.list_size = 0;

 int j;
 int found;

// first check against numtokens:
 for (i = 0; i < completion.table_size; i ++)
	{
		if (completion.list_size >= COMPLETION_LIST_LENGTH)
			break;

  j = 0;
  found = 0;

  while(TRUE)
		{
   if (j >= check_word_length)
			{
				found = 1;
				break;
			}
   if (check_word [j] != completion_table [i] [j]
			 || completion_table [i] [j] == '\0')
		 {
		 	break;
		 }
		 j ++;
		};
		if (found == 1)
		{
//			completion.list_entry_type [completion.list_size] = COMPLETION_TYPE_NUMTOKEN;
			completion.list_entry_index [completion.list_size] = i;
			completion.list_size++;
		}

	}

/*
 fprintf(stdout, "\nlist size: %i", completion.list_size);

 for (i = 0; i < completion.list_size; i ++)
	{
		fprintf(stdout, "\n%s in list at %i", numtoken[completion.list_entry_index [i]].name, i);
	}
*/

 if (completion.list_size > 0)
	{
		completion.window_pos = 0; // think about how to avoid resetting window_pos and select_line if the user selects something after the first line then keeps typing a word consistent with the selection
		completion.select_line = 0;


		completion.box_x = editor.panel_x + EDIT_WINDOW_X + (((se->cursor_pos+1) - se->window_pos) * editor.text_width) - SOURCE_WINDOW_MARGIN;
 	if (completion.box_x > editor.panel_x + editor.panel_w - COMPLETION_BOX_W)
	  completion.box_x = editor.panel_x + editor.panel_w - COMPLETION_BOX_W;
		completion.box_y = editor.panel_y + EDIT_WINDOW_Y + (se->cursor_line - se->window_line) * EDIT_LINE_H + EDIT_LINE_OFFSET + 12;
		completion.box_x2 = completion.box_x + COMPLETION_BOX_W;
		completion.box_lines = completion.list_size;
		if (completion.box_lines > COMPLETION_BOX_MAX_LINES)
			completion.box_lines = COMPLETION_BOX_MAX_LINES;
		completion.box_y2 = completion.box_y + 3 + completion.box_lines * COMPLETION_BOX_LINE_H;
	}

}








// assumes that target bitmap is set
//  and clipping rectangle is suitable (probably needs to be able to draw on the entire panel)
void draw_code_completion_box(void)
{
 if	(completion.list_size == 0)
		return;

 int x = completion.box_x;
 int y = completion.box_y;

 al_draw_filled_rectangle(x, y, completion.box_x2, completion.box_y2, colours.base [COL_BLUE] [SHADE_LOW]);
 al_draw_rectangle(x + 0.5, y + 0.5, completion.box_x2 + 0.5, completion.box_y2 + 0.5, colours.base [COL_BLUE] [SHADE_HIGH], 1);
 int i;

 for (i = 0; i < completion.box_lines; i ++)
	{
		if (completion.select_line == i + completion.window_pos)
   al_draw_filled_rectangle(x + 1.5, y + i * COMPLETION_BOX_LINE_H + 2, completion.box_x2 - 0.5, y + i * COMPLETION_BOX_LINE_H + 14, colours.base [COL_BLUE] [SHADE_MED]);

  al_draw_textf(font[FONT_BASIC].fnt, colours.base [COL_BLUE] [SHADE_MAX], x + 3, y + i * COMPLETION_BOX_LINE_H + COMPLETION_BOX_LINE_Y_OFFSET, ALLEGRO_ALIGN_LEFT, "%s", completion_table [completion.list_entry_index [i + completion.window_pos]]);

//		switch(completion.list_entry_type [i])
//		{
//			case COMPLETION_TYPE_NUMTOKEN:
//	   al_draw_textf(font[FONT_BASIC].fnt, colours.base [COL_BLUE] [SHADE_MAX], x + 3, y + i * COMPLETION_BOX_LINE_H + 4, ALLEGRO_ALIGN_LEFT, "%s", numtoken [completion.list_entry_index [i]].name);
//	   al_draw_textf(font[FONT_BASIC].fnt, colours.base [COL_BLUE] [SHADE_MAX], x + 3, y + i * COMPLETION_BOX_LINE_H + COMPLETION_BOX_LINE_Y_OFFSET, ALLEGRO_ALIGN_LEFT, "%s", numtoken [completion.list_entry_index [i + completion.window_pos]].name);
//	   break;
//		}
	}


}


void completion_box_select_line_up(void)
{

			completion.select_line --;
			if (completion.select_line < 0)
			{
				completion.select_line = completion.list_size - 1;
				if (completion.select_line >= completion.window_pos + completion.box_lines - 1)
					completion.window_pos = completion.select_line - completion.box_lines + 1;
			}
			 else
				{
			  if (completion.select_line == completion.window_pos - 1)
						completion.window_pos --;
				}

}

void completion_box_select_line_down(void)
{

			completion.select_line ++;
			if (completion.select_line >= completion.list_size)
			{
				completion.select_line = 0;
				completion.window_pos = 0;
			}
			 else
				{
				 if (completion.select_line >= completion.window_pos + completion.box_lines)
					 completion.window_pos = completion.select_line - completion.box_lines + 1;
				}

}


// like line_up but doesn't wrap
void completion_box_select_lines_up(int amount)
{

			completion.select_line -= amount;
			if (completion.select_line < 0)
				completion.select_line = 0;
			completion.window_pos = completion.select_line;

}

void completion_box_select_lines_down(int amount)
{

			completion.select_line += amount;
			if (completion.select_line >= completion.list_size)
				completion.select_line = completion.list_size - 1;
			if (completion.select_line >= completion.window_pos + completion.box_lines)
				completion.window_pos = completion.select_line - completion.box_lines + 1;
			if (completion.window_pos < 0)
			 completion.window_pos = 0;

}


void scroll_completion_box_up(int amount)
{

			completion.window_pos -= amount;
			if (completion.window_pos < 0)
					completion.window_pos = 0;

			if (completion.select_line > completion.window_pos + completion.box_lines)
				completion.select_line = completion.window_pos + completion.box_lines;

}

void scroll_completion_box_down(int amount)
{

			completion.window_pos += amount;
			if (completion.window_pos > completion.list_size - completion.box_lines)
				completion.window_pos = completion.list_size - completion.box_lines;
			if (completion.window_pos < 0)
					completion.window_pos = 0;

			if (completion.select_line < completion.window_pos)
				completion.select_line = completion.window_pos;

}




void complete_code(struct source_editstruct* se, int select_line)
{

	if (se == NULL)
		return;

	if (select_line < 0
	 || select_line >= completion.list_size)
		return;

 char add_word [PTOKEN_LENGTH + 1];

		 strcpy(add_word, completion_table [completion.list_entry_index [select_line]]);


// switch(completion.list_entry_type [select_line])
// {
//  case COMPLETION_TYPE_NUMTOKEN:
//		 strcpy(add_word, numtoken[completion.list_entry_index [select_line]].name);
//		 break;
//	 default:
// 		return;
// }

 int i = completion.word_length;

 while(add_word [i] != '\0')
	{
		if (!add_char(add_word [i], 0))
		{
			return;
		}
		i++;
	};

	completion.list_size = 0;
// se->cursor_base = se->cursor_pos;

}




