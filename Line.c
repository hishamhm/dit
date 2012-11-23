
#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <ctype.h>

#include "Prototypes.h"
//#needs List

/*{

struct LineClass_ {
   ListItemClass super;
};

struct Line_ {
   ListItem super;
   char* text;
   int textSize;
   int len;
   HighlightContext* context;
};

extern LineClass LineType;

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

Line* Line_new(List* list, char* text, int len, HighlightContext* context) {
   Line* this = Pool_allocate(list->pool);
   Bless(Line);
   Call0(ListItem, init, this);
   assert(len == strlen(text));
   this->text = text;
   this->textSize = len + 1;
   this->len = len;
   this->context = context;
   assert(this->text[this->len] == '\0');
   return this;
}

void Line_delete(Object* cast) {
   Line* this = (Line*) cast;
   free(this->text);
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
   int len = this->len;
   int outIndex = 0;
   int textIndex = 0;
   Highlight* hl = buffer->hl;
   int tabWidth = buffer->tabWidth;
   int outSize = (len+1) * tabWidth;
   unsigned char out[outSize];
   int inAttrs[len];
   int attrs[outSize];

   HighlightContext* context = this->super.prev
                             ? ((Line*)this->super.prev)->context
                             : hl->mainContext;
   Highlight_setContext(hl, context);

   Highlight_setAttrs(hl, (unsigned char*) this->text, inAttrs, len, y + 1);
  
   while (textIndex < this->len) {
      unsigned char curr = this->text[textIndex];
      attrs[outIndex] = inAttrs[textIndex];
      if (curr == '\t') {
         int tabSize = tabWidth - (outIndex % tabWidth);
         for (int i = 0; i < tabSize; i++) {
            attrs[outIndex] = inAttrs[textIndex];
            out[outIndex++] = ' ';
         }
         /* show tabs
         out[outIndex - tabSize] = '.';
         attrs[outIndex - tabSize] |= CRT_colors[DimColor];
         */
      } else if (curr < 32) {
         attrs[outIndex] = CRT_colors[AlertColor];
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
            from = Line_widthUntil(this, xFrom, tabWidth);
            to = Line_widthUntil(this, xTo, tabWidth);
         } else if (y == yFrom && y < yTo) {
            from = Line_widthUntil(this, xFrom, tabWidth);
            out[outIndex++] = ' ';
            out[outIndex] = '\0';
            to = outIndex;
         } else if (y > yFrom && y == yTo) {
            from = 0;
            to = Line_widthUntil(this, xTo, tabWidth);
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
      RichString_appendn(str, 0, out + scrollH, outIndex - scrollH);
      RichString_paintAttrs(str, attrs + scrollH);
   }
   this->context = Highlight_getContext(hl);
}

inline char Line_charAt(Line* this, int at) {
   assert(this);
   assert(at >= 0 && at < this->len);
   return this->text[at];
}

int Line_widthUntil(Line* this, int n, int tabWidth) {
   int width = 0;
   n = MIN(n, this->len);
   for (int i = 0; i < n; i++) {
      char curr = this->text[i];
      if (curr == '\t')
         width += tabWidth - (width % tabWidth);
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

inline int Line_getIndentWidth(Line* this, int tabWidth) {
   int indentWidth = 0;
   for (int i = 0; i < this->len && isblank(this->text[i]); i++) {
      if (this->text[i] == '\t')
         indentWidth += tabWidth - (indentWidth % tabWidth);
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
   Line* newLine = Line_new(this->super.list, rest, restLen + indent, this->context);
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

void Line_insertStringAt(Line* this, int at, const char* text, int len) {
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
   Line_insertStringAt(this, 0, indent, width);
   if (lines > 1) {
      assert(this->super.next);
      Line* next = (Line*) this->super.next;
      Line_indent(next, lines - 1, indentSpaces);
   }
   assert(this->text[this->len] == '\0');
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
      int n = MIN(width, l->len);
      for (int i = 0; i < n; i++)
         if (l->text[i] != spacer) {
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

bool Line_insertBlock(Line* this, int x, char* block, int len, int* newX, int* newY) {
   /* newY must contain the current value of y on input */
   char* nl = memchr(block, '\n', len);
   Line* at = this;
   bool multiline = (nl);
   if (!multiline) {
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
         Line* newLine = Line_new(this->super.list, text, lineLen, this->context);
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
   return multiline;
}
