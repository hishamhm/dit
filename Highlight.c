
#define _GNU_SOURCE
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <regex.h>
#include <stdbool.h>

#include "Prototypes.h"
//#needs Object

/*{

struct HighlightContextClass_ {
   ObjectClass super;
};

struct HighlightContext_ {
   Object super;
   int id;
   PatternMatcher* follows;
   HighlightContext* nextLine;
   Color defaultColor;
   PatternMatcher* rules;
};

extern HighlightContextClass HighlightContextType;

struct Highlight_ {
   Vector* contexts;
   HighlightContext* mainContext;
   HighlightContext* currentContext;
   bool toLower;
   lua_State* L;
   bool hasScript;
};

struct ReadHighlightFileArgs_ {
   Highlight* this;
   const char* fileName;
   const char* firstLine;
};

#ifndef isword
#define isword(x) (isalpha(x) || x == '_')
#endif

}*/

HighlightContextClass HighlightContextType = {
   .super = {
      .size = sizeof(HighlightContext),
      .display = NULL,
      .equals = Object_equals,
      .delete = HighlightContext_delete
   }
};

static Color Highlight_translateColor(char* color) {
   if (String_eq(color, "bright")) return BrightColor;
   if (String_eq(color, "symbol")) return SymbolColor;
   if (String_eq(color, "brightsymbol")) return BrightSymbolColor;
   if (String_eq(color, "alt")) return AltColor;
   if (String_eq(color, "brightalt")) return BrightAltColor;
   if (String_eq(color, "diff")) return DiffColor;
   if (String_eq(color, "brightdiff")) return BrightDiffColor;
   if (String_eq(color, "special")) return SpecialColor;
   if (String_eq(color, "brightspecial")) return BrightSpecialColor;
   if (String_eq(color, "specialdiff")) return SpecialDiffColor;
   if (String_eq(color, "brightspecialdiff")) return BrightSpecialDiffColor;
   if (String_eq(color, "veryspecial")) return VerySpecialColor;
   if (String_eq(color, "dim")) return DimColor;
   return NormalColor;
}

Highlight* Highlight_new(const char* fileName, const char* firstLine, lua_State* L) {
   Highlight* this = (Highlight*) malloc(sizeof(Highlight));
   this->L = L;
   this->hasScript = false;

   this->contexts = Vector_new(ClassAs(HighlightContext, Object), true, DEFAULT_SIZE);
   this->currentContext = NULL;
   
   ReadHighlightFileArgs args;
   args.this = this;
   args.fileName = fileName;
   args.firstLine = firstLine;
   Files_forEachInDir("highlight", (Method_Files_fileHandler)Highlight_readHighlightFile, &args);
   if (!this->currentContext) {
      this->mainContext = Highlight_addContext(this, NULL, NULL, NULL, NormalColor);
      this->currentContext = this->mainContext;
   }
   Script_highlightFile(this, fileName);
   return this;
}

void Highlight_delete(Highlight* this) {
   Vector_delete(this->contexts);
   free(this);
}

bool Highlight_readHighlightFile(ReadHighlightFileArgs* args, char* name) {
   Highlight* this = args->this;
   const char* fileName = args->fileName;
   const char* firstLine = args->firstLine;
   FILE* file = fopen(name, "r");
   if (!file) return false;

   char buffer[4096];
   int state = 1;
   bool success = true;
   this->mainContext = Highlight_addContext(this, NULL, NULL, NULL, NormalColor);
   HighlightContext* context = this->mainContext;
   Stack* contexts = Stack_new(ClassAs(HighlightContext, Object), false);
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
         if (!fileName) {
            success = false;
            break;
         }
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
         if (String_eq(tokens[0], "RULES") && ntokens == 1) {
            state = 4;
         }
         break;
      }
      case 4:
      {
         // Read RULES section
         if (String_eq(tokens[0], "context") && (ntokens == 4 || ntokens == 6)) {
            char* open = tokens[1];
            char* close = (String_eq(tokens[2], "`$") ? NULL : tokens[2]);
            Color color;
            if (ntokens == 6) {
               HighlightContext_addRule(context, open, Highlight_translateColor(tokens[3]), PM_RULE);
               color = Highlight_translateColor(tokens[5]);
            } else {
               color = Highlight_translateColor(tokens[3]);
               HighlightContext_addRule(context, open, color, PM_RULE);
            }
            context = Highlight_addContext(this, open, close, Stack_peek(contexts, NULL), color);
            if (close) {
               color = (ntokens == 6 ? Highlight_translateColor(tokens[4]) : color);
               HighlightContext_addRule(context, close, color, PM_RULE);
            }
            Stack_push(contexts, context, 0);
         } else if (String_eq(tokens[0], "/context") && ntokens == 1) {
            if (contexts->size > 1) {
               Stack_pop(contexts, NULL);
               context = Stack_peek(contexts, NULL);
            }
         } else if (String_eq(tokens[0], "rule") && ntokens == 3) {
            HighlightContext_addRule(context, tokens[1], Highlight_translateColor(tokens[2]), PM_RULE);
         } else if (String_eq(tokens[0], "eager_rule") && ntokens == 3) {
            HighlightContext_addRule(context, tokens[1], Highlight_translateColor(tokens[2]), PM_EAGER_RULE);
         } else if (String_eq(tokens[0], "insensitive") && ntokens == 1) {
            this->toLower = true;
         } else if (String_eq(tokens[0], "script") && ntokens == 2) {
            this->hasScript = Script_load(this->L, tokens[1]);
         } else {
            clear();
            mvprintw(0,0,"Error reading %s: line %d: %s", name, lineno, buffer);
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
      clear();
      mvprintw(0,0,"Error reading %s: %d context%s still open", name, contexts->size - 1, contexts->size > 2 ? "s" : "");
      getch();
      success = false;
   }
   Stack_delete(contexts);
   if (success && state == 4) {
      this->currentContext = this->mainContext;
      return true;
   }
   Vector_prune(this->contexts);
   return false;
}

