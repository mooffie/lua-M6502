
local mpu = require('M6502').new()

local function it_throws(fn)
  return not pcall(fn)
end

------------------------------------------------------------------------------

local function test_poke(direct)

  print('testing poke()')

  mpu:poke(0xff00, -3, direct)
  assert(mpu:peek(0xff00, direct) == 253)   -- Because `(uint8_t) -3 == 0xFD`

  assert(it_throws(function()      -- Reading outside memory should throw.
    mpu:peek(0x10000)
  end))
  assert(it_throws(function()      -- Negative address should throw.
    mpu:peek(-1)
  end))

end

local function test_pokew(direct)

  print('testing pokew()')

  mpu:pokew(0xff00, 0x123456, direct)
  mpu:pokew(0xff02, -300, direct)

  assert(mpu:peekw(0xff00, direct) == 0x3456)  -- Clipped to word.
  assert(mpu:peek(0xff00, direct) == 0x56)   -- Tests endianess.

  assert(mpu:peekw(0xff02, direct) == 65236)   -- Because `(uint16_t) -300 == 0xFED4`

  assert(it_throws(function()  -- Reading last byte as a word should throw.
    mpu:peekw(0xffff)
  end))

end

local function test_pokes(direct)
  print('testing pokes()')
  mpu:pokes(0xfffc, 'Hello', direct)    -- The last byte won't be written (as it's outside memory).
  assert(mpu:peeks(0xfffc, 9999, direct) == 'Hell')
end

------------------------------------------------------------------------------

test_poke()
test_poke(true)
test_pokew()
test_pokew(true)
test_pokes()
test_pokes(true)
