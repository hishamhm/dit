
#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <math.h>
#include <sys/param.h>
#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include <libgen.h>

#include "Prototypes.h"

#include "config.h"
#include "debug.h"


//#link m

static void printVersionFlag() {
   clear();
   printf("dit " VERSION " - (C) 2005-2006 Hisham Muhammad.\n");
   printf("Released under the GNU GPL.\n\n");
   exit(0);
}

static void clearStatusBar() {
   int y, x;
   getyx(stdscr, y, x);
   attrset(CRT_colors[StatusColor]);
   mvhline(LINES - 1, 0, ' ', COLS);
   move(y, x);
}

static void statusMessage(char* message) {
   int cursorX, cursorY;
   getyx(stdscr, cursorY, cursorX);
   attrset(CRT_colors[StatusColor]);
   mvhline(LINES - 1, 0, ' ', COLS);
   mvprintw(LINES - 1, 0, message);
   attrset(CRT_colors[NormalColor]);
   move(cursorY, cursorX);
}

static bool saveAs(Field* saveAsField, Buffer* buffer, char* name) {
   Field_setValue(saveAsField, name);
   bool quitMask[255] = {0};
   int ch = Field_quickRun(saveAsField, quitMask);
   if (ch == 13) {
      free(buffer->fileName);
      buffer->fileName = Field_getValue(saveAsField);
   } else
      return false;
   return true;
}

static bool Dit_save(Buffer* buffer, TabManager* tabs) {
   Field* saveAsField = Field_new("Save failed. Save as:", 0, LINES-1, COLS-2);
   bool saved = false;
   if (!buffer->fileName) {
      Field* saveAsField = Field_new("Save as:", 0, LINES-1, COLS-2);
      if (!saveAs(saveAsField, buffer, ""))
         return false;
      Field_delete(saveAsField);
   }
   while (true) {
      saved = Buffer_save(buffer);
      if (!saved) {
         if (!saveAs(saveAsField, buffer, buffer->fileName))
            break;
      } else
         break;
   }
   Field_delete(saveAsField);
   TabManager_refreshCurrent(tabs);
   return saved;
}

void Dit_saveAs(Buffer* buffer, TabManager* tabs) {
   Field* saveAsField = Field_new("Save as:", 0, LINES-1, COLS-2);
   if (!saveAs(saveAsField, buffer, buffer->fileName ? buffer->fileName : ""))
      return;
   Field_delete(saveAsField);
   Dit_save(buffer, tabs);
}

static bool confirmClose(Buffer* buffer, TabManager* tabs, char* question) {
   int opt = TabManager_question(tabs, question, "ync");
   if (opt == 0)
      if (!Dit_save(buffer, tabs)) {
         return false;
      }
   if (opt == 2) {
      return false;
   }
   return true;
}

static Clipboard* Dit_clipboard = NULL;

static void Dit_cut(Buffer* buffer) {
   if (!Dit_clipboard)
      Dit_clipboard = Clipboard_new();
   int blockLen;
   char* block = Buffer_copyBlock(buffer, &blockLen);
   if (block) {
      Clipboard_set(Dit_clipboard, block, blockLen);
      Buffer_deleteBlock(buffer);
   }
   buffer->selecting = false;
}

static void Dit_copy(Buffer* buffer) {
   if (!Dit_clipboard)
      Dit_clipboard = Clipboard_new();
   int blockLen;
   char* block = Buffer_copyBlock(buffer, &blockLen);
   if (block)
      Clipboard_set(Dit_clipboard, block, blockLen);
   buffer->selecting = false;
}

static void Dit_paste(Buffer* buffer) {
   if (!Dit_clipboard)
      Dit_clipboard = Clipboard_new();
   int blockLen;
   char* block = Clipboard_get(Dit_clipboard, &blockLen);
   if (block) {
      Buffer_pasteBlock(buffer, block, blockLen);
      free(block);
   }
   buffer->selecting = false;
}

