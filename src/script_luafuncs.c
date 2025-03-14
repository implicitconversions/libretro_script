#include "l.h"
#include "script_luafuncs.h"
#include "memmap.h"
#include "core.h"
#include "script.h"
#include "script_list.h"
#include "lram.h"

int retro_script_luafunc_input_poll(lua_State* L)
{
    if (frontend_callbacks.retro_input_poll)
    {
        frontend_callbacks.retro_input_poll();
    }
    return 0;
}

int retro_script_luafunc_input_state(lua_State* L)
{
    // validate args
    int n = lua_gettop(L);
    if (n != 4) return 0; // number of args
    if (!lua_isinteger(L, 1)) return 0; // port
    if (!lua_isinteger(L, 2)) return 0; // device
    if (!lua_isinteger(L, 3)) return 0; // index
    if (!lua_isinteger(L, 4)) return 0; // id
    
    if (frontend_callbacks.retro_input_state)
    {
        int16_t result = frontend_callbacks.retro_input_state(
            lua_tointeger(L, 1),
            lua_tointeger(L, 2),
            lua_tointeger(L, 3),
            lua_tointeger(L, 4)
        );
        
        lua_pushinteger(L, result);
        return 1;
    }
    else
    {
        return 0;
    }
}

static int _lua_error(lua_State* L, const char* message)
{
    return luaL_error(L, "%s", message);
}

static int luafunc_lram_peek(lua_State* L)
{
    // validate arguments
    if (lua_gettop(L) != 2) return _lua_error(L, "invalid number of arguments to \"lram:peek\"");
    if (!lua_istable(L, 1)) return _lua_error(L, "lram table expected as first argument for \"lram:peek\"");
    if (lua_rawgetfield(L, 1, "index") != LUA_TNUMBER) return _lua_error(L, "lram table missing 'index' field (\"lram:peek\")");
    int index = lua_tointeger(L, -1);
    if (!lua_istable(L, 1)) return _lua_error(L, "lram table expected as first argument for \"lram:peek\"");
    if (!lua_isinteger(L, 2)) return _lua_error(L, "invalid address value for \"lram:peek\""); // index
    int address = lua_tointeger(L, 2);
    
    script_state_t* script = script_find_lua(L);
    if (!script) return _lua_error(L, "invalid lua context");
    
    void* lram_data = retro_script_get_lram_data(script, index);
    size_t lram_size = retro_script_get_lram_size(script, index);
    if (lram_data == NULL) return _lua_error(L, "no associated lram data found (\"lram:peek\")");
    if (address < 0 || (size_t)address >= lram_size)
        return _lua_error(L, "address out of bounds (\"lram:peek\")");
    
    // retrieve value
    lua_pushinteger(L, ((unsigned char*)lram_data)[address]);
    return 1;
}

static int luafunc_lram_size(lua_State* L)
{
    // validate arguments
    if (lua_gettop(L) != 1) return _lua_error(L, "invalid number of arguments to \"lram:__len\"");
    if (!lua_istable(L, 1)) return _lua_error(L, "lram table expected as first argument for \"lram:poke\"");
    if (lua_rawgetfield(L, 1, "index") != LUA_TNUMBER) return _lua_error(L, "lram table missing 'index' field (\"lram:poke\")");
    int index = lua_tointeger(L, -1);
    
    script_state_t* script = script_find_lua(L);
    if (!script) return _lua_error(L, "invalid lua context");
    size_t lram_size = retro_script_get_lram_size(script, index);
    lua_pushinteger(L, lram_size);
    return 1;
}

static int luafunc_lram_poke(lua_State* L)
{
    // validate arguments
    if (lua_gettop(L) != 3) return _lua_error(L, "invalid number of arguments to \"lram:poke\"");
    if (!lua_istable(L, 1)) return _lua_error(L, "lram table expected as first argument for \"lram:poke\"");
    if (lua_rawgetfield(L, 1, "index") != LUA_TNUMBER) return _lua_error(L, "lram table missing 'index' field (\"lram:poke\")");
    int index = lua_tointeger(L, -1);
    if (!lua_istable(L, 1)) return _lua_error(L, "lram table expected as first argument for \"lram:poke\"");
    if (!lua_isinteger(L, 2)) return _lua_error(L, "invalid address for \"lram:poke\"");
    int address = lua_tointeger(L, 2);
    
    if (!lua_isinteger(L, 3)) return _lua_error(L, "invalid value for \"lram:poke\"");
    int value = lua_tointeger(L, 3);
    
    script_state_t* script = script_find_lua(L);
    if (!script) return _lua_error(L, "invalid lua context");
    
    void* lram_data = retro_script_get_lram_data(script, index);
    size_t lram_size = retro_script_get_lram_size(script, index);
    if (lram_data == NULL) return _lua_error(L, "no associated lram data found (\"lram:poke\")");

    if (address < 0 || (size_t)address >= lram_size)
        return _lua_error(L, "address out of bounds (\"lram:poke\")");
    
    // set value
    ((unsigned char*)lram_data)[address] = (unsigned char)value;
    return 0;
}

