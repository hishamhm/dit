
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <ctype.h>

#include "Prototypes.h"
//#needs List

/*{

#define TAB_WIDTH 8
#define INDENT_WIDTH 3

struct Line_ {
   ListItem super;
   char* text;
   int textSize;
   int len;
   HighlightContext* context;
   Buffer* buffer;
};

extern char* LINE_CLASS;

}*/

/* private property */
char* LINE_CLASS = "Line";

Line* Line_new(char* text, int len, Buffer* buffer) {
   Line* this = (Line*) malloc(sizeof(Line));
   ((Object*)this)->class = LINE_CLASS;
   ((Object*)this)->display = Line_display;
   ((Object*)this)->equals = Line_equals;
   ((Object*)this)->delete = Line_delete;
   ListItem_init((ListItem*)this);
   assert(len == strlen(text));
   this->text = text;
   this->textSize = len + 1;
   this->len = len;
   this->context = buffer->hl->mainContext;
   this->buffer = buffer;
   assert(this->text[this->len] == '\0');
   return this;
}

void Line_delete(Object* cast) {
   Line* this = (Line*) cast;
   free(this->text);
   free(this);
}

void Line_updateContext(Line* this) {
   // temporarily disable selection
   bool selecting = this->buffer->selecting;
   this->buffer->selecting = false;
   Line_display((Object*)this, NULL);
   this->buffer->selecting = selecting;
}

void Line_display(Object* cast, RichString* str) {
   Line* this = (Line*) cast;
   Buffer* buffer = this->buffer;
   int scrollH = buffer->panel->scrollH;
   int y = buffer->panel->displaying;
   int len = this->len;
   int outIndex = 0;
   int textIndex = 0;
   Highlight* hl = buffer->hl;
   int outSize = (len+1) * TAB_WIDTH;
   unsigned char out[outSize];
   int inAttrs[len];
   int attrs[outSize];

   HighlightContext* context = this->super.prev
                             ? ((Line*)this->super.prev)->context
                             : hl->mainContext;
   Highlight_setContext(hl, context);

   Highlight_setAttrs(hl, (unsigned char*) this->text, inAttrs, len);
  
   while (textIndex < this->len) {
      unsigned char curr = this->text[textIndex];
      attrs[outIndex] = inAttrs[textIndex];
      if (curr == '\t') {
         int tabSize = TAB_WIDTH - (outIndex % TAB_WIDTH);
         for (int i = 0; i < tabSize; i++) {
            attrs[outIndex] = inAttrs[textIndex];
            out[outIndex++] = ' ';
         }
      } else if (curr < 32) {
         out[outIndex++] = curr + 'A' - 1;
      } else {
         out[outIndex++] = curr;
      }
      textIndex++;
   }
   out[outIndex] = '\0';

   if (buffer->bracketY == y && buffer->bracketX < outIndex) {
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
            from = Line_widthUntil(this, xFrom);
            to = Line_widthUntil(this, xTo);
         } else if (y == yFrom && y < yTo) {
            from = Line_widthUntil(this, xFrom);
            out[outIndex++] = ' ';
            out[outIndex] = '\0';
            to = outIndex;
         } else if (y > yFrom && y == yTo) {
            from = 0;
            to = Line_widthUntil(this, xTo);
         } else { // if (y > yFrom && y < yTo) {
            from = 0;
            out[outIndex++] = ' ';
            out[outIndex] = '\0';
            to = outIndex;
         }
         for (int i = from; i < to; i++) {
            attrs[i] = CRT_colors[SelectionColor];
         }
      }
   }
   
   if (str && outIndex >= scrollH) {
      RichString_append(str, 0, out + scrollH, outIndex - scrollH);
      RichString_setAttrs(str, attrs + scrollH);
   }
   this->context = Highlight_getContext(hl);
}

inline char Line_charAt(Line* this, int at) {
   assert(this);
   assert(at >= 0 && at < this->len);
   return this->text[at];
}

int Line_widthUntil(Line* this, int n) {
   int width = 0;
   n = MIN(n, this->len);
   for (int i = 0; i < n; i++) {
      char curr = this->text[i];
      if (curr == '\t')
         width += TAB_WIDTH - (width % TAB_WIDTH);
      else
         width++;
   }
   return width;
}

