local jok, json = pcall(require, "cjson")

if not jok then
   return
end

local lines = {}

function highlight_file()
   lines = {}
   local src = table.concat(buffer, "\n")
   local pok, err = pcall(json.decode, src)
   if pok then 
      return
   end
   local chr = err:match("character (%d+)")
   chr = tonumber(chr)
   
   local c = 0
   for i = 1, #buffer do
      local l = #(buffer[i])
      local n = c + l + 1
      if chr <= n then
         lines[i] = chr - c
      end
      c = n
   end
end

function highlight_line(line, y)
   if not lines then
      return
   end
   local ret = {}
   local ex = lines[y]
   if ex then
      for i = 1, #line do ret[i] = " " end
      ret[ex] = "*"
      return table.concat(ret)
   end
end

function on_change()
   lines = nil
end

function on_save(filename)
   highlight_file()
end
