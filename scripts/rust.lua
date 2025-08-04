require("compat53")

local config = require("dit.config")
local line_commit = require("dit.line_commit")
local smart_enter = require("dit.smart_enter")
local tab_complete = require("dit.tab_complete")
local code = require("dit.code")

config.add_handlers("on_ctrl", {
   ["O"] = line_commit.line_commit,
   ["_"] = function()
      code.comment_block("//")
   end,
})

smart_enter.activate()
tab_complete.activate()
