#![feature(const_fn)]

extern crate esh;
extern crate termios;
#[macro_use] extern crate sig;
#[macro_use] extern crate lazy_static;
use termios::*;
use std::io;
use std::process;
use std::sync::*;
use esh::*;

const STDIN_FILENO: i32 = 1;
lazy_static! {
    static ref TERMIOS: Mutex<Termios> = Mutex::new(Termios::from_fd(STDIN_FILENO).unwrap());
}

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
    let termios = *TERMIOS.lock().unwrap();
    let _ = tcsetattr(STDIN_FILENO, TCSAFLUSH, &termios);

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

fn esh_command_cb(esh: &Esh, args: &[&str])
{
    let _ = esh;

    println!("argc: {}\r", args.len());

    for i in 0..args.len() {
        println!("argv[{:2}] = {}\r", i, args[i]);
    }

    if *args[0] == *"exit" || *args[0] == *"quit" {
        restore_terminal(0);
        process::exit(0);
    }
}

fn set_terminal_raw()
{
    let mut newterm = TERMIOS.lock().unwrap().clone();
    newterm.c_iflag &= !(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    newterm.c_oflag &= !(OPOST);
    newterm.c_cflag |= CS8;
    newterm.c_lflag &= !(ECHO | ICANON | IEXTEN | ISIG);
    newterm.c_cc[VMIN] = 0;
    newterm.c_cc[VTIME] = 8;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &newterm).unwrap();
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

    // Initialize termios
    *TERMIOS.lock().unwrap();

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
