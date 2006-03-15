
#define _GNU_SOURCE
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <regex.h>

#include "Prototypes.h"
//#needs Object

/*{

extern char* HIGHLIGHTRULE_CLASS;

typedef enum {
   NormalColor = 0,
   SelectionColor,
   BracketColor,
   BrightColor,
   SymbolColor,
   BrightSymbolColor,
   AltColor,
   BrightAltColor,
   DiffColor,
   BrightDiffColor,
   SpecialColor,
   BrightSpecialColor,
   VerySpecialColor,
   DimColor,
   HighlightColors
} HighlightColor;

struct HighlightContext_ {
   Object super;
   int id;
   PatternMatcher* follows;
   HighlightContext* nextLine;
   HighlightColor defaultColor;
   PatternMatcher* rules;
};

extern char* HIGHLIGHTCONTEXT_CLASS;

struct Highlight_ {
   Vector* contexts;
   int colors[HighlightColors];
   HighlightContext* mainContext;
   HighlightContext* currentContext;
   // PatternMatcher* words;
   bool toLower;
};

#ifndef isword
#define isword(x) (isalpha(x) || x == '_')
#endif

//#define ANTARCTIC_THEME
//#define CLASSIC_TURBO_THEME
//#define BLACK_TURBO_THEME

#ifdef BLACK_TURBO_THEME
#define NormalColor (A_NORMAL)
#define SelectionColor (A_REVERSE | CRT_color(Blue, White))
#define BracketColor (A_REVERSE | CRT_color(Cyan, Black))
#define BrightColor (A_BOLD | CRT_color(Yellow, Black))
#define SymbolColor (A_BOLD | CRT_color(Cyan, Black))
#define BrightSymbolColor (A_BOLD | CRT_color(Yellow, Black))
#define AltColor (CRT_color(Green, Black))
#define BrightAltColor (A_BOLD | CRT_color(Green, Black))
#define DiffColor (CRT_color(Cyan, Black))
#define BrightDiffColor (A_BOLD | CRT_color(Cyan, Black))
#define SpecialColor (CRT_color(Red, Black))
#define BrightSpecialColor (A_BOLD | CRT_color(Red, Black))
#define VerySpecialColor (A_BOLD | CRT_color(Yellow, Red))
#define DimColor (CRT_color(Yellow, Black))
#endif

#ifdef CLASSIC_TURBO_THEME
#define NormalColor (CRT_color(White, Blue))
#define SelectionColor (A_REVERSE | CRT_color(Cyan, Black))
#define BracketColor (A_REVERSE | CRT_color(Green, Black))
#define BrightColor (A_BOLD | CRT_color(Yellow, Blue))
#define SymbolColor (A_BOLD | CRT_color(Cyan, Blue))
#define BrightSymbolColor (A_BOLD | CRT_color(Yellow, Blue))
#define AltColor (CRT_color(Green, Blue))
#define BrightAltColor (A_BOLD | CRT_color(Green, Blue))
#define DiffColor (CRT_color(Cyan, Blue))
#define BrightDiffColor (A_BOLD | CRT_color(Cyan, Blue))
#define SpecialColor (CRT_color(Red, Blue))
#define BrightSpecialColor (A_BOLD | CRT_color(Red, Blue))
#define VerySpecialColor (A_BOLD | CRT_color(Yellow, Red))
#define DimColor (CRT_color(Yellow, Blue))
#endif

#ifdef ANTARCTIC_THEME
#define NormalColor (A_NORMAL)
#define SelectionColor (A_REVERSE | CRT_color(Blue, White))
#define BracketColor (A_REVERSE | CRT_color(Cyan, Black))
#define BrightColor (A_BOLD | CRT_color(White, Black))
#define SymbolColor (A_BOLD | CRT_color(White, Black))
#define BrightSymbolColor (A_BOLD | CRT_color(Cyan, Black))
#define AltColor (CRT_color(Cyan, Black))
#define BrightAltColor (A_BOLD | CRT_color(Cyan, Black))
#define DiffColor (CRT_color(Green, Black))
#define BrightDiffColor (A_BOLD | CRT_color(Green, Black))
#define SpecialColor (CRT_color(Yellow, Black))
#define BrightSpecialColor (A_BOLD | CRT_color(Yellow, Black))
#define VerySpecialColor (A_BOLD | CRT_color(Yellow, Red))
#define DimColor (A_BOLD | CRT_color(Black, Black))
#endif

}*/

