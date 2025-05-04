#include <common.h>

#define PAGE_SIZE 4096
#define Slab_num 1024
#define Buddy_num 1024
#define DEBUG 0

static int isinit = 0;
uintptr_t start_addr, end_addr, size;
uintptr_t slab_bound, buddy_bound;
bool *tag_the_used;
spinlock_t big_kernel_lock = spin_init("big_kernel_lock");
spinlock_t slab_lock = spin_init("slab_lock");
spinlock_t buddy_lock = spin_init("buddy_lock"); 

struct Manager_area{
    uintptr_t pos;
    uintptr_t start;
    uintptr_t end;
}manager_area;
// Use slab_page to make detailed divisions
typedef struct {
    bool *is_used;  // waste
    size_t obj_count;
    size_t obj_size;
    size_t used_count;
}slab_page;
// Use slab_info to save different sizes of slabs
// from 32B to 4KB
struct Slab_Info{
    uintptr_t start;
    uintptr_t end;
    size_t size;
    slab_page page[Slab_num];
}slab_info[8];
// Use buddy_page to make detailed divisions
typedef struct {
    bool *is_used;  // waste
    size_t obj_count;
    size_t obj_size;
    size_t used_count;
}buddy_page;
// Use buddy_info to save different sizes of pages
// from 8KB to 1M
struct Buddy_Info{
    uintptr_t start;
    uintptr_t end;
    size_t size;
    buddy_page page;
}buddy_info[8];

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
    // init manager_area
    // the distance between the start and end address is 20MB
    manager_area.start = end_addr-PAGE_SIZE * 256 * 20;
    manager_area.pos = manager_area.start;
    manager_area.end = end_addr;
    // init slab: O(5*Slab_num*128)
    for(int i = 0; i < 8; i++)
    {
        slab_info[i].size = two_pow(i+5);
        slab_info[i].start = start_addr + i * PAGE_SIZE * Slab_num;  // every slab is 4MB
        slab_info[i].end = slab_info[i].start + PAGE_SIZE * Slab_num;
        for(int j=0; j < Slab_num; j++)
        {
            slab_info[i].page[j].obj_size = slab_info[i].size;
            slab_info[i].page[j].obj_count = PAGE_SIZE / slab_info[i].size;
            slab_info[i].page[j].used_count = 0;
            for(int k=0; k<slab_info[i].page[j].obj_count ; k++)
            {
                if(k == 0) {
                    slab_info[i].page[j].is_used = (bool *)manager_area.pos;
                    manager_area.pos += slab_info[i].page[j].obj_count * sizeof(bool);
                }
                slab_info[i].page[j].is_used[k] = false;
            }
        }
#if DEBUG
        printf("Slab %d: start = %p, end = %p, size = %d\n", i, slab_info[i].start, slab_info[i].end, slab_info[i].size);
        printf("obj_size = %d, obj_count = %d\n", slab_info[i].page[0].obj_size, slab_info[i].page[0].obj_count);
#endif
    }
    slab_bound = slab_info[7].end;
    
    // init buddy
    for(int i=0;i<8;i++)
    {
        buddy_info[i].size = two_pow(i+13);
        buddy_info[i].start = slab_bound + i * PAGE_SIZE * Buddy_num;
        buddy_info[i].end = buddy_info[i].start + PAGE_SIZE * Buddy_num;
        buddy_info[i].page.obj_size = buddy_info[i].size;
        buddy_info[i].page.obj_count = PAGE_SIZE * Buddy_num / buddy_info[i].size;
        buddy_info[i].page.used_count = 0;
        for(int j=0;j<buddy_info[i].page.obj_count;j++)
        {
            if(j == 0) {
                buddy_info[i].page.is_used = (bool *)manager_area.pos;
                manager_area.pos += buddy_info[i].page.obj_count * sizeof(bool);
            }
            buddy_info[i].page.is_used[j] = false;
        }
#if DEBUG
        printf("Buddy %d: start = %p, end = %p, size = %d\n", i, buddy_info[i].start, buddy_info[i].end, buddy_info[i].size);
        printf("obj_size = %d, obj_count = %d\n", buddy_info[i].page.obj_size, buddy_info[i].page.obj_count);
#endif
    }
    buddy_bound = buddy_info[7].end;
}

