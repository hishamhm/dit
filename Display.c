
#include "Prototypes.h"
#include "Structures.h"

#include <stdbool.h>

#if HAVE_CURSES

static SCREEN* CRT_term;

#else

#include <sys/ioctl.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <poll.h>

static int cursorX;
static int cursorY;
static struct termios stdoutSettings;
static Hashtable* Display_terminalSequences;

#endif

/*{
#include "config.h"

#ifndef _XOPEN_SOURCE_EXTENDED
#define _XOPEN_SOURCE_EXTENDED
#endif

#ifdef HAVE_NCURSESW_CURSES_H
   #include <ncursesw/curses.h>
   #define HAVE_CURSES 1
   #undef mvaddchnstr
   #define mvaddchnstr mvadd_wchnstr
#elif HAVE_NCURSES_NCURSES_H
   #include <ncurses/ncurses.h>
   #define HAVE_CURSES 1
#elif HAVE_NCURSES_H
   #include <ncurses.h>
   #define HAVE_CURSES 1
#elif HAVE_CURSES
   #include <curses.h>
   #define HAVE_CURSES 1
#else

#define OK 0
#define ERR -1

#define BUTTON1_PRESSED 1000

typedef struct mevent {
   int x;
   int y;
} MEVENT;

// Copyright (c) 1998-2007,2008 Free Software Foundation, Inc.
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, distribute with modifications, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE ABOVE COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
// THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// Except as contained in this notice, the name(s) of the above copyright
// holders shall not be used in advertising or otherwise to promote the
// sale, use or other dealings in this Software without prior written
// authorization.

#define NCURSES_ATTR_SHIFT       8
#define NCURSES_BITS(mask,shift) ((mask) << ((shift) + NCURSES_ATTR_SHIFT))

#define COLOR_PAIR(n)   NCURSES_BITS(n, 0)

#define A_NORMAL        (1UL - 1UL)
#define A_ATTRIBUTES    NCURSES_BITS(~(1UL - 1UL),0)
#define A_CHARTEXT      (NCURSES_BITS(1UL,0) - 1UL)
#define A_COLOR         NCURSES_BITS(((1UL) << 8) - 1UL,0)
#define A_STANDOUT      NCURSES_BITS(1UL,8)
#define A_UNDERLINE     NCURSES_BITS(1UL,9)
#define A_REVERSE       NCURSES_BITS(1UL,10)
#define A_BLINK         NCURSES_BITS(1UL,11)
#define A_DIM           NCURSES_BITS(1UL,12)
#define A_BOLD          NCURSES_BITS(1UL,13)

#define KEY_DOWN        0402            // down-arrow key
#define KEY_UP          0403            // up-arrow key
#define KEY_LEFT        0404            // left-arrow key
#define KEY_RIGHT       0405            // right-arrow key
#define KEY_HOME        0406            // home key
#define KEY_BACKSPACE   0407            // backspace key
#define KEY_F0          0410            // Function keys.  Space for 64
#define KEY_F(n)        (KEY_F0+(n))    // Value of function key n
#define KEY_DL          0510            // delete-line key
#define KEY_IL          0511            // insert-line key
#define KEY_DC          0512            // delete-character key
#define KEY_IC          0513            // insert-character key
#define KEY_EIC         0514            // sent by rmir or smir in insert mode
#define KEY_CLEAR       0515            // clear-screen or erase key
#define KEY_EOS         0516            // clear-to-end-of-screen key
#define KEY_EOL         0517            // clear-to-end-of-line key
#define KEY_SF          0520            // scroll-forward key
#define KEY_SR          0521            // scroll-backward key
#define KEY_NPAGE       0522            // next-page key
#define KEY_PPAGE       0523            // previous-page key
#define KEY_STAB        0524            // set-tab key
#define KEY_CTAB        0525            // clear-tab key
#define KEY_CATAB       0526            // clear-all-tabs key
#define KEY_ENTER       0527            // enter/send key
#define KEY_PRINT       0532            // print key
#define KEY_LL          0533            // lower-left key (home down)
#define KEY_A1          0534            // upper left of keypad
#define KEY_A3          0535            // upper right of keypad
#define KEY_B2          0536            // center of keypad
#define KEY_C1          0537            // lower left of keypad
#define KEY_C3          0540            // lower right of keypad
#define KEY_BTAB        0541            // back-tab key
#define KEY_BEG         0542            // begin key
#define KEY_CANCEL      0543            // cancel key
#define KEY_CLOSE       0544            // close key
#define KEY_COMMAND     0545            // command key
#define KEY_COPY        0546            // copy key
#define KEY_CREATE      0547            // create key
#define KEY_END         0550            // end key
#define KEY_EXIT        0551            // exit key
#define KEY_FIND        0552            // find key
#define KEY_HELP        0553            // help key
#define KEY_MARK        0554            // mark key
#define KEY_MESSAGE     0555            // message key
#define KEY_MOVE        0556            // move key
#define KEY_NEXT        0557            // next key
#define KEY_OPEN        0560            // open key
#define KEY_OPTIONS     0561            // options key
#define KEY_PREVIOUS    0562            // previous key
#define KEY_REDO        0563            // redo key
#define KEY_REFERENCE   0564            // reference key
#define KEY_REFRESH     0565            // refresh key
#define KEY_REPLACE     0566            // replace key
#define KEY_RESTART     0567            // restart key
#define KEY_RESUME      0570            // resume key
#define KEY_SAVE        0571            // save key
#define KEY_SBEG        0572            // shifted begin key
#define KEY_SCANCEL     0573            // shifted cancel key
#define KEY_SCOMMAND    0574            // shifted command key
#define KEY_SCOPY       0575            // shifted copy key
#define KEY_SCREATE     0576            // shifted create key
#define KEY_SDC         0577            // shifted delete-character key
#define KEY_SDL         0600            // shifted delete-line key
#define KEY_SELECT      0601            // select key
#define KEY_SEND        0602            // shifted end key
#define KEY_SEOL        0603            // shifted clear-to-end-of-line key
#define KEY_SEXIT       0604            // shifted exit key
#define KEY_SFIND       0605            // shifted find key
#define KEY_SHELP       0606            // shifted help key
#define KEY_SHOME       0607            // shifted home key
#define KEY_SIC         0610            // shifted insert-character key
#define KEY_SLEFT       0611            // shifted left-arrow key
#define KEY_SMESSAGE    0612            // shifted message key
#define KEY_SMOVE       0613            // shifted move key
#define KEY_SNEXT       0614            // shifted next key
#define KEY_SOPTIONS    0615            // shifted options key
#define KEY_SPREVIOUS   0616            // shifted previous key
#define KEY_SPRINT      0617            // shifted print key
#define KEY_SREDO       0620            // shifted redo key
#define KEY_SREPLACE    0621            // shifted replace key
#define KEY_SRIGHT      0622            // shifted right-arrow key
#define KEY_SRSUME      0623            // shifted resume key
#define KEY_SSAVE       0624            // shifted save key
#define KEY_SSUSPEND    0625            // shifted suspend key
#define KEY_SUNDO       0626            // shifted undo key
#define KEY_SUSPEND     0627            // suspend key
#define KEY_UNDO        0630            // undo key
#define KEY_MOUSE       0631            // Mouse event has occurred
#define KEY_RESIZE      0632            // Terminal resize event
#define KEY_EVENT       0633            // We were interrupted by an event
#define KEY_MAX         0777

#define COLOR_BLACK     0
#define COLOR_RED       1
#define COLOR_GREEN     2
#define COLOR_YELLOW    3
#define COLOR_BLUE      4
#define COLOR_MAGENTA   5
#define COLOR_CYAN      6
#define COLOR_WHITE     7

#endif

}*/

