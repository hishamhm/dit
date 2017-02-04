
local code = require("dit.code")

function on_ctrl(key)
   if key == "_" then
      code.comment_block("%", "%%")
   end
end

function on_fkey(key)
   if key == "F7" then
      code.expand_selection()
   end
end