static void Dit_pushPull(Buffer* buffer, TabManager* tabs) {
   statusMessage("Pull or push with <- or ->");

   buffer->selecting = false;
   buffer->marking = false;
   int mode = getch();
   switch (mode)
   {
   case KEY_LEFT:
      Buffer_pullText(buffer);
      break;
   case KEY_RIGHT:
      Buffer_pushText(buffer);
      break;
   default:
      break;
   }
   TabManager_refreshCurrent(tabs);
}

static Field* Dit_gotoField = NULL;

static void Dit_goto(Buffer* buffer, TabManager* tabs) {
   clearStatusBar();
   if (!Dit_gotoField)
      Dit_gotoField = Field_new("Go to line:", 0, LINES - 1, MIN(20, COLS - 20));

   Field_start(Dit_gotoField);
   bool quit = false;
   int saveX = buffer->x;
   int saveY = buffer->y;
   int lastY = buffer->y + 1;
   while (!quit) {
      bool handled;
      int ch = Field_run(Dit_gotoField, false, &handled);
      if (!handled) {
         if ((ch >= '0' && ch <= '9') || (ch == '-' && Dit_gotoField->x == 0)) {
            Field_insertChar(Dit_gotoField, ch);
         } else if (ch == 13) {
            quit = true; 
         } else if (ch == 27) {
            quit = true;
            Buffer_goto(buffer, saveX, saveY);
         }
      }
      int y = atoi(Dit_gotoField->current->text);
      if (y > 0)
         y--;
      if (y != lastY) {
         Buffer_goto(buffer, 0, y);
         Buffer_draw(buffer);
         lastY = y;
      }
   }
   TabManager_refreshCurrent(tabs);
}

static Field* Dit_findField = NULL;
static Field* Dit_replaceField = NULL;

