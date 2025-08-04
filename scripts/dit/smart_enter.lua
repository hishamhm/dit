local smart_enter = {}

local config = require("dit.config")

function smart_enter.activate()
   config.add("on_key", function(code)
      local handled = false
      local selection, startx, starty, stopx, stopy = buffer:selection()
      if selection == "" then
         if code == 13 then
            local x, y = buffer:xy()
            local line = buffer[y]
            if line:sub(1, x - 1):match("^%s*$") and line:sub(x):match("^[^%s]") then
               buffer:begin_undo()
               buffer:emit("\n")
               buffer:go_to(x, y, false)
               buffer:end_undo()
               handled = true
            end
         elseif code == 330 then
            local x, y = buffer:xy()
            local line = buffer[y]
            local nextline = buffer[y+1]
            if x == #line + 1 and line:match("^%s*$") and nextline:match("^"..line) then
               buffer:begin_undo()
               buffer:select(x, y, x, y + 1)
               buffer:emit("\8")
               buffer:end_undo()
               handled = true
            end
         end
      end
      return handled
   end)
end

return smart_enter
