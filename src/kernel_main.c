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



unsigned char keyboard_map[128] =
{
   0,  27, '1', '2', '3', '4', '5', '6', '7', '8',     /* 9 */
 '9', '0', '-', '=', '\b',     /* Backspace */
 '\t',                 /* Tab */
 'q', 'w', 'e', 'r',   /* 19 */
 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', /* Enter key */
   0,                  /* 29   - Control */
 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',     /* 39 */
'\'', '`',   0,                /* Left shift */
'\\', 'z', 'x', 'c', 'v', 'b', 'n',                    /* 49 */
 'm', ',', '.', '/',   0,                              /* Right shift */
 '*',
   0,  /* Alt */
 ' ',  /* Space bar */
   0,  /* Caps lock */
   0,  /* 59 - F1 key ... > */
   0,   0,   0,   0,   0,   0,   0,   0,  
   0,  /* < ... F10 */
   0,  /* 69 - Num lock*/
   0,  /* Scroll Lock */
   0,  /* Home key */
   0,  /* Up Arrow */
   0,  /* Page Up */
 '-',
   0,  /* Left Arrow */
   0,  
   0,  /* Right Arrow */
 '+',
   0,  /* 79 - End key*/
   0,  /* Down Arrow */
   0,  /* Page Down */
   0,  /* Insert Key */
   0,  /* Delete Key */
   0,   0,   0,  
   0,  /* F11 Key */
   0,  /* F12 Key */
   0,  /* All other keys are undefined */
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

    int print_string(void (*pc)(char), char *s){
        while(*s != 0){
            uint8_t status = inb(0x64);
            pc(*s);
            s++;
        }
    }
// Kernel entry point
void main() {
    esp_printf(putc, "Hello, World!\n");
    esp_printf(putc, "Execution level: %d\n", 0);


    while (1){
         // Read keyboard controller status port (0x64)
        uint8_t status = inb(0x64);

        // If output buffer is full (bit 0 = 1), read scancode
        if (status & 1) {
            uint8_t scancode = inb(0x60);   // Get scancode from port 0x60

            // Only process valid scancodes (< 128 = key press)
            if (scancode < 128) {
                rprintf(putc, "0x%02x %c\n", scancode, keyboard_map[scancode]);
            } else {
                // Key release codes (>= 128)
                rprintf(putc, "0x%02x (release)\n", scancode);
        }
    } // Prevent CPU from running into invalid instructions
}