#ifndef Prototypes_HEADER
#define Prototypes_HEADER

#define _GNU_SOURCE
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "Structures.h"

#include <wctype.h>
#include <unistd.h>
#include <time.h>
#include <termios.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include "Structures.h"
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <signal.h>
#include <regex.h>
#include "Prototypes.h"
#include <poll.h>
#include "md5.h"
#include <math.h>
#include <lualib.h>
#include <lua.h>
#include "lua-compat-5.3/compat-5.3.h"
#include <locale.h>
#include <limits.h>
#include <libgen.h>
#include <lauxlib.h>
#include <iconv.h>
#include <errno.h>
#include <editorconfig/editorconfig.h>
#include <dirent.h>
#include "debug.h"
#include <ctype.h>
#include "config.h"
#include <assert.h>
void Object_delete(Object* this);
void Object_display(Object* this, RichString* out);
bool Object_equals(const Object* this, const Object* o);
bool Object_instanceOf(Object* this, ObjectClass* class);
 void List_reset(List* this);
List* List_new(ListItemClass* type, void* data);
void List_resetIterator(List* this);
void List_delete(List* this);
void List_prune(List* this);
void List_add(List* this, ListItem* item);
 ListItem* List_getLast(List* this);
ListItem* List_get(List* this, int i);
void List_set(List* this, int i, ListItem* item);
void List_remove(List* this, int i);
 int List_size(List* this);
void ListItem_init(ListItem* this);
void ListItem_addAfter(ListItem* this, ListItem* item);
void ListItem_remove(ListItem* this);
bool UTF8_isValid(const char* text);
int UTF8_bytes(const char sc);
int UTF8_chars(const char* s);
int UTF8_copyChar(char* sdest, const char* ssrc);
wchar_t UTF8_stringToCodePoint(const char* ss);
const char* UTF8_forward(const char* ss, int n);
Text Text_new(char* data);
Text* Text_replace(Text* t, char* data, int bytes);
Text Text_copy(Text this);
Text Text_null();
void Text_prune(Text* this);
bool Text_hasChar(Text t, int c);
wchar_t Text_at(Text t, int n);
const char* Text_stringAt(Text t, int n);
int Text_bytesUntil(Text t, int n);
const Text Text_textAt(Text t, int n);
int Text_forwardWord(Text this, int cursor);
int Text_backwardWord(Text this, int cursor);
Text Text_wordAt(Text this, int cursor);
int Text_indexOf(Text haystack, Text needle);
int Text_indexOfFrom(Text haystack, Text needle, int from);
int Text_indexOfi(Text haystack, Text needle);
int Text_strncmp(Text haystack, Text needle);
int Text_strncasecmp(Text haystack, Text needle);
Text* Text_strcat(Text* dest, Text src);
int Text_cellsUntil(Text t, int n, int tabSize);
void Text_deleteChar(Text* t, int at);
void Text_deleteChars(Text* t, int at, int n);
void Text_clear(Text* t);
void Text_insertChar(Text* t, int at, wchar_t ch);
void Text_insert(Text* t, int at, Text new);
void Text_insertString(Text* t, int at, const char* s, int bytes);
Text Text_breakIndenting(Text* t, int at, int indent);
Line* Line_new(List* list, Text text, HighlightContext* context);
void Line_delete(Object* cast);
int Line_charAt(Line* this, int n);
void Line_updateContext(Line* this);
void Line_display(Object* cast, RichString* str);
int Line_widthUntil(Line* this, int n, int tabSize);
bool Line_equals(const Object* o1, const Object* o2);
void Line_insertChar(Line* this, int at, wchar_t ch);
void Line_deleteChars(Line* this, int at, int n);
int Line_breakAt(Line* this, int at, bool doIndent);
void Line_joinNext(Line* this);
StringBuffer* Line_deleteBlock(Line* this, int lines, int xFrom, int xTo);
StringBuffer* Line_copyBlock(Line* this, int lines, int xFrom, int xTo);
void Line_insertTextAt(Line* this, Text text, int at);
void Line_indent(Line* this, int lines, int indentSpaces);
int* Line_unindent(Line* this, int lines, int indentSpaces);
bool Line_insertBlock(Line* this, int x, Text block, int* newX, int* newY);
void Script_initState(ScriptState* state, TabManager* tabs, Buffer* buffer);
void Script_doneState(ScriptState* state);
bool Script_load(ScriptState* this, const char* scriptName);
 bool callFunction(lua_State* L, const char* fn, const char* arg);
