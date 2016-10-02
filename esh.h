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

#define ESH_INTERNAL_INCLUDE
#include <esh_incl_config.h>
#include <esh_hist.h>
#undef ESH_INTERNAL_INCLUDE
#include <stddef.h>
#include <stdbool.h>
#include <inttypes.h>

struct esh;

#ifndef ESH_STATIC_CALLBACKS
/**
 * Callback to handle commands.
 * @param argc - number of arguments, including the command name
 * @param argv - arguments
 * @param arg - arbitrary argument you passed to esh_register_command()
 */
typedef void (*esh_cb_command)(esh_t * esh, int argc, char ** argv, void * arg);

/**
 * Callback to print a character.
 * @param esh - the esh instance calling
 * @param c - the character to print
 * @param arg - arbitrary argument you passed to esh_register_print()
 */
typedef void (*esh_print)(esh_t * esh, char c, void * arg);

/**
 * Callback to notify about overflow.
 * @param esh - the esh instance calling
 * @param buffer - the internal buffer, NUL-terminated
 * @param arg - arbitrary argument you passed to esh_register_overflow()
 */
typedef void (*esh_overflow)(esh_t * esh, char const * buffer, void * arg);
#endif // ESH_STATIC_CALLBACKS

/**
 * Return a pointer to an initialized esh object. Must be called before
 * any other functions.
 *
 * See ESH_ALLOC in esh_config.h - this should be STATIC or MALLOC.
 * If STATIC, ESH_INSTANCES must be defined to the maximum number of
 * instances.
 *
 * @return esh instance, or NULL on failure. Failure can only happen in the
 * following cases:
 *  - using malloc to allocate either the esh struct itself or the history
 *      buffer, and malloc returns NULL
 *  - using static allocation and you tried to initialize more than
 *      ESH_INSTANCES.
 */
esh_t * esh_init(void);

#ifndef ESH_STATIC_CALLBACKS
/**
 * Register a callback to execute a command.
 * @param arg - arbitrary argument to pass to the callback
 */
void esh_register_command(esh_t * esh, esh_cb_command callback, void * arg);

/**
 * Register a callback to print a character.
 * @param arg - arbitrary argument to pass to the callback
 */
void esh_register_print(esh_t * esh, esh_print callback, void * arg);

/**
 * Register a callback to notify about overflow. Optional; esh has an internal
 * overflow handler. To reset to that, set the handler to NULL.
 * @param arg - arbitrary argument to pass to the callback
 */
void esh_register_overflow_callback(esh_t * esh, esh_overflow overflow, void * arg);
#endif

/**
 * Pass in a character that was received.
 */
void esh_rx(esh_t * esh, char c);

/**
 * Set the location of the history buffer, if ESH_HIST_ALLOC is defined and
 * set to MANUAL. If ESH_HIST_ALLOC is not defined or not set to MANUAL, this
 * is a no-op.
 */
void esh_set_histbuf(esh_t * esh, char * buffer);

#endif // ESH_H
