
local cscope = require("cscope")
local tab_complete = require("dit.tab_complete")

config.add_handlers("on_ctrl", {
   ["D"] = function()
      cscope.go_to_definition_in_files("*.java")
   end,
})

tab_complete.activate()
