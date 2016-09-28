
local tab_complete = {}

local words = {}

-- Build a tree of words in the buffer and store it in `words`.
local function populate_word_tree()
   for i = 1, #buffer do
      local line = buffer[i]
      for token in line:gmatch("[%a_\129-\255]+") do
         local node = words
         for c in token:gmatch(".") do
            c = c:lower()
            if not node[c] then
               node[c] = {}
            end
            node = node[c]
         end
         if not node.word then
            node.word = { token }
         else
            if not node.word[token] then
               table.insert(node.word, token)
            end
         end
         node.word[token] = true
      end
   end
end

local function build_matches(matches, node, token)
   if node.word then
      for _, word in ipairs(node.word) do
         if word ~= token then
            table.insert(matches, word)
         end
      end
   end
   for c, sub in pairs(node) do
      if #c == 1 then
         build_matches(matches, sub, token)
      end
   end
end

local function match_words(token)
   local node = words
   for c in token:gmatch(".") do
      if node[c] then
         node = node[c]
      elseif node[c:upper()] then
         node = node[c:upper()]
      elseif node[c:lower()] then
         node = node[c:lower()]
      else
         return {}
      end
   end
   if not node then return {} end
   local matches = {}
   build_matches(matches, node, token)
   table.sort(matches, function(a,b)
      local a_match = a:sub(1,#token) == token
      local b_match = b:sub(1,#token) == token
      if a_match and b_match then
         return a:sub(#token + 1) < b:sub(#token + 1)
      elseif a_match then
         return true
      elseif b_match then
         return false
      end
      return a < b
   end)
   if matches[1] == token then
      table.remove(matches, 1)
   end
   table.insert(matches, token)
   return matches
end

local matching

local function display_match()
   local curr = matching.matches[matching.i]
   buffer[matching.y] = matching.before .. curr .. matching.after
   local selstart, selend = matching.x, matching.tx + #curr
   if selend > selstart then
      buffer:select(selstart, matching.y, selend, matching.y)
   end
   matching.lx = matching.tx + #curr
   buffer:go_to(matching.lx, matching.y, false)
end

local TAB = ("\t"):byte()
local SHIFT_TAB = 353

local function get_token()
   local x, y = buffer:xy()
   local token, tx, ty, len = buffer:token()
   if not token then
      return false
   end
   if tx + #token > x then
      return false
   end
   return token, tx, x, y
end

local function set_matching(matches, tx, x, y, suggestion)
   local before = buffer[y]:sub(1, tx - 1)
   local after = buffer[y]:sub(x)
   if #matches > 0 then
      matching = { matches = matches, i = 1, x = x, y = y, tx = tx, before = before, after = after, suggestion = suggestion }
      display_match()
      return true
   end
end

function tab_complete.suggest(word)
   local token, tx, x, y = get_token()
   if not token then
      return false
   end
   set_matching({ word }, tx, x, y, true)
end

function tab_complete.on_key(code)
   if matching then
      local x, y = buffer:xy()
      if x ~= matching.lx then
         matching = nil
      elseif code == TAB then
         matching.i = matching.i + 1
         if matching.i > #matching.matches then
            matching.i = 1
         end
         display_match()
         return true
      elseif code == SHIFT_TAB then
         matching.i = matching.i - 1
         if matching.i < 1 then
            matching.i = #matching.matches
         end
         display_match()
         return true
      elseif code >= 32 and code <= 255 and code ~= 128 then
         if matching.suggestion then
            if (code >= 65 and code <= 90) or (code >= 97 and code <= 122) then
               matching = nil
               return false
            end
         end
         local curr = matching.matches[matching.i]
         buffer:select(matching.tx + #curr, matching.y, matching.tx + #curr, matching.y)
         matching = nil
      else
         matching = nil
      end
   else
      if code == ("\t"):byte() then
         local token, tx, x, y = get_token()
         if not token then
            return false
         end
         populate_word_tree()
         local matches = match_words(token)
         if set_matching(matches, tx, x, y, false) then
            return true
         end
      end
   end
   return false
end

return tab_complete
