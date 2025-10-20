#ifndef PAGE_H
#define PAGE_H

#include <stddef.h>
#include <stdint.h>

// 2 MiB pages, as specified
#define PFA_PAGE_BYTES (2u * 1024u * 1024u)

// Doubly-linked list node describing one physical page
struct ppage {
    struct ppage *next;
    struct ppage *prev;
    void *physical_addr; // physical address of the start of this page
};

// Global head of the free list (defined in page.c)
extern struct ppage *free_physical_pages;

// Initialize the free-page list from the static array
void init_pfa_list(void);

// Allocate exactly npages contiguous list nodes from the free list.
// Returns the head of a new list, or NULL if not enough pages are available.
struct ppage *allocate_physical_pages(unsigned int npages);

// Return a list of pages back to the free list (order doesn’t matter).
void free_physical_pages(struct ppage *ppage_list);

#endif // PAGE_H
