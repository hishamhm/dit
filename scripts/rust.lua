require("compat53")

local line_commit = require("dit.line_commit")
local tab_complete = require("dit.tab_complete")
local code = require("dit.code")

function on_ctrl(key)
   if key == "O" then
      line_commit.line_commit()
   elseif key == "_" then
      code.comment_block("//")
   end
end

function on_key(key)
   return tab_complete.on_key(key)
end

