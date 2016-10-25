   
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
#include <locale.h>
#include <limits.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>

#include "Prototypes.h"

#include "config.h"
#include "debug.h"

#if __linux__
#define BUSY_WAIT_SLEEP "0.1"
#else
#define BUSY_WAIT_SLEEP "1"
#endif

#define NOT_A_COORD -1

//#link m

static int lines, cols;

static void printVersionFlag() {
   Display_clear();
   printf("dit " VERSION " - (C) 2005-2006 Hisham Muhammad.\n");
   printf("Released under the GNU GPL.\n\n");
   exit(0);
}

static void Dit_refresh(Buffer* buffer, TabManager* tabs) {
   Display_attrset(NormalColor);
   Display_clear();
   Buffer_refreshHighlight(buffer);
   TabManager_refreshCurrent(tabs);
}

static char* saveAs(const char* label, char* defaultName, bool* sudo) {
   Field* field = Field_new(label, 0, lines-1, cols-2);
   if (defaultName) Field_setValue(field, Text_new(defaultName));
   bool quitMask[255] = {0};
   quitMask[19] = true; /* CTRL+S */
   int ch = Field_quickRun(field, quitMask);
   char* name = (ch == 13 || ch == 19)
                ? Field_getValue(field)
                : NULL;
   if (ch == 19) {
      *sudo = true;
   }
   Field_delete(field);
   return name;
}

static bool Dit_save(Buffer* buffer, TabManager* tabs) {
   bool sudo = false;
   if (!buffer->fileName) {
      char* name = saveAs("Save as:", "", &sudo);
      if (!name) {
         return false;
      }
      buffer->fileName = name;
   }
   bool saved = false;
   while (true) {
      char fifoName[1025];
      if (sudo) {
         snprintf(fifoName, 1024, "%s/dit.write.%d.%ld", getenv("TMPDIR"), getpid(), time(NULL));
         unlink(fifoName);
         int err = mkfifo(fifoName, 0600);
         if (!err) {
            int pid = fork();
            if (pid == 0) {
               char command[1025];
               char* shell = getenv("SHELL");
               if (!shell) {
                  shell = "sh";
               }
               int printed = snprintf(command, 1024, "ff=\"\%s\"; "
                                       "clear; "
                                       "sudo \"%s\" -c '"
                                          "touch \"'$ff'.work\"; "
                                          "cat \"'$ff'\" > \"%s\"; "
                                          "touch \"'$ff'.done\"; "
                                          "chown \"'$USER'\" \"'$ff'.done\"; "
                                          "while [ -e \"'$ff'.done\" ]; "
                                             "do sleep %s; "
                                          "done; "
                                          "rm \"'$ff'.work\""
                                       "' || touch \"$ff.fail\"", fifoName, shell, buffer->fileName, BUSY_WAIT_SLEEP);
               Display_clear();
               CRT_done();
               system(command);
               CRT_init();
               exit(0);
            } else if (pid > 0) {
               char doneName[1025];
               char failName[1025];
               char workName[1025];
               bool done = false;
               bool fail = false;
               bool work = false;
               snprintf(doneName, 1024, "%s.done", fifoName);
               snprintf(failName, 1024, "%s.fail", fifoName);
               snprintf(workName, 1024, "%s.work", fifoName);
               do {
                  if (!work) {
                     work = (access(workName, F_OK) == 0);
                     if (work) {
                        FILE* fifoFd = fopen(fifoName, "w");
                        Buffer_saveAndCloseFd(buffer, fifoFd);
                     }
                  }
                  done = (access(doneName, F_OK) == 0);
                  fail = (access(failName, F_OK) == 0);
                  usleep(100000);
               } while (!((work && done) || fail));
               if (done) {
                  saved = true;
               }
               unlink(doneName);
               unlink(failName);
               unlink(fifoName);
               while (work && access(workName, F_OK) == 0) {
                  usleep(100000);
               }
               tabs->redrawBar = true;
               Dit_refresh(buffer, tabs);
            } else {
               saved = false;
            }
         }
      } else {
         saved = Buffer_save(buffer);
      }
      if (saved) {
         char* rpath = realpath(buffer->fileName, NULL);
         if (rpath) {
            free(buffer->fileName);
            buffer->fileName = rpath;
         }
         break;
      }
      char* name = saveAs("Save failed (Ctrl-S to sudo). Save as:", buffer->fileName, &sudo);
      if (!name) {
         break;
      }
      free(buffer->fileName);
      buffer->fileName = name;
   }
   TabManager_refreshCurrent(tabs);
   return saved;
}

void Dit_saveAs(Buffer* buffer, TabManager* tabs) {
   free(buffer->fileName);
   buffer->fileName = NULL;
   Dit_save(buffer, tabs);
}

