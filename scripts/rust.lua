require("compat53")

local config = require("dit.config")
local line_commit = require("dit.line_commit")
local smart_enter = require("dit.smart_enter")
local tab_complete = require("dit.tab_complete")
local code = require("dit.code")

config.add_handlers("on_ctrl", "rust", {
   ["O"] = line_commit.line_commit,
   ["_"] = function()
      code.comment_block("//")
   end,
})

config.add("on_save", "rust", function()
   local name = buffer:filename()
   local x, y = buffer:xy()
   local dn, fn = name:match("^(.*)/([^/]*)$")
   os.execute("( cd " .. dn .. "; rustfmt --edition=2024 " .. fn .. " &>/dev/null )")
   buffer:reload()
   buffer:go_to(x, y)
   return true
end)

smart_enter.activate()
tab_complete.activate()
