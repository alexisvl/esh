#![no_std]
#![crate_name = "esh"]
#![crate_type = "rlib"]

/**
 * esh - embedded shell
 * ====================
 *
 * *****************************************************************************
 * * PLEASE read ALL of this documentation (all comment blocks starting with a *
 * * double-asterisk **). esh is simple, but a number of things need to be     *
 * * addressed by every esh user.                                              *
 * *****************************************************************************
 *
 * esh is a lightweight command shell for embedded applications in C or rust,
 * small enough to be used for (and intended for) debug UART consoles on
 * microcontrollers. Features include line editing, automatic argument
 * tokenizing (including sh-like quoting), and an optional history ring buffer.
 *
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
 *
 * -----------------------------------------------------------------------------
 *
 * 1.   Rust users
 * 2.   Configuring esh
 * 2.1.     Line endings
 * 2.2.     History (optional)
 * 3.   Compiling esh
 * 3.1.     C components
 * 3.2.     Rust components
 * 3.3.     Linking
 * 4.   Code documentation
 * 4.1.     Basic interface: initialization and input
 * 4.2.     Callback registration functions
 * 5.   Private functions
 *
 * -----------------------------------------------------------------------------
 *
 * 1. Rust users
 * =============
 *
 * Congrats, you're in the right place! Ignore the documentation for the C API,
 * including the "Configuring esh" section. Your usage is a bit different.
 *
 * 2. Configuring esh
 * ==================
 *
 * esh expects a file called `esh_config.h` to be on the quoted include path for
 * the C compiler. It should define the following:
 *
 *     #define ESH_PROMPT       "% "        // Prompt string
 *     #define ESH_BUFFER_LEN   200         // Maximum length of a command
 *     #define ESH_ARGC_MAX     10          // Maximum argument count
 *     #define ESH_ALLOC        STATIC      // How to allocate esh_t (or MALLOC)
 *
 * Then, to use esh, use `extern crate esh`, and initialize an esh instance:
 *
 *     let mut esh = Esh::init().unwrap();
 *
 * Register your callbacks with:
 *
 *     esh.register_command(command_callback);
 *     esh.register_print(print_callback);
 *
 *     // Optional, see the documentation for this function:
 *     esh.register_overflow(overflow_callback);
 *
 * Now, just begin receiving characters from your serial interface and feeding
 * them in with:
 *
 *     esh.rx(c);
 *
 * 2.1. Line endings
 * -----------------
 *
 * Internally, esh uses strictly `\n` line endings. A great many IO sources use
 * different line endings; the user is responsible for translating them for esh.
 * In general, most raw-mode unix-like terminals will give `\r` from the
 * keyboard and require `\r\n` as output, so your input functions should
 * translate `\r` to `\n`, and your output function should insert `\r` before
 * `\n`.
 *
 * 2.2. History (optional)
 * -----------------------
 *
 * To enable the optional history, define the following in `esh_config.h`:
 *
 *     #define ESH_HIST_ALLOC   STATIC      // STATIC or MALLOC
 *     #define ESH_HIST_LEN     512         // Length. Use powers of 2 for
 *                                          //   efficiency on arithmetic-weak
 *                                          //   devices.
 *
 * WARNING: static allocation is only valid when using a SINGLE esh instance.
 * Using multiple esh instances with static allocation is undefined and WILL
 * make demons fly out your nose.
 *
 * 3. Compiling esh
 * ================
 *
 * esh has Rust and C components, so you need to build and link both. See the
 * included demo under `demo_rust/` for an example using Cargo to do this.
 *
 * 3.1. C components
 * -----------------
 *
 *  1. Put the `esh` subdirectory on the include path.
 *  2. Make sure `esh_config.h` is on the quoted include path (`-iquote`).
 *  3. Make sure selected C standard is one of `c99`, `c11`, `gnu99`, or
 *       `gnu11`.
 *  4. Include *all* esh C source files in the build (whether or not you used
 *       the feature - e.g. esh_hist.c).
 *
 * esh should compile quietly with most warning settings, including
 * `-Wall -Wextra -pedantic`.
 *
 * 3.2. Rust components
 * --------------------
 *
 * The Rust bindings can be compiled as any crate. Either use Cargo and make esh
 * a dependency, or build directly with rustc:
 *
 *     rustc --crate-name=esh -o libesh.rlib esh/esh_rust/src/esh/lib.rs
 *
 * 3.3. Linking
 * ------------
 *
 * The final executable must link together the C *.o files and the Rust crate.
 * rlib files are static libraries, and so can be given directly to the linker.
 */

