
#define _GNU_SOURCE
#include <ctype.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#include "Prototypes.h"

/*{

#ifndef isword
#define isword(x) (isalpha(x) || x == '_')
#endif

#ifndef last_x
#define last_x(this) (this->dosLineBreaks ? this->line->len - 1 : this->line->len)
#endif

struct Buffer_ {
   char* fileName;
   bool modified;
   bool readOnly;
   // logical position of the cursor in the line
   // (character of the line where cursor is.
   int x;
   int y;
   Line* line;
   // save X so that cursor returns to original X
   // after passing by a shorter line.
   int savedX;
   int savedY;
   Highlight* hl;
   // save context between calls to Buffer_draw
   // to detect highlight changes that demand
   // a redraw
   HighlightContext* savedContext;
   // coordinates of selection
   int selectXfrom;
   int selectYfrom;
   int selectXto;
   int selectYto;
   // coordinates of highlighted bracket
   int bracketX;
   int bracketY;
   // block is being selected
   bool wasSelecting;
   bool selecting;
   // mark block using 'mark' key (ie, without shift)
   bool marking;
   Panel* panel;
   Undo* undo;
   // previous key for compound actions
   int lastKey;
   // document uses tab characters
   int tabCharacters;
   // document uses DOS-style ctrl-M
   int dosLineBreaks;
   int indentSpaces;
   // time tracker to disable auto-indent when pasting;
   double lastTime;
   // marks for error highlighting
   int* marks;
};

struct FilePosition_ {
   int x;
   int y;
   char* name;
   FilePosition* next;
};

}*/

char* realpath(const char* path, char* resolved_path);

Buffer* Buffer_new(int x, int y, int w, int h, char* fileName, bool command) {
   Buffer* this = (Buffer*) malloc(sizeof(Buffer));

   this->x = 0;
   this->y = 0;
   this->savedX = 0;
   this->selectXfrom = -1;
   this->selectYfrom = -1;
   this->selectXto = -1;
   this->selectYto = -1;
   this->wasSelecting = false;
   this->selecting = false;
   this->marking = false;
   this->bracketX = -1;
   this->bracketY = -1;
   this->lastKey = 0;
   this->modified = false;
   this->tabCharacters = false;
   this->dosLineBreaks = false;
   this->indentSpaces = 3;
   this->marks = NULL;
   
   /* Hack to disable auto-indent when pasting through X11, part 1 */
   struct timeval tv;
   gettimeofday(&tv, NULL);
   this->lastTime = tv.tv_sec * 1000000 + tv.tv_usec;

   Panel* p = Panel_new(x, y, w-1, h, CRT_colors[NormalColor], ClassAs(Line, ListItem), true, this);
   this->panel = p;
   this->undo = Undo_new(p->items);
   this->fileName = NULL;
   bool newFile = true;
   if (fileName) {
      this->fileName = String_copy(fileName);
      this->readOnly = (access(fileName, R_OK) == 0 && access(fileName, W_OK) != 0);
     
      FileReader* file = FileReader_new(fileName, command);
      int i = 0;
      if (file && !FileReader_eof(file)) {
         newFile = false;
         int len = 0;
         char* text = FileReader_readLine(file, &len);
         this->hl = Highlight_new(fileName, text);
         if (!this->tabCharacters && strchr(text, '\t')) this->tabCharacters = true;
         if (!this->dosLineBreaks && strchr(text, '\015')) this->dosLineBreaks = true;
         Line* line = Line_new(p->items, text, len, this->hl->mainContext);
         Panel_set(p, i, (ListItem*) line);
         while (!FileReader_eof(file)) {
            i++;
            char* text = FileReader_readLine(file, &len);
            if (text) {
               if (!this->tabCharacters && strchr(text, '\t')) this->tabCharacters = true;
               Line* line = Line_new(p->items, text, len, this->hl->mainContext);
               Panel_set(p, i, (ListItem*) line);
            }
         }
         FileReader_delete(file);
         Buffer_restorePosition(this);
         Undo_restore(this->undo, fileName);
      }
   } else {
      this->readOnly = false;
   }
   if (newFile) {
      this->hl = Highlight_new(fileName, "");
      Panel_set(p, 0, (ListItem*) Line_new(p->items, String_copy(""), 0, this->hl->mainContext));
   }

   this->savedContext = this->hl->mainContext;

   return this;
}

