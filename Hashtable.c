
#include <stdlib.h>
#include <string.h>

#include "Prototypes.h"

/*{

#define Hashtable_BORROW_REFS 0
#define Hashtable_OWN_REFS 1

typedef enum {
   Hashtable_PTR,
   Hashtable_I,
   Hashtable_STR
} Hashtable_type;

typedef union {
   void* ptr;
   int i;
   char* str;
} HashtableKey;

typedef int(*Hashtable_hash_fn)(Hashtable*, HashtableKey);
typedef int(*Hashtable_eq_fn)(Hashtable*, HashtableKey, HashtableKey);

struct HashtableItem_ {
   HashtableKey key;
   void* value;
   HashtableItem* next;
};

struct Hashtable_ {
   int size;
   HashtableItem** buckets;
   int items;
   Hashtable_type type;
   Hashtable_hash_fn hash;
   Hashtable_eq_fn eq;
   int owner;
};

struct HashtableIterator_ {
   Hashtable* table;
   int bucket;
   HashtableItem* item;
};

}*/

static HashtableItem* HashtableItem_new(HashtableKey key, void* value) {
   HashtableItem* this;
   
   this = (HashtableItem*) calloc(sizeof(HashtableItem), 1);
   this->key = key;
   this->value = value;
   this->next = NULL;
   return this;
}

static HashtableItem* HashtableItem_newString(char* key, void* value) {
   HashtableItem* this;
   int len = strlen(key);
   
   this = (HashtableItem*) calloc(sizeof(HashtableItem)+len+1, 1);
   this->key.str = (char*) this+sizeof(HashtableItem);
   strcpy(this->key.str, key);
   this->value = value;
   this->next = NULL;
   return this;
}

static int Hashtable_intEq(Hashtable* this, HashtableKey a, HashtableKey b) {
   return a.i == b.i;
}

static int Hashtable_intHash(Hashtable* this, HashtableKey key) {
   return (abs(key.i) % this->size);
}

static inline int Hashtable_stringEq(Hashtable* this, HashtableKey a, HashtableKey b) {
   int lena = strlen(a.str);
   int lenb = strlen(b.str);
   if (lena != lenb)
      return 0;
   return (strncmp(a.str, b.str, lena) == 0);
}

static inline int Hashtable_stringHash(Hashtable* this, HashtableKey key) {
   const char* str = key.str;
   unsigned long hash = 0;
   int c;

   /* http://www.cs.yorku.ca/~oz/hash.html */
   while ((c = *str++))
      hash = c + (hash << 6) + (hash << 16) - hash;

   return hash % this->size;
}

Hashtable* Hashtable_new(int size, Hashtable_type type, int owner) {
   Hashtable* this;
   
   this = (Hashtable*) calloc(sizeof(Hashtable), 1);
   this->size = size;
   this->buckets = (HashtableItem**) calloc(sizeof(HashtableItem*), size);
   this->type = type;
   switch (type) {
   case Hashtable_STR:
      this->eq = Hashtable_stringEq;
      this->hash = Hashtable_stringHash;
      break;
   case Hashtable_PTR:
   case Hashtable_I:
      this->eq = Hashtable_intEq;
      this->hash = Hashtable_intHash;
   }
   this->owner = owner;
   return this;
}

void Hashtable_delete(Hashtable* this) {
   int i;
   for (i = 0; i < this->size; i++) {
      HashtableItem* walk = this->buckets[i];
      while (walk != NULL) {
         if (this->owner)
            free(walk->value);
         HashtableItem* save = walk;
         walk = save->next;
         free(save);
      }
   }
   free(this->buckets);
   free(this);
}

int Hashtable_size(Hashtable* this) {
   return this->items;
}

void Hashtable_putInt(Hashtable* this, int key, void* value) {
   HashtableKey hk;
   hk.i = key;
   int index = Hashtable_intHash(this, hk);
   HashtableItem** bucket;
   for (bucket = &(this->buckets[index]); *bucket; bucket = &((*bucket)->next) ) {
      if (Hashtable_intEq(this, (*bucket)->key, hk)) {
         if (this->owner)
            free((*bucket)->value);
         (*bucket)->value = value;
         return;
      }
   }
   *bucket = HashtableItem_new(hk, value);
}

