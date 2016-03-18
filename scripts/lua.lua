
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

function highlight_line(line, y)
   if not lines then return end
   local curr = lines[y]
   if not curr then return end
   local start = 1
   local ret = {}
   for _, note in ipairs(curr) do
      local fchar = note.column
      local lchar = fchar
      if fchar >= start then -- I miss the 'continue' statement.
         if note.name then
            lchar = fchar + #note.name - 1
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
         -- For error numbers, see http://luacheck.readthedocs.org/en/0.11.0/warnings.html
         elseif note.secondary or note.code == "421" or (note.code == "411" and note.name == "err") then
            key = " "
         end
         table.insert(ret, string.rep(key, lchar - fchar + 1))
         start = lchar + 1
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

function on_key(code)
   return tab_complete.on_key(code)
end
