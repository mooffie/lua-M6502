/** */

/**
 * Emulator for the 6502 microprocessor.
 *
 * @module M6502
 */

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "lutils.h"

#include "utils.h"

/* ------------------------------------------------------------------------ */

/**
 * This is the Lua userdata representing a M6502 object.
 */
typedef struct
{
    M6502 *mpu;

    /* The following hold Lua refs to Lua functions (callbacks). */
    int read[0x10000];
    int write[0x10000];
    int call[0x10000];

    lua_State *L;               /* The engine which created this instance. */

} LuaMPU;

static LuaMPU *
get_mpu_self(M6502 * mpu)
{
    return (LuaMPU *) mpu->custom_data;
}

/* ------------------------------------------------------------------------ */

/**
 * Utils: common getters.
 */

#define SELF(L, idx)  luaU_checkudata__unsafe(L, idx, "LuaMPU")

static uint16_t
luaM_checkaddr(lua_State * L, int idx)
{
    int addr = luaL_checkinteger(L, idx);
    if (addr < 0 || addr >= 0x10000)
        luaL_error(L,
                   E_("address out of memory range (should be within [0, 0xffff], but I got %d)."),
                   addr);
    return addr;
}

/* ------------------------------------------------------------------------ */

/**
 * MPUs registration table
 * -----------------------
 *
 * The problem:
 *
 *   We need to be able to convert a M6502 C object to its corresponding Lua
 *   object (LuaMPU).
 *
 *   This is because our callback functions (mpu_{read,write,call}_callback)
 *   are invoked, by the underlying C library, with only a pointer to a M6502
 *   object, and we need to invoke, in turn, the lua callbacks, handing them
 *   a Lua object.
 *
 * The solution:
 *
 *   We record all the living MPU objects in a table. This table associates
 *   M6502 C objects with their corresponding Lua objects. The conversion
 *   task is then simple: a table lookup.
 */

/**
 * Creates the registration table.
 *
 * In pseudo code, it does:
 *
 *    REGISTRY["mpus"] = new_weak_table()
 */
static void
registry__create(lua_State * L)
{
    luaU_new_weak_table(L, "kv");
    lua_setfield(L, LUA_REGISTRYINDEX, "mpus");
}

/**
 * Records an MPU object in the registration table.
 *
 * In pseudo code, it does:
 *
 *    REGISTRY["mpus"][mpu] = lmpu
 */
static void
registry__record_lmpu(lua_State * L, int lmpu_idx)
{
    LUAU_GUARD(L);

    LuaMPU *lmpu = luaU_checkudata__unsafe(L, lmpu_idx, "LuaMPU");

    lmpu_idx = lua_absindex(L, lmpu_idx);

    lua_pushlightuserdata(L, lmpu->mpu);
    lua_pushvalue(L, lmpu_idx);
    luaU_registry_settable(L, "mpus");

    LUAU_UNGUARD(L);
}

/**
 * Gets an MPU from the registration table.
 *
 * In pseudo code, it does:
 *
 *    stack.push( REGISTRY["mpus"][mpu] )
 */
static void
registry__push_lmpu(lua_State * L, M6502 * mpu)
{
    LUAU_GUARD(L);

    lua_pushlightuserdata(L, mpu);
    luaU_registry_gettable(L, "mpus");

    assert(!lua_isnil(L, -1));  /* This shouldn't happen. */

    LUAU_UNGUARD_BY(L, 1);
}

/* ------------------------------------------------------------------------ */

/**
 * Module-level functions.
 *
 * @section
 */

/**
 * Returns a new MPU object.
 *
 * The stack is initialized to 0xFF, and the BRK handler is set to one
 * that @{run|terminates the program}. All other state (memory and
 * registers) is set to zero.
 *
 * @function new
 */
static int
l_new(lua_State * L)
{
    LuaMPU *lmpu;

    lmpu = luaU_newuserdata0(L, sizeof *lmpu, "LuaMPU");

    lmpu->mpu = M6502_new(NULL, NULL, NULL);
    lmpu->L = L;
    lmpu->mpu->custom_data = lmpu;      /* See all places using get_mpu_self() to see why it's needed */

    /* Setup registers. */
    lmpu->mpu->registers->s = 0xff;

    /*
     * After initialization, the word at 0xfffe, which points to where BRK
     * is handled, is 0x0000. We install our BRK handler there. Note that
     * the user is still free to read/write the bytes at 0x0000 and 0x0001
     * without interfering with this mechanism (that's because it's as if
     * callbacks, in our machine, are handled at a different address space).
     */
    M6502_setCallback(lmpu->mpu, call, 0x0000, default_BRK_handler);    /* The user can override this handler. */

    registry__record_lmpu(L, -1);

    return 1;
}

