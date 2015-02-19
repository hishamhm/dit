
local code = require("dit.code")
local luacheck = require("luacheck.init")

local lines
local commented_lines = {}
local controlled_change = false

function highlight_file(filename)
   local report = luacheck({filename})
   if not report then return end
   lines = {}
   for _, note in ipairs(report[1]) do
      local t = lines[note.line] or {}
      t[#t+1] = note
      lines[note.line] = t
   end
end

function highlight_line(line, y)
   if not lines then return end
   local curr = lines[y]
   if not curr then return end
   local start = 1
   local ret = {}
   for _, note in ipairs(curr) do
      local fchar = note.column
      local lchar = fchar
      if note.name then
         lchar = lchar + #note.name - 1
      else
         while line[lchar+1]:match("[A-Za-z0-9_]") do
            lchar = lchar + 1
         end
      end
      if fchar > start then
         table.insert(ret, string.rep(" ", fchar - start))
      end
      local key = "*"
      if note.code == "111" then
         key = "D"
      elseif note.code == "113" then
         key = " "
      end
      table.insert(ret, string.rep(key, lchar - fchar + 1))
      start = lchar + 1
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

function on_ctrl(key)
   if key == '_' then
      controlled_change = true
      code.comment_block("--", "%-%-", lines, commented_lines)
      controlled_change = false
   end
   return true
end

function on_fkey(key)
   if key == "F7" then
      code.expand_selection()
   end
end
