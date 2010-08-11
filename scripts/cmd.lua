
module("cmd", package.seeall)

function run(cmd, ...)
   local pd = io.popen(cmd:format(...))
   local out = pd:read("*a")
   pd:close()
   return out
end

