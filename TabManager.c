
#include "Prototypes.h"
//#needs Buffer

/*{

struct TabPageClass_ {
   ObjectClass super;
};

struct TabPage_ {
   Object super;
   char* label;
   char* name;
   Buffer* buffer;
};

struct TabManager_ {
   Vector* items;
   int x;
   int y;
   int w;
   int h;
   int tabOffset;
   int currentPage;
};

extern TabPageClass TabPageType;
   
}*/

TabPageClass TabPageType = {
   .super = {
      .size = sizeof(TabPage),
      .delete = TabPage_delete
   }
};

TabPage* TabPage_new(char* name, char* label, Buffer* buffer) {
   TabPage* this = Alloc(TabPage);
   this->name = String_copy(name);
   this->label = String_copy(label);
   this->buffer = buffer;
   return this;
}

void TabPage_delete(Object* super) {
   TabPage* this = (TabPage*) super;
   free(this->name);
   free(this->label);
   if (this->buffer)
      Buffer_delete(this->buffer);
   free(this);
}

TabManager* TabManager_new(int x, int y, int w, int h, int tabOffset) {
   TabManager* this = (TabManager*) malloc(sizeof(TabManager));
   this->x = x;
   this->y = y;
   this->w = w;
   this->h = h;
   this->tabOffset = tabOffset;
   this->items = Vector_new(ClassAs(TabPage, Object), true, DEFAULT_SIZE);
   this->currentPage = 0;
   return this;
}

void TabManager_delete(TabManager* this) {
   Vector_delete(this->items);
   free(this);
}

void TabManager_add(TabManager* this, char* name, char* label, Buffer* buffer) {
   Vector_add(this->items, TabPage_new(name, label, buffer));
}

Buffer* TabManager_draw(TabManager* this) {
   int items = Vector_size(this->items);
   int current = this->currentPage;
   assert(current < items);
   int x = this->x + this->tabOffset;
   for (int i = 0; i < items; i++) {
      TabPage* page = (TabPage*) Vector_get(this->items, i);
      if (i == current)
         attrset(CRT_colors[CurrentTabColor]);
      else
         attrset(CRT_colors[TabColor]);
      mvprintw(this->y + this->h - 1, x, " [%c]%s ", (page->buffer && page->buffer->modified) ? '*' : ' ', page->label);
      x += strlen(page->label) + 5;
   }
   attrset(CRT_colors[NormalColor]);
   TabPage* page = (TabPage*) Vector_get(this->items, this->currentPage);
   if (!page->buffer) {
      page->buffer = Buffer_new(this->x, this->y, this->w, this->h-1, page->name, false);
   }
   Buffer_draw(page->buffer);
   return page->buffer;
}

void TabManager_resize(TabManager* this, int w, int h) {
   this->w = w;
   this->h = h;
   int items = Vector_size(this->items);
   for (int i = 0; i < items; i++) {
      TabPage* page = (TabPage*) Vector_get(this->items, i);
      if (page->buffer)
         Buffer_resize(page->buffer, this->w, this->h - 1);
   }
}

int TabManager_find(TabManager* this, char* name) {
   int items = Vector_size(this->items);
   for (int i = 0; i < items; i++) {
      TabPage* page = (TabPage*) Vector_get(this->items, i);
      if (String_eq(page->name, name))
         return 1;
   }
   return 0;
}

/* private */
static inline void TabManager_refreshCurrent(TabManager* this) {
   TabPage* page = (TabPage*) Vector_get(this->items, this->currentPage);
   if (page->buffer)
      Buffer_refresh(page->buffer);
}

void TabManager_previousPage(TabManager* this) {
   this->currentPage--;
   if (this->currentPage == -1)
      this->currentPage = Vector_size(this->items) - 1;
   TabManager_refreshCurrent(this);
}

void TabManager_nextPage(TabManager* this) {
   this->currentPage++;
   int items = Vector_size(this->items);
   if (this->currentPage == items)
      this->currentPage = 0;
   TabManager_refreshCurrent(this);
}

void TabManager_setPage(TabManager* this, int i) {
   assert(i >= 0 && i < Vector_size(this->items));
   this->currentPage = i;
   TabManager_refreshCurrent(this);
}