static void Dit_find(Buffer* buffer, TabManager* tabs) {
   clearStatusBar();
   if (!Dit_findField)
      Dit_findField = Field_new("Find:", 0, LINES - 1, MIN(100, COLS - 20));
   Field_start(Dit_findField);
   bool quit = false;
   int saveX = buffer->x;
   int saveY = buffer->y;
   bool wrapped = false;
   int firstX = -1;
   int firstY = -1;
   bool caseSensitive = false;
   bool searched = false;
   bool found = false;
   while (!quit) {
      bool handled;
      Field_printfLabel(Dit_findField, "Lin=%d Col=%d (%c)Case %sFind:", buffer->y + 1, buffer->x + 1, caseSensitive ? '*' : ' ', wrapped ? "Wrapped " : "");
      int ch = Field_run(Dit_findField, false, &handled);
      if (!handled) {
         if ((ch >= 32 && ch <= 255) || ch == 9 || ch == CTRL('T')) {
            if (ch == 9) {
               if (buffer->x < buffer->line->len)
                  ch = buffer->line->text[buffer->x];
               else
                  continue;
            } else if (ch == CTRL('T'))
               ch = 9;
            Field_insertChar(Dit_findField, ch);
            wrapped = false;                  
            firstX = -1;
            firstY = -1;
            found = Buffer_find(buffer, Dit_findField->current->text, false, caseSensitive, true);
            searched = true;
         } else {
            switch (ch) {
            case KEY_C_UP:
               Buffer_slideUpLine(buffer);
               Buffer_draw(buffer);
               break;
            case KEY_C_DOWN:
               Buffer_slideDownLine(buffer);
               Buffer_draw(buffer);
               break;
            case KEY_CTRL('V'):
            {
               if (!Dit_clipboard)
                  break;
               int blockLen = 0;
               char* block = Clipboard_get(Dit_clipboard, &blockLen);
               if (block) {
                  Field_setValue(Dit_findField, block);
                  free(block);
               }
               break;
            }
            case KEY_CTRL('I'):
            case KEY_CTRL('C'):
            case KEY_F(2):
            {
               caseSensitive = !caseSensitive;
            }
            case KEY_F(3):
            case KEY_CTRL('F'):
            case KEY_CTRL('N'):
            {
               found = Buffer_find(buffer, Dit_findField->current->text, true, caseSensitive, true);
               searched = true;
               break;
            }
            case KEY_CTRL('P'):
            {
               found = Buffer_find(buffer, Dit_findField->current->text, true, caseSensitive, false);
               searched = true;
               break;
            }
            case KEY_CTRL('R'):
            {
               int rch = 0;
               if (!Dit_replaceField)
                  Dit_replaceField = Field_new("Replace with:", 0, LINES - 1, MIN(100, COLS - 20));
               while (true) {
                  if (searched) {
                     if (found) {
                        if (firstX == -1) {
                           firstX = buffer->x;
                           firstY = buffer->y;
                        } else {
                           if (buffer->y == firstY && buffer->x == firstX)
                              wrapped = true;
                        }
                        Dit_findField->fieldColor = CRT_colors[FieldColor];
                        Buffer_draw(buffer);
                     } else {
                        Dit_findField->fieldColor = CRT_colors[FieldFailColor];
                        break;
                     }
                  }
                  bool quitMask[255] = {0};
                  quitMask[KEY_CTRL('R')] = true;
                  quitMask[KEY_CTRL('F')] = true;
                  quitMask[KEY_CTRL('N')] = true;
                  quitMask[KEY_CTRL('P')] = true;
                  rch = Field_quickRun(Dit_replaceField, quitMask);
                  if (rch == 13 || rch == KEY_CTRL('R')) {
                     if (buffer->selecting) {
                        Buffer_deleteChar(buffer);
                        Buffer_pasteBlock(buffer, Dit_replaceField->current->text, strlen(Dit_replaceField->current->text));
                        buffer->selecting = false;
                        Buffer_draw(buffer);
                        found = Buffer_find(buffer, Dit_findField->current->text, true, caseSensitive, true);
                        searched = true;
                     }
                  } else if (rch == KEY_CTRL('P')) {
                     found = Buffer_find(buffer, Dit_findField->current->text, true, caseSensitive, false);
                     searched = true;
                  } else if (rch == 27) {
                     break;
                  } else {
                     found = Buffer_find(buffer, Dit_findField->current->text, true, caseSensitive, true);
                     searched = true;
                  }
               }
               break;
            }
            default:
               quit = true;
               if (ch == 27)
                  Buffer_goto(buffer, saveX, saveY);
            }
         }
      } else {
         if (ch == KEY_UP || ch == KEY_DOWN) {
            Buffer_goto(buffer, saveX, saveY);
            wrapped = false;
            firstX = -1;
            firstY = -1;
            found = Buffer_find(buffer, Dit_findField->current->text, true, caseSensitive, true);
            searched = true;
         }
      }
      if (searched) {
         if (found) {
            if (firstX == -1) {
               firstX = buffer->x;
               firstY = buffer->y;
            } else {
               if (buffer->y == firstY && buffer->x == firstX)
                  wrapped = true;
            }
            Dit_findField->fieldColor = CRT_colors[FieldColor];
            Buffer_draw(buffer);
         } else
            Dit_findField->fieldColor = CRT_colors[FieldFailColor];
      }
   }
   buffer->selecting = false;
   TabManager_refreshCurrent(tabs);
}

static void Dit_refresh(Buffer* buffer, TabManager* tabs) {
   attrset(NormalColor);
   clear();
   Buffer_refreshHighlight(buffer);
   TabManager_refreshCurrent(tabs);
}

static void Dit_wordWrap(Buffer* buffer) {
   buffer->selecting = false;
   Buffer_wordWrap(buffer, 78);
}

static void Dit_undo(Buffer* buffer, TabManager* tabs) {
   if (Buffer_checkDiskState(buffer)) {
      int answer = TabManager_question(tabs, "Undo past save point?", "yn");
      if (answer == 1)
         return;
   }
   Buffer_undo(buffer);
}

static void Dit_closeCurrent(Buffer* buffer, TabManager* tabs) {
   if (buffer && buffer->modified)
      if (!confirmClose(buffer, tabs, "Buffer was modified. Save before closing?"))
         return;
   if (TabManager_size(tabs) == 1)
      TabManager_add(tabs, NULL, NULL);
   TabManager_removeCurrent(tabs);
}

