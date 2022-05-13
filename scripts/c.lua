require("compat53")

local cscope = require("cscope")
local tmux = require("tmux")
local code = require("dit.code")
local tab_complete = require("dit.tab_complete")
local line_commit = require("dit.line_commit")

local current_file = nil
local errors = nil

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
   tabs:mark_jump()
   tabs:set_page(page)
end

local function popup_error()
   local x, y = buffer:xy()
   if errors and errors[y] then
      for ex, err in pairs(errors[y]) do
         if x >= ex then
            buffer:draw_popup({err})
            return
         end
      end
   end
end

function on_ctrl(key)
   if key == "D" then
      cscope.go_to_definition()
   elseif key == "R" then
      popup_error()
   elseif key == "H" then
      open_header()
   elseif key == "O" then
      line_commit.line_commit()
   elseif key == "_" then
      code.comment_block("//")
   elseif key == "P" then
      tmux.man()
   end
end

function on_fkey(key)
   if key == "F7" then
      code.expand_selection()
   end
end

function highlight_file(filename)
   current_file = filename
end

function highlight_line(line, y)
   if errors and errors[y] then
      local out = {}
      for i = 1, #line do out[i] = " " end
      for x, err in pairs(errors[y]) do
         local k = "*" -- err:match("^warning") and "S" or "*"
         if x <= #line then
            out[x] = k
            local xs = x
            while line[xs]:match("[%w_%->]") do
               out[xs] = k
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
   if not current_file then
      return true
   end
   local cmd = io.popen("LANG=C gcc -I. -Ieditorconfig/include -c "..current_file.." --std=c99 2>&1")
   local cmdout = cmd:read("*a")
   cmd:close()
   errors = {}
   for ey, ex, err in cmdout:gmatch("[^\n]*:([0-9]+):([0-9]+): error: ([^\n]*)") do
      ey = tonumber(ey)
      ex = tonumber(ex)
      if not errors[ey] then errors[ey] = {} end
      errors[ey][ex] = err
   end
   for ey, ex, err in cmdout:gmatch("[^\n]*:([0-9]+):([0-9]+): warning: ([^\n]*)") do
      ey = tonumber(ey)
      ex = tonumber(ex)
      if not errors[ey] then errors[ey] = {} end
      errors[ey][ex] = "warning: " .. err
   end
   return true
end

function on_key(code)
   return tab_complete.on_key(code)
end
