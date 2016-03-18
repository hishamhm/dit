
local cscope = require("cscope")
local tab_complete = require("dit.tab_complete")

function on_ctrl(key)
   if key == "D" then
      cscope.go_to_definition_in_files("*.java")
   end
end

function on_key(code)
   return tab_complete.on_key(code)
end
