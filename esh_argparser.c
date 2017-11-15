/*
 * esh - embedded shell
 * Copyright (C) 2017 Chris Pavlina
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <ctype.h>

#define ESH_INTERNAL
#include <esh.h>
#define ESH_INTERNAL_INCLUDE
#include <esh_argparser.h>
#include <esh_internal.h>

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
static void consume_quoted(esh_t * esh, size_t *src_i, size_t *dest_i)
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


int esh_parse_args(esh_t * esh)
{
    int argc = 0;
    bool last_was_space = true;
    size_t dest = 0;

    for (size_t i = 0; i < esh->cnt; ++i) {
        if (esh->buffer[i] == ' ') {
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