static bool confirmClose(Buffer* buffer, TabManager* tabs, char* question) {
   char message[512];
   const char* file = buffer->fileName;
   if (file) {
      const char* base = strrchr(file, '/');
      if (base) file = base + 1;
   } else
      file = "Buffer";
   snprintf(message, sizeof(message), "%s was modified. %s", file, question);

   int opt = TabManager_question(tabs, message, "ync");
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

static int xclipOk = 1;

static void copy(Buffer* buffer, bool x11copy) {
   if (!Dit_clipboard)
      Dit_clipboard = Clipboard_new();
   int blockLen;
   char* block = Buffer_copyBlock(buffer, &blockLen);
   if (block) {
      if (x11copy && xclipOk) {
         if (getenv("DISPLAY")) {
            FILE* xclip = popen("xclip -i 2> /dev/null", "w");
            if (xclip) {
               xclipOk = 1;
               fwrite(block, 1, blockLen, xclip);
               pclose(xclip);
            } else {
               xclipOk = 0;
            }
         } else {
            xclipOk = 0;
         }
      }
      Clipboard_set(Dit_clipboard, block, blockLen);
   }
}

static void Dit_cut(Buffer* buffer) {
   copy(buffer, false);
   Buffer_deleteBlock(buffer);
   buffer->selecting = false;
}

static void Dit_copy(Buffer* buffer) {
   copy(buffer, false);
   buffer->selecting = false;
}

static void Dit_x11copy(Buffer* buffer) {
   copy(buffer, true);
   buffer->selecting = false;
}

static void Dit_paste(Buffer* buffer) {
   if (!Dit_clipboard)
      Dit_clipboard = Clipboard_new();
   Text block = Clipboard_get(Dit_clipboard);
   if (Text_isSet(block)) {
      Buffer_pasteBlock(buffer, block);
      Text_prune(&block);
   }
   buffer->selecting = false;
}

static void x11paste(Buffer* buffer) {
   if (!xclipOk) {
      return;
   }
   if (!getenv("DISPLAY")) {
      return;
   }
   FileReader* xclip = FileReader_new("xclip -o 2> /dev/null", true);
   if (!xclip) {
      xclipOk = 0;
      return;
   }
   char* data = FileReader_readAllAndDelete(xclip);
   Text block = Text_new(data);
   Buffer_pasteBlock(buffer, block);
   Text_prune(&block);
   buffer->selecting = false;
}

static void Dit_jumpBack(Buffer* buffer, TabManager* tabs) {
   TabManager_jumpBack(tabs);
}

static Field* Dit_gotoField = NULL;

static void pasteInField(Field* field) {
   if (!Dit_clipboard)
      Dit_clipboard = Clipboard_new();
   Text block = Clipboard_get(Dit_clipboard);
   if (Text_isSet(block)) {
      if (!Text_hasChar(block, '\n'))
         Field_setValue(field, block);
      Text_prune(&block);
   }
}

static void Dit_scrollTo(Buffer* buffer, int x, int y, bool dummy) {
   // animate scrolling only when not via SSH
   if (!getenv("SSH_TTY")) {
      int diff = y - Buffer_y(buffer);
      int absDiff = abs(diff);
      if (diff == 0)
         return;
      int n = floor(absDiff / 500) + 1;
      for (int i = absDiff; i > 0;) {
         int delta = MIN(i, n);
         Buffer_slideLines(buffer, diff < 0 ? -delta : delta);
         i -= delta;
         Buffer_draw(buffer);
         Display_refresh();
         if (i > 1) {
            usleep(50000 / abs(diff));
         }
      }
   }
   Buffer_goto(buffer, x, y, false);
}

static void Dit_goto(Buffer* buffer, TabManager* tabs) {
   TabManager_markJump(tabs);
   if (!Dit_gotoField)
      Dit_gotoField = Field_new("Go to:", 0, lines - 1, MIN(20, cols - 20));

   Field_start(Dit_gotoField);
   int saveX = buffer->x;
   int saveY = buffer->y;
   int lastY = buffer->y + 1;
   bool switchedTab = false;
   for(;;) {
      bool handled;
      bool code;
      int ch = Field_run(Dit_gotoField, false, &handled, &code);
      if (!handled) {
         if ((ch >= '0' && ch <= '9') || (ch == '-' && Dit_gotoField->x == 0)) {
            Field_insertChar(Dit_gotoField, ch);
         } else if (isprint(ch)) {
            Field_insertChar(Dit_gotoField, ch);
         } else if (ch == 27) {
            if (switchedTab) {
               TabManager_jumpBack(tabs);
            }
            Dit_scrollTo(buffer, saveX, saveY, true);
            break;
         }
      }
      char* endptr;
      if (!Dit_gotoField->current->text.data)
         break;
      int y = strtol(Dit_gotoField->current->text.data, &endptr, 10);
      if (endptr && *endptr == '\0') {
         if (y > 0)
            y--;
         if (y != lastY) {
            Dit_scrollTo(buffer, 0, y, true);
            Buffer_draw(buffer);
            lastY = y;
         }
      } else {
         int pages = TabManager_getPageCount(tabs);
         for (int i = 0; i < pages; i++) {
            const char* name = TabManager_getPageName(tabs, i);
               if (name && strcasestr(name, Dit_gotoField->current->text.data)) {
                  switchedTab = true;
                  TabManager_setPage(tabs, i);
                  TabManager_draw(tabs, cols);
                  break;
               }
         }
      }
      if (ch == 13) {
         break;
      }
   }
   TabManager_refreshCurrent(tabs);
}

static Field* Dit_findField = NULL;
static Field* Dit_replaceField = NULL;

static void resizeScreen(TabManager* tabs) {
   Display_getScreenSize(&cols, &lines);
   TabManager_resize(tabs, cols, lines);
   if (Dit_findField) {
      Dit_findField->y = lines - 1;
      Dit_findField->w = cols;
   }
   if (Dit_replaceField) {
      Dit_replaceField->y = lines - 1;
      Dit_replaceField->w = cols;
   }
   Dit_refresh(TabManager_getBuffer(tabs, tabs->currentPage), tabs);
}

typedef struct MouseState_ {
   int fromX;
   int fromY;
} MouseState;

static int handleMouse(MouseState* mstate, TabManager* tabs) {
   MEVENT mevent;
   int ok = getmouse(&mevent);
   if (ok != OK) {
      return ERR;
   }
   Buffer* buf = TabManager_getBuffer(tabs, tabs->currentPage);
   int bx = Buffer_scrollH(buf) + mevent.x;
   int by = Buffer_scrollV(buf) + mevent.y;
   if (mevent.bstate & REPORT_MOUSE_POSITION) {
      if (mstate->fromX != NOT_A_COORD && (mstate->fromX != bx || mstate->fromY != by)) {
         Buffer_setSelection(buf, mstate->fromX, mstate->fromY, bx, by);
      }
      Buffer_goto(buf, bx, by, false);
   } else if (mevent.bstate & BUTTON1_PRESSED) {
      mstate->fromX = bx;
      mstate->fromY = by;
      Buffer_goto(buf, bx, by, false);
      buf->selecting = false;
   } else if (mevent.bstate & BUTTON1_RELEASED) {
      if (mevent.y == LINES - 1) {
         // TODO tab bar
         return ERR;
      }
      if (mevent.x == COLS - 1) {
         // TODO scroll bar
         return ERR;
      }
      if (mstate->fromX != NOT_A_COORD && (mstate->fromX != bx || mstate->fromY != by)) {
         Buffer_setSelection(buf, mstate->fromX, mstate->fromY, bx, by);
         copy(buf, true);
      }
      Buffer_goto(buf, bx, by, false);
      mstate->fromX = NOT_A_COORD;
      return ERR;
   } else if (mevent.bstate & BUTTON2_RELEASED) {
      Buffer_goto(buf, bx, by, false);
      x11paste(buf);
   #if NCURSES_MOUSE_VERSION > 1
   } else if (mevent.bstate & BUTTON4_PRESSED) {
      return KEY_WHEELUP;
   } else if (mevent.bstate & BUTTON5_PRESSED) {
      return KEY_WHEELDOWN;
   #endif
   }
   return ERR;
}

static void moveIfFound(Buffer* buffer, TabManager* tabs, int len, Coords found, Coords* first, bool* stopWrap, bool* failing, bool* searched, bool* wrapped) {
   *stopWrap = false;
   if (!*searched) {
      return;
   }
   if (found.x == NOT_A_COORD) {
      Dit_findField->fieldColor = CRT_colors[FieldFailColor];
      Dit_replaceField->fieldColor = CRT_colors[FieldFailColor];
      return;
   }
   if (first->x == NOT_A_COORD) {
      first->x = found.x;
      first->y = found.y;
      *failing = true;
   } else {
      if (found.y == first->y && found.x == first->x) {
         if (!*wrapped) {
            *wrapped = true;
            Dit_scrollTo(buffer, found.x, found.y, true);
            Buffer_setSelection(buffer, found.x, found.y, found.x + len, found.y);
            Buffer_draw(buffer);
            int answer = TabManager_question(tabs, "Search is back at the beginning. Continue?", "yn");
            if (answer == 1) {
               *stopWrap = true;
               return;
            }
         }
      }
   }
   Dit_findField->fieldColor = CRT_colors[FieldColor];
   Dit_replaceField->fieldColor = CRT_colors[FieldColor];
   Dit_scrollTo(buffer, found.x, found.y, true);
   Buffer_setSelection(buffer, found.x, found.y, found.x + len, found.y);
   Buffer_draw(buffer);
   *failing = false;
}

static void Dit_find(Buffer* buffer, TabManager* tabs) {
   TabManager_markJump(tabs);
   if (!Dit_findField)
      Dit_findField = Field_new("Find:", 0, lines - 1, cols);
   if (!Dit_replaceField)
      Dit_replaceField = Field_new("", 0, lines - 1, cols);
   Field_start(Dit_findField);
   bool quit = false;
   int saveX = buffer->x;
   int saveY = buffer->y;
   bool wrapped = false;
   Coords first = { .x = NOT_A_COORD, .y = NOT_A_COORD };
   bool caseSensitive = false;
   bool wholeWord = false;
   bool failing = true;
   bool stopWrap = false;
   
   bool selectionAutoMatch = false;
   if (buffer->selecting && buffer->selectYfrom == buffer->selectYto) {
      int blockLen;
      char* block = Buffer_copyBlock(buffer, &blockLen);
      Text text = Text_new(block);
      for (int i = 0; i < text.chars; i++) {
         Field_insertChar(Dit_findField, Text_at(text, i));
      }
      free(block);
      failing = false;
      selectionAutoMatch = true;
      first.x = buffer->selectXfrom;
      first.y = buffer->selectYfrom;
      buffer->x = buffer->selectXto;
      buffer->y = buffer->selectYto;
      buffer->selecting = true; // Buffer_copyBlock auto-deselects
   }
   
   Coords notFound = { .x = -1, .y = -1 };
   while (!quit) {
      Field_printfLabel(Dit_findField, "L:%d C:%d [%c%c] %sFind:", buffer->y + 1, buffer->x + 1, caseSensitive ? 'C' : ' ', wholeWord ? 'W' : ' ', wrapped ? "Wrapped " : "");
      bool searched = false;
      Coords found = notFound;
      
      if (selectionAutoMatch) {
         searched = true;
         found.x = buffer->x;
         found.y = buffer->y;
         selectionAutoMatch = false;
      }
      
      bool handled;
      bool code;
      int ch = Field_run(Dit_findField, false, &handled, &code);
     
      int lastY = buffer->y + 1;
      if (!handled) {
         if (!code && (ch >= 32 || ch == 9 || ch == KEY_CTRL('T'))) {
            if (ch == 9) {
               int at = buffer->x;
               if (failing) {
                  Field_clear(Dit_findField);
               }
               if (Field_getLength(Dit_findField) == 0) {
                  while (at > 0) {
                     ch = Line_charAt(buffer->line, at - 1);
                     if (isword(ch)) {
                        at--;
                        buffer->x--;
                     } else
                        break;
                  }
               }
               while (at < Line_chars(buffer->line)) {
                  ch = Line_charAt(buffer->line, at);
                  if (isword(ch)) {
                     Field_insertChar(Dit_findField, ch);
                     at++;
                  } else {
                     break;
                  }
               }
            } else if (ch == KEY_CTRL('T')) {
               ch = 9;
               Field_insertChar(Dit_findField, ch);
            } else {
               Field_insertChar(Dit_findField, ch);
            }
            wrapped = false;
            first.x = NOT_A_COORD;
            first.y = NOT_A_COORD;
            found = Buffer_find(buffer, Field_text(Dit_findField), false, caseSensitive, wholeWord, true);
            searched = true;
         } else {
            switch (ch) {
            case KEY_C_UP:
               Buffer_slideLines(buffer, 1);
               Buffer_draw(buffer);
               break;
            case KEY_C_DOWN:
               Buffer_slideLines(buffer, -1);
               Buffer_draw(buffer);
               break;
            case KEY_CTRL('V'):
               pasteInField(Dit_findField);
               found = Buffer_find(buffer, Field_text(Dit_findField), true, caseSensitive, wholeWord, true);
               searched = true;
               break;
            case KEY_CTRL('I'):
            case KEY_CTRL('C'):
            case KEY_F(5):
            {
               caseSensitive = !caseSensitive;
               found = Buffer_find(buffer, Field_text(Dit_findField), false, caseSensitive, wholeWord, true);
               searched = true;
               break;
            }
            case KEY_CTRL('W'):
            case KEY_F(6):
            {
               wholeWord = !wholeWord;
               found = Buffer_find(buffer, Field_text(Dit_findField), false, caseSensitive, wholeWord, true);
               searched = true;
               break;
            }
            case KEY_F(3):
            case KEY_CTRL('F'):
            case KEY_CTRL('N'):
            {
               if (Text_chars(Field_text(Dit_findField)) == 0) {
                  Field_previousInHistory(Dit_findField);
                  break;
               }
               found = Buffer_find(buffer, Field_text(Dit_findField), true, caseSensitive, wholeWord, true);
               searched = true;
               break;
            }
            case KEY_CTRL('G'):
            {
               const char* text = Text_toString(Field_text(Dit_findField));
               if (!text) break;
               int y = atoi(text);
               if (y > 0)
                  y--;
               if (y != lastY) {
                  Dit_scrollTo(buffer, 0, y, true);
                  Buffer_draw(buffer);
                  lastY = y;
               }
               break;
            }
            case KEY_CTRL('P'):
            {
               found = Buffer_find(buffer, Field_text(Dit_findField), true, caseSensitive, wholeWord, false);
               searched = true;
               break;
            }
            case KEY_CTRL('R'):
            {
               int rch = 0;
               if (failing)
                  continue;
               Dit_replaceField->fieldColor = CRT_colors[FieldColor];
               Field_printfLabel(Dit_replaceField, "L:%d C:%d [%c%c] %sReplace with:", buffer->y + 1, buffer->x + 1, caseSensitive ? 'C' : ' ', wholeWord ? 'W' : ' ', wrapped ? "Wrapped " : "");
               Field_start(Dit_replaceField);
               while (true) {
                  moveIfFound(buffer, tabs, Text_chars(Field_text(Dit_findField)), found, &first, &stopWrap, &failing, &searched, &wrapped);
                  if (stopWrap) {
                     quit = 1;
                     break;
                  }
                  bool quitMask[255] = {0};
                  quitMask[KEY_CTRL('R')] = true;
                  quitMask[KEY_CTRL('C')] = true;
                  quitMask[KEY_CTRL('F')] = true;
                  quitMask[KEY_CTRL('N')] = true;
                  quitMask[KEY_CTRL('P')] = true;
                  quitMask[KEY_CTRL('V')] = true;
                  quitMask[KEY_CTRL('T')] = true;
                  rch = Field_quickRun(Dit_replaceField, quitMask);
                  if (rch == KEY_CTRL('R')) {
                     if (buffer->selecting) {
                        Buffer_pasteBlock(buffer, Dit_replaceField->current->text);
                        buffer->selecting = false;
                        Buffer_draw(buffer);
                        found = Buffer_find(buffer, Field_text(Dit_findField), true, caseSensitive, wholeWord, true);
                        searched = true;
                     }
                  } else if (rch == KEY_CTRL('C')) { // Case-matching replace
                     if (buffer->selecting) {
                        char* newText = strdup(Text_toString(Field_text(Dit_replaceField)));
                        int newLen = strlen(newText);
                        int len = buffer->selectXto - buffer->selectXfrom;
                        // FIXME UTF-8
                        for (int i = 0; i < newLen; i++) {
                           char oldCh = Line_charAt(buffer->line, buffer->selectXfrom + ((i < len) ? i : len-1) );
                           if (isalpha(newText[i])) {
                              if (oldCh == tolower(oldCh)) {
                                 newText[i] = tolower(newText[i]);
                              } else if (oldCh == toupper(oldCh)) {
                                 newText[i] = toupper(newText[i]);
                              }
                           }
                        }
                        Buffer_pasteBlock(buffer, Text_new(newText));
                        free(newText);
                        buffer->selecting = false;
                        Buffer_draw(buffer);
                        found = Buffer_find(buffer, Field_text(Dit_findField), true, caseSensitive, wholeWord, true);
                        searched = true;
                     }
                  } else if (rch == KEY_CTRL('P')) {
                     found = Buffer_find(buffer, Field_text(Dit_findField), true, caseSensitive, wholeWord, false);
                     searched = true;
                  } else if (rch == 13 || rch == 27) {
                     break;
                  } else if (rch == KEY_CTRL('V')) {
                     pasteInField(Dit_replaceField);
                  } else if (rch == KEY_RESIZE) {
                     resizeScreen(tabs);
                     Buffer_draw(buffer);
                  } else {
                     if (rch == KEY_CTRL('T')) {
                        Field_insertChar(Dit_replaceField, 9);
                        continue; 
                     }
                     found = Buffer_find(buffer, Field_text(Dit_findField), true, caseSensitive, wholeWord, true);
                     searched = true;
                  }
               }
               break;
            }
            case 13:
               quit = true;
               break;
            case 27:
               quit = true;
               Dit_scrollTo(buffer, saveX, saveY, true);
               break;
            case KEY_RESIZE:
               resizeScreen(tabs);
               Buffer_draw(buffer);
               break;
            default:
               ;// ignore
            }
         }
      } else {
         if (code && (ch == KEY_UP || ch == KEY_DOWN)) {
            Dit_scrollTo(buffer, saveX, saveY, true);
            wrapped = false;
            first.x = NOT_A_COORD;
            first.y = NOT_A_COORD;
            found = Buffer_find(buffer, Field_text(Dit_findField), true, caseSensitive, wholeWord, true);
            searched = true;
         } else if (code && (ch == KEY_BACKSPACE || ch == KEY_DC)) {
            wrapped = false;
            first.x = NOT_A_COORD;
            first.y = NOT_A_COORD;
            found = Buffer_find(buffer, Field_text(Dit_findField), false, caseSensitive, wholeWord, true);
            searched = true;
         }
      }
      moveIfFound(buffer, tabs, Text_chars(Field_text(Dit_findField)), found, &first, &stopWrap, &failing, &searched, &wrapped);
      if (stopWrap)
         break;
   }
   buffer->selecting = false;
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
   if (buffer && buffer->modified) {
      if (!confirmClose(buffer, tabs, "Save before closing?"))
         return;
   }
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
         TabManager_draw(tabs, cols);
         if (!confirmClose(buffer, tabs, "Save before exit?")) {
            reallyQuit = false;
            break;
         }
      }
   }
   if (reallyQuit)
      *quit = 1;
}

