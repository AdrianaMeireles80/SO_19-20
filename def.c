#include <stdlib.h>
#include <string.h>
#include "def.h"

Task newTask(int n, char * com, int pid){
    Task novo = malloc(sizeof(Task));

    novo->num = n;
    novo->commands = strdup(com);
    novo->pid = pid;
    return novo;
}