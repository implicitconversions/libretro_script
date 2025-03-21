#include "script_luafuncs.h"
#include "hc_hooks.h"

#include "libretro_script.h"
#include "script.h"
#include "script_list.h"
#include "error.h"
#include "memmap.h"
#include "core.h"
#include "util.h"
#include "l.h"

#include <stdio.h>

// NOTE: There is an issue with the ON_INIT() and ON_DEINIT() macros where it creates symbols with the same name if they are
// on the same line in different source files.  As a workaround for now I have nudged them to different lines to avoid this error
// but we should fix the macro to avoid this issue in the future.

// clear all scripts when a core is loaded
ON_INIT()
{
    script_clear_all();
}

// clear all scripts when a core is unloaded
ON_DEINIT()
{
    script_clear_all();
}

// default lua error response
// we manipulate the error message to add the stack trace
static int attach_stacktrace(lua_State* L)
{
    if (!lua_isstring(L, 1))
    {
        // don't modify non-string error.
        return 1;
    }
    lua_getglobal(L, LUA_DBLIBNAME);
    if (!lua_istable(L, -1))
    {
        lua_pop(L, 1);
        lua_pushstring(L, "(unable to attach stacktrace -- no debug lib) ");
        lua_rotate(L, -2, 1);
        lua_concat(L, 2);
        return 1;
    }
    lua_rawgetfield(L, -1, "traceback");
    if (!lua_isfunction(L, -1))
    {
        lua_pop(L, 2);
        lua_pushstring(L, "(unable to attach stacktrace -- debug.traceback corrupted or missing) ");
        lua_rotate(L, -2, 1);
        lua_concat(L, 2);
        return 1;
    }
    lua_pushvalue(L, 1); // error
    lua_pushinteger(L, 2);  // skip fn and traceback
    lua_pcall(L, 2, 1, 0);  // debug.traceback()
    return 1;
}

static const char* lua_status_string(int status)
{
    switch (status)
    {
    case LUA_OK:
        return "OK";
    case LUA_YIELD:
        return "YIELD";
    case LUA_ERRRUN:
        return "RUNTIME ERROR";
    case LUA_ERRSYNTAX:
        return "SYNTAX ERROR";
    case LUA_ERRMEM:
        return "MEMORY ERROR";
    case LUA_ERRERR:
        // TODO: double check -- this is an error while handling an error?
        return "HANDLING ERROR";
    default:
        return "UNKOWN ERROR";
    }
}

static void print_error_message(retro_script_id_t id, int status, const char* error_msg)
{
    // TODO: if id == 0, clarify that the retro script id is unknown.
    if (error_msg)
    {
        printf("%s occurred in retro-script %d: %s\n", lua_status_string(status), id, error_msg);
    }
    else
    {
        printf("%s occurred in retro-script %d without message.\n", lua_status_string(status), id);
    }
    fflush(stdout);
}

// sets retro script error mesage from top of stack
static const char* get_lua_error_string(lua_State* L)
{
    if (lua_gettop(L) == 0)
    {
        return "Lua error, but stack empty (cannot display)";
    }

    if (!lua_isstring(L, -1))
    {
        return "Lua error, but not a string (cannot display)";
    }
    else
    {
        lua_pushstring(L, "Lua error: ");
        lua_concat(L, 2);
        const char* errmsg = lua_tostring(L, -1);
        if (errmsg)
        {
            return errmsg;
        }
        else
        {
            return "Lua error (cannot display)";
        }
    }
}

// lua error response (customizable)
static lua_CFunction lua_on_error = attach_stacktrace;
static retro_script_lua_uncaught_error_cb lua_on_uncaught_error = print_error_message;

void retro_script_on_uncaught_error(lua_State* L, int status)
{
    if (status == 0) return;
    script_state_t* script = script_find_lua(L);
    if (lua_on_uncaught_error) lua_on_uncaught_error(script ? script->id : 0, status, get_lua_error_string(L));
}

