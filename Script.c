
#include "Prototypes.h"

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

static inline bool callFunctionString(lua_State* L, const char* fn, const char* arg) {
   lua_getglobal(L, fn);
   if (!lua_isfunction(L, -1))
      return false;
   lua_pushstring(L, arg);
   int err = lua_pcall(L, 1, 0, 0);
   // TODO ignore errors for now
   lua_pop(L, lua_gettop(L));
   return (err == 0);
}

void Script_highlightFile(Highlight* this, const char* fileName) {
   if (!this->hasScript)
      return;
   this->hasScript = callFunctionString(this->L, "highlight_file", fileName);
}

void Script_highlightLine(Highlight* this, unsigned char* buffer, int* attrs, int len, int y) {
   lua_State* L = this->L;
   if (!this->hasScript)
      return;
   lua_getglobal(L, "highlight_line");
   if (!lua_isfunction(L, -1)) {
      this->hasScript = false;
      return;
   }
   lua_pushstring(L, buffer);
   lua_pushinteger(L, y);
   int err = lua_pcall(L, 2, 1, 0);
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
      // TODO ignore errors for now
   }
   lua_pop(L, lua_gettop(L));
}

void Script_onChange(Buffer* this) {
   if (!this->onChange)
      return;
   lua_getglobal(this->L, "on_change");
   if (!lua_isfunction(this->L, -1)) {
      this->onChange = false;
      return;
   }
   int err = lua_pcall(this->L, 0, 0, 0);
   // TODO ignore errors for now
   lua_pop(this->L, lua_gettop(this->L));
}

void Script_onSave(Buffer* this, const char* fileName) {
   if (!this->onSave)
      return;
   this->onSave = callFunctionString(this->L, "on_save", fileName);
}
