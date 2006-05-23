
#define _GNU_SOURCE
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>
#include <stdlib.h>

#include "Prototypes.h"
//#needs Object

#include "md5.h"

/*{

typedef enum UndoActionType_ {
   UndoBreak,
   UndoJoinNext,
   UndoBackwardDeleteChar,
   UndoDeleteChar,
   UndoInsertChar,
   UndoDeleteBlock,
   UndoInsertBlock,
   UndoIndent,
   UndoUnindent,
   UndoBeginGroup,
   UndoEndGroup,
} UndoActionType;

struct UndoAction_ {
   Object super;
   UndoActionType type;
   int x;
   int y;
   union {
      char c;
      struct {
         char* buf;
         int len;
      } str;
      struct {
         int xTo;
         int yTo;
      } coord;
      int size;
      struct {
         int* buf;
         int len;
      } arr;
      bool backspace;
   } data;
};

extern char* UNDOACTION_CLASS;

struct Undo_ {
   List* list;
   Stack* actions;
   int group;
};

}*/

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

char* UNDOACTION_CLASS = "UndoAction";

inline UndoAction* UndoAction_new(UndoActionType type, int x, int y) {
   UndoAction* this = (UndoAction*) malloc(sizeof(UndoAction));
   ((Object*)this)->class = UNDOACTION_CLASS;
   ((Object*)this)->delete = UndoAction_delete;
   this->type = type;
   this->x = x;
   this->y = y;
   return this;
}

void UndoAction_delete(Object* cast) {
   UndoAction* this = (UndoAction*) cast;
   if (this->type == UndoDeleteBlock) {
      free(this->data.str.buf);
   } else if (this->type == UndoUnindent) {
      free(this->data.arr.buf);
   }
   free(this);
}

Undo* Undo_new(List* list) {
   Undo* this = (Undo*) malloc(sizeof(Undo));
   this->list = list;
   this->actions = Stack_new(UNDOACTION_CLASS, true);
   this->group = 0;
   return this;
}

void Undo_delete(Undo* this) {
   Stack_delete(this->actions);
   free(this);
}

inline void Undo_char(Undo* this, UndoActionType type, int x, int y, char data) {
   UndoAction* action = UndoAction_new(type, x, y);
   action->data.c = data;
   Stack_push(this->actions, action, 0);
}

void Undo_deleteCharAt(Undo* this, int x, int y, char c) {
   Undo_char(this, UndoDeleteChar, x, y, c);
}

void Undo_backwardDeleteCharAt(Undo* this, int x, int y, char c) {
   Undo_char(this, UndoBackwardDeleteChar, x, y, c);
}

void Undo_insertCharAt(Undo* this, int x, int y, char c) {
   UndoAction* top = (UndoAction*) Stack_peek(this->actions, NULL);
   if (top) {
      if ((top->type == UndoInsertChar && top->y == y && top->x == x-1)
       || (top->type == UndoInsertBlock && top->data.coord.yTo == y && top->data.coord.xTo == x)) {
         top->type = UndoInsertBlock;
         top->data.coord.xTo = x+1;
         top->data.coord.yTo = y;
         return;
      }
   }
   Undo_char(this, UndoInsertChar, x, y, c);
}

void Undo_breakAt(Undo* this, int x, int y, int indent) {
   UndoAction* action = UndoAction_new(UndoBreak, x, y);
   action->data.size = indent;
   Stack_push(this->actions, action, 0);
}

void Undo_joinNext(Undo* this, int x, int y, bool backspace) {
   UndoAction* action = UndoAction_new(UndoJoinNext, x, y);
   action->data.backspace = backspace;
   Stack_push(this->actions, action, 0);
}

void Undo_deleteBlock(Undo* this, int x, int y, char* block, int len) {
   assert(len > 0);
   UndoAction* action = UndoAction_new(UndoDeleteBlock, x, y);
   action->data.str.buf = block;
   action->data.str.len = len;
   Stack_push(this->actions, action, 0);
}

void Undo_indent(Undo* this, int x, int y, int lines) {
   UndoAction* action = UndoAction_new(UndoIndent, x, y);
   action->data.size = lines;
   Stack_push(this->actions, action, 0);
}

void Undo_unindent(Undo* this, int x, int y, int* counts, int lines) {
   UndoAction* action = UndoAction_new(UndoUnindent, x, y);
   action->data.arr.buf = counts;
   action->data.arr.len = lines;
   Stack_push(this->actions, action, 0);
}

void Undo_beginGroup(Undo* this, int x, int y) {
   UndoAction* action = UndoAction_new(UndoBeginGroup, x, y);
   Stack_push(this->actions, action, 0);
}

