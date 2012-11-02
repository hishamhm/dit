
local cscope = require("cscope")
local code = require("dit.code")

local function open_header()
   local name = buffer:filename()
   if name:match("%.c$") then
      name = name:gsub("%.c$", ".h")
   elseif name:match("%.h$") then
      name = name:gsub("%.h$", ".c")
   else
      return
   end
   local page = tabs:open(name)
   tabs:markJump()
   tabs:setPage(page)
end

function on_ctrl(key)
   if key == "D" then
      cscope.goto_definition()
   elseif key == "H" then
      open_header()
   elseif key == "_" then
      code.comment_block("//")
   end
end

function on_fkey(key)
   if key == "F7" then
      code.expand_selection()
   end
end
