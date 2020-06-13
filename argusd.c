#include "argus.h"

int maxInactivity = -1;
int maxExecution = -1;
int indexCur = 0;
Task *currentTasks;

//Função que remove tarefa do array de tarefas em execução
void removeTask(int pid){
    int i = 0;
    int j;

    while(currentTasks[i] != NULL && currentTasks[i]->pid != pid){
        i++;
    }
    if(currentTasks[i] != NULL){
        for(j = i; currentTasks[j] != NULL && currentTasks[j+1] != NULL; j++)
        currentTasks[j] = currentTasks[j+1];
        currentTasks[j] = NULL;
        indexCur--;
    }
}

//Função que conta o número de tarefas em execução
int numCurrentTasks(){
    int n = 0;
    for(int i = 0; currentTasks[i] != NULL; i++)
        n++;

    return n;
}

//Handler do alarm
void alarm_handler(int signum){
    int fd, n = 0;
    char buf[MAX];
    fd = open("historico.txt", O_APPEND | O_WRONLY, 0666);

    for(int i = 0; currentTasks[i] != NULL; i++){
        if(currentTasks[i]->pid == getpid()){
            n = sprintf(buf, "#%d, max execução : %s\n", currentTasks[i]->num, currentTasks[i]->commands);
            write(fd, buf, n);
        }
    }
    close(fd);
    _exit(1);
}

//Handler do SIGCHLD
void sigchild_handler(int signum){
    int status, child;
    child = wait(&status);
    
    removeTask(child);
}


//Função que executa uma tarefa
void executeTask(int fd, Task t) {
    int f, n = 0, x = 0;
    char answer[100], out[MAX];


    n = sprintf(answer, "Tarefa #%d\n", t->num);
    write(fd,&answer, n);
    
    int pos = numCurrentTasks();
    currentTasks[pos] = newTask(t->num, t->commands, t->pid);

    f = fork();
    switch (f){
    case -1 :
        perror("fork");
        break;
    case 0:
        currentTasks[pos]->pid = getpid();
        if(maxExecution != -1)
            alarm(maxExecution); 

        int ret = mysystem(strdup(t->commands), t->num);

        int fd2 = open("historico.txt", O_APPEND | O_WRONLY, 0666);
        x = sprintf(out, "#%d, concluida : %s\n", t->num, t->commands);
        write(fd2, out, x);
        close(fd2);

        _exit(ret);
    default:
        currentTasks[pos]->pid = f;
        break;
    }
}

//Função que lista as tarefas em execução
void listTasks(int fd){
    int i, n = 0;
    char aux[MAX],ret[MAX];
    bzero(ret,MAX);
    
    for(i = 0; currentTasks[i] != NULL; i++){
        bzero(aux,MAX);

        n += sprintf(aux,"#%d: %s\n",currentTasks[i]->num,currentTasks[i]->commands);
        strcat(ret,aux);
    }
      if(i == 0){
        n = sprintf(ret,"Não há tarefas a executar\n");
      }

      write(fd,&ret,n);
     
}

//Função que termina uma tarefa
void endTask(int task){
    int i, fd, n = 0;
    char buf[MAX];
    fd = open("historico.txt", O_APPEND | O_WRONLY, 0666);

     for(i = 0; currentTasks[i] != NULL; i++){
        if(task == currentTasks[i]->num){
            kill(currentTasks[i]->pid, SIGKILL);
            n = sprintf(buf, "#%d, terminada por cliente : %s\n", task, currentTasks[i]->commands);
            write(fd,buf,n);
            
        }
    }
    close(fd);
}

//Função que apresenta o histórico de tarefas
void history(int fd){
    char buf[MAX];
    int r = 0;

    int fd2 = open("historico.txt", O_RDONLY);
    lseek(fd2, 0, SEEK_SET);

    while((r = read(fd2,&buf,sizeof(buf)))>0){  
        write(fd,&buf,r);
        bzero(buf,MAX);
    }
    close(fd2);
}

