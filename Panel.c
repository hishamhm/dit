
#include "Prototypes.h"
//#needs Object RichString

#include <math.h>
#include <sys/param.h>
#include <stdbool.h>

#include <curses.h>
//#link ncurses

/*{

typedef enum HandlerResult_ {
   HANDLED,
   IGNORED,
   BREAK_LOOP
} HandlerResult;

typedef HandlerResult(*Panel_eventHandler)(Panel*, int);

struct Panel_ {
   Object super;
   int x, y, w, h;
   WINDOW* window;
   List* items;
   int selected;
   int scrollV, scrollH;
   int oldSelected;
   bool needsRedraw;
   RichString header;
   Panel_eventHandler eventHandler;
   bool highlightBar;
   int cursorX;
   int displaying;
   int color;
};

extern char* PANEL_CLASS;

}*/

/* private property */
char* PANEL_CLASS = "Panel";

Panel* Panel_new(int x, int y, int w, int h, int color, char* type, bool owner) {
   Panel* this = (Panel*) malloc(sizeof(Panel));
   Panel_init(this, x, y, w, h, color, type, owner);
   return this;
}

void Panel_delete(Object* cast) {
   Panel* this = (Panel*)cast;
   Panel_done(this);
   free(this);
}

void Panel_init(Panel* this, int x, int y, int w, int h, int color, char* type, bool owner) {
   Object* super = (Object*) this;
   super->class = PANEL_CLASS;
   super->delete = Panel_delete;
   this->x = x;
   this->y = y;
   this->w = w;
   this->h = h;
   this->color = color;
   this->eventHandler = NULL;
   this->items = List_new(type);
   this->scrollV = 0;
   this->scrollH = 0;
   this->selected = 0;
   this->oldSelected = 0;
   this->needsRedraw = true;
   this->highlightBar = false;
   this->cursorX = 0;
   this->displaying = 0;
   RichString_prune(&(this->header));
}

void Panel_done(Panel* this) {
   assert (this != NULL);
   RichString_delete(this->header);
   List_delete(this->items);
}

void Panel_setHeader(Panel* this, RichString header) {
   assert (this != NULL);

   if (this->header.len > 0) {
      RichString_delete(this->header);
   }
   this->header = header;
   this->needsRedraw = true;
}

void Panel_move(Panel* this, int x, int y) {
   assert (this != NULL);

   this->x = x;
   this->y = y;
   this->needsRedraw = true;
}

void Panel_resize(Panel* this, int w, int h) {
   assert (this != NULL);

   if (this->header.len > 0)
      h--;
   this->w = w;
   this->h = h;
   this->needsRedraw = true;
}

void Panel_prune(Panel* this) {
   assert (this != NULL);

   List_prune(this->items);
   this->scrollV = 0;
   this->selected = 0;
   this->oldSelected = 0;
   this->needsRedraw = true;
}

void Panel_add(Panel* this, ListItem* l) {
   assert (this != NULL);

   List_add(this->items, l);
   this->needsRedraw = true;
}

void Panel_set(Panel* this, int i, ListItem* l) {
   assert (this != NULL);

   List_set(this->items, i, l);
}

ListItem* Panel_get(Panel* this, int i) {
   assert (this != NULL);

   return List_get(this->items, i);
}

Object* Panel_remove(Panel* this, int i) {
   assert (this != NULL);

   this->needsRedraw = true;
   Object* removed = NULL;
   List_remove(this->items, i);
   if (this->selected > 0 && this->selected >= List_size(this->items))
      this->selected--;
   return removed;
}

ListItem* Panel_getSelected(Panel* this) {
   assert (this != NULL);

   return List_get(this->items, this->selected);
}

/*
void Panel_moveSelectedUp(Panel* this) {
   assert (this != NULL);

   List_moveUp(this->items, this->selected);
   if (this->selected > 0)
      this->selected--;
}

void Panel_moveSelectedDown(Panel* this) {
   assert (this != NULL);

   List_moveDown(this->items, this->selected);
   if (this->selected + 1 < List_size(this->items))
      this->selected++;
}
*/

int Panel_getSelectedIndex(Panel* this) {
   assert (this != NULL);

   return this->selected;
}

int Panel_size(Panel* this) {
   assert (this != NULL);

   return List_size(this->items);
}

void Panel_setSelected(Panel* this, int selected) {
   assert (this != NULL);

   selected = MAX(0, MIN(List_size(this->items) - 1, selected));
   this->selected = selected;
}

