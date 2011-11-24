
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

local function expand_selection() 
   local selection, startx, starty, stopx, stopy = buffer:selection()
   local expanded = false
   local line, x, y = buffer:line()
   -- try to expand through current word
   if starty == stopy then
      for _, pattern in ipairs{"[%w]", "[%w_]", "[*%w_]"} do
         while startx > 1 and line:sub(startx-1,startx-1):match(pattern) do
            startx = startx - 1
            expanded = true
         end
         while stopx < #line - 1 and line:sub(stopx,stopx):match(pattern) do
            stopx = stopx + 1
            expanded = true
         end
         if expanded then break end
      end
      -- try to expand through function call
      if not expanded then
         if line:sub(stopx,stopx) == "(" then
            expanded = true
            local parens = 0
            repeat
               local c = line:sub(stopx,stopx)
               if c == "(" then
                  parens = parens + 1
               elseif c == ")" then
                  parens = parens - 1
               end
               stopx = stopx + 1
            until stopx == #line or parens == 0
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
         local prev = buffer:line(starty-1)
         while not prev:match("^%s*$") do
            starty = starty - 1
            prev = buffer:line(starty-1)
            expanded = true
         end
         local next = buffer:line(stopy)
         while not next:match("^%s*$") do
            stopy = stopy + 1
            next = buffer:line(stopy)
            expanded = true
         end
      end
   end
   buffer:select(startx, starty, stopx, stopy)
end
--[[
void Buffer_expandSelection(Buffer* this) {
   if (!this->selecting) {
      this->selectXfrom = this->x;
      this->selectXto = this->x;
      this->selectYfrom = this->y;
      this->selectYto = this->y;
   }
   bool expanded = false;
   // step 1: try to expand through current word
   while (this->selectXfrom > 0 && isalnum(this->line->text[this->selectXfrom - 1])) {
      expanded = true;
      this->selectXfrom--;
   }
   while (this->selectXto < this->line->len - 1 && isalnum(this->line->text[this->selectXto + 1])) {
      expanded = true;
      this->selectXto++;
   }
   if (expanded) return;
}
]]
function on_ctrl(key)
   if key == "D" then
      cscope.goto_definition()
   elseif key == "H" then
      open_header()
   elseif key == "B" then
      expand_selection()
   end
end

