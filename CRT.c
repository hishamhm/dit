
#define _GNU_SOURCE
#include <curses.h>
#include <signal.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/ioctl.h>

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

#define KEY_S_UP      KEY_F(30)
#define KEY_S_DOWN    KEY_F(31)
#define KEY_S_RIGHT   KEY_F(32)
#define KEY_S_LEFT    KEY_F(33)
#define KEY_S_HOME    KEY_F(34)
#define KEY_S_END     KEY_F(35)
#define KEY_S_INSERT  KEY_F(36)
#define KEY_S_DELETE  KEY_F(37)
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

#define KEY_CTRL(x)  (x - 'A' + 1)

#define FOCUS_HIGHLIGHT_PAIR   CRT_color(Black, Cyan)
#define UNFOCUS_HIGHLIGHT_PAIR CRT_color(Black, White)
#define FUNCTIONBAR_PAIR       CRT_color(Black, Cyan)
#define FUNCTIONKEY_PAIR       CRT_color(White, Black)
#define HEADER_PAIR            CRT_color(Black, Green)
#define FAILEDSEARCH_PAIR      CRT_color(Red, Cyan)

#define SHIFT_MASK 1
#define ALTR_MASK 2
#define CTRL_MASK 4
#define ALTL_MASK 8

extern int CRT_delay;

int putenv(char*);

}*/

//#link ncurses

bool CRT_hasColors;

/* private property */
int CRT_delay;

/* private property */
static SCREEN* CRT_term;

void CRT_init() {

   if (strcmp(getenv("TERM"), "xterm") == 0) {
      putenv("TERM=xterm-color");
   }

   CRT_delay = 0;
   //initscr();
   CRT_term = newterm(getenv("TERM"), stdout, stdin);
   raw();
   noecho();
   if (CRT_delay)
   halfdelay(CRT_delay);
   nonl();
   intrflush(stdscr, false);
   keypad(stdscr, true);
   //curs_set(0);
   if (has_colors()) {
      start_color();
      CRT_hasColors = true;
      use_default_colors();
      for (int i = 0; i < 8; i++)
         for (int j = 0; j < 8; j++)
            init_pair(i*8+j, i==7?-1:i, j==0?-1:j);
      init_pair(White*8+Black, Black, -1);
   } else {
      CRT_hasColors = false;
   }
//   char* termType = getenv("TERM");

//   #define teq(t) String_eq(termType, t)
   // if (teq("xterm") || teq("xterm-color") || teq("vt220"))
      define_key("\033OH", KEY_HOME);
      define_key("\033OF", KEY_END);
      define_key("\033OP", KEY_F(1));
      define_key("\033OQ", KEY_F(2));
      define_key("\033OR", KEY_F(3));
      define_key("\033OS", KEY_F(4));

   // if (teq("rxvt"))
      define_key("\033[a", KEY_S_UP);
      define_key("\033[b", KEY_S_DOWN);
      define_key("\033[c", KEY_S_RIGHT);
      define_key("\033[d", KEY_S_LEFT);
      define_key("\033Oa", KEY_C_UP);
      define_key("\033Ob", KEY_C_DOWN);
      define_key("\033Oc", KEY_C_RIGHT);
      define_key("\033Od", KEY_C_LEFT);
      define_key("\033[7$", KEY_S_HOME);
      define_key("\033[8$", KEY_S_END);
      define_key("\033[2^", KEY_C_INSERT);
      define_key("\033[2$", KEY_S_INSERT);
      define_key("\033[2@", KEY_CS_INSERT);
      define_key("\033[3^", KEY_C_DELETE);
      define_key("\033[3$", KEY_S_DELETE);
      define_key("\033[7^", KEY_C_HOME);
      define_key("\033[8^", KEY_C_END);
      define_key("\033[5~", KEY_PPAGE);
      define_key("\033[6~", KEY_NPAGE);
      define_key("\033[7~", KEY_HOME);
      define_key("\033[8~", KEY_END);
   
   // if (teq("linux") || teq("xterm") || teq("xterm-color") || teq("vt220"))

      // did I invent this one?
      define_key("\033[Z", KEY_BTAB);
   
      define_key("\033[1;5A", KEY_C_UP);
      define_key("\033[1;5B", KEY_C_DOWN);
      define_key("\033[1;5C", KEY_C_RIGHT);
      define_key("\033[1;5D", KEY_C_LEFT);
      define_key("\033[1;5H", KEY_C_HOME);
      define_key("\033[1;5F", KEY_C_END);
      define_key("\033[2;5~", KEY_C_INSERT);
      define_key("\033[3;5~", KEY_C_DELETE);

      define_key("\033[1;2A", KEY_S_UP);
      define_key("\033[1;2B", KEY_S_DOWN);
      define_key("\033[1;2C", KEY_S_RIGHT);
      define_key("\033[1;2D", KEY_S_LEFT);
      define_key("\033[1;2H", KEY_S_HOME);
      define_key("\033[1;2F", KEY_S_END);
      define_key("\033[2;2~", KEY_S_INSERT);
      define_key("\033[3;2~", KEY_S_DELETE);

      define_key("\033[5;2", KEY_S_PPAGE);
      define_key("\033[6;2", KEY_S_NPAGE);

      // Konsole on KDE 3.4
      define_key("\033[H", KEY_S_HOME);
      define_key("\033[F", KEY_S_END);
      define_key("\033O2H", KEY_S_HOME);
      define_key("\033O2F", KEY_S_END);
      // Actually simple insert
      define_key("\033[2~", KEY_C_INSERT);

      define_key("\033[1;6A", KEY_CS_UP);
      define_key("\033[1;6B", KEY_CS_DOWN);
      define_key("\033[1;6C", KEY_CS_RIGHT);
      define_key("\033[1;6D", KEY_CS_LEFT);
      define_key("\033[1;6H", KEY_CS_HOME);
      define_key("\033[1;6F", KEY_CS_END);
      define_key("\033[2;6~", KEY_CS_INSERT);
      define_key("\033[3;6~", KEY_CS_DELETE);
   
//   #undef teq

#ifndef DEBUG
//   signal(11, CRT_handleSIGSEGV);
#endif
   signal(SIGTERM, CRT_handleSIGTERM);

   mousemask(BUTTON1_PRESSED, NULL);
}

void CRT_done() {
   curs_set(1);
   endwin();
   delscreen(CRT_term);
}

int CRT_readKey() {
   nocbreak();
   cbreak();
   int ret = getch();
   if (CRT_delay)
      halfdelay(CRT_delay);
   return ret;
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

int CRT_getCharacter() {
   int ch = getch();
   if (ch == KEY_LEFT || ch == KEY_RIGHT || ch == KEY_UP || ch == KEY_DOWN
       || ch == KEY_HOME || ch == KEY_END || ch == KEY_IC || ch == KEY_DC || ch == '\t') {
      unsigned char modifiers = 6;
      int err = ioctl(0, TIOCLINUX, &modifiers);
      if (err) return ch;
      switch (modifiers) {
      case SHIFT_MASK:
         switch (ch) {
         case KEY_LEFT: return KEY_S_LEFT;
         case KEY_RIGHT: return KEY_S_RIGHT;
         case KEY_UP: return KEY_S_UP;
         case KEY_DOWN: return KEY_S_DOWN;
         case KEY_HOME: return KEY_S_HOME;
         case KEY_END: return KEY_S_END;
         case KEY_IC: return KEY_S_INSERT;
         case KEY_DC: return KEY_S_DELETE;
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
   return ch;
}
