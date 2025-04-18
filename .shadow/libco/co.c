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

// co's state
enum co_status {
    CO_NEW = 1, // create a co
    CO_RUNNING, // running
    CO_WAITING, // on wait
    CO_DEAD,    // has finished
};

struct context {
    uintptr_t rsp;
    uintptr_t rip;
    uintptr_t rbx;
    uintptr_t rbp;
    uintptr_t r12;
    uintptr_t r13;
    uintptr_t r14;
    uintptr_t r15;
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
    new_co->waiter = NULL;
    
    // calculate the stack point
    uintptr_t sp = (uintptr_t)(new_co->stack+STACK_SIZE);
    sp = sp-(sp % 16);

    // push the co_trampoline's frame
    sp -= sizeof(void*);
    *(void**)sp = (void*)co_trampoline;
    new_co->context.rsp = sp;
    new_co->context.rip = (uintptr_t)co_trampoline;
    co_list[co_count++] = new_co;

    return new_co;
}

void co_wait(struct co *co) {

}

__attribute__((noinline))
int setjmp(struct context *ctx)
{
#if __x86_64__
    asm volatile(
        // save the rsp&rip&other regs
        "movq %%rsp, 0(%0)\n\t"
        "leaq 1f(%%rip), %%rax\n\t"
        "movq %%rax, 8(%0)\n\t"
        "movq %%rbx, 16(%0)\n\t"
        "movq %%rbp, 24(%0)\n\t"
        "movq %%r12, 32(%0)\n\t"
        "movq %%r13, 40(%0)\n\t"
        "movq %%r14, 48(%0)\n\t"
        "movq %%r15, 56(%0)\n\t"
        "xor %%eax, %%eax\n\t"
        "1:\n\t"
        : /* no output */
        : "r"(ctx)
        : "memory", "rax"
    );
#else
    asm volatile(
        "movl %%esp, 0(%0)\n\t"
        "call 1f\n\t"
        "1:\n\t"
        "popl %%eax\n\t"
        "movl %%eax, 4(%0)\n\t"
        "movl %%ebx, 8(%0)\n\t"
        "movl %%ebp, 12(%0)\n\t"
        "movl %%esi, 16(%0)\n\t"
        "movl %%edi, 20(%0)\n\t"
        "xorl %%eax, %%eax\n\t"
        :
        : "r"(ctx)
        : "memory", "eax"
    );
#endif
    return 0;
}
__attribute__((noinline))
void longjmp(struct context *ctx)
{
#if __x86_64__
    asm volatile(
         "movq 16(%0), %%rbx\n\t"
         "movq 24(%0), %%rbp\n\t"
         "movq 32(%0), %%r12\n\t"
         "movq 40(%0), %%r13\n\t"
         "movq 48(%0), %%r14\n\t"
         "movq 56(%0), %%r15\n\t"
         "movq 0(%0), %%rsp\n\t"
         "jmp *8(%0)\n\t"
         :
         : "r"(ctx)
         : "memory"
    );
#else
    asm volatile(
        "movl 8(%0), %%ebx\n\t"
        "movl 12(%0), %%ebp\n\t"
        "movl 16(%0), %%esi\n\t"
        "movl 20(%0), %%edi\n\t"
        "movl 0(%0), %%esp\n\t"
        "jmp *4(%0)\n\t"
        :
        : "r"(ctx)
        : "memory"
    );
#endif
    __builtin_unreachable(); // never return
}
void co_yield() {
    if(setjmp(&current->context) == 0) {
        if(current->status != CO_DEAD) {
            current->status = CO_WAITING;
        }
    }
    // not rand co
    struct co *next = NULL;
    for(int i = 0; i < co_count; i++) {
        if(co_list[i]->status == CO_NEW || co_list[i]->status == CO_WAITING) {
            next = co_list[i];
            break;
        }
    }
    // if other co's are dead
    if(next == NULL&&current->status == CO_DEAD) {
        printf("There is no co alive\n");
        return;
    }
    else next = current;
    // switch to next co
    current = next;
    current->status = CO_RUNNING;
    longjmp(&current->context);
    return;
}
