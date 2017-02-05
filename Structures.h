#ifndef Structures_HEADER
#define Structures_HEADER
#include "config.h"

typedef struct Buffer_ Buffer;
typedef struct Coords_ Coords;
typedef struct FilePosition_ FilePosition;
typedef struct Clipboard_ Clipboard;
typedef struct DebugMemoryItem_ DebugMemoryItem;
typedef struct Field_ Field;
typedef struct FieldItemClass_ FieldItemClass;
typedef struct FieldItem_ FieldItem;
typedef struct FileReader_ FileReader;
typedef struct ForEachData_ ForEachData;
typedef struct FunctionBar_ FunctionBar;
typedef struct HashtableItem_ HashtableItem;
typedef struct Hashtable_ Hashtable;
typedef struct HashtableIterator_ HashtableIterator;
typedef struct HighlightContextClass_ HighlightContextClass;
typedef struct HighlightContext_ HighlightContext;
typedef struct Highlight_ Highlight;
typedef struct ReadHighlightFileArgs_ ReadHighlightFileArgs;
typedef struct MatchArgs_ MatchArgs;
typedef struct LineClass_ LineClass;
typedef struct Line_ Line;
typedef struct ListItemClass_ ListItemClass;
typedef struct ListItem_ ListItem;
typedef struct List_ List;
typedef struct ObjectClass_ ObjectClass;
typedef struct Object_ Object;
typedef struct PanelClass_ PanelClass;
typedef struct Panel_ Panel;
typedef struct GraphNode_ GraphNode;
typedef struct PatternMatcher_ PatternMatcher;
typedef struct Pool_ Pool;
typedef struct RichString_ RichString;
typedef struct ScriptState_ ScriptState;
typedef struct StackItem_ StackItem;
typedef struct Stack_ Stack;
typedef struct StringBuffer_ StringBuffer;
typedef struct Jump_ Jump;
typedef struct TabPageClass_ TabPageClass;
typedef struct TabPage_ TabPage;
typedef struct TabManager_ TabManager;
typedef struct Text_ Text;
typedef struct UndoActionClass_ UndoActionClass;
typedef struct UndoAction_ UndoAction;
typedef struct Undo_ Undo;
typedef struct Vector_ Vector;

#define Msg(c,m,o,...) ((c ## Class*)(((Object*)o)->class))->m((c*) o, ## __VA_ARGS__)
#define Msg0(c,m,o) ((c ## Class*)(((Object*)o)->class))->m((c*) o)
#define Call(c,m,o,...) c ## _ ## m((c*) o, ## __VA_ARGS__)
#define Call0(c,m,o) c ## _ ## m((c*) o)
#define CallAs0(c,a,m,o) c ## _ ## m((a*) o)
#define Class(c) ((c ## Class*)& c ## Type)
#define ClassAs(c,a) ((a ## Class*)& c ## Type)

#define Alloc(c) ((c*)calloc(sizeof(c), 1)); ((Object*)this)->class = (ObjectClass*)&(c ## Type)
#define Bless(c) ((Object*)this)->class = (ObjectClass*)&(c ## Type)

typedef void(*Method_Object_display)(Object*, RichString*);
typedef bool(*Method_Object_equals)(const Object*, const Object*);
typedef void(*Method_Object_delete)(Object*);

struct ObjectClass_ {
   int size;
   Method_Object_display display;
   Method_Object_equals equals;
   Method_Object_delete delete;
};

struct Object_ {
   ObjectClass* class;
};

extern ObjectClass ObjectType;


struct ListItemClass_ {
   ObjectClass super;
};

struct ListItem_ {
   Object super;
   ListItem* prev;
   ListItem* next;
   List* list;
};

struct List_ {
   ListItemClass* type;
   ListItem* head;
   ListItem* tail;
   ListItem* atPtr;
   int atCtr;
   int size;
   void* data;
   Pool* pool;
};

extern ListItemClass ListItemType;


struct Text_ {
   char* data;
   int dataSize;
   int bytes;
   int chars;
};

#define Text_isSet(this) ((this).data)
#define Text_chars(this) ((this).chars)
#define Text_bytes(this) ((this).bytes)
#define Text_toString(this) ((this).data)


struct LineClass_ {
   ListItemClass super;
};

struct Line_ {
   ListItem super;
   Text text;
   HighlightContext* context;
};

