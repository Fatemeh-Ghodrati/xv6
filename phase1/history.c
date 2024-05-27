#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"

#define MAX_HISTORY 16
#define INPUT_BUF 128

int
main(int argc, char *argv[])
{
    char buffer[INPUT_BUF];
    char requested_command[INPUT_BUF];
    int return_value = 0;
    for(int i = 0; i < MAX_HISTORY; i++) {
        memset(buffer, 0, 128 * sizeof(char));
    }
    for(int i = 0; i < MAX_HISTORY && (return_value==0 || return_value == 1); i++){

        return_value = history(buffer, i);

        if(return_value == 0 || return_value == 1){
            printf("%s\n", buffer);
        }
        if(return_value == 1){
            strcpy(requested_command, buffer);
        }
    }
    printf("%s %s\n", "requested command:", requested_command);
    return return_value;
}
