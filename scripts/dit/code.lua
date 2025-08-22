
local code = {}

function code.comment_block(comment, comment_pattern, lines, commented_lines)
   if not comment_pattern then
      comment_pattern = comment
   end
   local selection, x1, y1, x2, y2 = buffer:selection()
   local ox1, oy1, ox2, oy2 = x1, y1, x2, y2
   if y2 < y1 then
      if x1 == 1 then
         y1 = y1 - 1
      end
      local swap = y2
      y2 = y1
      y1 = swap
   elseif y1 ~= y2 and x2 == 1 then
      y2 = y2 - 1         
   end
   buffer:begin_undo()
   local comment_match = "^(%s*)"..comment_pattern.."(.*)$"
   
   -- check if commenting or uncommenting:
   -- uncomment only if all lines are comments.
   local uncommenting = true
   for y = y1, y2 do
      local line = buffer[y]
      local indent, rest = line:match(comment_match)
      if not indent then
         uncommenting = false
      end
   end
   
   for y = y1, y2 do
      local line = buffer[y]
      local indent, rest = line:match(comment_match)
      if uncommenting then
         buffer[y] = indent .. rest
         if lines then lines[y] = commented_lines[y] end
      else
         buffer[y] = comment .. line
         if lines then
            commented_lines[y] = lines[y]
            lines[y] = nil
         end
      end
   end
   buffer:end_undo()
   buffer:select(ox1, oy1, ox2, oy2)
end


local function match_until(line, open, close, parens, stopx)
   repeat
      local c = line[stopx]
      if c == open then
         parens = parens + 1
      elseif c == close then
         parens = parens - 1
      end
      stopx = stopx + 1
   until stopx >= #line or parens == 0
   return stopx
end

local bracket = {
   ["("] = ")",
   ["["] = "]",
   ["{"] = "}",
}

function code.expand_selection() 
   local selection, startx, starty, stopx, stopy = buffer:selection()
   local expanded = false
   local line, x, y = buffer:line()
   -- try to expand through current word
   if starty == stopy then
      for _, pattern in ipairs{"[%w]", "[%w_]", "[*%w_]"} do
         while startx > 1 and line[startx-1]:match(pattern) do
            startx = startx - 1
            expanded = true
         end
         while stopx < #line - 1 and line[stopx]:match(pattern) do
            stopx = stopx + 1
            expanded = true
         end
         if expanded then break end
      end
      -- try to expand through function call
      if not expanded then
         if line[stopx] == "(" then
            expanded = true
            stopx = match_until(line, "(", ")", 0, stopx)
         end
      end
      if not expanded then
         local at = startx - 1
         local c, close
         while true do
            c = line[at]
            close = bracket[c]
            if at <= 1 or close then break end
            at = at - 1
         end
         if close then
            startx = at
            stopx = match_until(line, c, close, 1, stopx)
            expanded = true
         end
      end
      -- try to expand through line
      if not expanded then
         startx = 1
         stopx = 1
         stopy = starty + 1
         expanded = true
      end
   else
      -- try to expand through function
      if startx == 1 and stopx == 1 then
         local prev = buffer[starty-1]
         while prev and not prev:match("^%s*$") do
            starty = starty - 1
            prev = buffer[starty-1]
            expanded = true
         end
         local next = buffer[stopy]
         while next and not next:match("^%s*$") do
            stopy = stopy + 1
            next = buffer[stopy]
            expanded = true
         end
         if next and next:match("^%s*$") then
            stopy = stopy + 1
         end
      end
   end
   buffer:select(startx, starty, stopx, stopy)
end

local function find_divider(y, direction, topmatch, bottommatch)
   local at = y
   while true do
      local line = buffer[at]
      if line == nil then
         return nil
      end
      if line:match(topmatch) then
         return at - direction, true, false
      elseif bottommatch and line:match(bottommatch) then
         return at - direction, false, true
      end
      at = at + direction
   end
end

function code.alert_if_has_conflict()
   local chunkfrom = find_divider(1, 1, "^<<<<", "^====")
   if chunkfrom then
      buffer:set_alert(true)
      return chunkfrom + 1
   else
      buffer:draw_popup({"no git conflicts in this file"})
      buffer:set_alert(false)
   end
end

function code.go_to_conflict()
   local line, x, y = buffer:line()
   local action = nil
   if line:match("^<<<<") then
      action = "descend"
   elseif line:match("^>>>>") then
      action = "ascend"
   end

   if not action then
      local found, above_top, in_top = find_divider(y, 1, "^<<<<", "^====")
      if above_top then
         buffer:go_to(1, found + 1)
         buffer:set_alert(true)
         return
      end
      if in_top then
         action = "descend"
      end
   end

   if not action then
      local found, in_bottom, below_bottom = find_divider(y, -1, "^====", "^>>>>")
      if below_bottom then
         buffer:go_to(1, found - 1)
         buffer:set_alert(true)
         return
      end
      if in_bottom then
         action = "ascend"
      end
   end

   if action == "ascend" then
      buffer:set_alert(true)
      local at = find_divider(y, -1, "^====")
      if at then
         at = find_divider(at - 1, -1, "^<<<<")
      end
      if at then
         buffer:go_to(1, at - 1)
      end
      return
   elseif action == "descend" then
      buffer:set_alert(true)
      local at = find_divider(y, 1, "^====")
      if at then
         at = find_divider(at + 1, 1, "^>>>>")
      end
      if at then
         buffer:go_to(1, at)
      end
      return
   else
      buffer:set_alert(false)
   end
end

function code.pick_merge_conflict_branch()
   local line, x, y = buffer:line()
   local is_top_chunk
   local chunkfrom, chunkto
   local otherfrom, otherto
   local endy
   if line:match("^<<<<") then
      is_top_chunk = true
      chunkfrom = y + 1
   elseif line:match("^>>>>") then
      is_top_chunk = false
      chunkto = y - 1
   elseif line:match("^====") then
      return
   end
   if chunkfrom == nil then
      chunkfrom, is_top_chunk = find_divider(y, -1, "^<<<<", "^====")
      if chunkfrom == nil then
         local conflict = code.alert_if_has_conflict()
         if conflict then
            buffer:go_to(1, conflict)
         end
         return nil
      end
   end
   if chunkto == nil then
      chunkto = find_divider(y, 1, "^====", "^>>>>")
      if chunkto == nil then
         local conflict = code.alert_if_has_conflict()
         if conflict then
            buffer:go_to(1, conflict)
         end
         return nil
      end
   end
   if is_top_chunk then
      otherfrom = chunkto + 2
      otherto = find_divider(otherfrom, 1, "^>>>>")
      endy = chunkfrom - 1
   else
      otherto = chunkfrom - 2
      otherfrom = find_divider(otherto, -1, "^<<<<")
      endy = otherfrom - 1
   end
   buffer:begin_undo()
   if is_top_chunk then
      buffer:select(1, otherfrom - 1, 1, otherto + 2)
      buffer:emit("\b")
      buffer:select(1, chunkfrom - 1, 1, chunkfrom)
      buffer:emit("\b")
   else
      buffer:select(1, chunkto + 1, 1, chunkto + 2)
      buffer:emit("\b")
      buffer:select(1, otherfrom - 1, 1, otherto + 2)
      buffer:emit("\b")
   end
   buffer:go_to(1, endy, false)
   buffer:end_undo()
   code.alert_if_has_conflict()
end

return code
