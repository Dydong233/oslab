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

void test2(void) //混合的内存申请
{
	void *add = pmm->alloc(4096);
	if (add == NULL)
	{
		printf("add is NULL");
		assert(0);
	}
	else
		printf("add: %p\n", add);
	char *add_char=(char *)add;
	*add_char='a';
	*(add_char+4095)='b';
	void *add1 = pmm->alloc(1024);
	if (add1 == NULL)
	{
		printf("add1 is NULL");
		assert(0);
	}
	else
		printf("add1: %p\n", add1);
	char *add_char1=(char *)add1;
	*add_char1='c';
	*(add_char1+1023)='d';
	assert(*add_char=='a');
	assert(*(add_char+4095)=='b');
	pmm->free(add);
	assert(*add_char1=='c');
	assert(*(add_char1+1023)=='d');
	void *add2 = pmm->alloc(45);
	if (add2 == NULL)
	{
		printf("add2 is NULL");
		assert(0);
	}
	else
		printf("add2: %p\n", add2);
	char *add_char2=(char *)add2;
	*add_char2='e';
	*(add_char2+44)='f';
	pmm->free(add1);
	assert(*add_char2=='e');
	assert(*(add_char2+44)=='f');
	add = pmm->alloc(4096);
	if (add == NULL)
	{
		printf("add is NULL");
		assert(0);
	}
	else
		printf("add: %p\n", add);
	add_char=(char *)add;
	*add_char='g';
	*(add_char+4095)='h';
	pmm->free(add2);
	assert(*add_char=='g');
	assert(*(add_char+4095)=='h');
	pmm->free(add);
}