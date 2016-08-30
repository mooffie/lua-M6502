
local mpu = require('M6502').new()

local utils = require('M6502.utils')

------------------------------------------------------------------------------

-- Test callbacks

local function test_on_read()

  print('testing on_read()')

  mpu:poke(0x1234, 7)

  assert(mpu:peek(0x1234) == 7)

  local function reader(mpu, addr)
    return 111
  end
  mpu:on_read(0x1234, reader)

  assert(mpu:peek(0x1234) == 111)

  mpu:on_read(0x1234, nil)  -- remove the callback.

  assert(mpu:peek(0x1234) == 7)  -- we see the old value again.

end

local function test_on_call()

  print('testing on_call()')

  local counter = 0

  mpu:on_call(0x1234, function()
    --print('0x1234 called!')
    counter = counter + 1
    return "t"
  end)

  mpu:on_call(0x8000, function()
    --print('counter is:', counter)
    assert(counter == 3)
  end)

  mpu:pokew(0xFFFE, 0x0000)
  mpu:on_call(0x0000, function()
    print("BRK instruction reached")
    print("All is ok")
    --print(mpu:dump())
    os.exit()
  end)

  local prog = [[
    ; increment 'counter' 3 times:
    20 34 12   ; JSR $1234
    20 34 12   ; JSR $1234
    20 34 12   ; JSR $1234
    ; ensure that 'counter' equals 3:
    20 00 80   ; JSR $8000
    00         ; BRK
  ]]

  mpu:pokes(0x600, utils.parse_hex(prog))
  --utils.dump_range(mpu, 0x600, 20)
  mpu:pc(0x600)
  mpu:run()

end

------------------------------------------------------

test_on_read()
test_on_call()