inline void Buffer_restorePosition(Buffer* this) {
   char rpath[256];
   realpath(this->fileName, rpath);
   
   FILE* fd = Files_openHome("r", "filepos", NULL);
   if (fd) {
      int x, y;
      char line[256];
      while (!feof(fd)) {
         fscanf(fd, "%d %d %255[^\n]\n", &x, &y, line);
         if (strcmp(line, rpath) == 0) {
         
            Line* line = (Line*) this->panel->items->head;
            for (int i = 0; line && i <= y; i++) {
               Line_display((Object*)line, NULL);
               line = (Line*) line->super.next;
            }
         
            Buffer_goto(this, x, y);
            break;
         }
      }
      fclose(fd);
   }
}

inline void Buffer_storePosition(Buffer* this) {

   char rpath[4097];
   realpath(this->fileName, rpath);

   FILE* fd = Files_openHome("r", "filepos", NULL);
   FilePosition* fps = NULL;
   FilePosition* prev = NULL;
   bool set = false;
   if (fd) {
      while (!feof(fd)) {
         FilePosition* fp = malloc(sizeof(FilePosition));
         fp->name = malloc(4097);
         fp->next = NULL;
         fscanf(fd, "%d %d %4096[^\n]\n", &(fp->x), &(fp->y), fp->name);
         if (strcmp(fp->name, rpath) == 0) {
            fp->x = this->x;
            fp->y = this->y;
            set = true;
         }
         if (prev)
            prev->next = fp;
         else
            fps = fp;
         prev = fp;
      }
      fclose(fd);
   }
   if (!set) {
      if (!(this->x == 0 && this->y == 0)) {
         FilePosition* fp = malloc(sizeof(FilePosition));
         fp->name = malloc(4097);
         strcpy(fp->name, rpath);
         fp->x = this->x;
         fp->y = this->y;
         fp->next = NULL;
         if (!fps) {
            fps = fp;
         } else {
            prev->next = fp;
         }
      }
   }
   if (!fps)
      return;
   fd = Files_openHome("w", "filepos", NULL);
   if (fd) {
      FilePosition* fp = fps;
      while (fp) {
         fprintf(fd, "%d %d %s\n", fp->x, fp->y, fp->name);
         free(fp->name);
         FilePosition* save = fp;
         fp = fp->next;
         free(save);
      }
      fclose(fd);
   } else
      return;
}

void Buffer_goto(Buffer* this, int x, int y) {
   if (y < 0)
      y += Panel_size(this->panel);
   Panel_setSelected(this->panel, y);
   this->line = (Line*) Panel_getSelected(this->panel);
   this->y = Panel_getSelectedIndex(this->panel);
   this->x = MIN(x, last_x(this));
   this->panel->scrollV = MAX(0, this->y - (this->panel->h / 2));
   this->panel->needsRedraw = true;
}

void Buffer_draw(Buffer* this) {
   Panel* p = this->panel;

   if (this->wasSelecting && !this->selecting)
      p->needsRedraw = true;

   this->y = Panel_getSelectedIndex(p);
   this->line = (Line*) Panel_getSelected(p);
   
   Line_updateContext(this->line);
   
   if (this->line->context != this->savedContext)
      p->needsRedraw = true;
   this->savedContext = this->line->context;
      
   if (this->x > last_x(this))
      this->x = last_x(this);

   /* actual X position (expanding tabs, etc) */
   int screenX = Line_widthUntil(this->line, this->x);

   if (screenX - p->scrollH >= p->w) {
      p->scrollH = screenX - p->w + 1;
      p->needsRedraw = true;
   } else if (screenX < p->scrollH) {
      p->scrollH = screenX;
      p->needsRedraw = true;
   }

   Buffer_highlightBracket(this);
   
   p->cursorX = screenX - p->scrollH;
   Panel_draw(p);
   
   this->wasSelecting = this->selecting;
}

int Buffer_y(Buffer* this) {
   return this->y;
}

char* Buffer_currentLine(Buffer* this) {
   return this->line->text;
}

inline char* Buffer_getLine(Buffer* this, int i) {
   Line* line = (Line*) Panel_get(this->panel, i);
   if (line) 
      return line->text;
   else
      return NULL;
}

char* Buffer_previousLine(Buffer* this) {
   return Buffer_getLine(this, this->y - 1);
}

