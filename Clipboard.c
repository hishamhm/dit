
#include "Prototypes.h"

/*{

struct Clipboard_ {
   char clipFileName[128];
   bool disk;
   char* text;
   int len;
};

}*/

Clipboard* Clipboard_new() {
   Clipboard* this = (Clipboard*) malloc(sizeof(Clipboard));
   sprintf(this->clipFileName, "%s/.clipboard", getenv("HOME"));
   this->text = NULL;
   this->disk = true;
   return this;
}

void Clipboard_delete(Clipboard* this) {
   if (this->text)
      free(this->text);
   free(this);
}

Text Clipboard_get(Clipboard* this) {
   if (this->disk) {
      FILE* fd = fopen(this->clipFileName, "r");
      if (fd) {
         int size = 100;
         char* out = malloc(size);
         int len = 0;
         while (!feof(fd)) {
            if (size - len < 100) {
               size = len + 100;
               out = realloc(out, size+1);
            }
            char* walk = out + len;
            int amt = fread(walk, 1, 100, fd);
            len += amt;
         }
         out[len] = '\0';
         fclose(fd);
         return Text_new(out);
      }
      this->disk = false;
   }
   if (this->text) {
      return Text_new(strdup(this->text));
   }
   return Text_null();
}

void Clipboard_set(Clipboard* this, char* text, int len) {
   this->len = len;
   if (this->disk) {
      FILE* fd = fopen(this->clipFileName, "w");
      if (fd) {
         int pend = len;
         while (pend > 0) {
            int wrote = fwrite(text + (len - pend), 1, pend, fd);
            pend -= wrote;
         }
         fclose(fd);
         free(text);
         return;
      }
      this->disk = false;
   }
   this->text = text;
}

