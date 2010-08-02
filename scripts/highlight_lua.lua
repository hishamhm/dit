
local LI = require "luainspect.init"

local function loadfile(filename)
  local fh = assert(io.open(filename, 'r'))
  local data = fh:read'*a'
  fh:close()
  return data
end

local notes
local lines = {}

function highlight_file(filename)
   local src = loadfile(filename)
   local ast, err, linenum, colnum, linenum2 = LI.ast_from_string(src, filename)
   notes = LI.inspect(ast)
   local total = 1
   for line in src:gmatch("[^\n]*\n?") do
      table.insert(lines, { len = #line, offset = total })
      total = total + #line
   end
   table.insert(lines, { offset = total, len = #src - total })
   local line = 1
   local curr = lines[1]
local f = io.open("teste.txt", "w")
   for _,note in ipairs(notes) do
      local fchar, lchar = note[1], note[2]
      while fchar > curr.offset + curr.len do
         line = line + 1
         curr = lines[line]
      end
      note[1] = fchar - curr.offset + 1
      note[2] = lchar - curr.offset + 1
      table.insert(curr, note)
for k,v in pairs(note) do
   f:write(tostring(k).." = "..tostring(v).."\n")
end
f:write("\n")
  end
f:close()
end

function highlight_line(line, y)
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
       key = "X"
    end
    table.insert(ret, string.rep(key, lchar - fchar + 1))
    start = lchar + 1
  end
  return table.concat(ret)
end