static inline bool Buffer_matchBracket(char ch, char* oth, int* dir) {
   switch (ch) {
   case '(': *oth = ')'; *dir = 1; return true;
   case '[': *oth = ']'; *dir = 1; return true;
   case '{': *oth = '}'; *dir = 1; return true;
   case ')': *oth = '('; *dir = -1; return true;
   case ']': *oth = '['; *dir = -1; return true;
   case '}': *oth = '{'; *dir = -1; return true;
   default: return false;
   }
}

inline void Buffer_highlightBracket(Buffer* this) {
   assert (this->x >= 0 && this->x <= last_x(this));
   if (this->bracketY > -1 && this->bracketY != this->y) {
      this->panel->needsRedraw = true;
   }
   this->bracketX = -1;
   this->bracketY = -1;
   Line* l = this->line;
   char oth;
   int dir;
   int x = this->x;
   if (l->len == 0) return;
   char ch = l->text[x];
   if (!Buffer_matchBracket(ch, &oth, &dir)) {
      if (x == 0)
         return;
      ch = l->text[--x];
      if (!Buffer_matchBracket(ch, &oth, &dir)) {
         return;
      }
   }
   x += dir;
   int y = this->y;
   int level = 1;

   while (true) {
      if (x < 0) {
         if (l->super.prev) {
            l = (Line*) l->super.prev;
            y--;
            x = l->len - 1;
            if (y < this->y - this->panel->h) {
               return;
            }
         } else
            return;
      } else if (x >= l->len) {
         if (l->super.next) {
            l = (Line*) l->super.next;
            y++;
            x = 0;
            if (y > this->y + this->panel->h) {
               return;
            }
         } else {
            return;
         }
      }
      char at = l->text[x];
      if (at == ch) {
         level++;
      } else if (at == oth) {
         level--;
         if (level == 0) {
            this->bracketX = Line_widthUntil(l, x);
            this->bracketY = y;
            if (y != this->y)
               this->panel->needsRedraw = true;
            return;
         }
      }
      x += dir;
   }
}

void Buffer_delete(Buffer* this) {
   if (this->fileName) {
      Buffer_storePosition(this);
      free(this->fileName);
   }
   Msg0(Object, delete, this->panel);

   Undo_delete(this->undo);
   Highlight_delete(this->hl);
   free(this);
}

void Buffer_refreshHighlight(Buffer* this) {
   Highlight_delete(this->hl);
   Line* firstLine = (Line*) Panel_get(this->panel, 0);
   char* firstText;
   if (firstLine)
      firstText = firstLine->text;
   else
      firstText = "";
   Highlight* hl = Highlight_new(this->fileName, firstText);
   this->hl = hl;
   int size = Panel_size(this->panel);
   for (int i = 0; i < size; i++) {
      Line* line = (Line*) Panel_get(this->panel, i);
      line->context = hl->mainContext;
   }
   
   this->panel->needsRedraw = true;
}

static inline void Buffer_getMarks(Buffer* this) {
   if (this->marks) {
      free(this->marks);
   }
   char* command = malloc(strlen("./highlight_errors.sh ") + strlen(this->fileName) + 2);
   sprintf(command, "./highlight_errors.sh %s", this->fileName);
   FILE* cmd = popen(command, "r");
   int* marks = malloc(sizeof(int) * 255);
   this->marks = marks;
   int x = 0;
   while (!feof(cmd)) {
      int n = fscanf(cmd, "%d %d", marks, marks+1);
      if (n < 2)
         break;
      marks += 2;
      x++;
   }
/*
mvprintw(LINES - 1, 0, "%d", marks[0]);
refresh();
CRT_readKey();
*/
   marks[0] = 0;
   marks[1] = 0;
   fclose(cmd);
   this->panel->needsRedraw = true;
}

void Buffer_select(Buffer* this, void(*motion)(Buffer*)) {
   if (!this->selecting) {
      this->selectXfrom = this->x;
      this->selectYfrom = this->y;
   }
   motion(this);
   this->selectXto = this->x;
   this->selectYto = this->y;
   this->selecting = true;
   this->panel->needsRedraw = true;
}

bool Buffer_checkDiskState(Buffer* this) {
   return Undo_checkDiskState(this->undo);
}

