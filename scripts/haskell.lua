
local code = require("dit.code")
local tab_complete = require("dit.tab_complete")

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

local function find_definition()
   local token = buffer:token()
   if not token then
      return
   end
   local i = 1
   local found = false
   local line
   
   while true do
      line = buffer[i]
      if not line then break end
      if line:match("^%s*data "..token.."[^%w]") or line:match("^%s*type "..token.."[^%w]") then
         found = true
         break
      end
      i = i + 1
   end
   return found, line, i
end

function on_ctrl(key)
   if key == "_" then
      if current_file:match("%.lhs$") then
         code.comment_block("%", "%%")
      else
         code.comment_block("--", "%-%-")
      end
   elseif key == "]" then
      code.expand_selection()
   elseif key == "R" then
      local found, line, i = find_definition()
      if found then
         tabs:markJump()
         buffer:go_to(1, i)
      end
   elseif key == "D" then
      local x, y = buffer:xy()
      if errors and errors[y] and errors[y][x] then
         buffer:draw_popup(errors[y][x])
         return
      end
      local found, line, i = find_definition()
      if found then
         local data = { line }
         local indent = line:match("^(%s*)")
         while true do
            i = i + 1
            line = buffer[i]
            if not line then break end
            local curindent = line:match("^(%s*)")
            if curindent > indent then
               table.insert(data, line)
            else
               break
            end
         end
         buffer:draw_popup(data)
      else
         local cmd = os.getenv("HOME").."/.cabal/bin/ghc-mod type "..buffer:filename().." "..y.." "..x.." 2> /dev/null"
         local pd = io.popen(cmd, "r")
         local typ = "not found"
         if pd then
            local line = pd:read("*l")
            if line then
               typ = line:match("[0-9]+ [0-9]+ [0-9]+ [0-9]+ \"(.*)\"")
            end
            pd:close()
         end
         buffer:draw_popup({ typ })
      end
   end
end

function on_save()
   local cmd = io.popen("LANG=C ghc -i. -c "..current_file.." 2>&1")
   local curr_error
   errors = {}
   for line in cmd:lines() do
      local ey, ex = line:match("[^\n]*:([0-9]+):([0-9]+):")
      if ex then
         ey = tonumber(ey)
         ex = tonumber(ex)
         if not errors[ey] then errors[ey] = {} end
         curr_error = {}
         errors[ey][ex] = curr_error
      elseif curr_error then
         if #line == 0 or line:match("^%s*In ") then
            curr_error = nil
         else
            table.insert(curr_error, (line:gsub("%s$", "")))
         end
      end
   end
   cmd:close()
end

function on_key(code)
   return tab_complete.on_key(code)
end
