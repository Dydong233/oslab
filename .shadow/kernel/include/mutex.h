#define UNLOCKED  0
#define LOCKED    1

typedef struct {
    const char *name;
    int status;
} spinlock_t;

#define spin_init(name_) \
    ((spinlock_t) { \
        .name = name_, \
        .status = UNLOCKED, \
    })
void spin_lock(spinlock_t *lk);
void spin_unlock(spinlock_t *lk);
