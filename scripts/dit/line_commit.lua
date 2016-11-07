
local line_commit = {}

function line_commit.line_commit()
   local x, y = buffer:xy()
   local pd = io.popen("git blame "..buffer:filename().." -L "..y, "r")
   local info = {}
   if pd then
      local blame = pd:read("*l")
      pd:close()
      local commit = blame:match("([^ ]+) ")
      pd = io.popen("git show "..commit.." -s")
      if pd then
         for line in pd:lines() do
            table.insert(info, line)
         end
         pd:close()
      end
   end
   if #info == 0 then
      info[1] = "not found"
   end
   buffer:draw_popup(info)
end

return line_commit
