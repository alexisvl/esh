/* esh - embedded shell
 *
 * Copyright (c) 2016, Chris Pavlina.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef ESH_H
#define ESH_H

#include "esh_config.h"
#include <stddef.h>
#include <stdbool.h>
#include <inttypes.h>

struct esh;

/**
 * Callback to handle commands.
 * @param argc - number of arguments, including the command name
 * @param argv - arguments
 */
typedef void (*esh_callback)(int argc, char ** argv);

/**
 * Callback to print a character.
 * @param esh - the esh instance calling
 * @param s - the string to print
 */
typedef void (*esh_print)(struct esh const * esh, char const * s);

/**
 * Callback to notify about overflow.
 * @param esh - the esh instance calling
 * @param buffer - the internal buffer, NUL-terminated
 * @return TODO document this
 */
typedef int (*esh_overflow)(struct esh const * esh, char const * buffer);

/**
 * @internal
 * esh instance struct.
 */
struct esh {
    char buffer[ESH_BUFFER_LEN + 1];
    size_t cnt;
    char * argv[ESH_ARGC_MAX];
    esh_callback callback;
    esh_print print;
    esh_overflow overflow;
    uint_fast8_t flags;
};

/**
 * Initialize esh. Must be called before any other functions.
 * @param esh - esh instance
 */
void esh_init(struct esh * esh);

/**
 * Register a callback to execute a command.
 */
void esh_register_callback(struct esh * esh, esh_callback callback);

/**
 * Register a callback to print a character.
 */
void esh_register_print(struct esh * esh, esh_print print);

/**
 * Register a callback to notify about overflow. Optional; esh has an internal
 * overflow handler. To reset to that, set the handler to NULL.
 */
void esh_register_overflow_callback(struct esh * esh, esh_overflow overflow);

/**
 * Pass in a character that was received.
 */
void esh_rx(struct esh * esh, char c);

#endif // ESH_H
