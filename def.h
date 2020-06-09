typedef struct{
    int num; //Nr da tarefa
    char * commands; //tarefa a executar
    int pid; //pid que efetua a tarefa
}*Task;

Task newTask(int n, char * com, int pid);