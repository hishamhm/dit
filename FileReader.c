
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
   int used;
   char* start;
   bool eof;
   bool finalEnter;
   bool command;
};

}*/


FileReader* FileReader_new(char* filename, bool command) {
   FileReader* this = malloc(sizeof(FileReader));
   if (command) {
      this->fd = popen(filename, "r");
      this->command = true;
   } else {
      this->command = false;
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
   this->used = fread(this->buffer, 1, this->size, this->fd);
   this->eof = feof(this->fd);
   this->finalEnter = false;
   return this;
}

void FileReader_delete(FileReader* this) {
   if (this->command) {
      pclose(this->fd);
   } else {
      fclose(this->fd);
   }
   free(this->buffer);
   free(this);
}

bool FileReader_eof(FileReader* this) {
   return this->eof && !this->finalEnter && this->used == 0;
}

char* FileReader_readAllAndDelete(FileReader* this) {
   if (this->start > this->buffer) {
      memmove(this->buffer, this->start, this->used);
   }
   while (!this->eof) {
      int free = this->size - this->used;
      if (free == 0) {
         this->size *= 2;
         this->buffer = realloc(this->buffer, this->size);
      }
      this->used += fread(this->buffer + this->used, 1, this->size - this->used, this->fd);
      this->eof = feof(this->fd);
   }
   if (this->command) {
      pclose(this->fd);
   } else {
      fclose(this->fd);
   }
   char* buf = this->buffer;
   free(this);
   return buf;
}

char* FileReader_readLine(FileReader* this) {
   int chunkSize = 0;
   char* newline = NULL;
   while (true) {
      assert(this->start + this->used <= this->buffer + this->size);
      newline = memchr(this->start, '\n', this->used);
      if (newline) {
         chunkSize = newline - this->start;
         break;
      }
      assert(this->used <= this->size);
      if (!this->eof) {
         if (this->used) {
            if (this->used < this->size) {
               assert(this->start > this->buffer);
               memmove(this->buffer, this->start, this->used);
            } else {
               assert(this->start == this->buffer);
               this->size *= 2;
               this->buffer = realloc(this->buffer, this->size);
            }
         }
         this->start = this->buffer;
         this->used += fread(this->buffer + this->used, 1, this->size - this->used, this->fd);
         this->eof = feof(this->fd);
      } else {
         chunkSize = this->used;
         break;
      }
   }
   if (newline || chunkSize) {
      assert(chunkSize <= this->used);
      char* result = malloc(chunkSize + 1);
      memcpy(result, this->start, chunkSize);
      result[chunkSize] = '\0';
      if (newline)
         chunkSize++;
      this->start += chunkSize;
      this->used -= chunkSize;
      if (newline && this->eof && this->used == 0)
         this->finalEnter = true;
      return result;
   }
   if (this->finalEnter) {
      this->finalEnter = false;
      return strdup("");
   }
   return NULL;
}
