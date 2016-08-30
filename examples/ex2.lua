-- Runs a simple program loaded from disk.
--
-- Before it runs, it shows you the disassembled program.

local M6 = require('M6502')
local utils = require('M6502.utils')

local mpu = M6.new()

mpu:pokes(0x600, utils.read_hex_file('ex2.hex'))
print(utils.dis_range(mpu, 0x600, 20))
mpu:pc(0x600)
mpu:run()
