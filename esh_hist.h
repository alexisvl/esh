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
#error "esh_hist.h is an internal header and should not be included by the user."
#endif // ESH_INTERNAL_INCLUDE

#ifndef ESH_HIST_H
#define ESH_HIST_H

#include <stdbool.h>
#include <stddef.h>

/*
 * esh history support. This provides either a full history implementation or
 * a placeholder, depending on whether history was enabled in configuration.
 * This allows the main esh code to not be conditionally compiled.
 */

struct esh;

#ifdef ESH_HIST_ALLOC
// Begin actual history implementation

struct esh_hist {
    char * hist;
    int tail;
    int idx;
};

/**
 * Initialize history.
 * @param esh - esh instance
 * @return true on error. Can only return error if the history buffer is to be
 * allocated on heap.
 */
bool esh_hist_init(struct esh * esh);

/**
 * Count back n strings from the current tail of the ring buffer and return the
 * index the string starts at.
 *
 * @param esh - esh instance
 * @param n - count to the nth string back, where 0 is the last string added
 * @return offset in the ring buffer where the nth string starts, or -1 if
 *  there are not n-1 strings in the buffer.
 */
int esh_hist_nth(struct esh * esh, int n);

/**
 * Add a string into the buffer. If the string doesn't fit, the buffer is
 * intentionally reset to avoid restoring a corrupted string later.
 * @param esh - esh instance
 * @param s - string to add
 * @return true iff the string didn't fit (this is destructive!)
 */
bool esh_hist_add(struct esh * esh, char const * s);

/**
 * Given an offset in the ring buffer, call the callback once for each
 * character in the string starting there. This is meant to abstract away
 * ring buffer access.
 *
 * @param esh - esh instance
 * @param offset - offset into the ring buffer
 * @param callback - will be called once per character
 *      - callback:param esh - esh instance
 *      - callback:param c - character
 *      - callback:return - true to stop iterating, false to continue
 *
 * Regardless of the callback's return value, iteration will always stop at NUL
 * or if the loop wraps all the way around.
 */
void esh_hist_for_each_char(struct esh * esh, int offset,
        bool (*callback)(struct esh * esh, char c));

/**
 * Overwrite the prompt and print a history suggestion.
 * @param esh - esh instance
 * @param offset - offset into the ring buffer
 */
void esh_hist_print(struct esh * esh, int offset);

/**
 * Overwrite the prompt and restore the buffer.
 * @param esh - esh instance
 */
void esh_hist_restore(struct esh * esh);

/**
 * Put the selected history item in the buffer. Make sure to call
 * esh_hist_restore afterward to display the buffer.
 * @param esh - esh instance
 * @param offset - offset into the ring buffer
 */
void esh_hist_clobber(struct esh * esh, int offset);

/**
 * If history is currently being browsed, substitute the selected history item
 * for the buffer and redraw the buffer for editing.
 * @param esh - esh instance
 * @return true iff the substitution was made (i.e. history was being browsed)
 */
bool esh_hist_substitute(struct esh * esh);

#else // ESH_HIST_ALLOC
// Begin placeholder implementation

struct esh_hist {
    size_t idx;
};

#define INL static inline __attribute__((always_inline))

INL bool esh_hist_init(struct esh * esh)
{
    (void) esh;
    return false;
}

INL int esh_hist_nth(struct esh * esh, int n)
{
    (void) esh;
    (void) n;
    return -1;
}

INL bool esh_hist_add(struct esh * esh, char const * s)
{
    (void) esh;
    (void) s;
    return true;
}

INL void esh_hist_for_each_char( struct esh * esh, int offset,
        bool (*callback)(struct esh * esh, char c))
{
    (void) esh;
    (void) offset;
    (void) callback;
}

INL void esh_hist_print(struct esh * esh, int offset)
{
    (void) esh;
    (void) offset;
}

INL void esh_hist_restore(struct esh * esh)
{
    (void) esh;
}

INL void esh_hist_clobber(struct esh * esh, int offset)
{
    (void) esh;
    (void) offset;
}

INL bool esh_hist_substitute(struct esh * esh)
{
    (void) esh;
    return false;
}

#undef INL

#endif // ESH_HIST_ALLOC

#endif // ESH_HIST_H