void Buffer_undo(Buffer* this) {
   int ux = this->x;
   int uy = this->y;
   bool modified = Undo_undo(this->undo, &ux, &uy);
   this->x = ux;
   Panel_setSelected(this->panel, uy);
   this->line = (Line*) Panel_getSelected(this->panel);
   this->panel->needsRedraw = true;
   this->savedX = Line_widthUntil(this->line, this->x);
   this->selecting = false;
   this->modified = modified;
}

inline void Buffer_breakIndenting(Buffer* this, int indent) {
   indent = MIN(indent, last_x(this));
   Undo_breakAt(this->undo, this->x, this->y, indent);
   Line_breakAt(this->line, this->x, indent);
}

char Buffer_getLastKey(Buffer* this) {
   return this->lastKey;
}

inline int Buffer_getIndentSize(Buffer* this) {
   return MIN(Line_getIndentChars(this->line), this->x);
}

void Buffer_breakLine(Buffer* this) {
   if (this->selecting) {
      Buffer_deleteBlock(this);
   }

   if (this->dosLineBreaks) {
      Undo_beginGroup(this->undo, this->x, this->y);
      Buffer_defaultKeyHandler(this, '\015');
   }

   int indent = Buffer_getIndentSize(this);

   /* Hack to disable auto-indent when pasting through X11, part 2 */
   struct timeval tv;
   gettimeofday(&tv, NULL);
   double now = tv.tv_sec * 1000000 + tv.tv_usec;
   if (now - this->lastTime < 10000)
      indent = 0;
   this->lastTime = now;

   Buffer_breakIndenting(this, indent);
   Panel_onKey(this->panel, KEY_DOWN);
   this->line = (Line*) Panel_getSelected(this->panel);
   this->x = indent;
   this->y++;
   this->panel->needsRedraw = true;
   this->modified = true;

   if (this->dosLineBreaks) {
      Undo_endGroup(this->undo, this->x, this->y);
   }

   Script_hook("onBreakLine");

   this->lastKey = 0;
}

void Buffer_forwardChar(Buffer* this) {
   if (this->x < last_x(this)) {
      this->x++;
   } else if (this->line->super.next) {
      this->x = 0;
      Panel_onKey(this->panel, KEY_DOWN);
      this->y++;
   }
   this->savedX = Line_widthUntil(this->line, this->x);
   this->selecting = false;
}

void Buffer_forwardWord(Buffer* this) {
   if (this->x < last_x(this)) {
      this->x = String_forwardWord(this->line->text, last_x(this), this->x);
   } else if (this->line->super.next) {
      this->x = 0;
      Panel_onKey(this->panel, KEY_DOWN);
      this->y++;
   }
   this->savedX = Line_widthUntil(this->line, this->x);
   this->selecting = false;
}

void Buffer_backwardWord(Buffer* this) {
   if (this->x > 0) {
      this->x = String_backwardWord(this->line->text, last_x(this), this->x);
   } else if (this->y > 0) {
      Panel_onKey(this->panel, KEY_UP);
      this->line = (Line*) Panel_getSelected(this->panel);
      this->x = last_x(this);
      this->y--;
   }
   this->savedX = Line_widthUntil(this->line, this->x);
   this->selecting = false;
}

void Buffer_backwardChar(Buffer* this) {
   if (this->x > 0) {
      this->x--;
   } else if (this->y > 0) {
      Panel_onKey(this->panel, KEY_UP);
      this->line = (Line*) Panel_getSelected(this->panel);
      this->x = last_x(this);
      this->y--;
   }
   this->savedX = Line_widthUntil(this->line, this->x);
   this->selecting = false;
}

void Buffer_beginningOfLine(Buffer* this) {
   int prevX = this->x;
   this->x = 0;
   while (isblank(this->line->text[this->x])) {
      this->x++;
   }
   if (prevX == this->x)
      this->x = 0;
   this->savedX = Line_widthUntil(this->line, this->x);
   this->selecting = false;
}

void Buffer_endOfLine(Buffer* this) {
   this->x = last_x(this);
   this->savedX = Line_widthUntil(this->line, this->x);
   this->selecting = false;
}

void Buffer_beginningOfFile(Buffer* this) {
   Buffer_goto(this, 0, 0);
   this->selecting = false;
}