size_t align_the_size(size_t size)
{
    size_t res;
    // calculate the size
    if(size <= PAGE_SIZE){
        res = PAGE_SIZE;
        for(int i = 7;i >= 0;i--){
            if(slab_info[i].size >= size)
                res = slab_info[i].size;
            else break;
        }
    }
    else if(size <= PAGE_SIZE * Buddy_num){
        res = PAGE_SIZE * Buddy_num;
        for(int i = 7;i >= 0;i--){
            if(buddy_info[i].size >= size)
                res = buddy_info[i].size;
            else break;
        }
    }
    else    panic("You need to many memories!!!\n");
    return res;
}
uintptr_t *get_slab(size_t size)
{
    int idx = 0;
    for(int i=0;i<8;i++)
        if(slab_info[i].size == size)
            {idx = i; break;}
    for(int i=0;i<Slab_num;i++)
    {
        if(slab_info[idx].page[i].used_count == slab_info[idx].page[i].obj_count)   continue;
        // search for the free page
        for(int j=0;j<slab_info[idx].page[i].obj_count;j++)
        {
            if(slab_info[idx].page[i].is_used[j] == false)
            {
                slab_info[idx].page[i].is_used[j] = true;
                slab_info[idx].page[i].used_count++;
                uintptr_t *res_ptr = (uintptr_t *)(slab_info[idx].start + i * PAGE_SIZE + j * slab_info[idx].size);
#if DEBUG
                printf("Allocate the memory address is %p\n", res_ptr);
#endif
                return res_ptr;
            }
        }
    }
    return NULL;
}
uintptr_t *get_buddy(size_t size)
{
    int idx = 0;
    for(int i=0;i<8;i++)
        if(buddy_info[i].size == size)
            {idx = i; break;}
    if(buddy_info[idx].page.used_count == buddy_info[idx].page.obj_count)    return NULL;
    // search for the free page
    for(int i=0;i<buddy_info[idx].page.obj_count;i++)
    {
        if(buddy_info[idx].page.is_used[i] == false)
        {
            buddy_info[idx].page.is_used[i] = true;
            buddy_info[idx].page.used_count++;
            uintptr_t *res_ptr = (uintptr_t *)(buddy_info[idx].start + i * buddy_info[idx].size);
#if DEBUG
            printf("Allocate the memory address is %p\n", res_ptr);
#endif
            return res_ptr;
        }
    }
    return NULL;
}
static void *kalloc(size_t size)
{
    size = align_the_size(size);
    uintptr_t *res_ptr = NULL;

    // get the memory
    if(size <= PAGE_SIZE)
    {
        // choose the lock and find the slab
        spin_lock(&slab_lock);
        res_ptr = get_slab(size);
        spin_unlock(&slab_lock);
    }
    else{
        // choose the lock and find the buddy
        spin_lock(&buddy_lock);
        res_ptr = get_buddy(size);
        spin_unlock(&buddy_lock);
    }
    return res_ptr;
}

// kfree's part
void ret_the_slab(void *ptr)
{
    int idx = 0;
    for(int i=0;i<8;i++)
        if((uintptr_t)ptr >= slab_info[i].start && (uintptr_t)ptr < slab_info[i].end)
            {idx = i; break;}
    int page_num = ((uintptr_t)ptr - slab_info[idx].start) / PAGE_SIZE;
    int obj_num = ((uintptr_t)ptr - slab_info[idx].start - page_num * PAGE_SIZE) / slab_info[idx].size;
    assert(slab_info[idx].start + page_num * PAGE_SIZE + obj_num * slab_info[idx].size == (uintptr_t)ptr);
    slab_info[idx].page[page_num].is_used[obj_num] = false;
    slab_info[idx].page[page_num].used_count--;
#if DEBUG
    printf("Free the memory address is %p\n", ptr);
#endif
}
void ret_the_buddy(void *ptr)
{
    int idx = 0;
    for(int i=0;i<8;i++)
        if((uintptr_t)ptr >= buddy_info[i].start && (uintptr_t)ptr < buddy_info[i].end)
            {idx = i; break;}
    int obj_num = ((uintptr_t)ptr - buddy_info[idx].start) / buddy_info[idx].size;
    assert(buddy_info[idx].start + obj_num * buddy_info[idx].size == (uintptr_t)ptr);
    buddy_info[idx].page.is_used[obj_num] = false;
    buddy_info[idx].page.used_count--;
#if DEBUG
    printf("Free the memory address is %p\n", ptr);
#endif
}
static void kfree(void *ptr)
{
    if((uintptr_t)ptr <= slab_bound)
    {
        spin_lock(&slab_lock);
        ret_the_slab(ptr);
        spin_unlock(&slab_lock);
    }
    else if((uintptr_t)ptr <= buddy_bound)
    {
        spin_lock(&buddy_lock);
        ret_the_buddy(ptr);
        spin_unlock(&buddy_lock);
    }
    else    panic("Kfree a wrong address\n");
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
