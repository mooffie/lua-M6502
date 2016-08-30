-- -*- Lua -*-
title = "M6502 Reference"
project = "M6502"
description = "MOS 6502 Emulator"

dir = "html"  -- output directory.

local PREFIX = "../"

topics = {
  PREFIX .. 'README.md',
}

file = {
  PREFIX .. "src/main.c",
  PREFIX .. "src/lua/utils.lua",
}

examples = PREFIX .. "examples"

kind_names = { topic='Guide' }
boilerplate = true
backtick_references = true
format = "markdown"
sort = false
no_space_before_args = true
style="!pale"

postprocess_html = function(s, item)

  ----------------------------------------------------------------------------

  -- For parameters that have no descriptions, mark them as such so in
  -- the CSS we don't print ":" after their names.
  s = s:gsub([[(<li><span class="parameter)(">[%w_]+</span>%s*</li>)]], "%1 without-description %2")

  -----------------------------------------------------------------------------

  -- Inject our own CSS.
  local level = item and "../" or "" -- the index page (item == nil) is at the top dir already.
  --level = '' -- @todo: remove when we add README.md?
  s = s:gsub('</head>', '    <link rel="stylesheet" href="' .. level .. 'mc.css" type="text/css" />\n%0')

  -----------------------------------------------------------------------------

  -- Handle mdash issues.

  -- Don't have it in code.
  s = s:gsub('<code>.-</code>', function(code)
    return code:gsub('&mdash;', '--')
  end)

  -----------------------------------------------------------------------------

  return s
end
