local notes = {}

local config = require("dit.config")

local lines = {}
local last_line = 0

function notes.reset()
   lines = {}
   last_line = 0
end

function notes.add(y, x, note)
   note.x = x
   lines[y] = lines[y] or {}
   table.insert(lines[y], note)
   if y > last_line then
      last_line = y
   end
end


local nil_function = function() end 

function notes.each_note(y, x)
   if not lines then return nil_function end
   local curr = lines[y]
   if not curr then return nil_function end
   local line = buffer[y]
   return coroutine.wrap(function()
      for _, note in ipairs(curr) do
         local fchar = note.column
         local lchar = fchar
         if note.name then
            lchar = fchar + #note.name - 1
         else
            while line[lchar+1]:match("[A-Za-z0-9_]") do
               lchar = lchar + 1
            end
         end
         if not x or (x >= fchar and x <= lchar) then
            coroutine.yield(note, fchar, lchar)
         end
      end
   end)
end

function notes.activate()
   config.add_handlers("on_ctrl", {
      ["N"] = function()
         if not lines then
            return
         end
         local x, y = buffer:xy()
         if lines[y] then
            for _, note in ipairs(lines[y]) do
               if note.column > x then
                  buffer:go_to(note.column, y)
                  return
               end
            end
         end
         for line = y+1, last_line do
            if lines[line] then
               buffer:go_to(lines[line][1].column, line)
               return
            end
         end
      end,
   })
   config.add("after_key", function()
      local x, y = buffer:xy()
      local out
      for note in notes.each_note(y, x) do
         if note.text then
            out = out or {}
            table.insert(out, note.text)
         end
      end
      if out then
         buffer:draw_popup(out)
         return true
      end
      return false
   end)
end

return notes
