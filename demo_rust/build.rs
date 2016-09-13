extern crate gcc;

fn main()
{
    gcc::Config::new()
        .file("../esh.c")
        .file("../esh_hist.c")
        .file("../esh_argparser.c")
        .include("..")
        .flag("-iquotesrc")
        .flag("-Wall").flag("-Wextra").flag("-Werror")
        .flag("-pedantic").flag("-std=c11")
        .debug(true)
        .compile("libesh.a");
}
