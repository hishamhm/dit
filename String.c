
#define _GNU_SOURCE
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "Prototypes.h"

int String_forwardWord(char* s, int len, int cursor) {
   char curr = s[cursor];
   if (isalnum(curr)) {
      while (isalnum(curr) && cursor < len)
         curr = s[++cursor];
   } else if (!isalnum(curr) && !isblank(curr)) {
      while (!isalnum(curr) && !isblank(curr) && cursor < len)
         curr = s[++cursor];
   } else {
      while (isblank(curr) && cursor < len)
         curr = s[++cursor];
   }
   return cursor;
}

int String_backwardWord(char* s, int len, int cursor) {
   char curr = s[cursor - 1];
   if (isalnum(curr)) {
      while (isalnum(curr) && cursor > 0)
         curr = s[--cursor - 1];
   } else if (!isalnum(curr) && !isblank(curr)) {
      while (!isalnum(curr) && !isblank(curr) && cursor > 0)
         curr = s[--cursor - 1];
   } else {
      while (isblank(curr) && cursor > 0)
         curr = s[--cursor - 1];
   }
   return cursor;
}

inline void String_delete(char* s) {
   assert(s);
   free(s);
}

char* String_cat(char* s1, char* s2) {
   int l1 = strlen(s1);
   int l2 = strlen(s2);
   char* out = malloc(l1 + l2 + 1);
   strncpy(out, s1, l1);
   strncpy(out+l1, s2, l2+1);
   return out;
}

char* String_trim(char* in) {
   while (in[0] == ' ' || in[0] == '\t' || in[0] == '\n') {
      in++;
   }
   int len = strlen(in);
   while (len > 0 && (in[len-1] == ' ' || in[len-1] == '\t' || in[len-1] == '\n')) {
      len--;
   }
   char* out = malloc(len+1);
   strncpy(out, in, len);
   out[len] = '\0';
   return out;
}

char* strdupUpTo(char* orig, char upTo) {
   int len;
   
   int origLen = strlen(orig);
   char* at = index(orig, upTo);
   if (at != NULL)
      len = at - orig;
   else
      len = origLen;
   char* copy = (char*) malloc(len+1);
   strncpy(copy, orig, len);
   copy[len] = '\0';
   return copy;
}

char* String_sub(char* orig, int from, int to) {
   char* copy;
   int len;
   
   len = strlen(orig);
   if (to > len)
      to = len;
   if (from > len)
      to = len;
   len = to-from+1;
   copy = (char*) malloc(len+1);
   strncpy(copy, orig+from, len);
   copy[len] = '\0';
   return copy;
}

void String_println(char* s) {
   printf("%s\n", s);
}

void String_print(char* s) {
   printf("%s", s);
}

void String_printInt(int i) {
   printf("%i", i);
}

void String_printPointer(void* p) {
   printf("%p", p);
}

inline int String_eq(const char* s1, const char* s2) {
   if (s1 == NULL || s2 == NULL) {
      if (s1 == NULL && s2 == NULL)
         return 1;
      else
         return 0;
   }
   return (strcmp(s1, s2) == 0);
}

inline int String_startsWith(const char* s, const char* match) {
   return (strstr(s, match) == s);
}

inline int String_endsWith(const char* s, const char* match) {
   int slen = strlen(s);
   int matchlen = strlen(match);
   if (matchlen > slen) return 0;
   return String_startsWith(s+(slen-matchlen), match);
}

static inline char* String_skipSep(char* start, char sep, bool positive) {
   if (sep)
      while (*start && ((*start == sep) == positive)) start++;
   else
      while (*start && (isspace(*start) == positive)) start++;
   return start;
}

char** String_split(char* s, char sep) {
   const int rate = 10;
   char** out = (char**) malloc(sizeof(char*) * rate);
   int ctr = 0;
   int blocks = rate;
   char* start = s;
   start = String_skipSep(start, sep, true);
   while (*start) {
      char* where = start;
      where = String_skipSep(where, sep, false);
      int size = where - start;
      char* token = (char*) malloc(size + 1);
      strncpy(token, start, size);
      token[size] = '\0';
      out[ctr] = token;
      ctr++;
      if (ctr == blocks) {
         blocks += rate;
         out = (char**) realloc(out, sizeof(char*) * blocks);
      }
      if (!*where)
         break;
      where++;
      where = String_skipSep(where, sep, true);
      start = where;
   }
   out = realloc(out, sizeof(char*) * (ctr + 1));
   out[ctr] = NULL;
   return out;
}

void String_freeArray(char** s) {
   for (int i = 0; s[i] != NULL; i++) {
      free(s[i]);
   }
   free(s);
}

int String_startsWith_i(char* s, char* match) {
   return (strncasecmp(s, match, strlen(match)) == 0);
}

/* deprecated */
int String_contains_i(char* s, char* match) {
   int lens = strlen(s);
   int lenmatch = strlen(match);
   for (int i = 0; i < (lens-lenmatch)+1; i++) {
      if (strncasecmp(s, match, lenmatch) == 0)
         return 1;
      s++;
   }
   return 0;
}

/* deprecated */
int String_indexOf_i(char* s, char* match, int lens) {
   int lenmatch = strlen(match);
   for (int i = 0; i < (lens-lenmatch)+1; i++) {
      if (strncasecmp(s, match, lenmatch) == 0)
         return i;
      s++;
   }
   return -1;
}

/* deprecated */
int String_indexOf(char* s, char* match, int lens) {
   int lenmatch = strlen(match);
   for (int i = 0; i < (lens-lenmatch)+1; i++) {
      if (strncmp(s, match, lenmatch) == 0)
         return i;
      s++;
   }
   return -1;
}

void String_convertEscape(char* s, char* escape, char value) {
   char* esc;
   if ((esc = strstr(s, escape))) {
      int len = strlen(escape);
      *esc = value;
      for (char* c = esc+len;;c++) {
         *(c-(len-1)) = *c;
         if (!*c) break;
      }
   }
}
