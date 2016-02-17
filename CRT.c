
#define _GNU_SOURCE
#include <signal.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#ifdef __linux
#include <sys/ioctl.h>
#endif
#ifdef __CYGWIN__
#include <sys/ioctl.h>
#include <termios.h>
#endif

#include "Prototypes.h"

/*{

#define CRT_color(a,b) COLOR_PAIR( (a==7&&b==0)?0:((a==0&&b==0)?7*8:a*8+b) )

#define Black   COLOR_BLACK
#define Red     COLOR_RED
#define Green   COLOR_GREEN
#define Yellow  COLOR_YELLOW
#define Blue    COLOR_BLUE
#define Magenta COLOR_MAGENTA
#define Cyan    COLOR_CYAN
#define White   COLOR_WHITE


typedef enum {
   NormalColor = 0,
   TabColor,
   CurrentTabColor,
   CurrentTabROColor,
   CurrentTabShadeColor,
   SelectionColor,
   UnfocusedSelectionColor,
   BracketColor,
   BrightColor,
   SymbolColor,
   BrightSymbolColor,
   AltColor,
   BrightAltColor,
   DiffColor,
   BrightDiffColor,
   SpecialColor,
   BrightSpecialColor,
   SpecialDiffColor,
   BrightSpecialDiffColor,
   VerySpecialColor,
   DimColor,
   ScrollBarColor,
   ScrollHandleColor,
   HeaderColor,
   StatusColor,
   KeyColor,
   FieldColor,
   FieldFailColor,
   AlertColor,
   Colors
} Color;

#define KEY_S_UP      KEY_F(30)
#define KEY_S_DOWN    KEY_F(31)
//#define KEY_S_RIGHT   KEY_F(32)
//#define KEY_S_LEFT    KEY_F(33)
//#define KEY_S_HOME    KEY_F(34)
//#define KEY_S_END     KEY_F(35)
//#define KEY_S_INSERT  KEY_F(36)
//#define KEY_S_DELETE  KEY_F(37)
#define KEY_S_NPAGE   KEY_F(38)
#define KEY_S_PPAGE   KEY_F(39)
#define KEY_C_UP      KEY_F(40)
#define KEY_C_DOWN    KEY_F(41)
#define KEY_C_RIGHT   KEY_F(42)
#define KEY_C_LEFT    KEY_F(43)
#define KEY_C_HOME    KEY_F(44)
#define KEY_C_END     KEY_F(45)
#define KEY_C_INSERT  KEY_F(46)
#define KEY_C_DELETE  KEY_F(47)
#define KEY_CS_UP     KEY_F(48)
#define KEY_CS_DOWN   KEY_F(49)
#define KEY_CS_RIGHT  KEY_F(50)
#define KEY_CS_LEFT   KEY_F(51)
#define KEY_CS_HOME   KEY_F(52)
#define KEY_CS_END    KEY_F(53)
#define KEY_CS_INSERT KEY_F(54)
#define KEY_CS_DELETE KEY_F(55)
#define KEY_CS_PPAGE  KEY_F(56)
#define KEY_CS_NPAGE  KEY_F(57)
#define KEY_C_PPAGE   KEY_F(58)
#define KEY_C_NPAGE   KEY_F(59)
#define KEY_ALT(x)    KEY_F(60) + (x - 'A')

#define KEY_CTRL(x)  (x - 'A' + 1)

#define SHIFT_MASK 1
#define ALTR_MASK 2
#define CTRL_MASK 4
#define ALTL_MASK 8

extern int CRT_delay;

extern char CRT_scrollHandle;

extern char CRT_scrollBar;

extern int CRT_colors[Colors];

extern Hashtable* CRT_keys;

int putenv(char*);

}*/

bool CRT_linuxConsole = false;

bool CRT_hasColors;

int CRT_delay;

char CRT_scrollHandle;

char CRT_scrollBar;

int CRT_colors[Colors];

Hashtable* CRT_keys;