/* ------------------------------------------------------------------------ */

/**
 * Registers.
 *
 * The following functions lets you access the registers. They all work the
 * same: if you provide an argument, they are setters. If you don't, they are
 * getters.
 *
 * **For convenience, you can spell the registers in both lower and
 * upper-case.** (Well, to be frank, the only reason we allow upper-case is
 * because of a limitation of the documentation tool, `ldoc`).
 *
 * Example:
 *
 *    -- Increment the A register.
 *    mpu:a(mpu:a() + 1)
 *
 * @section
 */

#define REGACC(reg) \
    static int \
    l_mpu_ ## reg (lua_State * L) \
    { \
        LuaMPU *self = SELF(L, 1); \
        if (lua_gettop(L) > 1) { \
            self->mpu->registers->reg = luaL_checkinteger(L, 2); \
            return 0; \
        } else { \
            lua_pushinteger(L, self->mpu->registers->reg); \
            return 1; \
        } \
    }

/**
 * Reads/writes the A register.
 *
 * @param[opt] value
 * @function mpu:A
 */
REGACC(a);

/**
 * Reads/writes the X register.
 *
 * @param[opt] value
 * @function mpu:X
 */
REGACC(x);

/**
 * Reads/writes the Y register.
 *
 * @param[opt] value
 * @function mpu:Y
 */
REGACC(y);

/**
 * Reads/writes the P (flags) register.
 *
 * @param[opt] value
 * @function mpu:P
 */
REGACC(p);

/**
 * Reads/writes the S (stack) register.
 *
 * (The `S` register, after calling @{new}, is initialized to `0xFF`. You
 * don't need to do this initialization yourself.)
 *
 * @param[opt] value
 * @function mpu:S
 */
REGACC(s);

/**
 * Reads/writes the PC (program counter) register.
 *
 * This is where the MPU starts executing instructions. You should set it
 * to the start of your program before calling @{run}.
 *
 * @param[opt] value
 * @function mpu:PC
 */
REGACC(pc);

/* ------------------------------------------------------------------------ */

/**
 * Peeking and poking.
 *
 * A bunch of functions to let you easily read/write data from memory.
 *
 * All the functions accept an optional boolean flag, "direct". If
 * __true__, @{on_read|callbacks} will be bypassed. You'll usually want to
 * use this flag inside write callbacks (see example at @{on_write}), or
 * else you'll end up with infinite recursion
 *
 * @section
 */

/**
 * Reads a byte from memory.
 *
 * @param addr
 * @param[opt] direct Boolean.
 *
 * @function mpu:peek
 */
static int
l_mpu_peek(lua_State * L)
{
    LuaMPU *lmpu = SELF(L, 1);
    uint16_t addr = luaM_checkaddr(L, 2);
    gboolean direct = lua_toboolean(L, 3);

    if (direct)
        lua_pushinteger(L, lmpu->mpu->memory[addr]);
    else
        lua_pushinteger(L, read_byte(lmpu->mpu, addr));

    return 1;
}

/**
 * Writes a byte to memory.
 *
 * @function mpu:poke
 *
 * @param addr
 * @param byte
 * @param[opt] direct Boolean.
 */
static int
l_mpu_poke(lua_State * L)
{
    LuaMPU *lmpu = SELF(L, 1);
    uint16_t addr = luaM_checkaddr(L, 2);
    uint8_t value = luaL_checkinteger(L, 3);
    gboolean direct = lua_toboolean(L, 4);

    if (direct)
        lmpu->mpu->memory[addr] = value;
    else
        write_byte(lmpu->mpu, addr, value);

    return 0;
}

/**
 * Reads a word from memory.
 *
 * @param addr
 * @param[opt] direct Boolean.
 *
 * @function mpu:peekw
 */
static int
l_mpu_peekw(lua_State * L)
{
    LuaMPU *lmpu = SELF(L, 1);
    uint16_t addr = luaM_checkaddr(L, 2);
    gboolean direct = lua_toboolean(L, 3);

    if (addr == 0xffff)
        luaL_error(L, E_("Cannot read/write a word at the last byte."));

    if (direct)
    {
        lua_pushinteger(L, *(uint16_t *) (lmpu->mpu->memory + addr));
    }
    else
    {
        lua_pushinteger(L, read_byte(lmpu->mpu, addr) | read_byte(lmpu->mpu, addr + 1) << 8);
    }

    return 1;
}

