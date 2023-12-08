
#define _GNU_SOURCE
#include <ctype.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include <iconv.h>
#include <editorconfig/editorconfig.h>

#include "Prototypes.h"
//#needs Line
//#needs Script

/*{

#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif
#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

#ifndef iswword
#define iswword(x) (iswalpha(x) || x == '_')
#endif

#ifndef last_x_line
#define last_x_line(this, line) (this->dosLineBreaks ? Line_chars(line) - 1 : Line_chars(line))
#endif

#ifndef last_x
#define last_x(this) last_x_line(this, this->line)
#endif

typedef int chars;
typedef int cells;

typedef enum UndoMode_ {
   UNDO,
   REDO,
} UndoMode;

struct Buffer_ {
   char* fileName;
   bool modified;
   bool readOnly;

   char* autosave;
   int autosaveCounter;

   // logical position of the cursor in the line
   // (character of the line where cursor is.
   chars x;
   int y;
   Line* line;
   // save X so that cursor returns to original X
   // after passing by a shorter line.
   cells savedX;
   int savedY;
   Highlight* hl;
   // save context between calls to Buffer_draw
   // to detect highlight changes that demand
   // a redraw
   HighlightContext* savedContext;
   // transient popup
   char** popup;
   // coordinates of selection
   chars selectXfrom;
   int selectYfrom;
   chars selectXto;
   int selectYto;
   // coordinates of highlighted bracket
   cells bracketX;
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
   // tabulation used in document: 0 means Tabs; n>0 means n spaces.
   int tabulation;
   // backup variable for toggling tab mode
   int saveTabulation;
   chars saveTabulationX;
   int saveTabulationY;
   // document uses DOS-style ctrl-M
   bool dosLineBreaks;
   // trim trailing whitespace when saving
   bool trimTrailingWhitespace;
   // make sure file ends with a line break
   bool insertFinalNewline;
   // document uses UTF-8 (if false, assume ISO-8859-15)
   bool isUTF8;
   // time tracker to disable auto-indent when pasting;
   double lastTime;
   // size of Tab (\t) in cells
   int tabSize;
   int nCursors;
   struct {
      chars x;
      int y;
      chars savedX;
      int savedY;
      chars selectXfrom;
      int selectYfrom;
      chars selectXto;
      int selectYto;
      bool selecting;
      int xLen;
      int yLen;
   } cursors[999];
   // Lua state
   ScriptState script;
   bool skipOnChange;
   bool skipOnKey;
   bool skipOnCtrl;
   bool skipOnAlt;
   bool skipOnFKey;
   bool skipOnSave;
   bool skipAfterKey;
};

struct Coords_ {
   int x;
   int y;
};

struct FilePosition_ {
   chars x;
   int y;
   char* name;
   FilePosition* next;
};

}*/

static inline void onChange(Buffer* this) {
   this->autosaveCounter++;
   Script_onChange(this);
}

inline void Buffer_restorePosition(Buffer* this) {
   char* rpath = realpath(this->fileName, NULL);
   
   FILE* fd = Files_openHome("r", "filepos", NULL);
   if (fd) {
      chars x;
      int y;
      char line[256];
      while (!feof(fd)) {
         int n = fscanf(fd, "%d %d %255[^\n]\n", &x, &y, line);
         if (n != 3) {
            break;
         }
         if (strcmp(line, rpath) == 0) {
            Line* line = (Line*) this->panel->items->head;
            for (int i = 0; line && i <= y; i++) {
               Line_display((Object*)line, NULL);
               line = (Line*) line->super.next;
            }
            Buffer_goto(this, x, y, true);
            break;
         }
      }
      fclose(fd);
   }
   free(rpath);
}

void Buffer_autoConfigureIndent(Buffer* this, int indents[]) {
   int detectedIndent = 3;
   if (indents[3] > indents[2] && indents[3] > indents[4]) {
      detectedIndent = 3;
   } else if (indents[2] > 0 && indents[6] > 0) {
      detectedIndent = 2;
   } else if (indents[4] > 0) {
      detectedIndent = 4;
   }

   if (indents[0]) {
      this->tabulation = 0;
   } else {
      this->tabulation = detectedIndent;
   }
   this->saveTabulation = detectedIndent;
}

static void Buffer_convertToUTF8(Buffer* this) {
   iconv_t cd = iconv_open("UTF-8", "ISO-8859-15");
   Line* l = (Line*) this->panel->items->head;
   while (l) {
      char* intext = Line_toString(l);
      size_t insize = Line_bytes(l);
      size_t outsize = insize * 4 + 1;
      char* outtext = calloc(outsize, 1);
      size_t outleft = outsize;
      char* outptr = outtext;
      int err = iconv(cd, &intext, &insize, &outptr, &outleft);
      if (err != -1) {
         int size = outsize - outleft;
         outtext = realloc(outtext, size + 1);
         Text_replace(&(l->text), outtext, size);
      }
      l = (Line*) l->super.next;
   }
   iconv_close(cd);
}

