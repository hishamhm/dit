
//#link script
#include <libscript.h>

#include "Prototypes.h"

/*{

typedef enum {
   VOID_FUNCTION,
   STRING_GETTER,
   INT_GETTER
} ScriptFunctionType;

typedef void(*ScriptVoidFunction)(void*);
typedef char*(*ScriptStringGetter)(void*);
typedef int(*ScriptIntGetter)(void*);

struct ScriptFunction_ {
   ScriptFunctionType type;
   union {
      ScriptVoidFunction vf;
      ScriptStringGetter sg;
      ScriptIntGetter ig;
   } f;
   char* name;
};

}*/

static script_env* Script_env;

static Buffer* Script_buffer;

static ScriptFunction Script_dispatchTable[] = {
   { .name = "backwardChar",  .type = VOID_FUNCTION, .f.vf = (ScriptVoidFunction) Buffer_backwardChar  },
   { .name = "indent",        .type = VOID_FUNCTION, .f.vf = (ScriptVoidFunction) Buffer_indent        },
   { .name = "currentLine",   .type = STRING_GETTER, .f.sg = (ScriptStringGetter) Buffer_currentLine   },
   { .name = "previousLine",  .type = STRING_GETTER, .f.sg = (ScriptStringGetter) Buffer_previousLine  },
   { .name = "getIndentSize", .type = INT_GETTER,    .f.ig = (ScriptIntGetter)    Buffer_getIndentSize },
   { .name = "y",             .type = INT_GETTER,    .f.ig = (ScriptIntGetter)    Buffer_y             },
   { .name = NULL }
};

static inline int Script_emptyBuffer(script_env* env) {
   if (!Script_buffer) {
      script_out_bool(env, SCRIPT_FALSE);
      script_out_string(env, "No buffer at this point.");
      return 1;
   }
   return 0;
}

script_err Script_fn_write(script_env* env) {
   const char* text = script_in_string(env);
   SCRIPT_CHECK_INPUTS(env);
   if (Script_emptyBuffer(env)) return SCRIPT_OK;
   for (; *text; text++) {
      Buffer_defaultKeyHandler(Script_buffer, *text);
   }
   script_out_bool(env, SCRIPT_TRUE);
   return SCRIPT_OK;
}

script_err Script_fn_getLastKey(script_env* env) {
   char key[2];
   SCRIPT_CHECK_INPUTS(env);
   if (Script_emptyBuffer(env)) return SCRIPT_OK;
   key[0] = Buffer_getLastKey(Script_buffer);
   key[1] = '\0';
   script_out_string(env, key);
   return SCRIPT_OK;
}

script_err Script_fn_breakIndenting(script_env* env) {
   int indent = script_in_int(env);
   SCRIPT_CHECK_INPUTS(env);
   if (Script_emptyBuffer(env)) return SCRIPT_OK;
   Buffer_breakIndenting(Script_buffer, indent);
   return SCRIPT_OK;
}

script_err Script_fn_dispatcher(script_env* env) {
   const char* name = script_fn_name(env);
   int i;
   SCRIPT_CHECK_INPUTS(env);
   if (Script_emptyBuffer(env)) return SCRIPT_OK;
   for (i = 0; Script_dispatchTable[i].name; i++)
      if (strcmp(name, Script_dispatchTable[i].name) == 0) {
         switch (Script_dispatchTable[i].type) {
         case VOID_FUNCTION:
            Script_dispatchTable[i].f.vf(Script_buffer);
            return SCRIPT_OK;
         case STRING_GETTER:
            script_out_string(env, Script_dispatchTable[i].f.sg(Script_buffer));
            return SCRIPT_OK;
         case INT_GETTER:
            script_out_int(env, Script_dispatchTable[i].f.ig(Script_buffer));
            return SCRIPT_OK;
         }
      }
   return SCRIPT_OK;
}

void Script_init() {
   int i;
   Script_env = script_init("dit");
   script_new_function(Script_env, Script_fn_write, "write");
   script_new_function(Script_env, Script_fn_getLastKey, "getLastKey");
   script_new_function(Script_env, Script_fn_breakIndenting, "breakIndenting");
   for (i = 0; Script_dispatchTable[i].name; i++)
      script_new_function(Script_env, Script_fn_dispatcher, Script_dispatchTable[i].name);
}

// TODO: script environments should vary based on buffer! Oh my!
void Script_loadExtensions(char* name) {
   char fullPath[4096];
   snprintf(fullPath, 4095, "%s_mode", name);   
   script_run_file(Script_env, fullPath);
}

void Script_setCurrentBuffer(Buffer* buffer) {
   Script_buffer = buffer;
}

void Script_hook(const char* name) {
   script_call(Script_env, name);
}
