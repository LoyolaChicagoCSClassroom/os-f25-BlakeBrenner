#include <stdint.h>
#include "rprintf.h"

// VGA text buffer dimensions
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

// VGA text mode memory starts at 0xB8000
struct termbuf {
    char ascii;   // Character to display
    char color;   // Color attribute (foreground/background)
};

// Cursor position
int cursor_x = 0;
int cursor_y = 0;

// Forward declaration
void putc(int ch);
void scroll_screen(void);
void clear_screen(void);

// SCROLLING FUNCTION
void scroll_screen() {
    volatile struct termbuf *vram = (volatile struct termbuf *)0xB8000;

    // Move each line up
    for (int row = 1; row < VGA_HEIGHT; row++) {
        for (int col = 0; col < VGA_WIDTH; col++) {
            vram[(row - 1) * VGA_WIDTH + col] = vram[row * VGA_WIDTH + col];
        }
    }

    // Clear the last line
    for (int col = 0; col < VGA_WIDTH; col++) {
        vram[(VGA_HEIGHT - 1) * VGA_WIDTH + col].ascii = ' ';
        vram[(VGA_HEIGHT - 1) * VGA_WIDTH + col].color = 0x07;
    }
}

// CLEAR SCREEN FUNCTION
void clear_screen() {
    volatile struct termbuf *vram = (volatile struct termbuf *)0xB8000;
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vram[i].ascii = ' ';
        vram[i].color = 0x07;
    }
    cursor_x = 0;
    cursor_y = 0;
}

// PUT CHARACTER (Required for assignment)
void putc(int ch) {
    volatile struct termbuf *vram = (volatile struct termbuf *)0xB8000;
    char c = (char)(ch & 0xFF);

    // Handle carriage return
    if (c == '\r') {
        cursor_x = 0;
        return;
    }
    // Handle newline
    else if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    }
    // Regular character
    else {
        int index = cursor_y * VGA_WIDTH + cursor_x;
        vram[index].ascii = c;
        vram[index].color = 0x07; // Light gray on black
        cursor_x++;
    }

    // Wrap to the next line if we reach end of screen width
    if (cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }

    // Scroll if we go past the last line
    if (cursor_y >= VGA_HEIGHT) {
        scroll_screen();
        cursor_y = VGA_HEIGHT - 1;
    }
}

// PRINT STRING USING PUT CHARACTER
void print_string(const char *s) {
    while (*s != '\0') {
        putc(*s++);
    }
}

// READ CURRENT EXECUTION LEVEL (CPL)
static inline unsigned short read_cs() {
    unsigned short cs;
    __asm__ volatile ("mov %%cs, %0" : "=r"(cs));
    return cs;
}

void print_cpl() {
    int cpl = read_cs() & 0x3; // CPL is low 2 bits of CS
    print_string("Current execution level: ");
    putc('0' + cpl); // convert number to ASCII
    putc('\n');
}
// MAIN ENTRY POINT
void main() {
    clear_screen();

    // Test direct character output
    putc('B');
    putc('L');
    putc('A');
    putc('K');
    putc('E');
    putc('\n');

    // Test print_string
    print_string("Hello, World!\n");

    // Test scrolling by printing many lines
    for (int i = 0; i < 30; i++) {
        print_string("This is a test line that will scroll...\n");
    }

    // Print the current execution level
    print_cpl();
}
