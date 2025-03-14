#pragma once

// set error text. Ownership of string transfers to error module; string will eventually be freed.
#define set_error           retro_script_set_error

// Set error text to the given string. Caller must guarantee string's persistence.
#define set_error_nofree    retro_script_set_error_nofree


#define clear_error         retro_script_clear_error

void retro_script_set_error(const char*);
void retro_script_set_error_nofree(const char*);
void retro_script_clear_error();