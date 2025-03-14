#include "hc_hooks.h"
#include "core.h"
#include "hashmap.h"
#include "error.h"

#include <hcdebug.h>

typedef struct breakpoint_entry
{
    retro_script_hc_breakpoint_userdata userdata;
    retro_script_breakpoint_cb cb;
} breakpoint_entry;

static struct retro_script_hashmap* breakpoint_hashmap = NULL;

ON_INIT()
{
    if (breakpoint_hashmap) retro_script_hashmap_destroy(breakpoint_hashmap);
    breakpoint_hashmap = retro_script_hashmap_create(sizeof(breakpoint_entry));
}

// clear all scripts when a core is unloaded
ON_DEINIT()
{
    if (breakpoint_hashmap) retro_script_hashmap_destroy(breakpoint_hashmap);
}

static void on_breakpoint(void* ud, hc_SubscriptionID id, hc_Event const* event)
{
    // check if this is one of ours...
    breakpoint_entry* entry = (breakpoint_entry*)(
        retro_script_hashmap_get(breakpoint_hashmap, id)
    );

    if (entry)
    {
        entry->cb(entry->userdata, id, event);
    }

    // otherwise, forward breakpoint callback to frontend
    else if (frontend_callbacks.breakpoint_cb)
    {
        frontend_callbacks.breakpoint_cb(ud, id, event);
    }
}

int retro_script_hc_register_breakpoint(retro_script_hc_breakpoint_userdata const* userdata, hc_SubscriptionID breakpoint_id, retro_script_breakpoint_cb cb)
{
    if (!cb) return 1;
    if (!retro_script_hc_get_debugger()) return 1;

    breakpoint_entry* entry = (breakpoint_entry*)(
        retro_script_hashmap_add(breakpoint_hashmap, breakpoint_id)
    );

    if (!entry) return 1;
    memcpy(&entry->userdata, userdata, sizeof(*userdata));
    entry->cb = cb;
    return 0;
}

int retro_script_hc_unregister_breakpoint(hc_SubscriptionID breakpoint_id)
{
    if (!retro_script_hc_get_debugger()) return 1;
    return !retro_script_hashmap_remove(breakpoint_hashmap, breakpoint_id);
}

static int init_debugger(hc_DebuggerIf* debugger)
{
    if (debugger->frontend_api_version != 0 && debugger->frontend_api_version != HC_API_VERSION)
    {
        retro_script_set_error("HC_API_VERSION mismatch");
        return 0;
    }
    *(unsigned*)&debugger->frontend_api_version = HC_API_VERSION;
    frontend_callbacks.breakpoint_cb = debugger->v1.handle_event;
    *(breakpoint_cb_t*)&debugger->v1.handle_event = on_breakpoint;
    return 1;
}

int retro_script_set_debugger(hc_DebuggerIf* debugger)
{
    if (core.hc.debugger != NULL)
    {
        set_error_nofree("Debugger already set. (Remember to call retro_script_set_debugger before retro_init).");
        return 0;
    }
    core.hc.debugger = debugger;
    return init_debugger(debugger);
}

// use this debugger if frontend doesn't provide one.
static hc_DebuggerIf g_debugger;

hc_DebuggerIf* retro_script_hc_get_debugger()
{
    // obtain debugger if we don't have it yet
    if (!core.hc.debugger)
    {
        if (!core.hc.set_debugger)
        {
            if (core.retro_get_proc_address)
            {
                core.hc.set_debugger = (hc_Set)core.retro_get_proc_address("hc_set_debugger");
                if (!core.hc.set_debugger)
                {
                    // backup -- try with 3 gs, this is api v1
                    core.hc.set_debugger = (hc_Set)core.retro_get_proc_address("hc_set_debuggger");
                }
                if (!core.hc.set_debugger)
                {
                    // no debugger available
                    return NULL;
                }
            }
            else
            {
                // no debugger available.
                return NULL;
            }
        }

        // initialize debugger
        core.hc.debugger = &g_debugger;
        memset(&g_debugger, 0, sizeof(g_debugger));
        if (init_debugger(core.hc.debugger) == 0)
        {
            return NULL;
        }
        core.hc.set_debugger(core.hc.debugger);
    }

    return core.hc.debugger;
}
