#include <stdint.h>

// VGA text buffer is 80x25 characters
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

struct termbuf {
    char ascii;   // Character to display
    char color;   // Color attribute (foreground/background)
};

// Cursor position
int cursor_x = 0;
int cursor_y = 0;

// Print a single character to the VGA text buffer
void print_char(char c) {
    volatile struct termbuf *vram = (volatile struct termbuf *)0xB8000;

    // Handle newlines
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else {
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
        cursor_y = VGA_HEIGHT - 1;
        // Scrolling would be implemented here
    }
}

// Print a string of characters
void print_string(const char *s) {
    while (*s != '\0') {
        print_char(*s);
        s++;
    }
}

// Entry point
void main() {
    print_char('B');
    print_char('L');
    print_char('A');
    print_char('K');
    print_char('E');

    print_char('\n');
    print_string("Hello, World!");
}