//Função que apresenta o output de uma tarefa
void logs(int fdW, int task){
    int fdidx = open("log.idx", O_RDONLY);
    int fdlog = open("log", O_RDONLY);
    char linha[MAX], *token, *tok;
    char *campos[2];
    int r = 0, inicio = 0, fim = -1, flag = 0;

    while((r = readLine(fdidx, linha, sizeof(linha))) > 0){
        token = strdup(linha);
        for(int i = 0; (tok = strsep(&token, ":")) != NULL; i++){
            campos[i] = strdup(tok+1);
        } 
        if(task == atoi(campos[0])){
            inicio = atoi(campos[1]);
            flag = 1;
        }
        else if(flag){
            fim = atoi(campos[1]);
            break;
        }
    }

    if(fim == -1){
        fim = lseek(fdlog, 0, SEEK_END);
    }

    close(fdidx);

    int bytesOutput = fim - inicio;
    char out[MAX], aux[MAX];
    int j = 0;

    lseek(fdlog, inicio, SEEK_SET);
    bzero(out, MAX);
    while((r = readLine(fdlog, aux, sizeof(aux))) > 0){
        j += r;
        if(j >= bytesOutput)
            break;

        strcat(out, aux);
        bzero(aux, MAX);
    }
    close(fdlog);
    write(fdW, out, bytesOutput);

}

int main(int argc, char *argv[]){
    int r, fd_fifoR, fd_fifoW;
    char buf[MAX];
    char *token, *command;
    char *tokens[20];
    int numTask = 0;
    currentTasks = malloc(sizeof(Task) * 20);

    signal(SIGALRM, alarm_handler);
    signal(SIGCHLD, sigchild_handler);

    //Ler do histórico para buscar o nr para a tarefa seguinte
    int hist_fd = open("historico.txt", O_CREAT | O_RDONLY, 0666);
    int max = -1, x = 0;
    char aux[200];
    while((x = readLine(hist_fd, aux, sizeof(aux))) > 0){
       char *linha = strdup(aux);
       char *tok = strtok(linha, ",");
       if(tok != NULL){
           char *num = tok + 1;
           int n = atoi(num);
           if(n > max)
               max = n;
       }
    }
    if(max != -1)
        numTask = max + 1;
    close(hist_fd);

    //Criação dos fifos
    if (mkfifo("fifo1", 0666) == -1){
        perror("mkfifo");
    }

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
        //Ler do fifo
        if((r = readLine(fd_fifoR, buf, sizeof(buf))) > 0){
            buf[r] = '\0';
            command = strdup(buf);

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
                write(fd_fifoW, "Execução máxima definida\n", 29);            
            }
            else if (!strcmp(tokens[0], "executar")){
                int pos = strlen(tokens[0]) + 2;
                char *task = strdup(buf + pos);
                task[strlen(task)-1] = '\0';
                Task t = newTask(numTask, task, -1);
                numTask++;

                char b[50];
                sprintf(b,"EXECUTAR TAREFA: %s\n", task);
                write(1, b, strlen(b));

                executeTask(fd_fifoW, t);
            }
            else if (!strcmp(tokens[0], "listar")){
                write(1,"LISTAR TAREFAS\n",16);      
                listTasks(fd_fifoW);
            }
            else if (!strcmp(tokens[0], "terminar")){
                int task = atoi(tokens[1]);

                char b[50];
                sprintf(b,"TERMINAR TAREFA %d\n", task);
                write(1, b, strlen(b));
                write(fd_fifoW, "Terminar tarefa\n", 17);
                endTask(task);
            }
            else if (!strcmp(tokens[0], "historico")){
                write(1,"HISTORICO DE TAREFAS\n",22);
                history(fd_fifoW);
            }
            else if (!strcmp(tokens[0], "output")){
                int task = atoi(tokens[1]);

                char b[50];
                sprintf(b,"OUTPUT DA TAREFA %d\n", task);
                write(1, b, strlen(b));

                logs(fd_fifoW, task);
            }
        }
    }

    free(*currentTasks);
    close(fd_fifoR);
    close(fd_fifoW);
    return 0;

}
