#include "libretro_script.h"
#include <stddef.h>

#define LUA_TNONE		(-1)

#define LUA_TNIL		0
#define LUA_TBOOLEAN		1
#define LUA_TLIGHTUSERDATA	2
#define LUA_TNUMBER		3
#define LUA_TSTRING		4
#define LUA_TTABLE		5
#define LUA_TFUNCTION		6
#define LUA_TUSERDATA		7
#define LUA_TTHREAD		8

#define LUA_NUMTYPES		9

extern struct retro_script_lua_api retro_script_lua_api_global;

typedef struct lua_State lua_State;
typedef intptr_t lua_KContext;
typedef long long lua_Integer;
typedef double lua_Number;
typedef unsigned long long lua_Unsigned;
typedef int (*lua_KFunction) (lua_State *L, int status, lua_KContext ctx);

// primary functions
#define lua_tointegerx(L, idx, pisnum)  (((lua_Integer(*)(lua_State *, int, int *))retro_script_lua_api_global.lua_tointegerx)(L, idx, pisnum))
#define lua_tonumberx(L, idx, pisnum)   (((lua_Number(*)(lua_State *, int, int *))retro_script_lua_api_global.lua_tonumberx)(L, idx, pisnum))
#define lua_tolstring(L, idx, len)      (((const char*(*)(lua_State*L, int, size_t*))retro_script_lua_api_global.lua_tolstring)(L, idx, len))
#define lua_touserdata(L, idx)          (((void*(*)(lua_State*, int))retro_script_lua_api_global.lua_touserdata)(L, idx))
#define lua_next(L, idx)                (((int(*)(lua_State*, int))retro_script_lua_api_global.lua_next)(L, idx))
#define lua_typename(L, tp)             (((const char*(*)(lua_State *, int))retro_script_lua_api_global.lua_typename)(L, tp))
#define lua_isinteger(L, idx)           (((int(*)(lua_State *L, int))retro_script_lua_api_global.lua_isinteger)(L, idx))
#define lua_type(L, idx)                (((int(*)(lua_State *, int))retro_script_lua_api_global.lua_type)(L, idx))
#define lua_settop(L, n)                (((void(*)(lua_State*, int))retro_script_lua_api_global.lua_settop)(L, n))
#define lua_gettop(L)                   (((int(*)(lua_State *))retro_script_lua_api_global.lua_gettop)(L))
#define lua_createtable(L, narr, nrec)  (((void(*)(lua_State *, int, int))retro_script_lua_api_global.lua_createtable)(L, narr, nrec))
#define lua_pushnil(L)                  (((void(*)(lua_State *))retro_script_lua_api_global.lua_pushnil)(L))
#define lua_pushvalue(L, idx)           (((void(*)(lua_State *, int))retro_script_lua_api_global.lua_pushvalue)(L, idx))
#define lua_pushinteger(L, n)           (((void(*)(lua_State *, lua_Integer))retro_script_lua_api_global.lua_pushinteger)(L, n))
#define lua_pushnumber(L, v)            (((void(*)(lua_State *, lua_Number))retro_script_lua_api_global.lua_pushnumber)(L, v))
#define lua_pushlightuserdata(L, p)     (((void(*)(lua_State *, void *))retro_script_lua_api_global.lua_pushlightuserdata)(L, p))
#define lua_pushstring(L, s)            (((const char*(*)(lua_State *, const char*))retro_script_lua_api_global.lua_pushstring)(L, s))
#define lua_pushcclosure(L, fn, n)      (((void(*)(lua_State *, lua_CFunction, int))retro_script_lua_api_global.lua_pushcclosure)(L, fn, n))
#define lua_rawgeti(L, idx, n)          (((int(*)(lua_State *, int, lua_Integer))retro_script_lua_api_global.lua_rawgeti)(L, idx, n))
#define lua_rawseti(L, idx, n)          (((void(*)(lua_State *, int, lua_Integer))retro_script_lua_api_global.lua_rawseti)(L, idx, n))
#define lua_rawget(L, idx)              (((int(*)(lua_State *, int))retro_script_lua_api_global.lua_rawget)(L, idx))
#define lua_rawset(L, idx)              (((void(*)(lua_State *, int))retro_script_lua_api_global.lua_rawset)(L, idx))
#define lua_setfield(L, idx, k)         (((void(*)(lua_State *, int, char const*))retro_script_lua_api_global.lua_setfield)(L, idx, k))
#define lua_close(L)                    (((void(*)(lua_State *))retro_script_lua_api_global.lua_close)(L))
#define lua_concat(L, n)                (((void(*)(lua_State *, int))retro_script_lua_api_global.lua_concat)(L, n))
#define lua_rotate(L, idx, n)           (((void(*)(lua_State *, int, int))retro_script_lua_api_global.lua_rotate)(L, idx, n))
#define lua_pcallk(L, nargs, nresults, errfunc, ctx, k) (((int(*)(lua_State *, int, int, int, lua_KContext, lua_KFunction))retro_script_lua_api_global.lua_pcallk)(L, nargs, nresults, errfunc, ctx, k))
#define lua_getglobal(L, name)          (((int(*)(lua_State *, const char*))retro_script_lua_api_global.lua_getglobal)(L, name))
#define lua_rawlen(L, n)                (((lua_Unsigned(*)(lua_State *, int))retro_script_lua_api_global.lua_rawlen)(L, n))
#define luaL_ref(L, t)                  (((int(*)(lua_State *, int))retro_script_lua_api_global.luaL_ref)(L, t))
#define luaL_error(L, ...)              (((int(*)(lua_State *, const char *, ...))retro_script_lua_api_global.luaL_error)(L, __VA_ARGS__))
#define luaL_newstate()                 (((lua_State*(*)())retro_script_lua_api_global.luaL_newstate)())
#define luaL_requiref(L, modname, openf, glb) (((void(*)(lua_State *, const char *, lua_CFunction, int))retro_script_lua_api_global.luaL_requiref)(L, modname, openf, glb))
#define luaL_getsubtable(L, idx, fname) (((int(*)(lua_State *, int, const char*))retro_script_lua_api_global.luaL_getsubtable)(L, idx, fname))
#define luaL_loadfilex(L, filename, mode) (((int(*)(lua_State *, const char *, const char *))retro_script_lua_api_global.luaL_loadfilex)(L, filename, mode))
#define luaopen_base ((lua_CFunction)retro_script_lua_api_global.luaopen_base)
#define luaopen_math ((lua_CFunction)retro_script_lua_api_global.luaopen_math)
#define luaopen_string ((lua_CFunction)retro_script_lua_api_global.luaopen_string)
#define luaopen_table ((lua_CFunction)retro_script_lua_api_global.luaopen_table)
#define luaopen_utf8 ((lua_CFunction)retro_script_lua_api_global.luaopen_utf8)
#define luaopen_coroutine ((lua_CFunction)retro_script_lua_api_global.luaopen_coroutine)
#define luaopen_package ((lua_CFunction)retro_script_lua_api_global.luaopen_package)
#define luaopen_debug ((lua_CFunction)retro_script_lua_api_global.luaopen_debug)

