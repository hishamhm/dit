
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
   int width;
};

extern TabPageClass TabPageType;
   
}*/

static const char* TabManager_Untitled = "*untitled*";

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
   TabManager_releaseLock(this->name);
   free(this->name);
   if (this->buffer) {
      Buffer_delete(this->buffer);
   }
   free(this);
}

TabManager* TabManager_new(int x, int y, int w, int h, int tabOffset) {
   TabManager* this = (TabManager*) malloc(sizeof(TabManager));
   this->x = x;
   this->y = y;
   this->w = w;
   this->h = h;
   this->tabOffset = tabOffset;
   this->width = 0;
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

static inline int TabManager_nameLength(const char* name) {
   int len;
   if (name) {
      char* base = strrchr(name, '/');
      len = strlen(base) + 4;
   } else {
      len = strlen(TabManager_Untitled) + 4;
   }
   return len;
}

void TabManager_add(TabManager* this, char* name, Buffer* buffer) {
   Vector_add(this->items, TabPage_new(name, buffer));
   this->width += TabManager_nameLength(name);
}

void TabManager_removeCurrent(TabManager* this) {
   const char* name = TabManager_current(this)->name;
   this->width -= TabManager_nameLength(name);
   assert(Vector_size(this->items) > 1);
   Vector_remove(this->items, this->currentPage);
   TabManager_setPage(this, this->currentPage);
}

TabPage* TabManager_current(TabManager* this) {
   return (TabPage*) Vector_get(this->items, this->currentPage);
}

static inline void TabManager_drawBar(TabManager* this, int width) {
   int items = Vector_size(this->items);
   int current = this->currentPage;
   assert(current < items);
   int x = this->x + this->tabOffset;
   attrset(CRT_colors[TabColor]);
   mvhline(LINES - 1, x, ' ', COLS - x);
   char buffer[256];
   int tabWidth = -1;
   if (this->width + this->tabOffset > width + 1)
      tabWidth = MAX(((width - this->tabOffset) / items), 15);
   for (int i = 0; i < items; i++) {
      TabPage* page = (TabPage*) Vector_get(this->items, i);
      if (i == current)
         attrset(CRT_colors[CurrentTabColor]);
      else
         attrset(CRT_colors[TabColor]);
      char modified;
      const char* label = TabManager_Untitled;
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
      if (i == current && tabWidth == -1)
         mvprintw(this->y + this->h - 1, x, " ");
      snprintf(buffer, 255, "[%c]%s ", modified, label);
      mvaddnstr(this->y + this->h - 1, x+1, buffer, tabWidth);
      x += (tabWidth == -1 ? strlen(label) + 4 : tabWidth);
   }
   attrset(CRT_colors[NormalColor]);
   this->redrawBar = false;
}

Buffer* TabManager_draw(TabManager* this, int width) {
   TabPage* page = (TabPage*) Vector_get(this->items, this->currentPage);
   if (page->buffer) {
      if (page->buffer->modified != this->bufferModified) {
         this->redrawBar = true;
         this->bufferModified = page->buffer->modified;
      }
   } else {
      if (page->name && !TabManager_checkLock(this, page->name)) {
         free(page->name);
         page->name = NULL;
      }
      page->buffer = Buffer_new(this->x, this->y, this->w, this->h-1, page->name, false);
      this->bufferModified = false;
   }
   if (this->redrawBar)
      TabManager_drawBar(this, width);
   Buffer_draw(page->buffer);
   return page->buffer;
}

bool TabManager_checkLock(TabManager* this, char* fileName) {
   if (!fileName)
      return true;
   char lockFileName[4097];
   Files_encodePathInFileName(fileName, lockFileName);
   if (Files_existsHome("lock/%s", lockFileName)) {
      char question[1024];
      sprintf(question, "Looks like %s is already open in another instance. Open anyway?", fileName);
      return (TabManager_question(this, question, "yn") == 0);
   } else {
      FILE* lfd = Files_openHome("w", "lock/%s", lockFileName);
      fclose(lfd);
   }
   return true;
}

void TabManager_releaseLock(char* fileName) {
   if (!fileName)
      return;
   char lockFileName[4097];
   Files_encodePathInFileName(fileName, lockFileName);
   Files_deleteHome("lock/%s", lockFileName);
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

void TabManager_load(TabManager* this, const char* fileName, int limit) {
   FILE* fd = Files_openHome("r", fileName, NULL);
   if (fd) {
      char line[4097];
      while (!feof(fd)) {
         char* ok = fgets(line, 4096, fd);
         if (ok) {
            char* enter = strrchr(line, '\n');
            if (enter) *enter = '\0';
            if (*line == '\0') continue;
            if (!TabManager_find(this, line) && access(line, F_OK) == 0) {
               TabManager_add(this, line, NULL);
               limit--;
               if (!limit) break;
            }
         }
      }
      fclose(fd);
   }
}

void TabManager_save(TabManager* this, const char* fileName) {
   FILE* fd = Files_openHome("w", fileName, NULL);
   if (fd) {
      int items = Vector_size(this->items);
      for (int i = 0; i < items; i++) {
         char* name = TabManager_getPageName(this, i);
         if (name)
            fprintf(fd, "%s\n", name);
      }
      fclose(fd);
   }
}

int TabManager_size(TabManager* this) {
   return Vector_size(this->items);
}

int TabManager_question(TabManager* this, char* question, char* options) {
   attrset(CRT_colors[StatusColor]);
   mvprintw(LINES - 1, 0, "%s [%s]", question, options);
   clrtoeol();
   attrset(CRT_colors[NormalColor]);
   refresh();
   int opt;
   char* which;
   beep();
   do {
      opt = getch();
   } while (!(which = strchr(options, opt)));
   TabManager_refreshCurrent(this);
   return which - options;
}
