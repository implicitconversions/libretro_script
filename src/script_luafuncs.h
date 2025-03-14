#pragma once

#include "hc_luafuncs.h"

int retro_script_luafunc_input_poll(struct lua_State* L);
int retro_script_luafunc_input_state(struct lua_State* L);

int retro_script_luafunc_memory_read_char(struct lua_State* L);
int retro_script_luafunc_memory_read_byte(struct lua_State* L);
int retro_script_luafunc_memory_write_char(struct lua_State* L);
int retro_script_luafunc_memory_write_byte(struct lua_State* L);

#define DECLARE_LUAFUNCS_MEMORY_ACCESS(type, ctype, luatype) \
    DECLARE_LUAFUNCS_MEMORY_ACCESS_ENDIAN(type, ctype, luatype, le) \
    DECLARE_LUAFUNCS_MEMORY_ACCESS_ENDIAN(type, ctype, luatype, be)
    
#define DECLARE_LUAFUNCS_MEMORY_ACCESS_ENDIAN(type, ctype, luatype, le) \
int retro_script_luafunc_memory_read_##type##_##le(struct lua_State* L); \
int retro_script_luafunc_memory_write_##type##_##le(struct lua_State* L);

DECLARE_LUAFUNCS_MEMORY_ACCESS(int16, int16_t, integer);
DECLARE_LUAFUNCS_MEMORY_ACCESS(uint16, uint16_t, integer);
DECLARE_LUAFUNCS_MEMORY_ACCESS(int32, int32_t, integer);
DECLARE_LUAFUNCS_MEMORY_ACCESS(uint32, uint32_t, integer);
DECLARE_LUAFUNCS_MEMORY_ACCESS(int64, int64_t, integer);
DECLARE_LUAFUNCS_MEMORY_ACCESS(uint64, uint64_t, integer);
DECLARE_LUAFUNCS_MEMORY_ACCESS(float32, float, number);
DECLARE_LUAFUNCS_MEMORY_ACCESS(float64, double, number);

// sets various libretro.h constants
void retro_script_luafield_constants(struct lua_State* L);

int retro_script_luafunc_reserve_lram(struct lua_State* L);