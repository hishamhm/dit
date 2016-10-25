
#include "Prototypes.h"
//#needs Object RichString Display

#include <math.h>
#include <sys/param.h>
#include <stdbool.h>

/*{

typedef enum HandlerResult_ {
   HANDLED,
   IGNORED,
   BREAK_LOOP
} HandlerResult;

typedef HandlerResult(*Method_Panel_eventHandler)(Panel*, int);

struct PanelClass_ {
   ObjectClass super;
};

struct Panel_ {
   Object super;
   int x, y, w, h;
   List* items;
   int selected;
   int scrollV, scrollH;
   int oldSelected;
   bool needsRedraw;
   RichString header;
   bool highlightBar;
   int cursorX;
   int displaying;
   int color;
   bool focus;
   Method_Panel_eventHandler eventHandler;
};

extern PanelClass PanelType;

}*/

PanelClass PanelType = {
   .super = {
      .size = sizeof(Panel),
      .display = NULL,
      .equals = Object_equals,
      .delete = Panel_delete
   }
};

Panel* Panel_new(int x, int y, int w, int h, int color, ListItemClass* class, bool owner, void* data) {
   Panel* this = Alloc(Panel);
   Panel_init(this, x, y, w, h, color, class, owner, data);
   return this;
}

void Panel_delete(Object* cast) {
   Panel* this = (Panel*)cast;
   RichString_end(this->header);
   Panel_done(this);
   free(this);
}

void Panel_init(Panel* this, int x, int y, int w, int h, int color, ListItemClass* class, bool owner, void* data) {
   this->x = x;
   this->y = y;
   this->w = w;
   this->h = h;
   this->color = color;
   this->eventHandler = NULL;
   this->items = List_new(class, data);
   this->scrollV = 0;
   this->scrollH = 0;
   this->selected = 0;
   this->oldSelected = 0;
   this->needsRedraw = true;
   this->highlightBar = false;
   this->cursorX = 0;
   this->displaying = 0;
   this->focus = true;
   RichString_beginAllocated(this->header);
}

void Panel_done(Panel* this) {
   assert (this != NULL);
   List_delete(this->items);
}

void Panel_setFocus(Panel* this, bool focus) {
   this->focus = focus;
}

void Panel_setHeader(Panel* this, RichString header) {
   assert (this != NULL);
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

   if (RichString_sizeVal(this->header) > 0)
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

void Panel_draw(Panel* this) {
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

   int headerLen = RichString_sizeVal(this->header);
   if (headerLen > 0) {
      Display_attrset(CRT_colors[HeaderColor]);
      Display_mvhline(y, x, ' ', w);
      if (scrollH < headerLen) {
         assert(headerLen > 0);
         Display_writeChstrAtn(y, x, RichString_at(this->header, scrollH),
                     MIN(headerLen - scrollH, w));
      }
      y++;
   }

   scrollH = 0;
   
   int highlight;
   if (this->focus) {
      highlight = CRT_colors[SelectionColor];
   } else {
      highlight = CRT_colors[UnfocusedSelectionColor];
   }
   Display_attrset(this->color);
   if (this->needsRedraw) {
      for(int i = first, j = 0; j < h && i < last; i++, j++) {
         Object* itemObj = (Object*) List_get(this->items, i);
         assert(itemObj);
         RichString_begin(itemRef);
         this->displaying = i;
         Msg(Object, display, itemObj, &itemRef);
         int amt = MIN(RichString_sizeVal(itemRef) - scrollH, w);
         if (i == this->selected) {
            if (this->highlightBar) {
               Display_attrset(highlight);
               RichString_setAttr(&itemRef, highlight);
            }
            cursorY = y + j;
            Display_mvhline(cursorY, x+amt, ' ', w-amt);
            if (amt > 0)
               Display_writeChstrAtn(y+j, x+0, RichString_at(itemRef, scrollH), amt);
            if (this->highlightBar)
               Display_attrset(this->color);
         } else {
            Display_mvhline(y+j, x+amt, ' ', w-amt);

            if (amt > 0)
               Display_writeChstrAtn(y+j, x+0, RichString_at(itemRef, scrollH), amt);
         }
         RichString_end(itemRef);
      }
      for (int i = y + (last - first); i < y + h; i++)
         Display_mvhline(i, x+0, ' ', w);

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
               Display_attrset(handle);
               ch = CRT_scrollHandle;
               handleHeight--;
            } else {
               Display_attrset(bar);
               ch = CRT_scrollBar;
            }
            Display_writeChAt(y + i, w, ch);
         }
         Display_attrset(this->color);
      }

      this->needsRedraw = false;

   } else {
      Object* oldObj = (Object*) List_get(this->items, this->oldSelected);
      RichString_begin(oldRef);
      this->displaying = this->oldSelected;
      Msg(Object, display, oldObj, &oldRef);
      Object* newObj = (Object*) List_get(this->items, this->selected);
      RichString_begin(newRef);
      this->displaying = this->selected;
      Msg(Object, display, newObj, &newRef);
      Display_mvhline(y+ this->oldSelected - this->scrollV, x+0, ' ', w);
      int oldLen = RichString_sizeVal(oldRef);
      if (scrollH < oldLen)
         Display_writeChstrAtn(y+ this->oldSelected - this->scrollV, x+0, RichString_at(oldRef, scrollH), MIN(oldLen - scrollH, w));
      if (this->highlightBar)
         Display_attrset(highlight);
      cursorY = y+this->selected - this->scrollV;
      Display_mvhline(cursorY, x+0, ' ', w);
      if (this->highlightBar)
         RichString_setAttr(&newRef, highlight);
      int newLen = RichString_sizeVal(newRef);
      if (scrollH < newLen)
         Display_writeChstrAtn(y+this->selected - this->scrollV, x+0, RichString_at(newRef, scrollH), MIN(newLen - scrollH, w));
      if (this->highlightBar)
         Display_attrset(this->color);
      RichString_end(oldRef);
      RichString_end(newRef);
   }
   this->oldSelected = this->selected;

   Display_move(cursorY, this->cursorX);
}

void Panel_slide(Panel* this, int n) {
   while (n != 0) {
      if (n > 0) {
         if (this->selected + 1 < List_size(this->items)) {
            this->selected++;
            if (this->scrollV < List_size(this->items) - this->h) {
               this->scrollV++;
               this->needsRedraw = true;
            }
         }
         n--;
      } else {
         if (this->selected > 0) {
            this->selected--;
            if (this->scrollV > 0) {
               this->scrollV--;
               this->needsRedraw = true;
            }
         }
         n++;
      }
   }
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
      Panel_slide(this, 1);
      return true;
   case KEY_C_UP:
      Panel_slide(this, -1);
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
