# Simplified error handling for C

Ever wished you could ignore errors and just keep going like Visual Basic's
great `On Error Resume Next`, but in C? Well, now you can!

An "error" in this case means segmentation fault. More signals may be caught
and ignored in the future. A mode that catches and hides all error codes from
system calls may also be considered.


## How to use

See the source of the demo executable in `segvign.c` for more inspiration and
usage, and the doc comments in `on_error.h` for actual documentation.

Including the header defines a handful of functions/macros for setting up error
handling:

* `on_error_resume_next()`: the star of the show, just lets you get on with
  stuff and not do errors at all.

* `on_error_goto(fail)`: goto fail, but do it automatically on errors. The
  label should be defined in the same function.

* `on_error_goto_unsafe(fail)`: like above, but even less safe.

Link against `lib_on_error.so` for the runtime functions used by the macros.


## License

The code itself is MIT license, but uses bitdefender's bddisasm which is
APLv2 licensed.


## How to build

1. Clone bddisasm repo into a subdirectory
2. Run `make`

This will produce a `lib_on_error.so` shared library that can be linked into
your own programs, as well as a demo executable in statically linked
(`segvign`) and dynamically linked (`segvign_dyn`) forms.


## Missing features

There's no way to disable the error handling, you just have to make sure to
update the error handling label as you go.

Or call `on_error_resume_next()` once at startup and leave it. You never want
your program to crash anyway.
