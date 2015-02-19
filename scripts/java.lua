
local cscope = require("cscope")

function on_ctrl(key)
   if key == "D" then
      cscope.go_to_definition_in_files("*.java")
   end
end

