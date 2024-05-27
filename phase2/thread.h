#define FREE 0x0
#define RUNNING 0x1
#define RUNNABLE 0x2
#define STACK_SIZE 8192
#define MAX_THREAD 4
#define QUANTUM 10

struct registers {
  uint64 ra;
  uint64 sp;

  // callee-saved*
  uint64 s0;
  uint64 s1;
  uint64 s2;
  uint64 s3;
  uint64 s4;
  uint64 s5;
  uint64 s6;
  uint64 s7;
  uint64 s8;
  uint64 s9;
  uint64 s10;
  uint64 s11;
};

typedef struct thread thread_t, *thread_p;

struct thread {
  char stack[STACK_SIZE];       // the thread's stack 
  int        state;             // running, runnable 
  char * name;
  struct registers registers; // Register of each thread
  int ticks; // Number of ticks this thread has executed
};

extern thread_t all_thread[MAX_THREAD];

extern thread_p  current_thread;
extern thread_p  next_thread;

void thread_init(void);
void thread_create(void (*func)());
void thread_schedule(void);
void thread_yield(void);