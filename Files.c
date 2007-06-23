
#include <stdio.h>

#include "Prototypes.h"

/*{

typedef bool(*Method_Files_fileHandler)(void*, char*);
  
}*/

FILE* Files_open(char* mode, char* picture, char* value) {
   char fileName[4097];
   char* dataDir = PKGDATADIR;
   char* sysconfDir = SYSCONFDIR;
   FILE* fd = Files_openHome(mode, picture, value);
   if (fd) return fd;

   snprintf(fileName, 4096, "%s/dit/", sysconfDir);
   int len = strlen(fileName);
   snprintf(fileName + len, 4096 - len, picture, value);
   fd = fopen(fileName, mode);
   if (fd) return fd;

   snprintf(fileName, 4096, "%s/", dataDir);
   len = strlen(fileName);
   snprintf(fileName + len, 4096 - len, picture, value);
   fd = fopen(fileName, mode);

   return fd;
}

FILE* Files_openHome(char* mode, char* picture, char* value) {
   FILE* fd;
   char fileName[4097];
   char* homeDir = getenv("HOME");
   snprintf(fileName, 4096, "%s/.dit/", homeDir);
   int len = strlen(fileName);
   snprintf(fileName + len, 4096 - len, picture, value);
   fd = fopen(fileName, mode);
   return fd;
}

void Files_makeHome() {
   char fileName[4097];
   snprintf(fileName, 4096, "%s/.dit", getenv("HOME"));
   fileName[4095] = '\0';
   mkdir(fileName, 0755);
   snprintf(fileName, 4096, "%s/.dit/undo", getenv("HOME"));
   fileName[4095] = '\0';
   mkdir(fileName, 0755);
}

void Files_forEachInDir(char* dirName, Method_Files_fileHandler fileHandler, void* data) {
   bool result = false;
   FILE* fd;
   char homeName[4097];
   char* homeDir = getenv("HOME");
   snprintf(homeName, 4096, "%s/.dit/", homeDir);
   int homeLen = strlen(homeName);
   snprintf(homeName + homeLen, 4096 - homeLen, "%s", dirName);
   DIR* dir = opendir(homeName);
   while (dir) {
      struct dirent* entry = readdir(dir);
      if (!entry) break;
      if (entry->d_name[0] == '.') continue;
      snprintf(homeName + homeLen, 4096 - homeLen, "%s/%s", dirName, entry->d_name);
      if (fileHandler(data, homeName)) {
         if (dir) closedir(dir);
         return;
      }
   }
   char dataName[4097];
   char* dataDir = PKGDATADIR;
   snprintf(dataName, 4096, "%s/", dataDir);
   int dataLen = strlen(dataName);
   snprintf(dataName + dataLen, 4096 - homeLen, "%s", dirName);
   dir = opendir(dataName);
   while (dir) {
      struct dirent* entry = readdir(dir);
      if (!entry) break;
      if (entry->d_name[0] == '.') continue;
      snprintf(homeName + homeLen, 4096 - homeLen, "%s/%s", dirName, entry->d_name);
      if (access(homeName, R_OK) == 0)
         continue;
      snprintf(dataName + dataLen, 4096 - dataLen, "%s/%s", dirName, entry->d_name);
      if (fileHandler(data, dataName)) {
         if (dir) closedir(dir);
         return;
      }
   }
}