#define SET_SCRIPT_REF(ref) retro_script_luafunc_set_##ref
#define DEF_SET_SCRIPT_REF(REF, ALLOW_LIST) \
static int SET_SCRIPT_REF(REF)(struct lua_State* L) \
{   \
    script_state_t* script = script_find_lua(L); \
    if (script) \
    {   \
        if (script->refs.REF == LUA_NOREF) \
        { \
            lua_newtable(L); \
            script->refs.REF = luaL_ref(L, LUA_REGISTRYINDEX);\
        } \
        lua_rawgeti(L, LUA_REGISTRYINDEX, script->refs.REF); \
        lua_rotate(L, -2, 1); \
        if (!ALLOW_LIST) \
        { \
            lua_rawseti(L, -2, 1); \
        } \
        else { \
            int idx = lua_rawlen(L, -2) + 1; \
            lua_rawseti(L, -2, idx); \
        } \
        lua_pop(L, 1); \
    }   \
    return 0; \
}

DEF_SET_SCRIPT_REF(on_run_begin, true);
DEF_SET_SCRIPT_REF(on_run_end, true);

RETRO_SCRIPT_API
void retro_script_load_lua_baselibs(lua_State* L)
{
    luaL_requiref(L, LUA_GNAME, luaopen_base, true);
    luaL_requiref(L, LUA_LOADLIBNAME, luaopen_package, true);
    luaL_requiref(L, LUA_MATHLIBNAME, luaopen_math, true);
    luaL_requiref(L, LUA_STRLIBNAME, luaopen_string, true);
    luaL_requiref(L, LUA_TABLIBNAME, luaopen_table, true);
    luaL_requiref(L, LUA_UTF8LIBNAME, luaopen_utf8, true);
    luaL_requiref(L, LUA_COLIBNAME, luaopen_coroutine, true);
    luaL_requiref(L, LUA_DBLIBNAME, luaopen_debug, true);
}

RETRO_SCRIPT_API
bool retro_script_set_package_path(lua_State* L, char const* spath, const char* default_package_path) {
    int type = lua_getglobal(L, "package");
    if (type == LUA_TNIL || type == LUA_TNONE) {
        fprintf(stderr, "retro_script_set_package_path: lua package library hasn't been loaded yet. Please load luaopen_package.\n");
        return 0;
    }
    lua_pushstring(L, default_package_path);
    lua_setfield(L, -2, spath);
    lua_pop(L, 1); // get rid of package table from top of stack

    return 1;
}