void Buffer_endOfFile(Buffer* this) {
   Buffer_goto(this, 0, Panel_size(this->panel) - 1);
   Buffer_endOfLine(this);
   this->selecting = false;
}

int Buffer_size(Buffer* this) {
   return Panel_size(this->panel);
}

void Buffer_previousPage(Buffer* this) {
   Buffer_goto(this, this->x, MAX(this->y - this->panel->h, 0));
   this->selecting = false;
}

void Buffer_nextPage(Buffer* this) {
   Buffer_goto(this, this->x, MAX(this->y + this->panel->h, 0));
   this->selecting = false;
}

void Buffer_wordWrap(Buffer* this, int wrap) {
   Undo_beginGroup(this->undo, this->x, this->y);
   while (this->line->super.next && ((Line*)this->line->super.next)->len > 0) {
      this->x = last_x(this);
      if (this->dosLineBreaks) {
         Undo_deleteCharAt(this->undo, this->x, this->y, Line_charAt(this->line, this->x));
         Line_deleteChars(this->line, this->x, 1);
      }
      if (this->line->text[this->x-1] != ' ')
         Buffer_defaultKeyHandler(this, ' ');
      Undo_joinNext(this->undo, this->x, this->y, false);
      Line_joinNext(this->line);
      while (this->line->text[this->x] == ' ') {
         Buffer_deleteChar(this);
      }
      this->line = (Line*) Panel_getSelected(this->panel);
      Buffer_wordWrap(this, wrap);
   }
   while (last_x(this) > wrap) {
      Line* oldLine = this->line;
      for(int i = wrap; i > 0; i--) {
         if (this->line->text[i] == ' ') {
            this->x = i;
            Buffer_deleteChar(this);
            Buffer_breakLine(this);
            break;
         }
      }
      if (this->line == oldLine)
         break;
   }
   this->panel->needsRedraw = true;
   Undo_endGroup(this->undo, this->x, this->y);
   Buffer_endOfLine(this);
}

void Buffer_deleteBlock(Buffer* this) {
   int yFrom = this->selectYfrom;
   int yTo = this->selectYto;
   int xFrom = this->selectXfrom;
   int xTo = this->selectXto;
   if (xFrom == xTo && yFrom == yTo)
      return;
   if (yFrom > yTo || (yFrom == yTo && xFrom > xTo)) {
      int tmp = yFrom; yFrom = yTo; yTo = tmp;
      tmp = xFrom; xFrom = xTo; xTo = tmp;
   }
   this->x = xFrom;
   this->y = yFrom;
   Panel_setSelected(this->panel, this->y);
   this->line = (Line*) Panel_getSelected(this->panel);
   StringBuffer* str = Line_deleteBlock(this->line, yTo - yFrom + 1, xFrom, xTo);
   int len = StringBuffer_len(str);
   assert (len > 0);
   char* block = StringBuffer_deleteGet(str);
   Undo_deleteBlock(this->undo, this->x, this->y, block, len);
   this->savedX = Line_widthUntil(this->line, this->x);
   this->selecting = false;
   this->modified = true;
   this->panel->needsRedraw = true;
}

char* Buffer_copyBlock(Buffer* this, int *len) {
   if (!this->selecting) {
      return NULL;
   }
   int yFrom = this->selectYfrom;
   int yTo = this->selectYto;
   int xFrom = this->selectXfrom;
   int xTo = this->selectXto;
   if (yFrom > yTo || (yFrom == yTo && xFrom > xTo)) {
      int tmp = yFrom; yFrom = yTo; yTo = tmp;
      tmp = xFrom; xFrom = xTo; xTo = tmp;
   }

   StringBuffer* str = Line_copyBlock((Line*) Panel_get(this->panel, yFrom), yTo - yFrom + 1, xFrom, xTo);
   this->selecting = false;
   this->panel->needsRedraw = true;
   *len = StringBuffer_len(str);
   return StringBuffer_deleteGet(str);
}

void Buffer_pasteBlock(Buffer* this, char* block, int len) {
   Undo_beginGroup(this->undo, this->x, this->y);
   if (this->selecting)
      Buffer_deleteBlock(this);
   if (len == 0) {
      Undo_endGroup(this->undo, this->x, this->y);
      return;
   }
   int newX;
   int newY = this->y;
   Undo_insertBlock(this->undo, this->x, this->y, block, len);
   Line_insertBlock(this->line, this->x, block, len, &newX, &newY);

   this->x = newX;
   this->y = newY;
   Panel_setSelected(this->panel, this->y);
   this->line = (Line*) Panel_getSelected(this->panel);
   this->savedX = Line_widthUntil(this->line, this->x);
   this->panel->needsRedraw = true;
   
   Undo_endGroup(this->undo, this->x, this->y);
   this->selecting = false;
   this->modified = true;
}

