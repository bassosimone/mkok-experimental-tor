# MeasurementKit Code for Running Tor

[![Build Status](https://travis-ci.org/bassosimone/mkok-libevent-ng.svg?branch=master)](https://travis-ci.org/bassosimone/mkok-libevent-ng) [![Coverage Status](https://coveralls.io/repos/bassosimone/mkok-libevent-ng/badge.svg?branch=master&service=github)](https://coveralls.io/github/bassosimone/mkok-libevent-ng?branch=master)

This repository contains
[MeasurementKit](https://github.com/measurement-kit/measurement-kit)
[C++11](https://en.wikipedia.org/wiki/C%2B%2B11) code for running Tor, along
with improvements developed along with this feature.
The plan is to merge this code in MeasurementKit as part of the release
process of MeasurementKit 0.2.0.

## Usage

To clone, test, and install on your system:

```
git clone https://github.com/bassosimone/mkok-experimental-tor
cd mkok-experimental-tor
./autogen.sh
./script/mkpm install `cat mkpm-requirements.txt`
./script/mkpm shell ./configure
make V=0
make check V=0
make run-valgrind
```

The MKPM script ensures that all dependencies are compiled and installed
and that the proper environment variables are passed to `./configure`.

The above procedure would also compile the [many examples](https://github.com/bassosimone/mkok-experimental-tor/tree/master/example)
and compile and run all the
[unit and integration tests](https://github.com/bassosimone/mkok-experimental-tor/tree/master/test).

## Design

This repository is based on a subset of MeasurementKit and contains the
following enhancements with respect to it:

- [code for running inside Tor event loop](https://github.com/bassosimone/mkok-experimental-tor/blob/master/src/tor/onion-poller.hpp)

- [code to communicate with Tor using the control port](https://github.com/bassosimone/mkok-experimental-tor/blob/master/src/tor/onion-ctrl.hpp)

- [improved libevent wrappers](https://github.com/bassosimone/mkok-experimental-tor/tree/master/src/common)

- [new socks5 code based on libevent wrappers](https://github.com/bassosimone/mkok-experimental-tor/blob/master/src/net/socks.hpp)

- better integration with autotools and automatic download, patching,
  and building of the dependencies [using the mkpm script](https://github.com/bassosimone/mkok-experimental-tor/tree/master/script)

Compared to code in MeasurementKit, this code also:

- uses a safer wrapper for `std::function`,
  [mk::Func](https://github.com/bassosimone/mkok-experimental-tor/blob/master/include/measurement_kit/common/func.hpp),
  that overcomes the problem described in
  [measurement-kit/measurement-kit#111](https://github.com/measurement-kit/measurement-kit/issues/111)
  and ensures that we can safely use lambda functions

- typically uses static methods taking the
  [mk::Var](https://github.com/bassosimone/mkok-experimental-tor/blob/master/include/measurement_kit/common/var.hpp)
  shared smart pointer as first parameter (as opposed to non-static methods
  implicitly taking `this`) such that we guarantee that an object cannot be
  deleted while it is being manipulated by some code

- uses a slightly improved
  [mk::Error](https://github.com/bassosimone/mkok-experimental-tor/blob/master/include/measurement_kit/common/error.hpp)
  that also records the file name, the line number, and function name
  where an Error occurred
