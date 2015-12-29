# MeasurementKit C++11 Libevent Wrappers

[![Build Status](https://travis-ci.org/bassosimone/mkok-libevent-ng.svg?branch=master)](https://travis-ci.org/bassosimone/mkok-libevent-ng) [![Coverage Status](https://coveralls.io/repos/bassosimone/mkok-libevent-ng/badge.svg?branch=master&service=github)](https://coveralls.io/github/bassosimone/mkok-libevent-ng?branch=master)

This repository contains
[MeasurementKit](https://github.com/measurement-kit/measurement-kit)
[C++11](https://en.wikipedia.org/wiki/C%2B%2B11)
wrappers for [Libevent](https://github.com/libevent/libevent). They
are implemented by a single, all-in-one C++ header. They are primarily
meant to be used inside MeasurementKit (hence the all-in-one C++
header design), but they are implemented as a standalone piece of
software, with a clean, reusable API.

Mkok-libevent means "zero problem measurement-kit libevent wrappers".
The `-ng` suffix is because a previous-attempt at implementing this
exists, but has now been supersed by this new repository.

## Design goals

The aim is to wrap Libevent API with useful C++11 features such as
smart pointers, lambda, and closures, to ensure an easier and safer
programming experience. More specifically, with mkok-libevent-ng
we have the following design goals:

1) Always use smart pointers in place of raw pointers to structures
such that, if you can access an object, it is still alive.

2) Allow to safely keep alive an object by storing the corresponding
smart pointer inside a C++11 lambda closure.

3) Convert Libevent errors into exceptions such that no libevent
error could easily be ignored by measurement-kit code.

To achieve goal 1, we implemented the `Var` sharable smart pointer
(i.e.  many pointers can point to the same pointee at the same time)
with the no-null-pointer-returned guarantee (i.e. an exception is
raised if the pointee is `nullptr`).

To achieve goal 2, we implemented the `Func` wrapper for C++11
`std::function` that overcomes the problem described in
[measurement-kit/measurement-kit#111](https://github.com/measurement-kit/measurement-kit/issues/111).
Basically, if one uses a lambda to keep alive an object, this could
lead to troubles if such lambda is later assigned to a `std::function`
inside the object itself and such `std::function` is later overriden
with another lambda. In fact, that leads to the destruction of the
original lambda closure and hence to use after free in `std::function`
code.

Moreover, we want to be sure that an object cannot be destroyed while
code is manipulating it (a situation that may result from the usage
of smart pointers and from keeping them alive using C++11 lambda
closures). To achieve this goal, in this library all "methods" are
actually static and receive a smart pointer as their first argument.
Thus, when an object is being manipulated, it is alive by design.

And, of course, all structures that contain raw pointers are non-copyable
and non-movable, for safety.

To achieve goal 3, we just wrote code that deals with error return
values and throws the corresponding exceptions.

## Usage and install instructions

### How to clone this repository

Clone this git project from github with the following command:

```
git clone https://github.com/bassosimone/mkok-libevent-ng
```

### Dependencies and how to use this library

Just copy and include the header inside your project. The only
dependencies are [libevent2](https://github.com/libevent/libevent)
and [OpenSSL](https://github.com/openssl/openssl) (or
[LibReSSL](https://github.com/libressl-portable/portable)), whose
headers and libraries must be installed on your system. Mkok-libevent-ng's
header is written assuming a C++11 capable compiler (such as [GNU
GCC](https://gcc.gnu.org/) 4.8 or [Clang](http://clang.llvm.org/)
3.5). To enable C++11, you typically pass the `-std=c+11` to the
compiler.


### Compiling examples

This repository also contains example code and regress tests. To
compile example code and run regress tests, in addition to what was
said above, you also need autoconf, automake, and libtool.

To compile example code, follow the standard procedure:

```
autoreconf -i
./configure --silent
make V=0
```

(The `--silent` and `V=0` flags could be omitted if you prefer
the standard, verbose-style build process.)

The `./configure` script should check whether your system is suitable
for compiling the example code and regress tests. It also has a
variety of command line options to customize the build, which you
can access by passing it the `--help` flags as in `./configure
--help`.

### Running regress tests

To also compile and run regress tests, in addition to the three
above commands required to compile examples, you also need to type:

```
make check V=0
```

### Running regress tests using Valgrind

To force the test suite to run through [Valgrind](http://valgrind.org/) (to
check for common memory errors such as use-after-free), run:

```
make run-valgrind V=0
```

Of course, this requires Valgrind to be installed. (Note that
Valgrind on MacOS may report errors and is generally less reliable
than on GNU/Linux).

### How to install this library

To install mkok-libevent-header, type:

```
make install
```

You typically need to be root to install, since the default prefix
where to install is `/usr/local`. You can customize the prefix where
to install invoking `./configure` with the `--prefix` flag:

```
./configure --prefix=/your/prefix
```

(After this command, `make install` will install mkok-libevent-ng
header at `/your/prefix/include`.)

You can also, er, *prefix* and additional directory to the above
specified prefix as follows:

```
make install DESTDIR=/abs/path
```

In that case, root privileges may not be needed, if you have write
permission of the specified `/abs/path`.

With the above command, you will install mkok-libevent-ng header
at `/abs/path/your/prefix/include`.