void Script_highlightFile(Highlight* this, const char* fileName);
void Script_highlightLine(Highlight* this, const char* buffer, int* attrs, int len, int y);
bool Script_onKey(Buffer* this, int key);
void Script_onCtrl(Buffer* this, int key);
void Script_onFKey(Buffer* this, int key);
void Script_onSave(Buffer* this, const char* fileName);
void Script_onChange(Buffer* this);
 void Buffer_restorePosition(Buffer* this);
void Buffer_autoConfigureIndent(Buffer* this, int indents[]);
Buffer* Buffer_new(int x, int y, int w, int h, char* fileName, bool command, TabManager* tabs);
 void Buffer_storePosition(Buffer* this);
void Buffer_goto(Buffer* this, chars x, int y, bool adjustScroll);
void Buffer_move(Buffer* this, int x);
 void Buffer_highlightBracket(Buffer* this);
void Buffer_draw(Buffer* this);
int Buffer_x(Buffer* this);
int Buffer_y(Buffer* this);
int Buffer_scrollV(Buffer* this);
int Buffer_scrollH(Buffer* this);
const char* Buffer_currentLine(Buffer* this);
 const char* Buffer_getLine(Buffer* this, int i);
 int Buffer_getLineLength(Buffer* this, int i);
const char* Buffer_previousLine(Buffer* this);
void Buffer_delete(Buffer* this);
void Buffer_refreshHighlight(Buffer* this);
void Buffer_select(Buffer* this, void(*motion)(Buffer*));
void Buffer_setSelection(Buffer* this, int xFrom, int yFrom, int xTo, int yTo);
void Buffer_selectAll(Buffer* this);
bool Buffer_checkDiskState(Buffer* this);
void Buffer_undo(Buffer* this);
char Buffer_getLastKey(Buffer* this);
void Buffer_breakLine(Buffer* this);
void Buffer_forwardChar(Buffer* this);
void Buffer_forwardWord(Buffer* this);
void Buffer_backwardWord(Buffer* this);
void Buffer_backwardChar(Buffer* this);
void Buffer_beginUndoGroup(Buffer* this);
void Buffer_endUndoGroup(Buffer* this);
void Buffer_beginningOfLine(Buffer* this);
void Buffer_endOfLine(Buffer* this);
void Buffer_beginningOfFile(Buffer* this);
void Buffer_endOfFile(Buffer* this);
int Buffer_size(Buffer* this);
void Buffer_previousPage(Buffer* this);
void Buffer_nextPage(Buffer* this);
void Buffer_wordWrap(Buffer* this, int wrap);
void Buffer_deleteBlock(Buffer* this);
char* Buffer_copyBlock(Buffer* this, int *len);
void Buffer_pasteBlock(Buffer* this, Text block);
bool Buffer_setLine(Buffer* this, int y, const Text text);
void Buffer_deleteChar(Buffer* this);
void Buffer_backwardDeleteChar(Buffer* this);
void Buffer_upLine(Buffer* this);
void Buffer_downLine(Buffer* this);
void Buffer_slideLines(Buffer* this, int n);
void Buffer_slideUpLine(Buffer* this);
void Buffer_slideDownLine(Buffer* this);
void Buffer_correctPosition(Buffer* this);
void Buffer_validateCoordinate(Buffer* this, int *x, int* y);
 void Buffer_blockOperation(Buffer* this, Line** firstLine, int* yStart, int* lines);
