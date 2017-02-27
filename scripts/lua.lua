
local code = require("dit.code")
local tab_complete = require("dit.tab_complete")
local luacheck = require("luacheck.init")

local lines
local commented_lines = {}
local controlled_change = false

function highlight_file(filename)
   lines = {}
   local report, err = luacheck({filename})
   if not report then 
      return
   end
   if #report == 1 and report[1].error == "syntax" then
      local ok, err = loadfile(filename)
      if err then
         local nr = err:match("^[^:]*:([%d]+):.*")
         if nr then 
            lines[tonumber(nr)] = {{ column = 1, name = (" "):rep(255), }}
         end
      end
      return
   end
   for _, note in ipairs(report[1]) do
      local t = lines[note.line] or {}
      t[#t+1] = note
      lines[note.line] = t
   end
end

local function each_note(y, x)
   return coroutine.wrap(function()
      if not lines then return end
      local curr = lines[y]
      local line = buffer[y]
      if not curr then return end
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

function highlight_line(line, y)
   local ret = {}
   for i = 1, #line do ret[i] = " " end
   for note, fchar, lchar in each_note(y) do
      local key = "*"
      if note.code == "111" then
         key = "D"
      -- For error numbers, see http://luacheck.readthedocs.org/en/0.11.0/warnings.html
      elseif note.secondary or note.code == "421" or (note.code == "411" and note.name == "err") then
         key = " "
      end
      for i = fchar, lchar do
         ret[i] = key
      end
   end
   return table.concat(ret)
end

function on_change()
   if not controlled_change then
      lines = nil
   end
end

function on_save(filename)
   highlight_file(filename)
end

local message_formats = {
   ["111"] = function(w)
      if w.module then return "setting non-module global variable %s"
         else return "setting non-standard global variable %s" end end,
   ["112"] = "mutating non-standard global variable %s",
   ["113"] = "accessing undefined variable %s",
   ["121"] = "setting read-only global variable %s",
   ["122"] = "mutating read-only global variable %s",
   ["131"] = "unused global variable %s",
   ["211"] = function(w)
      if w.func then return "unused function %s"
         else return "unused variable %s" end end,
   ["212"] = function(w)
      if w.vararg then return "unused variable length argument"
         else return "unused argument %s" end end,
   ["213"] = "unused loop variable %s",
   ["221"] = "variable %s is never set",
   ["231"] = "variable %s is never accessed",
   ["232"] = "argument %s is never accessed",
   ["233"] = "loop variable %s is never accessed",
   ["311"] = "value assigned to variable %s is unused",
   ["312"] = "value of argument %s is unused",
   ["313"] = "value of loop variable %s is unused",
   ["321"] = "accessing uninitialized variable %s",
   ["411"] = "variable %s was previously defined on line %s",
   ["412"] = "variable %s was previously defined as an argument on line %s",
   ["413"] = "variable %s was previously defined as a loop variable on line %s",
   ["421"] = "shadowing definition of variable %s on line %s",
   ["422"] = "shadowing definition of argument %s on line %s",
   ["423"] = "shadowing definition of loop variable %s on line %s",
   ["511"] = "unreachable code",
   ["512"] = "loop is executed at most once",
   ["521"] = "unused label %s",
   ["531"] = "left-hand side of assignment is too short",
   ["532"] = "left-hand side of assignment is too long",
   ["541"] = "empty do..end block",
   ["542"] = "empty if branch"
}

local function get_message_format(warning)
   if warning.invalid then
      return "invalid inline option"
   elseif warning.unpaired then
      return "unpaired inline option"
   end

   local message_format = message_formats[warning.code]

   if type(message_format) == "function" then
      return message_format(warning)
   else
      return message_format
   end
end

function on_ctrl(key)
   if key == '_' then
      controlled_change = true
      code.comment_block("--", "%-%-", lines, commented_lines)
      controlled_change = false
   elseif key == "D" then
      local x, y = buffer:xy()
      for note in each_note(y, x) do
         local message_format = get_message_format(note)
         local message = message_format:format(note.name, note.prev_line)
         buffer:draw_popup({message}) -- lines[y][x].description)
         return true
      end
   end
   return true
end

function on_fkey(key)
   if key == "F7" then
      code.expand_selection()
   end
end

function on_key(code)
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
