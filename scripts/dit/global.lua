
local align_by_char
do
   local function which_char_at_buffer(x, y)
      local line = buffer[y]
      local ch = line[x]
      local i = 1
      local nth = 0
      for c in line:gmatch(".") do
         if c == ch then
            nth = nth + 1
            if i == x then
               break
            end
         end
         i = i + 1
      end
      return ch, nth
   end
   
   local function find_nth(line, ch, nth)
      local pos = 0
      for i = 1, nth do
         pos = pos + 1
         pos = line:find(ch, pos, true)
         if not pos then return nil end
      end
      return pos
   end

   align_by_char = function()
      local x, y = buffer:xy()
      local ch, nth = which_char_at_buffer(x, y)
      local at = y - 1
      local maxx = x
      local positions = {}
      positions[y] = x
      local start = y
      while at > 1 do
         local pos = find_nth(buffer[at], ch, nth)
         if not pos then
            break
         end
         positions[at] = pos
         maxx = math.max(pos, maxx)
         start = at
         at = at - 1
      end
      local stop = y
      at = y + 1
      while at < #buffer do
         local pos = find_nth(buffer[at], ch, nth)
         if not pos then
            break
         end
         positions[at] = pos
         maxx = math.max(pos, maxx)
         stop = at 
         at = at + 1
      end

      buffer:begin_undo()
      for i = start, stop do
         local pos = positions[i]
         if pos < maxx then
            buffer[i] = buffer[i]:sub(1, pos-1) .. (" "):rep(maxx - pos) .. buffer[i]:sub(pos)
         elseif pos > maxx then
            buffer[i] = buffer[i]:sub(1, pos) .. (" "):rep(pos - maxx) .. buffer[i]:sub(pos + 1)
         end
      end
      buffer:end_undo()
      return
   end
end

local flip_quotes
do
   local function flip_quote(line, x, q, p)
      local a, b = line:sub(1, x-1):match("^(.*)"..q.."([^"..q.."]*)$")
      local c, d = line:sub(x):match("^([^"..q.."]*)"..q.."(.*)$")
      if not (a and c) then
         return nil
      end
      return a..p..b..c..p..d
   end

   flip_quotes = function()
      local x, y = buffer:xy()
      local line = buffer[y]
      local flip = flip_quote(line, x, '"', "'") 
                   or flip_quote(line, x, "'", '"')
      if flip then
         buffer[y] = flip
      end
   end
end

local ctrl_p_latch = false

local old_on_ctrl = on_ctrl

function on_ctrl(key)
   if ctrl_p_latch then
      ctrl_p_latch = false
      if key == "Q" then
         flip_quotes()
         return true
      elseif key == "A" then
         align_by_char()
         return true
      end
      return
   end
   ctrl_p_latch = false
   if key == "P" then
      ctrl_p_latch = true
      return
   end
   if old_on_ctrl then
      return old_on_ctrl(key)
   end
end

local old_on_key = on_key

if old_on_key then
   function on_key(k)
      if k ~= 16 then ctrl_p_latch = false end
      return old_on_key(k)
   end
end