// could be renamed to "lram_new" if we added a metatable
int retro_script_luafunc_reserve_lram(lua_State* L)
{
    // validate args
    if (lua_gettop(L) != 1) return _lua_error(L, "invalid number of arguments to reserve_lram");
    if (!lua_isinteger(L, 1)) return _lua_error(L, "invalid argument \"size\" for reserve_lram");
    
    int size = lua_tointeger(L, 1);
    if (size <= 0) return _lua_error(L, "invalid argument \"size\" for reserve_lram");
    
    if (!core.retro_serialize) return 0;
    
    script_state_t* script = script_find_lua(L);
    if (!script) return _lua_error(L, "invalid lua context");
    
    int index = retro_script_create_lram(script, size);
    if (index < 0) return _lua_error(L, "failed to create lram");
    
    // create lram table
    {
        lua_newtable(L);
        
        // note: we could accelerate further with a direct pointer to lua_ram_t*
        lua_pushinteger(L, index);
        lua_rawsetfield(L, -2, "index");
        
        lua_pushcfunction(L, luafunc_lram_peek);
        lua_rawsetfield(L, -2, "peek");
        
        lua_pushcfunction(L, luafunc_lram_poke);
        lua_rawsetfield(L, -2, "poke");
        
        lua_pushcfunction(L, luafunc_lram_size);
        lua_rawsetfield(L, -2, "size");
    }
    
    return 1;
}

// adds the macro twice, one with the full name and again with the RETRO_ prefix cropped.
static void registerIntMacro(struct lua_State* L, int value, const char* name) {
    const char prefix[] = "RETRO_";
    const size_t prefixLen = sizeof(prefix) - 1;

    lua_pushinteger(L, value);
    lua_rawsetfield(L, -2, name);

    if (strncmp(name, prefix, prefixLen) == 0)
    {
      lua_pushinteger(L, value);
      lua_rawsetfield(L, -2, name + prefixLen);
    }
};

void retro_script_luafield_constants(struct lua_State* L)
{
#define REGISTER_INT_MACRO(macro) registerIntMacro(L, macro, #macro)

    REGISTER_INT_MACRO(RETRO_DEVICE_NONE);
    REGISTER_INT_MACRO(RETRO_DEVICE_JOYPAD);
    REGISTER_INT_MACRO(RETRO_DEVICE_MOUSE);
    REGISTER_INT_MACRO(RETRO_DEVICE_KEYBOARD);
    REGISTER_INT_MACRO(RETRO_DEVICE_LIGHTGUN);
    REGISTER_INT_MACRO(RETRO_DEVICE_ANALOG);
    REGISTER_INT_MACRO(RETRO_DEVICE_POINTER);
    
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_JOYPAD_B);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_JOYPAD_Y);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_JOYPAD_SELECT);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_JOYPAD_START);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_JOYPAD_UP);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_JOYPAD_DOWN);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_JOYPAD_LEFT);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_JOYPAD_RIGHT);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_JOYPAD_A);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_JOYPAD_X);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_JOYPAD_L);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_JOYPAD_R);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_JOYPAD_L2);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_JOYPAD_R2);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_JOYPAD_L3);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_JOYPAD_R3);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_JOYPAD_MASK);
    
    REGISTER_INT_MACRO(RETRO_DEVICE_INDEX_ANALOG_LEFT);
    REGISTER_INT_MACRO(RETRO_DEVICE_INDEX_ANALOG_RIGHT);
    REGISTER_INT_MACRO(RETRO_DEVICE_INDEX_ANALOG_BUTTON);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_ANALOG_X);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_ANALOG_Y);
    
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_MOUSE_X);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_MOUSE_Y);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_MOUSE_LEFT);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_MOUSE_RIGHT);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_MOUSE_WHEELUP);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_MOUSE_WHEELDOWN);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_MOUSE_MIDDLE);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_MOUSE_HORIZ_WHEELUP);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_MOUSE_HORIZ_WHEELDOWN);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_MOUSE_BUTTON_4);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_MOUSE_BUTTON_5);
    
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_LIGHTGUN_SCREEN_X);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_LIGHTGUN_SCREEN_Y);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_LIGHTGUN_IS_OFFSCREEN);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_LIGHTGUN_TRIGGER);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_LIGHTGUN_RELOAD);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_LIGHTGUN_AUX_A);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_LIGHTGUN_AUX_B);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_LIGHTGUN_START);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_LIGHTGUN_SELECT);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_LIGHTGUN_AUX_C);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_LIGHTGUN_DPAD_UP);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_LIGHTGUN_DPAD_DOWN);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_LIGHTGUN_DPAD_LEFT);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_LIGHTGUN_DPAD_RIGHT);
    
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_POINTER_X);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_POINTER_Y);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_POINTER_PRESSED);
    REGISTER_INT_MACRO(RETRO_DEVICE_ID_POINTER_COUNT);
    
    REGISTER_INT_MACRO(RETRO_REGION_NTSC);
    REGISTER_INT_MACRO(RETRO_REGION_PAL);

