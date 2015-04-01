
#include "Prototypes.h"

/*{

#include <lua.h>

struct ScriptState_ {
   lua_State* L;
};

typedef struct Proxy_ {
   void* ptr;
} Proxy;

}*/

#include <lualib.h>
#include <lauxlib.h>
#include "lua-compat-5.3/compat-5.3.h"

static void error(lua_State* L) {
   int lines, cols;
   Display_getScreenSize(&cols, &lines);
   Display_errorScreen("%s", lua_tostring(L, -1));
   lua_getglobal(L, "tabs");
   TabManager* tabs = (TabManager*) ((Proxy*)lua_touserdata(L, -1))->ptr;
   TabManager_refreshCurrent(tabs);
   TabManager_draw(tabs, cols);
}

static void Script_pushObject(lua_State* L, void* ptr, const char* klass, const luaL_Reg* functions) {
   Proxy* proxy = lua_newuserdata(L, sizeof(Proxy));
   proxy->ptr = ptr;
   if (luaL_newmetatable(L, klass)) {
      luaL_setfuncs(L, functions, 0);
      lua_pushvalue(L, -1);
      lua_setfield(L, -2, "__index");
   }
   lua_setmetatable(L, -2);
}

static int Script_Buffer_goto(lua_State* L) {
   Buffer* buffer = (Buffer*) ((Proxy*)luaL_checkudata(L, 1, "Buffer"))->ptr;
   int x = luaL_checkinteger(L, 2);
   int y = luaL_checkinteger(L, 3);
   Buffer_validateCoordinate(buffer, &x, &y);
   Buffer_goto(buffer, x-1, y-1);
   buffer->savedX = buffer->x;
   return 0;
}

static int Script_Buffer_line(lua_State* L) {
   Buffer* buffer = (Buffer*) ((Proxy*)luaL_checkudata(L, 1, "Buffer"))->ptr;
   const char* line;
   if (lua_gettop(L) == 1) {
      line = Buffer_currentLine(buffer);
      lua_pushstring(L, line);
      lua_pushinteger(L, buffer->x + 1);
      lua_pushinteger(L, buffer->y + 1);
      return 3;
   } else {
      int y = luaL_checkinteger(L, 2);
      line = Buffer_getLine(buffer, y-1);
      if (line) lua_pushstring(L, line);
      else lua_pushliteral(L, "");
      return 1;
   }
}

static int Script_Buffer_select(lua_State* L) {
   Buffer* buffer = (Buffer*) ((Proxy*)luaL_checkudata(L, 1, "Buffer"))->ptr;
   int xFrom = luaL_checkinteger(L, 2) - 1;
   int yFrom = luaL_checkinteger(L, 3) - 1;
   int xTo = luaL_checkinteger(L, 4) - 1;
   int yTo = luaL_checkinteger(L, 5) - 1;
   Buffer_validateCoordinate(buffer, &xFrom, &yFrom);
   Buffer_validateCoordinate(buffer, &xTo, &yTo);
   buffer->selecting = true;
   buffer->selectXfrom = xFrom;
   buffer->selectYfrom = yFrom;
   buffer->selectXto = xTo;
   buffer->selectYto = yTo;
   buffer->x = xTo;
   buffer->y = yTo;
   buffer->panel->needsRedraw = true;
   buffer->savedX = buffer->x;
   return 0;
}

static int Script_Buffer_selection(lua_State* L) {
   Buffer* buffer = (Buffer*) ((Proxy*)luaL_checkudata(L, 1, "Buffer"))->ptr;
   if (!buffer->selecting) {
      lua_pushliteral(L, "");
      lua_pushinteger(L, buffer->x + 1);
      lua_pushinteger(L, buffer->y + 1);
      lua_pushinteger(L, buffer->x + 1);
      lua_pushinteger(L, buffer->y + 1);
      return 5;
   }
   int len = 0;
   char* block = Buffer_copyBlock(buffer, &len);
   lua_pushstring(L, block);
   lua_pushinteger(L, buffer->selectXfrom + 1);
   lua_pushinteger(L, buffer->selectYfrom + 1);
   lua_pushinteger(L, buffer->selectXto + 1);
   lua_pushinteger(L, buffer->selectYto + 1);
   free(block);
   return 5;
}

