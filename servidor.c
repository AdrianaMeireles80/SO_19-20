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

#define MAX 1024

int maxInactivity = -1;
int maxExecution = -1;
int indexCur = 0;
Task *currentTasks; //Array de tasks a executar


void alarm_handler(int signum){
    _exit(1);
}

int numCurrentTasks(){
    int n = 0;
    for(int i = 0; currentTasks[i] != NULL; i++)
        n++;

    return n;
}

/* parece nao ser necessario ja que depois de executar a tarefa sai do array ao fazer _exit
void removeCurrentTask(int pid){
    int i = 0;
    while(currentTasks[i] != NULL && currentTasks[i]->pid != pid)
        i++;
    for(int j = i; currentTasks[j] != NULL && currentTasks[j+1] != NULL; j++)
        currentTasks[j] = currentTasks[j+1];
    indexCur--;
}*/


void executeTask(int fd, Task t) {
    int f, status, n;
    char answer[100], out[MAX];


    n = sprintf(answer, "Tarefa #%d\n", t->num);
    write(fd,&answer, n);
    f = fork();

    switch (f){
    case -1 :
        perror("fork");
        break;
    case 0:
        printf("Processo-filho com pid %d a executar tarefa %d\n", getpid(), t->num);
        t->pid = getpid();
        int pos = numCurrentTasks();
        printf("%d\n", pos);
        currentTasks[indexCur] = newTask(t->num, t->commands, t->pid);
        indexCur++;
        printf("NA FILA: tarefa %d a executar %s pelo processo %d\n", currentTasks[pos]->num,
         currentTasks[pos]->commands, currentTasks[pos]->pid);
        printf("%d tarefas na fila\n", numCurrentTasks());
        
        if(maxExecution != -1)
            alarm(maxExecution); //se ao fim deste tempo o processo ainda estiver a executar ao receber o sinal o processo da exit

        int ret = mysystem(t->commands);
        indexCur--; //ao fazer exit dps de executar os comandos na mysystem a tarefa sai do array nao sei pq
                    //entao temos de diminuir o index, mas n sei se ta correto isto
        _exit(ret);
    default:
        waitpid(f, &status, 0);
        int hist_fd = open("historico.txt", O_CREAT | O_APPEND | O_WRONLY, 0666);
        if(WIFEXITED(status)){
            bzero(out, sizeof(out));
            if(WEXITSTATUS(status) > 0)
                n = sprintf(out, "#%d, max execução : %s\n", t->num, t->commands);
            else 
                n = sprintf(out, "#%d, concluida : %s\n", t->num, t->commands);

            write(hist_fd, &out, n);
        }
        else {
            bzero(out, sizeof(out));
            int n = sprintf(out, "#%d, terminada por cliente : %s\n", t->num, t->commands);
            write(hist_fd, &out, n);
        }
        close(hist_fd);
        break;
    }
}

void listTasks(int fd){
    //Ler as currentTasks e imprimir para o fifo
    int i,n=0;

    char aux[MAX],ret[MAX];

    bzero(ret,MAX);
    
    for(i=0;currentTasks[i] != NULL;i++){
               
        bzero(aux,MAX);

        n += sprintf(aux,"#%d: %s\n",currentTasks[i]->num,currentTasks[i]->commands);

        strcat(ret,aux);
    }

      write(fd,&ret,n);
     
}

void endTask(int task){
    //kill da task
}

void history(int fd){
    //Ler do ficheiro do historico
    write(fd,"Historico", 9);
}

int main(int argc, char *argv[]){
    int f, r, fd_fifoR, fd_fifoW;
    char buf[MAX];
    char *token, *command;
    char *tokens[20];
    int numTask = 0;
    currentTasks = malloc(sizeof(Task) * 20);

    signal(SIGALRM, alarm_handler);


    //Criação dos fifos
    //Fifo usado pelo servidor para ler o que vem do cliente 
    if (mkfifo("fifo1", 0666) == -1){
        perror("mkfifo");
    }
    //Fifo usado pelo servidor para enviar respostas para o cliente 
    if (mkfifo("fifo2", 0666) == -1){
        perror("mkfifo");
    }

    //Abertura dos fifos
    fd_fifoR = open("fifo1", O_RDONLY);
    fd_fifoW = open("fifo2", O_WRONLY);

    if(fd_fifoR == -1 || fd_fifoW == -1){
        perror("open fifo");
        exit(-1);
    }

    while(1){
        if((r = readLine(fd_fifoR, buf, sizeof(buf))) > 0){
            buf[r] = '\0';
            command = strdup(buf);
            printf("LEU DO PIPE: %s\n", command);
            int i = 0;
            while((token = strsep(&command, " ")) != NULL){
                tokens[i] = token;
                i++;
            }

            if(!strcmp(tokens[0], "tempo-inactividade")){
                maxInactivity = atoi(tokens[1]);
                write(fd_fifoW, "Inatividade definida\n", 21);
            }
            else if (!strcmp(tokens[0], "tempo-execucao")){
                maxExecution = atoi(tokens[1]);
                write(fd_fifoW, "Execucao max definida\n", 22);
            }
            else if (!strcmp(tokens[0], "executar")){
                int pos = strlen(tokens[0]) + 2;
                char *task = strdup(buf + pos);
                task[strlen(task)-1] = '\0';
                printf("EXECUTAR TAREFA: %s\n", task);
                Task t = newTask(numTask, task, -1);
                numTask++;
                executeTask(fd_fifoW, t);
            }
            else if (!strcmp(tokens[0], "listar")){
                printf("LISTAR TAREFAS\n");
                listTasks(fd_fifoW);
            }
            else if (!strcmp(tokens[0], "terminar")){
                int task = atoi(tokens[1]);
                printf("TERMINAR TAREFA %d\n", task);
                write(fd_fifoW, "Terminar tarefa\n", 17);
                endTask(task);
            }
            else if (!strcmp(tokens[0], "historico")){
                printf("HISTORICO DE TAREFAS\n");
                history(fd_fifoW);
            }
        }
    }

    free(*currentTasks);
    close(fd_fifoR);
    close(fd_fifoW);
    return 0;

}