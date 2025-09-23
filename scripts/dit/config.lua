
local config = {}

local local_handlers = {}
local local_handler_names = {}
local main_handlers = {}

function config.add(callback, name, fn)
   assert(type(name) == "string")
   if local_handler_names[callback] and local_handler_names[callback][name] then
      return
   end

   local_handlers[callback] = local_handlers[callback] or {}
   table.insert(local_handlers[callback], fn)

   local_handler_names[callback] = local_handler_names[callback] or {}
   local_handler_names[callback] = name
   
   if not main_handlers[callback] then
      main_handlers[callback] = function(...)
         for _, handler in ipairs(local_handlers[callback]) do
            local ok = handler(...)
            if ok then
               return ok
            end
         end
         return false
      end
   end

   _G[callback] = main_handlers[callback]
end

function config.add_handlers(callback, name, handlers)
   assert(type(name) == "string")

   config.add(callback, name, function(key)
      if handlers[key] then
         local res = handlers[key](key)
         if res == false then
            return false
         end
         return true
      end
      return false
   end)
end

return config
