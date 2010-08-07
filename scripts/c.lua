
function on_ctrl(key)
   if key == "D" then
      local token, x, y, len = buffer:token()
      if #token == 0 then return end
      local dir = buffer:dir()
      local cscope_file = dir:gsub("/", "_")
      local decl = io.popen("cscope -s "..dir.." -f /tmp/cscope_"..cscope_file.." -L -1 '"..token.."'"):read("*a")
      local file, word, line = decl:match("([^%s]+) ([^%s]+) ([^%s]+)")
      if file then
         local page = tabs:open(file)
         tabs:setPage(page)
         local buf = tabs:getBuffer(page)
         buf:goto(1, tonumber(line))
      end
   end
end