void Buffer_unindent(Buffer* this);
void Buffer_indent(Buffer* this);
void Buffer_defaultKeyHandler(Buffer* this, int ch, bool code);
char Buffer_currentChar(Buffer* this);
Text Buffer_currentWord(Buffer* this);
Coords Buffer_find(Buffer* this, Text needle, bool findNext, bool caseSensitive, bool wholeWord, bool forward);
void Buffer_saveAndCloseFd(Buffer* this, FILE* fd);
bool Buffer_save(Buffer* this);
void Buffer_resize(Buffer* this, int w, int h);
void Buffer_refresh(Buffer* this);
void Buffer_toggleMarking(Buffer* this);
void Buffer_toggleTabCharacters(Buffer* this);
void Buffer_toggleDosLineBreaks(Buffer* this);
Clipboard* Clipboard_new(bool disk);
void Clipboard_delete(Clipboard* this);
Text Clipboard_get(Clipboard* this);
void Clipboard_set(Clipboard* this, char* text, int len);
bool CRT_parseTerminalFile(char* term);
void CRT_init();
void CRT_done();
void CRT_handleSIGSEGV(int signal);
void CRT_handleSIGTERM(int signal);
int CRT_getCharacter(bool* code);
void DebugMemory_new();
void* DebugMemory_malloc(int size, char* file, int line, char* str);
void* DebugMemory_calloc(int a, int b, char* file, int line);
void* DebugMemory_realloc(void* ptr, int size, char* file, int line, char* str);
void* DebugMemory_strdup(char* str, char* file, int line);
void DebugMemory_free(void* data, char* file, int line);
void DebugMemory_assertSize();
int DebugMemory_getBlockCount();
void DebugMemory_registerAllocation(void* data, char* file, int line);
void DebugMemory_registerDeallocation(void* data, char* file, int line);
void DebugMemory_report();
void Display_getScreenSize(int* w, int* h);
void Display_setWindowTitle(const char* title);
void Display_printAt(int y, int x, const char* fmt, ...);
void Display_errorScreen(const char* fmt, ...);
void Display_writeAt(int y, int x, const char* str);
void Display_writeAtn(int y, int x, const char* str, int n);
void Display_writeChAt(int y, int x, const char ch);
void Display_move(int y, int x);
void Display_mvhline(int y, int x, char c, int qty);
void Display_mvvline(int y, int x, char c, int qty);
void Display_attrToEscape(unsigned long attr, char* escape);
void Display_attrset(unsigned long attr);
void Display_clear();
void Display_manualWriteChstrAtn(int y, int x, CharType* chstr, int n);
void Display_clearToEol();
void Display_getyx(int* y, int* x);
int Display_getch(bool* code);
int Display_waitKey();
void Display_defineKey(const char* sequence, int keynum);
void Display_bkgdset(int color);
void Display_beep();
int Display_getmouse(MEVENT* mevent);
void Display_refresh();
bool Display_init(char* term);
void Display_done();
void Dit_saveAs(Buffer* buffer, TabManager* tabs);
int Dit_open(TabManager* tabs, const char* name);
void Dit_checkFileAccess(char** argv, char* name, int* jump, int* column);
int main(int argc, char** argv);
Field* Field_new(const char* label, int x, int y, int w);
void Field_delete(Field* this);
FieldItem* FieldItem_new(List* list, int w);
void Field_printfLabel(Field* this, char* picture, ...);
void FieldItem_delete(Object* cast);
void Field_start(Field* this);
FieldItem* Field_previousInHistory(Field* this);
int Field_run(Field* this, bool setCursor, bool* handled, bool* code);
void Field_insertChar(Field* this, wchar_t ch);
void Field_clear(Field* this);
void Field_setValue(Field* this, Text value);
char* Field_getValue(Field* this);
int Field_getLength(Field* this);
int Field_quickRun(Field* this, bool* quitMask);
FileReader* FileReader_new(char* filename, bool command);
void FileReader_delete(FileReader* this);
bool FileReader_eof(FileReader* this);
char* FileReader_readAllAndDelete(FileReader* this);
char* FileReader_readLine(FileReader* this);
FILE* Files_open(const char* mode, const char* picture, const char* value);
char* Files_findFile(const char* picture, const char* value, int* dirEndsAt);
bool Files_existsHome(const char* picture, const char* value);
FILE* Files_openHome(const char* mode, const char* picture, const char* value);
int Files_deleteHome(const char* picture, const char* value);
char* Files_encodePathAsFileName(char* fileName);
void Files_makeHome();
void Files_forEachInDir(char* dirName, Method_Files_fileHandler fn, void* data);
FunctionBar* FunctionBar_new(int size, char** functions, char** keys, int* events);
void FunctionBar_delete(FunctionBar* this);
void FunctionBar_draw(FunctionBar* this, char* buffer);
void FunctionBar_drawAttr(FunctionBar* this, char* buffer, int attr);
int FunctionBar_synthesizeEvent(FunctionBar* this, int pos);
Hashtable* Hashtable_new(int size, Hashtable_type type, int owner);
void Hashtable_delete(Hashtable* this);
int Hashtable_size(Hashtable* this);
void Hashtable_putInt(Hashtable* this, int key, void* value);
void Hashtable_put(Hashtable* this, HashtableKey key, void* value);
void Hashtable_putString(Hashtable* this, const char* key, void* value);
void* Hashtable_take(Hashtable* this, HashtableKey key);
void* Hashtable_remove(Hashtable* this, HashtableKey key);
void* Hashtable_get(Hashtable* this, HashtableKey key);
void* Hashtable_getInt(Hashtable* this, int key);
void* Hashtable_getString(Hashtable* this, const char* key);
void* Hashtable_takeFirst(Hashtable* this);
void Hashtable_start(Hashtable* this, HashtableIterator* iter);
void* Hashtable_iterate(HashtableIterator* iter);
PatternMatcher* PatternMatcher_new();
void PatternMatcher_delete(PatternMatcher* this);
void GraphNode_build(GraphNode* current, unsigned char* input, unsigned char* special, intptr_t value, bool eager, bool handOver);
void PatternMatcher_add(PatternMatcher* this, unsigned char* pattern, intptr_t value, bool eager, bool handOver);
bool PatternMatcher_partialMatch(GraphNode* node, const char* sinput, int inputLen, char* rest, int restLen);
int PatternMatcher_match(GraphNode* node, const char* sinput, intptr_t* value, bool* eager, bool* handOver);
int PatternMatcher_match_toLower(GraphNode* node, const char* sinput, intptr_t* value, bool* eager, bool* handOver);
GraphNode* GraphNode_new();
void GraphNode_delete(GraphNode* this, GraphNode* prev);
void GraphNode_link(GraphNode* this, unsigned char* mask, GraphNode* next);
Color Highlight_translateColorKey(char c);
Highlight* Highlight_new(const char* fileName, Text firstLine, ScriptState* script);
void Highlight_delete(Highlight* this);
HighlightParserState parseFile(ReadHighlightFileArgs* args, FILE* file, const char* name, HighlightParserState state);
bool Highlight_readHighlightFile(ReadHighlightFileArgs* args, char* name);
HighlightContext* Highlight_addContext(Highlight* this, char* open, char* close, HighlightContext* parent, Color color);
void Highlight_setAttrs(Highlight* this, const char* buffer, int* attrs, int len, int y);
 HighlightContext* Highlight_getContext(Highlight* this);
 void Highlight_setContext(Highlight* this, HighlightContext* context);
