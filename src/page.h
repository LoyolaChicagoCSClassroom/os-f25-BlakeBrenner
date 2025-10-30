#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>
#include "page.h"   // for struct ppage and PFA_PAGE_BYTES / allocator types

/* ===== i386 paging entry formats (from assignment) ===== */
struct page_directory_entry
{
   uint32_t present       : 1;   // Page present in memory
   uint32_t rw            : 1;   // Read-only if clear, R/W if set
   uint32_t user          : 1;   // Supervisor only if clear
   uint32_t writethru     : 1;   // Cache this directory as write-thru only
   uint32_t cachedisabled : 1;   // Disable cache on this page table?
   uint32_t accessed      : 1;   // Accessed
   uint32_t pagesize      : 1;   // 0 => 4 KiB pages
   uint32_t ignored       : 2;
   uint32_t os_specific   : 3;
   uint32_t frame         : 20;  // physical addr >> 12 of the page table
};

struct page
{
   uint32_t present  : 1;   // Page present in memory
   uint32_t rw       : 1;   // Read-only if clear, read/write if set
   uint32_t user     : 1;   // Supervisor only if clear
   uint32_t accessed : 1;   // Accessed
   uint32_t dirty    : 1;   // Written
   uint32_t unused   : 7;
   uint32_t frame    : 20;  // physical addr >> 12 of the 4 KiB frame
};

/* ===== constants ===== */
#define PAGE_SIZE    4096u
#define PD_ENTRIES   1024u
#define PT_ENTRIES   1024u

#ifndef PFA_PAGE_BYTES
#define PFA_PAGE_BYTES PAGE_SIZE   // if allocator gives 4 KiB pages, this is fine
#endif

/* ===== global paging state =====
   These MUST be global and 4096-byte aligned (requirement). */
extern struct page_directory_entry kernel_pd[PD_ENTRIES] __attribute__((aligned(4096)));

/* Pool of page tables we can hand out when we need a new PT.
   Each PT is its own 4 KiB page, aligned. */
#ifndef PT_POOL_COUNT
#define PT_POOL_COUNT 64
#endif
extern struct page kernel_pt_pool[PT_POOL_COUNT][PT_ENTRIES] __attribute__((aligned(4096)));

/* Map a linked list of physical pages (pglist) starting at virtual address vaddr.
 * Returns page-aligned starting VA.
 *
 * Used for:
 *   - identity mapping kernel [0x100000 .. &_end_kernel)
 *   - identity mapping stack window around ESP
 *   - identity mapping VGA 0xB8000
 *
 * NOTE: During identity mapping you will build a temporary struct ppage
 * with .physical_addr = same address as vaddr and .next = NULL,
 * then call map_pages() in a loop, per the assignment.
 */
void *map_pages(void *vaddr, struct ppage *pglist, struct page_directory_entry *pd);

/* Load CR3 with the physical address of the page directory. */
void loadPageDirectory(struct page_directory_entry *pd);

/* Enable paging: set CR0.PE (bit0) and CR0.PG (bit31). */
void enablePaging(void);

/* Set up recursive paging:
 * kernel_pd[1023] points back to kernel_pd itself.
 * Call this ONCE before loadPageDirectory().
 * After paging is enabled:
 *   - Page directory is visible at 0xFFFFF000
 *   - Page table i is visible at 0xFFC00000 + (i * 0x1000)
 */
void paging_init_recursive(struct page_directory_entry *pd);

void *get_physaddr(void *virtualaddr);


int map_page(void *physaddr, void *virtualaddr, unsigned int flags);

#endif /* PAGING_H */
