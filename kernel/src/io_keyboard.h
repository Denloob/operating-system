#pragma once

#include "compiler_macros.h"
#include "res.h"
#include <stdbool.h>

int io_keyboard_wait_key();

#define res_SELF_TEST_FAILED "Keyboard self test failed"
res io_keyboard_reset_and_self_test() WUR;

// Check if a keyboard key was pressed and ready to be read.
//  When it is, io_keyboard_wait_key will return without blocking.
bool io_keyboard_is_key_ready() WUR;
