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

enum esh_flags {
    IN_ESCAPE = 0x01,
    IN_BRACKET_ESCAPE = 0x02,
};

static int internal_overflow(struct esh * esh, char const * buffer);
static void execute_command(struct esh * esh);
static void handle_char(struct esh * esh, char c);
static void handle_esc(struct esh * esh, char esc);
static void handle_ctrl(struct esh * esh, char c);
static int make_arg_array(struct esh * esh);
static void ins_del(struct esh * esh, char c);
static void cursor_move(struct esh * esh, int n);

bool esh_init(struct esh * esh)
{
    memset(esh, 0, sizeof(*esh));
    esh->overflow = internal_overflow;

    return esh_hist_init(esh);
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


static void handle_ctrl(struct esh * esh, char c)
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


static void handle_esc(struct esh * esh, char esc)
{
    bool vert_up = false;

    switch (esc) {
    case 'A': // UP
        vert_up = true;
        // fall through
    case 'B': // DOWN
        {
            if (vert_up) {
                ++esh->hist.idx;
            } else if (esh->hist.idx) {
                --esh->hist.idx;
            }
            if (esh->hist.idx) {
                int offset = esh_hist_nth(esh, esh->hist.idx - 1);
                if (offset >= 0 || !vert_up) {
                    esh_hist_print(esh, offset);
                } else if (vert_up) {
                    // Don't overscroll the top
                    --esh->hist.idx;
                }
            } else {
                esh_restore(esh);
            }
            break;
        }

    case 'C': // RIGHT
        if (esh->ins < esh->cnt) {
            ++esh->ins;
            esh_puts(esh, FSTR(ESC_CURSOR_RIGHT));
        }
        break;
    case 'D': // LEFT
        if (esh->ins) {
            --esh->ins;
            esh_puts(esh, FSTR(ESC_CURSOR_LEFT));
        }
        break;
    case 'H': // HOME
        cursor_move(esh, -esh->ins);
        esh->ins = 0;
        break;
    case 'F': // END
        cursor_move(esh, esh->cnt - esh->ins);
        esh->ins = esh->cnt;
        break;
    default:
        break;
    }
}


static void execute_command(struct esh * esh)
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

    int argc = make_arg_array(esh);

    if (argc > ESH_ARGC_MAX) {
        esh->overflow(esh, esh->buffer);
    } else if (argc > 0) {
        esh->callback(esh, argc, esh->argv);
    }

    esh->cnt = esh->ins = 0;
    esh_print_prompt(esh);
}


static void handle_char(struct esh * esh, char c)
{
    esh_hist_substitute(esh);

    if (esh->cnt >= ESH_BUFFER_LEN) {
        esh->cnt = ESH_BUFFER_LEN + 1;
        esh->buffer[ESH_BUFFER_LEN] = 0;
        esh->overflow(esh, esh->buffer);
        return;
    }

    ins_del(esh, c);
}


void esh_print_prompt(struct esh * esh)
{
    esh_puts(esh, FSTR(ESH_PROMPT));
}


static int internal_overflow(struct esh * esh, char const * buffer)
{
    (void) buffer;
    esh_puts(esh, FSTR("\n\nesh: command buffer overflow\n"));
    return 0;
}


/**
 * Map the buffer to the argv array, and return argc. If argc exceeds the
 * maximum, the full buffer will still be processed; argument pointers will
 * just not be stored beyond the maximum. The number that would have been
 * stored is returned.
 *
 * Handles whitespace and quotes. Following is the buffer before and after
 * processing (# for NUL), with pointers stored in argv[] marked with ^
 *
 *
 * before: git   config user.name "My Name"
 * after:  git###config#user.name#My Name#
 * argv:   ^     ^      ^         ^
 *
 * Rearranging the buffer is necessary because quotes can occur in the
 * middle of arguments. For example:
 *
 * before: why" would you ever"'"'"do this??"
 * after:  why would you ever"do this??#
 * argv:   ^
 */
static int make_arg_array(struct esh * esh)
{
    int argc = 0;
    bool last_was_space = true;
    size_t dest = 0;
    char quote = 0;

    for (size_t i = 0; i < esh->cnt; ++i) {
        if (quote) {
            if (esh->buffer[i] == quote) {
                quote = 0;
            } else {
                esh->buffer[dest] = esh->buffer[i];
                ++dest;
            }
            last_was_space = false;
        } else {
            if (isspace(esh->buffer[i])) {
                last_was_space = true;
                esh->buffer[dest] = 0;
                ++dest;
            } else {
                if (last_was_space) {
                    if (argc < ESH_ARGC_MAX) {
                        esh->argv[argc] = &esh->buffer[i];
                    }
                    ++argc;
                }
                if (esh->buffer[i] == '\'' || esh->buffer[i] == '\"') {
                    quote = esh->buffer[i];
                } else {
                    esh->buffer[dest] = esh->buffer[i];
                    ++dest;
                }
                last_was_space = false;
            }
        }
    }
    esh->buffer[dest] = 0;
    esh->buffer[ESH_BUFFER_LEN] = 0;
    return argc;
}


bool esh_putc(struct esh * esh, char c)
{
    char c_as_string[] = {c, 0};
    esh->print(esh, c_as_string);
    return false;
}


bool esh_puts(struct esh * esh, char const AVR_ONLY(__memx) * s)
{
    for (size_t i = 0; s[i]; ++i) {
        esh_putc(esh, s[i]);
    }
    return false;
}


void esh_restore(struct esh * esh)
{
    esh_puts(esh, FSTR(ESC_ERASE_LINE "\r")); // Clear line
    esh_print_prompt(esh);
    esh->buffer[esh->cnt] = 0;
    esh_puts(esh, esh->buffer);
    // Move cursor back again to the insertion point. Easier to loop than to
    // printf the number into the esc sequence...
    cursor_move(esh, -(int)(esh->cnt - esh->ins));
}


static void cursor_move(struct esh * esh, int n)
{
    for ( ; n > 0; --n) {
        esh_puts(esh, FSTR(ESC_CURSOR_RIGHT));
    }

    for ( ; n < 0; ++n) {
        esh_puts(esh, FSTR(ESC_CURSOR_LEFT));
    }
}


/**
 * Either insert or delete a character at the current insertion point.
 * @param esh - esh instance
 * @param c - character to insert, or 0 to delete
 */
static void ins_del(struct esh * esh, char c)
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
