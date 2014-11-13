
#include "Prototypes.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

/*{

#define Msg(c,m,o,...) ((c ## Class*)(((Object*)o)->class))->m((c*) o, ## __VA_ARGS__)
#define Msg0(c,m,o) ((c ## Class*)(((Object*)o)->class))->m((c*) o)
#define Call(c,m,o,...) c ## _ ## m((c*) o, ## __VA_ARGS__)
#define Call0(c,m,o) c ## _ ## m((c*) o)
#define CallAs0(c,a,m,o) c ## _ ## m((a*) o)
#define Class(c) ((c ## Class*)& c ## Type)
#define ClassAs(c,a) ((a ## Class*)& c ## Type)

#define Alloc(c) ((c*)calloc(sizeof(c), 1)); ((Object*)this)->class = (ObjectClass*)&(c ## Type)
#define Bless(c) ((Object*)this)->class = (ObjectClass*)&(c ## Type)

typedef void(*Method_Object_display)(Object*, RichString*);
typedef bool(*Method_Object_equals)(const Object*, const Object*);
typedef void(*Method_Object_delete)(Object*);

struct ObjectClass_ {
   int size;
   Method_Object_display display;
   Method_Object_equals equals;
   Method_Object_delete delete;
};

struct Object_ {
   ObjectClass* class;
};

extern ObjectClass ObjectType;

}*/

ObjectClass ObjectType = {
   .size = sizeof(Object),
   .display = Object_display,
   .equals = Object_equals,
   .delete = Object_delete
};

void Object_delete(Object* this) {
   free(this);
}

void Object_display(Object* this, RichString* out) {
   char objAddress[50];
   sprintf(objAddress, "O:%p C:%p", (void*) this, (void*) this->class);
   RichString_write(out, 0, objAddress);
}

bool Object_equals(const Object* this, const Object* o) {
   return (this == o);
}

bool Object_instanceOf(Object* this, ObjectClass* class) {
   return this->class == class;
}