/**
 * Writes a word to memory.
 *
 * @function mpu:pokew
 *
 * @param addr
 * @param byte
 * @param[opt] direct Boolean.
 */
static int
l_mpu_pokew(lua_State * L)
{
    LuaMPU *lmpu = SELF(L, 1);
    uint16_t addr = luaM_checkaddr(L, 2);
    uint16_t value = luaL_checkinteger(L, 3);
    gboolean direct = lua_toboolean(L, 4);

    if (addr == 0xffff)
        luaL_error(L, E_("Cannot read/write a word at the last byte."));

    if (direct)
    {
        *(uint16_t *) (lmpu->mpu->memory + addr) = value;
    }
    else
    {
        write_byte(lmpu->mpu, addr, value & 0xFF);
        write_byte(lmpu->mpu, addr + 1, value >> 8);
    }

    return 0;
}

/**
 * Reads a string (bytes) from memory.
 *
 * @function mpu:peeks
 *
 * @return The string (bytes) read.
 *
 * @param addr
 * @param len How many bytes to read. (It's permissible to read more than the
 *   available memory: the resulting string will be trimmed down.)
 * @param[opt] direct Boolean.
 */
static int
l_mpu_peeks(lua_State * L)
{
    LuaMPU *lmpu = SELF(L, 1);
    uint16_t addr = luaM_checkaddr(L, 2);
    int len = luaL_checkinteger(L, 3);
    gboolean direct = lua_toboolean(L, 4);

    len = MIN(len, 0x10000 - addr);

    if (direct)
    {
        lua_pushlstring(L, (const char *) &lmpu->mpu->memory[addr], len);
    }
    else
    {
        luaL_Buffer sb;
        luaL_buffinit(L, &sb);
        {
            int i;
            for (i = 0; i < len; i++)
                luaL_addchar(&sb, read_byte(lmpu->mpu, addr + i));
        }
        luaL_pushresult(&sb);
    }
    return 1;
}

/**
 * Writes a string (bytes) to memory.
 *
 * This is the way to load a program into memory. Example:
 *
 *    local mpu = require('M6502').new()
 *
 *    mpu:pokes(0x600, "\169\007")  -- "LDA #7" (loads 7 into the A register)
 *    mpu:pc(0x600)
 *    mpu:run()
 *
 * See @{M6502.utils.parse_hex} for another example.
 *
 * @function mpu:pokes
 *
 * @param addr
 * @param s The string (bytes) to write. (It's permissible for this string to
 *   be longer than the available memory: the excessive bytes will be ignored.)
 * @param[opt] direct Boolean.
 */
static int
l_mpu_pokes(lua_State * L)
{
    LuaMPU *lmpu = SELF(L, 1);
    uint16_t addr = luaM_checkaddr(L, 2);
    size_t len;
    const char *s = luaL_checklstring(L, 3, &len);
    gboolean direct = lua_toboolean(L, 4);

    len = MIN(len, 0x10000 - addr);

    if (direct)
    {
        memcpy(&lmpu->mpu->memory[addr], s, len);
    }
    else
    {
        int i;
        for (i = 0; i < len; i++)
            write_byte(lmpu->mpu, addr + i, s[i]);
    }
    return 0;
}

/* ------------------------------------------------------------------------ */

/**
 * Stack operations.
 *
 * Pushes and pops data from the stack.
 *
 * Note that these are just convenience functions; Instead of:
 *
 *    mpu:push(123)
 *
 * You can do:
 *
 *    mpu:poke(0x100 + mpu:s(), 123)
 *    mpu:s(mpu:s() - 1)
 *
 * If you're trying to pop form an empty stack, or push onto a full
 *  stack, the 'S' register will simply wrap around.
 *
 * @section
 */

/**
 * Pushes a byte onto the stack.
 *
 * @param byte
 *
 * @function mpu:push
 */
static int
l_mpu_push(lua_State * L)
{
    LuaMPU *self = SELF(L, 1);
    uint8_t b = luaL_checkinteger(L, 2);

    pushb(self->mpu, b);

    return 0;
}

/**
 * Pops a byte from the stack.
 *
 * @function mpu:pop
 */
static int
l_mpu_pop(lua_State * L)
{
    LuaMPU *self = SELF(L, 1);

    lua_pushinteger(L, popb(self->mpu));

    return 1;
}