HighlightContext* HighlightContext_new(int id, Color defaultColor, HighlightContext* parent);
void HighlightContext_delete(Object* cast);
void HighlightContext_addRule(HighlightContext* this, char* rule, Color color, bool eager, bool handOver);
void RichString_setAttrn(RichString* this, int attrs, int start, int finish);
int RichString_findChar(RichString* this, char c, int start);
void RichString_prune(RichString* this);
void RichString_setAttr(RichString* this, int attrs);
void RichString_append(RichString* this, int attrs, const char* data);
void RichString_appendn(RichString* this, int attrs, const char* data, int len);
void RichString_write(RichString* this, int attrs, const char* data);
void RichString_paintAttrs(RichString* this, int* attrs);
Panel* Panel_new(int x, int y, int w, int h, int color, ListItemClass* class, bool owner, void* data);
void Panel_delete(Object* cast);
void Panel_init(Panel* this, int x, int y, int w, int h, int color, ListItemClass* class, bool owner, void* data);
void Panel_done(Panel* this);
void Panel_setFocus(Panel* this, bool focus);
void Panel_setHeader(Panel* this, RichString header);
void Panel_move(Panel* this, int x, int y);
void Panel_resize(Panel* this, int w, int h);
void Panel_prune(Panel* this);
void Panel_add(Panel* this, ListItem* l);
void Panel_set(Panel* this, int i, ListItem* l);
ListItem* Panel_get(Panel* this, int i);
Object* Panel_remove(Panel* this, int i);
ListItem* Panel_getSelected(Panel* this);
int Panel_getSelectedIndex(Panel* this);
int Panel_size(Panel* this);
void Panel_setSelected(Panel* this, int selected);
void Panel_draw(Panel* this);
void Panel_slide(Panel* this, int n);
bool Panel_onKey(Panel* this, int key);
Pool* Pool_new(ObjectClass* type);
void Pool_initiateDestruction(Pool* this);
void* Pool_allocate(Pool* this);
 void* Pool_allocateClear(Pool* this);
