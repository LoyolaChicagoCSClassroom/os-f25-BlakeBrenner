#include <stdint.h>
#include "paging.h"
#include "page.h"

#define PT_POOL_COUNT 64  // supports up to 64 page tables (64 * 4MiB = 256MiB of VA span)

// Root page directory (must be global + 4KiB-aligned)
struct page_directory_entry kernel_pd[PD_ENTRIES] __attribute__((aligned(4096)));

// Small on-demand page table pool (global + 4KiB-aligned)
static struct page kernel_pts[PT_POOL_COUNT][PT_ENTRIES] __attribute__((aligned(4096)));
static uint32_t kernel_pt_count = 0;

/* ---------- tiny helpers (no libc needed) ---------- */
static inline uint32_t align_down(uint32_t x, uint32_t a) { return x & ~(a - 1u); }
static inline uint32_t vaddr_pdi(uint32_t v) { return (v >> 22) & 0x3FFu; }
static inline uint32_t vaddr_pti(uint32_t v) { return (v >> 12) & 0x3FFu; }

static void bzero(void *dst, uint32_t bytes) {
    uint32_t *p = (uint32_t*)dst;
    for (uint32_t n = bytes >> 2; n; --n) { *p++ = 0; }
    uint8_t *q = (uint8_t*)p;
    for (uint32_t i = 0; i < (bytes & 3u); ++i) q[i] = 0;
}

/* Get existing PT for PDE index, or create a new one from pool. */
static struct page* get_or_make_pt(struct page_directory_entry *pd, uint32_t pdi) {
    if (pd[pdi].present) {
        uint32_t pt_phys = pd[pdi].frame << 12;
        return (struct page*)pt_phys; // identity during bring-up
    }
    if (kernel_pt_count >= PT_POOL_COUNT) return 0; // out of PTs

    struct page *pt = &kernel_pts[kernel_pt_count++][0];
    bzero(pt, sizeof(kernel_pts[0]));

    pd[pdi].present       = 1;
    pd[pdi].rw            = 1;
    pd[pdi].user          = 0;
    pd[pdi].writethru     = 0;
    pd[pdi].cachedisabled = 0;
    pd[pdi].accessed      = 0;
    pd[pdi].pagesize      = 0;                 // 4KiB pages
    pd[pdi].frame         = ((uint32_t)pt) >> 12; // physical (identity) address >> 12
    return pt;
}

/* Map a single 4KiB page: VA 'va' -> PA 'pa' */
static void map_4k(struct page_directory_entry *pd, uint32_t va, uint32_t pa) {
    uint32_t pdi = vaddr_pdi(va);
    uint32_t pti = vaddr_pti(va);

    struct page *pt = get_or_make_pt(pd, pdi);
    if (!pt) return;

    pt[pti].present  = 1;
    pt[pti].rw       = 1;
    pt[pti].user     = 0;
    pt[pti].accessed = 0;
    pt[pti].dirty    = 0;
    pt[pti].frame    = pa >> 12;
}

/* Public: map a list of *2MiB* physical pages starting at vaddr as 4KiB PTEs. */
void *map_pages(void *vaddr, struct ppage *pglist, struct page_directory_entry *pd) {
    uint32_t va = align_down((uint32_t)vaddr, PAGE_SIZE);
    for (struct ppage *cur = pglist; cur; cur = cur->next) {
        uint32_t pa_base = (uint32_t)(uintptr_t)cur->physical_addr;
        // Expand the 2MiB frame into 512 x 4KiB PTEs
        for (uint32_t off = 0; off < PFA_PAGE_BYTES; off += PAGE_SIZE) {
            map_4k(pd, va, pa_base + off);
            va += PAGE_SIZE;
        }
    }
    return (void*)align_down((uint32_t)vaddr, PAGE_SIZE);
}

/* Public: identity-map [start, end) in 4KiB steps. */
void identity_map_range(uint32_t start, uint32_t end, struct page_directory_entry *pd) {
    uint32_t va = align_down(start, PAGE_SIZE);
    uint32_t lim = align_down(end + PAGE_SIZE - 1, PAGE_SIZE);
    for (uint32_t a = va; a < lim; a += PAGE_SIZE) {
        map_4k(pd, a, a);
    }
}

/* Public: paging control */
void load_page_directory(struct page_directory_entry *pd) {
    asm volatile("mov %0, %%cr3" :: "r"(pd) : "memory");
}
void enable_paging(void) {
    asm volatile(
        "mov %%cr0, %%eax\n"
        "or  $0x80000001, %%eax\n"  // set PG (bit 31) and PE (bit 0)
        "mov %%eax, %%cr0\n"
        ::: "eax", "memory"
    );
}