static void Dit_quit(Buffer* buffer, TabManager* tabs, int* ch, int* quit) {
   bool reallyQuit = true;
   int items = Vector_size(tabs->items);
   for (int i = 0; i < items; i++) {
      TabManager_setPage(tabs, i);
      TabPage* page = (TabPage*) Vector_get(tabs->items, i);
      buffer = page->buffer;
   
      if (buffer && buffer->modified) {
         TabManager_draw(tabs, COLS);
         if (!confirmClose(buffer, tabs, "Buffer was modified. Save before exit?")) {
            reallyQuit = false;
            break;
         }
      }
   }
   if (reallyQuit)
      *quit = 1;
}

static void Dit_breakLine(Buffer* buffer) {
   Buffer_breakLine(buffer);
   buffer->selecting = false;
}

static void Dit_previousTabPage(Buffer* buffer, TabManager* tabs, int* ch) {
   TabManager_previousPage(tabs);
   *ch = 0;
}

static void Dit_nextTabPage(Buffer* buffer, TabManager* tabs, int* ch) {
   TabManager_nextPage(tabs);
   *ch = 0;
}

static void Dit_selectForwardWord(Buffer* buffer)     { Buffer_select(buffer, Buffer_forwardWord);     }
static void Dit_selectForwardChar(Buffer* buffer)     { Buffer_select(buffer, Buffer_forwardChar);     }
static void Dit_selectBackwardWord(Buffer* buffer)    { Buffer_select(buffer, Buffer_backwardWord);    }
static void Dit_selectBackwardChar(Buffer* buffer)    { Buffer_select(buffer, Buffer_backwardChar);    }
static void Dit_selectSlideDownLine(Buffer* buffer)   { Buffer_select(buffer, Buffer_slideDownLine);   }
static void Dit_selectDownLine(Buffer* buffer)        { Buffer_select(buffer, Buffer_downLine);        }
static void Dit_selectSlideUpLine(Buffer* buffer)     { Buffer_select(buffer, Buffer_slideUpLine);     }
static void Dit_selectUpLine(Buffer* buffer)          { Buffer_select(buffer, Buffer_upLine);          }
static void Dit_selectBeginningOfLine(Buffer* buffer) { Buffer_select(buffer, Buffer_beginningOfLine); }
static void Dit_selectEndOfLine(Buffer* buffer)       { Buffer_select(buffer, Buffer_endOfLine);       }
static void Dit_selectPreviousPage(Buffer* buffer)    { Buffer_select(buffer, Buffer_previousPage);    }
static void Dit_selectNextPage(Buffer* buffer)        { Buffer_select(buffer, Buffer_nextPage);        }

typedef bool (*Dit_Action)(Buffer*, TabManager*, int*, int*);

static Hashtable* Dit_actions = NULL;