void Buffer_deleteChar(Buffer* this) {
   if (this->selecting) {
      Buffer_deleteBlock(this);
      return;
   }
   if (this->x < last_x(this)) {
      Undo_deleteCharAt(this->undo, this->x, this->y, Line_charAt(this->line, this->x));
      Line_deleteChars(this->line, this->x, 1);
   } else {
      if (this->line->super.next) {
         Undo_joinNext(this->undo, this->x, this->y, false);
         Line_joinNext(this->line);
         this->line = (Line*) Panel_getSelected(this->panel);
         this->y--;
      }
      this->panel->needsRedraw = true;
   }
   this->savedX = Line_widthUntil(this->line, this->x);
   this->modified = true;
}

void Buffer_pullText(Buffer* this) {
   Line* ref;
   if (this->y > this->savedY)
      ref = (Line*) this->line->super.prev;
   else
      ref = (Line*) this->line->super.next;
   if (!ref)
      return;

   int end = this->x;
   
   while (end > 0 && this->line->text[end] != ' ') end--;
   while (end < last_x(this) && this->line->text[end] == ' ') end++;
   if (end == last_x(this)) return;
   int at = end;
   if (at > 0) {
      do {
         at--;
      } while (at > 0 && ref->text[at] == ' ' && this->line->text[at] == ' ');
      if (at > 0) {
         while (at > 0 && ref->text[at] != ' ' && this->line->text[at] == ' ')
            at--;
         at++;
      }
   }
   int amt = end - at;
   if (amt <= 0) return;

   StringBuffer* str = Line_deleteBlock(this->line, 1, at, end);
   assert(amt == StringBuffer_len(str));
   char* block = StringBuffer_deleteGet(str);
   Undo_deleteBlock(this->undo, at, this->y, block, amt);

   if (at < this->x) {
      this->x = at;
      this->savedX = Line_widthUntil(this->line, this->x);
   }
   this->modified = true;
   this->selecting = false;
}

void Buffer_pushText(Buffer* this) {
   Line* ref;
   if (this->y > this->savedY)
      ref = (Line*) this->line->super.prev;
   else
      ref = (Line*) this->line->super.next;
   if (!ref)
      return;

   int start = this->x;
   if (this->line->text[this->x] != ' ')
      while (start > 0 && this->line->text[start] != ' ')
         start--;

   while (start < last_x(this) && this->line->text[start] == ' ') start++;
   if (start == last_x(this)) return;
   int at = start;
   if (ref->len > at) {
      if (ref->len > at && ref->text[at] != ' ')
         while (ref->len > at && ref->text[at] != ' ')
            at++;
      while (ref->len > at && ref->text[at] == ' ')
         at++;
   }
   int amt = at - start;
   char blanks[amt + 1];
   for (int i = 0; i < amt; i++) {
      blanks[i] = ' ';
   }
   blanks[amt] = '\0';
   
   Undo_insertBlanks(this->undo, start, this->y, amt);
   Line_insertStringAt(this->line, start, blanks, amt);

   this->modified = true;
   this->selecting = false;
}

void Buffer_backwardDeleteChar(Buffer* this) {
   if (this->selecting) {
      Buffer_deleteBlock(this);
      return;
   }
   if (this->x > 0) {
      this->x--;
      Undo_backwardDeleteCharAt(this->undo, this->x, this->y, Line_charAt(this->line, this->x));
      Line_deleteChars(this->line, this->x, 1);
   } else {
      if (this->line->super.prev) {
         if (this->dosLineBreaks) {
            Undo_beginGroup(this->undo, this->x, this->y);
         }
         Line* prev = (Line*) this->line->super.prev;
         this->x = prev->len;
         Undo_joinNext(this->undo, this->x, this->y - 1, true);
         Line_joinNext(prev);
         Panel_onKey(this->panel, KEY_UP);
         this->line = (Line*) Panel_getSelected(this->panel);
         this->y--;
         if (this->dosLineBreaks) {
            Buffer_backwardDeleteChar(this);
            Undo_endGroup(this->undo, this->x, this->y);
         }
      }
      this->panel->needsRedraw = true;
   }
   this->savedX = Line_widthUntil(this->line, this->x);
   this->modified = true;
}

