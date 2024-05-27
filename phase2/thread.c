#include "../kernel/types.h"
#include "../kernel/stat.h"
#include "../user/user.h"
#include "../user/thread.h"

thread_t all_thread[MAX_THREAD]; // An array for all threads

thread_p  current_thread; // Current thread that is currently running
thread_p  next_thread; // Next thread that would be changed

extern void thread_switch(struct registers*, struct registers*); // Call thread_switch from thread_switch.S

void thread_init(void){
    for (int i = 0; i < MAX_THREAD; i++){
        all_thread[i].state = FREE;
    }
    current_thread = &all_thread[0];
    current_thread->state = RUNNING;
    current_thread->ticks = 0;
}

void thread_create(void (*func)()){
    thread_p t;

    for (t = all_thread; t < all_thread + MAX_THREAD; t++){
        if(t->state == FREE){
            break;
        }
    }
    t->state = RUNNABLE;
    memset(&t->registers, 0, sizeof t->registers);
    t->registers.ra = (uint64)func;

    t->registers.sp = (uint64)(t->stack + STACK_SIZE);
    t->ticks = 0;
}

void thread_schedule(void) {
    thread_p t;
    next_thread = 0;
    t = current_thread + 1;
    for(int i = 0; i < MAX_THREAD; i++){
        if(t >= all_thread + MAX_THREAD)
            t = all_thread;
        if(t->state == RUNNABLE) {
            next_thread = t;
            break;
        }
        t = t + 1;
    }
    if (next_thread == 0) {
        printf("thread_schedule: no runnable threads\n");
        exit(-1);
    }
    if (current_thread != next_thread) {
        t = current_thread;
        int current_time = uptime();
        if(current_time - t->ticks >= QUANTUM){
            next_thread->state = RUNNING;
            next_thread->ticks = uptime();
            current_thread = next_thread;
            thread_switch(&t->registers, &next_thread->registers);
        } else {
            current_thread->state = RUNNING;
        }
    }else{
        current_thread->state = RUNNING;
    }
}
void thread_yield(void) {
    current_thread->state = RUNNABLE;
    thread_schedule();
}