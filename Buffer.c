
#define _GNU_SOURCE
#include <ctype.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "Prototypes.h"

/*{

#ifndef isword
#define isword(x) (isalpha(x) || x == '_')
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
   // save context between calls to Buffer_Draw
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
   ListBox* panel;
   Undo* undo;
   // previous key for compound actions
   int lastKey;
};

struct FilePosition_ {
   int x;
   int y;
   char* name;
   FilePosition* next;
};

}*/

char* realpath(const char* path, char* resolved_path);

Buffer* Buffer_new(char* fileName, bool command) {
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

   this->readOnly = (access(fileName, R_OK) == 0 && access(fileName, W_OK) != 0);
   
   this->panel = ListBox_new(0, 0, COLS, LINES - 1, NormalColor, LINE_CLASS, true);
   this->undo = Undo_new(this->panel->items);
   this->fileName = String_copy(fileName);
   
   FileReader* file = FileReader_new(fileName, command);
   int i = 0;
   if (file && !FileReader_eof(file)) {
      int len = 0;
      char* text = FileReader_readLine(file, &len);
      this->hl = Highlight_new(fileName, text);
      Line* line = Line_new(text, len, this);
      ListBox_set(this->panel, i, (ListItem*) line);
      while (!FileReader_eof(file)) {
         i++;
         char* text = FileReader_readLine(file, &len);
         Line* line = Line_new(text, len, this);
         ListBox_set(this->panel, i, (ListItem*) line);
      }
   } else {
      this->hl = Highlight_new(fileName, "");
      ListBox_set(this->panel, 0, (ListItem*) Line_new(String_copy(""), 0, this));
   }
   if (file)
      FileReader_delete(file);

   this->savedContext = this->hl->mainContext;

   Buffer_restorePosition(this);
   Undo_restore(this->undo, fileName);

   return this;
}