extern LineClass LineType;

#define Line_chars(this) (Text_chars((this)->text))
#define Line_toString(this) (Text_toString((this)->text))
#define Line_bytes(this) (Text_bytes((this)->text))


#ifdef __linux__
 #include <execinfo.h>
#define STATIC
#else
#define STATIC static
#endif

#include <lua.h>

struct ScriptState_ {
   lua_State* L;
};

typedef struct Proxy_ {
   void* ptr;
} Proxy;


#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif
#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

#ifndef iswword
#define iswword(x) (iswalpha(x) || x == '_')
#endif

#ifndef last_x_line
#define last_x_line(this, line) (this->dosLineBreaks ? Line_chars(line) - 1 : Line_chars(line))
#endif

#ifndef last_x
#define last_x(this) last_x_line(this, this->line)
#endif

typedef int chars;
typedef int cells;

struct Buffer_ {
   char* fileName;
   bool modified;
   bool readOnly;
   // logical position of the cursor in the line
   // (character of the line where cursor is.
   chars x;
   int y;
   Line* line;
   // save X so that cursor returns to original X
   // after passing by a shorter line.
   cells savedX;
   int savedY;
   Highlight* hl;
   // save context between calls to Buffer_draw
   // to detect highlight changes that demand
   // a redraw
   HighlightContext* savedContext;
   // transient popup
   char** popup;
   // coordinates of selection
   chars selectXfrom;
   int selectYfrom;
   chars selectXto;
   int selectYto;
   // coordinates of highlighted bracket
   cells bracketX;
   int bracketY;
   // block is being selected
   bool wasSelecting;
   bool selecting;
   // mark block using 'mark' key (ie, without shift)
   bool marking;
   Panel* panel;
   Undo* undo;
   // previous key for compound actions
   int lastKey;
   // tabulation used in document: 0 means Tabs; n>0 means n spaces.
   int tabulation;
   // backup variable for toggling tab mode
   int saveTabulation;
   chars saveTabulationX;
   int saveTabulationY;
   // document uses DOS-style ctrl-M
   bool dosLineBreaks;
   // document uses UTF-8 (if false, assume ISO-8859-15)
   bool isUTF8;
   // time tracker to disable auto-indent when pasting;
   double lastTime;
   // size of Tab (\t) in cells
   int tabSize;
   int nCursors;
   struct {
      chars x;
      int y;
      chars savedX;
      int savedY;
      chars selectXfrom;
      int selectYfrom;
      chars selectXto;
      int selectYto;
      bool selecting;
      int lineLen;
   } cursors[100];
   // Lua state
   ScriptState script;
   bool skipOnChange;
   bool skipOnKey;
   bool skipOnCtrl;
   bool skipOnFKey;
   bool skipOnSave;
};

struct Coords_ {
   int x;
   int y;
};

struct FilePosition_ {
   chars x;
   int y;
   char* name;
   FilePosition* next;
};


struct Clipboard_ {
   char clipFileName[128];
   bool disk;
   char* text;
   int len;
};


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
   ScrollHandleTopColor,
   ScrollHandleBottomColor,
   HeaderColor,
   StatusColor,
   KeyColor,
   FieldColor,
   FieldFailColor,
   AlertColor,
   PopupColor,
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
#define KEY_ALT(x)    KEY_F((x=='C'?60:(x=='J'?61:(x=='K'?62:63))))

#define KEY_CTRL(x)  (x - 'A' + 1)

#define SHIFT_MASK 1
#define ALTR_MASK 2
#define CTRL_MASK 4
#define ALTL_MASK 8

extern bool CRT_linuxConsole;

extern int CRT_delay;

extern char* CRT_scrollHandle;

extern char* CRT_scrollHandleTop;

extern char* CRT_scrollHandleBottom;

extern char* CRT_scrollBar;

extern int CRT_colors[Colors];

extern Hashtable* CRT_keys;

int putenv(char*);


struct DebugMemoryItem_ {
   int magic;
   void* data;
   char* file;
   int line;
   DebugMemoryItem* next;
};

typedef struct DebugMemory_ {
   DebugMemoryItem* first;
   int allocations;
   int deallocations;
   int size;
   bool totals;
   FILE* file;
} DebugMemory;

#include "config.h"

#ifndef _XOPEN_SOURCE_EXTENDED
#define _XOPEN_SOURCE_EXTENDED
#endif

