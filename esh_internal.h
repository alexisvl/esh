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

#ifndef ESH_INTERNAL_INCLUDE
#error "esh_internal.h is an internal header and should not be included by the user."
#endif // ESH_INTERNAL_INCLUDE

#ifndef ESH_INTERNAL_H
#define ESH_INTERNAL_H

#include <esh_incl_config.h>
#include <esh_hist.h>


/**
 * @internal
 * esh instance struct.
 */
typedef struct esh {
    char buffer[ESH_BUFFER_LEN + 1];
    char * argv[ESH_ARGC_MAX];
    size_t cnt;
    size_t ins;
    uint_fast8_t flags;
    struct esh_hist hist;
#ifndef ESH_STATIC_CALLBACKS
    esh_cb_command cb_command;
    esh_print print;
    esh_overflow overflow;
    void *cb_command_arg;
    void *cb_print_arg;
    void *cb_overflow_arg;
#endif
} esh_t;

/******************************************************************************
 * INTERNAL FUNCTIONS shared by esh.c and esh_hist.c
 */

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
bool esh_putc(esh_t * esh, char c);

/**
 * @internal
 * Print a string, using __memx on AVR.
 */
bool esh_puts(esh_t * esh, char const AVR_ONLY(__memx) * s);

/**
 * @internal
 * Print the prompt
 */
void esh_print_prompt(esh_t * esh);

/**
 * Overwrite the prompt and restore the buffer.
 * @param esh - esh instance
 */
void esh_restore(esh_t * esh);

/**
 * Call the print callback. Wrapper to avoid ifdefs for static callback.
 */
void esh_do_print_callback(esh_t * esh, char c);

/**
 * Call the main callback. Wrapper to avoid ifdefs for static callback.
 */
void esh_do_callback(esh_t * esh, int argc, char ** argv);

/**
 * Call the overflow callback. Wrapper to avoid ifdefs for the static
 * callback.
 */
void esh_do_overflow_callback(esh_t * esh, char const * buffer);


#define ESC_CURSOR_RIGHT    "\33[1C"
#define ESC_CURSOR_LEFT     "\33[1D"
#define ESC_ERASE_LINE      "\33[2K"

#define ESCCHAR_UP      'A'
#define ESCCHAR_DOWN    'B'
#define ESCCHAR_RIGHT   'C'
#define ESCCHAR_LEFT    'D'
#define ESCCHAR_HOME    'H'
#define ESCCHAR_END     'F'

#endif // ESH_INTERNAL_H