static int Script_Buffer_token(lua_State* L) {
   Buffer* buffer = (Buffer*) ((Proxy*)luaL_checkudata(L, 1, "Buffer"))->ptr;
   const char* line = Buffer_currentLine(buffer);
   int x = buffer->x;
   if (!isword(line[x])) return 0;
   while (x > 0 && isword(line[x-1])) x--;
   int len = 0;
   while (isword(line[x+len])) len++;
   lua_pushlstring(L, line+x, len);
   lua_pushinteger(L, x + 1);
   lua_pushinteger(L, buffer->y + 1);
   lua_pushinteger(L, len);
   return 4;
}

static int Script_Buffer_dir(lua_State* L) {
   Buffer* buffer = (Buffer*) ((Proxy*)luaL_checkudata(L, 1, "Buffer"))->ptr;
   char* name = buffer->fileName;
   char* slash = strrchr(name, '/');
   if (!slash)
      return 0;
   lua_pushlstring(L, name, slash - name);
   return 1;
}

static int Script_Buffer_filename(lua_State* L) {
   Buffer* buffer = (Buffer*) ((Proxy*)luaL_checkudata(L, 1, "Buffer"))->ptr;
   lua_pushstring(L, buffer->fileName);
   return 1;
}

static int Script_Buffer_basename(lua_State* L) {
   Buffer* buffer = (Buffer*) ((Proxy*)luaL_checkudata(L, 1, "Buffer"))->ptr;
   const char* baseName = strrchr(buffer->fileName, '/');
   baseName = baseName ? baseName + 1 : buffer->fileName;
   lua_pushstring(L, baseName);
   return 1;
}

static int Script_Buffer_xy(lua_State* L) {
   Buffer* buffer = (Buffer*) ((Proxy*)luaL_checkudata(L, 1, "Buffer"))->ptr;
   lua_pushinteger(L, buffer->x + 1);
   lua_pushinteger(L, buffer->y + 1);
   return 2;
}

static int Script_Buffer_beginUndoGroup(lua_State* L) {
   Buffer* buffer = (Buffer*) ((Proxy*)luaL_checkudata(L, 1, "Buffer"))->ptr;
   Buffer_beginUndoGroup(buffer);
   return 0;
}

static int Script_Buffer_endUndoGroup(lua_State* L) {
   Buffer* buffer = (Buffer*) ((Proxy*)luaL_checkudata(L, 1, "Buffer"))->ptr;
   Buffer_endUndoGroup(buffer);
   return 0;
}

static int Script_Buffer___index(lua_State* L) {
   if (lua_isnumber(L, 2)) {
      return Script_Buffer_line(L);
   } else {
      lua_getfield(L, LUA_REGISTRYINDEX, "Buffer_functions");
      lua_pushvalue(L, -2); // push key (function name)
      lua_gettable(L, -2);  // get it from Buffer_functions
      lua_remove(L, -2);    // remove Buffer_functions table from stack
      return 1;
   }
}

static int Script_Buffer___newindex(lua_State* L) {
   Buffer* buffer = (Buffer*) ((Proxy*)luaL_checkudata(L, 1, "Buffer"))->ptr;
   if (lua_gettop(L) == 3) {
      int y = luaL_checkinteger(L, 2) - 1;
      luaL_checkstring(L, 3);
      size_t len;
      const char* text = lua_tolstring(L, 3, &len);
      Buffer_setLine(buffer, y, Text_new((char*) text));
   }
   return 0;
}

static int Script_Buffer___len(lua_State* L) {
   Buffer* buffer = (Buffer*) ((Proxy*)luaL_checkudata(L, 1, "Buffer"))->ptr;
   lua_pushinteger(L, Panel_size(buffer->panel));
   return 1;
}

static void Script_Buffer_new(lua_State* L, void* ptr) {
   Proxy* proxy = lua_newuserdata(L, sizeof(Proxy));
   proxy->ptr = ptr;
   if (luaL_newmetatable(L, "Buffer")) {
      lua_pushcfunction(L, Script_Buffer___index);
      lua_setfield(L, -2, "__index");
      lua_pushcfunction(L, Script_Buffer___newindex);
      lua_setfield(L, -2, "__newindex");
      lua_pushcfunction(L, Script_Buffer___len);
      lua_setfield(L, -2, "__len");
   }
   lua_setmetatable(L, -2);
}

