#include <common.h>

#define PAGE_SIZE 4096
#define DEBUG 1

static int isinit = 0;
uintptr_t start_addr, end_addr, size;

struct Slab_Info{
    uintptr_t start;
    uintptr_t end;
    size_t size;

}slab_info[5];

int two_pow(int n)
{
    int ans = 1;
    for(int i = 0; i < n; i++)
        ans *= 2;
    return ans;
}
void init_memory()
{
    start_addr = (uintptr_t)heap.start;
    end_addr = (uintptr_t)heap.end;
    size = end_addr - start_addr;
    for(int i = 0; i < 5; i++)
    {
        slab_info[i].size = two_pow(i+5);
        slab_info[i].start = start_addr + i * PAGE_SIZE * 1024;  // every slab is 4MB
        slab_info[i].end = slab_info[i].start + PAGE_SIZE * 1024; // every slab is 4MB

#if DEBUG
        printf("Slab %d: start = %p, end = %p, size = %d\n", i, slab_info[i].start, slab_info[i].end, slab_info[i].size);
#endif

    }

}


static void *kalloc(size_t size)
{
    if(!isinit){
        isinit = 1;
        init_memory();
    }

    return NULL;
}

static void kfree(void *ptr) {
    // TODO
    // You can add more .c files to the repo.
}

static void pmm_init() {
    uintptr_t pmsize = (
        (uintptr_t)heap.end
        - (uintptr_t)heap.start
    );

    printf(
        "Got %d MiB heap: [%p, %p)\n",
        pmsize >> 20, heap.start, heap.end
    );
}

MODULE_DEF(pmm) = {
    .init  = pmm_init,
    .alloc = kalloc,
    .free  = kfree,
};
