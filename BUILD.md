# Introduction

This repository contains the source for two overlapping objects: A
command-line utility called `iftb` and a wasm client object split
between `iftb.js` and `iftb.wasm` files.  These can be built
separately if desired.

# Homebrew Prerequisites

I had good luck getting the non-submodule dependencies discussed below with
these commands:

```
brew install meson
brew install yaml-cpp
brew install emscripten
```

(Note that `emscripten` pulls in a number of other dependencies, including
 `brotli`.)

# `iftb`

The `iftb` code is writen in C++ and requires the normal C++ build
chain.  The Makefile specifies `g++` as the compiler.  It has these
dependencies:

1. `argparse.hpp`, a header-only command-line argument parser. This
   is included in the `src` directory for convenience.
2. `yaml-cpp`, a YAML parser. This is available at
   [https://github.com/jbeder/yaml-cpp](https://github.com/jbeder/yaml-cpp)
   and there is a Homebrew Formula for it.
3. `woff2`: This is originally from
   [https://github.com/google/woff2](https://github.com/google/woff2)
   but is loaded as a submodule. The encoder in the submodule branch
   has been updated to optionally retain the source file's table order,
   so you need to build the `woff2` libraries to link to the `iftb` command.
4. `brotli`: This is available at
   [https://github.com/google/brotli](https://github.com/google/brotli)
   but will also be loaded as a sub-submodule of `woff2`.  It is recommended
   that you do *not* use the submodule to compile `iftb` but instead install
   the library and header files some more standard way on your system. If you
   do use the submodule you will probably have to adjust the Makefile in this
   directory.
5. `harfbuzz`: This is not the standard release of `harfbuzz` but a slightly
   modified version in the `chunkmods` branch of
   [https://github.com/skef/harfbuzz](https://github.com/skef/harfbuzz).
   You need to build the `harfbuzz` submodule to link to the `iftb` command.

To build the modified `harfbuzz` and `woff2` libraries you can start by
populating the git submodules by running `git submodule update --init
--recursive` in the top-level directory.  Then go into the `harfbuzz` directory
and run `meson build`, and after that cd into the `build` directory and run
`ninja`. After that go into the `woff2` directory, run `cmake -B build -GNinja`,
and then go into the `build` directory and run `ninja`.

The top-level Makefile should then have what it needs to link the `iftb`
command (assuming the other dependencies are installed). You can run `make
iftb` to build just the command.

# `iftb.js` and `iftb.wasm`

The WASM client-side code depends on `brotli` and `woff2`, which is another
reason those git submodules are included. Once they are populated by running
`git submodule update --init --recursive` it should be possible to build
`iftb.js` using `emcc` from the `emscripten` toolset (which must be installed).
The `Makefile` has the necessary directives.  You can run `make iftb.js` to
build the wasm code.
