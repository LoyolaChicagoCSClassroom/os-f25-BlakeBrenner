#include "page.h"

// Statically allocated descriptor array for 128 pages (covers 256 MiB)
static struct ppage physical_page_array[128];

// Head of free physical pages (doubly-linked list)
struct ppage *free_physical_pages = NULL;

/* ---------- internal list helpers ---------- */

static void list_push_front(struct ppage **head, struct ppage *node) {
    node->prev = NULL;
    node->next = *head;
    if (*head) (*head)->prev = node;
    *head = node;
}

static struct ppage *list_pop_front(struct ppage **head) {
    if (!*head) return NULL;
    struct ppage *n = *head;
    *head = n->next;
    if (*head) (*head)->prev = NULL;
    n->next = n->prev = NULL;
    return n;
}

static struct ppage *list_tail(struct ppage *head) {
    if (!head) return NULL;
    while (head->next) head = head->next;
    return head;
}

static unsigned int list_length(struct ppage *head) {
    unsigned int n = 0;
    while (head) { ++n; head = head->next; }
    return n;
}

/* ---------- public API ---------- */

void init_pfa_list(void) {
    // Start with an empty free list
    free_physical_pages = NULL;

    // Initialize each descriptor and push to free list
    // Assign physical addresses 0, 2MiB, 4MiB, ...
    for (unsigned i = 0; i < (unsigned)(sizeof(physical_page_array)/sizeof(physical_page_array[0])); ++i) {
        struct ppage *pp = &physical_page_array[i];
        pp->next = pp->prev = NULL;

        // If your kernel knows the true base of available RAM, replace 0 with that base.
        pp->physical_addr = (void *)(uintptr_t)(i * (uintptr_t)PFA_PAGE_BYTES);

        list_push_front(&free_physical_pages, pp);
    }
}

struct ppage *allocate_physical_pages(unsigned int npages) {
    if (npages == 0) return NULL;

    // All-or-nothing: if we can’t get npages, roll back and return NULL.
    struct ppage *alloc_head = NULL;
    struct ppage *alloc_tail = NULL;

    for (unsigned int i = 0; i < npages; ++i) {
        struct ppage *got = list_pop_front(&free_physical_pages);
        if (!got) {
            // Roll back anything we already took
            if (alloc_head) {
                free_physical_pages(alloc_head);
            }
            return NULL;
        }
        // Append to the end of the allocated list
        if (!alloc_head) {
            alloc_head = alloc_tail = got;
        } else {
            alloc_tail->next = got;
            got->prev = alloc_tail;
            alloc_tail = got;
        }
    }

    return alloc_head;
}

void free_physical_pages(struct ppage *ppage_list) {
    if (!ppage_list) return;

    // Splice returned list at the front of the free list (O(length(list)) to find tail)
    struct ppage *tail = list_tail(ppage_list);

    // Link old head after tail
    tail->next = free_physical_pages;
    if (free_physical_pages) {
        free_physical_pages->prev = tail;
    }

    // New head is the returned list's head
    ppage_list->prev = NULL;
    free_physical_pages = ppage_list;
}

/* ---------- optional: lightweight diagnostics ---------- */

unsigned int pfa_free_count(void) {
    return list_length(free_physical_pages);
}
