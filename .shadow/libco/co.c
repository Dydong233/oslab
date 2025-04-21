#include "co.h"
#include <stdlib.h>
#include <stdint.h> 
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <setjmp.h>

#define STACK_SIZE 1024*64
#define MAX_CO 1<<10

#ifdef LOCAL_MACHINE
    #define debug(...) printf(__VA_ARGS__)
#else
    #define debug(...)
#endif

struct co *current = NULL;
struct co *co_list[MAX_CO];
int co_count = 0,alive_co = 0;

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
    jmp_buf context; // save co's regs
    uint8_t stack[STACK_SIZE];  // co's stack point
};

void co_trampoline()
{
    if(current->status == CO_DEAD) return;
    current->status = CO_RUNNING;
    current->func(current->arg);
    current->status = CO_DEAD;
    alive_co--;
    co_yield();
}

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
    }
    return new_co;
}

void co_wait(struct co *co) {
    // situation 1: co is dead
    if(co->status == CO_DEAD) {
        printf("co %s is dead\n", co->name);
        free(co);
        return;
    }
    // situation 2: co is running
    while(co->status != CO_DEAD) {
        co_yield();
    }
    free(co);
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
    if(!alive_co)   return;

    // save current co's context(This co must be running!)
    if(current != NULL){
        if(setjmp(&current->context) == 0) {
            if(current->status != CO_DEAD)    current->status = CO_WAITING;
        }
    }

    // find the next co
    struct co *next = NULL;
    int rand_co;
    while(233) {
        // The first situation : current is NULL
        // The second situation : There is only one co
        rand_co = rand() % co_count;
        if(co_list[rand_co]->status != CO_DEAD) {
            next = co_list[rand_co];
            // printf("This time choose co %s\n", next->name);
            break;
        }
    }
    // if the next co is new
    if(next->status == CO_NEW){
        next->status = CO_RUNNING;
        
    }


    // switch to next co
    if(next!=NULL)  current = next;
    current->status = CO_RUNNING;
    longjmp(&current->context);
    return;
}
