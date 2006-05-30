
#include "Prototypes.h"

/*{

#define POOL_FREELIST_RATE 100
#define POOL_CHUNKLIST_RATE 10
#define POOL_CHUNK_SIZE 200

struct Pool_ {
   int objectSize;
   char* chunk;
   int used;
   char** freeList;
   int freeListSize;
   int freeListUsed;
   char** chunkList;
   int chunkListSize;
   int chunkListUsed;
   bool destroying;
};
   
}*/

static inline void Pool_addChunk(Pool* this) {
   this->chunk = malloc(POOL_CHUNK_SIZE * this->objectSize);
   this->chunkList[this->chunkListUsed++] = this->chunk;
   this->used = 0;
}

Pool* Pool_new(ObjectClass* type) {
   Pool* this = (Pool*) malloc(sizeof(Pool));
   this->objectSize = type->size;
   this->chunkListUsed = 0;
   this->freeList = malloc(sizeof(void*) * POOL_FREELIST_RATE);
   this->freeListSize = POOL_FREELIST_RATE;
   this->freeListUsed = 0;
   this->chunkListSize = POOL_CHUNKLIST_RATE;
   this->chunkList = malloc(sizeof(void*) * this->chunkListSize);
   this->destroying = false;
   Pool_addChunk(this);
   return this;
}

void Pool_initiateDestruction(Pool* this) {
   this->destroying = true;
}

static inline void* Pool_allocateInChunk(Pool* this) {
   void* result = this->chunk + (this->objectSize * this->used);
   this->used++;
   return result;
}

void* Pool_allocate(Pool* this) {
   if (this->freeListUsed > 0)
      return this->freeList[--this->freeListUsed];
   if (this->used < POOL_CHUNK_SIZE)
      return Pool_allocateInChunk(this);
   else if (this->chunkListUsed < this->chunkListSize) {
      Pool_addChunk(this);
      return Pool_allocateInChunk(this);
   } else {
      this->chunkListSize += POOL_CHUNKLIST_RATE;
      this->chunkList = realloc(this->chunkList, sizeof(void*) * this->chunkListSize);
      Pool_addChunk(this);
      return Pool_allocateInChunk(this);
   }
}

void Pool_free(Pool* this, void* item) {
   if (this->destroying)
      return;
   if (this->freeListUsed < this->freeListSize) {
      this->freeList[this->freeListSize++] = item;
   } else {
      this->freeListSize += POOL_FREELIST_RATE;
      this->freeList = realloc(this->freeList, sizeof(void*) * this->freeListSize);
      this->freeList[this->freeListSize++] = item;
   }
}

void Pool_delete(Pool* this) {
   free(this->freeList);
   for (int i = 0; i < this->chunkListUsed; i++)
      free(this->chunkList[i]);
   free(this->chunkList);
   free(this);
}
