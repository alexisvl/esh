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

#define ESH_INTERNAL
#include <esh.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#define ESH_INTERNAL_INCLUDE
#include <esh_argparser.h>

enum esh_flags {
    IN_ESCAPE = 0x01,
    IN_BRACKET_ESCAPE = 0x02,
};

static void internal_overflow(esh_t * esh, char const * buffer);
static void execute_command(esh_t * esh);
static void handle_char(esh_t * esh, char c);
static void handle_esc(esh_t * esh, char esc);
static void handle_ctrl(esh_t * esh, char c);
static void ins_del(esh_t * esh, char c);
static void term_cursor_move(esh_t * esh, int n);
static void cursor_move(esh_t * esh, int n);

#ifdef ESH_STATIC_CALLBACKS
void ESH_PRINT_CALLBACK(esh_t * esh, char const * s);
void ESH_CALLBACK(esh_t * esh, int argc, char ** argv);

void __attribute__((weak)) ESH_OVERFLOW_CALLBACK(esh_t * esh,
        char const * buffer)
{
    internal_overflow(esh, buffer);
}


void esh_do_print_callback(esh_t * esh, char const * s)
{
    ESH_PRINT_CALLBACK(esh, s);
}


void esh_do_callback(esh_t * esh, int argc, char ** argv)
{
    ESH_CALLBACK(esh, argc, argv);
}


void esh_do_overflow_callback(esh_t * esh, char const * buffer)
{
    ESH_OVERFLOW_CALLBACK(esh, buffer);
}

#else // ESH_STATIC_CALLBACKS

void esh_do_print_callback(esh_t * esh, char const * s)
{
    esh->print(esh, s);
}


void esh_do_callback(esh_t * esh, int argc, char ** argv)
{
    esh->callback(esh, argc, argv);
}


void esh_do_overflow_callback(esh_t * esh, char const * buffer)
{
    esh->overflow(esh, buffer);
}


void esh_register_callback(esh_t * esh, esh_callback callback)
{
    esh->callback = callback;
}


void esh_register_print(esh_t * esh, esh_print print)
{
    esh->print = print;
}


void esh_register_overflow_callback(esh_t * esh, esh_overflow overflow)
{
    esh->overflow = (overflow ? overflow : &internal_overflow);
}

#endif // ESH_STATIC_CALLBACKS

bool esh_init(esh_t * esh)
{
    memset(esh, 0, sizeof(*esh));
#ifndef ESH_STATIC_CALLBACKS
    esh->overflow = internal_overflow;
#endif

    return esh_hist_init(esh);
}


void esh_rx(esh_t * esh, char c)
{
    if (esh->flags & IN_BRACKET_ESCAPE) {
        if (isalpha(c)) {
            esh->flags &= ~(IN_ESCAPE | IN_BRACKET_ESCAPE);
            handle_esc(esh, c);
        }
    } else if (esh->flags & IN_ESCAPE) {
        if (c == '[' || c == 'O') {
            esh->flags |= IN_BRACKET_ESCAPE;
        } else {
            esh->flags &= ~(IN_ESCAPE | IN_BRACKET_ESCAPE);
        }
    } else {
        if (isprint(c)) {
            handle_char(esh, c);
        } else {
            handle_ctrl(esh, c);
        }
    }
}


static void handle_ctrl(esh_t * esh, char c)
{
    switch (c) {
        case 27: // escape
            esh->flags |= IN_ESCAPE;
            break;
        case 3:  // ^C
            esh_puts(esh, FSTR("^C\n"));
            esh_print_prompt(esh);
            esh->cnt = esh->ins = 0;
            break;
        case '\n':
            execute_command(esh);
            break;
        case 8:     // backspace
        case 127:   // delete
            esh_hist_substitute(esh);
            if (esh->cnt > 0 && esh->cnt <= ESH_BUFFER_LEN) {
                ins_del(esh, 0);
            }
            break;
        default:
            // nop
            ;
    }
}


static void handle_esc(esh_t * esh, char esc)
{
    int cdelta;

    switch (esc) {
    case ESCCHAR_UP:
    case ESCCHAR_DOWN:
        if (esc == ESCCHAR_UP) {
            ++esh->hist.idx;
        } else if (esh->hist.idx) {
            --esh->hist.idx;
        }
        if (esh->hist.idx) {
            int offset = esh_hist_nth(esh, esh->hist.idx - 1);
            if (offset >= 0 || esc == ESCCHAR_DOWN) {
                esh_hist_print(esh, offset);
            } else if (esc == ESCCHAR_UP) {
                // Don't overscroll the top
                --esh->hist.idx;
            }
        } else {
            esh_restore(esh);
        }
        break;

    case ESCCHAR_LEFT:
        cdelta = -1;
        goto cmove;
    case ESCCHAR_RIGHT:
        cdelta = 1;
        goto cmove;
    case ESCCHAR_HOME:
        cdelta = -esh->ins;
        goto cmove;
    case ESCCHAR_END:
        cdelta = esh->cnt - esh->ins;
        goto cmove;
    }

    return;
cmove: // micro-optimization, yo!
    cursor_move(esh, cdelta);
}