static int Script_TabManager_open(lua_State* L) {
   TabManager* tabs = (TabManager*) ((Proxy*)luaL_checkudata(L, 1, "TabManager"))->ptr;
   const char* name = luaL_checkstring(L, 2);
   
   int page = Dit_open(tabs, name);
   
   lua_pushinteger(L, page);
   return 1;
}

static int Script_TabManager_setPage(lua_State* L) {
   TabManager* tabs = (TabManager*) ((Proxy*)luaL_checkudata(L, 1, "TabManager"))->ptr;
   int page = luaL_checkinteger(L, 2);
   
   TabManager_setPage(tabs, page);
   
   return 0;
}

static int Script_TabManager_markJump(lua_State* L) {
   TabManager* tabs = (TabManager*) ((Proxy*)luaL_checkudata(L, 1, "TabManager"))->ptr;
   
   TabManager_markJump(tabs);
   
   return 0;
}

static int Script_TabManager_getBuffer(lua_State* L) {
   TabManager* tabs = (TabManager*) ((Proxy*)luaL_checkudata(L, 1, "TabManager"))->ptr;
   int page = luaL_checkinteger(L, 2);
   
   Buffer* buffer = TabManager_getBuffer(tabs, page);
   if (!buffer)
      return 0;
   Script_Buffer_new(L, buffer);
   return 1;
}

static luaL_Reg TabManager_functions[] = {
   { "open", Script_TabManager_open },
   { "setPage", Script_TabManager_setPage },
   { "getBuffer", Script_TabManager_getBuffer },
   { "markJump", Script_TabManager_markJump },
   { NULL, NULL }
};

static int Script_string___index(lua_State* L) {
   if (lua_isnumber(L, 2)) {
      int at = lua_tointeger(L, 2);
      size_t len;
      char out[2];
      const char* str = lua_tolstring(L, 1, &len);
      if (at < 0) {
         at += len + 1;
      }
      if (at > len || at < 1) {
         lua_pop(L, 2);
         lua_pushliteral(L, "");
         return 1;
      } else {
         out[0] = str[at-1];
         out[1] = '\0';
         lua_pop(L, 2);
         lua_pushlstring(L, out, 1);
      }
      return 1;
   } else {
      lua_getglobal(L, "string");
      lua_pushvalue(L, -2); // push key (function name)
      lua_gettable(L, -2);  // get it from string functions
      lua_remove(L, -2);    // remove string functions table from stack
      return 1;
   }
}

static luaL_Reg Buffer_functions[] = {
   // getters:
   { "line", Script_Buffer_line },
   { "selection", Script_Buffer_selection },
   { "token", Script_Buffer_token },
   { "xy", Script_Buffer_xy },
   { "dir", Script_Buffer_dir },
   { "basename", Script_Buffer_basename },
   { "filename", Script_Buffer_filename },
   // actions:
   { "go_to", Script_Buffer_goto },
   { "select", Script_Buffer_select },
   { "begin_undo", Script_Buffer_beginUndoGroup },
   { "end_undo", Script_Buffer_endUndoGroup },
   { NULL, NULL }
};

void Script_initState(ScriptState* state, TabManager* tabs, Buffer* buffer) {
   lua_State* L = luaL_newstate();
   state->L = L;
   luaL_openlibs(L);

   lua_pushstring(L, "buffer");                 // push a random string
   lua_getmetatable(L, -1);                     // get metatable of strings
   lua_remove(L, -2);                           // remove random string
   lua_pushcfunction(L, Script_string___index); // push custom index fn
   lua_setfield(L, -2, "__index");              // set it in metatable
   lua_pop(L, 1);                               // pop metatable

   lua_pushstring(L, "Buffer_functions");
   luaL_newlib(L, Buffer_functions);
   lua_settable(L, LUA_REGISTRYINDEX);

   Script_pushObject(L, tabs, "TabManager", TabManager_functions);
   lua_setglobal(L, "tabs");
   Script_Buffer_new(L, buffer);
   lua_setglobal(L, "buffer");
}

void Script_doneState(ScriptState* state) {
   lua_close(state->L);
}

