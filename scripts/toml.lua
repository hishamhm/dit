require("compat53")

local line_commit = require("dit.line_commit")
local tab_complete = require("dit.tab_complete")
local code = require("dit.code")

function on_ctrl(key)
   if key == "O" then
      line_commit.line_commit()
   elseif key == "_" then
      code.comment_block("#")
   end
end

local key_handlers = {
   -- ["F3"] = find,
   -- ["F5"] = multiple_cursors,
   -- ["F8"] = delete_line,
   ["F9"] = code.pick_merge_conflict_branch,
   ["SHIFT_F9"] = code.go_to_conflict,
   -- ["F10"] = quit
   -- ["F12"] = debug_keyboard_codes,
}

function on_fkey(key)
   if key_handlers[key] then
      key_handlers[key]()
   end
end

function on_key(key)
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
   local tab_handled = false
   if not handled and starty == stopy then
      tab_handled = tab_complete.on_key(code)
   end
   return tab_handled or handled
end

