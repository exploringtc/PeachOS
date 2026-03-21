[BITS 32]

section .text

global _start
extern kernel_main

; EDIT 11: These bootstrap constants define the selectors, stack, MSR, and control-register 
;flags needed to enter 64-bit long mode with four-level paging.
CODE_SEG equ 0x08
DATA_SEG equ 0x10
STACK_TOP equ 0x00090000
IA32_EFER equ 0xC0000080
CR0_PG equ 0x80000000
CR4_PAE equ 0x00000020
PAGE_PRESENT_WRITE equ 0x03

; EDIT 12: This startup block performs the 32-bit to 64-bit transition by loading the 64-bit GDT, 
; enabling PAE and LME, loading CR3 with the PML4, enabling paging, and far-jumping into 64-bit code.
_start:
    cli

    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax
    mov esp, STACK_TOP

    call setup_page_tables

    lgdt [gdt64_descriptor]

    mov eax, cr4
    or eax, CR4_PAE
    mov cr4, eax

    mov ecx, IA32_EFER
    rdmsr
    or eax, 0x00000100
    wrmsr

    mov eax, pml4_table
    mov cr3, eax

    mov eax, cr0
    or eax, CR0_PG
    mov cr0, eax

    jmp CODE_SEG:long_mode_start

; EDIT 13: This routine builds the required four-level paging hierarchy (PML4 -> PDPT -> PD -> PT) 
; and identity-maps the first 2 MiB with 4 KiB pages for early boot.
setup_page_tables:
    mov eax, pdpt_table
    or eax, PAGE_PRESENT_WRITE
    mov [pml4_table], eax
    mov dword [pml4_table + 4], 0

    mov eax, pd_table
    or eax, PAGE_PRESENT_WRITE
    mov [pdpt_table], eax
    mov dword [pdpt_table + 4], 0

    mov eax, pt_table
    or eax, PAGE_PRESENT_WRITE
    mov [pd_table], eax
    mov dword [pd_table + 4], 0

    mov edi, pt_table
    xor ebx, ebx
    mov ecx, 512

.map_pt:
    mov eax, ebx
    or eax, PAGE_PRESENT_WRITE
    mov [edi], eax
    mov dword [edi + 4], 0
    add ebx, 0x1000
    add edi, 8
    loop .map_pt

    ret

[BITS 64]
; EDIT 14: This is the first 64-bit code path, where the segment registers and 
; stack are reloaded before calling kernel_main in long mode.
long_mode_start:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov fs, ax
    mov gs, ax
    mov rsp, STACK_TOP
    xor rbp, rbp

    call kernel_main

.hang:
    hlt
    jmp .hang

section .rodata
align 8
; EDIT 15: This 64-bit GDT and these aligned paging tables provide the descriptor state and 
;storage required for the long mode and PML4 demonstration in the project.
gdt64:
    dq 0x0000000000000000
    dq 0x00209A0000000000
    dq 0x0000920000000000
gdt64_end:

gdt64_descriptor:
    dw gdt64_end - gdt64 - 1
    dd gdt64

section .data
align 4096
pml4_table:
    times 512 dq 0

align 4096
pdpt_table:
    times 512 dq 0

align 4096
pd_table:
    times 512 dq 0

align 4096
pt_table:
    times 512 dq 0