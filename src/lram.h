#pragma once

#include "script.h"

#include <stdlib.h>
#include <stdint.h>

// returns index of lram (lua ram), or negative on failure.
// Should be called before the first retro_run().
int retro_script_create_lram(script_state_t*, size_t size);

// frees all lram associated with the given script.
void retro_script_free_lram(script_state_t*);

// retrieves data/size of lram.
size_t retro_script_get_lram_size(script_state_t*, size_t index); // 0: error
void* retro_script_get_lram_data(script_state_t*, size_t index); // nullptr: error

size_t retro_script_lram_serialize_size();
void retro_script_lram_serialize(void* data, size_t size); // never fails
void retro_script_lram_unserialize(const void* data, size_t size); // never fails