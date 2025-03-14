#include <hcdebug.h>
#include "libretro_script.h"

#include "script.h"
#include "script_list.h"
#include "memmap.h"
#include "hc_hooks.h"
#include "core.h"
#include "error.h"
#include "lram.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

// convenience
#define DECLT(x) RETRO_SCRIPT_DECLT(x)
#define INTERCEPT(name) RETRO_SCRIPT_DECLT(name) retro_script_intercept_##name(RETRO_SCRIPT_DECLT(name) f)
#define INTERCEPT_HANDLER(name) _interceptor_##name

struct core_t core;
struct frontend_callbacks_t frontend_callbacks;

enum
{
    RS_INIT, // retro-script init'd, but core not yet init'd
    CORE_INIT, // core init'd
    CORE_DEINIT, // core deinit'd (may be skipped if frontend does not deinit core)
    RS_DEINIT // retro-script de-init'd
} state = RS_DEINIT;

// how many core_on_init/deinit can be registered
#define MAX_INIT_FUNCTIONS 8

static size_t retro_script_core_init_count = 0;
static size_t retro_script_core_deinit_count = 0;
static core_init_cb_t core_on_init_fn[MAX_INIT_FUNCTIONS];
static core_init_cb_t core_on_deinit_fn[MAX_INIT_FUNCTIONS];

// registers functions to run on init
void retro_script_register_on_init(core_init_cb_t cb)
{
    assert(retro_script_core_init_count < MAX_INIT_FUNCTIONS);
    assert(cb);
    core_on_init_fn[retro_script_core_init_count++] = cb;
}

void retro_script_register_on_deinit(core_init_cb_t cb)
{
    assert(retro_script_core_deinit_count < MAX_INIT_FUNCTIONS);
    assert(cb);
    core_on_deinit_fn[retro_script_core_deinit_count++] = cb;
}

uint32_t retro_script_init()
{
    #ifdef RETRO_SCRIPT_DEBUG
    retro_script_debug_force_include();
    #endif

    if (state == RS_DEINIT)
    {
        memset(&core, 0, sizeof(core));
        memset(&frontend_callbacks, 0, sizeof(frontend_callbacks));
        retro_script_clear_memory_map();
    }
    else
    {
        // de-init before init'ing
        retro_script_deinit();
    }

    // run init functions
    for (size_t i = 0; i < retro_script_core_init_count; ++i)
    {
        core_on_init_fn[i]();
    }

    state = RS_INIT;
    return RETRO_SCRIPT_API_VERSION;
}

void retro_script_deinit()
{
    if (state != RS_DEINIT)
    {
        for (size_t i = 0; i < retro_script_core_deinit_count; ++i)
        {
            core_on_deinit_fn[i]();
        }
        retro_script_clear_memory_map();
    }

    state = RS_DEINIT;
}

