
#define _GNU_SOURCE
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#include "Prototypes.h"

/*{

#define PM_RULE 1
#define PM_EAGER_RULE 2

typedef int(*Method_PatternMatcher_match)(GraphNode*, unsigned char*, int*, char*);

struct GraphNode_ {
   unsigned char min;
   unsigned char max;
   char endNode;
   int value;
   union {
      GraphNode* simple;
      struct {
         unsigned char* links;
         unsigned char nptrs;
         union {
            GraphNode* single;
            GraphNode** list;
         } p;
      } l;
   } u;
};

struct PatternMatcher_ {
   GraphNode* start;
   GraphNode* lineStart;
};

}*/

#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

PatternMatcher* PatternMatcher_new() {
   PatternMatcher* this = malloc(sizeof(PatternMatcher));
   this->start = GraphNode_new();
   this->lineStart = NULL;
   return this;
}

void PatternMatcher_delete(PatternMatcher* this) {
   GraphNode_delete(this->start, NULL);
   if (this->lineStart)
      GraphNode_delete(this->lineStart, NULL);
   free(this);
}

void GraphNode_build(GraphNode* current, unsigned char* input, unsigned char* special, int value, char nodeType) {
#define SPECIAL(c) (*special && *input == c)
#define NEXT do { special++; input++; } while (0)
   assert(current); assert(input); assert(special);
   unsigned char mask[256];
   while (*input) {
      memset(mask, 0, 256);
      assert(*special == 0 || *special == 1);
      unsigned char ch = 0;
      if (SPECIAL('[')) {
         NEXT;
         while (*input && !SPECIAL(']')) {
            unsigned char first = *input;
            mask[first] = 1;
            NEXT;
            if (SPECIAL('-')) {
               NEXT;
               for (int j = first; j <= *input; j++)
                  mask[j] = 1;
               NEXT;
            }
            if (SPECIAL('|'))
               NEXT;
         }
         if (!*input)
            break;
      } else if (!*special) {
         ch = *input;
         mask[ch] = 1;
      }
      NEXT;
      if (SPECIAL('+')) {
         NEXT;
         GraphNode* next = GraphNode_new();
         GraphNode_link(current, mask, next);
         current = next;
         GraphNode_link(current, mask, current);
      } else if (SPECIAL('*')) {
         NEXT;
         GraphNode_link(current, mask, current);
      } else if (SPECIAL('?')) {
         NEXT;
         GraphNode* next = GraphNode_new();
         GraphNode_link(current, mask, next);
         GraphNode_build(current, input, special, value, nodeType);
         current = next;
      } else {
         GraphNode* next = NULL;
         if (ch)
            next = GraphNode_follow(current, ch);
         if (!next)
            next = GraphNode_new();
         GraphNode_link(current, mask, next);
         current = next;
      }
   }
   current->value = value;
   current->endNode = nodeType;
#undef SPECIAL
#undef NEXT
}

void PatternMatcher_add(PatternMatcher* this, unsigned char* pattern, int value, int nodeType) {
   assert(this); assert(pattern);
   int len = strlen((char*)pattern) + 1;
   unsigned char input[len];
   unsigned char special[len];
   int i = 0;
   while (*pattern) {
      unsigned char ch = *pattern;
      special[i] = 0;
      if (ch == '`') {
         pattern++;
         ch = *pattern;
         switch (ch) {
         case 't': ch = '\t'; break;
         case 's': ch = ' '; break;
         case '`': break;
         default: special[i] = 1;
         }
      }
      input[i] = ch;
      pattern++;
      i++;
   }
   input[i] = '\0';
   special[i] = '\0';
   GraphNode* start = this->start;
   if (*special && *input == '^') {
      if (!this->lineStart)
         this->lineStart = GraphNode_new();
      start = this->lineStart;
      GraphNode_build(start, input+1, special+1, value, nodeType);
   } else {
      GraphNode_build(start, input, special, value, nodeType);
   }
}

bool PatternMatcher_partialMatch(GraphNode* node, unsigned char* input, int inputLen, char* rest, int restLen) {
   int i = 0;
   while (i < inputLen) {
      assert(node);
      char c = input[i];
      if (c < node->min || c > node->max) {
         node = NULL;
         break;
      }
      if (node->min == node->max) {
         assert(c == node->min);
         node = node->u.simple;
      } else {
         int id = c - node->min;
         int ptrid = node->u.l.links[id];
         if (ptrid == 0)
            break;
         if (node->u.l.nptrs == 1) {
            assert(node->u.l.p.single);
            node = node->u.l.p.single;
         } else {
            assert(node->u.l.p.list[ptrid - 1]);
            node = node->u.l.p.list[ptrid - 1];
         }
      }
      i++;
   }
   if (node) {
      for(int r = 0; r < restLen && node->min; r++) {
         *rest = node->min;
         rest++;
         node = GraphNode_follow(node, node->min);
         if (node->endNode) {
            *rest = '\0';
            return true;
         }
      }
   }
   return false;
}

int PatternMatcher_match(GraphNode* node, unsigned char* input, int* value, char* endNode) {
   int i = 0;
   int match = 0;
   *value = 0;
   while (input[i]) {
      assert(node);
      // node = GraphNode_follow(node, input[i]);

      // inlined version follows
      char c = input[i];
      if (c < node->min || c > node->max)
         break;
      if (node->min == node->max) {
         assert(c == node->min);
         node = node->u.simple;
      } else {
         int id = c - node->min;
         int ptrid = node->u.l.links[id];
         if (ptrid == 0)
            break;
         if (node->u.l.nptrs == 1) {
            assert(node->u.l.p.single);
            node = node->u.l.p.single;
         } else {
            assert(node->u.l.p.list[ptrid - 1]);
            node = node->u.l.p.list[ptrid - 1];
         }
      }
      i++;
      if (node->endNode) {
         match = i;
         *value = node->value;
         *endNode = node->endNode;
      }
   }
   return match;
}

