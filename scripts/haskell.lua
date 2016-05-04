
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
}

local color_unknown = " "

colors[0] = ' '

local current_file = nil
local errors = nil

function highlight_file(filename)
   current_file = filename
end

local brackets = { ["("] = ")", ["["] = "]" }

local default_term = { [")"] = true, ["]"] = true, [" "] = true }

local function highlight_token(line, out, x, term)
   out[x] = "*"
   while x <= #line do
      local ch = line[x]
      if ch == term then
         out[x] = "*"
         return x
      elseif not term and default_term[ch] then
         return x
      elseif brackets[ch] then
         out[x] = "*"
         x = highlight_token(line, out, x + 1, brackets[ch])
      else
         out[x] = "*"
         x = x + 1
      end
   end
   return x
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
            key = color_unknown
            level = 1
         else
            table.remove(stack)
            key = colors[level]
         end
         level = level - 1
         if level < 0 then
            key = color_unknown
         end
      elseif ch == '[' then
         table.insert(stack, {'[', at=i})
      elseif ch == ']' then
         if #stack == 0 or stack[#stack][1] ~= '[' then
            key = color_unknown
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
      out[err.at] = color_unknown
   end

   if errors and errors[y] then
      for x, _ in pairs(errors[y]) do
         if x <= #line then
            highlight_token(line, out, x)
         end
      end
   end

   return table.concat(out)
end

function on_ctrl(key)
   if key == "_" then
      code.comment_block("--", "%-%-")
   elseif key == "[" or key == "]" then
      code.expand_selection()
   end
end

function on_save()
   local cmd = io.popen("LANG=C ghc -i. -c "..current_file.." 2>&1")
   local cmdout = cmd:read("*a")
   cmd:close()
   errors = {}
   for ey, ex in cmdout:gmatch("[^\n]*:([0-9]+):([0-9]+):") do
      ey = tonumber(ey)
      ex = tonumber(ex)
      if not errors[ey] then errors[ey] = {} end
      errors[ey][ex] = true
   end
   return true
end
