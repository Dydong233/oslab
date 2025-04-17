#include "co.h"
#include <stdlib.h>
#include <stdint.h> 
#include <stdio.h>
#include <string.h>

#define STACK_SIZE 1024*64
#define MAX_CO 1<<10

#ifdef LOCAL_MACHINE
    #define debug(...) printf(__VA_ARGS__)
#else
    #define debug(...)
#endif

struct co *current = NULL;
struct co *co_list[MAX_CO];
int co_count = 0;

enum co_status {
    CO_NEW = 1, // create a co
    CO_RUNNING, // running
    CO_WAITING, // on wait
    CO_DEAD,    // has finished
};

struct context {
    uint64_t rsp;
    uint64_t rip;
    uint64_t rbx;
    uint64_t rbp;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
};

struct co {
    char *name; // co's name
    void (*func)(void *); // co's entry place
    void *arg;  // co's arg

    enum co_status status;  // co's state
    struct co *    waiter;  // other waiters
    struct context context; // save co's regs
    uint8_t stack[STACK_SIZE];  // co's stack point
};

void co_trampoline()
{
    current->status = CO_RUNNING;
    current->func(current->arg);
    current->status = CO_DEAD;
    co_yield();
}

struct co *co_start(const char *name, void (*func)(void *), void *arg) {
    // init the new_co
    struct co *new_co = malloc(sizeof(struct co));
    memset(new_co,0,sizeof(struct co));
    new_co->name = strdup(name);
    new_co->func = func;
    new_co->arg = arg;
    new_co->status = CO_NEW;
    
    // calculate the stack point
    uintptr_t sp = (uintptr_t)(new_co->stack+STACK_SIZE);
    sp = sp-(sp % 16);
    printf("This co's stack point is: %p to %d\n",new_co->stack,sp);

    // push the co_trampoline's frame
    sp -= sizeof(void*);
    *(void**)sp = (void*)co_trampoline;
    new_co->context.rsp = sp;
    new_co->context.rip = (uint64_t)co_trampoline;
    co_list[co_count++] = new_co;

    return new_co;
}

void co_wait(struct co *co) {
}

void co_yield() {
}
