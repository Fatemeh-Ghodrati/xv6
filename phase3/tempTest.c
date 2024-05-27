#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"

#define NUM_CHILDREN 7

int main(int argc, char *argv[])
{
    int priority = -1;
    int status;
    for (int i = 0; i < NUM_CHILDREN; i++)
    {
        if (fork() == 0)
        {
            int pid = getpid();
            priority = (i / 5) + 1;
            set_priority(priority);
            for (int j = 1; j < 1000000000 ; ++j) {}
            printf("process %d finished\n", pid);
            
            exit(0);
        }
       
    }
    while(wait(&status) != -1);
    exit(0);
}