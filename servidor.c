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

int maxInactivity = -1;
int maxExecution = -1;
int indexCur = 0;
Task *currentTasks; //Array de tasks a executar


void removeTask(int pid){
    int i = 0;
    int j;

    while(currentTasks[i] != NULL && currentTasks[i]->pid != pid){
        i++;
    }
    if(currentTasks[i] != NULL){ //se encontrou a task
        for(j = i; currentTasks[j] != NULL && currentTasks[j+1] != NULL; j++)
        currentTasks[j] = currentTasks[j+1];
        currentTasks[j] = NULL;
        indexCur--;
    }
}

int numCurrentTasks(){
    int n = 0;
    for(int i = 0; currentTasks[i] != NULL; i++)
        n++;

    return n;
}

//HANDLER DO ALARM - para o max execuçao
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

//HANDLER DO SIGCHLD - para remover as tarefas que ja acabaram
void sigchild_handler(int signum){
    int status, child;
    child = wait(&status);
    
    removeTask(child);
}



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
        currentTasks[pos]->pid = getpid(); // atualiza aqui tbm para no handler do alarm conseguir buscar a task
        printf("NA FILA: tarefa %d a executar %s pelo processo %d\n", currentTasks[pos]->num,
         currentTasks[pos]->commands, currentTasks[pos]->pid); 
        printf("%d tarefas na fila\n", numCurrentTasks());
        //se ao fim deste tempo o processo ainda estiver a executar ao receber o sinal o processo da exit
        if(maxExecution != -1)
            alarm(maxExecution); 

        //Executar
        int ret = mysystem(strdup(t->commands), t->num);
        //Escrever no historico
        int fd2 = open("historico.txt", O_APPEND | O_WRONLY, 0666);
        x = sprintf(out, "#%d, concluida : %s\n", t->num, t->commands);
        write(fd2, out, x);
        close(fd2);

        _exit(ret);
    default:
        currentTasks[pos]->pid = f; //Atualizar a tarefa com o pid do filho, que é quem a esta a executar
        break;
    }
}

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

void history(int fd){
    char buf[MAX];
    int r = 0;

    int fd2 = open("historico.txt", O_RDONLY);
    lseek(fd2, 0, SEEK_SET);//começar a ler a partir do offset q é 0
        
    while((r = read(fd2,&buf,sizeof(buf)))>0){  
        write(fd,&buf,r);
        bzero(buf,MAX);
    }
    close(fd2);
}


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

    int bytesOutput = fim - inicio;
    char out[MAX], aux[MAX];
    int j = 0;

    lseek(fdlog, inicio, SEEK_SET); //começa no inicio do output
    bzero(out, MAX);
    while((r = readLine(fdlog, aux, sizeof(aux))) > 0){
        j += r;
        if(j >= bytesOutput)
            break;

        strcat(out, aux);
        bzero(aux, MAX);
    }
    write(fdW, out, bytesOutput);

}


void readLogs(int fdW, int task){
	int size = 128;
	char c[size], aux[size];
	char log;
	char *ret;
	int r = 0, res, fim = -2, i, j;
	int fd = open("log.idx",O_RDONLY);
	int fd2 = open("log",O_RDONLY);

	while((r = readLine(fd, c, size)) > 0){
		if(c[0] == '#'){
			bzero(aux, size);
			for(i = 1; c[i] != ':' && i < size; i++)//encontrar o numero antes de dois pontos que é o num da tarefa
				aux[i-1] = c[i];

			if(task == atoi(aux) || fim == -1){
				bzero(aux, size);
				for(j = 0, i = i + 1; c[i] != '\n' && i < size && j < size; i++){
					if(c[i] != ' '){
						aux[j] = c[i]; //guardar o numero que está depois de dois pontos que é a posição em q a tarefa "task" começa no ficheiro log
						j++;
					}
				}

				if(fim == -1){// saber quando se acaba de ler a tarefa atual
					fim = atoi(aux);
					printf("Acaba em: %d\n", fim);
					break;
				}
				else{ //saber quando se passa a ler a próxima tarefa
					res = atoi(aux);
					printf("Comeca em: %d\n", res);
					fim = -1;
				}
			}
		}		
	}
    close(fd);
    i = j = 0;
    ret = malloc(sizeof(char)*size);
    bzero(ret, size);
    
	while((r = read(fd2, &log, 1)) > 0){
		if(i >= res){ 
			if(i == fim) //quando se encontra o numero máximo de linhas da tarefa pedida sau do ciclo para escrever o resultado no fifo
				break;

			if(log == '\n') //para ver quantas linhas a partir da posição encontrada
				i++;

			if(j == size){ //caso se exceda o tamanho 128, para realocar
				char *auxlogs = strdup(ret);
				size = size*2;
				ret = malloc(sizeof(char)*size);
				bzero(ret, size);
				for(int k = 0; k < strlen(auxlogs); k++)
					ret[k] = auxlogs[k];
			}

			ret[j++] = log; //guardar o output da tarefa em ret

		}

		if(log == '\n') //para encontrar no ficheiro log a posição a partir do qual começa uma dada task
			i++;
	}

    close(fd2);

	ret[j++] = '\n';
	write(fdW, ret, j); //escrever no fifo   
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

    //Buscar ao historico quantas tarefas ja foram efetuadas
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

            if(!strcmp(tokens[0], "tempo-inactividade")){
                maxInactivity = atoi(tokens[1]);
                write(fd_fifoW, "Inatividade definida\n", 21);
            }
            else if (!strcmp(tokens[0], "tempo-execucao")){
                maxExecution = atoi(tokens[1]);
                write(fd_fifoW, "Execucao Max definida\n", 22);            
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
                endTask(task);
            }
            else if (!strcmp(tokens[0], "historico")){
                printf("HISTORICO DE TAREFAS\n");
                history(fd_fifoW);
            }
            else if (!strcmp(tokens[0], "output")){
                printf("OUTPUT\n");
                logs(fd_fifoW, atoi(tokens[1]));
            }
        }
    }

    free(*currentTasks);
    close(fd_fifoR);
    close(fd_fifoW);
    return 0;

}
