struct bakery_lock_t {
    struct semaphore* sem;
    spinlock_t *splock;
    unsigned int dispenser;
    volatile unsigned int display;
};
typedef struct bakery_lock_t bakery_lock_t;

// Part A
void bakery_init (bakery_lock_t* b)
{
    if (b == NULL) {
        return -1;
    }

    sema_init(b->sem, 1);   // ?
    
    b->display = 1;
    b->dispenser = 1;
}

// Part B
void bakery_lock (bakery_lock_t* b)
{
    unsigned int my_dispenser;
    
}

// Part C



// Part D