
#include "Prototypes.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

/*{

typedef void(*Method_Object_display)(Object*, RichString*);
typedef bool(*Method_Object_equals)(const Object*, const Object*);
typedef void(*Method_Object_delete)(Object*);

struct Object_ {
   char* class;
   Method_Object_display display;
   Method_Object_equals equals;
   Method_Object_delete delete;
};

}*/

/* private property */
char* OBJECT_CLASS = "Object";

void Object_new() {
   Object* this = malloc(sizeof(Object));
   Object_init(this, OBJECT_CLASS);
}

inline void Object_init(Object* this, char* class) {
   this->class = class;
   this->display = Object_display;
   this->equals = Object_equals;
   this->delete = Object_delete;
}

bool Object_instanceOf(Object* this, char* class) {
   return this->class == class;
}

void Object_delete(Object* this) {
   free(this);
}

void Object_display(Object* this, RichString* out) {
   unsigned char objAddress[50];
   sprintf((char*)objAddress, "%s @ %p", this->class, (void*) this);
   RichString_write(out, A_NORMAL, objAddress);
}

bool Object_equals(const Object* this, const Object* o) {
   return (this == o);
}
