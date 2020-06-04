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

 
void removeCurrentTask(int pid){
    int i = 0;
    int j;
    while(currentTasks[i] != NULL && currentTasks[i]->pid != pid)
        i++;
    for(j = i; currentTasks[j] != NULL && currentTasks[j+1] != NULL; j++)
        currentTasks[j] = currentTasks[j+1];
    currentTasks[j] = NULL;
    indexCur--;
}


void executeTask(int fd, Task t) {
    int f, status, n;
    char answer[100], out[MAX];


    n = sprintf(answer, "Tarefa #%d\n", t->num);
    write(fd,&answer, n);
    
    int pos = numCurrentTasks();
    t->pid = getpid();
    currentTasks[pos] = newTask(t->num, t->commands, t->pid);
    //indexCur++;

    f = fork();
    switch (f){
    case -1 :
        perror("fork");
        break;
    case 0:
        printf("Processo-filho com pid %d a executar tarefa %d\n", getpid(), t->num);
        
        printf("%d\n", pos);
     
        printf("NA FILA: tarefa %d a executar %s pelo processo %d\n", currentTasks[pos]->num,
         currentTasks[pos]->commands, currentTasks[pos]->pid);
        printf("%d tarefas na fila\n", numCurrentTasks());
        
        if(maxExecution != -1)
            alarm(maxExecution); //se ao fim deste tempo o processo ainda estiver a executar ao receber o sinal o processo da exit

        int ret = mysystem(t->commands);
      
        _exit(ret);
    default:

        waitpid(f, &status, 0);
       // wait(&status);
        int hist_fd = open("historico.txt", O_CREAT | O_APPEND | O_WRONLY, 0666);
        if(WIFEXITED(status)){
            bzero(out, sizeof(out));
            if(WEXITSTATUS(status) > 0)
                n = sprintf(out, "#%d, max execução : %s\n", t->num, t->commands);
            else 
                n = sprintf(out, "#%d, concluida : %s\n", t->num, t->commands);

            write(hist_fd, &out, n);
            removeCurrentTask(t->pid);

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
      if(i==0){
        n = sprintf(ret,"Não há tarefas a executar\n");
       
      }
      lseek(fd,SEEK_END,0);
      write(fd,&ret,n);
     
}

//so da para testar quando o servidor estiver em paralelo
void endTask(int task){
    //kill da task
    int i,fd;
    fd = open("historico.txt",O_WRONLY);

     for(i=0;currentTasks[i] != NULL;i++){
        if(task==currentTasks[i]->num){
            kill(currentTasks[i]->pid,SIGKILL);
            write(fd,"./argus -t\n",11);
        }
    }
}

void history(int fd){
    //Ler do ficheiro do historicoTasks[i]
    char buf[MAX];
    int r=0;

    int fd2 = open("historico.txt",O_RDONLY);
    lseek(fd2,SEEK_SET,0);//começar a ler a partir do offset q é 0
    bzero(buf,MAX);
        
    while((r = read(fd2,&buf,sizeof(buf)))>0){
       
        write(fd,&buf,r);
       
        bzero(buf,MAX);
    }
}

int main(int argc, char *argv[]){
    int f, r, fd_fifoR, fd_fifoW, status;
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
        bzero(buf,MAX);
        if((r = readLine(fd_fifoR, buf, sizeof(buf))) > 0){
            buf[r] = '\0';
            command = strdup(buf);
            printf("LEU DO PIPE: %s\n", command);
            int i = 0;
            while((token = strsep(&command, " ")) != NULL){
                tokens[i] = token;
                i++;
            }
            //switch(fork()){
              //  case -1:
                //    perror("fork");
                  // break;

                //case 0:

                    if(!strcmp(tokens[0], "tempo-inactividade")){
                        maxInactivity = atoi(tokens[1]);
                        lseek(fd_fifoW,SEEK_END,0);
                        write(fd_fifoW, "Inatividade definida\n", 21);
                        
                    }
                    else if (!strcmp(tokens[0], "tempo-execucao")){
                        maxExecution = atoi(tokens[1]);
                        lseek(fd_fifoW,SEEK_END,0);
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
                        lseek(fd_fifoW,SEEK_END,0);
                        write(fd_fifoW, "Terminar tarefa\n", 17);
                        endTask(task);
                       
                    }
                    else if (!strcmp(tokens[0], "historico")){
                        printf("HISTORICO DE TAREFAS\n");
                        history(fd_fifoW);
                       
                    }

                    //printf("PUTAS E VINHO VERDE!\n");
                   // break;


                //default:

                    //break;

 //            }
          
        }
    }

    free(*currentTasks);
    close(fd_fifoR);
    close(fd_fifoW);
    return 0;

}
