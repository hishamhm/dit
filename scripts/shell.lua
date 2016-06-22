
local code = require("dit.code")

function on_ctrl(key)
   if key == "_" then
      code.comment_block("#", "#")
   end
end

