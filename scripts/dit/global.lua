
local old_on_ctrl = on_ctrl

function on_ctrl(key)
   if key == "R" then
      local x, y = buffer:xy()
      local ch = buffer[y][x]
      local at = y - 1
      local maxx = x
      local positions = {}
      positions[y] = x
      local start = y
      while at > 1 do
         local pos = buffer[at]:find(ch, 1, true)
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
         local pos = buffer[at]:find(ch, 1, true)
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
   if old_on_ctrl then
      return old_on_ctrl(key)
   end
end

