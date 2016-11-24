
local cscope = require("cscope")
local code = require("dit.code")
local tab_complete = require("dit.tab_complete")
local line_commit = require("dit.line_commit")

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
      cscope.go_to_definition()
   elseif key == "H" then
      open_header()
   elseif key == "O" then
      line_commit.line_commit()
   elseif key == "_" then
      code.comment_block("//")
   end
end

function on_fkey(key)
   if key == "F7" then
      code.expand_selection()
   end
end

current_file = nil
errors = nil

function highlight_file(filename)
   current_file = filename
end

function highlight_line(line, y)
   if errors and errors[y] then
      local out = {}
      for i = 1, #line do out[i] = " " end
      local marking = false
      for x, _ in pairs(errors[y]) do
         if x <= #line then
            out[x] = "*"
            local xs = x
            while line[xs]:match("[%w_%->]") do
               out[xs] = "*"
               xs = xs + 1
            end
         end
      end
      return table.concat(out)
   else
      return ""
   end
end

function on_change()
   errors = nil
   return true
end

function on_save()
   local cmd = io.popen("LANG=C gcc -I. -Ieditorconfig/include -c "..current_file.." --std=c99 2>&1")
   local cmdout = cmd:read("*a")
   cmd:close()
   errors = {}
   for ey, ex in cmdout:gmatch("[^\n]*:([0-9]+):([0-9]+): error:[^\n]*") do
      ey = tonumber(ey)
      ex = tonumber(ex)
      if not errors[ey] then errors[ey] = {} end
      errors[ey][ex] = true
   end
   return true
end

function on_key(code)
   return tab_complete.on_key(code)
end
