/* esh - embedded shell
 * Rust bindings
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

#![no_std]
#![crate_name = "esh"]
#![crate_type = "rlib"]
use core::ptr;
use core::mem;
use core::slice;
use core::str;

/// The main esh object. This is an opaque object representing an esh instance,
/// and having methods for interacting with it.
pub enum Esh {}
enum Void {}

extern "C" {
    fn esh_init() -> *mut Esh;
    fn esh_register_command(
        esh: *mut Esh,
        cb: extern fn(esh: *mut Esh, argc: i32, argv: *mut *mut u8, arg: *mut Void),
        arg: *mut Void);
    fn esh_register_print(
        esh: *mut Esh,
        cb: extern "C" fn(esh: *mut Esh, c: u8, arg: *mut Void),
        arg: *mut Void);
    fn esh_register_overflow_callback(
        esh: *mut Esh,
        cb: extern "C" fn(*mut Esh, *const u8, *mut Void),
        arg: *mut Void);
    fn esh_rx(esh: *mut Esh, c: u8);
    fn esh_get_slice_size() -> usize;
}

fn strlen(s: *const u8) -> usize {
    let mut i: usize = 0;
    loop {
        let c = unsafe{*s.offset(i as isize)};
        if c == 0 {
            break;
        } else {
            i += 1;
        }
    }
    return i;
}

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

impl Esh {
    /// Return an initialized esh object. Must be called before any other
    /// functions.
    ///
    /// See `ESH_ALLOC` in `esh_config.h` - this should be `STATIC` or
    /// `MALLOC`. If `STATIC`, `ESH_INSTANCES` must be defined to the
    /// maximum number of instances, and references to these instances
    /// will be returned.
    ///
    /// Note that the reference is always `'static`, even when MALLOC is used.
    /// This is because esh has no destructor: despite being allocated on
    /// demand, it will never be destroyed, so from the moment it is returned
    /// it can be considered to have infinite lifetime.
    ///
    /// Return value:
    ///
    /// * `Some(&'static mut Esh)` - successful initialization
    /// * `None` - static instance count was exceeded or malloc failed.
    pub fn init() -> Option<&'static mut Esh> {
        let esh = unsafe{esh_init()};
        if esh == ptr::null_mut() {
            return None;
        } else {
            return Some(unsafe{&mut *esh});
        }
    }

    extern "C" fn print_callback_wrapper(esh: *mut Esh, c: u8, arg: *mut Void) {
        let func: fn(&Esh, char) = unsafe{mem::transmute(arg)};
        let esh_self = unsafe{&*esh};

        func(esh_self, c as char);
    }

    /// Register a callback to print a string.
    ///
    /// Callback arguments:
    ///
    /// * `esh` - the originating esh instance, allowing identification
    /// * `c` - the character to print
    pub fn register_print(&mut self, cb: fn(esh: &Esh, c: char)) {
        let fp = cb as *mut Void;
        unsafe {
            esh_register_print(self, Esh::print_callback_wrapper, fp);
        }
    }

    extern "C" fn command_callback_wrapper(
            esh: *mut Esh, argc: i32, argv: *mut *mut u8, arg: *mut Void) {

        let func: fn(&Esh, &[&str]) = unsafe{mem::transmute(arg)};

        // Unsafe: this poisons argv! Don't use argv after this.
        let argv_slices = unsafe{map_argv_to_slice(argv, argc)};
        let esh_self = unsafe{&*esh};
        func(esh_self, argv_slices);
    }

    /// Register a callback to execute a command.
    ///
    /// Callback arguments:
    ///
    /// * `esh` - the originating esh instance, allowing identification
    /// * `args` - a reference to an argument accessor object
    pub fn register_command(&mut self, cb: fn(esh: &Esh, args: &[&str])) {
        let fp = cb as *mut Void;
        unsafe {
            esh_register_command(self, Esh::command_callback_wrapper, fp);
        }
    }

    extern "C" fn overflow_callback_wrapper(
            esh: *mut Esh, buf: *const u8, arg: *mut Void) {

        let func: fn(&Esh, &[u8]) = unsafe{mem::transmute(arg)};
        let i = strlen(buf);
        let buf_slice = unsafe{slice::from_raw_parts(buf, i)};
        let esh_self = unsafe{&*esh};

        func(esh_self, buf_slice);
    }

    /// Register a callback to notify about overflow. Optional; esh has an
    /// internal overflow handler.
    ///
    /// Callback arguments:
    ///
    /// * `esh` - the originating esh instance, allowing identification
    /// * `s` - the contents of the buffer, excluding overflow
    pub fn register_overflow(&mut self, cb: fn(esh: &Esh, s: &[u8])) {
        let fp = cb as *mut Void;
        unsafe {
            esh_register_overflow_callback(self, Esh::overflow_callback_wrapper, fp);
        }
    }

    /// Pass in a character that was received. This takes u8 instead of
    /// char because most inputs are byte-oriented; to pass in a Unicode
    /// character, pass in its UTF-8 representation one byte at a time.
    pub fn rx(&mut self, c: u8) {
        unsafe {
            esh_rx(self, c);
        }
    }
}
