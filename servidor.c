#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "def.c"

#define MAX 1024

int maxInactivity = -1;
int maxExecution = -1;
Task currentTasks[20]; //Array de tasks a executar
char **endedTasks; // seque escreve se num ficheiro e n é preciso este array

int readLine(int fd, char* buf, int tam){
	int j = 0;
	while(j < tam && read(fd,buf+j,1) > 0 && buf[j] != '\n')
		j++;

	if(j >= tam)	
        buf[j] = '\0';
	else buf[j+1] = '\0';

	return j;
}

int numCurrentTasks(){
    int n = 0;
    for(int i = 0; currentTasks[i] != NULL; i++)
        n++;

    return n;
}

void removeCurrentTask(int pid){
    int i = 0;
    while(currentTasks[i]->pid != pid)
        i++;
    for(int j = i; currentTasks[j] != NULL && currentTasks[j+1] != NULL; j++)
        currentTasks[j] = currentTasks[j+1];
}

void executeTask(int fd, Task t) {
    int f,status;
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
        currentTasks[pos] = newTask(t->num, t->commands, t->pid);
        printf("NA FILA: tarefa %d a executar %s pelo processo %d\n", currentTasks[pos]->num,
         currentTasks[pos]->commands, currentTasks[pos]->pid);
        printf("%d tarefas na fila\n", numCurrentTasks());
        //Não esta a guardar nao sei porque
        //Execução do comando
        _exit(0);
    default:
        wait(&status);
        //check how it ended bla bla
        break;
    }

    //Tipo a mysystem das aulas
    write(fd,"Executar", 8);
}

void listTasks(int fd){
    //Ler as currentTasks e imprimir para o fifo
    write(fd,"Listar", 6);
}

void endTask(int task){
    //kill da task, remover das current para as ended
}

void history(int fd){
    //Ler as endedTasks e imprimir para o fifo
    //guardar o historico em ficheiro?
    write(fd,"Historico", 9);
}

int main(int argc, char *argv[]){
    int f, r, fd_fifoR, fd_fifoW;
    char buf[MAX];
    char *token, *command;
    char *tokens[20];
    int numTask = 0;
    *currentTasks = malloc(sizeof(Task) * 20); //isto cria uma na 1a posiçao
    printf("%d tarefas\n", numCurrentTasks()); 

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
    while((r = readLine(fd_fifoR, buf, sizeof(buf))) > 0){
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

    free(*currentTasks);
    close(fd_fifoR);
    close(fd_fifoW);
    return 0;

}