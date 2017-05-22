
local code = require("dit.code")
local tab_complete = require("dit.tab_complete")

local check = require "luacheck.check"
local filter = require "luacheck.filter"
local utils = require "luacheck.utils"

local ok, picotyped = pcall(require, "picotyped")
if not ok then
   picotyped = nil
end

local lines
local commented_lines = {}
local controlled_change = false

local function run_luacheck(src)
   if picotyped then
      src = picotyped.translate(src)
   end
   local checker = type(check) == "function" and check or check.check
   local ok, report = pcall(checker, src)
   if not ok then report = {error = "syntax"} end
   return filter.filter({ report })
end

function highlight_file()
   lines = {}
   local src = table.concat(buffer, "\n")
   local report, err = run_luacheck(src)
   if not report then 
      return
   end
   if report.error == "syntax" then
      local _, err = load(src)
      if err then
         local nr = err:match("^[^:]*:([%d]+):.*")
         local errmsg = err:match("^[^:]*:[%d]+: (.*)")
         if nr then 
            lines[tonumber(nr)] = {{ column = 1, name = (" "):rep(255), code = 0, message = errmsg }}
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
      elseif note.code:sub(1,1) == "6" then -- cosmetic notes
         key = " "
      end
      for i = fchar, lchar do
         ret[i] = key
      end
   end
   if ret == nil then
      return ""
   end
   for i = 1, #ret do
      if not ret[i] then
         ret[i] = " "
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
   highlight_file()
end

local function prefix_if_indirect(fmt)
   return function(w)
      if w.indirect then
         return "indirectly " .. fmt
      else
         return fmt
      end
   end
end

local message_formats = {
   ["011"] = "{msg}",
   ["021"] = "invalid inline option",
   ["022"] = "unpaired push directive",
   ["023"] = "unpaired pop directive",
   ["111"] = function(w)
      if w.module then
         return "setting non-module global variable {name!}"
      else
         return "setting non-standard global variable {name!}"
      end
   end,
   ["112"] = "mutating non-standard global variable {name!}",
   ["113"] = "accessing undefined variable {name!}",
   ["121"] = "setting read-only global variable {name!}",
   ["122"] = prefix_if_indirect("setting read-only field {field!} of global {name!}"),
   ["131"] = "unused global variable {name!}",
   ["142"] = prefix_if_indirect("setting undefined field {field!} of global {name!}"),
   ["143"] = prefix_if_indirect("accessing undefined field {field!} of global {name!}"),
   ["211"] = function(w)
      if w.func then
         if w.recursive then
            return "unused recursive function {name!}"
         elseif w.mutually_recursive then
            return "unused mutually recursive function {name!}"
         else
            return "unused function {name!}"
         end
      else
         return "unused variable {name!}"
      end
   end,
   ["212"] = function(w)
      if w.name == "..." then
         return "unused variable length argument"
      else
         return "unused argument {name!}"
      end
   end,
   ["213"] = "unused loop variable {name!}",
   ["221"] = "variable {name!} is never set",
   ["231"] = "variable {name!} is never accessed",
   ["232"] = "argument {name!} is never accessed",
   ["233"] = "loop variable {name!} is never accessed",
   ["241"] = "variable {name!} is mutated but never accessed",
   ["311"] = "value assigned to variable {name!} is unused",
   ["312"] = "value of argument {name!} is unused",
   ["313"] = "value of loop variable {name!} is unused",
   ["314"] = function(w)
      return "value assigned to " .. (w.index and "index" or "field") .. " {field!} is unused"
   end,
   ["321"] = "accessing uninitialized variable {name!}",
   ["331"] = "value assigned to variable {name!} is mutated but never accessed",
   ["341"] = "mutating uninitialized variable {name!}",
   ["411"] = "variable {name!} was previously defined on line {prev_line}",
   ["412"] = "variable {name!} was previously defined as an argument on line {prev_line}",
   ["413"] = "variable {name!} was previously defined as a loop variable on line {prev_line}",
   ["421"] = "shadowing definition of variable {name!} on line {prev_line}",
   ["422"] = "shadowing definition of argument {name!} on line {prev_line}",
   ["423"] = "shadowing definition of loop variable {name!} on line {prev_line}",
   ["431"] = "shadowing upvalue {name!} on line {prev_line}",
   ["432"] = "shadowing upvalue argument {name!} on line {prev_line}",
   ["433"] = "shadowing upvalue loop variable {name!} on line {prev_line}",
   ["511"] = "unreachable code",
   ["512"] = "loop is executed at most once",
   ["521"] = "unused label {label!}",
   ["531"] = "left-hand side of assignment is too short",
   ["532"] = "left-hand side of assignment is too long",
   ["541"] = "empty do..end block",
   ["542"] = "empty if branch",
   ["551"] = "empty statement",
   ["611"] = "line contains only whitespace",
   ["612"] = "line contains trailing whitespace",
   ["613"] = "trailing whitespace in a string",
   ["614"] = "trailing whitespace in a comment",
   ["621"] = "inconsistent indentation (SPACE followed by TAB)",
   ["631"] = "line is too long ({end_column} > {max_length})"
}

local function get_message_format(warning)
   local message_format = message_formats[warning.code]

   if type(message_format) == "function" then
      return message_format(warning)
   else
      return message_format
   end
end

-- Substitutes markers within string format with values from a table.
-- "{field_name}" marker is replaced with `values.field_name`.
-- "{field_name!}" marker adds quoting.
local function substitute(string_format, values)
   return (string_format:gsub("{([_a-zA-Z0-9]+)(!?)}", function(field_name, highlight)
      local value = tostring(assert(values[field_name], "No field " .. field_name))

      if highlight == "!" then
         return "'" .. value .. "'"
      else
         return value
      end
   end))
end

function on_ctrl(key)
   if key == '_' then
      controlled_change = true
      code.comment_block("--", "%-%-", lines, commented_lines)
      controlled_change = false
   elseif key == "D" then
      local x, y = buffer:xy()
      for note in each_note(y, x) do
         local message = substitute(get_message_format(note), note)
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