// secondary functions
#define lua_isstring(L, idx) (lua_type(L, idx) == LUA_TSTRING)
#define lua_istable(L, idx) (lua_type(L, idx) == LUA_TTABLE)
#define lua_isnil(L, idx) (lua_type(L, idx) == LUA_TNIL)
#define lua_isnumber(L, idx) (lua_type(L, idx) == LUA_TNUMBER)
#define lua_isfunction(L, idx) (lua_type(L, idx) == LUA_TFUNCTION)
#define lua_pop(L, n) lua_settop(L, -(n)-1)
#define lua_pcall(L,n,r,f)	lua_pcallk(L, (n), (r), (f), 0, NULL)
#define lua_tostring(L,i)	lua_tolstring(L, (i), NULL)
#define lua_tonumber(L,i)	lua_tonumberx(L,(i),NULL)
#define lua_tointeger(L,i)	lua_tointegerx(L,(i),NULL)
#define lua_newtable(L)		lua_createtable(L, 0, 0)
#define lua_pushcfunction(L,f)	lua_pushcclosure(L, (f), 0)
#define luaL_loadfile(L,f)	luaL_loadfilex(L,f,NULL)
#define luaL_dofile(L, fn) \
	(luaL_loadfile(L, fn) || lua_pcall(L, 0, LUA_MULTRET, 0))

#define LUA_GNAME	"_G"
#define LUA_MATHLIBNAME	"math"
#define LUA_DBLIBNAME	"debug"
#define LUA_STRLIBNAME	"string"
#define LUA_TABLIBNAME	"table"
#define LUA_LOADLIBNAME	"package"
#define LUA_UTF8LIBNAME	"utf8"
#define LUA_COLIBNAME	"coroutine"

#define LUA_OK		0
#define LUA_YIELD	1
#define LUA_ERRRUN	2
#define LUA_ERRSYNTAX	3
#define LUA_ERRMEM	4
#define LUA_ERRERR	5

#define LUA_NOREF       (-2)
#define LUA_REFNIL      (-1)

// TOOD: make these arguments
#define LUA_REGISTRYINDEX	(-LUAI_MAXSTACK - 1000)
#define lua_upvalueindex(i)	(LUA_REGISTRYINDEX - (i))
#define LUAI_MAXSTACK		1000000

#define LUA_LOADED_TABLE	"_LOADED"

#define LUA_PRELOAD_TABLE	"_PRELOAD"

#define LUA_MULTRET	(-1)