#undef REGISTER_INT_MACRO
}

int retro_script_luafunc_memory_read_char(lua_State* L)
{
    int n = lua_gettop(L);
    if (n > 0 && lua_isinteger(L, -1))
    {
        lua_Integer addr = lua_tointeger(L, -1);
        if (addr < 0) return 0; // invalid usage
        
        char out;
        if (retro_script_memory_read_char(addr, &out))
        {
            lua_pushinteger(L, out);
            return 1;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        // invalid usage.
        return 0;
    }
}

int retro_script_luafunc_memory_read_byte(lua_State* L)
{
    int n = lua_gettop(L);
    if (n >= 1 && lua_isinteger(L, -1))
    {
        lua_Integer addr = lua_tointeger(L, -1);
        if (addr < 0) return 0; // invalid usage
        
        unsigned char out;
        if (retro_script_memory_read_byte(addr, &out))
        {
            lua_pushinteger(L, out);
            return 1;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        // invalid usage.
        return 0;
    }
}

int retro_script_luafunc_memory_write_char(lua_State* L)
{
    int n = lua_gettop(L);
    if (n >= 2 && lua_isinteger(L, -1) && lua_isinteger(L, -2))
    {
        lua_Integer addr = lua_tointeger(L, -2);
        if (addr < 0) return 0; // invalid usage
        
        char in = lua_tointeger(L, -1);
        lua_pushinteger(L,
            retro_script_memory_write_char(addr, in)
        );
        
        return 1;
    }
    else
    {
        // invalid usage.
        return 0;
    }
}

int retro_script_luafunc_memory_write_byte(lua_State* L)
{
    int n = lua_gettop(L);
    if (n >= 2 && lua_isinteger(L, -1) && lua_isinteger(L, -2))
    {
        lua_Integer addr = lua_tointeger(L, -2);
        if (addr < 0) return 0; // invalid usage
        
        unsigned char in = lua_tointeger(L, -1);
        lua_pushinteger(L,
            retro_script_memory_write_byte(addr, in)
        );
        
        return 1;
    }
    else
    {
        // invalid usage.
        return 0;
    }
}

#define DEFINE_LUAFUNCS_MEMORY_ACCESS(type, ctype, luatype) \
    DEFINE_LUAFUNCS_MEMORY_ACCESS_ENDIAN(type, ctype, luatype, le) \
    DEFINE_LUAFUNCS_MEMORY_ACCESS_ENDIAN(type, ctype, luatype, be)

#define DEFINE_LUAFUNCS_MEMORY_ACCESS_ENDIAN(type, ctype, luatype, le) \
int retro_script_luafunc_memory_read_##type##_##le(lua_State* L) \
{ \
    int n = lua_gettop(L); \
    if (n >= 1 && lua_isinteger(L, -1)) \
    { \
        lua_Integer addr = lua_tointeger(L, -1); \
        if (addr < 0) return 0; \
        ctype out; \
        if (retro_script_memory_read_##type##_##le(addr, &out)) \
        { \
            lua_push##luatype(L, out); \
            return 1; \
        } \
    } \
    return 0; \
} \
int retro_script_luafunc_memory_write_##type##_##le(lua_State* L) \
{ \
    int n = lua_gettop(L); \
    if (n >= 2 && lua_isinteger(L, -2) && lua_is##luatype(L, -1)) \
    { \
        lua_Integer addr = lua_tointeger(L, -2); \
        if (addr < 0) return 0; \
        ctype in = lua_to##luatype(L, -1); \
        lua_pushinteger(L, \
            retro_script_memory_write_##type##_##le(addr, in) \
        ); \
        return 1; \
    } \
    return 0; \
}

DEFINE_LUAFUNCS_MEMORY_ACCESS(int16, int16_t, integer);
DEFINE_LUAFUNCS_MEMORY_ACCESS(uint16, uint16_t, integer);
DEFINE_LUAFUNCS_MEMORY_ACCESS(int32, int32_t, integer);
DEFINE_LUAFUNCS_MEMORY_ACCESS(uint32, uint32_t, integer);
DEFINE_LUAFUNCS_MEMORY_ACCESS(int64, int64_t, integer);
DEFINE_LUAFUNCS_MEMORY_ACCESS(uint64, uint64_t, integer);
DEFINE_LUAFUNCS_MEMORY_ACCESS(float32, float, number);
DEFINE_LUAFUNCS_MEMORY_ACCESS(float64, double, number);