/**
 * Pushes a word onto the stack.
 *
 * @param word
 *
 * @function mpu:pushw
 */
static int
l_mpu_pushw(lua_State * L)
{
    LuaMPU *self = SELF(L, 1);
    uint16_t w = luaL_checkinteger(L, 2);

    pushw(self->mpu, w);

    return 0;
}

/**
 * Pops a word from the stack.
 *
 * @function mpu:popw
 */
static int
l_mpu_popw(lua_State * L)
{
    LuaMPU *self = SELF(L, 1);

    lua_pushinteger(L, popw(self->mpu));

    return 1;
}

/* ------------------------------------------------------------------------ */

/**
 * Callbacks.
 *
 * Callback are the primary means by which you can tie Lua code into the
 * microprocessor.
 *
 * @section
 */

static int
mpu_read_callback(M6502 * mpu, uint16_t addr, uint8_t data)
{
    LuaMPU *self = get_mpu_self(mpu);

    (void) data;

    d_message(("read of addr %x, by ref %d.\n", addr, self->read[addr]));

    /* Push the function: */
    lua_rawgeti(self->L, LUA_REGISTRYINDEX, self->read[addr]);
    /* Push the arguments it's to receive: */
    registry__push_lmpu(self->L, mpu);
    lua_pushinteger(self->L, addr);
    /* Call it: */
    lua_call(self->L, 2, 1);

    /* @todo: Do we want to implicitly convert float to int? 3.4 to 3? It's
     * already the case for Lua 5.1 and 5.2, but 5.3 would return zero if
     * the callback returned float. */
    int result = luaU_pop_integer(self->L);

    return result;
}

static int
mpu_write_callback(M6502 * mpu, uint16_t addr, uint8_t data)
{
    LuaMPU *self = get_mpu_self(mpu);

    d_message(("write of addr %x, by ref %d.\n", addr, self->write[addr]));

    /* Push the function: */
    lua_rawgeti(self->L, LUA_REGISTRYINDEX, self->write[addr]);
    /* Push the arguments it's to receive: */
    registry__push_lmpu(self->L, mpu);
    lua_pushinteger(self->L, addr);
    lua_pushinteger(self->L, data);
    /* Call it: */
    lua_call(self->L, 3, 0);

    return 0;
}

#define OP_BRK 0x00
#define OP_JMP 0x4C
#define OP_JSR 0x20

static int
mpu_call_callback(M6502 * mpu, uint16_t addr, uint8_t inst)
{
    LuaMPU *self = get_mpu_self(mpu);
    int result;

    LUAU_GUARD(self->L);

    if (inst == OP_BRK)
        addr = *(uint16_t *) & self->mpu->memory[0xFFFE];

    d_message(("call of addr %x, by ref %d.\n", addr, self->call[addr]));

    /* Push the function: */
    lua_rawgeti(self->L, LUA_REGISTRYINDEX, self->call[addr]);
    /* Push the arguments it's to receive: */
    registry__push_lmpu(self->L, mpu);
    lua_pushinteger(self->L, addr);
    lua_pushinteger(self->L, inst);
    /* Call it: */
    lua_call(self->L, 3, 1);

    result = luaU_pop_integer(self->L);

    LUAU_UNGUARD(self->L);

    if (inst == OP_JSR && result == 0)
        return popw(mpu) + 1;   /* JSR pushes next insn addr - 1 */
    else
        return result;
}

#undef OP_BRK
#undef OP_JMP
#undef OP_JSR

static void
mpu_on_xxx(lua_State * L, uint16_t addr, int *callbacks_lua, M6502_Callback * callbacks_c,
           M6502_Callback c_handler)
{
    /* Release the previous callback, if installed: */

    if (callbacks_lua[addr] != 0)
    {
        luaL_unref(L, LUA_REGISTRYINDEX, callbacks_lua[addr]);
        callbacks_lua[addr] = 0;
        callbacks_c[addr] = NULL;
    }

    /* Install the new callback, if provided: */

    /* The callback is always at index #3 (it's the `fn` in `mpu:on_write(addr, fn)`). */
    if (!lua_isnoneornil(L, 3))
    {
        if (lua_type(L, 3) != LUA_TFUNCTION)
            luaL_typerror(L, 3, "function");

        lua_pushvalue(L, 3);    // ensure it's at top
        callbacks_lua[addr] = luaL_ref(L, LUA_REGISTRYINDEX);
        callbacks_c[addr] = c_handler;
    }
}

