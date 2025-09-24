#include <stdint.h>
#include "rprintf.h"

#define MULTIBOOT2_HEADER_MAGIC 0xe85250d6

// Multiboot2 header for GRUB
const unsigned int multiboot_header[] __attribute__((section(".multiboot"))) = {
    MULTIBOOT2_HEADER_MAGIC, 0, 16, -(16 + MULTIBOOT2_HEADER_MAGIC), 0, 12
};

uint8_t inb(uint16_t _port) {
    uint8_t rv;
    __asm__ __volatile__("inb %1, %0" : "=a"(rv) : "dN"(_port));
    return rv;
}

// VGA definitions
#define VGA_ADDRESS 0xb8000
#define VGA_WIDTH   80
#define VGA_HEIGHT  25

struct termbuf {
    char ascii;
    char color;
};

static int cursor_row = 0;
static int cursor_column = 0;

// Pointer to VGA memory
static struct termbuf* const vram = (struct termbuf*)VGA_ADDRESS;

// Scroll the screen up by one line
void scroll() {
    // Move each line up
    for (int row = 1; row < VGA_HEIGHT; row++) {
        for (int col = 0; col < VGA_WIDTH; col++) {
            vram[(row - 1) * VGA_WIDTH + col] = vram[row * VGA_WIDTH + col];
        }
    }

    // Clear the last line
    for (int col = 0; col < VGA_WIDTH; col++) {
        vram[(VGA_HEIGHT - 1) * VGA_WIDTH + col].ascii = ' ';
        vram[(VGA_HEIGHT - 1) * VGA_WIDTH + col].color = 7; // White on black
    }
}

// Print a single character
int putc(int ch) {
    if (ch == '\n') {
        cursor_row++;
        cursor_column = 0;
    } else {
        vram[cursor_row * VGA_WIDTH + cursor_column].ascii = (char)ch;
        vram[cursor_row * VGA_WIDTH + cursor_column].color = 7;
        cursor_column++;
    }

    // Wrap to next line if end of row
    if (cursor_column >= VGA_WIDTH) {
        cursor_column = 0;
        cursor_row++;
    }

    // Scroll if we're past the bottom
    if (cursor_row >= VGA_HEIGHT) {
        scroll();
        cursor_row = VGA_HEIGHT - 1;
    }

    return ch;
}

// Kernel entry point
void main() {
    esp_printf(putc, "Hello, World!\n");
    esp_printf(putc, "Execution level: %d\n", 0);

    // Test scrolling
    for (int i = 0; i < 30; i++) {
        esp_printf(putc, "Scrolling at line %d\n", i);
    }

    while (1); // Prevent CPU from running into invalid instructions
}