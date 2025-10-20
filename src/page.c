#include "page.h"

// Static array of 128 page descriptors (2 MiB each = 256 MiB total)
static struct ppage physical_page_array[128];

// Head of the free physical pages list
struct ppage *free_physical_pages = NULL;

/* ---------- internal helper functions ---------- */

// Push a node to the front of a doubly linked list
static void list_push_front(struct ppage **head, struct ppage *node) {
    node->prev = NULL;
    node->next = *head;
    if (*head)
        (*head)->prev = node;
    *head = node;
}

// Pop a node from the front of a list
static struct ppage *list_pop_front(struct ppage **head) {
    if (!*head)
        return NULL;
    struct ppage *n = *head;
    *head = n->next;
    if (*head)
        (*head)->prev = NULL;
    n->next = n->prev = NULL;
    return n;
}

// Return the tail of a list
static struct ppage *list_tail(struct ppage *head) {
    if (!head)
        return NULL;
    while (head->next)
        head = head->next;
    return head;
}

// Count list nodes
static unsigned int list_length(struct ppage *head) {
    unsigned int n = 0;
    while (head) {
        ++n;
        head = head->next;
    }
    return n;
}

/* ---------- public API ---------- */

// Initialize free list of pages
void init_pfa_list(void) {
    free_physical_pages = NULL;

    for (unsigned int i = 0; i < (sizeof(physical_page_array) / sizeof(physical_page_array[0])); ++i) {
        struct ppage *pp = &physical_page_array[i];
        pp->next = pp->prev = NULL;
        pp->physical_addr = (void *)(uintptr_t)(i * (uintptr_t)PFA_PAGE_BYTES);
        list_push_front(&free_physical_pages, pp);
    }
}

// Allocate npages pages, or return NULL if not enough
struct ppage *allocate_physical_pages(unsigned int npages) {
    if (npages == 0)
        return NULL;

    struct ppage *alloc_head = NULL;
    struct ppage *alloc_tail = NULL;

    for (unsigned int i = 0; i < npages; ++i) {
        struct ppage *got = list_pop_front(&free_physical_pages);
        if (!got) {
            // Roll back
            if (alloc_head)
                free_physical_pages(alloc_head);
            return NULL;
        }
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

// Free a linked list of pages back to the free list
void free_physical_pages(struct ppage *ppage_list) {
    if (!ppage_list)
        return;

    struct ppage *tail = list_tail(ppage_list);

    tail->next = free_physical_pages;
    if (free_physical_pages)
        free_physical_pages->prev = tail;

    ppage_list->prev = NULL;
    free_physical_pages = ppage_list;
}

// Return count of free pages (for debugging/testing)
unsigned int pfa_free_count(void) {
    return list_length(free_physical_pages);
}
