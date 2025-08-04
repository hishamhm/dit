local config = require("dit.config")
local code = require("dit.code")

config.add_handlers("on_fkey", {
   ["F9"] = code.pick_merge_conflict_branch,
   ["SHIFT_F9"] = code.go_to_conflict,
})


