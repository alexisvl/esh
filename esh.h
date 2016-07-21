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
#define ESH_INTERNAL_INCLUDE
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
 */
typedef void (*esh_callback)(struct esh * esh, int argc, char ** argv);

/**
 * Callback to print a character.
 * @param esh - the esh instance calling
 * @param s - the string to print
 */
typedef void (*esh_print)(struct esh * esh, char const * s);

/**
 * Callback to notify about overflow.
 * @param esh - the esh instance calling
 * @param buffer - the internal buffer, NUL-terminated
 * @return TODO document this
 */
typedef int (*esh_overflow)(struct esh * esh, char const * buffer);
#endif // ESH_STATIC_CALLBACKS

/**
 * @internal
 * esh instance struct.
 */
struct esh {
    char buffer[ESH_BUFFER_LEN + 1];
    char * argv[ESH_ARGC_MAX];
    size_t cnt;
    size_t ins;
    uint_fast8_t flags;
    struct esh_hist hist;
#ifndef ESH_STATIC_CALLBACKS
    esh_callback callback;
    esh_print print;
    esh_overflow overflow;
#endif
};

/**
 * Initialize esh. Must be called before any other functions.
 * @param esh - esh instance
 * @return true on error. Can only return true when using malloc to allocate
 * history buffer; in all other cases you can ignore the return value.
 */
bool esh_init(struct esh * esh);

#ifndef ESH_STATIC_CALLBACKS
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
#endif

/**
 * Pass in a character that was received.
 */
void esh_rx(struct esh * esh, char c);

/**
 * Set the location of the history buffer, if ESH_HIST_ALLOC is defined and
 * set to MANUAL. If ESH_HIST_ALLOC is not defined or not set to MANUAL, this
 * is a no-op.
 */
void esh_set_histbuf(struct esh * esh, char * buffer);

/******************************************************************************
 * INTERNAL FUNCTIONS shared by esh.c and esh_hist.c
 */

#ifdef ESH_INTERNAL
#ifdef __AVR_ARCH__
#   define FSTR(s) (__extension__({static const __flash char __c[] = (s); &__c[0];}))
#   define AVR_ONLY(x) x
#else
#   define FSTR(s) (s)
#   define AVR_ONLY(x)
#endif // __AVR_ARCH__

/**
 * @internal
 * Print one character.
 * @return false (allows it to be an esh_hist_for_each_char callback)
 */
bool esh_putc(struct esh * esh, char c);

/**
 * @internal
 * Print a string, using __memx on AVR.
 */
bool esh_puts(struct esh * esh, char const AVR_ONLY(__memx) * s);

/**
 * @internal
 * Print the prompt
 */
void esh_print_prompt(struct esh * esh);

/**
 * Overwrite the prompt and restore the buffer.
 * @param esh - esh instance
 */
void esh_restore(struct esh * esh);

/**
 * Call the print callback. Wrapper to avoid ifdefs for static callback.
 */
void esh_do_print_callback(struct esh * esh, char const * s);

/**
 * Call the main callback. Wrapper to avoid ifdefs for static callback.
 */
void esh_do_callback(struct esh * esh, int argc, char ** argv);

/**
 * Call the overflow callback. Wrapper to avoid ifdefs for the static
 * callback.
 */
void esh_do_overflow_callback(struct esh * esh, char const * buffer);


#define ESC_CURSOR_RIGHT    "\33[1C"
#define ESC_CURSOR_LEFT     "\33[1D"
#define ESC_ERASE_LINE      "\33[2K"

#define ESCCHAR_UP      'A'
#define ESCCHAR_DOWN    'B'
#define ESCCHAR_RIGHT   'C'
#define ESCCHAR_LEFT    'D'
#define ESCCHAR_HOME    'H'
#define ESCCHAR_END     'F'

#endif // ESH_INTERNAL

#endif // ESH_H