use core::ptr;
use core::mem;
use core::slice;
use core::str;

pub enum Esh {}
pub enum Void {}

extern "C" {
    fn esh_init() -> *mut Esh;
    fn esh_set_command_arg(esh: *mut Esh, arg: *mut Void);
    fn esh_set_print_arg(esh: *mut Esh, arg: *mut Void);
    fn esh_set_overflow_arg(esh: *mut Esh, arg: *mut Void);
    fn esh_rx(esh: *mut Esh, c: u8);
    fn esh_default_overflow(esh: *mut Esh, buf: *const u8, arg: *mut Void);
    fn esh_get_slice_size() -> usize;
    fn strlen(s: *const u8) -> usize;
}

/**
 * -----------------------------------------------------------------------------
 *
 * 4. Code documentation
 */


impl Esh {
    /**
     * -------------------------------------------------------------------------
     * 4.1. Basic interface: initialization and input
     */

    /**
     * Return an initialized esh object. Must be called before any other
     * functions.
     *
     * See `ESH_ALLOC` in `esh_config.h` - this should be `STATIC` or `MALLOC`.
     * If `STATIC`, only a single instance can be used. init() will return it
     * on the first call, and all subsequent calls will return None.
     *
     * Note that the reference returned always has static lifetime, even when
     * `MALLOC` is used. This is because esh has no destructor: despite being
     * allocated on demand, it will never be destroyed, so from the moment it
     * is returned it can be considered to have infinite lifetime.
     *
     * Return value:
     *
     * * `Some(&'static mut Esh)` - successful initialization
     * * `None` - static instance count was exceeded or malloc failed
     */
    pub fn init() -> Option<&'static mut Esh> {
        // Safe: C API function always returns valid pointer or NULL
        let esh = unsafe{esh_init()};
        if esh == ptr::null_mut() {
            return None;
        } else {
            // Safe: we already checked that the pointer is valid
            return Some(unsafe{&mut *esh});
        }
    }

    /**
     * Pass in a character that was received.
     *
     * This takes u8 instead of char because most inputs are byte-oriented.
     * Note that esh does not currently have Unicode support; to properly play
     * along with Rust (where &str is always UTF-8), only bytes in the
     * intersection of ASCII and UTF-8 will be accepted; others will be silently
     * dropped.
     */
    pub fn rx(&mut self, c: u8) {
        // Safe: C API function is taking a known valid reference as a pointer
        unsafe {
            esh_rx(self, c);
        }
    }

    /**
     * -------------------------------------------------------------------------
     * 4.2. Callback registration functions
     */

    /**
     * Register a callback to print a character.
     *
     * Callback arguments:
     *
     * `esh` - the originating esh instance, allowing identification
     * `c` - the character to print
     */
    pub fn register_print(&mut self, cb: fn(esh: &Esh, c: char)) {
        let fp = cb as *mut Void;
        // Safe: C API function is taking a known valid reference as a pointer
        unsafe {
            esh_set_print_arg(self, fp);
        }
    }

    /**
     * Register a callback to execute a command.
     *
     * Callback arguments:
     *
     * `esh` - the originating esh instance, allowing identification
     * `args` - arguments, including the command itself
     */
    pub fn register_command(&mut self, cb: fn(esh: &Esh, args: &[&str])) {
        let fp = cb as *mut Void;
        // Safe: C API function is taking a known valid reference as a pointer
        unsafe {
            esh_set_command_arg(self, fp);
        }
    }

    /**
     * Register a callback to notify about overflow. Optional; esh has an
     * internal overflow handler.
     *
     * Callback arguments:
     *
     * * `esh` - the originating esh instance, allowing identification
     * * `s` - the contents of the buffer before overflow
     */
    pub fn register_overflow(&mut self, cb: fn(esh: &Esh, s: &[u8])) {
        let fp = cb as *mut Void;
        // Safe: C API function is taking a known valid reference as a pointer
        unsafe {
            esh_set_overflow_arg(self, fp);
        }
    }
}

