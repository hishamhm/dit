
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>

#include "Prototypes.h"

/*{

struct FileReader_ {
   FILE* fd;
   char* buffer;
   int size;
   int available;
   char* start;
   bool eof;
   bool finalEnter;
};

}*/


FileReader* FileReader_new(char* filename, bool command) {
   FileReader* this = malloc(sizeof(FileReader));
   if (command) {
      this->fd = popen(filename, "r");
   } else {
      struct stat st = {0};
      stat(filename, &st);
      if (S_ISDIR(st.st_mode)) {
         free(this);
         return NULL;
      }
      this->fd = fopen(filename, "r");
   }
   if (this->fd == NULL) {
      free(this);
      return NULL;
   }
   this->size = 600;
   this->buffer = malloc(this->size);
   this->start = this->buffer;
   this->available = fread(this->buffer, 1, this->size, this->fd);
   this->eof = feof(this->fd);
   this->finalEnter = false;
   return this;
}

void FileReader_delete(FileReader* this) {
   fclose(this->fd);
   free(this->buffer);
   free(this);
}

bool FileReader_eof(FileReader* this) {
   return this->eof && !this->finalEnter && this->available == 0;
}

char* FileReader_readLine(FileReader* this, int* len) {
   int chunkSize = 0;
   char* newline = NULL;
   while (true) {
      assert(this->start + this->available <= this->buffer + this->size);
      newline = memchr(this->start, '\n', this->available);
      if (newline) {
         chunkSize = newline - this->start;
         break;
      }
      assert(this->available <= this->size);
      if (!this->eof) {
         if (this->available) {
            if (this->available < this->size) {
               assert(this->start > this->buffer);
               memmove(this->buffer, this->start, this->available);
            } else {
               assert(this->start == this->buffer);
               this->size *= 2;
               this->buffer = realloc(this->buffer, this->size);
            }
         }
         this->start = this->buffer;
         this->available += fread(this->buffer + this->available, 1, this->size - this->available, this->fd);
         this->eof = feof(this->fd);
      } else {
         chunkSize = this->available;
         break;
      }
   }
   if (newline || chunkSize) {
      assert(chunkSize <= this->available);
      char* result = malloc(chunkSize + 1);
      memcpy(result, this->start, chunkSize);
      result[chunkSize] = '\0';
      *len = chunkSize;
      if (newline)
         chunkSize++;
      this->start += chunkSize;
      this->available -= chunkSize;
      if (newline && this->eof && this->available == 0)
         this->finalEnter = true;
      return result;
   }
   if (this->finalEnter) {
      this->finalEnter = false;
      *len = 0;
      return strdup("");
   }
   *len = 0;
   return NULL;
}
