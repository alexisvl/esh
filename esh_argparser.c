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

#include <ctype.h>
#include <string.h>

#define ESH_ENVVARS     //XXX
#define ESH_INTERNAL
#include <esh.h>
#define ESH_INTERNAL_INCLUDE
#include <esh_argparser.h>

#define DEST(esh) ((esh)->buffer)


/**
 * Consume a quoted string. The source string will be modified into the
 * destination string as follows:
 *
 * source:  " b"
 * dest:     b
 *
 * This is safe to use when the destination and source buffer are the same;
 * it will only ever contract the data, not expand it.
 */
static void consume_quoted(struct esh * esh, size_t *src_i, size_t *dest_i)
{
    char quote = esh->buffer[*src_i];

    for (++*src_i; *src_i < esh->cnt; ++*src_i) {
        char c = esh->buffer[*src_i];
        if (c == quote) {
            // End of quoted string
            break;
        } else {
            DEST(esh)[*dest_i] = c;
            ++*dest_i;
        }
    }
}


/**
 * True if a character is valid in an identifier. [A-Za-z0-9_]
 */
static bool isident(char c)
{
    return isalnum(c) || c == '_';
}

/**
 * Consume an environment variable and write in its value. Requires a second
 * buffer, as the value may be longer than the variable. The source string
 * will be modified into the destination string as follows:
 *
 * source:  $CAT
 * dest:    meow
 *
 * source:  ${COW}
 * dest:    moo
 *
 * The source string will _also_ be modified, to use it for in-place
 * NUL-terminated substrings.
 *
 * @return true on error (value overflows buffer)
 */
static bool consume_var(struct esh * esh, size_t *src_i, size_t *dest_i)
{
    // First, we need to make the variable name into a proper substring. The
    // mapping is as follows:
    //
    // before:  $CAT
    // after:   CAT#
    //
    // before:  ${COW}
    // after:   $COW##

    bool in_braces = false;
    bool found_end = false;
    size_t start = *src_i;

    for (++*src_i; *src_i < esh->cnt; ++*src_i) {
        char c = esh->buffer[*src_i];
        if ((*src_i == start + 1) && (c == '{')) {
            in_braces = true;
        } else if (in_braces && c == '}') {
            esh->buffer[*src_i] = 0;
            found_end = true;
            break;
        } else if (!in_braces && !isident(c)) {
            found_end = true;
            break;
        } else {
            esh->buffer[*src_i - 1] = esh->buffer[*src_i];
        }
    }

    if (!found_end) {
        return true;
    }

    // Now, get the value
    size_t var_index = (in_braces ? start + 1 : start);
    if (esh->var_callback(esh, &esh->buffer[var_index],
                &esh->argbuffer[0], ESH_BUFFER_LEN)) {
        return true;
    }

    // Figure out how much space we need to make
    size_t var_length = *src_i - start;
    size_t value_length = strlen(esh->argbuffer);

    return false;
}


int esh_parse_args(struct esh * esh)
{
    int argc = 0;
    bool last_was_space = true;
    size_t dest = 0;

    for (size_t i = 0; i < esh->cnt; ++i) {
        if (isspace(esh->buffer[i])) {
            last_was_space = true;
            DEST(esh)[dest] = 0;
            ++dest;
        } else {
            if (last_was_space) {
                if (argc < ESH_ARGC_MAX) {
                    esh->argv[argc] = &DEST(esh)[dest];
                }
                ++argc;
            }
            if (esh->buffer[i] == '\'' || esh->buffer[i] == '\"') {
                consume_quoted(esh, &i, &dest);
            } else {
                DEST(esh)[dest] = esh->buffer[i];
                ++dest;
            }
            last_was_space = false;
        }
    }
    esh->buffer[dest] = 0;
    esh->buffer[ESH_BUFFER_LEN] = 0;
    return argc;
}
