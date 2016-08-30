/**
 * This is a mini-library of utility functions (and macros) for Lua.
 *
 * See 'lutils.c' for details.
 *
 * #Include this file when you want to use Lua's API. It pulls in Lua's
 * headers.
 */
#ifndef LUTILS_CAPI_H
#define LUTILS_CAPI_H

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#ifndef GLIB_MAJOR_VERSION
#  define gboolean int
#  define TRUE 1
#  define FALSE 0
#endif

/* Like gettext's '_'. Marks programmer errors. */
#define E_

/* ------------------------------- Scalars -------------------------------- */

gboolean luaU_pop_boolean(lua_State * L);
lua_Integer luaU_pop_integer(lua_State * L);

/* -------------------------------- Tables -------------------------------- */

void luaU_new_weak_table(lua_State * L, const char *what /* k, v, kv */ );
void luaU_registry_settable(lua_State * L, const char *table_name);
void luaU_registry_gettable(lua_State * L, const char *table_name);

/* ------------------------------- Userdata ------------------------------- */

void *luaU_newuserdata(lua_State * L, size_t size, const char *tname);
void *luaU_newuserdata0(lua_State * L, size_t size, const char *tname);
void *luaU_checkudata__unsafe(lua_State * L, int index, const char *tname);

/* ---------------------- Stuff missing from Lua 5.1 ---------------------- */

#if LUA_VERSION_NUM < 502

int lua_absindex(lua_State * L, int idx);

void luaL_setmetatable(lua_State * L, const char *tname);

void luaL_newlib(lua_State * L, const luaL_Reg * l);

/* Lua 5.1 and 5.2+ have different ways to calc len, so we standardize on 5.2+'s. */
#define lua_rawlen lua_objlen

#define luaL_setfuncs(L, l, n) (luaL_register (L, NULL, l))

#endif

/* --------------------- Borrowings from Lua 5.1 -------------------------- */

#if LUA_VERSION_NUM > 501
int luaL_typerror(lua_State * L, int narg, const char *tname);
#endif

/* ---------------- Registering modules/functions/constants --------------- */

typedef struct luaU_constReg
{
    const char *name;
    int value;
} luaU_constReg;

void luaU_register_constants(lua_State * L, const luaU_constReg * l);
void luaU_register_metatable(lua_State * L, const char *tname, const luaL_Reg * l,
                             gboolean create_index);

/* ------------------------------- Options -------------------------------- */

#define luaU_checkoption(L, n, def, names, values) values[ luaL_checkoption (L, n, def, names) ]

/**
 * luaU_push_option() is the opposite of luaU_checkoption().
 *
 * It's a macro because the type of 'val' and 'values' isn't known.
 */
#define luaU_push_option(L, val, fallback, names, values) \
    do { \
        int i; \
        for (i = 0; names[i] != NULL; i++) \
            if (values[i] == val) { \
                lua_pushstring (L, names[i]); \
                break; \
            } \
        if (names[i] == NULL) \
            lua_pushstring (L, fallback); \
    } while (0)

/* -------------------------- Programming aids ---------------------------- */

void luaU_checkargcount(lua_State * L, int count, gboolean is_method);

/*
 * Use LUAU_GUARD() and LUAU_UNGUARD() to make sure your code's pushes and
 * pops are balanced.
 *
 * (To prevent the 'indent' program from messing things up, put a semicolon
 * after both macros. With LUAU_GUARD() we even manage to force you to do
 * this.)
 */

#define LUAU_GUARD(L)    \
    { \
        int __top = lua_gettop (L)

#define LUAU_UNGUARD(L)    \
        if (lua_gettop (L) != __top) { \
            fprintf (stderr, "Lua stack error: I started at %d, but now am at %d\n", __top, lua_gettop (L)); \
            abort (); \
        } \
    }

#define LUAU_UNGUARD_BY(L, by)    \
        if (lua_gettop (L) != __top + (by)) { \
            fprintf (stderr, "Lua stack error: I started at %d, but now am at %d\n", __top, lua_gettop (L)); \
            abort (); \
        } \
    }

#endif /* MC__LUA_CAPI_H */
