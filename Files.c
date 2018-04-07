
#define _GNU_SOURCE
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

#include "Prototypes.h"

/*{

typedef bool(*Method_Files_fileHandler)(void*, char*);

struct ForEachData_ {
   Method_Files_fileHandler fn;
   void* data;
};

}*/

typedef void*(*Method_Files_tryEachHandler)(const char*, void*);

static char* makeFileName(char* fileName, const char* dir, const char* subdir, const char* picture, const char* value, int* dirEndsAt) {
   snprintf(fileName, 4096, "%s/%s", dir, subdir);
   int len = strlen(fileName);
   if (dirEndsAt) *dirEndsAt = len;
   snprintf(fileName + len, 4096 - len, picture, value);
   return fileName;
}

static void* tryEach(const char* picture, const char* value, int* dirEndsAt, void* arg, Method_Files_tryEachHandler fn) {
   char fileName[4097];
   void* ret = fn(makeFileName(fileName, getenv("HOME"), "/.dit/", picture, value, dirEndsAt), arg);
   if (ret) return ret;
   ret = fn(makeFileName(fileName, SYSCONFDIR, "/dit/", picture, value, dirEndsAt), arg);
   if (ret) return ret;
   ret = fn(makeFileName(fileName, PKGDATADIR, "/", picture, value, dirEndsAt), arg);
   if (ret) return ret;
   return fn(makeFileName(fileName, ".", "/", picture, value, dirEndsAt), arg);
}
 
static void* handleOpen(const char* name, void* mode) {
   return fopen(name, (const char*) mode);
}
 
FILE* Files_open(const char* mode, const char* picture, const char* value) {
   return (FILE*) tryEach(picture, value, NULL, (void*) mode, handleOpen);
}

static void* handleFindFile(const char* name, void* _) {
   return access(name, R_OK) == 0 ? strdup(name) : NULL;
}

char* Files_findFile(const char* picture, const char* value, int* dirEndsAt) {
   return (char*) tryEach(picture, value, dirEndsAt, NULL, handleFindFile);
}
    
static void Files_nameHome(char* fileName, const char* picture, const char* value) {
   int _;
   makeFileName(fileName, getenv("HOME"), "/.dit/", picture, value, &_);
}

bool Files_existsHome(const char* picture, const char* value) {
   char fileName[4097];
   Files_nameHome(fileName, picture, value);
   return (access(fileName, F_OK) == 0);
}

FILE* Files_openHome(const char* mode, const char* picture, const char* value) {
   char fileName[4097];
   Files_nameHome(fileName, picture, value);
   return fopen(fileName, mode);
}

int Files_deleteHome(const char* picture, const char* value) {
   char fileName[4097];
   Files_nameHome(fileName, picture, value);
   return unlink(fileName);
}

char* Files_encodePathAsFileName(char* fileName) {
   char* rpath;
   if (fileName[0] == '/')
      rpath = strdup(fileName);
   else
      rpath = realpath(fileName, NULL);
   for(char *c = rpath; *c; c++)
      if (*c == '/')
         *c = ':';
   return rpath;
}

void Files_makeHome() {
   static const char* dirs[] = {
      "%s/.dit",
      "%s/.dit/undo",
      "%s/.dit/lock",
      NULL
   };
   char fileName[4097];
   const char* home = getenv("HOME");
   for (int i = 0; dirs[i]; i++) {
      snprintf(fileName, 4096, dirs[i], home);
      fileName[4095] = '\0';
      mkdir(fileName, 0700);
   }
}

static void* handleForEachInDir(const char* dirName, void* arg) {
   const ForEachData* fed = (ForEachData*) arg;
   DIR* dir = opendir(dirName);
   while (dir) {
      struct dirent* entry = readdir(dir);
      if (!entry) break;
      if (entry->d_name[0] == '.') continue;
      char fileName[4097];
      makeFileName(fileName, dirName, "/", "%s", entry->d_name, NULL);
      if (fed->fn(fed->data, fileName)) {
         if (dir) closedir(dir);
         return &handleForEachInDir; // a non-nil pointer to tell tryEach to stop.
      }
   }
   if (dir) closedir(dir);
   return NULL;
}

void Files_forEachInDir(char* dirName, Method_Files_fileHandler fn, void* data) {
   ForEachData fed = { .fn = fn, .data = data };
   tryEach("%s", dirName, NULL, &fed, handleForEachInDir);
}