/**
 * Installs a callback to be used when a byte is read from memory.
 *
 * Example:
 *
 *    -- This makes reading the byte at address 0xfe return
 *    -- a random number.
 *
 *    mpu:on_read(0x00fe, function()
 *      return math.random(0, 255)
 *    end)
 *
 * @function mpu:on_read
 *
 * @param addr
 * @param fn The function to install. Or __nil__ to clear any installed one.
 *   The function gets two arguments: the __mpu__ object, and the __address__
 *   read from. The function should return a number (a byte).
 */
static int
l_mpu_on_read(lua_State * L)
{
    LuaMPU *self = SELF(L, 1);
    uint16_t addr = luaM_checkaddr(L, 2);

    mpu_on_xxx(L, addr, self->read, self->mpu->callbacks->read, mpu_read_callback);

    return 0;
}

/**
 * Installs a callback to be used when a byte is written to memory.
 *
 * Example:
 *
 *    -- This makes writing a byte to address 0xf0 print
 *    -- this character on the screen.
 *
 *    mpu:on_write(0x00f0, function(mpu, addr, byte)
 *      print(string.char(byte))
 *    end)
 *
 * Here's a more elaborate example showing how to implement a screen of
 * 32x32 pixels at address 0x200:
 *
 *    local SCREEN_ADDR = 0x200
 *
 *    --
 *    -- Implement the screen.
 *    --
 *    local function setup_screen(mpu)
 *
 *      local function write(mpu, addr, byte)
 *
 *        -- Write the byte to actual memory (although we don't *have* to).
 *        -- Note the 'true' 3'rd parameter: if we omit it, we'll end up
 *        -- calling ourselves recursively.
 *        mpu:poke(addr, byte, true)
 *
 *        local x = (addr - SCREEN_ADDR) % 32
 *        local y = math.floor( (addr - SCREEN_ADDR) / 32 )
 *
 *        -- Write a pixel at (x, y):
 *
 *        draw_pixel(x, y, byte)  -- You'll have to define this function, of course.
 *
 *      end
 *
 *      for addr = SCREEN_ADDR, SCREEN_ADDR + 32*32 - 1 do
 *        mpu:on_write(addr, write)
 *      end
 *
 *    end
 *
 *    local mpu = require('M6502').new()
 *
 *    setup_screen(mpu)
 *
 *    -- The following writes a pixel at coordinates (5, 0)
 *    mpu:poke(0x205, 1)
 *
 * @function mpu:on_write
 *
 * @param addr
 * @param fn The function to install. Or __nil__ to clear any installed one.
 *   The function gets three arguments: the __mpu__ object, and the __address__
 *   written to, and the __byte__ written.
 */
static int
l_mpu_on_write(lua_State * L)
{
    LuaMPU *self = SELF(L, 1);
    uint16_t addr = luaM_checkaddr(L, 2);

    mpu_on_xxx(L, addr, self->write, self->mpu->callbacks->write, mpu_write_callback);

    return 0;
}

/**
 * Installs a callback to be used when an address is called.
 *
 * Example:
 *
 *    local mpu = require('M6502').new()
 *    local hex = require('M6502.utils').parse_hex
 *
 *    -- Install at 0xFFEE a "putchar" routine that prints
 *    -- the character contained in register A.
 *    mpu:on_call(0xFFEE, function(mpu)
 *      print(string.char( mpu:a() ))
 *    end)
 *
 *    -- A program to print the letter "A".
 *    local prog = hex [[
 *      a9 41     ; LDA #65  (65 is ASCII for "A")
 *      20 ee ff  ; JSR $ffee
 *      00        ; BRK
 *    ]]
 *
 *    mpu:pokes(0x600, prog)
 *    mpu:pc(0x600)
 *    mpu:run()
 *
 * @function mpu:on_call
 *
 * @param addr
 * @param fn The function to install. Or __nil__ to clear any installed one.
 *   The function gets two arguments: the __mpu__ object, and the __address__
 *   called (it also get a third --currently undocumented-- argument: the
 *   type of the call (JSR, JMP, BRK)). __If__ the function returns a number,
 *   it will be assigned to the PC register (that is, it's where execution
 *   will resume), but you don't need to bother returning anything if
 *   you're just implementing a JSR target (as in the example above).
 */