#ifdef HAVE_NCURSESW_CURSES_H
   #include <ncursesw/curses.h>
   #define HAVE_CURSES 1
#elif HAVE_NCURSES_NCURSES_H
   #include <ncurses/ncurses.h>
   #define HAVE_CURSES 1
#elif HAVE_NCURSES_H
   #include <ncurses.h>
   #define HAVE_CURSES 1
#elif HAVE_CURSES_H
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

#define KEY_WHEELUP KEY_F(20)
#define KEY_WHEELDOWN KEY_F(21)

#ifdef HAVE_LIBNCURSESW
   #define Display_writeChstrAtn mvadd_wchnstr
#elif HAVE_CURSES
   #define Display_writeChstrAtn mvaddchnstr
#else
   #define Display_writeChstrAtn Display_manualWriteChstrAtn
#endif


struct Field_ {
   char* label;
   int labelLen;
   int x;
   int y;
   int w;
   int labelColor;
   int fieldColor;
   List* history;
   FieldItem* current;
   int cursor;
};

struct FieldItemClass_ {
   ListItemClass super;
};

struct FieldItem_ {
   ListItem super;
   Text text;
};

extern FieldItemClass FieldItemType;

#define Field_text(this) ((this)->current->text)


struct FileReader_ {
   FILE* fd;
   char* buffer;
   int size;
   int used;
   char* start;
   bool eof;
   bool finalEnter;
   bool command;
};


typedef bool(*Method_Files_fileHandler)(void*, char*);

struct ForEachData_ {
   Method_Files_fileHandler fn;
   void* data;
};


struct FunctionBar_ {
   int size;
   char** functions;
   char** keys;
   int* events;
};


#define Hashtable_BORROW_REFS 0
#define Hashtable_OWN_REFS 1

typedef enum {
   Hashtable_PTR,
   Hashtable_I,
   Hashtable_STR
} Hashtable_type;

typedef union {
   void* ptr;
   int i;
   const char* str;
} HashtableKey;

typedef int(*Hashtable_hash_fn)(Hashtable*, HashtableKey);
typedef int(*Hashtable_eq_fn)(Hashtable*, HashtableKey, HashtableKey);

struct HashtableItem_ {
   HashtableKey key;
   void* value;
   HashtableItem* next;
};

struct Hashtable_ {
   int size;
   HashtableItem** buckets;
   int items;
   Hashtable_type type;
   Hashtable_hash_fn hash;
   Hashtable_eq_fn eq;
   int owner;
};

struct HashtableIterator_ {
   Hashtable* table;
   int bucket;
   HashtableItem* item;
};


typedef int(*Method_PatternMatcher_match)(GraphNode*, const char*, intptr_t*, bool*, bool*);

struct GraphNode_ {
   unsigned char min;
   unsigned char max;
   intptr_t value;
   bool endNode;
   bool eager;
   bool handOver;
   union {
      GraphNode* simple;
      struct {
         unsigned char* links;
         unsigned char nptrs;
         union {
            GraphNode* single;
            GraphNode** list;
         } p;
      } l;
   } u;
};

struct PatternMatcher_ {
   GraphNode* start;
   GraphNode* lineStart;
};


typedef enum HighlightParserState_  {
   HPS_START,
   HPS_READ_FILES,
   HPS_SKIP_FILES,
   HPS_READ_RULES,
   HPS_ERROR
} HighlightParserState;

struct HighlightContextClass_ {
   ObjectClass super;
};

struct HighlightContext_ {
   Object super;
   PatternMatcher* follows;
   HighlightContext* parent;
   HighlightContext* nextLine;
   PatternMatcher* rules;
   int id;
   Color defaultColor;
};

extern HighlightContextClass HighlightContextType;

struct Highlight_ {
   Vector* contexts;
   HighlightContext* mainContext;
   HighlightContext* currentContext;
   bool toLower;
   ScriptState* script;
   bool hasScript;
};

struct ReadHighlightFileArgs_ {
   Highlight* this;
   const char* fileName;
   Text firstLine;
   HighlightContext* context;
   Stack* contexts;
};

struct MatchArgs_ {
   const char* buffer;
   int* attrs;
   HighlightContext* ctx;
   Method_PatternMatcher_match match;
   int attrsAt;
   int y;
   bool paintUnmatched;
   bool lineStart;
};

