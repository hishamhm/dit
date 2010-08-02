
#include "Prototypes.h"
//#needs Object

/*{

struct ListItemClass_ {
   ObjectClass super;
};

struct ListItem_ {
   Object super;
   ListItem* prev;
   ListItem* next;
   List* list;
};

struct List_ {
   ListItemClass* type;
   ListItem* head;
   ListItem* tail;
   ListItem* atPtr;
   int atCtr;
   int size;
   void* data;
   Pool* pool;
};

extern ListItemClass ListItemType;

}*/

ListItemClass ListItemType = {
   .super = {
      .size = sizeof(ListItem),
      .display = NULL,
      .equals = Object_equals,
      .delete = Object_delete
   }
};

inline void List_reset(List* this) {
   this->head = NULL;
   this->tail = NULL;
   this->size = 0;
   this->atPtr = NULL;
   this->atCtr = 0;
}

List* List_new(ListItemClass* type, void* data) {
   List* this = (List*) malloc(sizeof(List));
   this->type = type;
   this->data = data;
   List_reset(this);
   this->pool = Pool_new((ObjectClass*)type);
   return this;
}

void List_resetIterator(List* this) {
   assert(this->head);
   this->atPtr = this->head;
   this->atCtr = 0;
}

void List_delete(List* this) {
   Pool_initiateDestruction(this->pool);
   List_prune(this);
   Pool_delete(this->pool);
   free(this);
}

void List_prune(List* this) {
   ListItem* walk = this->head;
   while (walk) {
      ListItem* save = walk->next;
      Msg0(Object, delete, walk);
      walk = save;
   }
   List_reset(this);
}

void List_add(List* this, ListItem* item) {
   assert( Call(Object, instanceOf, item, (ObjectClass*)this->type) );
   item->next = NULL;
   item->list = this;
   if (this->tail) {
      item->prev = this->tail;
      this->tail->next = item;
      this->tail = item;
   } else {
      item->prev = NULL;
      this->head = item;
      this->tail = item;
      this->atPtr = item;
      this->atCtr = 0;
   }
   this->size++;
}

inline ListItem* List_getLast(List* this) {
   return this->tail;
}

ListItem* List_get(List* this, int i) {
   assert(this->atPtr);
   assert(i >= 0 && i < this->size);

   if (i == this->atCtr + 1) {
      this->atCtr++;
      this->atPtr = this->atPtr->next;
      return this->atPtr;
   }

   int diffAt = i - this->atCtr;
   int absDiffAt = abs(diffAt);
   int diffHead = i;
   int diffTail = abs(i - (this->size - 1));
   if (absDiffAt < diffHead && absDiffAt < diffTail) {
      if (diffAt > 0) {
         while (this->atCtr < i) {
            this->atPtr = this->atPtr->next;
            this->atCtr++;
         }
      } else if (diffAt < 0) {
         while (this->atCtr > i) {
            this->atPtr = this->atPtr->prev;
            this->atCtr--;
         }
      }
   } else if (diffHead < diffTail) {
      this->atCtr = 0;
      this->atPtr = this->head;
      while (this->atCtr < i) {
         this->atPtr = this->atPtr->next;
         this->atCtr++;
      }
   } else {
      this->atCtr = this->size - 1;
      this->atPtr = this->tail;
      while (this->atCtr > i) {
         this->atPtr = this->atPtr->prev;
         this->atCtr--;
      }
   }
   return this->atPtr;
}

void List_set(List* this, int i, ListItem* item) {
   assert( Call(Object, instanceOf, item, (ObjectClass*)this->type) );
   assert (i >= 0 && i <= this->size);
   if (i == this->size) {
      List_add(this, item);
      return;
   }
   
   ListItem* old = List_get(this, i);
   if (old->prev) {
      old->prev->next = item;
   } else {
      this->head = item;
   }
   if (old->next) {
      old->next->prev = item;
   } else {
      this->tail = item;
   }
   item->prev = old->prev;
   item->next = old->next;
   item->list = this;
   Msg0(Object, delete, old);
}

void List_remove(List* this, int i) {
   assert (i >= 0 && i < this->size);
   ListItem* old = List_get(this, i);
   if (old->prev) {
      old->prev->next = old->next;
   } else {
      this->head = old->next;
   }
   if (old->next) {
      old->next->prev = old->prev;
   } else {
      this->tail = old->prev;
   }
   Msg0(Object, delete, old);
   this->size--;
}

inline int List_size(List* this) {
   return this->size;
}

void ListItem_init(ListItem* this) {
   this->prev = NULL;
   this->next = NULL;
   this->list = NULL;
}

void ListItem_addAfter(ListItem* this, ListItem* item) {
   item->list = this->list;
   if (this->next)
      this->next->prev = item;
   else
      this->list->tail = item;
   item->prev = this;
   item->next = this->next;
   this->next = item;
   this->list->size++;
   List_resetIterator(this->list);
}

void ListItem_remove(ListItem* this) {
   if (this->prev)
      this->prev->next = this->next;
   else
      this->list->head = this->next;
   if (this->next)
      this->next->prev = this->prev;
   else
      this->list->tail = this->prev;
   this->list->size--;
   List_resetIterator(this->list);
   Msg0(Object, delete, this);
}