static void Dit_deleteLine(Buffer* buffer) {
   int saveX = buffer->x;
   buffer->selecting = false;
   buffer->x = 0;
   buffer->savedX = 0;
   Buffer_select(buffer, Buffer_downLine);
   Buffer_deleteBlock(buffer);
   Buffer_move(buffer, saveX);
   buffer->savedX = saveX;
}

static void Dit_breakLine(Buffer* buffer) {
   Buffer_breakLine(buffer);
   buffer->selecting = false;
}

static void Dit_previousTabPage(Buffer* buffer, TabManager* tabs, int* ch) {
   TabManager_markJump(tabs);
   TabManager_previousPage(tabs);
   *ch = 0;
}

static void Dit_nextTabPage(Buffer* buffer, TabManager* tabs, int* ch) {
   TabManager_markJump(tabs);
   TabManager_nextPage(tabs);
   *ch = 0;
}

static void Dit_moveTabPageLeft(Buffer* buffer, TabManager* tabs, int* ch) {
   TabManager_moveTabLeft(tabs);
   *ch = 0;
}

static void Dit_moveTabPageRight(Buffer* buffer, TabManager* tabs, int* ch) {
   TabManager_moveTabRight(tabs);
   *ch = 0;
}

static bool Dit_dirExists(const char* name) {
   char* rpath = realpath(name, NULL);
   bool dirExists = true;
   if (rpath) {
      char* dir = dirname(rpath);
      dirExists = (access(dir, F_OK) == 0);
      free(rpath);
   } else {
      char* dirc = strdup(name);
      char* dir = dirname(dirc);
      dirExists = (access(dir, F_OK) == 0);
      free(dirc);
   }
   return dirExists;
}

