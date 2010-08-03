
local LI = require "luainspect.init"

local function loadfile(filename)
  local fh = assert(io.open(filename, 'r'))
  local data = fh:read'*a'
  fh:close()
  return data
end

local file
local lines

function highlight_file(filename)
   file = filename
   lines = {}
   local src = loadfile(filename)
   local ast, err, linenum, colnum, linenum2 = LI.ast_from_string(src, filename)
   local notes = LI.inspect(ast)
   local total = 1
   for line in src:gmatch("[^\n]*\n?") do
      table.insert(lines, { len = #line, offset = total })
      total = total + #line
   end
   table.insert(lines, { offset = total, len = #src - total })
   local line = 1
   local curr = lines[1]
--local f = io.open("teste.txt", "w")
   for _,note in ipairs(notes) do
      if note.type ~= "comment" and note.type ~= "string" then
         local fchar, lchar = note[1], note[2]
         while fchar > curr.offset + curr.len do
            line = line + 1
            curr = lines[line]
         end
         note[1] = fchar - curr.offset + 1
         note[2] = lchar - curr.offset + 1
         table.insert(curr, note)
      end
--[[
for k,v in pairs(note) do
   f:write(tostring(k).." = "..tostring(v).."\n")
end
f:write("\n")
--]]
  end
--f:close()
end

function highlight_line(line, y)
   if not lines then return end
   local curr = lines[y]
   local start = 1
   local ret = {}
   for _, note in ipairs(curr) do
    local fchar, lchar = note[1], note[2]
    if fchar > start then
      table.insert(ret, string.rep(" ", fchar - start))
    end
    local key = " "
    if note.type == "global" and not note.definedglobal then
       key = "*"
    elseif note.type == "global" then
       key = "D"
    elseif note.type == "field" then
       if note.definedglobal or note.ast.seevalue.value ~= nil then
          key = "d"
       end
    elseif note.type == "local" and note.isparam then
       key = "S"
    end
    table.insert(ret, string.rep(key, lchar - fchar + 1))
    start = lchar + 1
  end
  return table.concat(ret)
end

function on_change()
   lines = nil
end

function on_save(filename)
   lines = {}
   highlight_file(filename)
end