void Hashtable_put(Hashtable* this, HashtableKey key, void* value) {
   if (this->type == Hashtable_STR) {
      Hashtable_putString(this, key.str, value);
      return;
   }
   int index = this->hash(this, key);
   HashtableItem** bucket;
   for (bucket = &(this->buckets[index]); *bucket; bucket = &((*bucket)->next) ) {
      if (this->eq(this, (*bucket)->key, key)) {
         if (this->owner)
            free((*bucket)->value);
         (*bucket)->value = value;
         return;
      }
   }
   *bucket = HashtableItem_new(key, value);
   this->items++;
   return;
}

void Hashtable_putString(Hashtable* this, char* key, void* value) {
   HashtableKey hk;
   hk.str = key;
   int index = Hashtable_stringHash(this, hk);
   HashtableItem** bucket;
   for (bucket = &(this->buckets[index]); *bucket; bucket = &((*bucket)->next) ) {
      if (Hashtable_stringEq(this, (*bucket)->key, hk)) {
         if (this->owner)
            free((*bucket)->value);
         (*bucket)->value = value;
         return;
      }
   }
   *bucket = HashtableItem_newString(key, value);
}

void* Hashtable_take(Hashtable* this, HashtableKey key) {
   int index = this->hash(this, key);
   HashtableItem** bucket; 
   
   for (bucket = &(this->buckets[index]); *bucket; bucket = &((*bucket)->next) ) {
      if (this->eq(this, (*bucket)->key, key)) {
         void* value = (*bucket)->value;
         HashtableItem* next = (*bucket)->next;
         free(*bucket);
         (*bucket) = next;
         this->items--;
         return value;
      }
   }
   return NULL;
}

void* Hashtable_remove(Hashtable* this, HashtableKey key) {
   void* value = Hashtable_take(this, key);
   if (this->owner) {
      free(value);
      return NULL;
   }
   return value;
}

void* Hashtable_get(Hashtable* this, HashtableKey key) {
   int index = this->hash(this, key);
   HashtableItem* bucket;

   for(bucket = this->buckets[index]; bucket; bucket = bucket->next)
      if (this->eq(this, bucket->key, key))
         return bucket->value;
   return NULL;
}

void* Hashtable_getInt(Hashtable* this, int key) {
   HashtableKey hk;
   hk.i = key;
   int index = Hashtable_intHash(this, hk);
   HashtableItem* bucket;

   for(bucket = this->buckets[index]; bucket; bucket = bucket->next)
      if (Hashtable_intEq(this, bucket->key, hk))
         return bucket->value;
   return NULL;
}

void* Hashtable_getString(Hashtable* this, char* key) {
   HashtableKey hk;
   hk.str = key;
   int index = Hashtable_stringHash(this, hk);
   HashtableItem* bucket;

   for(bucket = this->buckets[index]; bucket; bucket = bucket->next)
      if (Hashtable_stringEq(this, bucket->key, hk))
         return bucket->value;
   return NULL;
}

void* Hashtable_takeFirst(Hashtable* this) {
   int i;
   for (i = 0; i < this->size; i++) {
      HashtableItem* bucket = this->buckets[i];
      if (bucket)
         return Hashtable_take(this, bucket->key);
   }
   return NULL;
}

void Hashtable_start(Hashtable* this, HashtableIterator* iter) {
   iter->table = this;
   iter->bucket = 0;
   iter->item = this->buckets[0];
}

void* Hashtable_iterate(HashtableIterator* iter) {
   void* result;
   for(;;) {
      if (!iter->item) {
         iter->bucket++;
         if (iter->bucket >= iter->table->size)
            return NULL;
         iter->item = iter->table->buckets[iter->bucket];
         continue;
      }
      result = iter->item->value;
      iter->item = iter->item->next;
      return result;
   }
}

