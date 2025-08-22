
local config = {}

local local_handlers = {}
local main_handlers = {}
local global_handlers = {}

function config.add(callback, fn)
   local g = _G[callback]
   if g and g ~= main_handlers[callback] or not g then
      global_handlers[callback] = g
      local m = main_handlers[callback] or function(...)
         for _, handler in ipairs(local_handlers[callback]) do
            local ok = handler(...)
            if ok then
               return ok
            end
         end
         if global_handlers[callback] then
            return global_handlers[callback](...)
         end
         return false
      end
      main_handlers[callback] = m
      _G[callback] = m
   end
   local_handlers[callback] = local_handlers[callback] or {}
   table.insert(local_handlers[callback], fn)
end

function config.add_handlers(callback, handlers)
   config.add(callback, function(key)
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
