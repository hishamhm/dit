local ttd = {}

local config = require("dit.config")

local lfs = require("lfs")

local cfg = require("luarocks.core.cfg")
cfg.init()
local fs = require("luarocks.fs")
local dir = require("luarocks.dir")
fs.init()

local filename = dir.normalize(fs.absolute_name(buffer:filename()))
local filename_code = 0

local trace
local locals
local trace_at = 0

local function tracing_this_file(info)
   return info[1] == filename_code
end

local function load_trace()
   if lfs.attributes(filename .. ".trace") then
      local cbor = require("cbor")
      local fd = io.open(filename .. ".trace", "r")
      if fd then
         trace = cbor.decode(fd:read("*a"))
         fd:close()
      end
      for i, f in ipairs(trace.filenames) do
         if f == filename then
            filename_code = i
            break
         end
      end
      if trace then
         trace_at = 1
         for i, t in ipairs(trace.trace) do
            if tracing_this_file(t) then
               trace_at = i
               break
            end
         end
      end
   end
end

local function show_trace_location()
   if trace_at > 0 then
      local info = trace.trace[trace_at]
      if not info then
         return
      end
      local out = {}
      table.insert(out, "#: " .. trace_at)
      table.insert(out, "filename: " .. trace.filenames[info[1]])
      table.insert(out, "line: " .. info[2])
      buffer:go_to(1, info[2])
      locals = {}
      for k, v in pairs(info[3]) do
         locals[trace.strings[k]] = trace.strings[v]
      end
      buffer:draw_popup(out)
   end
end

local function trace_forward(y)
   local found
   for i = trace_at + 1, #trace.trace do
      local t = trace.trace[i]
      if tracing_this_file(t) and ((not y) or t[2] == y) then
         found = i
         break
      end
   end
   if not found then
      for i = 1, trace_at - 1 do
         local t = trace.trace[i]
         if tracing_this_file(t) and ((not y) or t[2] == y) then
            found = i
            break
         end
      end
   end
   if found then
      trace_at = found
   end
end

local function trace_backward(y)
   local found
   for i = trace_at - 1, 1, -1 do
      local t = trace.trace[i]
      if tracing_this_file(t) and ((not y) or t[2] == y) then
         found = i
         break
      end
   end
   if not found then
      for i = #trace.trace, trace_at + 1, -1 do
         local t = trace.trace[i]
         if tracing_this_file(t) and ((not y) or t[2] == y) then
            found = i
            break
         end
      end
   end
   if found then
      trace_at = found
   end
end

config.add_handlers("on_fkey", {
   ["F1"] = function()
      if not trace then
         load_trace()
      end

      trace_backward()
      show_trace_location()
   end,
   ["F2"] = function()
      if not trace then
         load_trace()
      end
      
      local x, y = buffer:xy()
      trace_forward(y)

      show_trace_location()
   end,
   ["F12"] = function()
      if not trace then
         load_trace()
      end

      -- search trace history backwards for current line
      local x, y = buffer:xy()
      trace_backward(y)

      show_trace_location()
   end,
   ["F4"] = function()
      if not trace then
         load_trace()
      end

      trace_forward()
      show_trace_location()
   end,
})

function ttd.highlight_line(line, y)
   if trace_at > 0 then
      local info = trace.trace[trace_at]
      if tracing_this_file(info) and info[2] == y then
         return string.rep("*", #line)
      end
   end
end

function ttd.after_key(out)
   if trace then
      local tk = buffer:token()
      out = out or {}
      if locals[tk] then
         table.insert(out, "= " .. locals[tk])
      end
   end
   return out
end

return ttd
