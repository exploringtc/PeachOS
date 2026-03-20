# EDIT 07: This updates the sample assembly program to 64-bit mode so it matches the cross-toolchain migration done for the project.
[BITS 64]

section .asm

global _start

_start:

label:
    jmp label