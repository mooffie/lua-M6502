# 6502 microprocessor emulator

This is an emulator for the [6502 microprocessor](http://6502.org/),
which was dominant in the late '70s and early '80s.

This is but a Lua binding for [Ian Piumarta's emulator](http://piumarta.com/software),
which is written in C (and is bundled here; you don't need to install it
separately).

The use of Lua, a high-level "scripting" language, lets you easily build
the "computer" around the processor using easy Lua code instead of C.

## Installation

Using luarocks:

    $ luarocks install lua-m6502

(Add `--local` (or prepend with `sudo`) if desired.)

Alternatively, clone the repository and do `luarocks make`.

## Example

    -- Instantiate a microprocessor.
    local mpu = require('M6502').new()

    -- Write into memory, at address $600, a sample program.
    mpu:pokes(0x600, "\169\007")  -- "LDA #7" (loads 7 into the A register)

    -- Set the program counter to $600.
    mpu:pc(0x600)

    -- Run!
    mpu:run()

## Documentation

The API is fully documented in `doc/html`. The documentation is generated
by `ldoc`.

## Getting the source code

Do:

    $ git clone https://github.com/mooffie/lua-M6502