void Buffer_upLine(Buffer* this) {
   this->savedY = this->y;
   Panel_onKey(this->panel, KEY_UP);
   Buffer_correctPosition(this);
   this->selecting = false;
}

void Buffer_downLine(Buffer* this) {
   this->savedY = this->y;
   Panel_onKey(this->panel, KEY_DOWN);
   Buffer_correctPosition(this);
   this->selecting = false;
}

void Buffer_slideUpLine(Buffer* this) {
   Panel_onKey(this->panel, KEY_C_UP);
   Buffer_correctPosition(this);
   this->selecting = false;
}

void Buffer_slideDownLine(Buffer* this) {
   Panel_onKey(this->panel, KEY_C_DOWN);
   Buffer_correctPosition(this);
   this->selecting = false;
}

void Buffer_correctPosition(Buffer* this) {
   this->line = (Line*) Panel_getSelected(this->panel);
   this->x = MIN(this->x, last_x(this));
   int screenX = Line_widthUntil(this->line, this->x);
   if (screenX > this->savedX) {
      while (this->x > 0 && Line_widthUntil(this->line, this->x) > this->savedX)
         this->x--;
   } else if (screenX < this->savedX) {
      while (this->x < last_x(this) && Line_widthUntil(this->line, this->x) < this->savedX)
         this->x++;
   }
   this->y = Panel_getSelectedIndex(this->panel);
}

inline void Buffer_blockOperation(Buffer* this, Line** firstLine, int* yStart, int* lines) {
   if (this->selecting) {
      int yFrom = this->selectYfrom;
      int yTo = this->selectYto;
      if (yFrom > yTo) {
         int tmp = yFrom; yFrom = yTo; yTo = tmp;
      }
      *lines = yTo - yFrom + (this->selectXto > 0 ? 1 : 0);
      *yStart = yFrom;
      *firstLine = (Line*) Panel_get(this->panel, *yStart);
   } else {
      *lines = 1;
      *yStart = this->y;
      *firstLine = this->line;
   }
}

void Buffer_unindent(Buffer* this) {
   Line* firstLine;
   int yStart, lines, spaces, move;
   if (this->tabCharacters) {
      spaces = 0;
      move = 1;
   } else {
      spaces = this->indentSpaces;
      move = spaces;
   }
   Buffer_blockOperation(this, &firstLine, &yStart, &lines);
   int* unindented = Line_unindent(firstLine, lines, spaces);
   Undo_unindent(this->undo, this->x, yStart, unindented, lines, this->tabCharacters);
   if (unindented[0] > 0 && this->y >= yStart && this->y <= (yStart + lines - 1))
      this->x = MAX(0, this->x - move);
   this->panel->needsRedraw = true;
   this->modified = true;
}

void Buffer_indent(Buffer* this) {
   Line* firstLine;
   int yStart, lines, spaces, move;
   if (this->tabCharacters) {
      if (!this->selecting) {
         Buffer_defaultKeyHandler(this, '\t');
         return;
      }
      spaces = 0;
      move = 1;
   } else {
      spaces = this->indentSpaces;
      move = spaces;
   }
   Buffer_blockOperation(this, &firstLine, &yStart, &lines);
   Undo_indent(this->undo, this->x, yStart, lines, spaces);
   Line_indent(firstLine, lines, spaces);
   if (this->y >= yStart && this->y <= (yStart + lines - 1))
      this->x += move;
   this->panel->needsRedraw = true;
   this->modified = true;
}
      
void Buffer_defaultKeyHandler(Buffer* this, int ch) {
   if ((ch >= 32 && ch <= 255) || ch == '\t' || ch == '\015') {
      if (this->selecting) {
         Buffer_deleteBlock(this);
      }
      Undo_insertCharAt(this->undo, this->x, this->y, ch);
      Line_insertCharAt(this->line, ch, this->x);
      this->x++;
      this->savedX = Line_widthUntil(this->line, this->x);
      this->modified = true;
   } else {
      Buffer_correctPosition(this);
   }
   this->selecting = false;
}

