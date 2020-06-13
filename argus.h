#ifndef __ARGUS__
#define __ARGUS__


#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "def.c"
#include "aux.c"
//#include "def.h"


#define MAX 1024

//cliente
void helpGuide();
int parseCommand(char *com);
char* parseLinha(char* args[]);

//servidor
void removeTask(int pid);
int numCurrentTasks();
void alarm_handler(int signum);
void sigchild_handler(int signum);
void executeTask(int fd, Task t);
void listTasks(int fd);
void endTask(int task);
void history(int fd);
void logs(int fdW, int task);

#endif