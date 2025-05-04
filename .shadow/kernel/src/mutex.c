#include <common.h>

void spin_lock(spinlock_t *lk) {
    int got;
    do {
        got = atomic_xchg(&lk->status, LOCKED);
    } while (got != UNLOCKED);
}

void spin_unlock(spinlock_t *lk) {
    atomic_xchg(&lk->status, UNLOCKED);
}