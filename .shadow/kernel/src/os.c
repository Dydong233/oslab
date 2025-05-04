#include <common.h>

static void os_init() {
    pmm->init();
}

static void os_run() {
    for (const char *s = "Hello World from CPU #*\n"; *s; s++) {
        putch(*s == '*' ? '0' + cpu_current() : *s);
    }
    // int cnt = 0;
    while (1){
        // if(cnt == 10)   break;
        // printf("This is the %d round of testing\n",++cnt);
        // test0();
        // test1();
        // test2();
    };
}

MODULE_DEF(os) = {
    .init = os_init,
    .run  = os_run,
};
