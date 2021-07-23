
local code = require("dit.code")
local tab_complete = require("dit.tab_complete")
local mobdebug = require("dit.lua.mobdebug")
local json = require("cjson")

local lines
local commented_lines = {}
local controlled_change = false
local type_report

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
         if note.what == "error" then
            ret[i] = "*"
         elseif note.what == "warning" then
            if ret[i] ~= "*" then
               ret[i] = "S"
            end
         end
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
   local pd = io.popen("tl types " .. filename .. " 2>&1")
   lines = {}
   local state = "start"
   local buf = {}
   for line in pd:lines() do
      if state == "start" then
         if line == "" then
            state = "skip"
         elseif line:match("^========") then
            state = "error"
         else
            state = "json"
         end
      elseif state == "error" then
         if line == "" then
            state = "skip"
         elseif line:match("^%d+ warning") then
            state = "warning"
         end
      elseif state == "warning" then
         if line == "" then
            state = "skip"
         elseif line:match("^========") then
            state = "error"
         end
      end
      
      if state == "json" then
         table.insert(buf, line)
      elseif state == "skip" then
         state = "json"
      elseif state == "error" or state == "warning" then
         local file, y, x, err = line:match("([^:]*):(%d*):(%d*): (.*)")
         if file and filename:sub(-#file) == file then
            y = tonumber(y)
            x = tonumber(x)
            lines[y] = lines[y] or {}
            table.insert(lines[y], {
               column = x,
               text = err,
               what = state,
            })
         end
      end
   end
   if #buf > 0 then
      local input = table.concat(buf)
      local pok, data = pcall(json.decode, input)
      if not pok then
         error("Error decoding JSON: " .. data .. "\n" .. input:sub(1, 100))
      else
         type_report = data
      end
   end
   pd:close()
end

local function type_at(px, py)
   if not type_report then
      return
   end
   local ty = type_report.by_pos[buffer:filename()][tostring(py)]
   if not ty then
      return
   end
   local xs = {}
   local ts = {}
   for x, t in pairs(ty) do
      x = tonumber(x)
      xs[#xs + 1] = math.floor(x)
      ts[x] = math.floor(t)
   end
   table.sort(xs)

   for i = #xs, 1, -1 do
      local x = xs[i]
      if px >= x then
         return type_report.types[tostring(ts[x])]
      end
   end
end

function on_ctrl(key)
   if key == '_' then
      controlled_change = true
      code.comment_block("--", "%-%-", lines, commented_lines)
      controlled_change = false
   elseif key == "O" then
--      local str = buffer:selection()
--      if str == "" then
--         str = buffer:token()
--      end
--      if str and str ~= "" then
--         local out = mobdebug.command("eval " .. str)
--         if type(out) == "table" then
--            buffer:draw_popup(out)
--         end
--      else
--         buffer:draw_popup({ "Select a token to evaluate" })
--      end
   elseif key == "D" then
      local x, y = buffer:xy()
      local t = type_at(x, y)
      if t and t.x then
         tabs:mark_jump()
         if t.file and t.file ~= buffer:filename() then
            local page = tabs:open(t.file)
            tabs:set_page(page)
         end
         if t.x then
            buffer:go_to(t.x, t.y)
         end
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

--      local ok, err = mobdebug.command("over")
--      if err then
--         buffer:draw_popup({err})
--      end
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

function after_key(code)
   local x, y = buffer:xy()

   local out
   
   local has_note = false
   for note in each_note(y, x) do
      out = out or {}
      table.insert(out, note.text)
      has_note = true
   end
   
   if not has_note then
      local t = type_at(x, y)
      if t then
         out = out or {}
         table.insert(out, t.str)
         has_note = true
      end
   end
   
   if has_note then
      buffer:draw_popup(out) -- lines[y][x].description)
   end
end