#define CHECK_INTERCEPT(name) \
    if (!core.name) \
        printf("[retro script] WARNING: missing intercept: %1$s. See RETRO_SCRIPT_INTERCEPT(%1$s) in libretro_script.h\n", #name);

static void retro_check_intercepts()
{
    CHECK_INTERCEPT(retro_set_environment);
    CHECK_INTERCEPT(retro_get_memory_data);
    CHECK_INTERCEPT(retro_get_memory_size);
    CHECK_INTERCEPT(retro_init);
    CHECK_INTERCEPT(retro_deinit);
    CHECK_INTERCEPT(retro_run);
    CHECK_INTERCEPT(retro_set_input_poll);
    CHECK_INTERCEPT(retro_set_input_state);
    
    // if we have one, we need them all.
    if (core.retro_serialize_size || core.retro_serialize || core.retro_unserialize)
    {
        CHECK_INTERCEPT(retro_serialize_size);
        CHECK_INTERCEPT(retro_serialize);
        CHECK_INTERCEPT(retro_unserialize);
    }
}

static void INTERCEPT_HANDLER(retro_run)(void)
{
    SCRIPT_ITERATE(script_state)
    {
        retro_script_execute_cb(script_state, script_state->refs.on_run_begin);
    }
    core.retro_run();
    SCRIPT_ITERATE(script_state)
    {
        retro_script_execute_cb(script_state, script_state->refs.on_run_end);
    }
}

static bool retro_environment(unsigned int cmd, void* data)
{
    bool result = false;

    switch (cmd)
    {
    case RETRO_ENVIRONMENT_SET_MEMORY_MAPS:
        result = retro_script_set_memory_map((struct retro_memory_map*)data);
        break;
    case RETRO_ENVIRONMENT_SET_PROC_ADDRESS_CALLBACK:
        core.retro_get_proc_address = ((struct retro_get_proc_address_interface*)data)->get_proc_address;
        result = !!core.retro_get_proc_address;
        break;
    }

    return frontend_callbacks.retro_environment(cmd, data) || result;
}

static void INTERCEPT_HANDLER(retro_set_environment)(retro_environment_t cb)
{
    frontend_callbacks.retro_environment = cb;
    core.retro_set_environment(retro_environment);
}

static void INTERCEPT_HANDLER(retro_init)(void)
{
    assert(state == RS_INIT);
    retro_check_intercepts();
    core.retro_init();
    state = CORE_INIT;
}

static void INTERCEPT_HANDLER(retro_deinit)(void)
{ 
    assert(state == CORE_INIT);
    core.retro_deinit();
    state = CORE_DEINIT;
}

static void INTERCEPT_HANDLER(retro_set_input_poll)(retro_input_poll_t cb)
{
    frontend_callbacks.retro_input_poll = cb;
    core.retro_set_input_poll(cb);
}

static void INTERCEPT_HANDLER(retro_set_input_state)(retro_input_state_t cb)
{
    frontend_callbacks.retro_input_state = cb;
    core.retro_set_input_state(cb);
}

static void* INTERCEPT_HANDLER(retro_get_memory_data)(unsigned v)
{
    return core.retro_get_memory_data(v);
}

static size_t INTERCEPT_HANDLER(retro_get_memory_size)(unsigned v)
{
    return core.retro_get_memory_size(v);
}

static size_t INTERCEPT_HANDLER(retro_serialize_size)(void)
{
    // serialize the core as usual, but append lua-allocated ram if applicable.
    
    return core.retro_serialize_size() + retro_script_lram_serialize_size();
}

static bool INTERCEPT_HANDLER(retro_serialize)(void* data, size_t size)
{
    // serialize the core as usual, but append lua-allocated ram if applicable.
    
    const size_t lram_size = retro_script_lram_serialize_size();
    if (lram_size > size) return false;
    
    const bool success = core.retro_serialize(data, size - lram_size);
    if (!success) return false;
    
    retro_script_lram_serialize((char*)data + size - lram_size, lram_size);
    return true;
}

static bool INTERCEPT_HANDLER(retro_unserialize)(const void* data, size_t size)
{
    // unserialize the core as usual, and unserialize lua-allocated ram if applicable.
    const size_t lram_size = retro_script_lram_serialize_size();
    if (lram_size > size) return false;
    
    const bool success = core.retro_unserialize(data, size - lram_size);
    if (!success) return false;
    
    retro_script_lram_unserialize((const char*)data + size - lram_size, lram_size);
    return true;
}

#define DEFINE_INTERCEPT(name) \
    INTERCEPT(name) { return (core.name = f), INTERCEPT_HANDLER(name); }

DEFINE_INTERCEPT(retro_set_environment)
DEFINE_INTERCEPT(retro_get_memory_data)
DEFINE_INTERCEPT(retro_get_memory_size)
DEFINE_INTERCEPT(retro_init)
DEFINE_INTERCEPT(retro_deinit)
DEFINE_INTERCEPT(retro_run)
DEFINE_INTERCEPT(retro_set_input_poll) 
DEFINE_INTERCEPT(retro_set_input_state)
DEFINE_INTERCEPT(retro_serialize_size)
DEFINE_INTERCEPT(retro_serialize)
DEFINE_INTERCEPT(retro_unserialize)