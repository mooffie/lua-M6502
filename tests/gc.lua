
local M6 = require('M6502')

------------------------------------------------------------------------------
--
-- Tests that only living MPUs are recorded in the 'mpus' registry table.
-- It's a weak table and dead MPUs aren't supposed to be there.
--

local function test_gc()

  print('testing the registration mechanism.')

  -- Utility: How many elements in a table?
  local function count(tbl)
    local cnt = 0
    for _ in pairs(tbl) do
      cnt = cnt + 1
    end
    return cnt
  end

  local reg = debug.getregistry()['mpus']

  assert(count(reg) == 0)

  do
    local m = M6.new()
    assert(count(reg) == 1)
    local m2 = M6.new()
    assert(count(reg) == 2)
  end

  collectgarbage()
  collectgarbage()

  assert(count(reg) == 0)

end

------------------------------------------------------------------------------

test_gc()