/**
 * -----------------------------------------------------------------------------
 *
 * 5. Private functions
 */

/// Remap argv[] to a slice array in-place, returning the slice array.
/// This is unsafe as hell. It depends on esh_internal.h having defined argv
/// as a union of a char array and a slice array, to guarantee that we have
/// enough space for the slices.
///
/// This will verify (at runtime, unfortunately) that C and Rust agree on how
/// long a slice is, and panic otherwise.
unsafe fn map_argv_to_slice<'a>(argv: *mut *mut u8, argc: i32) ->&'a[&'a str]
{
    if ::core::mem::size_of::<&str>() != esh_get_slice_size() {
        panic!("Expected size of string slice in esh_internal.h does \
                not match with real size!");
    }

    // The mapping is done in place to conserve memory. (Sorry! but embedded
    // devices tend to have quite restricted RAM.) The mapping is done in from
    // the right end to keep things from stepping on each other.

    let as_slices: *mut &'a str = mem::transmute(argv);

    for i in 0..(argc as isize) {
        let source = argv.offset(argc as isize - i - 1);
        let target = as_slices.offset(argc as isize - i - 1);

        let as_u8slice = slice::from_raw_parts(*source, strlen(*source));
        *target = str::from_utf8_unchecked(as_u8slice);
    }

    slice::from_raw_parts(as_slices, argc as usize)
}

#[allow(non_snake_case)]
#[no_mangle]
pub extern "C" fn ESH_COMMAND_CALLBACK(
        esh: *mut Esh, argc: i32, argv: *mut *mut u8, arg: *mut Void)
{
    if arg != ptr::null_mut() {
        // Safe: `arg` came from us originally, transmuted from the same type
        let func: fn(&Esh, &[&str]) = unsafe{mem::transmute(arg)};

        // Safe: this poisons argv, but we won't use argv again
        let argv_slices = unsafe{map_argv_to_slice(argv, argc)};

        // Safe: `esh` came from us originally, known to be a good reference
        let esh_self = unsafe{&*esh};

        func(esh_self, argv_slices);
    }
}

#[allow(non_snake_case)]
#[no_mangle]
pub extern "C" fn ESH_OVERFLOW_CALLBACK(
        esh: *mut Esh, buf: *const u8, arg: *mut Void)
{
    if arg != ptr::null_mut() {
        // Safe: `arg` came from us originally, transmuted from the same type
        let func: fn(&Esh, &[u8]) = unsafe{mem::transmute(arg)};

        // Safe: esh guarantees this will be a valid, NUL-terminated string
        let i = unsafe{strlen(buf)};

        // Safe: we just checked length
        let buf_slice = unsafe{slice::from_raw_parts(buf, i)};

        // Safe: `esh` came from us originally, known to be a good reference
        let esh_self = unsafe{&*esh};

        func(esh_self, buf_slice);
    } else {
        // Safe: passing known-good pointers
        unsafe{esh_default_overflow(esh, buf, arg)};
    }
}

#[allow(non_snake_case)]
#[no_mangle]
pub extern "C" fn ESH_PRINT_CALLBACK(esh: *mut Esh, c: u8, arg: *mut Void)
{
    if arg != ptr::null_mut() {
        // Safe: `arg` came from us originally, transmuted from the same type
        let func: fn(&Esh, char) = unsafe{mem::transmute(arg)};

        // Safe: `esh` came from us originally, known to be a good reference
        let esh_self = unsafe{&*esh};

        func(esh_self, c as char);
    }
}
