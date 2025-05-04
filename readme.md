## 实验描述

### 多分配器的内存申请

AbstractMachine在初始化的时候会使用`[heap.start, heap.end)`标记一段内核可用的内存，需要实现允许多个处理器并发申请或释放内存的分配器。需要满足以下几个条件：

- 原子性：当多个处理器同时请求内存分配或释放时，分配器必须确保同时进行的分配/回收操作能够正确完成。
- 无重叠：分配器返回的内存块之间不能重叠。
- 对齐：对于大小为 s 的内存分配请求，返回的内存地址必须对齐到 $$2^i$$。
- 无内存泄露：要将没用的内存使用kfree进行释放。
- 错误处理：当无法满足内存分配请求时，kalloc返回NULL。

```c
static void *kalloc(size_t size) {
    // TODO
    // You can add more .c files to the repo.
    return NULL;
}
static void kfree(void *ptr) {
    // TODO
    // You can add more .c files to the repo.
}
```

### 性能优化

在kalloc和kfree实现正确的前提下，不允许在系统内存压力很小的时候依然频繁分配失败，不允许代码的性能过低 (例如用全局的锁保护空闲链表，并用遍历链表的方式寻找可用的内存)。但是多处理器上的内存分配存在以下挑战，这两点挑战是矛盾的：

- 希望申请的正确性，意味着用一把大锁保护一个串行数据结构（如线段树），能在多处理器上实现内存分配。
- 希望分配在多处理器上的性能和伸缩性，意味着在不同的处理器能够并行地申请内存。

### 正确性表达

实验维护一个数据结构，维护一个不相交的集合：*H*={[ℓ0,*r*0),[ℓ1,*r*1),…,[ℓ*n*,rn)}，在kalloc申请时候要满足上面提到的几点，返回一个左端点L，在kfree时候删除一个内存区域，当L不是H的任意一个区间的左端点时，产生`undefined behavior`。

同时对于管理堆区的数据结构需要在堆区之中进行分配，不能在静态区分配。

### 测试用例

**测试环境**

实验测试环境有以下四点，同时要满足safety和liveness：

- 不超过 8 个处理器、不少于 64 MiB、不多于 4 GiB 内存的堆区；
- 大量、频繁的小内存分配/释放；其中绝大部分不超过 128 字节；
- 较为频繁的，以物理页面大小为单位的分配/释放 (4 KiB)；
- 非常罕见的大内存分配。

**测试代码**

测试主要分为3个模块：

- 测试一页以下小内存的分配
- 测试一页以上的分配
- 测试混合分配

### 目录文件

```shell
.
├── framework
│   ├── kernel.h
│   └── main.c
├── include
│   ├── common.h
│   ├── mutex.h
│   └── test.h
├── kernel.c
├── Makefile
└── src
    ├── mutex.c	# 实现锁
    ├── os.c
    ├── pmm.c	# 实现kalloc和kfree等一系列操作
    └── test.c	# 测试代码
```

------

## 实验设计

### 编译运行选项

```shell
# 修改abstract-machine/scripts/platform/qemu.mk为多核测试
smp        ?= 4
LDFLAGS    += -N -Ttext-segment=0x00100000
QEMU_FLAGS += -serial mon:stdio \
              -machine accel=tcg \
              -smp "$(smp),cores=1,sockets=$(smp)" \
              -drive format=raw,file=$(IMAGE)
# 编译运行
make run ARCH=x86_64-qemu
```

### 具体设计

**manager_area**

manager_area用于保存一些需要大空间的变量，如map，设计时候选用高地址的区域保存。

```c
struct Manager_area{
    uintptr_t pos;
    uintptr_t start;
    uintptr_t end;
}manager_area;
// init manager_area
// the distance between the start and end address is 20MB
manager_area.start = end_addr-PAGE_SIZE * 256 * 20;
manager_area.pos = manager_area.start;
manager_area.end = end_addr;
```

**Slab区域**

把多个页分配给slab使用，从32B到4KB，按照$$2^i$$形式增长。同时访问的时候用一把`slab_lock`锁保存起来。

```c
#define PAGE_SIZE 4096
#define Slab_num 1024
spinlock_t slab_lock = spin_init("slab_lock");
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
void init_memory()
{
    ...
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
    ...
}
```

**Buddy区域**

同slab区域，用于保存8KB到1MB的大小。

**Kalloc**

首先对申请的大小进行对齐，然后根据申请的大小来选择从slab还是buddy区域获取空间。

```c
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
```

**Kfree**

Kfree相对简单，在实现上直接计算出相应的下标，然后归还内存。

```c
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
```

------





















​		
