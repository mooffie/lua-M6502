
local mpu = require('M6502').new()

------------------------------------------------------------------------------

local function test_push()

  print('testing push() / pop()')

  mpu:s(0xff)

  -- push
  mpu:push(7)
  assert(mpu:s() == 0xfe)
  assert(mpu:peek(0x1ff) == 7)

  -- pop
  assert(mpu:pop() == 7)
  assert(mpu:s() == 0xff)

  -- pushw
  mpu:pushw(0x1234)
  assert(mpu:s() == 0xfd)
  assert(mpu:peekw(0x1fd + 1) == 0x1234)

  -- popw
  assert(mpu:popw() == 0x1234)
  assert(mpu:s() == 0xff)

end

------------------------------------------------------

test_push()
