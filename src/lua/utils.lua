--- << this comment line skips an ldoc bug(?) >>

---
-- A few utility functions.
--
-- @module M6502.utils

local M = {}

---
-- Reads the whole contents of a file.
--
-- Example:
--
--    -- Load a program from disk.
--    mpu:pokes(0x600, utils.read_file('program.bin'))
--
-- Raises an exception in case of error.
--
-- @param path
function M.read_file(path)
  local f = assert(io.open(path, 'rb'))
  local s = f:read('*a')
  f:close()
  return s
end

---
-- Writes a file.
--
-- Example:
--
--    -- Dump the whole memory image to a file.
--    utils.write_file('image.bin', mpu:peeks(0, 0x10000))
--
-- Raises an exception in case of error.
--
-- @param path
-- @param s String. The contents of the file.
function M.write_file(path, s)
  local f = assert(io.open(path, 'wb'))
  assert(f:write(s))
  assert(f:close())
end

---
-- Parses a hex string. E.g., turns "A0 FE 03" into the three bytes "\160\254\003".
--
-- The string may contain comments: anything from ";" till end of lines.
--
-- Example:
--
--    local mpu = require('M6502').new()
--    local utils = require('M6502.utils')
--
--    local program = utils.parse_hex [[
--      a9 01   ; LDA #1
--      a2 02   ; LDX #2
--      a0 03   ; LDY #3
--    ]]
--
--    mpu:pokes(0x600, program)
--    mpu:pc(0x600)
--    mpu:run()
--
-- If the function detects an error in the input (e.g., invalid hex numerals,
-- odd lengths of hex runs), an exception will be raised.
--
-- @param s
function M.parse_hex(s)

  -- Delete comments.
  s = (s .. '\n'):gsub(';.-[\r\n]', '\n')

  badc = s:match('[^%x%s]')
  if badc then
    error(('Input contains non-hex digit (%s)'):format(badc))
  end

  for hex in s:gmatch '%S+' do
    if hex:len() % 2 ~= 0 then
      error(('Input contains an odd length of hex-digits run (%s).'):format(hex:len() < 10 and hex or hex:len()))
    end
  end

  s = s:gsub('%s', '')
  s = s:gsub('..', function(dd)
    return string.char(tonumber(dd, 16))
  end)

  return s

end

---
-- Reads the whole contents of a file, and parse it as hex.
--
-- This is equivalent to doing `parse_hex(read_file(path))`.
--
-- Example:
--
--    -- Load a program from disk.
--    mpu:pokes(0x600, utils.read_hex_file('program.hex'))
--
function M.read_hex_file(path)
  return M.parse_hex(M.read_file(path))
end

---
-- Disassembles a memory range.
--
-- Example:
--
--    print( mpu:dis(0x600, 10) )
--
-- @return A string containing the disassembled code.
--
-- @param mpu
-- @param addr The starting address.
-- @param len How many bytes to disassemble.
function M.dis_range(mpu, addr, len)
  local acc = {}
  local finish = addr + len
  while addr < finish do
    local text, count = mpu:dis(addr)
    local hex = mpu:peeks(addr, count)
    hex = hex:gsub('.', function(c) return string.format('%02x ', string.byte(c)) end)
    table.insert(acc, ("%04x    %-9s %s"):format(addr, hex, text))
    addr = addr + count
  end
  return table.concat(acc, "\n")
end

---
-- Dumps a memory range.
--
-- Example:
--
--    print( mpu:dump_range(0x200, 1024) )
--
-- @return A string containing the hex dump.
--
-- @param mpu
-- @param addr The starting address.
-- @param len How many bytes to dump.
function M.dump_range(mpu, addr, len)
  -- @todo: this excessive string concatenation is inefficient!
  -- @todo: split to lines; put the starting address at beginning of lines.
  local s = ""
  for i = 1, len do
    local b = mpu:peek(addr + i - 1)
    s = s .. string.format('%x', b) .. " "
  end
  print(s)
end

return M
