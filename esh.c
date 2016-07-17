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

#include <esh.h>
#include <string.h>
#include <ctype.h>

#ifdef __AVR_ARCH__
#   define FSTR(s) (__extension__({static const __flash char __c[] = (s); &__c[0];}))
#   define AVR(x) x
#else
#   define FSTR(s) (s)
#   define AVR(x)
#endif // __AVR_ARCH__

static int internal_overflow(struct esh const * esh, char const * buffer);
static void execute_command(struct esh * esh);
static void handle_char(struct esh * esh, char c);
static void print_prompt(struct esh * esh);
static int make_arg_array(struct esh * esh);

void esh_init(struct esh * esh)
{
    memset(esh, 0, sizeof(*esh));
}


void esh_register_callback(struct esh * esh, esh_callback callback)
{
    esh->callback = callback;
}


void esh_register_print(struct esh * esh, esh_print print)
{
    esh->print = print;
}


void esh_register_overflow_callback(struct esh * esh, esh_overflow overflow)
{
    esh->overflow = (overflow ? overflow : &internal_overflow);
}


void esh_rx(struct esh * esh, char c)
{
    char c_as_string[2] = {c, 0};

    switch (c) {
    case 0:
        break;
    case '\n':
        esh->print(esh, c_as_string);
        execute_command(esh);
        break;
    default:
        esh->print(esh, c_as_string);
        handle_char(esh, c);
        break;
    }
}


static void execute_command(struct esh * esh)
{
    int argc = make_arg_array(esh);

    if (argc > ESH_ARGC_MAX) {
        esh->overflow(esh, esh->buffer);
    } else if (argc > 0) {
        esh->callback(argc, esh->argv);
    }

    esh->cnt = 0;
    print_prompt(esh);
}


static void handle_char(struct esh * esh, char c)
{
    bool is_bksp = (c == '\b');

    // esh->cnt == ESH_BUFFER_LEN means we're *right* at the limit, and the
    // user can still backspace the last character. Beyond that, the last
    // character has been lost and it doesn't make much sense to backspace
    // it.
    if (esh->cnt > ESH_BUFFER_LEN || (esh->cnt == ESH_BUFFER_LEN && !is_bksp)) {
        esh->cnt = ESH_BUFFER_LEN + 1;
        esh->buffer[ESH_BUFFER_LEN] = 0;
        esh_overflow(esh, esh->buffer);
        return;
    }

    if (is_bksp) {
        --esh_cnt;
    } else {
        esh->buffer[esh->cnt] = c;
        ++esh->cnt;
    }
}


static void print_prompt(struct esh * esh)
{
    // The print callback only needs to take a standard C string - it doesn't
    // have to understand avr-gcc named address spaces on that platform. The
    // string is always passed in RAM. To avoid spending so much RAM to contain
    // the entire printed string, we iterate through it here. Code space is way
    // cheaper than RAM on microcontrollers.

    char const AVR(__flash) * const prompt = FSTR(ESH_PROMPT);

    for (size_t i = 0; prompt[i]; ++i) {
        char c_as_string[2] = {prompt[i], 0};
        esh->print(esh, c_as_string);
    }
}


static int internal_overflow(struct esh const * esh, char const * buffer)
{
    // The print callback only needs to take a standard C string - it doesn't
    // have to understand avr-gcc named address spaces on that platform. The
    // string is always passed in RAM. To avoid spending so much RAM to contain
    // the entire printed string, we iterate through it here. Code space is way
    // cheaper than RAM on microcontrollers.

    (void) buffer;
    char const AVR(__flash) * const err = FSTR("\n\nesh: command buffer overflow\n");

    for (size_t i = 0; err[i]; ++i) {
        char c_as_string[2] = {err[i], 0};
        esh->print(esh, c_as_string);
    }

    return 0;
}


// TODO: add quoting
static int make_arg_array(struct esh * esh)
{
    int argc = 0;
    bool last_was_space = true;

    for (size_t i = 0; i < esh->cnt; ++i) {
        if (isspace(esh->buffer[i])) {
            esh->buffer[i] = 0;
            last_was_space = true;
        } else {
            if (last_was_space) {
                if (argc < ESH_ARGC_MAX) {
                    esh->argv[argc] = &esh->buffer[i];
                }
                ++argc;
            }
            last_was_space = false;
        }
    }
    esh->buffer[ESH_BUFFER_LEN] = 0;
    return argc;
}

#endif // ESH_H