void Display_getScreenSize(int* w, int* h) {
#if HAVE_CURSES
   *w = COLS;
   *h = LINES;
#else
   struct winsize ws;
   int err = ioctl(1, TIOCGWINSZ, &ws);
   if (err == 0) {
      *w = ws.ws_col;
      *h = ws.ws_row;
   } else {
      *w = 80;
      *h = 25;
   }
#endif
}

#if HAVE_CURSES
#define Display_printAt mvprintw
#else
void Display_printAt(int y, int x, const char* fmt, ...) {
   va_list ap;
   printf("\033[%d;%df", y+1, x+1);
   va_start(ap, fmt);
   vprintf(fmt, ap);
   va_end(ap);
}
#endif

#if HAVE_CURSES
#define Display_writeAt mvaddstr
#define Display_writeAtn mvaddnstr
#define Display_writeChAt mvaddch
#else
void Display_writeAt(int y, int x, const char* str) {
   printf("\033[%d;%df%s", y+1, x+1, str);
}
void Display_writeAtn(int y, int x, const char* str, int n) {
   char buf[n+1];
   strncpy(buf, str, MIN(n,strlen(str))+1);
   buf[n] = '\0';
   printf("\033[%d;%df%s", y+1, x+1, buf);
}
void Display_writeChAt(int y, int x, const char ch) {
   printf("\033[%d;%df%c", y+1, x+1, ch);
}
#endif

