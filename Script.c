
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
