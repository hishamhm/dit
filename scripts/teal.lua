
local code = require("dit.code")
local tab_complete = require("dit.tab_complete")
local mobdebug = require("dit.lua.mobdebug")

local lines
local commented_lines = {}
local controlled_change = false

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

   if mobdebug.is_debugging() then
      local filename = buffer:filename()
      if mobdebug.is_breakpoint(filename, y) then
         ret[1] = "*"
      end
      if mobdebug.file == filename and mobdebug.line == y then
         return ("*"):rep(#line)
      end
   end

   for note, fchar, lchar in each_note(y) do
      for i = fchar, lchar do
         ret[i] = "*"
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
   local pd = io.popen("tl check " .. filename .. " 2>&1")
   lines = {}
   for line in pd:lines() do
      local file, y, x, err = line:match("([^:]*):(%d*):(%d*): (.*)")
      if file and filename:sub(-#file) == file then
         y = tonumber(y)
         x = tonumber(x)
         lines[y] = lines[y] or {}
         table.insert(lines[y], {
            column = x,
            text = err,
         })
      end
   end
   pd:close()
end

function on_ctrl(key)
   if key == '_' then
      controlled_change = true
      code.comment_block("--", "%-%-", lines, commented_lines)
      controlled_change = false
   elseif key == "O" then
      local str = buffer:selection()
      if str == "" then
         str = buffer:token()
      end
      if str and str ~= "" then
         local out = mobdebug.command("eval " .. str)
         if type(out) == "table" then
            buffer:draw_popup(out)
         end
      else
         buffer:draw_popup({ "Select a token to evaluate" })
      end
   elseif key == "D" then
      local x, y = buffer:xy()
      for note in each_note(y, x) do
         buffer:draw_popup({note.text}) -- lines[y][x].description)
         return true
      end
   end
   return true
end

function on_fkey(key)
   if key == "F7" then
      code.expand_selection()
   elseif key == "F2" then
      local ok, err = mobdebug.listen()
      if ok then
         buffer:draw_popup({
            "Now debbuging. Press:",
            "F4 to step-over",
            "Shift-F4 to step-into",
            "F6 to toggle breakpoint",
            "F11 to run until breakpoint",
         })
      else
         buffer:draw_popup({err})
      end
   elseif key == "SHIFT_F4" then
      local ok, err = mobdebug.command("step")
      if err then
         buffer:draw_popup({err})
      end
   elseif key == "F4" then
      local ok, err = mobdebug.command("over")
      if err then
         buffer:draw_popup({err})
      end
   elseif key == "F11" then
      local ok, err = mobdebug.command("run")
      if err then
         buffer:draw_popup({err})
      end
   elseif key == "F6" then
      local filename = buffer:filename()
      local x, y = buffer:xy()
      if mobdebug.is_breakpoint(filename, y) then
         mobdebug.command("delb " .. filename .. " " .. y)
         mobdebug.set_breakpoint(filename, y, nil)
      else
         mobdebug.command("setb " .. filename .. " " .. y)
         mobdebug.set_breakpoint(filename, y, true)
      end
      buffer:go_to(1, y+1)
   elseif key == "F9" then
      code.pick_merge_conflict_branch()
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
