
#include "Prototypes.h"

#include <stdbool.h>
#include <stdlib.h>

/*{

struct StackItem_ {
   void* data;
   StackItem* next;
};

struct Stack_ {
   ObjectClass* type;
   StackItem* head;
   bool owner;
   int size;
};

}*/

Stack* Stack_new(ObjectClass* type, bool owner) {
   Stack* this = (Stack*) malloc(sizeof(Stack));
   this->type = type;
   this->head = NULL;
   this->owner = owner;
   this->size = 0;
   return this;
}

void Stack_empty(Stack* this) {
   StackItem* item = this->head;
   while (item) {
      StackItem* saved = item;
      if (this->owner) {
         if (this->type) {
            Object* obj = (Object*)item->data;
            Msg0(Object, delete, obj);
         } else {
            if (item->data)
               free(item->data);
         }
      }
      item = item->next;
      free(saved);
   }
   this->head = NULL;
   this->size = 0;
}

void Stack_delete(Stack* this) {
   Stack_empty(this);
   free(this);
}

void Stack_push(Stack* this, void* data) {
   assert( !this->type || Call(Object, instanceOf, data, this->type) );
   assert( data );
   StackItem* item = (StackItem*) malloc(sizeof(StackItem));
   item->data = data;
   item->next = this->head;
   this->head = item;
   this->size++;
}

void* Stack_pop(Stack* this) {
   if (!this->head)
      return NULL;
   void* result = this->head->data;
   StackItem* headNext = this->head->next;
   free(this->head);
   this->head = headNext;
   assert( !this->type || Call(Object, instanceOf, result, this->type) );
   this->size--;
   return result;
}

void* Stack_peek(Stack* this) {
   if (!this->head) {
      return NULL;
   }
   return this->head->data;
}

void* Stack_peekAt(Stack* this, int n) {
   StackItem* at = this->head;
   for (int i = 0; i < n; i++) {
      if (at)
         at = at->next;
   }
   if (!at) {
      return NULL;
   }
   return at->data;
}
