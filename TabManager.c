
#include "Prototypes.h"
//#needs Buffer

/*{

struct TabPageClass_ {
   ObjectClass super;
};

struct TabPage_ {
   Object super;
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
   bool redrawBar;
   bool bufferModified;
};

extern TabPageClass TabPageType;
   
}*/

TabPageClass TabPageType = {
   .super = {
      .size = sizeof(TabPage),
      .delete = TabPage_delete
   }
};

TabPage* TabPage_new(char* name, Buffer* buffer) {
   TabPage* this = Alloc(TabPage);
   if (name) {
      this->name = String_copy(name);
   } else {
      this->name = NULL;
   }
   this->buffer = buffer;
   return this;
}

void TabPage_delete(Object* super) {
   TabPage* this = (TabPage*) super;
   free(this->name);
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
   this->redrawBar = true;
   this->bufferModified = false;
   return this;
}

void TabManager_delete(TabManager* this) {
   Vector_delete(this->items);
   free(this);
}

void TabManager_add(TabManager* this, char* name, Buffer* buffer) {
   Vector_add(this->items, TabPage_new(name, buffer));
}

void TabManager_removeCurrent(TabManager* this) {
   Vector_remove(this->items, this->currentPage);
   TabManager_setPage(this, this->currentPage + 1);
}

TabPage* TabManager_current(TabManager* this) {
   return (TabPage*) Vector_get(this->items, this->currentPage);
}

static inline void TabManager_drawBar(TabManager* this) {
   int items = Vector_size(this->items);
   int current = this->currentPage;
   assert(current < items);
   int x = this->x + this->tabOffset;
   attrset(CRT_colors[TabColor]);
   mvhline(LINES - 1, x, ' ', COLS - x);
   for (int i = 0; i < items; i++) {
      TabPage* page = (TabPage*) Vector_get(this->items, i);
      if (i == current)
         attrset(CRT_colors[CurrentTabColor]);
      else
         attrset(CRT_colors[TabColor]);
      char modified;
      char* label = "*untitled*";
      if (page->buffer) {
         modified = page->buffer->modified ? '*' : ' ';
         if (page->buffer->fileName)
            label = page->buffer->fileName;
      } else {
         modified = ' ';
         if (page->name)
            label = page->name;
      }
      char* base = strrchr(label, '/');
      if (base) label = base + 1;
      mvprintw(this->y + this->h - 1, x, " [%c]%s ", modified, label);
      x += strlen(label) + 5;
   }
   attrset(CRT_colors[NormalColor]);
   this->redrawBar = false;
}

Buffer* TabManager_draw(TabManager* this) {
   TabPage* page = (TabPage*) Vector_get(this->items, this->currentPage);
   if (page->buffer) {
      if (page->buffer->modified != this->bufferModified) {
         this->redrawBar = true;
         this->bufferModified = page->buffer->modified;
      }
   } else {
      page->buffer = Buffer_new(this->x, this->y, this->w, this->h-1, page->name, false);
      this->bufferModified = false;
   }
   if (this->redrawBar)
      TabManager_drawBar(this);
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
   this->redrawBar = true;
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

inline void TabManager_refreshCurrent(TabManager* this) {
   TabPage* page = (TabPage*) Vector_get(this->items, this->currentPage);
   if (page->buffer) {
      Buffer_refresh(page->buffer);
      this->bufferModified = page->buffer->modified;
   }
   this->redrawBar = true;
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
   assert(i >= 0);
   if (i >= Vector_size(this->items))
      i = 0;
   this->currentPage = i;
   TabManager_refreshCurrent(this);
}

char* TabManager_getPageName(TabManager* this, int i) {
   TabPage* page = (TabPage*) Vector_get(this->items, i);
   if (page->buffer) {
      return page->buffer->fileName;
   } else {
      return page->name;
   }
}
