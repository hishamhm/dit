
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
   for y = y1, y2 do
      local line = buffer[y]
      local indent, rest = line:match(comment_match)
      if indent then
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

return code
