
#include "Prototypes.h"

#include <stdbool.h>

/*{

typedef enum Orientation_ {
   VERTICAL,
   HORIZONTAL
} Orientation;

struct ScreenManager_ {
   int x1;
   int y1;
   int x2;
   int y2;
   Orientation orientation;
   Vector* items;
   int itemCount;
   FunctionBar* fuBar;
   bool owner;
};

}*/

ScreenManager* ScreenManager_new(int x1, int y1, int x2, int y2, Orientation orientation, bool owner) {
   ScreenManager* this;
   this = malloc(sizeof(ScreenManager));
   this->x1 = x1;
   this->y1 = y1;
   this->x2 = x2;
   this->y2 = y2;
   this->fuBar = NULL;
   this->orientation = orientation;
   this->items = Vector_new(ClassAs(Panel, Object), owner, DEFAULT_SIZE);
   this->itemCount = 0;
   this->owner = owner;
   return this;
}

void ScreenManager_delete(ScreenManager* this) {
   Vector_delete(this->items);
   if (this->owner)
      FunctionBar_delete(this->fuBar);
   free(this);
}

inline int ScreenManager_size(ScreenManager* this) {
   return this->itemCount;
}

void ScreenManager_add(ScreenManager* this, Panel* item, int size) {
   if (this->orientation == HORIZONTAL) {
      int lastX = 0;
      if (this->itemCount > 0) {
         Panel* last = (Panel*) Vector_get(this->items, this->itemCount - 1);
         lastX = last->x + last->w + 1;
      }
      if (size > 0) {
         Panel_resize(item, size, LINES-this->y1+this->y2);
      } else {
         Panel_resize(item, COLS-this->x1+this->x2-lastX, LINES-this->y1+this->y2);
      }
      Panel_move(item, lastX, this->y1);
   }
   // TODO: VERTICAL
   Vector_add(this->items, item);
   item->needsRedraw = true;
   this->itemCount++;
}

Panel* ScreenManager_remove(ScreenManager* this, int index) {
   assert(this->itemCount > index);
   Panel* lb = (Panel*) Vector_remove(this->items, index);
   this->itemCount--;
   return lb;
}

void ScreenManager_setFunctionBar(ScreenManager* this, FunctionBar* fuBar) {
   if (this->owner && this->fuBar)
      FunctionBar_delete(this->fuBar);
   this->fuBar = fuBar;
}

void ScreenManager_resize(ScreenManager* this, int x1, int y1, int x2, int y2) {
   this->x1 = x1;
   this->y1 = y1;
   this->x2 = x2;
   this->y2 = y2;
   int items = this->itemCount;
   int lastX = 0;
   for (int i = 0; i < items - 1; i++) {
      Panel* lb = (Panel*) Vector_get(this->items, i);
      Panel_resize(lb, lb->w, LINES-y1+y2);
      Panel_move(lb, lastX, y1);
      lastX = lb->x + lb->w + 1;
   }
   Panel* lb = (Panel*) Vector_get(this->items, items-1);
   Panel_resize(lb, COLS-x1+x2-lastX, LINES-y1+y2);
   Panel_move(lb, lastX, y1);
}

void ScreenManager_run(ScreenManager* this, Panel** lastFocus, int* lastKey) {
   bool quit = false;
   int focus = 0;
         
   Panel* lbFocus = (Panel*) Vector_get(this->items, focus);
   if (this->fuBar)
      FunctionBar_draw(this->fuBar, NULL);
   
   int ch;
   while (!quit) {
      int items = this->itemCount;
      for (int i = 0; i < items; i++) {
         Panel* lb = (Panel*) Vector_get(this->items, i);
         Panel_setFocus(lb, i == focus);
         Panel_draw(lb);
         if (i < items) {
            if (this->orientation == HORIZONTAL) {
               mvvline(lb->y, lb->x+lb->w, ' ', lb->h+1);
            }
         }
      }
      if (this->fuBar)
         FunctionBar_draw(this->fuBar, NULL);

      ch = getch();
      
      bool loop = false;
      if (ch == KEY_MOUSE) {
         MEVENT mevent;
         int ok = getmouse(&mevent);
         if (ok == OK) {
            if (mevent.y == LINES - 1) {
               ch = FunctionBar_synthesizeEvent(this->fuBar, mevent.x);
            } else {
               for (int i = 0; i < this->itemCount; i++) {
                  Panel* lb = (Panel*) Vector_get(this->items, i);
                  if (mevent.x > lb->x && mevent.x <= lb->x+lb->w &&
                     mevent.y > lb->y && mevent.y <= lb->y+lb->h) {
                     focus = i;
                     lbFocus = lb;
                     Panel_setSelected(lb, mevent.y - lb->y + lb->scrollV - 1);
                     loop = true;
                     break;
                  }
               }
            }
         }
      }
      if (loop) continue;
      
      if (lbFocus->eventHandler) {
         HandlerResult result = lbFocus->eventHandler(lbFocus, ch);
         if (result == HANDLED) {
            continue;
         } else if (result == BREAK_LOOP) {
            quit = true;
            continue;
         }
      }
      
      switch (ch) {
      case ERR:
         continue;
      case KEY_RESIZE:
      {
         ScreenManager_resize(this, this->x1, this->y1, this->x2, this->y2);
         continue;
      }
      case KEY_LEFT:
         focus--;
         if (focus == -1) {
            focus = this->itemCount - 1;
         }
         lbFocus = (Panel*) Vector_get(this->items, focus);
         break;
      case KEY_RIGHT:
      case 9:
         focus++;
         if (focus == this->itemCount) {
            focus = 0;
         }
         lbFocus = (Panel*) Vector_get(this->items, focus);
         break;
      case KEY_F(10):
      case 'q':
      case 27:
         quit = true;
         continue;
      default:
         Panel_onKey(lbFocus, ch);
         break;
      }
   }

   *lastFocus = lbFocus;
   *lastKey = ch;
}
