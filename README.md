esh - embedded shell
====================

This is a very simple embedded 'shell' for microcontroller use, for
implementing debug UART consoles and such.


Configuring esh
---------------

esh expects a file called esh_config.h to be on the quoted include path. It
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
        void (*callback)(int argc, char const * const * argv));
    esh_register_print(
        struct esh * esh,
        void (*print)(struct esh const * esh, char const * s));


Optionally, you can call the following as well:

    esh_register_overflow_callback(
        struct esh * esh,
        int (*callback)(struct esh const * esh, char const * buffer));


Then, as characters are received from your serial interface, feed them in with:

    esh_rx(struct esh * esh, char c);


Compiling esh
-------------

esh has no build script of its own; building it is trivial and it's meant to be
integrated directly into your project.

To build esh, simply ensure that the esh subdirectory is on the include path,
and that all esh source files are built (whether or not you use the features
they define - they may define stubs for unused features). On AVR, esh requires
`-std=gnu99` or `-std=gnu11`; on other platforms `-std=c99` or `-std=c11` will
suffice. esh should generally compile quietly with most warning settings,
including `-Wall -Wextra -pedantic`.

Optional features
=================

History
-------

esh supports history, using a ring buffer to store a fixed number of characters
(so more commands if they're shorter). To use history, define the following
in esh_config.h:

    #define ESH_HIST_ALLOC  STATIC      // STATIC, MANUAL, or MALLOC
    #define ESH_HIST_LEN    512         // Length. Use powers of 2 for efficiency

If you chose `MANUAL` allocation, call esh_set_buffer() once you have allocated
the buffer:

    esh_set_histbuf(struct esh * esh, char * buffer);

Manual allocation was created for one specific purpose - history buffer in
external SRAM on AVR (the compiler and malloc don't generally know about
external SRAM unless you jump through hoops). However, it's there for
whatever you like :)

WARNING: static allocation is only valid when using a SINGLE esh instance.
Using multiple esh instances with static allocation is undefined and will make
demons fly out your nose.