inline void Buffer_restorePosition(Buffer* this) {
   char rpath[256];
   realpath(this->fileName, rpath);
   
   char fileposFileName[4097];
   snprintf(fileposFileName, 4096, "%s/.e/filepos", getenv("HOME"));
   fileposFileName[4095] = '\0';
   FILE* fd = fopen(fileposFileName, "r");
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

void Buffer_goto(Buffer* this, int x, int y) {
   ListBox_setSelected(this->panel, y);
   this->line = (Line*) ListBox_getSelected(this->panel);
   this->y = ListBox_getSelectedIndex(this->panel);
   this->x = MIN(x, this->line->len);
   this->panel->scrollV = MAX(0, this->y - (this->panel->h / 2));
   this->panel->needsRedraw = true;
}

inline void Buffer_storePosition(Buffer* this) {

   char rpath[4097];
   realpath(this->fileName, rpath);

   char fileposFileName[4097];
   snprintf(fileposFileName, 4096, "%s/.e", getenv("HOME"));
   fileposFileName[4095] = '\0';
   mkdir(fileposFileName, 0755);
   snprintf(fileposFileName, 4096, "%s/.e/filepos", getenv("HOME"));
   fileposFileName[4095] = '\0';
   FILE* fd = fopen(fileposFileName, "r");
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
   fd = fopen(fileposFileName, "w");
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

void Buffer_draw(Buffer* this) {
   ListBox* p = this->panel;

   if (this->wasSelecting && !this->selecting)
      p->needsRedraw = true;

   this->y = ListBox_getSelectedIndex(p);
   this->line = (Line*) ListBox_getSelected(p);
   
   Line_updateContext(this->line);
   
   if (this->line->context != this->savedContext)
      p->needsRedraw = true;
   this->savedContext = this->line->context;
      
   if (this->x > this->line->len)
      this->x = this->line->len;

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
   ListBox_draw(p, true);
   
   this->wasSelecting = this->selecting;
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
   assert (this->x >= 0 && this->x <= this->line->len);
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
   Buffer_storePosition(this);
   free(this->fileName);
   ((Object*) this->panel)->delete((Object*) this->panel);

   Undo_delete(this->undo);
   Highlight_delete(this->hl);
   free(this);
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

void Buffer_undo(Buffer* this) {
   int ux = this->x;
   int uy = this->y;
   Undo_undo(this->undo, &ux, &uy);
   this->x = ux;
   ListBox_setSelected(this->panel, uy);
   this->line = (Line*) ListBox_getSelected(this->panel);
   this->panel->needsRedraw = true;
   this->savedX = Line_widthUntil(this->line, this->x);
   this->selecting = false;
   this->modified = true;
}

void Buffer_breakLine(Buffer* this) {
   if (this->selecting) {
      Buffer_deleteBlock(this);
   }

   int indent = MIN(Line_getIndentWidth(this->line), this->x);
   Undo_breakAt(this->undo, this->x, this->y, indent);
   Line_breakAt(this->line, this->x, indent);
   ListBox_onKey(this->panel, KEY_DOWN);
   this->line = (Line*) ListBox_getSelected(this->panel);
   this->x = indent;
   this->y++;
   this->panel->needsRedraw = true;
   this->modified = true;
   
   if (this->lastKey == '{') {
      Buffer_defaultKeyHandler(this, '}');
      Buffer_backwardChar(this);
      Buffer_breakLine(this);
      Buffer_upLine(this);
      Buffer_indent(this);
   }
}

void Buffer_forwardChar(Buffer* this) {
   if (this->x < this->line->len) {
      this->x++;
   } else if (this->line->super.next) {
      this->x = 0;
      ListBox_onKey(this->panel, KEY_DOWN);
      this->y++;
   }
   this->savedX = Line_widthUntil(this->line, this->x);
}

void Buffer_forwardWord(Buffer* this) {
   if (this->x < this->line->len) {
      this->x = String_forwardWord(this->line->text, this->line->len, this->x);
   } else if (this->line->super.next) {
      this->x = 0;
      ListBox_onKey(this->panel, KEY_DOWN);
      this->y++;
   }
   this->savedX = Line_widthUntil(this->line, this->x);
}

void Buffer_backwardWord(Buffer* this) {
   if (this->x > 0) {
      this->x = String_backwardWord(this->line->text, this->line->len, this->x);
   } else if (this->y > 0) {
      ListBox_onKey(this->panel, KEY_UP);
      this->line = (Line*) ListBox_getSelected(this->panel);
      this->x = this->line->len;
      this->y--;
   }
   this->savedX = Line_widthUntil(this->line, this->x);
}

void Buffer_backwardChar(Buffer* this) {
   if (this->x > 0) {
      this->x--;
   } else if (this->y > 0) {
      ListBox_onKey(this->panel, KEY_UP);
      this->line = (Line*) ListBox_getSelected(this->panel);
      this->x = this->line->len;
      this->y--;
   }
   this->savedX = Line_widthUntil(this->line, this->x);
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
}

void Buffer_endOfLine(Buffer* this) {
   this->x = this->line->len;
   this->savedX = Line_widthUntil(this->line, this->x);
}

void Buffer_beginningOfFile(Buffer* this) {
   Buffer_goto(this, 0, 0);
}

void Buffer_endOfFile(Buffer* this) {
   Buffer_goto(this, 0, ListBox_size(this->panel) - 1);
   Buffer_endOfLine(this);
}

void Buffer_previousPage(Buffer* this) {
   Buffer_goto(this, this->x, this->y - this->panel->h);
}

void Buffer_nextPage(Buffer* this) {
   Buffer_goto(this, this->x, this->y + this->panel->h);
}

void Buffer_deleteBlock(Buffer* this) {
   int yFrom = this->selectYfrom;
   int yTo = this->selectYto;
   int xFrom = this->selectXfrom;
   int xTo = this->selectXto;
   if (yFrom > yTo || (yFrom == yTo && xFrom > xTo)) {
      int tmp = yFrom; yFrom = yTo; yTo = tmp;
      tmp = xFrom; xFrom = xTo; xTo = tmp;
   }
   this->x = xFrom;
   this->y = yFrom;
   ListBox_setSelected(this->panel, this->y);
   this->line = (Line*) ListBox_getSelected(this->panel);
   StringBuffer* str = Line_deleteBlock(this->line, yTo - yFrom + 1, xFrom, xTo);
   int len = StringBuffer_len(str);
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

   StringBuffer* str = Line_copyBlock((Line*) ListBox_get(this->panel, yFrom), yTo - yFrom + 1, xFrom, xTo);
   this->selecting = false;
   this->panel->needsRedraw = true;
   *len = StringBuffer_len(str);
   return StringBuffer_deleteGet(str);
}

void Buffer_pasteBlock(Buffer* this, char* block, int len) {
   if (this->selecting)
      Buffer_deleteBlock(this);
   if (len == 0)
      return;
   int newX;
   int newY = this->y;
   Undo_insertBlock(this->undo, this->x, this->y, block, len);
   Line_insertBlock(this->line, this->x, block, len, &newX, &newY);

   this->x = newX;
   this->y = newY;
   ListBox_setSelected(this->panel, this->y);
   this->line = (Line*) ListBox_getSelected(this->panel);
   this->savedX = Line_widthUntil(this->line, this->x);
   this->panel->needsRedraw = true;

   this->selecting = false;
   this->modified = true;
}

void Buffer_deleteChar(Buffer* this) {
   if (this->selecting) {
      Buffer_deleteBlock(this);
      return;
   }
   if (this->x < this->line->len) {
      Undo_deleteCharAt(this->undo, this->x, this->y, Line_charAt(this->line, this->x));
      Line_deleteChars(this->line, this->x, 1);
   } else {
      if (this->line->super.next) {
         Undo_joinNext(this->undo, this->x, this->y);
         Line_joinNext(this->line);
         this->line = (Line*) ListBox_getSelected(this->panel);
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
   while (end < this->line->len && this->line->text[end] == ' ') end++;
   if (end == this->line->len) return;
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

   while (start < this->line->len && this->line->text[start] == ' ') start++;
   if (start == this->line->len) return;
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
      Undo_deleteCharAt(this->undo, this->x, this->y, Line_charAt(this->line, this->x));
      Line_deleteChars(this->line, this->x, 1);
   } else {
      if (this->line->super.prev) {
         Line* prev = (Line*) this->line->super.prev;
         this->x = prev->len;
         Undo_joinNext(this->undo, this->x, this->y - 1);
         Line_joinNext(prev);
         ListBox_onKey(this->panel, KEY_UP);
         this->line = (Line*) ListBox_getSelected(this->panel);
         this->y--;
      }
      this->panel->needsRedraw = true;
   }
   this->savedX = Line_widthUntil(this->line, this->x);
   this->modified = true;
}

void Buffer_upLine(Buffer* this) {
   this->savedY = this->y;
   ListBox_onKey(this->panel, KEY_UP);
   Buffer_correctPosition(this);
}

void Buffer_downLine(Buffer* this) {
   this->savedY = this->y;
   ListBox_onKey(this->panel, KEY_DOWN);
   Buffer_correctPosition(this);
}

void Buffer_slideUpLine(Buffer* this) {
   ListBox_onKey(this->panel, KEY_C_UP);
   Buffer_correctPosition(this);
}

void Buffer_slideDownLine(Buffer* this) {
   ListBox_onKey(this->panel, KEY_C_DOWN);
   Buffer_correctPosition(this);
}

void Buffer_correctPosition(Buffer* this) {
   this->line = (Line*) ListBox_getSelected(this->panel);
   this->x = MIN(this->x, this->line->len);
   int screenX = Line_widthUntil(this->line, this->x);
   if (screenX > this->savedX) {
      while (this->x > 0 && Line_widthUntil(this->line, this->x) > this->savedX)
         this->x--;
   } else if (screenX < this->savedX) {
      while (this->x < this->line->len && Line_widthUntil(this->line, this->x) < this->savedX)
         this->x++;
   }
   this->lastKey = 0;
   this->y = ListBox_getSelectedIndex(this->panel);
}

inline void Buffer_blockOperation(Buffer* this, Line** firstLine, int* yStart, int* lines) {
   if (this->selecting) {
      int yFrom = this->selectYfrom;
      int yTo = this->selectYto;
      if (yFrom > yTo) {
         int tmp = yFrom; yFrom = yTo; yTo = tmp;
      }
      *lines = yTo - yFrom;
      *yStart = yFrom;
      *firstLine = (Line*) ListBox_get(this->panel, *yStart);
   } else {
      *lines = 1;
      *yStart = this->y;
      *firstLine = this->line;
   }
}

void Buffer_unindent(Buffer* this) {
   Line* firstLine;
   int yStart;
   int lines;
   Buffer_blockOperation(this, &firstLine, &yStart, &lines);
   int* unindented = Line_unindent(firstLine, lines);
   Undo_unindent(this->undo, this->x, yStart, unindented, lines);
   if (this->y >= yStart && this->y <= (yStart + lines - 1))
      this->y = MAX(0, this->y - INDENT_WIDTH);
   this->panel->needsRedraw = true;
   this->modified = true;
   this->lastKey = 0;
}

void Buffer_indent(Buffer* this) {
   Line* firstLine;
   int yStart;
   int lines;
   Buffer_blockOperation(this, &firstLine, &yStart, &lines);
   Undo_indent(this->undo, this->x, yStart, lines);
   Line_indent(firstLine, lines);
   if (this->y >= yStart && this->y <= (yStart + lines - 1))
      this->x += INDENT_WIDTH;
   this->panel->needsRedraw = true;
   this->modified = true;
   this->lastKey = 0;
}
      
void Buffer_defaultKeyHandler(Buffer* this, int ch) {
   bool handled = ListBox_onKey(this->panel, ch);

   this->lastKey = ch;
   if (!handled && ((ch >= 32 && ch <= 255) || ch == '\t')) {
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
   int len = this->line->len;
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
   int len = this->line->len;
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
   return true;
}

void Buffer_resize(Buffer* this) {
   ListBox_resize(this->panel, COLS, LINES - 1);
}
