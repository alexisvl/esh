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


int esh_parse_args(struct esh * esh)
{
    int argc = 0;
    bool last_was_space = true;
    size_t dest = 0;

    for (size_t i = 0; i < esh->cnt; ++i) {
        if (isspace(esh->buffer[i])) {
            last_was_space = true;
            esh->buffer[dest] = 0;
            ++dest;
        } else {
            if (last_was_space) {
                if (argc < ESH_ARGC_MAX) {
                    esh->argv[argc] = &esh->buffer[dest];
                }
                ++argc;
            }
            if (esh->buffer[i] == '\'' || esh->buffer[i] == '\"') {
                consume_quoted(esh, &i, &dest);
            } else {
                esh->buffer[dest] = esh->buffer[i];
                ++dest;
            }
            last_was_space = false;
        }
    }
    esh->buffer[dest] = 0;
    esh->buffer[ESH_BUFFER_LEN] = 0;
    return argc;
}
