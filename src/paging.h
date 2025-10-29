#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>
#include "page.h"   // for struct ppage and (optionally) PFA_PAGE_BYTES

/* ===== i386 paging entry formats (exactly as in the assignment) ===== */
struct page_directory_entry
{
   uint32_t present       : 1;
   uint32_t rw            : 1;
   uint32_t user          : 1;
   uint32_t writethru     : 1;
   uint32_t cachedisabled : 1;
   uint32_t accessed      : 1;
   uint32_t pagesize      : 1;   // 0 => 4 KiB pages
   uint32_t ignored       : 2;
   uint32_t os_specific   : 3;
   uint32_t frame         : 20;  // phys >> 12 of page table
};

struct page
{
   uint32_t present  : 1;
   uint32_t rw       : 1;
   uint32_t user     : 1;
   uint32_t accessed : 1;
   uint32_t dirty    : 1;
   uint32_t unused   : 7;
   uint32_t frame    : 20;       // phys >> 12 of 4 KiB frame
};

/* ===== constants ===== */
#define PAGE_SIZE   4096u
#define PD_ENTRIES  1024u
#define PT_ENTRIES  1024u

#ifndef PFA_PAGE_BYTES
#define PFA_PAGE_BYTES PAGE_SIZE   // if your allocator returns 4 KiB pages, this is fine
#endif

/* ===== required global, 4096-byte aligned structures (ONE PD + ONE PT) ===== */
extern struct page_directory_entry kernel_pd[PD_ENTRIES] __attribute__((aligned(4096)));
extern struct page                 kernel_pt0[PT_ENTRIES] __attribute__((aligned(4096)));

/* ===== assignment API ===== */
void *map_pages(void *vaddr, struct ppage *pglist, struct page_directory_entry *pd);
void  loadPageDirectory(struct page_directory_entry *pd);
void  enablePaging(void);

#endif /* PAGING_H */
