#include "paging.h"

/* ===== single page directory + single page table (covers 0..4MiB VA) ===== */
struct page_directory_entry kernel_pd[PD_ENTRIES] __attribute__((aligned(4096)));
struct page                 kernel_pt0[PT_ENTRIES] __attribute__((aligned(4096)));

/* tiny helpers */
static inline uint32_t align_down(uint32_t x) { return x & ~0xFFFu; }
static inline uint32_t vaddr_pdi(uint32_t v)  { return (v >> 22) & 0x3FFu; }
static inline uint32_t vaddr_pti(uint32_t v)  { return (v >> 12) & 0x3FFu; }

/* Ensure PDE[0] points to our single page table (4 KiB pages). */
static void ensure_pde0(void) {
    if (!kernel_pd[0].present) {
        // zero-init PT
        for (uint32_t i = 0; i < PT_ENTRIES; ++i) {
            kernel_pt0[i].present  = 0;
            kernel_pt0[i].rw       = 0;
            kernel_pt0[i].user     = 0;
            kernel_pt0[i].accessed = 0;
            kernel_pt0[i].dirty    = 0;
            kernel_pt0[i].unused   = 0;
            kernel_pt0[i].frame    = 0;
        }
        // point PDE[0] to PT0 (identity assumption during bring-up)
        kernel_pd[0].present       = 1;
        kernel_pd[0].rw            = 1;
        kernel_pd[0].user          = 0;
        kernel_pd[0].writethru     = 0;
        kernel_pd[0].cachedisabled = 0;
        kernel_pd[0].accessed      = 0;
        kernel_pd[0].pagesize      = 0;                 // 4 KiB pages
        kernel_pd[0].ignored       = 0;
        kernel_pd[0].os_specific   = 0;
        kernel_pd[0].frame         = ((uint32_t)(uintptr_t)kernel_pt0) >> 12;
    }
}

/* Map a list of physical pages at starting virtual address (FIRST 4MiB ONLY). */
void *map_pages(void *vaddr, struct ppage *pglist, struct page_directory_entry *pd) {
    (void)pd; // we only support kernel_pd in this minimal version
    ensure_pde0();

    uint32_t va = align_down((uint32_t)(uintptr_t)vaddr);

    for (struct ppage *cur = pglist; cur; cur = cur->next) {
        uint32_t pa_base = (uint32_t)(uintptr_t)cur->physical_addr;

        // Only support addresses in PDE index 0 (0x00000000..0x003FFFFF)
        if (vaddr_pdi(va) != 0) {
            // silently stop (or you could assert / print if you have logging)
            break;
        }

        // If your allocator returns >4KiB (e.g., 2MiB), expand to 4KiB PTEs:
        for (uint32_t off = 0; off < PFA_PAGE_BYTES; off += PAGE_SIZE) {
            if (vaddr_pdi(va) != 0) break;     // don't cross out of PT0
            uint32_t pti = vaddr_pti(va);

            kernel_pt0[pti].present  = 1;
            kernel_pt0[pti].rw       = 1;
            kernel_pt0[pti].user     = 0;
            kernel_pt0[pti].accessed = 0;
            kernel_pt0[pti].dirty    = 0;
            kernel_pt0[pti].unused   = 0;
            kernel_pt0[pti].frame    = (pa_base + off) >> 12;

            va += PAGE_SIZE;
        }
    }
    return (void*)align_down((uint32_t)(uintptr_t)vaddr);
}

/* CR3 := PD base (must be phys & 4 KiB aligned). */
void loadPageDirectory(struct page_directory_entry *pd) {
    __asm__ __volatile__("mov %0, %%cr3" :: "r"(pd) : "memory");
}

/* Enable protected mode + paging: set CR0.PE|PG */
void enablePaging(void) {
    __asm__ __volatile__(
        "mov %%cr0, %%eax\n"
        "or  $0x80000001, %%eax\n"
        "mov %%eax, %%cr0\n"
        ::: "eax", "memory"
    );
}
