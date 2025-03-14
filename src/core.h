#pragma once

#include "libretro_script.h"
#include "util.h"
#include <hcdebug.h>

// TODO: clear these on deinit
#define RETRO_SCRIPT_INTERCEPTED_MEMBER(name) \
    RETRO_SCRIPT_DECLT(name) name
extern struct core_t
{
    // these are set by the front-end via the "intercept" functions.
    RETRO_SCRIPT_INTERCEPTED_MEMBER(retro_set_environment);
    RETRO_SCRIPT_INTERCEPTED_MEMBER(retro_get_memory_data);
    RETRO_SCRIPT_INTERCEPTED_MEMBER(retro_get_memory_size);
    RETRO_SCRIPT_INTERCEPTED_MEMBER(retro_init);
    RETRO_SCRIPT_INTERCEPTED_MEMBER(retro_deinit);
    RETRO_SCRIPT_INTERCEPTED_MEMBER(retro_run);
    RETRO_SCRIPT_INTERCEPTED_MEMBER(retro_set_input_poll);
    RETRO_SCRIPT_INTERCEPTED_MEMBER(retro_set_input_state);
    RETRO_SCRIPT_INTERCEPTED_MEMBER(retro_serialize_size);
    RETRO_SCRIPT_INTERCEPTED_MEMBER(retro_serialize);
    RETRO_SCRIPT_INTERCEPTED_MEMBER(retro_unserialize);

    // this is set by the backend via retro_set_environment callback.
    retro_get_proc_address_t retro_get_proc_address;

    // optional, set via retro_get_proc_address
    struct
    {
        hc_Set set_debugger;
        hc_DebuggerIf* debugger;
        void* userdata;
    } hc;
} retro_script_core;
#define core retro_script_core

typedef void (* breakpoint_cb_t)(void* ud, hc_SubscriptionID, hc_Event const*);

extern struct frontend_callbacks_t
{
    retro_environment_t retro_environment;
    retro_input_poll_t retro_input_poll;
    retro_input_state_t retro_input_state;
    breakpoint_cb_t breakpoint_cb;
} retro_script_callbacks;
#define frontend_callbacks retro_script_callbacks

typedef void (*core_init_cb_t)();

// attach a function that runs once every time a new core is created/destroyed
void retro_script_register_on_init(core_init_cb_t);
void retro_script_register_on_deinit(core_init_cb_t);

#define _ON_INIT_GLUE_(a,b) a ## b
#define ON_INIT_GLUE(a,b) _ON_INIT_GLUE_(a,b)

// write code that runs directly when a core is created/destroyed
#define ON_INIT() \
    static void _core_init_fn(); \
    INITIALIZER(ON_INIT_GLUE(_init_core_init_fn, __LINE__)) { retro_script_register_on_init(_core_init_fn); }; \
    static void _core_init_fn()

#define ON_DEINIT() \
    static void _core_deinit_fn(); \
    INITIALIZER(ON_INIT_GLUE(_init_core_deinit_fn, __LINE__)) { retro_script_register_on_deinit(_core_deinit_fn); }; \
    static void _core_deinit_fn()
