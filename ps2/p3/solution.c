typedef struct martian_english_message_lock {
    // The type of synchronization primitive you may use is {\fix spinlock\_t}.
    // You may add up to 3 elements to this struct.
    volatile unsigned int eng_num, mar_num; // The number of translated messages, max eng = 10, max mar = 4
    spinlock_t *splock;
} me_lock;

// Melon Input: input english to Martian
void melon_input(me_lock* lock, msg* message) {
    unsigned long flags;
    int not_finish_bool = 1; // 1 means not finish yet 

    while(not_finish_bool){ // Like polling to the queue/lock
        if (lock->mar_num < 4 && lock->eng_num == 0) {
            // Status Changes
            // if mar queue is available but also no englist message to listen
            spin_lock_irqsave(lock->splock, flags); // Try get lock
                // Check again if condition valid in the gap between
                if (lock->mar_num < 4 && lock->eng_num == 0){
                    translate_to_martian(message);
                    lock->mar_num ++;
                    not_finish_bool = 0;
                }
            spin_unlcok_irqrestore(lock->splock, flags);
        }
        // Otherwise, listen or wait input queue available
    }
    return ;
}

void martian_input(me_lock* lock, msg* message) {
    unsigned long flags;
    int not_finish_bool = 1; // 1 means not finish yet 

    while(not_finish_bool){ // Like polling to the queue/lock
        if (lock->eng_num < 10) {
            // Status Changes
            // if eng queue is available
            spin_lock_irqsave(lock->splock, flags); // Try get lock
                // Check again if condition valid in the gap between
                if (lock->eng_num < 10){
                    translate_to_english(message);
                    lock->eng_num ++;
                    not_finish_bool = 0;
                }
            spin_unlcok_irqrestore(lock->splock, flags);
        }
        // Otherwise,  wait input queue available
    }
    return ;
}

int melon_get_output(me_lock* lock, msg* message) {
    unsigned long flags;
    spin_lock_irqsave(lock->splock, flags); // Try get lock
    if (lock->eng_num > 0){
        get_translation_in_english(message);
        lock->eng_num --;
        spin_unlcok_irqrestore(lock->splock, flags);
        return 0;
    }
    // Output queue is empty
    spin_unlcok_irqrestore(lock->splock, flags);
    return -1;
}

int martian_get_output(me_lock* lock, msg* message) {
    unsigned long flags;
    spin_lock_irqsave(lock->splock, flags); // Try get lock
    if (lock->mar_num > 0){
        get_translation_in_martian(message);
        lock->mar_num --;
        spin_unlcok_irqrestore(lock->splock, flags);
        return 0;
    }
    // Output queue is empty
    spin_unlcok_irqrestore(lock->splock, flags);
    return -1;
}

/* 
Use these four routines to interface to the translation hardware and
queueing system.  Note that the translate_to routines do not check for
full output queues, nor do the get_transation routines check for 
empty queues.  None of the four routines should be called simultaneously
with any others (including themselves).

You do not need to define these functions, but you need to call them in 
your synchronization interface.
*/

/* 
 * translate_to_english
 *   DESCRIPTION: translate the message in the buffer to English;
 *      then put the message in the corresponding queue.
 *   INPUTS: message - a pointer to the input message buffer.
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECT: none
 */
void translate_to_english(msg* message);

/* 
 * translate_to_martian
 *   DESCRIPTION: translate the message in the buffer to Martian;
 *      then put the message in the corresponding queue.
 *   INPUTS: message - a pointer to the input message buffer.
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECT: none
 */
void translate_to_martian(msg* message);

/* 
 * get_translation_in_english
 *   DESCRIPTION: get a translated message in English.
 *   INPUTS: message - a pointer to the message buffer which will
 *      be filled in with a translated English message.
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECT: Will fill in the given message buffer.
 */
void get_translation_in_english(msg* message);

/* 
 * get_translation_in_martian
 *   DESCRIPTION: get a translated message in Martian.
 *   INPUTS: message - a pointer to the message buffer which will
 *      be filled in with a translated Martian message.
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECT: Will fill in the given message buffer.
 */
void get_translation_in_martian(msg* message);
