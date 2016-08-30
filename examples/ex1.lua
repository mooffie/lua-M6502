-- Runs a simple program.

local mpu = require('M6502').new()
local utils = require('M6502.utils')

local program = utils.parse_hex [[
  a9 01   ; LDA #1
  a2 02   ; LDX #2
  a0 03   ; LDY #3
]]

mpu:pokes(0x600, program)
mpu:pc(0x600)
mpu:run()

-- When it finishes, a one-line dump is printed showing you the values of
-- the registers. Note how A, X and Y hold 1, 2 and 3 respectively.