bool CRT_parseTerminalFile(char* term) {

   FILE* fd = Files_open("r", "terminals/%s", term);
   if (!fd) {
      Display_errorScreen("Warning: could not open terminal rules file terminals/%s", term);
      return false;
   }
   while (!feof(fd)) {
      char buffer[256];
      char* ok = fgets(buffer, 255, fd);
      if (!ok) break;
      char** tokens = String_split(buffer, 0);
      char* sequence = tokens[0]; if (!sequence) goto nextLine;
      char* key = tokens[1]; if (!key) goto nextLine;
      String_convertEscape(sequence, "\\033", 033);
      String_convertEscape(sequence, "\\177", 0177);
      String_convertEscape(sequence, "^[", 033);
      long int keynum = (long int) Hashtable_getString(CRT_keys, key);
      if (keynum)
         Display_defineKey(sequence, keynum);
      nextLine:
      String_freeArray(tokens);
   }
   fclose(fd);
   return true;
}

void CRT_init() {
   
   char* term = getenv("TERM");

   if (strcmp(term, "xterm") == 0) {
      putenv("TERM=xterm-color");
      term = "xterm-color";
   } else if (strcmp(term, "linux") == 0) {
      CRT_linuxConsole = true;
   }

   CRT_delay = 0;
   
   CRT_hasColors = Display_init(term);
   if (CRT_hasColors) {
      CRT_scrollHandle = ' ';
      CRT_scrollBar = ' ';
   } else {
      CRT_scrollHandle = '*';
      CRT_scrollBar = '|';
   }

   #define ANTARCTIC_THEME

   #ifdef ANTARCTIC_THEME
   CRT_colors[NormalColor] = A_NORMAL;
   CRT_colors[TabColor] = CRT_color(Black, Cyan);
   CRT_colors[CurrentTabShadeColor] = A_BOLD | CRT_color(Black, Blue);
   CRT_colors[CurrentTabColor] = A_BOLD | CRT_color(Yellow, Blue);
   CRT_colors[CurrentTabROColor] = A_BOLD | CRT_color(Red, Blue);
   CRT_colors[SelectionColor] = A_REVERSE | CRT_color(Blue, White);
   CRT_colors[UnfocusedSelectionColor] = A_REVERSE | CRT_color(White, Blue);
   CRT_colors[BracketColor] = A_REVERSE | CRT_color(Cyan, Black);
   CRT_colors[BrightColor] = A_BOLD | CRT_color(White, Black);
   CRT_colors[SymbolColor] = A_BOLD | CRT_color(White, Black);
   CRT_colors[BrightSymbolColor] = A_BOLD | CRT_color(White, Black);
   CRT_colors[AltColor] = CRT_color(Cyan, Black);
   CRT_colors[BrightAltColor] = A_BOLD | CRT_color(Cyan, Black);
   CRT_colors[DiffColor] = CRT_color(Green, Black);
   CRT_colors[BrightDiffColor] = A_BOLD | CRT_color(Green, Black);
   CRT_colors[SpecialColor] = CRT_color(Yellow, Black);
   CRT_colors[BrightSpecialColor] = A_BOLD | CRT_color(Yellow, Black);
   CRT_colors[SpecialDiffColor] = CRT_color(Red, Black);
   CRT_colors[BrightSpecialDiffColor] = A_BOLD | CRT_color(Red, Black);
   CRT_colors[VerySpecialColor] = A_BOLD | CRT_color(Yellow, Red);
   CRT_colors[DimColor] = CRT_color(Yellow, Black);
   CRT_colors[ScrollBarColor] = CRT_color(White, Blue);
   CRT_colors[ScrollHandleColor] = CRT_color(White, Cyan);
   CRT_colors[StatusColor] = CRT_color(Black, Cyan);
   CRT_colors[KeyColor] = A_REVERSE | CRT_color(Black, White);
   CRT_colors[FieldColor] = CRT_color(White, Blue);
   CRT_colors[FieldFailColor] = CRT_color(Red, Blue);
   CRT_colors[AlertColor] = CRT_color(White, Red);
   #endif
   #ifdef VIM_THEME
   CRT_colors[NormalColor] = A_NORMAL;
   CRT_colors[TabColor] = A_BOLD | A_REVERSE | CRT_color(White, Black);
   CRT_colors[CurrentTabColor] = A_BOLD | CRT_color(White, Black);
   CRT_colors[CurrentTabROColor] = A_BOLD | CRT_color(Red, Black);
   CRT_colors[CurrentTabShadeColor] = A_BOLD | CRT_color(Black, White);
   CRT_colors[SelectionColor] = A_REVERSE | CRT_color(White, Black);
   CRT_colors[UnfocusedSelectionColor] = A_BOLD | CRT_color(White, Black);
   CRT_colors[BracketColor] = A_NORMAL;
   CRT_colors[BrightColor] = A_BOLD | CRT_color(Yellow, Black);
   CRT_colors[SymbolColor] = A_NORMAL;
   CRT_colors[BrightSymbolColor] = CRT_color(Yellow, Black);
   CRT_colors[AltColor] = A_BOLD | CRT_color(Magenta, Black);
   CRT_colors[BrightAltColor] = A_BOLD | CRT_color(Red, Black);
   CRT_colors[VerySpecialColor] = A_REVERSE | CRT_color(Yellow, Black);
   CRT_colors[DimColor] = A_BOLD | CRT_color(Cyan, Black);
   CRT_colors[SpecialColor] = A_BOLD | CRT_color(Magenta, Black);
   CRT_colors[BrightSpecialColor] = A_BOLD | CRT_color(Blue, Black); //
   CRT_colors[SpecialDiffColor] = CRT_color(Red, Black);
   CRT_colors[BrightSpecialDiffColor] = A_BOLD | CRT_color(Red, Black);
   CRT_colors[DiffColor] = A_BOLD | CRT_color(Green, Black);
   CRT_colors[BrightDiffColor] = CRT_color(Magenta, Black);
   CRT_colors[ScrollBarColor] = CRT_color(White, Black);
   CRT_colors[ScrollHandleColor] = CRT_color(White, Green);
   CRT_colors[StatusColor] = CRT_color(White, Black);
   CRT_colors[KeyColor] = CRT_color(Black, White);
   CRT_colors[FieldColor] = CRT_color(White, Black);
   CRT_colors[FieldFailColor] = CRT_color(Red, Black);
   CRT_colors[AlertColor] = CRT_color(Red, Black);
   #endif
   #ifdef EMACS_THEME
   CRT_colors[NormalColor] = A_NORMAL;
   CRT_colors[TabColor] = CRT_color(Black, White);
   CRT_colors[CurrentTabColor] = A_REVERSE | CRT_color(Black, White);
   CRT_colors[CurrentTabROColor] = A_BOLD | CRT_color(Red, White);
   CRT_colors[CurrentTabShadeColor] = A_BOLD | CRT_color(Black, White);
   CRT_colors[SelectionColor] = A_REVERSE | CRT_color(White, Black);
   CRT_colors[UnfocusedSelectionColor] = A_BOLD | CRT_color(White, Black);
   CRT_colors[BracketColor] = A_NORMAL;
   CRT_colors[BrightColor] = A_BOLD | CRT_color(Cyan, Black);
   CRT_colors[SymbolColor] = A_NORMAL;
   CRT_colors[BrightSymbolColor] = CRT_color(White, Black);
   CRT_colors[AltColor] = CRT_color(Green, Black);
   CRT_colors[BrightAltColor] = CRT_color(Green, Black);
   CRT_colors[VerySpecialColor] = CRT_color(Red, Black);
   CRT_colors[DimColor] = CRT_color(Red, Black);
   CRT_colors[SpecialColor] = CRT_color(Green, Black);
   CRT_colors[BrightSpecialColor] = CRT_color(Blue, Black);
   CRT_colors[SpecialDiffColor] = CRT_color(Red, Black);
   CRT_colors[BrightSpecialDiffColor] = A_BOLD | CRT_color(Red, Black);
   CRT_colors[DiffColor] = CRT_color(Green, Black);
   CRT_colors[BrightDiffColor] = CRT_color(Magenta, Black);
   CRT_colors[ScrollBarColor] = CRT_color(White, Black);
   CRT_colors[ScrollHandleColor] = CRT_color(Black, White);
   CRT_colors[StatusColor] = CRT_color(Black, White);
   CRT_colors[KeyColor] = A_REVERSE | CRT_color(Black, White);
   CRT_colors[FieldColor] = CRT_color(White, Black);
   CRT_colors[FieldFailColor] = CRT_color(Red, Black);
   CRT_colors[AlertColor] = CRT_color(White, Red);
   #endif
   #ifdef CLASSIC_TURBO_THEME
   CRT_colors[NormalColor] = (CRT_color(White, Blue));
   CRT_colors[TabColor] = CRT_color(Black, Cyan);
   CRT_colors[CurrentTabColor] = A_REVERSE | CRT_color(Green, Black);
   CRT_colors[CurrentTabROColor] = A_BOLD | CRT_color(Red, Green);
   CRT_colors[CurrentTabShadeColor] = A_BOLD | CRT_color(Black, Green);
   CRT_colors[SelectionColor] = (A_REVERSE | CRT_color(Cyan, Black));
   CRT_colors[UnfocusedSelectionColor] = A_REVERSE | CRT_color(White, Blue);
   CRT_colors[BracketColor] = (A_REVERSE | CRT_color(Green, Black));
   CRT_colors[BrightColor] = (A_BOLD | CRT_color(Yellow, Blue));
   CRT_colors[SymbolColor] = (A_BOLD | CRT_color(Cyan, Blue));
   CRT_colors[BrightSymbolColor] = (A_BOLD | CRT_color(Yellow, Blue));
   CRT_colors[AltColor] = (CRT_color(Green, Blue));
   CRT_colors[BrightAltColor] = (A_BOLD | CRT_color(Green, Blue));
   CRT_colors[DiffColor] = (CRT_color(Cyan, Blue));
   CRT_colors[BrightDiffColor] = (A_BOLD | CRT_color(Cyan, Blue));
   CRT_colors[SpecialColor] = (CRT_color(Red, Blue));
   CRT_colors[BrightSpecialColor] = (A_BOLD | CRT_color(Red, Blue));
   CRT_colors[SpecialDiffColor] = (CRT_color(Magenta, Black));
   CRT_colors[BrightSpecialDiffColor] = (A_BOLD | CRT_color(Magenta, Black));
   CRT_colors[VerySpecialColor] = (A_BOLD | CRT_color(Yellow, Red));
   CRT_colors[DimColor] = (CRT_color(Yellow, Blue));
   CRT_colors[ScrollBarColor] = CRT_color(White, Black);
   CRT_colors[ScrollHandleColor] = CRT_color(White, Green);
   CRT_colors[StatusColor] = CRT_color(Black, Cyan);
   CRT_colors[KeyColor] = A_REVERSE | CRT_color(Black, White);
   CRT_colors[FieldColor] = CRT_color(White, Blue);
   CRT_colors[FieldFailColor] = CRT_color(Red, Blue);
   CRT_colors[AlertColor] = CRT_color(White, Red);
   #endif
   #ifdef BLACK_TURBO_THEME
   CRT_colors[NormalColor] = (CRT_color(White, Black));
   CRT_colors[TabColor] = CRT_color(Black, Cyan);
   CRT_colors[CurrentTabColor] = A_REVERSE | CRT_color(Green, Black);
   CRT_colors[CurrentTabROColor] = A_BOLD | CRT_color(Red, Green);
   CRT_colors[CurrentTabShadeColor] = A_BOLD | CRT_color(Black, Green);
   CRT_colors[SelectionColor] = (A_REVERSE | CRT_color(Cyan, Black));
   CRT_colors[UnfocusedSelectionColor] = A_REVERSE | CRT_color(White, Blue);
   CRT_colors[BracketColor] = (A_REVERSE | CRT_color(Green, Black));
   CRT_colors[BrightColor] = (A_BOLD | CRT_color(Yellow, Black));
   CRT_colors[SymbolColor] = (A_BOLD | CRT_color(Cyan, Black));
   CRT_colors[BrightSymbolColor] = (A_BOLD | CRT_color(Yellow, Black));
   CRT_colors[AltColor] = (CRT_color(Green, Black));
   CRT_colors[BrightAltColor] = (A_BOLD | CRT_color(Green, Black));
   CRT_colors[DiffColor] = (CRT_color(Cyan, Black));
   CRT_colors[BrightDiffColor] = (A_BOLD | CRT_color(Cyan, Black));
   CRT_colors[SpecialColor] = (CRT_color(Red, Black));
   CRT_colors[BrightSpecialColor] = (A_BOLD | CRT_color(Red, Black));
   CRT_colors[SpecialDiffColor] = (CRT_color(Magenta, Black));
   CRT_colors[BrightSpecialDiffColor] = (A_BOLD | CRT_color(Magenta, Black));
   CRT_colors[VerySpecialColor] = (A_BOLD | CRT_color(Yellow, Red));
   CRT_colors[DimColor] = (CRT_color(Yellow, Black));
   CRT_colors[ScrollBarColor] = CRT_color(White, Blue);
   CRT_colors[ScrollHandleColor] = CRT_color(White, Cyan);
   CRT_colors[StatusColor] = CRT_color(Black, Cyan);
   CRT_colors[KeyColor] = A_REVERSE | CRT_color(Black, White);
   CRT_colors[FieldColor] = CRT_color(White, Blue);
   CRT_colors[FieldFailColor] = CRT_color(Red, Blue);
   CRT_colors[AlertColor] = CRT_color(White, Red);
   #endif

   CRT_keys = Hashtable_new(200, Hashtable_STR, Hashtable_BORROW_REFS);
   for (int k = 'A'; k <= 'Z'; k++) {
      char key[8];
      snprintf(key, 7, "CTRL_%c", k);
      Hashtable_putString(CRT_keys, key, (void*) (long int) KEY_CTRL(k));
      snprintf(key, 7, "ALT_%c", k);
      Hashtable_putString(CRT_keys, key, (void*) (long int) KEY_ALT(k));
   }
   Hashtable_putString(CRT_keys, "ESC", (void*) 27);
   Hashtable_putString(CRT_keys, "A1", (void*) KEY_A1);
   Hashtable_putString(CRT_keys, "A3", (void*) KEY_A3);
   Hashtable_putString(CRT_keys, "B2", (void*) KEY_B2);
   Hashtable_putString(CRT_keys, "C1", (void*) KEY_C1);
   Hashtable_putString(CRT_keys, "C3", (void*) KEY_C3);
   Hashtable_putString(CRT_keys, "BACKSPACE", (void*) KEY_BACKSPACE);
   Hashtable_putString(CRT_keys, "BEGIN", (void*) KEY_BEG);
   Hashtable_putString(CRT_keys, "BACK_TAB", (void*) KEY_BTAB);
   Hashtable_putString(CRT_keys, "CANCEL", (void*) KEY_CANCEL);
   Hashtable_putString(CRT_keys, "CLEAR_ALL_TABS", (void*) KEY_CATAB);
   Hashtable_putString(CRT_keys, "CLEAR", (void*) KEY_CLEAR);
   Hashtable_putString(CRT_keys, "CLOSE", (void*) KEY_CLOSE);
   Hashtable_putString(CRT_keys, "COMMAND", (void*) KEY_COMMAND);
   Hashtable_putString(CRT_keys, "COPY", (void*) KEY_COPY);
   Hashtable_putString(CRT_keys, "CREATE", (void*) KEY_CREATE);
   Hashtable_putString(CRT_keys, "CLEAR_TAB", (void*) KEY_CTAB);
   Hashtable_putString(CRT_keys, "CTRL_DELETE", (void*) KEY_C_DELETE);
   Hashtable_putString(CRT_keys, "CTRL_DOWN", (void*) KEY_C_DOWN);
   Hashtable_putString(CRT_keys, "CTRL_END", (void*) KEY_C_END);
   Hashtable_putString(CRT_keys, "CTRL_HOME", (void*) KEY_C_HOME);
   Hashtable_putString(CRT_keys, "CTRL_INSERT", (void*) KEY_C_INSERT);
   Hashtable_putString(CRT_keys, "CTRL_LEFT", (void*) KEY_C_LEFT);
   Hashtable_putString(CRT_keys, "CTRL_RIGHT", (void*) KEY_C_RIGHT);
   Hashtable_putString(CRT_keys, "CTRL_PPAGE", (void*) KEY_C_PPAGE);
   Hashtable_putString(CRT_keys, "CTRL_NPAGE", (void*) KEY_C_NPAGE);
   Hashtable_putString(CRT_keys, "CTRL_SHIFT_DELETE", (void*) KEY_CS_DELETE);
   Hashtable_putString(CRT_keys, "CTRL_SHIFT_DOWN", (void*) KEY_CS_DOWN);
   Hashtable_putString(CRT_keys, "CTRL_SHIFT_END", (void*) KEY_CS_END);
   Hashtable_putString(CRT_keys, "CTRL_SHIFT_HOME", (void*) KEY_CS_HOME);
   Hashtable_putString(CRT_keys, "CTRL_SHIFT_INSERT", (void*) KEY_CS_INSERT);
   Hashtable_putString(CRT_keys, "CTRL_SHIFT_LEFT", (void*) KEY_CS_LEFT);
   Hashtable_putString(CRT_keys, "CTRL_SHIFT_NPAGE", (void*) KEY_CS_NPAGE);
   Hashtable_putString(CRT_keys, "CTRL_SHIFT_PPAGE", (void*) KEY_CS_PPAGE);
   Hashtable_putString(CRT_keys, "CTRL_SHIFT_RIGHT", (void*) KEY_CS_RIGHT);
   Hashtable_putString(CRT_keys, "CTRL_SHIFT_UP", (void*) KEY_CS_UP);
   Hashtable_putString(CRT_keys, "CTRL_UP", (void*) KEY_C_UP);
   Hashtable_putString(CRT_keys, "DELETE", (void*) KEY_DC);
   Hashtable_putString(CRT_keys, "DELETE_LINE", (void*) KEY_DL);
   Hashtable_putString(CRT_keys, "DOWN", (void*) KEY_DOWN);
   Hashtable_putString(CRT_keys, "EIC", (void*) KEY_EIC);
   Hashtable_putString(CRT_keys, "END", (void*) KEY_END);
   Hashtable_putString(CRT_keys, "ENTER", (void*) KEY_ENTER);
   Hashtable_putString(CRT_keys, "EOL", (void*) KEY_EOL);
   Hashtable_putString(CRT_keys, "EOS", (void*) KEY_EOS);
   Hashtable_putString(CRT_keys, "EXIT", (void*) KEY_EXIT);
   Hashtable_putString(CRT_keys, "F0", (void*) KEY_F0);
   Hashtable_putString(CRT_keys, "F1", (void*) KEY_F(1));
   Hashtable_putString(CRT_keys, "F2", (void*) KEY_F(2));
   Hashtable_putString(CRT_keys, "F3", (void*) KEY_F(3));
   Hashtable_putString(CRT_keys, "F4", (void*) KEY_F(4));
   Hashtable_putString(CRT_keys, "F5", (void*) KEY_F(5));
   Hashtable_putString(CRT_keys, "F6", (void*) KEY_F(6));
   Hashtable_putString(CRT_keys, "F7", (void*) KEY_F(7));
   Hashtable_putString(CRT_keys, "F8", (void*) KEY_F(8));
   Hashtable_putString(CRT_keys, "F9", (void*) KEY_F(9));
   Hashtable_putString(CRT_keys, "F10", (void*) KEY_F(10));
   Hashtable_putString(CRT_keys, "F11", (void*) KEY_F(11));
   Hashtable_putString(CRT_keys, "F12", (void*) KEY_F(12));
   Hashtable_putString(CRT_keys, "FIND", (void*) KEY_FIND);
   Hashtable_putString(CRT_keys, "HELP", (void*) KEY_HELP);
   Hashtable_putString(CRT_keys, "HOME", (void*) KEY_HOME);
   Hashtable_putString(CRT_keys, "INSERT", (void*) KEY_IC);
   Hashtable_putString(CRT_keys, "INSERT_LINE", (void*) KEY_IL);
   Hashtable_putString(CRT_keys, "LEFT", (void*) KEY_LEFT);
   Hashtable_putString(CRT_keys, "LL", (void*) KEY_LL);
   Hashtable_putString(CRT_keys, "MARK", (void*) KEY_MARK);
   Hashtable_putString(CRT_keys, "MESSAGE", (void*) KEY_MESSAGE);
   Hashtable_putString(CRT_keys, "MOVE", (void*) KEY_MOVE);
   Hashtable_putString(CRT_keys, "NEXT", (void*) KEY_NEXT);
   Hashtable_putString(CRT_keys, "NPAGE", (void*) KEY_NPAGE);
   Hashtable_putString(CRT_keys, "OPEN", (void*) KEY_OPEN);
   Hashtable_putString(CRT_keys, "OPTIONS", (void*) KEY_OPTIONS);
   Hashtable_putString(CRT_keys, "PPAGE", (void*) KEY_PPAGE);
   Hashtable_putString(CRT_keys, "PREVIOUS", (void*) KEY_PREVIOUS);
   Hashtable_putString(CRT_keys, "PRINT", (void*) KEY_PRINT);
   Hashtable_putString(CRT_keys, "REDO", (void*) KEY_REDO);
   Hashtable_putString(CRT_keys, "REFERENCE", (void*) KEY_REFERENCE);
   Hashtable_putString(CRT_keys, "REFRESH", (void*) KEY_REFRESH);
   Hashtable_putString(CRT_keys, "REPLACE", (void*) KEY_REPLACE);
   Hashtable_putString(CRT_keys, "RESTART", (void*) KEY_RESTART);
   Hashtable_putString(CRT_keys, "RESUME", (void*) KEY_RESUME);
   Hashtable_putString(CRT_keys, "RETURN", (void*) 0x0d);
   Hashtable_putString(CRT_keys, "RIGHT", (void*) KEY_RIGHT);
   Hashtable_putString(CRT_keys, "SAVE", (void*) KEY_SAVE);
   Hashtable_putString(CRT_keys, "SHIFT_CANCEL", (void*) KEY_SCANCEL);
   Hashtable_putString(CRT_keys, "SHIFT_COMMAND", (void*) KEY_SCOMMAND);
   Hashtable_putString(CRT_keys, "SHIFT_COPY", (void*) KEY_SCOPY);
   Hashtable_putString(CRT_keys, "SHIFT_CREATE", (void*) KEY_SCREATE);
   Hashtable_putString(CRT_keys, "SELECT", (void*) KEY_SELECT);
   Hashtable_putString(CRT_keys, "SCROLL_FORWARD", (void*) KEY_SF);
   Hashtable_putString(CRT_keys, "SHIFT_BEGIN", (void*) KEY_SBEG);
   Hashtable_putString(CRT_keys, "SHIFT_DELETE", (void*) KEY_SDC);
   Hashtable_putString(CRT_keys, "SHIFT_DL", (void*) KEY_SDL);
   Hashtable_putString(CRT_keys, "SHIFT_DOWN", (void*) KEY_S_DOWN);
   Hashtable_putString(CRT_keys, "SHIFT_END", (void*) KEY_SEND);
   Hashtable_putString(CRT_keys, "SHIFT_EOL", (void*) KEY_SEOL);
   Hashtable_putString(CRT_keys, "SHIFT_EXIT", (void*) KEY_SEXIT);
   Hashtable_putString(CRT_keys, "SHIFT_FIND", (void*) KEY_SFIND);
   Hashtable_putString(CRT_keys, "SHIFT_HELP", (void*) KEY_SHELP);
   Hashtable_putString(CRT_keys, "SHIFT_HOME", (void*) KEY_SHOME);
   Hashtable_putString(CRT_keys, "SHIFT_INSERT", (void*) KEY_SIC);
   Hashtable_putString(CRT_keys, "SHIFT_LEFT", (void*) KEY_SLEFT);
   Hashtable_putString(CRT_keys, "SHIFT_MESSAGE", (void*) KEY_SMESSAGE);
   Hashtable_putString(CRT_keys, "SHIFT_MOVE", (void*) KEY_SMOVE);
   Hashtable_putString(CRT_keys, "SHIFT_NEXT", (void*) KEY_SNEXT);
   Hashtable_putString(CRT_keys, "SHIFT_NPAGE", (void*) KEY_S_NPAGE);
   Hashtable_putString(CRT_keys, "SHIFT_OPTIONS", (void*) KEY_SOPTIONS);
   Hashtable_putString(CRT_keys, "SHIFT_PPAGE", (void*) KEY_S_PPAGE);
   Hashtable_putString(CRT_keys, "SHIFT_PREVIOUS", (void*) KEY_SPREVIOUS);
   Hashtable_putString(CRT_keys, "SHIFT_PRINT", (void*) KEY_SPRINT);
   Hashtable_putString(CRT_keys, "SHIFT_REDO", (void*) KEY_SREDO);
   Hashtable_putString(CRT_keys, "SHIFT_REPLACE", (void*) KEY_SREPLACE);
   Hashtable_putString(CRT_keys, "SHIFT_RIGHT", (void*) KEY_SRIGHT);
   Hashtable_putString(CRT_keys, "SHIFT_RSUME", (void*) KEY_SRSUME);
   Hashtable_putString(CRT_keys, "SHIFT_SAVE", (void*) KEY_SSAVE);
   Hashtable_putString(CRT_keys, "SHIFT_SUSPEND", (void*) KEY_SSUSPEND);
   Hashtable_putString(CRT_keys, "SHIFT_UNDO", (void*) KEY_SUNDO);
   Hashtable_putString(CRT_keys, "SHIFT_UP", (void*) KEY_S_UP);
   Hashtable_putString(CRT_keys, "SCROLL_BACKWARD", (void*) KEY_SR);
   Hashtable_putString(CRT_keys, "SET_TAB", (void*) KEY_STAB);
   Hashtable_putString(CRT_keys, "SUSPEND", (void*) KEY_SUSPEND);
   Hashtable_putString(CRT_keys, "TAB", (void*) '\t');
   Hashtable_putString(CRT_keys, "UNDO", (void*) KEY_UNDO);
   Hashtable_putString(CRT_keys, "UP", (void*) KEY_UP);

   bool loadedTerm = CRT_parseTerminalFile(term);
   if (!loadedTerm) {
      (void) CRT_parseTerminalFile("xterm-color");
   }

#ifndef DEBUG
//   signal(11, CRT_handleSIGSEGV);
#endif
   signal(SIGTERM, CRT_handleSIGTERM);
}