void Pool_free(Pool* this, void* item);
void Pool_delete(Pool* this);
Stack* Stack_new(ObjectClass* type, bool owner);
void Stack_delete(Stack* this);
void Stack_push(Stack* this, void* data, int size);
void* Stack_pop(Stack* this, int* size);
void* Stack_peek(Stack* this, int* size);
void* Stack_peekAt(Stack* this, int n, int* size);
StringBuffer* StringBuffer_new(char* data);
void StringBuffer_delete(StringBuffer* this);
char* StringBuffer_deleteGet(StringBuffer* this);
 int StringBuffer_len(StringBuffer* this);
void StringBuffer_makeRoom(StringBuffer* this, int neededSize);
void StringBuffer_addChar(StringBuffer* this, char ch);
void StringBuffer_set(StringBuffer* this, const char* str);
void StringBuffer_add(StringBuffer* this, char* str);
void StringBuffer_addN(StringBuffer* this, const char* str, int len);
void StringBuffer_prepend(StringBuffer* this, char* str);
void StringBuffer_addAll(StringBuffer* this, int n, ...);
void StringBuffer_printf(StringBuffer* this, char* format, ...);
void StringBuffer_addPrintf(StringBuffer* this, char* format, ...);
char* StringBuffer_getCopy(StringBuffer* this);
char* StringBuffer_getRef(StringBuffer* this);
char* StringBuffer_getBuffer(StringBuffer* this);
void StringBuffer_prune(StringBuffer* this);
 void String_delete(char* s);
char* String_cat(char* s1, char* s2);
char* String_trim(char* in);
void String_println(char* s);
void String_print(char* s);
void String_printInt(int i);
void String_printPointer(void* p);
 int String_eq(const char* s1, const char* s2);
 int String_startsWith(const char* s, const char* match);
 int String_endsWith(const char* s, const char* match);
char** String_split(char* s, char sep);
void String_freeArray(char** s);
int String_startsWith_i(char* s, char* match);
int String_contains_i(char* s, char* match);
int String_indexOf_i(char* s, char* match, int lens);
int String_indexOf(char* s, char* match, int lens);
void String_convertEscape(char* s, char* escape, char value);
void Jump_purge(Jump* jump);
TabPage* TabPage_new(char* name, Buffer* buffer);
void TabPage_delete(Object* super);
TabManager* TabManager_new(int x, int y, int w, int h, int tabOffset);
void TabManager_delete(TabManager* this);
void TabManager_moveTabLeft(TabManager* this);
void TabManager_moveTabRight(TabManager* this);
int TabManager_add(TabManager* this, char* name, Buffer* buffer);
void TabManager_removeCurrent(TabManager* this);
TabPage* TabManager_current(TabManager* this);
Buffer* TabManager_getBuffer(TabManager* this, int pageNr);
Buffer* TabManager_draw(TabManager* this, int width);
bool TabManager_checkLock(TabManager* this, char* fileName);
void TabManager_releaseLock(char* fileName);
void TabManager_resize(TabManager* this, int w, int h);
int TabManager_find(TabManager* this, char* name);
 void TabManager_refreshCurrent(TabManager* this);