static void Dit_registerActions() {
   Dit_actions = Hashtable_new(100, Hashtable_STR, Hashtable_BORROW_REFS);
   Hashtable_putString(Dit_actions, "Buffer_backwardChar", (void*)(long) Buffer_backwardChar);
   Hashtable_putString(Dit_actions, "Buffer_backwardDeleteChar", (void*)(long) Buffer_backwardDeleteChar);
   Hashtable_putString(Dit_actions, "Buffer_backwardWord", (void*)(long) Buffer_backwardWord);
   Hashtable_putString(Dit_actions, "Buffer_beginningOfFile", (void*)(long) Buffer_beginningOfFile);
   Hashtable_putString(Dit_actions, "Buffer_beginningOfLine", (void*)(long) Buffer_beginningOfLine );
   Hashtable_putString(Dit_actions, "Buffer_beginningOfLine", (void*)(long) Buffer_beginningOfLine);
   Hashtable_putString(Dit_actions, "Buffer_deleteChar", (void*)(long) Buffer_deleteChar);
   Hashtable_putString(Dit_actions, "Buffer_downLine", (void*)(long) Buffer_downLine);
   Hashtable_putString(Dit_actions, "Buffer_endOfFile", (void*)(long) Buffer_endOfFile);
   Hashtable_putString(Dit_actions, "Buffer_endOfLine", (void*)(long) Buffer_endOfLine);
   Hashtable_putString(Dit_actions, "Buffer_forwardChar", (void*)(long) Buffer_forwardChar);
   Hashtable_putString(Dit_actions, "Buffer_forwardWord", (void*)(long) Buffer_forwardWord);
   Hashtable_putString(Dit_actions, "Buffer_indent", (void*)(long) Buffer_indent);
   Hashtable_putString(Dit_actions, "Buffer_nextPage", (void*)(long) Buffer_nextPage);
   Hashtable_putString(Dit_actions, "Buffer_previousPage", (void*)(long) Buffer_previousPage);
   Hashtable_putString(Dit_actions, "Buffer_slideDownLine", (void*)(long) Buffer_slideDownLine);
   Hashtable_putString(Dit_actions, "Buffer_slideUpLine", (void*)(long) Buffer_slideUpLine);
   Hashtable_putString(Dit_actions, "Buffer_toggleMarking", (void*)(long) Buffer_toggleMarking);
   Hashtable_putString(Dit_actions, "Buffer_toggleTabCharacters", (void*)(long) Buffer_toggleTabCharacters);
   Hashtable_putString(Dit_actions, "Buffer_toggleDosLineBreaks", (void*)(long) Buffer_toggleDosLineBreaks);
   Hashtable_putString(Dit_actions, "Buffer_undo", (void*)(long) Buffer_undo);
   Hashtable_putString(Dit_actions, "Buffer_unindent", (void*)(long) Buffer_unindent);
   Hashtable_putString(Dit_actions, "Buffer_upLine", (void*)(long) Buffer_upLine);
   Hashtable_putString(Dit_actions, "Dit_breakLine", (void*)(long) Dit_breakLine);
   Hashtable_putString(Dit_actions, "Dit_closeCurrent", (void*)(long) Dit_closeCurrent);
   Hashtable_putString(Dit_actions, "Dit_copy", (void*)(long) Dit_copy);
   Hashtable_putString(Dit_actions, "Dit_cut", (void*)(long) Dit_cut);
   Hashtable_putString(Dit_actions, "Dit_find", (void*)(long) Dit_find);
   Hashtable_putString(Dit_actions, "Dit_goto", (void*)(long) Dit_goto);
   Hashtable_putString(Dit_actions, "Dit_nextTabPage", (void*)(long) Dit_nextTabPage);
   Hashtable_putString(Dit_actions, "Dit_paste", (void*)(long) Dit_paste);
   Hashtable_putString(Dit_actions, "Dit_previousTabPage", (void*)(long) Dit_previousTabPage);
   Hashtable_putString(Dit_actions, "Dit_pushPull", (void*)(long) Dit_pushPull);
   Hashtable_putString(Dit_actions, "Dit_quit", (void*)(long) Dit_quit);
   Hashtable_putString(Dit_actions, "Dit_refresh", (void*)(long) Dit_refresh);
   Hashtable_putString(Dit_actions, "Dit_save", (void*)(long) Dit_save);
   Hashtable_putString(Dit_actions, "Dit_selectBackwardChar", (void*)(long) Dit_selectBackwardChar);
   Hashtable_putString(Dit_actions, "Dit_selectBackwardWord", (void*)(long) Dit_selectBackwardWord);
   Hashtable_putString(Dit_actions, "Dit_selectBeginningOfLine", (void*)(long) Dit_selectBeginningOfLine);
   Hashtable_putString(Dit_actions, "Dit_selectDownLine", (void*)(long) Dit_selectDownLine);
   Hashtable_putString(Dit_actions, "Dit_selectEndOfLine", (void*)(long) Dit_selectEndOfLine);
   Hashtable_putString(Dit_actions, "Dit_selectForwardChar", (void*)(long) Dit_selectForwardChar);
   Hashtable_putString(Dit_actions, "Dit_selectForwardWord", (void*)(long) Dit_selectForwardWord);
   Hashtable_putString(Dit_actions, "Dit_selectNextPage", (void*)(long) Dit_selectNextPage);
   Hashtable_putString(Dit_actions, "Dit_selectPreviousPage", (void*)(long) Dit_selectPreviousPage);
   Hashtable_putString(Dit_actions, "Dit_selectSlideDownLine", (void*)(long) Dit_selectSlideDownLine);
   Hashtable_putString(Dit_actions, "Dit_selectSlideUpLine", (void*)(long) Dit_selectSlideUpLine);
   Hashtable_putString(Dit_actions, "Dit_selectUpLine", (void*)(long) Dit_selectUpLine);
   Hashtable_putString(Dit_actions, "Dit_undo", (void*)(long) Dit_undo);
   Hashtable_putString(Dit_actions, "Dit_wordWrap", (void*)(long) Dit_wordWrap);
}

