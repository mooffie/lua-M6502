
local mpu = require('M6502').new()

------------------------------------------------------------------------------

local function test_registers()

  print('testing registers accessors.')

  local tbl = {
    { val=10, lc='a', uc='A' },
    { val=20, lc='x', uc='X' },
    { val=30, lc='y', uc='Y' },
    { val=40, lc='p', uc='P' },
    { val=50, lc='s', uc='S' },
    { val=60, lc='pc', uc='PC' },
  }

  for _, r in ipairs(tbl) do
    -- mpu:x(20)
    mpu[r.lc](mpu, r.val)
  end

  for _, r in ipairs(tbl) do
    -- assert(mpu:x() == 20)
    -- assert(mpu:x() == mpu:X())
    assert(mpu[r.lc](mpu) == r.val)
    assert(mpu[r.lc](mpu) == mpu[r.uc](mpu))
  end

end

local function test_registers_overflow()

  print('testing registers overflow')

  mpu:a(-3)
  assert(mpu:a() == 253)   -- Because `(uint8_t) -3 == 0xFD`

end

------------------------------------------------------------------------------

test_registers()
test_registers_overflow()
