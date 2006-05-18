
//#link script
#include <libscript.h>

#include "Prototypes.h"

/* private property */
static script_env* Script_env;

/* private property */
static Buffer* Script_buffer;

script_err Script_fn_write(script_env* env) {
   const char* text = script_in_string(env);
   SCRIPT_CHECK_INPUTS(env);
   if (!Script_buffer) {
      script_out_bool(env, SCRIPT_FALSE);
      script_out_string(env, "No buffer at this point.");
      return SCRIPT_OK;
   }
   for (; *text; text++) {
      Buffer_defaultKeyHandler(Script_buffer, *text);
   }
   script_out_bool(env, SCRIPT_TRUE);
   return SCRIPT_OK;
}

void Script_init() {
   Script_env = script_init("dit");
   script_new_function(Script_env, Script_fn_write, "write");
}

void Script_loadExtensions(char* name) {
   char fullPath[4096];
   snprintf(fullPath, 4095, "%s.lua", name);
   script_run_file(Script_env, fullPath);
}

void Script_setCurrentBuffer(Buffer* buffer) {
   Script_buffer = buffer;
}

void Script_hook(char* name) {
   script_call(Script_env, name);
}