static void Dit_loadHardcodedBindings(Dit_Action* keys) {
   keys[KEY_CTRL('A')] = (Dit_Action) Buffer_beginningOfLine;
   keys[KEY_CTRL('B')] = (Dit_Action) Buffer_toggleMarking;
   keys[KEY_CTRL('C')] = (Dit_Action) Dit_copy;
   keys[KEY_CTRL('D')] = (Dit_Action) Buffer_toggleDosLineBreaks;
   keys[KEY_CTRL('E')] = (Dit_Action) Buffer_endOfLine;
   keys[KEY_CTRL('F')] = (Dit_Action) Dit_find;
   keys[KEY_CTRL('G')] = (Dit_Action) Dit_goto;
   /* Ctrl H is backspace */
   /* Ctrl I is tab */
   keys[KEY_CTRL('J')] = (Dit_Action) Dit_previousTabPage;
   keys[KEY_CTRL('K')] = (Dit_Action) Dit_nextTabPage;
   keys[KEY_CTRL('L')] = (Dit_Action) Dit_refresh;
   /* Ctrl M is enter */
   keys[KEY_CTRL('N')] = (Dit_Action) Dit_wordWrap;
   /* Ctrl O is FREE */
   keys[KEY_CTRL('P')] = (Dit_Action) Dit_pushPull;
   keys[KEY_CTRL('Q')] = (Dit_Action) Dit_quit;
   /* Ctrl R is FREE */
   keys[KEY_CTRL('S')] = (Dit_Action) Dit_save;
   keys[KEY_CTRL('T')] = (Dit_Action) Buffer_toggleTabCharacters;
   keys[KEY_CTRL('U')] = (Dit_Action) Dit_undo;
   keys[KEY_CTRL('V')] = (Dit_Action) Dit_paste;
   keys[KEY_CTRL('W')] = (Dit_Action) Dit_closeCurrent;
   keys[KEY_CTRL('X')] = (Dit_Action) Dit_cut;
   /* Ctrl Y is FREE */
   /* Ctrl Z is FREE */
   keys[KEY_C_INSERT]  = (Dit_Action) Dit_copy;
   keys[KEY_F(3)]      = (Dit_Action) Dit_find;
   keys[KEY_SIC]       = (Dit_Action) Dit_paste;
   keys[KEY_CS_INSERT] = (Dit_Action) Dit_paste;
   keys[KEY_SDC]       = (Dit_Action) Dit_cut;
   keys[KEY_F(10)]     = (Dit_Action) Dit_quit;
   keys[0x0d]          = (Dit_Action) Dit_breakLine;
   keys[KEY_ENTER]     = (Dit_Action) Dit_breakLine;
   keys['\t']          = (Dit_Action) Buffer_indent;
   keys[KEY_BTAB]      = (Dit_Action) Buffer_unindent;
   keys[KEY_CS_RIGHT]  = (Dit_Action) Dit_selectForwardWord;
   keys[KEY_SRIGHT]    = (Dit_Action) Dit_selectForwardChar;
   keys[KEY_CS_LEFT]   = (Dit_Action) Dit_selectBackwardWord;
   keys[KEY_SLEFT]     = (Dit_Action) Dit_selectBackwardChar;
   keys[KEY_CS_DOWN]   = (Dit_Action) Dit_selectSlideDownLine;
   keys[KEY_S_DOWN]    = (Dit_Action) Dit_selectDownLine;
   keys[KEY_CS_UP]     = (Dit_Action) Dit_selectSlideUpLine;
   keys[KEY_S_UP]      = (Dit_Action) Dit_selectUpLine;
   keys[KEY_CS_HOME]   = (Dit_Action) Dit_selectBeginningOfLine;
   keys[KEY_SHOME]     = (Dit_Action) Dit_selectBeginningOfLine;
   keys[KEY_CS_END]    = (Dit_Action) Dit_selectEndOfLine;
   keys[KEY_SEND]      = (Dit_Action) Dit_selectEndOfLine;
   keys[KEY_CS_PPAGE]  = (Dit_Action) Dit_selectPreviousPage;
   keys[KEY_S_PPAGE]   = (Dit_Action) Dit_selectPreviousPage;
   keys[KEY_CS_NPAGE]  = (Dit_Action) Dit_selectNextPage;
   keys[KEY_S_NPAGE]   = (Dit_Action) Dit_selectNextPage;
   keys[KEY_C_RIGHT]   = (Dit_Action) Buffer_forwardWord;
   keys[KEY_RIGHT]     = (Dit_Action) Buffer_forwardChar;
   keys[KEY_C_LEFT]    = (Dit_Action) Buffer_backwardWord;
   keys[KEY_LEFT]      = (Dit_Action) Buffer_backwardChar;
   keys[KEY_C_DOWN]    = (Dit_Action) Buffer_slideDownLine;
   keys[KEY_DOWN]      = (Dit_Action) Buffer_downLine;
   keys[KEY_C_UP]      = (Dit_Action) Buffer_slideUpLine;
   keys[KEY_UP]        = (Dit_Action) Buffer_upLine;
   keys[KEY_HOME]      = (Dit_Action) Buffer_beginningOfLine;
   keys[KEY_END]       = (Dit_Action) Buffer_endOfLine;
   keys[KEY_C_HOME]    = (Dit_Action) Buffer_beginningOfFile;
   keys[KEY_C_END]     = (Dit_Action) Buffer_endOfFile;
   keys[KEY_DC]        = (Dit_Action) Buffer_deleteChar;
   keys['\177']        = (Dit_Action) Buffer_backwardDeleteChar;
   keys[KEY_BACKSPACE] = (Dit_Action) Buffer_backwardDeleteChar;
   keys[KEY_SF]        = (Dit_Action) Dit_selectDownLine;
   keys[KEY_SR]        = (Dit_Action) Dit_selectUpLine;
}

