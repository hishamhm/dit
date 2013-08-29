
#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <ctype.h>

#include "Prototypes.h"
//#needs List
//#needs Text

/*{

struct LineClass_ {
   ListItemClass super;
};

struct Line_ {
   ListItem super;
   Text text;
   HighlightContext* context;
};

extern LineClass LineType;

#define Line_chars(this) (Text_chars((this)->text))
#define Line_toString(this) (Text_toString((this)->text))
#define Line_bytes(this) (Text_bytes((this)->text))

}*/

LineClass LineType = {
   .super = {
      .super = {
         .size = sizeof(Line),
         .display = Line_display,
         .equals = Line_equals,
         .delete = Line_delete
      }
   }
};

Line* Line_new(List* list, Text text, HighlightContext* context) {
   Line* this = Pool_allocate(list->pool);
   Bless(Line);
   Call0(ListItem, init, this);
   this->text = text;
   this->context = context;
   //FIXME// assert(this->data[this->bytes] == '\0');
   return this;
}

void Line_delete(Object* cast) {
   Line* this = (Line*) cast;
   Text_prune(&(this->text));
}

int Line_charAt(Line* this, int n) {
   return Text_at(this->text, n);
}

void Line_updateContext(Line* this) {
   // temporarily disable selection
   Buffer* buffer = (Buffer*)(this->super.list->data);
   bool selecting = buffer->selecting;
   buffer->selecting = false;
   Line_display((Object*)this, NULL);
   buffer->selecting = selecting;
}

void Line_display(Object* cast, RichString* str) {
   Line* this = (Line*) cast;
   Buffer* buffer = (Buffer*)(this->super.list->data);
   int scrollH = buffer->panel->scrollH;
   int y = buffer->panel->displaying;
   Highlight* hl = buffer->hl;
   int tabWidth = buffer->tabWidth;

   int inIdx = 0;
   int hlAttrs[this->text.bytes];
   int sizeAttrs = this->text.chars * tabWidth + 1;
   int* attrs = malloc(sizeAttrs * sizeof(int));
   memset(attrs, 0, sizeAttrs * sizeof(int));
   
   int outIdx = 0;
   char out[this->text.bytes * tabWidth + 1];

   HighlightContext* context = this->super.prev
                             ? ((Line*)this->super.prev)->context
                             : hl->mainContext;
   Highlight_setContext(hl, context);

   // FIXME UTF-8 highlighting
   Highlight_setAttrs(hl, this->text.data, hlAttrs, this->text.bytes, y + 1);

   const unsigned char* start = Text_toString(this->text);
   int attrIdx = 0;

   for (const unsigned char* curr = start; *curr; ) {
      int inIdx = curr - start;
      if (*curr == '\t') {
         int tabSize = tabWidth - (outIdx % tabWidth);
         for (int i = 0; i < tabSize; i++) {
            attrs[attrIdx++] = hlAttrs[inIdx];
            out[outIdx++] = ' ';
         }
         curr++;
      } else if (*curr < 32) {
         attrs[attrIdx++] = CRT_colors[AlertColor];
         out[outIdx++] = *curr + 'A' - 1;
         curr++;
      } else {
         attrs[attrIdx++] = hlAttrs[inIdx];
         int offset = UTF8_copyChar(out + outIdx, curr);
         outIdx += offset;
         curr += offset;
      }
   }
   out[outIdx] = '\0';

   if (buffer->bracketY == y && buffer->bracketX < attrIdx) {
      attrs[buffer->bracketX] = CRT_colors[BracketColor];
   }
   
   if (buffer->selecting) {
      int yFrom = buffer->selectYfrom;
      int yTo = buffer->selectYto;
      if ((y >= yFrom && y <= yTo) || (y >= yTo && y <= yFrom)) {
         int from, to;
         int xFrom = buffer->selectXfrom;
         int xTo = buffer->selectXto;
         if (yFrom > yTo || (yFrom == yTo && xFrom > xTo)) {
            int tmp = yFrom; yFrom = yTo; yTo = tmp;
            tmp = xFrom; xFrom = xTo; xTo = tmp;
         }
         if (y == yFrom && y == yTo) {
            from = Text_cellsUntil(this->text, xFrom, tabWidth);
            to = Text_cellsUntil(this->text, xTo, tabWidth);
         } else if (y == yFrom && y < yTo) {
            from = Text_cellsUntil(this->text, xFrom, tabWidth);
            out[outIdx++] = ' ';
            out[outIdx] = '\0';
            to = outIdx;
         } else if (y > yFrom && y == yTo) {
            from = 0;
            to = Text_cellsUntil(this->text, xTo, tabWidth);
         } else { // if (y > yFrom && y < yTo) {
            from = 0;
            out[outIdx++] = ' ';
            out[outIdx] = '\0';
            to = outIdx;
         }
         for (int i = from; i < to; i++) {
            attrs[i] = CRT_colors[SelectionColor];
         }
      }
   }
   
   Text outText = Text_new(out);
   if (str && Text_chars(outText) >= scrollH) {
      RichString_appendn(str, 0, Text_stringAt(outText, scrollH), Text_chars(outText) - scrollH);
      RichString_paintAttrs(str, attrs + scrollH);
   }
   this->context = Highlight_getContext(hl);
   free(attrs);
}