void Undo_endGroup(Undo* this, int x, int y) {
   UndoAction* action = UndoAction_new(UndoEndGroup, x, y);
   Stack_push(this->actions, action, 0);
}

void Undo_insertBlock(Undo* this, int x, int y, char* block, int len) {
   UndoAction* action = UndoAction_new(UndoInsertBlock, x, y);
   // Don't keep block in undo structure. Just calculate its size.
   char* walk = block;
   int yTo = y;
   int xTo = 0;
   while (walk - block < len) {
      char* at = memchr(walk, '\n', len - (walk - block));
      if (at) {
         yTo++;
         walk = at + 1;
      } else {
         xTo += (len - (walk - block)) + (yTo == y ? x : 0);
         break;
      }
   }
   action->data.coord.xTo = xTo;
   action->data.coord.yTo = yTo;
   Stack_push(this->actions, action, 0);
}

void Undo_insertBlanks(Undo* this, int x, int y, int len) {
   UndoAction* action = UndoAction_new(UndoInsertBlock, x, y);
   action->data.coord.xTo = x + len;
   action->data.coord.yTo = y;
   Stack_push(this->actions, action, 0);
}

void Undo_undo(Undo* this, int* x, int* y) {
   assert(x); assert(y);
   UndoAction* action = (UndoAction*) Stack_pop(this->actions, NULL);
   if (!action)
      return;
   *x = action->x;
   *y = action->y;
   Line* line = (Line*) List_get(this->list, action->y);
   switch (action->type) {
   case UndoBeginGroup:
   {
      this->group++;
      break;
   }
   case UndoEndGroup:
   {
      this->group--;
      break;
   }
   case UndoBreak:
   {
      Line_joinNext(line);
      if (action->data.size > 0)
         Line_deleteChars(line, action->x, action->data.size);
      break;
   }
   case UndoJoinNext:
   {
      Line_breakAt(line, action->x, 0);
      if (action->data.backspace) {
         *x = 0;
         *y = action->y + 1;
      }
      break;
   }
   case UndoBackwardDeleteChar:
   {
      Line_insertCharAt(line, action->data.c, action->x);
      *x = action->x + 1;
      break;
   }
   case UndoDeleteChar:
   {
      Line_insertCharAt(line, action->data.c, action->x);
      break;
   }
   case UndoInsertChar:
   {
      assert(action->data.c == line->text[action->x]);
      Line_deleteChars(line, action->x, 1);
      break;
   }
   case UndoInsertBlock:
   {
      int lines = (action->data.coord.yTo - action->y) + 1;
      StringBuffer_delete(Line_deleteBlock(line, lines, action->x, action->data.coord.xTo));
      break;
   }
   case UndoDeleteBlock:
   {
      int newX, newY;
      assert(action->data.str.len > 0);
      Line_insertBlock(line, action->x, action->data.str.buf, action->data.str.len, &newX, &newY);
      break;
   }
   case UndoIndent:
   {
      Line_unindent(line, action->data.size);
      break;
   }
   case UndoUnindent:
   {
      for (int i = 0; i < action->data.arr.len; i++) {
         for (int j = 0; j < action->data.arr.buf[i]; j++)
            Line_insertCharAt(line, ' ', 0);
         line = (Line*) line->super.next;
         assert(line);
      }
      break;
   }
   }
   UndoAction_delete((Object*)action);
   if (this->group)
      Undo_undo(this, x, y);
}

static void Undo_makeFileName(Undo* this, char* fileName, char* undoFileName) {
   char rpath[4097];
   realpath(fileName, rpath);
   snprintf(undoFileName, 4096, "%s/.dit", getenv("HOME"));
   undoFileName[4095] = '\0';
   mkdir(undoFileName, 0755);
   snprintf(undoFileName, 4096, "%s/.dit/undo", getenv("HOME"));
   undoFileName[4095] = '\0';
   mkdir(undoFileName, 0755);
   for(char *c = rpath; *c; c++)
      if (*c == '/')
         *c = ':';
   snprintf(undoFileName, 4096, "%s/.dit/undo/%s", getenv("HOME"), rpath);
   undoFileName[4095] = '\0';
}

