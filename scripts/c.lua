
local cscope = require("cscope")

local function open_header()
   local name = buffer:filename()
   if name:match("%.c$") then
      name = name:gsub("%.c$", ".h")
   elseif name:match("%.h$") then
      name = name:gsub("%.h$", ".c")
   else
      return
   end
   local page = tabs:open(name)
   tabs:setPage(page)
end

local function isalnum(c)
   return c:match("%w")
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
   until stopx == #line or parens == 0
   return stopx
end

local bracket = {
   ["("] = ")",
   ["["] = "]",
   ["{"] = "}",
}

local function expand_selection() 
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
            if at == 1 or close then break end
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
         while not prev:match("^%s*$") do
            starty = starty - 1
            prev = buffer[starty-1]
            expanded = true
         end
         local next = buffer[stopy]
         while not next:match("^%s*$") do
            stopy = stopy + 1
            next = buffer[stopy]
            expanded = true
         end
      end
   end
   buffer:select(startx, starty, stopx, stopy)
end

function on_ctrl(key)
   if key == "D" then
      cscope.goto_definition()
   elseif key == "H" then
      open_header()
   elseif key == "B" then
      expand_selection()
   end
end

