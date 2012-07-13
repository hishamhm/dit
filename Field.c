
#define _GNU_SOURCE
#include <string.h>

#include "Prototypes.h"
//#needs Object
//#needs List

/*{

struct Field_ {
   char* label;
   int labelLen;
   int x;
   int y;
   int w;
   int labelColor;
   int fieldColor;
   List* history;
   FieldItem* current;
   int cursor;
};

struct FieldItemClass_ {
   ListItemClass super;
};

struct FieldItem_ {
   ListItem super;
   char* text;
   int len;
   int w;
};

extern FieldItemClass FieldItemType;

}*/

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

FieldItemClass FieldItemType = {
   .super = {
      .super = {
         .size = sizeof(FieldItem),
         .display = NULL,
         .equals = Object_equals,
         .delete = FieldItem_delete
      }
   }
};

Field* Field_new(char* label, int x, int y, int w) {
   Field* this = (Field*) malloc(sizeof(Field));
   this->x = x;
   this->y = y;
   this->w = w;
   this->labelColor = CRT_colors[StatusColor];
   this->fieldColor = CRT_colors[FieldColor];
   this->cursor = 0;
   this->history = List_new( ClassAs(FieldItem, ListItem), NULL );
   this->current = NULL;
   this->label = malloc(w + 1);
   this->labelLen = strlen(label);
   strncpy(this->label, label, this->w);
   assert(w - this->labelLen > 0);
   return this;
}

void Field_delete(Field* this) {
   List_delete(this->history);
   free(this->label);
   free(this);
}

FieldItem* FieldItem_new(List* list, int w) {
   FieldItem* this = Pool_allocate(list->pool);
   Bless(FieldItem);
   this->text = calloc(w, 1);
   this->len = 0;
   this->w = w;
   return this;
}

void Field_printfLabel(Field* this, char* picture, ...) {
   va_list ap;
   va_start(ap, picture);
   vsnprintf(this->label, this->w, picture, ap);
   va_end(ap);
   this->labelLen = strlen(this->label);
}

void FieldItem_delete(Object* cast) {
   FieldItem* this = (FieldItem*) cast;
   free(this->text);
}

void Field_start(Field* this) {
   this->current = (FieldItem*) List_getLast(this->history);
   if (!this->current || this->current->len > 0) {
      this->current = FieldItem_new(this->history, this->w);
      List_add(this->history, (ListItem*) this->current);
   }
   this->cursor = 0;
}

FieldItem* Field_previousInHistory(Field* this) {
   FieldItem* curr = this->current;
   if (curr->super.prev) {
      curr = (FieldItem*) curr->super.prev;
      this->current = curr;
      this->cursor = curr->len;
   }
   return curr;
}

int Field_run(Field* this, bool setCursor, bool* handled) {
   int cursorX, cursorY;
   Display_getyx(&cursorY, &cursorX);
   assert(this->current);
   FieldItem* curr = this->current;
   Display_attrset(this->labelColor);
   int x = this->x;
   Display_writeAtn(this->y, x, this->label, this->labelLen);
   x += this->labelLen;
   Display_writeAt(this->y, x, " [");
   x += 2;
   Display_writeAt(this->y, this->x + this->w - 1, "]");
   int w = this->w - 3  - this->labelLen;
   int scrollH = 0;

   Display_attrset(this->fieldColor);
   int display = curr->len - scrollH;
   if (display) {
      Display_writeAtn(this->y, x, curr->text + scrollH, display);
   }
   int rest = w - display;
   if (rest)
      Display_mvhline(this->y, x + display, ' ', rest);
   Display_attrset(A_NORMAL);
   if (setCursor) {
      Display_move(this->y, this->x + this->labelLen + 2 + this->cursor - scrollH);
   } else {
      Display_move(this->y, this->x + this->labelLen + 2 + this->cursor - scrollH);
      Display_attrset(A_REVERSE);
      if (this->cursor < curr->len) {
         Display_writeAtn(this->y, x + this->cursor, curr->text + scrollH + this->cursor, 1);
      } else {
         Display_writeAtn(this->y, x + this->cursor, " ", 1);
      }
      Display_move(cursorY, cursorX);
   }
   int ch = Display_getch();
   *handled = true;
   switch (ch) {
   case KEY_LEFT:
      if (this->cursor > 0)
         this->cursor--;
      break;
   case KEY_RIGHT:
      if (this->cursor < curr->len)
         this->cursor++;
      break;
   case KEY_UP:
      curr = Field_previousInHistory(this);
      break;
   case KEY_DOWN:
      if (curr->super.next) {
         curr = (FieldItem*) curr->super.next;
         this->current = curr;
         this->cursor = curr->len;
      }
      break;
   case KEY_C_LEFT:
      this->cursor = String_forwardWord(curr->text, curr->len, this->cursor);
      break;
   case KEY_C_RIGHT:
      this->cursor = String_backwardWord(curr->text, curr->len, this->cursor);
      break;
   case KEY_CTRL('A'):
   case KEY_HOME:
      this->cursor = 0;
      break;
   case KEY_CTRL('E'):
   case KEY_END:
      this->cursor = curr->len;
      break;
   case KEY_DC:
      if (curr->len > 0 && this->cursor < curr->len) {
         for (int i = this->cursor; i < this->w - 1; i++)
            curr->text[i] = curr->text[i+1];
         curr->len--;
         curr->text[curr->len] = '\0';
      }
      break;
   case '\177':
   case KEY_BACKSPACE:
      if (curr->len > 0 && this->x < curr->len && this->cursor > 0) {
         this->cursor--;
         for (int i = this->cursor; i < this->w - 1; i++)
            curr->text[i] = curr->text[i+1];
         curr->len--;
         curr->text[curr->len] = '\0';
      }
      break;
   default:
      *handled = false;
      break;
   }
   return ch;
}

void Field_insertChar(Field* this, int ch) {
   if (!this->current)
      Field_start(this);
   FieldItem* curr = this->current;
   if (curr->len < curr->w) {
      for (int i = curr->len; i > this->cursor; i--) {
         curr->text[i] = curr->text[i-1];
      }
      curr->text[this->cursor] = ch;
      curr->len++;
      curr->text[curr->len] = '\0';
      this->cursor++;
   }
}

void Field_setValue(Field* this, char* value) {
   if (!this->current)
      Field_start(this);
   FieldItem* curr = this->current;
   curr->len = MIN(strlen(value), curr->w);
   strncpy(curr->text, value, curr->len);
   this->cursor = MIN(curr->len, curr->w - 1);
}

char* Field_getValue(Field* this) {
   if (!this->current)
      return strdup("");
   return strdup(this->current->text);
}

int Field_quickRun(Field* this, bool* quitMask) {
   int result = 0;
   bool quit = false;
   if (!this->current)
      Field_start(this);
   while (!quit) {
      bool handled;
      int ch = Field_run(this, false, &handled);
      if (!handled) {
         if (ch >= 32)
            Field_insertChar(this, ch);
         else if (ch == 27 || ch == 13 || (ch >= 0 && ch <= 255 && quitMask[ch])) {
            quit = true;
            result = ch;
         }
      }
   }
   return result;
}
