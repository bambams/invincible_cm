

#include <allegro5/allegro.h>
#include <allegro5/allegro_native_dialog.h>

#include <stdio.h>
#include <string.h>

#include "m_config.h"

#include "g_header.h"
#include "m_globvars.h"
#include "m_input.h"

//#include "g_misc.h"

#include "c_header.h"
#include "c_prepr.h"

#include "e_slider.h"
#include "e_header.h"
#include "c_init.h"
#include "e_log.h"
#include "e_files.h"

#include "e_editor.h"

//#include "c_comp.h"

extern struct editorstruct editor;


int compare_search_string(struct source_editstruct* se, int sline, int spos);

void find_next(void)
{

 if (editor.search_string [0] == '\0')
  return;

 struct source_editstruct* se = get_current_source_edit();

 if (se == NULL
  || se->type != SOURCE_EDIT_TYPE_SOURCE)
  return;

 int search_line = se->cursor_line;
 int search_pos = se->cursor_pos;

 while(TRUE)
 {
  search_pos ++;
  while (se->text [search_line] [search_pos] == '\0')
  {
   search_pos = 0;
   search_line ++;
   if (search_line == se->cursor_line)
    return; // looped back around - nothing found
   if (search_line >= SOURCE_TEXT_LINES)
    search_line = 0;
  }
// compare_search_string comes after search_pos increment because otherwise a repeated search will just find the same string in the same position again
  if (compare_search_string(se, search_line, search_pos))
  {
   se->cursor_line = search_line;
   se->cursor_pos = search_pos;
   return;
  }
 };

}

int compare_search_string(struct source_editstruct* se, int sline, int spos)
{

 int i = 0;

 while(TRUE)
 {
  if (editor.search_string [i] == '\0')
   return 1; // success!
  if (editor.search_string [i] != se->text [sline] [spos + i])
   return 0; // this will return on finding the end of the source line, so don't need to expressly check for it
  i++;
 };

 return 0;

}

