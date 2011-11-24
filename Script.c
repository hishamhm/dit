
#include "Prototypes.h"

/*{

typedef struct Proxy_ {
   void* ptr;
} Proxy;

}*/

static void error(lua_State* L) {
   clear();
   mvprintw(0,0,"%s", lua_tostring(L, -1));
   getch();
   lua_getglobal(L, "tabs");
   TabManager* tabs = (TabManager*) ((Proxy*)lua_touserdata(L, -1))->ptr;
   TabManager_refreshCurrent(tabs);
   TabManager_draw(tabs, COLS);
}

static void Script_pushObject(lua_State* L, void* ptr, const char* klass, const luaL_Reg* functions) {
   Proxy* proxy = lua_newuserdata(L, sizeof(Proxy));
   proxy->ptr = ptr;
   if (luaL_newmetatable(L, klass)) {
      luaL_register(L, NULL, functions);
      lua_pushvalue(L, -1);
      lua_setfield(L, -2, "__index");
   }
   lua_setmetatable(L, -2);
}

static int Script_Buffer_goto(lua_State* L) {
   Buffer* buffer = (Buffer*) ((Proxy*)luaL_checkudata(L, 1, "Buffer"))->ptr;
   int x = luaL_checkint(L, 2);
   int y = luaL_checkint(L, 3);
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
      int y = luaL_checkint(L, 2);
      line = Buffer_getLine(buffer, y-1);
      if (line) lua_pushstring(L, line);
      else lua_pushliteral(L, "");
      return 1;
   }
}

static int Script_Buffer_select(lua_State* L) {
   Buffer* buffer = (Buffer*) ((Proxy*)luaL_checkudata(L, 1, "Buffer"))->ptr;
   int xFrom = luaL_checkint(L, 2);
   int yFrom = luaL_checkint(L, 3);
   int xTo = luaL_checkint(L, 4);
   int yTo = luaL_checkint(L, 5);
   // FIXME? validate these values
   buffer->selecting = true;
   buffer->selectXfrom = xFrom - 1;
   buffer->selectYfrom = yFrom - 1;
   buffer->selectXto = xTo - 1;
   buffer->selectYto = yTo - 1;
   buffer->panel->needsRedraw = true;
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
   { "goto", Script_Buffer_goto },
   { "select", Script_Buffer_select },
   { NULL, NULL }
};

static int Script_TabManager_open(lua_State* L) {
   TabManager* tabs = (TabManager*) ((Proxy*)luaL_checkudata(L, 1, "TabManager"))->ptr;
   const char* name = luaL_checkstring(L, 2);
   
   int page = Dit_open(tabs, name);
   
   lua_pushinteger(L, page);
   return 1;
}

static int Script_TabManager_setPage(lua_State* L) {
   TabManager* tabs = (TabManager*) ((Proxy*)luaL_checkudata(L, 1, "TabManager"))->ptr;
   int page = luaL_checkint(L, 2);
   
   TabManager_setPage(tabs, page);
   
   return 0;
}

static int Script_TabManager_getBuffer(lua_State* L) {
   TabManager* tabs = (TabManager*) ((Proxy*)luaL_checkudata(L, 1, "TabManager"))->ptr;
   int page = luaL_checkint(L, 2);
   
   Buffer* buffer = TabManager_getBuffer(tabs, page);
   if (!buffer)
      return 0;
   Script_pushObject(L, buffer, "Buffer", Buffer_functions);   
   return 1;
}

static luaL_Reg TabManager_functions[] = {
   { "open", Script_TabManager_open },
   { "setPage", Script_TabManager_setPage },
   { "getBuffer", Script_TabManager_getBuffer },
   { NULL, NULL }
};

lua_State* Script_newState(TabManager* tabs, Buffer* buffer) {
   lua_State* L = luaL_newstate();
   luaL_openlibs(L);

   Script_pushObject(L, tabs, "TabManager", TabManager_functions);
   lua_setglobal(L, "tabs");
   Script_pushObject(L, buffer, "Buffer", Buffer_functions);
   lua_setglobal(L, "buffer");

   return L;
}

bool Script_load(lua_State* L, const char* scriptName) {
   int dirEndsAt;
   char* foundFile = Files_findFile("scripts/%s", scriptName, &dirEndsAt);
   if (!foundFile) {
      return false;
   }
   char* foundDir = strdup(foundFile);
   foundDir[dirEndsAt] = '\0';   

   lua_getglobal(L, "package");
   lua_getfield(L, -1, "path");
   int len;
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
      clear();
      mvprintw(0,0,"Error loading script %s", lua_tostring(L, -1));
      getch();
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
   this->hasScript = callFunction(this->L, "highlight_file", fileName);
}

void Script_highlightLine(Highlight* this, unsigned char* buffer, int* attrs, int len, int y) {
   lua_State* L = this->L;
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
         int attr = CRT_colors[VerySpecialColor];
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
   if (this->skipOnKey) return;
   
   lua_pushcfunction(this->L, errorHandler);
   int errFunc = lua_gettop(this->L);
   lua_getglobal(this->L, "on_key");
   if (!lua_isfunction(this->L, -1)) {
      this->skipOnKey = true;
      return;
   }
   lua_pushinteger(this->L, key);
   int err = lua_pcall(this->L, 1, 0, errFunc);
   if (err) error(this->L);
   lua_pop(this->L, lua_gettop(this->L));
}

void Script_onCtrl(Buffer* this, int key) {
   if (this->skipOnCtrl) return;
   
   char ch[2] = { 'A' + key - 1, '\0' };
   this->skipOnCtrl = !callFunction(this->L, "on_ctrl", ch);
}

void Script_onSave(Buffer* this, const char* fileName) {
   if (this->skipOnSave) return;
   
   this->skipOnSave = !callFunction(this->L, "on_save", fileName);
}

void Script_onChange(Buffer* this) {
   if (this->skipOnChange) return;
   
   this->skipOnChange = !callFunction(this->L, "on_change", NULL);
}