static void Buffer_checkEditorConfig(Buffer* this, const char* fileName) {
   editorconfig_handle eh;
   eh = editorconfig_handle_init();
   int ehErr = editorconfig_parse(fileName, eh);
   if (ehErr == 0) {
      int n = editorconfig_handle_get_name_value_count(eh);
      bool useTab = false;
      int indentSize = 0;
      int tabWidth = 0;
      for (int i = 0; i < n; i++) {
         const char *name, *value;
         editorconfig_handle_get_name_value(eh, i, &name, &value);
         int v = 0;
         if (value) v = atoi(value);
         if (strcmp(name, "indent_style") == 0) {
            if (strcmp(value, "tab") == 0) {
               useTab = true;
            }
         } else if (strcmp(name, "indent_size") == 0) {
            if (strcmp(value, "tab") == 0) {
               useTab = true;
            } else if (v >= 1 && v <= 8) {
               indentSize = v;
            }
         } else if (strcmp(name, "tab_width") == 0) {
            if (v >= 1 && v <= 8) {
               tabWidth = v;
            }
         } else if (strcmp(name, "end_of_line") == 0) {
            if (strcmp(value, "crlf") == 0) {
               this->dosLineBreaks = true;
            } else {
               this->dosLineBreaks = false;
            }
         } else if (strcmp(name, "charset") == 0) {
            if (strcmp(value, "latin1") == 0) {
               this->isUTF8 = false;
            } else {
               this->isUTF8 = true;
            }
         } else if (strcmp(name, "trim_trailing_whitespace") == 0) {
            if (strcmp(value, "true") == 0) {
               this->trimTrailingWhitespace = true;
            } else {
               this->trimTrailingWhitespace = false;
            }
         } else if (strcmp(name, "insert_final_newline") == 0) {
            if (strcmp(value, "true") == 0) {
               this->insertFinalNewline = true;
            } else {
               this->insertFinalNewline = false;
            }
         }
      }
      if (tabWidth > 0) {
         this->tabSize = tabWidth;
      }
      if (useTab) {
         this->tabulation = 0;
         if (indentSize > 0) {
            this->tabSize = indentSize;
         }
      } else {
         if (indentSize > 0) {
            this->tabulation = indentSize;
         }
      }
   }
   editorconfig_handle_destroy(eh);
}

Buffer* Buffer_new(int x, int y, int w, int h, char* fileName, bool command, TabManager* tabs) {
   Buffer* this = (Buffer*) calloc(sizeof(Buffer), 1);

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
   this->tabulation = 3;
   this->dosLineBreaks = false;
   this->trimTrailingWhitespace = false;
   this->insertFinalNewline = false;
   this->tabSize = 8;
   this->nCursors = 0;
   
   Script_initState(&this->script, tabs, this);
   
   /* Hack to disable auto-indent when pasting through X11, part 1 */
   struct timeval tv;
   gettimeofday(&tv, NULL);
   this->lastTime = tv.tv_sec * 1000000 + tv.tv_usec;

   Panel* p = Panel_new(x, y, w-1, h, CRT_colors[NormalColor], ClassAs(Line, ListItem), true, this);
   this->panel = p;
   this->undo = Undo_new(p->items);
   this->fileName = NULL;
   this->isUTF8 = true;
   bool newFile = true;
   if (fileName) {
      int indents[9] = { 0,0,0,0,0,0,0,0,0 };
      this->fileName = strdup(fileName);
      this->readOnly = (access(fileName, R_OK) == 0 && access(fileName, W_OK) != 0);
     
      FileReader* file = FileReader_new(fileName, command);
      int i = 0;
      if (file && !FileReader_eof(file)) {
         newFile = false;
         Text text = Text_new(FileReader_readLine(file));
         this->isUTF8 = (this->isUTF8 && UTF8_isValid(Text_toString(text)));
         this->hl = Highlight_new(fileName, text, &this->script);
         if (Text_toString(text)[0] == '\t') indents[0] = 1;
         if (Text_hasChar(text, '\015')) this->dosLineBreaks = true;
         Line* line = Line_new(p->items, text, this->hl->mainContext);
         Panel_set(p, i, (ListItem*) line);
         while (!FileReader_eof(file)) {
            i++;
            char* t = FileReader_readLine(file);
            if (!t)
               continue;
            Text text = Text_new(t);
            this->isUTF8 = (this->isUTF8 && UTF8_isValid(t));
            if (t[0] == '\t') {
               indents[0]++;
            } else {
               int spaces = 0;
               while (*t == ' ' && spaces < 8) { spaces++; t++; }
               if (spaces >= 2) {
                  indents[spaces]++;
               }
            }
            Line* line = Line_new(p->items, text, this->hl->mainContext);
            Panel_set(p, i, (ListItem*) line);
         }
         FileReader_delete(file);
         Buffer_restorePosition(this);
         Undo_restore(this->undo, fileName);
         Buffer_autoConfigureIndent(this, indents);
      } else {
         this->modified = true;
      }
      Buffer_checkEditorConfig(this, fileName);
      if (!this->isUTF8) {
         Buffer_convertToUTF8(this);
      }
   } else {
      this->readOnly = false;
   }
   if (newFile) {
      this->hl = Highlight_new(fileName, Text_new(""), &this->script);
      Panel_set(p, 0, (ListItem*) Line_new(p->items, Text_new(strdup("")), this->hl->mainContext));
   }
   Script_highlightFile(this->hl, fileName);
   
   this->savedContext = this->hl->mainContext;

   return this;
}

