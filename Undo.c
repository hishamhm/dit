
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

typedef enum UndoActionKind_ {
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
   UndoDiskState
} UndoActionKind;

struct UndoActionClass_ {
   ObjectClass super;
};

struct UndoAction_ {
   Object super;
   UndoActionKind kind;
   int x;
   int y;
   union {
      char c;
      struct {
         char* fileName;
         char* md5;
      } diskState;
      struct {
         char* buf;
         int len;
      } str;
      struct {
         int xTo;
         int yTo;
      } coord;
      struct {
         int lines;
         char width;
      } indent;
      int size;
      struct {
         int* buf;
         int len;
         bool tab;
      } unindent;
      bool backspace;
   } data;
};

extern UndoActionClass UndoActionType;

struct Undo_ {
   List* list;
   Stack* actions;
   int group;
};

}*/

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

UndoActionClass UndoActionType = {
   .super = {
      .size = sizeof(UndoAction),
      .delete = UndoAction_delete
   }
};

inline UndoAction* UndoAction_new(UndoActionKind kind, int x, int y) {
   UndoAction* this = Alloc(UndoAction);
   this->kind = kind;
   this->x = x;
   this->y = y;
   return this;
}

void UndoAction_delete(Object* cast) {
   UndoAction* this = (UndoAction*) cast;
   if (this->kind == UndoDeleteBlock) {
      free(this->data.str.buf);
   } else if (this->kind == UndoUnindent) {
      free(this->data.unindent.buf);
   } else if (this->kind == UndoDiskState) {
      free(this->data.diskState.fileName);
      free(this->data.diskState.md5);
   }
   free(this);
}

Undo* Undo_new(List* list) {
   Undo* this = (Undo*) malloc(sizeof(Undo));
   this->list = list;
   this->actions = Stack_new(ClassAs(UndoAction, Object), true);
   this->group = 0;
   return this;
}

void Undo_delete(Undo* this) {
   Stack_delete(this->actions);
   free(this);
}

