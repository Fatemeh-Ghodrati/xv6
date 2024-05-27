#include "kernel/types.h"
#include "kernel/param.h"
#include "kernel/spinlock.h"
#include "kernel/riscv.h"
#include "kernel/proc.h"
#include "user.h"

char * enumToString(enum procstate state){
    char* enumString = "";
    if(state == UNUSED){
        enumString = "unused";
    }
    else if(state == USED){
        enumString = "used";
    }
    else if(state == SLEEPING){
        enumString = "sleeping";
    }
    else if(state == RUNNABLE){
        enumString = "runable";
    }
    else if(state == RUNNING){
        enumString = "running";
    }
    else if(state == ZOMBIE){
        enumString = "zombie";
    }
    return enumString;
}


int main(int argc, char *argv[]){
    struct top topStruct;
    top(&topStruct);
    printf("uptime:%d seconds\n", topStruct.uptime);
    printf("total process:%d\n", topStruct.total_process);
    printf("running process:%d\n", topStruct.running_process);
    printf("sleeping process:%d\n", topStruct.sleeping_process);
    printf("process data:\n");
    printf("name\t\tPID\t\tPPID\t\tstate\n");

    for(int i = 0; i < topStruct.total_process; i++){
        if(topStruct.p_list[i].pid != 0){
            char * state = enumToString(topStruct.p_list[i].state);
            printf("%s\t\t%d\t\t%d\t\t%s\n", topStruct.p_list[i].name, topStruct.p_list[i].pid,
                topStruct.p_list[i].ppid, state);
        }
    }
    return 0;
}
