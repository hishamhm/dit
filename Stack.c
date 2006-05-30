
#include "Prototypes.h"

#include <stdbool.h>
#include <stdlib.h>

/*{

struct StackItem_ {
   void* data;
   int size;
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

void Stack_delete(Stack* this) {
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
   free(this);
}

void Stack_push(Stack* this, void* data, int size) {
   assert( !this->type || Call(Object, instanceOf, data, this->type) );
   assert( data );
   StackItem* item = (StackItem*) malloc(sizeof(StackItem));
   item->data = data;
   item->size = size;
   item->next = this->head;
   this->head = item;
   this->size++;
}

void* Stack_pop(Stack* this, int* size) {
   if (!this->head)
      return NULL;
   void* result = this->head->data;
   if (size)
      *size = this->head->size;
   StackItem* headNext = this->head->next;
   free(this->head);
   this->head = headNext;
   assert( !this->type || Call(Object, instanceOf, result, this->type) );
   this->size--;
   return result;
}

void* Stack_peek(Stack* this, int* size) {
   if (!this->head) {
      if (size)
         *size = 0;
      return NULL;
   }
   if (size)
      *size = this->head->size;
   return this->head->data;
}

void* Stack_peekAt(Stack* this, int n, int* size) {
   StackItem* at = this->head;
   for (int i = 0; i < n; i++) {
      if (at)
         at = at->next;
   }
   if (!at) {
      if (size)
         *size = 0;
      return NULL;
   }
   if (size)
      *size = at->size;
   return at->data;
}