void Undo_store(Undo* this, char* fileName) {
   char undoFileName[4097];
   Undo_makeFileName(this, fileName, undoFileName);
   FILE* fd = fopen(fileName, "r");
   char md5buf[32];
   md5_stream(fd, &md5buf);
   fclose(fd);
   FILE* ufd = fopen(undoFileName, "w");
   if (!ufd)
      return;
   fwrite(md5buf, 16, 1, ufd);
   int items = this->actions->size;
   items = MIN(items, 1000);
   fwrite(&items, sizeof(int), 1, ufd);
   for (int i = items - 1; i >= 0; i--) {
      UndoAction* action = (UndoAction*) Stack_peekAt(this->actions, i, NULL);
      fwrite(&action->type, sizeof(UndoActionType), 1, ufd);
      fwrite(&action->x, sizeof(int), 1, ufd);
      fwrite(&action->y, sizeof(int), 1, ufd);
      switch(action->type) {
      case UndoBeginGroup:
      case UndoEndGroup:
         break;
      case UndoIndent:
      case UndoBreak:
         fwrite(&action->data.size, sizeof(int), 1, ufd);
         break;
      case UndoJoinNext:
         fwrite(&action->data.backspace, sizeof(bool), 1, ufd);
         break;
      case UndoBackwardDeleteChar:
      case UndoDeleteChar:
      case UndoInsertChar:
         fwrite(&action->data.c, sizeof(char), 1, ufd);
         break;
      case UndoDeleteBlock:
         assert(action->data.str.len > 0);
         fwrite(&action->data.str.len, sizeof(int), 1, ufd);
         fwrite(action->data.str.buf, sizeof(char), action->data.str.len, ufd);
         break;
      case UndoInsertBlock:
         fwrite(&action->data.coord.xTo, sizeof(char), 1, ufd);
         fwrite(&action->data.coord.yTo, sizeof(char), 1, ufd);
         break;
      case UndoUnindent:
         fwrite(&action->data.arr.len, sizeof(int), 1, ufd);
         fwrite(action->data.arr.buf, sizeof(int), action->data.arr.len, ufd);
         break;
      }
   }
   fclose(ufd);
}

void Undo_restore(Undo* this, char* fileName) {
   char undoFileName[4097];
   Undo_makeFileName(this, fileName, undoFileName);
   FILE* fd = fopen(fileName, "r");
   if (!fd)
      return;
   char md5curr[32], md5saved[32];
   md5_stream(fd, md5curr);
   fclose(fd);
   FILE* ufd = fopen(undoFileName, "r");
   if (!ufd)
      return;
   int read = fread(md5saved, 16, 1, ufd);
   if (read < 1) { fclose(ufd); return; }
   if (memcmp(md5curr, md5saved, 16) != 0) {
      fclose(ufd);
      return;
   }
   int items;
   read = fread(&items, sizeof(int), 1, ufd);
   if (read < 1) { fclose(ufd); return; }
   for (int i = items - 1; i >= 0; i--) {
      UndoActionType type;
      int x, y;
      read = fread(&type, sizeof(UndoActionType), 1, ufd);
      if (read < 1) { fclose(ufd); return; }
      read = fread(&x, sizeof(int), 1, ufd);
      if (read < 1) { fclose(ufd); return; }
      read = fread(&y, sizeof(int), 1, ufd);
      if (read < 1) { fclose(ufd); return; }
      UndoAction* action = UndoAction_new(type, x, y);
      switch(action->type) {
      case UndoBeginGroup:
      case UndoEndGroup:
         break;
      case UndoIndent:
      case UndoBreak:
         read = fread(&action->data.size, sizeof(int), 1, ufd);
         if (read < 1) { fclose(ufd); return; }
         break;
      case UndoJoinNext:
         read = fread(&action->data.backspace, sizeof(bool), 1, ufd);
         if (read < 1) { fclose(ufd); return; }
         break;
      case UndoBackwardDeleteChar:
      case UndoDeleteChar:
      case UndoInsertChar:
         read = fread(&action->data.c, sizeof(char), 1, ufd);
         if (read < 1) { fclose(ufd); return; }
         break;
      case UndoDeleteBlock:
         read = fread(&action->data.str.len, sizeof(int), 1, ufd);
         if (read < 1) { fclose(ufd); return; }
         action->data.str.buf = malloc(sizeof(int) * action->data.str.len);
         read = fread(action->data.str.buf, sizeof(char), action->data.str.len, ufd);
         if (read < action->data.str.len) { fclose(ufd); return; }
         break;
      case UndoInsertBlock:
         read = fread(&action->data.coord.xTo, sizeof(char), 1, ufd);
         if (read < 1) { fclose(ufd); return; }
         read = fread(&action->data.coord.yTo, sizeof(char), 1, ufd);
         if (read < 1) { fclose(ufd); return; }
         break;
      case UndoUnindent:
         read = fread(&action->data.arr.len, sizeof(int), 1, ufd);
         if (read < 1) { fclose(ufd); return; }
         action->data.arr.buf = malloc(sizeof(int) * action->data.arr.len);
         read = fread(action->data.arr.buf, sizeof(int), action->data.arr.len, ufd);
         if (read < action->data.str.len) { fclose(ufd); return; }
         break;
      }
      Stack_push(this->actions, action, 0);
   }
   fclose(ufd);
}