static int
l_mpu_on_call(lua_State * L)
{
    LuaMPU *self = SELF(L, 1);
    uint16_t addr = luaM_checkaddr(L, 2);

    mpu_on_xxx(L, addr, self->call, self->mpu->callbacks->call, mpu_call_callback);

    return 0;
}

/* ------------------------------------------------------------------------ */

/**
 * Misc.
 *
 * @section
 */

/**
 * Disassembles one machine instruction.
 *
 * Example:
 *
 *    local text, ins_size = mpu:dis(0x603)
 *    print(text)
 *
 *    -- Print the next instruction.
 *    local text = mpu:dis(0x603 + ins_size)
 *    print(text)
 *
 * See @{M6502.utils.dis_range} for an easier function to use.
 *
 * @param addr The instruction's address.
 *
 * @return The mnemonic assembly code for the instruction.
 * @return The length, in bytes, of the instruction and its operands (this
 *   lets you advance to the next instruction).
 *
 * @function mpu:dis
 */
static int
l_mpu_dis(lua_State * L)
{
    /* input vars */
    LuaMPU *self = SELF(L, 1);
    uint16_t addr = luaM_checkaddr(L, 2);

    /* output vars */
    char insn[64];
    int len;

    len = M6502_disassemble(self->mpu, addr, insn);

    lua_pushstring(L, insn);
    lua_pushinteger(L, len);
    return 2;
}

/**
 * Returns a string describing the MPU status.
 *
 * The values of the registers (and flags) is included.
 *
 * Example:
 *
 *    print( mpu:dump() )
 *
 * (If you want to dump a memory range, see @{M6502.utils.dump_range}.)
 *
 * @function mpu:dump
 */
static int
l_mpu_dump(lua_State * L)
{
    LuaMPU *lmpu = SELF(L, 1);

    /* output var */
    char buf[64];

    M6502_dump(lmpu->mpu, buf);
    lua_pushstring(L, buf);
    return 1;
}

/**
 * Makes the MPU start executing instructions.
 *
 * See @{PC}.
 *
 * The MPU runs till a BRK (`0x00`) instructions is reached. By default the
 * handler for this instruction terminates the program after printing the
 * @{dump|MPU status}.
 *
 * @function mpu:run
 */
static int
l_mpu_run(lua_State * L)
{
    LuaMPU *lmpu = SELF(L, 1);

    M6502_run(lmpu->mpu);
    return 0;
}

static int
l_mpu_gc(lua_State * L)
{
    LuaMPU *self = SELF(L, 1);

    d_message(("deleting %p\n", self));
    M6502_delete(self->mpu);
    return 0;
}

/* ------------------------------------------------------------------------ */

/* *INDENT-OFF* */

static const luaL_Reg functions[] = {
    { "new", l_new },
    { NULL, NULL }
};

static const luaL_Reg mpu_methods[] = {
    { "A", l_mpu_a },
    { "a", l_mpu_a },
    { "X", l_mpu_x },
    { "x", l_mpu_x },
    { "Y", l_mpu_y },
    { "y", l_mpu_y },
    { "P", l_mpu_p },
    { "p", l_mpu_p },
    { "S", l_mpu_s },
    { "s", l_mpu_s },
    { "PC", l_mpu_pc },
    { "pc", l_mpu_pc },
    { "peek", l_mpu_peek },
    { "poke", l_mpu_poke },
    { "peekw", l_mpu_peekw },
    { "pokew", l_mpu_pokew },
    { "peeks", l_mpu_peeks },
    { "pokes", l_mpu_pokes },
    { "push", l_mpu_push },
    { "pop", l_mpu_pop },
    { "pushw", l_mpu_pushw },
    { "popw", l_mpu_popw },
    { "on_read", l_mpu_on_read },
    { "on_write", l_mpu_on_write },
    { "on_call", l_mpu_on_call },
    { "run", l_mpu_run },
    { "dis", l_mpu_dis },
    { "dump", l_mpu_dump },
    { "__gc", l_mpu_gc },
    { NULL, NULL }
};

/* Exported constants */
static const luaU_constReg constants[] = {
    /* We currently export no constants. */
    /* { "RST_Vector", M6502_RSTVector }, */
    { NULL, 0 }
};

/* *INDENT-ON* */

/* ------------------------------------------------------------------------ */

LUALIB_API int
luaopen_M6502(lua_State * L)
{
    registry__create(L);

    luaU_register_metatable(L, "LuaMPU", mpu_methods, TRUE);

    luaL_newlib(L, functions);

    luaU_register_constants(L, constants);

    return 1;
}