static void Dit_parseBindings(Dit_Action* keys) {
   for (int i = 0; i < KEY_MAX; i++)
      keys[i] = 0;
   FILE* fd = Files_open("r", "bindings/default", NULL);
   if (!fd) {
      clear();
      mvprintw(0,0,"Warning: could not parse key bindings file bindings/default");
      mvprintw(1,0,"Press any key to load hardcoded defaults.");
      getch();
      Dit_loadHardcodedBindings(keys);
      return;
   }
   while (!feof(fd)) {
      char buffer[256];
      fgets(buffer, 255, fd);
      char** tokens = String_split(buffer, 0);
      char* key = tokens[0]; if (!key) goto nextLine;
      char* action = tokens[1]; if (!action) goto nextLine;
      int keynum = (int) Hashtable_getString(CRT_keys, key);
      Dit_Action actionfn = (Dit_Action)(long) Hashtable_getString(Dit_actions, action);
      if (keynum && actionfn)
         keys[keynum] = actionfn;
      nextLine:
      String_freeArray(tokens);
   }
   fclose(fd);
}

int main(int argc, char** argv) {

   int jump = 0;
   
   char* name = argv[1];
   if (argc > 1) {
      if (String_eq(argv[1], "--version")) {
         printVersionFlag();
      } else if (argv[1][0] == '+') {
         argv[1]++;
         jump = atoi(argv[1]);
         argc--;
         name = argv[2];
      }
   }
   if (argc < 2) {
      name = NULL;
   }

   int quit = 0;
   
   struct stat st = {0};
   stat(name, &st);
   if (S_ISDIR(st.st_mode)) {
      fprintf(stderr, "dit: %s is a directory.\n", name);
      exit(0);
   }
   if (name) {
      char dirbuf[1000];
      realpath(name, dirbuf);
      char* dir = dirname(dirbuf);
      bool exists = (access(name, F_OK) == 0);
      bool canWriteDir = (access(dir, W_OK) == 0);
      bool canWrite = (access(name, W_OK) == 0);
      if ((exists && !canWrite) || (!exists && !canWriteDir)) {
         char buffer[4096];
         if (jump)
            snprintf(buffer, 4095, "sudo %s +%d %s", argv[0], jump, name);
         else
            snprintf(buffer, 4095, "sudo %s %s", argv[0], name);
   
         int ret = system(buffer);
         if (ret == 0)
            exit(0);
      }
   }
   Files_makeHome();
   CRT_init();
   Dit_Action keys[KEY_MAX];
   Dit_registerActions();
   Dit_parseBindings(keys);
   Hashtable_delete(Dit_actions);

   Script_init();
   
   TabManager* tabs = TabManager_new(0, 0, COLS, LINES, 20);

   if (name) {
      char rpath[4097];
      realpath(name, rpath);
      rpath[4096] = '\0';
      TabManager_add(tabs, rpath, NULL);
   } else {
      TabManager_add(tabs, NULL, NULL);
   }

   TabManager_load(tabs, "recent", 10);

   bkgdset(NormalColor);
   
   Buffer* buffer = TabManager_draw(tabs, COLS);
   if (jump > 0)
      Buffer_goto(buffer, 0, jump - 1);

   int ch = 0;
   while (!quit) {
      int y, x;

      attrset(CRT_colors[TabColor]);
      mvhline(LINES - 1, 0, ' ', tabs->tabOffset);

      Buffer* buffer = TabManager_draw(tabs, COLS);
      Script_setCurrentBuffer(buffer);
      getyx(stdscr, y, x);

      attrset(CRT_colors[TabColor]);
      mvprintw(LINES - 1, 0, "L:%d C:%d %s%s",
                             buffer->y + 1, buffer->x + 1,
                             (buffer->tabCharacters ? " TABS" : ""),
                             (buffer->dosLineBreaks ? " DOS" : ""));

      attrset(A_NORMAL);
      buffer->lastKey = ch;
      move(y, x);
      ch = CRT_getCharacter();
      
      //TODO: mouse

      if (buffer->marking) {
         switch (ch) {
         case KEY_C_RIGHT: ch = KEY_CS_RIGHT; break;
         case KEY_C_LEFT:  ch = KEY_CS_LEFT;  break;
         case KEY_C_UP:    ch = KEY_CS_UP;    break;
         case KEY_C_DOWN:  ch = KEY_CS_DOWN;  break;
         case KEY_C_HOME:  ch = KEY_CS_HOME;  break;
         case KEY_C_END:   ch = KEY_CS_END;   break;
         case KEY_RIGHT:   ch = KEY_SRIGHT;   break;
         case KEY_LEFT:    ch = KEY_SLEFT;    break;
         case KEY_UP:      ch = KEY_S_UP;     break;
         case KEY_DOWN:    ch = KEY_S_DOWN;   break;
         case KEY_HOME:    ch = KEY_SHOME;    break;
         case KEY_END:     ch = KEY_SEND;     break;
         default:          if (keys[ch] != (Dit_Action) Buffer_toggleMarking) buffer->marking = false;
         }
      }

      switch (ch) {
      case ERR:
         continue;
      case KEY_RESIZE:
         TabManager_resize(tabs, COLS, LINES);
         if (Dit_findField)
            Dit_findField->y = LINES - 1;
         break;
      }
      
      if (keys[ch])
         (keys[ch])(buffer,tabs, &ch, &quit);
      else
         Buffer_defaultKeyHandler(buffer, ch);
   }

   attron(CRT_colors[NormalColor]);
   mvhline(LINES-1, 0, ' ', COLS);
   attroff(CRT_colors[NormalColor]);
   refresh();
   
   CRT_done();
   if (Dit_clipboard)
      Clipboard_delete(Dit_clipboard);
   if (Dit_gotoField)
      Field_delete(Dit_gotoField);
   if (Dit_findField)
      Field_delete(Dit_findField);
   if (Dit_replaceField)
      Field_delete(Dit_replaceField);
   TabManager_save(tabs, "recent");
   TabManager_delete(tabs);
   debug_done();
   return 0;
}

/** This is a very long line. 1 This is a very long line. 2 This is a very long line. 3 This is a very long line. 4 This is a very long line. 5 This is a very long line. */