bool Line_equals(const Object* o1, const Object* o2) {
   Line* l1 = (Line*) o1;
   Line* l2 = (Line*) o2;
   if (l1->len != l2->len)
      return false;
   return (memcmp(l1->text, l2->text, l1->len) == 0);
}

void Line_insertCharAt(Line* this, char ch, int at) {
   assert(at >= 0 && at <= this->len);
   assert(this->text);
   assert(this->len < this->textSize);
   if (this->len+1 == this->textSize) {
      this->textSize += MIN(this->textSize + 1, 256);
      this->text = realloc(this->text, sizeof(char) * this->textSize);
   }
   assert(this->textSize > this->len + 1);
   for (int i = this->len; i >= at; i--)
      this->text[i+1] = this->text[i];
   this->text[at] = ch;
   this->len++;
   assert(this->text[this->len] == '\0');
}

void Line_deleteChars(Line* this, int at, int n) {
   assert(at >= 0 && at < this->len);
   assert(at + n <= this->len);
   for (int i = at; i < this->len - n; i++)
      this->text[i] = this->text[i+n];
   this->len -= n;
   this->text[this->len] = '\0';
}

inline int Line_getIndentChars(Line* this) {
   int count = 0;
   for (count = 0; count < this->len && isblank(this->text[count]); count++);
   return count;
}

inline int Line_getIndentWidth(Line* this) {
   int indentWidth = 0;
   for (int i = 0; i < this->len && isblank(this->text[i]); i++) {
      if (this->text[i] == '\t')
         indentWidth += TAB_WIDTH - (indentWidth % TAB_WIDTH);
      else
         indentWidth++;
   }
   return indentWidth;
}

void Line_breakAt(Line* this, int at, int indent) {
   assert(at >= 0 && at <= this->len);

   int restLen = this->len - at;
   char* rest = malloc(sizeof(char) * (restLen + indent + 1));
   for (int i = 0; i < indent; i++) {
      rest[i] = this->text[i];
   }
   memcpy(rest + indent, this->text + at, restLen);
   rest[restLen + indent] = '\0';
   this->text[at] = '\0';
   this->len = at;
   Line* newLine = Line_new(rest, restLen + indent, this->buffer);
   ListItem_addAfter((ListItem*) this, (ListItem*) newLine);
   assert(this->text[this->len] == '\0');
}

void Line_joinNext(Line* this) {
   assert(this->super.next);
   Line* next = (Line*) this->super.next;
   int newSize = this->len + next->len + 1;
   if (this->textSize < newSize) {
      this->text = realloc(this->text, newSize);
      this->textSize = newSize;
      this->text[newSize-1] = '\0';
   }
   memcpy(this->text + this->len, next->text, next->len);
   this->len += next->len;
   this->text[this->len] = '\0';
   ListItem_remove((ListItem*) next);
}

StringBuffer* Line_deleteBlock(Line* this, int lines, int xFrom, int xTo) {
   assert(this->len >= xFrom);
   StringBuffer* str = StringBuffer_new(NULL);
   Line* l = this;
   Line* first = this;
   if (lines == 1) {
      StringBuffer_addN(str, l->text + xFrom, xTo - xFrom);
      for (int i = xTo; i <= l->len; i++)
         l->text[xFrom + (i - xTo)] = l->text[i];
      l->len -= xTo - xFrom;
   } else {
      if (xFrom > 0) {
         StringBuffer_addN(str, l->text + xFrom, l->len - xFrom);
         StringBuffer_addChar(str, '\n');
         l->len = xFrom;
      } else {
         StringBuffer_addN(str, l->text, l->len);
         StringBuffer_addChar(str, '\n');
         l->len = 0;
      }
      l = (Line*) l->super.next;
      for (int i = 2; i < lines; i++) {
         Line* next = (Line*) l->super.next;
         StringBuffer_addN(str, l->text, l->len);
         StringBuffer_addChar(str, '\n');
         ListItem_remove((ListItem*) l);
         l = next;
      }
      if (xTo < l->len) {
         StringBuffer_addN(str, l->text, xTo);
         for (int i = xTo; i < l->len; i++)
            l->text[i - xTo] = l->text[i];
         l->len -= xTo;
      } else {
         StringBuffer_addN(str, l->text, l->len);
         l->len = 0;
      }
      Line_joinNext(first);
   }
   assert(this->text[this->len] == '\0');
   return str;
}