int Dit_open(TabManager* tabs, const char* name) {
   int page;
   if (name) {
      char* rpath = realpath(name, NULL);
      if (!rpath) {
         if (!Dit_dirExists(name)) {
            CRT_done();
            fprintf(stderr, "dit: directory %s does not exist.\n", name);
            exit(0);
         }
         char* basec = strdup(name);
         char* base = basename(basec);
         char* dirc = strdup(name);
         char* dir = dirname(dirc);
         char* realdir = realpath(dir, NULL);
         asprintf(&rpath, "%s/%s", realdir, base);
         free(realdir);
         free(basec);
         free(dirc);
      }
      page = TabManager_find(tabs, rpath);
      if (page == -1)
         page = TabManager_add(tabs, rpath, NULL);
      free(rpath);
   } else {
      page = TabManager_add(tabs, NULL, NULL);
   }
   return page;
}

static void Dit_scrollUp(Buffer* buffer) {
   Dit_scrollTo(buffer, Buffer_x(buffer), Buffer_y(buffer) - 5, false);
}

static void Dit_scrollDown(Buffer* buffer) {
   Dit_scrollTo(buffer, Buffer_x(buffer), Buffer_y(buffer) + 5, false);
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

static void Dit_selectBeginningOfFile(Buffer* buffer, TabManager* tabs) { TabManager_markJump(tabs); Buffer_select(buffer, Buffer_beginningOfFile); }
static void Dit_selectEndOfFile(Buffer* buffer, TabManager* tabs)       { TabManager_markJump(tabs); Buffer_select(buffer, Buffer_endOfFile);       }

static void Dit_beginningOfFile(Buffer* buffer, TabManager* tabs) { TabManager_markJump(tabs); Buffer_beginningOfFile(buffer); }
static void Dit_endOfFile(Buffer* buffer, TabManager* tabs)       { TabManager_markJump(tabs); Buffer_endOfFile(buffer);       }

typedef bool (*Dit_Action)(Buffer*, TabManager*, int*, int*);

static Hashtable* Dit_actions = NULL;

static void Dit_registerActions() {
   Dit_actions = Hashtable_new(100, Hashtable_STR, Hashtable_BORROW_REFS);
   Hashtable_putString(Dit_actions, "Buffer_backwardChar", (void*)(long) Buffer_backwardChar);
   Hashtable_putString(Dit_actions, "Buffer_backwardDeleteChar", (void*)(long) Buffer_backwardDeleteChar);
   Hashtable_putString(Dit_actions, "Buffer_backwardWord", (void*)(long) Buffer_backwardWord);
   Hashtable_putString(Dit_actions, "Buffer_beginningOfLine", (void*)(long) Buffer_beginningOfLine);
   Hashtable_putString(Dit_actions, "Buffer_deleteChar", (void*)(long) Buffer_deleteChar);
   Hashtable_putString(Dit_actions, "Buffer_downLine", (void*)(long) Buffer_downLine);
   Hashtable_putString(Dit_actions, "Buffer_endOfLine", (void*)(long) Buffer_endOfLine);
   Hashtable_putString(Dit_actions, "Buffer_forwardChar", (void*)(long) Buffer_forwardChar);
   Hashtable_putString(Dit_actions, "Buffer_forwardWord", (void*)(long) Buffer_forwardWord);
   Hashtable_putString(Dit_actions, "Buffer_indent", (void*)(long) Buffer_indent);
   Hashtable_putString(Dit_actions, "Buffer_nextPage", (void*)(long) Buffer_nextPage);
   Hashtable_putString(Dit_actions, "Buffer_previousPage", (void*)(long) Buffer_previousPage);
   Hashtable_putString(Dit_actions, "Buffer_selectAll", (void*)(long) Buffer_selectAll);
   Hashtable_putString(Dit_actions, "Buffer_slideDownLine", (void*)(long) Buffer_slideDownLine);
   Hashtable_putString(Dit_actions, "Buffer_slideUpLine", (void*)(long) Buffer_slideUpLine);
   Hashtable_putString(Dit_actions, "Buffer_toggleMarking", (void*)(long) Buffer_toggleMarking);
   Hashtable_putString(Dit_actions, "Buffer_toggleTabCharacters", (void*)(long) Buffer_toggleTabCharacters);
   Hashtable_putString(Dit_actions, "Buffer_toggleDosLineBreaks", (void*)(long) Buffer_toggleDosLineBreaks);
   Hashtable_putString(Dit_actions, "Buffer_undo", (void*)(long) Buffer_undo);
   Hashtable_putString(Dit_actions, "Buffer_unindent", (void*)(long) Buffer_unindent);
   Hashtable_putString(Dit_actions, "Buffer_upLine", (void*)(long) Buffer_upLine);
   Hashtable_putString(Dit_actions, "Dit_beginningOfFile", (void*)(long) Dit_beginningOfFile);
   Hashtable_putString(Dit_actions, "Dit_breakLine", (void*)(long) Dit_breakLine);
   Hashtable_putString(Dit_actions, "Dit_closeCurrent", (void*)(long) Dit_closeCurrent);
   Hashtable_putString(Dit_actions, "Dit_copy", (void*)(long) Dit_copy);
   Hashtable_putString(Dit_actions, "Dit_cut", (void*)(long) Dit_cut);
   Hashtable_putString(Dit_actions, "Dit_deleteLine", (void*)(long) Dit_deleteLine);
   Hashtable_putString(Dit_actions, "Dit_endOfFile", (void*)(long) Dit_endOfFile);
   Hashtable_putString(Dit_actions, "Dit_find", (void*)(long) Dit_find);
   Hashtable_putString(Dit_actions, "Dit_goto", (void*)(long) Dit_goto);
   Hashtable_putString(Dit_actions, "Dit_jumpBack", (void*)(long) Dit_jumpBack);
   Hashtable_putString(Dit_actions, "Dit_moveTabPageLeft", (void*)(long) Dit_moveTabPageLeft);
   Hashtable_putString(Dit_actions, "Dit_moveTabPageRight", (void*)(long) Dit_moveTabPageRight);
   Hashtable_putString(Dit_actions, "Dit_nextTabPage", (void*)(long) Dit_nextTabPage);
   Hashtable_putString(Dit_actions, "Dit_paste", (void*)(long) Dit_paste);
   Hashtable_putString(Dit_actions, "Dit_previousTabPage", (void*)(long) Dit_previousTabPage);
   Hashtable_putString(Dit_actions, "Dit_quit", (void*)(long) Dit_quit);
   Hashtable_putString(Dit_actions, "Dit_refresh", (void*)(long) Dit_refresh);
   Hashtable_putString(Dit_actions, "Dit_save", (void*)(long) Dit_save);
   Hashtable_putString(Dit_actions, "Dit_scrollUp", (void*)(long) Dit_scrollUp);
   Hashtable_putString(Dit_actions, "Dit_scrollDown", (void*)(long) Dit_scrollDown);
   Hashtable_putString(Dit_actions, "Dit_selectBackwardChar", (void*)(long) Dit_selectBackwardChar);
   Hashtable_putString(Dit_actions, "Dit_selectBackwardWord", (void*)(long) Dit_selectBackwardWord);
   Hashtable_putString(Dit_actions, "Dit_selectBeginningOfLine", (void*)(long) Dit_selectBeginningOfLine);
   Hashtable_putString(Dit_actions, "Dit_selectBeginningOfFile", (void*)(long) Dit_selectBeginningOfFile);
   Hashtable_putString(Dit_actions, "Dit_selectDownLine", (void*)(long) Dit_selectDownLine);
   Hashtable_putString(Dit_actions, "Dit_selectEndOfLine", (void*)(long) Dit_selectEndOfLine);
   Hashtable_putString(Dit_actions, "Dit_selectEndOfFile", (void*)(long) Dit_selectEndOfFile);
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
   keys[KEY_CTRL('B')] = (Dit_Action) Dit_jumpBack;
   keys[KEY_CTRL('C')] = (Dit_Action) Dit_copy;
   /* Ctrl D is FREE */
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
   /* Ctrl P is FREE */
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
   keys[KEY_F(8)]      = (Dit_Action) Dit_deleteLine;
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
   keys[KEY_CS_HOME]   = (Dit_Action) Dit_selectBeginningOfFile;
   keys[KEY_SHOME]     = (Dit_Action) Dit_selectBeginningOfLine;
   keys[KEY_CS_END]    = (Dit_Action) Dit_selectEndOfFile;
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

   keys[KEY_ALT('C')] = (Dit_Action) Dit_x11copy;
   keys[KEY_ALT('J')] = (Dit_Action) Dit_moveTabPageLeft;
   keys[KEY_ALT('K')] = (Dit_Action) Dit_moveTabPageRight;

   keys[KEY_WHEELUP]   = (Dit_Action) Dit_scrollUp;
   keys[KEY_WHEELDOWN] = (Dit_Action) Dit_scrollDown;
}

void Dit_checkFileAccess(char** argv, char* name, int* jump, int* column) {
   if (!name)
      return;
   
   if (!Dit_dirExists(name)) {
      fprintf(stderr, "dit: directory %s does not exist.\n", name);
      exit(0);
   }
   bool exists = (access(name, F_OK) == 0);
   int len = strlen(name);
   if (!exists && len > 2 && name[len - 1] == ':') {
       name[len - 1] = '\0';
       char* line = strrchr(name, ':');
       if (line) {
          *line = '\0';
          line++;
          char* line2 = strrchr(name, ':');
          char* colon2 = line;
          if (line2) {
             *line2 = '\0';
             line2++;
             *column = atoi(line);
             line = line2;
          }
          *jump = atoi(line);
          exists = (access(name, F_OK) == 0);
          if (!exists) {
             name[len - 1] = ':';
             line--;
             *line = ':';
             *colon2 = ':';
             return;
          }
       } else {
          name[len - 1] = ':';
       }
   }
}

static void Dit_parseBindings(Dit_Action* keys) {
   for (int i = 0; i < KEY_MAX; i++)
      keys[i] = 0;
   FILE* fd = Files_open("r", "bindings/default", NULL);
   if (!fd) {
      Display_errorScreen("Warning: could not parse key bindings file bindings/default. Loading hardcoded defaults.");
      Dit_loadHardcodedBindings(keys);
      return;
   }
   while (!feof(fd)) {
      char buffer[256];
      fgets(buffer, 255, fd);
      char** tokens = String_split(buffer, 0);
      char* key = tokens[0]; if (!key) goto nextLine;
      char* action = tokens[1]; if (!action) goto nextLine;
      long int keynum = (long int) Hashtable_getString(CRT_keys, key);
      Dit_Action actionfn = (Dit_Action)(long) Hashtable_getString(Dit_actions, action);
      if (keynum && actionfn)
         keys[keynum] = actionfn;
      nextLine:
      String_freeArray(tokens);
   }
   fclose(fd);
}

int main(int argc, char** argv) {

   setlocale(LC_ALL, "");

   int jump = 0;
   int column = 1;
   int tabSize = 8;
   
   if (!isatty(fileno(stdout))) {
      fprintf(stderr, "Error: trying to run dit through a pipe!\n");
      exit(1);
   }
   
   if (getenv("KONSOLE_DCOP")) setenv("TERM", "konsole", 1);
   
   char* name = argv[1];
   if (argc > 1) {
      if (String_eq(argv[1], "--version")) {
         printVersionFlag();
      } else if (String_eq(argv[1], "-t")) {
         tabSize = atoi(argv[2]);
         name = argv[3];
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
   if (name) {
      stat(name, &st);
      if (S_ISDIR(st.st_mode)) {
         fprintf(stderr, "dit: %s is a directory.\n", name);
         exit(0);
      }
      Dit_checkFileAccess(argv, name, &jump, &column);
   }
   Files_makeHome();

   CRT_init();
   
   Dit_Action keys[KEY_MAX];
   Dit_registerActions();
   Dit_parseBindings(keys);
   Hashtable_delete(Dit_actions);

   Display_getScreenSize(&cols, &lines);
   
   TabManager* tabs = TabManager_new(0, 0, cols, lines, 20);
   if (tabSize > 0) tabs->defaultTabSize = tabSize;

   Dit_open(tabs, name);

   TabManager_load(tabs, "recent", 15);

   Display_bkgdset(NormalColor);
   
   Buffer* buffer = TabManager_draw(tabs, cols);
   if (jump > 0)
      Buffer_goto(buffer, column - 1, jump - 1, true);

   bool code;
   int ch = 0;
   MouseState mstate = {
      .fromX = NOT_A_COORD,
      .fromY = NOT_A_COORD,
   };
   while (!quit) {
      int y, x;

      Display_attrset(CRT_colors[TabColor]);
      Display_mvhline(lines - 1, 0, ' ', tabs->tabOffset);

      Buffer* buffer = TabManager_draw(tabs, cols);
      Display_getyx(&y, &x);

      Display_attrset(CRT_colors[TabColor]);
      Display_printAt(lines - 1, 0, "L:%d C:%d", buffer->y + 1, buffer->x + 1);

      if (!buffer->isUTF8) {
         Display_printAt(lines - 1, 11, "ISO");
      }
      if (buffer->tabulation == 0) {
         Display_printAt(lines - 1, 14, "TAB");
      } else {
         Display_printAt(lines - 1, 14, "%dsp", buffer->tabulation);
      }
      if (buffer->dosLineBreaks) {
         Display_printAt(lines - 1, 17, "DOS");
      }

      Display_attrset(A_NORMAL);
      buffer->lastKey = ch;
      Display_move(y, x);
      
      ch = CRT_getCharacter(&code);
      int limit = 32;
      if (code) {
         limit = KEY_MAX;
         if (buffer->marking) {
            switch (ch) {
            case KEY_C_RIGHT: ch = KEY_CS_RIGHT; break;
            case KEY_C_LEFT:  ch = KEY_CS_LEFT;  break;
            case KEY_C_UP:    ch = KEY_CS_UP;    break;
            case KEY_C_DOWN:  ch = KEY_CS_DOWN;  break;
            case KEY_C_HOME:  ch = KEY_CS_HOME;  break;
            case KEY_C_END:   ch = KEY_CS_END;   break;
            case KEY_C_PPAGE: ch = KEY_CS_PPAGE; break;
            case KEY_C_NPAGE: ch = KEY_CS_NPAGE; break;
            case KEY_RIGHT:   ch = KEY_SRIGHT;   break;
            case KEY_LEFT:    ch = KEY_SLEFT;    break;
            case KEY_UP:      ch = KEY_S_UP;     break;
            case KEY_DOWN:    ch = KEY_S_DOWN;   break;
            case KEY_HOME:    ch = KEY_SHOME;    break;
            case KEY_END:     ch = KEY_SEND;     break;
            case KEY_PPAGE:   ch = KEY_S_PPAGE;  break;
            case KEY_NPAGE:   ch = KEY_S_NPAGE;  break;
            default:          if (keys[ch] != (Dit_Action) Buffer_toggleMarking) buffer->marking = false;
            }
         }

         if (ch == KEY_MOUSE) {
            ch = handleMouse(&mstate, tabs);
         }
   
         switch (ch) {
         case ERR:
            continue;
         case KEY_RESIZE:
            resizeScreen(tabs);
            break;
         }
      }
      
      bool done = Script_onKey(buffer, ch);
      if (!done) {
         if (ch < limit && keys[ch]) {
            (keys[ch])(buffer, tabs, &ch, &quit);
         } else {
            Buffer_defaultKeyHandler(buffer, ch, code);
         }
      }
   }

   Display_attrset(CRT_colors[NormalColor]);
   Display_mvhline(lines-1, 0, ' ', cols);
   Display_refresh();
   
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
