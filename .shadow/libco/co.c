#include "co.h"
#include <stdlib.h>
#include <stdint.h> 
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <setjmp.h>
#include <assert.h>

#define STACK_SIZE 4 * 1024 * 8

#ifdef LOCAL_MACHINE
    #define debug(...) printf(__VA_ARGS__)
#else
    #define debug(...)
#endif

struct co *current = NULL;

// co's state
enum co_status {
    CO_NEW = 1, // create a co
    CO_RUNNING = 2, // running
    CO_WAITING = 3, // on wait
    CO_DEAD = 4,    // has finished
};

struct co {
    char *name; // co's name
    void (*func)(void *); // co's entry place
    void *arg;  // co's arg

    struct co *next;
    enum co_status status;  // co's state
    struct co *    waiter;  // other waiters
    jmp_buf context; // save co's reg
    uint8_t stack[STACK_SIZE];  // co's stack point
};

struct co *co_start(const char *name, void (*func)(void *), void *arg) {
    struct co *new_co = (struct co *)malloc(sizeof(struct co));
    memset(new_co, 0, sizeof(struct co));
    new_co->name = strdup(name);
    new_co->func = func;
    new_co->arg = arg;
    new_co->status = CO_NEW;
    // init main
    if(current == NULL)
    {
        current = (struct co *)malloc(sizeof(struct co));
        memset(current, 0, sizeof(struct co));
        current->name = strdup("main");
        current->status = CO_RUNNING;
        current->waiter = NULL;
        current->next = current;
    }
    // insert to the list
    struct co *h = current;
    while(h){
        if(h->next == current){
            h->next = new_co;
            new_co->next = current;
            break;
        }
        h = h->next;
    }
    return new_co;
}

void co_wait(struct co *co) {
    current->status = CO_WAITING;
    co->waiter = current;
    while(co->status != CO_DEAD)    co_yield();
    current->status = CO_RUNNING;
    // delete co from the list
    struct co *h = current;
    while(h){
        if(h->next == co){
            h->next = co->next;
            break;
        }
        h = h->next;
    }
    free(co);
}

void stack_switch_call(void *sp, void *entry, uintptr_t arg) {
    asm volatile(
#if __x86_64__
        "movq %%rsp,-0x10(%0); leaq -0x20(%0),%%rsp; movq %2,%%rdi; call *%1; movq -0x10(%0) ,%%rsp;"
        :
		: "b"((uintptr_t)sp), "d"(entry), "a"(arg)
		: "memory"
#else
        "movl %%esp, -0x8(%0); leal -0xC(%0), %%esp; movl %2, -0xC(%0); call *%1;movl -0x8(%0), %%esp"
        :
        : "b"((uintptr_t)sp), "d"(entry), "a"(arg)
        : "memory"
#endif
    );
}

void co_yield ()
{
	if (current == NULL) // init main
	{
		current = (struct co *)malloc(sizeof(struct co));
		current->status = CO_RUNNING;
		strcpy(current->name, "main");
		current->next = current;
	}
	assert(current);
	int val = setjmp(current->context);
	if (val == 0) // co_yield() 被调用时，setjmp 保存寄存器现场后会立即返回 0，此时我们需要选择下一个待运行的协程 (相当于修改 current)，并切换到这个协程运行。
	{
		// choose co_next
		struct co *co_next = current;
		do
		{
			co_next = co_next->next;
		} while (co_next->status == CO_DEAD || co_next->status == CO_WAITING);
		current = co_next;
		if (co_next->status == CO_NEW)
		{
			assert(co_next->status == CO_NEW);
			((struct co volatile *)current)->status = CO_RUNNING; //  fogot!!!
			stack_switch_call(&current->stack[STACK_SIZE], current->func, (uintptr_t)current->arg);
			((struct co volatile *)current)->status = CO_DEAD;
			if (current->waiter)
				current = current->waiter;
		}
		else
		{
			longjmp(current->context, 1);
		}
	}
	else // longjmp returned(1) ,don't need to do anything
	{
		return;
	}
}
