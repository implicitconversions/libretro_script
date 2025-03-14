#include "lram.h"

#include "error.h"
#include "script_list.h"

#include <string.h>
#include <assert.h>

typedef struct lua_ram {
    struct lua_ram* next;
    size_t size;
    char data[];
} lua_ram_t;

int retro_script_create_lram(script_state_t* script, size_t size)
{
    if (script == NULL)
        return set_error_nofree("script is NULL."), -1;
    if (size == 0)
        return set_error_nofree("invalid lram size."), -2;
    
    lua_ram_t* lram = malloc(size + sizeof(lua_ram_t));
    if (!lram)
        return set_error_nofree("Unable to allocate lram."), -3;
    
    // initialize lram
    memset(lram, 0, size + sizeof(lua_ram_t));
    lram->size = size;
    
    // add lram to linked list.
    size_t index = 0;
    lua_ram_t** next = &script->lram;
    while (*next) { next = &(*next)->next; ++index; };
    *next = lram;
    
    // return index of lram
    return index;
}

void retro_script_free_lram(script_state_t* script)
{
    if (!script) return;
    
    lua_ram_t* lram = script->lram;
    while (lram)
    {
        lua_ram_t* next = lram->next;
        free(lram);
        lram = next;
    }
    script->lram = NULL;
}

static lua_ram_t* get_lram(script_state_t* script, size_t index)
{
    lua_ram_t* lram = script->lram;
    while (lram && index --> 0)
    {
        lram = lram->next;
    }
    
    return lram;
}

size_t retro_script_get_lram_size(script_state_t* script, size_t index)
{
    lua_ram_t* lram = get_lram(script, index);
    if (!lram) return 0;
    return lram->size;
}

void* retro_script_get_lram_data(script_state_t* script, size_t index)
{
    lua_ram_t* lram = get_lram(script, index);
    if (!lram) return 0;
    return lram->data;
}

size_t retro_script_lram_serialize_size()
{
    size_t size = 0;
    for (script_state_t* script = script_first(); script; script = script->next)
    {
        for (lua_ram_t* lram = script->lram; lram; lram = lram->next)
        {
            size += lram->size;
        }
    }
    return size;
}

void retro_script_lram_serialize(void* _data, size_t size)
{
    char* data = (char*)_data;
    
    for (script_state_t* script = script_first(); script; script = script->next)
    {
        for (lua_ram_t* lram = script->lram; lram; lram = lram->next)
        {
            memcpy(data, lram->data, lram->size);
            
            assert(size >= lram->size);
            data += lram->size;
            size -= lram->size;
        }
    }
    
    assert(size == 0);
}

void retro_script_lram_unserialize(const void* _data, size_t size)
{
    const char* data = (const char*)_data;
    
    for (script_state_t* script = script_first(); script; script = script->next)
    {
        for (lua_ram_t* lram = script->lram; lram; lram = lram->next)
        {
            memcpy(lram->data, data, lram->size);
            
            assert(size >= lram->size);
            data += lram->size;
            size -= lram->size;
        }
    }
    
    assert(size == 0);
}