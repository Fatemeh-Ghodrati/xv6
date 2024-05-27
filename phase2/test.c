#include "kernel/types.h"
#include "user.h"
#include "thread.h"

volatile int a_started, b_started, c_started;
volatile int a_done, b_done, c_done;
volatile int a_n, b_n, c_n;

void
thread_a(void)
{
    int i;
    printf("thread_a started\n");
    a_started = 1;
    while(b_started == 0 || c_started == 0)
        thread_yield();

    for (i = 0; i < 100; i++) {
        printf("thread_a %d\n", i);
        a_n += 1;
        thread_yield();
    }
    printf("thread_a: exit after %d\n", a_n);

    a_done = 1;
    current_thread->state = FREE;
    thread_schedule();
}

void
thread_b(void)
{
    int i;
    printf("thread_b started\n");
    b_started = 1;
    while(a_started == 0 || c_started == 0)
        thread_yield();

    for (i = 0; i < 100; i++) {
        printf("thread_b %d\n", i);
        b_n += 1;
        thread_yield();
    }
    printf("thread_b: exit after %d\n", b_n);

    b_done = 1;
    current_thread->state = FREE;
    thread_schedule();
}

void
thread_c(void)
{
    int i;
    printf("thread_c started\n");
    c_started = 1;
    while(a_started == 0 || b_started == 0)
        thread_yield();

    for (i = 0; i < 100; i++) {
        printf("thread_c %d\n", i);
        c_n += 1;
        thread_yield();
    }
    printf("thread_c: exit after %d\n", c_n);

    c_done = 1;
    current_thread->state = FREE;
    thread_schedule();
}
int main(int argc, char const *argv[]){
    a_started = b_started = c_started = 0;
    a_done = b_done = c_done = 0;
    a_n = b_n = c_n = 0;
    thread_init();
    thread_create(thread_a);
    thread_create(thread_b);
    thread_create(thread_c);
    thread_yield();

    while (!a_done || !b_done || !c_done) {
      thread_yield();
    }
    exit(0);
}