
#define _GNU_SOURCE
#include <string.h>

#include "Prototypes.h"
//#needs Object
//#needs List
//#needs Text

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
   Text text;
};

extern FieldItemClass FieldItemType;

#define Field_text(this) ((this)->current->text)

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

Field* Field_new(const char* label, int x, int y, int w) {
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
   this->text = Text_null();
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
   Text_prune(&(this->text));
   Pool_free(this->super.list->pool, this);
}

void Field_start(Field* this) {
   this->current = (FieldItem*) List_getLast(this->history);
   if (!this->current || Text_chars(this->current->text) > 0) {
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
      this->cursor = Text_chars(curr->text);
   }
   return curr;
}

int Field_run(Field* this, bool setCursor, bool* handled, bool* code) {
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

   Display_attrset(this->fieldColor);
   int display = Text_chars(curr->text);
   if (display) {
      Display_writeAtn(this->y, x, Text_toString(curr->text), Text_bytes(curr->text));
   }
   int rest = w - display;
   if (rest)
      Display_mvhline(this->y, x + display, ' ', rest);
   Display_attrset(A_NORMAL);
   if (setCursor) {
      Display_move(this->y, this->x + this->labelLen + 2 + this->cursor);
   } else {
      Display_move(this->y, this->x + this->labelLen + 2 + this->cursor);
      Display_attrset(A_REVERSE);
      if (this->cursor < Text_chars(curr->text)) {
         const char* ch = Text_stringAt(curr->text, this->cursor);
         Display_writeAtn(this->y, x + this->cursor, ch, UTF8_bytes(*ch));
      } else {
         Display_writeAtn(this->y, x + this->cursor, " ", 1);
      }
      Display_move(cursorY, cursorX);
   }
   int ch = Display_getch(code);
   if (!*code) {
      *handled = false;
      if (ch == KEY_CTRL('A'))      { ch = KEY_HOME; *code = true; }
      else if (ch == KEY_CTRL('E')) { ch = KEY_END; *code = true; }
      else if (ch == '\177')        { ch = KEY_BACKSPACE; *code = true; }
   }
   if (*code) {
      *handled = true;
      switch (ch) {
      case KEY_LEFT:
         if (this->cursor > 0)
            this->cursor--;
         break;
      case KEY_RIGHT:
         if (this->cursor < Text_chars(curr->text))
            this->cursor++;
         break;
      case KEY_UP:
         curr = Field_previousInHistory(this);
         break;
      case KEY_DOWN:
         if (curr->super.next) {
            curr = (FieldItem*) curr->super.next;
            this->current = curr;
            this->cursor = Text_chars(curr->text);
         }
         break;
      case KEY_C_LEFT:
         this->cursor = Text_forwardWord(curr->text, this->cursor);
         break;
      case KEY_C_RIGHT:
         this->cursor = Text_backwardWord(curr->text, this->cursor);
         break;
      case KEY_HOME:
         this->cursor = 0;
         break;
      case KEY_END:
         this->cursor = Text_chars(curr->text);
         break;
      case KEY_DC:
         Text_deleteChar(&(curr->text), this->cursor);
         break;
      case KEY_BACKSPACE:
         if (this->cursor > 0) {
            this->cursor--;
            Text_deleteChar(&(curr->text), this->cursor);
         }
         break;
      default:
         *handled = false;
         break;
      }
   }
   return ch;
}

void Field_insertChar(Field* this, wchar_t ch) {
   if (!this->current)
      Field_start(this);
   FieldItem* curr = this->current;
   Text_insertChar(&(curr->text), this->cursor, ch);
   this->cursor++;
}

void Field_clear(Field* this) {
   if (!this->current)
      Field_start(this);
   FieldItem* curr = this->current;
   Text_clear(&(curr->text));
   this->cursor = 0;
}

void Field_setValue(Field* this, Text value) {
   if (!this->current)
      Field_start(this);
   FieldItem* curr = this->current;
   curr->text = Text_copy(value);
   this->cursor = MIN(Text_chars(curr->text), this->cursor);
}

char* Field_getValue(Field* this) {
   if (this->current && Text_toString(this->current->text)) {
      return strdup(Text_toString(this->current->text));
   }
   return strdup("");
}

int Field_getLength(Field* this) {
   if (!this->current)
      return 0;
   return Text_chars(this->current->text);
}

int Field_quickRun(Field* this, bool* quitMask) {
   int result = 0;
   bool quit = false;
   if (!this->current)
      Field_start(this);
   while (!quit) {
      bool handled;
      bool code;
      int ch = Field_run(this, false, &handled, &code);
      if (!handled) {
         if (!code && ch >= 32)
            Field_insertChar(this, ch);
         else if (ch == 27 || ch == 13 || (ch >= 0 && ch <= 255 && quitMask[ch])) {
            quit = true;
            result = ch;
         }
      }
   }
   return result;
}