StringBuffer* Line_copyBlock(Line* this, int lines, int xFrom, int xTo) {
   StringBuffer* str = StringBuffer_new(NULL);
   Line* l = this;
   if (lines == 1) {
      StringBuffer_addN(str, l->text + xFrom, xTo - xFrom);
   } else {
      if (xFrom > 0) {
         StringBuffer_addN(str, l->text + xFrom, l->len - xFrom);
         StringBuffer_addChar(str, '\n');
      } else {
         StringBuffer_addN(str, l->text, l->len);
         StringBuffer_addChar(str, '\n');
      }
      l = (Line*) l->super.next;
      for (int i = 2; i < lines; i++) {
         Line* next = (Line*) l->super.next;
         StringBuffer_addN(str, l->text, l->len);
         StringBuffer_addChar(str, '\n');
         l = next;
      }
      if (xTo < l->len)
         StringBuffer_addN(str, l->text, xTo);
      else
         StringBuffer_addN(str, l->text, l->len);
   }
   assert(this->text[this->len] == '\0');
   return str;
}

void Line_insertStringAt(Line* this, int at, char* text, int len) {
   assert(at >= 0 && at <= this->len);
   if (this->len + len + 1 > this->textSize) {
      int newSize = this->textSize + len + 1;
      assert(this->text);
      this->text = realloc(this->text, sizeof(char) * newSize + 1);
      this->textSize = newSize;
   }
   for (int i = this->len; i >= at; i--)
      this->text[i+len] = this->text[i];
   memcpy(this->text + at, text, len);
   this->len += len;
   assert(this->text[this->len] == '\0');
}

// TODO: optionally indent with tab
void Line_indent(Line* this, int lines) {
   char indent[INDENT_WIDTH + 1];
   for (int i = 0; i < INDENT_WIDTH; i++)
      indent[i] = ' ';
   indent[INDENT_WIDTH] = '\0';
   // TODO: expand tabs
   Line_insertStringAt(this, 0, indent, INDENT_WIDTH);
   if (lines > 1) {
      assert(this->super.next);
      Line* next = (Line*) this->super.next;
      Line_indent(next, lines - 1);
   }
   assert(this->text[this->len] == '\0');
}

// TODO: optionally indent with tab
int* Line_unindent(Line* this, int lines) {
   int* result = (int*) malloc(sizeof(int*) * lines);
   Line* l = this;
   for (int c = 0; c < lines; c++) {
      // TODO: expand tabs
      assert(l);
      int n = MIN(INDENT_WIDTH, l->len);
      for (int i = 0; i < n; i++)
         if (l->text[i] != ' ') {
            n = i;
            break;
         }
      result[c] = n;
      if (n)
         Line_deleteChars(l, 0, n);
      l = (Line*) l->super.next;
   }
   assert(this->text[this->len] == '\0');
   return result;
}

void Line_insertBlock(Line* this, int x, char* block, int len, int* newX, int* newY) {
   /* newY must contain the current value of y on input */
   char* nl = memchr(block, '\n', len);
   Line* at = this;
   if (!nl) {
      Line_insertStringAt(this, x, block, len);
      *newX = x + len;
   } else {
      int lineLen = nl - block;
      Line_breakAt(this, x, 0);
      Line* last = (Line*) this->super.next;
      Line_insertStringAt(this, x, block, lineLen);
      char* walk = ++nl;
      (*newY)++;
      while ( (nl = memchr(walk, '\n', len - (walk - block) )) ) {
         lineLen = nl - walk;
         char* text = malloc(lineLen+1);
         text[lineLen] = '\0';
         memcpy(text, walk, lineLen);
         Line* newLine = Line_new(text, lineLen, this->buffer);
         ListItem_addAfter((ListItem*) at, (ListItem*) newLine);
         at = newLine;
         walk = ++nl;
         (*newY)++;
      }
      if (walk - block < len) {
         int lastLineLen = len - (walk - block);
         Line_insertStringAt(last, 0, walk, lastLineLen);
         *newX = lastLineLen;
      } else {
         *newX = 0;
      }
   }
   assert(this->text[this->len] == '\0');
}