void TabManager_previousPage(TabManager* this);
void TabManager_nextPage(TabManager* this);
void TabManager_setPage(TabManager* this, int i);
void TabManager_markJump(TabManager* this);
void TabManager_jumpBack(TabManager* this);
int TabManager_getPageCount(TabManager* this);
char* TabManager_getPageName(TabManager* this, int i);
void TabManager_load(TabManager* this, char* fileName, int limit);
void TabManager_save(TabManager* this, char* fileName);
int TabManager_size(TabManager* this);
int TabManager_question(TabManager* this, char* question, char* options);
 UndoAction* UndoAction_new(UndoActionKind kind, int x, int y);
void UndoAction_delete(Object* cast);
Undo* Undo_new(List* list);
void Undo_delete(Undo* this);
 void Undo_char(Undo* this, UndoActionKind kind, int x, int y, wchar_t ch);
void Undo_deleteCharAt(Undo* this, int x, int y, wchar_t ch);
void Undo_backwardDeleteCharAt(Undo* this, int x, int y, wchar_t ch);
void Undo_insertCharAt(Undo* this, int x, int y, wchar_t ch);
void Undo_breakAt(Undo* this, int x, int y, int indent);
void Undo_joinNext(Undo* this, int x, int y, bool backspace);
void Undo_deleteBlock(Undo* this, int x, int y, char* block, int len);
void Undo_indent(Undo* this, int x, int y, int lines, int width);
void Undo_unindent(Undo* this, int x, int y, int* counts, int lines, int tabul);
void Undo_beginGroup(Undo* this, int x, int y);
void Undo_endGroup(Undo* this, int x, int y);
void Undo_insertBlock(Undo* this, int x, int y, Text block);
void Undo_insertBlanks(Undo* this, int x, int y, int len);
void Undo_diskState(Undo* this, int x, int y, char* md5, char* fileName);
bool Undo_checkDiskState(Undo* this);
bool Undo_undo(Undo* this, int* x, int* y);
void Undo_store(Undo* this, char* fileName);
void Undo_restore(Undo* this, char* fileName);
Vector* Vector_new(ObjectClass* type, bool owner, int size);
void Vector_delete(Vector* this);
void Vector_prune(Vector* this);
int Vector_compareFunction(const Object* v1, const Object* v2);
void Vector_setCompareFunction(Vector* this, Vector_booleanFunction f);
void Vector_sort(Vector* this);
void Vector_insert(Vector* this, int index, void* data_);
Object* Vector_take(Vector* this, int index);
Object* Vector_remove(Vector* this, int index);
void Vector_moveUp(Vector* this, int index);
void Vector_moveDown(Vector* this, int index);
void Vector_set(Vector* this, int index, void* data_);
 Object* Vector_get(Vector* this, int index);
 int Vector_size(Vector* this);
void Vector_merge(Vector* this, Vector* v2);
void Vector_add(Vector* this, void* data_);
int Vector_indexOf(Vector* this, void* search_);
void Vector_foreach(Vector* this, Vector_procedure f);
#if HAVE_CURSES
#define Display_printAt mvprintw
#define Display_writeAt mvaddstr
#define Display_writeAtn mvaddnstr
#define Display_writeChAt mvaddch
#define Display_move move
#define Display_mvhline mvhline
#define Display_mvvline mvvline
#define Display_attrset attrset
#define Display_clear clear
#define Display_clearToEol clrtoeol
#define Display_defineKey define_key
#define Display_bkgdset bkgdset
#define Display_beep beep
#define Display_getmouse getmouse
#define Display_refresh refresh
#endif

#include <stdlib.h>
#include "debug.h"
#include <assert.h>

#endif