/* private property */
char* HIGHLIGHTCONTEXT_CLASS = "HighlightContext";

/* private property */
char* HIGHLIGHTRULE_CLASS = "HighlightRule";

static HighlightColor Highlight_translateColor(char* color) {
   if (String_eq(color, "bright")) return BrightColor;
   if (String_eq(color, "symbol")) return SymbolColor;
   if (String_eq(color, "brightsymbol")) return BrightSymbolColor;
   if (String_eq(color, "alt")) return AltColor;
   if (String_eq(color, "brightalt")) return BrightAltColor;
   if (String_eq(color, "diff")) return DiffColor;
   if (String_eq(color, "brightdiff")) return BrightDiffColor;
   if (String_eq(color, "special")) return SpecialColor;
   if (String_eq(color, "brightspecial")) return BrightSpecialColor;
   if (String_eq(color, "veryspecial")) return VerySpecialColor;
   if (String_eq(color, "dim")) return DimColor;
   return NormalColor;
}

Highlight* Highlight_new(const char* fileName, const char* firstLine) {
   Highlight* this = (Highlight*) malloc(sizeof(Highlight));
   // this->words = PatternMatcher_new();
   this->colors[NormalColor] = (A_NORMAL);
   this->colors[SelectionColor] = (A_REVERSE | CRT_color(Blue, White));
   this->colors[BracketColor] = (A_REVERSE | CRT_color(Cyan, Black));
   this->colors[BrightColor] = (A_BOLD | CRT_color(White, Black));
   this->colors[SymbolColor] = (A_BOLD | CRT_color(White, Black));
   this->colors[BrightSymbolColor] = (A_BOLD | CRT_color(Cyan, Black));
   this->colors[AltColor] = (CRT_color(Cyan, Black));
   this->colors[BrightAltColor] = (A_BOLD | CRT_color(Cyan, Black));
   this->colors[DiffColor] = (CRT_color(Green, Black));
   this->colors[BrightDiffColor] = (A_BOLD | CRT_color(Green, Black));
   this->colors[SpecialColor] = (CRT_color(Yellow, Black));
   this->colors[BrightSpecialColor] = (A_BOLD | CRT_color(Yellow, Black));
   this->colors[VerySpecialColor] = (A_BOLD | CRT_color(Yellow, Red));
   this->colors[DimColor] = (A_BOLD | CRT_color(Black, Black));

   this->contexts = Vector_new(HIGHLIGHTCONTEXT_CLASS, true, DEFAULT_SIZE);
   this->currentContext = NULL;
   char highlightPath[4096];
   snprintf(highlightPath, 4095, "%s/.e/highlight", getenv("HOME"));
   highlightPath[4095] = '\0';
   DIR* dir = opendir(highlightPath);
   while (dir) {
      struct dirent* entry = readdir(dir);
      if (!entry) break;
      if (entry->d_name[0] == '.') continue;
      snprintf(highlightPath, 4095, "%s/.e/highlight/%s", getenv("HOME"), entry->d_name);
      highlightPath[4095] = '\0';
      FILE* file = fopen(highlightPath, "r");
      if (!file) continue;
      char buffer[4096];
      int state = 1;
      bool success = true;
      this->mainContext = Highlight_addContext(this, NULL, NULL, NULL, NormalColor);
      HighlightContext* context = this->mainContext;
      Stack* contexts = Stack_new(HIGHLIGHTCONTEXT_CLASS, false);
      Stack_push(contexts, context, 0);
      int lineno = 0;
      this->toLower = false;
      while (success && !feof(file)) {
         fgets(buffer, 4095, file);
         lineno++;
         char* ch = strchr(buffer, '\n');
         if (ch) *ch = '\0';
         ch = buffer;
         while (!(isalpha(*ch) || *ch == '\0')) ch++;
         if (*ch == '\0') continue;

         buffer[4095] = '\0';
         char** tokens = String_split(buffer, ' ');
         int ntokens;
         for (ntokens = 0; tokens[ntokens]; ntokens++);
         if (ntokens > 0 && tokens[0][0] != '#')
         switch (state) {
         case 1:
         {
            // Read FILES header
            if (!String_eq(tokens[0], "FILES") && ntokens == 1)
               success = false;
            state = 2;
            break;
         }
         case 2:
         {
            // Try to match FILES rule
            const char* subject = NULL;
            if (String_eq(tokens[0],"name")) {
               subject = fileName;
               char* lastSlash = strrchr(subject, '/');
               if (lastSlash)
                  subject = lastSlash + 1;
            } else if (String_eq(tokens[0], "firstline"))
               subject = firstLine;
            else
               success = false;
            if (success) {
               if (String_eq(tokens[1], "prefix") && ntokens == 3) {
                  if (String_startsWith(subject, tokens[2]))
                     state = 3;
               } else if (String_eq(tokens[1], "suffix") && ntokens == 3) {
                  if (String_endsWith(subject, tokens[2]))
                     state = 3;
               } else if (String_eq(tokens[1], "regex") && ntokens == 3) {
                  regex_t magic;
                  regcomp(&magic, tokens[2], REG_EXTENDED | REG_NOSUB);
                  if (regexec(&magic, firstLine, 0, NULL, 0) == 0)
                     state = 3;
                  regfree(&magic);
               } else {
                  success = false;
               }
            }
            break;
         }
         case 3:
         {
            // FILES match succeeded. Skip over other FILES section,
            // waiting for RULES section
            if (String_eq(tokens[0], "RULES") && ntokens == 1)
               state = 4;
            break;
         }
         case 4:
         {
            // Read RULES section
            if (String_eq(tokens[0], "context") && (ntokens == 4 || ntokens == 6)) {
               char* open = tokens[1];
               char* close = (String_eq(tokens[2], "`$") ? NULL : tokens[2]);
               HighlightColor color;
               if (ntokens == 6) {
                  HighlightContext_addRule(context, open, Highlight_translateColor(tokens[3]));
                  color = Highlight_translateColor(tokens[5]);
               } else {
                  color = Highlight_translateColor(tokens[3]);
                  HighlightContext_addRule(context, open, color);
               }
               context = Highlight_addContext(this, open, close, Stack_peek(contexts, NULL), color);
               if (close) {
                  color = (ntokens == 6 ? Highlight_translateColor(tokens[4]) : color);
                  HighlightContext_addRule(context, close, color);
               }
               Stack_push(contexts, context, 0);
            } else if (String_eq(tokens[0], "/context") && ntokens == 1) {
               if (contexts->size > 1) {
                  Stack_pop(contexts, NULL);
                  context = Stack_peek(contexts, NULL);
               }
            } else if (String_eq(tokens[0], "rule") && ntokens == 3) {
               HighlightContext_addRule(context, tokens[1], Highlight_translateColor(tokens[2]));
            } else if (String_eq(tokens[0], "insensitive") && ntokens == 1) {
               this->toLower = true;
            } else {
               mvprintw(0,0,"Error reading %s: line %d: %s", highlightPath, lineno, buffer);
               getch();
               success = false;
            }
            break;
         }
         }
         for (int i = 0; tokens[i]; i++)
            free(tokens[i]);
         free(tokens);
      }
      fclose(file);
      if (contexts->size != 1) {
         mvprintw(0,0,"Error reading %s: %d context%s still open", highlightPath, contexts->size - 1, contexts->size > 2 ? "s" : "");
         getch();
         success = false;
      }
      Stack_delete(contexts);
      if (success && state == 4) {
         this->currentContext = this->mainContext;
         break;
      } else {
         Vector_prune(this->contexts);
      }
   }
   if (dir) {
      closedir(dir);
   }
   if (!this->currentContext) {
      this->mainContext = Highlight_addContext(this, NULL, NULL, NULL, NormalColor);
      this->currentContext = this->mainContext;
   }
   return this;
}