// sets up the libretro-script functions.
RETRO_SCRIPT_API
void retro_script_register_retro_global(lua_State* L)
{
    // register c functions
    #define REGISTER_FUNC(name, func) \
        lua_pushcfunction(L, func); \
        lua_rawsetfield(L, -2, name)
    #define REGISTER_SCRIPT_SET_REF(ref) \
        REGISTER_FUNC(#ref, SET_SCRIPT_REF(ref))

    #define REGISTER_MEMORY_ACCESS_ENDIAN(type, le) \
        REGISTER_FUNC("read_" #type "_" #le, retro_script_luafunc_memory_read_##type##_##le); \
        REGISTER_FUNC("write_" #type "_" #le, retro_script_luafunc_memory_write_##type##_##le); \

    #define REGISTER_MEMORY_ACCESS(type) \
        REGISTER_MEMORY_ACCESS_ENDIAN(type, le); \
        REGISTER_MEMORY_ACCESS_ENDIAN(type, be);

    // create a global 'retro'
    lua_newtable(L);

    REGISTER_FUNC("input_poll", retro_script_luafunc_input_poll);
    REGISTER_FUNC("input_state", retro_script_luafunc_input_state);

    retro_script_luafield_constants(L);

    REGISTER_FUNC("read_char", retro_script_luafunc_memory_read_char);
    REGISTER_FUNC("write_char", retro_script_luafunc_memory_write_char);
    REGISTER_FUNC("read_byte", retro_script_luafunc_memory_read_byte);
    REGISTER_FUNC("write_byte", retro_script_luafunc_memory_write_byte);

    REGISTER_MEMORY_ACCESS(int16);
    REGISTER_MEMORY_ACCESS(uint16);
    REGISTER_MEMORY_ACCESS(int32);
    REGISTER_MEMORY_ACCESS(uint32);
    REGISTER_MEMORY_ACCESS(int64);
    REGISTER_MEMORY_ACCESS(uint64);
    REGISTER_MEMORY_ACCESS(float32);
    REGISTER_MEMORY_ACCESS(float64);

    REGISTER_SCRIPT_SET_REF(on_run_begin);
    REGISTER_SCRIPT_SET_REF(on_run_end);

    // retro.hc
    if (retro_script_hc_get_debugger())
    {
        lua_newtable(L);

        REGISTER_FUNC("system_get_description", retro_script_luafunc_hc_system_get_description);
        REGISTER_FUNC("system_get_memory_regions", retro_script_luafunc_hc_system_get_memory_regions);
        REGISTER_FUNC("system_get_breakpoints", retro_script_luafunc_hc_system_get_breakpoints);
        REGISTER_FUNC("system_get_cpus", retro_script_luafunc_hc_system_get_cpus);
        REGISTER_FUNC("breakpoint_clear", retro_script_luafunc_hc_breakpoint_clear);

        retro_script_luafield_hc_main_cpu_and_memory(L);

        lua_rawsetfield(L, -2, "hc");
    }

    //REGISTER_FUNC("reserve_save_data", retro_script_luafunc_reserve_save_data);
    REGISTER_FUNC("reserve_lram", retro_script_luafunc_reserve_lram);

    // set this as package.loaded["retro"]
    luaL_getsubtable(L, LUA_REGISTRYINDEX, LUA_LOADED_TABLE);
    lua_rotate(L, -2, 1);
    lua_rawsetfield(L, -2, "retro");

    // lazy cleanup.
    lua_settop(L, 0);
}

/*static void set_error_and_free_L(lua_State* L) {
    set_error(lua_tostring(L, -1));
    lua_pop(L, 1);
    script_state_t* state = script_find_lua(L);
    if (state) {
        script_free(state->id);
    }
}*/

static void set_error_and_free(script_state_t* state) {
    if (!state) return;
    lua_State* L = state->L;
    set_error(lua_tostring(L, -1));
    lua_pop(L, 1);
    script_free(state->id);
}


RETRO_SCRIPT_API
void retro_script_set_lua_error_handler(lua_CFunction cb)
{
    lua_on_error = cb;
}

RETRO_SCRIPT_API
lua_CFunction retro_script_get_lua_error_handler(void)
{
    return lua_on_error;
}

RETRO_SCRIPT_API
void retro_script_set_lua_uncaught_error_handler(retro_script_lua_uncaught_error_cb cb)
{
    lua_on_uncaught_error = cb;
}

int retro_script_lua_pcall(lua_State* L, int argc, int retc)
{
    if (argc > 0)
    {
        // move function before arguments.
        lua_rotate(L, -argc-1, 1);
    }
    if (lua_on_error == NULL)
    {
        return lua_pcall(L, argc, retc, 0);
    }
    else
    {
        lua_pushcfunction(L, lua_on_error);
        lua_rotate(L, -argc-2, 1);
        return lua_pcall(L, argc, retc, -argc-2);
    }
}

void retro_script_execute_cb(script_state_t* script, int ref)
{
    if (!script || ref == LUA_NOREF || ref == LUA_REFNIL)
    {
        return;
    }

    lua_State* L = script->L;

    lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
    if (lua_isfunction(L, -1))
    {
        int result = retro_script_lua_pcall(L, 0, 0);
        if (result != LUA_OK)
        {
            if (lua_on_uncaught_error) lua_on_uncaught_error(script->id, result, get_lua_error_string(L));
            lua_pop(L, 1);
        }
    }
    else if (lua_istable(L, -1))
    {
        const int len = lua_rawlen(L, -1);
        const int top = lua_gettop(L);
        for (int i = 1; i <= len; ++i)
        {
            lua_rawgeti(L, -1, i);
            int result = retro_script_lua_pcall(L, 0, 0);
            if (result != LUA_OK)
            {
                if (lua_on_uncaught_error) lua_on_uncaught_error(script->id, result, get_lua_error_string(L));
                lua_pop(L, 1);
            }
            lua_settop(L, top);
        }
    }
    lua_settop(L, 0);
}

struct retro_script_lua_api retro_script_lua_api_global;
RETRO_SCRIPT_API bool retro_script_lua_init(const struct retro_script_lua_api* api)
{
    memcpy(&retro_script_lua_api_global, api, sizeof(struct retro_script_lua_api));
    return true;
}

RETRO_SCRIPT_API lua_State* retro_script_alloc()
{
    script_state_t* script_state = script_alloc();
    if (!script_state)
    {
        set_error_nofree("Unable to allocate script");
        return NULL;
    }
    return script_state->L;
}

RETRO_SCRIPT_API bool retro_script_core_setup(lua_State* L)
{
    int no_error = 1; // becomes 0 if error.

    // core can modify lua state.
    if (core.retro_get_proc_address)
    {
        retro_script_setup_lua_t core_setup = (retro_script_setup_lua_t)core.retro_get_proc_address("retro_script_setup_lua");
        if (core_setup)
        {
            lua_getglobal(L, "retro");
            no_error = core_setup(L);
            lua_pop(L, 1);
        }
    }

    if (no_error) return 1;

    // set an error but let the caller decide if they want to give up or continue on.
    set_error(lua_tostring(L, -1));
    lua_pop(L, 1);

    return 0;
}

RETRO_SCRIPT_API bool retro_script_lua_dofile(lua_State* L, const char* script_path)
{
    if (!luaL_dofile(L, script_path)) {
        return 1;
    }

    set_error_and_free(script_find_lua(L));
    return 0;
}


static void set_default_package_path(lua_State* L, const char* script_path)
{
    char* p, *f, *packagepath = NULL;
    retro_script_split_path_file(&p, &f, script_path);
    if (f) free(f);
    if (p)
    {
        size_t plen = strlen(p);
        packagepath = malloc(plen + 1 + strlen("?.lua"));
        memcpy(packagepath, p, plen);
        strcpy(packagepath + plen, "?.lua");
        free(p);
    }

    // loadlib ("package") gets special attention, as we set the path manually.
    if (p) {
        retro_script_set_package_path(L, "path", packagepath);
    }
    if (packagepath) free(packagepath);
}

static bool load_lua_special(lua_State* L, const char* script_path, retro_script_setup_lua_t frontend_setup)
{
    if (!L) return 0;

    script_state_t* state = script_find_lua(L);
    retro_script_load_lua_baselibs(L);
    retro_script_register_retro_global(L);

    set_default_package_path(L, script_path);

    if (!retro_script_core_setup(L)) {
        if (state) {
            script_free(state->id);
        }
        return 0;
    }

    // front-end can modify lua state
    if (frontend_setup)
    {
        lua_getglobal(L, "retro");
        int no_error = frontend_setup(L);
        lua_pop(L, 1);

        if (!no_error) {
            set_error_and_free(state);
            return 0;
        }
    }

    if (!retro_script_lua_dofile(L, script_path)) {
        return 0;
    }

    return state->id;
}

RETRO_SCRIPT_API retro_script_id_t retro_script_load_lua(const char* script_path)
{
    lua_State* L = retro_script_alloc();
    return load_lua_special(L, script_path, NULL);
}

RETRO_SCRIPT_API retro_script_id_t retro_script_load_lua_special(const char* script_path, retro_script_setup_lua_t setup_func)
{
    lua_State* L = retro_script_alloc();
    return load_lua_special(L, script_path, setup_func);
}