bool Script_load(ScriptState* this, const char* scriptName) {
   lua_State* L = this->L;
   int dirEndsAt;
   char* foundFile = Files_findFile("scripts/%s", scriptName, &dirEndsAt);
   if (!foundFile) {
      return false;
   }
   char* foundDir = strdup(foundFile);
   foundDir[dirEndsAt] = '\0';   

   lua_getglobal(L, "package");
   lua_getfield(L, -1, "path");
   size_t len;
   const char* oldPackagePath = lua_tolstring(L, -1, &len);
   len += strlen(foundDir) + 18; // ";scripts/?.lua\0"
   char newPackagePath[len];
   snprintf(newPackagePath, len-1, "%s;%sscripts/?.lua", oldPackagePath, foundDir);
   lua_pop(L, 1);
   lua_pushstring(L, newPackagePath);
   lua_setfield(L, -2, "path");
   free(foundDir);
   
   int err = luaL_dofile(L, foundFile);
   free(foundFile);
   if (err != 0) {
      Display_errorScreen("Error loading script %s", lua_tostring(L, -1));
      return false;
   }
   return true;
}

static int errorHandler(lua_State* L) {
   lua_pushliteral(L, "\n");
   lua_getglobal(L, "debug");
   lua_getfield(L, -1, "traceback");
   lua_call(L, 0, 1);
   lua_remove(L, -2);
   lua_concat(L, 3);
   return 1;
}

static inline bool callFunction(lua_State* L, const char* fn, const char* arg) {
   lua_pushcfunction(L, errorHandler);
   int errFunc = lua_gettop(L);
   lua_getglobal(L, fn);
   if (!lua_isfunction(L, -1))
      return false;
   if (arg)
      lua_pushstring(L, arg);
   int err = lua_pcall(L, arg ? 1 : 0, 0, errFunc);
   if (err) error(L);
   lua_pop(L, lua_gettop(L));
   return (err == 0);
}

void Script_highlightFile(Highlight* this, const char* fileName) {
   if (!this->hasScript)
      return;
   lua_State* L = this->script->L;
   this->hasScript = callFunction(L, "highlight_file", fileName);
}

void Script_highlightLine(Highlight* this, const char* buffer, int* attrs, int len, int y) {
   lua_State* L = this->script->L;
   if (!this->hasScript)
      return;
   lua_pushcfunction(L, errorHandler);
   int errFunc = lua_gettop(L);
   lua_getglobal(L, "highlight_line");
   if (!lua_isfunction(L, -1)) {
      this->hasScript = false;
      return;
   }
   lua_pushstring(L, buffer);
   lua_pushinteger(L, y);
   int err = lua_pcall(L, 2, 1, errFunc);
   if (err == 0) {
      if (lua_gettop(L) > 0 && lua_isstring(L, -1)) {
         const char* ret = lua_tostring(L, -1);
         for (int i = 0; ret[i] && i < len; i++) {
            if (ret[i] != ' ') {
               int attr = CRT_colors[Highlight_translateColorKey(ret[i])];
               attrs[i] = attr;
            }
         }
      }
   } else {
      if (err) error(L);
   }
   lua_pop(L, lua_gettop(L));
}

void Script_onKey(Buffer* this, int key) {
   lua_State* L = this->script.L;
   if (this->skipOnKey) return;
   
   lua_pushcfunction(L, errorHandler);
   int errFunc = lua_gettop(L);
   lua_getglobal(L, "on_key");
   if (!lua_isfunction(L, -1)) {
      this->skipOnKey = true;
      return;
   }
   lua_pushinteger(L, key);
   int err = lua_pcall(L, 1, 0, errFunc);
   if (err) error(L);
   lua_pop(L, lua_gettop(L));
}

void Script_onCtrl(Buffer* this, int key) {
   if (this->skipOnCtrl) return;
   
   lua_State* L = this->script.L;
   char ch[2] = { 'A' + key - 1, '\0' };
   this->skipOnCtrl = !callFunction(L, "on_ctrl", ch);
}

void Script_onFKey(Buffer* this, int key) {
   if (this->skipOnFKey) return;
   
   lua_State* L = this->script.L;
   char ch[10];
   snprintf(ch, 10, "F%d", key - KEY_F(1) + 1);
   this->skipOnFKey = !callFunction(L, "on_fkey", ch);
}

void Script_onSave(Buffer* this, const char* fileName) {
   if (this->skipOnSave) return;
   
   lua_State* L = this->script.L;
   this->skipOnSave = !callFunction(L, "on_save", fileName);
}

void Script_onChange(Buffer* this) {
   if (this->skipOnChange) return;
   
   lua_State* L = this->script.L;
   this->skipOnChange = !callFunction(L, "on_change", NULL);
}