inline void Buffer_storePosition(Buffer* this) {

   const char* rpath = this->fileName;

   FILE* fd = Files_openHome("r", "filepos", NULL);
   FilePosition* fps = NULL;
   FilePosition* prev = NULL;
   bool set = false;
   if (fd) {
      while (!feof(fd)) {
         FilePosition* fp = malloc(sizeof(FilePosition));
         fp->name = malloc(4097);
         fp->next = NULL;
         int n = fscanf(fd, "%d %d %4096[^\n]\n", &(fp->x), &(fp->y), fp->name);
         if (n != 3) {
            break;
         }
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

void Buffer_goto(Buffer* this, chars x, int y, bool adjustScroll) {
   if (y < 0)
      y += Panel_size(this->panel);
   Panel_setSelected(this->panel, y);
   this->line = (Line*) Panel_getSelected(this->panel);
   this->y = Panel_getSelectedIndex(this->panel);
   this->x = MIN(MAX(0, x), last_x(this));
   if (adjustScroll) {
      this->panel->scrollV = MAX(0, this->y - (this->panel->h / 2));
   }
   this->panel->needsRedraw = true;
}

void Buffer_move(Buffer* this, int x) {
   this->x = MIN(x, last_x(this));
   this->panel->needsRedraw = true;
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
   if (Line_chars(l) == 0) return;
   int ch = Line_charAt(l, x);
   if (!Buffer_matchBracket(ch, &oth, &dir)) {
      if (x == 0)
         return;
      ch = Line_charAt(l, --x);
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
            x = Line_chars(l) - 1;
            if (y < this->y - this->panel->h) {
               return;
            }
         } else
            return;
      } else if (x >= Line_chars(l)) {
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
      char at = Line_charAt(l, x);
      if (at == ch) {
         level++;
      } else if (at == oth) {
         level--;
         if (level == 0) {
            this->bracketX = Line_widthUntil(l, x, this->tabSize);
            this->bracketY = y;
            if (y != this->y)
               this->panel->needsRedraw = true;
            return;
         }
      }
      x += dir;
   }
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
   int screenX = Line_widthUntil(this->line, this->x, this->tabSize);

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
   
   if (this->popup) {
      int cursorY, cursorX;
      Display_getyx(&cursorY, &cursorX);
      int maxlen = 0;
      int height = 0;
      for (int i = 0; this->popup[i]; i++) {
         int len = strlen(this->popup[i]);
         maxlen = MAX(maxlen, len);
         height++;
      }
      char buf[maxlen + 1];
      memset(buf, ' ', maxlen);
      buf[maxlen] = '\0';
      int firstY = this->y - p->scrollV + 1;
      if (firstY + height > p->h && (firstY - 1 > height)) {
         firstY = firstY - 1 - height;
      }
      for (int i = 0; this->popup[i]; i++) {
         RichString_begin(line);
         RichString_write(&line, 0, this->popup[i]);
         RichString_appendn(&line, 0, buf, maxlen);
         RichString_setAttrn(&line, CRT_colors[PopupColor], 0, maxlen);
         int y = firstY + i;
         if (y < p->h) {
            int x = screenX - p->scrollH;
            if (p->w - x < maxlen && p->w > maxlen) {
               x = p->w - maxlen;
            }
            Display_writeChstrAtn(y, x, RichString_at(line, 0), MIN(maxlen, p->w - x));
         }
         RichString_end(line);
         free(this->popup[i]);
      }
      free(this->popup);
      p->needsRedraw = true;
      Display_move(cursorY, cursorX);
   }
   this->popup = NULL;
   
   this->wasSelecting = this->selecting;
}

int Buffer_x(Buffer* this) {
   return this->x;
}

int Buffer_y(Buffer* this) {
   return this->y;
}

int Buffer_scrollV(Buffer* this) {
   return this->panel->scrollV;
}

int Buffer_scrollH(Buffer* this) {
   return this->panel->scrollH;
}

const char* Buffer_currentLine(Buffer* this) {
   return Line_toString(this->line);
}

inline const char* Buffer_getLine(Buffer* this, int i) {
   Line* line = (Line*) Panel_get(this->panel, i);
   if (line) 
      return Line_toString(line);
   else
      return NULL;
}

inline int Buffer_getLineLength(Buffer* this, int i) {
   Line* line = (Line*) Panel_get(this->panel, i);
   if (line) 
      return Line_chars(line);
   else
      return -1;
}

const char* Buffer_previousLine(Buffer* this) {
   return Buffer_getLine(this, this->y - 1);
}

void Buffer_delete(Buffer* this) {
   if (this->fileName) {
      Buffer_storePosition(this);
      free(this->fileName);
   }
   if (this->autosave) {
      remove(this->autosave);
      free(this->autosave);
   }
   Msg0(Object, delete, this->panel);

   Undo_delete(this->undo);
   Highlight_delete(this->hl);
   Script_doneState(&this->script);
   free(this);
}

void Buffer_refreshHighlight(Buffer* this) {
   Highlight_delete(this->hl);
   Line* firstLine = (Line*) Panel_get(this->panel, 0);
   Text firstText;
   if (firstLine)
      firstText = firstLine->text;
   else
      firstText = Text_new("");
   Highlight* hl = Highlight_new(this->fileName, firstText, &this->script);
   this->hl = hl;
   int size = Panel_size(this->panel);
   
   for (int i = 0; i < size; i++) {
      Line* line = (Line*) Panel_get(this->panel, i);
      int hlAttrs[line->text.bytes];
      Highlight_setAttrs(hl, line->text.data, hlAttrs, line->text.bytes, i + 1);
      line->context = Highlight_getContext(hl);
   }
   
   this->panel->needsRedraw = true;
}

void Buffer_select(Buffer* this, void(*motion)(Buffer*)) {
   if (!this->selecting) {
      this->selectXfrom = this->x;
      this->selectYfrom = this->y;
      this->selectYto = this->y;
   }
   int oldSelectYfrom = this->selectYfrom;
   int oldSelectYto = this->selectYto;
   motion(this);
   this->selectXto = this->x;
   this->selectYto = this->y;
   this->selecting = true;
   this->panel->needsRedraw = true;
   if (this->selectYfrom != oldSelectYfrom || this->selectYto != oldSelectYto)
      this->panel->needsRedraw = true;
}

void Buffer_setSelection(Buffer* this, int xFrom, int yFrom, int xTo, int yTo) {
   Buffer_validateCoordinate(this, &xFrom, &yFrom);
   Buffer_validateCoordinate(this, &xTo, &yTo);
   this->selecting = true;
   this->selectXfrom = xFrom;
   this->selectYfrom = yFrom;
   this->selectXto = xTo;
   this->selectYto = yTo;
   this->x = xTo;
   this->y = yTo;
   this->panel->needsRedraw = true;
   this->savedX = this->x;
}

void Buffer_selectAll(Buffer* this) {
   Buffer_goto(this, 1, 1, true);
   Buffer_select(this, Buffer_endOfFile);
}

bool Buffer_checkDiskState(Buffer* this) {
   return Undo_checkDiskState(this->undo);
}

void Buffer_undo(Buffer* this, UndoMode mode) {
   int ux = this->x;
   int uy = this->y;
   bool modified;
   if (mode == UNDO) {
      modified = Undo_undo(this->undo, &ux, &uy);
   } else {
      modified = Undo_redo(this->undo, &ux, &uy);
   }
   this->x = ux;
   Panel_setSelected(this->panel, uy);
   this->line = (Line*) Panel_getSelected(this->panel);
   this->panel->needsRedraw = true;
   this->savedX = Line_widthUntil(this->line, this->x, this->tabSize);
   this->selecting = false;
   this->modified = modified;
   
   onChange(this);
}

char Buffer_getLastKey(Buffer* this) {
   return this->lastKey;
}

void Buffer_breakLine(Buffer* this) {
   if (this->selecting) {
      Buffer_deleteBlock(this);
   }

   if (this->dosLineBreaks) {
      Undo_beginGroup(this->undo, this->x, this->y);
      Buffer_defaultKeyHandler(this, '\015', false);
   }

   bool doIndent = true;
   if (this->nCursors < 2) {
      /* Hack to disable auto-indent when pasting through X11, part 2 */
      struct timeval tv;
      gettimeofday(&tv, NULL);
      double now = tv.tv_sec * 1000000 + tv.tv_usec;
      if (now - this->lastTime < 10000)
         doIndent = false;
      this->lastTime = now;
   }

   int indent = Line_breakAt(this->line, this->x, doIndent);
   Undo_breakAt(this->undo, this->x, this->y, indent);
   
   Panel_onKey(this->panel, KEY_DOWN);
   this->line = (Line*) Panel_getSelected(this->panel);
   this->x = indent;
   this->y++;
   this->panel->needsRedraw = true;
   this->modified = true;

   if (this->dosLineBreaks) {
      Undo_endGroup(this->undo, this->x, this->y);
   }

   this->lastKey = 0;

   onChange(this);
}

void Buffer_forwardChar(Buffer* this) {
   if (this->x < last_x(this)) {
      this->x++;
   } else if (this->line->super.next) {
      this->x = 0;
      Panel_onKey(this->panel, KEY_DOWN);
      this->y++;
   }
   this->savedX = Line_widthUntil(this->line, this->x, this->tabSize);
   this->selecting = false;
}

void Buffer_forwardWord(Buffer* this) {
   if (this->x < last_x(this)) {
      this->x = Text_forwardWord(this->line->text, this->x, this->nCursors > 1);
   } else if (this->line->super.next) {
      this->x = 0;
      Panel_onKey(this->panel, KEY_DOWN);
      this->y++;
   }
   this->savedX = Line_widthUntil(this->line, this->x, this->tabSize);
   this->selecting = false;
}

void Buffer_backwardWord(Buffer* this) {
   if (this->x > 0) {
      this->x = Text_backwardWord(this->line->text, this->x, this->nCursors > 1);
   } else if (this->y > 0) {
      Panel_onKey(this->panel, KEY_UP);
      this->line = (Line*) Panel_getSelected(this->panel);
      this->x = last_x(this);
      this->y--;
   }
   this->savedX = Line_widthUntil(this->line, this->x, this->tabSize);
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
   this->savedX = Line_widthUntil(this->line, this->x, this->tabSize);
   this->selecting = false;
}

void Buffer_beginUndoGroup(Buffer* this) {
   Undo_beginGroup(this->undo, this->x, this->y);
}

void Buffer_endUndoGroup(Buffer* this) {
   Undo_endGroup(this->undo, this->x, this->y);
}

void Buffer_beginningOfLine(Buffer* this) {
   int prevX = this->x;
   this->x = 0;
   while (iswblank(Line_charAt(this->line, this->x))) {
      this->x++;
   }
   if (prevX == this->x)
      this->x = 0;
   this->savedX = Line_widthUntil(this->line, this->x, this->tabSize);
   this->selecting = false;
}

void Buffer_endOfLine(Buffer* this) {
   this->x = last_x(this);
   this->savedX = Line_widthUntil(this->line, this->x, this->tabSize);
   this->selecting = false;
}

void Buffer_beginningOfFile(Buffer* this) {
   Buffer_goto(this, 0, 0, true);
   this->selecting = false;
}

void Buffer_endOfFile(Buffer* this) {
   Buffer_goto(this, 0, Panel_size(this->panel) - 1, true);
   Buffer_endOfLine(this);
   this->selecting = false;
}

int Buffer_size(Buffer* this) {
   return Panel_size(this->panel);
}

void Buffer_previousPage(Buffer* this) {
   Buffer_goto(this, this->x, MAX(this->y - this->panel->h, 0), true);
   this->selecting = false;
}

void Buffer_nextPage(Buffer* this) {
   Buffer_goto(this, this->x, MAX(this->y + this->panel->h, 0), true);
   this->selecting = false;
}

void Buffer_wordWrap(Buffer* this, int wrap) {
   Undo_beginGroup(this->undo, this->x, this->y);
   int minLen = this->dosLineBreaks ? 1 : 0;
   while (this->line->super.next && Line_chars((Line*)this->line->super.next) > minLen) {
      this->x = last_x(this);
      if (this->dosLineBreaks) {
         Undo_deleteCharAt(this->undo, this->x, this->y, Line_charAt(this->line, this->x));
         Line_deleteChars(this->line, this->x, 1);
      }
      if (Line_charAt(this->line, this->x-1) != ' ')
         Buffer_defaultKeyHandler(this, ' ', false);
      Undo_joinNext(this->undo, this->x, this->y, false);
      Line_joinNext(this->line);
      while (Line_charAt(this->line, this->x) == ' ') {
         Buffer_deleteChar(this);
      }
      this->line = (Line*) Panel_getSelected(this->panel);
      Buffer_wordWrap(this, wrap);
   }
   while (last_x(this) > wrap) {
      Line* oldLine = this->line;
      for(int i = wrap; i > 0; i--) {
         if (Line_charAt(this->line, i) == ' ') {
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

   onChange(this);
}

void Buffer_deleteBlock(Buffer* this) {
   int yFrom = this->selectYfrom;
   int yTo = this->selectYto;
   int xFrom = this->selectXfrom;
   int xTo = this->selectXto;
   if (! this->selecting || (xFrom == xTo && yFrom == yTo))
      return;
   if (yFrom > yTo || (yFrom == yTo && xFrom > xTo)) {
      int swap = yFrom; yFrom = yTo; yTo = swap;
      swap = xFrom; xFrom = xTo; xTo = swap;
   }
   this->x = xFrom;
   this->y = yFrom;
   Panel_setSelected(this->panel, this->y);

   this->line = (Line*) Panel_getSelected(this->panel);
   int lines = yTo - yFrom + 1;
   StringBuffer* str = Line_deleteBlock(this->line, lines, xFrom, xTo);
   int len = StringBuffer_len(str);
   assert (len > 0);
   char* block = StringBuffer_deleteGet(str);
   Undo_deleteBlock(this->undo, this->x, this->y, block, len);
   this->savedX = Line_widthUntil(this->line, this->x, this->tabSize);
   this->selecting = false;
   this->modified = true;
   if (lines > 1)
      this->panel->needsRedraw = true;

   onChange(this);
}

/*
 * @param len Output parameters: returns the length of the copied block.
 */
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
   this->panel->needsRedraw = true;
   *len = StringBuffer_len(str);
   return StringBuffer_deleteGet(str);
}

void Buffer_pasteBlock(Buffer* this, Text block) {
   Undo_beginGroup(this->undo, this->x, this->y);
   if (this->selecting)
      Buffer_deleteBlock(this);
   if (block.chars == 0) {
      Undo_endGroup(this->undo, this->x, this->y);
      return;
   }
   int newX;
   int newY = this->y;
   Undo_insertBlock(this->undo, this->x, this->y, block);
   bool multiline = Line_insertBlock(this->line, this->x, block, &newX, &newY);

   this->x = newX;
   if (multiline) {
      this->panel->needsRedraw = true;
      this->y = newY;
      Panel_setSelected(this->panel, this->y);
   }
   this->line = (Line*) Panel_getSelected(this->panel);
   this->savedX = Line_widthUntil(this->line, this->x, this->tabSize);
   
   Undo_endGroup(this->undo, this->x, this->y);
   this->selecting = false;
   this->modified = true;

   onChange(this);
}

bool Buffer_setLine(Buffer* this, int y, const Text text) {
   if (y < 0)
      y += Panel_size(this->panel);
   if (y < 0 || y > Panel_size(this->panel)-1)
      return false;
   Undo_beginGroup(this->undo, this->x, this->y);
   Line* line = (Line*) Panel_get(this->panel, y);
   char* oldContent = calloc(line->text.bytes+1, 1);
   memcpy(oldContent, line->text.data, line->text.bytes);
   Undo_deleteBlock(this->undo, 0, y, oldContent, line->text.bytes);
   Line_deleteChars(line, 0, Line_chars(line));
   Undo_insertBlock(this->undo, 0, y, text);
   Line_insertTextAt(line, text, 0);
   Undo_endGroup(this->undo, this->x, this->y);
   this->modified = true;
   onChange(this);
   return true;
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
      if (this->dosLineBreaks) {
         Undo_beginGroup(this->undo, this->x, this->y);
         Undo_deleteCharAt(this->undo, this->x, this->y, Line_charAt(this->line, this->x));
         Line_deleteChars(this->line, this->x, 1);
      }
      if (this->line->super.next) {
         Undo_joinNext(this->undo, this->x, this->y, false);
         Line_joinNext(this->line);
         this->line = (Line*) Panel_getSelected(this->panel);
      }
      if (this->dosLineBreaks) {
         Undo_endGroup(this->undo, this->x, this->y);
      }
      this->panel->needsRedraw = true;
   }
   this->savedX = Line_widthUntil(this->line, this->x, this->tabSize);
   this->modified = true;
   onChange(this);
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
         this->x = Line_chars(prev);
         Undo_joinNext(this->undo, this->x, this->y - 1, true);
         Line_joinNext(prev);
         Panel_onKey(this->panel, KEY_UP);
         this->line = (Line*) Panel_getSelected(this->panel);
         this->y--;
         if (this->dosLineBreaks) {
            Buffer_backwardDeleteChar(this);
            Undo_endGroup(this->undo, this->x, this->y);
         }
         this->panel->needsRedraw = true;
      }
   }
   this->savedX = Line_widthUntil(this->line, this->x, this->tabSize);
   this->modified = true;
   onChange(this);
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

void Buffer_slideLines(Buffer* this, int n) {
   Panel_slide(this->panel, n);
   Buffer_correctPosition(this);
   this->selecting = false;
}

void Buffer_slideUpLine(Buffer* this) {
   Buffer_slideLines(this, -1);
}

void Buffer_slideDownLine(Buffer* this) {
   Buffer_slideLines(this, 1);
}

void Buffer_correctPosition(Buffer* this) {
   this->line = (Line*) Panel_getSelected(this->panel);
   this->x = MIN(this->x, last_x(this));
   int screenX = Line_widthUntil(this->line, this->x, this->tabSize);
   if (screenX > this->savedX) {
      while (this->x > 0 && Line_widthUntil(this->line, this->x, this->tabSize) > this->savedX)
         this->x--;
   } else if (screenX < this->savedX) {
      while (this->x < last_x(this) && Line_widthUntil(this->line, this->x, this->tabSize) < this->savedX)
         this->x++;
   }
   this->y = Panel_getSelectedIndex(this->panel);
}

void Buffer_validateCoordinate(Buffer* this, int *x, int* y) {
   int tx = *x;
   int ty = *y;
   ty = MIN(MAX(ty, 0), Panel_size(this->panel)-1);
   Line* line = (Line*) Panel_get(this->panel, ty);
   tx = MIN(MAX(tx, 0), last_x_line(this, line) + 1);
   *x = tx;
   *y = ty;
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
   if (this->tabulation == 0) {
      spaces = 0;
      move = 1;
   } else {
      spaces = this->tabulation;
      move = spaces;
   }
   Buffer_blockOperation(this, &firstLine, &yStart, &lines);
   int* unindented = Line_unindent(firstLine, lines, spaces);
   Undo_unindent(this->undo, this->x, yStart, unindented, lines, this->tabulation);
   if (unindented[0] > 0 && this->y >= yStart && this->y <= (yStart + lines - 1))
      this->x = MAX(0, this->x - move);
   this->panel->needsRedraw = true;
   this->modified = true;
   onChange(this);
}

void Buffer_indent(Buffer* this) {
   Line* firstLine;
   int yStart, lines, spaces, move;
   if (this->tabulation == 0) {
      if (!this->selecting) {
         Buffer_defaultKeyHandler(this, '\t', false);
         return;
      }
      spaces = 0;
      move = 1;
   } else {
      spaces = this->tabulation;
      move = spaces;
   }
   Buffer_blockOperation(this, &firstLine, &yStart, &lines);
   Undo_indent(this->undo, this->x, yStart, lines, spaces);
   Line_indent(firstLine, lines, spaces);
   if (this->y >= yStart && this->y <= (yStart + lines - 1))
      this->x += move;
   this->panel->needsRedraw = true;
   this->modified = true;
   onChange(this);
}
      
void Buffer_defaultKeyHandler(Buffer* this, int ch, bool code) {
   if ((!code && ch >= 32) || ch == '\t' || ch == '\015') {
      if (this->selecting) {
         Buffer_deleteBlock(this);
      }
      Undo_insertCharAt(this->undo, this->x, this->y, ch);
      Line_insertChar(this->line, this->x, ch);
      this->x++;
      this->savedX = Line_widthUntil(this->line, this->x, this->tabSize);
      this->modified = true;
      this->selecting = false;
      onChange(this);
   } else if (ch >= 1 && ch <= 31) {
      Script_onCtrl(this, ch);
      Buffer_correctPosition(this);
   } else if (ch >= KEY_F(1) && ch <= KEY_F(20)) {
      Script_onFKey(this, ch);
      Buffer_correctPosition(this);
   } else if (ch >= KEY_ALT('A') && ch <= KEY_ALT('Z')) {
      Script_onAlt(this, ch - KEY_ALT('A') + 1);
      Buffer_correctPosition(this);
   } else {
      Buffer_correctPosition(this);
      this->selecting = false;
   }
}

char Buffer_currentChar(Buffer* this) {
   assert(this->line);
   int len = last_x(this);
   int offset = MIN(this->x, len);
   if (this->x < len) {
      return Line_charAt(this->line, offset);
   } else {
      return '\0';
   }
}

Text Buffer_currentWord(Buffer* this) {
   assert(this->line);
   int len = last_x(this);
   int offset = MIN(this->x, len);
   return Text_wordAt(this->line->text, offset);
}

Coords Buffer_find(Buffer* this, Text needle, bool findNext, bool caseSensitive, bool wholeWord, bool forward) {
   assert(this->line);
   Coords notFound = { .x = -1, .y = -1 };
   if (!Text_chars(needle))
      return notFound;
   if (this->selecting && (!findNext || !forward) )
      this->x = this->selectXfrom;
   Line* line = this->line;
   Line* start = line;
   int y = this->y;
   chars x = this->x;
   Text haystack = line->text;
   bool last = false;
   for (;;) {
      int found = -1;
      if (forward) {
         const Text haystackX = Text_textAt(haystack, x);
         if (Text_chars(haystack) - x >= Text_chars(needle)) {
            if (caseSensitive)
               found = Text_indexOf(haystackX, needle);
            else
               found = Text_indexOfi(haystackX, needle);
            if (found != -1)
               found += x;
         }
      } else {
         if (x >= Text_chars(needle)) {
            x -= Text_chars(needle);
            while (x >= 0) {
               const Text haystackX = Text_textAt(haystack, x);
               int cmp;
               if (caseSensitive)
                  cmp = Text_strncmp(haystackX, needle);
               else
                  cmp = Text_strncasecmp(haystackX, needle);
               if (cmp == 0) {
                  found = x;
                  break;
               }
               x--;
            }
         }
      }

      if (wholeWord && found != -1) {
         int pastFound = found + Text_chars(needle);
         bool moreToTheLeft  = found > 0;
         bool moreToTheRight = pastFound < Text_chars(haystack);
         bool charToTheLeft  = moreToTheLeft && iswword(Text_at(haystack, found - 1));
         bool charToTheRight = moreToTheRight && iswword(Text_at(haystack, pastFound));
         if (charToTheLeft || charToTheRight) {
            if (forward) {
               if (moreToTheRight) {
                  x = pastFound;
                  continue;
               }
            } else {
               if (moreToTheLeft) {
                  x--;
                  continue;
               }
            }
            found = -1;
         }
      }

      if (found != -1) {
         return (Coords){ .x = found, .y = y };
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
      haystack = line->text;
      if (forward)
         x = 0;
      else
         x = Text_chars(haystack);
      if (last) {
         break;
      } else {
         if (line == start)
            last = true;
      }
   }
   return notFound;
}

static int writeLineInFormat(FILE* fd, Line* l, bool utf8, bool trimTrailingWhitespace, iconv_t cd) {
   char* intext = Line_toString(l);
   size_t insize = Line_bytes(l);
   if (trimTrailingWhitespace) {
      while(insize > 0 && intext[insize - 1] == ' ') {
         insize--;
      }
   }
   if (utf8) {
      fwrite(intext, insize, 1, fd);
      return insize;
   }
   size_t outsize = insize + 1;
   char* outtext = calloc(outsize, 1);
   size_t outleft = outsize;
   char* outptr = outtext;
   int err = iconv(cd, &intext, &insize, &outptr, &outleft);
   if (err == -1) {
      fwrite(intext, insize, 1, fd);
      return insize;
   }
   int size = outsize - outleft;
   fwrite(outtext, size, 1, fd);
   free(outtext);
   return size;
}

static void Buffer_coreSaveAndCloseFd(Buffer* this, FILE* fd) {
   Line* l = (Line*) this->panel->items->head;
   iconv_t cd;
   if (!this->isUTF8) {
      cd = iconv_open("ISO-8859-15", "UTF-8");
   }
   int lastLineLen;
   while (l) {
      lastLineLen = writeLineInFormat(fd, l, this->isUTF8, this->trimTrailingWhitespace, cd);
      l = (Line*) l->super.next;
      if (l)
         fwrite("\n", 1, 1, fd);
   }
   if (this->insertFinalNewline && lastLineLen != 0) {
      if (this->dosLineBreaks) {
         fwrite("\r\n", 2, 1, fd);
      } else {
         fwrite("\n", 1, 1, fd);
      }
   }
   fclose(fd);
   if (!this->isUTF8) {
      iconv_close(cd);
   }
}

void Buffer_saveAndCloseFd(Buffer* this, FILE* fd) {
   assert(this->fileName);

   Buffer_coreSaveAndCloseFd(this, fd);

   Undo_store(this->undo, this->fileName);
   Script_onSave(this, this->fileName);
   this->modified = false;
   this->readOnly = false;
}

bool Buffer_save(Buffer* this) {
   assert(this->fileName);
   FILE* fd = fopen(this->fileName, "w");
   if (!fd)
      return false;
   Buffer_saveAndCloseFd(this, fd);
   return true;
}

void Buffer_autosave(Buffer* this, bool timeout) {
   if (timeout && this->autosaveCounter == 0) {
      return;
   } else if (this->autosaveCounter < 200) {
      return;
   }

   char* autosave;
   FILE* fd;
   if (this->autosave) {
      autosave = this->autosave;
      fd = fopen(autosave, "w");
   } else if (this->fileName) {
      int len = strlen(this->fileName);
      autosave = malloc(len + 2);
      strncpy(autosave, this->fileName, len);
      autosave[len] = '~';
      autosave[len + 1] = '\0';
      fd = fopen(autosave, "w");
   } else {
      const char* template = "/tmp/dit.autosave.XXXXXX";
      autosave = strdup(template);
      int fdesc = mkstemp(autosave);
      if (fdesc == -1) {
         free(autosave);
         return;
      }
      fd = fdopen(fdesc, "w");
   }

   if (!fd) {
      free(autosave);
      this->autosave = NULL;
      return;
   }
   this->autosave = autosave;
   this->autosaveCounter = 0;
   Buffer_coreSaveAndCloseFd(this, fd);
   return;
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
   if (this->saveTabulationX == this->x && this->saveTabulationY == this->y) {
      switch (this->tabulation) {
      case 0: this->tabulation = 2; break;
      case 2: this->tabulation = 3; break;
      case 3: this->tabulation = 4; break;
      case 4: this->tabulation = 8; break;
      case 8: this->tabulation = 0; break;
      }
   } else {
      if (this->tabulation == 0) {
         this->tabulation = this->saveTabulation;
      } else {
         this->saveTabulation = this->tabulation;
         this->tabulation = 0;
      }
   }
   this->saveTabulationX = this->x;
   this->saveTabulationY = this->y;
}

void Buffer_toggleDosLineBreaks(Buffer* this) {
   this->dosLineBreaks = !this->dosLineBreaks;
}
