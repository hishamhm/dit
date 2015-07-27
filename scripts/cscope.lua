
local cscope = {}

local cmd = require("cmd")

local cscope_files_cache = {}

local function get_cscope_files(dir, pattern)
   if cscope_files_cache[dir] then
      return cscope_files_cache[dir]
   end
   local cscope_files = "/tmp/cscope_"..dir:gsub("/", "_")..".files"
   if os.execute("test -e "..cscope_files) ~= 0 then
      cmd.run("find '%s' -name '%s' > '%s'", dir, pattern, cscope_files)
   end
   cscope_files_cache[dir] = cscope_files
   return cscope_files
end

local function get_context()
   local token, x, y, len = buffer:token()
   if not token then return end
   local dir = buffer:dir()
   local cscope_db = "/tmp/cscope_"..dir:gsub("/", "_")
   return token, dir, cscope_db
end

local function go_to_result(decl, word)
   local file, _, line = decl:match("([^%s]+) ([^%s]+) ([^%s]+)")
   line = tonumber(line)
   if file then
      local page = tabs:open(file)
      tabs:markJump()
      tabs:setPage(page)
      local buf = tabs:getBuffer(page)
      buf:go_to(1, line)
      local text = buf:line()
      if word then
         local x = text:find(word)
         if x then buf:go_to(x, line) end
         return file, line
      end
   end
end

function cscope.go_to_definition()
   local token, dir, cscope_db = get_context()
   if not token then return end
   local decl = cmd.run("cscope -s '%s' -f '%s' -L -1 '%s'", dir, cscope_db, token)
   local thisfile = buffer:basename()
   local thisx, thisy = buffer:xy()
   local file, y = go_to_result(decl, token)
   if file then
      file = file:match("^.*/([^/]+)$")
      if file == thisfile and y == thisy then
         local all = cmd.run("cscope -s '%s' -f '%s' -L -0 '%s'", dir, cscope_db, token)
         for line in all:gmatch("[^\n]+") do
            if line:match("^([^%s]+%.h) ") then
               go_to_result(line, token)
            end
         end
      end
   end
end

function cscope.go_to_definition_in_files(pattern)
   local token, dir, cscope_db = get_context()
   if not token then return end
   local cscope_files = get_cscope_files(dir, pattern)
   local decl = cmd.run("cscope -i '%s' -f '%s' -L -1 '%s'", cscope_files, cscope_db, token)
   local file, word, line = decl:match("([^%s]+) ([^%s]+) ([^%s]+)")
   go_to_result(decl)
end

return cscope