inline void Undo_char(Undo* this, UndoActionKind kind, int x, int y, char data) {
   UndoAction* action = UndoAction_new(kind, x, y);
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
   if (top && c != '\t') {
      if ((top->kind == UndoInsertChar && top->y == y && top->x == x-1)
       || (top->kind == UndoInsertBlock && top->data.coord.yTo == y && top->data.coord.xTo == x)) {
         top->kind = UndoInsertBlock;
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

void Undo_indent(Undo* this, int x, int y, int lines, int width) {
   UndoAction* action = UndoAction_new(UndoIndent, x, y);
   action->data.indent.lines = lines;
   action->data.indent.width = width;
   Stack_push(this->actions, action, 0);
}

void Undo_unindent(Undo* this, int x, int y, int* counts, int lines, bool tab) {
   UndoAction* action = UndoAction_new(UndoUnindent, x, y);
   action->data.unindent.buf = counts;
   action->data.unindent.len = lines;
   action->data.unindent.tab = tab;
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

void Undo_diskState(Undo* this, int x, int y, char* md5, char* fileName) {
   char md5buffer[32];
   if (!fileName)
      return;
   if (!md5) {
      FILE* fd = fopen(fileName, "r");
      if (!fd)
         return;
      md5_stream(fd, md5buffer);
      fclose(fd);
      md5 = md5buffer;
   }
   UndoAction* action = UndoAction_new(UndoDiskState, x, y);
   action->data.diskState.md5 = malloc(16);
   action->data.diskState.fileName = strdup(fileName);
   memcpy(action->data.diskState.md5, md5, 16);
   Stack_push(this->actions, action, 0);
}

bool Undo_checkDiskState(Undo* this) {
   UndoAction* action = (UndoAction*) Stack_peek(this->actions, NULL);
   if (!action)
      return false;
   return (action->kind == UndoDiskState);
}

bool Undo_undo(Undo* this, int* x, int* y) {
   assert(x); assert(y);
   bool modified = true;
   UndoAction* action = (UndoAction*) Stack_pop(this->actions, NULL);
   if (!action)
      return false;
   *x = action->x;
   *y = action->y;
   Line* line = (Line*) List_get(this->list, action->y);
   switch (action->kind) {
   case UndoDiskState:
   {
      UndoAction_delete((Object*)action);
      return Undo_undo(this, x, y);
   }
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
      Line_unindent(line, action->data.indent.lines, action->data.indent.width);
      break;
   }
   case UndoUnindent:
   {
      for (int i = 0; i < action->data.unindent.len; i++) {
         for (int j = 0; j < action->data.unindent.buf[i]; j++)
            Line_insertCharAt(line, action->data.unindent.tab ? '\t' : ' ', 0);
         line = (Line*) line->super.next;
         assert(line);
      }
      break;
   }
   }
   UndoAction_delete((Object*)action);
   action = (UndoAction*) Stack_peek(this->actions, NULL);
   if (action && action->kind == UndoDiskState && action->data.diskState.fileName) {
      FILE* fd = fopen(action->data.diskState.fileName, "r");
      if (fd) {
         char md5[32];
         md5_stream(fd, md5);
         fclose(fd);
         if (memcmp(md5, action->data.diskState.md5, 16) == 0)
            modified = false;
      }
   }
   if (this->group)
      return Undo_undo(this, x, y);
   return modified;
}

static void Undo_makeFileName(Undo* this, char* fileName, char* undoFileName) {
   char rpath[4097];
   realpath(fileName, rpath);
   for(char *c = rpath; *c; c++)
      if (*c == '/')
         *c = ':';
   strncpy(undoFileName, rpath, 4096);
   undoFileName[4095] = '\0';
}

void Undo_store(Undo* this, char* fileName) {
   char undoFileName[4097];
   Undo_makeFileName(this, fileName, undoFileName);
   FILE* fd = fopen(fileName, "r");
   char md5buf[32];
   md5_stream(fd, &md5buf);
   fclose(fd);
   FILE* ufd = Files_openHome("w", "undo/%s", undoFileName);
   if (!ufd)
      return;
   fwrite(md5buf, 16, 1, ufd);
   if (Undo_checkDiskState(this))
      UndoAction_delete(Stack_pop(this->actions, NULL));
   int items = this->actions->size;
   items = MIN(items, 1000);
   fwrite(&items, sizeof(int), 1, ufd);
   int x, y;
   for (int i = items - 1; i >= 0; i--) {
      UndoAction* action = (UndoAction*) Stack_peekAt(this->actions, i, NULL);
      fwrite(&action->kind, sizeof(UndoActionKind), 1, ufd);
      fwrite(&action->x, sizeof(int), 1, ufd);
      fwrite(&action->y, sizeof(int), 1, ufd);
      x = action->x; 
      y = action->y;
      switch(action->kind) {
      case UndoBeginGroup:
      case UndoEndGroup:
      case UndoDiskState:
         break;
      case UndoIndent:
         fwrite(&action->data.indent.lines, sizeof(int), 1, ufd);
         fwrite(&action->data.indent.width, sizeof(int), 1, ufd);
         break;
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
         fwrite(&action->data.coord.xTo, sizeof(int), 1, ufd);
         fwrite(&action->data.coord.yTo, sizeof(int), 1, ufd);
         break;
      case UndoUnindent:
         fwrite(&action->data.unindent.len, sizeof(int), 1, ufd);
         fwrite(action->data.unindent.buf, sizeof(int), action->data.unindent.len, ufd);
         fwrite(&action->data.unindent.tab, sizeof(bool), 1, ufd);
         break;
      }
   }
   fclose(ufd);
   Undo_diskState(this, x, y, NULL, fileName);
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
   FILE* ufd = Files_openHome("r", "undo/%s", undoFileName);
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
   int x, y;
   for (int i = items - 1; i >= 0; i--) {
      UndoActionKind kind;
      read = fread(&kind, sizeof(UndoActionKind), 1, ufd);
      if (read < 1) { fclose(ufd); return; }
      read = fread(&x, sizeof(int), 1, ufd);
      if (read < 1) { fclose(ufd); return; }
      read = fread(&y, sizeof(int), 1, ufd);
      if (read < 1) { fclose(ufd); return; }
      UndoAction* action = UndoAction_new(kind, x, y);
      switch(action->kind) {
      case UndoBeginGroup:
      case UndoEndGroup:
         break;
      case UndoDiskState:
         action->data.diskState.fileName = NULL;
         action->data.diskState.md5 = NULL;
         break;
      case UndoIndent:
         read = fread(&action->data.indent.lines, sizeof(int), 1, ufd);
         if (read < 1) { fclose(ufd); return; }
         read = fread(&action->data.indent.width, sizeof(int), 1, ufd);
         if (read < 1) { fclose(ufd); return; }
         break;
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
         action->data.str.buf = malloc(sizeof(char) * action->data.str.len);
         read = fread(action->data.str.buf, sizeof(char), action->data.str.len, ufd);
         if (read < action->data.str.len) { fclose(ufd); return; }
         break;
      case UndoInsertBlock:
         read = fread(&action->data.coord.xTo, sizeof(int), 1, ufd);
         if (read < 1) { fclose(ufd); return; }
         read = fread(&action->data.coord.yTo, sizeof(int), 1, ufd);
         if (read < 1) { fclose(ufd); return; }
         break;
      case UndoUnindent:
         read = fread(&action->data.unindent.len, sizeof(int), 1, ufd);
         if (read < 1) { fclose(ufd); return; }
         action->data.unindent.buf = malloc(sizeof(int) * action->data.unindent.len);
         read = fread(action->data.unindent.buf, sizeof(int), action->data.unindent.len, ufd);
         if (read < action->data.unindent.len) { fclose(ufd); return; }
         read = fread(&action->data.unindent.tab, sizeof(bool), 1, ufd);
         if (read < 1) { fclose(ufd); return; }
         break;
      }
      Stack_push(this->actions, action, 0);
   }
   Undo_diskState(this, x, y, md5curr, fileName);
   fclose(ufd);
}
