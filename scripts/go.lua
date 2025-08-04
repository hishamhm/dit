require("compat53")

local cscope = require("cscope")
local tmux = require("dit.tmux")
local code = require("dit.code")
local tab_complete = require("dit.tab_complete")
local line_commit = require("dit.line_commit")

local current_file = nil
local errors = nil

local function popup_error()
   local _, x, y = buffer:token()
   if errors and errors[y] then
      for ex, err in pairs(errors[y]) do
         if x == ex then
            buffer:draw_popup({err})
            return
         end
      end
   end
end

config.add_handlers("on_ctrl", {
   ["D"] = cscope.go_to_definition,
   ["R"] = popup_error,
   ["O"] = line_commit.line_commit,
   ["_"] = function() code.comment_block("//") end,
   ["P"] = tmux.man,
})

config.add_handlers("on_fkey", {
   ["F7"] = code.expand_selection,
})

function highlight_file(filename)
   current_file = filename
end

function highlight_line(line, y)
   if errors and errors[y] then
      local out = {}
      for i = 1, #line do out[i] = " " end
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

config.add("on_change", function()
   errors = nil
   return true
end)

config.add("on_save", function()
   if not current_file then
      return true
   end
   local cmd = io.popen("go build -o /dev/null "..current_file.." 2>&1")
   local cmdout = cmd:read("*a")
   cmd:close()
   errors = {}
   for ey, ex, err in cmdout:gmatch("[^\n]*:([0-9]+):([0-9]+): ([^\n]*)") do
      ey = tonumber(ey)
      ex = tonumber(ex)
      if not errors[ey] then errors[ey] = {} end
      errors[ey][ex] = err
   end
   return true
end)

tab_complete.activate()