int PatternMatcher_match_toLower(GraphNode* node, unsigned char* input, int* value, char* endNode) {
   int i = 0;
   int match = 0;
   *value = 0;
   while (input[i]) {
      // node = GraphNode_follow(node, input[i]);

      // inlined version follows
      char c = tolower(input[i]);
      if (c < node->min || c > node->max)
         break;
      if (node->min == node->max) {
         assert(c == node->min);
         node = node->u.simple;
      } else {
         int id = c - node->min;
         int ptrid = node->u.l.links[id];
         if (ptrid == 0)
            break;
         if (node->u.l.nptrs == 1)
            node = node->u.l.p.single;
         else
            node = node->u.l.p.list[ptrid - 1];
      }

      i++;
      if (node->endNode) {
         match = i;
         *value = node->value;
         *endNode = node->endNode;
      }
   }
   return match;
}

GraphNode* GraphNode_new() {
   GraphNode* this = calloc(sizeof(GraphNode), 1);
   return this;
}

void GraphNode_delete(GraphNode* this, GraphNode* prev) {
   assert(this);
   // Only loops in graph are self-inflicting edges
   if (this == prev)
      return;
   if (this->min) {
      if (this->min == this->max) {
         GraphNode_delete(this->u.simple, this);
      } else {
         if (this->u.l.links)
            free(this->u.l.links);
         if (this->u.l.nptrs == 1) {
            GraphNode_delete(this->u.l.p.single, this);
         } else {
            assert(this->u.l.p.list);
            for (int i = 0; i < this->u.l.nptrs; i++) {
               assert(this->u.l.p.list[i]);
               GraphNode_delete(this->u.l.p.list[i], this);
            }
            free(this->u.l.p.list);
         }
      }
   } else {
      assert (!this->max);
      assert (!this->u.l.links);
      assert (!this->u.l.p.list);
   }
   free(this);
}

inline GraphNode* GraphNode_follow(GraphNode* this, unsigned char c) {
   if (c < this->min || c > this->max) {
      return NULL;
   }
   if (this->min == this->max) {
      assert(c == this->min);
      return this->u.simple;
   } else {
      int id = c - this->min;
      int ptrid = this->u.l.links[id];
      if (ptrid == 0)
         return NULL;
      assert(ptrid >= 1 && ptrid <= this->u.l.nptrs);
      if (this->u.l.nptrs == 1) {
         assert(this->u.l.p.single);
         return this->u.l.p.single;
      } else {
         assert(this->u.l.p.list[ptrid - 1]);
         return this->u.l.p.list[ptrid - 1];
      }
   }
}

void GraphNode_link(GraphNode* this, unsigned char* mask, GraphNode* next) {

   // Find maskmin and maskmax
   int maskmin = 0;
   int maskmax = 0;
   for (int i = 0; i < 256; i++) {
      if (mask[i] == 1) {
         maskmin = i;
         break;
      }
   }
   for (int i = 255; i >= 0; i--) {
      if (mask[i] == 1) {
         maskmax = i;
         break;
      }
   }
   assert(maskmax >= maskmin);
   int newmin = this->min ? MIN(maskmin, this->min) : maskmin;
   int newmax = MAX(maskmax, this->max);
   assert(newmin && newmax);
   // If node should be/stay simple
   if (newmin == newmax) {
      this->min = newmin;
      this->max = newmax;
      this->u.simple = next;
      return;
   }

   int id = 0;
   // If node is simple, "de-simplify" it
   if (this->min == this->max) {
      GraphNode* oldNode = this->u.simple;
      this->u.l.links = calloc(newmax - newmin + 1, 1);
      if (oldNode)
         this->u.l.links[this->min - newmin] = 1;
      this->u.l.nptrs = 1;
      this->u.l.p.single = oldNode;
   } else if (maskmin < this->min || maskmax > this->max) {
      // Expand the links list if needed
      unsigned char* newlinks = calloc(newmax - newmin + 1, 1);
      memcpy(newlinks + (this->min - newmin), this->u.l.links, this->max - this->min + 1);
      free(this->u.l.links);
      this->u.l.links = newlinks;
   } 
   // If node is single-pointer
   if (this->u.l.nptrs == 1) {
      GraphNode* oldNode = this->u.l.p.single;
      // Turn into multi-pointer if needed
      if (next == oldNode) {
         id = 1;
      } else if (oldNode) {
         this->u.l.nptrs = 2;
         this->u.l.p.list = calloc(sizeof(GraphNode*), 2);
         this->u.l.p.list[0] = oldNode;
         this->u.l.p.list[1] = next;
         id = 2;
      } else {
         this->u.l.nptrs = 1;
         this->u.l.p.single = next;
         id = 1;
      }
   } else {
      // Multi-pointer: check if pointer already in list
      for (int i = 0; i < this->u.l.nptrs; i++) {
         if (this->u.l.p.list[i] == next) {
            id = i + 1;
            break;
         }
      }
      // Add to list if needed
      if (id == 0) {
         this->u.l.nptrs++;
         id = this->u.l.nptrs;
         this->u.l.p.list = realloc(this->u.l.p.list, sizeof(GraphNode*) * this->u.l.nptrs);
         this->u.l.p.list[this->u.l.nptrs - 1] = next;
      }
   }
   this->min = newmin;
   this->max = newmax;
   assert(id > 0);
   for (int i = maskmin; i <= maskmax; i++) {
      if (mask[i])
         this->u.l.links[i - newmin] = id;
   }
}
