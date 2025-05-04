#include <common.h>

#define PAGE_SIZE 4096
#define Slab_num 1024
#define DEBUG 1

static int isinit = 0;
uintptr_t start_addr, end_addr, size;
spinlock_t big_kernel_lock = spin_init("big_kernel_lock");
spinlock_t slab_lock = spin_init("slab_lock");

// Use slab_page to make detailed divisions
typedef struct {
    bool is_used[128];  // waste
    size_t obj_count;
    size_t obj_size;
    size_t used_count;
}slab_page;
// Use slab_info to save different sizes of slabs
struct Slab_Info{
    uintptr_t start;
    uintptr_t end;
    size_t size;
    slab_page page[Slab_num];
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
    if(isinit)  return;
    isinit = 1;
    // init heap
    start_addr = (uintptr_t)heap.start;
    end_addr = (uintptr_t)heap.end;
    size = end_addr - start_addr;
    // init slab: O(5*Slab_num*128)
    for(int i = 0; i < 5; i++)
    {
        slab_info[i].size = two_pow(i+5);
        slab_info[i].start = start_addr + i * PAGE_SIZE * Slab_num;  // every slab is 4MB
        slab_info[i].end = slab_info[i].start + PAGE_SIZE * Slab_num;
        for(int j=0; j < Slab_num; j++)
        {
            slab_info[i].page[j].obj_size = slab_info[i].size;
            slab_info[i].page[j].obj_count = PAGE_SIZE / slab_info[i].size;
            slab_info[i].page[j].used_count = 0;
            for(int k = 0; k < 128; k++)
                slab_info[i].page[j].is_used[k] = false;
        }
#if DEBUG
        printf("Slab %d: start = %p, end = %p, size = %d\n", i, slab_info[i].start, slab_info[i].end, slab_info[i].size);
        printf("obj_size = %d, obj_count = %d\n", slab_info[i].page[0].obj_size, slab_info[i].page[0].obj_count);
#endif
    }

}

size_t align_the_size(size_t size)
{
    size_t res;
    // ask for the slab
    if(size <= 512){
        res = 512;
        for(int i = 4;i >= 0;i--){
            if(slab_info[i].size >= size)
                res = slab_info[i].size;
            else break;
        }
    }
    else    panic("wait to exploit");
    return res;
}
void get_slab(size_t size)
{
    
}
static void *kalloc(size_t size)
{
    size = align_the_size(size);
    uintptr_t* res_ptr = NULL;

    // get the memory
    if(size <= 512)
    {
        // choose the lock and find the slab
        spin_lock(&slab_lock);
        get_slab(size);
        spin_unlock(&slab_lock);
    }
    else{
        panic("wait to exploit");
    }
    return res_ptr;
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

    spin_lock(&big_kernel_lock);
    init_memory();
    spin_unlock(&big_kernel_lock);
}

MODULE_DEF(pmm) = {
    .init  = pmm_init,
    .alloc = kalloc,
    .free  = kfree,
};
