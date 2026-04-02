#!/bin/bash
set -e

export PREFIX="$HOME/opt/cross"
export TARGET=i686-elf
export PATH="$PREFIX/bin:$PATH"

# Prevent a mid-build hang on sudo mount/cp in Makefile by requesting auth once.
sudo -v

make all