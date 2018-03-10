
#define _GNU_SOURCE
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "Prototypes.h"
//#needs Object

#include "md5.h"

#define DIT_UNDO_MAGIC "XDIT1"

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
      wchar_t ch;
      struct {
         char* fileName;
         char* md5;
      } diskState;
      struct {
         Text text;
         struct {
            int xTo;
            int yTo;
         } coord;
      } block;
      int size;
      struct {
         int* buf;
         int len;
         int size;
      } tab;
      bool backspace;
   } data;
};

extern UndoActionClass UndoActionType;

struct Undo_ {
   List* list;
   Stack* actions;
   Stack* redoActions;
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
   if (this->kind == UndoDeleteBlock || this->kind == UndoInsertBlock) {
      Text_prune(&(this->data.block.text));
   } else if (this->kind == UndoUnindent || this->kind == UndoIndent) {
      free(this->data.tab.buf);
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
   this->redoActions = Stack_new(ClassAs(UndoAction, Object), true);
   this->group = 0;
   return this;
}

void Undo_delete(Undo* this) {
   Stack_delete(this->actions);
   Stack_delete(this->redoActions);
   free(this);
}

static inline void pushUndo(Undo* this, UndoAction* action) {
   Stack_empty(this->redoActions);
   Stack_push(this->actions, action);
}

static inline void Undo_char(Undo* this, UndoActionKind kind, int x, int y, wchar_t ch) {
   UndoAction* action = UndoAction_new(kind, x, y);
   action->data.ch = ch;
   pushUndo(this, action);
}

static inline void Undo_block(Undo* this, UndoActionKind kind, int x, int y, Text text) {
   assert(len > 0);
   UndoAction* action = UndoAction_new(UndoDeleteBlock, x, y);
   action->data.block.text = text;

   /* calculate block coordinates */
   const char* walk = text.data;
   int len = text.bytes;
   int yTo = y;
   int xTo = 0;
   while (walk - text.data < len) {
      char* at = memchr(walk, '\n', len - (walk - text.data));
      if (at) {
         yTo++;
         walk = at + 1;
      } else {
         xTo += Text_chars(Text_new((char*)walk)) + (yTo == y ? x : 0);
         break;
      }
   }
   action->data.block.coord.xTo = xTo;
   action->data.block.coord.yTo = yTo;

   pushUndo(this, action);
}

static inline void Undo_tab(Undo* this, UndoActionKind kind, int x, int y, int* counts, int lines, int size) {
   UndoAction* action = UndoAction_new(kind, x, y);
   action->data.tab.buf = counts;
   action->data.tab.len = lines;
   action->data.tab.size = size;
   pushUndo(this, action);
}

void Undo_deleteCharAt(Undo* this, int x, int y, wchar_t ch) {
   Undo_char(this, UndoDeleteChar, x, y, ch);
}

void Undo_backwardDeleteCharAt(Undo* this, int x, int y, wchar_t ch) {
   Undo_char(this, UndoBackwardDeleteChar, x, y, ch);
}

static inline void convertToBlock(UndoAction* top) {
   wchar_t first = top->data.ch;
   top->kind = UndoInsertBlock;
   top->data.block.text = Text_new(strdup(""));
   Text_appendChar(&(top->data.block.text), first);
}

static inline void addToBlock(UndoAction* top, int x, int y, wchar_t ch) {
   Text_appendChar(&(top->data.block.text), ch);
   top->data.block.coord.xTo = x+1;
   top->data.block.coord.yTo = y;
}

void Undo_insertCharAt(Undo* this, int x, int y, wchar_t ch) {
   UndoAction* top = (UndoAction*) Stack_peek(this->actions);
   if (ch == '\t' || !top) { // is this case for '\t' still valid?
      Undo_char(this, UndoInsertChar, x, y, ch);
   } else if (top->kind == UndoInsertChar && top->y == y && top->x == x-1) {
      convertToBlock(top);
      addToBlock(top, x, y, ch);
   } else if (top->kind == UndoInsertBlock && top->data.block.coord.yTo == y && top->data.block.coord.xTo == x) {
      addToBlock(top, x, y, ch);
   } else {
      Undo_char(this, UndoInsertChar, x, y, ch);
   }
}

void Undo_breakAt(Undo* this, int x, int y, int indent) {
   UndoAction* action = UndoAction_new(UndoBreak, x, y);
   action->data.size = indent;
   pushUndo(this, action);
}

void Undo_joinNext(Undo* this, int x, int y, bool backspace) {
   UndoAction* action = UndoAction_new(UndoJoinNext, x, y);
   action->data.backspace = backspace;
   pushUndo(this, action);
}

void Undo_deleteBlock(Undo* this, int x, int y, char* block, int len) {
   Undo_block(this, UndoDeleteBlock, x, y, Text_newWithSize(block, len));
}

void Undo_insertBlock(Undo* this, int x, int y, Text text) {
   Undo_block(this, UndoInsertBlock, x, y, text);
}

void Undo_indent(Undo* this, int x, int y, int lines, int size) {
   int* counts = malloc(sizeof(int) * lines);
   for (int i = 0; i < lines; i++) {
      counts[i] = size;
   }
   Undo_tab(this, UndoIndent, x, y, counts, lines, size);
}

void Undo_unindent(Undo* this, int x, int y, int* counts, int lines, int size) {
   Undo_tab(this, UndoUnindent, x, y, counts, lines, size);
}

void Undo_beginGroup(Undo* this, int x, int y) {
   UndoAction* action = UndoAction_new(UndoBeginGroup, x, y);
   pushUndo(this, action);
}

void Undo_endGroup(Undo* this, int x, int y) {
   UndoAction* action = UndoAction_new(UndoEndGroup, x, y);
   pushUndo(this, action);
}

void Undo_insertBlanks(Undo* this, int x, int y, int len) {
   UndoAction* action = UndoAction_new(UndoInsertBlock, x, y);
   action->data.block.coord.xTo = x + len;
   action->data.block.coord.yTo = y;
   pushUndo(this, action);
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
   pushUndo(this, action);
}

bool Undo_checkDiskState(Undo* this) {
   UndoAction* action = (UndoAction*) Stack_peek(this->actions);
   if (!action)
      return false;
   return (action->kind == UndoDiskState);
}

bool Undo_undo(Undo* this, int* x, int* y) {
   assert(x); assert(y);
   bool modified = true;
   UndoAction* action = (UndoAction*) Stack_pop(this->actions);
   if (!action)
      return false;
   *x = action->x;
   *y = action->y;
   Line* line = (Line*) List_get(this->list, action->y);
   switch (action->kind) {
   case UndoDiskState:
   {
      Stack_push(this->redoActions, action);
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
      Line_insertChar(line, action->x, action->data.ch);
      *x = action->x + 1;
      break;
   }
   case UndoDeleteChar:
   {
      Line_insertChar(line, action->x, action->data.ch);
      break;
   }
   case UndoInsertChar:
   {
      assert(action->data.ch == Line_charAt(line, action->x));
      Line_deleteChars(line, action->x, 1);
      break;
   }
   case UndoInsertBlock:
   {
      int lines = (action->data.block.coord.yTo - action->y) + 1;
      StringBuffer_delete(Line_deleteBlock(line, lines, action->x, action->data.block.coord.xTo));
      break;
   }
   case UndoDeleteBlock:
   {
      int newX, newY;
      assert(Text_chars(action->data.block.text) > 0);
      Line_insertBlock(line, action->x, action->data.block.text, &newX, &newY);
      break;
   }
   case UndoIndent:
   {
      int* lines = Line_unindent(line, action->data.tab.len, action->data.tab.size);
      free(lines);
      break;
   }
   case UndoUnindent:
   {
      for (int i = 0; i < action->data.tab.len; i++) {
         for (int j = 0; j < action->data.tab.buf[i]; j++)
            Line_insertChar(line, 0, action->data.tab.size == 0 ? '\t' : ' ');
         line = (Line*) line->super.next;
         assert(line);
      }
      break;
   }
   }
   Stack_push(this->redoActions, action);
   action = (UndoAction*) Stack_peek(this->actions);
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

bool Undo_redo(Undo* this, int* x, int* y) {
   assert(x); assert(y);
   bool modified = true;
   UndoAction* action = (UndoAction*) Stack_pop(this->redoActions);
   if (!action)
      return false;
   *x = action->x;
   *y = action->y;
   Line* line = (Line*) List_get(this->list, action->y);
   switch (action->kind) {
   case UndoDiskState:
   {
      Stack_push(this->actions, action);
      return Undo_redo(this, x, y);
   }
   case UndoBeginGroup:
   {
      this->group--;
      break;
   }
   case UndoEndGroup:
   {
      this->group++;
      break;
   }
   case UndoBreak:
   {
      Line_breakAt(line, action->x, action->data.size);
      *y = action->y + 1;
      *x = action->data.size;
      break;
   }
   case UndoJoinNext:
   {
      Line_joinNext(line);
      break;
   }
   case UndoBackwardDeleteChar:
   {
      Line_deleteChars(line, action->x, 1);
      break;
   }
   case UndoDeleteChar:
   {
      Line_deleteChars(line, action->x, 1);
      break;
   }
   case UndoInsertChar:
   {
      Line_insertChar(line, action->x, action->data.ch);
      *x = action->x + 1;
      break;
   }
   case UndoInsertBlock:
   {
      int newX, newY;
      if (Text_chars(action->data.block.text) > 0) {
         Line_insertBlock(line, action->x, action->data.block.text, &newX, &newY);
         *x = action->data.block.coord.xTo;
         *y = action->data.block.coord.yTo;
      }
      break;
   }
   case UndoDeleteBlock:
   {
      int lines = (action->data.block.coord.yTo - action->y) + 1;
      StringBuffer_delete(Line_deleteBlock(line, lines, action->x, action->data.block.coord.xTo));
      break;
   }
   case UndoIndent:
   {
      for (int i = 0; i < action->data.tab.len; i++) {
         for (int j = 0; j < action->data.tab.buf[i]; j++)
            Line_insertChar(line, 0, action->data.tab.size == 0 ? '\t' : ' ');
         line = (Line*) line->super.next;
         assert(line);
      }
      *x = action->x + (action->data.tab.size == 0 ? 1 : action->data.tab.size);
      break;
   }
   case UndoUnindent:
   {
      int* lines = Line_unindent(line, action->data.tab.len, action->data.tab.size);
      free(lines);
      *x = action->x - (action->data.tab.size == 0 ? 1 : action->data.tab.size);
      break;
   }
   }
   Stack_push(this->actions, action);
   action = (UndoAction*) Stack_peek(this->actions);
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
      return Undo_redo(this, x, y);
   return modified;
}

void Undo_store(Undo* this, char* fileName) {
   char* undoFileName = Files_encodePathAsFileName(fileName);
   FILE* fd = fopen(fileName, "r");
   char md5buf[32];
   md5_stream(fd, &md5buf);
   fclose(fd);
   FILE* ufd = Files_openHome("w", "undo/%s", undoFileName);
   free(undoFileName);
   if (!ufd)
      return;

   char* magic = DIT_UNDO_MAGIC;
   fwrite(magic, 5, 1, ufd);

   fwrite(md5buf, 16, 1, ufd);
   if (Undo_checkDiskState(this))
      UndoAction_delete(Stack_pop(this->actions));
   int items = this->actions->size;
   items = MIN(items, 1000);
   fwrite(&items, sizeof(int), 1, ufd);
   int x = -1;
   int y = -1;
   for (int i = items - 1; i >= 0; i--) {
      UndoAction* action = (UndoAction*) Stack_peekAt(this->actions, i);
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
      case UndoBreak:
         fwrite(&action->data.size, sizeof(int), 1, ufd);
         break;
      case UndoJoinNext:
         fwrite(&action->data.backspace, sizeof(bool), 1, ufd);
         break;
      case UndoBackwardDeleteChar:
      case UndoDeleteChar:
      case UndoInsertChar:
         fwrite(&action->data.ch, sizeof(wchar_t), 1, ufd);
         break;
      case UndoInsertBlock:
      case UndoDeleteBlock:
         assert(Text_chars(action->data.block.text) > 0);
         fwrite(&(Text_bytes(action->data.block.text)), sizeof(int), 1, ufd);
         fwrite(Text_toString(action->data.block.text), sizeof(char), Text_bytes(action->data.block.text), ufd);
         fwrite(&action->data.block.coord.xTo, sizeof(int), 1, ufd);
         fwrite(&action->data.block.coord.yTo, sizeof(int), 1, ufd);
         break;
      case UndoIndent:
      case UndoUnindent:
         fwrite(&action->data.tab.len, sizeof(int), 1, ufd);
         fwrite(action->data.tab.buf, sizeof(int), action->data.tab.len, ufd);
         fwrite(&action->data.tab.size, sizeof(bool), 1, ufd);
         break;
      }
   }
   assert(x != -1 && y != -1);
   fclose(ufd);
   Undo_diskState(this, x, y, NULL, fileName);
}

void Undo_restore(Undo* this, char* fileName) {
   FILE* fd = fopen(fileName, "r");
   if (!fd)
      return;
   char* undoFileName = Files_encodePathAsFileName(fileName);
   char md5curr[32], md5saved[32];
   md5_stream(fd, md5curr);
   fclose(fd);
   FILE* ufd = Files_openHome("r", "undo/%s", undoFileName);
   free(undoFileName);
   if (!ufd)
      return;

   char magic[6]; magic[5] = '\0';
   int read = fread(magic, 5, 1, ufd);
   if (read < 1) { fclose(ufd); return; }
   if (strcmp(magic, DIT_UNDO_MAGIC) != 0) { fclose(ufd); return; }

   read = fread(md5saved, 16, 1, ufd);
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
         read = fread(&action->data.ch, sizeof(wchar_t), 1, ufd);
         if (read < 1) { fclose(ufd); return; }
         break;
      case UndoInsertBlock:
      case UndoDeleteBlock:
         {
            int len;
            read = fread(&len, sizeof(int), 1, ufd);
            if (read < 1) { fclose(ufd); return; }
            char* buf = malloc(sizeof(char) * (len + 1));
            read = fread(buf, sizeof(char), len, ufd);
            buf[len] = '\0';
            if (read < len) { fclose(ufd); return; }
            action->data.block.text = Text_newWithSize(buf, len);
            read = fread(&action->data.block.coord.xTo, sizeof(int), 1, ufd);
            if (read < 1) { fclose(ufd); return; }
            read = fread(&action->data.block.coord.yTo, sizeof(int), 1, ufd);
            if (read < 1) { fclose(ufd); return; }
            break;
         }
      case UndoIndent:
      case UndoUnindent:
         read = fread(&action->data.tab.len, sizeof(int), 1, ufd);
         if (read < 1) { fclose(ufd); return; }
         action->data.tab.buf = malloc(sizeof(int) * action->data.tab.len);
         read = fread(action->data.tab.buf, sizeof(int), action->data.tab.len, ufd);
         if (read < action->data.tab.len) { fclose(ufd); return; }
         read = fread(&action->data.tab.size, sizeof(bool), 1, ufd);
         if (read < 1) { fclose(ufd); return; }
         break;
      }
      Stack_push(this->actions, action);
   }

   Undo_diskState(this, x, y, md5curr, fileName);
   assert(Undo_checkDiskState(this));
   fclose(ufd);
}
