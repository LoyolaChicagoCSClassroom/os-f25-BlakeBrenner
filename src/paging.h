#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>
#include "page.h"

// 4 KiB paging constants
#define PAGE_SIZE      4096u
#define PD_ENTRIES     1024u
#define PT_ENTRIES     1024u

// i386 paging entries (bitfields match assignment)
struct page_directory_entry {
    uint32_t present       : 1;
    uint32_t rw            : 1;
    uint32_t user          : 1;
    uint32_t writethru     : 1;
    uint32_t cachedisabled : 1;
    uint32_t accessed      : 1;
    uint32_t pagesize      : 1;   // 0 = 4KiB, 1 = 4MiB
    uint32_t ignored       : 2;
    uint32_t os_specific   : 3;
    uint32_t frame         : 20;  // top 20 bits of physical addr
};

struct page {
    uint32_t present  : 1;
    uint32_t rw       : 1;
    uint32_t user     : 1;
    uint32_t accessed : 1;
    uint32_t dirty    : 1;
    uint32_t unused   : 7;
    uint32_t frame    : 20;       // top 20 bits of physical addr
};

// Global, 4KiB-aligned directory (declared+defined in paging.c)
extern struct page_directory_entry kernel_pd[PD_ENTRIES];

// Map a linked list of *2 MiB* physical pages (from your PFA) starting at vaddr.
// Returns the (page-aligned) virtual address that was mapped.
void *map_pages(void *vaddr, struct ppage *pglist, struct page_directory_entry *pd);

// Identity-map an arbitrary [start, end) range using 4 KiB pages.
void identity_map_range(uint32_t start, uint32_t end, struct page_directory_entry *pd);

// Load CR3 and enable paging (CR0.PG|PE).
void load_page_directory(struct page_directory_entry *pd);
void enable_paging(void);

#endif