void Panel_draw(Panel* this, bool focus) {
   assert (this != NULL);

   int cursorY = 0;
   int first, last;
   int itemCount = List_size(this->items);
   int scrollH = this->scrollH;
   int y = this->y; int x = this->x;
   int w = this->w; int h = this->h;
   first = this->scrollV;

   if (h > itemCount) {
      last = itemCount;
   } else {
      last = MIN(itemCount, this->scrollV + h);
   }
   if (this->selected < first) {
      first = this->selected;
      this->scrollV = first;
      this->needsRedraw = true;
   }
   if (this->selected >= last) {
      last = MIN(itemCount, this->selected + 1);
      first = MAX(0, last - h);
      this->scrollV = first;
      this->needsRedraw = true;
   }
   assert(first >= 0);
   assert(last <= itemCount);

   if (this->header.len > 0) {
      int attr = CRT_colors[HeaderColor];
      attron(attr);
      mvhline(y, x, ' ', w);
      attroff(attr);
      if (scrollH < this->header.len) {
         assert(this->header.len > 0);
         mvaddchnstr(y, x, this->header.chstr + scrollH,
                     MIN(this->header.len - scrollH, w));
      }
      y++;
   }

   scrollH = 0;
   
   int highlight;
   if (focus) {
      highlight = CRT_colors[SelectionColor];
   } else {
      highlight = CRT_colors[UnfocusedSelectionColor];
   }

   attrset(this->color);
   if (this->needsRedraw) {

      for(int i = first, j = 0; j < h && i < last; i++, j++) {
         Object* itemObj = (Object*) List_get(this->items, i);
         assert(itemObj);
         RichString itemRef; RichString_init(&itemRef);
         this->displaying = i;
         itemObj->display(itemObj, &itemRef);
         int amt = MIN(itemRef.len - scrollH, w);
         if (i == this->selected) {
            if (this->highlightBar) {
               attron(highlight);
               RichString_setAttr(&itemRef, highlight);
            }
            cursorY = y + j;
            mvhline(cursorY, x+amt, ' ', w-amt);
            if (amt > 0)
               mvaddchnstr(y+j, x+0, itemRef.chstr + scrollH, amt);
            if (this->highlightBar)
               attroff(highlight);
         } else {
            mvhline(y+j, x+amt, ' ', w-amt);

            if (amt > 0)
               mvaddchnstr(y+j, x+0, itemRef.chstr + scrollH, amt);
         }
      }
      for (int i = y + (last - first); i < y + h; i++)
         mvhline(i, x+0, ' ', w);

      /* paint scrollbar */
      {
         float step = (float)h / (float) itemCount ;
         int handleHeight = ceil(step * (float)h);
         int startAt = step * (float)this->scrollV;
         Color bar = CRT_colors[ScrollBarColor];
         Color handle = CRT_colors[ScrollHandleColor];
         for (int i = 0; i < h; i++) {
            char ch;
            if (i >= startAt && handleHeight) {
               attrset(handle);
               ch = CRT_scrollHandle;
               handleHeight--;
            } else {
               attrset(bar);
               ch = CRT_scrollBar;
            }
            mvaddch(y + i, w, ch);
         }
         attrset(this->color);
      }

      this->needsRedraw = false;

   } else {
      Object* oldObj = (Object*) List_get(this->items, this->oldSelected);
      RichString oldRef; RichString_init(&oldRef);
      this->displaying = this->oldSelected;
      oldObj->display(oldObj, &oldRef);
      Object* newObj = (Object*) List_get(this->items, this->selected);
      RichString newRef; RichString_init(&newRef);
      this->displaying = this->selected;
      newObj->display(newObj, &newRef);
      mvhline(y+ this->oldSelected - this->scrollV, x+0, ' ', w);
      if (scrollH < oldRef.len)
         mvaddchnstr(y+ this->oldSelected - this->scrollV, x+0, oldRef.chstr + scrollH, MIN(oldRef.len - scrollH, w));
      if (this->highlightBar)
         attron(highlight);
      cursorY = y+this->selected - this->scrollV;
      mvhline(cursorY, x+0, ' ', w);
      if (this->highlightBar)
         RichString_setAttr(&newRef, highlight);
      if (scrollH < newRef.len)
         mvaddchnstr(y+this->selected - this->scrollV, x+0, newRef.chstr + scrollH, MIN(newRef.len - scrollH, w));
      if (this->highlightBar)
         attroff(highlight);
   }
   this->oldSelected = this->selected;

   move(cursorY, this->cursorX);
}

bool Panel_onKey(Panel* this, int key) {
   assert (this != NULL);
   switch (key) {
   case KEY_DOWN:
      if (this->selected + 1 < List_size(this->items))
         this->selected++;
      return true;
   case KEY_UP:
      if (this->selected > 0)
         this->selected--;
      return true;
   case KEY_C_DOWN:
      if (this->selected + 1 < List_size(this->items)) {
         this->selected++;
         if (this->scrollV < List_size(this->items) - this->h) {
            this->scrollV++;
            this->needsRedraw = true;
         }
      }
      return true;
   case KEY_C_UP:
      if (this->selected > 0) {
         this->selected--;
         if (this->scrollV > 0) {
            this->scrollV--;
            this->needsRedraw = true;
         }
      }
      return true;
   case KEY_LEFT:
      if (this->scrollH > 0) {
         this->scrollH -= 5;
         this->needsRedraw = true;
      }
      return true;
   case KEY_RIGHT:
      this->scrollH += 5;
      this->needsRedraw = true;
      return true;
   case KEY_PPAGE:
      this->selected -= (this->h - 1);
      this->scrollV -= (this->h - 1);
      if (this->selected < 0)
         this->selected = 0;
      if (this->scrollV < 0)
         this->scrollV = 0;
      this->needsRedraw = true;
      return true;
   case KEY_NPAGE:
      this->selected += (this->h - 1);
      int size = List_size(this->items);
      if (this->selected >= size)
         this->selected = size - 1;
      this->scrollV += (this->h - 1);
      if (this->scrollV >= MAX(0, size - this->h))
         this->scrollV = MAX(0, size - this->h - 1);
      this->needsRedraw = true;
      return true;
   case KEY_HOME:
      this->selected = 0;
      return true;
   case KEY_END:
      this->selected = List_size(this->items) - 1;
      return true;
   }
   return false;
}
