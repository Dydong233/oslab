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

}

void co_yield() {
    
}
