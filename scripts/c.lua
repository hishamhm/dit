
local cscope = require("cscope")

function on_ctrl(key)
   if key == "D" then
      cscope.goto_definition()
   elseif key == "H" then
      local name = buffer:filename()
      if name:match("%.c$") then
         name = name:gsub("%.c$", ".h")
      elseif name:match("%.h$") then
         name = name:gsub("%.h$", ".c")
      else
         return
      end
      local page = tabs:open(name)
      tabs:setPage(page)
   end
end

