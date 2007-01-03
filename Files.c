
#include <stdio.h>

#include "Prototypes.h"

FILE* Files_open(char* mode, char* picture, char* value) {
   char fileName[4097];
   char* dataDir = DATADIR;
   FILE* fd = Files_openHome(mode, picture, value);
   if (fd) return fd;
   snprintf(fileName, 4096, "%s/", dataDir);
   int len = strlen(fileName);
   snprintf(fileName + len, 4096 - len, picture, value);
   return fopen(fileName, mode);
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
