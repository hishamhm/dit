
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
   local stack = {}
   for i=1, #line do
      local key = " "
      local ch = line[i]
      if ch == '(' then
         table.insert(stack, {'(', at=i})
         level = level + 1
         key = colors[level]
      elseif ch == ')' then
         if #stack == 0 or stack[#stack][1] ~= '(' then
            key = "*"
            level = 1
         else
            table.remove(stack)
            key = colors[level]
         end
         level = level - 1
         if level < 0 then
            key = "*"
         end
      elseif ch == '[' then
         table.insert(stack, {'[', at=i})
      elseif ch == ']' then
         if #stack == 0 or stack[#stack][1] ~= '[' then
            key = "*"
         else
            table.remove(stack)
         end
      else
         key = " "
      end
      table.insert(out, key)
   end
   while #stack > 0 do
      local err = table.remove(stack)
      out[err.at] = "*"
   end
   return table.concat(out)
end

function on_ctrl(key)
   if key == "_" then
      code.comment_block("--", "%-%-")
   end
end
