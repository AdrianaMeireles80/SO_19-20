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