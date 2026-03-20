#!/bin/bash
export PREFIX="$HOME/opt/cross"
# EDIT 01: This switches the project to the x86_64 cross-toolchain so the build targets 64-bit long mode instead of the original i686 setup.
export TARGET="x86_64-elf"
export PATH="$PREFIX/bin:$PATH"
make all