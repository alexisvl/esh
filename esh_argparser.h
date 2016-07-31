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
#error "esh_argparser.h is an internal header and should not be included by the user."
#endif // ESH_INTERNAL_INCLUDE

#ifndef ESH_ARGPARSER_H
#define ESH_ARGPARSER_H

/**
 * Map the buffer to the argv array, and return argc. If argc exceeds the
 * maximum, the full buffer will still be processed; argument pointers will
 * just not be stored beyond the maximum. The number that would have been
 * stored is returned.
 *
 * Handles whitespace, quotes, and if ESH_ENVVARS is defined, environment
 * variables. When ESH_ENVVARS is not defined, the entire buffer is processed
 * in place, with any changes leaving the length equal or shorter. Because
 * environment variable values can be longer than their names, ESH_ENVVARS
 * being defined will result in a secondary buffer being used to store the
 * resulting values.
 *
 * Following is an example buffer before and after processing (# for NUL),
 * with pointers stored in argv[] marked with ^
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
 *
 * When environment variables are supported, the first buffer is still edited;
 * it is used as a source of variable name substrings:
 *
 * before: git config user.name ${NAME}
 * after:  git config user.name NAME#
 * second: git#config#user.name#My Name#
 * argv:   ^   ^      ^         ^
 */
int esh_parse_args(esh_t * esh);

#endif // ESH_ARGPARSER_H
