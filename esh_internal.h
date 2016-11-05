/**
 * esh - embedded shell
 * Internal, private declarations and types
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
#error "esh_internal.h is an internal file and should not be included directly"
#endif // ESH_INTERNAL_INCLUDE

#ifndef ESH_INTERNAL_H
#define ESH_INTERNAL_H

#include <esh_incl_config.h>
#include <esh_hist.h>

/**
 * If we're building for Rust, we need to know the size of a &[u8] in order
 * to allocate space for it. This definition should be equivalent. Because the
 * internal representation of a slice has not been stabilized [1], this is not
 * guaranteed to remain constant in the future; the Rust bindings will check
 * sizeof(struct char_slice) against mem::size_of::<&[u8]>().
 *
 * [1] https://github.com/rust-lang/rust/issues/27751
 */

#ifdef ESH_RUST
struct char_slice {
    char *p;
    size_t sz;
};
#endif

/**
 * esh instance struct. This holds all of the state that needs to be saved
 * between calls to esh_rx().
 */
typedef struct esh {
    /**
     * The config item ESH_BUFFER_LEN is only the number of characters to be
     * stored, not characters plus termination.
     */
    char buffer[ESH_BUFFER_LEN + 1];

    /**
     * The Rust bindings require space allocated for an argv array of &[u8],
     * which can share memory with C's char* array to save limited SRAM.
     */
#ifdef ESH_RUST
    union {
        char * argv[ESH_ARGC_MAX];
        struct char_slice rust_argv[ESH_ARGC_MAX];
    };
#else
    char * argv[ESH_ARGC_MAX];
#endif

    size_t cnt;             ///< Number of characters currently held in .buffer
    size_t ins;             ///< Position of the current insertion point
    uint8_t flags;          ///< State flags for escape sequence parser
    struct esh_hist hist;
#ifndef ESH_STATIC_CALLBACKS
    esh_cb_command cb_command;
    esh_cb_print print;
    esh_cb_overflow overflow;
    void *cb_command_arg;
    void *cb_print_arg;
    void *cb_overflow_arg;
#endif
} esh_t;

/**
 * On AVR, a number of strings should be stored in and read from flash space.
 * Other architectures have linearized address spaces and don't require this.
 */
#ifdef __AVR_ARCH__
#   define FSTR(s) (__extension__({ \
            static const __flash char __c[] = (s); \
            &__c[0];}))
#   define AVR_ONLY(x) x
#else
#   define FSTR(s) (s)
#   define AVR_ONLY(x)
#endif // __AVR_ARCH__

/**
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
 * Print the prompt string
 */
void esh_print_prompt(esh_t * esh);

/**
 * Overwrite the prompt and restore the buffer.
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

#ifdef ESH_RUST
/**
 * Return what we think the size of a Rust &[u8] slice is. This is used to
 * verify that the statically allocated slice array is long enough, and also
 * to make sure a linker error is produced if ESH_RUST wasn't enabled
 * (which would mean the slice array wasn't allocated at all).
 */
size_t esh_get_slice_size(void);
#endif

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