#ifndef isword
#define isword(x) (isalpha(x) || x == '_')
#endif


#ifndef RICHSTRING_MAXLEN
#define RICHSTRING_MAXLEN 350
#endif

#include "config.h"
#include <ctype.h>

#include <assert.h>
#ifdef HAVE_NCURSESW_CURSES_H
   #include <ncursesw/curses.h>
#elif HAVE_NCURSES_NCURSES_H
   #include <ncurses/ncurses.h>
#elif HAVE_NCURSES_CURSES_H
   #include <ncurses/curses.h>
#elif HAVE_NCURSES_H
   #include <ncurses.h>
#elif HAVE_CURSES_H
   #include <curses.h>
#endif

#ifdef HAVE_LIBNCURSESW
#include <wctype.h>
#endif

#define RichString_size(this) ((this)->chlen)
#define RichString_sizeVal(this) ((this).chlen)

#define RichString_begin(this) RichString (this); memset(&this, 0, sizeof(RichString)); (this).chptr = (this).chstr;
#define RichString_beginAllocated(this) memset(&this, 0, sizeof(RichString)); (this).chptr = (this).chstr;
#define RichString_end(this) RichString_prune(&(this));

#ifdef HAVE_LIBNCURSESW
#define RichString_printVal(this, y, x) mvadd_wchstr(y, x, (this).chptr)
#define RichString_printoffnVal(this, y, x, off, n) mvadd_wchnstr(y, x, (this).chptr + off, n)
#define RichString_getCharVal(this, i) ((this).chptr[i].chars[0] & 255)
#define RichString_setChar(this, at, ch) do{ (this)->chptr[(at)] = (CharType) { .chars = { ch, 0 } }; } while(0)
#define CharType_setAttr(ch, attrs) (ch)->attr = (attrs)
#define CharType cchar_t
#else
#define RichString_printVal(this, y, x) mvaddchstr(y, x, (this).chptr)
#define RichString_printoffnVal(this, y, x, off, n) mvaddchnstr(y, x, (this).chptr + off, n)
#define RichString_getCharVal(this, i) ((this).chptr[i])
#define RichString_setChar(this, at, ch) do{ (this)->chptr[(at)] = ch; } while(0)
#define CharType_setAttr(ch, attrs) *(ch) = (*(ch) & 0xff) | (attrs)
#define CharType chtype
#endif

#define RichString_at(this, at) ((this).chptr + at)

struct RichString_ {
   int chlen;
   CharType* chptr;
   CharType chstr[RICHSTRING_MAXLEN+1];
};


typedef enum HandlerResult_ {
   HANDLED,
   IGNORED,
   BREAK_LOOP
} HandlerResult;

typedef HandlerResult(*Method_Panel_eventHandler)(Panel*, int);

struct PanelClass_ {
   ObjectClass super;
};

struct Panel_ {
   Object super;
   int x, y, w, h;
   List* items;
   int selected;
   int scrollV, scrollH;
   int oldSelected;
   bool needsRedraw;
   RichString header;
   bool highlightBar;
   int cursorX;
   int displaying;
   int color;
   bool focus;
   Method_Panel_eventHandler eventHandler;
};

extern PanelClass PanelType;


#define POOL_FREELIST_RATE 100
#define POOL_CHUNKLIST_RATE 10
#define POOL_CHUNK_SIZE 200

struct Pool_ {
   int objectSize;
   char* chunk;
   int used;
   char** freeList;
   int freeListSize;
   int freeListUsed;
   char** chunkList;
   int chunkListSize;
   int chunkListUsed;
   bool destroying;
};
   

struct StackItem_ {
   void* data;
   int size;
   StackItem* next;
};

struct Stack_ {
   ObjectClass* type;
   StackItem* head;
   bool owner;
   int size;
};


struct StringBuffer_ {
   char* buffer;
   int bufferSize;
   int usedSize;
};


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
         int tabul;
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


#ifndef DEFAULT_SIZE
#define DEFAULT_SIZE -1
#endif

typedef void(*Vector_procedure)(void*);
typedef int(*Vector_booleanFunction)(const Object*,const Object*);

struct Vector_ {
   ObjectClass* type;
   Object **array;
   int items;
   int arraySize;
   int growthRate;
   Vector_booleanFunction compareFunction;
   bool owner;
};

#endif
