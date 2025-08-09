
local config = require("dit.config")
local code = require("dit.code")
local smart_enter = require("dit.smart_enter")
local tab_complete = require("dit.tab_complete")
--local sort_selection = require("dit.sort_selection")
local json = require("cjson")

local cfg = require("luarocks.core.cfg")
cfg.init()
local fs = require("luarocks.fs")
local dir = require("luarocks.dir")
fs.init()

local notes = require("dit.notes")
local ttd = require("dit.teal.ttd")

local commented_lines = {}
local controlled_change = false
local type_report
local name_map = {}


function highlight_line(line, y)
   local ret = {}
   for i = 1, #line do ret[i] = " " end
   
   local hl = ttd.highlight_line(line, y)
   if hl then
      return hl
   end
   
   for note, fchar, lchar in notes.each_note(y) do
      for i = fchar, lchar do
         if note.what == "error" then
            ret[i] = "*"
         elseif note.what == "warning" then
            if ret[i] ~= "*" then
               ret[i] = "S"
            end
         end
      end
   end
   if ret == nil then
      return ""
   end
   for i = 1, #ret do
      if not ret[i] then
         ret[i] = " "
      end
   end
   return table.concat(ret)
end

function on_change()
   if not controlled_change then
      lines = nil
   end
end

function on_save(filename)
   local fn = dir.normalize(fs.absolute_name(buffer:filename()))

   code.alert_if_has_conflict()

   local d = dir.dir_name(fn)
   while not (fs.exists(d .. "/tlconfig.lua") or fs.exists(d .. "/.git")) do
      d = dir.dir_name(d)
   end
   local filename = fn:sub(#d + 2)
   name_map[buffer:filename()] = filename
   local cmd = "cd " .. d .. "; tl types " .. filename .. " 2>&1"
   
   local pd = io.popen(cmd)
   notes.reset()
   local state = "start"
   local buf = {}
   for line in pd:lines() do
      if state == "start" then
         if line == "" then
            state = "skip"
         elseif line:match("^========") then
            state = "error"
         else
            state = "json"
         end
      elseif state == "error" then
         if line == "" then
            state = "skip"
         elseif line:match("^%d+ warning") then
            state = "warning"
         end
      elseif state == "warning" then
         if line == "" then
            state = "skip"
         elseif line:match("^========") then
            state = "error"
         end
      end
      
      if state == "json" then
         table.insert(buf, line)
      elseif state == "skip" then
         state = "json"
      elseif state == "error" or state == "warning" then
         local file, y, x, err = line:match("([^:]*):(%d*):(%d*): (.*)")
         if file and filename:sub(-#file) == file then
            y = tonumber(y)
            x = tonumber(x)
            notes.add(y, x, {
               text = err,
               what = state,
            })
         end
      end
   end
   if #buf > 0 then
      local input = table.concat(buf)
      local pok, data = pcall(json.decode, input)
      if not pok then
         error("Error decoding JSON: " .. data .. "\n" .. input:sub(1, 100))
      else
         type_report = data
      end
   end
   pd:close()
end

function highlight_file(filename)
   on_save(filename)
end

local function type_at(px, py)
   if not type_report then
      return
   end
   local ty = type_report.by_pos[name_map[buffer:filename()]][tostring(py)]
   if not ty then
      return
   end
   local xs = {}
   local ts = {}
   for x, t in pairs(ty) do
      x = tonumber(x)
      xs[#xs + 1] = math.floor(x)
      ts[x] = math.floor(t)
   end
   table.sort(xs)

   for i = #xs, 1, -1 do
      local x = xs[i]
      if px >= x then
         return type_report.types[tostring(ts[x])]
      end
   end
end

config.add_handlers("on_alt", {
   ["L"] = function()
      local filename = buffer:filename()
      local x, y = buffer:xy()
      local page = tabs:open(filename:gsub("%.tl$", ".lua"))
      tabs:set_page(page)
      tabs:get_buffer(page):go_to(x, y)
   end
})

config.add_handlers("on_ctrl", {
   ["_"] = function()
      controlled_change = true
      code.comment_block("--", "%-%-", lines, commented_lines)
      controlled_change = false
   end,
   ["D"] = function()
      local x, y = buffer:xy()
      local t = type_at(x, y)
      if t and t.x then
         local tx, ty = t.x, t.y
         if tx == x and ty == y then
            local name = buffer[ty]:match("local%s*([A-Za-z_][A-Za-z0-9_]*)%s*:")
            if not name then
               name = buffer[ty]:match("global%s*([A-Za-z_][A-Za-z0-9_]*)%s*:")
            end
            if not name then
               return true
            end
            local l = y + 1
            while true do
               local line = buffer[l]
               if not line then
                  return true
               end
               local found = line:match("^%s*()" .. name .. "%s*=%s*")
               if found then
                  tx = found
                  ty = l
                  break
               end
               l = l + 1
            end
         end

         tabs:mark_jump()
         if t.file and t.file ~= buffer:filename() then
            local page = tabs:open(t.file)
            tabs:set_page(page)
         end
         if tx then
            buffer:go_to(tx, ty)
         end
      end
   end,
--   ["O"] = function()
--      sort_selection()
--   end,
})

function after_key()
   local x, y = buffer:xy()

   local out
   
   for note in notes.each_note(y, x) do
      out = out or {}
      table.insert(out, note.text)
   end
   
   if not out then
      local t = type_at(x, y)
      if t then
         out = out or {}
         table.insert(out, t.str)
      end
   end
   
   out = ttd.after_key(out)
      
   if out then
      buffer:draw_popup(out) -- lines[y][x].description)
   end
end

smart_enter.activate()
tab_complete.activate()
