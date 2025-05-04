#include <common.h>

void test0(void)
{
    // printf("start\n");
	void *add1 = pmm->alloc(1020);
	if (add1 == NULL)
	{
		printf("add is NULL");
		assert(0);
	}
	else
		printf("add1: %p", add1);

	int *add1_int=(int *)add1;
	*add1_int=9876;
	*(add1_int+(1020-4)/4)=114514;
	void *add2 = pmm->alloc(20);
	if (add2 == NULL)
	{
		printf("add2 is NULL");
		assert(0);
	}
	else
		printf("add2: %p", add2);
	
	void *add3 = pmm->alloc(512);
	if (add3 == NULL)
	{
		printf("add3 is NULL");
		assert(0);
	}
	else
		printf("add3: %p", add3);
	int *add3_int=(int *)add3;
	*add3_int=543210;
	*(add3_int+(512-4)/4)=114514;
	pmm->free(add2);
    printf("cpu:%d\n",cpu_current());

    // printf("add1:%d\n",*add1_int);
	assert(*add1_int==9876);
	assert(*(add1_int+(1020-4)/4)==114514);
	assert(*add3_int==543210);
	assert(*(add3_int+(512-4)/4)==114514);
	pmm->free(add1);
	assert(*add3_int==543210);
	assert(*(add3_int+(512-4)/4)==114514);
	pmm->free(add3);
}

void test1(void)
{
    // 分配 16KB = 16 * 1024 字节
    void *add16k = pmm->alloc(16 * 1024);
    if (add16k == NULL) {
        printf("add16k is NULL\n");
        assert(0);
    } else {
        printf("add16k: %p\n", add16k);
    }

    int *add16k_int = (int *)add16k;
    *add16k_int = 123456;
    *(add16k_int + ((16 * 1024 - 4) / 4)) = 654321; // 写入最后一个 int

    // 分配 128KB = 128 * 1024 字节
    void *add128k = pmm->alloc(128 * 1024);
    if (add128k == NULL) {
        printf("add128k is NULL\n");
        assert(0);
    } else {
        printf("add128k: %p\n", add128k);
    }

    int *add128k_int = (int *)add128k;
    *add128k_int = 111111;
    *(add128k_int + ((128 * 1024 - 4) / 4)) = 222222;

    // 分配 512KB = 512 * 1024 字节
    void *add512k = pmm->alloc(512 * 1024);
    if (add512k == NULL) {
        printf("add512k is NULL\n");
        assert(0);
    } else {
        printf("add512k: %p\n", add512k);
    }

    int *add512k_int = (int *)add512k;
    *add512k_int = 333333;
    *(add512k_int + ((512 * 1024 - 4) / 4)) = 444444;

    printf("cpu: %d\n", cpu_current());

    // 验证内容
    assert(*add16k_int == 123456);
    assert(*(add16k_int + ((16 * 1024 - 4) / 4)) == 654321);

    assert(*add128k_int == 111111);
    assert(*(add128k_int + ((128 * 1024 - 4) / 4)) == 222222);

    assert(*add512k_int == 333333);
    assert(*(add512k_int + ((512 * 1024 - 4) / 4)) == 444444);

    // 释放并验证数据保留（仅演示是否仍可访问）
    pmm->free(add16k);
    pmm->free(add128k);

    assert(*add512k_int == 333333);
    assert(*(add512k_int + ((512 * 1024 - 4) / 4)) == 444444);

    pmm->free(add512k);

    printf("test0: large block allocator passed\n");
}

void test2(void)
{
    // 分配 64B
    void *add64 = pmm->alloc(64);
    assert(add64 != NULL);
    printf("add64: %p\n", add64);
    int *add64_int = (int *)add64;
    *add64_int = 111;
    *(add64_int + ((64 - 4) / 4)) = 222;

    // 分配 256B
    void *add256 = pmm->alloc(256);
    assert(add256 != NULL);
    printf("add256: %p\n", add256);
    int *add256_int = (int *)add256;
    *add256_int = 333;
    *(add256_int + ((256 - 4) / 4)) = 444;

    // 分配 1024B
    void *add1k = pmm->alloc(1024);
    assert(add1k != NULL);
    printf("add1k: %p\n", add1k);
    int *add1k_int = (int *)add1k;
    *add1k_int = 555;
    *(add1k_int + ((1024 - 4) / 4)) = 666;

    // 分配 16KB
    void *add16k = pmm->alloc(16 * 1024);
    assert(add16k != NULL);
    printf("add16k: %p\n", add16k);
    int *add16k_int = (int *)add16k;
    *add16k_int = 777;
    *(add16k_int + ((16 * 1024 - 4) / 4)) = 888;

    // 分配 256KB
    void *add256k = pmm->alloc(256 * 1024);
    assert(add256k != NULL);
    printf("add256k: %p\n", add256k);
    int *add256k_int = (int *)add256k;
    *add256k_int = 999;
    *(add256k_int + ((256 * 1024 - 4) / 4)) = 101010;

    printf("cpu: %d\n", cpu_current());

    // 验证写入数据
    assert(*add64_int == 111);
    assert(*(add64_int + ((64 - 4) / 4)) == 222);

    assert(*add256_int == 333);
    assert(*(add256_int + ((256 - 4) / 4)) == 444);

    assert(*add1k_int == 555);
    assert(*(add1k_int + ((1024 - 4) / 4)) == 666);

    assert(*add16k_int == 777);
    assert(*(add16k_int + ((16 * 1024 - 4) / 4)) == 888);

    assert(*add256k_int == 999);
    assert(*(add256k_int + ((256 * 1024 - 4) / 4)) == 101010);

    // 释放部分并测试保留内容
    pmm->free(add64);
    pmm->free(add256);
    pmm->free(add1k);

    assert(*add16k_int == 777);
    assert(*(add16k_int + ((16 * 1024 - 4) / 4)) == 888);

    assert(*add256k_int == 999);
    assert(*(add256k_int + ((256 * 1024 - 4) / 4)) == 101010);

    // 最终释放
    pmm->free(add16k);
    pmm->free(add256k);

    printf("stress_test_mixed_alloc: passed\n");
}