char Buffer_currentChar(Buffer* this) {
   assert(this->line);
   char* text = this->line->text;
   int len = last_x(this);
   int offset = MIN(this->x, len);
   if (this->x < len) {
      return text[offset];
   } else {
      return '\0';
   }
}

int Buffer_currentWord(Buffer* this, char* result, int resultLen) {

   assert(this->line);
   char* text = this->line->text;
   int len = last_x(this);
   int offset = MIN(this->x, len);
   int at = 0;
   int saveOffset = offset;
   if (this->x <= len) {
      while (offset > 0 && isword(text[offset-1]))
         offset--;
      for(int i = 0; i < resultLen && isword(text[offset]); i++) {
         *result = text[offset];
         result++;
         offset++;
         if (offset == saveOffset)
            at = i+1;
      }
   }
   *result = '\0';
   return at;
}

bool Buffer_find(Buffer* this, char* needle, bool findNext, bool caseSensitive, bool forward) {
   assert(this->line);
   if (this->selecting && !findNext)
      this->x = this->selectXfrom;
   Line* line = this->line;
   Line* start = line;
   int needleLen = strlen(needle);
   int y = this->y;
   char* haystack = line->text;
   int haystackLen = line->len;
   assert(line->text[line->len] == '\0');
   int offset = 0;
   if (forward) {
      offset = MIN(this->x, strlen(line->text));
      haystack += offset;
      haystackLen -= offset;
   } else {
      haystackLen = MAX(this->x - needleLen, 0);
   }
   do {
      int at;
      if (caseSensitive)
         at = String_indexOf(haystack, needle, haystackLen);
      else
         at = String_indexOf_i(haystack, needle, haystackLen);
      if (at != -1) {
         int x = at + offset;
         this->selectXfrom = x;
         this->selectYfrom = y;
         this->selectXto = x + needleLen;
         this->selectYto = y;
         this->selecting = true;
         Buffer_goto(this, x + needleLen, y);
         return true;
      }
      if (forward) {
         if (line->super.next) {
            line = (Line*) line->super.next;
            y++;
         } else {
            line = (Line*) this->panel->items->head;
            y = 0;
         }
      } else {
         if (line->super.prev) {
            line = (Line*) line->super.prev;
            y--;
         } else {
            line = (Line*) this->panel->items->tail;
            y = this->panel->items->size - 1;
         }
      }
      offset = 0;
      haystack = line->text;
      haystackLen = line->len;
   } while (line != start);
   if (this->x > 0) {
      if (!forward) {
         offset = MIN(this->x, strlen(line->text));
         haystack += offset;
         haystackLen -= offset;
      }
      int at;
      if (caseSensitive)
         at = String_indexOf(haystack, needle, haystackLen);
      else
         at = String_indexOf_i(haystack, needle, haystackLen);
      if (at != -1) {
         int x = at + offset;
         if (x < this->x + (findNext ? needleLen : 0)) {
            this->selectXfrom = x;
            this->selectYfrom = y;
            this->selectXto = x + strlen(needle);
            this->selectYto = y;
            this->selecting = true;
            Buffer_goto(this, x, y);
            return true;
         }
      }
   }
   return false;
}

bool Buffer_save(Buffer* this) {
   assert(this->fileName);
   FILE* fd = fopen(this->fileName, "w");
   if (!fd)
      return false;
   Line* l = (Line*) this->panel->items->head;
   while (l) {
      fwrite(l->text, l->len, 1, fd);
      l = (Line*) l->super.next;
      if (l)
         fwrite("\n", 1, 1, fd);
   }
   fclose(fd);
   Undo_store(this->undo, this->fileName);
   this->modified = false;
   this->readOnly = false;
   // Buffer_getMarks(this);
   return true;
}

void Buffer_resize(Buffer* this, int w, int h) {
   Panel_resize(this->panel, w-1, h);
}

void Buffer_refresh(Buffer* this) {
   this->panel->needsRedraw = true;
}

void Buffer_toggleMarking(Buffer* this) {
   this->marking = !this->marking;
}

void Buffer_toggleTabCharacters(Buffer* this) {
   this->tabCharacters = !this->tabCharacters;
}

void Buffer_toggleDosLineBreaks(Buffer* this) {
   this->dosLineBreaks = !this->dosLineBreaks;
}