void CRT_done() {
   Hashtable_delete(CRT_keys);
   Display_done();
}

void CRT_handleSIGSEGV(int signal) {
   CRT_done();
   fprintf(stderr, "Aborted. Please report bug to the author.");
   exit(1);
}

void CRT_handleSIGTERM(int signal) {
   CRT_done();
   exit(0);
}

int CRT_getCharacter(bool* code) {
   Display_refresh();
   int ch = Display_getch(code);
   #if defined __linux || defined __CYGWIN__
   if ((*code && (ch == KEY_LEFT || ch == KEY_RIGHT || ch == KEY_UP || ch == KEY_DOWN
       || ch == KEY_HOME || ch == KEY_END || ch == KEY_IC || ch == KEY_DC)) || (!*code && ch == '\t')) {
      #ifndef __CYGWIN__
      unsigned char modifiers = 6;
      #else
      unsigned int modifiers = 6;
      #endif
      if (CRT_linuxConsole) {
         int err = ioctl(0, TIOCLINUX, &modifiers);
         if (err) return ch;
      }
      switch (modifiers) {
      case SHIFT_MASK:
         switch (ch) {
         case KEY_LEFT: return KEY_SLEFT;
         case KEY_RIGHT: return KEY_SRIGHT;
         case KEY_UP: return KEY_S_UP;
         case KEY_DOWN: return KEY_S_DOWN;
         case KEY_HOME: return KEY_SHOME;
         case KEY_END: return KEY_SEND;
         case KEY_IC: return KEY_SIC;
         case KEY_DC: return KEY_SDC;
         case KEY_NPAGE: return KEY_S_NPAGE;
         case KEY_PPAGE: return KEY_S_PPAGE;
         case '\t': return KEY_BTAB;
         }
      case SHIFT_MASK | CTRL_MASK:
         switch (ch) {
         case KEY_LEFT: return KEY_CS_LEFT;
         case KEY_RIGHT: return KEY_CS_RIGHT;
         case KEY_UP: return KEY_CS_UP;
         case KEY_DOWN: return KEY_CS_DOWN;
         case KEY_HOME: return KEY_CS_HOME;
         case KEY_END: return KEY_CS_END;
         case KEY_IC: return KEY_CS_INSERT;
         case KEY_DC: return KEY_CS_DELETE;
         case '\t': return KEY_BTAB;
         }
      case CTRL_MASK:
         switch (ch) {
         case KEY_LEFT: return KEY_C_LEFT;
         case KEY_RIGHT: return KEY_C_RIGHT;
         case KEY_UP: return KEY_C_UP;
         case KEY_DOWN: return KEY_C_DOWN;
         case KEY_HOME: return KEY_C_HOME;
         case KEY_END: return KEY_C_END;
         case KEY_IC: return KEY_C_INSERT;
         case KEY_DC: return KEY_C_DELETE;
         case '\t': return '\t';
         }
      }
   }
   #endif
   return ch;
}
