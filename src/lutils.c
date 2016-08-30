/**
 * This is a mini-library of utility functions (and macros) for Lua.
 *
 * It's independent of the rest of the application.
 *
 * This library, in lutils.{c,h} (which stands for "Lua utils"),
 * serves two purposes:
 *
 * - It handles compatibility issues between the different Lua versions.
 *   E.g., it ensures lua_absindex() always exists.
 *
 * - It provides utility functions that make it easy to do common Lua tasks.
 *
 * Rule:
 *
 * All the functions here are *independent* of your application. Please
 * keep it this way.
 *
 * Style:
 *
 * We follow "luaL"'s example by prefixing our functions' names with
 * "luaU".
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>             /* memset() */
#include <assert.h>

#include "lutils.h"

/* ----------------------------- Scalars ---------------------------------- */

gboolean
luaU_pop_boolean(lua_State * L)
{
    gboolean b;

    b = lua_toboolean(L, -1);
    lua_pop(L, 1);
    return b;
}

lua_Integer
luaU_pop_integer(lua_State * L)
{
    lua_Integer i;

    i = lua_tointeger(L, -1);
    lua_pop(L, 1);
    return i;
}

/* ------------------------------- Tables --------------------------------- */

/* Creates a weak table. */
void
luaU_new_weak_table(lua_State * L, const char *what /* k, v, kv */ )
{
    lua_newtable(L);            /* the table itself. */
    lua_newtable(L);            /* its meta table. */
    lua_pushstring(L, what);
    lua_setfield(L, -2, "__mode");
    lua_setmetatable(L, -2);
}

/**
 * A utility function. Just like lua_gettable(), but accepts a table name
 * instead of its index.
 *
 * Pseudo:
 *
 *    k = stack.pop()
 *    stack.push( registry[table_name][k] )
 */
void
luaU_registry_gettable(lua_State * L, const char *table_name)
{
    lua_getfield(L, LUA_REGISTRYINDEX, table_name);
    lua_insert(L, -2);
    lua_gettable(L, -2);
    lua_remove(L, -2);
}

/**
 * A utility function. Just like lua_settable(), but accepts a table name
 * instead of its index.
 *
 * Pseudo:
 *
 *    registry[table_name][ stack[-2] ] = stack[-1]
 *    stack.pop(2)
 */
void
luaU_registry_settable(lua_State * L, const char *table_name)
{
    lua_getfield(L, LUA_REGISTRYINDEX, table_name);
    lua_insert(L, -3);
    lua_settable(L, -3);
    lua_pop(L, 1);
}

/* ----------------------------- Userdata --------------------------------- */

/**
 * Like lua_newuserdata() but also calls luaL_setmetatable().
 *
 * Warning: if your userdata holds a pointer, and your metatable's __gc frees
 * this pointer, make sure not to run code similar to:
 *
 *    MyData *p = luaU_newuserdata (L, sizeof(MyData), "myprog.MyData");
 *    ... code which may raise exception ...
 *    p->the_pointer = whatever;
 *
 * In this case, if an exception is raised, your __gc will be called with a
 * garbage address (use luaU_newuserdata0() instead if it's a likely
 * scenario for you).
 */
void *
luaU_newuserdata(lua_State * L, size_t size, const char *tname)
{
    void *p;

    p = lua_newuserdata(L, size);
    luaL_setmetatable(L, tname);
    return p;
}

/**
 * Like luaU_newuserdata(), but also zeros all the bytes.
 */
void *
luaU_newuserdata0(lua_State * L, size_t size, const char *tname)
{
    void *p;

    p = lua_newuserdata(L, size);
    memset(p, 0, size);
    luaL_setmetatable(L, tname);
    return p;
}

/**
 * Like luaL_checkudata() but doesn't check for the udata type. So it's faster.
 *
 * The 'tname' doesn't need to be a real metatable name: since it's only used
 * in error messages, you may pick something that users are more likely to
 * understand.
 */
void *
luaU_checkudata__unsafe(lua_State * L, int index, const char *tname)
{
    void *p;

    p = lua_touserdata(L, index);
    if (p)
        return p;
    else
        return luaL_checkudata(L, index, tname);
}

/* ---------------------- Stuff missing from Lua 5.1 ---------------------- */

#if LUA_VERSION_NUM < 502

int
lua_absindex(lua_State * L, int idx)
{
    int top = lua_gettop(L);
    if (idx < 0 && -idx <= top)
        return top + idx + 1;
    else
        return idx;
}

void
luaL_setmetatable(lua_State * L, const char *tname)
{
    luaL_getmetatable(L, tname);
    lua_setmetatable(L, -2);
}

void
luaL_newlib(lua_State * L, const luaL_Reg * l)
{
    lua_newtable(L);
    luaL_setfuncs(L, l, 0);
}

#endif

/* --------------------- Borrowings from Lua 5.1 -------------------------- */

#if LUA_VERSION_NUM > 501

int
luaL_typerror(lua_State * L, int narg, const char *tname)
{
    const char *info = lua_pushfstring(L, E_("%s expected, got %s"), tname, luaL_typename(L, narg));
    luaL_argerror(L, narg, info);
    return 0;                   /* We never reach here. */
}

#endif

/* ---------------- Registering modules/functions/constants --------------- */

/* Registers constants with the table at top of stack. */
void
luaU_register_constants(lua_State * L, const luaU_constReg * l)
{
    while (l->name)
    {
        lua_pushinteger(L, l->value);
        lua_setfield(L, -2, l->name);
        ++l;
    }
}

/**
 * Creates a metatable.
 *
 * Since it's common to have an '__index' field pointing to self, this
 * function also optionally does this for you.
 */
void
luaU_register_metatable(lua_State * L, const char *tname, const luaL_Reg * l, gboolean create_index)
{
    luaL_newmetatable(L, tname);
    luaL_setfuncs(L, l, 0);
    if (create_index)
    {
        lua_pushvalue(L, -1);
        lua_setfield(L, -2, "__index");
    }
}

/* -------------------------- Programming aids ---------------------------- */

/**
 * Ensures the function doesn't have more than 'count' arguments.
 *
 * (We don't check for less or equal to 'count': the user may leave off
 * optional arguments.)
 */
void
luaU_checkargcount(lua_State * L, int count, gboolean is_method)
{
    if (lua_gettop(L) > count)
    {
        if (is_method)
            luaL_error(L, E_("Too many arguments for method; only %d expected"), count - 1);
        else
            luaL_error(L, E_("Too many arguments for function; only %d expected"), count);
    }
}
