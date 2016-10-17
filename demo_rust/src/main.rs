#![feature(const_fn)]

extern crate esh;
extern crate termios;
#[macro_use]
extern crate sig;
use termios::*;
use std::io;
use std::ptr;
use std::process;
use esh::*;

const STDIN_FILENO: i32 = 1;
static mut termios_ptr: *mut Termios = ptr::null_mut();

extern "C" {
    fn isatty(fd: i32) -> bool;
    fn getchar() -> i32;
}

use std::io::Write;

macro_rules! println_err(
    ($($arg:tt)*) => { {
        let r = writeln!(&mut ::std::io::stderr(), $($arg)*);
        r.expect("failed printing to stderr");
    } }
    );

fn restore_terminal(sig: i32)
{
    unsafe {
        if termios_ptr != ptr::null_mut() {
            let _ = tcsetattr(STDIN_FILENO, TCSAFLUSH, &*termios_ptr);
            drop(Box::from_raw(termios_ptr));
        }
    }

    if sig == 15 {
        process::exit(0);
    }
}

fn esh_print_cb(_esh: &Esh, c: char)
{
    if c == '\n' {
        print!("\r\n");
    } else {
        print!("{}", c);
    }
    io::stdout().flush().unwrap();
}

fn esh_command_cb(esh: &Esh, args: &EshArgArray)
{
    let _ = esh;
    let _ = args;

    println!("argc: {}\r", args.len());

    for i in 0..args.len() {
        println!("argv[{:2}] = {}\r", i, args.get_str(i).unwrap());
    }

    if args.get_slice(0) == *b"exit" || args.get_slice(0) == *b"quit" {
        restore_terminal(0);
        process::exit(0);
    }
}

fn set_terminal_raw()
{
    let mut term = unsafe{*termios_ptr}.clone();
    term.c_iflag &= !(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    term.c_oflag &= !(OPOST);
    term.c_cflag |= CS8;
    term.c_lflag &= !(ECHO | ICANON | IEXTEN | ISIG);
    term.c_cc[VMIN] = 0;
    term.c_cc[VTIME] = 8;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &term).unwrap();
}

fn main()
{
    let mut esh = Esh::init().unwrap();
    esh.register_command(esh_command_cb);
    esh.register_print(esh_print_cb);

    if ! unsafe{isatty(STDIN_FILENO)} {
        println_err!("esh demo must run on a tty");
        process::exit(1);
    }

    let termios = Termios::from_fd(STDIN_FILENO).unwrap();
    unsafe{ termios_ptr = Box::into_raw(Box::new(termios)); }

    signal!(sig::ffi::Sig::TERM, restore_terminal);

    set_terminal_raw();

    println!("Use 'quit' or 'exit' to quit.\r");
    esh.rx(b'\n');

    loop {
        let c = unsafe{getchar()};
        let c_replaced =
            if c == b'\r' as i32    { b'\n' as i32 }
            else                    { c };

        if c_replaced > 0 && c_replaced <= 255 {
            esh.rx(c_replaced as u8);
        }
    }
}