static void execute_command(esh_t * esh)
{
    esh_hist_substitute(esh);
    esh_putc(esh, '\n');

    // Have to add to the history buffer before make_arg_array messes it up.
    // However, we do not want to add empty commands.
    bool cmd_is_nop = true;
    esh->buffer[esh->cnt] = 0;
    for (size_t i = 0; esh->buffer[i]; ++i) {
        if (!isspace(esh->buffer[i])) {
            cmd_is_nop = false;
            break;
        }
    }

    if (!cmd_is_nop) {
        esh_hist_add(esh, esh->buffer);
    }

    int argc = esh_parse_args(esh);

    if (argc > ESH_ARGC_MAX) {
        esh_do_overflow_callback(esh, esh->buffer);
    } else if (argc > 0) {
        esh_do_callback(esh, argc, esh->argv);
    }

    esh->cnt = esh->ins = 0;
    esh_print_prompt(esh);
}


static void handle_char(esh_t * esh, char c)
{
    esh_hist_substitute(esh);

    if (esh->cnt >= ESH_BUFFER_LEN) {
        esh->cnt = ESH_BUFFER_LEN + 1;
        esh->buffer[ESH_BUFFER_LEN] = 0;
        esh_do_overflow_callback(esh, esh->buffer);
        return;
    }

    ins_del(esh, c);
}


void esh_print_prompt(esh_t * esh)
{
    esh_puts(esh, FSTR(ESH_PROMPT));
}


static void internal_overflow(esh_t * esh, char const * buffer)
{
    (void) buffer;
    esh_puts(esh, FSTR("\n\nesh: command buffer overflow\n"));
}


bool esh_putc(esh_t * esh, char c)
{
    char c_as_string[] = {c, 0};
    esh_do_print_callback(esh, c_as_string);
    return false;
}


bool esh_puts(esh_t * esh, char const AVR_ONLY(__memx) * s)
{
    for (size_t i = 0; s[i]; ++i) {
        esh_putc(esh, s[i]);
    }
    return false;
}


void esh_restore(esh_t * esh)
{
    esh_puts(esh, FSTR(ESC_ERASE_LINE "\r")); // Clear line
    esh_print_prompt(esh);
    esh->buffer[esh->cnt] = 0;
    esh_puts(esh, esh->buffer);
    // Move cursor back again to the insertion point. Easier to loop than to
    // printf the number into the esc sequence...
    term_cursor_move(esh, -(int)(esh->cnt - esh->ins));
}


/**
 * Move only the terminal cursor. This does not move the insertion point.
 */
static void term_cursor_move(esh_t * esh, int n)
{
    for ( ; n > 0; --n) {
        esh_puts(esh, FSTR(ESC_CURSOR_RIGHT));
    }

    for ( ; n < 0; ++n) {
        esh_puts(esh, FSTR(ESC_CURSOR_LEFT));
    }
}


/**
 * Move the esh cursor. This applies history substitution, moves the terminal
 * cursor, and moves the insertion point.
 */
static void cursor_move(esh_t * esh, int n)
{
    esh_hist_substitute(esh);
    if ((int) esh->ins + n < 0) {
        n = -esh->ins;
    } else if ((int) esh->ins + n > (int) esh->cnt) {
        n = esh->cnt - esh->ins;
    }

    term_cursor_move(esh, n);
    esh->ins += n;
}


/**
 * Either insert or delete a character at the current insertion point.
 * @param esh - esh instance
 * @param c - character to insert, or 0 to delete
 */
static void ins_del(esh_t * esh, char c)
{
    int sgn = c ? 1 : -1;
    bool move = (esh->ins != esh->cnt);

    memmove(&esh->buffer[esh->ins + sgn], &esh->buffer[esh->ins],
            esh->cnt - esh->ins);

    if (c) {
        esh->buffer[esh->ins] = c;
    }

    esh->cnt += sgn;
    esh->ins += sgn;

    if (move) {
        esh_restore(esh);
    } else if (!c) {
        esh_puts(esh, FSTR("\b \b"));
    } else {
        esh_putc(esh, c);
    }
}