int Line_widthUntil(Line* this, int n, int tabWidth) {
   return Text_cellsUntil(this->text, n, tabWidth);
}

bool Line_equals(const Object* o1, const Object* o2) {
   Line* l1 = (Line*) o1;
   Line* l2 = (Line*) o2;
   if (Text_bytes(l1->text) != Text_bytes(l2->text))
      return false;
   return (strcmp(Text_toString(l1->text), Text_toString(l2->text)) == 0);
}

void Line_insertChar(Line* this, int at, wchar_t ch) {
   Text_insertChar(&(this->text), at, ch);
}

void Line_deleteChars(Line* this, int at, int n) {
   Text_deleteChars(&(this->text), at, n);
}

/*
inline int Line_getIndentWidth(Line* this, int tabWidth) {
   int indentWidth = 0;
   // UTF-8: indent chars are always ASCII
   for (int i = 0; i < Text_chars(this->text) && isblank(this->text.data[i]); i++) {
      if (this->text.data[i] == '\t')
         indentWidth += tabWidth - (indentWidth % tabWidth);
      else
         indentWidth++;
   }
   return indentWidth;
}
*/

int Line_breakAt(Line* this, int at, bool doIndent) {
   assert(at >= 0 && at <= Text_chars(this->text));

   int indentBytes = 0;
   if (doIndent) {
      // UTF-8: indent chars are always ASCII
      for (; indentBytes < Text_chars(this->text) && isblank(this->text.data[indentBytes]) && indentBytes < at; indentBytes++);
   }
   
   Text new = Text_breakIndenting(&(this->text), at, indentBytes);
   Line* newLine = Line_new(this->super.list, new, this->context);
   ListItem_addAfter((ListItem*) this, (ListItem*) newLine);
   return indentBytes;
}

void Line_joinNext(Line* this) {
   assert(this->super.next);
   Line* next = (Line*) this->super.next;
   Text_strcat(&(this->text), next->text);
   ListItem_remove((ListItem*) next);
}

static int lineToBufferFromTo(StringBuffer* str, Line* l, int xFrom, int xTo) {
   const char* from = Text_stringAt(l->text, xFrom);
   const char* to = Text_stringAt(l->text, xTo);
   StringBuffer_addN(str, from, to - from);
}

static int lineToBufferFrom(StringBuffer* str, Line* l, int xFrom) {
   const unsigned char* from = Text_stringAt(l->text, xFrom);
   StringBuffer_addN(str, from, Text_toString(l->text) + Text_bytes(l->text) - from);
}

static int lineToBufferTo(StringBuffer* str, Line* l, int xTo) {
   StringBuffer_addN(str, Text_toString(l->text), Text_bytesUntil(l->text, xTo));
}

static int lineToBuffer(StringBuffer* str, Line* l) {
   StringBuffer_addN(str, Text_toString(l->text), Text_bytes(l->text));
}