void Highlight_delete(Highlight* this) {
   Vector_delete(this->contexts);
   free(this);
}

HighlightContext* Highlight_addContext(Highlight* this, char* open, char* close, HighlightContext* parent, HighlightColor color) {
   int id = Vector_size(this->contexts);
   HighlightContext* ctx = HighlightContext_new(id, color);
   Vector_add(this->contexts, ctx);
   if (open) {
      assert(parent);
      PatternMatcher_add(parent->follows, (unsigned char*) open, (int) ctx);
   }
   if (close) {
      assert(parent);
      PatternMatcher_add(ctx->follows, (unsigned char*) close, (int) parent);
   } else {
      if (parent)
         ctx->nextLine = parent;
   }
   return ctx;
}

/* private */
static inline int Highlight_tryMatch(Highlight* this, unsigned char* buffer, int* attrs, int at, GraphNode* rules, GraphNode* follows, Method_PatternMatcher_match match, HighlightContext** ctx, bool paintUnmatched) {
   unsigned char* here = buffer+at;
   int intColor;
   int matchlen = match(rules, here, &intColor);
   HighlightColor color = (HighlightColor) intColor;
   assert(color >= 0 && color < HighlightColors);
   int attr = this->colors[color];
   int word = isword(*here);
   if (matchlen && !(word && isword(here[matchlen]))) {
      for (int i = at; i < at+matchlen; i++)
         attrs[i] = attr;
      int nextCtx = 0;
      int followMatchlen = match(follows, here, &nextCtx);
      if (followMatchlen == matchlen) {
         assert(nextCtx);
         *ctx = (HighlightContext*) nextCtx;
      }
      at += matchlen;
   } else if (paintUnmatched) {
      int defaultAttr = this->colors[(*ctx)->defaultColor];
      if (word) {
         while (isword(buffer[at]))
            attrs[at++] = defaultAttr;
      } else {
         attrs[at++] = defaultAttr;
      }
   }
   return at;
}

