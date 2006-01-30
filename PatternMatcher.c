
#define _GNU_SOURCE
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#include "Prototypes.h"

/*{

typedef int(*Method_PatternMatcher_match)(GraphNode*, unsigned char*, int*);

struct GraphNode_ {
   unsigned char min;
   unsigned char max;
   bool endNode;
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

void GraphNode_build(GraphNode* current, unsigned char* input, unsigned char* special, int value) {
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
         GraphNode_build(current, input, special, value);
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
   current->endNode = true;
#undef SPECIAL
#undef NEXT
}

void PatternMatcher_add(PatternMatcher* this, unsigned char* pattern, int value) {
   assert(this); assert(pattern);
   unsigned char* input = (unsigned char*) strdup((char*)pattern);
   unsigned char* special = (unsigned char*) strdup((char*)pattern);
   unsigned char* walk = pattern;
   int i = 0;
   while (*walk) {
      special[i] = 0;
      if (*walk == '`') {
         walk++;
         if (*walk == 't')
            *walk = '\t';
         if (*walk == 's')
            *walk = ' ';
         else if (*walk != '`')
            special[i] = 1;
      }
      input[i] = *walk;
      walk++;
      i++;
   }
   input[i] = '\0';
   GraphNode* start = this->start;
   if (*special && *input == '^') {
      if (!this->lineStart)
         start = GraphNode_new();
      this->lineStart = start;
      GraphNode_build(start, input+1, special+1, value);
   } else {
      GraphNode_build(start, input, special, value);
   }
   free(input);
   free(special);
}

int PatternMatcher_match(GraphNode* node, unsigned char* input, int* value) {
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
      }
   }
   return match;
}

int PatternMatcher_match_toLower(GraphNode* node, unsigned char* input, int* value) {
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
      }
   }
   return match;
}

GraphNode* GraphNode_new() {
   GraphNode* this = malloc(sizeof(GraphNode));
   this->min = 0;
   this->max = 0;
   this->value = 0;
   this->endNode = false;
   this->u.simple = NULL;
   this->u.l.links = NULL;
   this->u.l.nptrs = 0;
   this->u.l.p.single = NULL;
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
 //mvprintw(12,0,"c='%c'; this->min='%c',this->make='%c'          ",c,this->min,this->max);
 //getch();
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

assert(next);
 clear();
 mvprintw(0,0, "GraphNode_link( %p : [%c]-[%c]", (void*) this, this->min ? this->min : '0', this->max ? this->max : '0');
 if (this->min == this->max) {
    mvprintw(1, 0, "%p", this->u.simple);
 } else {
    for(int i = this->min; i <= this->max; i++) {
       int id = this->u.l.links[i-this->min];
       mvprintw(1,i-this->min, "%c", id ? i : '_');
       mvprintw(2,i-this->min, "%d", id ? id : 0);
    }
 }
 for(int i = 32; i < 128; i++) {
    mvprintw(3,i-32, "%c", mask[i] ? i : '_');
 }
 ////mvprintw(4,0, "%p)\n", (void*) next);
 assert(next);

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
   // If node should be/stay simple
   if (newmin == newmax) {
 ////mvprintw(7,0,"maskmin=%d newmin=%d this->min=%d", maskmin, newmin, this->min);
 ////mvprintw(8,0,"maskmax=%d newmax=%d this->max=%d", maskmax, newmax, this->max);
      this->min = newmin;
      this->max = newmax;
 ////mvprintw(9,0,"this->min=%d this->max=%d", this->min, this->max);
 ////mvprintw(10,0,"this->min=%c this->max=%c", this->min, this->max);
      this->u.simple = next;
 ////mvprintw(6, 0, "made simple: [%c]-[%c]", this->min ? this->min : '0', this->max ? this->max : '0');
 ////getch();
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
 ////mvprintw(6,0, "desimplified");
   } else if (maskmin < this->min || maskmax > this->max) {
      // Expand the links list if needed
      unsigned char* newlinks = calloc(newmax - newmin + 1, 1);
      memcpy(newlinks + (this->min - newmin), this->u.l.links, this->max - this->min + 1);
      free(this->u.l.links);
      this->u.l.links = newlinks;
 ////mvprintw(6,0, "expanded");
   } 
   // If node is single-pointer
   if (this->u.l.nptrs == 1) {
      GraphNode* oldNode = this->u.l.p.single;
      // Turn into multi-pointer if needed
      if (next == oldNode) {
         id = 1;
 ////mvprintw(7,0, "single-pointer: use same pointer");
      } else if (oldNode) {
 ////mvprintw(7,0, "single-pointer: turn into multi-ptr");
         this->u.l.nptrs = 2;
         this->u.l.p.list = calloc(sizeof(GraphNode*), 2);
         this->u.l.p.list[0] = oldNode;
         this->u.l.p.list[1] = next;
         id = 2;
      } else {
 ////mvprintw(7,0, "single-pointer: alloc first pointer");
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
 ////mvprintw(8,0, "added to list");
         this->u.l.nptrs++;
         id = this->u.l.nptrs;
         this->u.l.p.list = realloc(this->u.l.p.list, sizeof(GraphNode*) * this->u.l.nptrs);
         this->u.l.p.list[this->u.l.nptrs - 1] = next;
      }
   }
   this->min = newmin;
   this->max = newmax;
 ////assert(this->min >= 32 && this->min <= 128);
 ////assert(this->max >= 32 && this->max <= 128);
   assert(id > 0);
 ////mvprintw(8,0, "settings link ids");
   for (int i = maskmin; i <= maskmax; i++) {
      if (mask[i])
         this->u.l.links[i - newmin] = id;
   }
 ////getch();
}