static StringBuffer* getBlock(Line* this, int lines, int xFrom, int xTo, bool delete) {
   assert(Text_chars(this->text) >= xFrom);
   StringBuffer* str = StringBuffer_new(NULL);
   Line* l = this;
   Line* first = this;
   if (lines == 1) {
      lineToBufferFromTo(str, l, xFrom, xTo);
      if (delete) Text_deleteChars(&(l->text), xFrom, xTo - xFrom);
   } else {
      if (xFrom > 0) {
         lineToBufferFrom(str, l, xFrom);
         StringBuffer_addChar(str, '\n');
         if (delete) Text_deleteChars(&(l->text), xFrom, Text_chars(l->text) - xFrom);
      } else {
         lineToBuffer(str, l);
         StringBuffer_addChar(str, '\n');
         if (delete) Text_prune(&(l->text));
      }
      l = (Line*) l->super.next;
      for (int i = 2; i < lines; i++) {
         Line* next = (Line*) l->super.next;
         lineToBuffer(str, l);
         StringBuffer_addChar(str, '\n');
         if (delete) ListItem_remove((ListItem*) l);
         l = next;
      }
      if (xTo < Text_chars(l->text)) {
         lineToBufferTo(str, l, xTo);
         if (delete) Text_deleteChars(&(l->text), 0, xTo);
      } else {
         lineToBuffer(str, l);
         if (delete) Text_prune(&(l->text));
      }
      if (delete) Line_joinNext(first);
   }
   //FIXME// assert(this->text[this->len] == '\0');
   return str;
}

StringBuffer* Line_deleteBlock(Line* this, int lines, int xFrom, int xTo) {
   return getBlock(this, lines, xFrom, xTo, true);
}

StringBuffer* Line_copyBlock(Line* this, int lines, int xFrom, int xTo) {
   return getBlock(this, lines, xFrom, xTo, false);
}

void Line_insertTextAt(Line* this, Text text, int at) {
   Text_insert(&(this->text), at, text);
}

void Line_indent(Line* this, int lines, int indentSpaces) {
   char spacer = ' ';
   int width = indentSpaces;
   if (width == 0) {
      width = 1;
      spacer = '\t';
   }
   char indent[width + 1];
   for (int i = 0; i < width; i++)
      indent[i] = spacer;
   indent[width] = '\0';
   Line_insertTextAt(this, Text_new(indent), 0);
   if (lines > 1) {
      assert(this->super.next);
      Line* next = (Line*) this->super.next;
      Line_indent(next, lines - 1, indentSpaces);
   }
   //FIXME// assert(this->text[this->len] == '\0');
}

int* Line_unindent(Line* this, int lines, int indentSpaces) {
   char spacer = ' ';
   int width = indentSpaces;
   if (width == 0) {
      width = 1;
      spacer = '\t';
   }
   int* result = (int*) malloc(sizeof(int*) * lines);
   Line* l = this;
   for (int c = 0; c < lines; c++) {
      assert(l);
      // UTF-8: indent chars are always ASCII
      int n = MIN(width, Text_chars(l->text));
      for (int i = 0; i < n; i++)
         if (l->text.data[i] != spacer) {
            n = i;
            break;
         }
      result[c] = n;
      if (n)
         Line_deleteChars(l, 0, n);
      l = (Line*) l->super.next;
   }
   //FIXME// assert(this->text[this->len] == '\0');
   return result;
}

bool Line_insertBlock(Line* this, int x, Text block, int* newX, int* newY) {
   // newY must contain the current value of y on input
   int blockBytes = Text_bytes(block);
   unsigned char* nl = memchr(block.data, '\n', block.bytes);
   Line* at = this;
   bool multiline = (nl);
   if (!multiline) {
      Line_insertTextAt(this, block, x);
      *newX = x + Text_chars(block);
   } else {
      int lineLen = nl - block.data;
      Line_breakAt(this, x, 0);
      Line* last = (Line*) this->super.next;
      Text_insertString(&(this->text), x, block.data, lineLen);
      unsigned char* walk = ++nl;
      (*newY)++;
      while ( (nl = memchr(walk, '\n', blockBytes - (walk - block.data) )) ) {
         lineLen = nl - walk;
         char* text = malloc(lineLen+1);
         text[lineLen] = '\0';
         memcpy(text, walk, lineLen);
         Line* newLine = Line_new(this->super.list, Text_new(text), this->context);
         ListItem_addAfter((ListItem*) at, (ListItem*) newLine);
         at = newLine;
         walk = ++nl;
         (*newY)++;
      }
      if (walk - block.data < blockBytes) {
         int lastLineLen = blockBytes - (walk - block.data);
         Text_insertString(&(last->text), 0, walk, lastLineLen);
         *newX = lastLineLen;
      } else {
         *newX = 0;
      }
   }
   return multiline;
}