#if HAVE_CURSES
#define Display_move move
#else
void Display_move(int y, int x) {
   printf("\033[%d;%df", y+1, x+1);
   cursorY = y;
   cursorX = x;
}
#endif

#if HAVE_CURSES
#define Display_mvhline mvhline
#else
void Display_mvhline(int y, int x, char c, int qty) {
   printf("\033[%d;%df", y+1, x+1);
   for (int i = 0; i < qty; i++)
      putchar(c);
}
#endif

#if HAVE_CURSES
#define Display_mvvline mvvline
#else
void Display_mvvline(int y, int x, char c, int qty) {
   for (int i = 0; i < qty; i++) {
      printf("\033[%d;%df%c", y+1+i, x+1, c);
   }
}
#endif

#if HAVE_CURSES
#define Display_attrset attrset
#else

void Display_attrToEscape(unsigned long attr, char* escape) {
   escape[0] = '\033';
   escape[1] = '[';
   int i = 2;
   escape[i] = '0'; i++;
   if (attr > 0) {
      if (attr & A_BOLD) {
         escape[i] = ';'; i++;
         escape[i] = '1'; i++;
//      } else {
//         escape[i] = '2'; i++;
//         escape[i] = '2'; i++;
      }
      if (attr & A_REVERSE) {
         escape[i] = ';'; i++;
         escape[i] = '7'; i++;
//      } else {
//         escape[i] = '2'; i++;
//         escape[i] = '7'; i++;
      }
      if (attr & (7<<8)) {
         escape[i] = ';'; i++;
         escape[i] = '4'; i++;
         escape[i] = '0' + ((attr & (7<<8))>>8); i++;
      }
      if (attr & (077<<8)) {
         escape[i] = ';'; i++;
         escape[i] = '3'; i++;
         escape[i] = '0' + ((attr & (7<<11)) >> 11); i++;
      }
   }
   escape[i] = 'm';
   escape[i+1] = '\0';
}

void Display_attrset(unsigned long attr) {
   char escape[20];
   Display_attrToEscape(attr, escape);
   fwrite(escape, strlen(escape), 1, stdout);
}
#endif

#if HAVE_CURSES
#define Display_clear clear
#else
void Display_clear() {
   fwrite("\033[H\033[2J", 6, 1, stdout);
}
#endif

#if HAVE_CURSES
#define Display_writeChstrAtn mvaddchnstr
#else
void Display_writeChstrAtn(int y, int x, CharType* chstr, int n) {
   printf("\033[%d;%df", y+1, x+1, y, x, n);
/*
   char str[n];
   for (int i = 0; i < n; i++) {
      int attr = (chstr[i] & 0xff00) >> 8;
      char c = chstr[i] & 0xff;
      str[i] = c;
   }
   fwrite(str, n, 1, stdout);
*/
   int lastAttr = -1;
   char buffer[1024];
   int len = 0;
   for (int i = 0; i < n; i++) {
      int attr = (chstr[i] & 0xffffff00);
      char c = chstr[i] & 0xff;
      if (attr != lastAttr) {
         char escape[20];
         Display_attrToEscape(attr, escape);
         len += snprintf(buffer+len, sizeof(buffer)-len, "%s%c", escape, c);
         lastAttr = attr;
      } else {
         len += snprintf(buffer+len, sizeof(buffer)-len, "%c", c);
      }
      if (len > 1000) {
         fwrite(buffer, len, 1, stdout);
         len = 0;
      }
   }
   len += snprintf(buffer+len, sizeof(buffer)-len, "\033[0m");
   fwrite(buffer, len, 1, stdout);
}
#endif

