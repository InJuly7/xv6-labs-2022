// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"


void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
    struct run *next;
};

struct {
    struct spinlock lock;
    struct run *freelist;
} kmem;

// struct to maintain the ref counts
struct refc_stru{
    struct spinlock lock;
    int count[PGROUNDUP(PHYSTOP) / PGSIZE];
}refc;

void refcinc(void *pa) {
    acquire(&refc.lock);
    refc.count[PA2IDX(pa)]++;
    release(&refc.lock);
}

int refcdec(void *pa) {
    acquire(&refc.lock);
    refc.count[PA2IDX(pa)]--;
    release(&refc.lock);
    return refc.count[PA2IDX(pa)];
}

void refcset(void *pa, int cnt) {
    acquire(&refc.lock);
    refc.count[PA2IDX(pa)] = cnt;
    release(&refc.lock);
}

int getrefc(void *pa) {return refc.count[PA2IDX(pa)]; }

void kinit() {
    initlock(&refc.lock, "refc");
    initlock(&kmem.lock, "kmem");
    freerange(end, (void *)PHYSTOP);
}

void freerange(void *pa_start, void *pa_end) {
    char *p;
    p = (char *)PGROUNDUP((uint64)pa_start);
    for (; p + PGSIZE <= (char *)pa_end; p += PGSIZE) {
        refcset(p, 1);
        kfree(p);
    }
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void kfree(void *pa) {
    struct run *r;

    if (((uint64)pa % PGSIZE) != 0 || (char *)pa < end || (uint64)pa >= PHYSTOP)
        panic("kfree");

    if (refcdec(pa) > 0)
        return;

    // 如果引用数为0, 释放页面
    // Fill with junk to catch dangling refs.
    memset(pa, 1, PGSIZE);

    r = (struct run *)pa;

    acquire(&kmem.lock);
    r->next = kmem.freelist;
    kmem.freelist = r;
    release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *kalloc(void) {
    struct run *r;

    acquire(&kmem.lock);
    r = kmem.freelist;
    if (r) {
        kmem.freelist = r->next;
        refcset((void *)r, 1); // 引用数加1
    }
    release(&kmem.lock);

    if (r) {
        memset((char *)r, 5, PGSIZE); // fill with junk
    }
    return (void *)r;
}
