require("compat53")

local config = require("dit.config")
local line_commit = require("dit.line_commit")
local code = require("dit.code")

config.add_handlers("on_ctrl", "latex", {
   ["O"] = line_commit.line_commit,
   ["_"] = function()
      code.comment_block("%", "%%")
   end,
})