#if HAVE_CURSES
#define Display_clearToEol clrtoeol
#else
void Display_clearToEol() {
   fwrite("\033[K", 3, 1, stdout);
}
#endif

#if HAVE_CURSES
void Display_getyx(int* y, int* x) {
   getyx(stdscr, *y, *x);
}
#else
void Display_getyx(int* y, int* x) {
   *y = cursorY;
   *x = cursorX;
}
#endif

int Display_getch(bool* code) {
   #if HAVE_CURSES
      #if HAVE_NCURSESW_CURSES_H
         wint_t ch;
         int isCode = get_wch(&ch);
         *code = (isCode == KEY_CODE_YES || isCode == ERR);
         if (isCode == ERR) return ERR;
         return ch;
      #else
         int ch = getch();
         *code = (ch > 0xff || ch == ERR);
         return ch;
      #endif
   #else
      char sequence[11] = { 0 };
      // TODO: UTF-8 loop
      int ch = getchar();
      sequence[0] = ch;
      if (ch == 27) {
         for (int i = 1; i <= 10; i++) {
            struct pollfd pfd = { .fd = 0, .events = POLLIN, .revents = 0 };
            int any = poll(&pfd, 1, 30);
            if (any > 0) {
               sequence[i] = getchar();
            } else {
               break;
            }
         }
      }
      int keynum = (int) Hashtable_getString(Display_terminalSequences, sequence);
      if (keynum) {
         *code = true;
         return keynum;
      }
      *code = false;
      return ch;
   #endif
}

#if HAVE_CURSES
#define Display_defineKey define_key
#else
void Display_defineKey(const char* sequence, int keynum) {
   Hashtable_putString(Display_terminalSequences, sequence, (void*) keynum);
}
#endif

#if HAVE_CURSES
#define Display_bkgdset bkgdset
#else
void Display_bkgdset(int color) {
   // TODO
}
#endif

#if HAVE_CURSES
#define Display_beep beep
#else
void Display_beep() {
   fwrite("\007", 1, 1, stdout);
}
#endif

#if HAVE_CURSES
#define Display_getmouse getmouse
#else
int Display_getmouse(MEVENT* mevent) {
   // TODO
   return ERR;
}
#endif

#if HAVE_CURSES
#define Display_refresh refresh
#else
void Display_refresh() {
   // TODO
}
#endif

#if HAVE_CURSES
bool Display_init(char* term) {
   CRT_term = newterm(term, stdout, stdin);
   raw();
   noecho();
   if (CRT_delay)
      halfdelay(CRT_delay);
   nonl();
   intrflush(stdscr, false);
   keypad(stdscr, true);
   mousemask(BUTTON1_PRESSED, NULL);
   ESCDELAY = 100;
   if (has_colors()) {
      start_color();
      use_default_colors();
      for (int i = 0; i < 8; i++)
         for (int j = 0; j < 8; j++)
            init_pair(i*8+j, i==7?-1:i, j==0?-1:j);
      init_pair(White*8+Black, Black, -1);
      return true;
   }
   return false;
}
#else
bool Display_init(char* term) {

   tcgetattr(0, &stdoutSettings);
   struct termios rawTerm = stdoutSettings;

   rawTerm.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP
                      | INLCR | IGNCR | ICRNL | IXON);
   rawTerm.c_oflag &= ~OPOST;
   rawTerm.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);                                                                      
   rawTerm.c_cflag &= ~(CSIZE | PARENB);
   rawTerm.c_cflag |= CS8;

   //rawTerm.c_lflag &= ~(ECHO|ECHONL|ICANON|IEXTEN);
   tcsetattr(0, TCSADRAIN, &rawTerm);

   setbuf(stdin, NULL);

   Display_terminalSequences = Hashtable_new(200, Hashtable_STR, Hashtable_BORROW_REFS);
   return true; 
}
#endif

#if HAVE_CURSES
void Display_done() {
   curs_set(1);
   endwin();
   //delscreen(CRT_term);
}
#else
void Display_done() {
   tcsetattr(0, TCSADRAIN, &stdoutSettings);
   Hashtable_delete(Display_terminalSequences);
}
#endif
