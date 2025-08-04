local dit_lua_mobdebug = {}

local config = require("dit.config")
local mobdebug_ok, mobdebug = pcall(require, "mobdebug")

local server
local client

dit_lua_mobdebug.breakpoints = {}
dit_lua_mobdebug.where = {}
dit_lua_mobdebug.file = nil
dit_lua_mobdebug.line = nil

local function at(file, line)
   dit_lua_mobdebug.where[file] = line
   dit_lua_mobdebug.file = file
   dit_lua_mobdebug.line = line
end

function dit_lua_mobdebug.is_breakpoint(file, line)
   dit_lua_mobdebug.breakpoints[file] = dit_lua_mobdebug.breakpoints[file] or {}
   local f = dit_lua_mobdebug.breakpoints[file]
   return f[line] == true
end

function dit_lua_mobdebug.set_breakpoint(file, line, val)
   dit_lua_mobdebug.breakpoints[file] = dit_lua_mobdebug.breakpoints[file] or {}
   dit_lua_mobdebug.breakpoints[file][line] = val
end

function dit_lua_mobdebug.is_debugging()
   return client ~= nil
end

function dit_lua_mobdebug.listen()
    if not mobdebug_ok then
       return nil, "mobdebug not installed"
    end

    local host = "*"
    local port = mobdebug.port

    local socket = require "socket"

    server = socket.bind(host, port)
    if not server then
       return nil, "failed to start debugging socket"
    end
    client = server:accept()

    client:send("STEP\n")
    client:receive()

    local breakpoint = client:receive()
    local _, _, file, line = string.find(breakpoint, "^202 Paused%s+(.-)%s+(%d+)%s*$")
    if file and line then
       at(file, line)
    else
      local _, _, size = string.find(breakpoint, "^401 Error in Execution (%d+)%s*$")
      if size then
         return nil, client:receive(size)
      end
   end
   return true
end

function dit_lua_mobdebug.command(command)
   if not mobdebug_ok then
      return nil, "mobdebug not installed"
   end

   if not client then
      return nil, "not in a debugging session"
   end

   local out = {}
   local r1, r2, err = mobdebug.handle(command, client, {
      verbose = function(...)
         table.insert(out, table.concat({...}))
      end
   })
   if not r1 and err == false then
      client:close()
      server:close()
      client = nil
      server = nil
      dit_lua_mobdebug.where = {}
      dit_lua_mobdebug.file = nil
      dit_lua_mobdebug.line = nil
   end

   if err then
      return nil, err
   end
    
   if command == "step" or command == "over" or command == "run" then
      local y = tonumber(r2)
      if r1 ~= buffer:filename() then
-- TODO support multiple files in a single script
--         local page = tabs:open(r1)
--         tabs:mark_jump()
--         tabs:set_page(page)
--         tabs:get_buffer(page):go_to(1, y)
      else
         buffer:go_to(1, y)
      end
      at(r1, y)
   end
   return out
end

config.add_handlers("on_ctrl", {
   ["O"] = function()
      local str = buffer:selection()
      if str == "" then
         str = buffer:token()
      end
      if str and str ~= "" then
         local out = mobdebug.command("eval " .. str)
         if type(out) == "table" then
            buffer:draw_popup(out)
         end
      else
         buffer:draw_popup({ "Select a token to evaluate" })
      end
   end,
})

config.add_handlers("on_fkey", {
   ["F2"] = function()
      local ok, err = mobdebug.listen()
      if ok then
         buffer:draw_popup({
            "Now debbuging. Press:",
            "F4 to step-over",
            "Shift-F4 to step-into",
            "F6 to toggle breakpoint",
            "F11 to run until breakpoint",
         })
      else
         buffer:draw_popup({err})
      end
   end,
   ["SHIFT_F4"] = function()
      local ok, err = mobdebug.command("step")
      if err then
         buffer:draw_popup({err})
      end
   end,
   ["F4"] = function()
      local ok, err = mobdebug.command("over")
      if err then
         buffer:draw_popup({err})
      end
   end,
   ["F6"] = function()
      local filename = buffer:filename()
      local x, y = buffer:xy()
      if mobdebug.is_breakpoint(filename, y) then
         mobdebug.command("delb " .. filename .. " " .. y)
         mobdebug.set_breakpoint(filename, y, nil)
      else
         mobdebug.command("setb " .. filename .. " " .. y)
         mobdebug.set_breakpoint(filename, y, true)
      end
      buffer:go_to(1, y+1)
   end,
   ["F11"] = function()
      local ok, err = mobdebug.command("run")
      if err then
         buffer:draw_popup({err})
      end
   end,
})

return dit_lua_mobdebug