void Highlight_setAttrs(Highlight* this, unsigned char* buffer, int* attrs, int len) {
   HighlightContext* ctx = this->currentContext;
   int at = 0;
   Method_PatternMatcher_match match;
   if (this->toLower)
      match = PatternMatcher_match_toLower;
   else
      match = PatternMatcher_match;
   //at = Highlight_tryMatch(this, buffer, attrs, at, ctx->rules->lineStart, ctx->follows->lineStart, match, &ctx, false);
   while (at < len) {
      at = Highlight_tryMatch(this, buffer, attrs, at, ctx->rules->start, ctx->follows->start, match, &ctx, true);
   }
   this->currentContext = ctx->nextLine;
}

inline HighlightContext* Highlight_getContext(Highlight* this) {
   return this->currentContext;
}

inline void Highlight_setContext(Highlight* this, HighlightContext* context) {
   this->currentContext = (HighlightContext*) context;
}

HighlightContext* HighlightContext_new(int id, HighlightColor defaultColor) {
   HighlightContext* this = (HighlightContext*) malloc(sizeof(HighlightContext));
   Object_init((Object*) this, HIGHLIGHTCONTEXT_CLASS);
   ((Object*)this)->delete = HighlightContext_delete;
   this->id = id;
   this->follows = PatternMatcher_new();
   this->defaultColor = defaultColor;
   this->rules = PatternMatcher_new();
   this->nextLine = this;
   return this;
}

void HighlightContext_delete(Object* cast) {
   HighlightContext* this = (HighlightContext*) cast;
   PatternMatcher_delete(this->follows);
   PatternMatcher_delete(this->rules);
   free(this);
}

void HighlightContext_addRule(HighlightContext* this, char* rule, HighlightColor color) {
   PatternMatcher_add(this->rules, (unsigned char*) rule, (int) color);
}
