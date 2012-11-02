
local code = require("dit.code")

local colors = {
   'B',
   'A',
   'd',
   's',
   'p',
   'a',
   'D',
   'P',
   'S',
   '*',
}

colors[0] = ' '

function highlight_file(filename)
   return true
end

function highlight_line(line, y)
   local level = 0
   local out = {}
   for i=1, #line do
      local key
      if line[i] == '(' then
         level = level + 1
         key = colors[level]
      elseif line[i] == ')' then
         key = colors[level]
         level = level - 1
      else
         key = " "
      end
      table.insert(out, key)
   end
   return table.concat(out)
end

function on_ctrl(key)
   if key == "_" then
      code.comment_block("--", "%-%-")
   end
end
