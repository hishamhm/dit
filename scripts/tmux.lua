local tmux = {}

local cmd = require("cmd")

local function get_context()
   local token, x, y, len = buffer:token()
   if not token then return end
   return token
end

function tmux.man()
   local token = get_context()
   if not token then return end
   cmd.run("tmux splitw man '%s'", token)
end

return tmux
