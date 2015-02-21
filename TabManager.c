
#include "Prototypes.h"
//#needs Buffer

/*{

struct Jump_ {
   int x;
   int y;
   TabPage* page;
   Jump* prev;
};

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
   Jump* jumps;
   int x;
   int y;
   int w;
   int h;
   int tabOffset;
   int currentPage;
   int width;
   int defaultTabSize;
   bool redrawBar;
   bool bufferModified;
};

extern TabPageClass TabPageType;
   
extern bool CRT_hasColors;

}*/

static const char* TabManager_Untitled = "*untitled*";

void Jump_purge(Jump* jump) {
   while (jump) {
      Jump* prev = jump->prev;
      free(jump);
      jump = prev;
   }
}

TabPageClass TabPageType = {
   .super = {
      .size = sizeof(TabPage),
      .delete = TabPage_delete,
      .equals = Object_equals
   }
};

TabPage* TabPage_new(char* name, Buffer* buffer) {
   TabPage* this = Alloc(TabPage);
   if (name) {
      this->name = strdup(name);
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
   TabManager* this = (TabManager*) calloc(sizeof(TabManager), 1);
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
   Jump_purge(this->jumps);
   free(this);
}

static inline int TabManager_nameLength(const char* name) {
   int len;
   if (name) {
      const char* base = strrchr(name, '/');
      if (!base) {
         base = name;
      }
      len = strlen(base) + 4;
   } else {
      len = strlen(TabManager_Untitled) + 4;
   }
   return len;
}

int TabManager_add(TabManager* this, char* name, Buffer* buffer) {
   Vector_add(this->items, TabPage_new(name, buffer));
   this->width += TabManager_nameLength(name);
   return Vector_size(this->items) - 1;
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
   int lines, cols;
   Display_getScreenSize(&cols, &lines);
   int items = Vector_size(this->items);
   const int current = this->currentPage;
   assert(current < items);
   int x = this->x + this->tabOffset;
   Display_attrset(CRT_colors[TabColor]);
   Display_mvhline(lines - 1, x, ' ', cols - x);
   char buffer[256];
   int tabWidth = 15;
   for (int i = 0; i < items; i++) {
      TabPage* page = (TabPage*) Vector_get(this->items, i);
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
      Display_attrset(CRT_colors[TabColor]);
      Display_printAt(this->y + this->h - 1, x+1, "│");
      char* base = strrchr(label, '/');
      if (i == current) {
         Display_attrset(CRT_colors[CurrentTabColor]);
         tabWidth = 30;
         int offset = strlen(label) - (tabWidth - 2);
         if (offset > 0) {
            label += offset;
         }
      } else {
         tabWidth = 15;
         if (base) label = base + 1;
      }
      if (x+1 < cols-1) {
         Display_printAt(this->y + this->h - 1, x+2, "%c", modified);
         if ((!CRT_hasColors) && i == current) {
            snprintf(buffer, 255, ">%c<%s", modified, label);
         } else {
            snprintf(buffer, 255, "[%c]%s ", modified, label);
         }
         if (i == current && base > label) {
            int lenToSlash = base - label + 1;
            Display_attrset(CRT_colors[CurrentTabShadeColor]);
            Display_writeAtn(this->y + this->h - 1, x+3, label, MIN(lenToSlash, cols-x-1));
            Display_attrset(CRT_colors[CurrentTabColor]);
            Display_writeAtn(this->y + this->h - 1, x+3+lenToSlash, label + lenToSlash, MIN(tabWidth - lenToSlash, cols-x-1));
         } else {
            Display_writeAtn(this->y + this->h - 1, x+3, label, MIN(tabWidth-2, cols-x-1));
         }
      }
      x += tabWidth;
   }
   Display_attrset(CRT_colors[NormalColor]);
   this->redrawBar = false;
}

Buffer* TabManager_getBuffer(TabManager* this, int pageNr) {
   TabPage* page = (TabPage*) Vector_get(this->items, pageNr);
   if (!page) {
      return NULL;
   }
   if (page->buffer) {
      if (page->buffer->modified != this->bufferModified) {
         this->redrawBar = true;
         this->bufferModified = page->buffer->modified;
      }
   } else {
      page->buffer = Buffer_new(this->x, this->y, this->w, this->h-1, page->name, false, this);
      page->buffer->tabWidth = this->defaultTabSize;
      if (page->name && !TabManager_checkLock(this, page->name)) {
         page->buffer->modified = true;
         this->bufferModified = true;
      } else {
         this->bufferModified = false;
      }
   }
   return page->buffer;
}

Buffer* TabManager_draw(TabManager* this, int width) {
   Buffer* buffer = TabManager_getBuffer(this, this->currentPage);
   if (this->redrawBar)
      TabManager_drawBar(this, width);
   Buffer_draw(buffer);
   return buffer;
}

bool TabManager_checkLock(TabManager* this, char* fileName) {
   if (!fileName)
      return true;
   char* lockFileName = Files_encodePathAsFileName(fileName);
   bool exists = Files_existsHome("lock/%s", lockFileName);
   if (exists) {
      char question[1024];
      sprintf(question, "Looks like %s is already open in another instance. Open anyway?", fileName);
      return (TabManager_question(this, question, "yn") == 0);
   } else {
      FILE* lfd = Files_openHome("w", "lock/%s", lockFileName);
      if (lfd)
         fclose(lfd);
      else
         return false;
   }
   free(lockFileName);
   return true;
}

void TabManager_releaseLock(char* fileName) {
   if (!fileName)
      return;
   char* lockFileName = Files_encodePathAsFileName(fileName);
   Files_deleteHome("lock/%s", lockFileName);
   free(lockFileName);
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
         return i;
   }
   return -1;
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

void TabManager_markJump(TabManager* this) {
   Jump* jump = calloc(sizeof(Jump), 1);
   Buffer* buffer = TabManager_getBuffer(this, this->currentPage);
   jump->x = buffer->x;
   jump->y = buffer->y;
   jump->page = TabManager_current(this);
   jump->prev = this->jumps;
   this->jumps = jump;
}

void TabManager_jumpBack(TabManager* this) {
   Jump* jump = this->jumps;
   if (!jump)
      return;
   this->jumps = jump->prev;
   int idx = Vector_indexOf(this->items, jump->page);
   if (idx == -1) {
      free(jump);
      //TabManager_jumpBack(this);
      return;
   }
   TabManager_setPage(this, idx);
   Buffer_goto(TabManager_current(this)->buffer, jump->x, jump->y);
   free(jump);
}

int TabManager_getPageCount(TabManager* this) {
   return Vector_size(this->items);
}

char* TabManager_getPageName(TabManager* this, int i) {
   TabPage* page = (TabPage*) Vector_get(this->items, i);
   if (page->buffer) {
      return page->buffer->fileName;
   } else {
      return page->name;
   }
}

void TabManager_load(TabManager* this, char* fileName, int limit) {
   FILE* fd = Files_openHome("r", fileName, NULL);
   if (fd) {
      char line[4097];
      while (!feof(fd)) {
         char* ok = fgets(line, 4096, fd);
         if (ok) {
            char* enter = strrchr(line, '\n');
            if (enter) *enter = '\0';
            if (*line == '\0') continue;
            if (TabManager_find(this, (char*) line) == -1 && access(line, F_OK) == 0) {
               TabManager_add(this, (char*) line, NULL);
               limit--;
               if (!limit) break;
            }
         }
      }
      fclose(fd);
   }
}

void TabManager_save(TabManager* this, char* fileName) {
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
   int lines, cols;
   Display_getScreenSize(&cols, &lines);
   Display_attrset(CRT_colors[StatusColor]);
   Display_printAt(lines - 1, 0, "%s [%s]", question, options);
   Display_clearToEol();
   Display_attrset(CRT_colors[NormalColor]);
   Display_refresh();
   bool code;
   int opt;
   char* which;
   Display_beep();
   do {
      opt = Display_getch(&code);
   } while (code || !(which = strchr(options, opt)));
   TabManager_refreshCurrent(this);
   return which - options;
}
