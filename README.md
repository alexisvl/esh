esh - embedded shell
====================

esh is a lightweight command shell for embedded applications, small enough to
be used for (and intended for) debug UART consoles on microcontrollers.

Basic line editing
------------------

esh supports very basic line editing, understanding the backspace key to delete
characters and Ctrl-C to ditch the entire line. Insertion in the middle of the
line include left/right arrow and Ctrl-left/right is upcoming.

Argument tokenizing
-------------------

esh automatically splits a command string into arguments, and understands
bash-style quoting. The command handler callback receives a simple
argc/argv array of arguments, ready to use. Environment variables are not yet
supported, but may be in the future.

History (optional)
------------------

If compiled in, esh supports history, allowing the use of the up/down arrow keys
to browse previously entered commands and edit/re-issue them. A ring buffer is
used to store a fixed number of characters, so more commands can be remembered
if they're shorter. To use history, define the following in `esh_config.h`:

    #define ESH_HIST_ALLOC  STATIC      // STATIC, MANUAL, or MALLOC
    #define ESH_HIST_LEN    512         // Length. Use powers of 2 for efficiency

If you chose `MANUAL` allocation, call `esh_set_histbuf()` once you have allocated
the buffer:

    esh_set_histbuf(struct esh * esh, char * buffer);

Manual allocation was created for one specific purpose - history buffer in
external SRAM on AVR (the compiler and malloc don't generally know about
external SRAM unless you jump through hoops). However, it's there for
whatever you like :)

WARNING: static allocation is only valid when using a SINGLE esh instance.
Using multiple esh instances with static allocation is undefined and will make
demons fly out your nose.

This is a very simple embedded 'shell' for microcontroller use, for
implementing debug UART consoles and such.


Configuring esh
---------------

esh expects a file called `esh_config.h` to be on the quoted include path. It
should define the following:

    #define ESH_PROMPT      "% "        // Prompt string
    #define ESH_BUFFER_LEN  200         // Maximum length of a command
    #define ESH_ARGC_MAX    10          // Maximum argument count


Then, to use esh, include esh.h, define a "struct esh" for each esh instance you
want, and make sure to call the following setup functions (fully documented in
esh.h):

    esh_init(struct esh * esh);
    esh_register_callback(
        struct esh * esh,
        void (*callback)(struct esh * esh, int argc, char ** argv));
    esh_register_print(
        struct esh * esh,
        void (*print)(struct esh * esh, char const * s));


Optionally, you can call the following as well:

    esh_register_overflow_callback(
        struct esh * esh,
        int (*callback)(struct esh * esh, char const * buffer));


Then, as characters are received from your serial interface, feed them in with:

    esh_rx(struct esh * esh, char c);


Compiling esh
-------------

esh has no build script of its own; building it is trivial and it's meant to be
integrated directly into your project.

1. Put the `esh` subdirectory on the include path.
2. Make sure `esh_config.h` is on the quoted include path.
3. Make sure selected C standard is `c99` or `c11`, or for AVR,
    `gnu99` or `gnu11` (GCC extensions are used for handling multiple
    memory spaces).
4. Include *all* esh C source files in the build.

esh should compile quietly with most warning settings, including
`-Wall -Wextra -pedantic`.

