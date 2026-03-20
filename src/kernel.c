#include "kernel.h"
#include <stdint.h>

// EDIT 09: This minimal VGA/debug output path gives the project a stable 64-bit proof-of-execution target without relying on old 32-bit subsystems.
static volatile uint16_t* video_mem = (volatile uint16_t*) 0xB8000;
static uint16_t terminal_row = 0;
static uint16_t terminal_col = 0;

static inline void debug_putc(char c)
{
    __asm__ volatile ("outb %0, $0xE9" : : "a"((uint8_t)c));
}

static uint16_t terminal_make_char(char c, uint8_t colour)
{
    return ((uint16_t) colour << 8) | (uint8_t) c;
}

void terminal_putchar(int x, int y, char c, char colour)
{
    video_mem[(y * VGA_WIDTH) + x] = terminal_make_char(c, (uint8_t) colour);
}

void terminal_writechar(char c, char colour)
{
    debug_putc(c);

    if (c == '\n')
    {
        terminal_row += 1;
        terminal_col = 0;
        if (terminal_row >= VGA_HEIGHT)
        {
            terminal_row = 0;
        }
        return;
    }

    terminal_putchar(terminal_col, terminal_row, c, colour);
    terminal_col += 1;
    if (terminal_col >= VGA_WIDTH)
    {
        terminal_col = 0;
        terminal_row += 1;
        if (terminal_row >= VGA_HEIGHT)
        {
            terminal_row = 0;
        }
    }
}

void terminal_initialize(void)
{
    terminal_row = 0;
    terminal_col = 0;
    for (int y = 0; y < VGA_HEIGHT; y++)
    {
        for (int x = 0; x < VGA_WIDTH; x++)
        {
            terminal_putchar(x, y, ' ', 0x00);
        }
    }
}

void print(const char* str)
{
    if (!str)
    {
        return;
    }

    while (*str)
    {
        terminal_writechar(*str, 0x0F);
        str++;
    }
}

void panic(const char* msg)
{
    print("PANIC: ");
    print(msg);
    for (;;)
    {
        __asm__ volatile ("hlt");
    }
}

// EDIT 10: This simplified kernel entry prints confirmation messages after the long mode transition so QEMU and GDB can verify the four-level paging bootstrap cleanly.
void kernel_main(void)
{
    terminal_initialize();
    print("PeachOS entered 64-bit long mode\n");
    print("4-level paging bootstrap active\n");
    print("Use GDB to inspect RIP, CR0, CR3, CR4, and the PML4\n");

    for (;;)
    {
        __asm__ volatile ("hlt");
    }
}