HighlightContext* Highlight_addContext(Highlight* this, char* open, char* close, HighlightContext* parent, Color color) {
   int id = Vector_size(this->contexts);
   HighlightContext* ctx = HighlightContext_new(id, color);
   Vector_add(this->contexts, ctx);
   if (open) {
      assert(parent);
      PatternMatcher_add(parent->follows, (unsigned char*) open, (int) ctx, PM_RULE);
   }
   if (close) {
      assert(parent);
      PatternMatcher_add(ctx->follows, (unsigned char*) close, (int) parent, PM_RULE);
   } else {
      if (parent)
         ctx->nextLine = parent;
   }
   return ctx;
}

static inline int Highlight_tryMatch(Highlight* this, unsigned char* buffer, int* attrs, int at, GraphNode* rules, GraphNode* follows, Method_PatternMatcher_match match, HighlightContext** ctx, bool paintUnmatched, int y) {
   unsigned char* here = buffer + at;
   int intColor;
   char endNode;
   int matchlen = match(rules, here, &intColor, &endNode);
   Color color = (Color) intColor;
   assert(color >= 0 && color < Colors);
   if (matchlen && (endNode == PM_EAGER_RULE || ( !(isword(here[matchlen-1]) && isword(here[matchlen]))))) {
      int attr = CRT_colors[color];
      for (int i = at; i < at+matchlen; i++)
         attrs[i] = attr;
      int nextCtx = 0;
      int followMatchlen = match(follows, here, &nextCtx, &endNode);
      if (followMatchlen == matchlen) {
         assert(nextCtx);
         *ctx = (HighlightContext*) nextCtx;
      }
      at += matchlen;
   } else if (paintUnmatched) {
      int attr = CRT_colors[(*ctx)->defaultColor];
      int word = isword(*here);
      if (word) {
         while (isword(buffer[at]))
            attrs[at++] = attr;
      } else {
         attrs[at++] = attr;
      }
   }
   return at;
}

void Highlight_setAttrs(Highlight* this, unsigned char* buffer, int* attrs, int len, int y) {
   HighlightContext* ctx = this->currentContext;
   int at = 0;
   Method_PatternMatcher_match match;
   if (this->toLower)
      match = PatternMatcher_match_toLower;
   else
      match = PatternMatcher_match;
   if (ctx->rules->lineStart) {
      if (!ctx->follows->lineStart)
         ctx->follows->lineStart = GraphNode_new();
      at = Highlight_tryMatch(this, buffer, attrs, at, ctx->rules->lineStart, ctx->follows->lineStart, match, &ctx, false, y);
   }
   while (at < len) {
      at = Highlight_tryMatch(this, buffer, attrs, at, ctx->rules->start, ctx->follows->start, match, &ctx, true, y);
   }
   Script_highlightLine(this, buffer, attrs, len, y);
   this->currentContext = ctx->nextLine;
}

inline HighlightContext* Highlight_getContext(Highlight* this) {
   return this->currentContext;
}

inline void Highlight_setContext(Highlight* this, HighlightContext* context) {
   this->currentContext = (HighlightContext*) context;
}

HighlightContext* HighlightContext_new(int id, Color defaultColor) {
   HighlightContext* this = Alloc(HighlightContext);
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

void HighlightContext_addRule(HighlightContext* this, char* rule, Color color, char nodeType) {
   PatternMatcher_add(this->rules, (unsigned char*) rule, (int) color, nodeType);
}
